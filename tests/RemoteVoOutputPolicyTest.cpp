#include <StarfieldDualSense/RemoteVoOutputPolicy.h>
#include <StarfieldDualSense/Config.h>

#include <cstdio>

namespace
{
    int failures = 0;

    void expect(bool condition, const char* message)
    {
        if (!condition) {
            std::fprintf(stderr, "FAIL: %s\n", message);
            ++failures;
        }
    }
}

int main()
{
    using sds::RemoteVoOriginalOutputAction;
    using sds::SpeakerOutputMode;

    expect(
        sds::decideRemoteVoOriginalOutput(SpeakerOutputMode::Both, true, 42) ==
            RemoteVoOriginalOutputAction::KeepOriginal,
        "Both keeps Starfield original audio after accepted controller playback");

    expect(
        sds::decideRemoteVoOriginalOutput(SpeakerOutputMode::ControllerOnly, true, 42) ==
            RemoteVoOriginalOutputAction::StopOriginal,
        "ControllerOnly stops Starfield original audio after accepted controller playback");

    expect(
        sds::decideRemoteVoOriginalOutput(SpeakerOutputMode::ControllerOnly, false, 42) ==
            RemoteVoOriginalOutputAction::KeepOriginal,
        "ControllerOnly keeps Starfield original audio when controller submission fails");

    expect(
        sds::decideRemoteVoOriginalOutput(SpeakerOutputMode::ControllerOnly, true, 0) ==
            RemoteVoOriginalOutputAction::KeepOriginal,
        "ControllerOnly keeps Starfield original audio when there is no valid playing ID to stop");

    return failures == 0 ? 0 : 1;
}
