#include <StarfieldDualSense/HapticsEngine.h>
#include <StarfieldDualSense/HapticWaveforms.h>
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

    sds::GameEvent incomingDamage(float loss, std::chrono::steady_clock::time_point when)
    {
        sds::GameEvent event{};
        event.type = sds::GameEventType::IncomingDamage;
        event.value = loss;
        event.when = when;
        return event;
    }

    float actuatorRms(const sds::HapticWaveform& frames)
    {
        if (frames.empty()) {
            return 0.0F;
        }
        double sum = 0.0;
        for (const auto& frame : frames) {
            sum += static_cast<double>(frame[2]) * frame[2];
        }
        return static_cast<float>(std::sqrt(sum / static_cast<double>(frames.size())));
    }
}

int main()
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::steady_clock;
    const auto t0 = Clock::time_point{ 20s };

    sds::IncomingDamageDiagnostic diagnostic;

    sds::IncomingDamageEvidence sparse{};
    sparse.sequence = 1;
    sparse.when = t0;
    sparse.targetFormId = 0x14;
    sparse.causePresent = true;
    sparse.causeFormId = 0x14;
    sparse.prePollHealthAvailable = true;
    sparse.prePollHealthRatio = 0.84F;

    const auto first = diagnostic.beginOrMergeHit(sparse);
    expect(first.disposition == sds::IncomingDamageBeginDisposition::Started &&
               first.incidentSequence == 1 && first.mergedEventCount == 1 &&
               diagnostic.activeCount() == 1,
           "first TESHit opens one incoming-damage incident");

    sds::IncomingDamageEvidence useful = sparse;
    useful.sequence = 2;
    useful.when = t0 + 1ms;
    useful.causeFormId = 0xFF019AD4;
    useful.attackerDirection = sds::IncomingDirectionCandidate{
        .valid = true,
        .angleDegrees = 18.0F,
        .label = sds::IncomingDirectionLabel::Front,
    };
    const auto merged = diagnostic.beginOrMergeHit(useful);
    expect(merged.disposition == sds::IncomingDamageBeginDisposition::Merged &&
               merged.incidentSequence == 1 && merged.mergedEventCount == 2 &&
               diagnostic.activeCount() == 1,
           "TESHit duplicate inside five milliseconds coalesces into existing incident");

    const auto confirmed = diagnostic.confirmHealthDrop(0.84F, 0.59F, t0 + 100ms);
    expect(confirmed && confirmed->sequence == 1 &&
               std::fabs(confirmed->healthLossRatio - 0.25F) < 0.0001F,
           "first real health decrease confirms incident severity immediately");
    expect(confirmed && confirmed->incidentCount == 1 && confirmed->mergedEventCount == 2 &&
               !confirmed->attributionAmbiguous,
           "coalesced TESHit pair produces one confirmed incident rather than ambiguous duplicates");
    expect(confirmed && confirmed->attackerDirection.valid &&
               std::fabs(confirmed->attackerDirection.angleDegrees - 18.0F) < 0.01F,
           "coalescing keeps the useful attacker direction from the richer duplicate");
    expect(diagnostic.activeCount() == 0,
           "confirmed incident is retired so one health loss cannot retrigger it");

    sds::IncomingDamageEvidence noDamage{};
    noDamage.sequence = 3;
    noDamage.when = t0 + 1s;
    noDamage.prePollHealthAvailable = true;
    noDamage.prePollHealthRatio = 0.65F;
    expect(diagnostic.beginOrMergeHit(noDamage).disposition == sds::IncomingDamageBeginDisposition::Started,
           "zero-damage TESHit can arm an incident");
    expect(!diagnostic.confirmHealthDrop(0.65F, 0.65F, t0 + 1100ms),
           "unchanged health never confirms damage");
    const auto expiredNoDamage = diagnostic.takeExpired(t0 + 1300ms);
    expect(expiredNoDamage && std::fabs(expiredNoDamage->healthLossRatio) < 0.0001F,
           "unconfirmed zero-damage incident expires without fabricated severity");

    sds::IncomingDamageEvidence roundA{};
    roundA.sequence = 10;
    roundA.when = t0 + 2s;
    roundA.prePollHealthAvailable = true;
    roundA.prePollHealthRatio = 0.80F;
    sds::IncomingDamageEvidence roundB = roundA;
    roundB.sequence = 11;
    roundB.when = t0 + 2010ms;
    expect(diagnostic.beginOrMergeHit(roundA).disposition == sds::IncomingDamageBeginDisposition::Started &&
               diagnostic.beginOrMergeHit(roundB).disposition == sds::IncomingDamageBeginDisposition::Started &&
               diagnostic.activeCount() == 2,
           "TESHit events outside duplicate window remain separate incidents");
    const auto burst = diagnostic.confirmHealthDrop(0.80F, 0.70F, t0 + 2100ms);
    expect(burst && burst->incidentCount == 2 && burst->attributionAmbiguous &&
               std::fabs(burst->healthLossRatio - 0.10F) < 0.0001F,
           "one health poll confirms an ambiguous rapid-hit group only once");
    expect(diagnostic.activeCount() == 0,
           "all incidents covered by one confirmed health drop are retired together");

    sds::IncomingDamageEvidence cumulative{};
    cumulative.sequence = 20;
    cumulative.when = t0 + 500ms;
    cumulative.prePollHealthAvailable = true;
    cumulative.prePollHealthRatio = 0.5000F;
    expect(diagnostic.beginOrMergeHit(cumulative).disposition == sds::IncomingDamageBeginDisposition::Started,
           "ordinary damage can arm an incident before any one poll crosses the threshold");
    diagnostic.observeHealthSample(0.4990F, t0 + 550ms);
    expect(!diagnostic.confirmHealthDrop(0.5000F, 0.4990F, t0 + 550ms),
           "one tenth percent poll drop stays below confirmation threshold");
    diagnostic.observeHealthSample(0.4970F, t0 + 650ms);
    const auto cumulativeConfirmed = diagnostic.confirmHealthDrop(0.4990F, 0.4970F, t0 + 650ms);
    expect(cumulativeConfirmed &&
               std::fabs(cumulativeConfirmed->healthLossRatio - 0.0030F) < 0.0001F,
           "multiple sub-threshold polls confirm once cumulative incident loss exceeds quarter percent");
    expect(diagnostic.activeCount() == 0,
           "cumulatively confirmed incident retires immediately after its first pulse");

    sds::IncomingDamageEvidence preEvent{};
    preEvent.sequence = 21;
    preEvent.when = t0 + 800ms;
    preEvent.prePollHealthAvailable = true;
    preEvent.prePollHealthRatio = 0.6000F;
    preEvent.directHealthAvailable = true;
    preEvent.directHealthRatio = 0.5970F;
    expect(diagnostic.beginOrMergeHit(preEvent).disposition == sds::IncomingDamageBeginDisposition::Started,
           "incident keeps the last periodic health sample as its pre-hit baseline");
    const auto eventTimeConfirmed = diagnostic.confirmHealthDrop(0.6000F, 0.5995F, t0 + 825ms);
    expect(eventTimeConfirmed &&
               std::fabs(eventTimeConfirmed->healthLossRatio - 0.0030F) < 0.0001F,
           "event-time direct health is retained as an observed minimum even if the next poll has already healed upward");

    preEvent.sequence = 22;
    preEvent.when = t0 + 900ms;
    expect(diagnostic.beginOrMergeHit(preEvent).disposition == sds::IncomingDamageBeginDisposition::Started,
           "second pre-event case arms independently after direct-health confirmation retires the first");
    diagnostic.observeHealthSample(0.5970F, t0 + 950ms);
    const auto preEventConfirmed = diagnostic.confirmHealthDrop(0.5970F, 0.5970F, t0 + 950ms);
    expect(preEventConfirmed &&
               std::fabs(preEventConfirmed->healthLossRatio - 0.0030F) < 0.0001F,
           "damage already applied before TESHit is confirmed from pre-poll baseline even with a flat next poll");

    sds::HapticsEngine engine(1.0F);
    const auto tiny = engine.handle(incomingDamage(0.0035F, t0 + 3s));
    const auto light = engine.handle(incomingDamage(0.02F, t0 + 4s));
    const auto medium = engine.handle(incomingDamage(0.10F, t0 + 5s));
    const auto heavy = engine.handle(incomingDamage(0.23F, t0 + 6s));
    const auto severe = engine.handle(incomingDamage(0.25F, t0 + 7s));
    expect(tiny && light && medium && heavy && severe &&
               tiny->kind == sds::HapticEffectKind::IncomingDamageImpact &&
               light->kind == sds::HapticEffectKind::IncomingDamageImpact &&
               medium->kind == sds::HapticEffectKind::IncomingDamageImpact &&
               heavy->kind == sds::HapticEffectKind::IncomingDamageImpact &&
               severe->kind == sds::HapticEffectKind::IncomingDamageImpact,
           "incoming damage maps to generic impact haptics without requiring an equipped weapon");
    expect(tiny && light && medium && heavy && severe &&
               tiny->gain < light->gain && light->gain < medium->gain &&
               medium->gain < heavy->gain && heavy->gain < severe->gain,
           "health-loss severity continuously increases incoming haptic gain");
    expect(tiny && std::fabs(tiny->gain - 0.4675F) < 0.002F &&
               light && std::fabs(light->gain - 0.550F) < 0.002F &&
               medium && std::fabs(medium->gain - 0.680F) < 0.002F &&
               heavy && std::fabs(heavy->gain - 0.916F) < 0.002F &&
               severe && std::fabs(severe->gain - 0.980F) < 0.002F,
           "v0.3.15 raises tiny and medium hit gain while preserving the proven heavy-hit curve");

    const auto tinyWave = tiny ? sds::synthesizeHapticEffect(*tiny) : sds::HapticWaveform{};
    const auto lightWave = light ? sds::synthesizeHapticEffect(*light) : sds::HapticWaveform{};
    const auto heavyWave = heavy ? sds::synthesizeHapticEffect(*heavy) : sds::HapticWaveform{};
    const auto severeWave = severe ? sds::synthesizeHapticEffect(*severe) : sds::HapticWaveform{};
    expect(tinyWave.size() == 1536 && lightWave.size() == 1536,
           "tiny and light incoming hits use a fixed sharp thirty-two millisecond pulse");
    expect(!tinyWave.empty() && actuatorRms(tinyWave) > 0.18F,
           "runtime-scale tiny hits now carry a perceptible actuator-energy floor");
    float tinyPeak = 0.0F;
    for (const auto& frame : tinyWave) {
        tinyPeak = std::max(tinyPeak, std::fabs(frame[2]));
    }
    expect(tinyPeak > 0.80F,
           "tiny incoming hits are front-loaded with a strong tactile knock rather than a soft buzz");
    expect(!heavyWave.empty() && !severeWave.empty() && severeWave.size() > heavyWave.size(),
           "larger incoming hits retain the longer low-frequency body");
    expect(actuatorRms(severeWave) > actuatorRms(lightWave),
           "larger incoming hits have greater actuator energy");
    bool centered = !severeWave.empty();
    for (const auto& frame : severeWave) {
        centered = centered && frame[0] == 0.0F && frame[1] == 0.0F &&
            std::fabs(frame[2] - frame[3]) < 0.000001F;
    }
    expect(centered,
           "v0.3.15 incoming damage stays centered on both haptic actuators and never touches speaker lanes");

    return failures == 0 ? 0 : 1;
}
