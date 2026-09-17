#include <StarfieldDualSense/WwiseWemSourceProbe.h>

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

namespace
{
    [[nodiscard]] std::uint16_t readLe16(const std::uint8_t* bytes) noexcept
    {
        return static_cast<std::uint16_t>(bytes[0]) |
            static_cast<std::uint16_t>(bytes[1] << 8u);
    }

    [[nodiscard]] std::uint32_t readLe32(const std::uint8_t* bytes) noexcept
    {
        return static_cast<std::uint32_t>(bytes[0]) |
            (static_cast<std::uint32_t>(bytes[1]) << 8u) |
            (static_cast<std::uint32_t>(bytes[2]) << 16u) |
            (static_cast<std::uint32_t>(bytes[3]) << 24u);
    }

    [[nodiscard]] bool fourccEquals(const std::array<char, 4>& value, std::string_view expected) noexcept
    {
        return expected.size() == value.size() &&
            std::equal(value.begin(), value.end(), expected.begin());
    }

    [[nodiscard]] std::string narrowPath(std::wstring_view path)
    {
        std::string out;
        out.reserve(path.size());
        for (const wchar_t ch : path) {
            out.push_back(ch >= 0 && ch <= 0x7F ? static_cast<char>(ch) : '?');
        }
        return out;
    }
}

sds::WwiseWemSourceMetadata sds::probeWwiseWemFile(
    const std::filesystem::path& path,
    std::size_t maxChunks)
{
    WwiseWemSourceMetadata metadata{};
    if (maxChunks == 0) {
        metadata.error = "maxChunks=0";
        return metadata;
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        metadata.error = "open failed";
        return metadata;
    }
    metadata.openSucceeded = true;

    stream.seekg(0, std::ios::end);
    const auto end = stream.tellg();
    if (end < 0) {
        metadata.error = "size query failed";
        return metadata;
    }
    metadata.fileSize = static_cast<std::uint64_t>(end);
    stream.seekg(0, std::ios::beg);

    std::array<std::uint8_t, 12> riffHeader{};
    if (!stream.read(reinterpret_cast<char*>(riffHeader.data()), static_cast<std::streamsize>(riffHeader.size()))) {
        metadata.error = "short RIFF header";
        return metadata;
    }

    const std::array<char, 4> riff{
        static_cast<char>(riffHeader[0]), static_cast<char>(riffHeader[1]),
        static_cast<char>(riffHeader[2]), static_cast<char>(riffHeader[3]) };
    const std::array<char, 4> wave{
        static_cast<char>(riffHeader[8]), static_cast<char>(riffHeader[9]),
        static_cast<char>(riffHeader[10]), static_cast<char>(riffHeader[11]) };
    if (!fourccEquals(riff, "RIFF") || !fourccEquals(wave, "WAVE")) {
        metadata.error = "not RIFF/WAVE";
        return metadata;
    }
    metadata.riffWave = true;

    const std::uint64_t riffDeclaredEnd = static_cast<std::uint64_t>(readLe32(riffHeader.data() + 4)) + 8u;
    const std::uint64_t scanEnd = (std::min)(metadata.fileSize, riffDeclaredEnd);
    std::uint64_t cursor = 12;

    while (cursor + 8u <= scanEnd && metadata.chunksScanned < maxChunks) {
        stream.seekg(static_cast<std::streamoff>(cursor), std::ios::beg);
        std::array<std::uint8_t, 8> chunkHeader{};
        if (!stream.read(reinterpret_cast<char*>(chunkHeader.data()), static_cast<std::streamsize>(chunkHeader.size()))) {
            metadata.error = "short chunk header";
            return metadata;
        }

        const std::array<char, 4> chunkId{
            static_cast<char>(chunkHeader[0]), static_cast<char>(chunkHeader[1]),
            static_cast<char>(chunkHeader[2]), static_cast<char>(chunkHeader[3]) };
        const std::uint32_t chunkSize = readLe32(chunkHeader.data() + 4);
        const std::uint64_t payloadOffset = cursor + 8u;
        const std::uint64_t payloadEnd = payloadOffset + static_cast<std::uint64_t>(chunkSize);
        if (payloadEnd > scanEnd || payloadEnd < payloadOffset) {
            metadata.error = "chunk exceeds RIFF bounds";
            return metadata;
        }

        ++metadata.chunksScanned;
        if (fourccEquals(chunkId, "fmt ")) {
            metadata.fmtFound = true;
            if (chunkSize < 16u) {
                metadata.error = "fmt chunk too small";
                return metadata;
            }
            std::array<std::uint8_t, 16> fmt{};
            stream.seekg(static_cast<std::streamoff>(payloadOffset), std::ios::beg);
            if (!stream.read(reinterpret_cast<char*>(fmt.data()), static_cast<std::streamsize>(fmt.size()))) {
                metadata.error = "short fmt chunk";
                return metadata;
            }
            metadata.formatTag = readLe16(fmt.data());
            metadata.channels = readLe16(fmt.data() + 2);
            metadata.sampleRate = readLe32(fmt.data() + 4);
            metadata.averageBytesPerSecond = readLe32(fmt.data() + 8);
            metadata.blockAlign = readLe16(fmt.data() + 12);
            metadata.bitsPerSample = readLe16(fmt.data() + 14);
        } else if (fourccEquals(chunkId, "vorb")) {
            metadata.vorbFound = true;
            metadata.vorbSize = chunkSize;
        } else if (fourccEquals(chunkId, "data")) {
            metadata.dataFound = true;
            metadata.dataOffset = payloadOffset;
            metadata.dataSize = chunkSize;
        }

        const std::uint64_t paddedSize = static_cast<std::uint64_t>(chunkSize) + (chunkSize & 1u);
        if (payloadOffset > std::numeric_limits<std::uint64_t>::max() - paddedSize) {
            metadata.error = "chunk offset overflow";
            return metadata;
        }
        cursor = payloadOffset + paddedSize;
    }

    if (metadata.chunksScanned >= maxChunks && cursor + 8u <= scanEnd) {
        metadata.error = "maxChunks limit reached";
    }
    return metadata;
}

std::string sds::formatWwiseWemSourceProbe(
    std::wstring_view capturedPath,
    std::uint32_t codecId,
    const WwiseWemSourceMetadata& metadata)
{
    std::ostringstream out;
    out << "Remote VO WEM source probe: path=\"" << narrowPath(capturedPath) << "\"";
    if (codecId == 4u) {
        out << " codecId=4(WwiseVorbis)";
    } else {
        out << " codecId=" << codecId;
    }

    if (!metadata.openSucceeded) {
        out << " open=failed"
            << " error=\"" << metadata.error << "\"";
        return out.str();
    }

    out << " open=success"
        << " fileSize=" << metadata.fileSize
        << " container=" << (metadata.riffWave ? "RIFF/WAVE" : "unknown")
        << " fmt=" << (metadata.fmtFound ? "yes" : "no")
        << " formatTag=0x" << std::hex << std::uppercase << metadata.formatTag << std::dec
        << " channels=" << metadata.channels
        << " sampleRate=" << metadata.sampleRate
        << " vorb=" << (metadata.vorbFound ? "yes" : "no")
        << " vorbSize=" << metadata.vorbSize
        << " data=" << (metadata.dataFound ? "yes" : "no")
        << " dataOffset=" << metadata.dataOffset
        << " dataSize=" << metadata.dataSize
        << " chunksScanned=" << metadata.chunksScanned;
    if (!metadata.error.empty()) {
        out << " warning=\"" << metadata.error << "\"";
    }
    return out.str();
}
