#pragma once

#include <functional>
#include <string_view>

namespace sds
{
    using WwiseReconLog = std::function<void(std::string_view)>;

    [[nodiscard]] bool runWwiseApiRecon(WwiseReconLog log) noexcept;
}
