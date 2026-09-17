#include <StarfieldDualSense/SpeakerPcm.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace
{
    float decodeSample(const void* samples, std::size_t index, sds::SpeakerSampleFormat format) noexcept
    {
        switch (format) {
        case sds::SpeakerSampleFormat::Float32:
            return std::clamp(static_cast<const float*>(samples)[index], -1.0F, 1.0F);
        case sds::SpeakerSampleFormat::Pcm16: {
            const auto value = static_cast<const std::int16_t*>(samples)[index];
            if (value == (std::numeric_limits<std::int16_t>::min)()) {
                return -1.0F;
            }
            return static_cast<float>(value) / 32767.0F;
        }
        case sds::SpeakerSampleFormat::Pcm32: {
            const auto value = static_cast<const std::int32_t*>(samples)[index];
            if (value == (std::numeric_limits<std::int32_t>::min)()) {
                return -1.0F;
            }
            return static_cast<float>(static_cast<double>(value) / 2147483647.0);
        }
        default:
            return 0.0F;
        }
    }
}

std::optional<sds::PreparedSpeakerPcm> sds::prepareSpeakerPcm(
    const void* samples,
    std::size_t scalarSampleCount,
    SpeakerSampleFormat format,
    std::uint32_t sampleRate,
    std::uint16_t channels,
    float gain)
{
    if (!samples || scalarSampleCount == 0 || sampleRate == 0 || (channels != 1 && channels != 2) ||
        scalarSampleCount % channels != 0) {
        return std::nullopt;
    }

    const std::size_t inputFrames = scalarSampleCount / channels;
    if (inputFrames == 0) {
        return std::nullopt;
    }

    std::vector<StereoSpeakerFrame> source;
    source.reserve(inputFrames);
    for (std::size_t frame = 0; frame < inputFrames; ++frame) {
        const float left = decodeSample(samples, frame * channels, format);
        const float right = channels == 1 ? left : decodeSample(samples, frame * channels + 1, format);
        source.push_back({ left, right });
    }

    const auto outputFrames = (std::max)(
        std::size_t{ 1 },
        static_cast<std::size_t>(std::llround(
            static_cast<double>(inputFrames) * 48000.0 / static_cast<double>(sampleRate))));

    PreparedSpeakerPcm prepared{};
    prepared.gain = std::clamp(gain, 0.0F, 4.0F);
    prepared.frames.resize(outputFrames);

    if (inputFrames == 1) {
        std::fill(prepared.frames.begin(), prepared.frames.end(), source.front());
        return prepared;
    }

    const double step = static_cast<double>(sampleRate) / 48000.0;
    for (std::size_t out = 0; out < outputFrames; ++out) {
        const double position = static_cast<double>(out) * step;
        const auto lower = (std::min)(static_cast<std::size_t>(position), inputFrames - 1);
        const auto upper = (std::min)(lower + 1, inputFrames - 1);
        const float fraction = lower == upper ? 0.0F : static_cast<float>(position - static_cast<double>(lower));
        prepared.frames[out].left = source[lower].left + (source[upper].left - source[lower].left) * fraction;
        prepared.frames[out].right = source[lower].right + (source[upper].right - source[lower].right) * fraction;
    }

    return prepared;
}
