#include <StarfieldDualSense/SpeakerMixer.h>

#include <algorithm>
#include <cmath>
#include <numbers>

sds::SpeakerMixer::SpeakerMixer(float globalVolume) noexcept
{
    setGlobalVolume(globalVolume);
}

void sds::SpeakerMixer::setGlobalVolume(float volume) noexcept
{
    constexpr float kV0310MaxAtConfiguredVolume = 0.8F;
    _globalVolume = std::clamp(volume, 0.0F, 1.0F) / kV0310MaxAtConfiguredVolume;
}

bool sds::SpeakerMixer::addVoice(Voice voice)
{
    if (voice.frames.empty()) {
        return false;
    }
    if (_voices.size() >= kMaxSpeakerVoices) {
        ++_droppedSubmissions;
        return false;
    }
    _voices.push_back(std::move(voice));
    return true;
}

bool sds::SpeakerMixer::add(const SpeakerCommand& command)
{
    if (command.duration.count() <= 0 || !std::isfinite(command.frequencyHz) || command.frequencyHz <= 0.0F) {
        return false;
    }

    constexpr double sampleRate = 48000.0;
    const auto frameCount = static_cast<std::size_t>((std::max)(
        std::int64_t{ 1 }, static_cast<std::int64_t>(command.duration.count()) * std::int64_t{48}));
    const auto attackFrames = (std::min)(frameCount, std::size_t{ 96 });
    const auto releaseFrames = (std::min)(frameCount, std::size_t{ 192 });

    Voice voice{};
    voice.prepared = false;
    voice.gain = std::clamp(command.gain, 0.0F, 4.0F);
    voice.frames.resize(frameCount);
    for (std::size_t i = 0; i < frameCount; ++i) {
        const double phase = 2.0 * std::numbers::pi * static_cast<double>(command.frequencyHz) *
            static_cast<double>(i) / sampleRate;
        float envelope = 1.0F;
        if (attackFrames > 1 && i < attackFrames) {
            envelope *= static_cast<float>(i) / static_cast<float>(attackFrames - 1);
        }
        const auto remaining = frameCount - 1 - i;
        if (releaseFrames > 1 && remaining < releaseFrames) {
            envelope *= static_cast<float>(remaining) / static_cast<float>(releaseFrames - 1);
        }
        const float sample = static_cast<float>(std::sin(phase)) * envelope;
        switch (command.routing) {
        case SpeakerRoutingMode::Channel1:
            voice.frames[i] = { sample, 0.0F };
            break;
        case SpeakerRoutingMode::Channel2:
            voice.frames[i] = { 0.0F, sample };
            break;
        case SpeakerRoutingMode::Both:
        default:
            voice.frames[i] = { sample, sample };
            break;
        }
    }
    return addVoice(std::move(voice));
}

bool sds::SpeakerMixer::addPrepared(const PreparedSpeakerPcm& pcm)
{
    Voice voice{};
    voice.prepared = true;
    voice.frames = pcm.frames;
    voice.gain = std::clamp(pcm.gain, 0.0F, 4.0F);
    return addVoice(std::move(voice));
}

bool sds::SpeakerMixer::setPersistentPrepared(PersistentPreparedSpeakerPcm voice) noexcept
{
    try {
        if (voice.owner == 0u) {
            return false;
        }

        PersistentVoice persistent{};
        persistent.owner = voice.owner;
        persistent.gainScale = std::clamp(voice.gainScale, 0.0F, 4.0F);

        if (!voice.layers.empty()) {
            persistent.layers.reserve(voice.layers.size());
            for (auto& layer : voice.layers) {
                if (!layer.pcm || layer.pcm->frames.empty() ||
                    layer.loopResumeFrame >= layer.pcm->frames.size()) {
                    return false;
                }
                persistent.layers.push_back({ std::move(layer.pcm), 0u, layer.loopResumeFrame });
            }
        } else {
            if (!voice.pcm || voice.pcm->frames.empty() ||
                voice.loopResumeFrame >= voice.pcm->frames.size()) {
                return false;
            }
            persistent.layers.push_back({ std::move(voice.pcm), 0u, voice.loopResumeFrame });
        }

        _persistent = std::move(persistent);
        return true;
    } catch (...) {
        return false;
    }
}

bool sds::SpeakerMixer::clearPersistentPrepared(std::uint64_t owner, bool force) noexcept
{
    if (!_persistent || (!force && _persistent->owner != owner)) {
        return false;
    }
    _persistent.reset();
    return true;
}

std::uint64_t sds::SpeakerMixer::persistentOwner() const noexcept
{
    return _persistent ? _persistent->owner : 0u;
}

void sds::SpeakerMixer::render(std::span<StereoSpeakerFrame> output) noexcept
{
    std::fill(output.begin(), output.end(), StereoSpeakerFrame{});

    for (auto& voice : _voices) {
        for (std::size_t outIndex = 0; outIndex < output.size() && voice.cursor < voice.frames.size();
             ++outIndex, ++voice.cursor) {
            const auto& frame = voice.frames[voice.cursor];
            output[outIndex].left += frame.left * voice.gain * _globalVolume;
            output[outIndex].right += frame.right * voice.gain * _globalVolume;
        }
    }

    if (_persistent) {
        auto& voice = *_persistent;
        for (auto& layer : voice.layers) {
            if (!layer.pcm || layer.pcm->frames.empty()) {
                continue;
            }
            const auto& source = *layer.pcm;
            const float gain = source.gain * voice.gainScale * _globalVolume;
            for (std::size_t outIndex = 0; outIndex < output.size(); ++outIndex) {
                if (layer.cursor >= source.frames.size()) {
                    layer.cursor = layer.loopResumeFrame;
                }
                const auto& frame = source.frames[layer.cursor++];
                output[outIndex].left += frame.left * gain;
                output[outIndex].right += frame.right * gain;
            }
        }
    }

    for (auto& frame : output) {
        frame.left = std::clamp(frame.left, -1.0F, 1.0F);
        frame.right = std::clamp(frame.right, -1.0F, 1.0F);
    }

    std::erase_if(_voices, [](const Voice& voice) { return voice.cursor >= voice.frames.size(); });
}

void sds::SpeakerMixer::clearPrepared() noexcept
{
    std::erase_if(_voices, [](const Voice& voice) { return voice.prepared; });
}

void sds::SpeakerMixer::clear() noexcept
{
    _voices.clear();
    _persistent.reset();
}
