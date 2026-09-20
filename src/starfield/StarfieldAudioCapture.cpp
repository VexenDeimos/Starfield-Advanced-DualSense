#include <StarfieldDualSense/StarfieldAudioCapture.h>
#include <StarfieldDualSense/InputDiagnostics.h>
#include <StarfieldDualSense/MusicSelectionRecon.h>
#include <StarfieldDualSense/UiAudioCandidateCatalog.h>
#include <StarfieldDualSense/WwiseRemoteVoMirrorGate.h>
#include <StarfieldDualSense/WwiseRemoteVoDelay.h>

#include <RE/Starfield.h>
#include <REL/Relocation.h>
#include <REL/Trampoline.h>
#include <REL/Utility.h>

#include <Windows.h>
#include <intrin.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr auto kWwiseEndOfEventCallback = 0x0001u;
}

namespace
{
    constexpr std::size_t kDiagnosticQueueCapacity = 256;
    constexpr std::size_t kWeaponSfxQueueCapacity = 512;
    constexpr std::size_t kUiAudioQueueCapacity = 1024;
    constexpr std::size_t kMusicReconQueueCapacity = 4096;
    constexpr std::size_t kMusicSelectionQueueCapacity = 128;
    using sds::isMusicSelectionReconTargetEvent;
    using sds::kWwiseDurationCallback;
    using sds::kWwiseCallbackBitsMask;
    using sds::MusicSelectionCallbackChainContext;
    using sds::MusicSelectionCallbackChainPool;
    using sds::WwiseCallbackFunction;
    using sds::WwiseEventCallbackInfo;
    using sds::WwiseDurationCallbackInfo;
    constexpr std::size_t kSamplePrefixBytes = 16;
    constexpr std::size_t kMaxExternalSourcesInspected = 4;
    constexpr std::size_t kMaxFilePathCharacters = 260;
    constexpr std::size_t kMaxDirectCallsites = 256;

    struct AudioPostDiagnostic
    {
        std::uint64_t sequence{ 0 };
        std::int64_t captureSteadyMicros{ 0 };
        std::uint32_t threadId{ 0 };
        std::uintptr_t callsiteRva{ 0 };
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uint32_t flags{ 0 };
        std::uint32_t externalCount{ 0 };
        std::uint32_t externalIndex{ 0 };
        std::uint32_t externalCookie{ 0 };
        std::uint32_t codecId{ 0 };
        std::uint32_t fileId{ 0 };
        std::uint32_t memorySize{ 0 };
        std::uint32_t requestedPlayingId{ 0 };
        std::uint32_t returnedPlayingId{ 0 };
        bool hasMemory{ false };
        bool hasFilePath{ false };
        bool dialogueMenuActive{ false };
        std::uint16_t filePathLength{ 0 };
        std::array<wchar_t, kMaxFilePathCharacters> filePath{};
        std::uint8_t samplePrefixSize{ 0 };
        std::array<std::uint8_t, kSamplePrefixBytes> samplePrefix{};
    };

    struct WeaponSfxPostRecord
    {
        std::uint64_t sequence{ 0 };
        std::int64_t captureSteadyMicros{ 0 };
        std::uint32_t threadId{ 0 };
        std::uintptr_t callsiteRva{ 0 };
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uint32_t flags{ 0 };
        std::uint32_t externalCount{ 0 };
        bool hasExternalSources{ false };
        std::uint32_t requestedPlayingId{ 0 };
        std::uint32_t returnedPlayingId{ 0 };
    };

    struct UiAudioPostRecord
    {
        std::uint64_t sequence{ 0 };
        std::int64_t captureSteadyMicros{ 0 };
        std::uint32_t threadId{ 0 };
        std::uintptr_t callsiteRva{ 0 };
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uint32_t flags{ 0 };
        std::uint32_t externalCount{ 0 };
        bool hasExternalSources{ false };
        std::uint32_t requestedPlayingId{ 0 };
        std::uint32_t returnedPlayingId{ 0 };
    };


    struct MusicReconPostRecord
    {
        std::uint64_t sequence{ 0 };
        std::int64_t captureSteadyMicros{ 0 };
        std::uint32_t threadId{ 0 };
        std::uintptr_t callsiteRva{ 0 };
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uint32_t flags{ 0 };
        std::uint32_t externalCount{ 0 };
        std::uint32_t requestedPlayingId{ 0 };
        std::uint32_t returnedPlayingId{ 0 };
    };

    enum class MusicSelectionRecordKind : std::uint8_t
    {
        kPost,
        kDuration,
        kEnd,
    };

    struct MusicSelectionRecord
    {
        MusicSelectionRecordKind kind{ MusicSelectionRecordKind::kPost };
        std::int64_t captureSteadyMicros{ 0 };
        std::uint32_t eventId{ 0 };
        std::uint64_t gameObjectId{ 0 };
        std::uint32_t playingId{ 0 };
        std::uint32_t requestedPlayingId{ 0 };
        std::uint32_t originalFlags{ 0 };
        std::uint32_t effectiveFlags{ 0 };
        std::uint32_t externalCount{ 0 };
        bool hadCallback{ false };
        bool hadCookie{ false };
        bool injectedDurationCallback{ false };
        bool chainedExistingCallback{ false };
        bool chainContextUnavailable{ false };
        bool chainPlayingIdMismatch{ false };
        float durationMs{ 0.0F };
        float estimatedDurationMs{ 0.0F };
        std::uint32_t audioNodeId{ 0 };
        std::uint32_t mediaId{ 0 };
        bool streaming{ false };
    };

    template <class Record, std::size_t Capacity>
    class DeferredCaptureQueue
    {
    public:
        DeferredCaptureQueue() noexcept
        {
            for (std::size_t i = 0; i < Capacity; ++i) {
                _slots[i].sequence.store(i, std::memory_order_relaxed);
            }
        }

        [[nodiscard]] bool tryPush(const Record& record) noexcept
        {
            std::size_t pos = _enqueuePos.load(std::memory_order_relaxed);
            for (;;) {
                auto& slot = _slots[pos % Capacity];
                const std::size_t sequence = slot.sequence.load(std::memory_order_acquire);
                const auto difference = static_cast<std::intptr_t>(sequence) - static_cast<std::intptr_t>(pos);
                if (difference == 0) {
                    if (_enqueuePos.compare_exchange_weak(
                            pos,
                            pos + 1,
                            std::memory_order_relaxed,
                            std::memory_order_relaxed)) {
                        slot.record = record;
                        slot.sequence.store(pos + 1, std::memory_order_release);
                        return true;
                    }
                } else if (difference < 0) {
                    return false;
                } else {
                    pos = _enqueuePos.load(std::memory_order_relaxed);
                }
            }
        }

        [[nodiscard]] bool tryPop(Record& record) noexcept
        {
            std::size_t pos = _dequeuePos.load(std::memory_order_relaxed);
            for (;;) {
                auto& slot = _slots[pos % Capacity];
                const std::size_t sequence = slot.sequence.load(std::memory_order_acquire);
                const auto difference = static_cast<std::intptr_t>(sequence) - static_cast<std::intptr_t>(pos + 1);
                if (difference == 0) {
                    if (_dequeuePos.compare_exchange_weak(
                            pos,
                            pos + 1,
                            std::memory_order_relaxed,
                            std::memory_order_relaxed)) {
                        record = slot.record;
                        slot.sequence.store(pos + Capacity, std::memory_order_release);
                        return true;
                    }
                } else if (difference < 0) {
                    return false;
                } else {
                    pos = _dequeuePos.load(std::memory_order_relaxed);
                }
            }
        }

    private:
        struct Slot
        {
            std::atomic<std::size_t> sequence{ 0 };
            Record record{};
        };

        std::array<Slot, Capacity> _slots{};
        std::atomic<std::size_t> _enqueuePos{ 0 };
        std::atomic<std::size_t> _dequeuePos{ 0 };
    };

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
    std::atomic<void*> g_captureObserver{ nullptr };
    std::atomic<std::uintptr_t> g_starfieldModuleBase{ 0 };
    std::atomic<std::uint64_t> g_diagnosticSequence{ 0 };
    DeferredCaptureQueue<MusicSelectionRecord, kMusicSelectionQueueCapacity> g_musicSelectionRecords{};
    std::atomic<std::uint64_t> g_musicSelectionDropped{ 0 };
    std::atomic_bool g_musicSelectionCallbackArmed{ false };
    MusicSelectionCallbackChainPool g_musicSelectionCallbackChainPool{};

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

    [[nodiscard]] std::pair<std::uintptr_t, std::size_t> moduleTextSection(
        std::uintptr_t moduleBase) noexcept
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
            if (size == 0) {
                return {};
            }
            return { moduleBase + section.VirtualAddress, size };
        }
        return {};
    }

    [[nodiscard]] std::string formatDiagnostic(const AudioPostDiagnostic& record)
    {
        std::ostringstream out;
        out << "Starfield audio capture: seq=" << record.sequence
            << " thread=" << record.threadId
            << " callsite=Starfield+0x" << std::hex << std::uppercase << record.callsiteRva
            << " event=0x" << record.eventId
            << " gameObject=0x" << record.gameObjectId
            << " flags=0x" << record.flags
            << std::dec
            << " externals=" << record.externalCount
            << " extIndex=" << record.externalIndex
            << " cookie=0x" << std::hex << std::uppercase << record.externalCookie
            << std::dec
            << " codec=" << record.codecId
            << " fileId=" << record.fileId
            << " memorySize=" << record.memorySize
            << " hasMemory=" << (record.hasMemory ? "yes" : "no")
            << " hasFilePath=" << (record.hasFilePath ? "yes" : "no")
            << " dialogueMenu=" << (record.dialogueMenuActive ? "open" : "closed")
            << " filePath=";

        if (record.filePathLength == 0) {
            out << "<none>";
        } else {
            const int sourceLength = static_cast<int>(record.filePathLength);
            const int required = ::WideCharToMultiByte(CP_UTF8, 0, record.filePath.data(), sourceLength, nullptr, 0, nullptr, nullptr);
            if (required <= 0) {
                out << "<conversion-failed>";
            } else {
                std::string utf8(static_cast<std::size_t>(required), '\0');
                (void)::WideCharToMultiByte(CP_UTF8, 0, record.filePath.data(), sourceLength, utf8.data(), required, nullptr, nullptr);
                out << '\"' << utf8 << '\"';
            }
        }

        out << " requestedPlayingId=" << record.requestedPlayingId
            << " returnedPlayingId=" << record.returnedPlayingId
            << " samplePrefix=";

        if (record.samplePrefixSize == 0) {
            out << "<none>";
        } else {
            out << std::hex << std::setfill('0');
            for (std::size_t i = 0; i < record.samplePrefixSize; ++i) {
                if (i != 0) {
                    out << ' ';
                }
                out << std::setw(2) << static_cast<unsigned int>(record.samplePrefix[i]);
            }
        }
        return out.str();
    }
}

struct sds::StarfieldAudioCapture::Impl
{
    MusicSelectionObservationCallback musicSelectionObservation{};
    Impl(
        LogCallback callback,
        RemoteVoMirrorCallback mirrorCallback,
        VoMirrorQualificationProfile qualification,
        RemoteVoSourceCallback sourceProbeCallback,
        WeaponSfxObservationCallback weaponSfxObservationCallback,
        UiAudioObservationCallback uiAudioObservationCallback,
        MusicReconObservationCallback musicReconObservationCallback,
        MusicSelectionObservationCallback musicSelectionObservationCallback) :
        log(std::move(callback)),
        mirror(std::move(mirrorCallback)),
        sourceProbe(std::move(sourceProbeCallback)),
        weaponSfxObservation(std::move(weaponSfxObservationCallback)),
        uiAudioObservation(std::move(uiAudioObservationCallback)),
        musicReconObservation(std::move(musicReconObservationCallback)),
        musicSelectionObservation(std::move(musicSelectionObservationCallback)),
        qualification(qualification),
        mirrorGate(qualification),
        mirrorDelay(std::chrono::milliseconds(750))
    {}

    LogCallback log{};
    RemoteVoMirrorCallback mirror{};
    RemoteVoSourceCallback sourceProbe{};
    WeaponSfxObservationCallback weaponSfxObservation{};
    UiAudioObservationCallback uiAudioObservation{};
    MusicReconObservationCallback musicReconObservation{};
    VoMirrorQualificationProfile qualification{};
    OneShotRemoteVoMirrorGate mirrorGate{};
    DelayedRemoteVoMirrorDispatch mirrorDelay;
    DeferredCaptureQueue<AudioPostDiagnostic, kDiagnosticQueueCapacity> records{};
    DeferredCaptureQueue<WeaponSfxPostRecord, kWeaponSfxQueueCapacity> weaponSfxRecords{};
    DeferredCaptureQueue<UiAudioPostRecord, kUiAudioQueueCapacity> uiAudioRecords{};
    DeferredCaptureQueue<MusicReconPostRecord, kMusicReconQueueCapacity> musicReconRecords{};
    std::atomic<std::uint64_t> dropped{ 0 };
    std::atomic<std::uint64_t> weaponSfxDropped{ 0 };
    std::atomic<std::uint64_t> uiAudioDropped{ 0 };
    std::atomic<std::uint64_t> musicReconDropped{ 0 };
    std::atomic<bool> weaponSfxDiscoveryArmed{ false };
    std::atomic<bool> shipWeaponObservationArmed{ false };
    std::atomic<bool> uiAudioDiscoveryArmed{ false };
    std::atomic<bool> uiAudioPlaybackArmed{ false };
    std::atomic_bool musicReconArmed{ false };
    std::atomic<bool> started{ false };
    std::atomic<bool> dialogueMenuActive{ false };
    std::size_t patchedCallsites{ 0 };
};

namespace
{
    void enqueueMusicSelectionDurationRecord(const WwiseDurationCallbackInfo& info) noexcept
    {
        const auto capturedAt = std::chrono::steady_clock::now();
        const MusicSelectionRecord record{
            .kind = MusicSelectionRecordKind::kDuration,
            .captureSteadyMicros = std::chrono::duration_cast<std::chrono::microseconds>(
                capturedAt.time_since_epoch()).count(),
            .eventId = info.eventId,
            .gameObjectId = info.gameObjectId,
            .playingId = info.playingId,
            .durationMs = info.durationMs,
            .estimatedDurationMs = info.estimatedDurationMs,
            .audioNodeId = info.audioNodeId,
            .mediaId = info.mediaId,
            .streaming = info.streaming,
        };
        if (!g_musicSelectionRecords.tryPush(record)) {
            g_musicSelectionDropped.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void enqueueMusicSelectionEndRecord(const WwiseEventCallbackInfo& info) noexcept
    {
        const auto capturedAt = std::chrono::steady_clock::now();
        const MusicSelectionRecord record{
            .kind = MusicSelectionRecordKind::kEnd,
            .captureSteadyMicros = std::chrono::duration_cast<std::chrono::microseconds>(
                capturedAt.time_since_epoch()).count(),
            .eventId = info.eventId,
            .gameObjectId = info.gameObjectId,
            .playingId = info.playingId,
        };
        if (!g_musicSelectionRecords.tryPush(record)) {
            g_musicSelectionDropped.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void musicSelectionDurationCallback(std::uint32_t callbackType, void* callbackInfo) noexcept
    {
        if (callbackType != kWwiseDurationCallback || callbackInfo == nullptr ||
            !g_musicSelectionCallbackArmed.load(std::memory_order_acquire)) {
            return;
        }

        const auto* info = static_cast<const WwiseDurationCallbackInfo*>(callbackInfo);
        if (!isMusicSelectionReconTargetEvent(info->eventId)) {
            return;
        }

        enqueueMusicSelectionDurationRecord(*info);
    }

    void musicSelectionChainedCallback(std::uint32_t callbackType, void* callbackInfo) noexcept
    {
        if (callbackInfo == nullptr) {
            return;
        }

        const auto* eventInfo = static_cast<const WwiseEventCallbackInfo*>(callbackInfo);
        auto* chainContext = g_musicSelectionCallbackChainPool.findForCallback(*eventInfo);
        if (!g_musicSelectionCallbackChainPool.callbackReady(chainContext)) {
            return;
        }

        const auto targetEventId = chainContext->eventId;

        if (g_musicSelectionCallbackArmed.load(std::memory_order_acquire)) {
            if (callbackType == kWwiseDurationCallback) {
                const auto* durationInfo = static_cast<const WwiseDurationCallbackInfo*>(callbackInfo);
                if (durationInfo->eventId == targetEventId &&
                    isMusicSelectionReconTargetEvent(durationInfo->eventId)) {
                    enqueueMusicSelectionDurationRecord(*durationInfo);
                }
            } else if (callbackType == kWwiseEndOfEventCallback &&
                eventInfo->eventId == targetEventId &&
                isMusicSelectionReconTargetEvent(eventInfo->eventId)) {
                enqueueMusicSelectionEndRecord(*eventInfo);
            }
        }

        g_musicSelectionCallbackChainPool.forwardOriginalCallback(
            chainContext, callbackType, callbackInfo);
    }

    RE::BGSAudio::AkSoundEngine::AkPlayingID postEventDiagnosticThunk(
        RE::BGSAudio::AkSoundEngine::AkUniqueID eventId,
        RE::BGSAudio::AkSoundEngine::AkGameObjectID gameObjectId,
        std::uint32_t flags,
        void* callback,
        void* cookie,
        std::uint32_t externalCount,
        RE::BGSAudio::AkExternalSourceInfo* externalSources,
        RE::BGSAudio::AkSoundEngine::AkPlayingID requestedPlayingId)
    {
        AudioPostDiagnostic record{};
        bool shouldRecord = false;
        WeaponSfxPostRecord weaponRecord{};
        bool shouldRecordWeaponSfx = false;
        auto* observer = static_cast<sds::StarfieldAudioCapture::Impl*>(
            g_captureObserver.load(std::memory_order_acquire));
        if (observer && observer->started.load(std::memory_order_acquire) && externalSources && externalCount > 0) {
            const auto inspected = (std::min)(
                static_cast<std::size_t>(externalCount),
                kMaxExternalSourcesInspected);
            for (std::size_t i = 0; i < inspected; ++i) {
                const auto& source = externalSources[i];
                if (source.iExternalSrcCookie != RE::BGSAudio::kExternalSourceCookie) {
                    continue;
                }

                const auto moduleBase = g_starfieldModuleBase.load(std::memory_order_acquire);
                const auto returnAddress = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
                const auto callsite = returnAddress >= 5 ? returnAddress - 5 : returnAddress;

                record.sequence = g_diagnosticSequence.fetch_add(1, std::memory_order_relaxed) + 1;
                record.captureSteadyMicros = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
                record.threadId = ::GetCurrentThreadId();
                record.callsiteRva = moduleBase && callsite >= moduleBase ? callsite - moduleBase : 0;
                record.eventId = eventId;
                record.gameObjectId = gameObjectId;
                record.flags = flags;
                record.externalCount = externalCount;
                record.externalIndex = static_cast<std::uint32_t>(i);
                record.externalCookie = source.iExternalSrcCookie;
                record.codecId = source.idCodec;
                record.fileId = source.idFile;
                record.memorySize = source.uiMemorySize;
                record.requestedPlayingId = requestedPlayingId;
                record.hasMemory = source.pInMemory != nullptr;
                record.hasFilePath = source.szFile != nullptr;
                record.dialogueMenuActive = observer->dialogueMenuActive.load(std::memory_order_relaxed);
                if (source.szFile) {
                    for (std::size_t pathIndex = 0; pathIndex + 1 < record.filePath.size(); ++pathIndex) {
                        const wchar_t character = source.szFile[pathIndex];
                        if (character == L'\0') {
                            break;
                        }
                        record.filePath[pathIndex] = character;
                        record.filePathLength = static_cast<std::uint16_t>(pathIndex + 1);
                    }
                }
                if (source.pInMemory && source.uiMemorySize > 0) {
                    const auto prefixSize = (std::min)(
                        static_cast<std::size_t>(source.uiMemorySize),
                        kSamplePrefixBytes);
                    std::memcpy(record.samplePrefix.data(), source.pInMemory, prefixSize);
                    record.samplePrefixSize = static_cast<std::uint8_t>(prefixSize);
                }
                shouldRecord = true;
                break;
            }
        }

        UiAudioPostRecord uiAudioRecord{};
        bool shouldRecordUiAudio = false;
        MusicReconPostRecord musicReconRecord{};
        bool shouldRecordMusicRecon = false;
        const bool zeroExternal = externalCount == 0 || externalSources == nullptr;
        const bool wantsWeapon = observer && observer->started.load(std::memory_order_acquire) &&
            observer->weaponSfxDiscoveryArmed.load(std::memory_order_relaxed) && zeroExternal;
        const bool wantsShipWeapon = observer && observer->started.load(std::memory_order_acquire) &&
            observer->shipWeaponObservationArmed.load(std::memory_order_relaxed) && zeroExternal;
        const bool wantsUiDiscovery = observer && observer->started.load(std::memory_order_acquire) &&
            observer->uiAudioDiscoveryArmed.load(std::memory_order_relaxed) && zeroExternal &&
            !sds::isPromotedUiSpeakerEvent(eventId);
        const bool wantsUiPlayback = observer && observer->started.load(std::memory_order_acquire) &&
            observer->uiAudioPlaybackArmed.load(std::memory_order_relaxed) && zeroExternal &&
            sds::isPromotedUiSpeakerEvent(eventId);
        const bool wantsUi = wantsUiDiscovery || wantsUiPlayback;
        const bool wantsMusic = observer && observer->started.load(std::memory_order_acquire) &&
            observer->musicReconArmed.load(std::memory_order_acquire) && externalCount == 0;

        if (wantsWeapon || wantsShipWeapon || wantsUi || wantsMusic) {
            const auto sequence = g_diagnosticSequence.fetch_add(1, std::memory_order_relaxed) + 1;
            const auto captureSteadyMicros = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            const auto threadId = ::GetCurrentThreadId();
            const auto moduleBase = g_starfieldModuleBase.load(std::memory_order_acquire);
            const auto returnAddress = reinterpret_cast<std::uintptr_t>(_ReturnAddress());
            const auto callsite = returnAddress >= 5 ? returnAddress - 5 : returnAddress;
            const auto callsiteRva = moduleBase && callsite >= moduleBase ? callsite - moduleBase : 0;

            if (wantsWeapon || wantsShipWeapon) {
                weaponRecord.sequence = sequence;
                weaponRecord.captureSteadyMicros = captureSteadyMicros;
                weaponRecord.threadId = threadId;
                weaponRecord.callsiteRva = callsiteRva;
                weaponRecord.eventId = eventId;
                weaponRecord.gameObjectId = gameObjectId;
                weaponRecord.flags = flags;
                weaponRecord.externalCount = externalCount;
                weaponRecord.hasExternalSources = externalSources != nullptr;
                weaponRecord.requestedPlayingId = requestedPlayingId;
                shouldRecordWeaponSfx = true;
            }

            if (wantsUi) {
                uiAudioRecord.sequence = sequence;
                uiAudioRecord.captureSteadyMicros = captureSteadyMicros;
                uiAudioRecord.threadId = threadId;
                uiAudioRecord.callsiteRva = callsiteRva;
                uiAudioRecord.eventId = eventId;
                uiAudioRecord.gameObjectId = gameObjectId;
                uiAudioRecord.flags = flags;
                uiAudioRecord.externalCount = externalCount;
                uiAudioRecord.hasExternalSources = externalSources != nullptr;
                uiAudioRecord.requestedPlayingId = requestedPlayingId;
                shouldRecordUiAudio = true;
            }

            if (wantsMusic) {
                musicReconRecord.sequence = sequence;
                musicReconRecord.captureSteadyMicros = captureSteadyMicros;
                musicReconRecord.threadId = threadId;
                musicReconRecord.callsiteRva = callsiteRva;
                musicReconRecord.eventId = eventId;
                musicReconRecord.gameObjectId = gameObjectId;
                musicReconRecord.flags = flags;
                musicReconRecord.externalCount = externalCount;
                musicReconRecord.requestedPlayingId = requestedPlayingId;
                shouldRecordMusicRecon = true;
            }
        }

        const bool wantsMusicSelectionPost =
            g_musicSelectionCallbackArmed.load(std::memory_order_acquire) &&
            isMusicSelectionReconTargetEvent(eventId);
        const sds::MusicSelectionPostContext musicSelectionCandidate{
            .armed = wantsMusicSelectionPost,
            .eventId = eventId,
            .externalCount = externalCount,
            .hasCallback = callback != nullptr,
            .hasCookie = cookie != nullptr,
            .flags = flags,
        };
        const bool injectDurationCallback =
            sds::qualifiesMusicSelectionPost(musicSelectionCandidate);
        const bool wantsCallbackChain =
            sds::qualifiesMusicSelectionCallbackChain(musicSelectionCandidate);

        MusicSelectionCallbackChainContext* chainContext = nullptr;
        bool chainContextUnavailable = false;
        if (wantsCallbackChain) {
            chainContext = g_musicSelectionCallbackChainPool.acquire(
                reinterpret_cast<WwiseCallbackFunction>(callback),
                cookie,
                eventId,
                gameObjectId);
            chainContextUnavailable = chainContext == nullptr;
        }
        const bool chainedExistingCallback = chainContext != nullptr;

        const auto effectiveFlags = injectDurationCallback ? (flags | kWwiseDurationCallback) : flags;
        void* const effectiveCallback = injectDurationCallback
            ? reinterpret_cast<void*>(&musicSelectionDurationCallback)
            : (chainedExistingCallback
                    ? reinterpret_cast<void*>(&musicSelectionChainedCallback)
                    : callback);
        void* const effectiveCookie = injectDurationCallback ? nullptr : cookie;

        const auto original = g_originalPostEvent.load(std::memory_order_acquire);
        const auto returnedPlayingId = original ? original(
            eventId,
            gameObjectId,
            effectiveFlags,
            effectiveCallback,
            effectiveCookie,
            externalCount,
            externalSources,
            requestedPlayingId) : 0;

        bool chainPlayingIdMismatch = false;
        if (chainedExistingCallback) {
            chainPlayingIdMismatch =
                !g_musicSelectionCallbackChainPool.completePost(chainContext, returnedPlayingId);
        }

        if (shouldRecord) {
            record.returnedPlayingId = returnedPlayingId;
            if (auto* observer = static_cast<sds::StarfieldAudioCapture::Impl*>(
                    g_captureObserver.load(std::memory_order_acquire));
                observer && !observer->records.tryPush(record)) {
                observer->dropped.fetch_add(1, std::memory_order_relaxed);
            }
        }

        if (shouldRecordWeaponSfx) {
            weaponRecord.returnedPlayingId = returnedPlayingId;
            if (auto* liveObserver = static_cast<sds::StarfieldAudioCapture::Impl*>(
                    g_captureObserver.load(std::memory_order_acquire));
                liveObserver && !liveObserver->weaponSfxRecords.tryPush(weaponRecord)) {
                liveObserver->weaponSfxDropped.fetch_add(1, std::memory_order_relaxed);
            }
        }

        if (shouldRecordUiAudio) {
            uiAudioRecord.returnedPlayingId = returnedPlayingId;
            if (auto* liveObserver = static_cast<sds::StarfieldAudioCapture::Impl*>(
                    g_captureObserver.load(std::memory_order_acquire));
                liveObserver && !liveObserver->uiAudioRecords.tryPush(uiAudioRecord)) {
                liveObserver->uiAudioDropped.fetch_add(1, std::memory_order_relaxed);
            }
        }

        if (shouldRecordMusicRecon) {
            musicReconRecord.returnedPlayingId = returnedPlayingId;
            if (auto* liveObserver = static_cast<sds::StarfieldAudioCapture::Impl*>(
                    g_captureObserver.load(std::memory_order_acquire));
                liveObserver && !liveObserver->musicReconRecords.tryPush(musicReconRecord)) {
                liveObserver->musicReconDropped.fetch_add(1, std::memory_order_relaxed);
            }
        }

        if (wantsMusicSelectionPost) {
            const MusicSelectionRecord selectionPost{
                .kind = MusicSelectionRecordKind::kPost,
                .eventId = eventId,
                .gameObjectId = gameObjectId,
                .playingId = returnedPlayingId,
                .requestedPlayingId = requestedPlayingId,
                .originalFlags = flags,
                .effectiveFlags = effectiveFlags,
                .externalCount = externalCount,
                .hadCallback = callback != nullptr,
                .hadCookie = cookie != nullptr,
                .injectedDurationCallback = injectDurationCallback,
                .chainedExistingCallback = chainedExistingCallback,
                .chainContextUnavailable = chainContextUnavailable,
                .chainPlayingIdMismatch = chainPlayingIdMismatch,
            };
            if (!g_musicSelectionRecords.tryPush(selectionPost)) {
                g_musicSelectionDropped.fetch_add(1, std::memory_order_relaxed);
            }
        }

        return returnedPlayingId;
    }
}

sds::StarfieldAudioCapture::StarfieldAudioCapture(
    LogCallback log,
    RemoteVoMirrorCallback mirror,
    VoMirrorQualificationProfile qualification,
    RemoteVoSourceCallback sourceProbe,
    WeaponSfxObservationCallback weaponSfxObservation,
    UiAudioObservationCallback uiAudioObservation,
    MusicReconObservationCallback musicReconObservation,
    MusicSelectionObservationCallback musicSelectionObservation) :
    _impl(std::make_unique<Impl>(
        std::move(log),
        std::move(mirror),
        qualification,
        std::move(sourceProbe),
        std::move(weaponSfxObservation),
        std::move(uiAudioObservation),
        std::move(musicReconObservation),
        std::move(musicSelectionObservation)))
{}

sds::StarfieldAudioCapture::~StarfieldAudioCapture()
{
    stop();
}

bool sds::StarfieldAudioCapture::start()
{
    if (!_impl || _impl->started.load(std::memory_order_acquire)) {
        return _impl && _impl->started.load(std::memory_order_acquire);
    }

    const auto moduleBase = reinterpret_cast<std::uintptr_t>(::GetModuleHandleW(nullptr));
    REL::Relocation<std::uintptr_t> postEvent{ RE::ID::AkSoundEngine::PostEvent };
    const auto target = postEvent.address();
    if (!moduleBase || !target) {
        if (_impl->log) {
            _impl->log("Starfield audio capture: module/PostEvent relocation unavailable; diagnostic disabled");
        }
        return false;
    }

    const auto [textStart, textSize] = moduleTextSection(moduleBase);
    if (!textStart || textSize < 5) {
        if (_impl->log) {
            _impl->log("Starfield audio capture: Starfield .text section unavailable; diagnostic disabled");
        }
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
        if (_impl->log) {
            _impl->log("Starfield audio capture: failed to snapshot Starfield .text section; diagnostic disabled");
        }
        return false;
    }

    const auto callsites = sds::findDirectRel32CallsToTarget(textStart, textBytes, target);
    if (callsites.empty()) {
        if (_impl->log) {
            _impl->log("Starfield audio capture: no exact direct PostEvent callsites found; diagnostic disabled");
        }
        return false;
    }
    if (callsites.size() > kMaxDirectCallsites) {
        if (_impl->log) {
            char message[192]{};
            std::snprintf(
                message,
                sizeof(message),
                "Starfield audio capture: refusing to patch %llu direct PostEvent callers (limit=%llu)",
                static_cast<unsigned long long>(callsites.size()),
                static_cast<unsigned long long>(kMaxDirectCallsites));
            _impl->log(message);
        }
        return false;
    }

    auto& trampoline = REL::GetTrampoline();
    if (trampoline.empty()) {
        trampoline.create(256, reinterpret_cast<void*>(target));
    }
    if (trampoline.free_size() < 32) {
        if (_impl->log) {
            _impl->log("Starfield audio capture: CommonLib trampoline has insufficient free space; diagnostic disabled");
        }
        return false;
    }

    const auto thunkAddress = reinterpret_cast<std::uintptr_t>(&postEventDiagnosticThunk);
    const auto branchIsland = trampoline.allocate_branch5(thunkAddress);
    std::vector<std::array<std::uint8_t, 5>> patches;
    patches.reserve(callsites.size());
    for (const auto& callsite : callsites) {
        const auto patch = sds::buildRel32CallPatch(callsite.instructionAddress, branchIsland);
        if (!patch) {
            if (_impl->log) {
                _impl->log("Starfield audio capture: branch island outside rel32 range; diagnostic disabled");
            }
            return false;
        }
        patches.push_back(*patch);
    }

    g_originalPostEvent.store(reinterpret_cast<PostEventFunction>(target), std::memory_order_release);
    g_starfieldModuleBase.store(moduleBase, std::memory_order_release);

    std::size_t patched = 0;
    for (std::size_t i = 0; i < callsites.size(); ++i) {
        if (!REL::WriteSafe(
                callsites[i].instructionAddress,
                patches[i].data(),
                patches[i].size())) {
            break;
        }
        ++patched;
    }

    _impl->patchedCallsites = patched;
    if (patched == 0) {
        g_originalPostEvent.store(nullptr, std::memory_order_release);
        if (_impl->log) {
            _impl->log("Starfield audio capture: PostEvent callsite patching produced no verified hooks; diagnostic disabled");
        }
        return false;
    }

    g_captureObserver.store(_impl.get(), std::memory_order_release);
    _impl->started.store(true, std::memory_order_release);

    if (_impl->log) {
        char message[256]{};
        std::snprintf(
            message,
            sizeof(message),
            "Starfield audio capture: ACTIVE exact Wwise external-source VO diagnostic postEvent=0x%llX directCallsites=%llu patched=%llu; original game audio untouched",
            static_cast<unsigned long long>(target),
            static_cast<unsigned long long>(callsites.size()),
            static_cast<unsigned long long>(patched));
        _impl->log(message);
    }
    return true;
}

void sds::StarfieldAudioCapture::stop() noexcept
{
    if (!_impl) {
        return;
    }
    _impl->weaponSfxDiscoveryArmed.store(false, std::memory_order_release);
    _impl->shipWeaponObservationArmed.store(false, std::memory_order_release);
    _impl->uiAudioDiscoveryArmed.store(false, std::memory_order_release);
    _impl->uiAudioPlaybackArmed.store(false, std::memory_order_release);
    _impl->musicReconArmed.store(false, std::memory_order_release);
    g_musicSelectionCallbackArmed.store(false, std::memory_order_release);
    _impl->started.store(false, std::memory_order_release);
    void* expected = _impl.get();
    (void)g_captureObserver.compare_exchange_strong(
        expected,
        nullptr,
        std::memory_order_acq_rel,
        std::memory_order_acquire);
}

void sds::StarfieldAudioCapture::drainDiagnostics()
{
    if (!_impl) {
        return;
    }

    AudioPostDiagnostic record{};
    while (_impl->records.tryPop(record)) {
        if (_impl->log) {
            _impl->log(formatDiagnostic(record));
        }

        const RemoteVoMirrorCandidate candidate{
            .eventId = record.eventId,
            .externalCookie = record.externalCookie,
            .externalCount = record.externalCount,
            .codecId = record.codecId,
            .fileId = record.fileId,
            .memorySize = record.memorySize,
            .hasMemory = record.hasMemory,
            .hasFilePath = record.hasFilePath,
            .dialogueMenuActive = record.dialogueMenuActive,
            .filePathLength = record.filePathLength,
            .originalPlayingId = record.returnedPlayingId,
        };

        if (_impl->sourceProbe && qualifiesRemoteCommsVoCandidate(candidate)) {
            const RemoteVoMirrorRequest readyRequest{
                .eventId = record.eventId,
                .externalCookie = record.externalCookie,
                .codecId = record.codecId,
                .fileId = record.fileId,
                .memorySize = record.memorySize,
                .filePath = std::wstring_view(record.filePath.data(), record.filePathLength),
                .sequence = record.sequence,
                .captureSteadyMicros = record.captureSteadyMicros,
                .originalPlayingId = record.returnedPlayingId,
            };
            if (_impl->log) {
                char message[224]{};
                std::snprintf(
                    message,
                    sizeof(message),
                    "Remote VO WEM source probe: QUALIFIED seq=%llu event=0x%X; continuous metadata/decode begins on normal runtime tick",
                    static_cast<unsigned long long>(record.sequence),
                    record.eventId);
                _impl->log(message);
            }
            try {
                _impl->sourceProbe(readyRequest);
            } catch (...) {
                if (_impl->log) {
                    _impl->log("Remote VO WEM source probe: callback exception; no Wwise replay attempted");
                }
            }
        }

        if (_impl->mirror) {
            if (_impl->mirrorGate.tryClaim(candidate)) {
                const RemoteVoMirrorRequest request{
                    .eventId = record.eventId,
                    .externalCookie = record.externalCookie,
                    .codecId = record.codecId,
                    .fileId = record.fileId,
                    .memorySize = record.memorySize,
                    .filePath = std::wstring_view(record.filePath.data(), record.filePathLength),
                };
                const bool scheduled = _impl->mirrorDelay.schedule(
                    request,
                    DelayedRemoteVoMirrorDispatch::Clock::now());
                if (_impl->log) {
                    char message[224]{};
                    std::snprintf(
                        message,
                        sizeof(message),
                        scheduled
                            ? "Starfield audio mirror: one-shot CLAIM seq=%llu event=0x%X; delayed SCHEDULE delayMs=750 off the submission hook"
                            : "Starfield audio mirror: one-shot CLAIM seq=%llu event=0x%X; delayed SCHEDULE failed; mirror remains disarmed",
                        static_cast<unsigned long long>(record.sequence),
                        record.eventId);
                    _impl->log(message);
                }
            }
        }
    }

    WeaponSfxPostRecord weaponRecord{};
    while (_impl->weaponSfxRecords.tryPop(weaponRecord)) {
        if (!_impl->weaponSfxObservation) {
            continue;
        }
        const WeaponSfxWwiseObservation observation{
            .sequence = weaponRecord.sequence,
            .when = std::chrono::steady_clock::time_point{
                std::chrono::microseconds{ weaponRecord.captureSteadyMicros } },
            .threadId = weaponRecord.threadId,
            .callsiteRva = weaponRecord.callsiteRva,
            .eventId = weaponRecord.eventId,
            .gameObjectId = weaponRecord.gameObjectId,
            .flags = weaponRecord.flags,
            .externalCount = weaponRecord.externalCount,
            .hasExternalSources = weaponRecord.hasExternalSources,
            .requestedPlayingId = weaponRecord.requestedPlayingId,
            .returnedPlayingId = weaponRecord.returnedPlayingId,
        };
        try {
            _impl->weaponSfxObservation(observation);
        } catch (...) {
            if (_impl->log) {
                _impl->log("Weapon SFX probe: observation callback exception; diagnostic record dropped");
            }
        }
    }

    UiAudioPostRecord uiRecord{};
    while (_impl->uiAudioRecords.tryPop(uiRecord)) {
        if (!_impl->uiAudioObservation) {
            continue;
        }
        const UiAudioWwiseObservation observation{
            .sequence = uiRecord.sequence,
            .when = std::chrono::steady_clock::time_point{
                std::chrono::microseconds{ uiRecord.captureSteadyMicros } },
            .threadId = uiRecord.threadId,
            .callsiteRva = uiRecord.callsiteRva,
            .eventId = uiRecord.eventId,
            .gameObjectId = uiRecord.gameObjectId,
            .flags = uiRecord.flags,
            .externalCount = uiRecord.externalCount,
            .hasExternalSources = uiRecord.hasExternalSources,
            .requestedPlayingId = uiRecord.requestedPlayingId,
            .returnedPlayingId = uiRecord.returnedPlayingId,
        };
        try {
            _impl->uiAudioObservation(observation);
        } catch (...) {
            if (_impl->log) {
                _impl->log("UI audio discovery: observation callback exception; diagnostic record dropped");
            }
        }
    }

    MusicReconPostRecord musicReconRecord{};
    while (_impl->musicReconRecords.tryPop(musicReconRecord)) {
        if (!_impl->musicReconObservation) {
            continue;
        }
        const MusicReconWwiseObservation observation{
            .sequence = musicReconRecord.sequence,
            .when = std::chrono::steady_clock::time_point{
                std::chrono::microseconds{ musicReconRecord.captureSteadyMicros } },
            .threadId = musicReconRecord.threadId,
            .callsiteRva = musicReconRecord.callsiteRva,
            .eventId = musicReconRecord.eventId,
            .gameObjectId = musicReconRecord.gameObjectId,
            .flags = musicReconRecord.flags,
            .externalCount = musicReconRecord.externalCount,
            .hasExternalSources = musicReconRecord.externalCount != 0,
            .requestedPlayingId = musicReconRecord.requestedPlayingId,
            .returnedPlayingId = musicReconRecord.returnedPlayingId,
        };
        try {
            _impl->musicReconObservation(observation);
        } catch (...) {
            if (_impl->log) {
                _impl->log("Music recon: observation callback exception; diagnostic record dropped");
            }
        }
    }

    MusicSelectionRecord selectionRecord{};
    while (g_musicSelectionRecords.tryPop(selectionRecord)) {
        const auto capturedAt = std::chrono::steady_clock::time_point{
            std::chrono::microseconds{ selectionRecord.captureSteadyMicros } };
        if (_impl->musicSelectionObservation) {
            try {
                if (selectionRecord.kind == MusicSelectionRecordKind::kDuration) {
                    const WwiseDurationCallbackInfo durationInfo{
                        .gameObjectId = selectionRecord.gameObjectId,
                        .playingId = selectionRecord.playingId,
                        .eventId = selectionRecord.eventId,
                        .durationMs = selectionRecord.durationMs,
                        .estimatedDurationMs = selectionRecord.estimatedDurationMs,
                        .audioNodeId = selectionRecord.audioNodeId,
                        .mediaId = selectionRecord.mediaId,
                        .streaming = selectionRecord.streaming,
                    };
                    _impl->musicSelectionObservation(
                        makeMusicSelectionSelectedObservation(durationInfo, capturedAt));
                } else if (selectionRecord.kind == MusicSelectionRecordKind::kEnd) {
                    const WwiseEventCallbackInfo eventInfo{
                        .gameObjectId = selectionRecord.gameObjectId,
                        .playingId = selectionRecord.playingId,
                        .eventId = selectionRecord.eventId,
                    };
                    _impl->musicSelectionObservation(
                        makeMusicSelectionEndedObservation(eventInfo, capturedAt));
                }
            } catch (...) {
                if (_impl->log) {
                    _impl->log("Music selection: production observation callback exception; record dropped");
                }
            }
        }

        if (!_impl->log || !_impl->musicReconArmed.load(std::memory_order_acquire)) {
            continue;
        }
        char message[512]{};
        if (selectionRecord.kind == MusicSelectionRecordKind::kPost) {
            std::snprintf(
                message,
                sizeof(message),
                "Music selection recon: POST event=0x%X object=0x%llX returnedPlayingId=%u requestedPlayingId=%u originalFlags=0x%X effectiveFlags=0x%X externals=%u hadCallback=%s hadCookie=%s injectedDurationCallback=%s chainedExistingCallback=%s chainContextUnavailable=%s chainPlayingIdMismatch=%s",
                selectionRecord.eventId,
                static_cast<unsigned long long>(selectionRecord.gameObjectId),
                selectionRecord.playingId,
                selectionRecord.requestedPlayingId,
                selectionRecord.originalFlags,
                selectionRecord.effectiveFlags,
                selectionRecord.externalCount,
                selectionRecord.hadCallback ? "yes" : "no",
                selectionRecord.hadCookie ? "yes" : "no",
                selectionRecord.injectedDurationCallback ? "yes" : "no",
                selectionRecord.chainedExistingCallback ? "yes" : "no",
                selectionRecord.chainContextUnavailable ? "yes" : "no",
                selectionRecord.chainPlayingIdMismatch ? "yes" : "no");
        } else if (selectionRecord.kind == MusicSelectionRecordKind::kDuration) {
            std::snprintf(
                message,
                sizeof(message),
                "Music selection recon: SELECTED event=0x%X playingId=%u object=0x%llX mediaId=%u audioNodeId=%u durationMs=%.3f estimatedDurationMs=%.3f streaming=%s",
                selectionRecord.eventId,
                selectionRecord.playingId,
                static_cast<unsigned long long>(selectionRecord.gameObjectId),
                selectionRecord.mediaId,
                selectionRecord.audioNodeId,
                static_cast<double>(selectionRecord.durationMs),
                static_cast<double>(selectionRecord.estimatedDurationMs),
                selectionRecord.streaming ? "yes" : "no");
        } else {
            std::snprintf(
                message,
                sizeof(message),
                "Music selection recon: ENDED event=0x%X playingId=%u object=0x%llX",
                selectionRecord.eventId,
                selectionRecord.playingId,
                static_cast<unsigned long long>(selectionRecord.gameObjectId));
        }
        _impl->log(message);
    }

    const auto selectionDropped = g_musicSelectionDropped.exchange(0, std::memory_order_acq_rel);
    if (selectionDropped != 0 && _impl->log &&
        _impl->musicReconArmed.load(std::memory_order_acquire)) {
        char message[192]{};
        std::snprintf(
            message,
            sizeof(message),
            "Music selection recon: deferred callback queue dropped %llu records",
            static_cast<unsigned long long>(selectionDropped));
        _impl->log(message);
    }

    if (_impl->mirror) {
        if (auto ready = _impl->mirrorDelay.takeReady(DelayedRemoteVoMirrorDispatch::Clock::now())) {
            const auto readyRequest = ready->view();
            if (_impl->log) {
                char message[192]{};
                std::snprintf(
                    message,
                    sizeof(message),
                    "Starfield audio mirror: delayed DISPATCH delayMs=750 event=0x%X; Wwise repost begins on normal runtime tick",
                    readyRequest.eventId);
                _impl->log(message);
            }
            try {
                (void)_impl->mirror(readyRequest);
            } catch (...) {
                if (_impl->log) {
                    _impl->log("Starfield audio mirror: delayed callback exception; mirror remains disarmed for this launch");
                }
            }
        }
    }

    const auto dropped = _impl->dropped.exchange(0, std::memory_order_acq_rel);
    if (dropped != 0 && _impl->log) {
        char message[160]{};
        std::snprintf(
            message,
            sizeof(message),
            "Starfield audio capture: deferred diagnostic queue dropped %llu VO records",
            static_cast<unsigned long long>(dropped));
        _impl->log(message);
    }
}

void sds::StarfieldAudioCapture::setDialogueMenuActive(bool active) noexcept
{
    if (_impl) {
        _impl->dialogueMenuActive.store(active, std::memory_order_release);
    }
}

void sds::StarfieldAudioCapture::setWeaponSfxDiscoveryArmed(bool armed) noexcept
{
    if (_impl) {
        _impl->weaponSfxDiscoveryArmed.store(armed, std::memory_order_release);
    }
}

void sds::StarfieldAudioCapture::setShipWeaponObservationArmed(bool armed) noexcept
{
    if (_impl) {
        _impl->shipWeaponObservationArmed.store(armed, std::memory_order_release);
    }
}

std::uint64_t sds::StarfieldAudioCapture::takeWeaponSfxDropped() noexcept
{
    return _impl ? _impl->weaponSfxDropped.exchange(0, std::memory_order_acq_rel) : 0;
}

void sds::StarfieldAudioCapture::setUiAudioDiscoveryArmed(bool armed) noexcept
{
    if (_impl) {
        _impl->uiAudioDiscoveryArmed.store(armed, std::memory_order_release);
    }
}

void sds::StarfieldAudioCapture::setUiAudioPlaybackArmed(bool armed) noexcept
{
    if (_impl) {
        _impl->uiAudioPlaybackArmed.store(armed, std::memory_order_release);
    }
}

std::uint64_t sds::StarfieldAudioCapture::takeUiAudioDropped() noexcept
{
    return _impl ? _impl->uiAudioDropped.exchange(0, std::memory_order_acq_rel) : 0;
}

void sds::StarfieldAudioCapture::setMusicReconArmed(bool armed) noexcept
{
    if (_impl) {
        _impl->musicReconArmed.store(armed, std::memory_order_release);
    }
}

void sds::StarfieldAudioCapture::setMusicSelectionArmed(bool armed) noexcept
{
    if (_impl) {
        g_musicSelectionCallbackArmed.store(armed, std::memory_order_release);
    }
}

std::uint64_t sds::StarfieldAudioCapture::takeMusicReconDropped() noexcept
{
    return _impl ? _impl->musicReconDropped.exchange(0, std::memory_order_acq_rel) : 0;
}

bool sds::StarfieldAudioCapture::active() const noexcept
{
    return _impl && _impl->started.load(std::memory_order_acquire);
}
