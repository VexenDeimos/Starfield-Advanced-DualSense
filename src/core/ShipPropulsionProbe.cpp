#include <StarfieldDualSense/ShipPropulsionProbe.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

namespace
{
    constexpr std::array<std::uint8_t, 7> kFlightWriterPrefix{
        0x49, 0x8B, 0x85, 0xE8, 0x01, 0x00, 0x00
    };
    constexpr std::array<std::uint8_t, 8> kRollWriter{
        0x48, 0x8D, 0x50, 0x58, 0xC5, 0xFA, 0x11, 0x02
    };
    constexpr std::size_t kWriterSearchWindow = 0x180;

    template <std::size_t N>
    bool matchesAt(std::span<const std::uint8_t> bytes, std::size_t offset,
        const std::array<std::uint8_t, N>& pattern) noexcept
    {
        return offset <= bytes.size() && pattern.size() <= bytes.size() - offset &&
            std::equal(pattern.begin(), pattern.end(), bytes.begin() + static_cast<std::ptrdiff_t>(offset));
    }
}

std::optional<std::size_t> sds::findShipFlightControlWriterSite(
    std::span<const std::uint8_t> textBytes) noexcept
{
    std::vector<std::size_t> sites;
    for (std::size_t prefix = 0; prefix + kFlightWriterPrefix.size() <= textBytes.size(); ++prefix) {
        if (!matchesAt(textBytes, prefix, kFlightWriterPrefix)) {
            continue;
        }

        const auto limit = (std::min)(textBytes.size(), prefix + kWriterSearchWindow);
        for (std::size_t candidate = prefix + kFlightWriterPrefix.size();
             candidate + kRollWriter.size() <= limit; ++candidate) {
            if (matchesAt(textBytes, candidate, kRollWriter) &&
                std::find(sites.begin(), sites.end(), candidate) == sites.end()) {
                sites.push_back(candidate);
            }
        }
    }

    return sites.size() == 1 ? std::optional<std::size_t>{ sites.front() } : std::nullopt;
}

std::optional<std::array<std::uint8_t, 8>> sds::buildShipFlightControlCapturePatch(
    std::uintptr_t sourceAddress,
    std::uintptr_t destinationAddress) noexcept
{
    constexpr auto instructionSize = std::uintptr_t{ 5 };
    const auto nextAddress = sourceAddress + instructionSize;

    std::int64_t displacement = 0;
    if (destinationAddress >= nextAddress) {
        const auto delta = destinationAddress - nextAddress;
        if (delta > static_cast<std::uintptr_t>((std::numeric_limits<std::int32_t>::max)())) {
            return std::nullopt;
        }
        displacement = static_cast<std::int64_t>(delta);
    } else {
        const auto delta = nextAddress - destinationAddress;
        if (delta > static_cast<std::uintptr_t>(static_cast<std::uint64_t>(
                -(static_cast<std::int64_t>((std::numeric_limits<std::int32_t>::min)()))))) {
            return std::nullopt;
        }
        displacement = -static_cast<std::int64_t>(delta);
    }

    if (displacement < (std::numeric_limits<std::int32_t>::min)() ||
        displacement > (std::numeric_limits<std::int32_t>::max)()) {
        return std::nullopt;
    }

    std::array<std::uint8_t, 8> patch{ 0xE9, 0, 0, 0, 0, 0x90, 0x90, 0x90 };
    const auto rel32 = static_cast<std::int32_t>(displacement);
    std::memcpy(patch.data() + 1, &rel32, sizeof(rel32));
    return patch;
}


sds::ShipPropulsionProbe::ShipPropulsionProbe(std::chrono::milliseconds maxGap) noexcept :
    _maxGap(maxGap)
{}

std::optional<sds::ShipPropulsionKinematics> sds::ShipPropulsionProbe::observe(
    float x,
    float y,
    float z,
    std::chrono::steady_clock::time_point now) noexcept
{
    if (!_havePosition) {
        _x = x;
        _y = y;
        _z = z;
        _when = now;
        _havePosition = true;
        _haveSpeed = false;
        return std::nullopt;
    }

    const auto elapsed = now - _when;
    if (elapsed <= std::chrono::steady_clock::duration::zero() || elapsed > _maxGap) {
        _x = x;
        _y = y;
        _z = z;
        _when = now;
        _haveSpeed = false;
        return std::nullopt;
    }

    const float deltaSeconds = std::chrono::duration<float>(elapsed).count();
    const float dx = x - _x;
    const float dy = y - _y;
    const float dz = z - _z;
    const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    const float speed = distance / deltaSeconds;

    ShipPropulsionKinematics sample{};
    sample.speedUnitsPerSecond = speed;
    sample.deltaSeconds = deltaSeconds;
    if (_haveSpeed) {
        sample.accelerationUnitsPerSecondSquared = (speed - _lastSpeed) / deltaSeconds;
        sample.accelerationValid = true;
    }

    _x = x;
    _y = y;
    _z = z;
    _when = now;
    _lastSpeed = speed;
    _haveSpeed = true;
    return sample;
}

void sds::ShipPropulsionProbe::reset() noexcept
{
    _x = 0.0F;
    _y = 0.0F;
    _z = 0.0F;
    _lastSpeed = 0.0F;
    _when = {};
    _havePosition = false;
    _haveSpeed = false;
}
