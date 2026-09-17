#include <StarfieldDualSense/MaelstromSpeakerFireProof.h>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <utility>

bool sds::shapeMaelstromSpeakerFirePcm(PreparedSpeakerPcm& pcm) noexcept
{
    try {
        if (pcm.frames.empty()) {
            return false;
        }

        if (pcm.frames.size() > kMaelstromSpeakerFireMaxFrames) {
            pcm.frames.resize(kMaelstromSpeakerFireMaxFrames);
        }
        pcm.gain = kMaelstromSpeakerFireGain;

        const auto fadeFrames = (std::min)(kMaelstromSpeakerFireFadeFrames, pcm.frames.size());
        if (fadeFrames == 0u) {
            return false;
        }
        const auto fadeStart = pcm.frames.size() - fadeFrames;
        if (fadeFrames == 1u) {
            pcm.frames.back() = {};
            return true;
        }

        const auto denominator = static_cast<float>(fadeFrames - 1u);
        for (std::size_t offset = 0; offset < fadeFrames; ++offset) {
            const auto remaining = static_cast<float>(fadeFrames - 1u - offset);
            const float scale = remaining / denominator;
            auto& frame = pcm.frames[fadeStart + offset];
            frame.left *= scale;
            frame.right *= scale;
        }
        return true;
    } catch (...) {
        return false;
    }
}

sds::MaelstromSpeakerFireProof::MaelstromSpeakerFireProof(
    SubmitCallback submit,
    LogCallback log,
    bool debugLogging) :
    _submit(std::move(submit)),
    _log(std::move(log)),
    _debugLogging(debugLogging)
{}

std::string_view sds::MaelstromSpeakerFireProof::eventText(const GameEvent& event) noexcept
{
    const auto end = std::find(event.text.begin(), event.text.end(), '\0');
    return std::string_view(event.text.data(), static_cast<std::size_t>(end - event.text.begin()));
}

void sds::MaelstromSpeakerFireProof::logLine(std::string_view line) const noexcept
{
    if (!_debugLogging || !_log) {
        return;
    }
    try {
        _log(line);
    } catch (...) {
    }
}

bool sds::MaelstromSpeakerFireProof::setVariants(
    std::vector<MaelstromSpeakerFireVariant> variants) noexcept
{
    try {
        std::sort(variants.begin(), variants.end(), [](const auto& left, const auto& right) {
            return left.variant < right.variant;
        });

        bool valid = variants.size() == 6u;
        if (valid) {
            for (std::size_t index = 0; index < variants.size(); ++index) {
                const auto expected = static_cast<std::uint8_t>(index + 1u);
                if (variants[index].variant != expected || variants[index].mediaId == 0u ||
                    variants[index].pcm.frames.empty()) {
                    valid = false;
                    break;
                }
            }
        }

        std::scoped_lock lock(_mutex);
        _variants.clear();
        _nextIndex = 0u;
        _maelstromEquipped = false;
        _stats = {};
        if (!valid) {
            logLine("Maelstrom live fire speaker: cache rejected; expected six prepared PC_V3_01..06 variants");
            return false;
        }

        _variants = std::move(variants);
        _stats.cachedVariants = _variants.size();
        _stats.nextVariant = 1u;
        logLine("Maelstrom live fire speaker: cache ready variants=6 order=01,02,03,04,05,06 maxMs=600 fadeMs=10 gain=0.35 source=confirmed-WeaponFire");
        return true;
    } catch (...) {
        return false;
    }
}

bool sds::MaelstromSpeakerFireProof::observe(const GameEvent& event) noexcept
{
    try {
        const auto text = eventText(event);
        std::scoped_lock lock(_mutex);

        if (event.type == GameEventType::WeaponEquipped) {
            _maelstromEquipped = text == "Maelstrom";
            if (_maelstromEquipped) {
                _nextIndex = 0u;
                _stats.nextVariant = _variants.size() == 6u ? 1u : 0u;
                logLine("Maelstrom live fire speaker: armed weapon=Maelstrom source=exact-profile");
            }
            return false;
        }

        if (event.type != GameEventType::WeaponFired || text != "WeaponFire" ||
            !_maelstromEquipped || _variants.size() != 6u || !_submit) {
            return false;
        }

        const auto& selected = _variants[_nextIndex];
        _nextIndex = (_nextIndex + 1u) % _variants.size();
        ++_stats.shotsObserved;
        _stats.nextVariant = _variants[_nextIndex].variant;

        bool accepted = false;
        try {
            accepted = _submit(selected.pcm, selected.mediaId, selected.variant);
        } catch (...) {
            accepted = false;
        }
        if (accepted) {
            ++_stats.shotsSubmitted;
        } else {
            ++_stats.submissionsRejected;
        }

        if (_debugLogging && _stats.shotsObserved <= 6u) {
            std::ostringstream line;
            line << "Maelstrom live fire speaker: shot=" << _stats.shotsObserved
                 << " variant=" << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(selected.variant)
                 << " mediaId=" << selected.mediaId
                 << " submit=" << (accepted ? "accepted" : "rejected")
                 << " frames=" << selected.pcm.frames.size()
                 << " gain=" << std::fixed << std::setprecision(2) << selected.pcm.gain
                 << " source=confirmed-WeaponFire";
            logLine(line.str());
        } else if (_debugLogging && (_stats.shotsObserved % 32u) == 0u) {
            std::ostringstream line;
            line << "Maelstrom live fire speaker summary: shots=" << _stats.shotsObserved
                 << " submitted=" << _stats.shotsSubmitted
                 << " rejected=" << _stats.submissionsRejected
                 << " nextVariant=" << std::setw(2) << std::setfill('0')
                 << static_cast<unsigned int>(_stats.nextVariant)
                 << " cache=" << _stats.cachedVariants;
            logLine(line.str());
        }

        return accepted;
    } catch (...) {
        return false;
    }
}

bool sds::MaelstromSpeakerFireProof::ready() const noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        return _variants.size() == 6u;
    } catch (...) {
        return false;
    }
}

sds::MaelstromSpeakerFireStats sds::MaelstromSpeakerFireProof::stats() const noexcept
{
    try {
        std::scoped_lock lock(_mutex);
        return _stats;
    } catch (...) {
        return {};
    }
}
