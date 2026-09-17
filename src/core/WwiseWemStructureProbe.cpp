#include <StarfieldDualSense/WwiseWemStructureProbe.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string_view>

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

    [[nodiscard]] bool hasFourCc(
        const std::vector<unsigned char>& payload,
        std::size_t offset,
        std::string_view expected) noexcept
    {
        if (expected.size() != 4u || payload.size() < offset + 4u) {
            return false;
        }
        return std::equal(expected.begin(), expected.end(), payload.begin() + static_cast<std::ptrdiff_t>(offset));
    }

    [[nodiscard]] std::string readFourCc(const std::vector<unsigned char>& payload, std::size_t offset)
    {
        std::string id(4, '.');
        for (std::size_t i = 0; i < 4u; ++i) {
            const auto value = payload[offset + i];
            id[i] = std::isprint(static_cast<unsigned char>(value)) != 0 ? static_cast<char>(value) : '.';
        }
        return id;
    }

    [[nodiscard]] std::string codecHintFor(std::uint32_t codecId)
    {
        return codecId == 4u ? "WwiseVorbis" : "unknown";
    }

    [[nodiscard]] bool isObservedWwisePcm16(const sds::VoiceWemStructureProbeResult& result) noexcept
    {
        if (!result.validRiffWave || !result.scanComplete || !result.fmtFound || !result.dataFound ||
            result.vorbFound || result.formatTag != 0xFFFEu || result.bitsPerSample != 16u ||
            (result.channels != 1u && result.channels != 2u) ||
            (result.sampleRate != 44100u && result.sampleRate != 48000u) ||
            result.fmtChunkSize != 24u || result.fmtExtraDeclaredSize != 6u ||
            result.fmtExtraAvailableSize != 6u || result.fmtExtraPrefix.size() != 6u) {
            return false;
        }

        const auto expectedBlockAlign = static_cast<std::uint16_t>(result.channels * 2u);
        if (result.blockAlign != expectedBlockAlign ||
            result.averageBytesPerSecond != result.sampleRate * expectedBlockAlign ||
            result.dataSize == 0u || (result.dataSize % expectedBlockAlign) != 0u) {
            return false;
        }

        const std::array<unsigned char, 6> monoExtra{ 0x00, 0x00, 0x01, 0x41, 0x00, 0x00 };
        const std::array<unsigned char, 6> stereoExtra{ 0x00, 0x00, 0x02, 0x31, 0x00, 0x00 };
        const auto& expectedExtra = result.channels == 1u ? monoExtra : stereoExtra;
        return std::equal(result.fmtExtraPrefix.begin(), result.fmtExtraPrefix.end(), expectedExtra.begin());
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
}

sds::VoiceWemStructureProbeResult sds::probeVoiceWemStructure(
    const VoiceWemPayloadProbeResult& payloadResult,
    std::uint32_t sourceCodecId,
    std::size_t maxChunks,
    std::size_t maxFmtExtraBytes)
{
    VoiceWemStructureProbeResult result{};
    result.attempted = true;
    result.sourceCodecId = sourceCodecId;
    result.codecHint = codecHintFor(sourceCodecId);

    if (!payloadResult.readSucceeded || payloadResult.payload.empty() ||
        !payloadResult.riffWave || !payloadResult.riffSizeMatchesPayload) {
        result.error = "validated WEM payload unavailable";
        return result;
    }
    if (maxChunks == 0u) {
        result.error = "maxChunks=0";
        return result;
    }

    const auto& payload = payloadResult.payload;
    if (payload.size() < 12u || !hasFourCc(payload, 0u, "RIFF") || !hasFourCc(payload, 8u, "WAVE")) {
        result.error = "not RIFF/WAVE";
        return result;
    }
    result.validRiffWave = true;

    const std::uint64_t declaredEnd = static_cast<std::uint64_t>(readU32Le(payload.data() + 4u)) + 8u;
    if (declaredEnd != payload.size()) {
        result.error = "RIFF size does not match payload";
        return result;
    }

    const std::uint64_t scanEnd = declaredEnd;
    std::uint64_t cursor = 12u;

    while (cursor + 8u <= scanEnd && result.chunks.size() < maxChunks) {
        if (cursor > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            result.error = "chunk offset exceeds addressable payload";
            return result;
        }
        const auto headerOffset = static_cast<std::size_t>(cursor);
        const auto chunkSize = readU32Le(payload.data() + headerOffset + 4u);
        const std::uint64_t payloadOffset = cursor + 8u;
        const std::uint64_t payloadEnd = payloadOffset + static_cast<std::uint64_t>(chunkSize);
        if (payloadEnd < payloadOffset || payloadEnd > scanEnd) {
            result.error = "chunk exceeds RIFF bounds";
            return result;
        }

        WemRiffChunkInfo chunk{};
        chunk.id = readFourCc(payload, headerOffset);
        chunk.headerOffset = cursor;
        chunk.payloadOffset = payloadOffset;
        chunk.size = chunkSize;
        chunk.padded = (chunkSize & 1u) != 0u;
        result.chunks.push_back(chunk);

        const auto chunkPayloadOffset = static_cast<std::size_t>(payloadOffset);
        if (chunk.id == "fmt " && !result.fmtFound) {
            result.fmtFound = true;
            result.fmtChunkSize = chunkSize;
            if (chunkSize < 16u) {
                result.error = "fmt chunk too small";
                return result;
            }

            const auto* fmt = payload.data() + chunkPayloadOffset;
            result.formatTag = readU16Le(fmt + 0u);
            result.channels = readU16Le(fmt + 2u);
            result.sampleRate = readU32Le(fmt + 4u);
            result.averageBytesPerSecond = readU32Le(fmt + 8u);
            result.blockAlign = readU16Le(fmt + 12u);
            result.bitsPerSample = readU16Le(fmt + 14u);

            if (chunkSize >= 18u) {
                result.fmtExtraDeclaredSize = readU16Le(fmt + 16u);
                result.fmtExtraAvailableSize = chunkSize - 18u;
                if (result.fmtExtraDeclaredSize > result.fmtExtraAvailableSize) {
                    result.error = "fmt extra exceeds chunk bounds";
                    return result;
                }
                const auto prefixSize = (std::min)({
                    static_cast<std::size_t>(result.fmtExtraDeclaredSize),
                    static_cast<std::size_t>(result.fmtExtraAvailableSize),
                    maxFmtExtraBytes });
                result.fmtExtraPrefix.assign(fmt + 18u, fmt + 18u + prefixSize);
            }
        } else if (chunk.id == "vorb") {
            result.vorbFound = true;
            result.vorbSize = chunkSize;
        } else if (chunk.id == "data") {
            result.dataFound = true;
            result.dataOffset = payloadOffset;
            result.dataSize = chunkSize;
        }

        if ((chunkSize & 1u) != 0u && payloadEnd == scanEnd) {
            // Some Wwise WEMs omit the optional word-alignment byte when an odd-sized
            // chunk is the final chunk in the RIFF container. Its payload still ends
            // exactly at the declared RIFF boundary, so there is nothing left to skip.
            cursor = payloadEnd;
        } else {
            const std::uint64_t paddedSize = static_cast<std::uint64_t>(chunkSize) + (chunkSize & 1u);
            if (payloadOffset > std::numeric_limits<std::uint64_t>::max() - paddedSize) {
                result.error = "chunk offset overflow";
                return result;
            }
            cursor = payloadOffset + paddedSize;
        }
    }

    if (result.chunks.size() >= maxChunks && cursor + 8u <= scanEnd) {
        result.error = "maxChunks limit reached";
        return result;
    }
    if (cursor != scanEnd) {
        result.error = "trailing RIFF bytes";
        return result;
    }

    result.scanComplete = true;
    return result;
}

std::string sds::formatVoiceWemStructureProbeSummary(const VoiceWemStructureProbeResult& result)
{
    std::ostringstream out;
    out << "Voice WEM structure probe:"
        << " codecId=" << result.sourceCodecId
        << " codecHint=" << result.codecHint
        << " container=" << (result.validRiffWave ? "RIFF/WAVE" : "unknown")
        << " scan=" << (result.scanComplete ? "complete" : "incomplete")
        << " chunks=" << result.chunks.size()
        << " fmt=" << (result.fmtFound ? "yes" : "no");

    if (result.fmtFound) {
        out << " fmtChunkSize=" << result.fmtChunkSize
            << " formatTag=0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
            << result.formatTag << std::dec << std::nouppercase << std::setfill(' ')
            << " channels=" << result.channels
            << " sampleRate=" << result.sampleRate
            << " avgBytesPerSec=" << result.averageBytesPerSecond
            << " blockAlign=" << result.blockAlign
            << " bitsPerSample=" << result.bitsPerSample
            << " fmtExtraDeclared=" << result.fmtExtraDeclaredSize
            << " fmtExtraAvailable=" << result.fmtExtraAvailableSize
            << " fmtExtraPrefix=" << formatHexBytes(result.fmtExtraPrefix);
    }

    out << " vorb=" << (result.vorbFound ? "yes" : "no")
        << " vorbSize=" << result.vorbSize
        << " data=" << (result.dataFound ? "yes" : "no")
        << " dataOffset=" << result.dataOffset
        << " dataSize=" << result.dataSize;

    if (!result.error.empty()) {
        out << " error=\"" << result.error << "\"";
    }
    return out.str();
}

std::string sds::formatVoiceWemStructureProbeChunk(const WemRiffChunkInfo& chunk, std::size_t index)
{
    std::ostringstream out;
    out << "Voice WEM structure probe: chunk[" << index << "]"
        << " id=\"" << chunk.id << "\""
        << " headerOffset=" << chunk.headerOffset
        << " payloadOffset=" << chunk.payloadOffset
        << " size=" << chunk.size
        << " padded=" << (chunk.padded ? "yes" : "no");
    return out.str();
}

sds::WemStructureInfo sds::inspectWemStructure(std::span<const unsigned char> payload)
{
    WemStructureInfo out{};
    VoiceWemPayloadProbeResult p{};
    p.attempted = true;
    p.openSucceeded = true;
    p.readSucceeded = !payload.empty();
    p.payload.assign(payload.begin(), payload.end());
    if (payload.size() >= 12u &&
        std::equal(payload.begin(), payload.begin() + 4, "RIFF") &&
        std::equal(payload.begin() + 8, payload.begin() + 12, "WAVE")) {
        p.riffWave = true;
        p.riffDeclaredSize = readU32Le(payload.data() + 4u);
        p.riffTotalBytes = static_cast<std::uint64_t>(p.riffDeclaredSize) + 8u;
        p.riffSizeMatchesPayload = p.riffTotalBytes == payload.size();
    }
    const auto voice = probeVoiceWemStructure(p, 0u);
    out.validRiffWave = voice.validRiffWave;
    out.scanComplete = voice.scanComplete;
    out.formatTag = voice.formatTag;
    out.channels = voice.channels;
    out.sampleRate = voice.sampleRate;
    out.blockAlign = voice.blockAlign;
    out.bitsPerSample = voice.bitsPerSample;
    out.vorbFound = voice.vorbFound;
    out.dataFound = voice.dataFound;
    out.dataSize = voice.dataSize;
    out.error = voice.error;
    if (voice.formatTag == 0x0001u && voice.fmtFound) {
        out.codecLabel = "PCM";
    } else if (isObservedWwisePcm16(voice)) {
        out.codecLabel = "WwisePCM16";
    } else if (voice.formatTag == 0xFFFFu && voice.fmtFound &&
               (voice.vorbFound || voice.fmtChunkSize >= 0x42u)) {
        out.codecLabel = "WwiseVorbis";
    }
    return out;
}
