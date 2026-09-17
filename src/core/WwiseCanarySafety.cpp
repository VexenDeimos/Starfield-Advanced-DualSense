#include <StarfieldDualSense/WwiseCanarySafety.h>

#include <array>
#include <cstring>

namespace
{
    template <std::size_t N>
    [[nodiscard]] bool startsWith(
        std::span<const std::uint8_t> code,
        const std::array<std::uint8_t, N>& prefix) noexcept
    {
        return code.size() >= N && std::equal(prefix.begin(), prefix.end(), code.begin());
    }

    template <std::size_t N>
    [[nodiscard]] bool containsBytes(
        std::span<const std::uint8_t> code,
        const std::array<std::uint8_t, N>& needle) noexcept
    {
        if (code.size() < N) {
            return false;
        }
        for (std::size_t i = 0; i + N <= code.size(); ++i) {
            if (std::equal(needle.begin(), needle.end(), code.begin() + static_cast<std::ptrdiff_t>(i))) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool containsAsciiCaseInsensitive(std::string_view haystack, std::string_view needle) noexcept
    {
        if (needle.empty() || haystack.size() < needle.size()) {
            return false;
        }
        const auto lower = [](char c) noexcept {
            return c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A')) : c;
        };
        for (std::size_t i = 0; i + needle.size() <= haystack.size(); ++i) {
            bool match = true;
            for (std::size_t j = 0; j < needle.size(); ++j) {
                if (lower(haystack[i + j]) != lower(needle[j])) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return true;
            }
        }
        return false;
    }
}

bool sds::matchesWwiseCanarySignature(
    WwiseCanaryApi api,
    std::span<const std::uint8_t> code) noexcept
{
    constexpr std::array<std::uint8_t, 16> kAddOutputPrefix{
        0x48,0x89,0x74,0x24,0x20,0x41,0x54,0x41,0x56,0x41,0x57,0x48,0x83,0xEC,0x20,0x41
    };
    constexpr std::array<std::uint8_t, 9> kSingleIdPrefix{
        0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9
    };
    constexpr std::array<std::uint8_t, 13> kGameObjPrefix{
        0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0x48,0x83,0xF9,0xE0
    };

    switch (api) {
    case WwiseCanaryApi::AddOutput:
        return startsWith(code, kAddOutputPrefix) &&
            containsBytes(code, std::array<std::uint8_t, 5>{ 0xBA,0x1C,0x00,0x00,0x00 });
    case WwiseCanaryApi::RemoveOutput:
        return startsWith(code, kSingleIdPrefix) &&
            containsBytes(code, std::array<std::uint8_t, 5>{ 0xBA,0x1D,0x00,0x00,0x00 });
    case WwiseCanaryApi::RegisterGameObj:
        return startsWith(code, kGameObjPrefix) &&
            containsBytes(code, std::array<std::uint8_t, 5>{ 0xBA,0x0B,0x00,0x00,0x00 });
    case WwiseCanaryApi::UnregisterGameObj:
        return startsWith(code, kGameObjPrefix) &&
            containsBytes(code, std::array<std::uint8_t, 5>{ 0xBA,0x0C,0x00,0x00,0x00 });
    }
    return false;
}

bool sds::matchesSetListenersWrapper(
    std::span<const std::uint8_t> code,
    std::uintptr_t wrapperAddress,
    std::uintptr_t expectedTargetAddress) noexcept
{
    if (code.size() < 8 ||
        code[0] != 0x45 || code[1] != 0x33 || code[2] != 0xC9 || code[3] != 0xE9) {
        return false;
    }

    std::int32_t displacement = 0;
    std::memcpy(&displacement, code.data() + 4, sizeof(displacement));
    const auto nextInstruction = wrapperAddress + 8;
    const auto target = static_cast<std::uintptr_t>(
        static_cast<std::intptr_t>(nextInstruction) + static_cast<std::intptr_t>(displacement));
    return target == expectedTargetAddress;
}

std::size_t sds::selectDualSenseRenderEndpoint(
    std::span<const std::string_view> friendlyNames) noexcept
{
    std::size_t fallback = kNoEndpointIndex;
    for (std::size_t i = 0; i < friendlyNames.size(); ++i) {
        const auto name = friendlyNames[i];
        if (!containsAsciiCaseInsensitive(name, "DualSense")) {
            continue;
        }
        if (containsAsciiCaseInsensitive(name, "Speakers")) {
            return i;
        }
        if (fallback == kNoEndpointIndex) {
            fallback = i;
        }
    }
    return fallback;
}
