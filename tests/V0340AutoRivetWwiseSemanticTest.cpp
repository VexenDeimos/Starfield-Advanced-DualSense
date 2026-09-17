#include <StarfieldDualSense/AutoRivetChargeSemantics.h>

#include <cstdlib>
#include <iostream>
#include <string_view>

namespace
{
    void require(bool condition, std::string_view expression)
    {
        if (!condition) {
            std::cerr << "FAIL: " << expression << '\n';
            std::exit(1);
        }
        std::cout << "PASS " << expression << '\n';
    }

#define REQUIRE(condition) require((condition), #condition)
}

int main()
{
    REQUIRE(sds::kAutoRivetChargeStartEventId == 0xB963BD33u);
    REQUIRE(sds::kAutoRivetChargeStopEventId == 0xCDEE7F71u);

    REQUIRE(sds::routeAutoRivetChargeWwise(
                sds::kAutoRivetChargeStartEventId,
                sds::kAutoRivetPlayerGameObjectId) == sds::AutoRivetChargeAction::Start);
    REQUIRE(sds::routeAutoRivetChargeWwise(
                sds::kAutoRivetChargeStopEventId,
                sds::kAutoRivetPlayerGameObjectId) == sds::AutoRivetChargeAction::Stop);
    REQUIRE(sds::routeAutoRivetChargeWwise(
                sds::kAutoRivetChargeStartEventId,
                0x21u) == sds::AutoRivetChargeAction::None);
    REQUIRE(sds::routeAutoRivetChargeWwise(
                0xEA70494Bu,
                sds::kAutoRivetPlayerGameObjectId) == sds::AutoRivetChargeAction::None);

    REQUIRE(sds::autoRivetChargeMarker(sds::AutoRivetChargeAction::Start) == "AutoRivetChargeStart");
    REQUIRE(sds::autoRivetChargeMarker(sds::AutoRivetChargeAction::Stop) == "AutoRivetChargeStop");
    REQUIRE(sds::autoRivetChargeMarker(sds::AutoRivetChargeAction::None).empty());

    std::cout << "PASS Auto-Rivet Wwise semantic routing suite\n";
    return 0;
}
