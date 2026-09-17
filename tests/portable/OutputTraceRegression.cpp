#include <StarfieldDualSense/DualSenseReports.h>
#include <StarfieldDualSense/Types.h>

#include <iostream>
#include <string>

int main()
{
    sds::OutputState state{};
    state.lightbar = {0, 64, 255};
    state.rightTrigger.mode = sds::TriggerEffectMode::ContinuousResistance;
    state.rightTrigger.startPosition = 124;
    state.rightTrigger.force = 130;

    const auto report = sds::buildUsbOutputReport(state);
    const std::string text = sds::describeUsbOutputReport(report);

    auto require = [&](bool ok, const char* label) {
        if (!ok) {
            std::cerr << "FAIL " << label << "\n";
            return false;
        }
        std::cout << "PASS " << label << "\n";
        return true;
    };

    bool ok = true;
    ok &= require(text.find("bytes=48") != std::string::npos, "trace includes report length");
    ok &= require(text.find("flags0=0C") != std::string::npos, "trace includes flag0");
    ok &= require(text.find("flags1=04") != std::string::npos, "trace includes flag1");
    ok &= require(text.find("r2=01 7C 82") != std::string::npos, "trace includes encoded R2 block");
    ok &= require(text.find("rgb=0,64,255") != std::string::npos, "trace includes lightbar RGB");
    ok &= require(text.find("raw=02 0C 04") != std::string::npos, "trace includes raw bytes");
    return ok ? 0 : 1;
}
