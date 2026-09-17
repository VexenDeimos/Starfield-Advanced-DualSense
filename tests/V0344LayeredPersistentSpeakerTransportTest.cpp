#include <StarfieldDualSense/DualSenseAudioTransport.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL " << message << '\n';
            std::exit(1);
        }
    }

    std::shared_ptr<const sds::PreparedSpeakerPcm> layer(float marker, std::size_t frames)
    {
        auto pcm = std::make_shared<sds::PreparedSpeakerPcm>();
        pcm->frames.assign(frames, { marker, marker });
        pcm->gain = 0.35F;
        return pcm;
    }

    sds::PersistentPreparedSpeakerPcm layeredVoice(std::uint64_t owner)
    {
        sds::PersistentPreparedSpeakerPcm voice{};
        voice.owner = owner;
        voice.gainScale = 1.0F;
        voice.layers.push_back({ layer(0.10F, 3u), 1u });
        voice.layers.push_back({ layer(0.20F, 4u), 1u });
        voice.layers.push_back({ layer(0.30F, 5u), 1u });
        return voice;
    }
}

int main()
{
    sds::DualSenseAudioTransport transport({}, false, 0.8F);
    transport.testSetSpeakerClientActive(true);
    transport.testSetTransportActive(true);

    require(transport.setPersistentPreparedPcm(layeredVoice(44u)),
        "live transport accepts a layered persistent voice without legacy pcm");
    require(transport.testApplyPendingPersistentUpdate(), "layered persistent update reaches mixer");
    require(transport.testPersistentOwner() == 44u, "layered persistent owner is active");
    require(transport.clearPersistentPreparedPcm(44u), "one owner clear queues for all layers");
    require(transport.testApplyPendingPersistentUpdate(), "layered owner clear reaches mixer");
    require(transport.testPersistentOwner() == 0u, "layered owner clear removes the complete beam");

    std::cout << "PASS v0.3.44 layered persistent speaker transport\n";
    return 0;
}
