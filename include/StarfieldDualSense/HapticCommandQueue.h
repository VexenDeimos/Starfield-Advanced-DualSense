#pragma once

#include <StarfieldDualSense/HapticTypes.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <utility>
#include <vector>

namespace sds
{
    struct HapticQueueDropCounts
    {
        std::uint64_t overflow{ 0 };
        std::uint64_t stale{ 0 };
    };

    class HapticCommandQueue
    {
    public:
        static constexpr std::size_t kCapacity = 128;

        bool push(HapticCommand command) noexcept
        {
            std::scoped_lock lock(_mutex);
            if (_stopped) {
                return false;
            }
            if (_queue.size() == kCapacity) {
                _queue.pop_front();
                ++_drops.overflow;
            }
            _queue.push_back(std::move(command));
            return true;
        }

        std::vector<HapticCommand> drainFresh(
            std::chrono::steady_clock::time_point now,
            std::chrono::milliseconds maxAge)
        {
            std::vector<HapticCommand> fresh;
            std::scoped_lock lock(_mutex);
            fresh.reserve(_queue.size());
            while (!_queue.empty()) {
                auto command = std::move(_queue.front());
                _queue.pop_front();
                if (now - command.when > maxAge) {
                    ++_drops.stale;
                } else {
                    fresh.push_back(std::move(command));
                }
            }
            return fresh;
        }

        HapticQueueDropCounts takeDropCounts() noexcept
        {
            std::scoped_lock lock(_mutex);
            const auto result = _drops;
            _drops = {};
            return result;
        }

        void stop() noexcept
        {
            std::scoped_lock lock(_mutex);
            _stopped = true;
            _queue.clear();
        }

        void reset() noexcept
        {
            std::scoped_lock lock(_mutex);
            _stopped = false;
            _queue.clear();
            _drops = {};
        }

    private:
        std::mutex _mutex{};
        std::deque<HapticCommand> _queue{};
        HapticQueueDropCounts _drops{};
        bool _stopped{ false };
    };
}
