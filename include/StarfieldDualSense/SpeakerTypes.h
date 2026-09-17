#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace sds
{
    enum class SpeakerOutputMode : std::uint8_t
    {
        Both,
        ControllerOnly
    };

    enum class SpeakerCategory : std::uint8_t
    {
        Comms,
        ScannerUI,
        Weapons,
        Digipick,
        Crafting,
        ShipSystems,
        Boostpack
    };

    enum class SpeakerRoutingMode : std::uint8_t
    {
        Channel1,
        Channel2,
        Both
    };

    enum class SpeakerPersistentInvalidationReason : std::uint8_t
    {
        BackendStop,
        EndpointInvalidated
    };

    enum class SpeakerEffectKind : std::uint8_t
    {
        ScannerTick,
        UiConfirm,
        UiBack,
        SuitAlert,
        WeaponMechanical,
        ReloadMechanical,
        DigipickTick,
        DigipickSuccess,
        CraftingConfirm,
        EquipmentConfirm,
        ShipSystemTick,
        ShipSystemAlert,
        CommsOpen,
        CommsClose
    };

    enum class SpeakerSampleFormat : std::uint8_t
    {
        Float32,
        Pcm16,
        Pcm32
    };

    struct StereoSpeakerFrame
    {
        float left{ 0.0F };
        float right{ 0.0F };
    };

    struct SpeakerCommand
    {
        SpeakerCategory category{ SpeakerCategory::ScannerUI };
        SpeakerEffectKind effect{ SpeakerEffectKind::ScannerTick };
        std::chrono::steady_clock::time_point when{};
        float gain{ 1.0F };
        SpeakerRoutingMode routing{ SpeakerRoutingMode::Both };
        float frequencyHz{ 440.0F };
        std::chrono::milliseconds duration{ 250 };
    };

    struct CapturedSoundIdentity
    {
        std::uint64_t id{ 0 };
        bool controllerOnlySafe{ false };
    };

    struct PreparedSpeakerPcm
    {
        std::vector<StereoSpeakerFrame> frames{};
        float gain{ 1.0F };
    };

    struct PersistentPreparedSpeakerLayer
    {
        std::shared_ptr<const PreparedSpeakerPcm> pcm{};
        std::size_t loopResumeFrame{ 0 };
    };

    struct PersistentPreparedSpeakerPcm
    {
        std::uint64_t owner{ 0 };
        std::shared_ptr<const PreparedSpeakerPcm> pcm{};
        std::size_t loopResumeFrame{ 0 };
        float gainScale{ 1.0F };
        std::vector<PersistentPreparedSpeakerLayer> layers{};
    };

    inline constexpr std::size_t kMaxSpeakerVoices = 32;
    inline constexpr std::size_t kMaxPreparedSpeakerChunks = 64;
}
