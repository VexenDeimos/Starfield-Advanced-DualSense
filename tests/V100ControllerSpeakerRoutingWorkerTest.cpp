#include <StarfieldDualSense/ControllerManager.h>
#include <StarfieldDualSense/DualSenseReports.h>
#include <StarfieldDualSense/IControllerBackend.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string_view>

namespace
{
    int g_failures = 0;

    void check(bool condition, std::string_view label)
    {
        if (condition) {
            std::cout << "PASS " << label << '\n';
        } else {
            std::cerr << "FAIL " << label << '\n';
            ++g_failures;
        }
    }

    struct UnsupportedBackend final : sds::IControllerBackend
    {
        bool connect() override { return true; }
        void disconnect() noexcept override {}
        bool connected() const noexcept override { return true; }
        sds::DeviceIdentity identity() const override { return {}; }
        sds::Capabilities capabilities() const noexcept override { return {}; }
        std::optional<sds::TouchState> pollTouch() override { return std::nullopt; }
        bool setLightbar(sds::Color) override { return true; }
        bool setTriggers(const sds::TriggerEffect&, const sds::TriggerEffect&) override { return true; }
        void resetOutputs() noexcept override {}
    };
}

static_assert(requires(sds::ControllerManager& manager) {
    manager.setControllerSpeakerRoutingEnabled(true);
});

int main()
{
    UnsupportedBackend unsupported{};
    check(unsupported.setControllerSpeakerRoutingEnabled(false),
        "default backend treats speaker-route disable as harmless success");
    check(!unsupported.setControllerSpeakerRoutingEnabled(true),
        "default backend fails soft when live speaker-route enable is unsupported");

    sds::OutputState output{};
    output.lightbar = { .r = 0x12, .g = 0x34, .b = 0x56 };
    output.leftTrigger = {
        .mode = sds::TriggerEffectMode::ContinuousResistance,
        .startPosition = 0x20,
        .force = 0x40,
    };
    output.rightTrigger = {
        .mode = sds::TriggerEffectMode::SectionResistance,
        .startPosition = 0x22,
        .endPosition = 0x77,
    };

    const auto base = sds::buildUsbOutputReport(output);
    auto routed = base;
    sds::applyUsbInternalSpeakerRouting(routed, 0x64, 0x05);

    check((routed[1] & 0xA0U) == 0xA0U,
        "route ON validates speaker-volume and audio-control fields");
    check((routed[2] & 0x80U) == 0x80U,
        "route ON validates audio-control2 field");
    check(routed[6] == 0x64U && routed[8] == 0x30U && routed[38] == 0x05U,
        "route ON selects the proven internal-right speaker path");

    auto disabled = routed;
    sds::applyUsbInternalSpeakerRoutingDisabled(disabled);

    check((disabled[1] & 0xA0U) == 0xA0U,
        "route OFF explicitly validates speaker-volume and audio-control fields");
    check((disabled[2] & 0x80U) == 0x80U,
        "route OFF explicitly validates audio-control2 field");
    check(disabled[6] == 0x00U && disabled[8] == 0x00U && disabled[38] == 0x00U,
        "route OFF explicitly clears speaker volume, path selector, and preamp");
    check((disabled[1] & 0x0CU) == (base[1] & 0x0CU),
        "route OFF preserves adaptive-trigger ownership bits");
    check((disabled[2] & 0x04U) == (base[2] & 0x04U),
        "route OFF preserves lightbar ownership bit");

    bool unrelatedStable = true;
    for (std::size_t i = 0; i < disabled.size(); ++i) {
        if (i == 1 || i == 2 || i == 6 || i == 8 || i == 38) {
            continue;
        }
        if (disabled[i] != base[i]) {
            unrelatedStable = false;
            break;
        }
    }
    check(unrelatedStable, "route OFF changes no unrelated USB payload bytes");

    return g_failures == 0 ? 0 : 1;
}