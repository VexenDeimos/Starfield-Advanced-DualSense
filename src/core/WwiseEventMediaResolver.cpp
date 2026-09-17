#include <StarfieldDualSense/WwiseEventMediaResolver.h>
#include <StarfieldDualSense/WeaponSpeakerProfile.h>
#include <StarfieldDualSense/WwiseBa2ArchiveIndex.h>
#include <StarfieldDualSense/WwiseBankHircResolver.h>
#include <StarfieldDualSense/WwiseSoundBanksInfo.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace
{
    constexpr std::uint32_t kDiagnosticExteriorTailEventId = 0x0E00A9BBu;

    std::string lower(std::string value)
    {
        for (auto& c : value) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return value;
    }

    std::string normalizedMediaBasename(std::string_view value)
    {
        std::string normalized(value);
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        if (const auto slash = normalized.find_last_of('/'); slash != std::string::npos) {
            normalized.erase(0u, slash + 1u);
        }
        return lower(std::move(normalized));
    }

    std::vector<std::string> identityTokens(std::string_view value)
    {
        std::vector<std::string> tokens;
        std::string token;
        for (const auto raw : value) {
            const auto c = static_cast<unsigned char>(raw);
            if (std::isalnum(c) != 0) {
                token.push_back(static_cast<char>(std::tolower(c)));
                continue;
            }
            if (!token.empty()) {
                tokens.push_back(std::move(token));
                token.clear();
            }
        }
        if (!token.empty()) {
            tokens.push_back(std::move(token));
        }
        return tokens;
    }

    bool weaponIdentityMatchesMediaName(std::string_view weaponIdentity, std::string_view mediaName)
    {
        const auto weaponTokens = identityTokens(weaponIdentity);
        const auto mediaTokens = identityTokens(mediaName);
        if (weaponTokens.empty() || mediaTokens.size() < weaponTokens.size()) {
            return false;
        }
        return std::search(mediaTokens.begin(), mediaTokens.end(), weaponTokens.begin(), weaponTokens.end()) !=
            mediaTokens.end();
    }

    bool mediaNameMatches(std::string_view logicalName, std::string_view resolvedName)
    {
        return normalizedMediaBasename(logicalName) == normalizedMediaBasename(resolvedName);
    }

    bool variantMatchesResolvedMedia(
        const sds::WeaponSpeakerVariant& variant,
        std::uint32_t mediaId,
        std::string_view resolvedName)
    {
        return variant.pinnedMediaId != 0u
            ? variant.pinnedMediaId == mediaId
            : mediaNameMatches(variant.logicalName, resolvedName);
    }

    bool isStarfieldWwiseArchive(const std::filesystem::path& path)
    {
        const auto name = lower(path.filename().string());
        return name.rfind("starfield - wwisesounds", 0) == 0 && name.size() >= 4u &&
            name.substr(name.size() - 4u) == ".ba2";
    }

    bool isShatteredSpaceMainArchive(const std::filesystem::path& path)
    {
        const auto name = lower(path.filename().string());
        return name == "shatteredspace - main01.ba2" || name == "shatteredspace - main02.ba2";
    }

    bool archiveName(const std::filesystem::path& path)
    {
        return isStarfieldWwiseArchive(path) || isShatteredSpaceMainArchive(path);
    }

    std::uint8_t archivePriority(const std::filesystem::path& path)
    {
        if (isStarfieldWwiseArchive(path)) {
            return 0u;
        }
        const auto name = lower(path.filename().string());
        if (name == "shatteredspace - main01.ba2") {
            return 1u;
        }
        return 2u;
    }

    bool isPatchArchive(const std::filesystem::path& path)
    {
        return lower(path.filename().string()) == "starfield - wwisesoundspatch.ba2";
    }

    std::string_view shatteredSpaceCaptureAction(std::string_view weaponIdentity, std::uint32_t eventId)
    {
        const auto weapon = lower(std::string(weaponIdentity));
        if (weapon == "va'ruun penumbra") {
            switch (eventId) {
            case 0x09BBD2D5u: return "draw";
            case 0x40FB1CC4u: return "holster";
            case 0xE93349ACu: return "charge-start";
            case 0x5E9BD0B6u: return "charge-stop";
            case 0xF96C72FFu: return "fire";
            case 0x25F00FC7u:
            case 0xB08A20CCu:
            case 0x2E403A7Cu: return "reload";
            default: return {};
            }
        }
        if (weapon == "va'ruun starstorm") {
            switch (eventId) {
            case 0x0444A7EAu: return "draw";
            case 0xCCFD7993u: return "holster";
            case 0xF8229E3Cu: return "click";
            case 0xF785CDA0u: return "power-up";
            case 0xFB7756F7u: return "power-down";
            case 0xB6E1A82Eu: return "fire";
            case 0xA7514E2Du: return "fire-stop";
            case 0x1BFBE240u:
            case 0x00465690u:
            case 0xD957DC9Au:
            case 0xAFD57B91u: return "reload";
            default: return {};
            }
        }
        return {};
    }

    std::string capturePathComponent(std::string_view value)
    {
        std::string result;
        result.reserve(value.size());
        for (const auto raw : value) {
            const auto c = static_cast<unsigned char>(raw);
            const auto safe = std::isalnum(c) != 0 || raw == '-' || raw == '.';
            const auto out = safe ? static_cast<char>(raw) : '_';
            if (out == '_' && !result.empty() && result.back() == '_') {
                continue;
            }
            result.push_back(out);
        }
        while (!result.empty() && result.back() == '_') {
            result.pop_back();
        }
        return result.empty() ? "unknown" : result;
    }

    std::string captureTsvField(std::string_view value)
    {
        std::string result(value);
        for (auto& c : result) {
            if (c == '\t' || c == '\r' || c == '\n') {
                c = ' ';
            }
        }
        return result;
    }

    std::string captureEventHex(std::uint32_t value)
    {
        std::ostringstream out;
        out << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
        return out.str();
    }

    std::vector<unsigned char> readCaptureFile(const std::filesystem::path& path)
    {
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input) {
            return {};
        }
        const auto size = input.tellg();
        if (size < 0) {
            return {};
        }
        std::vector<unsigned char> bytes(static_cast<std::size_t>(size));
        input.seekg(0, std::ios::beg);
        if (!bytes.empty() && !input.read(reinterpret_cast<char*>(bytes.data()), size)) {
            return {};
        }
        return bytes;
    }

    sds::WwiseResolvedMediaWriteResult captureShatteredSpaceWem(
        const std::filesystem::path& dataPath,
        std::string_view weaponIdentity,
        std::uint32_t eventId,
        std::string_view eventName,
        const sds::WwiseWeaponDiscoveryMediaRecord& media,
        std::span<const unsigned char> payload)
    {
        sds::WwiseResolvedMediaWriteResult result{};
        const auto action = shatteredSpaceCaptureAction(weaponIdentity, eventId);
        if (action.empty() || !isShatteredSpaceMainArchive(media.archivePath)) {
            return result;
        }
        if (payload.empty() || payload.size() > 64u * 1024u * 1024u) {
            result.status = "rejected";
            result.error = payload.empty() ? "empty payload" : "payload exceeds maxBytes";
            return result;
        }

        const auto root = dataPath / "SFSE" / "Plugins" / "StarfieldDualSenseDiagnostics" /
            "v0.3.47" / "ShatteredSpaceWemCapture";
        const auto eventHex = captureEventHex(eventId);
        const auto directory = root / capturePathComponent(weaponIdentity) / std::string(action) / eventHex;
        std::error_code errorCode{};
        std::filesystem::create_directories(directory, errorCode);
        if (errorCode) {
            result.status = "error";
            result.error = errorCode.message();
            return result;
        }

        auto target = directory / (std::to_string(media.mediaId) + ".wem");
        if (std::filesystem::exists(target, errorCode) && !errorCode) {
            const auto existing = readCaptureFile(target);
            if (existing.size() == payload.size() && std::equal(existing.begin(), existing.end(), payload.begin())) {
                result.status = "exists-same";
            } else {
                bool selected = false;
                for (unsigned duplicate = 1u; duplicate < 10000u; ++duplicate) {
                    auto candidate = directory /
                        (std::to_string(media.mediaId) + "__dup" + std::to_string(duplicate) + ".wem");
                    if (!std::filesystem::exists(candidate, errorCode)) {
                        target = std::move(candidate);
                        result.status = "written-duplicate";
                        selected = true;
                        break;
                    }
                }
                if (!selected) {
                    result.status = "error";
                    result.error = "duplicate filename space exhausted";
                    return result;
                }
            }
        }

        if (result.status.empty() || result.status == "written-duplicate") {
            if (result.status.empty()) {
                result.status = "written";
            }
            std::ofstream output(target, std::ios::binary | std::ios::trunc);
            if (!output) {
                result.status = "error";
                result.error = "open output failed";
                return result;
            }
            output.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
            if (!output) {
                result.status = "error";
                result.error = "write failed";
                return result;
            }
        }
        result.path = target;

        const auto manifest = root / "manifest.tsv";
        const std::string header = "weapon\taction\tevent_id\tevent_name\tmedia_id\tchannels\tsample_rate\tcodec\tarchive\tfile\n";
        std::ostringstream row;
        row << captureTsvField(weaponIdentity) << '\t'
            << action << '\t'
            << eventHex << '\t'
            << captureTsvField(eventName) << '\t'
            << media.mediaId << '\t'
            << media.structure.channels << '\t'
            << media.structure.sampleRate << '\t'
            << captureTsvField(media.structure.codecLabel) << '\t'
            << captureTsvField(media.archivePath.filename().string()) << '\t'
            << captureTsvField(std::filesystem::relative(target, root, errorCode).generic_string()) << '\n';
        const auto rowText = row.str();

        std::string existingManifest;
        {
            std::ifstream input(manifest, std::ios::binary);
            if (input) {
                existingManifest.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
            }
        }
        if (existingManifest.find(rowText) == std::string::npos) {
            std::ofstream output(manifest, std::ios::binary | std::ios::app);
            if (!output) {
                result.status = "error";
                result.error = "manifest open failed";
                return result;
            }
            if (existingManifest.empty()) {
                output << header;
            }
            output << rowText;
            if (!output) {
                result.status = "error";
                result.error = "manifest write failed";
                return result;
            }
        }
        return result;
    }

    sds::WwiseResolvedMediaWriteResult captureUiCandidateWem(
        const std::filesystem::path& dataPath,
        std::string_view diagnosticVersion,
        std::string_view label,
        std::uint32_t eventId,
        std::string_view eventName,
        const sds::WwiseWeaponDiscoveryMediaRecord& media,
        std::span<const unsigned char> payload)
    {
        sds::WwiseResolvedMediaWriteResult result{};
        if (payload.empty() || payload.size() > 64u * 1024u * 1024u) {
            result.status = "rejected";
            result.error = payload.empty() ? "empty payload" : "payload exceeds maxBytes";
            return result;
        }

        const auto root = dataPath / "SFSE" / "Plugins" / "StarfieldDualSenseDiagnostics" /
            capturePathComponent(diagnosticVersion) / "UiWemCandidates";
        const auto eventHex = captureEventHex(eventId);
        const auto directory = root / capturePathComponent(label) / eventHex;
        std::error_code errorCode{};
        std::filesystem::create_directories(directory, errorCode);
        if (errorCode) {
            result.status = "error";
            result.error = errorCode.message();
            return result;
        }

        auto target = directory / (std::to_string(media.mediaId) + ".wem");
        if (std::filesystem::exists(target, errorCode) && !errorCode) {
            const auto existing = readCaptureFile(target);
            if (existing.size() == payload.size() && std::equal(existing.begin(), existing.end(), payload.begin())) {
                result.status = "exists-same";
            } else {
                bool selected = false;
                for (unsigned duplicate = 1u; duplicate < 10000u; ++duplicate) {
                    auto candidate = directory /
                        (std::to_string(media.mediaId) + "__dup" + std::to_string(duplicate) + ".wem");
                    if (!std::filesystem::exists(candidate, errorCode)) {
                        target = std::move(candidate);
                        result.status = "written-duplicate";
                        selected = true;
                        break;
                    }
                }
                if (!selected) {
                    result.status = "error";
                    result.error = "duplicate filename space exhausted";
                    return result;
                }
            }
        }

        if (result.status.empty() || result.status == "written-duplicate") {
            if (result.status.empty()) {
                result.status = "written";
            }
            std::ofstream output(target, std::ios::binary | std::ios::trunc);
            if (!output) {
                result.status = "error";
                result.error = "open output failed";
                return result;
            }
            output.write(reinterpret_cast<const char*>(payload.data()), static_cast<std::streamsize>(payload.size()));
            if (!output) {
                result.status = "error";
                result.error = "write failed";
                return result;
            }
        }
        result.path = target;

        const auto manifest = root / "manifest.tsv";
        const std::string header = "label\tevent_id\tevent_name\tmedia_id\tchannels\tsample_rate\tcodec\tarchive\tfile\n";
        std::ostringstream row;
        row << captureTsvField(label) << '\t'
            << eventHex << '\t'
            << captureTsvField(eventName) << '\t'
            << media.mediaId << '\t'
            << media.structure.channels << '\t'
            << media.structure.sampleRate << '\t'
            << captureTsvField(media.structure.codecLabel) << '\t'
            << captureTsvField(media.archivePath.filename().string()) << '\t'
            << captureTsvField(std::filesystem::relative(target, root, errorCode).generic_string()) << '\n';
        const auto rowText = row.str();

        std::string existingManifest;
        {
            std::ifstream input(manifest, std::ios::binary);
            if (input) {
                existingManifest.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
            }
        }
        if (existingManifest.find(rowText) == std::string::npos) {
            std::ofstream output(manifest, std::ios::binary | std::ios::app);
            if (!output) {
                result.status = "error";
                result.error = "manifest open failed";
                return result;
            }
            if (existingManifest.empty()) {
                output << header;
            }
            output << rowText;
            if (!output) {
                result.status = "error";
                result.error = "manifest write failed";
                return result;
            }
        }
        return result;
    }

    std::vector<std::uint32_t> buildResolverEvents()
    {
        std::vector<std::uint32_t> result;
        std::unordered_set<std::uint32_t> seen;
        bool diagnosticTailInserted = false;
        for (const auto& profile : sds::weaponSpeakerProfiles()) {
            for (const auto& cue : profile.cues) {
                if (cue.mediaEventId != 0u && seen.insert(cue.mediaEventId).second) {
                    result.push_back(cue.mediaEventId);
                }
                if (!diagnosticTailInserted && cue.trigger == sds::WeaponSpeakerTrigger::ConfirmedWeaponFire) {
                    if (seen.insert(kDiagnosticExteriorTailEventId).second) {
                        result.push_back(kDiagnosticExteriorTailEventId);
                    }
                    diagnosticTailInserted = true;
                }
            }
            if (profile.sustained) {
                for (const auto eventId : { profile.sustained->startWwiseEventId, profile.sustained->stopWwiseEventId }) {
                    if (eventId != 0u && seen.insert(eventId).second) {
                        result.push_back(eventId);
                    }
                }
            }
        }
        if (!diagnosticTailInserted && seen.insert(kDiagnosticExteriorTailEventId).second) {
            result.push_back(kDiagnosticExteriorTailEventId);
        }
        return result;
    }

    const std::vector<std::uint32_t>& resolverEvents()
    {
        static const auto events = buildResolverEvents();
        return events;
    }

    std::string semanticFor(std::uint32_t eventId)
    {
        if (eventId == kDiagnosticExteriorTailEventId) {
            return "fire";
        }
        for (const auto& profile : sds::weaponSpeakerProfiles()) {
            for (const auto& cue : profile.cues) {
                if (cue.mediaEventId != eventId) {
                    continue;
                }
                if (cue.action == "fire") {
                    return "fire";
                }
                if (cue.action == "draw") {
                    return "draw";
                }
                if (cue.action == "holster") {
                    return "holster";
                }
                return "reload";
            }
            if (profile.sustained &&
                (profile.sustained->startWwiseEventId == eventId || profile.sustained->stopWwiseEventId == eventId)) {
                return "fire";
            }
        }
        return "unknown";
    }

    struct VariantMatch
    {
        const sds::WeaponSpeakerProfile* profile{};
        const sds::WeaponSpeakerCue* finiteCue{};
        const sds::WeaponSpeakerVariant* variant{};
        std::string_view action{};
        std::uint32_t eventId{};
    };

    VariantMatch findVariantMatch(
        std::uint32_t eventId,
        std::uint32_t mediaId,
        std::string_view originalName)
    {
        for (const auto& profile : sds::weaponSpeakerProfiles()) {
            if (sds::speakerAudioFamily(profile) != profile.weaponIdentity) {
                continue;
            }
            const auto* canonical = sds::findWeaponSpeakerAudioFamilyProfile(profile.weaponIdentity);
            if (!canonical) {
                continue;
            }
            for (const auto& cue : canonical->cues) {
                if (cue.mediaEventId != eventId) {
                    continue;
                }
                for (const auto& variant : cue.variants) {
                    if (variantMatchesResolvedMedia(variant, mediaId, originalName)) {
                        return { canonical, &cue, &variant, cue.action, cue.mediaEventId };
                    }
                }
            }
            if (!canonical->sustained) {
                continue;
            }
            const auto& sustained = *canonical->sustained;
            if (sustained.startWwiseEventId == eventId) {
                for (const auto& variant : sustained.loopVariants) {
                    if (variantMatchesResolvedMedia(variant, mediaId, originalName)) {
                        return { canonical, nullptr, &variant, "sustained-loop", sustained.startWwiseEventId };
                    }
                }
                for (const auto& variant : sustained.startTransientVariants) {
                    if (variantMatchesResolvedMedia(variant, mediaId, originalName)) {
                        return { canonical, nullptr, &variant, "sustained-start", sustained.startWwiseEventId };
                    }
                }
            }
            if (sustained.stopWwiseEventId == eventId) {
                for (const auto& variant : sustained.stopTransientVariants) {
                    if (variantMatchesResolvedMedia(variant, mediaId, originalName)) {
                        return { canonical, nullptr, &variant, "sustained-stop", sustained.stopWwiseEventId };
                    }
                }
            }
        }
        return {};
    }

    void considerWeaponCandidate(
        sds::WwiseEventResolverRunResult& run,
        const VariantMatch& match,
        std::uint32_t mediaId,
        const std::string& originalName,
        const std::filesystem::path& archivePath,
        const std::string& archiveEntry,
        const std::vector<unsigned char>& payload)
    {
        if (!match.profile || !match.variant || match.action.empty() || match.eventId == 0u) {
            return;
        }

        const bool patch = isPatchArchive(archivePath);
        if (match.variant->archivePolicy == sds::WeaponSpeakerArchivePolicy::RequirePatch && !patch) {
            return;
        }

        auto existing = std::find_if(
            run.weaponVariants.begin(),
            run.weaponVariants.end(),
            [&](const auto& candidate) {
                return candidate.weaponIdentity == match.profile->weaponIdentity &&
                    candidate.action == match.action && candidate.variant == match.variant->variant;
            });

        if (existing != run.weaponVariants.end() && !(patch && !existing->patchPreferred)) {
            return;
        }

        sds::WwisePcmWeaponVariantCandidate candidate{};
        candidate.weaponIdentity = std::string(match.profile->weaponIdentity);
        candidate.action = std::string(match.action);
        candidate.eventId = match.eventId;
        candidate.variant = match.variant->variant;
        candidate.mediaId = mediaId;
        candidate.originalName = originalName;
        candidate.archivePath = archivePath;
        candidate.archiveEntry = archiveEntry;
        candidate.patchPreferred = patch;
        candidate.wemPayload = payload;

        if (existing == run.weaponVariants.end()) {
            run.weaponVariants.push_back(std::move(candidate));
        } else {
            *existing = std::move(candidate);
        }
    }

    std::size_t catalogActionOrder(std::string_view weapon, std::string_view action)
    {
        std::size_t order = 0u;
        for (const auto& profile : sds::weaponSpeakerProfiles()) {
            for (const auto& cue : profile.cues) {
                if (profile.weaponIdentity == weapon && cue.action == action) {
                    return order;
                }
                ++order;
            }
            if (profile.sustained) {
                for (const auto sustainedAction : { std::string_view("sustained-loop"),
                         std::string_view("sustained-start"), std::string_view("sustained-stop") }) {
                    if (profile.weaponIdentity == weapon && sustainedAction == action) {
                        return order;
                    }
                    ++order;
                }
            }
        }
        return static_cast<std::size_t>(-1);
    }

    const char* statusName(sds::WwiseEventResolutionStatus status)
    {
        switch (status) {
        case sds::WwiseEventResolutionStatus::Resolved:
            return "resolved";
        case sds::WwiseEventResolutionStatus::PartiallyResolved:
            return "partial";
        case sds::WwiseEventResolutionStatus::UnsupportedBoundary:
            return "unsupported-boundary";
        default:
            return "not-found";
        }
    }

    std::string hex8(std::uint32_t value)
    {
        std::ostringstream out;
        out << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
        return out.str();
    }

    struct Archive
    {
        std::filesystem::path path{};
        sds::WwiseBa2ArchiveIndex index{};
    };

    void mergeInfo(sds::WwiseSoundBanksInfoIndex& destination, const sds::WwiseSoundBanksInfoIndex& source)
    {
        for (const auto& [id, media] : source.mediaById) {
            auto& target = destination.mediaById[id];
            target.mediaId = id;
            if (target.shortName.empty()) {
                target.shortName = media.shortName;
            }
            if (target.originalPath.empty()) {
                target.originalPath = media.originalPath;
            }
        }
        for (const auto& [id, events] : source.eventsById) {
            auto& target = destination.eventsById[id];
            target.insert(target.end(), events.begin(), events.end());
        }
        destination.warnings.insert(destination.warnings.end(), source.warnings.begin(), source.warnings.end());
    }
}

struct sds::WwiseEventMediaResolver::PreparedCatalog
{
    std::vector<Archive> archives{};
    WwiseSoundBanksInfoIndex info{};
    std::vector<std::filesystem::path> indexedArchives{};
};

sds::WwiseEventMediaResolver::WwiseEventMediaResolver(std::filesystem::path dataPath) :
    dataPath_(std::move(dataPath))
{}

sds::WwiseEventMediaResolver::~WwiseEventMediaResolver() = default;
sds::WwiseEventMediaResolver::WwiseEventMediaResolver(WwiseEventMediaResolver&&) noexcept = default;
sds::WwiseEventMediaResolver& sds::WwiseEventMediaResolver::operator=(WwiseEventMediaResolver&&) noexcept = default;

sds::WwiseResolverPrepareResult sds::WwiseEventMediaResolver::prepare(bool debugLogging)
{
    WwiseResolverPrepareResult result{};
    if (!debugLogging) {
        return result;
    }
    result.attempted = true;

    if (catalog_) {
        result.ready = true;
        result.indexedArchives = catalog_->indexedArchives;
        return result;
    }

    std::error_code errorCode{};
    if (!std::filesystem::is_directory(dataPath_, errorCode)) {
        result.error = "Data directory unavailable";
        return result;
    }

    std::vector<std::filesystem::path> archivePaths;
    for (std::filesystem::directory_iterator it(dataPath_, errorCode), end; !errorCode && it != end; it.increment(errorCode)) {
        if (it->is_regular_file() && archiveName(it->path())) {
            archivePaths.push_back(it->path());
        }
    }
    std::sort(archivePaths.begin(), archivePaths.end(), [](const auto& left, const auto& right) {
        const auto leftPriority = archivePriority(left);
        const auto rightPriority = archivePriority(right);
        if (leftPriority != rightPriority) {
            return leftPriority < rightPriority;
        }
        return lower(left.filename().string()) < lower(right.filename().string());
    });

    auto preparedCatalog = std::make_unique<PreparedCatalog>();
    for (const auto& path : archivePaths) {
        std::string error;
        auto index = WwiseBa2ArchiveIndex::open(path, error);
        if (index) {
            preparedCatalog->indexedArchives.push_back(path);
            preparedCatalog->archives.push_back({ path, std::move(*index) });
        }
    }
    if (preparedCatalog->archives.empty()) {
        result.error = "no supported Wwise BA2 archives indexed";
        return result;
    }

    for (auto& archive : preparedCatalog->archives) {
        for (const auto& entry : archive.index.entries()) {
            if (entry.normalizedName.size() < 5u || entry.normalizedName.substr(entry.normalizedName.size() - 5u) != ".json") {
                continue;
            }
            const auto payload = archive.index.readPayload(entry, 64u * 1024u * 1024u);
            if (!payload.ok) {
                continue;
            }
            const auto parsed = parseWwiseSoundBanksInfo(
                std::string_view(reinterpret_cast<const char*>(payload.bytes.data()), payload.bytes.size()),
                entry.name);
            if (parsed.ok && (!parsed.index.mediaById.empty() || !parsed.index.eventsById.empty())) {
                mergeInfo(preparedCatalog->info, parsed.index);
            }
        }
    }

    result.ready = true;
    result.indexedArchives = preparedCatalog->indexedArchives;
    catalog_ = std::move(preparedCatalog);
    return result;
}

bool sds::WwiseEventMediaResolver::prepared() const noexcept
{
    return static_cast<bool>(catalog_);
}

sds::ShipWeaponSemanticCatalog sds::WwiseEventMediaResolver::shipWeaponSemanticCatalog() const
{
    if (!catalog_) {
        return {};
    }
    return buildShipWeaponSemanticCatalog(catalog_->info);
}

std::vector<std::uint32_t> sds::WwiseEventMediaResolver::musicSelectionReconTargetEvents() const
{
    std::vector<std::uint32_t> targets;
    if (!catalog_) {
        return targets;
    }

    for (const auto& [eventId, metadataRecords] : catalog_->info.eventsById) {
        std::unordered_set<std::uint32_t> uniqueMediaIds;
        for (const auto& metadata : metadataRecords) {
            if (metadata.bankName != "Starfield_MUS") {
                continue;
            }
            for (const auto mediaId : metadata.referencedMediaIds) {
                uniqueMediaIds.insert(mediaId);
            }
        }
        if (uniqueMediaIds.size() > 1u) {
            targets.push_back(eventId);
        }
    }

    std::sort(targets.begin(), targets.end());
    return targets;
}

sds::WwiseObservedEventResolutionRecord sds::WwiseEventMediaResolver::resolveSelectedMusicMedia(
    std::uint32_t eventId,
    std::uint32_t mediaId) const
{
    WwiseObservedEventResolutionRecord record{};
    record.action = "music-haptics";
    record.eventId = eventId;

    if (!catalog_) {
        record.error = "resolver catalog not prepared";
        return record;
    }
    if (eventId == 0u || mediaId == 0u) {
        record.error = "invalid selected media identity";
        return record;
    }

    const auto& info = catalog_->info;
    const auto eventIt = info.eventsById.find(eventId);
    if (eventIt == info.eventsById.end()) {
        record.error = "selected event not found in SoundBanksInfo";
        return record;
    }

    bool authorized = false;
    for (const auto& metadata : eventIt->second) {
        if (metadata.bankName != "Starfield_MUS") {
            continue;
        }
        const auto mediaIt = std::find(
            metadata.referencedMediaIds.begin(),
            metadata.referencedMediaIds.end(),
            mediaId);
        if (mediaIt == metadata.referencedMediaIds.end()) {
            continue;
        }
        record.eventName = metadata.eventName;
        record.bankName = metadata.bankName;
        record.metadataSource = "SoundBanksInfo";
        authorized = true;
        break;
    }
    if (!authorized) {
        record.error = "selected media is not referenced by Starfield_MUS event";
        return record;
    }

    WwiseWeaponDiscoveryMediaRecord mediaRecord{};
    mediaRecord.mediaId = mediaId;
    if (const auto mediaIt = info.mediaById.find(mediaId); mediaIt != info.mediaById.end()) {
        mediaRecord.originalName = mediaIt->second.shortName;
        mediaRecord.originalPath = mediaIt->second.originalPath;
    }

    const WwiseBa2Entry* chosen = nullptr;
    const Archive* owner = nullptr;
    for (const auto& archive : catalog_->archives) {
        const auto matches = archive.index.findFilename(std::to_string(mediaId) + ".wem");
        if (!matches.empty()) {
            chosen = matches.front();
            owner = &archive;
            break;
        }
    }
    if (!chosen || !owner) {
        record.error = "selected WEM " + std::to_string(mediaId) + " not found";
        return record;
    }

    const auto payload = owner->index.readPayload(*chosen, 64u * 1024u * 1024u);
    if (!payload.ok) {
        record.error = "selected WEM read failed: " + payload.error;
        return record;
    }

    mediaRecord.archivePath = owner->path;
    mediaRecord.archiveEntry = chosen->name;
    mediaRecord.packedSize = chosen->packedSize;
    mediaRecord.unpackedSize = chosen->unpackedSize;
    mediaRecord.structure = inspectWemStructure(payload.bytes);
    mediaRecord.wemPayload = payload.bytes;
    record.media.push_back(std::move(mediaRecord));
    record.found = true;
    return record;
}

sds::WwiseObservedEventResolutionRecord sds::WwiseEventMediaResolver::resolveObservedEvent(
    const WwiseObservedWeaponEvent& request) const
{
    return resolveObservedEventImpl(request, ObservedMediaCaptureMode::ShatteredSpaceDiagnostic, {});
}

sds::WwiseObservedEventResolutionRecord sds::WwiseEventMediaResolver::resolveObservedUiEvent(
    std::string_view label,
    std::uint32_t eventId,
    std::string_view diagnosticVersion) const
{
    const WwiseObservedWeaponEvent request{ "UI", label, eventId };
    return resolveObservedEventImpl(request, ObservedMediaCaptureMode::UiDiagnostic, diagnosticVersion);
}

sds::WwiseObservedEventResolutionRecord sds::WwiseEventMediaResolver::resolveObservedEventInMemory(
    std::string_view label,
    std::uint32_t eventId) const
{
    const WwiseObservedWeaponEvent request{ "", label, eventId };
    return resolveObservedEventImpl(request, ObservedMediaCaptureMode::None, {});
}

sds::WwiseObservedEventResolutionRecord sds::WwiseEventMediaResolver::resolveObservedUiEventInMemory(
    std::string_view label,
    std::uint32_t eventId) const
{
    return resolveObservedEventInMemory(label, eventId);
}

sds::WwiseObservedEventResolutionRecord sds::WwiseEventMediaResolver::resolveObservedEventImpl(
    const WwiseObservedWeaponEvent& request,
    ObservedMediaCaptureMode captureMode,
    std::string_view uiDiagnosticVersion) const
{
    WwiseObservedEventResolutionRecord record{};
    record.weaponIdentity = std::string(request.weaponIdentity);
    record.action = std::string(request.action);
    record.eventId = request.eventId;

    if (!catalog_) {
        record.error = "resolver catalog not prepared";
        return record;
    }

    const auto& info = catalog_->info;
    const auto& archives = catalog_->archives;
    std::vector<std::uint32_t> mediaIds;
    std::unordered_set<std::uint32_t> seen;
    if (const auto it = info.eventsById.find(request.eventId); it != info.eventsById.end()) {
        record.found = true;
        record.metadataSource = "SoundBanksInfo";
        for (const auto& metadata : it->second) {
            if (record.eventName.empty()) {
                record.eventName = metadata.eventName;
            }
            if (record.bankName.empty()) {
                record.bankName = metadata.bankName;
            }
            for (const auto mediaId : metadata.referencedMediaIds) {
                if (seen.insert(mediaId).second) {
                    mediaIds.push_back(mediaId);
                }
            }
        }
    }

    if (!record.found) {
        for (const auto& archive : archives) {
            bool stop = false;
            for (const auto& entry : archive.index.entries()) {
                if (entry.normalizedName.size() < 4u ||
                    entry.normalizedName.substr(entry.normalizedName.size() - 4u) != ".bnk") {
                    continue;
                }
                const auto bankPayload = archive.index.readPayload(entry, 128u * 1024u * 1024u);
                if (!bankPayload.ok) {
                    continue;
                }
                const auto resolved = resolveWwiseBankEventMedia(bankPayload.bytes, request.eventId);
                if (!resolved.eventFound) {
                    continue;
                }
                record.found = true;
                record.metadataSource = "HIRC:" + entry.name;
                record.bankName = entry.name;
                for (const auto mediaId : resolved.mediaIds) {
                    if (seen.insert(mediaId).second) {
                        mediaIds.push_back(mediaId);
                    }
                }
                stop = true;
                break;
            }
            if (stop) {
                break;
            }
        }
    }

    for (const auto mediaId : mediaIds) {
        WwiseWeaponDiscoveryMediaRecord mediaRecord{};
        mediaRecord.mediaId = mediaId;
        if (const auto mediaIt = info.mediaById.find(mediaId); mediaIt != info.mediaById.end()) {
            mediaRecord.originalName = mediaIt->second.shortName;
            mediaRecord.originalPath = mediaIt->second.originalPath;
        }

        const WwiseBa2Entry* chosen = nullptr;
        const Archive* owner = nullptr;
        for (const auto& archive : archives) {
            const auto matches = archive.index.findFilename(std::to_string(mediaId) + ".wem");
            if (!matches.empty()) {
                chosen = matches.front();
                owner = &archive;
                break;
            }
        }
        if (!chosen || !owner) {
            if (!record.error.empty()) {
                record.error += "; ";
            }
            record.error += "WEM " + std::to_string(mediaId) + " not found";
            continue;
        }

        const auto payload = owner->index.readPayload(*chosen, 64u * 1024u * 1024u);
        if (!payload.ok) {
            if (!record.error.empty()) {
                record.error += "; ";
            }
            record.error += "WEM read failed: " + payload.error;
            continue;
        }

        mediaRecord.archivePath = owner->path;
        mediaRecord.archiveEntry = chosen->name;
        mediaRecord.packedSize = chosen->packedSize;
        mediaRecord.unpackedSize = chosen->unpackedSize;
        mediaRecord.structure = inspectWemStructure(payload.bytes);
        mediaRecord.wemPayload = payload.bytes;
        switch (captureMode) {
        case ObservedMediaCaptureMode::UiDiagnostic:
            mediaRecord.capture = captureUiCandidateWem(
                dataPath_,
                uiDiagnosticVersion,
                record.action,
                record.eventId,
                record.eventName,
                mediaRecord,
                payload.bytes);
            break;
        case ObservedMediaCaptureMode::ShatteredSpaceDiagnostic:
            mediaRecord.capture = captureShatteredSpaceWem(
                dataPath_,
                record.weaponIdentity,
                record.eventId,
                record.eventName,
                mediaRecord,
                payload.bytes);
            break;
        case ObservedMediaCaptureMode::None:
            break;
        }
        record.media.push_back(std::move(mediaRecord));
    }

    std::sort(record.media.begin(), record.media.end(), [](const auto& left, const auto& right) {
        return left.mediaId < right.mediaId;
    });
    return record;
}

sds::WwiseEventResolverRunResult sds::WwiseEventMediaResolver::runBatch(
    bool debugLogging,
    std::span<const std::string_view> discoveryWeapons)
{
    return runBatch(debugLogging, discoveryWeapons, {});
}

sds::WwiseEventResolverRunResult sds::WwiseEventMediaResolver::runBatch(
    bool debugLogging,
    std::span<const std::string_view> discoveryWeapons,
    std::span<const WwiseObservedWeaponEvent> observedEvents)
{
    std::string joined;
    for (const auto weapon : discoveryWeapons) {
        if (weapon.empty()) {
            continue;
        }
        if (!joined.empty()) {
            joined.push_back(';');
        }
        joined.append(weapon);
    }
    return runImpl(debugLogging, joined, observedEvents);
}

std::span<const std::uint32_t> sds::weaponSpeakerResolverEventIds() noexcept
{
    const auto& events = resolverEvents();
    return { events.data(), events.size() };
}

sds::WwiseEventResolverRunResult sds::WwiseEventMediaResolver::run(bool debugLogging, std::string_view discoveryWeapon)
{
    return runImpl(debugLogging, discoveryWeapon, {});
}

sds::WwiseEventResolverRunResult sds::WwiseEventMediaResolver::runImpl(
    bool debugLogging,
    std::string_view discoveryWeapon,
    std::span<const WwiseObservedWeaponEvent> observedEvents)
{
    WwiseEventResolverRunResult run{};
    if (!debugLogging) {
        return run;
    }
    run.attempted = true;

    const auto preparation = prepare(debugLogging);
    run.indexedArchives = preparation.indexedArchives;
    if (!preparation.ready) {
        run.error = preparation.error;
        return run;
    }

    auto& archives = catalog_->archives;
    auto& info = catalog_->info;

    if (!discoveryWeapon.empty()) {
        std::vector<std::string> discoveryWeapons;
        std::size_t start = 0u;
        while (start <= discoveryWeapon.size()) {
            const auto separator = discoveryWeapon.find(';', start);
            const auto length = separator == std::string_view::npos
                ? discoveryWeapon.size() - start
                : separator - start;
            const auto token = discoveryWeapon.substr(start, length);
            if (!token.empty()) {
                discoveryWeapons.emplace_back(token);
            }
            if (separator == std::string_view::npos) {
                break;
            }
            start = separator + 1u;
        }

        for (const auto& weaponIdentity : discoveryWeapons) {
            for (const auto& [eventId, eventMetadata] : info.eventsById) {
                for (const auto& metadata : eventMetadata) {
                    std::vector<std::uint32_t> matchingMediaIds;
                    for (const auto mediaId : metadata.referencedMediaIds) {
                        const auto mediaIt = info.mediaById.find(mediaId);
                        if (mediaIt == info.mediaById.end()) {
                            continue;
                        }
                        if (weaponIdentityMatchesMediaName(weaponIdentity, mediaIt->second.shortName) ||
                            weaponIdentityMatchesMediaName(weaponIdentity, mediaIt->second.originalPath)) {
                            matchingMediaIds.push_back(mediaId);
                        }
                    }
                    if (matchingMediaIds.empty()) {
                        continue;
                    }

                    auto eventIt = std::find_if(
                        run.weaponDiscoveryEvents.begin(),
                        run.weaponDiscoveryEvents.end(),
                        [eventId, &weaponIdentity](const auto& record) {
                            return record.eventId == eventId && record.weaponIdentity == weaponIdentity;
                        });
                    if (eventIt == run.weaponDiscoveryEvents.end()) {
                        WwiseWeaponDiscoveryEventRecord record{};
                        record.weaponIdentity = weaponIdentity;
                        record.eventId = eventId;
                        record.eventName = metadata.eventName;
                        record.bankName = metadata.bankName;
                        run.weaponDiscoveryEvents.push_back(std::move(record));
                        eventIt = std::prev(run.weaponDiscoveryEvents.end());
                    }

                    for (const auto mediaId : matchingMediaIds) {
                        const auto duplicate = std::any_of(
                            eventIt->media.begin(),
                            eventIt->media.end(),
                            [mediaId](const auto& media) { return media.mediaId == mediaId; });
                        if (duplicate) {
                            continue;
                        }

                        WwiseWeaponDiscoveryMediaRecord mediaRecord{};
                        mediaRecord.mediaId = mediaId;
                        if (const auto mediaIt = info.mediaById.find(mediaId); mediaIt != info.mediaById.end()) {
                            mediaRecord.originalName = mediaIt->second.shortName;
                            mediaRecord.originalPath = mediaIt->second.originalPath;
                        }

                        const WwiseBa2Entry* chosen = nullptr;
                        Archive* owner = nullptr;
                        for (auto& archive : archives) {
                            const auto matches = archive.index.findFilename(std::to_string(mediaId) + ".wem");
                            if (!matches.empty()) {
                                chosen = matches.front();
                                owner = &archive;
                                break;
                            }
                        }
                        if (chosen && owner) {
                            mediaRecord.archivePath = owner->path;
                            mediaRecord.archiveEntry = chosen->name;
                            mediaRecord.packedSize = chosen->packedSize;
                            mediaRecord.unpackedSize = chosen->unpackedSize;
                            const auto payload = owner->index.readPayload(*chosen, 64u * 1024u * 1024u);
                            if (payload.ok) {
                                mediaRecord.structure = inspectWemStructure(payload.bytes);
                            }
                        }
                        eventIt->media.push_back(std::move(mediaRecord));
                    }
                }
            }
        }

        for (auto& event : run.weaponDiscoveryEvents) {
            std::sort(event.media.begin(), event.media.end(), [](const auto& left, const auto& right) {
                return left.mediaId < right.mediaId;
            });
        }
        std::sort(run.weaponDiscoveryEvents.begin(), run.weaponDiscoveryEvents.end(), [](const auto& left, const auto& right) {
            const auto leftWeapon = lower(left.weaponIdentity);
            const auto rightWeapon = lower(right.weaponIdentity);
            if (leftWeapon != rightWeapon) {
                return leftWeapon < rightWeapon;
            }
            return left.eventId < right.eventId;
        });
    }

    for (const auto& request : observedEvents) {
        run.observedEventResolutions.push_back(resolveObservedEvent(request));
    }

    const auto diagnosticRoot = dataPath_ / "SFSE" / "Plugins" / "StarfieldDualSenseDiagnostics" /
        "v0.3.21" / "MaelstromWwise";

    for (const auto eventId : resolverEvents()) {
        WwiseEventResolutionRecord eventRecord{};
        eventRecord.eventId = eventId;
        eventRecord.semantic = semanticFor(eventId);
        std::vector<std::uint32_t> mediaIds;
        std::unordered_set<std::uint32_t> seen;
        std::string bank;

        if (const auto it = info.eventsById.find(eventId); it != info.eventsById.end()) {
            eventRecord.metadataSource = "SoundBanksInfo";
            for (const auto& event : it->second) {
                if (bank.empty()) {
                    bank = event.bankName;
                }
                for (const auto mediaId : event.referencedMediaIds) {
                    if (seen.insert(mediaId).second) {
                        mediaIds.push_back(mediaId);
                    }
                }
            }
        }

        if (mediaIds.empty()) {
            bool found = false;
            bool unsupported = false;
            for (auto& archive : archives) {
                for (const auto& entry : archive.index.entries()) {
                    if (entry.normalizedName.size() < 4u || entry.normalizedName.substr(entry.normalizedName.size() - 4u) != ".bnk") {
                        continue;
                    }
                    auto bankPayload = archive.index.readPayload(entry, 128u * 1024u * 1024u);
                    if (!bankPayload.ok) {
                        continue;
                    }
                    const auto resolved = resolveWwiseBankEventMedia(bankPayload.bytes, eventId);
                    if (!resolved.eventFound) {
                        continue;
                    }
                    found = true;
                    eventRecord.metadataSource = "HIRC:" + entry.name;
                    bank = entry.name;
                    eventRecord.actionCount = resolved.actionIds.size();
                    eventRecord.traversedObjectCount = resolved.traversed.size();
                    eventRecord.unsupportedObjectCount = resolved.unsupported.size();
                    unsupported = !resolved.unsupported.empty();
                    for (const auto mediaId : resolved.mediaIds) {
                        if (seen.insert(mediaId).second) {
                            mediaIds.push_back(mediaId);
                        }
                    }
                    break;
                }
                if (found) {
                    break;
                }
            }
            if (found && mediaIds.empty() && unsupported) {
                eventRecord.status = WwiseEventResolutionStatus::UnsupportedBoundary;
            }
        }

        for (const auto mediaId : mediaIds) {
            WwiseResolvedMediaRecord mediaRecord{};
            mediaRecord.eventId = eventId;
            mediaRecord.semantic = eventRecord.semantic;
            mediaRecord.mediaId = mediaId;
            mediaRecord.bankName = bank;
            if (const auto mediaIt = info.mediaById.find(mediaId); mediaIt != info.mediaById.end()) {
                mediaRecord.originalName = mediaIt->second.shortName;
                mediaRecord.originalPath = mediaIt->second.originalPath;
            }
            if (mediaRecord.originalName.empty()) {
                mediaRecord.originalName = std::to_string(mediaId) + ".wem";
            }

            const WwiseBa2Entry* chosen = nullptr;
            Archive* owner = nullptr;
            for (auto& archive : archives) {
                const auto matches = archive.index.findFilename(std::to_string(mediaId) + ".wem");
                if (!matches.empty()) {
                    chosen = matches.front();
                    owner = &archive;
                    break;
                }
            }
            if (!chosen || !owner) {
                if (!eventRecord.error.empty()) {
                    eventRecord.error += "; ";
                }
                eventRecord.error += "WEM " + std::to_string(mediaId) + " not found";
                continue;
            }

            auto payload = owner->index.readPayload(*chosen, 64u * 1024u * 1024u);
            if (!payload.ok) {
                if (!eventRecord.error.empty()) {
                    eventRecord.error += "; ";
                }
                eventRecord.error += "WEM read failed: " + payload.error;
                continue;
            }

            mediaRecord.archivePath = owner->path;
            mediaRecord.archiveEntry = chosen->name;
            mediaRecord.packedSize = chosen->packedSize;
            mediaRecord.unpackedSize = chosen->unpackedSize;
            mediaRecord.structure = inspectWemStructure(payload.bytes);
            mediaRecord.extraction = writeResolvedWemDiagnostic(
                diagnosticRoot,
                eventRecord.semantic,
                eventId,
                mediaId,
                mediaRecord.originalName,
                payload.bytes);

            considerWeaponCandidate(
                run,
                findVariantMatch(eventId, mediaId, mediaRecord.originalName),
                mediaId,
                mediaRecord.originalName,
                owner->path,
                chosen->name,
                payload.bytes);

            eventRecord.media.push_back(std::move(mediaRecord));
        }

        if (!eventRecord.media.empty()) {
            const bool extracted = std::any_of(eventRecord.media.begin(), eventRecord.media.end(), [](const auto& media) {
                return media.extraction.status == "written" || media.extraction.status == "written-duplicate" ||
                    media.extraction.status == "exists-same";
            });
            eventRecord.status = extracted ? WwiseEventResolutionStatus::Resolved : WwiseEventResolutionStatus::PartiallyResolved;
        } else if (!mediaIds.empty() && eventRecord.status != WwiseEventResolutionStatus::UnsupportedBoundary) {
            eventRecord.status = WwiseEventResolutionStatus::PartiallyResolved;
        } else if (eventRecord.status != WwiseEventResolutionStatus::UnsupportedBoundary && !eventRecord.metadataSource.empty()) {
            eventRecord.status = WwiseEventResolutionStatus::PartiallyResolved;
        }

        run.events.push_back(std::move(eventRecord));
    }

    std::sort(run.weaponVariants.begin(), run.weaponVariants.end(), [](const auto& left, const auto& right) {
        if (left.weaponIdentity != right.weaponIdentity) {
            return left.weaponIdentity < right.weaponIdentity;
        }
        const auto leftOrder = catalogActionOrder(left.weaponIdentity, left.action);
        const auto rightOrder = catalogActionOrder(right.weaponIdentity, right.action);
        if (leftOrder != rightOrder) {
            return leftOrder < rightOrder;
        }
        return left.variant < right.variant;
    });
    return run;
}

std::string sds::formatWwiseResolverRunHeader(const WwiseEventResolverRunResult& result)
{
    std::ostringstream out;
    out << "Wwise event resolver: ACTIVE diagnostic-only events=" << resolverEvents().size()
        << " playback=no repost=no stopOriginal=no archiveWrites=no extractResolvedWem=yes attempted="
        << (result.attempted ? "yes" : "no") << " archives=" << result.indexedArchives.size();
    if (!result.error.empty()) {
        out << " error=\"" << result.error << "\"";
    }
    return out.str();
}

std::string sds::formatWwiseResolverEvent(const WwiseEventResolutionRecord& result)
{
    std::size_t extracted = 0u;
    for (const auto& media : result.media) {
        if (media.extraction.status == "written" || media.extraction.status == "written-duplicate" ||
            media.extraction.status == "exists-same") {
            ++extracted;
        }
    }
    std::ostringstream out;
    out << "Wwise event resolver: event=" << hex8(result.eventId)
        << " semantic=" << result.semantic
        << " status=" << statusName(result.status)
        << " source=\"" << result.metadataSource << "\""
        << " actions=" << result.actionCount
        << " traversed=" << result.traversedObjectCount
        << " media=" << result.media.size()
        << " extracted=" << extracted
        << " unsupported=" << result.unsupportedObjectCount;
    if (!result.error.empty()) {
        out << " error=\"" << result.error << "\"";
    }
    return out.str();
}

std::string sds::formatWwiseResolverMedia(const WwiseResolvedMediaRecord& result)
{
    std::ostringstream out;
    out << "Wwise event resolver media: event=" << hex8(result.eventId)
        << " semantic=" << result.semantic
        << " mediaId=" << result.mediaId
        << " name=\"" << result.originalName << "\""
        << " path=\"" << result.originalPath << "\""
        << " bank=\"" << result.bankName << "\""
        << " ba2=\"" << result.archivePath.filename().string() << "\""
        << " entry=\"" << result.archiveEntry << "\""
        << " packed=" << result.packedSize
        << " unpacked=" << result.unpackedSize
        << " codec=" << result.structure.codecLabel
        << " formatTag=" << hex8(result.structure.formatTag)
        << " channels=" << result.structure.channels
        << " rate=" << result.structure.sampleRate
        << " extraction=" << result.extraction.status
        << " output=\"" << result.extraction.path.string() << "\"";
    if (!result.extraction.error.empty()) {
        out << " extractionError=\"" << result.extraction.error << "\"";
    }
    return out.str();
}

std::string sds::formatWwiseWeaponDiscoveryHeader(
    const WwiseEventResolverRunResult& result,
    std::string_view weaponIdentity)
{
    std::size_t eventCount = 0u;
    std::size_t mediaCount = 0u;
    const auto requested = lower(std::string(weaponIdentity));
    for (const auto& event : result.weaponDiscoveryEvents) {
        if (lower(event.weaponIdentity) != requested) {
            continue;
        }
        ++eventCount;
        mediaCount += event.media.size();
    }
    std::ostringstream out;
    out << "Wwise weapon media discovery: weapon=" << weaponIdentity
        << " events=" << eventCount
        << " media=" << mediaCount
        << " source=SoundBanksInfo playback=no repost=no stopOriginal=no extraction=no";
    return out.str();
}

std::string sds::formatWwiseWeaponDiscoveryEvent(const WwiseWeaponDiscoveryEventRecord& result)
{
    std::ostringstream out;
    out << "Wwise weapon media discovery event: weapon=" << result.weaponIdentity
        << " event=" << hex8(result.eventId)
        << " name=\"" << result.eventName << "\""
        << " bank=\"" << result.bankName << "\""
        << " media=" << result.media.size()
        << " playback=no";
    return out.str();
}

std::string sds::formatWwiseWeaponDiscoveryMedia(
    const WwiseWeaponDiscoveryEventRecord& event,
    const WwiseWeaponDiscoveryMediaRecord& media)
{
    std::ostringstream out;
    out << "Wwise weapon media discovery media: weapon=" << event.weaponIdentity
        << " event=" << hex8(event.eventId)
        << " mediaId=" << media.mediaId
        << " name=\"" << media.originalName << "\""
        << " path=\"" << media.originalPath << "\""
        << " ba2=\"" << media.archivePath.filename().string() << "\""
        << " entry=\"" << media.archiveEntry << "\""
        << " packed=" << media.packedSize
        << " unpacked=" << media.unpackedSize
        << " codec=" << media.structure.codecLabel
        << " formatTag=" << hex8(media.structure.formatTag)
        << " channels=" << media.structure.channels
        << " rate=" << media.structure.sampleRate
        << " extraction=no playback=no";
    return out.str();
}


std::string sds::formatWwiseObservedEventResolution(const WwiseObservedEventResolutionRecord& result)
{
    const char* status = !result.found ? "not-found" : result.media.empty() ? "found-no-media" : "resolved";
    std::ostringstream out;
    out << "Wwise observed-event resolution: weapon=" << result.weaponIdentity
        << " action=" << result.action
        << " event=" << hex8(result.eventId)
        << " status=" << status
        << " source=\"" << result.metadataSource << "\""
        << " name=\"" << result.eventName << "\""
        << " bank=\"" << result.bankName << "\""
        << " media=" << result.media.size()
        << " playback=no repost=no stopOriginal=no extraction=no";
    if (!result.error.empty()) {
        out << " error=\"" << result.error << "\"";
    }
    return out.str();
}

std::string sds::formatWwiseObservedEventResolutionSummary(const WwiseEventResolverRunResult& result)
{
    std::size_t resolved = 0u;
    std::size_t notFound = 0u;
    std::size_t media = 0u;
    for (const auto& event : result.observedEventResolutions) {
        if (event.found) {
            ++resolved;
        } else {
            ++notFound;
        }
        media += event.media.size();
    }
    std::ostringstream out;
    out << "Wwise observed-event resolution summary: targets=" << result.observedEventResolutions.size()
        << " resolved=" << resolved
        << " notFound=" << notFound
        << " media=" << media
        << " source=exact-live-event-ids playback=no repost=no stopOriginal=no extraction=no";
    return out.str();
}

std::string sds::formatWwiseObservedEventMedia(
    const WwiseObservedEventResolutionRecord& event,
    const WwiseWeaponDiscoveryMediaRecord& media)
{
    std::ostringstream out;
    out << "Wwise observed-event media: weapon=" << event.weaponIdentity
        << " action=" << event.action
        << " event=" << hex8(event.eventId)
        << " mediaId=" << media.mediaId
        << " name=\"" << media.originalName << "\""
        << " path=\"" << media.originalPath << "\""
        << " ba2=\"" << media.archivePath.filename().string() << "\""
        << " entry=\"" << media.archiveEntry << "\""
        << " packed=" << media.packedSize
        << " unpacked=" << media.unpackedSize
        << " codec=" << media.structure.codecLabel
        << " formatTag=" << hex8(media.structure.formatTag)
        << " channels=" << media.structure.channels
        << " rate=" << media.structure.sampleRate
        << " extraction=no playback=no";
    return out.str();
}
