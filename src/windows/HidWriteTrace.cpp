#include <StarfieldDualSense/HidWriteTrace.h>
#include <StarfieldDualSense/HidOutputOwnership.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <winternl.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <mutex>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace
{
    constexpr ULONG kSystemExtendedHandleInformation = 64;
    constexpr ULONG kObjectNameInformation = 1;
    constexpr LONG kStatusInfoLengthMismatch = static_cast<LONG>(0xC0000004L);
    constexpr std::size_t kMaxTargetHandles = 16;
    constexpr DWORD kObjectNameTimeoutMs = 20;
    constexpr ULONGLONG kArbitrationLogIntervalMs = 5000;

    using NtQuerySystemInformationFn = LONG(NTAPI*)(ULONG, PVOID, ULONG, PULONG);
    using NtQueryObjectFn = LONG(NTAPI*)(HANDLE, ULONG, PVOID, ULONG, PULONG);
    struct NativeUnicodeString
    {
        USHORT length;
        USHORT maximumLength;
        PWSTR buffer;
    };

    struct SystemHandleEntryEx
    {
        PVOID object;
        ULONG_PTR processId;
        ULONG_PTR handleValue;
        ULONG grantedAccess;
        USHORT creatorBackTraceIndex;
        USHORT objectTypeIndex;
        ULONG handleAttributes;
        ULONG reserved;
    };

    struct SystemHandleInformationEx
    {
        ULONG_PTR numberOfHandles;
        ULONG_PTR reserved;
        SystemHandleEntryEx handles[1];
    };

    struct NameQueryContext
    {
        HANDLE handle{ INVALID_HANDLE_VALUE };
        NtQueryObjectFn query{};
        std::wstring objectName;
        LONG status{ -1 };
    };

    struct Patch
    {
        HMODULE module{};
        void** slot{};
        void* original{};
        void* replacement{};
        std::string function;
    };

    struct TraceState
    {
        std::mutex mutex;
        sds::HidWriteTrace::LogCallback log;
        std::vector<Patch> patches;
        std::unordered_set<void**> patchedSlots;
        std::array<std::atomic<std::uintptr_t>, kMaxTargetHandles> targetHandles{};
        std::atomic<std::uintptr_t> ownHandle{ 0 };
        std::atomic<ULONGLONG> lastArbitrationLogMs{ 0 };
        std::atomic<std::uint64_t> arbitrationCount{ 0 };
        std::atomic<bool> active{ false };
        std::wstring targetObjectName;
    };

    TraceState& traceState()
    {
        static TraceState state;
        return state;
    }

    thread_local bool g_inHook = false;

    struct HookScope
    {
        HookScope() noexcept : outer(!g_inHook)
        {
            if (outer) {
                g_inHook = true;
            }
        }
        ~HookScope()
        {
            if (outer) {
                g_inHook = false;
            }
        }
        bool outer{ false };
    };

    void emit(std::string_view message) noexcept
    {
        auto& state = traceState();
        sds::HidWriteTrace::LogCallback callback;
        {
            std::scoped_lock lock(state.mutex);
            callback = state.log;
        }
        if (!callback) {
            return;
        }
        try {
            callback(message);
        } catch (...) {
            // Diagnostic logging must never affect game/controller behavior.
        }
    }

    std::string hexValue(std::uintptr_t value)
    {
        std::ostringstream out;
        out << "0x" << std::hex << std::uppercase << value;
        return out.str();
    }

    std::string hexBytes(const void* buffer, std::size_t length, std::size_t maximum = 64)
    {
        if (!buffer || length == 0) {
            return "<none>";
        }
        const auto* bytes = static_cast<const std::uint8_t*>(buffer);
        const auto count = std::min(length, maximum);
        std::ostringstream out;
        out << std::hex << std::uppercase << std::setfill('0');
        for (std::size_t i = 0; i < count; ++i) {
            if (i != 0) {
                out << ' ';
            }
            out << std::setw(2) << static_cast<unsigned>(bytes[i]);
        }
        if (length > count) {
            out << " ...";
        }
        return out.str();
    }

    void* returnAddress() noexcept
    {
#ifdef _MSC_VER
        return _ReturnAddress();
#else
        return __builtin_return_address(0);
#endif
    }

    std::uintptr_t starfieldCallerRva(void* address) noexcept
    {
        HMODULE mainModule = GetModuleHandleW(nullptr);
        if (!mainModule || !address) {
            return 0;
        }
        const auto base = reinterpret_cast<std::uintptr_t>(mainModule);
        const auto value = reinterpret_cast<std::uintptr_t>(address);
        if (value < base) {
            return 0;
        }
        return value - base;
    }

    std::string callerDescription(void* address)
    {
        if (!address) {
            return "<unknown>";
        }

        HMODULE module = nullptr;
        if (!GetModuleHandleExW(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(address),
                &module) || !module) {
            return hexValue(reinterpret_cast<std::uintptr_t>(address));
        }

        std::array<wchar_t, 1024> path{};
        const DWORD length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
        std::wstring_view view(path.data(), length);
        const auto slash = view.find_last_of(L"\\/");
        if (slash != std::wstring_view::npos) {
            view.remove_prefix(slash + 1);
        }

        std::string moduleName;
        if (!view.empty()) {
            const int count = WideCharToMultiByte(
                CP_UTF8, 0, view.data(), static_cast<int>(view.size()), nullptr, 0, nullptr, nullptr);
            if (count > 0) {
                moduleName.resize(static_cast<std::size_t>(count));
                WideCharToMultiByte(
                    CP_UTF8, 0, view.data(), static_cast<int>(view.size()),
                    moduleName.data(), count, nullptr, nullptr);
            }
        }
        if (moduleName.empty()) {
            moduleName = "<module>";
        }

        const auto rva = reinterpret_cast<std::uintptr_t>(address) - reinterpret_cast<std::uintptr_t>(module);
        std::ostringstream out;
        out << moduleName << "+0x" << std::hex << std::uppercase << rva;
        return out.str();
    }

    bool isTargetHandle(HANDLE handle) noexcept
    {
        const auto value = reinterpret_cast<std::uintptr_t>(handle);
        if (!value) {
            return false;
        }
        const auto& targets = traceState().targetHandles;
        for (const auto& target : targets) {
            const auto candidate = target.load(std::memory_order_relaxed);
            if (candidate == 0) {
                continue;
            }
            if (candidate == value) {
                return true;
            }
        }
        return false;
    }

    bool isOwnHandle(HANDLE handle) noexcept
    {
        const auto value = reinterpret_cast<std::uintptr_t>(handle);
        return value != 0 &&
            value == traceState().ownHandle.load(std::memory_order_relaxed);
    }

    std::string handleOwnerTag(HANDLE handle)
    {
        return isOwnHandle(handle) ? "OUR" : "OTHER";
    }

    bool shouldLogArbitration() noexcept
    {
        auto& state = traceState();
        const ULONGLONG now = GetTickCount64();
        ULONGLONG previous = state.lastArbitrationLogMs.load(std::memory_order_relaxed);
        while (previous == 0 || now - previous >= kArbitrationLogIntervalMs) {
            if (state.lastArbitrationLogMs.compare_exchange_weak(
                    previous, now, std::memory_order_relaxed, std::memory_order_relaxed)) {
                return true;
            }
        }
        return false;
    }

    void logArbitration(
        HANDLE handle,
        void* caller,
        const sds::HidOutputOwnershipResult& result,
        std::uint64_t count) noexcept
    {
        if (!shouldLogArbitration()) {
            return;
        }
        std::ostringstream out;
        out << "HID arbitration: stripped native trigger/lightbar ownership in-place"
            << " handle=" << hexValue(reinterpret_cast<std::uintptr_t>(handle))
            << " caller=" << callerDescription(caller)
            << " beforeFlags=" << std::hex << std::uppercase
            << std::setw(2) << std::setfill('0') << static_cast<unsigned>(result.beforeFlags0)
            << '/' << std::setw(2) << static_cast<unsigned>(result.beforeFlags1)
            << " afterFlags="
            << std::setw(2) << static_cast<unsigned>(result.afterFlags0)
            << '/' << std::setw(2) << static_cast<unsigned>(result.afterFlags1)
            << std::dec << " filteredWrites=" << count;
        emit(out.str());
    }

    void traceBufferCall(
        std::string_view api,
        HANDLE handle,
        const void* buffer,
        std::size_t length,
        void* caller,
        std::string_view extra = {}) noexcept
    {
        auto& state = traceState();
        if (!state.active.load(std::memory_order_acquire) || !isTargetHandle(handle)) {
            return;
        }

        std::ostringstream out;
        out << "HID competing TX " << handleOwnerTag(handle)
            << " api=" << api
            << " handle=" << hexValue(reinterpret_cast<std::uintptr_t>(handle))
            << " len=" << length
            << " caller=" << callerDescription(caller);
        if (!extra.empty()) {
            out << ' ' << extra;
        }
        out << " raw=" << hexBytes(buffer, length);
        emit(out.str());
    }

    BOOL WINAPI hookWriteFile(
        HANDLE file,
        LPCVOID buffer,
        DWORD bytesToWrite,
        LPDWORD bytesWritten,
        LPOVERLAPPED overlapped)
    {
        HookScope scope;
        if (!scope.outer) {
            return ::WriteFile(file, buffer, bytesToWrite, bytesWritten, overlapped);
        }

        auto& state = traceState();
        void* caller = returnAddress();
        if (state.active.load(std::memory_order_acquire) && buffer && bytesToWrite == 48) {
            auto* mutableBuffer = const_cast<std::uint8_t*>(
                static_cast<const std::uint8_t*>(buffer));
            const auto ownership = sds::filterCompetingNativeDualSenseWriteInPlace(
                std::span<std::uint8_t>(mutableBuffer, bytesToWrite),
                isTargetHandle(file),
                isOwnHandle(file),
                starfieldCallerRva(caller));
            if (ownership.changed) {
                const auto count = state.arbitrationCount.fetch_add(1, std::memory_order_relaxed) + 1;
                logArbitration(file, caller, ownership, count);
            }
        }

        return ::WriteFile(file, buffer, bytesToWrite, bytesWritten, overlapped);
    }

    BOOL WINAPI hookDeviceIoControl(
        HANDLE device,
        DWORD ioControlCode,
        LPVOID inBuffer,
        DWORD inBufferSize,
        LPVOID outBuffer,
        DWORD outBufferSize,
        LPDWORD bytesReturned,
        LPOVERLAPPED overlapped)
    {
        HookScope scope;
        if (!scope.outer) {
            return ::DeviceIoControl(
                device, ioControlCode, inBuffer, inBufferSize,
                outBuffer, outBufferSize, bytesReturned, overlapped);
        }
        std::ostringstream extra;
        extra << "ioctl=0x" << std::hex << std::uppercase << ioControlCode
              << " outLen=" << std::dec << outBufferSize;
        traceBufferCall(
            "DeviceIoControl", device, inBuffer, inBufferSize, returnAddress(), extra.str());
        return ::DeviceIoControl(
            device, ioControlCode, inBuffer, inBufferSize,
            outBuffer, outBufferSize, bytesReturned, overlapped);
    }

    void* replacementFor(std::string_view functionName) noexcept
    {
        if (functionName == "WriteFile") {
            return reinterpret_cast<void*>(&hookWriteFile);
        }
        if (functionName == "DeviceIoControl") {
            return reinterpret_cast<void*>(&hookDeviceIoControl);
        }
        return nullptr;
    }

    std::wstring modulePath(HMODULE module)
    {
        std::array<wchar_t, 2048> buffer{};
        const DWORD count = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
        return std::wstring(buffer.data(), count);
    }

    std::string narrowBasename(std::wstring_view path)
    {
        const auto slash = path.find_last_of(L"\\/");
        if (slash != std::wstring_view::npos) {
            path.remove_prefix(slash + 1);
        }
        if (path.empty()) {
            return "<module>";
        }
        const int count = WideCharToMultiByte(
            CP_UTF8, 0, path.data(), static_cast<int>(path.size()), nullptr, 0, nullptr, nullptr);
        if (count <= 0) {
            return "<module>";
        }
        std::string out(static_cast<std::size_t>(count), '\0');
        WideCharToMultiByte(
            CP_UTF8, 0, path.data(), static_cast<int>(path.size()), out.data(), count, nullptr, nullptr);
        return out;
    }

    bool rangeWithinImage(std::uintptr_t rva, std::size_t length, std::size_t imageSize) noexcept
    {
        if (rva >= imageSize) {
            return false;
        }
        return length <= imageSize - static_cast<std::size_t>(rva);
    }

    std::optional<std::string_view> importNameWithinImage(
        std::uintptr_t base,
        std::size_t imageSize,
        std::uintptr_t importByNameRva) noexcept
    {
        constexpr std::size_t kNameOffset = offsetof(IMAGE_IMPORT_BY_NAME, Name);
        if (!rangeWithinImage(importByNameRva, kNameOffset + 1, imageSize)) {
            return std::nullopt;
        }
        const auto nameRva = importByNameRva + kNameOffset;
        const char* name = reinterpret_cast<const char*>(base + nameRva);
        const auto remaining = imageSize - static_cast<std::size_t>(nameRva);
        const void* terminator = std::memchr(name, '\0', remaining);
        if (!terminator) {
            return std::nullopt;
        }
        const auto length = static_cast<const char*>(terminator) - name;
        return std::string_view(name, static_cast<std::size_t>(length));
    }

    bool patchModuleImports(HMODULE module, std::size_t& patchedCount)
    {
        auto& state = traceState();
        if (!module) {
            emit("HID trace: narrow hook skipped; Starfield.exe module handle is null");
            return false;
        }

        const auto base = reinterpret_cast<std::uintptr_t>(module);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 0x100000) {
            emit("HID trace: narrow hook validation failed at DOS header");
            return false;
        }

        const auto ntRva = static_cast<std::uintptr_t>(dos->e_lfanew);
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + ntRva);
        if (nt->Signature != IMAGE_NT_SIGNATURE ||
            nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            emit("HID trace: narrow hook validation failed at NT headers");
            return false;
        }

        const std::size_t imageSize = nt->OptionalHeader.SizeOfImage;
        if (imageSize < sizeof(IMAGE_DOS_HEADER) ||
            !rangeWithinImage(ntRva, sizeof(IMAGE_NT_HEADERS64), imageSize)) {
            emit("HID trace: narrow hook validation failed at image size/NT range");
            return false;
        }

        const auto& importDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (!importDir.VirtualAddress || !importDir.Size) {
            emit("HID trace: Starfield.exe has no import directory; narrow hooks not installed");
            return true;
        }
        if (!rangeWithinImage(importDir.VirtualAddress, importDir.Size, imageSize)) {
            emit("HID trace: narrow hook validation failed at import-directory range");
            return false;
        }

        const auto moduleName = narrowBasename(modulePath(module));
        const auto descriptorCount =
            static_cast<std::size_t>(importDir.Size) / sizeof(IMAGE_IMPORT_DESCRIPTOR);
        auto* descriptors = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(base + importDir.VirtualAddress);

        for (std::size_t descriptorIndex = 0; descriptorIndex < descriptorCount; ++descriptorIndex) {
            const auto& descriptor = descriptors[descriptorIndex];
            if (descriptor.Name == 0 && descriptor.FirstThunk == 0 && descriptor.OriginalFirstThunk == 0) {
                break;
            }
            if (!descriptor.FirstThunk || !descriptor.OriginalFirstThunk) {
                continue;
            }
            if (!rangeWithinImage(descriptor.FirstThunk, sizeof(IMAGE_THUNK_DATA64), imageSize) ||
                !rangeWithinImage(descriptor.OriginalFirstThunk, sizeof(IMAGE_THUNK_DATA64), imageSize)) {
                emit("HID trace: import descriptor skipped because thunk RVA is outside Starfield.exe image");
                continue;
            }

            auto* firstThunk = reinterpret_cast<IMAGE_THUNK_DATA64*>(base + descriptor.FirstThunk);
            const auto* originalThunk = reinterpret_cast<const IMAGE_THUNK_DATA64*>(
                base + descriptor.OriginalFirstThunk);
            const auto firstThunkCapacity =
                (imageSize - static_cast<std::size_t>(descriptor.FirstThunk)) / sizeof(IMAGE_THUNK_DATA64);
            const auto originalThunkCapacity =
                (imageSize - static_cast<std::size_t>(descriptor.OriginalFirstThunk)) / sizeof(IMAGE_THUNK_DATA64);
            const auto thunkCount = std::min(firstThunkCapacity, originalThunkCapacity);

            for (std::size_t index = 0; index < thunkCount; ++index) {
                const auto lookup = originalThunk[index].u1;
                if (lookup.AddressOfData == 0) {
                    break;
                }
                if (IMAGE_SNAP_BY_ORDINAL64(lookup.Ordinal)) {
                    continue;
                }

                const auto name = importNameWithinImage(base, imageSize, lookup.AddressOfData);
                if (!name) {
                    continue;
                }
                void* replacement = replacementFor(*name);
                if (!replacement) {
                    continue;
                }

                auto** slot = reinterpret_cast<void**>(&firstThunk[index].u1.Function);
                {
                    std::scoped_lock lock(state.mutex);
                    if (state.patchedSlots.contains(slot)) {
                        continue;
                    }
                }

                void* original = *slot;
                {
                    std::ostringstream before;
                    before << "HID trace: hook candidate module=" << moduleName
                           << " api=" << *name
                           << " slot=" << hexValue(reinterpret_cast<std::uintptr_t>(slot))
                           << " original=" << hexValue(reinterpret_cast<std::uintptr_t>(original));
                    emit(before.str());
                }

                DWORD oldProtect = 0;
                if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &oldProtect)) {
                    std::ostringstream skipped;
                    skipped << "HID trace: hook skipped module=" << moduleName
                            << " api=" << *name << " reason=VirtualProtect";
                    emit(skipped.str());
                    continue;
                }

                original = InterlockedExchangePointer(
                    reinterpret_cast<PVOID volatile*>(slot), replacement);
                DWORD ignored = 0;
                const BOOL restoredProtection =
                    VirtualProtect(slot, sizeof(void*), oldProtect, &ignored);

                {
                    std::scoped_lock lock(state.mutex);
                    state.patches.push_back(Patch{ module, slot, original, replacement, std::string(*name) });
                    state.patchedSlots.insert(slot);
                }
                ++patchedCount;

                std::ostringstream after;
                after << "HID trace: hook installed module=" << moduleName
                      << " api=" << *name
                      << " slot=" << hexValue(reinterpret_cast<std::uintptr_t>(slot))
                      << " original=" << hexValue(reinterpret_cast<std::uintptr_t>(original))
                      << " replacement=" << hexValue(reinterpret_cast<std::uintptr_t>(replacement))
                      << " protectRestored=" << (restoredProtection ? "yes" : "no");
                emit(after.str());
            }
        }
        return true;
    }

    void installIatHooks()
    {
        HMODULE mainModule = GetModuleHandleW(nullptr);
        if (!mainModule) {
            emit("HID trace: GetModuleHandleW(nullptr) failed; narrow hooks not installed");
            return;
        }

        const auto moduleName = narrowBasename(modulePath(mainModule));
        std::ostringstream begin;
        begin << "HID trace: narrow IAT install begin module=" << moduleName
              << " base=" << hexValue(reinterpret_cast<std::uintptr_t>(mainModule))
              << " APIs=WriteFile,DeviceIoControl";
        emit(begin.str());

        std::size_t slots = 0;
        const bool parsed = patchModuleImports(mainModule, slots);

        std::ostringstream out;
        out << "HID trace: narrow IAT instrumentation installed module=" << moduleName
            << " parsed=" << (parsed ? "yes" : "no")
            << " patchedSlots=" << slots
            << " APIs=WriteFile,DeviceIoControl";
        emit(out.str());
    }

    void restorePatches() noexcept
    {
        auto& state = traceState();
        std::vector<Patch> patches;
        {
            std::scoped_lock lock(state.mutex);
            patches.swap(state.patches);
            state.patchedSlots.clear();
        }

        for (auto it = patches.rbegin(); it != patches.rend(); ++it) {
            if (!it->slot) {
                continue;
            }
            HMODULE containing = nullptr;
            if (!GetModuleHandleExW(
                    GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<LPCWSTR>(it->slot),
                    &containing) || containing != it->module) {
                continue;
            }
            DWORD oldProtect = 0;
            if (!VirtualProtect(it->slot, sizeof(void*), PAGE_READWRITE, &oldProtect)) {
                continue;
            }
            if (*it->slot == it->replacement) {
                InterlockedExchangePointer(
                    reinterpret_cast<PVOID volatile*>(it->slot), it->original);
            }
            DWORD ignored = 0;
            VirtualProtect(it->slot, sizeof(void*), oldProtect, &ignored);
        }
    }

    std::optional<std::vector<std::byte>> querySystemHandles()
    {
        const auto query = reinterpret_cast<NtQuerySystemInformationFn>(
            GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQuerySystemInformation"));
        if (!query) {
            return std::nullopt;
        }

        ULONG size = 1u << 20;
        for (int attempt = 0; attempt < 10; ++attempt) {
            std::vector<std::byte> buffer(size);
            ULONG needed = 0;
            const LONG status = query(kSystemExtendedHandleInformation, buffer.data(), size, &needed);
            if (status >= 0) {
                return buffer;
            }
            if (status != kStatusInfoLengthMismatch) {
                return std::nullopt;
            }
            size = std::max(size * 2u, needed + (1u << 16));
        }
        return std::nullopt;
    }

    NtQueryObjectFn resolveNtQueryObject() noexcept
    {
        return reinterpret_cast<NtQueryObjectFn>(
            GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryObject"));
    }

    std::wstring queryObjectNameDirect(HANDLE handle, NtQueryObjectFn query, LONG& status)
    {
        status = -1;
        if (!query || !handle || handle == INVALID_HANDLE_VALUE) {
            return {};
        }
        std::vector<std::byte> storage(64u * 1024u);
        ULONG returned = 0;
        status = query(
            handle,
            kObjectNameInformation,
            storage.data(),
            static_cast<ULONG>(storage.size()),
            &returned);
        if (status < 0) {
            return {};
        }
        const auto* native = reinterpret_cast<const NativeUnicodeString*>(storage.data());
        if (!native->buffer || native->length == 0) {
            return {};
        }
        return std::wstring(native->buffer, native->length / sizeof(wchar_t));
    }

    DWORD WINAPI objectNameThread(LPVOID parameter)
    {
        auto* context = static_cast<NameQueryContext*>(parameter);
        context->objectName = queryObjectNameDirect(context->handle, context->query, context->status);
        return 0;
    }

    std::optional<std::wstring> queryObjectNameWithTimeout(HANDLE handle, NtQueryObjectFn query)
    {
        auto context = std::make_unique<NameQueryContext>();
        context->handle = handle;
        context->query = query;
        HANDLE thread = CreateThread(nullptr, 0, objectNameThread, context.get(), 0, nullptr);
        if (!thread) {
            return std::nullopt;
        }
        const DWORD wait = WaitForSingleObject(thread, kObjectNameTimeoutMs);
        if (wait != WAIT_OBJECT_0) {
            // Do not terminate a thread blocked inside an object-manager query. Diagnostic-only:
            // leak the tiny context if Windows does not return promptly rather than stalling Starfield.
            CloseHandle(thread);
            (void)context.release();
            return std::nullopt;
        }
        CloseHandle(thread);
        if (context->status < 0 || context->objectName.empty()) {
            return std::nullopt;
        }
        return std::move(context->objectName);
    }

    bool potentialWriteHandle(ULONG access) noexcept
    {
        return (access & (FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_WRITE_ATTRIBUTES)) != 0;
    }

    bool refreshTargetHandles(HANDLE ownHandle)
    {
        auto& state = traceState();
        for (auto& target : state.targetHandles) {
            target.store(0, std::memory_order_relaxed);
        }
        state.ownHandle.store(reinterpret_cast<std::uintptr_t>(ownHandle), std::memory_order_relaxed);

        const auto buffer = querySystemHandles();
        const auto queryObject = resolveNtQueryObject();
        if (!buffer || !queryObject || buffer->size() < sizeof(SystemHandleInformationEx)) {
            return false;
        }

        const auto* info = reinterpret_cast<const SystemHandleInformationEx*>(buffer->data());
        const DWORD pid = GetCurrentProcessId();
        const auto ownValue = reinterpret_cast<ULONG_PTR>(ownHandle);
        USHORT targetTypeIndex = 0;
        ULONG targetAccess = 0;
        for (ULONG_PTR i = 0; i < info->numberOfHandles; ++i) {
            const auto& entry = info->handles[i];
            if (entry.processId == pid && entry.handleValue == ownValue) {
                targetTypeIndex = entry.objectTypeIndex;
                targetAccess = entry.grantedAccess;
                break;
            }
        }
        if (!targetTypeIndex) {
            return false;
        }

        const auto targetObjectName = queryObjectNameWithTimeout(ownHandle, queryObject);
        if (!targetObjectName || targetObjectName->empty()) {
            return false;
        }
        {
            std::scoped_lock lock(state.mutex);
            state.targetObjectName = *targetObjectName;
        }

        std::size_t found = 0;
        for (ULONG_PTR i = 0; i < info->numberOfHandles && found < kMaxTargetHandles; ++i) {
            const auto& entry = info->handles[i];
            if (entry.processId != pid || entry.objectTypeIndex != targetTypeIndex) {
                continue;
            }
            // The live owner probe on this machine showed all three Starfield DualSense
            // handles use the exact same access mask as our handle (0x12019F). Keeping the
            // diagnostic scan to that mask avoids interrogating thousands of unrelated file
            // handles inside the game process.
            if (entry.handleValue != ownValue && entry.grantedAccess != targetAccess) {
                continue;
            }

            HANDLE candidate = reinterpret_cast<HANDLE>(entry.handleValue);
            const auto objectName = queryObjectNameWithTimeout(candidate, queryObject);
            if (!objectName || _wcsicmp(objectName->c_str(), targetObjectName->c_str()) != 0) {
                continue;
            }
            state.targetHandles[found++].store(
                static_cast<std::uintptr_t>(entry.handleValue), std::memory_order_release);
        }

        std::ostringstream out;
        out << "HID trace: targetObjectName=";
        const int count = WideCharToMultiByte(
            CP_UTF8, 0, targetObjectName->data(), static_cast<int>(targetObjectName->size()),
            nullptr, 0, nullptr, nullptr);
        if (count > 0) {
            std::string utf8(static_cast<std::size_t>(count), '\0');
            WideCharToMultiByte(
                CP_UTF8, 0, targetObjectName->data(), static_cast<int>(targetObjectName->size()),
                utf8.data(), count, nullptr, nullptr);
            out << utf8;
        } else {
            out << "<conversion-failed>";
        }
        out << " ownHandle=" << hexValue(reinterpret_cast<std::uintptr_t>(ownHandle))
            << " matchingStarfieldHandles=" << found << " handles=";
        for (std::size_t i = 0; i < found; ++i) {
            if (i != 0) {
                out << ',';
            }
            out << hexValue(state.targetHandles[i].load(std::memory_order_relaxed));
        }
        emit(out.str());
        return found > 0;
    }

}

void sds::HidWriteTrace::start(void* nativeHandle, LogCallback log) noexcept
{
    try {
        stop();
        HANDLE ownHandle = static_cast<HANDLE>(nativeHandle);
        if (!ownHandle || ownHandle == INVALID_HANDLE_VALUE) {
            return;
        }

        auto& state = traceState();
        {
            std::scoped_lock lock(state.mutex);
            state.log = std::move(log);
        }

        if (!refreshTargetHandles(ownHandle)) {
            emit("HID trace: failed to enumerate matching in-process DualSense handles; hooks not installed");
            return;
        }

        state.lastArbitrationLogMs.store(0, std::memory_order_relaxed);
        state.arbitrationCount.store(0, std::memory_order_relaxed);
        state.active.store(true, std::memory_order_release);
        installIatHooks();
        emit("HID arbitration: ACTIVE; exact Starfield native DualSense writer is filtered in-place before synchronous or OVERLAPPED WriteFile submission");
    } catch (...) {
        emit("HID trace: startup threw; competing-write capture disabled");
        stop();
    }
}

void sds::HidWriteTrace::stop() noexcept
{
    auto& state = traceState();
    state.active.store(false, std::memory_order_release);
    restorePatches();
    for (auto& target : state.targetHandles) {
        target.store(0, std::memory_order_relaxed);
    }
    state.ownHandle.store(0, std::memory_order_relaxed);
    state.lastArbitrationLogMs.store(0, std::memory_order_relaxed);
    state.arbitrationCount.store(0, std::memory_order_relaxed);
    {
        std::scoped_lock lock(state.mutex);
        state.targetObjectName.clear();
        state.log = {};
    }
}
