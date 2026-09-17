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
        if (condition) std::cout << "PASS " << name << '\n';
        else { std::cerr << "FAIL " << name << '\n'; ++failures; }
    }
}

int main()
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::steady_clock;
    const auto t0 = Clock::time_point{ 10s };

    const auto front = sds::calculateIncomingDirection({0,0,0}, 0.0F, {0,10,0});
    const auto right = sds::calculateIncomingDirection({0,0,0}, 0.0F, {10,0,0});
    const auto back = sds::calculateIncomingDirection({0,0,0}, 0.0F, {0,-10,0});
    const auto left = sds::calculateIncomingDirection({0,0,0}, 0.0F, {-10,0,0});
    const auto frontRight = sds::calculateIncomingDirection({0,0,0}, 0.0F, {10,10,0});
    expect(front.valid && std::fabs(front.angleDegrees) < 0.01F && front.label == sds::IncomingDirectionLabel::Front,
           "direction maps +Y to front at zero heading");
    expect(right.valid && std::fabs(right.angleDegrees - 90.0F) < 0.01F && right.label == sds::IncomingDirectionLabel::Right,
           "direction maps +X to right at zero heading");
    expect(back.valid && std::fabs(std::fabs(back.angleDegrees) - 180.0F) < 0.01F && back.label == sds::IncomingDirectionLabel::Back,
           "direction maps -Y to back at zero heading");
    expect(left.valid && std::fabs(left.angleDegrees + 90.0F) < 0.01F && left.label == sds::IncomingDirectionLabel::Left,
           "direction maps -X to left at zero heading");
    expect(frontRight.valid && std::fabs(frontRight.angleDegrees - 45.0F) < 0.01F &&
               frontRight.label == sds::IncomingDirectionLabel::FrontRight,
           "direction preserves continuous diagonal angle and eight-way label");

    const auto turnedFront = sds::calculateIncomingDirection(
        {0,0,0}, 1.57079632679F, {10,0,0});
    expect(turnedFront.valid && std::fabs(turnedFront.angleDegrees) < 0.01F,
           "player heading rotates world direction into local front");

    const auto degenerate = sds::calculateIncomingDirection({0,0,0}, 0.0F, {0,0,10});
    const auto nonFinite = sds::calculateIncomingDirection({0,0,0}, 0.0F, {NAN,1,0});
    expect(!degenerate.valid && degenerate.label == sds::IncomingDirectionLabel::Unknown,
           "vertical-only displacement is unknown");
    expect(!nonFinite.valid && nonFinite.label == sds::IncomingDirectionLabel::Unknown,
           "non-finite direction evidence is unknown");
    expect(std::fabs(sds::incomingDirectionDisagreementDegrees(right, left) - 180.0F) < 0.01F,
           "direction disagreement uses shortest circular distance");

    sds::IncomingDamageDiagnostic diagnostic;
    sds::IncomingDamageEvidence hit{};
    hit.sequence = 1;
    hit.when = t0;
    hit.prePollHealthAvailable = true;
    hit.prePollHealthRatio = 0.90F;
    expect(diagnostic.beginHit(hit), "isolated hit opens correlation record");
    diagnostic.observeHealthSample(0.82F, t0 + 100ms);
    diagnostic.observeHealthSample(0.84F, t0 + 200ms); // regeneration must not erase minimum
    expect(!diagnostic.takeExpired(t0 + 299ms).has_value(), "record does not expire early");
    const auto isolated = diagnostic.takeExpired(t0 + 300ms);
    expect(isolated.has_value() && isolated->sequence == 1 && isolated->healthLossAvailable,
           "record expires exactly at 300 ms with health evidence");
    expect(std::fabs(isolated->minimumHealthRatio - 0.82F) < 0.0001F &&
               std::fabs(isolated->healthLossRatio - 0.08F) < 0.0001F,
           "health correlation tracks minimum and never creates negative damage");
    expect(!isolated->overlap && isolated->overlapCount == 0,
           "isolated hit is not marked ambiguous");

    sds::IncomingDamageEvidence a{}; a.sequence = 2; a.when = t0 + 1s; a.prePollHealthAvailable = true; a.prePollHealthRatio = 0.80F;
    sds::IncomingDamageEvidence b{}; b.sequence = 3; b.when = t0 + 1100ms; b.prePollHealthAvailable = true; b.prePollHealthRatio = 0.79F;
    expect(diagnostic.beginHit(a) && diagnostic.beginHit(b), "overlapping hits both open");
    diagnostic.observeHealthSample(0.70F, t0 + 1200ms);
    const auto firstOverlap = diagnostic.takeExpired(t0 + 1300ms);
    const auto secondOverlap = diagnostic.takeExpired(t0 + 1400ms);
    expect(firstOverlap && firstOverlap->overlap && firstOverlap->overlapCount == 1,
           "older overlapping hit is marked ambiguous");
    expect(secondOverlap && secondOverlap->overlap && secondOverlap->overlapCount == 1,
           "newer overlapping hit is marked ambiguous");

    sds::IncomingDamageDiagnostic bounded;
    for (std::uint64_t i = 0; i < sds::kIncomingDamageMaxActiveRecords; ++i) {
        sds::IncomingDamageEvidence e{}; e.sequence = 100 + i; e.when = t0;
        expect(bounded.beginHit(e), "bounded slot accepts active record");
    }
    sds::IncomingDamageEvidence overflow{}; overflow.sequence = 999; overflow.when = t0;
    expect(!bounded.beginHit(overflow) && bounded.droppedCount() == 1,
           "ninth concurrent hit is rejected and counted");
    while (bounded.takeExpired(t0 + 300ms).has_value()) {}
    expect(bounded.activeCount() == 0, "expired slots are released");
    expect(bounded.beginHit(overflow), "expired slot is reusable");

    sds::IncomingDamageEvidence dual{};
    dual.sequence = 700;
    dual.when = t0 + 2s;
    dual.impactDirection = sds::calculateIncomingDirection({0,0,0}, 0.0F, {10,10,0});
    dual.attackerDirection = sds::calculateIncomingDirection({0,0,0}, 0.0F, {-10,0,0});
    expect(dual.impactDirection.valid && dual.attackerDirection.valid,
           "impact and attacker direction candidates coexist");
    expect(std::fabs(sds::incomingDirectionDisagreementDegrees(
                    dual.impactDirection, dual.attackerDirection) - 135.0F) < 0.01F,
           "candidate disagreement is reported without choosing authority");

    sds::IncomingDamageDiagnostic sameExpiry;
    sds::IncomingDamageEvidence sameA{}; sameA.sequence = 800; sameA.when = t0 + 3s;
    sds::IncomingDamageEvidence sameB{}; sameB.sequence = 801; sameB.when = t0 + 3s;
    expect(sameExpiry.beginHit(sameA) && sameExpiry.beginHit(sameB),
           "same-expiry hits both open");
    const auto sameFirst = sameExpiry.takeExpired(t0 + 3300ms);
    const auto sameSecond = sameExpiry.takeExpired(t0 + 3300ms);
    const auto sameDone = sameExpiry.takeExpired(t0 + 3300ms);
    expect(sameFirst && sameSecond && !sameDone,
           "same-expiry records drain one at a time until empty");

    sds::IncomingDamageDiagnostic noHealth;
    sds::IncomingDamageEvidence unknown{}; unknown.sequence = 500; unknown.when = t0;
    expect(noHealth.beginHit(unknown), "hit remains valid without health baseline");
    const auto unknownSummary = noHealth.takeExpired(t0 + 300ms);
    expect(unknownSummary && !unknownSummary->healthLossAvailable,
           "missing pre-poll health is represented rather than fabricated");

    return failures == 0 ? 0 : 1;
}
