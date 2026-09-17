#include <StarfieldDualSense/ShipWeaponSemanticCatalog.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>
#include <unordered_set>

namespace
{
    std::vector<std::string> tokens(std::string_view value)
    {
        std::vector<std::string> out;
        std::string token;
        for (const auto raw : value) {
            const auto c = static_cast<unsigned char>(raw);
            if (std::isalnum(c) != 0) {
                token.push_back(static_cast<char>(std::tolower(c)));
            } else if (!token.empty()) {
                out.push_back(std::move(token));
                token.clear();
            }
        }
        if (!token.empty()) {
            out.push_back(std::move(token));
        }
        return out;
    }

    bool has(const std::vector<std::string>& values, std::string_view needle)
    {
        return std::find(values.begin(), values.end(), needle) != values.end();
    }

    sds::ShipWeaponFamily familyFromTokens(const std::vector<std::string>& values)
    {
        if (has(values, "ballistic")) return sds::ShipWeaponFamily::Ballistic;
        if (has(values, "laser")) return sds::ShipWeaponFamily::Laser;
        if (has(values, "particle")) return sds::ShipWeaponFamily::Particle;
        if (has(values, "missile")) return sds::ShipWeaponFamily::Missile;
        if (has(values, "em")) return sds::ShipWeaponFamily::EM;
        return sds::ShipWeaponFamily::Unknown;
    }

    std::optional<sds::ShipWeaponFamily> classifyPlayerShipFireMedia(std::string_view value)
    {
        const auto values = tokens(value);
        if (!has(values, "wpn") || !has(values, "ship") || !has(values, "fire") || !has(values, "pc")) {
            return std::nullopt;
        }
        const auto family = familyFromTokens(values);
        return family == sds::ShipWeaponFamily::Unknown ? std::nullopt : std::optional{ family };
    }
}

std::optional<sds::ShipWeaponSemanticEvent> sds::ShipWeaponSemanticCatalog::find(
    std::uint32_t eventId) const noexcept
{
    const auto it = std::lower_bound(events_.begin(), events_.end(), eventId,
        [](const auto& entry, std::uint32_t value) { return entry.eventId < value; });
    if (it == events_.end() || it->eventId != eventId) {
        return std::nullopt;
    }
    return *it;
}

sds::ShipWeaponSemanticCatalog sds::buildShipWeaponSemanticCatalog(
    const WwiseSoundBanksInfoIndex& index)
{
    ShipWeaponSemanticCatalog catalog{};
    for (const auto& [eventId, metadataList] : index.eventsById) {
        std::unordered_set<ShipWeaponFamily> families;
        for (const auto& metadata : metadataList) {
            for (const auto mediaId : metadata.referencedMediaIds) {
                const auto mediaIt = index.mediaById.find(mediaId);
                if (mediaIt == index.mediaById.end()) {
                    continue;
                }
                for (const auto source : { std::string_view(mediaIt->second.shortName),
                         std::string_view(mediaIt->second.originalPath) }) {
                    if (const auto family = classifyPlayerShipFireMedia(source)) {
                        families.insert(*family);
                    }
                }
            }
        }
        if (families.size() == 1u && *families.begin() == ShipWeaponFamily::Ballistic) {
            catalog.events_.push_back({ eventId, ShipWeaponFamily::Ballistic, ShipWeaponAction::Fire, true });
        }
    }
    std::sort(catalog.events_.begin(), catalog.events_.end(),
        [](const auto& left, const auto& right) { return left.eventId < right.eventId; });
    return catalog;
}

void sds::ShipWeaponSemanticCache::publish(ShipWeaponSemanticCatalog catalog) noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        catalog_ = std::move(catalog);
        ready_ = true;
    } catch (...) {
    }
}

void sds::ShipWeaponSemanticCache::clear() noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        catalog_ = {};
        ready_ = false;
    } catch (...) {
    }
}

bool sds::ShipWeaponSemanticCache::ready() const noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        return ready_;
    } catch (...) {
        return false;
    }
}

bool sds::ShipWeaponSemanticCache::isPlayerBallisticFire(std::uint32_t eventId) const noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        if (!ready_) {
            return false;
        }
        const auto entry = catalog_.find(eventId);
        return entry && entry->family == ShipWeaponFamily::Ballistic &&
            entry->action == ShipWeaponAction::Fire && entry->playerVariant;
    } catch (...) {
        return false;
    }
}

std::size_t sds::ShipWeaponSemanticCache::size() const noexcept
{
    try {
        std::scoped_lock lock(mutex_);
        return ready_ ? catalog_.size() : 0u;
    } catch (...) {
        return 0u;
    }
}
