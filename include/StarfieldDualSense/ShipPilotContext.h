#pragma once

#include <cstdint>
#include <string_view>

namespace sds
{
    enum class ShipPilotTransition : std::uint8_t
    {
        None,
        Entered,
        Exited,
        Invalidated,
        Resumed,
        ExitedAfterLoad
    };

    class ShipPilotContext
    {
    public:
        [[nodiscard]] ShipPilotTransition observeMenu(
            std::string_view menu,
            bool opening) noexcept;

        void reset() noexcept;

        [[nodiscard]] bool piloting() const noexcept { return _piloting; }
        [[nodiscard]] bool loading() const noexcept { return _loading; }

    private:
        bool _piloting{ false };
        bool _loading{ false };
        bool _spaceshipHudOpen{ false };
        bool _invalidatedPilotContext{ false };
    };
}
