#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string_view>

namespace sds
{
    struct FireMarkerCaptureContext
    {
        bool isPlayerEvent{ false };
        std::chrono::steady_clock::time_point now{};
        std::chrono::steady_clock::time_point captureUntil{};
        std::string_view tag{};
    };

    [[nodiscard]] constexpr bool shouldCaptureFireMarker(
        const FireMarkerCaptureContext& context) noexcept
    {
        return context.isPlayerEvent && !context.tag.empty() &&
            context.now < context.captureUntil;
    }

    enum class FireMarkerRecordKind : std::uint8_t
    {
        Marker,
        LimitReached,
    };

    struct FireMarkerRecord
    {
        FireMarkerRecordKind kind{ FireMarkerRecordKind::Marker };
        std::uint64_t sequence{ 0 };
        std::int64_t elapsedUs{ 0 };
        std::uintptr_t source{ 0 };
        std::array<char, 121> weapon{};
        std::array<char, 241> tag{};
        std::array<char, 241> payload{};
    };

    class FireMarkerCaptureState
    {
    public:
        static constexpr std::size_t kMaxLogRecords = 1024;

        void arm(
            std::string_view weapon,
            std::chrono::steady_clock::time_point now,
            std::chrono::steady_clock::duration duration) noexcept
        {
            std::scoped_lock lock(_mutex);
            copySanitized(_weapon, weapon);
            _captureStarted = now;
            _captureUntil = now + duration;
            _logRecordCount = 0;
            _armed = true;
        }

        void disarm() noexcept
        {
            std::scoped_lock lock(_mutex);
            _armed = false;
        }

        [[nodiscard]] bool active(std::chrono::steady_clock::time_point now) const noexcept
        {
            std::scoped_lock lock(_mutex);
            return _armed && now < _captureUntil;
        }

        [[nodiscard]] bool expired(std::chrono::steady_clock::time_point now) const noexcept
        {
            std::scoped_lock lock(_mutex);
            return _armed && now >= _captureUntil;
        }

        [[nodiscard]] std::optional<FireMarkerRecord> tryCapture(
            bool isPlayerEvent,
            std::chrono::steady_clock::time_point now,
            std::string_view tag,
            std::string_view payload,
            std::uintptr_t source) noexcept
        {
            std::scoped_lock lock(_mutex);
            if (!_armed || !shouldCaptureFireMarker({
                    .isPlayerEvent = isPlayerEvent,
                    .now = now,
                    .captureUntil = _captureUntil,
                    .tag = tag }) ||
                _logRecordCount >= kMaxLogRecords) {
                return std::nullopt;
            }

            FireMarkerRecord record{};
            record.elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(
                now - _captureStarted).count();
            record.source = source;
            record.weapon = _weapon;

            // Reserve the final physical log line for a single suppression
            // notice. Every callback after it is dropped under the same lock.
            if (_logRecordCount == kMaxLogRecords - 1) {
                record.kind = FireMarkerRecordKind::LimitReached;
                ++_logRecordCount;
                return record;
            }

            record.kind = FireMarkerRecordKind::Marker;
            record.sequence = ++_sequence;
            copySanitized(record.tag, tag);
            copySanitized(record.payload, payload);
            ++_logRecordCount;
            return record;
        }

    private:
        template <std::size_t N>
        static void copySanitized(std::array<char, N>& destination, std::string_view source) noexcept
        {
            destination.fill('\0');
            const auto count = (std::min)(source.size(), destination.size() - 1);
            for (std::size_t i = 0; i < count; ++i) {
                const auto value = static_cast<unsigned char>(source[i]);
                destination[i] = value < 0x20 || value == 0x7F ? ' ' : source[i];
            }
        }

        mutable std::mutex _mutex{};
        bool _armed{ false };
        std::size_t _logRecordCount{ 0 };
        std::uint64_t _sequence{ 0 };
        std::array<char, 121> _weapon{};
        std::chrono::steady_clock::time_point _captureStarted{};
        std::chrono::steady_clock::time_point _captureUntil{};
    };
}
