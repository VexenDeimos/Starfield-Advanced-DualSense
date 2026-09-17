#pragma once

#include <StarfieldDualSense/Types.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace sds
{
    enum class NativeInputEdge : std::uint8_t
    {
        Press,
        Release
    };

    struct NativeInputPulseFrame
    {
        NativeInputEdge edge{ NativeInputEdge::Press };
        std::uint32_t status{ 0 };
        float value{ 0.0F };
        float heldDownSecs{ 0.0F };
        float previousHeldDownSecs{ 0.0F };
        std::chrono::milliseconds offset{ 0 };
        std::uint8_t stepIndex{ 0 };
    };

    struct NativeInputActionDefinition
    {
        InputAction action{ InputAction::OpenInventory };
        std::string_view userEvent{ "QuickInventory" };
        std::uint32_t deviceType{ 2 };
        std::uint32_t deviceId{ 0 };
        std::uint32_t eventType{ 0 };
        std::int32_t idCode{ 73 };
        bool disabled{ false };
        std::span<const NativeInputPulseFrame> pulse{};
    };

    struct NativeInputEmission
    {
        InputAction action{ InputAction::OpenInventory };
        NativeInputEdge edge{ NativeInputEdge::Press };
        std::uint32_t deviceType{ 2 };
        std::uint32_t deviceId{ 0 };
        std::uint32_t eventType{ 0 };
        std::uint32_t status{ 0 };
        std::int32_t idCode{ 73 };
        std::string_view userEvent{ "QuickInventory" };
        bool disabled{ false };
        float value{ 0.0F };
        float heldDownSecs{ 0.0F };
        float previousHeldDownSecs{ 0.0F };
        std::uint8_t stepIndex{ 0 };
    };

    struct NativeButtonSlotState
    {
        std::uintptr_t primaryVtable{ 0 };
        std::uintptr_t idVtable{ 0 };
        std::uintptr_t userVtable{ 0 };
        std::uint32_t timeCode{ 0 };
    };

    [[nodiscard]] const NativeInputActionDefinition* nativeInputDefinitionForAction(
        InputAction action) noexcept;

    [[nodiscard]] bool isMappedNativeInputUserEvent(
        std::string_view userEvent) noexcept;

    [[nodiscard]] bool isReusableNativeButtonSlot(
        const NativeButtonSlotState& slot,
        std::uintptr_t expectedPrimaryVtable,
        std::uintptr_t expectedIdVtable,
        std::uintptr_t expectedUserVtable) noexcept;

    [[nodiscard]] std::optional<std::size_t> findReusableNativeButtonPoolSlot(
        std::span<const NativeButtonSlotState> slots,
        std::size_t startIndex,
        std::uintptr_t expectedPrimaryVtable,
        std::uintptr_t expectedIdVtable,
        std::uintptr_t expectedUserVtable) noexcept;


    struct NativeQueueRecycleLinks
    {
        std::uintptr_t head{ 0 };
        std::uintptr_t tail{ 0 };
        std::uintptr_t previous{ 0 };
        std::uintptr_t selected{ 0 };
        std::uintptr_t selectedNext{ 0 };
    };

    struct NativeQueueRecyclePlan
    {
        std::uintptr_t newHead{ 0 };
        std::uintptr_t newTail{ 0 };
        std::uintptr_t previousNext{ 0 };
        bool writePreviousNext{ false };
    };

    [[nodiscard]] std::optional<NativeQueueRecyclePlan> planNativeButtonQueueRecycle(
        const NativeQueueRecycleLinks& links) noexcept;

    [[nodiscard]] bool isNativeButtonPoolAddress(
        std::uintptr_t address,
        std::uintptr_t poolBase,
        std::size_t slotCount,
        std::size_t slotSize) noexcept;

    [[nodiscard]] std::uint64_t packNativeButtonDebounceState(
        std::int32_t idCode,
        float previousHeldDownSecs) noexcept;

    class NativeInputSequencer
    {
    public:
        using Clock = std::chrono::steady_clock;
        using TimePoint = Clock::time_point;

        [[nodiscard]] bool enqueue(InputAction action) noexcept;
        [[nodiscard]] bool enqueueUserEvent(std::string_view userEvent) noexcept;
        [[nodiscard]] std::optional<NativeInputEmission> poll(TimePoint now) noexcept;
        [[nodiscard]] bool active() const noexcept { return _activeDefinition != nullptr; }
        void reset() noexcept;

    private:
        const NativeInputActionDefinition* _activeDefinition{ nullptr };
        bool _started{ false };
        std::size_t _nextStep{ 0 };
        TimePoint _startedAt{};
    };
}
