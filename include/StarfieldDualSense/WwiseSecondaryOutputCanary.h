#pragma once

#include <cstdint>
#include <functional>
#include <string_view>

namespace sds
{
    using WwiseSecondaryOutputCanaryLog = std::function<void(std::string_view)>;

    class WwiseSecondaryOutputCanary
    {
    public:
        explicit WwiseSecondaryOutputCanary(WwiseSecondaryOutputCanaryLog log);
        ~WwiseSecondaryOutputCanary();

        WwiseSecondaryOutputCanary(const WwiseSecondaryOutputCanary&) = delete;
        WwiseSecondaryOutputCanary& operator=(const WwiseSecondaryOutputCanary&) = delete;

        [[nodiscard]] bool start() noexcept;
        void stop() noexcept;
        [[nodiscard]] bool active() const noexcept { return active_; }
        [[nodiscard]] std::uint64_t emitterGameObjectId() const noexcept;

    private:
        struct EntryPoints
        {
            std::uintptr_t addOutput{ 0 };
            std::uintptr_t removeOutput{ 0 };
            std::uintptr_t registerGameObj{ 0 };
            std::uintptr_t setListeners{ 0 };
            std::uintptr_t setListenersCore{ 0 };
            std::uintptr_t unregisterGameObj{ 0 };
            std::uintptr_t getIdFromString{ 0 };
        };

        WwiseSecondaryOutputCanaryLog log_;
        EntryPoints entryPoints_{};
        std::uint64_t outputDeviceId_{ 0 };
        std::uint32_t endpointDeviceId_{ 0 };
        bool listenerRegistered_{ false };
        bool emitterRegistered_{ false };
        bool outputAdded_{ false };
        bool active_{ false };
    };
}
