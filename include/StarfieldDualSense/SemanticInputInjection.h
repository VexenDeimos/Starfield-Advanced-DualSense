#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace sds
{
    inline constexpr std::size_t kQuickInventorySemanticPacketSize = 0x60;
    inline constexpr std::uintptr_t kQuickInventoryPrimaryVtableRva = 0x4D59F50;
    inline constexpr std::uintptr_t kQuickInventoryIdVtableRva = 0x4D59F28;
    inline constexpr std::uintptr_t kQuickInventoryUserVtableRva = 0x4D59F00;
    inline constexpr std::int32_t kQuickInventoryKeyboardIdCode = 73;

    enum class SemanticPulseEdge : std::uint8_t
    {
        Held,
        Release
    };

    struct SemanticPulseStep
    {
        SemanticPulseEdge edge{ SemanticPulseEdge::Held };
        float value{ 0.0F };
        float heldDownSecs{ 0.0F };
        float previousHeldDownSecs{ 0.0F };
        std::uint32_t status{ 0 };
        std::uint8_t stepIndex{ 0 };
    };


    struct QuickInventoryNativeTemplate
    {
        bool ready{ false };
        std::uintptr_t sourceAddress{ 0 };
        std::uint32_t lastObservedTimeCode{ 0 };
        std::array<std::uint8_t, kQuickInventorySemanticPacketSize> packet{};
    };

    [[nodiscard]] bool captureQuickInventoryNativeTemplate(
        QuickInventoryNativeTemplate& cache,
        std::uintptr_t sourceAddress,
        const std::array<std::uint8_t, kQuickInventorySemanticPacketSize>& packet,
        bool heldEdge,
        float heldDownSecs,
        std::uint32_t timeCode) noexcept;

    void recordNativeSemanticTimeCode(
        QuickInventoryNativeTemplate& cache,
        std::uint32_t timeCode) noexcept;

    class SemanticPulseSequencer
    {
    public:
        using Clock = std::chrono::steady_clock;
        using TimePoint = Clock::time_point;

        [[nodiscard]] bool queue(TimePoint now) noexcept;
        [[nodiscard]] std::optional<SemanticPulseStep> poll(TimePoint now) noexcept;
        [[nodiscard]] bool active() const noexcept { return _active; }
        void reset() noexcept;

    private:
        bool _active{ false };
        std::uint8_t _nextStep{ 0 };
        TimePoint _startedAt{};
    };

    [[nodiscard]] std::array<std::uint8_t, kQuickInventorySemanticPacketSize>
    buildQuickInventorySemanticPacket(
        std::uintptr_t moduleBase,
        std::uintptr_t actionPointer,
        std::uint32_t timeCode,
        const SemanticPulseStep& step) noexcept;

    [[nodiscard]] std::array<std::uint8_t, kQuickInventorySemanticPacketSize>
    buildQuickInventorySemanticPacketFromTemplate(
        const std::array<std::uint8_t, kQuickInventorySemanticPacketSize>& nativeTemplate,
        std::uint32_t timeCode,
        const SemanticPulseStep& step) noexcept;
}
