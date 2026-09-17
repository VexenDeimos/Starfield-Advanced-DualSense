#include <StarfieldDualSense/MusicHapticsMixer.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr double kCarrierHz = 78.0;
    constexpr double kAttackSeconds = 0.008;
    constexpr double kReleaseSeconds = 0.080;
    constexpr float kPrimaryMix = 0.85F;
    constexpr float kCrossfeedMix = 0.15F;
    constexpr float kEnvelopeMakeup = 1.40F;

    float followEnvelope(float current, float target) noexcept
    {
        const double tau = target >= current ? kAttackSeconds : kReleaseSeconds;
        const double step = 1.0 - std::exp(-1.0 / (kSampleRate * tau));
        return static_cast<float>(static_cast<double>(current) +
            (static_cast<double>(target) - static_cast<double>(current)) * step);
    }
}

void sds::mixMusicUnderGameplay(
    std::span<HapticFrame> gameplay,
    std::span<const HapticFrame> music) noexcept
{
    const auto count = (std::min)(gameplay.size(), music.size());
    for (std::size_t i = 0; i < count; ++i) {
        const float gameplayPeak = (std::max)(
            std::fabs(gameplay[i][2]),
            std::fabs(gameplay[i][3]));
        const float musicGain = std::clamp(
            1.0F - gameplayPeak / kMusicHapticsGameplayDuckFullScale,
            0.0F,
            1.0F);
        gameplay[i][2] = std::clamp(
            gameplay[i][2] + music[i][2] * musicGain,
            -1.0F,
            1.0F);
        gameplay[i][3] = std::clamp(
            gameplay[i][3] + music[i][3] * musicGain,
            -1.0F,
            1.0F);
    }
}

bool sds::MusicHapticsMixer::add(MusicHapticVoice voice) noexcept
{
    try {
        if (!voice.pcm || voice.pcm->frames.empty() || voice.playingId == 0u ||
            voice.eventId == 0u || voice.mediaId == 0u || voice.startFrame >= voice.pcm->frames.size()) {
            return false;
        }
        if (_voices.size() >= kMaxMusicHapticVoices) {
            ++_droppedSubmissions;
            return false;
        }

        voice.gain = std::clamp(voice.gain, 0.0F, 1.0F);
        const auto& first = voice.pcm->frames[voice.startFrame];
        const float sourceGain = std::clamp(voice.pcm->gain, 0.0F, 4.0F) * voice.gain;
        VoiceState state{};
        state.cursor = voice.startFrame;
        state.envelopeLeft = std::clamp(std::fabs(first.left) * sourceGain, 0.0F, 1.0F);
        state.envelopeRight = std::clamp(std::fabs(first.right) * sourceGain, 0.0F, 1.0F);
        state.voice = std::move(voice);
        _voices.push_back(std::move(state));
        return true;
    } catch (...) {
        ++_droppedSubmissions;
        return false;
    }
}

std::size_t sds::MusicHapticsMixer::stopPlayingId(std::uint32_t playingId) noexcept
{
    if (playingId == 0u) {
        return 0u;
    }
    const auto before = _voices.size();
    std::erase_if(_voices, [playingId](const VoiceState& state) {
        return state.voice.playingId == playingId;
    });
    return before - _voices.size();
}

void sds::MusicHapticsMixer::clear() noexcept
{
    _voices.clear();
    _carrierPhase = 0.0;
}

void sds::MusicHapticsMixer::render(std::span<HapticFrame> output) noexcept
{
    for (auto& frame : output) {
        frame = HapticFrame{};
    }

    for (auto& frame : output) {
        float leftEnergy = 0.0F;
        float rightEnergy = 0.0F;

        for (auto& state : _voices) {
            if (!state.voice.pcm || state.cursor >= state.voice.pcm->frames.size()) {
                continue;
            }

            const auto& source = state.voice.pcm->frames[state.cursor];
            const float sourceGain = std::clamp(state.voice.pcm->gain, 0.0F, 4.0F) * state.voice.gain;
            const float targetLeft = std::clamp(std::fabs(source.left) * sourceGain, 0.0F, 1.0F);
            const float targetRight = std::clamp(std::fabs(source.right) * sourceGain, 0.0F, 1.0F);
            state.envelopeLeft = followEnvelope(state.envelopeLeft, targetLeft);
            state.envelopeRight = followEnvelope(state.envelopeRight, targetRight);

            const float fade = std::clamp(
                static_cast<float>(state.renderedFrames + 1u) / static_cast<float>(kMusicHapticsFadeFrames),
                0.0F,
                1.0F);
            const float shapedLeft = std::clamp(state.envelopeLeft * kEnvelopeMakeup, 0.0F, 1.0F) * fade;
            const float shapedRight = std::clamp(state.envelopeRight * kEnvelopeMakeup, 0.0F, 1.0F) * fade;
            leftEnergy += kPrimaryMix * shapedLeft + kCrossfeedMix * shapedRight;
            rightEnergy += kCrossfeedMix * shapedLeft + kPrimaryMix * shapedRight;
            ++state.cursor;
            ++state.renderedFrames;
        }

        leftEnergy = std::clamp(leftEnergy, 0.0F, 1.0F);
        rightEnergy = std::clamp(rightEnergy, 0.0F, 1.0F);
        const float carrier = static_cast<float>(std::sin(_carrierPhase));
        frame[2] = std::clamp(carrier * leftEnergy * kMusicHapticsPeakCap, -kMusicHapticsPeakCap, kMusicHapticsPeakCap);
        frame[3] = std::clamp(carrier * rightEnergy * kMusicHapticsPeakCap, -kMusicHapticsPeakCap, kMusicHapticsPeakCap);
        _carrierPhase = std::fmod(
            _carrierPhase + 2.0 * std::numbers::pi * kCarrierHz / kSampleRate,
            2.0 * std::numbers::pi);
    }

    std::erase_if(_voices, [](const VoiceState& state) {
        return !state.voice.pcm || state.cursor >= state.voice.pcm->frames.size();
    });
}
