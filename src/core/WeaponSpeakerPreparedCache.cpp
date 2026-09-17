#include <StarfieldDualSense/WeaponSpeakerPreparedCache.h>

#include <algorithm>
#include <utility>

sds::WeaponSpeakerPreparedCache::WeaponSpeakerPreparedCache()
{
    _slots.reserve(weaponSpeakerAudioFamilyCount());
    for (const auto& logical : weaponSpeakerProfiles()) {
        const auto family = speakerAudioFamily(logical);
        if (findFamilySlot(family) == nullptr) {
            _slots.push_back(std::make_unique<Slot>(std::string(family)));
        }
    }
}

sds::WeaponSpeakerPreparedCache::Slot* sds::WeaponSpeakerPreparedCache::findFamilySlot(
    std::string_view familyIdentity) noexcept
{
    for (auto& slot : _slots) {
        if (slot && slot->familyIdentity == familyIdentity) {
            return slot.get();
        }
    }
    return nullptr;
}

const sds::WeaponSpeakerPreparedCache::Slot* sds::WeaponSpeakerPreparedCache::findFamilySlot(
    std::string_view familyIdentity) const noexcept
{
    for (const auto& slot : _slots) {
        if (slot && slot->familyIdentity == familyIdentity) {
            return slot.get();
        }
    }
    return nullptr;
}

bool sds::WeaponSpeakerPreparedCache::completeForCatalog(
    const PreparedWeaponSpeakerFamily& family,
    const WeaponSpeakerProfile& profile) noexcept
{
    if (family.familyIdentity != speakerAudioFamily(profile) || family.cues.size() != profile.cues.size()) {
        return false;
    }

    std::size_t variantCount = 0u;
    for (const auto& expectedCue : profile.cues) {
        const auto cueIt = std::find_if(family.cues.begin(), family.cues.end(), [&](const auto& preparedCue) {
            return preparedCue.action == expectedCue.action;
        });
        if (cueIt == family.cues.end() || cueIt->eventId != expectedCue.mediaEventId ||
            cueIt->variants.size() != expectedCue.variants.size()) {
            return false;
        }

        for (std::size_t index = 0; index < expectedCue.variants.size(); ++index) {
            const auto& expectedVariant = expectedCue.variants[index];
            const auto& preparedVariant = cueIt->variants[index];
            if (preparedVariant.variant != expectedVariant.variant || preparedVariant.mediaId == 0u ||
                preparedVariant.pcm.frames.empty() || preparedVariant.loopResumeFrame != 0u) {
                return false;
            }
        }
        variantCount += cueIt->variants.size();
    }

    if (!profile.sustained) {
        if (family.sustained) {
            return false;
        }
        return variantCount != 0u && family.preparedVariantCount == variantCount;
    }
    if (!family.sustained) {
        return false;
    }

    const auto& expected = *profile.sustained;
    const auto& prepared = *family.sustained;
    if (prepared.startWwiseEventId != expected.startWwiseEventId ||
        prepared.stopWwiseEventId != expected.stopWwiseEventId ||
        prepared.requiredGameObjectId != expected.requiredGameObjectId ||
        prepared.requireZeroExternalSources != expected.requireZeroExternalSources ||
        prepared.loopVariants.size() != expected.loopVariants.size() ||
        prepared.startTransientVariants.size() != expected.startTransientVariants.size() ||
        prepared.stopTransientVariants.size() != expected.stopTransientVariants.size()) {
        return false;
    }

    const auto validate = [](const auto& expectedVariants, const auto& preparedVariants, bool loop) {
        for (std::size_t i = 0; i < expectedVariants.size(); ++i) {
            const auto& expectedVariant = expectedVariants[i];
            const auto& preparedVariant = preparedVariants[i];
            if (preparedVariant.variant != expectedVariant.variant || preparedVariant.mediaId == 0u ||
                preparedVariant.pcm.frames.empty()) {
                return false;
            }
            if (loop) {
                if (preparedVariant.loopResumeFrame == 0u ||
                    preparedVariant.loopResumeFrame >= preparedVariant.pcm.frames.size()) {
                    return false;
                }
            } else if (preparedVariant.loopResumeFrame != 0u) {
                return false;
            }
        }
        return true;
    };
    if (!validate(expected.loopVariants, prepared.loopVariants, true) ||
        !validate(expected.startTransientVariants, prepared.startTransientVariants, false) ||
        !validate(expected.stopTransientVariants, prepared.stopTransientVariants, false)) {
        return false;
    }
    variantCount += prepared.loopVariants.size() + prepared.startTransientVariants.size() +
        prepared.stopTransientVariants.size();
    return variantCount != 0u && family.preparedVariantCount == variantCount;
}

bool sds::WeaponSpeakerPreparedCache::publish(PreparedWeaponSpeakerFamily family) noexcept
{
    try {
        const auto* profile = findWeaponSpeakerProfile(family.familyIdentity);
        auto* slot = findFamilySlot(family.familyIdentity);
        if (!profile || !slot || speakerAudioFamily(*profile) != profile->weaponIdentity ||
            !completeForCatalog(family, *profile)) {
            return false;
        }

        auto snapshot = std::make_shared<const PreparedWeaponSpeakerFamily>(std::move(family));
        slot->snapshot.store(std::move(snapshot), std::memory_order_release);
        return true;
    } catch (...) {
        return false;
    }
}

std::shared_ptr<const sds::PreparedWeaponSpeakerFamily> sds::WeaponSpeakerPreparedCache::find(
    std::string_view logicalWeapon) const noexcept
{
    try {
        const auto* logical = findWeaponSpeakerProfile(logicalWeapon);
        if (!logical) {
            return {};
        }
        const auto* slot = findFamilySlot(speakerAudioFamily(*logical));
        return slot ? slot->snapshot.load(std::memory_order_acquire) : nullptr;
    } catch (...) {
        return {};
    }
}

sds::WeaponSpeakerPreparedCacheStats sds::WeaponSpeakerPreparedCache::stats() const noexcept
{
    try {
        WeaponSpeakerPreparedCacheStats result{};
        result.logicalProfiles = weaponSpeakerProfiles().size();
        result.physicalFamilies = _slots.size();

        for (const auto& slot : _slots) {
            if (!slot) {
                continue;
            }
            const auto snapshot = slot->snapshot.load(std::memory_order_acquire);
            if (snapshot) {
                ++result.readyFamilies;
                result.preparedVariants += snapshot->preparedVariantCount;
            }
        }

        for (const auto& logical : weaponSpeakerProfiles()) {
            const auto* slot = findFamilySlot(speakerAudioFamily(logical));
            if (slot && slot->snapshot.load(std::memory_order_acquire)) {
                ++result.readyProfiles;
            }
        }
        return result;
    } catch (...) {
        return {};
    }
}
