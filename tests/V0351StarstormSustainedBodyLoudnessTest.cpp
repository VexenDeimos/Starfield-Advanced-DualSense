#include <StarfieldDualSense/WeaponSpeakerProfile.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            std::exit(1);
        }
    }

    bool near(float lhs, float rhs) noexcept
    {
        return std::fabs(lhs - rhs) < 0.0001F;
    }
}

int main()
{
    const auto* profile = sds::findWeaponSpeakerProfile("Va'ruun Starstorm");
    require(profile != nullptr, "Starstorm profile exists");
    require(profile->sustained.has_value(), "Starstorm sustained profile exists");

    const auto& sustained = *profile->sustained;
    require(sustained.loopVariants.size() == 1u, "Starstorm keeps one sustained body layer");
    require(sustained.loopVariants[0].pinnedMediaId == 963157375u,
        "Starstorm sustained body remains media 963157375");
    require(near(sustained.loopGain, 1.60F),
        "Starstorm sustained body uses approved 1.60 gain");
    require(near(sustained.startTransientGain, 0.35F),
        "Starstorm start accents remain at 0.35 gain");
    require(near(sustained.stopTransientGain, 0.35F),
        "Starstorm immediate stop remains at 0.35 gain");

    const auto powerDown = std::find_if(profile->cues.begin(), profile->cues.end(), [](const auto& cue) {
        return cue.action == "power-down";
    });
    require(powerDown != profile->cues.end(), "Starstorm power-down cue exists");
    require(near(powerDown->baseGain, 0.35F), "Starstorm power-down remains at 0.35 gain");

    std::cout << "PASS v0.3.51 Starstorm sustained body loudness isolation\n";
    return 0;
}
