#pragma once

#include <atomic>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>

namespace sds
{
    struct FireMarkerStageSnapshot
    {
        std::uint64_t callbacks{ 0 };
        std::uint64_t snapshots{ 0 };
        std::uint64_t playerMatches{ 0 };
        std::uint64_t nonEmptyTags{ 0 };
        std::uint64_t emitted{ 0 };
    };

    class FireMarkerStageCounters
    {
    public:
        void callbackEntered() noexcept { _callbacks.fetch_add(1, std::memory_order_relaxed); }
        void snapshotSucceeded() noexcept { _snapshots.fetch_add(1, std::memory_order_relaxed); }
        void holderMatchedPlayer() noexcept { _playerMatches.fetch_add(1, std::memory_order_relaxed); }
        void tagNonEmpty() noexcept { _nonEmptyTags.fetch_add(1, std::memory_order_relaxed); }
        void markerEmitted() noexcept { _emitted.fetch_add(1, std::memory_order_relaxed); }

        [[nodiscard]] FireMarkerStageSnapshot snapshot() const noexcept
        {
            return {
                _callbacks.load(std::memory_order_relaxed),
                _snapshots.load(std::memory_order_relaxed),
                _playerMatches.load(std::memory_order_relaxed),
                _nonEmptyTags.load(std::memory_order_relaxed),
                _emitted.load(std::memory_order_relaxed),
            };
        }

        void reset() noexcept
        {
            _callbacks.store(0, std::memory_order_relaxed);
            _snapshots.store(0, std::memory_order_relaxed);
            _playerMatches.store(0, std::memory_order_relaxed);
            _nonEmptyTags.store(0, std::memory_order_relaxed);
            _emitted.store(0, std::memory_order_relaxed);
        }

    private:
        std::atomic<std::uint64_t> _callbacks{ 0 };
        std::atomic<std::uint64_t> _snapshots{ 0 };
        std::atomic<std::uint64_t> _playerMatches{ 0 };
        std::atomic<std::uint64_t> _nonEmptyTags{ 0 };
        std::atomic<std::uint64_t> _emitted{ 0 };
    };

    inline std::string formatFireMarkerStageSummary(
        std::string_view weapon,
        const FireMarkerStageSnapshot& stages)
    {
        std::ostringstream out;
        out << "Fire marker trace summary: weapon='" << weapon << "'"
            << " callbacks=" << stages.callbacks
            << " snapshots=" << stages.snapshots
            << " playerMatches=" << stages.playerMatches
            << " nonEmptyTags=" << stages.nonEmptyTags
            << " emitted=" << stages.emitted;
        return out.str();
    }
}
