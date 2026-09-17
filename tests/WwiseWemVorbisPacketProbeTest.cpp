#include <StarfieldDualSense/WwiseWemStructureProbe.h>
#include <StarfieldDualSense/WwiseWemVorbisPacketProbe.h>

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

    void patch32(std::vector<unsigned char>& bytes, std::size_t offset, std::uint32_t value)
    {
        bytes[offset + 0u] = static_cast<unsigned char>(value & 0xFFu);
        bytes[offset + 1u] = static_cast<unsigned char>((value >> 8u) & 0xFFu);
        bytes[offset + 2u] = static_cast<unsigned char>((value >> 16u) & 0xFFu);
        bytes[offset + 3u] = static_cast<unsigned char>((value >> 24u) & 0xFFu);
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

    std::vector<unsigned char> makeNewFmt30Wem()
    {
        std::vector<unsigned char> bytes;
        fourcc(bytes, "RIFF");
        put32(bytes, 0u);
        fourcc(bytes, "WAVE");

        std::vector<unsigned char> fmt;
        put16(fmt, 0xFFFFu);          // format tag
        put16(fmt, 1u);               // channels
        put32(fmt, 44100u);           // sample rate
        put32(fmt, 7606u);            // average bytes/sec
        put16(fmt, 0u);               // block align
        put16(fmt, 0u);               // bits/sample
        put16(fmt, 0x30u);            // cbSize
        put16(fmt, 0u);               // +0x12 flag
        put32(fmt, 0x00004101u);       // +0x14 channel config
        put32(fmt, 183927u);           // +0x18 num samples
        put32(fmt, 6u);                // +0x1c data start-ish field
        put32(fmt, 22u);               // +0x20 data end-ish field
        put16(fmt, 0x49u);             // +0x24 unknown small 1
        put16(fmt, 0x28u);             // +0x26 unknown small 2
        put32(fmt, 4u);                // +0x28 setup offset within data
        put32(fmt, 10u);               // +0x2c audio offset within data
        put16(fmt, 5u);                // +0x30 biggest packet
        put16(fmt, 0u);                // +0x32 last granule extra
        put32(fmt, 0x1234u);           // +0x34 decode alloc
        put32(fmt, 0x5678u);           // +0x38 decode x64 alloc
        put32(fmt, 0x35A2AA01u);       // +0x3c codebook/hash field
        fmt.push_back(8u);             // +0x40 small block exponent
        fmt.push_back(11u);            // +0x41 large block exponent
        assert(fmt.size() == 0x42u);
        addChunk(bytes, "fmt ", fmt);

        std::vector<unsigned char> data = {
            0xDE, 0xAD, 0xBE, 0xEF,             // seek/leading bytes
            0x04, 0x00, 0xAA, 0xBB, 0xCC, 0xDD, // setup packet (size 4)
            0x03, 0x00, 0x10, 0x11, 0x12,       // audio packet 0
            0x02, 0x00, 0x20, 0x21,             // audio packet 1
            0x01, 0x00, 0x30                    // audio packet 2
        };
        assert(data.size() == 22u);
        addChunk(bytes, "data", data);

        patch32(bytes, 4u, static_cast<std::uint32_t>(bytes.size() - 8u));
        return bytes;
    }

    sds::VoiceWemPayloadProbeResult makePayload()
    {
        sds::VoiceWemPayloadProbeResult payload{};
        payload.attempted = true;
        payload.openSucceeded = true;
        payload.readSucceeded = true;
        payload.riffWave = true;
        payload.riffSizeMatchesPayload = true;
        payload.payload = makeNewFmt30Wem();
        payload.expectedSize = static_cast<std::uint32_t>(payload.payload.size());
        payload.bytesRead = payload.payload.size();
        payload.riffDeclaredSize = static_cast<std::uint32_t>(payload.payload.size() - 8u);
        payload.riffTotalBytes = payload.payload.size();
        return payload;
    }
}

int main()
{
    auto payload = makePayload();
    const auto structure = sds::probeVoiceWemStructure(payload, 4u, 16u, 64u);
    assert(structure.scanComplete);
    assert(structure.fmtChunkSize == 0x42u);
    assert(structure.fmtExtraDeclaredSize == 0x30u);
    assert(!structure.vorbFound);

    const auto result = sds::probeWwiseVorbisPackets(payload, structure, 3u, 4u);
    assert(result.attempted);
    assert(result.recognizedNewFmt30);
    assert(result.probeComplete);
    assert(result.variant == "new-0x30");
    assert(result.packetHeaderBytes == 2u);
    assert(result.modifiedPackets);
    assert(result.packetMode == "modified");
    assert(result.channels == 1u);
    assert(result.sampleRate == 44100u);
    assert(result.numSamples == 183927u);
    assert(result.durationMs == 4171u);
    assert(result.channelConfig == 0x00004101u);
    assert(result.setupOffset == 4u);
    assert(result.audioOffset == 10u);
    assert(result.metadataHashField == 0x35A2AA01u);
    assert(result.blockExpSmall == 8u);
    assert(result.blockExpLarge == 11u);
    assert(result.setupPacket.valid);
    assert(result.setupPacket.relativeHeaderOffset == 4u);
    assert(result.setupPacket.packetSize == 4u);
    assert(result.setupPacket.nextRelativeOffset == 10u);
    assert(result.setupEndsAtAudio);
    assert((result.setupPacket.prefix == std::vector<unsigned char>{ 0xAA, 0xBB, 0xCC, 0xDD }));
    assert(result.audioPackets.size() == 3u);
    assert(result.audioPackets[0].packetSize == 3u);
    assert(result.audioPackets[0].relativeHeaderOffset == 10u);
    assert(result.audioPackets[0].nextRelativeOffset == 15u);
    assert((result.audioPackets[0].prefix == std::vector<unsigned char>{ 0x10, 0x11, 0x12 }));
    assert(result.audioPackets[1].packetSize == 2u);
    assert(result.audioPackets[1].nextRelativeOffset == 19u);
    assert(result.audioPackets[2].packetSize == 1u);
    assert(result.audioPackets[2].nextRelativeOffset == 22u);
    assert(result.error.empty());

    const auto summary = sds::formatWwiseVorbisPacketProbeSummary(result);
    assert(summary.find("Voice Wwise Vorbis probe:") != std::string::npos);
    assert(summary.find("variant=new-0x30") != std::string::npos);
    assert(summary.find("samples=183927") != std::string::npos);
    assert(summary.find("durationMs=4171") != std::string::npos);
    assert(summary.find("setupOffset=4") != std::string::npos);
    assert(summary.find("audioOffset=10") != std::string::npos);
    assert(summary.find("blockExpSmall=8") != std::string::npos);
    assert(summary.find("blockExpLarge=11") != std::string::npos);
    assert(summary.find("metadataHash=0x35A2AA01") != std::string::npos);
    assert(summary.find("packetHeaderBytes=2") != std::string::npos);
    assert(summary.find("packetMode=modified") != std::string::npos);
    assert(summary.find("setupEndsAtAudio=yes") != std::string::npos);
    assert(summary.find("audioPackets=3") != std::string::npos);

    const auto setupLine = sds::formatWwiseVorbisSetupPacketProbe(result);
    assert(setupLine.find("setup") != std::string::npos);
    assert(setupLine.find("packetSize=4") != std::string::npos);
    assert(setupLine.find("prefix=AA BB CC DD") != std::string::npos);

    const auto audioLine = sds::formatWwiseVorbisAudioPacketProbe(result.audioPackets[0], 0u);
    assert(audioLine.find("audio[0]") != std::string::npos);
    assert(audioLine.find("packetSize=3") != std::string::npos);
    assert(audioLine.find("prefix=10 11 12") != std::string::npos);

    auto equalBlocks = payload;
    // fmt payload starts at byte 20; block exponents live at fmt + 0x40/+0x41.
    equalBlocks.payload[20u + 0x40u] = 9u;
    equalBlocks.payload[20u + 0x41u] = 9u;
    const auto equalStructure = sds::probeVoiceWemStructure(equalBlocks, 4u, 16u, 64u);
    const auto standard = sds::probeWwiseVorbisPackets(equalBlocks, equalStructure, 1u, 4u);
    assert(standard.probeComplete);
    assert(!standard.modifiedPackets);
    assert(standard.packetMode == "standard");

    auto badPayload = payload;
    // audio offset at fmt + 0x2c; point it beyond the data chunk.
    patch32(badPayload.payload, 20u + 0x2Cu, 0x1000u);
    const auto badStructure = sds::probeVoiceWemStructure(badPayload, 4u, 16u, 64u);
    const auto bad = sds::probeWwiseVorbisPackets(badPayload, badStructure, 3u, 4u);
    assert(bad.attempted);
    assert(bad.recognizedNewFmt30);
    assert(!bad.probeComplete);
    assert(bad.error == "audio offset outside data");

    return 0;
}
