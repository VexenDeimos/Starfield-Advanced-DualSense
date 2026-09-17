#include <StarfieldDualSense/WwisePcmWemDecode.h>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

namespace
{
    void put16(std::vector<unsigned char>& bytes, std::uint16_t value)
    {
        bytes.push_back(static_cast<unsigned char>(value & 0xFFu));
        bytes.push_back(static_cast<unsigned char>((value >> 8u) & 0xFFu));
    }

    void put32(std::vector<unsigned char>& bytes, std::uint32_t value)
    {
        bytes.push_back(static_cast<unsigned char>(value & 0xFFu));
        bytes.push_back(static_cast<unsigned char>((value >> 8u) & 0xFFu));
        bytes.push_back(static_cast<unsigned char>((value >> 16u) & 0xFFu));
        bytes.push_back(static_cast<unsigned char>((value >> 24u) & 0xFFu));
    }

    void fourcc(std::vector<unsigned char>& bytes, const char text[5])
    {
        bytes.insert(bytes.end(), text, text + 4);
    }

    void addChunk(std::vector<unsigned char>& bytes, const char id[5], const std::vector<unsigned char>& payload)
    {
        fourcc(bytes, id);
        put32(bytes, static_cast<std::uint32_t>(payload.size()));
        bytes.insert(bytes.end(), payload.begin(), payload.end());
        if ((payload.size() & 1u) != 0u) {
            bytes.push_back(0);
        }
    }

    std::vector<unsigned char> makeObservedWwisePcm(
        std::uint16_t channels,
        std::uint32_t sampleRate,
        const std::vector<std::int16_t>& samples,
        std::uint16_t formatTag = 0xFFFEu,
        std::uint16_t bitsPerSample = 16u,
        bool corruptExtra = false,
        std::uint32_t averageBytesPerSecondOverride = 0u)
    {
        std::vector<unsigned char> bytes;
        fourcc(bytes, "RIFF");
        put32(bytes, 0u);
        fourcc(bytes, "WAVE");

        const auto blockAlign = static_cast<std::uint16_t>(channels * 2u);
        const auto averageBytesPerSecond = averageBytesPerSecondOverride != 0u
            ? averageBytesPerSecondOverride
            : sampleRate * blockAlign;

        std::vector<unsigned char> fmt;
        put16(fmt, formatTag);
        put16(fmt, channels);
        put32(fmt, sampleRate);
        put32(fmt, averageBytesPerSecond);
        put16(fmt, blockAlign);
        put16(fmt, bitsPerSample);
        put16(fmt, 6u);
        if (channels == 1u) {
            fmt.insert(fmt.end(), { 0x00, 0x00, 0x01, 0x41, 0x00, 0x00 });
        } else {
            fmt.insert(fmt.end(), { 0x00, 0x00, 0x02, 0x31, 0x00, 0x00 });
        }
        if (corruptExtra) {
            fmt.back() = 0x7Fu;
        }
        addChunk(bytes, "fmt ", fmt);
        addChunk(bytes, "JUNK", { 0x00, 0x00, 0x00, 0x00 });

        std::vector<unsigned char> data;
        data.reserve(samples.size() * 2u);
        for (const auto sample : samples) {
            put16(data, static_cast<std::uint16_t>(sample));
        }
        addChunk(bytes, "data", data);

        const auto riffSize = static_cast<std::uint32_t>(bytes.size() - 8u);
        bytes[4] = static_cast<unsigned char>(riffSize & 0xFFu);
        bytes[5] = static_cast<unsigned char>((riffSize >> 8u) & 0xFFu);
        bytes[6] = static_cast<unsigned char>((riffSize >> 16u) & 0xFFu);
        bytes[7] = static_cast<unsigned char>((riffSize >> 24u) & 0xFFu);
        return bytes;
    }

    bool close(float lhs, float rhs, float tolerance = 0.0001F)
    {
        return std::fabs(lhs - rhs) <= tolerance;
    }
}

int main()
{
    const auto stereo = makeObservedWwisePcm(
        2u,
        48000u,
        { -32768, 32767, 0, 16384 });
    const auto decodedStereo = sds::decodeWwisePcmWemToSpeakerPcm(stereo);
    assert(decodedStereo.attempted);
    assert(decodedStereo.recognized);
    assert(decodedStereo.prepared);
    assert(decodedStereo.channels == 2u);
    assert(decodedStereo.sampleRate == 48000u);
    assert(decodedStereo.sourceFrames == 2u);
    assert(decodedStereo.pcm.frames.size() == 2u);
    assert(close(decodedStereo.pcm.frames[0].left, -1.0F));
    assert(close(decodedStereo.pcm.frames[0].right, 1.0F));
    assert(close(decodedStereo.pcm.frames[1].left, 0.0F));
    assert(decodedStereo.pcm.frames[1].right > 0.49F && decodedStereo.pcm.frames[1].right < 0.51F);
    assert(decodedStereo.error.empty());

    std::vector<std::int16_t> monoSamples(441u, 0);
    monoSamples.front() = 32767;
    const auto mono = makeObservedWwisePcm(1u, 44100u, monoSamples);
    const auto decodedMono = sds::decodeWwisePcmWemToSpeakerPcm(mono, 0.5F);
    assert(decodedMono.recognized);
    assert(decodedMono.prepared);
    assert(decodedMono.channels == 1u);
    assert(decodedMono.sampleRate == 44100u);
    assert(decodedMono.sourceFrames == 441u);
    assert(decodedMono.pcm.frames.size() == 480u);
    assert(close(decodedMono.pcm.frames.front().left, 1.0F));
    assert(close(decodedMono.pcm.frames.front().right, 1.0F));
    assert(close(decodedMono.pcm.gain, 0.5F));

    const auto ordinaryPcm = makeObservedWwisePcm(2u, 48000u, { 0, 0 }, 0x0001u);
    const auto rejectedTag = sds::decodeWwisePcmWemToSpeakerPcm(ordinaryPcm);
    assert(!rejectedTag.recognized);
    assert(!rejectedTag.prepared);
    assert(rejectedTag.error.find("formatTag") != std::string::npos);

    const auto corruptExtra = makeObservedWwisePcm(2u, 48000u, { 0, 0 }, 0xFFFEu, 16u, true);
    const auto rejectedExtra = sds::decodeWwisePcmWemToSpeakerPcm(corruptExtra);
    assert(!rejectedExtra.recognized);
    assert(rejectedExtra.error.find("fmt extra") != std::string::npos);

    const auto badRate = makeObservedWwisePcm(2u, 32000u, { 0, 0 });
    const auto rejectedRate = sds::decodeWwisePcmWemToSpeakerPcm(badRate);
    assert(!rejectedRate.recognized);
    assert(rejectedRate.error.find("sample rate") != std::string::npos);

    const auto badAverage = makeObservedWwisePcm(2u, 48000u, { 0, 0 }, 0xFFFEu, 16u, false, 12345u);
    const auto rejectedAverage = sds::decodeWwisePcmWemToSpeakerPcm(badAverage);
    assert(!rejectedAverage.recognized);
    assert(rejectedAverage.error.find("byte rate") != std::string::npos);

    const auto badBits = makeObservedWwisePcm(2u, 48000u, { 0, 0 }, 0xFFFEu, 24u);
    const auto rejectedBits = sds::decodeWwisePcmWemToSpeakerPcm(badBits);
    assert(!rejectedBits.recognized);
    assert(rejectedBits.error.find("16-bit") != std::string::npos);

    return 0;
}
