#include <StarfieldDualSense/NativeInputInjection.h>

#include <array>
#include <bit>

namespace
{
    using namespace std::chrono_literals;

    constexpr std::array<sds::NativeInputPulseFrame, 5> kQuickInventoryPulse{
        sds::NativeInputPulseFrame{
            .edge = sds::NativeInputEdge::Press,
            .status = 0,
            .value = 1.0F,
            .heldDownSecs = 0.000F,
            .previousHeldDownSecs = 0.000F,
            .offset = 0ms,
            .stepIndex = 1,
        },
        sds::NativeInputPulseFrame{
            .edge = sds::NativeInputEdge::Press,
            .status = 0,
            .value = 1.0F,
            .heldDownSecs = 0.015F,
            .previousHeldDownSecs = 0.000F,
            .offset = 15ms,
            .stepIndex = 2,
        },
        sds::NativeInputPulseFrame{
            .edge = sds::NativeInputEdge::Press,
            .status = 0,
            .value = 1.0F,
            .heldDownSecs = 0.030F,
            .previousHeldDownSecs = 0.015F,
            .offset = 30ms,
            .stepIndex = 3,
        },
        sds::NativeInputPulseFrame{
            .edge = sds::NativeInputEdge::Press,
            .status = 0,
            .value = 1.0F,
            .heldDownSecs = 0.045F,
            .previousHeldDownSecs = 0.030F,
            .offset = 45ms,
            .stepIndex = 4,
        },
        sds::NativeInputPulseFrame{
            .edge = sds::NativeInputEdge::Release,
            .status = 0,
            .value = 0.0F,
            .heldDownSecs = 0.045F,
            .previousHeldDownSecs = 0.045F,
            .offset = 60ms,
            .stepIndex = 5,
        },
    };

    constexpr sds::NativeInputActionDefinition kOpenInventoryDefinition{
        .action = sds::InputAction::OpenInventory,
        .userEvent = "QuickInventory",
        .deviceType = 2,
        .deviceId = 0,
        .eventType = 0,
        .idCode = 73,
        .disabled = false,
        .pulse = kQuickInventoryPulse,
    };

    constexpr sds::NativeInputActionDefinition kOpenMissionsDefinition{
        .action = sds::InputAction::OpenMissions,
        .userEvent = "QuickMission",
        .deviceType = 2,
        .deviceId = 0,
        .eventType = 0,
        .idCode = 76,
        .disabled = false,
        .pulse = kQuickInventoryPulse,
    };

    constexpr sds::NativeInputActionDefinition kOpenMapDefinition{
        .action = sds::InputAction::OpenMap,
        .userEvent = "QuickMap",
        .deviceType = 2,
        .deviceId = 0,
        .eventType = 0,
        .idCode = 77,
        .disabled = false,
        .pulse = kQuickInventoryPulse,
    };

    constexpr sds::NativeInputActionDefinition kOpenPowersDefinition{
        .action = sds::InputAction::OpenPowers,
        .userEvent = "QuickPowers",
        .deviceType = 2,
        .deviceId = 0,
        .eventType = 0,
        .idCode = 75,
        .disabled = false,
        .pulse = kQuickInventoryPulse,
    };

    constexpr sds::NativeInputActionDefinition kOpenSkillsDefinition{
        .action = sds::InputAction::OpenSkills,
        .userEvent = "QuickSkills",
        .deviceType = 2,
        .deviceId = 0,
        .eventType = 0,
        .idCode = 80,
        .disabled = false,
        .pulse = kQuickInventoryPulse,
    };

    constexpr sds::NativeInputActionDefinition kOpenPhotoModeDefinition{
        .action = sds::InputAction::OpenPhotoMode,
        .userEvent = "Monocle",
        .deviceType = 2,
        .deviceId = 0,
        .eventType = 0,
        .idCode = 70,
        .disabled = false,
        .pulse = kQuickInventoryPulse,
    };

    constexpr sds::NativeInputActionDefinition kEnterPhotoModeDefinition{
        .action = sds::InputAction::OpenPhotoMode,
        .userEvent = "PhotoMode",
        .deviceType = 2,
        .deviceId = 0,
        .eventType = 0,
        .idCode = 86,
        .disabled = false,
        .pulse = kQuickInventoryPulse,
    };

    const sds::NativeInputActionDefinition* nativeDefinitionForUserEvent(
        std::string_view userEvent) noexcept
    {
        const sds::NativeInputActionDefinition* definitions[]{
            &kOpenInventoryDefinition,
            &kOpenMissionsDefinition,
            &kOpenSkillsDefinition,
            &kOpenMapDefinition,
            &kOpenPowersDefinition,
            &kOpenPhotoModeDefinition,
            &kEnterPhotoModeDefinition,
        };
        for (const auto* definition : definitions) {
            if (definition->userEvent == userEvent) {
                return definition;
            }
        }
        return nullptr;
    }
}

const sds::NativeInputActionDefinition* sds::nativeInputDefinitionForAction(
    InputAction action) noexcept
{
    switch (action) {
    case InputAction::OpenInventory:
        return &kOpenInventoryDefinition;
    case InputAction::OpenMissions:
        return &kOpenMissionsDefinition;
    case InputAction::OpenSkills:
        return &kOpenSkillsDefinition;
    case InputAction::OpenMap:
        return &kOpenMapDefinition;
    case InputAction::OpenPowers:
        return &kOpenPowersDefinition;
    case InputAction::OpenPhotoMode:
        return &kOpenPhotoModeDefinition;
    default:
        return nullptr;
    }
}

bool sds::isMappedNativeInputUserEvent(std::string_view userEvent) noexcept
{
    return nativeDefinitionForUserEvent(userEvent) != nullptr;
}

bool sds::isReusableNativeButtonSlot(
    const NativeButtonSlotState& slot,
    std::uintptr_t expectedPrimaryVtable,
    std::uintptr_t expectedIdVtable,
    std::uintptr_t expectedUserVtable) noexcept
{
    return slot.timeCode == 0xFFFFFFFFU &&
        slot.primaryVtable == expectedPrimaryVtable &&
        slot.idVtable == expectedIdVtable &&
        slot.userVtable == expectedUserVtable;
}

std::optional<std::size_t> sds::findReusableNativeButtonPoolSlot(
    std::span<const NativeButtonSlotState> slots,
    std::size_t startIndex,
    std::uintptr_t expectedPrimaryVtable,
    std::uintptr_t expectedIdVtable,
    std::uintptr_t expectedUserVtable) noexcept
{
    if (slots.empty()) {
        return std::nullopt;
    }

    const auto normalizedStart = startIndex % slots.size();
    for (std::size_t offset = 0; offset < slots.size(); ++offset) {
        const auto index = (normalizedStart + offset) % slots.size();
        if (isReusableNativeButtonSlot(
                slots[index],
                expectedPrimaryVtable,
                expectedIdVtable,
                expectedUserVtable)) {
            return index;
        }
    }
    return std::nullopt;
}

std::optional<sds::NativeQueueRecyclePlan> sds::planNativeButtonQueueRecycle(
    const NativeQueueRecycleLinks& links) noexcept
{
    if (!links.selected || !links.head || !links.tail) {
        return std::nullopt;
    }
    if (!links.previous && links.selected != links.head) {
        return std::nullopt;
    }

    NativeQueueRecyclePlan plan{
        .newHead = links.head,
        .newTail = links.tail,
        .previousNext = links.selectedNext,
        .writePreviousNext = links.previous != 0,
    };

    if (!links.previous) {
        plan.newHead = links.selectedNext;
    }
    if (links.tail == links.selected) {
        plan.newTail = links.previous;
    }
    return plan;
}

bool sds::isNativeButtonPoolAddress(
    std::uintptr_t address,
    std::uintptr_t poolBase,
    std::size_t slotCount,
    std::size_t slotSize) noexcept
{
    if (!address || !poolBase || slotCount == 0 || slotSize == 0 || address < poolBase) {
        return false;
    }
    const auto offset = address - poolBase;
    if (offset % slotSize != 0) {
        return false;
    }
    return (offset / slotSize) < slotCount;
}

std::uint64_t sds::packNativeButtonDebounceState(
    std::int32_t idCode,
    float previousHeldDownSecs) noexcept
{
    const auto previousBits = std::bit_cast<std::uint32_t>(previousHeldDownSecs);
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(idCode)) << 32U) |
        static_cast<std::uint64_t>(previousBits);
}

bool sds::NativeInputSequencer::enqueue(InputAction action) noexcept
{
    if (_activeDefinition != nullptr) {
        return false;
    }

    const auto* definition = nativeInputDefinitionForAction(action);
    if (!definition || definition->pulse.empty()) {
        return false;
    }

    _activeDefinition = definition;
    _started = false;
    _nextStep = 0;
    _startedAt = {};
    return true;
}

bool sds::NativeInputSequencer::enqueueUserEvent(std::string_view userEvent) noexcept
{
    if (_activeDefinition != nullptr) {
        return false;
    }

    const auto* definition = nativeDefinitionForUserEvent(userEvent);
    if (!definition || definition->pulse.empty()) {
        return false;
    }

    _activeDefinition = definition;
    _started = false;
    _nextStep = 0;
    _startedAt = {};
    return true;
}

std::optional<sds::NativeInputEmission> sds::NativeInputSequencer::poll(TimePoint now) noexcept
{
    if (!_activeDefinition || _nextStep >= _activeDefinition->pulse.size()) {
        return std::nullopt;
    }

    if (!_started) {
        _startedAt = now;
        _started = true;
    }

    const auto& pulseFrame = _activeDefinition->pulse[_nextStep];
    if (now < _startedAt + pulseFrame.offset) {
        return std::nullopt;
    }

    NativeInputEmission emission{
        .action = _activeDefinition->action,
        .edge = pulseFrame.edge,
        .deviceType = _activeDefinition->deviceType,
        .deviceId = _activeDefinition->deviceId,
        .eventType = _activeDefinition->eventType,
        .status = pulseFrame.status,
        .idCode = _activeDefinition->idCode,
        .userEvent = _activeDefinition->userEvent,
        .disabled = _activeDefinition->disabled,
        .value = pulseFrame.value,
        .heldDownSecs = pulseFrame.heldDownSecs,
        .previousHeldDownSecs = pulseFrame.previousHeldDownSecs,
        .stepIndex = pulseFrame.stepIndex,
    };

    ++_nextStep;
    if (_nextStep >= _activeDefinition->pulse.size()) {
        _activeDefinition = nullptr;
    }
    return emission;
}

void sds::NativeInputSequencer::reset() noexcept
{
    _activeDefinition = nullptr;
    _started = false;
    _nextStep = 0;
    _startedAt = {};
}
