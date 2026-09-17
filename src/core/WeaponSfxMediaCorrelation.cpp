#include <StarfieldDualSense/WeaponSfxMediaCorrelation.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace
{
    template <std::size_t N>
    std::string boundedString(const std::array<char, N>& value)
    {
        const auto end = std::find(value.begin(), value.end(), '\0');
        return std::string(value.begin(), end);
    }

    std::string lowerCopy(std::string_view value)
    {
        std::string out(value);
        std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return out;
    }

    bool containsAny(std::string_view value, std::initializer_list<std::string_view> needles)
    {
        return std::any_of(needles.begin(), needles.end(), [value](std::string_view needle) {
            return value.find(needle) != std::string_view::npos;
        });
    }

    std::string actionName(const sds::WeaponSfxDiscoveryReport& report)
    {
        switch (report.action) {
        case sds::WeaponSfxAction::WeaponFired:
            return "fire";
        case sds::WeaponSfxAction::ReloadCompleted:
            return "reload";
        case sds::WeaponSfxAction::WeaponEquipped:
            return "equip";
        case sds::WeaponSfxAction::DrawHolsterMarker:
            break;
        }

        const auto marker = lowerCopy(boundedString(report.marker));
        if (containsAny(marker, { "draw", "weaponout" })) {
            return "draw";
        }
        if (containsAny(marker, { "sheath", "holster", "stow", "weaponin", "unequip", "putaway" })) {
            return "holster";
        }
        return "marker";
    }

    struct Classification
    {
        sds::WeaponSfxMediaClass value{ sds::WeaponSfxMediaClass::Unknown };
        bool excluded{ false };
    };

    Classification classify(
        std::string_view eventName,
        const sds::WwiseWeaponDiscoveryMediaRecord& media)
    {
        auto searchable = lowerCopy(eventName);
        searchable.push_back(' ');
        searchable += lowerCopy(media.originalName);
        searchable.push_back(' ');
        searchable += lowerCopy(media.originalPath);

        if (containsAny(searchable, { "npc", "thirdperson" })) {
            return { sds::WeaponSfxMediaClass::Npc, true };
        }
        if (containsAny(searchable, { "reverb", "tail" })) {
            return { sds::WeaponSfxMediaClass::ReverbTail, true };
        }
        if (containsAny(searchable, { "wwisemotion", "_motion" })) {
            return { sds::WeaponSfxMediaClass::Motion, true };
        }
        if (containsAny(searchable, { "lowammo", "low_ammo" })) {
            return { sds::WeaponSfxMediaClass::LowAmmo, true };
        }

        const bool playerCore = containsAny(searchable, { "_pc_", "\\pc\\", "player", "firstperson" });
        if (containsAny(searchable, { "trigger", "mechanical", "shared" }) && !playerCore) {
            return { sds::WeaponSfxMediaClass::HelperShared, true };
        }
        if (playerCore) {
            return { sds::WeaponSfxMediaClass::PlayerCore, false };
        }
        return { sds::WeaponSfxMediaClass::Unknown, false };
    }

    std::string hex8(std::uint32_t value)
    {
        std::ostringstream out;
        out << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << value;
        return out.str();
    }

    std::string hexObject(std::uint64_t value)
    {
        std::ostringstream out;
        out << "0x" << std::uppercase << std::hex << value;
        return out.str();
    }

    const char* yesNo(bool value) noexcept
    {
        return value ? "yes" : "no";
    }

    const char* className(sds::WeaponSfxMediaClass value) noexcept
    {
        switch (value) {
        case sds::WeaponSfxMediaClass::PlayerCore: return "player-core";
        case sds::WeaponSfxMediaClass::Npc: return "npc";
        case sds::WeaponSfxMediaClass::ReverbTail: return "reverb-tail";
        case sds::WeaponSfxMediaClass::Motion: return "motion";
        case sds::WeaponSfxMediaClass::LowAmmo: return "low-ammo";
        case sds::WeaponSfxMediaClass::HelperShared: return "helper-shared";
        case sds::WeaponSfxMediaClass::Unknown: return "unknown";
        }
        return "unknown";
    }

    bool patchArchive(const std::filesystem::path& archivePath)
    {
        return lowerCopy(archivePath.filename().string()) == "starfield - wwisesoundspatch.ba2";
    }
}

sds::WeaponSfxMediaCorrelation::WeaponSfxMediaCorrelation(const WwiseEventMediaResolver& resolver) :
    resolver_(&resolver)
{}

std::vector<sds::WeaponSfxResolvedEvent> sds::WeaponSfxMediaCorrelation::correlate(
    const WeaponSfxDiscoveryReport& report)
{
    std::vector<WeaponSfxResolvedEvent> out;
    if (resolver_ == nullptr || !resolver_->prepared()) {
        return out;
    }

    const auto weaponIdentity = boundedString(report.weapon);
    const auto action = actionName(report);

    for (const auto& summary : report.eventSummaries) {
        const auto key = weaponIdentity + "|" + action + "|" + hex8(summary.eventId);
        if (emitted_.contains(key)) {
            continue;
        }
        emitted_.insert(key);

        const auto resolution = resolver_->resolveObservedEvent({
            .weaponIdentity = weaponIdentity,
            .action = action,
            .eventId = summary.eventId,
        });

        WeaponSfxResolvedEvent record{};
        record.anchorSequence = report.anchorSequence;
        record.weaponFormId = report.weaponFormId;
        record.weaponIdentity = weaponIdentity;
        record.action = action;
        record.eventId = summary.eventId;
        record.gameObjectId = summary.gameObjectId;
        record.gameObjectConsistent = summary.gameObjectConsistent;
        record.closestDeltaUs = summary.closestDeltaUs;
        record.playerObject = summary.gameObjectId == 0x2u;
        record.eventName = resolution.eventName;
        record.bankName = resolution.bankName;
        record.metadataSource = resolution.metadataSource;
        record.found = resolution.found;
        record.media.reserve(resolution.media.size());
        for (const auto& resolvedMedia : resolution.media) {
            const auto classification = classify(record.eventName, resolvedMedia);
            record.media.push_back({
                .media = resolvedMedia,
                .classification = classification.value,
                .excluded = classification.excluded,
            });
        }
        out.push_back(std::move(record));
    }
    return out;
}

void sds::WeaponSfxMediaCorrelation::clear() noexcept
{
    emitted_.clear();
}

std::string sds::formatWeaponSfxResolvedEvent(const WeaponSfxResolvedEvent& record)
{
    std::ostringstream out;
    out << "Weapon SFX resolved event: weapon='" << record.weaponIdentity << "'"
        << " form=" << hex8(record.weaponFormId)
        << " action=" << record.action
        << " anchorSeq=" << record.anchorSequence
        << " event=" << hex8(record.eventId)
        << " gameObject=" << hexObject(record.gameObjectId)
        << " playerObject=" << yesNo(record.playerObject)
        << " gameObjectConsistent=" << yesNo(record.gameObjectConsistent)
        << " closestDeltaUs=" << record.closestDeltaUs
        << " status=" << (record.found ? "resolved" : "not-found")
        << " source=\"" << record.metadataSource << "\""
        << " name=\"" << record.eventName << "\""
        << " bank=\"" << record.bankName << "\""
        << " media=" << record.media.size();
    return out.str();
}

std::string sds::formatWeaponSfxResolvedMedia(
    const WeaponSfxResolvedEvent& event,
    const WeaponSfxResolvedMedia& media)
{
    const auto& item = media.media;
    std::ostringstream out;
    out << "Weapon SFX resolved media: weapon='" << event.weaponIdentity << "'"
        << " action=" << event.action
        << " event=" << hex8(event.eventId)
        << " mediaId=" << item.mediaId
        << " name=\"" << item.originalName << "\""
        << " path=\"" << item.originalPath << "\""
        << " archive=\"" << item.archivePath.filename().string() << "\""
        << " patch=" << yesNo(patchArchive(item.archivePath))
        << " codec=" << item.structure.codecLabel
        << " channels=" << item.structure.channels
        << " rate=" << item.structure.sampleRate
        << " class=" << className(media.classification)
        << " excluded=" << yesNo(media.excluded);
    if (!item.capture.status.empty()) {
        out << " capture=" << item.capture.status
            << " captureFile=\"" << item.capture.path.string() << "\"";
        if (!item.capture.error.empty()) {
            out << " captureError=\"" << item.capture.error << "\"";
        }
    }
    return out.str();
}
