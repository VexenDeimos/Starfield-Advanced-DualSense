#include <StarfieldDualSense/WwiseWemSourceProbe.h>

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <vector>

namespace
{
    void put16(std::vector<std::uint8_t>& bytes, std::uint16_t value)
    {
        bytes.push_back(static_cast<std::uint8_t>(value & 0xFFu));
        bytes.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFFu));
    }

    void put32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
    {
        for (int shift = 0; shift < 32; shift += 8) {
            bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFu));
        }
    }

    void fourcc(std::vector<std::uint8_t>& bytes, const char text[5])
    {
        bytes.insert(bytes.end(), text, text + 4);
    }

    std::vector<std::uint8_t> makeSyntheticWem()
    {
        std::vector<std::uint8_t> bytes;
        fourcc(bytes, "RIFF");
        put32(bytes, 0); // fixed below
        fourcc(bytes, "WAVE");

        fourcc(bytes, "fmt ");
        put32(bytes, 16);
        put16(bytes, 0xFFFFu); // Wwise Vorbis-style format tag
        put16(bytes, 1);       // mono
        put32(bytes, 48000);
        put32(bytes, 12000);
        put16(bytes, 1);
        put16(bytes, 0);

        fourcc(bytes, "vorb");
        put32(bytes, 8);
        for (int i = 0; i < 8; ++i) {
            bytes.push_back(static_cast<std::uint8_t>(i));
        }

        fourcc(bytes, "data");
        put32(bytes, 6);
        for (int i = 0; i < 6; ++i) {
            bytes.push_back(static_cast<std::uint8_t>(0xA0 + i));
        }

        const auto riffSize = static_cast<std::uint32_t>(bytes.size() - 8);
        bytes[4] = static_cast<std::uint8_t>(riffSize & 0xFFu);
        bytes[5] = static_cast<std::uint8_t>((riffSize >> 8) & 0xFFu);
        bytes[6] = static_cast<std::uint8_t>((riffSize >> 16) & 0xFFu);
        bytes[7] = static_cast<std::uint8_t>((riffSize >> 24) & 0xFFu);
        return bytes;
    }
}

int main()
{
    const auto temp = std::filesystem::temp_directory_path() / "sds_v099_probe_test.wem";
    {
        const auto bytes = makeSyntheticWem();
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    const auto metadata = sds::probeWwiseWemFile(temp, 16);
    assert(metadata.openSucceeded);
    assert(metadata.riffWave);
    assert(metadata.fmtFound);
    assert(metadata.formatTag == 0xFFFFu);
    assert(metadata.channels == 1u);
    assert(metadata.sampleRate == 48000u);
    assert(metadata.vorbFound);
    assert(metadata.vorbSize == 8u);
    assert(metadata.dataFound);
    assert(metadata.dataSize == 6u);
    assert(metadata.dataOffset > 0u);
    assert(metadata.chunksScanned == 3u);

    std::filesystem::remove(temp);

    const auto missing = sds::probeWwiseWemFile(temp, 16);
    assert(!missing.openSucceeded);
    assert(!missing.riffWave);

    return 0;
}
