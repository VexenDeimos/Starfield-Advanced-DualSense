#pragma once

#include <StarfieldDualSense/MusicReconTypes.h>
#include <StarfieldDualSense/Types.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace sds
{
    class MusicReconProbe
    {
    public:
        void observeGameEvent(const GameEvent& event) noexcept;
        [[nodiscard]] bool observeWwise(const MusicReconWwiseObservation& observation) noexcept;
        void noteDroppedWwise(std::uint64_t count) noexcept;
        [[nodiscard]] std::vector<MusicReconResolveRequest> takeResolveRequests(std::size_t maxCount);
        void observeResolved(MusicReconResolvedEvent result);
        [[nodiscard]] std::vector<std::string> takeDiagnostics(std::size_t maxCount);
        [[nodiscard]] std::string finalize(std::chrono::steady_clock::time_point when);
        [[nodiscard]] bool finalized() const noexcept;

    private:
        static constexpr std::size_t kMaxAggregates = 2048;
        static constexpr std::size_t kMaxUniqueEvents = 2048;
        static constexpr std::size_t kMaxMenuTransitions = 128;
        static constexpr std::size_t kMaxDiagnostics = 4096;

        struct AggregateKey
        {
            std::uint32_t eventId{};
            std::uint64_t gameObjectId{};
            std::uintptr_t callsiteRva{};

            [[nodiscard]] bool operator==(const AggregateKey&) const noexcept = default;
        };

        struct AggregateKeyHash
        {
            [[nodiscard]] std::size_t operator()(const AggregateKey& key) const noexcept;
        };

        struct Aggregate
        {
            std::uint64_t count{};
            MusicReconWwiseObservation first{};
            MusicReconWwiseObservation last{};
        };

        void pushDiagnostic(std::string line);

        std::unordered_map<AggregateKey, Aggregate, AggregateKeyHash> _aggregates{};
        std::unordered_set<std::uint32_t> _requestedEvents{};
        std::deque<MusicReconResolveRequest> _resolveRequests{};
        std::deque<std::string> _diagnostics{};
        std::size_t _menuTransitions{};
        std::uint64_t _resolved{};
        std::uint64_t _musicNameCandidates{};
        std::uint64_t _droppedWwise{};
        std::uint64_t _aggregateOverflow{};
        std::uint64_t _eventOverflow{};
        std::uint64_t _menuOverflow{};
        std::uint64_t _diagnosticOverflow{};
        bool _finalized{ false };
        std::string _finalSummary{};
    };
}
