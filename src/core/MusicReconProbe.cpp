#include <StarfieldDualSense/MusicReconProbe.h>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <tuple>
#include <utility>

namespace sds
{
    namespace
    {
        [[nodiscard]] bool containsMusicToken(std::string_view text) noexcept
        {
            constexpr std::string_view tokens[]{ "music", "score", "mus_", "_mus" };
            for (const auto token : tokens) {
                if (text.size() < token.size()) {
                    continue;
                }
                for (std::size_t offset = 0; offset + token.size() <= text.size(); ++offset) {
                    bool match = true;
                    for (std::size_t i = 0; i < token.size(); ++i) {
                        const auto lhs = static_cast<unsigned char>(text[offset + i]);
                        const auto rhs = static_cast<unsigned char>(token[i]);
                        if (std::tolower(lhs) != std::tolower(rhs)) {
                            match = false;
                            break;
                        }
                    }
                    if (match) {
                        return true;
                    }
                }
            }
            return false;
        }

        [[nodiscard]] std::string quoteText(std::string_view text)
        {
            std::string result;
            result.reserve(text.size() + 2u);
            result.push_back('"');
            for (const char ch : text) {
                if (ch == '"' || ch == '\\') {
                    result.push_back('\\');
                }
                result.push_back(ch);
            }
            result.push_back('"');
            return result;
        }

        [[nodiscard]] std::string hexValue(std::uint64_t value)
        {
            std::ostringstream stream;
            stream << "0x" << std::uppercase << std::hex << value;
            return stream.str();
        }

        [[nodiscard]] std::string eventText(const GameEvent& event)
        {
            const auto end = std::find(event.text.begin(), event.text.end(), '\0');
            return std::string(event.text.begin(), end);
        }
    }

    std::size_t MusicReconProbe::AggregateKeyHash::operator()(const AggregateKey& key) const noexcept
    {
        std::size_t seed = std::hash<std::uint32_t>{}(key.eventId);
        seed ^= std::hash<std::uint64_t>{}(key.gameObjectId) + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
        seed ^= std::hash<std::uintptr_t>{}(key.callsiteRva) + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
        return seed;
    }

    void MusicReconProbe::pushDiagnostic(std::string line)
    {
        if (_diagnostics.size() >= kMaxDiagnostics) {
            ++_diagnosticOverflow;
            return;
        }
        _diagnostics.push_back(std::move(line));
    }

    void MusicReconProbe::observeGameEvent(const GameEvent& event) noexcept
    {
        if (_finalized) {
            return;
        }

        try {
            std::string type;
            switch (event.type) {
            case GameEventType::MenuOpened:
                type = "open";
                break;
            case GameEventType::MenuClosed:
                type = "close";
                break;
            case GameEventType::Shutdown:
                type = "shutdown";
                break;
            default:
                return;
            }

            if (_menuTransitions >= kMaxMenuTransitions) {
                ++_menuOverflow;
                return;
            }
            ++_menuTransitions;

            std::string line = "Music recon: MENU type=" + type;
            if (event.type != GameEventType::Shutdown) {
                line += " name=" + eventText(event);
            }
            pushDiagnostic(std::move(line));
        } catch (...) {
            ++_diagnosticOverflow;
        }
    }

    bool MusicReconProbe::observeWwise(const MusicReconWwiseObservation& observation) noexcept
    {
        if (_finalized || observation.eventId == 0u || observation.hasExternalSources || observation.externalCount != 0u) {
            return false;
        }

        try {
            const AggregateKey key{ observation.eventId, observation.gameObjectId, observation.callsiteRva };
            const auto existing = _aggregates.find(key);
            if (existing != _aggregates.end()) {
                ++existing->second.count;
                existing->second.last = observation;
            } else if (_aggregates.size() < kMaxAggregates) {
                _aggregates.emplace(key, Aggregate{ 1u, observation, observation });
            } else {
                ++_aggregateOverflow;
            }

            if (!_requestedEvents.contains(observation.eventId)) {
                if (_requestedEvents.size() < kMaxUniqueEvents) {
                    _requestedEvents.insert(observation.eventId);
                    _resolveRequests.push_back(MusicReconResolveRequest{ observation.eventId });
                } else {
                    ++_eventOverflow;
                }
            }
            return true;
        } catch (...) {
            ++_diagnosticOverflow;
            return false;
        }
    }

    void MusicReconProbe::noteDroppedWwise(std::uint64_t count) noexcept
    {
        _droppedWwise += count;
    }

    std::vector<MusicReconResolveRequest> MusicReconProbe::takeResolveRequests(std::size_t maxCount)
    {
        std::vector<MusicReconResolveRequest> result;
        const auto count = std::min(maxCount, _resolveRequests.size());
        result.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            result.push_back(std::move(_resolveRequests.front()));
            _resolveRequests.pop_front();
        }
        return result;
    }

    void MusicReconProbe::observeResolved(MusicReconResolvedEvent result)
    {
        if (_finalized) {
            return;
        }

        bool candidate = result.musicNameCandidate || containsMusicToken(result.eventName) || containsMusicToken(result.bankName);
        for (const auto& media : result.media) {
            candidate = candidate || containsMusicToken(media.shortName) || containsMusicToken(media.originalPath);
        }
        result.musicNameCandidate = candidate;
        ++_resolved;
        if (candidate) {
            ++_musicNameCandidates;
        }

        std::ostringstream resolvedLine;
        resolvedLine << "Music recon: RESOLVED event=" << hexValue(result.eventId)
                     << " name=" << quoteText(result.eventName)
                     << " bank=" << quoteText(result.bankName)
                     << " candidate=" << (candidate ? "yes" : "no")
                     << " media=" << result.media.size();
        if (!result.metadataSource.empty()) {
            resolvedLine << " source=" << quoteText(result.metadataSource);
        }
        if (!result.error.empty()) {
            resolvedLine << " error=" << quoteText(result.error);
        }
        pushDiagnostic(resolvedLine.str());

        for (const auto& media : result.media) {
            std::ostringstream line;
            line << "Music recon: MEDIA event=" << hexValue(result.eventId)
                 << " media=" << media.mediaId
                 << " codec=" << media.structure.codecLabel
                 << " channels=" << media.structure.channels
                 << " rate=" << media.structure.sampleRate
                 << " decodeAttempted=" << (media.decodeAttempted ? "yes" : "no")
                 << " decodeReady=" << (media.decodeReady ? "yes" : "no")
                 << " frames=" << media.decodedFrames;
            if (!media.shortName.empty()) {
                line << " shortName=" << quoteText(media.shortName);
            }
            if (!media.originalPath.empty()) {
                line << " path=" << quoteText(media.originalPath);
            }
            pushDiagnostic(line.str());
        }
    }

    std::vector<std::string> MusicReconProbe::takeDiagnostics(std::size_t maxCount)
    {
        std::vector<std::string> result;
        const auto count = std::min(maxCount, _diagnostics.size());
        result.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            result.push_back(std::move(_diagnostics.front()));
            _diagnostics.pop_front();
        }
        return result;
    }

    std::string MusicReconProbe::finalize(std::chrono::steady_clock::time_point when)
    {
        (void)when;
        if (_finalized) {
            return _finalSummary;
        }
        _finalized = true;
        _resolveRequests.clear();

        std::vector<std::pair<AggregateKey, Aggregate>> ordered;
        ordered.reserve(_aggregates.size());
        for (const auto& [key, aggregate] : _aggregates) {
            ordered.emplace_back(key, aggregate);
        }
        std::sort(ordered.begin(), ordered.end(), [](const auto& left, const auto& right) {
            return std::tie(left.first.eventId, left.first.gameObjectId, left.first.callsiteRva) <
                std::tie(right.first.eventId, right.first.gameObjectId, right.first.callsiteRva);
        });

        for (const auto& [key, aggregate] : ordered) {
            std::ostringstream line;
            line << "Music recon: OBSERVED event=" << hexValue(key.eventId)
                 << " object=" << hexValue(key.gameObjectId)
                 << " callsite=" << hexValue(static_cast<std::uint64_t>(key.callsiteRva))
                 << " count=" << aggregate.count;
            pushDiagnostic(line.str());
        }

        std::ostringstream summary;
        summary << "Music recon: SUMMARY uniqueEvents=" << _requestedEvents.size()
                << " aggregates=" << _aggregates.size()
                << " resolved=" << _resolved
                << " musicNameCandidates=" << _musicNameCandidates
                << " droppedWwise=" << _droppedWwise
                << " aggregateOverflow=" << _aggregateOverflow
                << " eventOverflow=" << _eventOverflow
                << " menuOverflow=" << _menuOverflow
                << " diagnosticOverflow=" << _diagnosticOverflow;
        _finalSummary = summary.str();
        pushDiagnostic(_finalSummary);
        return _finalSummary;
    }

    bool MusicReconProbe::finalized() const noexcept
    {
        return _finalized;
    }
}
