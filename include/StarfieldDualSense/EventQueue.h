#pragma once

#include <StarfieldDualSense/Types.h>

#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>

namespace sds
{
    template <std::size_t Capacity>
    class EventQueue
    {
    public:
        static_assert(Capacity > 0);

        bool push(GameEvent event)
        {
            std::scoped_lock lock(_mutex);
            if (_stopped || _queue.size() >= Capacity) {
                return false;
            }
            _queue.push_back(std::move(event));
            return true;
        }

        [[nodiscard]] std::optional<GameEvent> tryPop()
        {
            std::scoped_lock lock(_mutex);
            if (_queue.empty()) {
                return std::nullopt;
            }
            auto event = std::move(_queue.front());
            _queue.pop_front();
            return event;
        }

        void stop()
        {
            std::scoped_lock lock(_mutex);
            _stopped = true;
        }

        [[nodiscard]] bool stopped() const
        {
            std::scoped_lock lock(_mutex);
            return _stopped;
        }

    private:
        mutable std::mutex _mutex;
        std::deque<GameEvent> _queue;
        bool _stopped{ false };
    };
}
