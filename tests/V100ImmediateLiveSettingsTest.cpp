#include "StarfieldDualSense/ImmediateLiveSettings.h"

#include <cmath>
#include <iostream>
#include <string_view>

namespace
{
    bool near(float a, float b)
    {
        return std::fabs(a - b) < 0.0001F;
    }
}

int main()
{
    int failures = 0;
    const auto expect = [&](bool condition, std::string_view message) {
        if (condition) {
            std::cout << "PASS " << message << '\n';
        } else {
            std::cerr << "FAIL " << message << '\n';
            ++failures;
        }
    };

    sds::Config config{};
    config.advancedHaptics = true;
    config.musicHapticsEnabled = true;
    config.hapticStrength = 0.70F;
    config.musicHapticsStrength = 1.50F;
    config.speakerVolume = 0.25F;

    auto live = sds::immediateLiveSettings(config);
    expect(live.musicHapticsEnabled, "music haptics enabled only when both gates are enabled");
    expect(near(live.musicHapticsBaseStrength, 0.70F), "global haptic strength reaches music live state");
    expect(near(live.musicHapticsUserScale, 1.50F), "music-only strength reaches live state");
    expect(near(live.speakerVolume, 0.25F), "speaker master volume reaches live state");

    config.advancedHaptics = false;
    live = sds::immediateLiveSettings(config);
    expect(!live.musicHapticsEnabled, "AdvancedHaptics disables music haptics live");

    config.advancedHaptics = true;
    config.hapticStrength = 3.0F;
    config.musicHapticsStrength = -2.0F;
    config.speakerVolume = 4.0F;
    live = sds::immediateLiveSettings(config);
    expect(near(live.musicHapticsBaseStrength, 1.0F), "global haptic live strength clamps high");
    expect(near(live.musicHapticsUserScale, 0.0F), "music live scale clamps low");
    expect(near(live.speakerVolume, 1.0F), "speaker live volume clamps high");

    return failures == 0 ? 0 : 1;
}