#include <StarfieldDualSense/WeaponSpeakerWemDecode.h>

#include <StarfieldDualSense/SpeakerPcm.h>
#include <StarfieldDualSense/WwisePcmWemDecode.h>
#include <StarfieldDualSense/WwiseWemStructureProbe.h>
#include <StarfieldDualSense/WwiseWemVorbisPacketProbe.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace
{
    std::uint32_t u32(const unsigned char* p) noexcept
    {
        return static_cast<std::uint32_t>(p[0]) |
            (static_cast<std::uint32_t>(p[1]) << 8u) |
            (static_cast<std::uint32_t>(p[2]) << 16u) |
            (static_cast<std::uint32_t>(p[3]) << 24u);
    }

    sds::VoiceWemPayloadProbeResult validatedPayload(std::span<const unsigned char> payload)
    {
        sds::VoiceWemPayloadProbeResult probe{};
        probe.attempted = true;
        probe.openSucceeded = true;
        probe.readSucceeded = !payload.empty();
        probe.bytesRead = payload.size();
        probe.expectedSize = payload.size();
        probe.payload.assign(payload.begin(), payload.end());
        if (payload.size() >= 12u &&
            std::equal(payload.begin(), payload.begin() + 4, "RIFF") &&
            std::equal(payload.begin() + 8, payload.begin() + 12, "WAVE")) {
            probe.riffWave = true;
            probe.riffDeclaredSize = u32(payload.data() + 4u);
            probe.riffTotalBytes = static_cast<std::uint64_t>(probe.riffDeclaredSize) + 8u;
            probe.riffSizeMatchesPayload = probe.riffTotalBytes == payload.size();
        }
        return probe;
    }

    sds::WeaponSpeakerWemDecodeResult prepareVorbisResult(
        const sds::VoiceWwiseVorbisDecodeResult& decoded,
        float gain)
    {
        sds::WeaponSpeakerWemDecodeResult result{};
        result.usedVorbis = true;
        result.channels = decoded.channels;
        result.sampleRate = decoded.sampleRate;
        result.sourceFrames = decoded.decodedSamples;
        if (!decoded.decodeSucceeded) {
            result.error = decoded.error.empty() ? "Wwise Vorbis decode failed" : decoded.error;
            return result;
        }
        const auto prepared = sds::prepareSpeakerPcm(
            decoded.pcm.data(), decoded.pcm.size(), sds::SpeakerSampleFormat::Pcm16,
            decoded.sampleRate, decoded.channels, gain);
        if (!prepared) {
            result.error = "speaker PCM preparation rejected decoded Vorbis";
            return result;
        }
        result.prepared = true;
        result.pcm = *prepared;
        return result;
    }

    template <class Decode>
    sds::WeaponSpeakerWemDecodeResult decodeVorbis(
        std::span<const unsigned char> payload,
        float gain,
        Decode&& decode)
    {
        auto probe = validatedPayload(payload);
        const auto structure = sds::probeVoiceWemStructure(probe, 4u, 64u, 64u);
        if (!structure.validRiffWave || !structure.scanComplete || structure.formatTag != 0xFFFFu) {
            sds::WeaponSpeakerWemDecodeResult result{};
            result.error = structure.error.empty() ? "unsupported weapon WEM codec" : structure.error;
            return result;
        }
        const auto packets = sds::probeWwiseVorbisPackets(probe, structure, 100000u, 16u);
        if (!packets.probeComplete || packets.audioScanTruncated) {
            sds::WeaponSpeakerWemDecodeResult result{};
            result.usedVorbis = true;
            result.error = packets.error.empty() ? "Wwise Vorbis packet scan incomplete" : packets.error;
            return result;
        }
        return prepareVorbisResult(decode(probe, structure, packets), gain);
    }
}

sds::WeaponSpeakerWemDecodeResult sds::decodeWeaponSpeakerWemToSpeakerPcmWithVorbisBackend(
    std::span<const unsigned char> payload,
    float gain,
    std::span<const unsigned char> packedCodebookLibrary,
    OggVorbisDecodeBackend backend)
{
    const auto pcm = decodeWwisePcmWemToSpeakerPcm(payload, gain);
    if (pcm.prepared) {
        WeaponSpeakerWemDecodeResult result{};
        result.prepared = true;
        result.usedPcm = true;
        result.channels = pcm.channels;
        result.sampleRate = pcm.sampleRate;
        result.sourceFrames = pcm.sourceFrames;
        result.pcm = pcm.pcm;
        return result;
    }
    return decodeVorbis(payload, gain, [&](const auto& probe, const auto& structure, const auto& packets) {
        return decodeWwiseVorbisToPcmWithBackend(
            probe, structure, packets, packedCodebookLibrary, backend, "weapon-speaker-test");
    });
}

sds::WeaponSpeakerWemDecodeResult sds::decodeWeaponSpeakerWemToSpeakerPcm(
    std::span<const unsigned char> payload,
    float gain)
{
    const auto pcm = decodeWwisePcmWemToSpeakerPcm(payload, gain);
    if (pcm.prepared) {
        WeaponSpeakerWemDecodeResult result{};
        result.prepared = true;
        result.usedPcm = true;
        result.channels = pcm.channels;
        result.sampleRate = pcm.sampleRate;
        result.sourceFrames = pcm.sourceFrames;
        result.pcm = pcm.pcm;
        return result;
    }
    return decodeVorbis(payload, gain, [&](const auto& probe, const auto& structure, const auto& packets) {
        return decodeWwiseVorbisToPcm(probe, structure, packets);
    });
}
