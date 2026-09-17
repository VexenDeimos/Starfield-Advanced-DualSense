#include <StarfieldDualSense/WwiseSpatialOutputProbe.h>
#include <StarfieldDualSense/InputDiagnostics.h>
#include <StarfieldDualSense/WwiseSpatialProbeGate.h>

#include <RE/Starfield.h>
#include <REL/Relocation.h>
#include <REL/Trampoline.h>
#include <REL/Utility.h>

#include <Windows.h>
#include <intrin.h>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr std::uint8_t kArmThreshold = 160;
    constexpr std::uint8_t kReleaseThreshold = 64;
    constexpr std::uint64_t kCaptureWindowMs = 1000;
    constexpr std::uint64_t kReplayDelayMs = 750;
    constexpr std::uint32_t kEonFireEventId = 0xE7205CE1u;
    constexpr std::size_t kMaxDirectCallsites = 256;

    using PostEventFunction = RE::BGSAudio::AkSoundEngine::AkPlayingID (*)(
        RE::BGSAudio::AkSoundEngine::AkUniqueID,
        RE::BGSAudio::AkSoundEngine::AkGameObjectID,
        std::uint32_t,
        void*,
        void*,
        std::uint32_t,
        RE::BGSAudio::AkExternalSourceInfo*,
        RE::BGSAudio::AkSoundEngine::AkPlayingID);

    std::atomic<PostEventFunction> g_originalPostEvent{ nullptr };
    std::atomic<void*> g_probeObserver{ nullptr };

    template <class T>
    [[nodiscard]] bool safeReadValue(std::uintptr_t address, T& out) noexcept
    {
        SIZE_T bytesRead = 0;
        return address != 0 &&
            ::ReadProcessMemory(
                ::GetCurrentProcess(),
                reinterpret_cast<const void*>(address),
                &out,
                sizeof(T),
                &bytesRead) != FALSE &&
            bytesRead == sizeof(T);
    }

    [[nodiscard]] std::pair<std::uintptr_t, std::size_t> moduleTextSection(std::uintptr_t moduleBase) noexcept
    {
        if (!moduleBase) {
            return {};
        }

        IMAGE_DOS_HEADER dos{};
        if (!safeReadValue(moduleBase, dos) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
            dos.e_lfanew <= 0 || dos.e_lfanew > 0x01000000) {
            return {};
        }

        IMAGE_NT_HEADERS64 nt{};
        const auto ntAddress = moduleBase + static_cast<std::uintptr_t>(dos.e_lfanew);
        if (!safeReadValue(ntAddress, nt) || nt.Signature != IMAGE_NT_SIGNATURE ||
            nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
            return {};
        }

        const auto sectionAddress = ntAddress + offsetof(IMAGE_NT_HEADERS64, OptionalHeader) +
            nt.FileHeader.SizeOfOptionalHeader;
        for (std::uint16_t i = 0; i < nt.FileHeader.NumberOfSections; ++i) {
            IMAGE_SECTION_HEADER section{};
            if (!safeReadValue(sectionAddress + static_cast<std::uintptr_t>(i) * sizeof(section), section)) {
                break;
            }

            char name[9]{};
            std::memcpy(name, section.Name, 8);
            const bool executableCode =
                (section.Characteristics & IMAGE_SCN_CNT_CODE) != 0 &&
                (section.Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
            if (!executableCode || std::string_view(name) != ".text") {
                continue;
            }

            const auto size = static_cast<std::size_t>(
                section.Misc.VirtualSize != 0 ? section.Misc.VirtualSize : section.SizeOfRawData);
            return size == 0 ? std::pair<std::uintptr_t, std::size_t>{} :
                std::pair<std::uintptr_t, std::size_t>{ moduleBase + section.VirtualAddress, size };
        }
        return {};
    }
}

struct sds::WwiseSpatialOutputProbe::Impl
{
    explicit Impl(LogCallback callback) :
        log(std::move(callback))
    {}

    LogCallback log{};
    std::atomic<bool> started{ false };
    std::atomic<bool> armLogPending{ false };
    std::atomic<bool> triggerLatched{ false };
    std::atomic<bool> oneShotClaimed{ false };
    std::atomic<bool> captureLogPending{ false };
    std::atomic<bool> replayPending{ false };
    std::atomic<std::uint64_t> armedUntilMs{ 0 };
    std::atomic<std::uint64_t> replayDueMs{ 0 };
    std::uint32_t capturedEventId{ 0 };
    std::uint64_t capturedGameObjectId{ 0 };
    std::uint32_t capturedOriginalPlayingId{ 0 };
    std::size_t patchedCallsites{ 0 };
};

namespace
{
    RE::BGSAudio::AkSoundEngine::AkPlayingID postEventSpatialProbeThunk(
        RE::BGSAudio::AkSoundEngine::AkUniqueID eventId,
        RE::BGSAudio::AkSoundEngine::AkGameObjectID gameObjectId,
        std::uint32_t flags,
        void* callback,
        void* cookie,
        std::uint32_t externalCount,
        RE::BGSAudio::AkExternalSourceInfo* externalSources,
        RE::BGSAudio::AkSoundEngine::AkPlayingID requestedPlayingId)
    {
        const auto original = g_originalPostEvent.load(std::memory_order_acquire);
        const auto returnedPlayingId = original ? original(
            eventId,
            gameObjectId,
            flags,
            callback,
            cookie,
            externalCount,
            externalSources,
            requestedPlayingId) : 0;

        auto* observer = static_cast<sds::WwiseSpatialOutputProbe::Impl*>(
            g_probeObserver.load(std::memory_order_acquire));
        if (!observer || !observer->started.load(std::memory_order_acquire) ||
            observer->oneShotClaimed.load(std::memory_order_acquire)) {
            return returnedPlayingId;
        }

        const auto nowMs = ::GetTickCount64();
        const auto armedUntil = observer->armedUntilMs.load(std::memory_order_acquire);
        if (armedUntil == 0 || nowMs > armedUntil) {
            return returnedPlayingId;
        }

        const sds::SpatialProbeCandidate candidate{
            .eventId = eventId,
            .gameObjectId = gameObjectId,
            .externalCount = externalCount,
            .originalPlayingId = returnedPlayingId,
        };
        if (!sds::qualifiesSpatialProbeCandidate(candidate)) {
            return returnedPlayingId;
        }

        bool expected = false;
        if (!observer->oneShotClaimed.compare_exchange_strong(
                expected,
                true,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            return returnedPlayingId;
        }

        observer->capturedEventId = eventId;
        observer->capturedGameObjectId = gameObjectId;
        observer->capturedOriginalPlayingId = returnedPlayingId;
        observer->armedUntilMs.store(0, std::memory_order_release);
        observer->replayDueMs.store(nowMs + kReplayDelayMs, std::memory_order_relaxed);
        observer->captureLogPending.store(true, std::memory_order_release);
        observer->replayPending.store(true, std::memory_order_release);
        return returnedPlayingId;
    }
}

sds::WwiseSpatialOutputProbe::WwiseSpatialOutputProbe(LogCallback log) :
    impl_(std::make_unique<Impl>(std::move(log)))
{}

sds::WwiseSpatialOutputProbe::~WwiseSpatialOutputProbe()
{
    stop();
}

bool sds::WwiseSpatialOutputProbe::start()
{
    if (!impl_ || impl_->started.load(std::memory_order_acquire)) {
        return impl_ && impl_->started.load(std::memory_order_acquire);
    }
    if (!impl_->log) {
        return false;
    }

    const auto moduleBase = reinterpret_cast<std::uintptr_t>(::GetModuleHandleW(nullptr));
    REL::Relocation<std::uintptr_t> postEvent{ RE::ID::AkSoundEngine::PostEvent };
    const auto target = postEvent.address();
    if (!moduleBase || !target) {
        impl_->log("Wwise Eon dynamic identity proof: FAIL module/PostEvent relocation unavailable; no hook installed");
        return false;
    }

    const auto [textStart, textSize] = moduleTextSection(moduleBase);
    if (!textStart || textSize < 5) {
        impl_->log("Wwise Eon dynamic identity proof: FAIL Starfield .text section unavailable; no hook installed");
        return false;
    }

    std::vector<std::uint8_t> textBytes(textSize);
    SIZE_T bytesRead = 0;
    if (::ReadProcessMemory(
            ::GetCurrentProcess(),
            reinterpret_cast<const void*>(textStart),
            textBytes.data(),
            textBytes.size(),
            &bytesRead) == FALSE ||
        bytesRead != textBytes.size()) {
        impl_->log("Wwise Eon dynamic identity proof: FAIL Starfield .text snapshot failed; no hook installed");
        return false;
    }

    const auto callsites = sds::findDirectRel32CallsToTarget(textStart, textBytes, target);
    if (callsites.empty() || callsites.size() > kMaxDirectCallsites) {
        impl_->log("Wwise Eon dynamic identity proof: FAIL exact direct PostEvent callsite count outside bounded range; no hook installed");
        return false;
    }

    auto& trampoline = REL::GetTrampoline();
    if (trampoline.empty()) {
        trampoline.create(256, reinterpret_cast<void*>(target));
    }
    if (trampoline.free_size() < 32) {
        impl_->log("Wwise Eon dynamic identity proof: FAIL CommonLib trampoline has insufficient free space; no hook installed");
        return false;
    }

    const auto branchIsland = trampoline.allocate_branch5(
        reinterpret_cast<std::uintptr_t>(&postEventSpatialProbeThunk));
    std::vector<std::array<std::uint8_t, 5>> patches;
    patches.reserve(callsites.size());
    for (const auto& callsite : callsites) {
        const auto patch = sds::buildRel32CallPatch(callsite.instructionAddress, branchIsland);
        if (!patch) {
            impl_->log("Wwise Eon dynamic identity proof: FAIL branch island outside rel32 range; no hook installed");
            return false;
        }
        patches.push_back(*patch);
    }

    g_originalPostEvent.store(reinterpret_cast<PostEventFunction>(target), std::memory_order_release);
    std::size_t patched = 0;
    for (std::size_t i = 0; i < callsites.size(); ++i) {
        if (!REL::WriteSafe(callsites[i].instructionAddress, patches[i].data(), patches[i].size())) {
            break;
        }
        ++patched;
    }
    if (patched == 0) {
        g_originalPostEvent.store(nullptr, std::memory_order_release);
        impl_->log("Wwise Eon dynamic identity proof: FAIL no verified PostEvent callsites patched");
        return false;
    }

    impl_->patchedCallsites = patched;
    impl_->started.store(true, std::memory_order_release);
    g_probeObserver.store(impl_.get(), std::memory_order_release);

    std::ostringstream message;
    message << "Wwise Eon dynamic identity proof: ACTIVE exact PostEvent hook patched=" << patched
            << " event=0xE7205CE1"
            << " dynamicGameObject=yes"
            << " arm=R2>=" << static_cast<unsigned>(kArmThreshold)
            << " windowMs=" << kCaptureWindowMs
            << " replayDelayMs=" << kReplayDelayMs
            << " one-shot=yes";
    impl_->log(message.str());
    return true;
}

void sds::WwiseSpatialOutputProbe::stop() noexcept
{
    if (!impl_) {
        return;
    }
    impl_->started.store(false, std::memory_order_release);
    impl_->armedUntilMs.store(0, std::memory_order_release);
    impl_->replayPending.store(false, std::memory_order_release);
    void* expected = impl_.get();
    (void)g_probeObserver.compare_exchange_strong(
        expected,
        nullptr,
        std::memory_order_acq_rel,
        std::memory_order_acquire);
}

void sds::WwiseSpatialOutputProbe::observeRightTrigger(
    std::uint8_t r2,
    std::chrono::steady_clock::time_point) noexcept
{
    if (!impl_ || !impl_->started.load(std::memory_order_acquire) ||
        impl_->oneShotClaimed.load(std::memory_order_acquire)) {
        return;
    }

    const bool latched = impl_->triggerLatched.load(std::memory_order_relaxed);
    if (r2 <= kReleaseThreshold) {
        impl_->triggerLatched.store(false, std::memory_order_relaxed);
        return;
    }
    if (latched || r2 < kArmThreshold) {
        return;
    }

    impl_->triggerLatched.store(true, std::memory_order_relaxed);
    impl_->armedUntilMs.store(::GetTickCount64() + kCaptureWindowMs, std::memory_order_release);
    impl_->armLogPending.store(true, std::memory_order_release);
}

void sds::WwiseSpatialOutputProbe::tick()
{
    if (!impl_ || !impl_->started.load(std::memory_order_acquire)) {
        return;
    }

    if (impl_->armLogPending.exchange(false, std::memory_order_acq_rel) && impl_->log) {
        impl_->log("Wwise Eon dynamic identity proof: ARMED by DualSense R2 crossing; waiting up to 1000 ms for event=0xE7205CE1 on any valid internal game object");
    }

    if (impl_->captureLogPending.exchange(false, std::memory_order_acq_rel) && impl_->log) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "Wwise Eon dynamic identity proof: one-shot CAPTURE event=0x%X originalGameObject=0x%llX originalPlayingId=%u; replay scheduled after 750 ms",
            impl_->capturedEventId,
            static_cast<unsigned long long>(impl_->capturedGameObjectId),
            impl_->capturedOriginalPlayingId);
        impl_->log(message);
    }

    const auto nowMs = ::GetTickCount64();
    if (!impl_->oneShotClaimed.load(std::memory_order_acquire)) {
        auto deadline = impl_->armedUntilMs.load(std::memory_order_acquire);
        if (deadline != 0 && nowMs > deadline &&
            impl_->armedUntilMs.compare_exchange_strong(
                deadline,
                0,
                std::memory_order_acq_rel,
                std::memory_order_acquire) && impl_->log) {
            impl_->log("Wwise Eon dynamic identity proof: capture window EXPIRED without the target event; release R2 and fire again to re-arm");
        }
    }

    if (!impl_->replayPending.load(std::memory_order_acquire) ||
        nowMs < impl_->replayDueMs.load(std::memory_order_relaxed)) {
        return;
    }
    if (!impl_->replayPending.exchange(false, std::memory_order_acq_rel)) {
        return;
    }

    if (impl_->log) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "Wwise Eon dynamic identity proof: delayed DISPATCH replayDelayMs=750 event=0x%X originalGameObject=0x%llX; Wwise repost begins on normal runtime tick",
            impl_->capturedEventId,
            static_cast<unsigned long long>(impl_->capturedGameObjectId));
        impl_->log(message);
    }

    const auto playingId = RE::BGSAudio::AkSoundEngine::PostEvent(
        impl_->capturedEventId,
        impl_->capturedGameObjectId,
        0,
        nullptr,
        nullptr,
        0,
        nullptr,
        0);

    if (impl_->log) {
        char message[224]{};
        std::snprintf(
            message,
            sizeof(message),
            "Wwise Eon dynamic identity proof: one-shot POST event=0x%X originalGameObject=0x%llX result=%s playingId=%u",
            impl_->capturedEventId,
            static_cast<unsigned long long>(impl_->capturedGameObjectId),
            playingId != 0 ? "accepted" : "rejected",
            playingId);
        impl_->log(message);
    }
}

bool sds::WwiseSpatialOutputProbe::active() const noexcept
{
    return impl_ && impl_->started.load(std::memory_order_acquire);
}
