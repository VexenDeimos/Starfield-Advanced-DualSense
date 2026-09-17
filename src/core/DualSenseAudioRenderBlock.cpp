#include <StarfieldDualSense/DualSenseAudioRenderBlock.h>

#include <algorithm>

sds::StereoSpeakerFrame sds::mapSpeakerToProvenUsbChannels(StereoSpeakerFrame logical) noexcept
{
    const float mono = std::clamp((logical.left + logical.right) * 0.5F, -1.0F, 1.0F);
    return { 0.0F, mono };
}

void sds::composeDualSenseAudioFrames(
    std::span<const StereoSpeakerFrame> speaker,
    std::span<const HapticFrame> haptics,
    std::span<DualSenseAudioFrame> output) noexcept
{
    const auto count = (std::min)({ speaker.size(), haptics.size(), output.size() });
    for (std::size_t i = 0; i < count; ++i) {
        const auto mappedSpeaker = mapSpeakerToProvenUsbChannels(speaker[i]);
        output[i].ch1 = mappedSpeaker.left;
        output[i].ch2 = mappedSpeaker.right;
        output[i].ch3 = std::clamp(haptics[i][2], -1.0F, 1.0F);
        output[i].ch4 = std::clamp(haptics[i][3], -1.0F, 1.0F);
    }
    for (std::size_t i = count; i < output.size(); ++i) {
        output[i] = {};
    }
}
