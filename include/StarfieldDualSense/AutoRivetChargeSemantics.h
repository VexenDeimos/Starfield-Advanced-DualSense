#pragma once

#include <cstdint>
#include <string_view>

namespace sds
{
    enum class AutoRivetChargeAction : std::uint8_t
    {
        None,
        Start,
        Stop,
    };

    // Wwise ShortIDs. Charge_Stop was observed live on the player object in the
    // v0.3.38 discovery run; Charge_Start is the paired Wwise event ShortID for
    // WPN_Rifle_AutoRivet_Charge_Start.
    inline constexpr std::uint32_t kAutoRivetChargeStartEventId = 0xB963BD33u;
    inline constexpr std::uint32_t kAutoRivetChargeStopEventId = 0xCDEE7F71u;
    inline constexpr std::uint64_t kAutoRivetPlayerGameObjectId = 0x2u;

    [[nodiscard]] constexpr AutoRivetChargeAction routeAutoRivetChargeWwise(
        std::uint32_t eventId,
        std::uint64_t gameObjectId) noexcept
    {
        if (gameObjectId != kAutoRivetPlayerGameObjectId) {
            return AutoRivetChargeAction::None;
        }
        if (eventId == kAutoRivetChargeStartEventId) {
            return AutoRivetChargeAction::Start;
        }
        if (eventId == kAutoRivetChargeStopEventId) {
            return AutoRivetChargeAction::Stop;
        }
        return AutoRivetChargeAction::None;
    }

    [[nodiscard]] constexpr std::string_view autoRivetChargeMarker(
        AutoRivetChargeAction action) noexcept
    {
        switch (action) {
            case AutoRivetChargeAction::Start:
                return "AutoRivetChargeStart";
            case AutoRivetChargeAction::Stop:
                return "AutoRivetChargeStop";
            default:
                return {};
        }
    }
}
