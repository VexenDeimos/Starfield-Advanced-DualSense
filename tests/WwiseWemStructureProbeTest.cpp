#include <StarfieldDualSense/WwiseWemStructureProbe.h>

#include <cassert>
#include <cstdint>
#include <string>
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

    std::vector<unsigned char> makeSyntheticWem()
    {
        std::vector<unsigned char> bytes;
        fourcc(bytes, "RIFF");
        put32(bytes, 0);
        fourcc(bytes, "WAVE");

        std::vector<unsigned char> fmt;
        put16(fmt, 0xFFFFu);
        put16(fmt, 1u);
        put32(fmt, 44100u);
        put32(fmt, 7467u);
        put16(fmt, 0u);
        put16(fmt, 0u);
        put16(fmt, 4u);
        fmt.insert(fmt.end(), { 0xAA, 0xBB, 0xCC, 0xDD });
        addChunk(bytes, "fmt ", fmt);

        addChunk(bytes, "vorb", { 1, 2, 3 });
        addChunk(bytes, "data", { 0x10, 0x11, 0x12, 0x13 });

        const auto riffSize = static_cast<std::uint32_t>(bytes.size() - 8u);
        bytes[4] = static_cast<unsigned char>(riffSize & 0xFFu);
        bytes[5] = static_cast<unsigned char>((riffSize >> 8u) & 0xFFu);
        bytes[6] = static_cast<unsigned char>((riffSize >> 16u) & 0xFFu);
        bytes[7] = static_cast<unsigned char>((riffSize >> 24u) & 0xFFu);
        return bytes;
    }


    std::vector<unsigned char> makeSyntheticObservedWwisePcmWem()
    {
        std::vector<unsigned char> bytes;
        fourcc(bytes, "RIFF");
        put32(bytes, 0);
        fourcc(bytes, "WAVE");

        std::vector<unsigned char> fmt;
        put16(fmt, 0xFFFEu);
        put16(fmt, 2u);
        put32(fmt, 48000u);
        put32(fmt, 192000u);
        put16(fmt, 4u);
        put16(fmt, 16u);
        put16(fmt, 6u);
        fmt.insert(fmt.end(), { 0x00, 0x00, 0x02, 0x31, 0x00, 0x00 });
        addChunk(bytes, "fmt ", fmt);
        addChunk(bytes, "JUNK", { 0x00, 0x00, 0x00, 0x00 });
        addChunk(bytes, "data", { 0x00, 0x00, 0x00, 0x00 });

        const auto riffSize = static_cast<std::uint32_t>(bytes.size() - 8u);
        bytes[4] = static_cast<unsigned char>(riffSize & 0xFFu);
        bytes[5] = static_cast<unsigned char>((riffSize >> 8u) & 0xFFu);
        bytes[6] = static_cast<unsigned char>((riffSize >> 16u) & 0xFFu);
        bytes[7] = static_cast<unsigned char>((riffSize >> 24u) & 0xFFu);
        return bytes;
    }

    std::vector<unsigned char> makeSyntheticWemWithUnpaddedOddFinalData()
    {
        std::vector<unsigned char> bytes;
        fourcc(bytes, "RIFF");
        put32(bytes, 0);
        fourcc(bytes, "WAVE");

        std::vector<unsigned char> fmt;
        put16(fmt, 0xFFFFu);
        put16(fmt, 1u);
        put32(fmt, 44100u);
        put32(fmt, 7467u);
        put16(fmt, 0u);
        put16(fmt, 0u);
        addChunk(bytes, "fmt ", fmt);

        fourcc(bytes, "data");
        put32(bytes, 5u);
        bytes.insert(bytes.end(), { 0x10, 0x11, 0x12, 0x13, 0x14 });

        const auto riffSize = static_cast<std::uint32_t>(bytes.size() - 8u);
        bytes[4] = static_cast<unsigned char>(riffSize & 0xFFu);
        bytes[5] = static_cast<unsigned char>((riffSize >> 8u) & 0xFFu);
        bytes[6] = static_cast<unsigned char>((riffSize >> 16u) & 0xFFu);
        bytes[7] = static_cast<unsigned char>((riffSize >> 24u) & 0xFFu);
        return bytes;
    }

}

int main()
{
    sds::VoiceWemPayloadProbeResult payload{};
    payload.attempted = true;
    payload.openSucceeded = true;
    payload.readSucceeded = true;
    payload.riffWave = true;
    payload.riffSizeMatchesPayload = true;
    payload.payload = makeSyntheticWem();
    payload.expectedSize = static_cast<std::uint32_t>(payload.payload.size());
    payload.bytesRead = payload.payload.size();
    payload.riffDeclaredSize = static_cast<std::uint32_t>(payload.payload.size() - 8u);
    payload.riffTotalBytes = payload.payload.size();

    const auto neutral = sds::inspectWemStructure(payload.payload);
    assert(neutral.validRiffWave);
    assert(neutral.scanComplete);
    assert(neutral.codecLabel == "WwiseVorbis");
    assert(neutral.channels == 1u);
    assert(neutral.sampleRate == 44100u);

    const auto result = sds::probeVoiceWemStructure(payload, 4u, 16u, 16u);
    assert(result.attempted);
    assert(result.validRiffWave);
    assert(result.scanComplete);
    assert(result.sourceCodecId == 4u);
    assert(result.codecHint == "WwiseVorbis");
    assert(result.chunks.size() == 3u);
    assert(result.chunks[0].id == "fmt ");
    assert(result.chunks[1].id == "vorb");
    assert(result.chunks[1].padded);
    assert(result.chunks[2].id == "data");
    assert(result.fmtFound);
    assert(result.fmtChunkSize == 22u);
    assert(result.formatTag == 0xFFFFu);
    assert(result.channels == 1u);
    assert(result.sampleRate == 44100u);
    assert(result.averageBytesPerSecond == 7467u);
    assert(result.blockAlign == 0u);
    assert(result.bitsPerSample == 0u);
    assert(result.fmtExtraDeclaredSize == 4u);
    assert(result.fmtExtraAvailableSize == 4u);
    assert((result.fmtExtraPrefix == std::vector<unsigned char>{ 0xAA, 0xBB, 0xCC, 0xDD }));
    assert(result.vorbFound);
    assert(result.vorbSize == 3u);
    assert(result.dataFound);
    assert(result.dataSize == 4u);
    assert(result.error.empty());

    const auto summary = sds::formatVoiceWemStructureProbeSummary(result);
    assert(summary.find("Voice WEM structure probe:") != std::string::npos);
    assert(summary.find("codecId=4") != std::string::npos);
    assert(summary.find("codecHint=WwiseVorbis") != std::string::npos);
    assert(summary.find("formatTag=0xFFFF") != std::string::npos);
    assert(summary.find("channels=1") != std::string::npos);
    assert(summary.find("sampleRate=44100") != std::string::npos);
    assert(summary.find("fmtChunkSize=22") != std::string::npos);
    assert(summary.find("fmtExtraDeclared=4") != std::string::npos);
    assert(summary.find("fmtExtraPrefix=AA BB CC DD") != std::string::npos);
    assert(summary.find("vorb=yes") != std::string::npos);
    assert(summary.find("dataSize=4") != std::string::npos);
    assert(summary.find("chunks=3") != std::string::npos);

    const auto chunkLine = sds::formatVoiceWemStructureProbeChunk(result.chunks[1], 1u);
    assert(chunkLine.find("chunk[1]") != std::string::npos);
    assert(chunkLine.find("id=\"vorb\"") != std::string::npos);
    assert(chunkLine.find("size=3") != std::string::npos);
    assert(chunkLine.find("padded=yes") != std::string::npos);


    const auto observedPcm = sds::inspectWemStructure(makeSyntheticObservedWwisePcmWem());
    assert(observedPcm.validRiffWave);
    assert(observedPcm.scanComplete);
    assert(observedPcm.formatTag == 0xFFFEu);
    assert(observedPcm.channels == 2u);
    assert(observedPcm.sampleRate == 48000u);
    assert(observedPcm.blockAlign == 4u);
    assert(observedPcm.bitsPerSample == 16u);
    assert(observedPcm.dataFound);
    assert(observedPcm.codecLabel == "WwisePCM16");

    auto oddFinalPayload = payload;
    oddFinalPayload.payload = makeSyntheticWemWithUnpaddedOddFinalData();
    oddFinalPayload.expectedSize = static_cast<std::uint32_t>(oddFinalPayload.payload.size());
    oddFinalPayload.bytesRead = oddFinalPayload.payload.size();
    oddFinalPayload.riffDeclaredSize = static_cast<std::uint32_t>(oddFinalPayload.payload.size() - 8u);
    oddFinalPayload.riffTotalBytes = oddFinalPayload.payload.size();
    const auto oddFinal = sds::probeVoiceWemStructure(oddFinalPayload, 4u, 16u, 16u);
    assert(oddFinal.attempted);
    assert(oddFinal.validRiffWave);
    assert(oddFinal.scanComplete);
    assert(oddFinal.chunks.size() == 2u);
    assert(oddFinal.chunks[1].id == "data");
    assert(oddFinal.chunks[1].size == 5u);
    assert(oddFinal.chunks[1].padded);
    assert(oddFinal.dataFound);
    assert(oddFinal.dataSize == 5u);
    assert(oddFinal.error.empty());

    auto malformed = payload;
    malformed.payload.resize(20u);
    malformed.payload[4] = 12u;
    malformed.payload[5] = 0u;
    malformed.payload[6] = 0u;
    malformed.payload[7] = 0u;
    malformed.payload[16] = 0xFF;
    malformed.payload[17] = 0xFF;
    malformed.payload[18] = 0xFF;
    malformed.payload[19] = 0x7F;
    malformed.riffDeclaredSize = 12u;
    malformed.riffTotalBytes = 20u;
    malformed.riffSizeMatchesPayload = true;
    const auto bad = sds::probeVoiceWemStructure(malformed, 4u, 16u, 16u);
    assert(bad.attempted);
    assert(bad.validRiffWave);
    assert(!bad.scanComplete);
    assert(bad.error == "chunk exceeds RIFF bounds");

    auto unread = payload;
    unread.readSucceeded = false;
    const auto noPayload = sds::probeVoiceWemStructure(unread, 4u, 16u, 16u);
    assert(noPayload.attempted);
    assert(!noPayload.validRiffWave);
    assert(noPayload.error == "validated WEM payload unavailable");

    return 0;
}
