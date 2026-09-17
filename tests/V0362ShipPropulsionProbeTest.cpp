#include <StarfieldDualSense/ShipPropulsionProbe.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace
{
    int failures = 0;

    void expect(bool condition, std::string_view name)
    {
        if (condition) {
            std::cout << "PASS " << name << '\n';
        } else {
            std::cout << "FAIL " << name << '\n';
            ++failures;
        }
    }

    bool near(float actual, float expected, float tolerance = 0.01F)
    {
        return std::fabs(actual - expected) <= tolerance;
    }
}

int main()
{
    using namespace std::chrono_literals;
    using sds::ShipPropulsionProbe;

    const auto t0 = std::chrono::steady_clock::time_point{};
    ShipPropulsionProbe probe{350ms};

    expect(!probe.observe(0.0F, 0.0F, 0.0F, t0).has_value(),
        "first ship position sample establishes an anchor only");

    const auto firstMotion = probe.observe(100.0F, 0.0F, 0.0F, t0 + 100ms);
    expect(firstMotion.has_value(), "second fresh ship position sample produces kinematics");
    expect(firstMotion && near(firstMotion->speedUnitsPerSecond, 1000.0F),
        "position delta produces scalar ship speed in game units per second");
    expect(firstMotion && !firstMotion->accelerationValid,
        "first derived speed has no fabricated acceleration history");

    const auto accelerating = probe.observe(250.0F, 0.0F, 0.0F, t0 + 200ms);
    expect(accelerating.has_value(), "third fresh ship sample produces kinematics");
    expect(accelerating && near(accelerating->speedUnitsPerSecond, 1500.0F),
        "later position delta updates scalar ship speed");
    expect(accelerating && accelerating->accelerationValid &&
        near(accelerating->accelerationUnitsPerSecondSquared, 5000.0F),
        "change in scalar speed produces observed ship acceleration");

    const auto coasting = probe.observe(400.0F, 0.0F, 0.0F, t0 + 300ms);
    expect(coasting && coasting->accelerationValid &&
        near(coasting->accelerationUnitsPerSecondSquared, 0.0F),
        "constant scalar speed produces zero observed acceleration");

    expect(!probe.observe(900.0F, 0.0F, 0.0F, t0 + 800ms).has_value(),
        "stale ship position gap resets rather than inventing propulsion state");
    expect(!probe.observe(900.0F, 0.0F, 0.0F, t0 + 800ms).has_value(),
        "duplicate timestamp is rejected and re-anchors safely");

    const auto postGap = probe.observe(950.0F, 0.0F, 0.0F, t0 + 900ms);
    expect(postGap && near(postGap->speedUnitsPerSecond, 500.0F) && !postGap->accelerationValid,
        "fresh motion after stale gap restarts without stale acceleration history");

    probe.reset();
    expect(!probe.observe(1000.0F, 0.0F, 0.0F, t0 + 1000ms).has_value(),
        "explicit reset drops all prior propulsion history");

    {
        std::vector<std::uint8_t> code(256, 0x90);
        const std::array<std::uint8_t, 7> writerPrefix{ 0x49, 0x8B, 0x85, 0xE8, 0x01, 0x00, 0x00 };
        const std::array<std::uint8_t, 8> rollWriter{ 0x48, 0x8D, 0x50, 0x58, 0xC5, 0xFA, 0x11, 0x02 };
        std::copy(writerPrefix.begin(), writerPrefix.end(), code.begin() + 40);
        std::copy(rollWriter.begin(), rollWriter.end(), code.begin() + 96);

        const auto site = sds::findShipFlightControlWriterSite(code);
        expect(site && *site == 96,
            "flight-control writer scanner resolves the unique validated roll writer");

        std::copy(writerPrefix.begin(), writerPrefix.end(), code.begin() + 140);
        std::copy(rollWriter.begin(), rollWriter.end(), code.begin() + 180);
        expect(!sds::findShipFlightControlWriterSite(code).has_value(),
            "flight-control writer scanner fails closed when more than one validated writer is present");
    }

    {
        constexpr std::uintptr_t source = 0x10000000ULL;
        constexpr std::uintptr_t destination = 0x10000100ULL;
        const auto patch = sds::buildShipFlightControlCapturePatch(source, destination);
        expect(patch.has_value(), "flight-control capture patch accepts an in-range rel32 destination");
        expect(patch && (*patch)[0] == 0xE9 && (*patch)[5] == 0x90 && (*patch)[6] == 0x90 && (*patch)[7] == 0x90,
            "flight-control capture patch replaces eight bytes with jump plus exact NOP tail");
        if (patch) {
            std::int32_t displacement = 0;
            std::memcpy(&displacement, patch->data() + 1, sizeof(displacement));
            expect(source + 5 + static_cast<std::intptr_t>(displacement) == destination,
                "flight-control capture patch lands exactly on the diagnostic trampoline");
        }

        expect(!sds::buildShipFlightControlCapturePatch(0x1000ULL, 0x900000000ULL).has_value(),
            "flight-control capture patch fails closed outside rel32 range");
    }

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
