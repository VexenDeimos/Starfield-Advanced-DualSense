#pragma once

#include <StarfieldDualSense/Types.h>

#include <functional>
#include <string_view>

namespace sds
{
    class RuntimeEventRouter
    {
    public:
        using EventSink = std::function<bool(GameEvent)>;
        using LogCallback = std::function<void(std::string_view)>;

        RuntimeEventRouter(EventSink primary, EventSink secondary = {}, LogCallback log = {});
        [[nodiscard]] bool dispatch(GameEvent event) noexcept;

    private:
        EventSink _primary{};
        EventSink _secondary{};
        LogCallback _log{};
    };
}
