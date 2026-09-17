#include <StarfieldDualSense/MusicHapticsMixer.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL " << message << '\n';
            std::exit(1);
        }
        std::cout << "PASS " << message << '\n';
    }

    bool near(float actual, float expected, float tolerance = 0.0001F)
    {
        return std::fabs(actual - expected) <= tolerance;
    }
}

int main()
{
    {
        std::vector<sds::HapticFrame> gameplay(1u);
        std::vector<sds::HapticFrame> music(1u);
        music[0][2] = 0.50F;
        music[0][3] = -0.25F;
        sds::mixMusicUnderGameplay(gameplay, music);
        require(near(gameplay[0][2], 0.50F), "silent gameplay preserves left music layer");
        require(near(gameplay[0][3], -0.25F), "silent gameplay preserves right music layer");
    }

    {
        std::vector<sds::HapticFrame> gameplay(1u);
        std::vector<sds::HapticFrame> music(1u);
        gameplay[0][2] = 0.325F;
        gameplay[0][3] = -0.10F;
        music[0][2] = 0.40F;
        music[0][3] = 0.40F;
        sds::mixMusicUnderGameplay(gameplay, music);
        require(near(gameplay[0][2], 0.525F), "half-scale gameplay ducks music by half before mixing");
        require(near(gameplay[0][3], 0.10F), "shared gameplay magnitude applies symmetric music ducking");
    }

    {
        std::vector<sds::HapticFrame> gameplay(1u);
        std::vector<sds::HapticFrame> music(1u);
        gameplay[0][2] = 0.80F;
        gameplay[0][3] = -0.70F;
        music[0][2] = -0.60F;
        music[0][3] = 0.60F;
        sds::mixMusicUnderGameplay(gameplay, music);
        require(near(gameplay[0][2], 0.80F), "strong gameplay fully suppresses left music contribution");
        require(near(gameplay[0][3], -0.70F), "strong gameplay fully suppresses right music contribution");
    }

    {
        std::vector<sds::HapticFrame> gameplay(2u);
        std::vector<sds::HapticFrame> music(1u);
        gameplay[0][2] = 0.95F;
        gameplay[1][2] = -0.95F;
        music[0][2] = 0.65F;
        sds::mixMusicUnderGameplay(gameplay, music);
        require(near(gameplay[0][2], 0.95F), "priority helper only consumes overlapping frames");
        require(near(gameplay[1][2], -0.95F), "priority helper leaves non-overlap gameplay untouched");
    }

    return 0;
}
