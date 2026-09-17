#include <StarfieldDualSense/WeaponProfiles.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace
{
    using namespace sds;

    constexpr std::array<WeaponProfile, 71> kProfiles{{
        { "Eon", "Base", "Ballistic handgun", WeaponTriggerFamily::BallisticHandgun, WeaponIntensity::Light, WeaponCadenceClass::ReceiverAware, "Single / receiver-aware", 3, 3 },
        { "Sidestar", "Base", "Ballistic handgun", WeaponTriggerFamily::BallisticHandgun, WeaponIntensity::Light, WeaponCadenceClass::Single, "Single", 3, 3 },
        { "Rattler", "Base", "Ballistic handgun", WeaponTriggerFamily::BallisticHandgun, WeaponIntensity::Light, WeaponCadenceClass::ReceiverAware, "Single / receiver-aware", 3, 3 },
        { "Old Earth Pistol", "Base", "Ballistic handgun", WeaponTriggerFamily::BallisticHandgun, WeaponIntensity::Normal, WeaponCadenceClass::Single, "Single", 4, 4 },
        { "XM-2311", "Base", "Ballistic handgun", WeaponTriggerFamily::BallisticHandgun, WeaponIntensity::Normal, WeaponCadenceClass::Single, "Single", 4, 4 },
        { "Custom Antique Pistol", "Terran Armada", "Ballistic handgun", WeaponTriggerFamily::BallisticHandgun, WeaponIntensity::Normal, WeaponCadenceClass::Single, "Single", 4, 4 },
        { "Kraken", "Base", "Ballistic SMG", WeaponTriggerFamily::BallisticRapid, WeaponIntensity::Light, WeaponCadenceClass::Rapid, "Rapid", 2, 2 },
        { "Urban Eagle", "Base", "Heavy handgun", WeaponTriggerFamily::BallisticHandgun, WeaponIntensity::Heavy, WeaponCadenceClass::Single, "Single", 5, 6 },
        { "Regulator", "Base", "Heavy handgun", WeaponTriggerFamily::BallisticHandgun, WeaponIntensity::Heavy, WeaponCadenceClass::Single, "Single", 6, 6 },
        { "Razorback", "Base", "Heavy handgun", WeaponTriggerFamily::BallisticHandgun, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Single, "Single", 7, 8 },
        { "Custom Antique Revolver", "Terran Armada", "Heavy handgun", WeaponTriggerFamily::BallisticHandgun, WeaponIntensity::Heavy, WeaponCadenceClass::Single, "Single", 6, 7 },
        { "Kodama", "Base", "Ballistic SMG", WeaponTriggerFamily::BallisticRapid, WeaponIntensity::Light, WeaponCadenceClass::Rapid, "Rapid", 3, 3 },
        { "Grendel", "Base", "Ballistic SMG / rifle", WeaponTriggerFamily::BallisticRapid, WeaponIntensity::Light, WeaponCadenceClass::Rapid, "Rapid", 3, 3 },
        { "Antique Submachine Gun", "Terran Armada", "Ballistic SMG", WeaponTriggerFamily::BallisticRapid, WeaponIntensity::Normal, WeaponCadenceClass::Rapid, "Rapid", 4, 4 },
        { "Maelstrom", "Base", "Ballistic rifle", WeaponTriggerFamily::BallisticRifle, WeaponIntensity::Normal, WeaponCadenceClass::ReceiverAware, "Receiver-aware", 4, 4 },
        { "AA-99", "Base", "Ballistic rifle", WeaponTriggerFamily::BallisticRifle, WeaponIntensity::Normal, WeaponCadenceClass::ReceiverAware, "Receiver-aware", 5, 5 },
        { "Drum Beat", "Base", "Ballistic rifle", WeaponTriggerFamily::BallisticRifle, WeaponIntensity::Normal, WeaponCadenceClass::Rapid, "Rapid", 4, 4 },
        { "Beowulf", "Base", "Ballistic rifle", WeaponTriggerFamily::BallisticRifle, WeaponIntensity::Heavy, WeaponCadenceClass::ReceiverAware, "Receiver-aware", 5, 6 },
        { "Tombstone", "Base", "Ballistic rifle", WeaponTriggerFamily::BallisticRifle, WeaponIntensity::Heavy, WeaponCadenceClass::Rapid, "Rapid", 5, 6 },
        { "Old Earth Assault Rifle", "Base", "Ballistic rifle", WeaponTriggerFamily::BallisticRifle, WeaponIntensity::Normal, WeaponCadenceClass::Rapid, "Rapid", 5, 5 },
        { "MGP Ballistic Rifle", "Terran Armada", "Ballistic rifle", WeaponTriggerFamily::BallisticRifle, WeaponIntensity::Heavy, WeaponCadenceClass::ReceiverAware, "Receiver-aware", 6, 6 },
        { "Lawgiver", "Base", "Precision ballistic", WeaponTriggerFamily::PrecisionBallistic, WeaponIntensity::Heavy, WeaponCadenceClass::Precision, "Precision single", 6, 7 },
        { "Old Earth Hunting Rifle", "Base", "Precision ballistic", WeaponTriggerFamily::PrecisionBallistic, WeaponIntensity::Heavy, WeaponCadenceClass::Precision, "Precision single", 6, 7 },
        { "Hard Target", "Base", "Precision ballistic", WeaponTriggerFamily::PrecisionBallistic, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Precision, "Precision single", 8, 9 },
        { "Coachman", "Base", "Shotgun", WeaponTriggerFamily::Shotgun, WeaponIntensity::Heavy, WeaponCadenceClass::Shotgun, "Break-action", 7, 8 },
        { "Old Earth Shotgun", "Base", "Shotgun", WeaponTriggerFamily::Shotgun, WeaponIntensity::Heavy, WeaponCadenceClass::Shotgun, "Shotgun cycle", 7, 8 },
        { "Pacifier", "Base", "Shotgun", WeaponTriggerFamily::Shotgun, WeaponIntensity::Heavy, WeaponCadenceClass::Shotgun, "Shotgun", 6, 7 },
        { "Breach", "Base", "Shotgun", WeaponTriggerFamily::Shotgun, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Shotgun, "Shotgun", 8, 9 },
        { "Shotty", "Base", "Shotgun", WeaponTriggerFamily::Shotgun, WeaponIntensity::Heavy, WeaponCadenceClass::Shotgun, "Rapid shotgun", 5, 6 },
        { "Auto-Rivet", "Base", "Industrial ballistic", WeaponTriggerFamily::HeavyBallistic, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Single, "Slow heavy fire", 7, 8 },
        { "Microgun", "Base", "Heavy ballistic auto", WeaponTriggerFamily::HeavyBallistic, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Sustained, "Sustained rapid", 7, 7 },
        { "Bridger", "Base", "Explosive launcher", WeaponTriggerFamily::Launcher, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Launcher, "Launcher", 9, 10 },
        { "Negotiator", "Base", "Explosive launcher", WeaponTriggerFamily::Launcher, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Launcher, "Launcher", 9, 10 },
        { "Breechblock", "Terran Armada", "Explosive heavy", WeaponTriggerFamily::Launcher, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Launcher, "Launcher", 9, 10 },
        { "Magshot", "Base", "Magnetic", WeaponTriggerFamily::Magnetic, WeaponIntensity::Heavy, WeaponCadenceClass::Single, "Single", 5, 6 },
        { "Magshear", "Base", "Magnetic", WeaponTriggerFamily::Magnetic, WeaponIntensity::Normal, WeaponCadenceClass::Rapid, "Rapid", 4, 5 },
        { "Magpulse", "Base", "Magnetic", WeaponTriggerFamily::Magnetic, WeaponIntensity::Heavy, WeaponCadenceClass::Single, "Pulse", 6, 7 },
        { "Magsniper", "Base", "Magnetic", WeaponTriggerFamily::Magnetic, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Charge, "Charge / precision", 8, 9 },
        { "Magstorm", "Base", "Magnetic heavy", WeaponTriggerFamily::Magnetic, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Sustained, "Sustained rapid", 7, 8 },
        { "Solstice", "Base", "Laser", WeaponTriggerFamily::Laser, WeaponIntensity::Light, WeaponCadenceClass::Single, "Energy pulse", 3, 2 },
        { "Va'ruun Quickstrike", "Shattered Space", "Laser", WeaponTriggerFamily::Laser, WeaponIntensity::Normal, WeaponCadenceClass::Rapid, "Fast pulse", 4, 3 },
        { "Cathode", "Terran Armada", "Laser", WeaponTriggerFamily::Laser, WeaponIntensity::Normal, WeaponCadenceClass::Rapid, "Burst / auto capable", 4, 3 },
        { "Equinox", "Base", "Laser", WeaponTriggerFamily::Laser, WeaponIntensity::Normal, WeaponCadenceClass::Rapid, "Rapid pulse", 4, 4 },
        { "Orion", "Base", "Laser", WeaponTriggerFamily::Laser, WeaponIntensity::Heavy, WeaponCadenceClass::Single, "Pulse rifle", 5, 5 },
        { "Va'ruun Longfang", "Shattered Space", "Laser", WeaponTriggerFamily::Laser, WeaponIntensity::Heavy, WeaponCadenceClass::Rapid, "Rapid pulse", 5, 5 },
        { "Va'ruun Starlash", "Shattered Space", "Laser", WeaponTriggerFamily::Laser, WeaponIntensity::Heavy, WeaponCadenceClass::Precision, "Precision pulse", 6, 6 },
        { "MGP Laser Rifle", "Terran Armada", "Laser", WeaponTriggerFamily::Laser, WeaponIntensity::Heavy, WeaponCadenceClass::ReceiverAware, "Receiver-aware", 5, 5 },
        { "Dogfight", "Terran Armada", "Laser", WeaponTriggerFamily::Laser, WeaponIntensity::Normal, WeaponCadenceClass::Rapid, "Rapid", 4, 4 },
        { "Resonator", "Terran Armada", "Laser precision", WeaponTriggerFamily::Laser, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Precision, "Precision single", 8, 7 },
        { "Novalight", "Base", "Particle", WeaponTriggerFamily::Particle, WeaponIntensity::Normal, WeaponCadenceClass::Single, "Single", 4, 5 },
        { "Va'ruun Starshard", "Base", "Particle", WeaponTriggerFamily::Particle, WeaponIntensity::Heavy, WeaponCadenceClass::Single, "Single", 5, 6 },
        { "Va'ruun Inflictor", "Base", "Particle", WeaponTriggerFamily::Particle, WeaponIntensity::Heavy, WeaponCadenceClass::Single, "Rifle pulse", 6, 7 },
        { "Big Bang", "Base", "Particle shotgun", WeaponTriggerFamily::Shotgun, WeaponIntensity::Heavy, WeaponCadenceClass::Shotgun, "Shotgun pulse", 7, 8 },
        { "Va'ruun Penumbra", "Shattered Space", "Particle explosive", WeaponTriggerFamily::Launcher, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Launcher, "Particle launcher", 8, 9 },
        { "Va'ruun Starstorm", "Shattered Space", "Particle heavy auto", WeaponTriggerFamily::Particle, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Sustained, "Sustained rapid", 7, 8 },
        { "Arc Welder", "Base", "Sustained energy / arc", WeaponTriggerFamily::SustainedEnergy, WeaponIntensity::Heavy, WeaponCadenceClass::Sustained, "Sustained", 5, 6 },
        { "Cutter", "Base", "Continuous beam", WeaponTriggerFamily::SustainedEnergy, WeaponIntensity::Normal, WeaponCadenceClass::Sustained, "Continuous beam", 4, 4 },
        { "Novablast Disruptor", "Base", "EM / stun", WeaponTriggerFamily::EM, WeaponIntensity::Heavy, WeaponCadenceClass::Charge, "Charge", 6, 5 },
        { "Ripshank", "Base", "Knife", WeaponTriggerFamily::Melee, WeaponIntensity::Light, WeaponCadenceClass::Melee, "Very fast stab / slash", 2, 2 },
        { "Combat Knife", "Base", "Knife", WeaponTriggerFamily::Melee, WeaponIntensity::Light, WeaponCadenceClass::Melee, "Fast stab / slash", 2, 3 },
        { "Barrow Knife", "Base", "Knife", WeaponTriggerFamily::Melee, WeaponIntensity::Light, WeaponCadenceClass::Melee, "Fast slash", 2, 3 },
        { "Injection Knife", "Terran Armada", "Knife", WeaponTriggerFamily::Melee, WeaponIntensity::Normal, WeaponCadenceClass::Melee, "Deliberate stab", 3, 4 },
        { "Osmium Dagger", "Base", "Dagger", WeaponTriggerFamily::Melee, WeaponIntensity::Normal, WeaponCadenceClass::Melee, "Dense short strike", 3, 4 },
        { "Tanto", "Base", "Short blade", WeaponTriggerFamily::Melee, WeaponIntensity::Normal, WeaponCadenceClass::Melee, "Fast slash", 3, 4 },
        { "Wakizashi", "Base", "Sword", WeaponTriggerFamily::Melee, WeaponIntensity::Heavy, WeaponCadenceClass::Melee, "Longer slash", 4, 5 },
        { "UC Naval Cutlass", "Base", "Sword", WeaponTriggerFamily::Melee, WeaponIntensity::Heavy, WeaponCadenceClass::Melee, "Broad slash", 4, 5 },
        { "Va'ruun Painblade", "Base", "Exotic blade", WeaponTriggerFamily::Melee, WeaponIntensity::Heavy, WeaponCadenceClass::Melee, "Aggressive slash / stab", 5, 6 },
        { "Va'ruun Schimaz", "Shattered Space", "Heavy sword", WeaponTriggerFamily::Melee, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Melee, "Heavy slash", 6, 7 },
        { "Rescue Axe", "Base", "Axe", WeaponTriggerFamily::Melee, WeaponIntensity::Heavy, WeaponCadenceClass::Melee, "Weighty chop", 5, 6 },
        { "Double Edged Hatchet", "Terran Armada", "Axe", WeaponTriggerFamily::Melee, WeaponIntensity::Heavy, WeaponCadenceClass::Melee, "Fast weighted chop", 5, 6 },
        { "Mauling Axe", "Terran Armada", "Heavy axe", WeaponTriggerFamily::Melee, WeaponIntensity::VeryHeavy, WeaponCadenceClass::Melee, "Slow brutal chop", 7, 9 },
    }};

    std::string normalizeIdentity(std::string_view value)
    {
        std::string normalized;
        normalized.reserve(value.size());
        for (const unsigned char ch : value) {
            if (std::isalnum(ch) != 0) {
                normalized.push_back(static_cast<char>(std::tolower(ch)));
            }
        }
        return normalized;
    }
}

std::span<const sds::WeaponProfile> sds::weaponProfiles() noexcept
{
    return kProfiles;
}

const sds::WeaponProfile* sds::findWeaponProfile(std::string_view identity) noexcept
{
    if (identity.empty()) {
        return nullptr;
    }

    const auto normalizedIdentity = normalizeIdentity(identity);
    if (normalizedIdentity.empty()) {
        return nullptr;
    }

    const WeaponProfile* bestMatch = nullptr;
    std::size_t bestMatchLength = 0u;
    for (const auto& profile : kProfiles) {
        const auto normalizedName = normalizeIdentity(profile.name);
        if (normalizedName.empty() || normalizedIdentity.find(normalizedName) == std::string::npos) {
            continue;
        }
        if (normalizedName.size() > bestMatchLength) {
            bestMatch = &profile;
            bestMatchLength = normalizedName.size();
        }
    }
    return bestMatch;
}

std::string_view sds::weaponIntensityName(WeaponIntensity intensity) noexcept
{
    switch (intensity) {
    case WeaponIntensity::Light: return "Light";
    case WeaponIntensity::Normal: return "Normal";
    case WeaponIntensity::Heavy: return "Heavy";
    case WeaponIntensity::VeryHeavy: return "Very Heavy";
    }
    return "Unknown";
}

std::string_view sds::weaponTriggerFamilyName(WeaponTriggerFamily family) noexcept
{
    switch (family) {
    case WeaponTriggerFamily::BallisticHandgun: return "Ballistic handgun";
    case WeaponTriggerFamily::BallisticRapid: return "Ballistic rapid";
    case WeaponTriggerFamily::BallisticRifle: return "Ballistic rifle";
    case WeaponTriggerFamily::PrecisionBallistic: return "Precision ballistic";
    case WeaponTriggerFamily::Shotgun: return "Shotgun";
    case WeaponTriggerFamily::HeavyBallistic: return "Heavy ballistic";
    case WeaponTriggerFamily::Launcher: return "Launcher";
    case WeaponTriggerFamily::Magnetic: return "Magnetic";
    case WeaponTriggerFamily::Laser: return "Laser";
    case WeaponTriggerFamily::Particle: return "Particle";
    case WeaponTriggerFamily::SustainedEnergy: return "Sustained energy";
    case WeaponTriggerFamily::EM: return "EM / stun";
    case WeaponTriggerFamily::Melee: return "Melee";
    }
    return "Unknown";
}
