#include <StarfieldDualSense/WwiseWemVorbisPacketProbe.h>

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <utility>

namespace
{
    [[nodiscard]] std::uint16_t readU16Le(const unsigned char* bytes) noexcept
    {
        return static_cast<std::uint16_t>(bytes[0]) |
            static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[1]) << 8u);
    }

    [[nodiscard]] std::uint32_t readU32Le(const unsigned char* bytes) noexcept
    {
        return static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8u) |
            (static_cast<std::uint32_t>(bytes[2]) << 16u) |
            (static_cast<std::uint32_t>(bytes[3]) << 24u);
    }

    [[nodiscard]] std::string formatHexBytes(const std::vector<unsigned char>& bytes)
    {
        std::ostringstream out;
        for (std::size_t i = 0; i < bytes.size(); ++i) {
            if (i != 0u) {
                out << ' ';
            }
            out << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                << static_cast<unsigned>(bytes[i]);
        }
        return out.str();
    }

    [[nodiscard]] const sds::WemRiffChunkInfo* findChunk(
        const sds::VoiceWemStructureProbeResult& structure,
        const char* id) noexcept
    {
        const auto it = std::find_if(
            structure.chunks.begin(),
            structure.chunks.end(),
            [id](const sds::WemRiffChunkInfo& chunk) { return chunk.id == id; });
        return it == structure.chunks.end() ? nullptr : &*it;
    }

    [[nodiscard]] bool parsePacket(
        const std::vector<unsigned char>& payload,
        std::uint64_t dataOffset,
        std::uint32_t dataSize,
        std::uint64_t relativeHeaderOffset,
        std::size_t maxPrefixBytes,
        sds::WwiseVorbisPacketInfo& packet,
        std::string& error)
    {
        const std::uint64_t dataSize64 = dataSize;
        if (relativeHeaderOffset > dataSize64 || dataSize64 - relativeHeaderOffset < 2u) {
            error = "packet header exceeds data bounds";
            return false;
        }
        if (dataOffset > std::numeric_limits<std::uint64_t>::max() - relativeHeaderOffset) {
            error = "packet offset overflow";
            return false;
        }

        const std::uint64_t wemHeaderOffset = dataOffset + relativeHeaderOffset;
        if (wemHeaderOffset > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) ||
            static_cast<std::uint64_t>(payload.size()) - wemHeaderOffset < 2u) {
            error = "packet header exceeds WEM bounds";
            return false;
        }

        const auto headerIndex = static_cast<std::size_t>(wemHeaderOffset);
        const auto packetSize = readU16Le(payload.data() + headerIndex);
        if (packetSize == 0u) {
            error = "zero-sized packet";
            return false;
        }

        const std::uint64_t relativePayloadOffset = relativeHeaderOffset + 2u;
        const std::uint64_t nextRelativeOffset = relativePayloadOffset + packetSize;
        if (nextRelativeOffset < relativePayloadOffset || nextRelativeOffset > dataSize64) {
            error = "packet exceeds data bounds";
            return false;
        }
        if (dataOffset > std::numeric_limits<std::uint64_t>::max() - relativePayloadOffset ||
            dataOffset > std::numeric_limits<std::uint64_t>::max() - nextRelativeOffset) {
            error = "packet payload offset overflow";
            return false;
        }

        const std::uint64_t wemPayloadOffset = dataOffset + relativePayloadOffset;
        const std::uint64_t nextWemOffset = dataOffset + nextRelativeOffset;
        if (nextWemOffset > payload.size()) {
            error = "packet exceeds WEM bounds";
            return false;
        }

        packet.valid = true;
        packet.relativeHeaderOffset = relativeHeaderOffset;
        packet.wemHeaderOffset = wemHeaderOffset;
        packet.packetSize = packetSize;
        packet.relativePayloadOffset = relativePayloadOffset;
        packet.wemPayloadOffset = wemPayloadOffset;
        packet.nextRelativeOffset = nextRelativeOffset;
        packet.nextWemOffset = nextWemOffset;

        const auto prefixSize = (std::min)(static_cast<std::size_t>(packetSize), maxPrefixBytes);
        const auto prefixBegin = static_cast<std::size_t>(wemPayloadOffset);
        packet.prefix.assign(payload.begin() + static_cast<std::ptrdiff_t>(prefixBegin),
            payload.begin() + static_cast<std::ptrdiff_t>(prefixBegin + prefixSize));
        return true;
    }

    [[nodiscard]] std::string formatPacketDetails(const sds::WwiseVorbisPacketInfo& packet)
    {
        std::ostringstream out;
        out << " relativeHeaderOffset=" << packet.relativeHeaderOffset
            << " wemHeaderOffset=" << packet.wemHeaderOffset
            << " packetSize=" << packet.packetSize
            << " relativePayloadOffset=" << packet.relativePayloadOffset
            << " wemPayloadOffset=" << packet.wemPayloadOffset
            << " nextRelativeOffset=" << packet.nextRelativeOffset
            << " nextWemOffset=" << packet.nextWemOffset
            << " prefix=" << formatHexBytes(packet.prefix);
        return out.str();
    }
}

sds::VoiceWwiseVorbisPacketProbeResult sds::probeWwiseVorbisPackets(
    const VoiceWemPayloadProbeResult& payloadResult,
    const VoiceWemStructureProbeResult& structureResult,
    std::size_t maxAudioPackets,
    std::size_t maxPacketPrefixBytes)
{
    VoiceWwiseVorbisPacketProbeResult result{};
    result.attempted = true;

    if (!payloadResult.readSucceeded || payloadResult.payload.empty() ||
        !structureResult.scanComplete || !structureResult.validRiffWave ||
        !structureResult.fmtFound || !structureResult.dataFound) {
        result.error = "validated WEM structure unavailable";
        return result;
    }
    if (maxAudioPackets == 0u) {
        result.error = "maxAudioPackets=0";
        return result;
    }

    const auto* fmtChunk = findChunk(structureResult, "fmt ");
    const auto* dataChunk = findChunk(structureResult, "data");
    if (!fmtChunk || !dataChunk) {
        result.error = "fmt/data chunk metadata unavailable";
        return result;
    }

    const bool recognized = structureResult.sourceCodecId == 4u &&
        structureResult.formatTag == 0xFFFFu &&
        structureResult.fmtChunkSize == 0x42u &&
        structureResult.fmtExtraDeclaredSize == 0x30u &&
        !structureResult.vorbFound;
    if (!recognized) {
        result.error = "unsupported Wwise Vorbis layout";
        return result;
    }

    result.recognizedNewFmt30 = true;
    result.variant = "new-0x30";
    result.packetHeaderBytes = 2u;
    result.channels = structureResult.channels;
    result.sampleRate = structureResult.sampleRate;
    result.dataOffset = dataChunk->payloadOffset;
    result.dataSize = dataChunk->size;

    if (fmtChunk->payloadOffset > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) ||
        fmtChunk->size < 0x42u ||
        fmtChunk->payloadOffset + 0x42u > payloadResult.payload.size()) {
        result.error = "fmt 0x30 metadata exceeds WEM bounds";
        return result;
    }

    const auto fmtOffset = static_cast<std::size_t>(fmtChunk->payloadOffset);
    const auto* fmt = payloadResult.payload.data() + fmtOffset;
    result.formatFlags = readU16Le(fmt + 0x12u);
    result.channelConfig = readU32Le(fmt + 0x14u);
    result.numSamples = readU32Le(fmt + 0x18u);
    result.dataStartField = readU32Le(fmt + 0x1Cu);
    result.dataEndField = readU32Le(fmt + 0x20u);
    result.smallField1 = readU16Le(fmt + 0x24u);
    result.smallField2 = readU16Le(fmt + 0x26u);
    result.setupOffset = readU32Le(fmt + 0x28u);
    result.audioOffset = readU32Le(fmt + 0x2Cu);
    result.maxPacketSize = readU16Le(fmt + 0x30u);
    result.lastGranuleExtra = readU16Le(fmt + 0x32u);
    result.decodeAllocSize = readU32Le(fmt + 0x34u);
    result.decodeX64AllocSize = readU32Le(fmt + 0x38u);
    result.metadataHashField = readU32Le(fmt + 0x3Cu);
    result.blockExpSmall = fmt[0x40u];
    result.blockExpLarge = fmt[0x41u];
    result.modifiedPackets = result.blockExpSmall != result.blockExpLarge;
    result.packetMode = result.modifiedPackets ? "modified" : "standard";

    if (result.sampleRate != 0u) {
        result.durationMs =
            (static_cast<std::uint64_t>(result.numSamples) * 1000u + result.sampleRate / 2u) /
            result.sampleRate;
    }

    if (result.setupOffset >= result.dataSize) {
        result.error = "setup offset outside data";
        return result;
    }
    if (result.audioOffset >= result.dataSize) {
        result.error = "audio offset outside data";
        return result;
    }
    if (result.setupOffset >= result.audioOffset) {
        result.error = "setup offset is not before audio offset";
        return result;
    }

    result.setupWemOffset = result.dataOffset + result.setupOffset;
    result.audioWemOffset = result.dataOffset + result.audioOffset;

    if (!parsePacket(
            payloadResult.payload,
            result.dataOffset,
            result.dataSize,
            result.setupOffset,
            maxPacketPrefixBytes,
            result.setupPacket,
            result.error)) {
        result.error = "setup " + result.error;
        return result;
    }
    result.setupEndsAtAudio = result.setupPacket.nextRelativeOffset == result.audioOffset;

    std::uint64_t cursor = result.audioOffset;
    for (std::size_t i = 0; i < maxAudioPackets && cursor < result.dataSize; ++i) {
        WwiseVorbisPacketInfo packet{};
        std::string packetError;
        if (!parsePacket(
                payloadResult.payload,
                result.dataOffset,
                result.dataSize,
                cursor,
                maxPacketPrefixBytes,
                packet,
                packetError)) {
            result.error = "audio " + packetError;
            return result;
        }
        cursor = packet.nextRelativeOffset;
        result.audioPackets.push_back(std::move(packet));
    }

    if (result.audioPackets.empty()) {
        result.error = "no audio packets available";
        return result;
    }
    result.audioScanTruncated = cursor < result.dataSize;
    result.probeComplete = true;
    return result;
}

std::string sds::formatWwiseVorbisPacketProbeSummary(const VoiceWwiseVorbisPacketProbeResult& result)
{
    std::ostringstream out;
    out << "Voice Wwise Vorbis probe:"
        << " variant=" << result.variant
        << " probe=" << (result.probeComplete ? "complete" : "incomplete")
        << " samples=" << result.numSamples
        << " durationMs=" << result.durationMs
        << " channels=" << result.channels
        << " sampleRate=" << result.sampleRate
        << " channelConfig=0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0')
        << result.channelConfig << std::dec << std::nouppercase << std::setfill(' ')
        << " setupOffset=" << result.setupOffset
        << " audioOffset=" << result.audioOffset
        << " setupWemOffset=" << result.setupWemOffset
        << " audioWemOffset=" << result.audioWemOffset
        << " maxPacketSize=" << result.maxPacketSize
        << " metadataHash=0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0')
        << result.metadataHashField << std::dec << std::nouppercase << std::setfill(' ')
        << " blockExpSmall=" << static_cast<unsigned>(result.blockExpSmall)
        << " blockExpLarge=" << static_cast<unsigned>(result.blockExpLarge)
        << " packetHeaderBytes=" << static_cast<unsigned>(result.packetHeaderBytes)
        << " packetMode=" << result.packetMode
        << " setupEndsAtAudio=" << (result.setupEndsAtAudio ? "yes" : "no")
        << " audioPackets=" << result.audioPackets.size()
        << " audioScanTruncated=" << (result.audioScanTruncated ? "yes" : "no");
    if (!result.error.empty()) {
        out << " error=\"" << result.error << "\"";
    }
    return out.str();
}

std::string sds::formatWwiseVorbisSetupPacketProbe(const VoiceWwiseVorbisPacketProbeResult& result)
{
    std::ostringstream out;
    out << "Voice Wwise Vorbis probe: setup"
        << " valid=" << (result.setupPacket.valid ? "yes" : "no")
        << formatPacketDetails(result.setupPacket)
        << " endsAtAudio=" << (result.setupEndsAtAudio ? "yes" : "no");
    return out.str();
}

std::string sds::formatWwiseVorbisAudioPacketProbe(const WwiseVorbisPacketInfo& packet, std::size_t index)
{
    std::ostringstream out;
    out << "Voice Wwise Vorbis probe: audio[" << index << "]"
        << " valid=" << (packet.valid ? "yes" : "no")
        << formatPacketDetails(packet);
    return out.str();
}
