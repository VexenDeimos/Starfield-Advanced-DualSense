#include <StarfieldDualSense/RemoteVoSpeakerPlayback.h>

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <vector>

namespace
{
    int failures = 0;

    void expect(bool condition, std::string_view name)
    {
        if (condition) {
            std::cout << "PASS " << name << '\n';
        } else {
            std::cerr << "FAIL " << name << '\n';
            ++failures;
        }
    }
}

int main()
{
    sds::VoiceWwiseVorbisDecodeResult decoded{};
    decoded.attempted = true;
    decoded.reconstructed = true;
    decoded.decodeSucceeded = true;
    decoded.channels = 1;
    decoded.sampleRate = 44100;
    decoded.expectedSamples = 297587;
    decoded.decodedSamples = 297587;
    decoded.durationMs = 6748;
    decoded.pcm.resize(decoded.decodedSamples);
    for (std::size_t i = 0; i < decoded.pcm.size(); ++i) {
        decoded.pcm[i] = static_cast<std::int16_t>((static_cast<int>(i % 200) - 100) * 200);
    }

    const auto prepared = sds::prepareRemoteVoControllerPlayback(decoded);
    expect(prepared.attempted, "controller playback preparation attempts successful decode");
    expect(prepared.prepared, "successful mono 44.1 kHz decode prepares controller PCM");
    expect(prepared.sourceFrames == 297587u, "playback preparation records source frame count");
    expect(prepared.outputFrames == 323904u, "44.1 kHz voice resamples to expected 48 kHz frame count");
    expect(prepared.outputSampleRate == 48000u, "controller playback output is 48 kHz");
    expect(prepared.pcm.frames.size() == prepared.outputFrames, "prepared PCM owns all resampled frames");
    expect(prepared.commsPrePeak > 0.0F && prepared.commsPreRms > 0.0F,
           "remote VO preparation records pre-processing comms levels");
    expect(prepared.commsPostPeak > prepared.commsPrePeak && prepared.commsPostPeak <= 1.0F,
           "remote VO comms processing raises peak while remaining limited");
    expect(prepared.commsPostRms > prepared.commsPreRms,
           "remote VO comms processing raises RMS loudness");
    expect(!prepared.pcm.frames.empty() &&
               std::fabs(prepared.pcm.frames[100].left - prepared.pcm.frames[100].right) < 0.000001F,
           "mono voice remains centered before proven controller channel mapping");

    sds::VoiceWwiseVorbisDecodeResult failed{};
    failed.attempted = true;
    const auto rejected = sds::prepareRemoteVoControllerPlayback(failed);
    expect(rejected.attempted && !rejected.prepared && !rejected.error.empty(),
           "failed decode is rejected before speaker submission");

    const auto summary = sds::formatRemoteVoControllerPlaybackPreparation(prepared);
    expect(summary.find("prepared=yes") != std::string::npos &&
               summary.find("outputRate=48000") != std::string::npos &&
               summary.find("outputFrames=323904") != std::string::npos &&
               summary.find("commsPrePeak=") != std::string::npos &&
               summary.find("commsPostRms=") != std::string::npos,
           "playback preparation summary exposes resample and comms loudness telemetry");

    return failures == 0 ? 0 : 1;
}
