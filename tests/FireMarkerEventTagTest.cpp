#include <StarfieldDualSense/FireMarkerEventTag.h>
#include <StarfieldDualSense/WeaponProfiles.h>

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string_view>

namespace
{
    std::array<std::byte, 64> makeTagObject(std::string_view tag)
    {
        std::array<std::byte, 64> bytes{};
        const auto length = static_cast<std::uint64_t>(tag.size());
        std::memcpy(bytes.data() + 0x08, &length, sizeof(length));
        std::memcpy(bytes.data() + 0x18, tag.data(), tag.size());
        bytes[0x18 + tag.size()] = std::byte{0};
        return bytes;
    }
}

int main()
{
    {
        const auto bytes = makeTagObject("WeaponFire");
        const auto decoded = sds::decodeInlineFireMarkerTag(bytes);
        assert(decoded.valid);
        assert(decoded.text == "WeaponFire");
        assert(decoded.kind == sds::FireMarkerKind::Shot);
    }

    {
        const auto bytes = makeTagObject("weaponFireStart");
        const auto decoded = sds::decodeInlineFireMarkerTag(bytes);
        assert(decoded.valid);
        assert(decoded.text == "weaponFireStart");
        assert(decoded.kind == sds::FireMarkerKind::FireStart);
    }

    {
        const auto bytes = makeTagObject("BeginWeaponDraw");
        const auto decoded = sds::decodeInlineFireMarkerTag(bytes);
        assert(decoded.valid);
        assert(decoded.kind == sds::FireMarkerKind::Other);
    }

    {
        const auto bytes = makeTagObject("weaponSwing");
        const auto decoded = sds::decodeInlineFireMarkerTag(bytes);
        assert(decoded.valid);
        assert(decoded.text == "weaponSwing");
        assert(decoded.kind == sds::FireMarkerKind::MeleeSwing);
    }

    {
        const auto bytes = makeTagObject("weaponFireEnd");
        const auto decoded = sds::decodeInlineFireMarkerTag(bytes);
        assert(decoded.valid);
        assert(decoded.text == "weaponFireEnd");
        assert(decoded.kind == sds::FireMarkerKind::FireEnd);
    }

    {
        auto bytes = makeTagObject("WeaponFire");
        const std::uint64_t impossibleLength = 40;
        std::memcpy(bytes.data() + 0x08, &impossibleLength, sizeof(impossibleLength));
        const auto decoded = sds::decodeInlineFireMarkerTag(bytes);
        assert(!decoded.valid);
        assert(decoded.kind == sds::FireMarkerKind::None);
    }

    {
        auto bytes = makeTagObject("WeaponFire");
        bytes[0x18 + 2] = std::byte{0x01};
        const auto decoded = sds::decodeInlineFireMarkerTag(bytes);
        assert(!decoded.valid);
    }

    assert(sds::routeFireMarker(sds::FireMarkerKind::Shot, sds::WeaponTriggerFamily::BallisticRifle) ==
        sds::FireMarkerAction::ShotPulse);
    assert(sds::routeFireMarker(sds::FireMarkerKind::FireStart, sds::WeaponTriggerFamily::BallisticRifle) ==
        sds::FireMarkerAction::None);
    assert(sds::routeFireMarker(sds::FireMarkerKind::FireStart, sds::WeaponTriggerFamily::SustainedEnergy) ==
        sds::FireMarkerAction::SustainedStart);
    assert(sds::routeFireMarker(sds::FireMarkerKind::Shot, sds::WeaponTriggerFamily::SustainedEnergy) ==
        sds::FireMarkerAction::SustainedHeartbeat);
    assert(sds::routeFireMarker(sds::FireMarkerKind::FireEnd, sds::WeaponTriggerFamily::SustainedEnergy) ==
        sds::FireMarkerAction::SustainedEnd);
    // A high-rate ballistic weapon can have Sustained cadence but it still
    // needs one real pulse per WeaponFire marker. Routing is therefore based
    // on trigger family, not cadence class.
    assert(sds::routeFireMarker(sds::FireMarkerKind::Shot, sds::WeaponTriggerFamily::HeavyBallistic) ==
        sds::FireMarkerAction::ShotPulse);
    assert(sds::routeFireMarker(sds::FireMarkerKind::MeleeSwing, sds::WeaponTriggerFamily::Melee) ==
        sds::FireMarkerAction::MeleeSwingPulse);
    assert(sds::routeFireMarker(sds::FireMarkerKind::MeleeSwing, sds::WeaponTriggerFamily::BallisticRifle) ==
        sds::FireMarkerAction::None);

    assert(sds::routeNovablastChargeMarker("SoundPlay", "WPN_Charge_Generic") ==
        sds::NovablastChargeMarkerAction::Start);
    assert(sds::routeNovablastChargeMarker("SoundStop", "WPN_Charge_Generic") ==
        sds::NovablastChargeMarkerAction::Stop);
    assert(sds::routeNovablastChargeMarker("SoundPlay", "WPN_Unrelated") ==
        sds::NovablastChargeMarkerAction::None);
    assert(sds::routeNovablastChargeMarker("WeaponFire", "WPN_Charge_Generic") ==
        sds::NovablastChargeMarkerAction::None);

    assert(sds::isSpeakerReloadMarker("ReloadComplete"));
    assert(!sds::isSpeakerReloadMarker("ReloadAbort"));
    assert(!sds::isSpeakerReloadMarker("OnReloadExit"));

    return 0;
}
