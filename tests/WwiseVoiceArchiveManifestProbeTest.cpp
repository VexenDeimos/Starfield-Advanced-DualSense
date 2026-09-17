#include <StarfieldDualSense/WwiseVoiceArchiveManifestProbe.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    void writeBytes(const std::filesystem::path& path, const std::array<std::uint8_t, 12>& bytes)
    {
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }
}

int main()
{
    const auto tempRoot = std::filesystem::temp_directory_path() / "sds-v0301-voice-archive-manifest-test";
    std::error_code cleanupError{};
    std::filesystem::remove_all(tempRoot, cleanupError);
    std::filesystem::create_directories(tempRoot / "Data");

    const auto executablePath = tempRoot / "Starfield.exe";
    const auto configPath = tempRoot / "Starfield.ini";
    {
        std::ofstream ini(configPath);
        ini << "[General]\nfoo=bar\n\n"
            << "[Archive]\n"
            << "sResourceEnglishVoiceList = Starfield - Voices01.ba2, Starfield - Voices02.ba2 , Starfield - VoicesPatch.ba2\n";
    }

    writeBytes(
        tempRoot / "Data" / "Starfield - Voices01.ba2",
        { 'B', 'T', 'D', 'X', 0x03, 0x00, 0x00, 0x00, 'G', 'N', 'R', 'L' });
    writeBytes(
        tempRoot / "Data" / "Starfield - Voices02.ba2",
        { 'B', 'T', 'D', 'X', 0x07, 0x00, 0x00, 0x00, 'G', 'N', 'R', 'L' });
    writeBytes(
        tempRoot / "Data" / "Starfield - VoicesPatch.ba2",
        { 'N', 'O', 'P', 'E', 0x01, 0x00, 0x00, 0x00, 'D', 'X', '1', '0' });

    const std::wstring captured = L"Sound\\Voice\\Starfield.esm\\GenericFemaleEvenToned\\00F59797.wem";
    const auto manifest = sds::probeVoiceArchiveManifest(captured, executablePath);

    assert(manifest.configPath == configPath);
    assert(manifest.dataRoot == tempRoot / "Data");
    assert(manifest.configOpened);
    assert(manifest.voiceListFound);
    assert(manifest.entries.size() == 3);

    assert(manifest.entries[0].archiveName == "Starfield - Voices01.ba2");
    assert(manifest.entries[0].exists);
    assert(manifest.entries[0].openSucceeded);
    assert(manifest.entries[0].fileSize == 12);
    assert(manifest.entries[0].magic == "BTDX");
    assert(manifest.entries[0].version == 3);
    assert(manifest.entries[0].type == "GNRL");
    assert(manifest.entries[0].validBtdxHeader);

    assert(manifest.entries[1].version == 7);
    assert(manifest.entries[1].validBtdxHeader);

    assert(manifest.entries[2].magic == "NOPE");
    assert(manifest.entries[2].type == "DX10");
    assert(!manifest.entries[2].validBtdxHeader);

    const auto context = sds::formatVoiceArchiveManifestContext(captured, manifest);
    assert(context.find("Voice archive manifest probe: captured=\"") != std::string::npos);
    assert(context.find("key=sResourceEnglishVoiceList") != std::string::npos);
    assert(context.find("archives=3") != std::string::npos);

    const auto entryText = sds::formatVoiceArchiveManifestEntry(manifest.entries[0]);
    assert(entryText.find("archive=\"Starfield - Voices01.ba2\"") != std::string::npos);
    assert(entryText.find("exists=yes") != std::string::npos);
    assert(entryText.find("open=success") != std::string::npos);
    assert(entryText.find("magic=BTDX") != std::string::npos);
    assert(entryText.find("version=3") != std::string::npos);
    assert(entryText.find("type=GNRL") != std::string::npos);

    const auto missing = sds::probeVoiceArchiveManifest(captured, tempRoot / "Missing" / "Starfield.exe");
    assert(!missing.configOpened);
    assert(!missing.voiceListFound);
    assert(missing.entries.empty());
    assert(!missing.error.empty());

    std::filesystem::remove_all(tempRoot, cleanupError);
    return 0;
}
