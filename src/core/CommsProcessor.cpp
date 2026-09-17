#include <StarfieldDualSense/CommsProcessor.h>

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float kSampleRate = 48000.0F;
    constexpr float kHighPassHz = 180.0F;
    constexpr float kLowPassHz = 5500.0F;
    constexpr float kCompressorThreshold = 0.25118864F;  // -12 dBFS
    constexpr float kCompressionRatio = 2.0F;
    constexpr float kSoftKneeWidth = 0.10F;
    constexpr float kMakeupGain = 1.99526231F;  // +6 dB
    constexpr float kLimiterThreshold = 0.90F;
    constexpr float kPi = 3.14159265358979323846F;

    struct FilterState
    {
        float hpPrevInput{ 0.0F };
        float hpPrevOutput{ 0.0F };
        float lpPrevOutput{ 0.0F };
    };

    [[nodiscard]] float highPass(float sample, FilterState& state) noexcept
    {
        const float rc = 1.0F / (2.0F * kPi * kHighPassHz);
        const float dt = 1.0F / kSampleRate;
        const float alpha = rc / (rc + dt);
        const float output = alpha * (state.hpPrevOutput + sample - state.hpPrevInput);
        state.hpPrevInput = sample;
        state.hpPrevOutput = output;
        return output;
    }

    [[nodiscard]] float lowPass(float sample, FilterState& state) noexcept
    {
        const float rc = 1.0F / (2.0F * kPi * kLowPassHz);
        const float dt = 1.0F / kSampleRate;
        const float alpha = dt / (rc + dt);
        state.lpPrevOutput += alpha * (sample - state.lpPrevOutput);
        return state.lpPrevOutput;
    }

    [[nodiscard]] float compress(float sample) noexcept
    {
        const float magnitude = std::fabs(sample);
        if (magnitude <= 0.0F) {
            return 0.0F;
        }

        const float kneeStart = kCompressorThreshold * (1.0F - kSoftKneeWidth);
        const float kneeEnd = kCompressorThreshold * (1.0F + kSoftKneeWidth);
        float outputMagnitude = magnitude;

        if (magnitude >= kneeEnd) {
            outputMagnitude = kCompressorThreshold +
                (magnitude - kCompressorThreshold) / kCompressionRatio;
        } else if (magnitude > kneeStart) {
            const float compressed = kCompressorThreshold +
                (magnitude - kCompressorThreshold) / kCompressionRatio;
            const float mix = (magnitude - kneeStart) / (kneeEnd - kneeStart);
            outputMagnitude = magnitude + (compressed - magnitude) * mix * mix;
        }

        return std::copysign(std::clamp(outputMagnitude, 0.0F, 1.0F), sample);
    }

    [[nodiscard]] float softLimit(float sample) noexcept
    {
        const float magnitude = std::fabs(sample);
        if (magnitude <= kLimiterThreshold) {
            return sample;
        }

        const float headroom = 1.0F - kLimiterThreshold;
        const float excess = magnitude - kLimiterThreshold;
        const float limitedMagnitude = kLimiterThreshold +
            headroom * (1.0F - std::exp(-excess / headroom));
        return std::copysign((std::min)(limitedMagnitude, 1.0F), sample);
    }

    [[nodiscard]] float processSample(float sample, FilterState& state) noexcept
    {
        const float compressed = compress(lowPass(highPass(sample, state), state));
        return softLimit(compressed * kMakeupGain);
    }
}

sds::PreparedSpeakerPcm sds::processCommsPcm(const PreparedSpeakerPcm& input)
{
    PreparedSpeakerPcm output{};
    output.gain = input.gain;
    output.frames.resize(input.frames.size());

    FilterState leftState{};
    FilterState rightState{};
    for (std::size_t i = 0; i < input.frames.size(); ++i) {
        output.frames[i].left = processSample(input.frames[i].left, leftState);
        output.frames[i].right = processSample(input.frames[i].right, rightState);
    }

    return output;
}
