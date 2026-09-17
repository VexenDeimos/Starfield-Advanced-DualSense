#pragma once

#include <StarfieldDualSense/HapticTypes.h>
#include <StarfieldDualSense/Types.h>
#include <StarfieldDualSense/WeaponProfiles.h>

#include <optional>

namespace sds
{
    class HapticsEngine
    {
    public:
        explicit HapticsEngine(float hapticStrength = 1.0F) noexcept;
        void setHapticStrength(float strength) noexcept;
        [[nodiscard]] std::optional<HapticCommand> handle(const GameEvent& event) noexcept;
        [[nodiscard]] HapticContinuousState handleRightTriggerInput(std::uint8_t r2) noexcept;

    private:
        float _hapticStrength{ 1.0F };
        const WeaponProfile* _equipped{ nullptr };
        std::uint32_t _equippedFormId{ 0 };
        bool _cutterBeamAuthorized{ false };
        bool _arcWelderAuthorized{ false };
        bool _penumbraStressActive{ false };
        bool _magsniperChargeActive{ false };
        bool _novablastChargeAuthorized{ false };
        bool _autoRivetChargeAuthorized{ false };
        bool _gamePaused{ false };
        std::uint8_t _blockingMenuMask{ 0 };
    };
}
