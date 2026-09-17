#include <StarfieldDualSense/HapticEndpointSelection.h>

#include <string>

namespace
{
    wchar_t asciiLower(wchar_t ch) noexcept
    {
        if (ch >= L'A' && ch <= L'Z') {
            return static_cast<wchar_t>(ch - L'A' + L'a');
        }
        return ch;
    }

    bool containsAsciiInsensitive(std::wstring_view haystack, std::wstring_view needle) noexcept
    {
        if (needle.empty() || needle.size() > haystack.size()) {
            return false;
        }
        for (std::size_t start = 0; start + needle.size() <= haystack.size(); ++start) {
            bool matches = true;
            for (std::size_t i = 0; i < needle.size(); ++i) {
                if (asciiLower(haystack[start + i]) != asciiLower(needle[i])) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                return true;
            }
        }
        return false;
    }
}

bool sds::isDualSenseHapticEndpointName(std::wstring_view name) noexcept
{
    return containsAsciiInsensitive(name, L"dualsense") ||
        containsAsciiInsensitive(name, L"wireless controller");
}

std::optional<std::size_t> sds::selectDualSenseHapticEndpoint(
    std::span<const HapticEndpointCandidate> candidates) noexcept
{
    std::optional<std::size_t> best;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const auto& candidate = candidates[i];
        if (!isDualSenseHapticEndpointName(candidate.friendlyName) ||
            candidate.channels != 4 || candidate.sampleRate != 48000 ||
            candidate.sampleFormat == HapticSampleFormat::Unsupported) {
            continue;
        }
        if (!best || candidate.id < candidates[*best].id) {
            best = i;
        }
    }
    return best;
}
