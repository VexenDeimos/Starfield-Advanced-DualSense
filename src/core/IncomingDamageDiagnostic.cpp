#include <StarfieldDualSense/IncomingDamageDiagnostic.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

namespace
{
    [[nodiscard]] float normalizeDegrees(float degrees) noexcept
    {
        while (degrees > 180.0F) {
            degrees -= 360.0F;
        }
        while (degrees <= -180.0F) {
            degrees += 360.0F;
        }
        return degrees;
    }

    [[nodiscard]] sds::IncomingDirectionLabel labelForAngle(float degrees) noexcept
    {
        if (degrees >= -22.5F && degrees < 22.5F) return sds::IncomingDirectionLabel::Front;
        if (degrees >= 22.5F && degrees < 67.5F) return sds::IncomingDirectionLabel::FrontRight;
        if (degrees >= 67.5F && degrees < 112.5F) return sds::IncomingDirectionLabel::Right;
        if (degrees >= 112.5F && degrees < 157.5F) return sds::IncomingDirectionLabel::BackRight;
        if (degrees >= 157.5F || degrees < -157.5F) return sds::IncomingDirectionLabel::Back;
        if (degrees >= -157.5F && degrees < -112.5F) return sds::IncomingDirectionLabel::BackLeft;
        if (degrees >= -112.5F && degrees < -67.5F) return sds::IncomingDirectionLabel::Left;
        return sds::IncomingDirectionLabel::FrontLeft;
    }

    [[nodiscard]] bool finitePoint(sds::IncomingWorldPoint point) noexcept
    {
        return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
    }

    [[nodiscard]] bool usefulAttackerEvidence(const sds::IncomingDamageEvidence& evidence) noexcept
    {
        return evidence.attackerDirection.valid ||
            (evidence.causePresent && evidence.causeFormId != 0 &&
             evidence.causeFormId != evidence.targetFormId);
    }

    void mergeEvidence(
        sds::IncomingDamageEvidence& destination,
        const sds::IncomingDamageEvidence& source) noexcept
    {
        if (usefulAttackerEvidence(source) && !usefulAttackerEvidence(destination)) {
            destination.causePresent = source.causePresent;
            destination.causeFormId = source.causeFormId;
            destination.attackerDirection = source.attackerDirection;
        } else if (!destination.attackerDirection.valid && source.attackerDirection.valid) {
            destination.attackerDirection = source.attackerDirection;
        }

        if (!destination.impactPresent && source.impactPresent) {
            destination.impact = source.impact;
            destination.impactPresent = true;
            destination.impactDirection = source.impactDirection;
        }
        if (!destination.usesHitData && source.usesHitData) {
            destination.usesHitData = true;
            destination.aggressorHandle = source.aggressorHandle;
            destination.hitTargetHandle = source.hitTargetHandle;
            destination.sourceRefHandle = source.sourceRefHandle;
            destination.hitWeaponFormId = source.hitWeaponFormId;
            destination.ammoFormId = source.ammoFormId;
            destination.attackData = source.attackData;
            destination.material = source.material;
        }
        if (destination.sourceFormId == 0 && source.sourceFormId != 0) {
            destination.sourceFormId = source.sourceFormId;
        }
        if (destination.projectileFormId == 0 && source.projectileFormId != 0) {
            destination.projectileFormId = source.projectileFormId;
        }
        if (source.directHealthAvailable && std::isfinite(source.directHealthRatio) &&
            (!destination.directHealthAvailable || !std::isfinite(destination.directHealthRatio) ||
             source.directHealthRatio < destination.directHealthRatio)) {
            destination.directHealthAvailable = true;
            destination.directHealthRatio = source.directHealthRatio;
        }
    }
}

sds::IncomingDirectionCandidate sds::calculateIncomingDirection(
    IncomingWorldPoint player,
    float playerHeadingRadians,
    IncomingWorldPoint source) noexcept
{
    if (!finitePoint(player) || !finitePoint(source) || !std::isfinite(playerHeadingRadians)) {
        return {};
    }

    const float dx = source.x - player.x;
    const float dy = source.y - player.y;
    const float horizontalDistanceSquared = dx * dx + dy * dy;
    if (!std::isfinite(horizontalDistanceSquared) || horizontalDistanceSquared <= 1.0e-6F) {
        return {};
    }

    const float worldAngle = std::atan2(dx, dy);
    const float localDegrees = normalizeDegrees(
        (worldAngle - playerHeadingRadians) * 180.0F / std::numbers::pi_v<float>);

    return {
        .valid = true,
        .angleDegrees = localDegrees,
        .label = labelForAngle(localDegrees),
    };
}

float sds::incomingDirectionDisagreementDegrees(
    IncomingDirectionCandidate first,
    IncomingDirectionCandidate second) noexcept
{
    if (!first.valid || !second.valid || !std::isfinite(first.angleDegrees) || !std::isfinite(second.angleDegrees)) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    const float delta = std::fabs(normalizeDegrees(first.angleDegrees - second.angleDegrees));
    return std::min(delta, 360.0F - delta);
}

std::string_view sds::incomingDirectionLabelName(IncomingDirectionLabel label) noexcept
{
    switch (label) {
    case IncomingDirectionLabel::Front: return "Front";
    case IncomingDirectionLabel::FrontRight: return "FrontRight";
    case IncomingDirectionLabel::Right: return "Right";
    case IncomingDirectionLabel::BackRight: return "BackRight";
    case IncomingDirectionLabel::Back: return "Back";
    case IncomingDirectionLabel::BackLeft: return "BackLeft";
    case IncomingDirectionLabel::Left: return "Left";
    case IncomingDirectionLabel::FrontLeft: return "FrontLeft";
    case IncomingDirectionLabel::Unknown:
    default:
        return "Unknown";
    }
}

bool sds::IncomingDamageDiagnostic::beginHit(const IncomingDamageEvidence& evidence) noexcept
{
    std::size_t existingCount = 0;
    for (auto& existing : _records) {
        if (!existing.active) {
            continue;
        }
        existing.overlap = true;
        ++existing.overlapCount;
        ++existingCount;
    }

    auto slot = std::find_if(_records.begin(), _records.end(), [](const ActiveRecord& record) {
        return !record.active;
    });
    if (slot == _records.end()) {
        ++_dropped;
        return false;
    }

    *slot = {};
    slot->active = true;
    slot->evidence = evidence;
    slot->overlap = existingCount != 0;
    slot->overlapCount = static_cast<std::uint32_t>(existingCount);
    if (evidence.prePollHealthAvailable && std::isfinite(evidence.prePollHealthRatio)) {
        slot->minimumHealthAvailable = true;
        slot->minimumHealthRatio = std::clamp(evidence.prePollHealthRatio, 0.0F, 1.0F);
    }
    if (evidence.directHealthAvailable && std::isfinite(evidence.directHealthRatio)) {
        const float direct = std::clamp(evidence.directHealthRatio, 0.0F, 1.0F);
        if (!slot->minimumHealthAvailable || direct < slot->minimumHealthRatio) {
            slot->minimumHealthAvailable = true;
            slot->minimumHealthRatio = direct;
        }
    }
    return true;
}

sds::IncomingDamageBeginResult sds::IncomingDamageDiagnostic::beginOrMergeHit(
    const IncomingDamageEvidence& evidence) noexcept
{
    ActiveRecord* mergeTarget = nullptr;
    for (auto& record : _records) {
        if (!record.active || evidence.when < record.evidence.when ||
            evidence.when > record.evidence.when + kIncomingDamageDuplicateWindow) {
            continue;
        }
        if (!mergeTarget || record.evidence.when > mergeTarget->evidence.when) {
            mergeTarget = &record;
        }
    }

    if (mergeTarget) {
        if (evidence.directHealthAvailable && std::isfinite(evidence.directHealthRatio)) {
            const float direct = std::clamp(evidence.directHealthRatio, 0.0F, 1.0F);
            if (!mergeTarget->minimumHealthAvailable || direct < mergeTarget->minimumHealthRatio) {
                mergeTarget->minimumHealthAvailable = true;
                mergeTarget->minimumHealthRatio = direct;
            }
        }
        mergeEvidence(mergeTarget->evidence, evidence);
        ++mergeTarget->mergedEventCount;
        return {
            .disposition = IncomingDamageBeginDisposition::Merged,
            .incidentSequence = mergeTarget->evidence.sequence,
            .mergedEventCount = mergeTarget->mergedEventCount,
        };
    }

    std::size_t existingCount = 0;
    for (auto& existing : _records) {
        if (!existing.active) {
            continue;
        }
        existing.overlap = true;
        ++existing.overlapCount;
        ++existingCount;
    }

    auto slot = std::find_if(_records.begin(), _records.end(), [](const ActiveRecord& record) {
        return !record.active;
    });
    if (slot == _records.end()) {
        ++_dropped;
        return {
            .disposition = IncomingDamageBeginDisposition::Dropped,
            .incidentSequence = evidence.sequence,
            .mergedEventCount = 0,
        };
    }

    *slot = {};
    slot->active = true;
    slot->evidence = evidence;
    slot->overlap = existingCount != 0;
    slot->overlapCount = static_cast<std::uint32_t>(existingCount);
    slot->mergedEventCount = 1;
    if (evidence.prePollHealthAvailable && std::isfinite(evidence.prePollHealthRatio)) {
        slot->minimumHealthAvailable = true;
        slot->minimumHealthRatio = std::clamp(evidence.prePollHealthRatio, 0.0F, 1.0F);
    }
    if (evidence.directHealthAvailable && std::isfinite(evidence.directHealthRatio)) {
        const float direct = std::clamp(evidence.directHealthRatio, 0.0F, 1.0F);
        if (!slot->minimumHealthAvailable || direct < slot->minimumHealthRatio) {
            slot->minimumHealthAvailable = true;
            slot->minimumHealthRatio = direct;
        }
    }

    return {
        .disposition = IncomingDamageBeginDisposition::Started,
        .incidentSequence = evidence.sequence,
        .mergedEventCount = 1,
    };
}

void sds::IncomingDamageDiagnostic::observeHealthSample(
    float ratio,
    std::chrono::steady_clock::time_point when) noexcept
{
    if (!std::isfinite(ratio)) {
        return;
    }

    const float boundedRatio = std::clamp(ratio, 0.0F, 1.0F);
    for (auto& record : _records) {
        if (!record.active || when < record.evidence.when ||
            when > record.evidence.when + kIncomingDamageCorrelationWindow) {
            continue;
        }

        if (!record.minimumHealthAvailable || boundedRatio < record.minimumHealthRatio) {
            record.minimumHealthAvailable = true;
            record.minimumHealthRatio = boundedRatio;
        }
    }
}

std::optional<sds::IncomingDamageConfirmation> sds::IncomingDamageDiagnostic::confirmHealthDrop(
    float previousRatio,
    float currentRatio,
    std::chrono::steady_clock::time_point when) noexcept
{
    if (!std::isfinite(previousRatio) || !std::isfinite(currentRatio)) {
        return std::nullopt;
    }

    const float previous = std::clamp(previousRatio, 0.0F, 1.0F);
    const float current = std::clamp(currentRatio, 0.0F, 1.0F);
    const float immediateLoss = std::max(0.0F, previous - current);

    float cumulativeLoss = 0.0F;
    bool hasEligibleRecord = false;
    for (const auto& record : _records) {
        if (!record.active || when < record.evidence.when ||
            when > record.evidence.when + kIncomingDamageCorrelationWindow) {
            continue;
        }

        hasEligibleRecord = true;
        if (record.evidence.prePollHealthAvailable && record.minimumHealthAvailable &&
            std::isfinite(record.evidence.prePollHealthRatio) &&
            std::isfinite(record.minimumHealthRatio)) {
            const float baseline = std::clamp(record.evidence.prePollHealthRatio, 0.0F, 1.0F);
            const float incidentLoss = std::max(0.0F, baseline - record.minimumHealthRatio);
            cumulativeLoss = std::max(cumulativeLoss, std::max(incidentLoss, immediateLoss));
        } else {
            cumulativeLoss = std::max(cumulativeLoss, immediateLoss);
        }
    }

    if (!hasEligibleRecord || cumulativeLoss < kIncomingDamageMinimumConfirmedLoss) {
        return std::nullopt;
    }

    IncomingDamageConfirmation confirmation{};
    bool found = false;
    for (auto& record : _records) {
        if (!record.active || when < record.evidence.when ||
            when > record.evidence.when + kIncomingDamageCorrelationWindow) {
            continue;
        }

        if (!found) {
            confirmation.sequence = record.evidence.sequence;
            confirmation.attackerDirection = record.evidence.attackerDirection;
            found = true;
        } else if (!confirmation.attackerDirection.valid && record.evidence.attackerDirection.valid) {
            confirmation.attackerDirection = record.evidence.attackerDirection;
        }
        ++confirmation.incidentCount;
        confirmation.mergedEventCount += record.mergedEventCount;
        record = {};
    }

    if (!found) {
        return std::nullopt;
    }

    confirmation.healthLossRatio = cumulativeLoss;
    confirmation.attributionAmbiguous = confirmation.incidentCount > 1;
    return confirmation;
}

std::optional<sds::IncomingDamageCorrelationSummary> sds::IncomingDamageDiagnostic::takeExpired(
    std::chrono::steady_clock::time_point now) noexcept
{
    for (auto& record : _records) {
        if (!record.active || now < record.evidence.when + kIncomingDamageCorrelationWindow) {
            continue;
        }

        IncomingDamageCorrelationSummary summary{};
        summary.sequence = record.evidence.sequence;
        summary.prePollHealthRatio = record.evidence.prePollHealthRatio;
        summary.directHealthAvailable = record.evidence.directHealthAvailable;
        summary.directHealthRatio = record.evidence.directHealthRatio;
        summary.minimumHealthAvailable = record.minimumHealthAvailable;
        summary.minimumHealthRatio = record.minimumHealthRatio;
        summary.overlap = record.overlap;
        summary.overlapCount = record.overlapCount;
        summary.healthLossAvailable = record.evidence.prePollHealthAvailable && record.minimumHealthAvailable;
        if (summary.healthLossAvailable) {
            const float baseline = std::clamp(record.evidence.prePollHealthRatio, 0.0F, 1.0F);
            summary.healthLossRatio = std::max(0.0F, baseline - record.minimumHealthRatio);
        }

        record = {};
        return summary;
    }
    return std::nullopt;
}

std::size_t sds::IncomingDamageDiagnostic::activeCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(_records.begin(), _records.end(), [](const ActiveRecord& record) {
        return record.active;
    }));
}

bool sds::IncomingDamageNullTargetFallback::armTesHit(
    std::uint64_t sequence,
    std::chrono::steady_clock::time_point when,
    bool targetMissing,
    bool causeIsPlayer,
    bool gameplayHudActive) noexcept
{
    if (!targetMissing || causeIsPlayer || !gameplayHudActive) {
        return false;
    }

    _pending = true;
    _sequence = sequence;
    _when = when;
    return true;
}

std::optional<sds::IncomingDamageFallbackConfirmation>
sds::IncomingDamageNullTargetFallback::confirmHealthDrop(
    float previousRatio,
    float currentRatio,
    std::chrono::steady_clock::time_point when,
    bool primaryIncidentActive) noexcept
{
    if (primaryIncidentActive) {
        clear();
        return std::nullopt;
    }
    if (!_pending) {
        return std::nullopt;
    }
    if (when < _when || when > _when + kIncomingDamageNullTargetFallbackWindow) {
        clear();
        return std::nullopt;
    }
    if (!std::isfinite(previousRatio) || !std::isfinite(currentRatio)) {
        return std::nullopt;
    }

    const float previous = std::clamp(previousRatio, 0.0F, 1.0F);
    const float current = std::clamp(currentRatio, 0.0F, 1.0F);
    const float loss = std::max(0.0F, previous - current);
    if (loss < kIncomingDamageMinimumConfirmedLoss) {
        return std::nullopt;
    }

    IncomingDamageFallbackConfirmation confirmation{};
    confirmation.sequence = _sequence;
    confirmation.healthLossRatio = loss;
    confirmation.ageMs = std::chrono::duration_cast<std::chrono::milliseconds>(when - _when).count();
    clear();
    return confirmation;
}

void sds::IncomingDamageNullTargetFallback::clear() noexcept
{
    _pending = false;
    _sequence = 0;
    _when = {};
}
