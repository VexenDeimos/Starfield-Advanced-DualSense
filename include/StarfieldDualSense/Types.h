#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <string>

namespace sds
{
    enum class ControllerType : std::uint8_t
    {
        Unknown,
        DualSense,
        DualSenseEdge
    };

    enum class ConnectionType : std::uint8_t
    {
        Unknown,
        Usb,
        Bluetooth,
        Virtual
    };

    struct Capabilities
    {
        bool adaptiveTriggers{ false };
        bool lightbar{ false };
        bool touchpadInput{ false };
        bool advancedHaptics{ false };
        bool controllerSpeaker{ false };
        bool bluetoothTransport{ false };
    };

    struct Color
    {
        std::uint8_t r{ 0 };
        std::uint8_t g{ 0 };
        std::uint8_t b{ 0 };

        friend constexpr bool operator==(const Color&, const Color&) = default;
    };

    enum class TriggerEffectMode : std::uint8_t
    {
        Off,
        ContinuousResistance,
        SectionResistance,
        EffectEx,
        Calibrate
    };

    struct TriggerEffect
    {
        TriggerEffectMode mode{ TriggerEffectMode::Off };
        std::uint8_t startPosition{ 0 };
        std::uint8_t endPosition{ 0 };
        std::uint8_t force{ 0 };
        bool keepEffect{ false };
        std::uint8_t beginForce{ 0 };
        std::uint8_t middleForce{ 0 };
        std::uint8_t endForce{ 0 };
        std::uint8_t frequency{ 0 };

        friend constexpr bool operator==(const TriggerEffect&, const TriggerEffect&) = default;
    };

    struct TouchPoint
    {
        std::uint16_t x{ 0 };
        std::uint16_t y{ 0 };
        std::uint8_t id{ 0 };
        bool down{ false };

        friend constexpr bool operator==(const TouchPoint&, const TouchPoint&) = default;
    };

    struct TouchState
    {
        TouchPoint first{};
        TouchPoint second{};
        std::uint8_t l2{ 0 };
        std::uint8_t r2{ 0 };
        bool r2Button{ false };
        bool click{ false };
        bool create{ false };

        friend constexpr bool operator==(const TouchState&, const TouchState&) = default;
    };

    enum class TouchGesture : std::uint8_t
    {
        None,
        ClickPressed,
        ClickReleased,
        SwipeLeft,
        SwipeRight,
        SwipeUp,
        SwipeDown,
        RightClick,
        RightHold,
        CreatePressed
    };

    enum class InputAction : std::uint8_t
    {
        OpenInventory,
        OpenMissions,
        OpenDataMenu,
        OpenSkills,
        OpenMap,
        OpenPowers,
        OpenPhotoMode
    };

    enum class GameEventType : std::uint8_t
    {
        WeaponEquipped,
        WeaponFired,
        ReloadCompleted,
        MeleeSwing,
        MeleeImpact,
        AimStarted,
        AimStopped,
        PlayerHealthChanged,
        MenuOpened,
        MenuClosed,
        GamePaused,
        GameUnpaused,
        Shutdown,
        IncomingDamage,
        ShipPilotEntered,
        ShipPilotExited,
        ShipPilotInvalidated,
        ShipPilotResumed,
        ShipBallisticWeaponFired,
        ShipLaserWeaponFired,
        ShipLaserWeaponStopped,
        ShipParticleWeaponFired,
        ShipMissileWeaponFired,
        ShipEMWeaponFired,
        ShipTouchdown,
        ShipLaunchLandingHapticsStarted,
        ShipLaunchLandingHapticsStopped,
        LandVehicleContextEntered,
        LandVehicleContextExited,
        LandVehicleAuthorityAcquired,
        LandVehicleAuthorityReleased,
        LandVehicleBoostStarted,
        LandVehicleTouchdown,
        LandVehicleGunFired,
        LandVehicleAimStarted,
        LandVehicleAimStopped
    };

    struct GameEvent
    {
        GameEventType type{ GameEventType::Shutdown };
        float value{ 0.0F };
        std::uint32_t formId{ 0 };
        std::array<char, 96> text{};
        std::chrono::steady_clock::time_point when{ std::chrono::steady_clock::now() };
    };

    struct DeviceIdentity
    {
        ControllerType type{ ControllerType::Unknown };
        ConnectionType connection{ ConnectionType::Unknown };
        std::uint16_t vendorId{ 0 };
        std::uint16_t productId{ 0 };
        std::uint16_t inputReportLength{ 0 };
        std::wstring path{};

        [[nodiscard]] constexpr bool supported() const noexcept
        {
            return type != ControllerType::Unknown;
        }
    };

    struct OutputState
    {
        Color lightbar{};
        TriggerEffect leftTrigger{};
        TriggerEffect rightTrigger{};
        std::uint8_t playerLeds{ 0 };
        bool disableLeds{ false };
    };

    struct EffectState
    {
        OutputState output{};
        bool transientTriggerActive{ false };
        std::chrono::steady_clock::time_point transientUntil{};
    };
}
