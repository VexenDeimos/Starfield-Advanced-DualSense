#pragma once

#include <StarfieldDualSense/WeaponProfiles.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

namespace sds
{
    enum class FireMarkerKind : std::uint8_t
    {
        None,
        Other,
        FireStart,
        FireEnd,
        Shot,
        MeleeSwing
    };


    enum class FireMarkerAction : std::uint8_t
    {
        None,
        ShotPulse,
        SustainedStart,
        SustainedEnd,
        SustainedHeartbeat,
        MeleeSwingPulse
    };

    enum class NovablastChargeMarkerAction : std::uint8_t
    {
        None,
        Start,
        Stop
    };

    [[nodiscard]] inline bool isSpeakerReloadMarker(std::string_view tag) noexcept
    {
        return tag == "ReloadComplete";
    }

    [[nodiscard]] inline NovablastChargeMarkerAction routeNovablastChargeMarker(
        std::string_view tag,
        std::string_view payload) noexcept
    {
        if (payload != "WPN_Charge_Generic") {
            return NovablastChargeMarkerAction::None;
        }
        if (tag == "SoundPlay") {
            return NovablastChargeMarkerAction::Start;
        }
        if (tag == "SoundStop") {
            return NovablastChargeMarkerAction::Stop;
        }
        return NovablastChargeMarkerAction::None;
    }

    inline FireMarkerAction routeFireMarker(
        FireMarkerKind kind,
        WeaponTriggerFamily family) noexcept
    {
        // Only true continuous-energy tools (Cutter/Arc Welder family) use
        // one sustained texture. High-rate guns such as Microgun/Magstorm may
        // have Sustained cadence, but still need one pulse per WeaponFire.
        if (family == WeaponTriggerFamily::SustainedEnergy) {
            if (kind == FireMarkerKind::FireStart) {
                return FireMarkerAction::SustainedStart;
            }
            if (kind == FireMarkerKind::FireEnd) {
                return FireMarkerAction::SustainedEnd;
            }
            if (kind == FireMarkerKind::Shot) {
                return FireMarkerAction::SustainedHeartbeat;
            }
            return FireMarkerAction::None;
        }
        if (family == WeaponTriggerFamily::Melee) {
            return kind == FireMarkerKind::MeleeSwing ?
                FireMarkerAction::MeleeSwingPulse :
                FireMarkerAction::None;
        }
        return kind == FireMarkerKind::Shot ? FireMarkerAction::ShotPulse : FireMarkerAction::None;
    }

    struct DecodedFireMarkerTag
    {
        bool valid{ false };
        FireMarkerKind kind{ FireMarkerKind::None };
        std::string text{};
    };

    inline DecodedFireMarkerTag decodeInlineFireMarkerTag(
        const std::array<std::byte, 64>& objectBytes) noexcept
    {
        constexpr std::size_t kLengthOffset = 0x08;
        constexpr std::size_t kInlineTextOffset = 0x18;
        constexpr std::size_t kMaxInlineTagLength = 31;

        std::uint64_t rawLength = 0;
        std::memcpy(&rawLength, objectBytes.data() + kLengthOffset, sizeof(rawLength));
        const auto length = static_cast<std::size_t>(rawLength);
        if (length == 0 || length > kMaxInlineTagLength ||
            kInlineTextOffset + length >= objectBytes.size()) {
            return {};
        }

        std::string text;
        text.reserve(length);
        for (std::size_t index = 0; index < length; ++index) {
            const auto value = static_cast<unsigned char>(objectBytes[kInlineTextOffset + index]);
            if (value < 0x20 || value > 0x7E) {
                return {};
            }
            text.push_back(static_cast<char>(value));
        }

        if (objectBytes[kInlineTextOffset + length] != std::byte{ 0 }) {
            return {};
        }

        FireMarkerKind kind = FireMarkerKind::Other;
        if (text == "weaponFireStart") {
            kind = FireMarkerKind::FireStart;
        } else if (text == "weaponFireEnd") {
            kind = FireMarkerKind::FireEnd;
        } else if (text == "WeaponFire") {
            kind = FireMarkerKind::Shot;
        } else if (text == "weaponSwing") {
            kind = FireMarkerKind::MeleeSwing;
        }

        return {
            .valid = true,
            .kind = kind,
            .text = std::move(text)
        };
    }
}
