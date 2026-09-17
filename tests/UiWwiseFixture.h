#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace sds::test
{
    struct UiWwiseFixtureSpec
    {
        std::uint32_t eventId{};
        std::string eventName{};
        std::uint32_t mediaId{};
        std::string mediaShortName{};
        std::int16_t sample{ 7 };
    };

    namespace detail
    {
        inline void p16(std::vector<unsigned char>& bytes, std::uint16_t value)
        {
            bytes.push_back(static_cast<unsigned char>(value & 0xFFu));
            bytes.push_back(static_cast<unsigned char>((value >> 8u) & 0xFFu));
        }

        inline void p32(std::vector<unsigned char>& bytes, std::uint32_t value)
        {
            for (int shift = 0; shift < 4; ++shift) {
                bytes.push_back(static_cast<unsigned char>((value >> (8 * shift)) & 0xFFu));
            }
        }

        inline void p64(std::vector<unsigned char>& bytes, std::uint64_t value)
        {
            for (int shift = 0; shift < 8; ++shift) {
                bytes.push_back(static_cast<unsigned char>((value >> (8 * shift)) & 0xFFu));
            }
        }

        inline void s32(std::vector<unsigned char>& bytes, std::size_t offset, std::uint32_t value)
        {
            for (int shift = 0; shift < 4; ++shift) {
                bytes[offset + shift] = static_cast<unsigned char>((value >> (8 * shift)) & 0xFFu);
            }
        }

        inline void s64(std::vector<unsigned char>& bytes, std::size_t offset, std::uint64_t value)
        {
            for (int shift = 0; shift < 8; ++shift) {
                bytes[offset + shift] = static_cast<unsigned char>((value >> (8 * shift)) & 0xFFu);
            }
        }

        inline std::vector<unsigned char> pcm(std::int16_t sample)
        {
            std::vector<unsigned char> bytes;
            bytes.insert(bytes.end(), { 'R', 'I', 'F', 'F' });
            p32(bytes, 0u);
            bytes.insert(bytes.end(), { 'W', 'A', 'V', 'E' });

            std::vector<unsigned char> fmt;
            p16(fmt, 0xFFFEu);
            p16(fmt, 2u);
            p32(fmt, 48000u);
            p32(fmt, 192000u);
            p16(fmt, 4u);
            p16(fmt, 16u);
            p16(fmt, 6u);
            fmt.insert(fmt.end(), { 0x00, 0x00, 0x02, 0x31, 0x00, 0x00 });
            bytes.insert(bytes.end(), { 'f', 'm', 't', ' ' });
            p32(bytes, static_cast<std::uint32_t>(fmt.size()));
            bytes.insert(bytes.end(), fmt.begin(), fmt.end());

            bytes.insert(bytes.end(), { 'd', 'a', 't', 'a' });
            p32(bytes, 4u);
            p16(bytes, static_cast<std::uint16_t>(sample));
            p16(bytes, static_cast<std::uint16_t>(sample));

            const auto riffSize = static_cast<std::uint32_t>(bytes.size() - 8u);
            s32(bytes, 4u, riffSize);
            return bytes;
        }

        inline void ba2(
            const std::filesystem::path& path,
            const std::vector<std::pair<std::string, std::vector<unsigned char>>>& entries)
        {
            std::vector<unsigned char> bytes;
            bytes.insert(bytes.end(), { 'B', 'T', 'D', 'X' });
            p32(bytes, 2u);
            bytes.insert(bytes.end(), { 'G', 'N', 'R', 'L' });
            p32(bytes, static_cast<std::uint32_t>(entries.size()));
            p64(bytes, 0u);
            p64(bytes, 0u);

            std::vector<std::size_t> starts;
            for (std::size_t i = 0; i < entries.size(); ++i) {
                starts.push_back(bytes.size());
                bytes.resize(bytes.size() + 36u);
            }
            for (std::size_t i = 0; i < entries.size(); ++i) {
                const auto payloadOffset = bytes.size();
                bytes.insert(bytes.end(), entries[i].second.begin(), entries[i].second.end());
                auto extension = std::filesystem::path(entries[i].first).extension().string();
                if (!extension.empty()) {
                    extension.erase(0, 1);
                }
                s32(bytes, starts[i], static_cast<std::uint32_t>(0x900u + i));
                for (std::size_t pos = 0; pos < 4u; ++pos) {
                    bytes[starts[i] + 4u + pos] = pos < extension.size()
                        ? static_cast<unsigned char>(extension[pos]) : 0u;
                }
                s64(bytes, starts[i] + 16u, payloadOffset);
                s32(bytes, starts[i] + 28u, static_cast<std::uint32_t>(entries[i].second.size()));
                s32(bytes, starts[i] + 32u, 0x12345678u);
            }

            const auto nameOffset = bytes.size();
            s64(bytes, 16u, nameOffset);
            for (const auto& entry : entries) {
                p16(bytes, static_cast<std::uint16_t>(entry.first.size()));
                bytes.insert(bytes.end(), entry.first.begin(), entry.first.end());
            }

            std::ofstream(path, std::ios::binary).write(
                reinterpret_cast<const char*>(bytes.data()),
                static_cast<std::streamsize>(bytes.size()));
        }
    }

    inline void writeSingleUiWwiseFixture(
        const std::filesystem::path& dataPath,
        const UiWwiseFixtureSpec& spec)
    {
        std::filesystem::create_directories(dataPath);
        const auto media = detail::pcm(spec.sample);
        std::string escapedName;
        escapedName.reserve(spec.mediaShortName.size() * 2u);
        for (const char ch : spec.mediaShortName) {
            if (ch == '\\' || ch == '"') {
                escapedName.push_back('\\');
            }
            escapedName.push_back(ch);
        }
        const std::string json =
            "{\"SoundBanksInfo\":{\"StreamedFiles\":["
            "{\"Id\":\"" + std::to_string(spec.mediaId) + "\",\"ShortName\":\"" + escapedName +
            "\",\"Path\":\"SFX\\\\UI\\\\" + escapedName + "\"}],"
            "\"SoundBanks\":[{\"ShortName\":\"Starfield_UI\",\"IncludedEvents\":["
            "{\"Id\":\"" + std::to_string(spec.eventId) + "\",\"Name\":\"" + spec.eventName +
            "\",\"ReferencedStreamedFiles\":[{\"Id\":\"" + std::to_string(spec.mediaId) + "\"}]}]}]}}";
        detail::ba2(dataPath / "Starfield - WwiseSounds01.ba2", {
            { "soundbanksinfo.json", std::vector<unsigned char>(json.begin(), json.end()) },
            { std::to_string(spec.mediaId) + ".wem", media },
        });
    }
}
