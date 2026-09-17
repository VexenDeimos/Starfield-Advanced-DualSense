#include <StarfieldDualSense/RemoteVoSpeakerPlayback.h>
#include <StarfieldDualSense/CommsProcessor.h>
#include <StarfieldDualSense/SpeakerPcm.h>

#include <algorithm>
#include <cmath>
#include <sstream>
#include <utility>

namespace
{
    struct SpeakerLevels
    {
        float peak{ 0.0F };
        float rms{ 0.0F };
    };

    [[nodiscard]] SpeakerLevels measureLevels(const sds::PreparedSpeakerPcm& pcm) noexcept
    {
        SpeakerLevels levels{};
        if (pcm.frames.empty()) {
            return levels;
        }

        double sumSquares = 0.0;
        for (const auto& frame : pcm.frames) {
            levels.peak = (std::max)(levels.peak, (std::max)(std::fabs(frame.left), std::fabs(frame.right)));
            sumSquares += static_cast<double>(frame.left) * frame.left;
            sumSquares += static_cast<double>(frame.right) * frame.right;
        }
        levels.rms = static_cast<float>(std::sqrt(sumSquares / static_cast<double>(pcm.frames.size() * 2u)));
        return levels;
    }
}

sds::RemoteVoControllerPlaybackPreparation sds::prepareRemoteVoControllerPlayback(
    const VoiceWwiseVorbisDecodeResult& decoded,
    float gain)
{
    RemoteVoControllerPlaybackPreparation result{};
    result.attempted = true;
    result.sourceChannels = decoded.channels;
    result.sourceSampleRate = decoded.sampleRate;

    if (!decoded.decodeSucceeded) {
        result.error = "decoded voice is not successful";
        return result;
    }
    if (decoded.channels == 0u || decoded.sampleRate == 0u || decoded.pcm.empty()) {
        result.error = "decoded voice PCM format is invalid";
        return result;
    }
    if ((decoded.pcm.size() % decoded.channels) != 0u) {
        result.error = "decoded voice PCM is not frame-aligned";
        return result;
    }

    result.sourceFrames = static_cast<std::uint32_t>(decoded.pcm.size() / decoded.channels);
    auto prepared = prepareSpeakerPcm(
        decoded.pcm.data(),
        decoded.pcm.size(),
        SpeakerSampleFormat::Pcm16,
        decoded.sampleRate,
        decoded.channels,
        gain);
    if (!prepared) {
        result.error = "controller speaker PCM preparation failed";
        return result;
    }

    result.outputSampleRate = 48000u;
    result.outputFrames = prepared->frames.size();
    result.outputDurationMs = (static_cast<std::uint64_t>(result.outputFrames) * 1000u) / result.outputSampleRate;

    const auto preLevels = measureLevels(*prepared);
    auto processed = processCommsPcm(*prepared);
    const auto postLevels = measureLevels(processed);
    result.commsPrePeak = preLevels.peak;
    result.commsPreRms = preLevels.rms;
    result.commsPostPeak = postLevels.peak;
    result.commsPostRms = postLevels.rms;
    result.pcm = std::move(processed);
    result.prepared = true;
    return result;
}

std::string sds::formatRemoteVoControllerPlaybackPreparation(
    const RemoteVoControllerPlaybackPreparation& result)
{
    std::ostringstream out;
    out << "Voice controller playback prepare:"
        << " prepared=" << (result.prepared ? "yes" : "no")
        << " sourceChannels=" << result.sourceChannels
        << " sourceRate=" << result.sourceSampleRate
        << " sourceFrames=" << result.sourceFrames
        << " outputRate=" << result.outputSampleRate
        << " outputFrames=" << result.outputFrames
        << " outputDurationMs=" << result.outputDurationMs
        << " commsPrePeak=" << result.commsPrePeak
        << " commsPreRms=" << result.commsPreRms
        << " commsPostPeak=" << result.commsPostPeak
        << " commsPostRms=" << result.commsPostRms;
    if (!result.error.empty()) {
        out << " error=\"" << result.error << '"';
    }
    return out.str();
}
