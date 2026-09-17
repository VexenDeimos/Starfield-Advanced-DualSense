#include <StarfieldDualSense/SemanticInputInjection.h>

#include <array>
#include <cstring>

namespace
{
    using namespace std::chrono_literals;

    constexpr std::array<sds::SemanticPulseStep, 5> kPulseSteps{
        sds::SemanticPulseStep{
            .edge = sds::SemanticPulseEdge::Held,
            .value = 1.0F,
            .heldDownSecs = 0.000F,
            .previousHeldDownSecs = 0.000F,
            .status = 0,
            .stepIndex = 1,
        },
        sds::SemanticPulseStep{
            .edge = sds::SemanticPulseEdge::Held,
            .value = 1.0F,
            .heldDownSecs = 0.015F,
            .previousHeldDownSecs = 0.000F,
            .status = 0,
            .stepIndex = 2,
        },
        sds::SemanticPulseStep{
            .edge = sds::SemanticPulseEdge::Held,
            .value = 1.0F,
            .heldDownSecs = 0.030F,
            .previousHeldDownSecs = 0.015F,
            .status = 0,
            .stepIndex = 3,
        },
        sds::SemanticPulseStep{
            .edge = sds::SemanticPulseEdge::Held,
            .value = 1.0F,
            .heldDownSecs = 0.045F,
            .previousHeldDownSecs = 0.030F,
            .status = 0,
            .stepIndex = 4,
        },
        sds::SemanticPulseStep{
            .edge = sds::SemanticPulseEdge::Release,
            .value = 0.0F,
            .heldDownSecs = 0.045F,
            .previousHeldDownSecs = 0.045F,
            .status = 2,
            .stepIndex = 5,
        },
    };

    constexpr std::array kStepOffsets{ 0ms, 15ms, 30ms, 45ms, 60ms };

    template <class T, std::size_t N>
    void writeField(std::array<std::uint8_t, N>& packet, std::size_t offset, const T& value) noexcept
    {
        std::memcpy(packet.data() + offset, &value, sizeof(value));
    }
}


void sds::recordNativeSemanticTimeCode(
    QuickInventoryNativeTemplate& cache,
    std::uint32_t timeCode) noexcept
{
    if (timeCode > cache.lastObservedTimeCode) {
        cache.lastObservedTimeCode = timeCode;
    }
}

bool sds::captureQuickInventoryNativeTemplate(
    QuickInventoryNativeTemplate& cache,
    std::uintptr_t sourceAddress,
    const std::array<std::uint8_t, kQuickInventorySemanticPacketSize>& packet,
    bool heldEdge,
    float heldDownSecs,
    std::uint32_t timeCode) noexcept
{
    recordNativeSemanticTimeCode(cache, timeCode);

    if (cache.ready || !heldEdge || sourceAddress == 0 || heldDownSecs < 0.0F || heldDownSecs > 0.020F) {
        return false;
    }

    cache.ready = true;
    cache.sourceAddress = sourceAddress;
    cache.packet = packet;
    return true;
}

bool sds::SemanticPulseSequencer::queue(TimePoint now) noexcept
{
    if (_active) {
        return false;
    }

    _active = true;
    _nextStep = 0;
    _startedAt = now;
    return true;
}

std::optional<sds::SemanticPulseStep> sds::SemanticPulseSequencer::poll(TimePoint now) noexcept
{
    if (!_active || _nextStep >= kPulseSteps.size()) {
        return std::nullopt;
    }

    if (now < _startedAt + kStepOffsets[_nextStep]) {
        return std::nullopt;
    }

    const auto step = kPulseSteps[_nextStep++];
    if (_nextStep >= kPulseSteps.size()) {
        _active = false;
    }
    return step;
}

void sds::SemanticPulseSequencer::reset() noexcept
{
    _active = false;
    _nextStep = 0;
    _startedAt = {};
}

std::array<std::uint8_t, sds::kQuickInventorySemanticPacketSize>
sds::buildQuickInventorySemanticPacket(
    std::uintptr_t moduleBase,
    std::uintptr_t actionPointer,
    std::uint32_t timeCode,
    const SemanticPulseStep& step) noexcept
{
    std::array<std::uint8_t, kQuickInventorySemanticPacketSize> packet{};

    const auto primaryVtable = moduleBase + kQuickInventoryPrimaryVtableRva;
    const auto idVtable = moduleBase + kQuickInventoryIdVtableRva;
    const auto userVtable = moduleBase + kQuickInventoryUserVtableRva;
    constexpr std::uint32_t keyboardDevice = 0;
    constexpr std::uint32_t deviceId = 0;
    constexpr std::uint32_t buttonEventType = 0;
    constexpr std::uintptr_t next = 0;
    constexpr bool disabled = false;

    writeField(packet, 0x00, primaryVtable);
    writeField(packet, 0x08, keyboardDevice);
    writeField(packet, 0x0C, deviceId);
    writeField(packet, 0x10, buttonEventType);
    writeField(packet, 0x18, next);
    writeField(packet, 0x20, timeCode);
    writeField(packet, 0x24, step.status);
    writeField(packet, 0x28, actionPointer);
    writeField(packet, 0x30, kQuickInventoryKeyboardIdCode);
    writeField(packet, 0x34, disabled);
    writeField(packet, 0x38, idVtable);
    writeField(packet, 0x40, userVtable);
    writeField(packet, 0x48, step.value);
    writeField(packet, 0x4C, step.heldDownSecs);
    return packet;
}

std::array<std::uint8_t, sds::kQuickInventorySemanticPacketSize>
sds::buildQuickInventorySemanticPacketFromTemplate(
    const std::array<std::uint8_t, kQuickInventorySemanticPacketSize>& nativeTemplate,
    std::uint32_t timeCode,
    const SemanticPulseStep& step) noexcept
{
    auto packet = nativeTemplate;
    constexpr std::uintptr_t next = 0;
    writeField(packet, 0x18, next);
    writeField(packet, 0x20, timeCode);
    writeField(packet, 0x24, step.status);
    writeField(packet, 0x48, step.value);
    writeField(packet, 0x4C, step.heldDownSecs);
    writeField(packet, 0x50, step.previousHeldDownSecs);
    return packet;
}
