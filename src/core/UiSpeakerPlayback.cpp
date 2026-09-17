#include <StarfieldDualSense/UiSpeakerPlayback.h>

#include <StarfieldDualSense/UiAudioCandidateCatalog.h>

#include <string_view>
#include <utility>

namespace
{
    constexpr std::uint32_t kWeaponsCraftingMenu = 1u << 0u;
    constexpr std::uint32_t kArmorCraftingMenu = 1u << 1u;
    constexpr std::uint32_t kIndustrialCraftingMenu = 1u << 2u;
    constexpr std::uint32_t kFoodCraftingMenu = 1u << 3u;
    constexpr std::uint32_t kDrugsCraftingMenu = 1u << 4u;
    constexpr std::uint32_t kResearchMenu = 1u << 5u;

    [[nodiscard]] constexpr std::uint32_t craftingMenuBit(std::string_view menu) noexcept
    {
        if (menu == "WeaponsCraftingMenu") {
            return kWeaponsCraftingMenu;
        }
        if (menu == "ArmorCraftingMenu") {
            return kArmorCraftingMenu;
        }
        if (menu == "IndustrialCraftingMenu") {
            return kIndustrialCraftingMenu;
        }
        if (menu == "FoodCraftingMenu") {
            return kFoodCraftingMenu;
        }
        if (menu == "DrugsCraftingMenu") {
            return kDrugsCraftingMenu;
        }
        if (menu == "ResearchMenu") {
            return kResearchMenu;
        }
        return 0u;
    }

    [[nodiscard]] constexpr bool isSharedGeneralCue(std::uint32_t eventId) noexcept
    {
        return eventId == 0x05234A32u || // UIMenuGeneralFocus
            eventId == 0x5C8034FCu ||   // UIMenuGeneralOK
            eventId == 0x7956E9B0u;     // UIMenuGeneralCancel
    }
}

sds::UiSpeakerPlayback::UiSpeakerPlayback(
    SubmitCallback submit,
    std::shared_ptr<UiSpeakerPreparedCache> preparedCache,
    Config,
    LogCallback log) :
    _submit(std::move(submit)),
    _preparedCache(std::move(preparedCache)),
    _log(std::move(log))
{}

bool sds::UiSpeakerPlayback::observeWwise(const UiAudioWwiseObservation& observation) noexcept
{
    _observed.fetch_add(1u, std::memory_order_relaxed);
    if (_shuttingDown.load(std::memory_order_acquire)) {
        _shutdownRejected.fetch_add(1u, std::memory_order_relaxed);
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }
    if (!_submit || !_preparedCache) {
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }

    const auto* definition = findUiSpeakerCueDefinition(observation.eventId);
    if (!definition) {
        _unknownEvent.fetch_add(1u, std::memory_order_relaxed);
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }
    if (definition->requiredGameObjectId != 0u &&
        observation.gameObjectId != definition->requiredGameObjectId) {
        _wrongGameObject.fetch_add(1u, std::memory_order_relaxed);
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }
    if (observation.externalCount != 0u || observation.hasExternalSources) {
        _externalSource.fetch_add(1u, std::memory_order_relaxed);
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }

    const auto prepared = _preparedCache->find(observation.eventId);
    if (!prepared) {
        _unprepared.fetch_add(1u, std::memory_order_relaxed);
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }

    const PreparedSpeakerPcm* pcm = &prepared->pcm;
    std::uint32_t mediaId = prepared->mediaId;
    if (!prepared->variants.empty()) {
        const auto sequence = _variantSequence.fetch_add(1u, std::memory_order_relaxed);
        const auto& variant = prepared->variants[
            static_cast<std::size_t>(sequence % prepared->variants.size())];
        pcm = &variant.pcm;
        mediaId = variant.mediaId;
    }
    if (!pcm || pcm->frames.empty() || mediaId == 0u) {
        _unprepared.fetch_add(1u, std::memory_order_relaxed);
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }

    SpeakerCategory category = definition->category;
    if (isSharedGeneralCue(observation.eventId)) {
        // SecurityMenu has dedicated authored Digipick events for selection,
        // rotation, insertion, success, enter, and exit. Suppress the shared
        // General* layer while it is active so the controller never doubles
        // a generic UI click on top of the dedicated Digipick click.
        if (_securityMenuActive.load(std::memory_order_acquire)) {
            _rejected.fetch_add(1u, std::memory_order_relaxed);
            return false;
        }
        if (_craftingMenuMask.load(std::memory_order_acquire) != 0u) {
            category = SpeakerCategory::Crafting;
        }
    }

    try {
        const bool accepted = _submit(*pcm, prepared->eventId, mediaId, category);
        if (accepted) {
            _submitted.fetch_add(1u, std::memory_order_relaxed);
        } else {
            _rejected.fetch_add(1u, std::memory_order_relaxed);
        }
        return accepted;
    } catch (...) {
        _rejected.fetch_add(1u, std::memory_order_relaxed);
        return false;
    }
}

void sds::UiSpeakerPlayback::observeGameEvent(const GameEvent& event) noexcept
{
    if (event.type != GameEventType::MenuOpened && event.type != GameEventType::MenuClosed) {
        return;
    }

    const auto menu = std::string_view(event.text.data());
    if (menu == "SecurityMenu") {
        _securityMenuActive.store(
            event.type == GameEventType::MenuOpened,
            std::memory_order_release);
        return;
    }

    const auto bit = craftingMenuBit(menu);
    if (bit == 0u) {
        return;
    }

    if (event.type == GameEventType::MenuOpened) {
        _craftingMenuMask.fetch_or(bit, std::memory_order_acq_rel);
    } else {
        _craftingMenuMask.fetch_and(~bit, std::memory_order_acq_rel);
    }
}

void sds::UiSpeakerPlayback::beginShutdown() noexcept
{
    _shuttingDown.store(true, std::memory_order_release);
    _craftingMenuMask.store(0u, std::memory_order_release);
    _securityMenuActive.store(false, std::memory_order_release);
}

bool sds::UiSpeakerPlayback::armed() const noexcept
{
    if (_shuttingDown.load(std::memory_order_acquire) || !_preparedCache || !_submit) {
        return false;
    }
    return _preparedCache->stats().readyCues != 0u;
}

sds::UiSpeakerPlaybackStats sds::UiSpeakerPlayback::stats() const noexcept
{
    return {
        .observed = _observed.load(std::memory_order_relaxed),
        .submitted = _submitted.load(std::memory_order_relaxed),
        .rejected = _rejected.load(std::memory_order_relaxed),
        .unknownEvent = _unknownEvent.load(std::memory_order_relaxed),
        .wrongGameObject = _wrongGameObject.load(std::memory_order_relaxed),
        .externalSource = _externalSource.load(std::memory_order_relaxed),
        .unprepared = _unprepared.load(std::memory_order_relaxed),
        .shutdownRejected = _shutdownRejected.load(std::memory_order_relaxed),
    };
}
