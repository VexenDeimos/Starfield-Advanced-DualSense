#include <StarfieldDualSense/HidProbePackets.h>

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace
{
    void require(bool condition, std::string_view message)
    {
        if (!condition) {
            std::cerr << "FAIL " << message << '\n';
            std::exit(1);
        }
        std::cout << "PASS " << message << '\n';
    }

    bool hasStrongR2(const std::array<std::uint8_t, 48>& report)
    {
        return report[11] == 0x01 && report[12] != 0x00 && report[13] >= 0xC0;
    }
}

int main()
{
    const auto legacy = sds::probe::buildLegacyKnownGoodReport();
    require(legacy.size() == 48, "legacy report is 48 bytes");
    require(legacy[0] == 0x02, "legacy uses USB report id 0x02");
    require(legacy[1] == 0xFF && legacy[2] == 0xF7, "legacy preserves broad known-good validity masks");
    require(legacy[39] == 0x03 && legacy[42] == 0x02, "legacy preserves repeated LED setup/control bytes");
    require(legacy[45] == 0xFF && legacy[46] == 0x00 && legacy[47] == 0x00, "legacy test color is red");
    require(hasStrongR2(legacy), "legacy carries strong R2 resistance");

    const auto init = sds::probe::buildCurrentInitializationReport();
    require(init[0] == 0x02, "current init uses USB report id 0x02");
    require(init[39] == 0x02 && init[42] == 0x02, "current init is one-shot lightbar fade/release");

    const auto scoped = sds::probe::buildCurrentScopedReport();
    require(scoped[1] == 0x0C && scoped[2] == 0x04, "current scoped report owns only triggers and lightbar");
    require(scoped[39] == 0x00 && scoped[42] == 0x00, "current scoped report omits setup command");
    require(scoped[45] == 0x00 && scoped[46] == 0xFF && scoped[47] == 0x00, "current scoped test color is green");
    require(hasStrongR2(scoped), "current scoped report carries strong R2 resistance");

    const auto hybrid = sds::probe::buildHybridReport();
    require(hybrid[1] == 0xFF && hybrid[2] == 0xF7, "hybrid keeps legacy validity masks");
    require(hybrid[39] == 0x01 && hybrid[42] == 0x00, "hybrid removes only the lightbar setup bit/command");
    require(hybrid[45] == 0x00 && hybrid[46] == 0x40 && hybrid[47] == 0xFF, "hybrid test color is blue");
    require(hasStrongR2(hybrid), "hybrid carries strong R2 resistance");

    const auto reset = sds::probe::buildResetReport();
    require(reset[0] == 0x02, "reset uses USB report id 0x02");
    require(reset[11] == 0x00 && reset[22] == 0x00, "reset disables both adaptive triggers");
    require(reset[45] == 0x00 && reset[46] == 0x00 && reset[47] == 0x00, "reset clears lightbar RGB");

    return 0;
}
