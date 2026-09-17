#include <StarfieldDualSense/WwisePcmWemDecode.h>

#include <StarfieldDualSense/SpeakerPcm.h>
#include <StarfieldDualSense/WwiseWemPayloadProbe.h>
#include <StarfieldDualSense/WwiseWemStructureProbe.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace
{
    [[nodiscard]] std::uint32_t readU32Le(const unsigned char* bytes) noexcept
    {
        return static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8u) |
            (static_cast<std::uint32_t>(bytes[2]) << 16u) |
            (static_cast<std::uint32_t>(bytes[3]) << 24u);
    }

    [[nodiscard]] std::int16_t readI16Le(const unsigned char* bytes) noexcept
    {
        const auto raw = static_cast<std::uint16_t>(
            static_cast<std::uint16_t>(bytes[0]) |
            static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[1]) << 8u));
        return std::bit_cast<std::int16_t>(raw);
    }

    [[nodiscard]] bool hasFourCc(
        std::span<const unsigned char> payload,
        std::size_t offset,
        const std::array<unsigned char, 4>& expected) noexcept
    {
        return payload.size() >= offset + expected.size() &&
            std::equal(expected.begin(), expected.end(), payload.begin() + static_cast<std::ptrdiff_t>(offset));
    }

    [[nodiscard]] sds::VoiceWemPayloadProbeResult makeValidatedPayloadProbe(
        std::span<const unsigned char> payload)
    {
        sds::VoiceWemPayloadProbeResult probe{};
        probe.attempted = true;
        probe.openSucceeded = true;
        probe.readSucceeded = !payload.empty();
        probe.bytesRead = payload.size();
        probe.payload.assign(payload.begin(), payload.end());

        constexpr std::array<unsigned char, 4> riff{ 'R', 'I', 'F', 'F' };
        constexpr std::array<unsigned char, 4> wave{ 'W', 'A', 'V', 'E' };
        if (payload.size() >= 12u && hasFourCc(payload, 0u, riff) && hasFourCc(payload, 8u, wave)) {
            probe.riffWave = true;
            probe.riffDeclaredSize = readU32Le(payload.data() + 4u);
            probe.riffTotalBytes = static_cast<std::uint64_t>(probe.riffDeclaredSize) + 8u;
            probe.riffSizeMatchesPayload = probe.riffTotalBytes == payload.size();
        }
        return probe;
    }
}

sds::WwisePcmWemDecodeResult sds::decodeWwisePcmWemToSpeakerPcm(
    std::span<const unsigned char> payload,
    float gain)
{
    WwisePcmWemDecodeResult result{};
    result.attempted = true;

    const auto payloadProbe = makeValidatedPayloadProbe(payload);
    const auto structure = probeVoiceWemStructure(payloadProbe, 0u, 64u, 6u);
    if (!structure.validRiffWave || !structure.scanComplete || !structure.error.empty()) {
        result.error = structure.error.empty() ? "invalid RIFF/WAVE payload" : structure.error;
        return result;
    }
    if (!structure.fmtFound || !structure.dataFound) {
        result.error = "required fmt/data chunk missing";
        return result;
    }
    if (structure.formatTag != 0xFFFEu) {
        result.error = "unsupported Wwise PCM formatTag; expected 0xFFFE";
        return result;
    }
    if (structure.bitsPerSample != 16u) {
        result.error = "only observed 16-bit Wwise PCM is supported";
        return result;
    }
    if (structure.channels != 1u && structure.channels != 2u) {
        result.error = "unsupported channel count";
        return result;
    }
    if (structure.sampleRate != 44100u && structure.sampleRate != 48000u) {
        result.error = "unsupported sample rate; expected 44100 or 48000";
        return result;
    }
    if (structure.fmtChunkSize != 24u || structure.fmtExtraDeclaredSize != 6u ||
        structure.fmtExtraAvailableSize != 6u || structure.fmtExtraPrefix.size() != 6u) {
        result.error = "unsupported fmt extra layout";
        return result;
    }

    constexpr std::array<unsigned char, 6> monoExtra{ 0x00, 0x00, 0x01, 0x41, 0x00, 0x00 };
    constexpr std::array<unsigned char, 6> stereoExtra{ 0x00, 0x00, 0x02, 0x31, 0x00, 0x00 };
    const auto& expectedExtra = structure.channels == 1u ? monoExtra : stereoExtra;
    if (!std::equal(structure.fmtExtraPrefix.begin(), structure.fmtExtraPrefix.end(), expectedExtra.begin())) {
        result.error = "unsupported fmt extra signature";
        return result;
    }

    const auto expectedBlockAlign = static_cast<std::uint16_t>(structure.channels * 2u);
    if (structure.blockAlign != expectedBlockAlign) {
        result.error = "invalid PCM block alignment";
        return result;
    }
    if (structure.averageBytesPerSecond != structure.sampleRate * expectedBlockAlign) {
        result.error = "invalid PCM byte rate";
        return result;
    }
    if (structure.vorbFound) {
        result.error = "Vorbis WEM is not accepted by the PCM reader";
        return result;
    }
    if (structure.dataSize == 0u || (structure.dataSize % expectedBlockAlign) != 0u) {
        result.error = "PCM data size is not frame-aligned";
        return result;
    }
    if (structure.dataOffset > payload.size() ||
        static_cast<std::uint64_t>(structure.dataSize) > payload.size() - structure.dataOffset) {
        result.error = "PCM data chunk exceeds payload";
        return result;
    }

    const auto scalarSamples = static_cast<std::size_t>(structure.dataSize / 2u);
    std::vector<std::int16_t> samples;
    samples.resize(scalarSamples);
    const auto* sampleBytes = payload.data() + static_cast<std::size_t>(structure.dataOffset);
    for (std::size_t index = 0; index < scalarSamples; ++index) {
        samples[index] = readI16Le(sampleBytes + index * 2u);
    }

    auto prepared = prepareSpeakerPcm(
        samples.data(),
        samples.size(),
        SpeakerSampleFormat::Pcm16,
        structure.sampleRate,
        structure.channels,
        gain);
    if (!prepared) {
        result.error = "speaker PCM preparation rejected decoded samples";
        return result;
    }

    result.recognized = true;
    result.prepared = true;
    result.channels = structure.channels;
    result.sampleRate = structure.sampleRate;
    result.sourceFrames = samples.size() / structure.channels;
    result.pcm = std::move(*prepared);
    return result;
}
