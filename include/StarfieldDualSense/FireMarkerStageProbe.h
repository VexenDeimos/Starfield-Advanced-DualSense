#pragma once

#include <StarfieldDualSense/FireMarkerStageCounters.h>
#include <StarfieldDualSense/FireMarkerSourceScope.h>

#include <RE/Starfield.h>
#include <RE/B/BSAnimationGraph.h>

#include <Windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sds
{
    class FireMarkerStageProbe final :
        public RE::BSTEventSink<RE::ActorItemEquipped::Event>,
        public RE::BSTEventSink<RE::BSAnimationGraphEvent>
    {
    public:
        using LogCallback = std::function<void(std::string_view)>;

        explicit FireMarkerStageProbe(LogCallback log = {}) :
            _log(std::move(log))
        {}

        ~FireMarkerStageProbe() override
        {
            stop();
        }

        FireMarkerStageProbe(const FireMarkerStageProbe&) = delete;
        FireMarkerStageProbe& operator=(const FireMarkerStageProbe&) = delete;

        bool start()
        {
            std::scoped_lock lock(_mutex);
            if (_started) {
                return true;
            }

            auto* equipSource = RE::ActorItemEquipped::Event::GetEventSource();
            if (!equipSource) {
                log("Fire marker layout diagnostic: player equip source unavailable");
                return false;
            }

            equipSource->RegisterSink(this);
            _started = true;
            log("Fire marker layout diagnostic: ACTIVE; player scope uses exact registered graph-source identity; first callbacks inspect bounded event/pointee memory; h4 arbitration and controller behavior unchanged");
            return true;
        }

        void stop() noexcept
        {
            try {
                std::scoped_lock lock(_mutex);
                if (_captureActive.load(std::memory_order_acquire) || !_graphs.empty()) {
                    closeCaptureLocked("shutdown");
                }
                if (_started) {
                    if (auto* equipSource = RE::ActorItemEquipped::Event::GetEventSource()) {
                        equipSource->UnregisterSink(this);
                    }
                }
                _started = false;
            } catch (...) {
                _captureActive.store(false, std::memory_order_release);
                _graphs.clear();
                clearRegisteredPlayerGraphSourcesLocked();
                _started = false;
            }
        }

        void tick() noexcept
        {
            if (!_captureActive.load(std::memory_order_acquire)) {
                return;
            }

            const auto nowTicks = steadyTicks(std::chrono::steady_clock::now());
            if (nowTicks < _deadlineTicks.load(std::memory_order_acquire)) {
                return;
            }

            try {
                std::scoped_lock lock(_mutex);
                if (_captureActive.load(std::memory_order_acquire) &&
                    steadyTicks(std::chrono::steady_clock::now()) >= _deadlineTicks.load(std::memory_order_acquire)) {
                    closeCaptureLocked("timeout");
                }
            } catch (...) {
                // Diagnostic teardown must never escape into the permanent SFSE task.
            }
        }

        RE::BSEventNotifyControl ProcessEvent(
            const RE::ActorItemEquipped::Event& event,
            RE::BSTEventSource<RE::ActorItemEquipped::Event>*) override
        {
            auto* player = RE::PlayerCharacter::GetSingleton();
            if (!player || event.actor.get() != player || !event.item ||
                event.item->GetFormType() != RE::FormType::kWEAP) {
                return RE::BSEventNotifyControl::kContinue;
            }

            try {
                armCapture(player, weaponIdentity(event.item));
            } catch (...) {
                log("Fire marker layout diagnostic: exception while arming capture window");
            }
            return RE::BSEventNotifyControl::kContinue;
        }

        RE::BSEventNotifyControl ProcessEvent(
            const RE::BSAnimationGraphEvent& event,
            RE::BSTEventSource<RE::BSAnimationGraphEvent>* source) override
        {
            if (!_captureActive.load(std::memory_order_acquire) ||
                steadyTicks(std::chrono::steady_clock::now()) >= _deadlineTicks.load(std::memory_order_acquire)) {
                return RE::BSEventNotifyControl::kContinue;
            }

            _stages.callbackEntered();

            RawAnimationEvent raw{};
            const bool snapshotOk = safeSnapshotEvent(event, raw);
            if (snapshotOk) {
                _stages.snapshotSucceeded();
            }

            const bool playerSourceMatch = sourceMatchesRegisteredPlayerGraph(source);
            if (snapshotOk && playerSourceMatch) {
                _stages.holderMatchedPlayer();
            }

            const auto sampleIndex = _sampleSequence.fetch_add(1, std::memory_order_relaxed);
            if (sampleIndex < kMaxCallbackSamples) {
                RawEventWindow window{};
                const bool windowOk = safeSnapshotEventWindow(event, window);
                logLayoutSample(
                    sampleIndex + 1,
                    source,
                    std::addressof(event),
                    snapshotOk,
                    playerSourceMatch,
                    windowOk,
                    window);

                if (windowOk && playerSourceMatch) {
                    for (std::size_t index = 0; index < window.words.size(); ++index) {
                        if (window.words[index] != 0) {
                            logPointerInspection(
                                sampleIndex + 1,
                                index * sizeof(std::uintptr_t),
                                window.words[index]);
                        }
                    }
                }
            }

            return RE::BSEventNotifyControl::kContinue;
        }

    private:
        struct RawAnimationEvent
        {
            std::array<std::uintptr_t, 3> words{};
        };
        static_assert(sizeof(RawAnimationEvent) == 0x18);

        struct RawEventWindow
        {
            std::array<std::uintptr_t, 8> words{};
        };
        static_assert(sizeof(RawEventWindow) == 0x40);

        static constexpr auto kCaptureDuration = std::chrono::seconds(20);
        static constexpr std::uint64_t kMaxCallbackSamples = 8;
        static constexpr std::size_t kMaxStringBytes = 96;
        static constexpr std::size_t kMaxPointeeWords = 8;
        static constexpr std::size_t kMaxPlayerGraphSources = 8;

        static std::int64_t steadyTicks(std::chrono::steady_clock::time_point time) noexcept
        {
            return std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();
        }

        static std::string weaponIdentity(const RE::TESForm* item)
        {
            if (!item) {
                return "<unknown>";
            }

            std::string identity;
            if (const char* editorId = item->GetFormEditorID(); editorId && *editorId) {
                identity = editorId;
            }
            if (const auto* weapon = static_cast<const RE::TESObjectWEAP*>(item)) {
                if (const char* fullName = weapon->GetFullName(); fullName && *fullName) {
                    if (!identity.empty()) {
                        identity += '|';
                    }
                    identity += fullName;
                }
            }
            return identity.empty() ? "<unknown>" : identity;
        }

        static bool safeSnapshotEvent(const RE::BSAnimationGraphEvent& event, RawAnimationEvent& out) noexcept
        {
            SIZE_T bytesRead = 0;
            return ::ReadProcessMemory(
                       ::GetCurrentProcess(),
                       std::addressof(event),
                       std::addressof(out),
                       sizeof(out),
                       std::addressof(bytesRead)) != FALSE &&
                bytesRead == sizeof(out);
        }

        static bool safeSnapshotEventWindow(const RE::BSAnimationGraphEvent& event, RawEventWindow& out) noexcept
        {
            SIZE_T bytesRead = 0;
            return ::ReadProcessMemory(
                       ::GetCurrentProcess(),
                       std::addressof(event),
                       std::addressof(out),
                       sizeof(out),
                       std::addressof(bytesRead)) != FALSE &&
                bytesRead == sizeof(out);
        }

        static std::size_t safeReadQwords(
            std::uintptr_t address,
            std::array<std::uintptr_t, kMaxPointeeWords>& out) noexcept
        {
            if (!address) {
                return 0;
            }

            SIZE_T bytesRead = 0;
            if (::ReadProcessMemory(
                    ::GetCurrentProcess(),
                    reinterpret_cast<const void*>(address),
                    out.data(),
                    sizeof(out),
                    std::addressof(bytesRead)) == FALSE ||
                bytesRead < sizeof(std::uintptr_t)) {
                return 0;
            }
            return (std::min)(out.size(), static_cast<std::size_t>(bytesRead / sizeof(std::uintptr_t)));
        }

        static std::string safeReadString(std::uintptr_t address) noexcept
        {
            if (!address) {
                return {};
            }

            std::array<char, kMaxStringBytes> bytes{};
            SIZE_T bytesRead = 0;
            if (::ReadProcessMemory(
                    ::GetCurrentProcess(),
                    reinterpret_cast<const void*>(address),
                    bytes.data(),
                    bytes.size() - 1,
                    std::addressof(bytesRead)) == FALSE ||
                bytesRead == 0) {
                return {};
            }

            const auto limit = (std::min)(static_cast<std::size_t>(bytesRead), bytes.size() - 1);
            std::size_t length = 0;
            while (length < limit && bytes[length] != '\0') {
                const auto value = static_cast<unsigned char>(bytes[length]);
                if (value < 0x20 || value > 0x7E) {
                    return {};
                }
                ++length;
            }
            if (length == 0 || length == limit) {
                return {};
            }
            return std::string(bytes.data(), length);
        }

        void armCapture(RE::PlayerCharacter* player, std::string weapon)
        {
            std::scoped_lock lock(_mutex);
            if (_captureActive.load(std::memory_order_acquire) || !_graphs.empty()) {
                closeCaptureLocked("rearmed");
            }

            const auto graphCount = reconcilePlayerGraphsLocked(player);
            _weapon = std::move(weapon);
            _stages.reset();
            _sampleSequence.store(0, std::memory_order_relaxed);
            _deadlineTicks.store(
                steadyTicks(std::chrono::steady_clock::now() + kCaptureDuration),
                std::memory_order_release);
            _captureActive.store(true, std::memory_order_release);

            char buffer[384]{};
            std::snprintf(
                buffer,
                sizeof(buffer),
                "Fire marker layout diagnostic: capture armed weapon='%s' graphs=%zu duration=20s; fire immediately",
                _weapon.c_str(),
                graphCount);
            log(buffer);
        }

        std::size_t reconcilePlayerGraphsLocked(RE::PlayerCharacter* player)
        {
            _graphs.clear();
            clearRegisteredPlayerGraphSourcesLocked();
            if (!player) {
                log("Fire marker layout diagnostic: player unavailable while reconciling animation graphs");
                return 0;
            }

            RE::BSTSmartPointer<RE::BSAnimationGraphManager> manager;
            if (!player->GetAnimationGraphManagerImpl(manager) || !manager) {
                log("Fire marker layout diagnostic: player animation graph manager unavailable");
                return 0;
            }

            const auto graphCount = static_cast<std::size_t>(manager->graphs.size());
            _graphs.reserve((std::min)(graphCount, kMaxPlayerGraphSources));
            for (std::uint32_t index = 0;
                 index < manager->graphs.size() && _graphs.size() < kMaxPlayerGraphSources;
                 ++index) {
                auto graph = manager->graphs[index];
                if (!graph) {
                    continue;
                }
                auto* source = static_cast<RE::BSTEventSource<RE::BSAnimationGraphEvent>*>(graph.get());
                source->RegisterSink(this);
                const auto registryIndex = _graphs.size();
                _playerGraphSources[registryIndex].store(
                    reinterpret_cast<std::uintptr_t>(source),
                    std::memory_order_relaxed);
                _graphs.push_back(std::move(graph));
            }
            _registeredGraphSourceCount.store(_graphs.size(), std::memory_order_release);

            char buffer[320]{};
            std::snprintf(
                buffer,
                sizeof(buffer),
                "Fire marker layout diagnostic: reconciled player animation graphs added=%zu total=%zu sourceIdentityGate=exact",
                _graphs.size(),
                _graphs.size());
            log(buffer);
            return _graphs.size();
        }

        void clearRegisteredPlayerGraphSourcesLocked() noexcept
        {
            _registeredGraphSourceCount.store(0, std::memory_order_release);
            for (auto& source : _playerGraphSources) {
                source.store(0, std::memory_order_relaxed);
            }
        }

        bool sourceMatchesRegisteredPlayerGraph(
            RE::BSTEventSource<RE::BSAnimationGraphEvent>* source) const noexcept
        {
            std::array<std::uintptr_t, kMaxPlayerGraphSources> registered{};
            const auto count = (std::min)(
                _registeredGraphSourceCount.load(std::memory_order_acquire),
                kMaxPlayerGraphSources);
            for (std::size_t index = 0; index < count; ++index) {
                registered[index] = _playerGraphSources[index].load(std::memory_order_acquire);
            }
            return isRegisteredPlayerGraphSource(
                reinterpret_cast<std::uintptr_t>(source),
                registered,
                count);
        }

        void closeCaptureLocked(std::string_view reason) noexcept
        {
            _captureActive.store(false, std::memory_order_release);

            const auto stages = _stages.snapshot();
            log(formatFireMarkerStageSummary(_weapon.empty() ? std::string_view("<unknown>") : std::string_view(_weapon), stages));

            for (auto& graph : _graphs) {
                try {
                    if (graph) {
                        auto* source = static_cast<RE::BSTEventSource<RE::BSAnimationGraphEvent>*>(graph.get());
                        source->UnregisterSink(this);
                    }
                } catch (...) {
                    // Teardown is diagnostic-only and must fail soft.
                }
            }
            _graphs.clear();
            clearRegisteredPlayerGraphSourcesLocked();

            char buffer[256]{};
            std::snprintf(
                buffer,
                sizeof(buffer),
                "Fire marker trace: capture window closed; stage-counter graph sinks released reason=%.*s",
                static_cast<int>(reason.size()),
                reason.data());
            log(buffer);
        }

        void logLayoutSample(
            std::uint64_t sequence,
            RE::BSTEventSource<RE::BSAnimationGraphEvent>* source,
            const void* event,
            bool snapshotOk,
            bool playerSourceMatch,
            bool windowOk,
            const RawEventWindow& window) const noexcept
        {
            std::ostringstream words;
            if (windowOk) {
                for (std::size_t index = 0; index < window.words.size(); ++index) {
                    if (index != 0) {
                        words << '/';
                    }
                    char value[24]{};
                    std::snprintf(
                        value,
                        sizeof(value),
                        "%016llX",
                        static_cast<unsigned long long>(window.words[index]));
                    words << value;
                }
            } else {
                words << "<unavailable>";
            }

            char buffer[1400]{};
            std::snprintf(
                buffer,
                sizeof(buffer),
                "Fire marker layout sample: seq=%llu source=0x%llX playerSource=%s event=0x%llX snapshot=%s window=%s words=%s",
                static_cast<unsigned long long>(sequence),
                static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(source)),
                playerSourceMatch ? "yes" : "no",
                static_cast<unsigned long long>(reinterpret_cast<std::uintptr_t>(event)),
                snapshotOk ? "yes" : "no",
                windowOk ? "yes" : "no",
                words.str().c_str());
            log(buffer);
        }

        void logPointerInspection(
            std::uint64_t sequence,
            std::size_t eventOffset,
            std::uintptr_t candidate) const noexcept
        {
            const auto direct = safeReadString(candidate);

            std::array<std::uintptr_t, kMaxPointeeWords> pointee{};
            const auto qwordCount = safeReadQwords(candidate, pointee);

            std::ostringstream qwords;
            for (std::size_t index = 0; index < qwordCount; ++index) {
                if (index != 0) {
                    qwords << '/';
                }
                char value[24]{};
                std::snprintf(
                    value,
                    sizeof(value),
                    "%016llX",
                    static_cast<unsigned long long>(pointee[index]));
                qwords << value;
            }
            if (qwordCount == 0) {
                qwords << "<unreadable>";
            }

            std::ostringstream strings;
            bool firstString = true;
            if (!direct.empty()) {
                strings << "direct='" << direct << "'";
                firstString = false;
            }
            for (std::size_t index = 0; index < qwordCount; ++index) {
                const auto child = safeReadString(pointee[index]);
                if (child.empty()) {
                    continue;
                }
                if (!firstString) {
                    strings << ';';
                }
                char path[24]{};
                std::snprintf(path, sizeof(path), "+%02llX", static_cast<unsigned long long>(index * sizeof(std::uintptr_t)));
                strings << path << "='" << child << "'";
                firstString = false;
            }
            if (firstString) {
                strings << "<none>";
            }

            char buffer[1800]{};
            std::snprintf(
                buffer,
                sizeof(buffer),
                "Fire marker layout ptr: seq=%llu event+%02llX=0x%llX direct='%s' pointeeQwords=%s strings=[%s]",
                static_cast<unsigned long long>(sequence),
                static_cast<unsigned long long>(eventOffset),
                static_cast<unsigned long long>(candidate),
                direct.c_str(),
                qwords.str().c_str(),
                strings.str().c_str());
            log(buffer);
        }

        void log(std::string_view message) const noexcept
        {
            if (!_log) {
                return;
            }
            try {
                _log(message);
            } catch (...) {
                // Diagnostics are non-critical.
            }
        }

        LogCallback _log{};
        mutable std::mutex _mutex{};
        bool _started{ false };
        std::atomic<bool> _captureActive{ false };
        std::atomic<std::int64_t> _deadlineTicks{ 0 };
        FireMarkerStageCounters _stages{};
        std::atomic<std::uint64_t> _sampleSequence{ 0 };
        std::string _weapon{};
        std::vector<RE::BSTSmartPointer<RE::BSAnimationGraph>> _graphs{};
        std::array<std::atomic<std::uintptr_t>, kMaxPlayerGraphSources> _playerGraphSources{};
        std::atomic<std::size_t> _registeredGraphSourceCount{ 0 };
    };
}
