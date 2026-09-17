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

    std::shared_ptr<const sds::PreparedSpeakerPcm> loopPcm()
    {
        auto pcm = std::make_shared<sds::PreparedSpeakerPcm>();
        pcm->frames = { { 0.1F, -0.1F }, { 0.2F, -0.2F }, { 0.3F, -0.3F } };
        pcm->gain = 1.0F;
        return pcm;
    }
}

int main()
{
    {
        sds::SpeakerMixer mixer(0.8F);
        auto loop = loopPcm();
        std::weak_ptr<const sds::PreparedSpeakerPcm> weak = loop;
        require(mixer.setPersistentPrepared({ 10u, loop, 1u, 1.0F }), "persistent voice is accepted");
        loop.reset();
        require(!weak.expired(), "mixer retains immutable persistent PCM ownership");
        std::vector<sds::StereoSpeakerFrame> out(12u);
        mixer.render(out);
        const float expected[] = { 0.1F, 0.2F, 0.3F, 0.2F, 0.3F, 0.2F, 0.3F, 0.2F, 0.3F, 0.2F, 0.3F, 0.2F };
        for (std::size_t i = 0; i < out.size(); ++i) {
            require(near(out[i].left, expected[i]), "persistent wrap sequence is deterministic");
        }
        require(!mixer.empty() && mixer.persistentOwner() == 10u, "persistent voice survives source length");
        require(!mixer.clearPersistentPrepared(9u), "wrong-owner clear does nothing");
        require(mixer.persistentOwner() == 10u, "wrong-owner clear preserves voice");
        require(mixer.clearPersistentPrepared(10u), "matching-owner clear succeeds");
        require(mixer.persistentOwner() == 0u, "matching-owner clear removes voice");
        require(weak.expired(), "matching clear releases immutable PCM ownership");
    }

    {
        sds::SpeakerMixer mixer(0.8F);
        auto loop = loopPcm();
        require(mixer.setPersistentPrepared({ 10u, loop, 1u, 1.0F }), "owner 10 starts");
        require(mixer.setPersistentPrepared({ 11u, loop, 1u, 1.0F }), "owner 11 replaces owner 10");
        require(mixer.persistentOwner() == 11u, "replacement leaves only latest owner");
        require(!mixer.clearPersistentPrepared(10u), "stale owner cannot clear replacement");
        require(mixer.persistentOwner() == 11u, "stale clear leaves latest owner active");
        require(mixer.clearPersistentPrepared(0u, true), "force clear succeeds");
        require(mixer.empty(), "force clear leaves mixer empty");
    }

    {
        sds::SpeakerMixer mixer(0.8F);
        auto loop = loopPcm();
        require(mixer.setPersistentPrepared({ 22u, loop, 1u, 1.0F }), "persistent slot does not use finite capacity");
        sds::PreparedSpeakerPcm finite{};
        finite.frames = { { 0.01F, 0.01F } };
        finite.gain = 1.0F;
        for (std::size_t i = 0; i < sds::kMaxSpeakerVoices; ++i) {
            require(mixer.addPrepared(finite), "all 32 finite voices remain accepted");
        }
        require(!mixer.addPrepared(finite), "33rd finite voice is rejected exactly as before");
        mixer.clearPrepared();
        require(!mixer.empty() && mixer.persistentOwner() == 22u, "clearPrepared leaves persistent voice untouched");
        mixer.clear();
        require(mixer.empty(), "clear removes finite and persistent voices");
    }

    {
        sds::SpeakerMixer mixer(0.8F);
        auto loud = std::make_shared<sds::PreparedSpeakerPcm>();
        loud->frames = { { 0.8F, -0.8F } };
        loud->gain = 1.0F;
        require(mixer.setPersistentPrepared({ 30u, loud, 0u, 1.0F }), "resume frame zero is valid for a one-frame persistent source");
        sds::PreparedSpeakerPcm finite{};
        finite.frames = { { 0.8F, -0.8F } };
        finite.gain = 1.0F;
        require(mixer.addPrepared(finite), "finite voice accepted for clamp check");
        std::vector<sds::StereoSpeakerFrame> out(1u);
        mixer.render(out);
        require(near(out[0].left, 1.0F) && near(out[0].right, -1.0F), "finite plus persistent mix clamps to [-1,1]");
    }

    std::cout << "PASS v0.3.43 persistent speaker mixer\n";
    return 0;
}
