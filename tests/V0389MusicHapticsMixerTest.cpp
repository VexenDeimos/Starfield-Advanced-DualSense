#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/MusicHapticsMixer.h>

#include <algorithm>
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
        std::cout << "PASS " << message << '\n';
    }

    std::shared_ptr<const sds::PreparedSpeakerPcm> constantPcm(
        std::size_t frames,
        float left,
        float right)
    {
        auto pcm = std::make_shared<sds::PreparedSpeakerPcm>();
        pcm->gain = 1.0F;
        pcm->frames.assign(frames, sds::StereoSpeakerFrame{ left, right });
        return pcm;
    }

    float peakChannel(const std::vector<sds::HapticFrame>& block, std::size_t channel)
    {
        float peak = 0.0F;
        for (const auto& frame : block) {
            peak = (std::max)(peak, std::fabs(frame[channel]));
        }
        return peak;
    }
}

int main()
{
    {
        const auto defaults = sds::Config::defaults();
        require(defaults.musicHapticsEnabled, "MusicHapticsEnabled defaults on");
        const auto disabled = sds::loadConfig("MusicHapticsEnabled = false\n");
        require(!disabled.musicHapticsEnabled, "MusicHapticsEnabled parses false independently");
        require(disabled.advancedHaptics, "music toggle does not disable AdvancedHaptics");
    }

    const auto longPcm = constantPcm(40000u, 1.0F, 0.0F);

    {
        sds::MusicHapticsMixer fresh;
        require(fresh.add({
            .playingId = 1u,
            .eventId = 2u,
            .mediaId = 3u,
            .pcm = longPcm,
            .startFrame = 0u,
            .gain = 1.0F,
        }), "fresh music voice is accepted");
        std::vector<sds::HapticFrame> block(128u);
        fresh.render(block);
        require(peakChannel(block, 2u) < 0.02F, "350 ms fade keeps a just-selected voice gentle");
    }

    {
        sds::MusicHapticsMixer aged;
        require(aged.add({
            .playingId = 4u,
            .eventId = 5u,
            .mediaId = 6u,
            .pcm = longPcm,
            .startFrame = sds::kMusicHapticsFadeFrames,
            .gain = 1.0F,
        }), "age-offset music voice is accepted");
        std::vector<sds::HapticFrame> block(1024u);
        aged.render(block);
        require(peakChannel(block, 2u) < 0.08F,
            "late-decoded age-offset voice still fades in from physical haptic onset");

        std::vector<sds::HapticFrame> settle(sds::kMusicHapticsFadeFrames);
        aged.render(settle);
        std::vector<sds::HapticFrame> fullBlock(1024u);
        aged.render(fullBlock);
        const float leftPeak = peakChannel(fullBlock, 2u);
        const float rightPeak = peakChannel(fullBlock, 3u);
        require(leftPeak > 0.40F, "age-derived start offset preserves media position while fade completes independently");
        require(rightPeak > 0.05F, "stereo-ish crossfeed keeps the opposite actuator lightly engaged");
        require(leftPeak > rightPeak * 4.0F, "85/15 stereo-ish mapping strongly favors the source side");
        require(leftPeak <= sds::kMusicHapticsPeakCap + 0.0001F, "music layer never exceeds the 0.65 peak cap");
    }

    {
        sds::MusicHapticsMixer bounded;
        for (std::size_t i = 0; i < sds::kMaxMusicHapticVoices; ++i) {
            require(bounded.add({
                .playingId = static_cast<std::uint32_t>(100u + i),
                .eventId = 10u,
                .mediaId = static_cast<std::uint32_t>(1000u + i),
                .pcm = longPcm,
                .startFrame = sds::kMusicHapticsFadeFrames,
                .gain = 1.0F,
            }), "bounded music voice slot accepts within capacity");
        }
        require(!bounded.add({
            .playingId = 999u,
            .eventId = 10u,
            .mediaId = 9999u,
            .pcm = longPcm,
            .startFrame = sds::kMusicHapticsFadeFrames,
            .gain = 1.0F,
        }), "33rd simultaneous music voice fails soft");
        require(bounded.droppedSubmissions() == 1u, "music voice overflow increments drop counter");
        std::vector<sds::HapticFrame> block(1024u);
        bounded.render(block);
        require(peakChannel(block, 2u) <= sds::kMusicHapticsPeakCap + 0.0001F,
            "many simultaneous voices remain capped at 0.65");
        require(bounded.stopPlayingId(100u) == 1u, "playing-id stop removes only matching voice");
        require(bounded.activeVoiceCount() == sds::kMaxMusicHapticVoices - 1u,
            "playing-id stop leaves sibling music voices active");
        bounded.clear();
        require(bounded.empty(), "music hard-clear removes every voice");
    }

    {
        sds::MusicHapticsMixer finite;
        auto shortPcm = constantPcm(4u, 1.0F, 1.0F);
        require(finite.add({
            .playingId = 77u,
            .eventId = 88u,
            .mediaId = 99u,
            .pcm = shortPcm,
            .startFrame = 0u,
            .gain = 1.0F,
        }), "finite music voice is accepted");
        std::vector<sds::HapticFrame> block(16u);
        finite.render(block);
        require(finite.empty(), "music voice retires naturally when decoded PCM ends");
    }

    return 0;
}
