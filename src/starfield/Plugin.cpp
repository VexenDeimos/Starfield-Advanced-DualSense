#include <StarfieldDualSense/BoostpackFeedbackAuthority.h>

#include <REL/Relocation.h>
#include <StarfieldDualSense/BoostpackSpeakerPreparedCache.h>
#include <StarfieldDualSense/BoostpackSpeakerPlayback.h>
#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/SettingsMenu.h>
#include <StarfieldDualSense/ImmediateLiveSettings.h>
#include <StarfieldDualSense/ControllerLiveSettings.h>
#include <StarfieldDualSense/ControllerSpeakerLiveSettings.h>
#include <StarfieldDualSense/GameplayHapticsLiveSettings.h>
#include <StarfieldDualSense/AutoRivetChargeSemantics.h>
#include <StarfieldDualSense/ControllerManager.h>
#include <StarfieldDualSense/DualSenseAudioTransport.h>
#include <StarfieldDualSense/DualSenseAudioHapticsClient.h>
#include <StarfieldDualSense/DualSenseAudioSpeakerClient.h>
#include <StarfieldDualSense/ControllerSpeakerManager.h>
#include <StarfieldDualSense/WeaponSpeakerPlayback.h>
#include <StarfieldDualSense/WeaponSpeakerPreparedCache.h>
#include <StarfieldDualSense/WeaponAudioPipeline.h>
#include <StarfieldDualSense/WeaponSpeakerProfile.h>
#include <StarfieldDualSense/WeaponSfxDiscoveryProbe.h>
#include <StarfieldDualSense/RemoteVoSpeakerPlayback.h>
#include <StarfieldDualSense/RemoteVoOutputPolicy.h>
#include <StarfieldDualSense/GameStateAdapter.h>
#include <StarfieldDualSense/LandVehicleActionGate.h>
#include <StarfieldDualSense/LandVehicleWwiseReconAggregator.h>
#include <StarfieldDualSense/FireMarkerBridge.h>
#include <StarfieldDualSense/HidWriteTrace.h>
#include <StarfieldDualSense/HapticsManager.h>
#include <StarfieldDualSense/MusicReconProbe.h>
#include <StarfieldDualSense/RuntimeEventRouter.h>
#include <StarfieldDualSense/NativeUsbBackend.h>
#include <StarfieldDualSense/StarfieldAudioCapture.h>
#include <StarfieldDualSense/ShipBallisticFireGate.h>
#include <StarfieldDualSense/ShipLaserReconProbe.h>
#include <StarfieldDualSense/ShipLaserFireGate.h>
#include <StarfieldDualSense/ShipParticleFireGate.h>
#include <StarfieldDualSense/ShipMissileFireGate.h>
#include <StarfieldDualSense/ShipEMFireGate.h>
#include <StarfieldDualSense/ShipLaunchLandingReconProbe.h>
#include <StarfieldDualSense/ShipWeaponSemanticCatalog.h>
#include <StarfieldDualSense/UiAudioDiscoveryProbe.h>
#include <StarfieldDualSense/UiAudioCandidateCatalog.h>
#include <StarfieldDualSense/UiSpeakerPreparedCache.h>
#include <StarfieldDualSense/UiSpeakerPlayback.h>
#include <StarfieldDualSense/WwiseApiRecon.h>
#include <StarfieldDualSense/WwiseWemSourceProbe.h>
#include <StarfieldDualSense/WwiseRemoteVoFilesystemProbe.h>
#include <StarfieldDualSense/WwiseVoiceArchiveManifestProbe.h>
#include <StarfieldDualSense/WwiseVoiceBa2IndexProbe.h>
#include <StarfieldDualSense/WwiseWemPayloadProbe.h>
#include <StarfieldDualSense/WwiseWemStructureProbe.h>
#include <StarfieldDualSense/WwiseWemVorbisPacketProbe.h>
#include <StarfieldDualSense/WwiseWemVorbisDecode.h>
#include <StarfieldDualSense/WwisePlayingIdStop.h>

#include <RE/Starfield.h>
#include <SFSE/SFSE.h>

#include <chrono>
#include <algorithm>
#include <array>
#include <atomic>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <Windows.h>

namespace
{
    constexpr std::string_view kVersion = "0.3.90";
    constexpr std::uint32_t kShipWeaponCaptureProbeLimit = 256;
    constexpr std::uint32_t kShipEmReconLogLimit = 512;
    constexpr std::uint32_t kLandVehicleRareWwiseLogLimit = 1024;
    constexpr std::uint64_t kMusicHapticsSampleRate = 48000u;
    constexpr std::size_t kMusicHapticsAuthoritySlots = 128u;
    constexpr auto kShipTakeoffHapticsMax = std::chrono::seconds(30);
    constexpr const char* kConfigPath = "Data/SFSE/Plugins/StarfieldDualSense.toml";
    constexpr std::array<std::string_view, 0> kWeaponSfxDiscoveryTargets{};

    enum class ShipLaunchLandingRumblePhase : std::uint8_t
    {
        None,
        Takeoff,
        Landing,
    };
    std::unique_ptr<sds::ControllerManager> g_controller;
    std::shared_ptr<sds::DualSenseAudioTransport> g_audioTransport;
    std::unique_ptr<sds::HapticsManager> g_haptics;
    std::unique_ptr<sds::ControllerSpeakerManager> g_speakerManager;
    std::shared_ptr<sds::WeaponSpeakerPreparedCache> g_weaponSpeakerPreparedCache;
    std::unique_ptr<sds::WeaponSpeakerPlayback> g_weaponSpeakerPlayback;
    std::unique_ptr<sds::WeaponSfxDiscoveryProbe> g_weaponSfxDiscovery;
    std::unique_ptr<sds::WeaponAudioPipeline> g_weaponAudioPipeline;
    std::unique_ptr<sds::WeaponAudioPipeline> g_mainMenuUiAudioPipeline;
    std::unique_ptr<sds::RuntimeEventRouter> g_eventRouter;
    std::unique_ptr<sds::GameStateAdapter> g_gameState;
    std::unique_ptr<sds::FireMarkerBridge> g_fireMarkerBridge;
    std::unique_ptr<sds::StarfieldAudioCapture> g_audioCapture;
    std::unique_ptr<sds::MusicReconProbe> g_musicRecon;
    std::shared_ptr<sds::ShipWeaponSemanticCache> g_shipWeaponSemanticCache;
    sds::ShipBallisticFireGate g_shipBallisticFireGate;
    sds::ShipLaserFireGate g_shipLaserFireGate;
    sds::ShipParticleFireGate g_shipParticleFireGate;
    sds::ShipMissileFireGate g_shipMissileFireGate;
    sds::ShipEMFireGate g_shipEMFireGate;
    sds::ShipLaserReconProbe g_shipEmReconProbe;
    sds::ShipLaunchLandingReconProbe g_shipLaunchLandingReconProbe;
    std::mutex g_boostpackFeedbackMutex{};
    std::unique_ptr<sds::BoostpackFeedbackAuthority> g_boostpackFeedbackAuthority;
    std::shared_ptr<sds::BoostpackSpeakerPreparedCache> g_boostpackSpeakerPreparedCache;
    std::unique_ptr<sds::BoostpackSpeakerPlayback> g_boostpackSpeakerPlayback;
    std::atomic_uint8_t g_boostpackProductionBlockingMenuMask{ 0 };
    std::atomic_bool g_boostpackProductionLandVehicleActive{ false };
    std::atomic_bool g_boostpackProductionControllerConnected{ false };
    std::atomic_bool g_boostpackProductionContextEligible{ false };
    bool boostpackProductionCaptureArmed() noexcept;

    std::uintptr_t g_nativeDualSenseReselectionHandlerAddress = 0;
    std::atomic_bool g_nativeDualSenseReconnectFixEnabled{ true };
    bool g_nativeDualSenseReselectionStartedFromDualSense = false;
    bool g_nativeDualSenseReselectionSawDisconnect = false;
    bool g_nativeDualSenseReselectionPending = false;
    bool g_nativeDualSenseReselectionAttemptedThisCycle = false;
    std::uint64_t g_nativeDualSenseReselectionCycle = 0;
    std::chrono::steady_clock::time_point
        g_nativeDualSenseReselectionDue{};
    std::chrono::steady_clock::time_point
        g_nativeDualSenseReselectionDiscoveryDue{};

    std::unique_ptr<sds::UiAudioDiscoveryProbe> g_uiAudioDiscovery;
    std::shared_ptr<sds::UiSpeakerPreparedCache> g_uiSpeakerPreparedCache;
    std::unique_ptr<sds::UiSpeakerPlayback> g_uiSpeakerPlayback;
    std::chrono::steady_clock::time_point g_remoteVoPlaybackDeadline{};
    bool g_runtimeInitialized = false;
    std::optional<sds::Config> g_startupConfig{};
    std::atomic<bool> g_autoRivetChargeCaptureEnabled{ false };
    std::atomic<bool> g_autoRivetChargeCaptureArmed{ false };
    std::atomic_bool g_shipPilotActive{ false };
    std::atomic<std::uint8_t> g_shipBallisticBlockingMenuMask{ 0 };
    std::atomic<std::uint8_t> g_musicHapticsBlockingMenuMask{ 0 };
    std::atomic_bool g_musicHapticsEnabled{ false };
    std::atomic<float> g_musicHapticsStrength{ 1.0F };
    std::atomic<float> g_musicHapticsUserScale{ 1.0F };
    void musicHapticsClearAuthority() noexcept;

    void applyImmediateLiveSettings(const sds::Config& config) noexcept
    {
        g_nativeDualSenseReconnectFixEnabled.store(
            config.operatingMode == sds::OperatingMode::ReconnectFixOnly ||
                config.dualSenseReconnectFix,
            std::memory_order_release);

        const auto live = sds::immediateLiveSettings(config);

        const bool wasMusicEnabled =
            g_musicHapticsEnabled.exchange(live.musicHapticsEnabled, std::memory_order_acq_rel);
        g_musicHapticsStrength.store(live.musicHapticsBaseStrength, std::memory_order_release);
        g_musicHapticsUserScale.store(live.musicHapticsUserScale, std::memory_order_release);

        if (wasMusicEnabled && !live.musicHapticsEnabled) {
            musicHapticsClearAuthority();
        }

        if (g_audioTransport) {
            g_audioTransport->setSpeakerVolume(live.speakerVolume);
        }

        if (g_controller) {
            g_controller->applyLiveSettings(sds::controllerLiveSettings(config));
        }

        const auto speakerLive = sds::controllerSpeakerLiveSettings(config);
        if (speakerLive.controllerSpeaker) {
            // Queue the native route first. The controller worker owns the
            // physical backend mutation; speaker playback then enables against
            // the already-requested route.
            if (g_controller) {
                g_controller->setControllerSpeakerRoutingEnabled(true);
            }
            if (g_speakerManager) {
                g_speakerManager->applyLiveSettings(speakerLive);
            }
        } else {
            // Fail closed: stop/clear controller-speaker playback before the
            // controller worker releases the native speaker route.
            if (g_speakerManager) {
                g_speakerManager->applyLiveSettings(speakerLive);
            }
            if (g_controller) {
                g_controller->setControllerSpeakerRoutingEnabled(false);
            }
        }

        if (g_haptics) {
            // Keep exact Auto-Rivet charge semantics available while gameplay
            // haptic output is muted so a later live re-enable has current state.
            g_autoRivetChargeCaptureEnabled.store(true, std::memory_order_release);
            g_haptics->applyLiveSettings(
                sds::gameplayHapticsLiveSettings(config));
        }
    }
    std::array<std::atomic<std::uint32_t>, kMusicHapticsAuthoritySlots> g_musicHapticsPlayingIds{};
    std::atomic<std::uint32_t> g_shipWeaponCaptureProbeCount{ 0 };
    std::atomic<std::uint32_t> g_shipEmReconLogCount{ 0 };
    std::atomic<std::uint32_t> g_landVehicleRareWwiseLogCount{ 0 };
    sds::LandVehicleWwiseReconAggregator g_landVehicleWwiseAggregator{};
    std::mutex g_landVehicleWwiseAggregatorMutex{};
    sds::LandVehicleActionGate g_landVehicleActionGate{};
    std::mutex g_landVehicleActionGateMutex{};
    std::atomic_bool g_shipEmReconEnabled{ false };
    std::atomic_bool g_shipLaunchLandingReconEnabled{ false };
    std::atomic_bool g_onFootRefreshPending{ false };
    std::atomic_bool g_lastShipStateValid{ false };
    std::atomic_bool g_lastShipLanded{ false };
    std::atomic_bool g_lastShipDocked{ false };
    std::atomic<ShipLaunchLandingRumblePhase> g_shipLaunchLandingRumblePhase{ ShipLaunchLandingRumblePhase::None };
    std::atomic_bool g_shipTakeoffFirstFaderClosed{ false };
    std::atomic_bool g_shipTakeoffBoundaryObserved{ false };
    std::atomic<std::int64_t> g_shipLaunchLandingStartedAtUs{ 0 };
    bool g_runtimeShutdown = false;
    bool g_startupWeaponBootstrapComplete = false;
    bool g_weaponAudioPipelineEnabled = false;
    std::filesystem::path g_weaponAudioPipelineDataPath{};

    void pluginLog(std::string_view message) noexcept
    {
        try {
            REX::INFO("{}", message);
        } catch (...) {
            // Logging cannot be allowed to escape into SFSE callbacks.
        }
    }


    enum class GamepadProbeKind : std::uint8_t
    {
        Other,
        DualSense,
        GenericGamepad,
        GamepadHandler,
        BaseGamepad,
    };

    struct GamepadProbeVtables
    {
        std::uintptr_t dualSense{ 0 };
        std::uintptr_t genericGamepad{ 0 };
        std::uintptr_t gamepadHandler{ 0 };
        std::uintptr_t baseGamepad{ 0 };
        std::array<std::uintptr_t, 3> inputManagers{};
    };

    std::uintptr_t resolveGamepadProbeId(std::uint64_t id) noexcept
    {
        try {
            REL::Relocation<std::uintptr_t> relocation{ REL::ID(id) };
            return relocation.address();
        } catch (...) {
            return 0;
        }
    }

    GamepadProbeVtables gamepadProbeVtables() noexcept
    {
        return {
            .dualSense = resolveGamepadProbeId(1037215),
            .genericGamepad = resolveGamepadProbeId(470133),
            .gamepadHandler = resolveGamepadProbeId(470111),
            .baseGamepad = resolveGamepadProbeId(470102),
            .inputManagers = {
                resolveGamepadProbeId(469745),
                resolveGamepadProbeId(469743),
                resolveGamepadProbeId(469747),
            },
        };
    }

    GamepadProbeKind classifyGamepadProbeVtable(
        std::uintptr_t vtable,
        const GamepadProbeVtables& known) noexcept
    {
        if (vtable != 0 && vtable == known.dualSense) {
            return GamepadProbeKind::DualSense;
        }
        if (vtable != 0 && vtable == known.genericGamepad) {
            return GamepadProbeKind::GenericGamepad;
        }
        if (vtable != 0 && vtable == known.gamepadHandler) {
            return GamepadProbeKind::GamepadHandler;
        }
        if (vtable != 0 && vtable == known.baseGamepad) {
            return GamepadProbeKind::BaseGamepad;
        }
        return GamepadProbeKind::Other;
    }

    std::string_view gamepadProbeKindName(GamepadProbeKind kind) noexcept
    {
        switch (kind) {
        case GamepadProbeKind::DualSense:
            return "DualSense";
        case GamepadProbeKind::GenericGamepad:
            return "GenericGamepad";
        case GamepadProbeKind::GamepadHandler:
            return "GamepadHandler";
        case GamepadProbeKind::BaseGamepad:
            return "BaseGamepad";
        default:
            return "Other";
        }
    }

    constexpr std::uintptr_t kNativeGamepadSelectorRva = 0x22FE670u;
    constexpr std::uint32_t kNativeDualSenseReselectionSettleMs = 250;
    constexpr std::uint32_t kNativeDualSenseDiscoveryRetryMs = 1000;

    bool nativeGamepadSelectorSignatureMatches(
        std::uintptr_t selectorAddress) noexcept
    {
        // Task 5I/5M verified Starfield 1.16.244.0 begins the selector with:
        // 48 89 4C 24 08 53 55 56 57 41 54 41 55 41 56 41 57 48 81 EC B8 00 00 00
        constexpr std::uint8_t expected[] = {
            0x48, 0x89, 0x4C, 0x24, 0x08,
            0x53, 0x55, 0x56, 0x57,
            0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x41, 0x57,
            0x48, 0x81, 0xEC, 0xB8, 0x00, 0x00, 0x00
        };

        std::uint8_t actual[sizeof(expected)]{};
        SIZE_T bytesRead = 0;

        if (!ReadProcessMemory(
                GetCurrentProcess(),
                reinterpret_cast<const void*>(selectorAddress),
                actual,
                sizeof(actual),
                &bytesRead) ||
            bytesRead != sizeof(actual)) {
            return false;
        }

        for (std::size_t i = 0; i < sizeof(expected); ++i) {
            if (actual[i] != expected[i]) {
                return false;
            }
        }

        return true;
    }

    bool readGamepadProbePointer(
        std::uintptr_t address,
        std::uintptr_t& value) noexcept;

    bool isInputManagerVtable(
        std::uintptr_t vtable,
        const GamepadProbeVtables& known) noexcept;

    bool discoverNativeDualSenseReselectionHandler() noexcept
    {
        const auto known = gamepadProbeVtables();
        if (known.dualSense == 0 ||
            known.gamepadHandler == 0) {
            return false;
        }

        const auto module =
            reinterpret_cast<std::uintptr_t>(
                GetModuleHandleW(nullptr));
        if (module == 0) {
            return false;
        }

        const auto* dos =
            reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
        if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) {
            return false;
        }

        const auto* nt =
            reinterpret_cast<const IMAGE_NT_HEADERS64*>(
                module +
                static_cast<std::uintptr_t>(
                    dos->e_lfanew));
        if (!nt || nt->Signature != IMAGE_NT_SIGNATURE) {
            return false;
        }

        const auto* section = IMAGE_FIRST_SECTION(nt);

        for (unsigned i = 0;
             i < nt->FileHeader.NumberOfSections;
             ++i) {
            const auto characteristics =
                section[i].Characteristics;

            if ((characteristics & IMAGE_SCN_MEM_WRITE) == 0 ||
                (characteristics & IMAGE_SCN_MEM_DISCARDABLE) != 0) {
                continue;
            }

            const auto start =
                module + section[i].VirtualAddress;
            const auto size =
                static_cast<std::size_t>(
                    section[i].Misc.VirtualSize != 0
                        ? section[i].Misc.VirtualSize
                        : section[i].SizeOfRawData);
            const auto end = start + size;

            auto managerAddress =
                (start + 7u) &
                ~static_cast<std::uintptr_t>(7u);

            for (;
                 managerAddress +
                         sizeof(std::uintptr_t) <= end;
                 managerAddress +=
                     sizeof(std::uintptr_t)) {
                std::uintptr_t managerVtable = 0;
                if (!readGamepadProbePointer(
                        managerAddress,
                        managerVtable) ||
                    !isInputManagerVtable(
                        managerVtable,
                        known)) {
                    continue;
                }

                std::uintptr_t handlerAddress = 0;
                if (!readGamepadProbePointer(
                        managerAddress + 0x78u,
                        handlerAddress) ||
                    handlerAddress < 0x10000u) {
                    continue;
                }

                std::uintptr_t handlerVtable = 0;
                if (!readGamepadProbePointer(
                        handlerAddress,
                        handlerVtable) ||
                    handlerVtable !=
                        known.gamepadHandler) {
                    continue;
                }

                std::uintptr_t delegateAddress = 0;
                if (!readGamepadProbePointer(
                        handlerAddress + 0xC0u,
                        delegateAddress) ||
                    delegateAddress < 0x10000u) {
                    continue;
                }

                std::uintptr_t delegateVtable = 0;
                if (!readGamepadProbePointer(
                        delegateAddress,
                        delegateVtable) ||
                    delegateVtable != known.dualSense) {
                    continue;
                }

                g_nativeDualSenseReselectionHandlerAddress =
                    handlerAddress;
                g_nativeDualSenseReselectionStartedFromDualSense =
                    true;

                std::ostringstream line;
                line << "Native DualSense reconnect:"
                     << " armed=yes"
                     << " source=BSInputDeviceManager+0x78"
                     << " handler=0x"
                     << std::hex << std::uppercase
                     << handlerAddress
                     << " initialDelegate=0x"
                     << delegateAddress
                     << std::dec
                     << " policy=one-attempt-per-physical-reconnect"
                     << " settleMs=250";
                pluginLog(line.str());
                return true;
            }
        }

        return false;
    }

    void updateNativeDualSenseReselection(
        std::chrono::steady_clock::time_point now) noexcept
    {
        if (!g_nativeDualSenseReconnectFixEnabled.load(std::memory_order_acquire)) {
            g_nativeDualSenseReselectionSawDisconnect = false;
            g_nativeDualSenseReselectionPending = false;
            g_nativeDualSenseReselectionAttemptedThisCycle = false;
            return;
        }

        if (!g_controller) {
            return;
        }

        const bool connected =
            g_controller->connected();

        if (!g_nativeDualSenseReselectionStartedFromDualSense) {
            if (!connected ||
                now <
                    g_nativeDualSenseReselectionDiscoveryDue) {
                return;
            }

            g_nativeDualSenseReselectionDiscoveryDue =
                now + std::chrono::milliseconds(
                    kNativeDualSenseDiscoveryRetryMs);

            if (!discoverNativeDualSenseReselectionHandler()) {
                return;
            }
        }

        if (g_nativeDualSenseReselectionHandlerAddress == 0) {
            return;
        }

        if (!connected) {
            if (!g_nativeDualSenseReselectionSawDisconnect) {
                g_nativeDualSenseReselectionSawDisconnect = true;
                g_nativeDualSenseReselectionPending = false;
                g_nativeDualSenseReselectionAttemptedThisCycle = false;
                ++g_nativeDualSenseReselectionCycle;
            } else {
                g_nativeDualSenseReselectionPending = false;
            }
            return;
        }

        if (!g_nativeDualSenseReselectionSawDisconnect ||
            g_nativeDualSenseReselectionAttemptedThisCycle) {
            return;
        }

        if (!g_nativeDualSenseReselectionPending) {
            g_nativeDualSenseReselectionPending = true;
            g_nativeDualSenseReselectionDue =
                now + std::chrono::milliseconds(
                    kNativeDualSenseReselectionSettleMs);

            std::ostringstream line;
            line << "Native DualSense reconnect:"
                 << " cycle="
                 << g_nativeDualSenseReselectionCycle
                 << " state=scheduled"
                 << " settleMs=250"
                 << " attempts=0/1";
            pluginLog(line.str());
            return;
        }

        if (now < g_nativeDualSenseReselectionDue) {
            return;
        }

        // One attempt for this physical disconnect/reconnect cycle. Consume
        // it before every guarded exit or native call so no failure can turn
        // into a retry loop.
        g_nativeDualSenseReselectionPending = false;
        g_nativeDualSenseReselectionAttemptedThisCycle = true;
        g_nativeDualSenseReselectionSawDisconnect = false;

        const auto handlerAddress =
            g_nativeDualSenseReselectionHandlerAddress;

        const auto known = gamepadProbeVtables();

        std::uintptr_t handlerVtable = 0;
        if (!readGamepadProbePointer(
                handlerAddress,
                handlerVtable) ||
            handlerVtable != known.gamepadHandler) {
            g_nativeDualSenseReselectionHandlerAddress = 0;
            g_nativeDualSenseReselectionStartedFromDualSense = false;

            std::ostringstream line;
            line << "Native DualSense reconnect:"
                 << " cycle="
                 << g_nativeDualSenseReselectionCycle
                 << " attempts=1/1"
                 << " result=skipped-invalid-handler";
            pluginLog(line.str());
            return;
        }

        std::uintptr_t beforeDelegate = 0;
        std::uintptr_t beforeVtable = 0;

        readGamepadProbePointer(
            handlerAddress + 0xC0u,
            beforeDelegate);
        if (beforeDelegate != 0) {
            readGamepadProbePointer(
                beforeDelegate,
                beforeVtable);
        }

        const auto beforeKind =
            classifyGamepadProbeVtable(
                beforeVtable,
                known);

        if (beforeKind !=
            GamepadProbeKind::GenericGamepad) {
            std::ostringstream line;
            line << "Native DualSense reconnect:"
                 << " cycle="
                 << g_nativeDualSenseReselectionCycle
                 << " before="
                 << gamepadProbeKindName(beforeKind)
                 << " after="
                 << gamepadProbeKindName(beforeKind)
                 << " attempts=1/1"
                 << " result=skipped-before-not-generic";
            pluginLog(line.str());
            return;
        }

        const auto starfieldBase =
            reinterpret_cast<std::uintptr_t>(
                GetModuleHandleW(nullptr));

        if (starfieldBase == 0) {
            std::ostringstream line;
            line << "Native DualSense reconnect:"
                 << " cycle="
                 << g_nativeDualSenseReselectionCycle
                 << " before=GenericGamepad"
                 << " after=GenericGamepad"
                 << " attempts=1/1"
                 << " result=skipped-no-starfield-base";
            pluginLog(line.str());
            return;
        }

        const auto selectorAddress =
            starfieldBase +
            kNativeGamepadSelectorRva;

        if (!nativeGamepadSelectorSignatureMatches(
                selectorAddress)) {
            std::ostringstream line;
            line << "Native DualSense reconnect:"
                 << " cycle="
                 << g_nativeDualSenseReselectionCycle
                 << " before=GenericGamepad"
                 << " after=GenericGamepad"
                 << " attempts=1/1"
                 << " result=selector-signature-mismatch";
            pluginLog(line.str());
            return;
        }

        using NativeGamepadSelector =
            void (*)(void*);
        const auto selector =
            reinterpret_cast<NativeGamepadSelector>(
                selectorAddress);

        selector(
            reinterpret_cast<void*>(
                handlerAddress));

        std::uintptr_t afterDelegate = 0;
        std::uintptr_t afterVtable = 0;

        readGamepadProbePointer(
            handlerAddress + 0xC0u,
            afterDelegate);
        if (afterDelegate != 0) {
            readGamepadProbePointer(
                afterDelegate,
                afterVtable);
        }

        const auto afterKind =
            classifyGamepadProbeVtable(
                afterVtable,
                known);

        std::ostringstream line;
        line << "Native DualSense reconnect:"
             << " cycle="
             << g_nativeDualSenseReselectionCycle
             << " before="
             << gamepadProbeKindName(beforeKind)
             << " after="
             << gamepadProbeKindName(afterKind)
             << " attempts=1/1"
             << " beforeDelegate=0x"
             << std::hex << std::uppercase
             << beforeDelegate
             << " afterDelegate=0x"
             << afterDelegate
             << std::dec
             << " result=native-selector-called";
        pluginLog(line.str());
    }

    bool isInputManagerVtable(
        std::uintptr_t vtable,
        const GamepadProbeVtables& known) noexcept
    {
        for (const auto candidate : known.inputManagers) {
            if (vtable != 0 && vtable == candidate) {
                return true;
            }
        }
        return false;
    }

    bool readGamepadProbePointer(
        std::uintptr_t address,
        std::uintptr_t& value) noexcept
    {
        value = 0;
        if (address < 0x10000u) {
            return false;
        }

        SIZE_T bytesRead = 0;
        return ReadProcessMemory(
                   GetCurrentProcess(),
                   reinterpret_cast<const void*>(address),
                   &value,
                   sizeof(value),
                   &bytesRead) != FALSE &&
            bytesRead == sizeof(value);
    }

    std::string_view eventIdentity(const sds::GameEvent& event) noexcept
    {
        const auto* begin = event.text.data();
        std::size_t length = 0;
        while (length < event.text.size() && begin[length] != '\0') {
            ++length;
        }
        return { begin, length };
    }

    std::int64_t steadyMicros(std::chrono::steady_clock::time_point when) noexcept
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(when.time_since_epoch()).count();
    }

    std::uint8_t musicHapticsMenuBit(std::string_view menu) noexcept
    {
        if (menu == "MainMenu") {
            return 0x01u;
        }
        if (menu == "DataMenu") {
            return 0x02u;
        }
        if (menu == "PauseMenu") {
            return 0x04u;
        }
        if (menu == "LoadingMenu") {
            return 0x08u;
        }
        if (menu == "FaderMenu") {
            return 0x10u;
        }
        return 0u;
    }

    bool musicHapticsBlocked() noexcept
    {
        return g_musicHapticsBlockingMenuMask.load(std::memory_order_acquire) != 0u;
    }

    bool musicHapticsPlayingIdActive(std::uint32_t playingId) noexcept
    {
        if (playingId == 0u) {
            return false;
        }
        for (const auto& slot : g_musicHapticsPlayingIds) {
            if (slot.load(std::memory_order_acquire) == playingId) {
                return true;
            }
        }
        return false;
    }

    bool musicHapticsMarkPlayingIdActive(std::uint32_t playingId) noexcept
    {
        if (playingId == 0u) {
            return false;
        }
        if (musicHapticsPlayingIdActive(playingId)) {
            return true;
        }
        for (auto& slot : g_musicHapticsPlayingIds) {
            std::uint32_t expected = 0u;
            if (slot.compare_exchange_strong(
                    expected, playingId, std::memory_order_acq_rel, std::memory_order_acquire)) {
                return true;
            }
            if (expected == playingId) {
                return true;
            }
        }
        return false;
    }

    void musicHapticsErasePlayingId(std::uint32_t playingId) noexcept
    {
        if (playingId == 0u) {
            return;
        }
        for (auto& slot : g_musicHapticsPlayingIds) {
            auto current = slot.load(std::memory_order_acquire);
            while (current == playingId &&
                   !slot.compare_exchange_weak(
                       current, 0u, std::memory_order_acq_rel, std::memory_order_acquire)) {
            }
        }
    }

    void musicHapticsClearAuthority() noexcept
    {
        for (auto& slot : g_musicHapticsPlayingIds) {
            slot.store(0u, std::memory_order_release);
        }
        if (g_audioTransport) {
            g_audioTransport->clearMusicHaptics();
        }
    }

    void syncMusicHapticsForMenu(std::string_view menu, bool opening) noexcept
    {
        const auto bit = musicHapticsMenuBit(menu);
        if (bit == 0u) {
            return;
        }

        if (opening) {
            (void)g_musicHapticsBlockingMenuMask.fetch_or(bit, std::memory_order_acq_rel);
            musicHapticsClearAuthority();
        } else {
            (void)g_musicHapticsBlockingMenuMask.fetch_and(
                static_cast<std::uint8_t>(~bit), std::memory_order_acq_rel);
        }
    }

    void initializeMusicHapticsMenuMask() noexcept
    {
        std::uint8_t mask = musicHapticsMenuBit("MainMenu");
        if (auto* ui = RE::UI::GetSingleton()) {
            mask = 0u;
            if (ui->IsMenuOpen(RE::BSFixedString("MainMenu"))) { mask |= musicHapticsMenuBit("MainMenu"); }
            if (ui->IsMenuOpen(RE::BSFixedString("DataMenu"))) { mask |= musicHapticsMenuBit("DataMenu"); }
            if (ui->IsMenuOpen(RE::BSFixedString("PauseMenu"))) { mask |= musicHapticsMenuBit("PauseMenu"); }
            if (ui->IsMenuOpen(RE::BSFixedString("LoadingMenu"))) { mask |= musicHapticsMenuBit("LoadingMenu"); }
            if (ui->IsMenuOpen(RE::BSFixedString("FaderMenu"))) { mask |= musicHapticsMenuBit("FaderMenu"); }
        }
        g_musicHapticsBlockingMenuMask.store(mask, std::memory_order_release);
        if (mask != 0u) {
            musicHapticsClearAuthority();
        }
    }

    const char* shipEmReconPhaseName(sds::ShipLaserReconPhase phase) noexcept
    {
        switch (phase) {
        case sds::ShipLaserReconPhase::PrePress:
            return "pre-press";
        case sds::ShipLaserReconPhase::Held:
            return "held";
        case sds::ShipLaserReconPhase::PostRelease:
            return "post-release";
        default:
            return "unknown";
        }
    }

    void logShipEmReconSample(const sds::ShipLaserReconSample& sample) noexcept
    {
        if (!g_shipEmReconEnabled.load(std::memory_order_acquire)) {
            return;
        }
        const auto ordinal = g_shipEmReconLogCount.fetch_add(1, std::memory_order_relaxed);
        if (ordinal >= kShipEmReconLogLimit) {
            if (ordinal == kShipEmReconLogLimit) {
                pluginLog("Ship EM recon: Wwise log limit reached; further raw samples suppressed, burst summaries remain active");
            }
            return;
        }

        const bool knownBallistic = sds::isHardwareObservedShipBallisticFireEvent(sample.eventId) ||
            (g_shipWeaponSemanticCache && g_shipWeaponSemanticCache->isPlayerBallisticFire(sample.eventId));
        const bool knownLaser = sds::isHardwareObservedShipPulseLaserFireEvent(sample.eventId);
        const bool knownParticle = sds::isHardwareObservedShipProtonBeamFireEvent(sample.eventId);
        const bool knownMissile = sds::isHardwareObservedShipMissileFireEvent(sample.eventId);
        std::ostringstream line;
        line << "Ship EM recon: Wwise burst=" << sample.burstId
             << " phase=" << shipEmReconPhaseName(sample.phase)
             << " seq=" << sample.sequence
             << " event=0x" << std::uppercase << std::hex
             << std::setw(8) << std::setfill('0') << sample.eventId
             << " gameObject=0x" << sample.gameObjectId
             << " callsite=Starfield+0x" << sample.callsiteRva
             << std::dec << std::setfill(' ')
             << " r2=" << static_cast<unsigned>(sample.triggerValue)
             << " deltaUs=" << sample.deltaMicros
             << " knownBallistic=" << (knownBallistic ? "yes" : "no")
             << " knownLaser=" << (knownLaser ? "yes" : "no")
             << " knownParticle=" << (knownParticle ? "yes" : "no")
             << " knownMissile=" << (knownMissile ? "yes" : "no")
             << " returnedPlayingId=" << sample.returnedPlayingId;
        pluginLog(line.str());
    }

    void observeShipEmReconWwise(
        const sds::WeaponSfxWwiseObservation& observation) noexcept
    {
        if (!g_shipEmReconEnabled.load(std::memory_order_acquire)) {
            return;
        }
        const auto sample = g_shipEmReconProbe.observeWwise({
            .sequence = observation.sequence,
            .eventId = observation.eventId,
            .gameObjectId = observation.gameObjectId,
            .callsiteRva = observation.callsiteRva,
            .returnedPlayingId = observation.returnedPlayingId,
            .when = observation.when,
        });
        if (sample) {
            logShipEmReconSample(*sample);
        }
    }

    void logLandVehicleWwiseAggregateSummary(
        const sds::LandVehicleWwiseAggregateSummary& summary,
        std::string_view reason) noexcept
    {
        if (summary.totalAggregated == 0) {
            return;
        }
        std::ostringstream line;
        line << "Land vehicle recon: Wwise aggregate reason=" << reason
             << " total=" << summary.totalAggregated
             << " families=" << summary.bucketCount;
        for (std::size_t i = 0; i < summary.bucketCount; ++i) {
            const auto& bucket = summary.buckets[i];
            line << " [event=0x" << std::uppercase << std::hex
                 << std::setw(8) << std::setfill('0') << bucket.eventId
                 << std::dec << std::setfill(' ')
                 << " count=" << bucket.count
                 << " seq=" << bucket.firstSequence << '-' << bucket.lastSequence
                 << "]";
        }
        line << " authority=correlation-only diagnostic-only controllerOutput=none";
        pluginLog(line.str());
    }

    void resetLandVehicleWwiseRecon(std::chrono::steady_clock::time_point when) noexcept
    {
        std::scoped_lock lock(g_landVehicleWwiseAggregatorMutex);
        g_landVehicleWwiseAggregator.reset(when);
        g_landVehicleRareWwiseLogCount.store(0, std::memory_order_release);
    }

    void flushLandVehicleWwiseRecon(std::string_view reason, std::chrono::steady_clock::time_point when) noexcept
    {
        sds::LandVehicleWwiseAggregateSummary summary{};
        {
            std::scoped_lock lock(g_landVehicleWwiseAggregatorMutex);
            summary = g_landVehicleWwiseAggregator.takeSummary(when);
        }
        logLandVehicleWwiseAggregateSummary(summary, reason);
    }

    void setLandVehicleActionGateAuthority(bool active, std::uint64_t epoch) noexcept
    {
        std::scoped_lock lock(g_landVehicleActionGateMutex);
        g_landVehicleActionGate.setAuthority(active, epoch);
    }

    void resetLandVehicleActionGate() noexcept
    {
        std::scoped_lock lock(g_landVehicleActionGateMutex);
        g_landVehicleActionGate.reset();
    }

    void syncLandVehicleActionGateForMenu(std::string_view menu, bool opening) noexcept
    {
        if (menu != "DataMenu" && menu != "PauseMenu" && menu != "LoadingMenu") {
            return;
        }
        if (opening) {
            setLandVehicleActionGateAuthority(false, 0);
            return;
        }
        if (menu == "LoadingMenu" || !g_gameState) {
            return;
        }
        const auto physics = g_gameState->latestLandVehiclePhysicsSnapshot();
        if (physics.authorityActive && physics.authorityEpoch != 0) {
            setLandVehicleActionGateAuthority(true, physics.authorityEpoch);
        }
    }

    void observeLandVehicleProductionSemantic(
        std::string_view semantic,
        bool active,
        std::chrono::steady_clock::time_point when) noexcept
    {
        sds::LandVehicleAction action = sds::LandVehicleAction::None;
        {
            std::scoped_lock lock(g_landVehicleActionGateMutex);
            if (semantic == "VehicleFireWeapon") {
                action = g_landVehicleActionGate.observeFireSemantic(active, when);
            }
            if (semantic == "VehicleAim") {
                action = g_landVehicleActionGate.observeAimSemantic(active);
            }
        }

        if (action == sds::LandVehicleAction::None || !g_eventRouter) {
            return;
        }

        sds::GameEvent event{};
        event.when = when;
        switch (action) {
        case sds::LandVehicleAction::GunFired:
            event.type = sds::GameEventType::LandVehicleGunFired;
            break;
        case sds::LandVehicleAction::AimStarted:
            event.type = sds::GameEventType::LandVehicleAimStarted;
            break;
        case sds::LandVehicleAction::AimStopped:
            event.type = sds::GameEventType::LandVehicleAimStopped;
            break;
        case sds::LandVehicleAction::None:
            return;
        }
        (void)g_eventRouter->dispatch(std::move(event));
    }

    std::string_view landVehicleRareWwiseClassification(std::uint32_t eventId) noexcept
    {
        if (eventId == sds::kHardwareObservedLandVehicleGunFireEventId) {
            return "gun-fire-hardware-observed";
        }
        if (eventId == sds::kHardwareObservedLandVehicleVerticalBoostEventId) {
            return "vertical-boost-hardware-observed";
        }
        if (eventId == sds::kLandVehicleTouchdownCandidatePrimaryEventId) {
            return "contact-suspension-corroboration";
        }
        if (eventId == sds::kLandVehicleTouchdownCandidateSecondaryEventId) {
            return "terrain-contact-not-touchdown";
        }
        return "rare-preserved";
    }
    bool isLandVehicleTouchdownCandidate(std::uint32_t eventId) noexcept
    {
        return eventId == sds::kLandVehicleTouchdownCandidatePrimaryEventId ||
            eventId == sds::kLandVehicleTouchdownCandidateSecondaryEventId;
    }

    void observeLandVehicleReconWwise(
        const sds::WeaponSfxWwiseObservation& observation) noexcept
    {
        if (!g_gameState || !g_gameState->landVehicleReconCorrelationArmed()) {
            return;
        }

        if (observation.eventId == sds::kHardwareObservedLandVehicleVerticalBoostEventId) {
            (void)g_gameState->armLandVehicleVerticalBoost();
        }

        sds::LandVehicleAction productionAction = sds::LandVehicleAction::None;
        {
            std::scoped_lock lock(g_landVehicleActionGateMutex);
            productionAction = g_landVehicleActionGate.observeWwise(observation.eventId, observation.when);
        }
        if (productionAction == sds::LandVehicleAction::GunFired && g_eventRouter) {
            sds::GameEvent event{};
            event.type = sds::GameEventType::LandVehicleGunFired;
            event.when = observation.when;
            (void)g_eventRouter->dispatch(std::move(event));
        }

        sds::LandVehicleWwiseReconObserveResult result{};
        std::optional<sds::LandVehicleWwiseAggregateSummary> summary{};
        {
            std::scoped_lock lock(g_landVehicleWwiseAggregatorMutex);
            result = g_landVehicleWwiseAggregator.observe({
                .sequence = observation.sequence,
                .eventId = observation.eventId,
                .gameObjectId = observation.gameObjectId,
                .callsiteRva = observation.callsiteRva,
                .returnedPlayingId = observation.returnedPlayingId,
                .when = observation.when,
            });
            summary = g_landVehicleWwiseAggregator.takeSummaryIfDue(observation.when);
        }

        if (summary) {
            logLandVehicleWwiseAggregateSummary(*summary, "periodic");
        }
        if (!result.preserveIndividually) {
            return;
        }

        const auto index = g_landVehicleRareWwiseLogCount.fetch_add(1, std::memory_order_acq_rel);
        if (index >= kLandVehicleRareWwiseLogLimit) {
            return;
        }
        const auto classification = landVehicleRareWwiseClassification(observation.eventId);
        std::ostringstream line;
        line << "Land vehicle recon: Wwise rare seq=" << observation.sequence
             << " event=0x" << std::uppercase << std::hex
             << std::setw(8) << std::setfill('0') << observation.eventId
             << " gameObject=0x" << observation.gameObjectId
             << " callsite=Starfield+0x" << observation.callsiteRva
             << std::dec << std::setfill(' ')
             << " returnedPlayingId=" << observation.returnedPlayingId
             << " classification=" << classification;
        if (isLandVehicleTouchdownCandidate(observation.eventId) && g_gameState) {
            const auto telemetry = g_gameState->latestLandVehicleTelemetrySnapshot();
            line << " telemetryRef=0x" << std::uppercase << std::hex
                 << std::setw(8) << std::setfill('0') << telemetry.referenceFormId
                 << std::dec << std::setfill(' ')
                 << " positionReadable=" << (telemetry.positionReadable ? "yes" : "no")
                 << " velocityReadable=" << (telemetry.velocityReadable ? "yes" : "no")
                 << " speed=" << std::fixed << std::setprecision(3) << telemetry.speed
                 << " verticalSpeed=" << telemetry.velocityZ
                 << " accelerationReadable=" << (telemetry.accelerationReadable ? "yes" : "no")
                 << " acceleration=" << telemetry.acceleration
                 << " sampleMs=" << (telemetry.sampleIntervalSeconds * 1000.0F);
        }
        line << " authority=correlation-only diagnostic-only controllerOutput=none";
        pluginLog(line.str());
    }

    void logShipEmReconSummary(const sds::ShipLaserReconBurstSummary& summary) noexcept
    {
        for (const auto& event : summary.events) {
            std::string phases;
            if (event.sawPrePress) phases += "pre";
            if (event.sawHeld) phases += phases.empty() ? "held" : "+held";
            if (event.sawPostRelease) phases += phases.empty() ? "post" : "+post";

            const bool knownBallistic = sds::isHardwareObservedShipBallisticFireEvent(event.eventId) ||
                (g_shipWeaponSemanticCache && g_shipWeaponSemanticCache->isPlayerBallisticFire(event.eventId));
            const bool knownLaser = sds::isHardwareObservedShipPulseLaserFireEvent(event.eventId);
            const bool knownParticle = sds::isHardwareObservedShipProtonBeamFireEvent(event.eventId);
            const bool knownMissile = sds::isHardwareObservedShipMissileFireEvent(event.eventId);
            std::ostringstream line;
            line << "Ship EM recon summary: burst=" << summary.burstId
                 << " heldUs=" << summary.heldDurationMicros
                 << " event=0x" << std::uppercase << std::hex
                 << std::setw(8) << std::setfill('0') << event.eventId
                 << " gameObject=0x" << event.gameObjectId
                 << std::dec << std::setfill(' ')
                 << " posts=" << event.posts
                 << " firstDeltaUs=" << event.firstDeltaMicros
                 << " lastDeltaUs=" << event.lastDeltaMicros
                 << " minIntervalUs=" << event.minIntervalMicros
                 << " maxIntervalUs=" << event.maxIntervalMicros
                 << " phases=" << (phases.empty() ? "none" : phases)
                 << " knownBallistic=" << (knownBallistic ? "yes" : "no")
                 << " knownLaser=" << (knownLaser ? "yes" : "no")
                 << " knownParticle=" << (knownParticle ? "yes" : "no")
                 << " knownMissile=" << (knownMissile ? "yes" : "no")
                 << " interpretation=diagnostic-only";
            pluginLog(line.str());
        }
        if (summary.events.empty()) {
            std::ostringstream line;
            line << "Ship EM recon summary: burst=" << summary.burstId
                 << " heldUs=" << summary.heldDurationMicros
                 << " events=0 interpretation=no-zero-external-Wwise-posts-in-window diagnostic-only";
            pluginLog(line.str());
        }
    }

    const char* shipLaunchLandingTransitionName(sds::ShipLaunchLandingTransition type) noexcept
    {
        switch (type) {
        case sds::ShipLaunchLandingTransition::Takeoff:
            return "takeoff";
        case sds::ShipLaunchLandingTransition::Touchdown:
            return "touchdown";
        case sds::ShipLaunchLandingTransition::LandingSequence:
            return "landing-sequence";
        default:
            return "unknown";
        }
    }

    const char* shipLaunchLandingPhaseName(sds::ShipLaunchLandingReconPhase phase) noexcept
    {
        switch (phase) {
        case sds::ShipLaunchLandingReconPhase::PreBoundary:
            return "pre";
        case sds::ShipLaunchLandingReconPhase::PostBoundary:
            return "post";
        default:
            return "unknown";
        }
    }

    void logShipLaunchLandingBoundary(const sds::ShipLaunchLandingReconBoundary& boundary) noexcept
    {
        std::ostringstream line;
        line << "Ship launch/landing recon: boundary=" << shipLaunchLandingTransitionName(boundary.type)
             << " transition=" << boundary.transitionId
             << " beforeLanded=" << (boundary.before.landed ? "yes" : "no")
             << " afterLanded=" << (boundary.after.landed ? "yes" : "no")
             << " beforeDocked=" << (boundary.before.docked ? "yes" : "no")
             << " afterDocked=" << (boundary.after.docked ? "yes" : "no")
             << " throttleTarget=" << boundary.after.throttleTarget
             << " throttleReadable=" << (boundary.after.throttleTargetReadable ? "yes" : "no")
             << " effectiveThrottle=" << boundary.after.effectiveThrottle
             << " effectiveReadable=" << (boundary.after.effectiveThrottleReadable ? "yes" : "no")
             << " velocity=" << boundary.after.velocity
             << " velocityReadable=" << (boundary.after.velocityReadable ? "yes" : "no")
             << " interpretation=state-boundary-only diagnostic-only";
        pluginLog(line.str());
    }

    const char* shipLaunchLandingRumblePhaseName(ShipLaunchLandingRumblePhase phase) noexcept
    {
        switch (phase) {
        case ShipLaunchLandingRumblePhase::Takeoff: return "takeoff";
        case ShipLaunchLandingRumblePhase::Landing: return "landing";
        default: return "none";
        }
    }

    void dispatchShipLaunchLandingTriggerSemantic(
        sds::GameEventType type,
        ShipLaunchLandingRumblePhase phase,
        std::chrono::steady_clock::time_point when) noexcept
    {
        if (!g_eventRouter) {
            return;
        }
        sds::GameEvent semantic{};
        semantic.type = type;
        semantic.when = when;
        const auto phaseName = std::string_view(shipLaunchLandingRumblePhaseName(phase));
        std::copy_n(phaseName.data(), std::min(phaseName.size(), semantic.text.size() - 1), semantic.text.begin());
        (void)g_eventRouter->dispatch(std::move(semantic));
    }

    void stopShipLaunchLandingHaptics(std::string_view reason) noexcept
    {
        const auto previous = g_shipLaunchLandingRumblePhase.exchange(
            ShipLaunchLandingRumblePhase::None, std::memory_order_acq_rel);
        if (previous == ShipLaunchLandingRumblePhase::None) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        g_shipLaunchLandingStartedAtUs.store(0, std::memory_order_release);
        g_shipTakeoffFirstFaderClosed.store(false, std::memory_order_release);
        g_shipTakeoffBoundaryObserved.store(false, std::memory_order_release);

        if (g_haptics) {
            (void)g_haptics->setShipLaunchLandingRumble(false, reason);
        }
        dispatchShipLaunchLandingTriggerSemantic(
            sds::GameEventType::ShipLaunchLandingHapticsStopped, previous, now);

        std::ostringstream line;
        line << "Ship launch/landing haptics: stage=stop phase="
             << shipLaunchLandingRumblePhaseName(previous)
             << " reason=" << reason
             << " triggers=release";
        pluginLog(line.str());
    }

    void startShipLaunchLandingHaptics(ShipLaunchLandingRumblePhase phase) noexcept
    {
        if (phase == ShipLaunchLandingRumblePhase::None) {
            return;
        }
        auto expected = ShipLaunchLandingRumblePhase::None;
        if (!g_shipLaunchLandingRumblePhase.compare_exchange_strong(
            expected, phase, std::memory_order_acq_rel, std::memory_order_acquire)) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        g_shipLaunchLandingStartedAtUs.store(steadyMicros(now), std::memory_order_release);
        g_shipTakeoffFirstFaderClosed.store(false, std::memory_order_release);
        g_shipTakeoffBoundaryObserved.store(false, std::memory_order_release);

        if (g_haptics) {
            (void)g_haptics->setShipLaunchLandingRumble(true, shipLaunchLandingRumblePhaseName(phase));
        }
        dispatchShipLaunchLandingTriggerSemantic(
            sds::GameEventType::ShipLaunchLandingHapticsStarted, phase, now);

        std::ostringstream line;
        line << "Ship launch/landing haptics: stage=start phase="
             << shipLaunchLandingRumblePhaseName(phase)
             << " body=ShipBoost gain=1.000 level=1.000"
             << " triggers=both-EffectEx-sustained";
        pluginLog(line.str());
    }

    void maybeStartShipTakeoffRumble(bool pilotActive, bool precisionTouchdownPoll) noexcept
    {
        if (!pilotActive || precisionTouchdownPoll ||
            g_shipLaunchLandingRumblePhase.load(std::memory_order_acquire) != ShipLaunchLandingRumblePhase::None ||
            !g_lastShipStateValid.load(std::memory_order_acquire) ||
            !g_lastShipLanded.load(std::memory_order_acquire) ||
            g_lastShipDocked.load(std::memory_order_acquire) ||
            g_shipBallisticBlockingMenuMask.load(std::memory_order_acquire) != 0) {
            return;
        }

        auto* ui = RE::UI::GetSingleton();
        if (!ui ||
            !ui->IsMenuOpen(RE::BSFixedString("FaderMenu")) ||
            ui->IsMenuOpen(RE::BSFixedString("TakeoffMenu"))) {
            return;
        }

        startShipLaunchLandingHaptics(ShipLaunchLandingRumblePhase::Takeoff);
    }

    void dispatchShipTouchdownHaptic(
        const sds::ShipLaunchLandingReconBoundary& boundary,
        std::chrono::steady_clock::time_point when) noexcept
    {
        if (!g_eventRouter ||
            boundary.type != sds::ShipLaunchLandingTransition::Touchdown ||
            boundary.before.landed || !boundary.after.landed ||
            boundary.before.docked || boundary.after.docked) {
            return;
        }

        sds::GameEvent semantic{};
        semantic.type = sds::GameEventType::ShipTouchdown;
        semantic.when = when;
        (void)g_eventRouter->dispatch(std::move(semantic));

        std::ostringstream line;
        line << "Ship touchdown haptics: authority=landing-sequence-landed-false-to-true"
             << " transition=" << boundary.transitionId
             << " eventWhenUs="
             << std::chrono::duration_cast<std::chrono::microseconds>(when.time_since_epoch()).count()
             << " WwiseAuthority=none diagnosticCorroborationOnly=yes";
        pluginLog(line.str());
    }

    void observeShipLaunchLandingReconWwise(
        const sds::WeaponSfxWwiseObservation& observation) noexcept
    {
        if (!g_shipLaunchLandingReconEnabled.load(std::memory_order_acquire)) {
            return;
        }
        g_shipLaunchLandingReconProbe.observeWwise({
            .sequence = observation.sequence,
            .eventId = observation.eventId,
            .gameObjectId = observation.gameObjectId,
            .callsiteRva = observation.callsiteRva,
            .returnedPlayingId = observation.returnedPlayingId,
            .when = observation.when,
        });
    }

    void logShipLaunchLandingReconReport(const sds::ShipLaunchLandingReconReport& report) noexcept
    {
        {
            std::ostringstream line;
            line << "Ship launch/landing recon report: transition=" << report.transitionId
                 << " boundary=" << shipLaunchLandingTransitionName(report.type)
                 << " samples=" << report.samples.size();
            if (report.type == sds::ShipLaunchLandingTransition::LandingSequence) {
                const auto appendDelta = [&line](const char* name, const auto& value) {
                    line << ' ' << name;
                    if (value) {
                        line << *value;
                    } else {
                        line << "na";
                    }
                };
                line << " anchor=FaderMenu-open-or-LoadingMenu-open"
                     << " beforeLanded=" << (report.before.landed ? "yes" : "no")
                     << " stateBoundaryObserved="
                     << (report.touchdownObservedDeltaMicros ? "yes" : "no")
                     << " afterLanded=" << (report.after.landed ? "yes" : "no");
                appendDelta("galaxyStarMapClosedDeltaUs=", report.galaxyStarMapClosedDeltaMicros);
                appendDelta("faderOpenedDeltaUs=", report.faderOpenedDeltaMicros);
                appendDelta("loadingOpenedDeltaUs=", report.loadingOpenedDeltaMicros);
                appendDelta("spaceshipHudClosedDeltaUs=", report.spaceshipHudClosedDeltaMicros);
                appendDelta("loadingClosedDeltaUs=", report.loadingClosedDeltaMicros);
                appendDelta("faderClosedDeltaUs=", report.faderClosedDeltaMicros);
                appendDelta("touchdownObservedDeltaUs=", report.touchdownObservedDeltaMicros);
                appendDelta("takeoffMenuOpenedDeltaUs=", report.takeoffMenuOpenedDeltaMicros);
                appendDelta("takeoffMenuClosedDeltaUs=", report.takeoffMenuClosedDeltaMicros);
                appendDelta("secondFaderOpenedDeltaUs=", report.secondFaderOpenedDeltaMicros);
                appendDelta("secondLoadingOpenedDeltaUs=", report.secondLoadingOpenedDeltaMicros);
                appendDelta("secondLoadingClosedDeltaUs=", report.secondLoadingClosedDeltaMicros);
                appendDelta("secondFaderClosedDeltaUs=", report.secondFaderClosedDeltaMicros);
                line << " cinematicTail="
                     << (report.takeoffMenuOpenedDeltaMicros ? "observed" : "fallback")
                     << " diagnosticOnly=yes";
            } else {
                line << " preMs=2000 postMs=2000"
                     << " beforeLanded=" << (report.before.landed ? "yes" : "no")
                     << " afterLanded=" << (report.after.landed ? "yes" : "no")
                     << " diagnosticOnly=yes";
            }
            pluginLog(line.str());
        }

        for (const auto& sample : report.samples) {
            const bool knownBallistic = sds::isHardwareObservedShipBallisticFireEvent(sample.eventId) ||
                (g_shipWeaponSemanticCache && g_shipWeaponSemanticCache->isPlayerBallisticFire(sample.eventId));
            const bool knownLaser = sds::isHardwareObservedShipPulseLaserFireEvent(sample.eventId);
            const bool knownParticle = sds::isHardwareObservedShipProtonBeamFireEvent(sample.eventId);
            const bool knownMissile = sds::isHardwareObservedShipMissileFireEvent(sample.eventId);
            const bool knownEM = sds::isHardwareObservedShipEMFireEvent(sample.eventId);
            std::ostringstream line;
            line << "Ship launch/landing recon report: transition=" << report.transitionId
                 << " boundary=" << shipLaunchLandingTransitionName(report.type)
                 << " phase=" << shipLaunchLandingPhaseName(sample.phase)
                 << " deltaUs=" << sample.deltaMicros
                 << " seq=" << sample.sequence
                 << " event=0x" << std::uppercase << std::hex
                 << std::setw(8) << std::setfill('0') << sample.eventId
                 << " gameObject=0x" << sample.gameObjectId
                 << " callsite=Starfield+0x" << sample.callsiteRva
                 << std::dec << std::setfill(' ')
                 << " returnedPlayingId=" << sample.returnedPlayingId
                 << " menus=map:" << (sample.galaxyStarMapOpen ? "open" : "closed")
                 << ",fader:" << (sample.faderOpen ? "open" : "closed")
                 << ",loading:" << (sample.loadingOpen ? "open" : "closed")
                 << ",hud:" << (sample.spaceshipHudOpen ? "open" : "closed")
                 << ",takeoff:" << (sample.takeoffMenuOpen ? "open" : "closed")
                 << " knownBallistic=" << (knownBallistic ? "yes" : "no")
                 << " knownLaser=" << (knownLaser ? "yes" : "no")
                 << " knownParticle=" << (knownParticle ? "yes" : "no")
                 << " knownMissile=" << (knownMissile ? "yes" : "no")
                 << " knownEM=" << (knownEM ? "yes" : "no")
                 << " diagnosticOnly=yes";
            pluginLog(line.str());
        }
    }

    void observeAutoRivetChargeSemantic(
        const sds::WeaponSfxWwiseObservation& observation) noexcept
    {
        if (!g_haptics ||
            !g_autoRivetChargeCaptureEnabled.load(std::memory_order_acquire)) {
            return;
        }

        const auto action = sds::routeAutoRivetChargeWwise(
            observation.eventId,
            observation.gameObjectId);
        const auto marker = sds::autoRivetChargeMarker(action);
        if (marker.empty()) {
            return;
        }

        sds::GameEvent semantic{};
        semantic.type = sds::GameEventType::WeaponFired;
        semantic.when = observation.when;
        const auto count = (std::min)(marker.size(), semantic.text.size() - 1);
        std::copy_n(marker.data(), count, semantic.text.data());
        semantic.text[count] = '\0';
        (void)g_haptics->handle(std::move(semantic));

        pluginLog(
            action == sds::AutoRivetChargeAction::Start ?
                "Auto-Rivet tension: charge-start semantic observed source=player-Wwise" :
                "Auto-Rivet tension: charge-stop semantic observed source=player-Wwise");
    }

    void observeShipBallisticWwise(
        const sds::WeaponSfxWwiseObservation& observation) noexcept
    {
        if (!g_shipWeaponSemanticCache || !g_eventRouter) {
            return;
        }

        const auto correlation = g_shipBallisticFireGate.inspectCorrelation(observation.when);
        if (correlation.recentR2) {
            const auto ordinal = g_shipWeaponCaptureProbeCount.fetch_add(1, std::memory_order_relaxed);
            if (ordinal < kShipWeaponCaptureProbeLimit) {
                const bool catalogMatch = g_shipWeaponSemanticCache->isPlayerBallisticFire(observation.eventId);
                const bool hardwareAlias =
                    observation.eventId == sds::kHardwareObservedShipBallisticFireEventId;
                std::ostringstream probe;
                probe << "Ship weapon capture probe: correlated seq=" << observation.sequence
                      << " event=0x" << std::uppercase << std::hex
                      << std::setw(8) << std::setfill('0') << observation.eventId
                      << " gameObject=0x" << observation.gameObjectId
                      << " callsite=Starfield+0x" << observation.callsiteRva
                      << std::dec << std::setfill(' ')
                      << " r2=" << static_cast<unsigned>(correlation.r2)
                      << " deltaUs=" << correlation.deltaMicros
                      << " catalogMatch=" << (catalogMatch ? "yes" : "no")
                      << " hardwareAlias=" << (hardwareAlias ? "yes" : "no")
                      << " returnedPlayingId=" << observation.returnedPlayingId;
                pluginLog(probe.str());
            } else if (ordinal == kShipWeaponCaptureProbeLimit) {
                pluginLog("Ship weapon capture probe: correlated log limit reached; further R2-window Wwise observations suppressed");
            }
        }

        const bool catalogMatch =
            g_shipWeaponSemanticCache->isPlayerBallisticFire(observation.eventId);
        const bool hardwareAlias =
            observation.eventId == sds::kHardwareObservedShipBallisticFireEventId;
        if (catalogMatch || hardwareAlias) {
            std::ostringstream candidate;
            candidate << "Ship ballistic capture: candidate event=0x" << std::uppercase << std::hex
                      << std::setw(8) << std::setfill('0') << observation.eventId
                      << " gameObject=0x" << observation.gameObjectId
                      << std::dec << std::setfill(' ')
                      << " catalogMatch=" << (catalogMatch ? "yes" : "no")
                      << " hardwareAlias=" << (hardwareAlias ? "yes" : "no")
                      << " semanticSource=" << (hardwareAlias ? "r3-hardware-correlation" : "SoundBanksInfo")
                      << " stage=pre-R2-correlation";
            pluginLog(candidate.str());
        }

        if (!g_shipBallisticFireGate.authorizeWwiseFire(
                observation.eventId, observation.gameObjectId, observation.when, *g_shipWeaponSemanticCache)) {
            return;
        }

        sds::GameEvent semantic{};
        semantic.type = sds::GameEventType::ShipBallisticWeaponFired;
        semantic.when = observation.when;
        (void)g_eventRouter->dispatch(std::move(semantic));

        std::ostringstream line;
        line << "Ship ballistic fire: event=0x" << std::uppercase << std::hex
             << std::setw(8) << std::setfill('0') << observation.eventId
             << " gameObject=0x" << observation.gameObjectId
             << std::dec << std::setfill(' ')
             << " family=ballistic control=R2 source=player-Wwise+R2-correlation"
             << " semanticSource="
             << (hardwareAlias ? "r3-hardware-correlation" : "SoundBanksInfo");
        pluginLog(line.str());
    }

    void observeShipLaserWwise(
        const sds::WeaponSfxWwiseObservation& observation) noexcept
    {
        if (!g_eventRouter ||
            !g_shipLaserFireGate.authorizeWwiseFire(
                observation.eventId, observation.gameObjectId, observation.when)) {
            return;
        }

        sds::GameEvent semantic{};
        semantic.type = sds::GameEventType::ShipLaserWeaponFired;
        semantic.when = observation.when;
        (void)g_eventRouter->dispatch(std::move(semantic));

        std::ostringstream line;
        line << "Ship laser fire: event=0x" << std::uppercase << std::hex
             << std::setw(8) << std::setfill('0') << observation.eventId
             << " gameObject=0x" << observation.gameObjectId
             << std::dec << std::setfill(' ')
             << " family=laser control=R2 source=player-Wwise+R2-correlation"
             << " semanticSource=v0364-hardware-recon";
        pluginLog(line.str());
    }

    void observeShipParticleWwise(
        const sds::WeaponSfxWwiseObservation& observation) noexcept
    {
        if (!g_eventRouter ||
            !g_shipParticleFireGate.authorizeWwiseFire(
                observation.eventId, observation.gameObjectId, observation.when)) {
            return;
        }

        sds::GameEvent semantic{};
        semantic.type = sds::GameEventType::ShipParticleWeaponFired;
        semantic.when = observation.when;
        (void)g_eventRouter->dispatch(std::move(semantic));

        std::ostringstream line;
        line << "Ship particle fire: event=0x" << std::uppercase << std::hex
             << std::setw(8) << std::setfill('0') << observation.eventId
             << " gameObject=0x" << observation.gameObjectId
             << std::dec << std::setfill(' ')
             << " family=particle control=R2 source=player-Wwise+R2-correlation"
             << " semanticSource=v0366-proton-beam-hardware-recon";
        pluginLog(line.str());
    }


    void observeShipMissileWwise(
        const sds::WeaponSfxWwiseObservation& observation) noexcept
    {
        if (!g_eventRouter ||
            !g_shipMissileFireGate.authorizeWwiseFire(
                observation.eventId, observation.gameObjectId, observation.when)) {
            return;
        }

        sds::GameEvent semantic{};
        semantic.type = sds::GameEventType::ShipMissileWeaponFired;
        semantic.when = observation.when;
        (void)g_eventRouter->dispatch(std::move(semantic));

        std::ostringstream line;
        line << "Ship missile fire: event=0x" << std::uppercase << std::hex
             << std::setw(8) << std::setfill('0') << observation.eventId
             << " gameObject=0x" << observation.gameObjectId
             << std::dec << std::setfill(' ')
             << " family=missile control=R2 source=player-Wwise+R2-correlation"
             << " semanticSource=v0368-missile-hardware-recon";
        pluginLog(line.str());
    }

    void observeShipEMWwise(
        const sds::WeaponSfxWwiseObservation& observation) noexcept
    {
        if (!g_eventRouter ||
            !g_shipEMFireGate.authorizeWwiseFire(
                observation.eventId, observation.gameObjectId, observation.when)) {
            return;
        }

        sds::GameEvent semantic{};
        semantic.type = sds::GameEventType::ShipEMWeaponFired;
        semantic.when = observation.when;
        (void)g_eventRouter->dispatch(std::move(semantic));

        std::ostringstream line;
        line << "Ship EM fire: event=0x" << std::uppercase << std::hex
             << std::setw(8) << std::setfill('0') << observation.eventId
             << " gameObject=0x" << observation.gameObjectId
             << std::dec << std::setfill(' ')
             << " family=em control=R2 source=player-Wwise+R2-correlation"
             << " semanticSource=v0370-em-hardware-recon";
        pluginLog(line.str());
    }

    std::string readTextFile(const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (!stream) {
            return {};
        }
        std::ostringstream text;
        text << stream.rdbuf();
        return text.str();
    }

    std::filesystem::path currentExecutablePath() noexcept
    {
        std::array<wchar_t, 32768> buffer{};
        const auto length = ::GetModuleFileNameW(
            nullptr,
            buffer.data(),
            static_cast<DWORD>(buffer.size()));
        if (length == 0 || length >= buffer.size()) {
            return {};
        }
        return std::filesystem::path(std::wstring(buffer.data(), length));
    }

    sds::Config loadRuntimeConfig()
    {
        const auto text = readTextFile(std::filesystem::path(kConfigPath));
        if (text.empty()) {
            pluginLog("Config: using built-in defaults (file absent or empty)");
            return sds::Config::defaults();
        }

        pluginLog("Config: loaded Data/SFSE/Plugins/StarfieldDualSense.toml");
        return sds::loadConfig(text);
    }

    void startEarlyMainMenuUiPreparation() noexcept
    {
        try {
            // Prewarm is producer/cache preparation, not live playback policy.
            // Keep it available even when ControllerSpeaker or ScannerUI starts OFF;
            // ControllerSpeakerManager remains the live admission authority.
            const auto executablePath = currentExecutablePath();
            if (executablePath.empty()) {
                pluginLog("Main-menu UI speaker prewarm: INACTIVE reason=data-path-unavailable");
                return;
            }
            g_weaponAudioPipelineDataPath = executablePath.parent_path() / "Data";
            if (!g_uiSpeakerPreparedCache) {
                g_uiSpeakerPreparedCache = std::make_shared<sds::UiSpeakerPreparedCache>();
            }

            const sds::AudioPipelineStartupOptions startupOptions{
                .prepareUi = true,
                .prepareMainMenuUiOnly = true,
                .prepareWeapons = false,
                .resolveUiDiagnostics = false,
            };
            g_mainMenuUiAudioPipeline = std::make_unique<sds::WeaponAudioPipeline>(
                std::shared_ptr<sds::WeaponSpeakerPreparedCache>{},
                sds::makeRealWeaponAudioPipelineBackend(g_weaponAudioPipelineDataPath, startupOptions),
                g_uiSpeakerPreparedCache);
            g_mainMenuUiAudioPipeline->start();
            pluginLog("Main-menu UI speaker prewarm: worker started cues=3 events=GeneralFocus,GeneralOK,GeneralCancel preparation=pre-PostDataLoad");
        } catch (const std::exception& exception) {
            pluginLog(std::string("Main-menu UI speaker prewarm: failed error=\"") + exception.what() + "\"");
        } catch (...) {
            pluginLog("Main-menu UI speaker prewarm: failed error=\"unknown exception\"");
        }
    }

    bool refreshCurrentEquippedWeaponState(std::uint32_t* refreshedFormId = nullptr) noexcept
    {
        if (g_runtimeShutdown || !g_gameState || !g_fireMarkerBridge) {
            return false;
        }

        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!player) {
            return false;
        }

        RE::TESBoundObject* equippedWeapon = nullptr;
        player->ForEachEquippedItem([&](const RE::BGSInventoryItem& item) {
            if (item.object && item.object->GetFormType() == RE::FormType::kWEAP) {
                equippedWeapon = item.object;
                return RE::BSContainer::ForEachResult::kStop;
            }
            return RE::BSContainer::ForEachResult::kContinue;
        });

        if (!equippedWeapon) {
            return false;
        }

        RE::ActorItemEquipped::Event equipEvent{};
        equipEvent.item = equippedWeapon;
        equipEvent.actor = RE::NiPointer<RE::Actor>(player);

        (void)g_gameState->ProcessEvent(equipEvent, nullptr);
        (void)g_fireMarkerBridge->ProcessEvent(equipEvent, nullptr);

        if (refreshedFormId) {
            *refreshedFormId = equippedWeapon->GetFormID();
        }
        return true;
    }

    void bootstrapStartupEquippedWeaponIfReady() noexcept
    {
        if (g_startupWeaponBootstrapComplete || g_runtimeShutdown || !g_gameState || !g_fireMarkerBridge) {
            return;
        }

        auto* ui = RE::UI::GetSingleton();
        auto* player = RE::PlayerCharacter::GetSingleton();
        if (!ui || !player || !ui->IsMenuOpen(RE::BSFixedString("HUDMenu"))) {
            return;
        }

        // HUD availability is the first stable in-game boundary after save loading.
        // Query the loaded inventory exactly once, then reuse the same equip-event
        // handlers that service real ActorItemEquipped notifications.
        g_startupWeaponBootstrapComplete = true;

        std::uint32_t formId = 0;
        if (!refreshCurrentEquippedWeaponState(&formId)) {
            pluginLog("Game state: startup equipped weapon bootstrap: no weapon equipped");
            return;
        }

        std::ostringstream diagnostic;
        diagnostic << "Game state: startup equipped weapon bootstrapped form=0x"
                   << std::hex << std::uppercase << formId;
        pluginLog(diagnostic.str());
    }

    std::uint8_t boostpackProductionMenuBit(std::string_view menu) noexcept
    {
        if (menu == "MainMenu") return 0x01u;
        if (menu == "DataMenu") return 0x02u;
        if (menu == "PauseMenu") return 0x04u;
        if (menu == "LoadingMenu") return 0x08u;
        if (menu == "FaderMenu") return 0x10u;
        return 0u;
    }

    bool boostpackProductionCaptureArmed() noexcept
    {
        return g_boostpackFeedbackAuthority != nullptr &&
            g_boostpackProductionContextEligible.load(std::memory_order_acquire) &&
            !g_runtimeShutdown;
    }

    void refreshBoostpackSemanticArming() noexcept
    {
        sds::setBoostpackSemanticObservationArmed(boostpackProductionCaptureArmed());
    }

    void setBoostpackProductionContextEligible(
        bool eligible,
        std::string_view reason) noexcept
    {
        bool wasActive = false;
        bool changed = false;
        {
            std::scoped_lock lock(g_boostpackFeedbackMutex);
            if (!g_boostpackFeedbackAuthority) {
                g_boostpackProductionContextEligible.store(false, std::memory_order_release);
                refreshBoostpackSemanticArming();
                return;
            }

            wasActive = g_boostpackFeedbackAuthority->active();
            const bool previous =
                g_boostpackProductionContextEligible.exchange(eligible, std::memory_order_acq_rel);
            changed = previous != eligible;
            g_boostpackFeedbackAuthority->setContextEligible(eligible);
        }

        if (!eligible && wasActive) {
            if (g_haptics) {
                (void)g_haptics->stopBoostpackThrust();
            }
            if (g_audioTransport) {
                g_audioTransport->clearSpeakerPlayback();
            }
        }

        refreshBoostpackSemanticArming();

        if (changed) {
            std::ostringstream line;
            line << "Boostpack production context: "
                 << (eligible ? "ELIGIBLE" : "BLOCKED")
                 << " reason=" << reason
                 << " authority=exact-Wwise-0x1BE06B49"
                 << " playerObject=0x2";
            pluginLog(line.str());
        }
    }

    void refreshBoostpackProductionContext(std::string_view reason) noexcept
    {
        const bool eligible =
            g_boostpackFeedbackAuthority != nullptr &&
            g_boostpackProductionControllerConnected.load(std::memory_order_acquire) &&
            !g_shipPilotActive.load(std::memory_order_acquire) &&
            !g_boostpackProductionLandVehicleActive.load(std::memory_order_acquire) &&
            g_boostpackProductionBlockingMenuMask.load(std::memory_order_acquire) == 0u &&
            !g_runtimeShutdown;
        setBoostpackProductionContextEligible(eligible, reason);
    }

    void syncBoostpackProductionForMenu(std::string_view menu, bool opened) noexcept
    {
        const auto bit = boostpackProductionMenuBit(menu);
        if (bit == 0u) {
            return;
        }

        if (opened) {
            g_boostpackProductionBlockingMenuMask.fetch_or(bit, std::memory_order_acq_rel);
        } else {
            g_boostpackProductionBlockingMenuMask.fetch_and(
                static_cast<std::uint8_t>(~bit),
                std::memory_order_acq_rel);
        }
        refreshBoostpackProductionContext(opened ? "menu-open" : "menu-close");
    }

    void initializeBoostpackProductionMenuMask() noexcept
    {
        std::uint8_t mask = 0u;
        if (auto* ui = RE::UI::GetSingleton()) {
            const std::pair<std::string_view, std::uint8_t> menus[]{
                { "MainMenu", 0x01u },
                { "DataMenu", 0x02u },
                { "PauseMenu", 0x04u },
                { "LoadingMenu", 0x08u },
                { "FaderMenu", 0x10u },
            };
            for (const auto& [name, bit] : menus) {
                if (ui->IsMenuOpen(RE::BSFixedString(name))) {
                    mask = static_cast<std::uint8_t>(mask | bit);
                }
            }
        }
        g_boostpackProductionBlockingMenuMask.store(mask, std::memory_order_release);
        refreshBoostpackProductionContext("startup-menu-snapshot");
    }

    void applyBoostpackProductionTransition(
        const sds::BoostpackFeedbackUpdate& update,
        std::chrono::steady_clock::time_point when) noexcept
    {
        if (!g_haptics) {
            return;
        }

        switch (update.transition) {
        case sds::BoostpackFeedbackTransition::Started:
            if (update.ignition) {
                (void)g_haptics->emitBoostpackIgnition(when);
            }
            (void)g_haptics->startBoostpackThrust();
            break;
        case sds::BoostpackFeedbackTransition::Refreshed:
            (void)g_haptics->refreshBoostpackThrust();
            break;
        case sds::BoostpackFeedbackTransition::Stopped:
            (void)g_haptics->stopBoostpackThrust();
            break;
        case sds::BoostpackFeedbackTransition::None:
        default:
            break;
        }
    }

    void observeBoostpackProductionWwise(
        const sds::WeaponSfxWwiseObservation& observation) noexcept
    {
        if (!boostpackProductionCaptureArmed() ||
            observation.externalCount != 0u || observation.hasExternalSources) {
            return;
        }

        sds::BoostpackFeedbackUpdate update{};
        {
            std::scoped_lock lock(g_boostpackFeedbackMutex);
            if (!g_boostpackFeedbackAuthority) {
                return;
            }
            update = g_boostpackFeedbackAuthority->observeWwise(
                observation.eventId,
                observation.gameObjectId,
                observation.when);
        }

        applyBoostpackProductionTransition(update, observation.when);

        if (update.speakerCue && g_boostpackSpeakerPlayback) {
            (void)g_boostpackSpeakerPlayback->observeWwise(observation);
        }
    }

    void observeBoostpackProductionSemantic(
        std::string_view semantic,
        bool active,
        std::chrono::steady_clock::time_point when) noexcept
    {
        if (semantic != "Jump" || active) {
            return;
        }

        sds::BoostpackFeedbackUpdate update{};
        {
            std::scoped_lock lock(g_boostpackFeedbackMutex);
            if (!g_boostpackFeedbackAuthority) {
                return;
            }
            update = g_boostpackFeedbackAuthority->observeJumpRelease(when);
        }

        applyBoostpackProductionTransition(update, when);
    }

    void tickBoostpackProduction(std::chrono::steady_clock::time_point now) noexcept
    {
        sds::BoostpackFeedbackUpdate update{};
        {
            std::scoped_lock lock(g_boostpackFeedbackMutex);
            if (!g_boostpackFeedbackAuthority) {
                return;
            }
            update = g_boostpackFeedbackAuthority->tick(now);
        }

        applyBoostpackProductionTransition(update, now);
    }
    void shutdownRuntime() noexcept
    {
        if (g_runtimeShutdown) {
            return;
        }
        g_runtimeShutdown = true;

        if (g_boostpackSpeakerPlayback) {
            g_boostpackSpeakerPlayback->beginShutdown();
        }
        setBoostpackProductionContextEligible(false, "shutdown-runtime");

        pluginLog("Runtime shutdown: Starfield quit requested; stopping haptics/controller workers before DLL teardown");

        g_autoRivetChargeCaptureArmed.store(false, std::memory_order_release);
        g_musicHapticsEnabled.store(false, std::memory_order_release);
        g_musicHapticsBlockingMenuMask.store(0u, std::memory_order_release);
        musicHapticsClearAuthority();
        g_shipPilotActive.store(false, std::memory_order_release);
        g_onFootRefreshPending.store(false, std::memory_order_release);
        g_lastShipStateValid.store(false, std::memory_order_release);
        stopShipLaunchLandingHaptics("shutdown");

        if (g_uiSpeakerPlayback) {
            g_uiSpeakerPlayback->beginShutdown();
        }
        if (g_audioCapture) {
            g_audioCapture->setUiAudioPlaybackArmed(false);
            g_audioCapture->setMusicReconArmed(false);
            g_audioCapture->setMusicSelectionArmed(false);
        }

        if (g_weaponSfxDiscovery) {
            sds::GameEvent shutdown{};
            shutdown.type = sds::GameEventType::Shutdown;
            shutdown.when = std::chrono::steady_clock::now();
            g_weaponSfxDiscovery->observeGameEvent(shutdown);
        }

        if (g_weaponSpeakerPlayback) {
            sds::GameEvent shutdown{};
            shutdown.type = sds::GameEventType::Shutdown;
            shutdown.when = std::chrono::steady_clock::now();
            (void)g_weaponSpeakerPlayback->observeGameEvent(shutdown);
        }

        if (g_audioCapture) {
            g_audioCapture->drainDiagnostics();
            if (g_uiAudioDiscovery) {
                g_uiAudioDiscovery->noteDroppedWwise(g_audioCapture->takeUiAudioDropped());
            }
            if (g_musicRecon) {
                g_musicRecon->noteDroppedWwise(g_audioCapture->takeMusicReconDropped());
            }
        }
        if (g_musicRecon) {
            try {
                sds::GameEvent shutdown{};
                shutdown.type = sds::GameEventType::Shutdown;
                shutdown.when = std::chrono::steady_clock::now();
                g_musicRecon->observeGameEvent(shutdown);
                pluginLog(g_musicRecon->finalize(shutdown.when));
                for (auto& line : g_musicRecon->takeDiagnostics(4096u)) {
                    pluginLog(line);
                }
            } catch (...) {
                pluginLog("Music recon: finalization exception ignored; shutdown ordering unchanged");
            }
        }
        if (g_uiAudioDiscovery) {
            for (auto& line : g_uiAudioDiscovery->finalize(std::chrono::steady_clock::now())) {
                pluginLog(line);
            }
        }
        if (g_audioCapture) {
            g_audioCapture->setUiAudioDiscoveryArmed(false);
            g_audioCapture->setUiAudioPlaybackArmed(false);
            g_audioCapture->setMusicReconArmed(false);
            g_audioCapture->setMusicSelectionArmed(false);
            g_audioCapture->stop();
        }

        if (g_fireMarkerBridge) {
            g_fireMarkerBridge->stop();
        }

        if (g_gameState) {
            g_gameState->unregisterSinks();
        }

        if (g_weaponAudioPipeline) {
            g_weaponAudioPipeline->stop();
        }
        g_boostpackSpeakerPlayback.reset();
        g_boostpackSpeakerPreparedCache.reset();
        {
            std::scoped_lock lock(g_boostpackFeedbackMutex);
            g_boostpackFeedbackAuthority.reset();
        }
        g_boostpackProductionContextEligible.store(false, std::memory_order_release);

        if (g_mainMenuUiAudioPipeline) {
            g_mainMenuUiAudioPipeline->stop();
        }

        if (g_haptics) {
            sds::GameEvent shutdown{};
            shutdown.type = sds::GameEventType::Shutdown;
            shutdown.when = std::chrono::steady_clock::now();
            (void)g_haptics->handle(std::move(shutdown));
        }

        if (g_controller) {
            g_controller->stop();
        }

        if (g_speakerManager) {
            g_speakerManager->stop();
        }

        if (g_haptics) {
            g_haptics->stop();
        }

        sds::HidWriteTrace::stop();
        pluginLog("Runtime shutdown: haptics/controller workers stopped");
    }

    void shutdownReconnectFixOnlyRuntime() noexcept
    {
        if (g_runtimeShutdown) {
            return;
        }
        g_runtimeShutdown = true;
        pluginLog(
            "Reconnect-fix-only shutdown: stopping passive controller presence monitor");
        if (g_controller) {
            g_controller->stop();
        }
        pluginLog(
            "Reconnect-fix-only shutdown: passive controller presence monitor stopped");
    }

    void reconnectFixOnlyRuntimeTick()
    {
        if (g_runtimeShutdown) {
            return;
        }

        if (auto* main = RE::Main::GetSingleton(); main && main->quitGame) {
            shutdownReconnectFixOnlyRuntime();
            return;
        }

        updateNativeDualSenseReselection(std::chrono::steady_clock::now());
    }

    void runtimeTick()
    {
        if (g_runtimeShutdown) {
            return;
        }

        if (auto* main = RE::Main::GetSingleton(); main && main->quitGame) {
            shutdownRuntime();
            return;
        }

        updateNativeDualSenseReselection(std::chrono::steady_clock::now());

        if (g_controller) {
            const bool connected = g_controller->connected();
            const bool previous =
                g_boostpackProductionControllerConnected.exchange(
                    connected,
                    std::memory_order_acq_rel);
            if (previous != connected) {
                refreshBoostpackProductionContext(
                    connected ? "controller-reconnected" : "controller-disconnected");
            }
        }

        if (g_controller && g_gameState) {
            while (const auto action = g_controller->tryPopInputAction()) {
                (void)g_gameState->queueNativeInputAction(*action);
            }
        }

        bootstrapStartupEquippedWeaponIfReady();

        if (!g_shipPilotActive.load(std::memory_order_acquire) &&
            g_onFootRefreshPending.load(std::memory_order_acquire)) {
            bool loadingMenuOpen = false;
            if (auto* ui = RE::UI::GetSingleton()) {
                loadingMenuOpen = ui->IsMenuOpen(RE::BSFixedString("LoadingMenu"));
            }

            if (!loadingMenuOpen &&
                g_onFootRefreshPending.exchange(false, std::memory_order_acq_rel)) {
                const bool weaponRefreshed = refreshCurrentEquippedWeaponState();
                const bool healthRefreshed = g_gameState && g_gameState->refreshPlayerHealth();

                std::ostringstream diagnostic;
                diagnostic << "Ship pilot context: ON-FOOT-REFRESH weapon="
                           << (weaponRefreshed ? "fresh" : "unavailable")
                           << " health=" << (healthRefreshed ? "fresh" : "unavailable");
                pluginLog(diagnostic.str());
            }
        }

        const auto runtimeNow = std::chrono::steady_clock::now();
        if (g_haptics) {
            (void)g_haptics->tick(runtimeNow);
        }
        tickBoostpackProduction(runtimeNow);

        if (g_gameState) {
            g_gameState->pollNativeInputInjection();
            const bool pilotActiveForShipPoll = g_shipPilotActive.load(std::memory_order_acquire);
            const bool launchLandingReconEnabled =
                g_shipLaunchLandingReconEnabled.load(std::memory_order_acquire);
            const bool precisionTouchdownPoll = launchLandingReconEnabled &&
                g_shipLaunchLandingReconProbe.requiresPrecisionTouchdownPolling();

            if (precisionTouchdownPoll &&
                g_shipLaunchLandingRumblePhase.load(std::memory_order_acquire) == ShipLaunchLandingRumblePhase::None) {
                startShipLaunchLandingHaptics(ShipLaunchLandingRumblePhase::Landing);
            } else if (!precisionTouchdownPoll &&
                g_shipLaunchLandingRumblePhase.load(std::memory_order_acquire) == ShipLaunchLandingRumblePhase::Landing) {
                stopShipLaunchLandingHaptics("landing-cancelled");
            }
            maybeStartShipTakeoffRumble(pilotActiveForShipPoll, precisionTouchdownPoll);

            if (pilotActiveForShipPoll) {
                if (const auto propulsion = g_gameState->pollShipPropulsionState()) {
                    const auto propulsionWhen = std::chrono::steady_clock::now();
                    g_lastShipLanded.store(propulsion->landed, std::memory_order_release);
                    g_lastShipDocked.store(propulsion->docked, std::memory_order_release);
                    g_lastShipStateValid.store(true, std::memory_order_release);
                    if (launchLandingReconEnabled && !precisionTouchdownPoll) {
                        if (const auto boundary = g_shipLaunchLandingReconProbe.observeShipState(*propulsion, propulsionWhen)) {
                            logShipLaunchLandingBoundary(*boundary);
                            if (boundary->type == sds::ShipLaunchLandingTransition::Takeoff &&
                                g_shipLaunchLandingRumblePhase.load(std::memory_order_acquire) ==
                                    ShipLaunchLandingRumblePhase::Takeoff) {
                                g_shipTakeoffBoundaryObserved.store(true, std::memory_order_release);
                                pluginLog("Ship launch/landing haptics: takeoff-boundary-observed action=continue-until-next-transition-boundary");
                            }
                        }
                    }
                    if (g_haptics) {
                        (void)g_haptics->handleShipPropulsionState(*propulsion);
                    }
                }
            }

            if (precisionTouchdownPoll) {
                if (const auto landingPrecision = g_gameState->pollShipLandingReconStatePrecision()) {
                    if (const auto boundary =
                        g_shipLaunchLandingReconProbe.observeShipState(landingPrecision->state, landingPrecision->when)) {
                        logShipLaunchLandingBoundary(*boundary);
                        if (boundary->type == sds::ShipLaunchLandingTransition::Touchdown) {
                            pluginLog("Ship launch/landing haptics: landing-touchdown-observed action=landing-sequence-end");
                            stopShipLaunchLandingHaptics("landing-sequence-end");
                            dispatchShipTouchdownHaptic(*boundary, landingPrecision->when);
                        }
                    }
                    if (g_shipLaunchLandingRumblePhase.load(std::memory_order_acquire) == ShipLaunchLandingRumblePhase::Landing &&
                        !g_shipLaunchLandingReconProbe.requiresPrecisionTouchdownPolling()) {
                        stopShipLaunchLandingHaptics("landing-cancelled");
                    }
                }
            } else if (!pilotActiveForShipPoll && launchLandingReconEnabled &&
                g_shipLaunchLandingReconProbe.requiresWwiseCapture()) {
                if (const auto landingDiagnostic = g_gameState->pollShipLandingReconState()) {
                    const auto diagnosticWhen = std::chrono::steady_clock::now();
                    if (const auto boundary =
                        g_shipLaunchLandingReconProbe.observeShipState(*landingDiagnostic, diagnosticWhen)) {
                        logShipLaunchLandingBoundary(*boundary);
                    }
                }
            }

            if (g_shipLaunchLandingRumblePhase.load(std::memory_order_acquire) == ShipLaunchLandingRumblePhase::Takeoff) {
                const auto now = std::chrono::steady_clock::now();
                const auto startedAtUs = g_shipLaunchLandingStartedAtUs.load(std::memory_order_acquire);
                const bool timedOut = startedAtUs > 0 &&
                    steadyMicros(now) - startedAtUs >=
                        std::chrono::duration_cast<std::chrono::microseconds>(kShipTakeoffHapticsMax).count();
                if (!pilotActiveForShipPoll) {
                    stopShipLaunchLandingHaptics("takeoff-pilot-lost");
                } else if (g_shipBallisticBlockingMenuMask.load(std::memory_order_acquire) != 0) {
                    stopShipLaunchLandingHaptics("takeoff-blocked");
                } else if (timedOut) {
                    stopShipLaunchLandingHaptics("takeoff-timeout");
                }
            }
            (void)g_gameState->pollLandVehicleReconState();
            g_gameState->pollHealth();
        }

        if (g_audioCapture) {
            bool dialogueMenuActive = false;
            if (auto* ui = RE::UI::GetSingleton()) {
                dialogueMenuActive = ui->IsMenuOpen(RE::BSFixedString("DialogueMenu"));
            }
            g_audioCapture->setDialogueMenuActive(dialogueMenuActive);
            const bool pilotActive = g_shipPilotActive.load(std::memory_order_acquire);
            const bool launchLandingReconCaptureArmed =
                g_shipLaunchLandingReconEnabled.load(std::memory_order_acquire) &&
                (pilotActive || g_shipLaunchLandingReconProbe.requiresWwiseCapture());
            const bool landVehicleReconCaptureArmed =
                g_gameState && g_gameState->landVehicleReconCorrelationArmed();
            const bool boostpackProductionArmed = boostpackProductionCaptureArmed();
            const bool shipCaptureArmed =
                launchLandingReconCaptureArmed || landVehicleReconCaptureArmed ||
                boostpackProductionArmed ||
                (pilotActive && g_shipWeaponSemanticCache && g_shipWeaponSemanticCache->ready());
            g_audioCapture->setShipWeaponObservationArmed(shipCaptureArmed);
            g_audioCapture->drainDiagnostics();
            if (g_musicRecon) {
                try {
                    g_musicRecon->noteDroppedWwise(g_audioCapture->takeMusicReconDropped());
                    for (auto& request : g_musicRecon->takeResolveRequests(32u)) {
                        if (!g_weaponAudioPipeline ||
                            !g_weaponAudioPipeline->tryEnqueueMusicRecon(std::move(request))) {
                            // Diagnostic resolution loss is non-fatal and never changes gameplay/controller behavior.
                        }
                    }
                } catch (...) {
                    pluginLog("Music recon: runtime handoff exception ignored; controller behavior unchanged");
                }
            }
            if (g_weaponSfxDiscovery) {
                g_weaponSfxDiscovery->noteDroppedWwise(g_audioCapture->takeWeaponSfxDropped());
                for (auto& report : g_weaponSfxDiscovery->takeReadyReports(std::chrono::steady_clock::now())) {
                    if (g_weaponAudioPipeline) {
                        (void)g_weaponAudioPipeline->tryEnqueueDiscovery(std::move(report));
                    }
                }
            }
            if (g_uiAudioDiscovery) {
                g_uiAudioDiscovery->noteDroppedWwise(g_audioCapture->takeUiAudioDropped());
                for (auto& line : g_uiAudioDiscovery->takeReadyDiagnostics(std::chrono::steady_clock::now())) {
                    pluginLog(line);
                }
                g_audioCapture->setUiAudioDiscoveryArmed(g_uiAudioDiscovery->captureArmed());
            }
            g_audioCapture->setUiAudioPlaybackArmed(
                (g_uiSpeakerPlayback && g_uiSpeakerPlayback->armed()) ||
                g_haptics != nullptr);
        }

        if (g_weaponAudioPipeline && g_musicHapticsEnabled.load(std::memory_order_acquire)) {
            try {
                for (auto& result : g_weaponAudioPipeline->tryTakeMusicHapticsResults(32u)) {
                    if (!result.ready || !result.pcm || result.pcm->frames.empty() ||
                        musicHapticsBlocked() || !musicHapticsPlayingIdActive(result.playingId)) {
                        continue;
                    }

                    const auto now = std::chrono::steady_clock::now();
                    const auto ageMicros = (std::max)(
                        std::int64_t{ 0 },
                        std::chrono::duration_cast<std::chrono::microseconds>(now - result.selectedAt).count());
                    const auto startFrame = static_cast<std::size_t>(
                        (static_cast<std::uint64_t>(ageMicros) * kMusicHapticsSampleRate) / 1000000u);
                    if (startFrame >= result.pcm->frames.size() || !g_audioTransport) {
                        continue;
                    }

                    (void)g_audioTransport->enqueueMusicHaptic(sds::MusicHapticVoice{
                        .playingId = result.playingId,
                        .eventId = result.eventId,
                        .mediaId = result.mediaId,
                        .pcm = std::move(result.pcm),
                        .startFrame = startFrame,
                        .gain = (g_musicHapticsStrength.load(std::memory_order_acquire)) * g_musicHapticsUserScale.load(std::memory_order_acquire),
                    });
                }
            } catch (...) {
                pluginLog("Music haptics: prepared-voice handoff exception ignored; gameplay haptics unchanged");
            }
        }

        if (g_musicRecon) {
            try {
                if (g_weaponAudioPipeline) {
                    for (auto& result : g_weaponAudioPipeline->tryTakeMusicReconResults(32u)) {
                        g_musicRecon->observeResolved(std::move(result));
                    }
                }
                for (auto& line : g_musicRecon->takeDiagnostics(64u)) {
                    pluginLog(line);
                }
            } catch (...) {
                pluginLog("Music recon: runtime handoff exception ignored; controller behavior unchanged");
            }
        }

        if (g_shipEmReconEnabled.load(std::memory_order_acquire)) {
            if (const auto summary = g_shipEmReconProbe.takeReadySummary(std::chrono::steady_clock::now())) {
                logShipEmReconSummary(*summary);
            }
        }

        if (g_shipLaunchLandingReconEnabled.load(std::memory_order_acquire)) {
            while (const auto report = g_shipLaunchLandingReconProbe.takeReadyReport(std::chrono::steady_clock::now())) {
                logShipLaunchLandingReconReport(*report);
            }
        }

        if (g_mainMenuUiAudioPipeline) {
            for (auto& line : g_mainMenuUiAudioPipeline->tryTakeDiagnostics(16u)) {
                pluginLog(std::string("Main-menu prewarm: ") + line);
            }
        }

        if (g_weaponAudioPipeline) {
            for (auto& line : g_weaponAudioPipeline->tryTakeDiagnostics(32u)) {
                pluginLog(line);
            }
        }

    }

    void initializeRuntime()
    {
        if (g_runtimeInitialized) {
            return;
        }
        g_runtimeInitialized = true;

        const auto config =
            g_startupConfig.has_value() ?
                *g_startupConfig :
                loadRuntimeConfig();

        g_nativeDualSenseReconnectFixEnabled.store(
            config.operatingMode == sds::OperatingMode::ReconnectFixOnly ||
                config.dualSenseReconnectFix,
            std::memory_order_release);

        if (config.operatingMode == sds::OperatingMode::ReconnectFixOnly) {
            auto nativeLog = [](std::string_view message) { pluginLog(message); };

            g_controller = std::make_unique<sds::ControllerManager>(
                config,
                [nativeLog] {
                    return std::make_unique<sds::NativeUsbBackend>(nativeLog, false, true);
                },
                nativeLog,
                std::chrono::milliseconds(250),
                std::chrono::milliseconds(50),
                sds::ControllerManager::RightTriggerObserver{},
                sds::ControllerRuntimeMode::PresenceOnly);
            g_controller->start();

            pluginLog(
                "Runtime mode: ReconnectFixOnly ACTIVE systems=passive-DualSense-presence+native-reselection "
                "adaptiveTriggers=off haptics=off audio=off speaker=off lightbar=off touchpad=off "
                "gameState=off Wwise=off weapon=off ship=off REV8=off boostpack=off");

            if (const auto* tasks = SFSE::GetTaskInterface()) {
                tasks->AddPermanentTask(reconnectFixOnlyRuntimeTick);
                pluginLog(
                    "Reconnect fix: SFSE permanent task ACTIVE policy=one-native-reselection-per-physical-reconnect");
            } else {
                pluginLog(
                    "Reconnect fix: INACTIVE reason=SFSE-task-interface-unavailable; stopping passive presence monitor");
                g_controller->stop();
            }
            return;
        }

        const bool musicReconEnabled = config.debugLogging;
        const bool musicHapticsEnabled = config.advancedHaptics && config.musicHapticsEnabled;
        const bool musicSelectionEnabled = musicReconEnabled || musicHapticsEnabled;
        g_musicHapticsEnabled.store(musicHapticsEnabled, std::memory_order_release);
        g_musicHapticsStrength.store(config.hapticStrength, std::memory_order_release);
        g_musicHapticsUserScale.store(config.musicHapticsStrength, std::memory_order_release);
        if (musicReconEnabled) {
            g_musicRecon = std::make_unique<sds::MusicReconProbe>();
        } else {
            g_musicRecon.reset();
        }
        // Keep producer/cache ownership stable across live speaker toggles.
        // ControllerSpeakerManager is the runtime policy authority.
        const bool uiSpeakerPlaybackEnabled = true;
        g_weaponAudioPipelineEnabled =
            config.operatingMode == sds::OperatingMode::Full;
        const bool uiAudioDiscoveryEnabled = config.debugLogging;
        const bool shipBallisticHapticsEnabled = config.adaptiveTriggers || config.advancedHaptics;
        const bool shipLaserHapticsEnabled = shipBallisticHapticsEnabled;
        const bool shipParticleHapticsEnabled = shipBallisticHapticsEnabled;
        const bool shipMissileHapticsEnabled = shipBallisticHapticsEnabled;
        const bool shipEMHapticsEnabled = shipBallisticHapticsEnabled;
        const bool shipTouchdownHapticsEnabled = config.advancedHaptics;
        const bool shipEmReconEnabled = false;
        const bool shipLaunchLandingReconEnabled = true;
        const bool landVehicleReconEnabled = true;
        g_shipEmReconEnabled.store(shipEmReconEnabled, std::memory_order_release);
        g_shipLaunchLandingReconEnabled.store(shipLaunchLandingReconEnabled, std::memory_order_release);
        g_shipEmReconProbe.setPilotActive(false);
        g_shipLaunchLandingReconProbe.setPilotActive(false);
        g_shipLaserFireGate.setPilotActive(false);
        g_shipParticleFireGate.setPilotActive(false);
        g_shipMissileFireGate.setPilotActive(false);
        g_shipEMFireGate.setPilotActive(false);
        if (shipBallisticHapticsEnabled) {
            g_shipWeaponSemanticCache = std::make_shared<sds::ShipWeaponSemanticCache>();
        } else {
            g_shipWeaponSemanticCache.reset();
        }
        g_shipBallisticFireGate.setPilotActive(false);
        const bool boostpackProductionEnabled =
            config.operatingMode == sds::OperatingMode::Full;
        const bool sharedAudioPreparationEnabled =
            uiSpeakerPlaybackEnabled || g_weaponAudioPipelineEnabled || uiAudioDiscoveryEnabled ||
            musicSelectionEnabled || shipBallisticHapticsEnabled || shipLaunchLandingReconEnabled ||
            landVehicleReconEnabled || boostpackProductionEnabled;
        if (sharedAudioPreparationEnabled) {
            const auto executablePath = currentExecutablePath();
            if (!executablePath.empty()) {
                g_weaponAudioPipelineDataPath = executablePath.parent_path() / "Data";
            }
        }
        // Keep the remote-VO observation path registered for process lifetime.
        // The live Comms gate below rejects before any filesystem/decode work.
        const bool remoteVoCaptureEnabled = true;
        const bool weaponSpeakerCaptureEnabled = g_weaponAudioPipelineEnabled;
        const bool autoRivetChargeCaptureEnabled = config.advancedHaptics;
        g_autoRivetChargeCaptureEnabled.store(
            autoRivetChargeCaptureEnabled,
            std::memory_order_release);
        g_autoRivetChargeCaptureArmed.store(false, std::memory_order_release);
        const bool weaponSfxDiscoveryEnabled =
            weaponSpeakerCaptureEnabled && !kWeaponSfxDiscoveryTargets.empty();
        if (weaponSfxDiscoveryEnabled) {
            g_weaponSfxDiscovery = std::make_unique<sds::WeaponSfxDiscoveryProbe>(kWeaponSfxDiscoveryTargets);
        } else {
            g_weaponSfxDiscovery.reset();
        }

        if (uiAudioDiscoveryEnabled) {
            g_uiAudioDiscovery = std::make_unique<sds::UiAudioDiscoveryProbe>();
        } else {
            g_uiAudioDiscovery.reset();
            if (config.debugLogging) {
                pluginLog("UI audio discovery: INACTIVE reason=DebugLogging-disabled broadCapture=no");
            }
        }

        REX::INFO(
            "StarfieldDualSense {} runtime start | SFSE {} | triggers={} lightbar={} touchpad={} debug={}",
            kVersion,
            SFSE::GetSFSEVersion(),
            config.adaptiveTriggers,
            config.lightbar,
            config.touchpad,
            config.debugLogging);

        if (!musicReconEnabled) {
            pluginLog("Music recon: INACTIVE reason=DebugLogging-disabled");
        }
        pluginLog("Land vehicle state: ACTIVE source=AIProcess.occupiedFurniture authority=tier-a-hardware-validated camera=kVehicle-corroboration-only positionTelemetry=reference-world-finite-difference physics=boost-armed-airborne+descent+touchdown touchdownAuthority=accepted-vertical-boost+vertical-motion WwiseContact=0xDC42D80F,0xA3BB6A5E gunEvent=0x3DD3DADD verticalBoostEvent=0xF6A67354 controllerOutput=haptics+triggers");
        pluginLog("REV-8 controller feel: ACTIVE authority=tier-a-occupiedFurniture motion=telemetry-load-biased boost=0xF6A67354 gun=VehicleFireWeapon+0x3DD3DADD aim=VehicleAim touchdown=boost-armed-physics haptics=chassis+boost+gun+touchdown triggers=R2-gun+L2-aim speaker=none lightbar=none");
        resetLandVehicleWwiseRecon(std::chrono::steady_clock::now());

        auto nativeLog = [](std::string_view message) { pluginLog(message); };
        g_audioTransport = std::make_shared<sds::DualSenseAudioTransport>(
            nativeLog,
            config.debugLogging,
            config.speakerVolume);
        g_haptics = std::make_unique<sds::HapticsManager>(
            config,
            [transport = g_audioTransport] {
                return std::make_unique<sds::DualSenseAudioHapticsClient>(transport);
            },
            nativeLog);
        g_haptics->start();
        initializeMusicHapticsMenuMask();
        if (musicHapticsEnabled) {
            pluginLog("Music haptics: ACTIVE authority=Starfield_MUS-selected-media callback=AK_Duration exactMedia=yes workerDecode=lazy transport=shared-WASAPI fadeMs=350 peakCap=0.65 stereoCrossfeed=85/15 gameplayPriority=sidechain menuMute=MainMenu,DataMenu,PauseMenu,LoadingMenu,FaderMenu wholeGameMix=no");
        } else {
            pluginLog("Music haptics: INACTIVE reason=AdvancedHaptics-or-MusicHapticsEnabled-disabled wholeGameMix=no");
        }

        auto speakerBackend = std::make_unique<sds::DualSenseAudioSpeakerClient>(g_audioTransport);
        g_speakerManager = std::make_unique<sds::ControllerSpeakerManager>(
            config,
            std::move(speakerBackend),
            nativeLog);
        // start() records stable ownership even when startup master is OFF;
        // Task 1 live-enable reuses this exact backend object later.
        g_speakerManager->start();

        if (boostpackProductionEnabled) {
            {
                std::scoped_lock lock(g_boostpackFeedbackMutex);
                g_boostpackFeedbackAuthority =
                    std::make_unique<sds::BoostpackFeedbackAuthority>();
            }
            g_boostpackSpeakerPreparedCache =
                std::make_shared<sds::BoostpackSpeakerPreparedCache>();
            g_boostpackSpeakerPlayback =
                std::make_unique<sds::BoostpackSpeakerPlayback>(
                    [](const sds::PreparedSpeakerPcm& pcm,
                       std::uint32_t eventId,
                       std::uint32_t mediaId) {
                        if (!g_speakerManager) {
                            return false;
                        }
                        return g_speakerManager->submitCaptured(
                            pcm,
                            sds::SpeakerCategory::Boostpack,
                            {
                                .id = (static_cast<std::uint64_t>(eventId) << 32u) |
                                    static_cast<std::uint64_t>(mediaId),
                                .controllerOnlySafe = false,
                            },
                            false);
                    },
                    g_boostpackSpeakerPreparedCache);
            g_boostpackProductionControllerConnected.store(
                g_controller && g_controller->connected(),
                std::memory_order_release);
            g_boostpackProductionLandVehicleActive.store(false, std::memory_order_release);
            initializeBoostpackProductionMenuMask();
            pluginLog(
                "Boostpack feedback: ACTIVE authority=exact-Wwise-0x1BE06B49 playerObject=0x2 "
                "zeroExternal=yes haptics=ignition+thrust speaker=real-Starfield-WEM");
        } else {
            g_boostpackSpeakerPlayback.reset();
            g_boostpackSpeakerPreparedCache.reset();
            {
                std::scoped_lock lock(g_boostpackFeedbackMutex);
                g_boostpackFeedbackAuthority.reset();
            }
            g_boostpackProductionContextEligible.store(false, std::memory_order_release);
            g_boostpackProductionControllerConnected.store(false, std::memory_order_release);
            g_boostpackProductionLandVehicleActive.store(false, std::memory_order_release);
            g_boostpackProductionBlockingMenuMask.store(0u, std::memory_order_release);
            pluginLog("Boostpack feedback: INACTIVE reason=OperatingMode-ReconnectFixOnly");
        }
        refreshBoostpackSemanticArming();

        if (config.debugLogging) {
            std::ostringstream speakerConfigLine;
            speakerConfigLine << "Controller speaker config: masterVolume=" << std::fixed << std::setprecision(2)
                              << config.speakerVolume
                              << " weapons=" << (config.speakerWeapons ? "enabled" : "disabled")
                              << " weaponsVolume=" << config.speakerWeaponsVolume;
            pluginLog(speakerConfigLine.str());
        }

        if (uiSpeakerPlaybackEnabled) {
            if (!g_uiSpeakerPreparedCache) {
                g_uiSpeakerPreparedCache = std::make_shared<sds::UiSpeakerPreparedCache>();
            }
            g_uiSpeakerPlayback = std::make_unique<sds::UiSpeakerPlayback>(
                [](const sds::PreparedSpeakerPcm& pcm,
                    std::uint32_t eventId,
                    std::uint32_t mediaId,
                    sds::SpeakerCategory category) {
                    if (!g_speakerManager) {
                        return false;
                    }
                    const sds::CapturedSoundIdentity identity{
                        .id = (static_cast<std::uint64_t>(eventId) << 32u) |
                            static_cast<std::uint64_t>(mediaId),
                        .controllerOnlySafe = false,
                    };
                    return g_speakerManager->submitCaptured(
                        pcm,
                        category,
                        identity,
                        false);
                },
                g_uiSpeakerPreparedCache,
                config,
                nativeLog);
            {
                std::ostringstream line;
                line << "UI/digipick/crafting speaker: ACTIVE cues=" << sds::uiSpeakerCueDefinitions().size()
                     << " categories=ScannerUI+Digipick+Crafting gameObject=0x3"
                     << " cadence=native-event variantPlayback=real-WEM-bounded-sequence preparation=background"
                     << " playback=additive normalGameAudio=untouched extraction=disabled";
                pluginLog(line.str());
            }
        } else {
            g_uiSpeakerPlayback.reset();
            g_uiSpeakerPreparedCache.reset();
            if (config.debugLogging) {
                pluginLog("UI/scanner speaker: INACTIVE reason=SpeakerScannerUI-or-master-disabled");
            }
        }

        if (weaponSpeakerCaptureEnabled) {
            g_weaponSpeakerPreparedCache = std::make_shared<sds::WeaponSpeakerPreparedCache>();
            g_weaponSpeakerPlayback = std::make_unique<sds::WeaponSpeakerPlayback>(
                [](const sds::PreparedSpeakerPcm& pcm,
                    std::string_view,
                    std::string_view,
                    std::uint32_t eventId,
                    std::uint32_t mediaId,
                    std::uint8_t) {
                    if (!g_speakerManager) {
                        return false;
                    }
                    const sds::CapturedSoundIdentity identity{
                        .id = (static_cast<std::uint64_t>(eventId) << 32u) |
                            static_cast<std::uint64_t>(mediaId),
                        .controllerOnlySafe = false,
                    };
                    return g_speakerManager->submitCaptured(
                        pcm,
                        sds::SpeakerCategory::Weapons,
                        identity,
                        false);
                },
                [](sds::PersistentPreparedSpeakerPcm voice,
                    std::string_view,
                    std::uint32_t,
                    std::uint32_t,
                    std::uint8_t) {
                    return g_speakerManager && g_speakerManager->setPersistentCaptured(
                        std::move(voice),
                        sds::SpeakerCategory::Weapons);
                },
                [](std::uint64_t owner, bool force) {
                    return g_speakerManager && g_speakerManager->clearPersistentCaptured(owner, force);
                },
                nativeLog,
                g_weaponSpeakerPreparedCache,
                true);

            g_audioTransport->setSpeakerPersistentInvalidationCallback(
                [](sds::SpeakerPersistentInvalidationReason reason) {
                    if (g_weaponSpeakerPlayback) {
                        g_weaponSpeakerPlayback->observeBackendInvalidation(reason);
                    }
                });
        }

        sds::ControllerManager::RightTriggerObserver rightTriggerObserver{};
        if (config.advancedHaptics || config.adaptiveTriggers || g_weaponSpeakerPlayback) {
            rightTriggerObserver = [](std::uint8_t r2, std::chrono::steady_clock::time_point when) {
                if (g_gameState && g_gameState->landVehicleReconCorrelationArmed()) {
                    std::ostringstream line;
                    line << "Land vehicle recon: input control=R2 raw=" << static_cast<unsigned>(r2)
                         << " state=" << (r2 > 12u ? "active" : "release")
                         << " role=annotation-only authority=no controllerOutput=none";
                    pluginLog(line.str());
                }
                const auto deferredFire = g_shipBallisticFireGate.observeRightTrigger(r2, when);
                const auto laserObservation = g_shipLaserFireGate.observeRightTrigger(r2, when);
                const auto particleObservation = g_shipParticleFireGate.observeRightTrigger(r2, when);
                const auto missileObservation = g_shipMissileFireGate.observeRightTrigger(r2, when);
                const auto emObservation = g_shipEMFireGate.observeRightTrigger(r2, when);
                if (g_haptics) {
                    (void)g_haptics->handleRightTriggerInput(r2, when);
                }
                if (deferredFire.authorized && g_eventRouter) {
                    sds::GameEvent semantic{};
                    semantic.type = sds::GameEventType::ShipBallisticWeaponFired;
                    // A forward-correlated first shot cannot be delivered in the past.
                    // Timestamp the finite effect at the real R2 proof that releases it.
                    semantic.when = when;
                    (void)g_eventRouter->dispatch(std::move(semantic));

                    std::ostringstream line;
                    line << "Ship ballistic fire: event=0x" << std::uppercase << std::hex
                         << std::setw(8) << std::setfill('0') << deferredFire.eventId
                         << " gameObject=0x" << deferredFire.gameObjectId
                         << std::dec << std::setfill(' ')
                         << " family=ballistic control=R2 source=player-Wwise+first-shot-forward-correlation"
                         << " semanticSource=r6-first-shot-forward-correlation"
                         << " ageUs=" << deferredFire.ageMicros;
                    pluginLog(line.str());
                }
                if (laserObservation.deferredFire.authorized && g_eventRouter) {
                    sds::GameEvent semantic{};
                    semantic.type = sds::GameEventType::ShipLaserWeaponFired;
                    semantic.when = when;
                    (void)g_eventRouter->dispatch(std::move(semantic));

                    std::ostringstream line;
                    line << "Ship laser fire: event=0x" << std::uppercase << std::hex
                         << std::setw(8) << std::setfill('0') << laserObservation.deferredFire.eventId
                         << " gameObject=0x" << laserObservation.deferredFire.gameObjectId
                         << std::dec << std::setfill(' ')
                         << " family=laser control=R2 source=player-Wwise+first-shot-forward-correlation"
                         << " semanticSource=v0365-first-shot-forward-correlation"
                         << " ageUs=" << laserObservation.deferredFire.ageMicros;
                    pluginLog(line.str());
                }
                if (laserObservation.stopped && g_eventRouter) {
                    sds::GameEvent semantic{};
                    semantic.type = sds::GameEventType::ShipLaserWeaponStopped;
                    semantic.when = when;
                    (void)g_eventRouter->dispatch(std::move(semantic));
                    pluginLog("Ship laser fire: STOP source=physical-R2-release semanticSource=v0365-stream-revoke");
                }
                if (particleObservation.authorized && g_eventRouter) {
                    sds::GameEvent semantic{};
                    semantic.type = sds::GameEventType::ShipParticleWeaponFired;
                    semantic.when = when;
                    (void)g_eventRouter->dispatch(std::move(semantic));

                    std::ostringstream line;
                    line << "Ship particle fire: event=0x" << std::uppercase << std::hex
                         << std::setw(8) << std::setfill('0') << particleObservation.eventId
                         << " gameObject=0x" << particleObservation.gameObjectId
                         << std::dec << std::setfill(' ')
                         << " family=particle control=R2 source=player-Wwise+first-shot-forward-correlation"
                         << " semanticSource=v0367-first-shot-forward-correlation"
                         << " ageUs=" << particleObservation.ageMicros;
                    pluginLog(line.str());
                }
                if (missileObservation.authorized && g_eventRouter) {
                    sds::GameEvent semantic{};
                    semantic.type = sds::GameEventType::ShipMissileWeaponFired;
                    semantic.when = when;
                    (void)g_eventRouter->dispatch(std::move(semantic));

                    std::ostringstream line;
                    line << "Ship missile fire: event=0x" << std::uppercase << std::hex
                         << std::setw(8) << std::setfill('0') << missileObservation.eventId
                         << " gameObject=0x" << missileObservation.gameObjectId
                         << std::dec << std::setfill(' ')
                         << " family=missile control=R2 source=player-Wwise+first-shot-forward-correlation"
                         << " semanticSource=v0369-first-shot-forward-correlation"
                         << " ageUs=" << missileObservation.ageMicros;
                    pluginLog(line.str());
                }
                if (emObservation.authorized && g_eventRouter) {
                    sds::GameEvent semantic{};
                    semantic.type = sds::GameEventType::ShipEMWeaponFired;
                    semantic.when = when;
                    (void)g_eventRouter->dispatch(std::move(semantic));

                    std::ostringstream line;
                    line << "Ship EM fire: event=0x" << std::uppercase << std::hex
                         << std::setw(8) << std::setfill('0') << emObservation.eventId
                         << " gameObject=0x" << emObservation.gameObjectId
                         << std::dec << std::setfill(' ')
                         << " family=em control=R2 source=player-Wwise+first-shot-forward-correlation"
                         << " semanticSource=v0371-first-shot-forward-correlation"
                         << " ageUs=" << emObservation.ageMicros;
                    pluginLog(line.str());
                }
                if (g_shipEmReconEnabled.load(std::memory_order_acquire)) {
                    const auto reconTrigger = g_shipEmReconProbe.observeRightTrigger(r2, when);
                    if (reconTrigger.pressed || reconTrigger.released) {
                        std::ostringstream line;
                        line << "Ship EM recon: trigger burst=" << reconTrigger.burstId
                             << " action=" << (reconTrigger.pressed ? "press" : "release")
                             << " r2=" << static_cast<unsigned>(r2)
                             << " recoveredPrePress=" << reconTrigger.prePressSamples.size()
                             << " diagnosticOnly=yes";
                        pluginLog(line.str());
                    }
                    for (const auto& sample : reconTrigger.prePressSamples) {
                        logShipEmReconSample(sample);
                    }
                }
                if (g_weaponSpeakerPlayback) {
                    g_weaponSpeakerPlayback->observeRightTrigger(r2, when);
                }
            };
        }

        g_controller = std::make_unique<sds::ControllerManager>(
            config,
            [nativeLog] { return std::make_unique<sds::NativeUsbBackend>(nativeLog, false); },
            nativeLog,
            std::chrono::milliseconds(2000),
            std::chrono::milliseconds(8),
            std::move(rightTriggerObserver));
        g_controller->start();

        g_eventRouter = std::make_unique<sds::RuntimeEventRouter>(
            [](sds::GameEvent event) {
                return g_controller && g_controller->enqueue(std::move(event));
            },
            [](sds::GameEvent event) {
                if (g_weaponSpeakerPlayback && event.type != sds::GameEventType::WeaponEquipped) {
                    (void)g_weaponSpeakerPlayback->observeGameEvent(event);
                }
                if (g_weaponSfxDiscovery && event.type != sds::GameEventType::WeaponEquipped) {
                    g_weaponSfxDiscovery->observeGameEvent(event);
                }
                auto speakerCopy = event;
                const bool hapticsAccepted = !g_haptics || g_haptics->handle(std::move(event));
                if (g_speakerManager) {
                    (void)g_speakerManager->handle(std::move(speakerCopy));
                }
                return hapticsAccepted;
            },
            nativeLog);

        auto runtimeEmit = [](sds::GameEvent event) {
            if (g_musicRecon) {
                try {
                    g_musicRecon->observeGameEvent(event);
                } catch (...) {
                    pluginLog("Music recon: game-event observer exception ignored; runtime routing unchanged");
                }
            }
            if (event.type == sds::GameEventType::MenuOpened ||
                event.type == sds::GameEventType::MenuClosed) {
                if (g_uiSpeakerPlayback) {
                    g_uiSpeakerPlayback->observeGameEvent(event);
                }
                const auto menu = std::string_view(event.text.data());
                syncLandVehicleActionGateForMenu(
                    menu, event.type == sds::GameEventType::MenuOpened);
                syncMusicHapticsForMenu(
                    menu, event.type == sds::GameEventType::MenuOpened);
                syncBoostpackProductionForMenu(
                    menu, event.type == sds::GameEventType::MenuOpened);
                if (g_shipLaunchLandingReconEnabled.load(std::memory_order_acquire)) {
                    g_shipLaunchLandingReconProbe.observeMenu(
                        menu, event.type == sds::GameEventType::MenuOpened, event.when);
                }
                if (g_shipLaunchLandingRumblePhase.load(std::memory_order_acquire) ==
                    ShipLaunchLandingRumblePhase::Takeoff) {
                    if (event.type == sds::GameEventType::MenuClosed &&
                        eventIdentity(event) == "FaderMenu") {
                        if (!g_shipTakeoffFirstFaderClosed.exchange(true, std::memory_order_acq_rel)) {
                            pluginLog("Ship launch/landing haptics: takeoff-first-fader-closed action=continue");
                        }
                    } else if (event.type == sds::GameEventType::MenuOpened &&
                               eventIdentity(event) == "FaderMenu" &&
                               g_shipTakeoffFirstFaderClosed.load(std::memory_order_acquire)) {
                        stopShipLaunchLandingHaptics("takeoff-next-fader-open");
                    } else if (event.type == sds::GameEventType::MenuOpened &&
                               eventIdentity(event) == "LoadingMenu" &&
                               g_shipTakeoffFirstFaderClosed.load(std::memory_order_acquire)) {
                        stopShipLaunchLandingHaptics("takeoff-loading-fallback");
                    }
                }
                std::uint8_t bit = 0;
                if (menu == "DataMenu") {
                    bit = 0x01u;
                } else if (menu == "PauseMenu") {
                    bit = 0x02u;
                }
                if (bit != 0) {
                    std::uint8_t mask = 0;
                    if (event.type == sds::GameEventType::MenuOpened) {
                        mask = static_cast<std::uint8_t>(
                            g_shipBallisticBlockingMenuMask.fetch_or(bit, std::memory_order_acq_rel) | bit);
                    } else {
                        mask = static_cast<std::uint8_t>(
                            g_shipBallisticBlockingMenuMask.fetch_and(
                                static_cast<std::uint8_t>(~bit), std::memory_order_acq_rel) &
                            static_cast<std::uint8_t>(~bit));
                    }
                    g_shipBallisticFireGate.setMenuBlocked(mask != 0);
                    g_shipLaserFireGate.setMenuBlocked(mask != 0);
                    g_shipParticleFireGate.setMenuBlocked(mask != 0);
                    g_shipMissileFireGate.setMenuBlocked(mask != 0);
                    g_shipEMFireGate.setMenuBlocked(mask != 0);
                    if (g_shipEmReconEnabled.load(std::memory_order_acquire)) {
                        g_shipEmReconProbe.setMenuBlocked(mask != 0);
                    }
                    g_shipLaunchLandingReconProbe.setMenuBlocked(mask != 0);
                }
            }

            switch (event.type) {
            case sds::GameEventType::ShipPilotEntered:
                resetLandVehicleActionGate();
                g_shipPilotActive.store(true, std::memory_order_release);
                refreshBoostpackProductionContext("ship-pilot-enter");
                g_lastShipStateValid.store(false, std::memory_order_release);
                g_shipBallisticFireGate.setPilotActive(true);
                g_shipLaserFireGate.setPilotActive(true);
                g_shipParticleFireGate.setPilotActive(true);
                g_shipMissileFireGate.setPilotActive(true);
                g_shipEMFireGate.setPilotActive(true);
                g_shipEmReconProbe.setPilotActive(true);
                g_shipLaunchLandingReconProbe.setPilotActive(true);
                g_shipWeaponCaptureProbeCount.store(0, std::memory_order_release);
                g_shipEmReconLogCount.store(0, std::memory_order_release);
                g_onFootRefreshPending.store(false, std::memory_order_release);
                g_autoRivetChargeCaptureArmed.store(false, std::memory_order_release);
                pluginLog("Ship pilot context: ENTER source=SpaceshipHudMenu authority=menu");
                break;

            case sds::GameEventType::ShipPilotExited:
                g_shipPilotActive.store(false, std::memory_order_release);
                refreshBoostpackProductionContext("ship-pilot-exit");
                g_lastShipStateValid.store(false, std::memory_order_release);
                g_shipBallisticFireGate.setPilotActive(false);
                g_shipLaserFireGate.setPilotActive(false);
                g_shipParticleFireGate.setPilotActive(false);
                g_shipMissileFireGate.setPilotActive(false);
                g_shipEMFireGate.setPilotActive(false);
                g_shipEmReconProbe.setPilotActive(false);
                g_shipLaunchLandingReconProbe.setPilotActive(false);
                g_onFootRefreshPending.store(true, std::memory_order_release);
                g_autoRivetChargeCaptureArmed.store(false, std::memory_order_release);
                if (std::string_view(event.text.data()) == "LoadingMenu") {
                    pluginLog("Ship pilot context: EXIT source=LoadingMenu-close reason=SpaceshipHudMenu-closed-during-load refresh=fresh-on-foot");
                } else {
                    pluginLog("Ship pilot context: EXIT source=SpaceshipHudMenu refresh=fresh-on-foot");
                }
                break;

            case sds::GameEventType::ShipPilotInvalidated:
                resetLandVehicleActionGate();
                g_shipPilotActive.store(false, std::memory_order_release);
                refreshBoostpackProductionContext("ship-pilot-invalidated");
                g_lastShipStateValid.store(false, std::memory_order_release);
                g_shipBallisticFireGate.setPilotActive(false);
                g_shipLaserFireGate.setPilotActive(false);
                g_shipParticleFireGate.setPilotActive(false);
                g_shipMissileFireGate.setPilotActive(false);
                g_shipEMFireGate.setPilotActive(false);
                g_shipEmReconProbe.setPilotActive(false);
                g_shipLaunchLandingReconProbe.setPilotActive(false);
                g_onFootRefreshPending.store(false, std::memory_order_release);
                g_autoRivetChargeCaptureArmed.store(false, std::memory_order_release);
                pluginLog("Ship pilot context: INVALIDATE source=LoadingMenu refresh=deferred-until-fresh-context");
                break;

            case sds::GameEventType::ShipPilotResumed:
                resetLandVehicleActionGate();
                g_shipPilotActive.store(true, std::memory_order_release);
                refreshBoostpackProductionContext("ship-pilot-resumed");
                g_lastShipStateValid.store(false, std::memory_order_release);
                g_shipBallisticFireGate.setPilotActive(true);
                g_shipLaserFireGate.setPilotActive(true);
                g_shipParticleFireGate.setPilotActive(true);
                g_shipMissileFireGate.setPilotActive(true);
                g_shipEMFireGate.setPilotActive(true);
                g_shipEmReconProbe.setPilotActive(true);
                g_shipLaunchLandingReconProbe.setPilotActive(true);
                g_shipWeaponCaptureProbeCount.store(0, std::memory_order_release);
                g_shipEmReconLogCount.store(0, std::memory_order_release);
                g_onFootRefreshPending.store(false, std::memory_order_release);
                g_autoRivetChargeCaptureArmed.store(false, std::memory_order_release);
                pluginLog("Ship pilot context: RESUME source=LoadingMenu-close evidence=SpaceshipHudMenu-still-open");
                break;

            case sds::GameEventType::LandVehicleAuthorityAcquired:
                if (g_gameState) {
                    const auto physics = g_gameState->latestLandVehiclePhysicsSnapshot();
                    setLandVehicleActionGateAuthority(physics.authorityActive, physics.authorityEpoch);
                }
                break;

            case sds::GameEventType::LandVehicleAuthorityReleased:
                setLandVehicleActionGateAuthority(false, 0);
                break;

            case sds::GameEventType::LandVehicleContextEntered:
                g_boostpackProductionLandVehicleActive.store(true, std::memory_order_release);
                refreshBoostpackProductionContext("land-vehicle-enter");
                resetLandVehicleActionGate();
                resetLandVehicleWwiseRecon(event.when);
                g_onFootRefreshPending.store(false, std::memory_order_release);
                g_autoRivetChargeCaptureArmed.store(false, std::memory_order_release);
                pluginLog("Land vehicle context: ENTER suppression=handheld-output-clear authority=kVehicle-suppression-only");
                break;

            case sds::GameEventType::LandVehicleContextExited:
                g_boostpackProductionLandVehicleActive.store(false, std::memory_order_release);
                refreshBoostpackProductionContext("land-vehicle-exit");
                resetLandVehicleActionGate();
                flushLandVehicleWwiseRecon("vehicle-exit", event.when);
                g_onFootRefreshPending.store(true, std::memory_order_release);
                g_autoRivetChargeCaptureArmed.store(false, std::memory_order_release);
                pluginLog("Land vehicle context: EXIT suppression=release refresh=fresh-on-foot");
                break;

            case sds::GameEventType::Shutdown:
                setBoostpackProductionContextEligible(false, "shutdown-event");
                resetLandVehicleActionGate();
                g_lastShipStateValid.store(false, std::memory_order_release);
                stopShipLaunchLandingHaptics("shutdown-event");
                g_shipBallisticFireGate.setPilotActive(false);
                g_shipLaserFireGate.setPilotActive(false);
                g_shipParticleFireGate.setPilotActive(false);
                g_shipMissileFireGate.setPilotActive(false);
                g_shipEMFireGate.setPilotActive(false);
                g_shipEmReconProbe.setPilotActive(false);
                g_shipLaunchLandingReconProbe.setPilotActive(false);
                g_shipLaunchLandingReconProbe.setMenuBlocked(true);
                if (g_audioCapture) {
                    g_audioCapture->setShipWeaponObservationArmed(false);
                }
                break;

            default:
                break;
            }

            if (g_audioCapture) {
                const bool pilotActive = g_shipPilotActive.load(std::memory_order_acquire);
                const bool launchLandingReconCaptureArmed =
                    g_shipLaunchLandingReconEnabled.load(std::memory_order_acquire) &&
                    (pilotActive || g_shipLaunchLandingReconProbe.requiresWwiseCapture());
                const bool landVehicleReconCaptureArmed =
                    event.type != sds::GameEventType::Shutdown &&
                    g_gameState && g_gameState->landVehicleReconCorrelationArmed();
                const bool boostpackProductionArmed =
                    event.type != sds::GameEventType::Shutdown && boostpackProductionCaptureArmed();
                const bool shipCaptureArmed =
                    launchLandingReconCaptureArmed || landVehicleReconCaptureArmed ||
                    boostpackProductionArmed ||
                    (pilotActive && g_shipWeaponSemanticCache && g_shipWeaponSemanticCache->ready());
                g_audioCapture->setShipWeaponObservationArmed(shipCaptureArmed);
            }

            if (g_uiAudioDiscovery &&
                (event.type == sds::GameEventType::MenuOpened ||
                 event.type == sds::GameEventType::MenuClosed)) {
                try {
                    g_uiAudioDiscovery->observeGameEvent(event);
                } catch (...) {
                    pluginLog("UI audio discovery: menu observer exception ignored; runtime routing unchanged");
                }
            }
            if (g_audioCapture) {
                g_audioCapture->setUiAudioDiscoveryArmed(
                    g_uiAudioDiscovery && g_uiAudioDiscovery->captureArmed());
            }
            if (g_weaponSpeakerPlayback && event.type == sds::GameEventType::WeaponEquipped) {
                try {
                    (void)g_weaponSpeakerPlayback->observeGameEvent(event);
                } catch (...) {
                    pluginLog("Weapon speaker: equip observer exception ignored; runtime routing unchanged");
                }
            }
            if (g_weaponSfxDiscovery && event.type == sds::GameEventType::WeaponEquipped) {
                try {
                    g_weaponSfxDiscovery->observeGameEvent(event);
                } catch (...) {
                    pluginLog("Weapon SFX discovery: equip observer exception ignored; runtime routing unchanged");
                }
            }
            if (event.type == sds::GameEventType::WeaponEquipped) {
                const bool autoRivetEquipped =
                    g_autoRivetChargeCaptureEnabled.load(std::memory_order_acquire) &&
                    std::string_view(event.text.data()) == "Auto-Rivet";
                g_autoRivetChargeCaptureArmed.store(
                    autoRivetEquipped,
                    std::memory_order_release);
            } else if (event.type == sds::GameEventType::Shutdown) {
                g_autoRivetChargeCaptureArmed.store(false, std::memory_order_release);
            }
            if (g_audioCapture) {
                const bool captureArmed =
                    g_autoRivetChargeCaptureArmed.load(std::memory_order_acquire) ||
                    (g_weaponSpeakerPlayback && g_weaponSpeakerPlayback->armed()) ||
                    (g_weaponSfxDiscovery && g_weaponSfxDiscovery->armed());
                g_audioCapture->setWeaponSfxDiscoveryArmed(captureArmed);
            }
            return g_eventRouter && g_eventRouter->dispatch(std::move(event));
        };

        g_gameState = std::make_unique<sds::GameStateAdapter>(
            runtimeEmit,
            nativeLog,
            false,
            config.advancedHaptics);
        g_gameState->setLandVehicleMotionCallback([](const sds::LandVehicleMotionState& state) {
            if (g_haptics) {
                g_haptics->handleLandVehicleMotionState(state);
            }
        });
        g_gameState->setLandVehicleSemanticCallback([](
            std::string_view semantic,
            bool active,
            std::chrono::steady_clock::time_point when) {
            observeLandVehicleProductionSemantic(semantic, active, when);
        });
        sds::setBoostpackSemanticObserver([](
            std::string_view semantic,
            bool active,
            std::chrono::steady_clock::time_point when) {
            observeBoostpackProductionSemantic(semantic, active, when);
        });
        refreshBoostpackSemanticArming();
        if (!g_gameState->registerSinks()) {
            pluginLog("Game state: event registration incomplete; controller worker remains active");
        }

        sds::FireMarkerBridge::AnimationMarkerObserver animationMarkerObserver{};
        if (g_weaponSfxDiscovery) {
            animationMarkerObserver = [](
                std::string_view tag,
                std::string_view payload,
                std::chrono::steady_clock::time_point when) {
                if (!g_weaponSfxDiscovery) {
                    return false;
                }
                try {
                    return g_weaponSfxDiscovery->observeAnimationMarker(tag, payload, when);
                } catch (...) {
                    pluginLog("Weapon SFX discovery: animation marker observer exception ignored; runtime routing unchanged");
                    return false;
                }
            };
        }

        g_fireMarkerBridge = std::make_unique<sds::FireMarkerBridge>(
            runtimeEmit,
            nativeLog,
            config.debugLogging,
            std::move(animationMarkerObserver));
        if (!g_fireMarkerBridge->start()) {
            pluginLog("Fire marker bridge: registration incomplete; persistent weapon walls remain active but live cadence is unavailable");
        } else {
            pluginLog("Fire marker bridge: confirmed player animation markers are wired to live weapon effects");
        }

        if (remoteVoCaptureEnabled || weaponSpeakerCaptureEnabled || autoRivetChargeCaptureEnabled ||
            uiAudioDiscoveryEnabled || uiSpeakerPlaybackEnabled || musicSelectionEnabled ||
            shipBallisticHapticsEnabled || shipLaunchLandingReconEnabled ||
            landVehicleReconEnabled || boostpackProductionEnabled) {
            const auto executablePath = currentExecutablePath();
            sds::StarfieldAudioCapture::RemoteVoSourceCallback sourceProbe{};
            if (remoteVoCaptureEnabled) {
                sourceProbe = [nativeLog, executablePath](const sds::RemoteVoMirrorRequest& request) {
                    if (!g_speakerManager ||
                        !g_speakerManager->categoryEnabled(sds::SpeakerCategory::Comms)) {
                        // RemoteVoSourceCallback is a notification callback.
                        // Exit before any filesystem/decode work without
                        // changing the lambda's return type from void.
                        return;
                    }

                    const auto steadyMicrosNow = []() noexcept {
                    return std::chrono::duration_cast<std::chrono::microseconds>(
                        std::chrono::steady_clock::now().time_since_epoch()).count();
                };
                const auto latencyMs = [&request, &steadyMicrosNow]() noexcept -> long long {
                    if (request.captureSteadyMicros <= 0) {
                        return -1;
                    }
                    return (steadyMicrosNow() - request.captureSteadyMicros) / 1000;
                };
                bool handled = false;
                const auto candidates = sds::buildRemoteVoFilesystemCandidates(request.filePath, executablePath);
                nativeLog(sds::formatRemoteVoFilesystemProbeContext(request.filePath, executablePath));

                const auto probeCandidate = [&nativeLog](const sds::RemoteVoFilesystemCandidate& candidate) {
                    std::error_code existsError{};
                    const bool exists = !candidate.path.empty() && std::filesystem::exists(candidate.path, existsError);

                    sds::WwiseWemSourceMetadata metadata{};
                    if (candidate.path.empty()) {
                        metadata.error = "path unavailable";
                    } else {
                        metadata = sds::probeWwiseWemFile(candidate.path, 64);
                    }
                    nativeLog(sds::formatRemoteVoFilesystemCandidateProbe(candidate, exists, metadata));
                    return metadata.openSucceeded;
                };

                const bool rawOpened = probeCandidate(candidates.raw);
                const bool dataRootOpened = probeCandidate(candidates.dataRoot);
                if (!rawOpened && !dataRootOpened) {
                    const auto manifest = sds::probeVoiceArchiveManifest(request.filePath, executablePath);
                    nativeLog(sds::formatVoiceArchiveManifestContext(request.filePath, manifest));
                    for (const auto& entry : manifest.entries) {
                        nativeLog(sds::formatVoiceArchiveManifestEntry(entry));
                        const auto indexProbe = sds::probeVoiceBa2Index(request.filePath, entry);
                        nativeLog(sds::formatVoiceBa2IndexProbe(indexProbe));
                        if (indexProbe.targetFound) {
                            const auto payloadProbe = sds::probeVoiceWemPayload(indexProbe);
                            nativeLog(sds::formatVoiceWemPayloadProbe(payloadProbe));
                            if (payloadProbe.readSucceeded) {
                                const auto structureProbe = sds::probeVoiceWemStructure(
                                    payloadProbe,
                                    request.codecId);
                                nativeLog(sds::formatVoiceWemStructureProbeSummary(structureProbe));
                                for (std::size_t chunkIndex = 0; chunkIndex < structureProbe.chunks.size(); ++chunkIndex) {
                                    nativeLog(sds::formatVoiceWemStructureProbeChunk(
                                        structureProbe.chunks[chunkIndex],
                                        chunkIndex));
                                }
                                if (structureProbe.scanComplete) {
                                    const auto vorbisProbe = sds::probeWwiseVorbisPackets(
                                        payloadProbe,
                                        structureProbe,
                                        5u,
                                        16u);
                                    nativeLog(sds::formatWwiseVorbisPacketProbeSummary(vorbisProbe));
                                    if (vorbisProbe.setupPacket.valid) {
                                        nativeLog(sds::formatWwiseVorbisSetupPacketProbe(vorbisProbe));
                                    }
                                    for (std::size_t packetIndex = 0; packetIndex < vorbisProbe.audioPackets.size(); ++packetIndex) {
                                        nativeLog(sds::formatWwiseVorbisAudioPacketProbe(
                                            vorbisProbe.audioPackets[packetIndex],
                                            packetIndex));
                                    }
                                    if (vorbisProbe.probeComplete) {
                                        const auto decodeResult = sds::decodeWwiseVorbisToPcm(
                                            payloadProbe,
                                            structureProbe,
                                            vorbisProbe);
                                        nativeLog(sds::formatWwiseVorbisDecodeSummary(decodeResult));
                                        const auto captureToDecodeMs = latencyMs();
                                        if (decodeResult.decodeSucceeded) {
                                            auto playback = sds::prepareRemoteVoControllerPlayback(decodeResult);
                                            nativeLog(sds::formatRemoteVoControllerPlaybackPreparation(playback));

                                            bool submitted = false;
                                            const bool speakerManagerPresent = static_cast<bool>(g_speakerManager);
                                            const bool speakerActive = g_speakerManager && g_speakerManager->active();
                                            if (playback.prepared && g_speakerManager) {
                                                const sds::CapturedSoundIdentity identity{
                                                    .id = (static_cast<std::uint64_t>(request.eventId) << 32u) |
                                                        static_cast<std::uint64_t>(request.externalCookie),
                                                    .controllerOnlySafe = false,
                                                };
                                                submitted = g_speakerManager->submitCaptured(
                                                    std::move(playback.pcm),
                                                    sds::SpeakerCategory::Comms,
                                                    identity,
                                                    true);
                                            }

                                            const auto submitNow = std::chrono::steady_clock::now();
                                            const bool replacedActive = submitted &&
                                                g_remoteVoPlaybackDeadline.time_since_epoch().count() != 0 &&
                                                submitNow < g_remoteVoPlaybackDeadline;
                                            if (submitted) {
                                                g_remoteVoPlaybackDeadline = submitNow +
                                                    std::chrono::milliseconds(playback.outputDurationMs);
                                                handled = true;
                                            }

                                            const auto speakerOutputMode =
                                                g_speakerManager ?
                                                    g_speakerManager->outputMode() :
                                                    sds::SpeakerOutputMode::Both;
                                            const auto originalAction = sds::decideRemoteVoOriginalOutput(
                                                speakerOutputMode,
                                                submitted,
                                                request.originalPlayingId);
                                            bool originalStopIssued = false;
                                            if (originalAction == sds::RemoteVoOriginalOutputAction::StopOriginal) {
                                                originalStopIssued = sds::stopWwisePlayingId(request.originalPlayingId);
                                            }

                                            const char* outputModeName = speakerOutputMode == sds::SpeakerOutputMode::ControllerOnly
                                                ? "ControllerOnly"
                                                : "Both";
                                            const char* originalOutput = "passthrough-both";
                                            if (speakerOutputMode == sds::SpeakerOutputMode::ControllerOnly) {
                                                if (!submitted) {
                                                    originalOutput = "failsafe-passthrough-submit-rejected";
                                                } else if (request.originalPlayingId == 0) {
                                                    originalOutput = "failsafe-passthrough-no-playing-id";
                                                } else {
                                                    originalOutput = originalStopIssued
                                                        ? "stopped"
                                                        : "failsafe-passthrough-stop-unavailable";
                                                }
                                            }

                                            char playbackLog[704]{};
                                            std::snprintf(
                                                playbackLog,
                                                sizeof(playbackLog),
                                                "Voice controller playback: seq=%llu submit=%s speakerManager=%s speakerActive=%s replacedActive=%s sourceFrames=%u sourceRate=%u outputFrames=%zu outputRate=%u durationMs=%llu captureToDecodeMs=%lld captureToSubmitMs=%lld category=Comms continuous=yes speakerOutputMode=%s originalPlayingId=%u originalOutput=%s",
                                                static_cast<unsigned long long>(request.sequence),
                                                submitted ? "accepted" : "rejected",
                                                speakerManagerPresent ? "yes" : "no",
                                                speakerActive ? "yes" : "no",
                                                replacedActive ? "yes" : "no",
                                                playback.sourceFrames,
                                                playback.sourceSampleRate,
                                                playback.outputFrames,
                                                playback.outputSampleRate,
                                                static_cast<unsigned long long>(playback.outputDurationMs),
                                                captureToDecodeMs,
                                                latencyMs(),
                                                outputModeName,
                                                request.originalPlayingId,
                                                originalOutput);
                                            nativeLog(playbackLog);
                                        }
                                    }
                                }
                            }
                        }
                        if (handled) {
                            char handledLog[256]{};
                            std::snprintf(
                                handledLog,
                                sizeof(handledLog),
                                "Voice archive probe: seq=%llu handled=yes archive=\"%s\"; remaining archives skipped",
                                static_cast<unsigned long long>(request.sequence),
                                entry.archiveName.c_str());
                            nativeLog(handledLog);
                            break;
                        }
                    }
                }
            };
            }

            sds::StarfieldAudioCapture::WeaponSfxObservationCallback weaponSfxObservation{};
            if (autoRivetChargeCaptureEnabled || g_weaponSpeakerPlayback || g_weaponSfxDiscovery || g_shipWeaponSemanticCache ||
                shipMissileHapticsEnabled || shipEMHapticsEnabled || shipLaunchLandingReconEnabled || landVehicleReconEnabled ||
                g_boostpackFeedbackAuthority != nullptr || g_boostpackSpeakerPlayback != nullptr ||
                g_shipEmReconEnabled.load(std::memory_order_acquire)) {
                weaponSfxObservation = [autoRivetChargeCaptureEnabled](const sds::WeaponSfxWwiseObservation& observation) {
                    observeShipBallisticWwise(observation);
                    observeShipLaserWwise(observation);
                    observeShipParticleWwise(observation);
                    observeShipMissileWwise(observation);
                    observeShipEMWwise(observation);
                    observeShipLaunchLandingReconWwise(observation);
                    observeLandVehicleReconWwise(observation);
                    observeBoostpackProductionWwise(observation);
                    if (g_shipEmReconEnabled.load(std::memory_order_acquire)) {
                        observeShipEmReconWwise(observation);
                    }
                    if (autoRivetChargeCaptureEnabled) {
                        observeAutoRivetChargeSemantic(observation);
                    }
                    if (g_weaponSpeakerPlayback) {
                        (void)g_weaponSpeakerPlayback->observeWwise(observation);
                    }
                    if (g_weaponSfxDiscovery) {
                        (void)g_weaponSfxDiscovery->observeWwise(observation);
                    }
                };
            }

            sds::StarfieldAudioCapture::UiAudioObservationCallback uiAudioObservation{};
            if (g_uiSpeakerPlayback || g_uiAudioDiscovery || g_haptics) {
                uiAudioObservation = [](const sds::UiAudioWwiseObservation& observation) {
                    if (g_haptics &&
                        observation.externalCount == 0u &&
                        !observation.hasExternalSources) {
                        (void)g_haptics->emitDigipickWwiseEvent(
                            observation.eventId,
                            observation.when);
                    }
                    if (g_uiSpeakerPlayback) {
                        (void)g_uiSpeakerPlayback->observeWwise(observation);
                    }
                    if (g_uiAudioDiscovery) {
                        (void)g_uiAudioDiscovery->observeWwise(observation);
                    }
                };
            }

            sds::StarfieldAudioCapture::MusicReconObservationCallback musicReconObservation{};
            if (g_musicRecon) {
                musicReconObservation = [](const sds::MusicReconWwiseObservation& observation) {
                    if (g_musicRecon) {
                        (void)g_musicRecon->observeWwise(observation);
                    }
                };
            }

            sds::StarfieldAudioCapture::MusicSelectionObservationCallback musicSelectionObservation{};
            if (musicHapticsEnabled) {
                musicSelectionObservation = [](const sds::MusicSelectionObservation& observation) {
                    if (!g_musicHapticsEnabled.load(std::memory_order_acquire) || observation.playingId == 0u) {
                        return;
                    }

                    if (observation.kind == sds::MusicSelectionObservationKind::Ended) {
                        musicHapticsErasePlayingId(observation.playingId);
                        if (g_audioTransport) {
                            (void)g_audioTransport->stopMusicHapticsPlayingId(observation.playingId);
                        }
                        return;
                    }

                    if (observation.kind != sds::MusicSelectionObservationKind::Selected ||
                        observation.eventId == 0u || observation.mediaId == 0u || musicHapticsBlocked()) {
                        return;
                    }
                    if (!musicHapticsMarkPlayingIdActive(observation.playingId)) {
                        return;
                    }

                    if (!g_weaponAudioPipeline || !g_weaponAudioPipeline->tryEnqueueMusicHaptics(
                            sds::MusicHapticsPrepareRequest{
                                .eventId = observation.eventId,
                                .mediaId = observation.mediaId,
                                .playingId = observation.playingId,
                                .selectedAt = observation.capturedAt,
                            })) {
                        // A later selected segment for the same Wwise playing ID may retry.
                    }
                };
            }

            g_audioCapture = std::make_unique<sds::StarfieldAudioCapture>(
                nativeLog,
                sds::StarfieldAudioCapture::RemoteVoMirrorCallback{},
                sds::kRemoteCommsVoMirrorProfile,
                std::move(sourceProbe),
                std::move(weaponSfxObservation),
                std::move(uiAudioObservation),
                std::move(musicReconObservation),
                std::move(musicSelectionObservation));
            if (!g_audioCapture->start()) {
                pluginLog("Starfield audio capture: PostEvent capture did not activate; controller voice/weapon/UI capture unavailable");
                if (g_musicRecon) {
                    pluginLog("Music recon: INACTIVE reason=existing-PostEvent-hook-unavailable output=none");
                }
            } else {
                const bool captureArmed =
                    g_autoRivetChargeCaptureArmed.load(std::memory_order_acquire) ||
                    (g_weaponSpeakerPlayback && g_weaponSpeakerPlayback->armed()) ||
                    (g_weaponSfxDiscovery && g_weaponSfxDiscovery->armed());
                g_audioCapture->setWeaponSfxDiscoveryArmed(captureArmed);
                g_audioCapture->setShipWeaponObservationArmed(boostpackProductionCaptureArmed());
                g_audioCapture->setUiAudioDiscoveryArmed(false);
                g_audioCapture->setUiAudioPlaybackArmed(
                    (g_uiSpeakerPlayback && g_uiSpeakerPlayback->armed()) ||
                    g_haptics != nullptr);
                g_audioCapture->setMusicReconArmed(g_musicRecon != nullptr);
                g_audioCapture->setMusicSelectionArmed(musicSelectionEnabled);
                if (uiAudioDiscoveryEnabled) {
                    {
                        std::ostringstream line;
                        line << "UI audio discovery: ACTIVE diagnostic-only trigger=first-target-menu"
                             << " end=DataMenu-close maxDurationMs=300000 hudFollowupMs=60000"
                             << " correlationMs=150 queue=1024 aggregates=512 transitions=128"
                             << " unpromotedOnly=yes existingPromotedPlayback=" << sds::uiSpeakerCueDefinitions().size()
                             << " playback=production-promoted targetedResolution="
                             << sds::v0359UiAudioResolutionTargets().size()
                             << " extraction=disabled normalGameAudio=untouched";
                        pluginLog(line.str());
                    }
                }
                if (autoRivetChargeCaptureEnabled) {
                    pluginLog("Auto-Rivet tension gate: rawR2=no-authority chargeSource=player-Wwise blockingMenus=DataMenu,PauseMenu,LoadingMenu resumeRequiresFreshChargeStart=yes");
                }
                if (shipBallisticHapticsEnabled) {
                    pluginLog("Ship ballistic haptics: ACTIVE control=R2 baselineWall=pilot-context authority=first-player-Wwise+nearby-R2 forwardFirstShot=hardware-alias+180ms-real-R2 automaticHeartbeat=same-event+same-gameObject+held-R2 catalog=SoundBanksInfo+hardware-alias family=ballistic confirmedShotKick=yes laser=promoted-v0365 otherFamilies=inert menuMute=DataMenu,PauseMenu");
                }
                if (shipLaserHapticsEnabled) {
                    pluginLog("Ship laser haptics: ACTIVE control=R2 authority=exact-player-Wwise+nearby-R2 forwardFirstShot=hardware-alias+180ms-real-R2 automaticHeartbeat=same-event+same-gameObject+held-R2 event=0xCE7B2EB1 leaseMs=250 haptic=continuous-energy+native-pulse-crest tactileRetune=r2 crestMs=40 crestGain=0.55 overlayGain=0.55 trigger=smooth-resistance menuMute=DataMenu,PauseMenu ballisticBaseline=r6-frozen");
                }
                if (shipParticleHapticsEnabled) {
                    pluginLog("Ship particle haptics: ACTIVE subtype=ProtonBeam control=R2 authority=exact-player-Wwise+nearby-R2 forwardFirstShot=hardware-alias+180ms-real-R2 cadence=native-heartbeat sameObject=yes event=0xC8BBBCEA haptic=discrete-particle-pulse pulseMs=60 pulseGain=0.70 continuousParticle=no trigger=finite-energy-break triggerMs=60 menuMute=DataMenu,PauseMenu ballisticBaseline=r6-frozen laserBaseline=v0365-r2-frozen reconEvidence=v0366");
                }
                if (shipMissileHapticsEnabled) {
                    pluginLog("Ship missile haptics: ACTIVE control=R2 authority=exact-player-Wwise+nearby-R2 forwardFirstShot=hardware-alias+180ms-real-R2 cadence=native-heartbeat sameObject=yes event=0x6846C9EC haptic=discrete-launch-thump launchMs=105 launchGain=0.90 continuousMissile=no trigger=heavy-finite-break triggerMs=105 menuMute=DataMenu,PauseMenu ballisticBaseline=r6-frozen laserBaseline=v0365-r2-frozen particleBaseline=v0367-frozen reconEvidence=v0368");
                }
                if (shipEMHapticsEnabled) {
                    pluginLog("Ship EM haptics: ACTIVE control=R2 authority=exact-player-Wwise+nearby-R2 forwardFirstShot=hardware-alias+180ms-real-R2 cadence=native-heartbeat sameObject=yes event=0x7A1A570C haptic=discrete-electrical-pulse pulseMs=75 pulseGain=0.70 continuousEM=no trigger=light-electrical-break triggerStart=72 triggerMs=90 tactileRetune=r1 triggerRearm=r2 partialRelease=48 neutralRefresh=one-cycle menuMute=DataMenu,PauseMenu ballisticBaseline=r6-frozen laserBaseline=v0365-r2-frozen particleBaseline=v0367-frozen missileBaseline=v0369-frozen reconEvidence=v0370");
                }
                if (shipLaunchLandingReconEnabled) {
                    pluginLog("Ship launch/landing recon: ACTIVE reconDiagnosticOnly=yes authority=pilot-landed-state+menu-lifecycle transitions=takeoff:true-to-false,touchdown:false-to-true landingLifecycle=first-fader-load-hud-close-load-close-fader-close+post-load-touchdown-state+bounded-tail docked=suppressed Wwise=zero-external takeoffPreMs=2000 takeoffPostMs=2000 touchdownPostMs=2000 landingMaxMs=30000 cinematicTailWaitMs=15000 maxSamples=256 postLoadTouchdownState=fresh-player-pilot-read-only touchdownPrecisionPoll=runtime-tick-after-first-fader-close precisionEnds=touchdown-or-cancel crossLoadingCapture=diagnostic-only controllerOutput=launch-landing-rumble+touchdown-haptic propulsion=unchanged shipWeaponBaselines=ballistic-r6,laser-r2,particle-v0367,missile-v0369,em-v0371-r2");
                    if (shipTouchdownHapticsEnabled) {
                        pluginLog("Ship touchdown haptics: ACTIVE authority=landing-sequence-landed-false-to-true haptic=ShipTouchdownThump durationMs=100 gain=0.90 adaptiveTrigger=none speaker=none lightbar=none WwiseAuthority=none precisionPoll=runtime-tick-until-touchdown");
                    }
                    if (shipBallisticHapticsEnabled) {
                        pluginLog("Ship launch/landing haptics: ACTIVE body=ShipBoost gain=1.00 level=1.00 triggers=both-EffectEx-sustained start=64 forces=220/255/210 frequency=24 takeoffStart=landed+non-docked+FaderMenu-without-TakeoffMenu takeoffStop=next-FaderMenu-open-after-first-close loadingFallback=yes timeout=30s landingStart=precision-touchdown-tail landingStop=landing-sequence-end touchdownThump=unchanged-100ms-0.90 speaker=none lightbar=none");
                    }
                }
                if (remoteVoCaptureEnabled) {
                    const auto startupSpeakerOutputMode =
                        g_speakerManager ?
                            g_speakerManager->outputMode() :
                            sds::SpeakerOutputMode::Both;
                    const char* outputModeName = startupSpeakerOutputMode == sds::SpeakerOutputMode::ControllerOnly
                        ? "ControllerOnly"
                        : "Both";
                    char activationLog[768]{};
                    std::snprintf(
                        activationLog,
                        sizeof(activationLog),
                        "Voice Wwise Vorbis decode: v0.3.11 ACTIVE event=0x89E658E8 looseCandidates=raw,data-root archiveConfig=sResourceEnglishVoiceList ba2HeaderBytes=32 gnrlRecordBytes=36 nameTableLookup=enabled payloadRead=matched-uncompressed riffChunkScan=enabled fmtExtendedParse=enabled vorbisNewFmt30=enabled packetHeaderBytes=2 packetRebuild=enabled codebooks=aoTuV-6.03 decoder=stb_vorbis pcm=16bit continuous=yes decompression=disabled resample=44100-to-48000 playback=continuous-controller-speaker replaceActive=yes speakerOutputMode=%s controllerOnlyStop=post-submit-failsafe commsMakeup=+6dB commsLimiter=soft hardwareSpeakerVolume=0x64 hardwarePreamp=0x05 speakerVolumeScale=0.8-v0310max-1.0-plus25pct replay=disabled extraction=disabled loopback=disabled; trigger=remote/radio-voice-only",
                        outputModeName);
                    pluginLog(activationLog);
                }
                if (weaponSpeakerCaptureEnabled) {
                    pluginLog("Weapon sustained speaker: ACTIVE weapons=Arc Welder,Cutter,Va'ruun Starstorm authority=player-Wwise startStop=yes loopMode=layered-stems+authored-smpl rawR2=start-no-authority release-stop=yes blockingMenus=DataMenu,PauseMenu,LoadingMenu");
                    std::ostringstream weaponCaptureLine;
                    weaponCaptureLine << "Weapon speaker: internal Wwise PostEvent capture ACTIVE profiles="
                                      << sds::weaponSpeakerProfiles().size()
                                      << " cues=";
                    bool firstCue = true;
                    for (const auto& profile : sds::weaponSpeakerProfiles()) {
                        for (const auto& cue : profile.cues) {
                            if (cue.trigger != sds::WeaponSpeakerTrigger::WwisePost) {
                                continue;
                            }
                            if (!firstCue) {
                                weaponCaptureLine << ',';
                            }
                            firstCue = false;
                            weaponCaptureLine << profile.weaponIdentity << ':' << cue.action
                                              << "@0x" << std::uppercase << std::hex << std::setw(8)
                                              << std::setfill('0') << cue.liveWwiseEventId << std::dec
                                              << std::setfill(' ') << "/obj0x" << std::uppercase << std::hex
                                              << cue.requiredGameObjectId << std::dec;
                        }
                    }
                    weaponCaptureLine << " normalGameAudio=untouched";
                    pluginLog(weaponCaptureLine.str());
                }
                if (g_weaponSfxDiscovery) {
                    pluginLog(
                        "Weapon SFX discovery: ACTIVE diagnostic-only mode=exact-target-set batch=6-shattered-space-wem-capture operatorTargets=2 startupNameScan=disabled onePassResolution=live-event workerResolution=background wemCapture=targeted-v0347 captureRoot=Data/SFSE/Plugins/StarfieldDualSenseDiagnostics/ShatteredSpaceWemCapture "
                        "internalWwise=zero-external fireWindowMs=-150/+350 "
                        "reloadWindowMs=-2500/+250 drawHolsterWindowMs=-250/+750 "
                        "markerLogging=candidates-only playback=no repost=no stopOriginal=no synthetic=no");
                } else if (weaponSpeakerCaptureEnabled) {
                    pluginLog("Weapon SFX discovery: INACTIVE reason=shattered-space-speaker-promotion playback=no normalGameAudio=untouched");
                } else {
                    pluginLog("Weapon SFX discovery: INACTIVE reason=weapon-speaker-capture-disabled playback=no normalGameAudio=untouched");
                }
            }
        }

        if (const auto* tasks = SFSE::GetTaskInterface()) {
            tasks->AddPermanentTask(runtimeTick);
            pluginLog("Game state: health polling, reusable native semantic input, verification, and quit-safe shutdown scheduled on SFSE permanent task");

            if (sharedAudioPreparationEnabled && !g_weaponAudioPipelineDataPath.empty() &&
                (!g_weaponAudioPipelineEnabled || g_weaponSpeakerPreparedCache)) {
                const sds::AudioPipelineStartupOptions startupOptions{
                    .prepareUi = uiSpeakerPlaybackEnabled,
                    .prepareMainMenuUiOnly = false,
                    .prepareWeapons = g_weaponAudioPipelineEnabled,
                    .resolveUiDiagnostics = uiAudioDiscoveryEnabled,
                    .prepareShipWeaponSemantics = shipBallisticHapticsEnabled,
                    .prepareMusicRecon = musicSelectionEnabled,
                    .prepareBoostpackSpeaker = boostpackProductionEnabled,
                };
                g_weaponAudioPipeline = std::make_unique<sds::WeaponAudioPipeline>(
                    g_weaponSpeakerPreparedCache,
                    sds::makeRealWeaponAudioPipelineBackend(g_weaponAudioPipelineDataPath, startupOptions),
                    g_uiSpeakerPreparedCache,
                    g_shipWeaponSemanticCache,
                    g_boostpackSpeakerPreparedCache);
                g_weaponAudioPipeline->start();
                if (g_musicRecon && g_audioCapture && g_audioCapture->active()) {
                    pluginLog("Music recon: ACTIVE diagnostic-only source=existing-PostEvent-hook externalSources=zero resolver=shared-background-worker decode=music-name-candidates output=none wholeGameMix=no");
                    pluginLog("Music selection recon: ACTIVE diagnostic-only targetSource=SoundBanksInfo-Starfield_MUS-multi-media callback=AK_Duration existingCallbackMode=chain-preserve-cookie retirement=AK_EndOfEvent pool=128 output=none");
                } else if (g_musicRecon) {
                    pluginLog("Music recon: INACTIVE reason=capture-unavailable output=none");
                }

                std::ostringstream pipelineLine;
                pipelineLine << "Weapon audio pipeline: worker started families="
                             << sds::weaponSpeakerAudioFamilyCount()
                             << " profiles=" << sds::weaponSpeakerProfiles().size()
                             << " uiPreparation=" << (uiSpeakerPlaybackEnabled ? "yes" : "no")
                             << " uiDiagnosticResolution=" << (uiAudioDiscoveryEnabled ? "yes" : "no")
                             << " uiDiagnosticTargets=" << sds::v0359UiAudioResolutionTargets().size()
                             << " weaponPreparation=" << (g_weaponAudioPipelineEnabled ? "yes" : "no")
                             << " shipWeaponSemantics=" << (shipBallisticHapticsEnabled ? "yes" : "no")
                             << " musicReconPreparation=" << (musicSelectionEnabled ? "yes" : "no")
                             << " discoveryTargets=" << kWeaponSfxDiscoveryTargets.size()
                             << " discoveryQueue=64 startupMode=ui-first workerResolution=background";
                pluginLog(pipelineLine.str());
                if (g_weaponAudioPipelineEnabled) {
                    pluginLog("Weapon speaker family: profile=XM-2311 family=Old Earth Pistol mode=explicit-shared");
                    pluginLog("Weapon speaker family: profile=Va'ruun Quickstrike family=Solstice mode=explicit-shared");
                    pluginLog("Weapon speaker family: profile=Va'ruun Longfang family=Orion mode=explicit-shared");
                    pluginLog("Weapon speaker family: profile=Va'ruun Starlash family=Equinox mode=explicit-shared");
                }
            } else if (sharedAudioPreparationEnabled) {
                pluginLog("Weapon audio pipeline: inactive reason=data-path-or-prepared-cache-unavailable; controller runtime remains active");
                if (g_musicRecon) {
                    pluginLog("Music recon: INACTIVE reason=shared-background-worker-unavailable output=none");
                }
            }
        } else {
            pluginLog("Game state: SFSE task interface unavailable; health polling disabled");
            pluginLog("Weapon audio pipeline: inactive reason=SFSE-task-interface-unavailable; controller runtime remains active");
            if (g_musicRecon) {
                pluginLog("Music recon: INACTIVE reason=SFSE-task-interface-unavailable output=none");
            }
        }
    }

    void onSfseMessage(SFSE::MessagingInterface::Message* message)
    {
        if (!message) {
            return;
        }

        if (message->type == SFSE::MessagingInterface::kPostLoad) {
            sds::registerSettingsMenu(std::filesystem::path(kConfigPath), pluginLog, applyImmediateLiveSettings);
            return;
        }

        if (message->type == SFSE::MessagingInterface::kPostDataLoad) {
            initializeRuntime();
        }
    }
}

SFSE_PLUGIN_LOAD(const SFSE::LoadInterface* sfse)
{
    SFSE::Init(sfse, { .logName = "StarfieldDualSense" });
    REX::INFO("StarfieldDualSense {} loading | Starfield 1.16.244.0 | fireMarkerABI=validated-live", kVersion);

    g_startupConfig = loadRuntimeConfig();
    g_nativeDualSenseReconnectFixEnabled.store(
        g_startupConfig->operatingMode == sds::OperatingMode::ReconnectFixOnly ||
            g_startupConfig->dualSenseReconnectFix,
        std::memory_order_release);

    if (g_startupConfig->operatingMode == sds::OperatingMode::Full) {
        startEarlyMainMenuUiPreparation();
    } else {
        pluginLog(
            "Main-menu UI speaker prewarm: SKIPPED reason=OperatingMode-ReconnectFixOnly");
    }

    const auto* messaging = SFSE::GetMessagingInterface();
    if (!messaging || !messaging->RegisterListener(onSfseMessage)) {
        pluginLog("StarfieldDualSense: failed to register SFSE messaging listener");
        return false;
    }

    return true;
}
