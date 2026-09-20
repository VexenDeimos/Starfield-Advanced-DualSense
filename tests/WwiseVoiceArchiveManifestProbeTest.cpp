#include <StarfieldDualSense/WwiseVoiceArchiveManifestProbe.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    void writeBytes(
        const std::filesystem::path& path,
        const std::array<std::uint8_t, 12>& bytes)
    {
        std::ofstream out(
            path,
            std::ios::binary);

        out.write(
            reinterpret_cast<const char*>(
                bytes.data()),
            static_cast<std::streamsize>(
                bytes.size()));
    }
}

int main()
{
    const auto tempRoot =
        std::filesystem::temp_directory_path() /
        "sds-voice-archive-language-test";

    std::error_code cleanupError{};

    std::filesystem::remove_all(
        tempRoot,
        cleanupError);

    std::filesystem::create_directories(
        tempRoot / "Data");

    const auto executablePath =
        tempRoot / "Starfield.exe";

    const auto englishConfigPath =
        tempRoot / "Starfield.ini";

    {
        std::ofstream ini(
            englishConfigPath);

        ini
            << "[General]\n"
            << "sLanguage=en\n\n"
            << "[Archive]\n"
            << "sResourceEnglishVoiceList = "
            << "Starfield - Voices01.ba2, "
            << "Starfield - Voices02.ba2, "
            << "Starfield - VoicesPatch.ba2\n";
    }

    const auto spanishConfigPath =
        tempRoot / "Starfield_es.ini";

    {
        std::ofstream ini(
            spanishConfigPath);

        ini
            << "[General]\n"
            << "sLanguage=es\n\n"
            << "[Archive]\n"
            << "sResourceLocaleVoiceList = "
            << "Starfield - Voices_es01.ba2, "
            << "Starfield - Voices_es02.ba2, "
            << "Starfield - Voices_es_Patch.ba2\n";
    }

    const std::array<std::uint8_t, 12> validHeader{
        'B', 'T', 'D', 'X',
        0x03, 0x00, 0x00, 0x00,
        'G', 'N', 'R', 'L'
    };

    writeBytes(
        tempRoot / "Data" /
            "Starfield - Voices01.ba2",
        validHeader);

    writeBytes(
        tempRoot / "Data" /
            "Starfield - Voices02.ba2",
        {
            'B', 'T', 'D', 'X',
            0x07, 0x00, 0x00, 0x00,
            'G', 'N', 'R', 'L'
        });

    writeBytes(
        tempRoot / "Data" /
            "Starfield - VoicesPatch.ba2",
        {
            'N', 'O', 'P', 'E',
            0x01, 0x00, 0x00, 0x00,
            'D', 'X', '1', '0'
        });

    writeBytes(
        tempRoot / "Data" /
            "Starfield - Voices_es01.ba2",
        validHeader);

    writeBytes(
        tempRoot / "Data" /
            "Starfield - Voices_es02.ba2",
        validHeader);

    writeBytes(
        tempRoot / "Data" /
            "Starfield - Voices_es_Patch.ba2",
        validHeader);

    const std::wstring captured =
        L"Sound\\Voice\\Starfield.esm\\GenericFemaleEvenToned\\00F59797.wem";

    const auto english =
        sds::probeVoiceArchiveManifest(
            captured,
            executablePath,
            sds::SpeakerVoiceLanguage::English);

    assert(
        english.language ==
        sds::SpeakerVoiceLanguage::English);

    assert(
        english.configPath ==
        englishConfigPath);

    assert(
        english.voiceListKey ==
        "sResourceEnglishVoiceList");

    assert(english.configOpened);
    assert(english.voiceListFound);
    assert(english.entries.size() == 3);

    assert(
        english.entries[0].archiveName ==
        "Starfield - Voices01.ba2");

    assert(english.entries[0].exists);
    assert(english.entries[0].openSucceeded);
    assert(english.entries[0].fileSize == 12);
    assert(english.entries[0].magic == "BTDX");
    assert(english.entries[0].version == 3);
    assert(english.entries[0].type == "GNRL");
    assert(english.entries[0].validBtdxHeader);

    assert(english.entries[1].version == 7);
    assert(english.entries[1].validBtdxHeader);

    assert(english.entries[2].magic == "NOPE");
    assert(english.entries[2].type == "DX10");
    assert(!english.entries[2].validBtdxHeader);

    const auto spanish =
        sds::probeVoiceArchiveManifest(
            captured,
            executablePath,
            sds::SpeakerVoiceLanguage::Spanish);

    assert(
        spanish.language ==
        sds::SpeakerVoiceLanguage::Spanish);

    assert(
        spanish.configPath ==
        spanishConfigPath);

    assert(
        spanish.voiceListKey ==
        "sResourceLocaleVoiceList");

    assert(spanish.configOpened);
    assert(spanish.voiceListFound);
    assert(spanish.entries.size() == 3);

    assert(
        spanish.entries[0].archiveName ==
        "Starfield - Voices_es01.ba2");

    assert(
        spanish.entries[1].archiveName ==
        "Starfield - Voices_es02.ba2");

    assert(
        spanish.entries[2].archiveName ==
        "Starfield - Voices_es_Patch.ba2");

    for (const auto& entry :
         spanish.entries) {
        assert(entry.exists);
        assert(entry.openSucceeded);
        assert(entry.validBtdxHeader);
    }

    assert(
        sds::speakerVoiceLanguageFromGameCode("en") ==
        sds::SpeakerVoiceLanguage::English);

    assert(
        sds::speakerVoiceLanguageFromGameCode("fr") ==
        sds::SpeakerVoiceLanguage::French);

    assert(
        sds::speakerVoiceLanguageFromGameCode("de") ==
        sds::SpeakerVoiceLanguage::German);

    assert(
        sds::speakerVoiceLanguageFromGameCode("es") ==
        sds::SpeakerVoiceLanguage::Spanish);

    assert(
        sds::speakerVoiceLanguageFromGameCode("ja") ==
        sds::SpeakerVoiceLanguage::Japanese);

    assert(
        sds::speakerVoiceLanguageFromGameCode("JP") ==
        sds::SpeakerVoiceLanguage::Japanese);

    assert(
        sds::speakerVoiceLanguageFromGameCode("it") ==
        sds::SpeakerVoiceLanguage::English);

    assert(
        sds::resolveAutoSpeakerVoiceLanguage(
            false,
            "es") ==
        sds::SpeakerVoiceLanguage::English);

    assert(
        sds::resolveAutoSpeakerVoiceLanguage(
            true,
            "es") ==
        sds::SpeakerVoiceLanguage::Spanish);

    assert(
        sds::resolveAutoSpeakerVoiceLanguage(
            true,
            "fr") ==
        sds::SpeakerVoiceLanguage::French);

    assert(
        sds::resolveAutoSpeakerVoiceLanguage(
            true,
            "de") ==
        sds::SpeakerVoiceLanguage::German);

    assert(
        sds::resolveAutoSpeakerVoiceLanguage(
            true,
            "jp") ==
        sds::SpeakerVoiceLanguage::Japanese);

    assert(
        sds::resolveAutoSpeakerVoiceLanguage(
            true,
            "it") ==
        sds::SpeakerVoiceLanguage::English);

    const auto autoFallback =
        sds::probeVoiceArchiveManifest(
            captured,
            executablePath,
            sds::SpeakerVoiceLanguage::Auto);

    assert(
        autoFallback.language ==
        sds::SpeakerVoiceLanguage::English);

    assert(
        autoFallback.configPath ==
        englishConfigPath);

    const auto germanMissing =
        sds::probeVoiceArchiveManifest(
            captured,
            executablePath,
            sds::SpeakerVoiceLanguage::German);

    assert(
        germanMissing.configPath ==
        tempRoot / "Starfield_de.ini");

    assert(!germanMissing.configOpened);
    assert(!germanMissing.voiceListFound);
    assert(germanMissing.entries.empty());

    const auto context =
        sds::formatVoiceArchiveManifestContext(
            captured,
            spanish);

    assert(
        context.find(
            "language=Spanish") !=
        std::string::npos);

    assert(
        context.find(
            "key=sResourceLocaleVoiceList") !=
        std::string::npos);

    assert(
        context.find(
            "archives=3") !=
        std::string::npos);

    const auto entryText =
        sds::formatVoiceArchiveManifestEntry(
            spanish.entries[0]);

    assert(
        entryText.find(
            "archive=\"Starfield - Voices_es01.ba2\"") !=
        std::string::npos);

    assert(
        entryText.find(
            "exists=yes") !=
        std::string::npos);

    assert(
        entryText.find(
            "open=success") !=
        std::string::npos);

    const auto missing =
        sds::probeVoiceArchiveManifest(
            captured,
            tempRoot /
                "Missing" /
                "Starfield.exe",
            sds::SpeakerVoiceLanguage::English);

    assert(!missing.configOpened);
    assert(!missing.voiceListFound);
    assert(missing.entries.empty());
    assert(!missing.error.empty());

    std::filesystem::remove_all(
        tempRoot,
        cleanupError);

    return 0;
}
