#include <StarfieldDualSense/SpeakerMixer.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
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

    std::shared_ptr<const sds::PreparedSpeakerPcm> pcm(std::initializer_list<float> samples)
    {
        auto out = std::make_shared<sds::PreparedSpeakerPcm>();
        for (const auto sample : samples) {
            out->frames.push_back({ sample, sample });
        }
        out->gain = 1.0F;
        return out;
    }
}

int main()
{
    sds::SpeakerMixer mixer(0.8F);

    sds::PersistentPreparedSpeakerPcm voice{};
    voice.owner = 44u;
    voice.gainScale = 1.0F;
    voice.layers.push_back({ pcm({ 0.10F, 0.20F, 0.30F }), 1u });
    voice.layers.push_back({ pcm({ 0.01F, 0.02F }), 1u });
    voice.layers.push_back({ pcm({ 0.001F, 0.002F, 0.003F, 0.004F }), 1u });

    require(mixer.setPersistentPrepared(std::move(voice)), "three-layer persistent voice is accepted");
    require(mixer.persistentOwner() == 44u, "layered voice keeps one logical owner");

    std::vector<sds::StereoSpeakerFrame> out(8u);
    mixer.render(out);

    // Each layer advances and wraps independently at its own resume point.
    const float expected[] = {
        0.111F, // .10 + .01 + .001
        0.222F, // .20 + .02 + .002
        0.323F, // .30 + .02 + .003
        0.224F, // .20 + .02 + .004
        0.322F, // .30 + .02 + .002 (layer 3 wraps to its resume frame 1)
        0.223F, // .20 + .02 + .003
        0.324F,
        0.222F,
    };
    for (std::size_t i = 0; i < out.size(); ++i) {
        require(near(out[i].left, expected[i]) && near(out[i].right, expected[i]),
            "all persistent layers mix simultaneously with independent cursors");
    }

    require(mixer.clearPersistentPrepared(44u), "one owner clear removes the full layered voice");
    require(mixer.empty(), "layered persistent voice is gone after owner clear");

    std::cout << "PASS v0.3.44 layered persistent speaker mixer\n";
    return 0;
}
