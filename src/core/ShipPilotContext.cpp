#include <StarfieldDualSense/ShipPilotContext.h>

sds::ShipPilotTransition sds::ShipPilotContext::observeMenu(
    std::string_view menu,
    bool opening) noexcept
{
    if (menu == "LoadingMenu") {
        if (opening) {
            _loading = true;
            if (_piloting) {
                _piloting = false;
                _invalidatedPilotContext = true;
                return ShipPilotTransition::Invalidated;
            }
            return ShipPilotTransition::None;
        }

        _loading = false;
        if (_spaceshipHudOpen) {
            _piloting = true;
            const auto transition = _invalidatedPilotContext ?
                ShipPilotTransition::Resumed : ShipPilotTransition::Entered;
            _invalidatedPilotContext = false;
            return transition;
        }

        if (_invalidatedPilotContext) {
            _invalidatedPilotContext = false;
            return ShipPilotTransition::ExitedAfterLoad;
        }

        return ShipPilotTransition::None;
    }

    if (menu != "SpaceshipHudMenu") {
        return ShipPilotTransition::None;
    }

    if (opening) {
        _spaceshipHudOpen = true;
        if (_loading || _piloting) {
            return ShipPilotTransition::None;
        }

        _invalidatedPilotContext = false;
        _piloting = true;
        return ShipPilotTransition::Entered;
    }

    _spaceshipHudOpen = false;
    if (_piloting) {
        _piloting = false;
        _invalidatedPilotContext = false;
        return ShipPilotTransition::Exited;
    }

    if (!_loading) {
        _invalidatedPilotContext = false;
    }
    return ShipPilotTransition::None;
}

void sds::ShipPilotContext::reset() noexcept
{
    _piloting = false;
    _loading = false;
    _spaceshipHudOpen = false;
    _invalidatedPilotContext = false;
}
