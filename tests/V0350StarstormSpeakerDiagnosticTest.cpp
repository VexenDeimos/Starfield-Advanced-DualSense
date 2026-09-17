#include <StarfieldDualSense/SpeakerPcmDiagnostics.h>
#include <StarfieldDualSense/DualSenseAudioRenderBlock.h>

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
    }

    bool near(float a, float b, float eps = 0.0001F)
    {
        return std::fabs(a - b) <= eps;
    }
}

int main()
{
    std::vector<sds::StereoSpeakerFrame> antiPhase(480u, { 0.5F, -0.5F });
    const auto anti = sds::measureSpeakerPcmLevels(antiPhase);
    require(near(anti.rmsLeft, 0.5F) && near(anti.rmsRight, 0.5F),
        "anti-phase source keeps strong independent channel RMS");
    require(anti.rmsMono < 0.0001F,
        "anti-phase source collapses to silence under proven Channel2 mono downmix");
    require(anti.correlation < -0.999F,
        "anti-phase source reports near-negative-one channel correlation");
    const auto mappedAnti = sds::mapSpeakerToProvenUsbChannels(antiPhase.front());
    require(near(mappedAnti.right, 0.0F),
        "anti-phase diagnostic prediction matches proven Channel2 mapping cancellation");

    std::vector<sds::StereoSpeakerFrame> inPhase(480u, { 0.25F, 0.25F });
    const auto in = sds::measureSpeakerPcmLevels(inPhase);
    require(near(in.rmsMono, 0.25F),
        "in-phase source preserves its RMS through mono downmix");
    require(in.correlation > 0.999F,
        "in-phase source reports near-positive-one channel correlation");

    const auto slice = sds::measureSpeakerPcmLevels(inPhase, 120u, 240u);
    require(slice.frames == 240u && near(slice.rmsMono, 0.25F),
        "diagnostic range measurement reports only the requested frame window");

    std::cout << "PASS v0.3.50 Starstorm speaker PCM diagnostic metrics\n";
    return 0;
}
