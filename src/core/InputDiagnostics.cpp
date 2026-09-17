#include <StarfieldDualSense/InputDiagnostics.h>

#include <algorithm>
#include <iomanip>
#include <cstring>
#include <limits>
#include <sstream>
#include <utility>

std::string sds::formatInputButtonDiagnostic(const InputButtonDiagnostic& diagnostic)
{
    std::ostringstream out;
    out << "Input diagnostic: keyboard button"
        << " edge=" << (diagnostic.edge == InputButtonEdge::Press ? "press" : "release")
        << " device=" << diagnostic.deviceType
        << " deviceId=" << diagnostic.deviceId
        << " eventType=" << diagnostic.eventType
        << " idCode=" << diagnostic.idCode
        << " userEvent='" << diagnostic.userEvent << '\''
        << " disabled=" << (diagnostic.disabled ? "true" : "false")
        << std::fixed << std::setprecision(3)
        << " value=" << diagnostic.value
        << " held=" << diagnostic.heldDownSecs
        << std::defaultfloat
        << " timeCode=" << diagnostic.timeCode
        << " status=" << diagnostic.status
        << std::hex << std::setfill('0')
        << " object=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.objectAddress
        << " vtable=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.primaryVtable
        << " chord=0x" << std::setw(16) << diagnostic.chordState
        << " debounce=0x" << std::setw(16) << diagnostic.debounceState
        << " unk50=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.unk50
        << " debounceManager=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.debounceManager
        << " raw60=";

    for (std::size_t i = 0; i < diagnostic.rawBytes.size(); ++i) {
        if (i != 0) {
            out << ' ';
        }
        out << std::setw(2) << static_cast<unsigned int>(diagnostic.rawBytes[i]);
    }

    return out.str();
}

sds::InputTraceBuffer::InputTraceBuffer(
    std::int64_t preWindowMs,
    std::int64_t postWindowMs,
    std::size_t maxSamples) :
    _preWindowMs(preWindowMs),
    _postWindowMs(postWindowMs),
    _maxSamples(maxSamples)
{}

void sds::InputTraceBuffer::push(const NativeInputDiagnostic& sample)
{
    _history.push_back(sample);
    while (_history.size() > _maxSamples) {
        _history.pop_front();
    }

    if (_captureId != 0 && sample.observedMs >= _armedMs && sample.observedMs <= _deadlineMs) {
        ++_observedCount;
    }
}

sds::InputTraceCaptureStart sds::InputTraceBuffer::beginCapture(std::int64_t nowMs)
{
    InputTraceCaptureStart result{};
    result.captureId = _nextCaptureId++;
    result.armedMs = nowMs;

    const auto earliest = nowMs - _preWindowMs;
    for (const auto& sample : _history) {
        if (sample.observedMs >= earliest && sample.observedMs <= nowMs) {
            result.buffered.push_back(sample);
        }
    }

    _captureId = result.captureId;
    _observedCount = static_cast<std::uint32_t>(result.buffered.size());
    _armedMs = nowMs;
    _deadlineMs = nowMs + _postWindowMs;
    return result;
}

std::optional<std::uint32_t> sds::InputTraceBuffer::activeCaptureId(std::int64_t nowMs) const noexcept
{
    if (_captureId == 0 || nowMs > _deadlineMs) {
        return std::nullopt;
    }
    return _captureId;
}

std::optional<sds::InputTraceCaptureFinish> sds::InputTraceBuffer::finishCapture(std::int64_t nowMs) noexcept
{
    if (_captureId == 0 || nowMs <= _deadlineMs) {
        return std::nullopt;
    }

    InputTraceCaptureFinish result{
        .captureId = _captureId,
        .observedCount = _observedCount,
    };
    _captureId = 0;
    _observedCount = 0;
    _armedMs = 0;
    _deadlineMs = 0;
    return result;
}

std::string sds::formatNativeInputDiagnostic(
    const NativeInputDiagnostic& diagnostic,
    std::uint32_t captureId,
    std::int64_t relativeMs)
{
    std::ostringstream out;
    out << "Native input trace: capture=" << captureId
        << " relativeMs=" << relativeMs
        << " device=" << diagnostic.deviceType
        << " deviceId=" << diagnostic.deviceId
        << " eventType=" << diagnostic.eventType
        << " timeCode=" << diagnostic.timeCode
        << " status=" << diagnostic.status;

    if (diagnostic.hasButtonDetails || diagnostic.idCode >= 0 || !diagnostic.userEvent.empty()) {
        out << " idCode=" << diagnostic.idCode
            << " userEvent='" << diagnostic.userEvent << '\''
            << " disabled=" << (diagnostic.disabled ? "true" : "false")
            << std::fixed << std::setprecision(3)
            << " value=" << diagnostic.value
            << " held=" << diagnostic.heldDownSecs
            << std::defaultfloat;
    }

    out << std::hex << std::setfill('0')
        << " object=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.objectAddress
        << " raw28=";
    for (std::size_t i = 0; i < diagnostic.rawBase.size(); ++i) {
        if (i != 0) {
            out << ' ';
        }
        out << std::setw(2) << static_cast<unsigned int>(diagnostic.rawBase[i]);
    }
    return out.str();
}


bool sds::isQuickInventorySemanticAction(std::string_view userEvent) noexcept
{
    return userEvent == "QuickInventory";
}

std::optional<std::array<std::uint8_t, 7>> sds::buildSemanticBroadcasterEntryPatch(
    std::uintptr_t sourceAddress,
    std::uintptr_t branchIslandAddress) noexcept
{
    constexpr std::int64_t instructionSize = 5;
    const auto sourceAfterJump = static_cast<std::int64_t>(sourceAddress) + instructionSize;
    const auto displacement = static_cast<std::int64_t>(branchIslandAddress) - sourceAfterJump;
    if (displacement < (std::numeric_limits<std::int32_t>::min)() ||
        displacement > (std::numeric_limits<std::int32_t>::max)()) {
        return std::nullopt;
    }

    const auto rel32 = static_cast<std::int32_t>(displacement);
    std::array<std::uint8_t, 7> patch{ 0xE9, 0, 0, 0, 0, 0x90, 0x90 };
    std::memcpy(patch.data() + 1, &rel32, sizeof(rel32));
    return patch;
}

bool sds::matchesSemanticBroadcasterPrologue(const std::array<std::uint8_t, 7>& bytes) noexcept
{
    constexpr std::array<std::uint8_t, 7> expected{ 0x48, 0x8B, 0xC4, 0x48, 0x89, 0x58, 0x10 };
    return bytes == expected;
}

std::string sds::formatSemanticButtonDiagnostic(const SemanticButtonDiagnostic& diagnostic)
{
    std::ostringstream out;
    out << "Semantic observer: action='" << diagnostic.userEvent << '\''
        << " edge=" << (diagnostic.edge == InputButtonEdge::Press ? "press" : "release")
        << std::hex << std::setfill('0')
        << " source=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.sourceAddress
        << " sourceVtable=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.sourceVtable
        << std::dec
        << " sourceExpected=" << (diagnostic.sourceVtableExpected ? "true" : "false")
        << std::hex
        << " event=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.eventAddress
        << " vtable=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.primaryVtable
        << std::dec
        << " eventExpected=" << (diagnostic.primaryVtableExpected ? "true" : "false")
        << " device=" << diagnostic.deviceType
        << " deviceId=" << diagnostic.deviceId
        << " eventType=" << diagnostic.eventType
        << " idCode=" << diagnostic.idCode
        << " disabled=" << (diagnostic.disabled ? "true" : "false")
        << std::fixed << std::setprecision(3)
        << " value=" << diagnostic.value
        << " held=" << diagnostic.heldDownSecs
        << std::defaultfloat
        << " timeCode=" << diagnostic.timeCode
        << " status=" << diagnostic.status
        << std::hex << std::setfill('0')
        << " idVtable=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.idVtable
        << " userVtable=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.userVtable
        << " unk50=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.unk50
        << " debounceManager=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.debounceManager
        << " raw60=";

    for (std::size_t i = 0; i < diagnostic.rawBytes.size(); ++i) {
        if (i != 0) {
            out << ' ';
        }
        out << std::setw(2) << static_cast<unsigned int>(diagnostic.rawBytes[i]);
    }
    return out.str();
}


void sds::NativeButtonConstructorHistory::record(
    std::uintptr_t objectAddress,
    std::uintptr_t callerReturnAddress,
    std::uintptr_t moduleBase,
    std::size_t moduleSize,
    const std::array<std::uintptr_t, kSemanticCallerTraceMaxFrames>& stackFrames,
    std::size_t stackFrameCount) noexcept
{
    if (!objectAddress) {
        return;
    }

    NativeButtonConstructorOriginDiagnostic record{};
    record.sequence = _nextSequence++;
    record.objectAddress = objectAddress;
    record.callerReturnAddress = callerReturnAddress;
    record.moduleBase = moduleBase;
    record.moduleSize = moduleSize;
    record.stackFrames = stackFrames;
    record.stackFrameCount = (std::min)(stackFrameCount, record.stackFrames.size());

    const auto slot = static_cast<std::size_t>((record.sequence - 1) % _records.size());
    _records[slot] = record;
}

std::optional<sds::NativeButtonConstructorOriginDiagnostic> sds::NativeButtonConstructorHistory::findNewest(
    std::uintptr_t objectAddress) const noexcept
{
    if (!objectAddress) {
        return std::nullopt;
    }

    const NativeButtonConstructorOriginDiagnostic* newest = nullptr;
    for (const auto& record : _records) {
        if (record.objectAddress != objectAddress || record.sequence == 0) {
            continue;
        }
        if (!newest || record.sequence > newest->sequence) {
            newest = &record;
        }
    }
    if (!newest) {
        return std::nullopt;
    }
    return *newest;
}

std::string sds::formatNativeButtonConstructorOrigin(
    const NativeButtonConstructorOriginDiagnostic& diagnostic)
{
    const auto inStarfield = [&](std::uintptr_t address) noexcept {
        if (diagnostic.moduleBase == 0 || diagnostic.moduleSize == 0 || address < diagnostic.moduleBase) {
            return false;
        }
        return address - diagnostic.moduleBase < diagnostic.moduleSize;
    };

    std::ostringstream out;
    out << "ButtonEvent constructor origin: seq=" << std::dec << diagnostic.sequence
        << std::hex << std::setfill('0')
        << " object=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.objectAddress
        << " caller=";
    if (inStarfield(diagnostic.callerReturnAddress)) {
        out << "Starfield+0x" << (diagnostic.callerReturnAddress - diagnostic.moduleBase);
    } else {
        out << "outside-Starfield";
    }

    out << " stack=";
    bool wroteFrame = false;
    const auto count = (std::min)(diagnostic.stackFrameCount, diagnostic.stackFrames.size());
    for (std::size_t i = 0; i < count; ++i) {
        const auto frame = diagnostic.stackFrames[i];
        if (!inStarfield(frame) || frame == diagnostic.callerReturnAddress) {
            continue;
        }
        if (wroteFrame) {
            out << " -> ";
        }
        out << "Starfield+0x" << (frame - diagnostic.moduleBase);
        wroteFrame = true;
    }
    if (!wroteFrame) {
        out << "(no additional Starfield frames)";
    }
    return out.str();
}

void sds::NativeEnqueueOriginHistory::record(
    std::uintptr_t eventAddress,
    std::uint32_t timeCode,
    std::uintptr_t callerReturnAddress,
    std::uintptr_t moduleBase,
    std::size_t moduleSize,
    const std::array<std::uintptr_t, kSemanticCallerTraceMaxFrames>& stackFrames,
    std::size_t stackFrameCount) noexcept
{
    if (!eventAddress) {
        return;
    }

    NativeEnqueueOriginDiagnostic record{};
    record.sequence = _nextSequence++;
    record.eventAddress = eventAddress;
    record.timeCode = timeCode;
    record.callerReturnAddress = callerReturnAddress;
    record.moduleBase = moduleBase;
    record.moduleSize = moduleSize;
    record.stackFrames = stackFrames;
    record.stackFrameCount = (std::min)(stackFrameCount, record.stackFrames.size());

    const auto slot = static_cast<std::size_t>((record.sequence - 1) % _records.size());
    _records[slot] = record;
}

std::optional<sds::NativeEnqueueOriginDiagnostic> sds::NativeEnqueueOriginHistory::findNewest(
    std::uintptr_t eventAddress,
    std::uint32_t timeCode) const noexcept
{
    if (!eventAddress) {
        return std::nullopt;
    }

    const NativeEnqueueOriginDiagnostic* newest = nullptr;
    for (const auto& record : _records) {
        if (record.sequence == 0 || record.eventAddress != eventAddress || record.timeCode != timeCode) {
            continue;
        }
        if (!newest || record.sequence > newest->sequence) {
            newest = &record;
        }
    }
    if (!newest) {
        return std::nullopt;
    }
    return *newest;
}

std::optional<sds::NativeEnqueueOriginDiagnostic> sds::NativeEnqueueOriginHistory::findNewest(
    std::uintptr_t eventAddress) const noexcept
{
    if (!eventAddress) {
        return std::nullopt;
    }

    const NativeEnqueueOriginDiagnostic* newest = nullptr;
    for (const auto& record : _records) {
        if (record.sequence == 0 || record.eventAddress != eventAddress) {
            continue;
        }
        if (!newest || record.sequence > newest->sequence) {
            newest = &record;
        }
    }
    if (!newest) {
        return std::nullopt;
    }
    return *newest;
}

std::string sds::formatNativeEnqueueOrigin(const NativeEnqueueOriginDiagnostic& diagnostic)
{
    const auto inStarfield = [&](std::uintptr_t address) noexcept {
        if (diagnostic.moduleBase == 0 || diagnostic.moduleSize == 0 || address < diagnostic.moduleBase) {
            return false;
        }
        return address - diagnostic.moduleBase < diagnostic.moduleSize;
    };

    std::ostringstream out;
    out << "Native enqueue origin: seq=" << std::dec << diagnostic.sequence
        << std::hex << std::setfill('0')
        << " event=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.eventAddress
        << std::dec << " timeCode=" << diagnostic.timeCode
        << std::hex << " caller=";
    if (inStarfield(diagnostic.callerReturnAddress)) {
        out << "Starfield+0x" << (diagnostic.callerReturnAddress - diagnostic.moduleBase);
    } else {
        out << "outside-Starfield";
    }

    out << " stack=";
    bool wroteFrame = false;
    const auto count = (std::min)(diagnostic.stackFrameCount, diagnostic.stackFrames.size());
    for (std::size_t i = 0; i < count; ++i) {
        const auto frame = diagnostic.stackFrames[i];
        if (!inStarfield(frame) || frame == diagnostic.callerReturnAddress) {
            continue;
        }
        if (wroteFrame) {
            out << " -> ";
        }
        out << "Starfield+0x" << (frame - diagnostic.moduleBase);
        wroteFrame = true;
    }
    if (!wroteFrame) {
        out << "(no additional Starfield frames)";
    }
    return out.str();
}

std::optional<std::array<std::uint8_t, 10>> sds::buildButtonEventConstructorEntryPatch(
    std::uintptr_t sourceAddress,
    std::uintptr_t branchIslandAddress) noexcept
{
    constexpr std::int64_t instructionSize = 5;
    const auto sourceAfterJump = static_cast<std::int64_t>(sourceAddress) + instructionSize;
    const auto displacement = static_cast<std::int64_t>(branchIslandAddress) - sourceAfterJump;
    if (displacement < (std::numeric_limits<std::int32_t>::min)() ||
        displacement > (std::numeric_limits<std::int32_t>::max)()) {
        return std::nullopt;
    }

    const auto rel32 = static_cast<std::int32_t>(displacement);
    std::array<std::uint8_t, 10> patch{ 0xE9, 0, 0, 0, 0, 0x90, 0x90, 0x90, 0x90, 0x90 };
    std::memcpy(patch.data() + 1, &rel32, sizeof(rel32));
    return patch;
}

bool sds::matchesButtonEventConstructorPrologue(const std::array<std::uint8_t, 10>& bytes) noexcept
{
    constexpr std::array<std::uint8_t, 10> expected{
        0x48, 0x89, 0x5C, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18
    };
    return bytes == expected;
}

std::string sds::formatSemanticCallerTrace(const SemanticCallerTraceDiagnostic& diagnostic)
{
    const auto inStarfield = [&](std::uintptr_t address) noexcept {
        if (diagnostic.moduleBase == 0 || diagnostic.moduleSize == 0 || address < diagnostic.moduleBase) {
            return false;
        }
        return address - diagnostic.moduleBase < diagnostic.moduleSize;
    };

    std::ostringstream out;
    out << "Semantic caller trace: edge="
        << (diagnostic.edge == InputButtonEdge::Press ? "press" : "release")
        << std::hex << std::setfill('0')
        << " return=0x"
        << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2))
        << diagnostic.returnAddress;

    if (inStarfield(diagnostic.returnAddress)) {
        out << " returnRva=Starfield+0x" << (diagnostic.returnAddress - diagnostic.moduleBase);
    } else {
        out << " returnRva=outside-Starfield";
    }

    out << " stack=";
    bool wroteFrame = false;
    const auto count = (std::min)(diagnostic.stackFrameCount, diagnostic.stackFrames.size());
    for (std::size_t i = 0; i < count; ++i) {
        const auto frame = diagnostic.stackFrames[i];
        if (!inStarfield(frame)) {
            continue;
        }
        if (wroteFrame) {
            out << " -> ";
        }
        out << "Starfield+0x" << (frame - diagnostic.moduleBase);
        wroteFrame = true;
    }
    if (!wroteFrame) {
        out << "(no Starfield frames)";
    }
    return out.str();
}

std::optional<std::uintptr_t> sds::decodeDirectRel32CallTarget(
    std::uintptr_t callAddress,
    const std::array<std::uint8_t, 5>& instruction) noexcept
{
    if (instruction[0] != 0xE8) {
        return std::nullopt;
    }

    std::int32_t displacement = 0;
    std::memcpy(&displacement, instruction.data() + 1, sizeof(displacement));
    const auto base = static_cast<std::int64_t>(callAddress + instruction.size());
    const auto target = base + static_cast<std::int64_t>(displacement);
    if (target < 0) {
        return std::nullopt;
    }
    return static_cast<std::uintptr_t>(target);
}

std::string sds::formatNativeCodeProbe(
    const NativeCodeProbeDiagnostic& diagnostic,
    std::size_t frameIndex)
{
    const auto inStarfield = [&](std::uintptr_t address) noexcept {
        if (diagnostic.moduleBase == 0 || diagnostic.moduleSize == 0 || address < diagnostic.moduleBase) {
            return false;
        }
        return address - diagnostic.moduleBase < diagnostic.moduleSize;
    };

    std::ostringstream out;
    out << "Semantic upstream probe: frame=" << frameIndex << std::hex << std::setfill('0');

    if (inStarfield(diagnostic.frameAddress)) {
        out << " frameRva=Starfield+0x" << (diagnostic.frameAddress - diagnostic.moduleBase);
    } else {
        out << " frameRva=outside-Starfield";
    }

    if (inStarfield(diagnostic.functionStart) &&
        diagnostic.functionEnd >= diagnostic.functionStart &&
        diagnostic.functionEnd <= diagnostic.moduleBase + diagnostic.moduleSize) {
        out << " function=Starfield+0x" << (diagnostic.functionStart - diagnostic.moduleBase)
            << "..0x" << (diagnostic.functionEnd - diagnostic.moduleBase);
        if (diagnostic.frameAddress >= diagnostic.functionStart) {
            out << " functionOffset=+0x" << (diagnostic.frameAddress - diagnostic.functionStart);
        }
    } else {
        out << " function=unknown";
    }

    if (diagnostic.callFound && inStarfield(diagnostic.callAddress)) {
        out << " callsite=Starfield+0x" << (diagnostic.callAddress - diagnostic.moduleBase);
        switch (diagnostic.callKind) {
        case NativeCallKind::DirectRel32:
            out << " callKind=direct-rel32";
            break;
        case NativeCallKind::IndirectRegister:
            out << " callKind=indirect-register";
            break;
        case NativeCallKind::IndirectMemory:
            out << " callKind=indirect-memory";
            break;
        }
    } else {
        out << " callsite=unknown callKind=unknown";
    }

    if (diagnostic.callTargetDecoded && inStarfield(diagnostic.callTarget)) {
        out << " callTarget=Starfield+0x" << (diagnostic.callTarget - diagnostic.moduleBase);
    } else if (diagnostic.callTargetDecoded) {
        out << " callTarget=outside-Starfield";
    } else {
        out << " callTarget=undecoded";
    }

    if (inStarfield(diagnostic.windowStart)) {
        out << " codeStart=Starfield+0x" << (diagnostic.windowStart - diagnostic.moduleBase);
    } else {
        out << " codeStart=unknown";
    }

    out << " code" << std::dec << diagnostic.codeSize << '=' << std::hex;
    const auto count = (std::min)(diagnostic.codeSize, diagnostic.codeBytes.size());
    for (std::size_t i = 0; i < count; ++i) {
        if (i != 0) {
            out << ' ';
        }
        out << std::setw(2) << static_cast<unsigned int>(diagnostic.codeBytes[i]);
    }
    return out.str();
}

std::optional<sds::NativeCallSite> sds::findNearestCallBefore(
    std::uintptr_t windowStart,
    const std::vector<std::uint8_t>& bytes,
    std::uintptr_t frameAddress) noexcept
{
    if (bytes.empty() || frameAddress <= windowStart) {
        return std::nullopt;
    }

    std::optional<NativeCallSite> nearest;
    const auto consider = [&](NativeCallSite candidate) {
        if (candidate.instructionAddress + candidate.instructionSize > frameAddress) {
            return;
        }
        if (!nearest || candidate.instructionAddress > nearest->instructionAddress) {
            nearest = candidate;
        }
    };

    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const auto address = windowStart + i;

        if (bytes[i] == 0xE8 && i + 5 <= bytes.size()) {
            NativeCallSite candidate{};
            candidate.kind = NativeCallKind::DirectRel32;
            candidate.instructionAddress = address;
            candidate.instructionSize = 5;
            candidate.instructionByteCount = 5;
            std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(i), 5, candidate.instructionBytes.begin());

            std::array<std::uint8_t, 5> instruction{};
            std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(i), 5, instruction.begin());
            if (const auto target = decodeDirectRel32CallTarget(address, instruction)) {
                candidate.targetAddress = *target;
                candidate.targetDecoded = true;
            }
            consider(candidate);
            continue;
        }

        // Do not reinterpret the FF opcode inside a REX-prefixed call as a
        // second, later instruction. The prefixed candidate starts one byte earlier.
        if (bytes[i] == 0xFF && i > 0 && bytes[i - 1] >= 0x40 && bytes[i - 1] <= 0x4F) {
            continue;
        }

        std::size_t opcodeOffset = i;
        bool hasRex = false;
        if (bytes[i] >= 0x40 && bytes[i] <= 0x4F) {
            hasRex = true;
            opcodeOffset = i + 1;
        }
        if (opcodeOffset + 2 > bytes.size() || bytes[opcodeOffset] != 0xFF) {
            continue;
        }

        const auto modrm = bytes[opcodeOffset + 1];
        if (((modrm >> 3) & 0x07U) != 2U) {
            continue;
        }

        const auto mod = (modrm >> 6) & 0x03U;
        const auto rm = modrm & 0x07U;
        std::size_t instructionSize = (hasRex ? 1U : 0U) + 2U;
        if (mod != 3U) {
            // Enough x64 ModRM/SIB sizing for diagnostic call-site discovery.
            if (rm == 4U) {
                const auto sibOffset = opcodeOffset + 2;
                if (sibOffset >= bytes.size()) {
                    continue;
                }
                ++instructionSize;
                const auto sibBase = bytes[sibOffset] & 0x07U;
                if (mod == 0U && sibBase == 5U) {
                    instructionSize += 4;
                }
            }
            if (mod == 0U && rm == 5U) {
                instructionSize += 4;
            } else if (mod == 1U) {
                instructionSize += 1;
            } else if (mod == 2U) {
                instructionSize += 4;
            }
        }
        if (i + instructionSize > bytes.size()) {
            continue;
        }

        NativeCallSite candidate{};
        candidate.kind = mod == 3U ? NativeCallKind::IndirectRegister : NativeCallKind::IndirectMemory;
        candidate.instructionAddress = address;
        candidate.instructionSize = instructionSize;
        candidate.instructionByteCount = (std::min)(instructionSize, candidate.instructionBytes.size());
        std::copy_n(
            bytes.begin() + static_cast<std::ptrdiff_t>(i),
            candidate.instructionByteCount,
            candidate.instructionBytes.begin());
        consider(candidate);
    }

    return nearest;
}

std::vector<sds::NativeRipReference> sds::findRipRelativeReferences(
    std::uintptr_t windowStart,
    const std::vector<std::uint8_t>& bytes,
    std::uintptr_t targetAddress) noexcept
{
    std::vector<NativeRipReference> references;
    if (bytes.empty()) {
        return references;
    }

    for (std::size_t i = 0; i < bytes.size(); ++i) {
        std::size_t opcodeOffset = i;
        bool hasRex = false;
        if (bytes[i] >= 0x40 && bytes[i] <= 0x4F) {
            hasRex = true;
            opcodeOffset = i + 1;
        }
        if (opcodeOffset + 6 > bytes.size()) {
            continue;
        }

        const auto opcode = bytes[opcodeOffset];
        NativeRipReferenceKind kind{};
        switch (opcode) {
        case 0x8D:
            kind = NativeRipReferenceKind::Lea;
            break;
        case 0x8B:
            kind = NativeRipReferenceKind::MovLoad;
            break;
        case 0x89:
            kind = NativeRipReferenceKind::MovStore;
            break;
        default:
            continue;
        }

        const auto modrm = bytes[opcodeOffset + 1];
        const auto mod = (modrm >> 6) & 0x03U;
        const auto rm = modrm & 0x07U;
        if (mod != 0U || rm != 5U) {
            continue;
        }

        const std::size_t instructionSize = (hasRex ? 1U : 0U) + 6U;
        if (i + instructionSize > bytes.size()) {
            continue;
        }

        std::int32_t displacement = 0;
        std::memcpy(&displacement, bytes.data() + opcodeOffset + 2, sizeof(displacement));
        const auto instructionAddress = windowStart + i;
        const auto nextInstruction = static_cast<std::int64_t>(instructionAddress + instructionSize);
        const auto resolvedTarget = nextInstruction + static_cast<std::int64_t>(displacement);
        if (resolvedTarget < 0 || static_cast<std::uintptr_t>(resolvedTarget) != targetAddress) {
            continue;
        }

        NativeRipReference reference{};
        reference.kind = kind;
        reference.instructionAddress = instructionAddress;
        reference.instructionSize = instructionSize;
        reference.targetAddress = static_cast<std::uintptr_t>(resolvedTarget);
        reference.instructionByteCount = (std::min)(instructionSize, reference.instructionBytes.size());
        std::copy_n(
            bytes.begin() + static_cast<std::ptrdiff_t>(i),
            reference.instructionByteCount,
            reference.instructionBytes.begin());
        references.push_back(reference);

        // Do not rediscover the opcode byte of this same instruction as a second candidate.
        i += instructionSize - 1;
    }

    return references;
}

std::string sds::formatNativeFunctionChunk(const NativeFunctionChunkDiagnostic& diagnostic)
{
    const auto inStarfield = [&](std::uintptr_t address) noexcept {
        return diagnostic.moduleBase != 0 && diagnostic.moduleSize != 0 &&
            address >= diagnostic.moduleBase && address - diagnostic.moduleBase < diagnostic.moduleSize;
    };

    std::ostringstream out;
    out << "Semantic pump dump:" << std::hex << std::setfill('0');
    if (inStarfield(diagnostic.functionStart) && diagnostic.functionEnd >= diagnostic.functionStart) {
        out << " function=Starfield+0x" << (diagnostic.functionStart - diagnostic.moduleBase)
            << "..0x" << (diagnostic.functionEnd - diagnostic.moduleBase);
    } else {
        out << " function=unknown";
    }
    out << std::dec << " chunk=" << (diagnostic.chunkIndex + 1) << '/' << diagnostic.chunkCount << std::hex;
    if (inStarfield(diagnostic.chunkStart)) {
        out << " start=Starfield+0x" << (diagnostic.chunkStart - diagnostic.moduleBase);
    } else {
        out << " start=unknown";
    }
    out << " bytes" << std::dec << diagnostic.byteCount << '=' << std::hex;
    const auto count = (std::min)(diagnostic.byteCount, diagnostic.bytes.size());
    for (std::size_t i = 0; i < count; ++i) {
        if (i != 0) {
            out << ' ';
        }
        out << std::setw(2) << static_cast<unsigned int>(diagnostic.bytes[i]);
    }
    return out.str();
}

std::string sds::formatNativeSourceSnapshot(const NativeSourceSnapshotDiagnostic& diagnostic)
{
    std::ostringstream out;
    out << "Semantic source snapshot:" << std::hex << std::setfill('0')
        << " manager=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.managerAddress
        << " observedSource=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.observedSourceAddress
        << " expectedSource=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.expectedSourceAddress
        << " sourceVtable=0x" << std::setw(static_cast<int>(sizeof(std::uintptr_t) * 2)) << diagnostic.sourceVtable
        << std::dec << " sourceMatchesManager=" << (diagnostic.sourceMatchesManager ? "true" : "false")
        << " bytes" << diagnostic.byteCount << '=' << std::hex;
    const auto count = (std::min)(diagnostic.byteCount, diagnostic.bytes.size());
    for (std::size_t i = 0; i < count; ++i) {
        if (i != 0) {
            out << ' ';
        }
        out << std::setw(2) << static_cast<unsigned int>(diagnostic.bytes[i]);
    }
    return out.str();
}

namespace
{
    std::uint32_t nativeButtonFieldMaskForOffset(std::size_t offset) noexcept
    {
        switch (offset) {
        case 0x20:
            return sds::kNativeButtonFieldTimeCode;
        case 0x28:
            return sds::kNativeButtonFieldAction;
        case 0x30:
            return sds::kNativeButtonFieldIdCode;
        case 0x48:
            return sds::kNativeButtonFieldValue;
        case 0x4C:
            return sds::kNativeButtonFieldHeld;
        default:
            return 0;
        }
    }

    std::optional<std::pair<std::size_t, std::size_t>> decodeMemoryDisplacement(
        const std::vector<std::uint8_t>& bytes,
        std::size_t modrmOffset) noexcept
    {
        if (modrmOffset >= bytes.size()) {
            return std::nullopt;
        }
        const auto modrm = bytes[modrmOffset];
        const auto mod = (modrm >> 6) & 0x03U;
        const auto rm = modrm & 0x07U;
        if (mod == 0U || mod == 3U || rm == 4U) {
            return std::nullopt;
        }
        const std::size_t displacementSize = mod == 1U ? 1U : 4U;
        const auto displacementOffset = modrmOffset + 1U;
        if (displacementOffset + displacementSize > bytes.size()) {
            return std::nullopt;
        }
        std::int64_t displacement = 0;
        if (displacementSize == 1U) {
            displacement = static_cast<std::int8_t>(bytes[displacementOffset]);
        } else {
            std::int32_t value = 0;
            std::memcpy(&value, bytes.data() + displacementOffset, sizeof(value));
            displacement = value;
        }
        if (displacement < 0) {
            return std::nullopt;
        }
        return std::pair{ static_cast<std::size_t>(displacement), displacementSize };
    }
}

std::vector<sds::NativeButtonFieldWrite> sds::findButtonEventFieldWrites(
    std::uintptr_t windowStart,
    const std::vector<std::uint8_t>& bytes) noexcept
{
    std::vector<NativeButtonFieldWrite> writes;
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        std::size_t opcodeOffset = i;
        if (bytes[opcodeOffset] >= 0x40 && bytes[opcodeOffset] <= 0x4F) {
            ++opcodeOffset;
        }
        if (opcodeOffset >= bytes.size()) {
            continue;
        }

        std::size_t modrmOffset = 0;
        std::size_t baseInstructionSize = 0;
        std::size_t immediateSize = 0;
        const auto opcode = bytes[opcodeOffset];
        if (opcode == 0x89 || opcode == 0x88) {
            modrmOffset = opcodeOffset + 1;
            baseInstructionSize = (opcodeOffset - i) + 2;
        } else if (opcode == 0xC7 || opcode == 0xC6) {
            modrmOffset = opcodeOffset + 1;
            baseInstructionSize = (opcodeOffset - i) + 2;
            immediateSize = opcode == 0xC7 ? 4U : 1U;
        } else if (opcode == 0xF3 && opcodeOffset + 3 < bytes.size() &&
                   bytes[opcodeOffset + 1] == 0x0F && bytes[opcodeOffset + 2] == 0x11) {
            modrmOffset = opcodeOffset + 3;
            baseInstructionSize = (opcodeOffset - i) + 4;
        } else if (bytes[i] == 0xC5 && i + 4 < bytes.size() && bytes[i + 2] == 0x11) {
            modrmOffset = i + 3;
            baseInstructionSize = 4;
        } else if (bytes[i] == 0xC4 && i + 5 < bytes.size() && bytes[i + 3] == 0x11) {
            modrmOffset = i + 4;
            baseInstructionSize = 5;
        } else {
            continue;
        }

        const auto displacement = decodeMemoryDisplacement(bytes, modrmOffset);
        if (!displacement) {
            continue;
        }
        const auto [fieldOffset, displacementSize] = *displacement;
        const auto fieldMask = nativeButtonFieldMaskForOffset(fieldOffset);
        if (!fieldMask) {
            continue;
        }

        const auto instructionSize = baseInstructionSize + displacementSize + immediateSize;
        if (i + instructionSize > bytes.size()) {
            continue;
        }
        NativeButtonFieldWrite write{};
        write.fieldMask = fieldMask;
        write.fieldOffset = fieldOffset;
        write.instructionAddress = windowStart + i;
        write.instructionByteCount = (std::min)(instructionSize, write.instructionBytes.size());
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(i),
            write.instructionByteCount, write.instructionBytes.begin());
        writes.push_back(write);
        i += instructionSize - 1;
    }
    return writes;
}

std::vector<sds::NativeCallSite> sds::findDirectRel32CallsToTarget(
    std::uintptr_t windowStart,
    const std::vector<std::uint8_t>& bytes,
    std::uintptr_t targetAddress) noexcept
{
    std::vector<NativeCallSite> matches;
    if (bytes.size() < 5 || !targetAddress) {
        return matches;
    }

    for (std::size_t i = 0; i + 5 <= bytes.size(); ++i) {
        if (bytes[i] != 0xE8) {
            continue;
        }
        std::array<std::uint8_t, 5> instruction{};
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(i), 5, instruction.begin());
        const auto address = windowStart + i;
        const auto target = decodeDirectRel32CallTarget(address, instruction);
        if (!target || *target != targetAddress) {
            continue;
        }

        NativeCallSite call{};
        call.kind = NativeCallKind::DirectRel32;
        call.instructionAddress = address;
        call.instructionSize = 5;
        call.targetAddress = *target;
        call.targetDecoded = true;
        call.instructionByteCount = 5;
        std::copy(instruction.begin(), instruction.end(), call.instructionBytes.begin());
        matches.push_back(call);
        i += 4;
    }
    return matches;
}

std::optional<std::array<std::uint8_t, 5>> sds::buildRel32CallPatch(
    std::uintptr_t sourceAddress,
    std::uintptr_t targetAddress) noexcept
{
    constexpr std::int64_t instructionSize = 5;
    const auto sourceAfterCall = static_cast<std::int64_t>(sourceAddress) + instructionSize;
    const auto displacement = static_cast<std::int64_t>(targetAddress) - sourceAfterCall;
    if (displacement < (std::numeric_limits<std::int32_t>::min)() ||
        displacement > (std::numeric_limits<std::int32_t>::max)()) {
        return std::nullopt;
    }

    const auto rel32 = static_cast<std::int32_t>(displacement);
    std::array<std::uint8_t, 5> patch{ 0xE8, 0, 0, 0, 0 };
    std::memcpy(patch.data() + 1, &rel32, sizeof(rel32));
    return patch;
}

std::vector<sds::NativeCallSite> sds::findNativeCallSites(
    std::uintptr_t windowStart,
    const std::vector<std::uint8_t>& bytes) noexcept
{
    std::vector<NativeCallSite> calls;
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const auto address = windowStart + i;
        if (bytes[i] == 0xE8 && i + 5 <= bytes.size()) {
            NativeCallSite call{};
            call.kind = NativeCallKind::DirectRel32;
            call.instructionAddress = address;
            call.instructionSize = 5;
            call.instructionByteCount = 5;
            std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(i), 5, call.instructionBytes.begin());
            std::array<std::uint8_t, 5> instruction{};
            std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(i), 5, instruction.begin());
            if (const auto target = decodeDirectRel32CallTarget(address, instruction)) {
                call.targetAddress = *target;
                call.targetDecoded = true;
            }
            calls.push_back(call);
            i += 4;
            continue;
        }

        std::size_t opcodeOffset = i;
        bool hasRex = false;
        if (bytes[i] >= 0x40 && bytes[i] <= 0x4F) {
            hasRex = true;
            opcodeOffset = i + 1;
        }
        if (opcodeOffset + 2 > bytes.size() || bytes[opcodeOffset] != 0xFF) {
            continue;
        }
        const auto modrm = bytes[opcodeOffset + 1];
        if (((modrm >> 3) & 0x07U) != 2U) {
            continue;
        }
        const auto mod = (modrm >> 6) & 0x03U;
        const auto rm = modrm & 0x07U;
        std::size_t instructionSize = (hasRex ? 1U : 0U) + 2U;
        if (mod != 3U) {
            if (rm == 4U) {
                const auto sibOffset = opcodeOffset + 2;
                if (sibOffset >= bytes.size()) {
                    continue;
                }
                ++instructionSize;
                const auto sibBase = bytes[sibOffset] & 0x07U;
                if (mod == 0U && sibBase == 5U) {
                    instructionSize += 4;
                }
            }
            if (mod == 0U && rm == 5U) {
                instructionSize += 4;
            } else if (mod == 1U) {
                instructionSize += 1;
            } else if (mod == 2U) {
                instructionSize += 4;
            }
        }
        if (i + instructionSize > bytes.size()) {
            continue;
        }
        NativeCallSite call{};
        call.kind = mod == 3U ? NativeCallKind::IndirectRegister : NativeCallKind::IndirectMemory;
        call.instructionAddress = address;
        call.instructionSize = instructionSize;
        call.instructionByteCount = (std::min)(instructionSize, call.instructionBytes.size());
        std::copy_n(bytes.begin() + static_cast<std::ptrdiff_t>(i),
            call.instructionByteCount, call.instructionBytes.begin());
        calls.push_back(call);
        i += instructionSize - 1;
    }
    return calls;
}
