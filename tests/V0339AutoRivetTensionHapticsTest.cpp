#include <StarfieldDualSense/HapticMixer.h>
#include <StarfieldDualSense/HapticsEngine.h>

#include <array>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <cmath>
#include <string_view>

namespace
{
    void require(bool condition, std::string_view expression)
    {
        if (!condition) {
            std::cerr << "FAIL: " << expression << '\n';
            std::exit(1);
        }
    }

#define REQUIRE(condition) require((condition), #condition)
    sds::GameEvent equip(std::string_view name)
    {
        sds::GameEvent event{};
        event.type = sds::GameEventType::WeaponEquipped;
        std::copy_n(name.data(), name.size(), event.text.data());
        return event;
    }

    sds::GameEvent fire(std::chrono::steady_clock::time_point when)
    {
        sds::GameEvent event{};
        event.type = sds::GameEventType::WeaponFired;
        event.when = when;
        constexpr std::string_view marker = "WeaponFire";
        std::copy_n(marker.data(), marker.size(), event.text.data());
        return event;
    }

    sds::GameEvent chargeStart()
    {
        sds::GameEvent event{};
        event.type = sds::GameEventType::WeaponFired;
        constexpr std::string_view marker = "AutoRivetChargeStart";
        std::copy_n(marker.data(), marker.size(), event.text.data());
        return event;
    }

    float actuatorRms(std::span<const sds::HapticFrame> frames)
    {
        double sum = 0.0;
        for (const auto& frame : frames) {
            sum += static_cast<double>(frame[2]) * frame[2];
            sum += static_cast<double>(frame[3]) * frame[3];
        }
        return frames.empty() ? 0.0F : static_cast<float>(std::sqrt(sum / (2.0 * frames.size())));
    }
}

int main()
{
    const auto now = std::chrono::steady_clock::now();
    sds::HapticsEngine engine(1.0F);
    (void)engine.handle(equip("Auto-Rivet"));

    const auto released = engine.handleRightTriggerInput(12);
    REQUIRE(released.kind == sds::HapticContinuousKind::None);

    REQUIRE(engine.handleRightTriggerInput(255).kind == sds::HapticContinuousKind::None);
    (void)engine.handle(chargeStart());

    const auto start = engine.handleRightTriggerInput(24);
    REQUIRE(start.kind != sds::HapticContinuousKind::None);
    REQUIRE(start.gain > 0.0F);

    const auto mid = engine.handleRightTriggerInput(128);
    const auto full = engine.handleRightTriggerInput(255);
    REQUIRE(mid.kind == start.kind && full.kind == start.kind);
    REQUIRE(mid.level > start.level);
    REQUIRE(full.level > mid.level);
    REQUIRE(std::fabs(full.level - 1.0F) < 0.0001F);

    sds::HapticMixer mixer;
    mixer.setContinuous(full);
    std::array<sds::HapticFrame, 1024> heldBlock{};
    mixer.render(heldBlock);
    const float heldRms = actuatorRms(heldBlock);
    REQUIRE(heldRms > 0.01F);

    const auto discharge = engine.handle(fire(now));
    REQUIRE(discharge.has_value());
    REQUIRE(discharge->kind == sds::HapticEffectKind::PrecisionBallisticKick);
    REQUIRE(std::fabs(discharge->gain - 0.8F) < 0.0001F);

    sds::HapticMixer dischargeMixer;
    dischargeMixer.add(sds::synthesizeHapticEffect(*discharge));
    std::array<sds::HapticFrame, 2880> dischargeBlock{};
    dischargeMixer.render(dischargeBlock);
    const float dischargeRms = actuatorRms(dischargeBlock);
    REQUIRE(dischargeRms > heldRms * 1.5F);

    const auto afterRelease = engine.handleRightTriggerInput(0);
    REQUIRE(afterRelease.kind == sds::HapticContinuousKind::None);

    (void)engine.handle(equip("Maelstrom"));
    REQUIRE(engine.handleRightTriggerInput(255).kind == sds::HapticContinuousKind::None);

    return 0;
}
