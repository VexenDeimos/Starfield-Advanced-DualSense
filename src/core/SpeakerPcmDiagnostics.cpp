#include <StarfieldDualSense/SpeakerPcmDiagnostics.h>

#include <algorithm>
#include <cmath>

sds::SpeakerPcmLevels sds::measureSpeakerPcmLevels(
    std::span<const StereoSpeakerFrame> frames,
    std::size_t startFrame,
    std::size_t frameCount) noexcept
{
    SpeakerPcmLevels levels{};
    if (startFrame >= frames.size() || frameCount == 0u) {
        return levels;
    }

    const auto available = frames.size() - startFrame;
    const auto count = (std::min)(available, frameCount);
    if (count == 0u) {
        return levels;
    }

    double sumLeft2 = 0.0;
    double sumRight2 = 0.0;
    double sumMono2 = 0.0;
    double sumLeftRight = 0.0;

    for (std::size_t index = 0; index < count; ++index) {
        const auto& frame = frames[startFrame + index];
        const float mono = (frame.left + frame.right) * 0.5F;
        levels.peakLeft = (std::max)(levels.peakLeft, std::fabs(frame.left));
        levels.peakRight = (std::max)(levels.peakRight, std::fabs(frame.right));
        levels.peakMono = (std::max)(levels.peakMono, std::fabs(mono));
        sumLeft2 += static_cast<double>(frame.left) * static_cast<double>(frame.left);
        sumRight2 += static_cast<double>(frame.right) * static_cast<double>(frame.right);
        sumMono2 += static_cast<double>(mono) * static_cast<double>(mono);
        sumLeftRight += static_cast<double>(frame.left) * static_cast<double>(frame.right);
    }

    levels.frames = count;
    const double denominator = static_cast<double>(count);
    levels.rmsLeft = static_cast<float>(std::sqrt(sumLeft2 / denominator));
    levels.rmsRight = static_cast<float>(std::sqrt(sumRight2 / denominator));
    levels.rmsMono = static_cast<float>(std::sqrt(sumMono2 / denominator));

    if (sumLeft2 > 0.0 && sumRight2 > 0.0) {
        levels.correlation = static_cast<float>(std::clamp(
            sumLeftRight / std::sqrt(sumLeft2 * sumRight2), -1.0, 1.0));
    }
    return levels;
}
