#include <StarfieldDualSense/WeaponReloadHaptics.h>
#include <StarfieldDualSense/HapticWaveforms.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>

namespace
{
    void check(bool result, std::string_view description)
    {
        if (!result) {
            std::cerr << "FAIL " << description << '\n';
            std::exit(1);
        }
        std::cout << "PASS " << description << '\n';
    }
}

int main()
{
    using sds::HapticEffectKind;
    using sds::weaponReloadHapticForAction;

    // v0.7.7 real-Wwise additions: each cue must trigger its own native-timed
    // haptic, while the duplicate simultaneous Eon mag-in must NOT vibrate twice.
    const auto checkAddedStage = [](std::string_view action, HapticEffectKind kind, float gain) {
        const auto cue = weaponReloadHapticForAction(action);
        check(cue && cue->kind == kind && std::abs(cue->baseGain - gain) < 0.001F, action);
    };
    checkAddedStage("reload-sound-mag-out", HapticEffectKind::ReloadMagRelease, 0.32F);
    checkAddedStage("reload-sound-mag-in-02", HapticEffectKind::ReloadMagInsert, 0.38F);
    checkAddedStage("reload-sound-bolt-open", HapticEffectKind::ReloadMechanism, 0.36F);
    checkAddedStage("reload-sound-bolt-close", HapticEffectKind::ReloadMechanism, 0.56F);
    checkAddedStage("reload-sound-mag-out-02", HapticEffectKind::ReloadMagRelease, 0.26F);
    checkAddedStage("reload-sound-bolt-close-06", HapticEffectKind::ReloadMechanism, 0.26F);
    checkAddedStage("reload-sound-bolt-close-07", HapticEffectKind::ReloadMechanism, 0.34F);
    checkAddedStage("reload-sound-bolt-close-08", HapticEffectKind::ReloadMechanism, 0.55F);
    check(!weaponReloadHapticForAction("reload-sound-mag-in-04"),
          "simultaneous Eon mag-in speaker events do not double-haptic");
    check(!weaponReloadHapticForAction("reload-sound-unmapped"),
          "unknown supplementary sounds remain sound-only");

    check(weaponReloadHapticForAction("mag-out") &&
              weaponReloadHapticForAction("mag-out-b")->kind == HapticEffectKind::ReloadMagRelease,
          "magazine removal stages classify as release");
    check(weaponReloadHapticForAction("mag-in-5") &&
              weaponReloadHapticForAction("slam-mag")->kind == HapticEffectKind::ReloadMagInsert,
          "magazine insertion and slaps classify as insert");
    check(weaponReloadHapticForAction("bolt-charge-c") &&
              weaponReloadHapticForAction("bolt-rack")->kind == HapticEffectKind::ReloadMechanism,
          "bolt operations classify as mechanical");
    check(weaponReloadHapticForAction("bullet-handling") &&
              weaponReloadHapticForAction("mag-grab")->kind == HapticEffectKind::ReloadHandling,
          "hand-operated ammunition stages classify as handling");
    for (const auto action : { "fire", "fire-body", "draw", "holster", "spin-up", "power-down",
                               "reload-start", "reload-end", "sparks", "raise", "adjust" }) {
        check(!weaponReloadHapticForAction(action), "non-reload or ambiguous sounds do not vibrate");
    }

    constexpr std::uint32_t rate = 48000u;
    for (const auto action : { "mag-out", "mag-in", "bolt-close", "bullet-handling" }) {
        const auto cue = weaponReloadHapticForAction(action);
        check(cue.has_value(), "known stage has a finite haptic cue");
        const auto frames = sds::synthesizeHapticEffect({
            .kind = cue->kind,
            .gain = cue->baseGain,
            .when = std::chrono::steady_clock::now(),
        }, rate);
        check(frames.size() >= 900u && frames.size() <= 4000u,
              "reload pulse lasts 19 to 83 milliseconds");
        const auto nonzero = std::any_of(frames.begin(), frames.end(), [](const auto& f) {
            return std::abs(f[2]) > 0.002F || std::abs(f[3]) > 0.002F;
        });
        check(nonzero, "reload pulse contains actual vibration");
        check(std::abs(frames.back()[2]) < 0.001F &&
                  std::abs(frames.back()[3]) < 0.001F,
              "reload pulse decays to silence");
    }
    std::cout << "PASS reload stage haptics regression complete\n";
}
