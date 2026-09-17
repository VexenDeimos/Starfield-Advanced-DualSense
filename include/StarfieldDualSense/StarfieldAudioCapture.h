#pragma once

#include <StarfieldDualSense/UiAudioDiscoveryProbe.h>
#include <StarfieldDualSense/MusicReconTypes.h>
#include <StarfieldDualSense/MusicSelectionRecon.h>
#include <StarfieldDualSense/WeaponSfxDiscoveryProbe.h>
#include <StarfieldDualSense/WwiseRemoteVoMirrorGate.h>

#include <functional>
#include <memory>
#include <string_view>

namespace sds
{
    class StarfieldAudioCapture
    {
    public:
        struct Impl;
        using LogCallback = std::function<void(std::string_view)>;
        using RemoteVoMirrorCallback = std::function<std::uint32_t(const RemoteVoMirrorRequest&)>;
        using RemoteVoSourceCallback = std::function<void(const RemoteVoMirrorRequest&)>;
        using WeaponSfxObservationCallback = std::function<void(const WeaponSfxWwiseObservation&)>;
        using UiAudioObservationCallback = std::function<void(const UiAudioWwiseObservation&)>;
        using MusicReconObservationCallback = std::function<void(const MusicReconWwiseObservation&)>;
        using MusicSelectionObservationCallback = std::function<void(const MusicSelectionObservation&)>;

        explicit StarfieldAudioCapture(
            LogCallback log = {},
            RemoteVoMirrorCallback mirror = {},
            VoMirrorQualificationProfile qualification = kRemoteCommsVoMirrorProfile,
            RemoteVoSourceCallback sourceProbe = {},
            WeaponSfxObservationCallback weaponSfxObservation = {},
            UiAudioObservationCallback uiAudioObservation = {},
            MusicReconObservationCallback musicReconObservation = {},
            MusicSelectionObservationCallback musicSelectionObservation = {});
        ~StarfieldAudioCapture();

        StarfieldAudioCapture(const StarfieldAudioCapture&) = delete;
        StarfieldAudioCapture& operator=(const StarfieldAudioCapture&) = delete;

        [[nodiscard]] bool start();
        void stop() noexcept;
        void drainDiagnostics();
        void setDialogueMenuActive(bool active) noexcept;
        void setWeaponSfxDiscoveryArmed(bool armed) noexcept;
        void setShipWeaponObservationArmed(bool armed) noexcept;
        [[nodiscard]] std::uint64_t takeWeaponSfxDropped() noexcept;
        void setUiAudioDiscoveryArmed(bool armed) noexcept;
        void setUiAudioPlaybackArmed(bool armed) noexcept;
        [[nodiscard]] std::uint64_t takeUiAudioDropped() noexcept;
        void setMusicReconArmed(bool armed) noexcept;
        void setMusicSelectionArmed(bool armed) noexcept;
        [[nodiscard]] std::uint64_t takeMusicReconDropped() noexcept;
        [[nodiscard]] bool active() const noexcept;

    private:
        std::unique_ptr<Impl> _impl;
    };
}
