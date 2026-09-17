#include <StarfieldDualSense/IncomingDamageDiagnostic.h>

#include <chrono>
#include <cmath>
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
            std::cerr << "FAIL " << name << '\n';
            ++failures;
        }
    }
}

int main()
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::steady_clock;
    const auto t0 = Clock::time_point{ 30s };

    sds::IncomingDamageNullTargetFallback fallback;

    expect(!fallback.confirmHealthDrop(0.90F, 0.88F, t0 + 50ms, false),
           "health loss alone cannot create fallback damage without TESHit evidence");

    expect(!fallback.armTesHit(1, t0, false, false, true),
           "non-null TESHit targets are not eligible for null-target fallback");
    expect(!fallback.armTesHit(2, t0, true, true, true),
           "player-caused TESHits cannot arm incoming fallback damage");
    expect(!fallback.armTesHit(3, t0, true, false, false),
           "null-target TESHits outside active gameplay HUD cannot arm fallback damage");

    expect(fallback.armTesHit(4, t0, true, false, true),
           "null-target non-player TESHit during gameplay HUD arms fallback evidence");
    expect(!fallback.confirmHealthDrop(0.9000F, 0.8980F, t0 + 75ms, false),
           "sub-quarter-percent health loss stays below fallback confirmation threshold");
    const auto confirmed = fallback.confirmHealthDrop(0.9000F, 0.8850F, t0 + 100ms, false);
    expect(confirmed && confirmed->sequence == 4 &&
               std::fabs(confirmed->healthLossRatio - 0.0150F) < 0.0001F &&
               confirmed->ageMs == 100,
           "eligible health loss inside 125ms confirms one generic fallback hit");
    expect(!fallback.confirmHealthDrop(0.8850F, 0.8700F, t0 + 110ms, false),
           "confirmed fallback evidence is consumed and cannot pulse twice");

    expect(fallback.armTesHit(5, t0 + 1s, true, false, true),
           "second null-target TESHit can arm independently");
    expect(!fallback.confirmHealthDrop(0.80F, 0.75F, t0 + 1126ms, false),
           "null-target TESHit evidence expires after the 125ms correlation window");

    expect(fallback.armTesHit(6, t0 + 2s, true, false, true),
           "fallback can arm before a normal player-target incident arrives");
    expect(!fallback.confirmHealthDrop(0.70F, 0.60F, t0 + 2050ms, true),
           "active positive-player incident suppresses null-target fallback to prevent double fire");
    expect(!fallback.confirmHealthDrop(0.60F, 0.55F, t0 + 2060ms, false),
           "suppressed fallback evidence is cleared rather than firing on a later poll");

    expect(fallback.armTesHit(7, t0 + 3s, true, false, true),
           "rapid null-target sequence can arm fallback");
    expect(fallback.armTesHit(8, t0 + 3060ms, true, false, true),
           "new null-target TESHit refreshes fallback evidence to the latest impact");
    const auto refreshed = fallback.confirmHealthDrop(0.50F, 0.47F, t0 + 3150ms, false);
    expect(refreshed && refreshed->sequence == 8 && refreshed->ageMs == 90,
           "health confirmation correlates with the most recent null-target TESHit during automatic fire");

    return failures == 0 ? 0 : 1;
}
