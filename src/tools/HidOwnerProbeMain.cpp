#include <Windows.h>
#include <TlHelp32.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
    constexpr USHORT kSonyVendorId = 0x054C;
    constexpr USHORT kDualSensePid = 0x0CE6;
    constexpr USHORT kDualSenseEdgePid = 0x0DF2;
    constexpr ULONG kSystemExtendedHandleInformation = 64;
    constexpr ULONG kObjectNameInformation = 1;
    constexpr LONG kStatusInfoLengthMismatch = static_cast<LONG>(0xC0000004L);
    constexpr DWORD kScanBudgetMs = 12000;
    constexpr DWORD kPerHandleNameQueryTimeoutMs = 25;
    constexpr std::size_t kMaxAbandonedNameQueries = 8;
    constexpr std::size_t kProgressInterval = 250;

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

    struct DeviceHandle
    {
        HANDLE handle{ INVALID_HANDLE_VALUE };
        HIDP_CAPS caps{};
        HIDD_ATTRIBUTES attributes{};
        std::wstring path;
        std::wstring serial;

        DeviceHandle() = default;
        DeviceHandle(const DeviceHandle&) = delete;
        DeviceHandle& operator=(const DeviceHandle&) = delete;

        DeviceHandle(DeviceHandle&& other) noexcept :
            handle(other.handle),
            caps(other.caps),
            attributes(other.attributes),
            path(std::move(other.path)),
            serial(std::move(other.serial))
        {
            other.handle = INVALID_HANDLE_VALUE;
        }

        DeviceHandle& operator=(DeviceHandle&& other) noexcept
        {
            if (this != &other) {
                close();
                handle = other.handle;
                caps = other.caps;
                attributes = other.attributes;
                path = std::move(other.path);
                serial = std::move(other.serial);
                other.handle = INVALID_HANDLE_VALUE;
            }
            return *this;
        }

        ~DeviceHandle()
        {
            close();
        }

        void close() noexcept
        {
            if (handle != INVALID_HANDLE_VALUE) {
                CloseHandle(handle);
                handle = INVALID_HANDLE_VALUE;
            }
        }

        [[nodiscard]] bool valid() const noexcept
        {
            return handle != INVALID_HANDLE_VALUE;
        }
    };

    struct OwnerRecord
    {
        DWORD processId{};
        std::wstring processName;
        ULONG_PTR handleValue{};
        ULONG grantedAccess{};
        std::wstring objectName;
    };

    struct NameQueryContext
    {
        HANDLE duplicated{ INVALID_HANDLE_VALUE };
        NtQueryObjectFn ntQueryObject{};
        std::wstring objectName;
        LONG status{};
    };

    std::string utf8(std::wstring_view text)
    {
        if (text.empty()) {
            return {};
        }
        const int count = WideCharToMultiByte(
            CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        if (count <= 0) {
            return "<utf8-conversion-failed>";
        }
        std::string out(static_cast<std::size_t>(count), '\0');
        WideCharToMultiByte(
            CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), count, nullptr, nullptr);
        return out;
    }

    std::wstring hidSerial(HANDLE handle)
    {
        std::array<wchar_t, 256> buffer{};
        if (!HidD_GetSerialNumberString(handle, buffer.data(), static_cast<ULONG>(buffer.size() * sizeof(wchar_t)))) {
            return {};
        }
        return buffer.data();
    }

    std::optional<HIDP_CAPS> hidCaps(HANDLE handle)
    {
        PHIDP_PREPARSED_DATA preparsed = nullptr;
        if (!HidD_GetPreparsedData(handle, &preparsed)) {
            return std::nullopt;
        }
        HIDP_CAPS caps{};
        const bool ok = HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS;
        HidD_FreePreparsedData(preparsed);
        if (!ok) {
            return std::nullopt;
        }
        return caps;
    }

    std::optional<DeviceHandle> openDualSense()
    {
        GUID hidGuid{};
        HidD_GetHidGuid(&hidGuid);

        HDEVINFO deviceInfoSet = SetupDiGetClassDevsW(
            &hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
        if (deviceInfoSet == INVALID_HANDLE_VALUE) {
            return std::nullopt;
        }

        std::optional<DeviceHandle> result;
        for (DWORD index = 0; !result; ++index) {
            SP_DEVICE_INTERFACE_DATA interfaceData{};
            interfaceData.cbSize = sizeof(interfaceData);
            if (!SetupDiEnumDeviceInterfaces(deviceInfoSet, nullptr, &hidGuid, index, &interfaceData)) {
                if (GetLastError() == ERROR_NO_MORE_ITEMS) {
                    break;
                }
                continue;
            }

            DWORD requiredSize = 0;
            SetupDiGetDeviceInterfaceDetailW(
                deviceInfoSet, &interfaceData, nullptr, 0, &requiredSize, nullptr);
            if (requiredSize < sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)) {
                continue;
            }

            std::vector<std::byte> storage(requiredSize);
            auto* detail = reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(storage.data());
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
            if (!SetupDiGetDeviceInterfaceDetailW(
                    deviceInfoSet, &interfaceData, detail, requiredSize, nullptr, nullptr)) {
                continue;
            }

            HANDLE candidate = CreateFileW(
                detail->DevicePath,
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                0,
                nullptr);
            if (candidate == INVALID_HANDLE_VALUE) {
                continue;
            }

            HIDD_ATTRIBUTES attributes{};
            attributes.Size = sizeof(attributes);
            const bool dualSense = HidD_GetAttributes(candidate, &attributes) &&
                attributes.VendorID == kSonyVendorId &&
                (attributes.ProductID == kDualSensePid || attributes.ProductID == kDualSenseEdgePid);
            const auto caps = dualSense ? hidCaps(candidate) : std::nullopt;
            if (!dualSense || !caps || caps->InputReportByteLength != 64 || caps->OutputReportByteLength != 48) {
                CloseHandle(candidate);
                continue;
            }

            DeviceHandle device;
            device.handle = candidate;
            device.caps = *caps;
            device.attributes = attributes;
            device.path = detail->DevicePath;
            device.serial = hidSerial(candidate);
            result.emplace(std::move(device));
        }

        SetupDiDestroyDeviceInfoList(deviceInfoSet);
        return result;
    }

    std::unordered_map<DWORD, std::wstring> processNames()
    {
        std::unordered_map<DWORD, std::wstring> names;
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return names;
        }

        PROCESSENTRY32W entry{};
        entry.dwSize = sizeof(entry);
        if (Process32FirstW(snapshot, &entry)) {
            do {
                names.emplace(entry.th32ProcessID, entry.szExeFile);
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
        return names;
    }

    bool processRunning(const std::unordered_map<DWORD, std::wstring>& names, std::wstring_view executable)
    {
        for (const auto& [pid, name] : names) {
            (void)pid;
            if (_wcsicmp(name.c_str(), executable.data()) == 0) {
                return true;
            }
        }
        return false;
    }

    bool enableSeDebugPrivilege()
    {
        HANDLE token = nullptr;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
            return false;
        }

        LUID luid{};
        if (!LookupPrivilegeValueW(nullptr, L"SeDebugPrivilege", &luid)) {
            CloseHandle(token);
            return false;
        }

        TOKEN_PRIVILEGES privileges{};
        privileges.PrivilegeCount = 1;
        privileges.Privileges[0].Luid = luid;
        privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        SetLastError(ERROR_SUCCESS);
        const BOOL adjusted = AdjustTokenPrivileges(token, FALSE, &privileges, 0, nullptr, nullptr);
        const DWORD error = GetLastError();
        CloseHandle(token);
        return adjusted && error == ERROR_SUCCESS;
    }

    std::optional<std::vector<std::byte>> querySystemHandles()
    {
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        if (!ntdll) {
            return std::nullopt;
        }
        const auto ntQuerySystemInformation = reinterpret_cast<NtQuerySystemInformationFn>(
            GetProcAddress(ntdll, "NtQuerySystemInformation"));
        if (!ntQuerySystemInformation) {
            return std::nullopt;
        }

        ULONG size = 1u << 20;
        for (int attempt = 0; attempt < 10; ++attempt) {
            std::vector<std::byte> buffer(size);
            ULONG needed = 0;
            const LONG status = ntQuerySystemInformation(
                kSystemExtendedHandleInformation, buffer.data(), size, &needed);
            if (status >= 0) {
                return buffer;
            }
            if (status != kStatusInfoLengthMismatch) {
                return std::nullopt;
            }
            const ULONG next = std::max(size * 2u, needed + (1u << 16));
            if (next <= size) {
                return std::nullopt;
            }
            size = next;
        }
        return std::nullopt;
    }

    NtQueryObjectFn resolveNtQueryObject()
    {
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        if (!ntdll) {
            return nullptr;
        }
        return reinterpret_cast<NtQueryObjectFn>(GetProcAddress(ntdll, "NtQueryObject"));
    }

    std::wstring queryObjectNameDirect(HANDLE handle, NtQueryObjectFn ntQueryObject, LONG& status)
    {
        status = -1;
        if (!ntQueryObject || handle == INVALID_HANDLE_VALUE || handle == nullptr) {
            return {};
        }

        // 64 KiB is intentionally generous. Object-manager names for HID/file handles are tiny,
        // and using one fixed buffer avoids a second potentially blocking query for the required size.
        std::vector<std::byte> storage(64u * 1024u);
        ULONG returned = 0;
        status = ntQueryObject(
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

    DWORD WINAPI nameQueryThreadProc(LPVOID parameter)
    {
        auto* context = static_cast<NameQueryContext*>(parameter);
        context->objectName = queryObjectNameDirect(context->duplicated, context->ntQueryObject, context->status);
        CloseHandle(context->duplicated);
        context->duplicated = INVALID_HANDLE_VALUE;
        return 0;
    }

    std::optional<std::wstring> queryObjectNameWithTimeout(
        HANDLE duplicated,
        NtQueryObjectFn ntQueryObject,
        DWORD timeoutMs,
        bool& timedOut)
    {
        timedOut = false;
        auto* context = new NameQueryContext{};
        context->duplicated = duplicated;
        context->ntQueryObject = ntQueryObject;

        HANDLE worker = CreateThread(nullptr, 0, nameQueryThreadProc, context, 0, nullptr);
        if (!worker) {
            CloseHandle(duplicated);
            delete context;
            return std::nullopt;
        }

        const DWORD wait = WaitForSingleObject(worker, timeoutMs);
        if (wait == WAIT_OBJECT_0) {
            CloseHandle(worker);
            std::optional<std::wstring> result;
            if (context->status >= 0 && !context->objectName.empty()) {
                result = std::move(context->objectName);
            }
            delete context;
            return result;
        }

        if (wait == WAIT_TIMEOUT) {
            timedOut = true;
            // Important: never wait for a blocked device/file query here. We intentionally abandon
            // this tiny worker/context/duplicated-handle bundle. The process will reclaim it on exit.
            // A strict cap on abandoned workers prevents runaway resource use.
            CloseHandle(worker);
            return std::nullopt;
        }

        CloseHandle(worker);
        // The worker may still own the duplicated handle/context on WAIT_FAILED. Treat it exactly
        // like a timeout rather than risking a blocking cleanup path.
        timedOut = true;
        return std::nullopt;
    }

    std::wstring nameForPid(
        const std::unordered_map<DWORD, std::wstring>& names,
        DWORD pid)
    {
        if (const auto it = names.find(pid); it != names.end()) {
            return it->second;
        }
        return L"<unknown>";
    }

    bool potentialWriteHandle(ULONG access) noexcept
    {
        constexpr ULONG kFileWriteData = FILE_WRITE_DATA;
        constexpr ULONG kFileAppendData = FILE_APPEND_DATA;
        constexpr ULONG kFileWriteAttributes = FILE_WRITE_ATTRIBUTES;
        return (access & (kFileWriteData | kFileAppendData | kFileWriteAttributes)) != 0;
    }

    int processPriority(std::wstring_view processName) noexcept
    {
        if (_wcsicmp(processName.data(), L"Starfield.exe") == 0) {
            return 0;
        }
        if (processName.find(L"steam") != std::wstring_view::npos ||
            processName.find(L"Steam") != std::wstring_view::npos) {
            return 1;
        }
        if (_wcsicmp(processName.data(), L"GameInputSvc.exe") == 0 ||
            _wcsicmp(processName.data(), L"svchost.exe") == 0) {
            return 2;
        }
        return 3;
    }

    std::vector<OwnerRecord> scanOwners(
        const DeviceHandle& target,
        const std::unordered_map<DWORD, std::wstring>& names,
        std::wstring& targetObjectName,
        std::size_t& skippedProcesses,
        std::size_t& candidateHandles,
        std::size_t& prefilteredWriteHandles,
        std::size_t& fileTypeMatches,
        std::size_t& matchingObjectNameHandles,
        std::size_t& abandonedNameQueries,
        bool& scanTimedOut,
        bool& selfCheckPassed)
    {
        std::vector<OwnerRecord> owners;
        targetObjectName.clear();
        skippedProcesses = 0;
        candidateHandles = 0;
        prefilteredWriteHandles = 0;
        fileTypeMatches = 0;
        matchingObjectNameHandles = 0;
        abandonedNameQueries = 0;
        scanTimedOut = false;
        selfCheckPassed = false;

        const auto buffer = querySystemHandles();
        if (!buffer || buffer->size() < sizeof(SystemHandleInformationEx)) {
            return owners;
        }
        const NtQueryObjectFn ntQueryObject = resolveNtQueryObject();
        if (!ntQueryObject) {
            return owners;
        }

        const auto* info = reinterpret_cast<const SystemHandleInformationEx*>(buffer->data());
        const ULONG_PTR count = info->numberOfHandles;
        const DWORD currentPid = GetCurrentProcessId();
        const ULONG_PTR targetHandleValue = reinterpret_cast<ULONG_PTR>(target.handle);

        USHORT targetTypeIndex = 0;
        ULONG targetGrantedAccess = 0;
        for (ULONG_PTR i = 0; i < count; ++i) {
            const auto& entry = info->handles[i];
            if (entry.processId == currentPid && entry.handleValue == targetHandleValue) {
                targetTypeIndex = entry.objectTypeIndex;
                targetGrantedAccess = entry.grantedAccess;
                break;
            }
        }
        if (targetTypeIndex == 0) {
            return owners;
        }

        SetLastError(NO_ERROR);
        const DWORD targetFileType = GetFileType(target.handle);
        if (targetFileType == FILE_TYPE_UNKNOWN && GetLastError() != NO_ERROR) {
            return owners;
        }

        HANDLE targetDuplicate = INVALID_HANDLE_VALUE;
        if (!DuplicateHandle(
                GetCurrentProcess(),
                target.handle,
                GetCurrentProcess(),
                &targetDuplicate,
                0,
                FALSE,
                DUPLICATE_SAME_ACCESS)) {
            return owners;
        }

        bool targetNameTimedOut = false;
        const auto targetName = queryObjectNameWithTimeout(
            targetDuplicate, ntQueryObject, 500, targetNameTimedOut);
        if (!targetName || targetName->empty() || targetNameTimedOut) {
            return owners;
        }
        targetObjectName = *targetName;

        std::vector<const SystemHandleEntryEx*> candidates;
        candidates.reserve(1024);
        for (ULONG_PTR i = 0; i < count; ++i) {
            const auto& entry = info->handles[i];
            if (entry.objectTypeIndex != targetTypeIndex || entry.processId > MAXDWORD) {
                continue;
            }

            const bool isSelfTarget = entry.processId == currentPid && entry.handleValue == targetHandleValue;
            if (!isSelfTarget && !potentialWriteHandle(entry.grantedAccess)) {
                continue;
            }
            ++prefilteredWriteHandles;
            candidates.push_back(&entry);
        }

        std::stable_sort(candidates.begin(), candidates.end(), [&](const auto* a, const auto* b) {
            const bool aSelf = a->processId == currentPid && a->handleValue == targetHandleValue;
            const bool bSelf = b->processId == currentPid && b->handleValue == targetHandleValue;
            if (aSelf != bSelf) {
                return aSelf;
            }
            const auto aName = nameForPid(names, static_cast<DWORD>(a->processId));
            const auto bName = nameForPid(names, static_cast<DWORD>(b->processId));
            const int aPriority = processPriority(aName);
            const int bPriority = processPriority(bName);
            if (aPriority != bPriority) {
                return aPriority < bPriority;
            }
            if (a->processId != b->processId) {
                return a->processId < b->processId;
            }
            return a->handleValue < b->handleValue;
        });

        const auto scanStarted = std::chrono::steady_clock::now();
        std::unordered_map<DWORD, HANDLE> processHandles;

        std::cout << "Target NT object name=" << utf8(targetObjectName) << '\n';
        std::cout << "Target kernel file type=" << targetFileType
                  << " target access=0x" << std::hex << std::uppercase << targetGrantedAccess << std::dec << '\n';
        std::cout << "Scanning write-capable File handles and comparing NT object names; per-handle name timeout="
                  << kPerHandleNameQueryTimeoutMs << " ms, overall cap=" << (kScanBudgetMs / 1000)
                  << " seconds, abandoned-query cap=" << kMaxAbandonedNameQueries << ".\n";

        for (const auto* entry : candidates) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - scanStarted).count() >= kScanBudgetMs) {
                scanTimedOut = true;
                break;
            }
            if (abandonedNameQueries >= kMaxAbandonedNameQueries) {
                scanTimedOut = true;
                break;
            }

            ++candidateHandles;
            if (candidateHandles % kProgressInterval == 0) {
                const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - scanStarted).count();
                std::cout << "  progress: " << candidateHandles << '/' << candidates.size()
                          << " candidates, file-type matches=" << fileTypeMatches
                          << ", object-name matches=" << matchingObjectNameHandles
                          << ", abandoned=" << abandonedNameQueries
                          << ", elapsed=" << elapsedMs << " ms\n";
            }

            const DWORD pid = static_cast<DWORD>(entry->processId);
            HANDLE process = nullptr;
            if (pid == currentPid) {
                process = GetCurrentProcess();
            } else if (const auto it = processHandles.find(pid); it != processHandles.end()) {
                process = it->second;
            } else {
                process = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid);
                if (!process) {
                    ++skippedProcesses;
                    processHandles.emplace(pid, nullptr);
                    continue;
                }
                processHandles.emplace(pid, process);
            }

            if (!process) {
                continue;
            }

            HANDLE duplicated = INVALID_HANDLE_VALUE;
            if (!DuplicateHandle(
                    process,
                    reinterpret_cast<HANDLE>(entry->handleValue),
                    GetCurrentProcess(),
                    &duplicated,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS)) {
                continue;
            }

            SetLastError(NO_ERROR);
            const DWORD candidateFileType = GetFileType(duplicated);
            const DWORD fileTypeError = GetLastError();
            if (candidateFileType != targetFileType ||
                (candidateFileType == FILE_TYPE_UNKNOWN && fileTypeError != NO_ERROR)) {
                CloseHandle(duplicated);
                continue;
            }
            ++fileTypeMatches;

            bool nameTimedOut = false;
            const auto candidateObjectName = queryObjectNameWithTimeout(
                duplicated, ntQueryObject, kPerHandleNameQueryTimeoutMs, nameTimedOut);
            if (nameTimedOut) {
                ++abandonedNameQueries;
                continue;
            }
            if (!candidateObjectName || candidateObjectName->empty()) {
                continue;
            }
            if (_wcsicmp(candidateObjectName->c_str(), targetObjectName.c_str()) != 0) {
                continue;
            }
            ++matchingObjectNameHandles;

            OwnerRecord owner;
            owner.processId = pid;
            owner.processName = nameForPid(names, pid);
            owner.handleValue = entry->handleValue;
            owner.grantedAccess = entry->grantedAccess;
            owner.objectName = *candidateObjectName;
            owners.push_back(std::move(owner));

            if (pid == currentPid && entry->handleValue == targetHandleValue) {
                selfCheckPassed = true;
            }
        }

        for (auto& [pid, process] : processHandles) {
            (void)pid;
            if (process) {
                CloseHandle(process);
            }
        }

        std::sort(owners.begin(), owners.end(), [](const OwnerRecord& a, const OwnerRecord& b) {
            if (a.processName != b.processName) {
                return a.processName < b.processName;
            }
            if (a.processId != b.processId) {
                return a.processId < b.processId;
            }
            return a.handleValue < b.handleValue;
        });
        return owners;
    }

    bool writeCapable(ULONG access) noexcept
    {
        constexpr ULONG kFileWriteData = 0x0002;
        constexpr ULONG kFileAppendData = 0x0004;
        constexpr ULONG kFileWriteAttributes = 0x0100;
        return (access & (kFileWriteData | kFileAppendData | kFileWriteAttributes)) != 0;
    }
}

int main()
{
    std::cout
        << "StarfieldDualSense HID handle-owner probe v0.2.27g3\n"
        << "This probe sends NO controller output reports. It only opens the wired DualSense\n"
        << "and searches the Windows handle table for other processes with the same HID collection open.\n\n";

    const auto names = processNames();
    const bool starfieldRunning = processRunning(names, L"Starfield.exe");
    const bool debugPrivilege = enableSeDebugPrivilege();

    auto device = openDualSense();
    if (!device || !device->valid()) {
        std::cerr
            << "No wired DualSense/DualSense Edge gamepad HID collection was found.\n"
            << "Connect the controller by USB, keep DSX closed, and retry.\n";
        return 2;
    }

    std::cout << "Mode: " << (starfieldRunning ? "STARFIELD RUNNING" : "STARFIELD CLOSED") << '\n';
    std::cout << "SeDebugPrivilege: " << (debugPrivilege ? "enabled" : "not enabled (scan may be partial)") << '\n';
    std::cout << "DevicePath: " << utf8(device->path) << '\n';
    std::cout << "VID=054C PID=" << std::hex << std::uppercase << device->attributes.ProductID << std::dec
              << " input=" << device->caps.InputReportByteLength
              << " output=" << device->caps.OutputReportByteLength
              << " feature=" << device->caps.FeatureReportByteLength << '\n';
    if (!device->serial.empty()) {
        std::cout << "Serial: " << utf8(device->serial) << '\n';
    }

    std::cout << "\nScanning system handles with NT object-name matching and per-handle timeouts...\n";

    std::wstring targetObjectName;
    std::size_t skippedProcesses = 0;
    std::size_t candidateHandles = 0;
    std::size_t prefilteredWriteHandles = 0;
    std::size_t fileTypeMatches = 0;
    std::size_t matchingObjectNameHandles = 0;
    std::size_t abandonedNameQueries = 0;
    bool scanTimedOut = false;
    bool selfCheckPassed = false;
    const auto owners = scanOwners(
        *device, names, targetObjectName, skippedProcesses, candidateHandles, prefilteredWriteHandles,
        fileTypeMatches, matchingObjectNameHandles, abandonedNameQueries, scanTimedOut, selfCheckPassed);

    std::ostringstream report;
    report << "StarfieldDualSense HID handle-owner probe v0.2.27g3\n";
    report << "mode=" << (starfieldRunning ? "starfield-running" : "starfield-closed") << '\n';
    report << "debug_privilege=" << (debugPrivilege ? "enabled" : "not-enabled") << '\n';
    report << "device_path=" << utf8(device->path) << '\n';
    report << "target_nt_object_name=" << utf8(targetObjectName) << '\n';
    report << "vid=0x054C pid=0x" << std::hex << std::uppercase << device->attributes.ProductID << std::dec
           << " input=" << device->caps.InputReportByteLength
           << " output=" << device->caps.OutputReportByteLength
           << " feature=" << device->caps.FeatureReportByteLength << '\n';
    if (!device->serial.empty()) {
        report << "serial=" << utf8(device->serial) << '\n';
    }
    report << "prefiltered_write_handles=" << prefilteredWriteHandles << '\n';
    report << "candidate_file_handles_scanned=" << candidateHandles << '\n';
    report << "matching_file_type_handles=" << fileTypeMatches << '\n';
    report << "matching_object_name_handles=" << matchingObjectNameHandles << '\n';
    report << "abandoned_name_queries=" << abandonedNameQueries << '\n';
    report << "scan_timed_out=" << (scanTimedOut ? "yes" : "no") << '\n';
    report << "processes_skipped_access_denied=" << skippedProcesses << '\n';
    report << "self_check=" << (selfCheckPassed ? "PASS" : "FAIL") << '\n';
    report << "\nMATCHING DUALSENSE HANDLES\n";

    if (owners.empty()) {
        report << "<none>\n";
    } else {
        for (const auto& owner : owners) {
            report << utf8(owner.processName)
                   << " pid=" << owner.processId
                   << " handle=0x" << std::hex << std::uppercase << owner.handleValue
                   << " access=0x" << owner.grantedAccess << std::dec
                   << " write_capable=" << (writeCapable(owner.grantedAccess) ? "yes" : "no")
                   << " object_name=" << utf8(owner.objectName)
                   << '\n';
        }
    }

    std::cout << "\n================ MATCHING DUALSENSE HANDLES ================\n";
    for (const auto& owner : owners) {
        std::cout << utf8(owner.processName)
                  << "  PID " << owner.processId
                  << "  handle 0x" << std::hex << std::uppercase << owner.handleValue
                  << "  access 0x" << owner.grantedAccess << std::dec
                  << "  write=" << (writeCapable(owner.grantedAccess) ? "yes" : "no")
                  << '\n';
    }

    if (scanTimedOut) {
        std::cout << "\nNOTE: scan stopped at a safety bound (overall time or abandoned-name-query cap). Results may be partial, but the main thread did not block.\n";
    }
    if (abandonedNameQueries > 0) {
        std::cout << "NOTE: " << abandonedNameQueries
                  << " object-name queries exceeded " << kPerHandleNameQueryTimeoutMs
                  << " ms and were abandoned instead of blocking the scan.\n";
    }
    if (!selfCheckPassed) {
        std::cerr
            << "\nSELF-CHECK FAILED: the scanner could not rediscover its own DualSense handle by NT object name.\n"
            << "Do not trust this report; send the console output instead.\n";
    }
    if (!debugPrivilege) {
        std::cout
            << "\nNOTE: SeDebugPrivilege was unavailable. Normal user processes were still scanned,\n"
            << "but protected/elevated processes may be missing. Running PowerShell as Administrator\n"
            << "will make the comparison more complete.\n";
    }

    const char* resultFile = starfieldRunning
        ? "sds-hid-owner-starfield.txt"
        : "sds-hid-owner-closed.txt";
    std::ofstream out(resultFile, std::ios::trunc | std::ios::binary);
    if (out) {
        const std::string text = report.str();
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        std::cout << "\nSaved: " << resultFile << '\n';
    } else {
        std::cerr << "\nCould not save result file: " << resultFile << '\n';
    }

    std::cout
        << "\nRun this once with Starfield CLOSED and once after your save is loaded in Starfield.\n"
        << "Send me both sds-hid-owner-*.txt files.\n";
    return selfCheckPassed ? 0 : 3;
}
