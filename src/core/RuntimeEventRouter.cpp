#include <StarfieldDualSense/RuntimeEventRouter.h>

#include <utility>

sds::RuntimeEventRouter::RuntimeEventRouter(EventSink primary, EventSink secondary, LogCallback log) :
    _primary(std::move(primary)),
    _secondary(std::move(secondary)),
    _log(std::move(log))
{}

bool sds::RuntimeEventRouter::dispatch(GameEvent event) noexcept
{
    try {
        auto secondaryCopy = event;
        if (!_primary || !_primary(std::move(event))) {
            return false;
        }
        if (_secondary) {
            try {
                (void)_secondary(std::move(secondaryCopy));
            } catch (...) {
                if (_log) {
                    try {
                        _log("Runtime events: haptics dispatch exception ignored; controller path unaffected");
                    } catch (...) {
                    }
                }
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}
