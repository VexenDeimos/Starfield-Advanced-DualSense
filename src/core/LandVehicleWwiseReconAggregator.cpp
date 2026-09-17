#include <StarfieldDualSense/LandVehicleWwiseReconAggregator.h>

#include <algorithm>

bool sds::LandVehicleWwiseReconAggregator::isNoisyEvent(std::uint32_t eventId) noexcept
{
    return std::find(kNoisyEventIds.begin(), kNoisyEventIds.end(), eventId) != kNoisyEventIds.end();
}

sds::LandVehicleWwiseReconObserveResult sds::LandVehicleWwiseReconAggregator::observe(
    const LandVehicleWwiseReconObservation& observation) noexcept
{
    if (_nextSummaryAt.time_since_epoch().count() == 0) {
        _nextSummaryAt = observation.when + kSummaryInterval;
    }

    const auto it = std::find(kNoisyEventIds.begin(), kNoisyEventIds.end(), observation.eventId);
    if (it == kNoisyEventIds.end()) {
        return {
            .preserveIndividually = true,
            .eventId = observation.eventId,
        };
    }

    const auto index = static_cast<std::size_t>(std::distance(kNoisyEventIds.begin(), it));
    auto& bucket = _buckets[index];
    if (bucket.count == 0) {
        bucket.firstSequence = observation.sequence;
    }
    ++bucket.count;
    bucket.lastSequence = observation.sequence;
    bucket.lastGameObjectId = observation.gameObjectId;
    bucket.lastCallsiteRva = observation.callsiteRva;

    return {
        .preserveIndividually = false,
        .eventId = observation.eventId,
    };
}

std::optional<sds::LandVehicleWwiseAggregateSummary>
sds::LandVehicleWwiseReconAggregator::takeSummaryIfDue(
    std::chrono::steady_clock::time_point now) noexcept
{
    if (_nextSummaryAt.time_since_epoch().count() == 0 || now < _nextSummaryAt) {
        return std::nullopt;
    }
    return takeSummary(now);
}

sds::LandVehicleWwiseAggregateSummary sds::LandVehicleWwiseReconAggregator::takeSummary(
    std::chrono::steady_clock::time_point now) noexcept
{
    LandVehicleWwiseAggregateSummary summary{};
    summary.when = now;

    for (std::size_t i = 0; i < _buckets.size(); ++i) {
        const auto& source = _buckets[i];
        if (source.count == 0) {
            continue;
        }
        auto& target = summary.buckets[summary.bucketCount++];
        target.eventId = kNoisyEventIds[i];
        target.count = source.count;
        target.firstSequence = source.firstSequence;
        target.lastSequence = source.lastSequence;
        target.lastGameObjectId = source.lastGameObjectId;
        target.lastCallsiteRva = source.lastCallsiteRva;
        summary.totalAggregated += source.count;
    }

    _buckets = {};
    _nextSummaryAt = now + kSummaryInterval;
    return summary;
}

void sds::LandVehicleWwiseReconAggregator::reset(std::chrono::steady_clock::time_point now) noexcept
{
    _buckets = {};
    _nextSummaryAt = now.time_since_epoch().count() == 0 ?
        std::chrono::steady_clock::time_point{} : now + kSummaryInterval;
}
