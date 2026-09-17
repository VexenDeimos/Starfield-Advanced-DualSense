#include <StarfieldDualSense/DualSenseAudioTransport.h>

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

    sds::PersistentPreparedSpeakerPcm voice(std::uint64_t owner)
    {
        auto pcm = std::make_shared<sds::PreparedSpeakerPcm>();
        pcm->frames = { { 0.1F, -0.1F }, { 0.2F, -0.2F }, { 0.3F, -0.3F } };
        pcm->gain = 0.35F;
        return { owner, std::move(pcm), 1u, 1.0F };
    }
}

int main()
{
    sds::DualSenseAudioTransport transport({}, false, 0.8F);
    std::vector<sds::SpeakerPersistentInvalidationReason> reasons;
    transport.setSpeakerPersistentInvalidationCallback(
        [&](sds::SpeakerPersistentInvalidationReason reason) { reasons.push_back(reason); });

    require(!transport.setPersistentPreparedPcm(voice(1u)),
        "speaker client disabled rejects persistent set");

    transport.testSetSpeakerClientActive(true);
    transport.testSetTransportActive(false);
    require(!transport.setPersistentPreparedPcm(voice(1u)),
        "active client without live endpoint rejects persistent set instead of queueing stale audio");

    transport.testSetTransportActive(true);
    require(transport.setPersistentPreparedPcm(voice(10u)), "live endpoint accepts persistent set A");
    require(transport.setPersistentPreparedPcm(voice(11u)), "latest persistent set B replaces pending A");
    require(transport.testApplyPendingPersistentUpdate(), "pending persistent set applies on render transfer");
    require(transport.testPersistentOwner() == 11u, "only latest owner B reaches mixer");

    require(transport.setPersistentPreparedPcm(voice(20u)), "pending A accepted for clear-wins test");
    require(transport.clearPersistentPreparedPcm(20u), "later pending clear accepted");
    require(transport.testApplyPendingPersistentUpdate(), "clear-wins pending update applies");
    require(transport.testPersistentOwner() == 11u,
        "clear for pending owner does not disturb previously active different owner");

    require(transport.clearPersistentPreparedPcm(10u), "stale clear can be queued");
    require(transport.testApplyPendingPersistentUpdate(), "stale clear transfers");
    require(transport.testPersistentOwner() == 11u, "stale owner clear cannot remove owner 11");

    sds::PreparedSpeakerPcm finite{};
    finite.frames = { { 0.2F, 0.2F } };
    finite.gain = 1.0F;
    require(transport.replacePreparedPcm(finite), "finite replace remains accepted");
    require(transport.testPersistentOwner() == 11u, "finite replace does not clear persistent state");

    require(transport.clearPersistentPreparedPcm(0u, true), "force clear queues");
    require(transport.testApplyPendingPersistentUpdate(), "force clear transfers");
    require(transport.testPersistentOwner() == 0u, "force clear produces persistent silence");

    require(transport.setPersistentPreparedPcm(voice(30u)), "owner 30 accepted before endpoint invalidation");
    require(transport.testApplyPendingPersistentUpdate(), "owner 30 reaches mixer");
    require(transport.testPersistentOwner() == 30u, "owner 30 active before endpoint invalidation");
    transport.testSimulateEndpointInvalidation();
    require(transport.testPersistentOwner() == 0u, "endpoint invalidation clears active persistent state");
    require(reasons.size() == 1u && reasons.back() == sds::SpeakerPersistentInvalidationReason::EndpointInvalidated,
        "endpoint invalidation reports exactly one typed invalidation");

    transport.testSetTransportActive(true);
    require(transport.testPersistentOwner() == 0u, "fresh endpoint activation never reconstructs stale persistent voice");
    require(transport.setPersistentPreparedPcm(voice(40u)), "fresh Loop_Play equivalent can start after reconnect");
    require(transport.testApplyPendingPersistentUpdate(), "fresh post-reconnect set applies");
    require(transport.testPersistentOwner() == 40u, "fresh post-reconnect owner is active");

    transport.stopSpeakerClient();
    require(transport.testPersistentOwner() == 0u, "backend stop clears pending and active persistent state");
    require(reasons.size() == 2u && reasons.back() == sds::SpeakerPersistentInvalidationReason::BackendStop,
        "backend stop reports exactly one BackendStop after endpoint invalidation");

    const auto reasonCount = reasons.size();
    transport.testSimulateEndpointInvalidation();
    require(reasons.size() == reasonCount,
        "intentional stopped backend does not also report EndpointInvalidated");

    std::cout << "PASS v0.3.43 persistent speaker transport\n";
    return 0;
}
