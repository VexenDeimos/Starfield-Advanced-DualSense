#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sds
{
    inline constexpr std::size_t kButtonEventRawSize = 0x60;


    inline constexpr std::size_t kInputEventBaseRawSize = 0x28;

    struct NativeInputDiagnostic
    {
        std::int64_t observedMs{ 0 };
        std::uint32_t deviceType{ 0 };
        std::uint32_t deviceId{ 0 };
        std::uint32_t eventType{ 0 };
        std::uint32_t status{ 0 };
        std::uint32_t timeCode{ 0 };
        std::uintptr_t objectAddress{ 0 };
        bool hasButtonDetails{ false };
        std::int32_t idCode{ -1 };
        std::string userEvent{};
        bool disabled{ false };
        float value{ 0.0F };
        float heldDownSecs{ 0.0F };
        std::array<std::uint8_t, kInputEventBaseRawSize> rawBase{};
    };

    struct InputTraceCaptureStart
    {
        std::uint32_t captureId{ 0 };
        std::int64_t armedMs{ 0 };
        std::vector<NativeInputDiagnostic> buffered{};
    };

    struct InputTraceCaptureFinish
    {
        std::uint32_t captureId{ 0 };
        std::uint32_t observedCount{ 0 };
    };

    class InputTraceBuffer
    {
    public:
        InputTraceBuffer(std::int64_t preWindowMs, std::int64_t postWindowMs, std::size_t maxSamples);

        void push(const NativeInputDiagnostic& sample);
        [[nodiscard]] InputTraceCaptureStart beginCapture(std::int64_t nowMs);
        [[nodiscard]] std::optional<std::uint32_t> activeCaptureId(std::int64_t nowMs) const noexcept;
        [[nodiscard]] std::optional<InputTraceCaptureFinish> finishCapture(std::int64_t nowMs) noexcept;
        [[nodiscard]] std::uint32_t observedCount() const noexcept { return _observedCount; }
        [[nodiscard]] std::int64_t armedMs() const noexcept { return _armedMs; }

    private:
        std::int64_t _preWindowMs{ 0 };
        std::int64_t _postWindowMs{ 0 };
        std::size_t _maxSamples{ 0 };
        std::deque<NativeInputDiagnostic> _history{};
        std::uint32_t _nextCaptureId{ 1 };
        std::uint32_t _captureId{ 0 };
        std::uint32_t _observedCount{ 0 };
        std::int64_t _armedMs{ 0 };
        std::int64_t _deadlineMs{ 0 };
    };

    [[nodiscard]] std::string formatNativeInputDiagnostic(
        const NativeInputDiagnostic& diagnostic,
        std::uint32_t captureId,
        std::int64_t relativeMs);

    enum class InputButtonEdge : std::uint8_t
    {
        Press,
        Release
    };

    struct InputButtonDiagnostic
    {
        InputButtonEdge edge{ InputButtonEdge::Press };
        std::uint32_t deviceType{ 0 };
        std::uint32_t deviceId{ 0 };
        std::uint32_t eventType{ 0 };
        std::uint32_t status{ 0 };
        std::uint32_t timeCode{ 0 };
        std::int32_t idCode{ -1 };
        std::string userEvent{};
        bool disabled{ false };
        float value{ 0.0F };
        float heldDownSecs{ 0.0F };
        std::uintptr_t objectAddress{ 0 };
        std::uintptr_t primaryVtable{ 0 };
        std::uint64_t chordState{ 0 };
        std::uint64_t debounceState{ 0 };
        std::uintptr_t unk50{ 0 };
        std::uintptr_t debounceManager{ 0 };
        std::array<std::uint8_t, kButtonEventRawSize> rawBytes{};
    };

    [[nodiscard]] std::string formatInputButtonDiagnostic(const InputButtonDiagnostic& diagnostic);

    inline constexpr std::size_t kSemanticCallerTraceMaxFrames = 16;

    struct SemanticCallerTraceDiagnostic
    {
        InputButtonEdge edge{ InputButtonEdge::Press };
        std::uintptr_t returnAddress{ 0 };
        std::uintptr_t moduleBase{ 0 };
        std::size_t moduleSize{ 0 };
        std::array<std::uintptr_t, kSemanticCallerTraceMaxFrames> stackFrames{};
        std::size_t stackFrameCount{ 0 };
    };

    [[nodiscard]] std::string formatSemanticCallerTrace(const SemanticCallerTraceDiagnostic& diagnostic);


    inline constexpr std::size_t kNativeButtonConstructorHistoryCapacity = 128;

    struct NativeButtonConstructorOriginDiagnostic
    {
        std::uint64_t sequence{ 0 };
        std::uintptr_t objectAddress{ 0 };
        std::uintptr_t callerReturnAddress{ 0 };
        std::uintptr_t moduleBase{ 0 };
        std::size_t moduleSize{ 0 };
        std::array<std::uintptr_t, kSemanticCallerTraceMaxFrames> stackFrames{};
        std::size_t stackFrameCount{ 0 };
    };

    class NativeButtonConstructorHistory
    {
    public:
        void record(
            std::uintptr_t objectAddress,
            std::uintptr_t callerReturnAddress,
            std::uintptr_t moduleBase,
            std::size_t moduleSize,
            const std::array<std::uintptr_t, kSemanticCallerTraceMaxFrames>& stackFrames,
            std::size_t stackFrameCount) noexcept;

        [[nodiscard]] std::optional<NativeButtonConstructorOriginDiagnostic> findNewest(
            std::uintptr_t objectAddress) const noexcept;

    private:
        std::array<NativeButtonConstructorOriginDiagnostic, kNativeButtonConstructorHistoryCapacity> _records{};
        std::uint64_t _nextSequence{ 1 };
    };

    [[nodiscard]] std::string formatNativeButtonConstructorOrigin(
        const NativeButtonConstructorOriginDiagnostic& diagnostic);

    inline constexpr std::size_t kNativeEnqueueOriginHistoryCapacity = 256;

    struct NativeEnqueueOriginDiagnostic
    {
        std::uint64_t sequence{ 0 };
        std::uintptr_t eventAddress{ 0 };
        std::uint32_t timeCode{ 0 };
        std::uintptr_t callerReturnAddress{ 0 };
        std::uintptr_t moduleBase{ 0 };
        std::size_t moduleSize{ 0 };
        std::array<std::uintptr_t, kSemanticCallerTraceMaxFrames> stackFrames{};
        std::size_t stackFrameCount{ 0 };
    };

    class NativeEnqueueOriginHistory
    {
    public:
        void record(
            std::uintptr_t eventAddress,
            std::uint32_t timeCode,
            std::uintptr_t callerReturnAddress,
            std::uintptr_t moduleBase,
            std::size_t moduleSize,
            const std::array<std::uintptr_t, kSemanticCallerTraceMaxFrames>& stackFrames,
            std::size_t stackFrameCount) noexcept;

        [[nodiscard]] std::optional<NativeEnqueueOriginDiagnostic> findNewest(
            std::uintptr_t eventAddress,
            std::uint32_t timeCode) const noexcept;
        [[nodiscard]] std::optional<NativeEnqueueOriginDiagnostic> findNewest(
            std::uintptr_t eventAddress) const noexcept;

    private:
        std::array<NativeEnqueueOriginDiagnostic, kNativeEnqueueOriginHistoryCapacity> _records{};
        std::uint64_t _nextSequence{ 1 };
    };

    [[nodiscard]] std::string formatNativeEnqueueOrigin(
        const NativeEnqueueOriginDiagnostic& diagnostic);
    [[nodiscard]] std::optional<std::array<std::uint8_t, 10>> buildButtonEventConstructorEntryPatch(
        std::uintptr_t sourceAddress,
        std::uintptr_t branchIslandAddress) noexcept;
    [[nodiscard]] bool matchesButtonEventConstructorPrologue(
        const std::array<std::uint8_t, 10>& bytes) noexcept;

    enum class NativeCallKind : std::uint8_t
    {
        DirectRel32,
        IndirectRegister,
        IndirectMemory,
    };

    inline constexpr std::size_t kNativeCodeProbeBytes = 128;

    struct NativeCodeProbeDiagnostic
    {
        std::uintptr_t frameAddress{ 0 };
        std::uintptr_t moduleBase{ 0 };
        std::size_t moduleSize{ 0 };
        std::uintptr_t functionStart{ 0 };
        std::uintptr_t functionEnd{ 0 };
        std::uintptr_t callAddress{ 0 };
        std::uintptr_t callTarget{ 0 };
        bool callTargetDecoded{ false };
        NativeCallKind callKind{ NativeCallKind::DirectRel32 };
        bool callFound{ false };
        std::uintptr_t windowStart{ 0 };
        std::array<std::uint8_t, kNativeCodeProbeBytes> codeBytes{};
        std::size_t codeSize{ 0 };
    };

    struct NativeCallSite
    {
        NativeCallKind kind{ NativeCallKind::DirectRel32 };
        std::uintptr_t instructionAddress{ 0 };
        std::size_t instructionSize{ 0 };
        std::uintptr_t targetAddress{ 0 };
        bool targetDecoded{ false };
        std::array<std::uint8_t, 8> instructionBytes{};
        std::size_t instructionByteCount{ 0 };
    };

    [[nodiscard]] std::optional<NativeCallSite> findNearestCallBefore(
        std::uintptr_t windowStart,
        const std::vector<std::uint8_t>& bytes,
        std::uintptr_t frameAddress) noexcept;

    enum class NativeRipReferenceKind : std::uint8_t
    {
        Lea,
        MovLoad,
        MovStore,
    };

    struct NativeRipReference
    {
        NativeRipReferenceKind kind{ NativeRipReferenceKind::Lea };
        std::uintptr_t instructionAddress{ 0 };
        std::size_t instructionSize{ 0 };
        std::uintptr_t targetAddress{ 0 };
        std::array<std::uint8_t, 8> instructionBytes{};
        std::size_t instructionByteCount{ 0 };
    };

    [[nodiscard]] std::vector<NativeRipReference> findRipRelativeReferences(
        std::uintptr_t windowStart,
        const std::vector<std::uint8_t>& bytes,
        std::uintptr_t targetAddress) noexcept;

    inline constexpr std::uint32_t kNativeButtonFieldTimeCode = 1U << 0;
    inline constexpr std::uint32_t kNativeButtonFieldAction = 1U << 1;
    inline constexpr std::uint32_t kNativeButtonFieldIdCode = 1U << 2;
    inline constexpr std::uint32_t kNativeButtonFieldValue = 1U << 3;
    inline constexpr std::uint32_t kNativeButtonFieldHeld = 1U << 4;

    struct NativeButtonFieldWrite
    {
        std::uint32_t fieldMask{ 0 };
        std::size_t fieldOffset{ 0 };
        std::uintptr_t instructionAddress{ 0 };
        std::array<std::uint8_t, 16> instructionBytes{};
        std::size_t instructionByteCount{ 0 };
    };

    [[nodiscard]] std::vector<NativeButtonFieldWrite> findButtonEventFieldWrites(
        std::uintptr_t windowStart,
        const std::vector<std::uint8_t>& bytes) noexcept;

    [[nodiscard]] std::vector<NativeCallSite> findNativeCallSites(
        std::uintptr_t windowStart,
        const std::vector<std::uint8_t>& bytes) noexcept;

    [[nodiscard]] std::vector<NativeCallSite> findDirectRel32CallsToTarget(
        std::uintptr_t windowStart,
        const std::vector<std::uint8_t>& bytes,
        std::uintptr_t targetAddress) noexcept;

    [[nodiscard]] std::optional<std::array<std::uint8_t, 5>> buildRel32CallPatch(
        std::uintptr_t sourceAddress,
        std::uintptr_t targetAddress) noexcept;

    inline constexpr std::size_t kNativeFunctionChunkBytes = 160;

    struct NativeFunctionChunkDiagnostic
    {
        std::uintptr_t moduleBase{ 0 };
        std::size_t moduleSize{ 0 };
        std::uintptr_t functionStart{ 0 };
        std::uintptr_t functionEnd{ 0 };
        std::uintptr_t chunkStart{ 0 };
        std::size_t chunkIndex{ 0 };
        std::size_t chunkCount{ 0 };
        std::array<std::uint8_t, kNativeFunctionChunkBytes> bytes{};
        std::size_t byteCount{ 0 };
    };

    [[nodiscard]] std::string formatNativeFunctionChunk(
        const NativeFunctionChunkDiagnostic& diagnostic);

    inline constexpr std::size_t kNativeSourceSnapshotBytes = 128;

    struct NativeSourceSnapshotDiagnostic
    {
        std::uintptr_t moduleBase{ 0 };
        std::size_t moduleSize{ 0 };
        std::uintptr_t managerAddress{ 0 };
        std::uintptr_t observedSourceAddress{ 0 };
        std::uintptr_t expectedSourceAddress{ 0 };
        std::uintptr_t sourceVtable{ 0 };
        bool sourceMatchesManager{ false };
        std::array<std::uint8_t, kNativeSourceSnapshotBytes> bytes{};
        std::size_t byteCount{ 0 };
    };

    [[nodiscard]] std::string formatNativeSourceSnapshot(
        const NativeSourceSnapshotDiagnostic& diagnostic);

    [[nodiscard]] std::optional<std::uintptr_t> decodeDirectRel32CallTarget(
        std::uintptr_t callAddress,
        const std::array<std::uint8_t, 5>& instruction) noexcept;
    [[nodiscard]] std::string formatNativeCodeProbe(
        const NativeCodeProbeDiagnostic& diagnostic,
        std::size_t frameIndex);

    struct SemanticButtonDiagnostic
    {
        InputButtonEdge edge{ InputButtonEdge::Press };
        std::uintptr_t sourceAddress{ 0 };
        std::uintptr_t sourceVtable{ 0 };
        bool sourceVtableExpected{ false };
        std::uintptr_t eventAddress{ 0 };
        std::uintptr_t primaryVtable{ 0 };
        bool primaryVtableExpected{ false };
        std::uint32_t deviceType{ 0 };
        std::uint32_t deviceId{ 0 };
        std::uint32_t eventType{ 0 };
        std::uint32_t status{ 0 };
        std::uint32_t timeCode{ 0 };
        std::int32_t idCode{ -1 };
        std::string userEvent{};
        bool disabled{ false };
        std::uintptr_t idVtable{ 0 };
        std::uintptr_t userVtable{ 0 };
        float value{ 0.0F };
        float heldDownSecs{ 0.0F };
        std::uintptr_t unk50{ 0 };
        std::uintptr_t debounceManager{ 0 };
        std::array<std::uint8_t, kButtonEventRawSize> rawBytes{};
    };

    [[nodiscard]] bool isQuickInventorySemanticAction(std::string_view userEvent) noexcept;
    [[nodiscard]] std::optional<std::array<std::uint8_t, 7>> buildSemanticBroadcasterEntryPatch(
        std::uintptr_t sourceAddress,
        std::uintptr_t branchIslandAddress) noexcept;
    [[nodiscard]] bool matchesSemanticBroadcasterPrologue(
        const std::array<std::uint8_t, 7>& bytes) noexcept;
    [[nodiscard]] std::string formatSemanticButtonDiagnostic(const SemanticButtonDiagnostic& diagnostic);
}
