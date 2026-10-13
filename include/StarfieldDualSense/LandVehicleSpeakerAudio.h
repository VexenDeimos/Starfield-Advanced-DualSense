#pragma once
#include <StarfieldDualSense/SpeakerTypes.h>
#include <atomic>
#include <memory>

namespace sds {
    // Exact filtered media, not an arbitrary variant in either broad Wwise event.
    inline constexpr std::uint32_t kRoverEngineEventId = 0x6DD5E5A1u;
    inline constexpr std::uint32_t kRoverMountedLaserEventId = 0x3DD3DADDu;
    inline constexpr std::uint32_t kRoverVerticalBoostEventId = 0xF6A67354u;
    inline constexpr std::uint64_t kRoverEngineSpeakerOwner = 0x534453524F564552ull;
    struct LandVehicleSpeakerClip {
        std::shared_ptr<const PreparedSpeakerPcm> pcm{};
        std::size_t loopResumeFrame{0};
    };
    struct LandVehicleSpeakerSounds {
        LandVehicleSpeakerClip idle{};
        LandVehicleSpeakerClip low{};
        LandVehicleSpeakerClip high{};
        LandVehicleSpeakerClip forwardThrust{};
        LandVehicleSpeakerClip verticalThrust{};
        std::shared_ptr<const PreparedSpeakerPcm> laser{};
    };
    class LandVehicleSpeakerCache {
    public:
        [[nodiscard]] bool publish(LandVehicleSpeakerSounds sounds) noexcept {
            // One failed WEM must not prevent the other rover sounds from playing.
            bool ready = sounds.laser && !sounds.laser->frames.empty();
            for (const auto* clip : {&sounds.idle, &sounds.low, &sounds.high,
                                     &sounds.forwardThrust, &sounds.verticalThrust}) {
                if (!clip->pcm) continue;
                if (clip->pcm->frames.empty() || clip->loopResumeFrame >= clip->pcm->frames.size())
                    return false;
                ready = true;
            }
            if (!ready) return false;
            try {
                _snapshot.store(std::make_shared<const LandVehicleSpeakerSounds>(std::move(sounds)),
                    std::memory_order_release);
                return true;
            } catch (...) { return false; }
        }
        [[nodiscard]] std::shared_ptr<const LandVehicleSpeakerSounds> snapshot() const noexcept {
            return _snapshot.load(std::memory_order_acquire);
        }
    private:
        std::atomic<std::shared_ptr<const LandVehicleSpeakerSounds>> _snapshot{};
    };
}
