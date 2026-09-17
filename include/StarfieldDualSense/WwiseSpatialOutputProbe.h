#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>

namespace sds
{
    class WwiseSpatialOutputProbe
    {
    public:
        using LogCallback = std::function<void(std::string_view)>;

        explicit WwiseSpatialOutputProbe(LogCallback log = {});
        ~WwiseSpatialOutputProbe();

        WwiseSpatialOutputProbe(const WwiseSpatialOutputProbe&) = delete;
        WwiseSpatialOutputProbe& operator=(const WwiseSpatialOutputProbe&) = delete;

        [[nodiscard]] bool start();
        void stop() noexcept;
        void observeRightTrigger(std::uint8_t r2, std::chrono::steady_clock::time_point when) noexcept;
        void tick();
        [[nodiscard]] bool active() const noexcept;

        struct Impl;

    private:
        std::unique_ptr<Impl> impl_;
    };
}
