#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <string_view>

namespace sds
{
    enum class HapticEffectKind : std::uint8_t
    {
        EonSnap,
        BridgerConcussion,
        MicrogunKick,
        BallisticHandgunKick,
        BallisticRapidKick,
        BallisticRifleKick,
        PrecisionBallisticKick,
        LauncherConcussion,
        ParticleLauncherConcussion,
        MagneticPulse,
        MagneticRapid,
        MagneticPrecision,
        ShotgunBlast,
        LaserPulse,
        ParticlePulse,
        NovablastDischarge,
        MeleeLightSwing,
        MeleeHeavySwing,
        MeleeVeryHeavySwing,
        MeleeLightImpact,
        MeleeHeavyImpact,
        MeleeVeryHeavyImpact,
        IncomingDamageImpact,
        ShipBallisticCannonKick,
        ShipLaserPulseCrest,
        ShipParticlePulse,
        ShipMissileLaunchThump,
        ShipEMPulse,
        ShipTouchdownThump,
        BoostpackIgnition,
        DigipickRotateTick,
        DigipickSelectClick,
        DigipickInsertClunk,
        DigipickSuccess,
        LandVehicleBoostKick,
        LandVehicleTouchdownThump,
        LandVehicleGunRecoil
    };

    [[nodiscard]] constexpr bool isMeleeImpactHapticEffect(HapticEffectKind kind) noexcept
    {
        return kind == HapticEffectKind::MeleeLightImpact ||
            kind == HapticEffectKind::MeleeHeavyImpact ||
            kind == HapticEffectKind::MeleeVeryHeavyImpact;
    }

    [[nodiscard]] constexpr std::string_view hapticEffectKindName(HapticEffectKind kind) noexcept
    {
        switch (kind) {
        case HapticEffectKind::EonSnap: return "EonSnap";
        case HapticEffectKind::BridgerConcussion: return "BridgerConcussion";
        case HapticEffectKind::MicrogunKick: return "MicrogunKick";
        case HapticEffectKind::BallisticHandgunKick: return "BallisticHandgunKick";
        case HapticEffectKind::BallisticRapidKick: return "BallisticRapidKick";
        case HapticEffectKind::BallisticRifleKick: return "BallisticRifleKick";
        case HapticEffectKind::PrecisionBallisticKick: return "PrecisionBallisticKick";
        case HapticEffectKind::LauncherConcussion: return "LauncherConcussion";
        case HapticEffectKind::ParticleLauncherConcussion: return "ParticleLauncherConcussion";
        case HapticEffectKind::MagneticPulse: return "MagneticPulse";
        case HapticEffectKind::MagneticRapid: return "MagneticRapid";
        case HapticEffectKind::MagneticPrecision: return "MagneticPrecision";
        case HapticEffectKind::ShotgunBlast: return "ShotgunBlast";
        case HapticEffectKind::LaserPulse: return "LaserPulse";
        case HapticEffectKind::ParticlePulse: return "ParticlePulse";
        case HapticEffectKind::NovablastDischarge: return "NovablastDischarge";
        case HapticEffectKind::MeleeLightSwing: return "MeleeLightSwing";
        case HapticEffectKind::MeleeHeavySwing: return "MeleeHeavySwing";
        case HapticEffectKind::MeleeVeryHeavySwing: return "MeleeVeryHeavySwing";
        case HapticEffectKind::MeleeLightImpact: return "MeleeLightImpact";
        case HapticEffectKind::MeleeHeavyImpact: return "MeleeHeavyImpact";
        case HapticEffectKind::MeleeVeryHeavyImpact: return "MeleeVeryHeavyImpact";
        case HapticEffectKind::IncomingDamageImpact: return "IncomingDamageImpact";
        case HapticEffectKind::ShipBallisticCannonKick: return "ShipBallisticCannonKick";
        case HapticEffectKind::ShipLaserPulseCrest: return "ShipLaserPulseCrest";
        case HapticEffectKind::ShipParticlePulse: return "ShipParticlePulse";
        case HapticEffectKind::ShipMissileLaunchThump: return "ShipMissileLaunchThump";
        case HapticEffectKind::ShipEMPulse: return "ShipEMPulse";
        case HapticEffectKind::ShipTouchdownThump: return "ShipTouchdownThump";
        case HapticEffectKind::BoostpackIgnition: return "BoostpackIgnition";
        case HapticEffectKind::DigipickRotateTick: return "DigipickRotateTick";
        case HapticEffectKind::DigipickSelectClick: return "DigipickSelectClick";
        case HapticEffectKind::DigipickInsertClunk: return "DigipickInsertClunk";
        case HapticEffectKind::DigipickSuccess: return "DigipickSuccess";
        case HapticEffectKind::LandVehicleBoostKick: return "LandVehicleBoostKick";
        case HapticEffectKind::LandVehicleTouchdownThump: return "LandVehicleTouchdownThump";
        case HapticEffectKind::LandVehicleGunRecoil: return "LandVehicleGunRecoil";
        }
        return "Unknown";
    }

    [[nodiscard]] inline std::int64_t hapticEventTimestampMicros(
        std::chrono::steady_clock::time_point when) noexcept
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            when.time_since_epoch()).count();
    }

    enum class HapticContinuousKind : std::uint8_t
    {
        None,
        NovablastCharge,
        CutterBeam,
        PenumbraStress,
        MagsniperCharge,
        ArcWelderArc,
        AutoRivetTension,
        BoostpackThrust,
        ShipPropulsion,
        ShipBoost,
        LandVehicleChassis
    };

    struct HapticContinuousState
    {
        HapticContinuousKind kind{ HapticContinuousKind::None };
        float gain{ 0.0F };
        float level{ 0.0F };
        float shipLaserGain{ 0.0F };

        friend bool operator==(const HapticContinuousState&, const HapticContinuousState&) = default;
    };

    [[nodiscard]] inline std::uint32_t packHapticContinuousState(
        HapticContinuousState state) noexcept
    {
        const auto quantize = [](float value) noexcept {
            const float clamped = std::clamp(value, 0.0F, 1.0F);
            return static_cast<std::uint32_t>(clamped * 255.0F + 0.5F);
        };

        const auto kind = static_cast<std::uint32_t>(state.kind) & 0xFFU;
        const auto gain = quantize(state.gain) & 0xFFU;
        const auto level = quantize(state.level) & 0xFFU;
        const auto shipLaserGain = quantize(state.shipLaserGain) & 0xFFU;
        if (kind == 0U && gain == 0U && level == 0U && shipLaserGain == 0U) {
            return 0U;
        }
        return kind | (gain << 8U) | (level << 16U) | (shipLaserGain << 24U);
    }

    [[nodiscard]] inline HapticContinuousState unpackHapticContinuousState(
        std::uint32_t packed) noexcept
    {
        if (packed == 0U) {
            return {};
        }

        return {
            .kind = static_cast<HapticContinuousKind>(packed & 0xFFU),
            .gain = static_cast<float>((packed >> 8U) & 0xFFU) / 255.0F,
            .level = static_cast<float>((packed >> 16U) & 0xFFU) / 255.0F,
            .shipLaserGain = static_cast<float>((packed >> 24U) & 0xFFU) / 255.0F,
        };
    }

    struct HapticCommand
    {
        HapticEffectKind kind{ HapticEffectKind::EonSnap };
        float gain{ 1.0F };
        std::chrono::steady_clock::time_point when{};
    };
}
