#pragma once

#include <functional>
#include <string_view>

namespace sds
{
    class HidWriteTrace
    {
    public:
        using LogCallback = std::function<void(std::string_view)>;

        // Diagnostic-only in-process HID tracing. nativeHandle is the plugin-owned DualSense HANDLE.
        static void start(void* nativeHandle, LogCallback log) noexcept;
        static void stop() noexcept;
    };
}
