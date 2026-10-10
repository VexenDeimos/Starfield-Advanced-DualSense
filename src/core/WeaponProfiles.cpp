#include <StarfieldDualSense/WeaponProfiles.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <memory>
#include <string>
#include <vector>

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
                normalized.push_back(
                    static_cast<char>(
                        std::tolower(ch)));
            }
        }

        return normalized;
    }

    std::string_view trim(std::string_view value)
    {
        while (
            !value.empty() &&
            std::isspace(
                static_cast<unsigned char>(
                    value.front())) != 0) {
            value.remove_prefix(1);
        }

        while (
            !value.empty() &&
            std::isspace(
                static_cast<unsigned char>(
                    value.back())) != 0) {
            value.remove_suffix(1);
        }

        return value;
    }

    std::string_view unquote(std::string_view value)
    {
        value = trim(value);

        if (
            value.size() >= 2 &&
            value.front() == '"' &&
            value.back() == '"') {
            value.remove_prefix(1);
            value.remove_suffix(1);
        }

        return value;
    }

    std::string_view removeInlineComment(
        std::string_view value)
    {
        bool quoted = false;
        bool escaped = false;

        for (
            std::size_t i = 0;
            i < value.size();
            ++i) {
            const char ch = value[i];

            if (escaped) {
                escaped = false;
                continue;
            }

            if (quoted && ch == '\\') {
                escaped = true;
                continue;
            }

            if (ch == '"') {
                quoted = !quoted;
                continue;
            }

            if (ch == '#' && !quoted) {
                return value.substr(0, i);
            }
        }

        return value;
    }

    const WeaponProfile* findBuiltInWeaponProfileExact(
        std::string_view name) noexcept
    {
        const auto normalized =
            normalizeIdentity(name);

        if (normalized.empty()) {
            return nullptr;
        }

        for (const auto& profile : kProfiles) {
            if (
                normalizeIdentity(profile.name) ==
                normalized) {
                return &profile;
            }
        }

        return nullptr;
    }

    const WeaponProfile* findBuiltInWeaponProfile(
        std::string_view identity) noexcept
    {
        if (identity.empty()) {
            return nullptr;
        }

        const auto normalizedIdentity =
            normalizeIdentity(identity);

        if (normalizedIdentity.empty()) {
            return nullptr;
        }

        const WeaponProfile* bestMatch = nullptr;
        std::size_t bestMatchLength = 0u;

        for (const auto& profile : kProfiles) {
            const auto normalizedName =
                normalizeIdentity(profile.name);

            if (
                normalizedName.empty() ||
                normalizedIdentity.find(
                    normalizedName) ==
                    std::string::npos) {
                continue;
            }

            if (
                normalizedName.size() >
                bestMatchLength) {
                bestMatch = &profile;
                bestMatchLength =
                    normalizedName.size();
            }
        }

        return bestMatch;
    }

    struct CustomWeaponAlias
    {
        std::string normalizedEditorId{};
        const WeaponProfile* profile{ nullptr };
    };

    using CustomWeaponAliasList =
        std::vector<CustomWeaponAlias>;

    std::atomic_bool
        g_customWeaponsEnabled{ true };

    std::atomic<
        std::shared_ptr<
            const CustomWeaponAliasList>>
        g_customWeaponAliases{};

    bool customEditorIdMatchesIdentity(
        std::string_view normalizedEditorId,
        std::string_view identity)
    {
        if (
            normalizedEditorId.empty() ||
            identity.empty()) {
            return false;
        }

        const auto separator =
            identity.find('|');

        const auto editorId =
            separator == std::string_view::npos
                ? identity
                : identity.substr(0, separator);

        const auto normalizedIdentityEditorId =
            normalizeIdentity(editorId);

        return
            !normalizedIdentityEditorId.empty() &&
            normalizedIdentityEditorId ==
                normalizedEditorId;
    }

    const WeaponProfile* findCustomWeaponProfile(
        std::string_view identity) noexcept
    {
        if (
            !g_customWeaponsEnabled.load(
                std::memory_order_acquire)) {
            return nullptr;
        }

        const auto aliases =
            g_customWeaponAliases.load(
                std::memory_order_acquire);

        if (!aliases) {
            return nullptr;
        }

        for (const auto& alias : *aliases) {
            if (
                alias.profile &&
                customEditorIdMatchesIdentity(
                    alias.normalizedEditorId,
                    identity)) {
                return alias.profile;
            }
        }

        return nullptr;
    }
}

std::span<const sds::WeaponProfile>
    sds::weaponProfiles() noexcept
{
    return kProfiles;
}

sds::CustomWeaponProfileConfigResult
    sds::configureCustomWeaponProfiles(
        bool enabled,
        std::string_view tomlText) noexcept
{
    CustomWeaponProfileConfigResult result{};

    g_customWeaponsEnabled.store(
        enabled,
        std::memory_order_release);

    try {
        CustomWeaponAliasList aliases;

        bool inCustomWeaponBlock = false;
        std::string editorId;
        std::string feedbackProfileName;

        const auto clearPending =
            [&]() {
                editorId.clear();
                feedbackProfileName.clear();
            };

        const auto commitPending =
            [&]() {
                if (!inCustomWeaponBlock) {
                    clearPending();
                    return;
                }

                // ControllerFeedbackProfile is optional so a block can be
                // speaker-only without counting as an invalid feedback entry.
                if (feedbackProfileName.empty()) {
                    clearPending();
                    return;
                }

                const auto normalizedEditorId =
                    normalizeIdentity(editorId);

                const auto* profile =
                    findBuiltInWeaponProfileExact(
                        feedbackProfileName);

                if (
                    normalizedEditorId.empty() ||
                    !profile) {
                    ++result.ignored;
                    clearPending();
                    return;
                }

                const auto existing =
                    std::find_if(
                        aliases.begin(),
                        aliases.end(),
                        [&](const CustomWeaponAlias& item) {
                            return
                                item.normalizedEditorId ==
                                normalizedEditorId;
                        });

                if (existing != aliases.end()) {
                    existing->profile = profile;
                } else {
                    aliases.push_back(
                        CustomWeaponAlias{
                            normalizedEditorId,
                            profile
                        });
                }

                clearPending();
            };

        while (!tomlText.empty()) {
            const auto newline =
                tomlText.find('\n');

            auto line =
                newline == std::string_view::npos
                    ? tomlText
                    : tomlText.substr(
                        0,
                        newline);

            tomlText =
                newline == std::string_view::npos
                    ? std::string_view{}
                    : tomlText.substr(
                        newline + 1);

            line =
                trim(
                    removeInlineComment(
                        line));

            if (line.empty()) {
                continue;
            }

            if (
                line.size() >= 4 &&
                line.starts_with("[[") &&
                line.ends_with("]]")) {

                commitPending();

                const auto section =
                    trim(
                        line.substr(
                            2,
                            line.size() - 4));

                inCustomWeaponBlock =
                    section == "CustomWeapons";

                continue;
            }

            if (
                line.size() >= 2 &&
                line.front() == '[' &&
                line.back() == ']') {

                commitPending();
                inCustomWeaponBlock = false;
                continue;
            }

            if (!inCustomWeaponBlock) {
                continue;
            }

            const auto equals =
                line.find('=');

            if (equals == std::string_view::npos) {
                continue;
            }

            const auto key =
                unquote(
                    line.substr(
                        0,
                        equals));

            const auto value =
                unquote(
                    line.substr(
                        equals + 1));

            if (key == "EditorID") {
                editorId =
                    std::string(value);
            } else if (
                key ==
                "ControllerFeedbackProfile") {

                feedbackProfileName =
                    std::string(value);
            }
        }

        commitPending();

        result.loaded =
            aliases.size();

        g_customWeaponAliases.store(
            std::make_shared<
                const CustomWeaponAliasList>(
                    std::move(aliases)),
            std::memory_order_release);
    } catch (...) {
        result.loaded = 0;

        g_customWeaponAliases.store(
            {},
            std::memory_order_release);
    }

    return result;
}

bool sds::customWeaponsEnabled() noexcept
{
    return
        g_customWeaponsEnabled.load(
            std::memory_order_acquire);
}

std::size_t
    sds::customWeaponProfileCount() noexcept
{
    const auto aliases =
        g_customWeaponAliases.load(
            std::memory_order_acquire);

    return aliases ?
        aliases->size() :
        0u;
}

bool sds::isCustomWeaponProfileMatch(
    std::string_view identity) noexcept
{
    return
        findCustomWeaponProfile(identity) !=
        nullptr;
}

const sds::WeaponProfile*
    sds::findWeaponProfile(
        std::string_view identity) noexcept
{
    if (
        const auto* custom =
            findCustomWeaponProfile(identity)) {
        return custom;
    }

    return
        findBuiltInWeaponProfile(identity);
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
