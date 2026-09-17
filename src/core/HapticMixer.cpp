#include <StarfieldDualSense/HapticMixer.h>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

sds::HapticBlockStats sds::measureHapticBlock(std::span<const HapticFrame> block) noexcept
{
    HapticBlockStats stats{};
    double sumSquaresCh3 = 0.0;
    double sumSquaresCh4 = 0.0;
    bool foundFirstNonZero = false;

    for (const auto& frame : block) {
        const float ch3 = frame[2];
        const float ch4 = frame[3];
        stats.peakCh3 = (std::max)(stats.peakCh3, std::fabs(ch3));
        stats.peakCh4 = (std::max)(stats.peakCh4, std::fabs(ch4));
        sumSquaresCh3 += static_cast<double>(ch3) * ch3;
        sumSquaresCh4 += static_cast<double>(ch4) * ch4;

        if (ch3 != 0.0F || ch4 != 0.0F) {
            ++stats.nonZeroFrames;
            if (!foundFirstNonZero) {
                stats.firstNonZeroCh3 = ch3;
                stats.firstNonZeroCh4 = ch4;
                foundFirstNonZero = true;
            }
        }
    }

    if (!block.empty()) {
        const double frameCount = static_cast<double>(block.size());
        stats.rmsCh3 = static_cast<float>(std::sqrt(sumSquaresCh3 / frameCount));
        stats.rmsCh4 = static_cast<float>(std::sqrt(sumSquaresCh4 / frameCount));
    }
    return stats;
}

void sds::HapticMixer::add(HapticWaveform waveform)
{
    if (!waveform.empty()) {
        _voices.push_back(Voice{ std::move(waveform), 0 });
    }
}

void sds::HapticMixer::setContinuous(HapticContinuousState state) noexcept
{
    state.gain = std::clamp(state.gain, 0.0F, 1.0F);
    state.level = std::clamp(state.level, 0.0F, 1.0F);
    state.shipLaserGain = std::clamp(state.shipLaserGain, 0.0F, 1.0F);

    const auto isShipContinuous = [](HapticContinuousKind kind) noexcept {
        return kind == HapticContinuousKind::ShipPropulsion ||
            kind == HapticContinuousKind::ShipBoost;
    };

    if (state.kind == HapticContinuousKind::None && state.shipLaserGain <= 0.0F) {
        _continuous = {};
        _shipSmoothedGain = 0.0;
        _shipSmoothedLevel = 0.0;
        _shipLaserBodyPhase = 0.0;
        _shipLaserEdgePhase = 0.0;
        _shipLaserFrames = 0;
        _continuousFrames = 0;
        return;
    }

    const bool wasInactive = _continuous.kind == HapticContinuousKind::None;
    const bool enteringShip = isShipContinuous(state.kind) &&
        !isShipContinuous(_continuous.kind);
    const bool enteringLaser = _continuous.shipLaserGain <= 0.0F && state.shipLaserGain > 0.0F;
    const bool leavingLaser = _continuous.shipLaserGain > 0.0F && state.shipLaserGain <= 0.0F;
    _continuous = state;
    if (wasInactive && state.kind != HapticContinuousKind::None) {
        _continuousFrames = 0;
    }
    if (state.kind == HapticContinuousKind::None) {
        _shipSmoothedGain = 0.0;
        _shipSmoothedLevel = 0.0;
        _continuousFrames = 0;
    } else if (enteringShip) {
        _shipSmoothedGain = 0.0;
        _shipSmoothedLevel = 0.0;
    }
    if (enteringLaser || leavingLaser) {
        _shipLaserBodyPhase = 0.0;
        _shipLaserEdgePhase = 0.0;
        _shipLaserFrames = 0;
    }
}

void sds::HapticMixer::render(std::span<HapticFrame> output) noexcept
{
    for (auto& frame : output) {
        frame = HapticFrame{};
    }

    for (auto& voice : _voices) {
        for (std::size_t out = 0; out < output.size() && voice.cursor < voice.waveform.size(); ++out, ++voice.cursor) {
            output[out][2] += voice.waveform[voice.cursor][2];
            output[out][3] += voice.waveform[voice.cursor][3];
        }
    }

    if (_continuous.kind == HapticContinuousKind::BoostpackThrust) {
        constexpr double sampleRate = 48000.0;
        constexpr double bodyHz = 48.0;
        constexpr double turbineHz = 96.0;
        const double gain = std::clamp(static_cast<double>(_continuous.gain), 0.0, 1.0);
        const double texture = std::clamp(static_cast<double>(_continuous.level), 0.0, 1.0);
        const double amplitude = gain * (0.30 + 0.10 * texture);

        for (auto& frame : output) {
            const double attackFrames = 0.012 * sampleRate;
            const double attackGain = (std::min)(
                1.0,
                static_cast<double>(_continuousFrames + 1) / attackFrames);
            const double signal =
                (0.82 * std::sin(_bodyPhase) + 0.18 * texture * std::sin(_sparkPhase)) *
                amplitude * attackGain;

            frame[2] += static_cast<float>(signal);
            frame[3] += static_cast<float>(signal);

            _bodyPhase = std::fmod(
                _bodyPhase + 2.0 * std::numbers::pi * bodyHz / sampleRate,
                2.0 * std::numbers::pi);
            _sparkPhase = std::fmod(
                _sparkPhase + 2.0 * std::numbers::pi * turbineHz / sampleRate,
                2.0 * std::numbers::pi);
            ++_continuousFrames;
        }
    } else if (_continuous.kind == HapticContinuousKind::ShipPropulsion) {
        constexpr double sampleRate = 48000.0;
        const double targetGain = static_cast<double>(_continuous.gain);
        const double targetLevel = static_cast<double>(_continuous.level);

        for (auto& frame : output) {
            const double gainTau = targetGain >= _shipSmoothedGain ? 0.040 : 0.080;
            const double levelTau = targetLevel >= _shipSmoothedLevel ? 0.035 : 0.070;
            const double gainStep = 1.0 - std::exp(-1.0 / (sampleRate * gainTau));
            const double levelStep = 1.0 - std::exp(-1.0 / (sampleRate * levelTau));
            _shipSmoothedGain += (targetGain - _shipSmoothedGain) * gainStep;
            _shipSmoothedLevel += (targetLevel - _shipSmoothedLevel) * levelStep;

            const double u = std::clamp(_shipSmoothedLevel, 0.0, 1.0);
            const double gain = std::clamp(_shipSmoothedGain, 0.0, 1.0);
            const double bodyHz = 52.0 + 20.0 * u;
            const double engineHz = 105.0 + 95.0 * u;
            const double amplitude = gain * (0.10 + 0.25 * u);
            const double signal =
                (0.72 * std::sin(_bodyPhase) + 0.26 * std::sin(_sparkPhase)) *
                amplitude;

            frame[2] += static_cast<float>(signal);
            frame[3] += static_cast<float>(signal);

            _bodyPhase = std::fmod(
                _bodyPhase + 2.0 * std::numbers::pi * bodyHz / sampleRate,
                2.0 * std::numbers::pi);
            _sparkPhase = std::fmod(
                _sparkPhase + 2.0 * std::numbers::pi * engineHz / sampleRate,
                2.0 * std::numbers::pi);
            ++_continuousFrames;
        }
    } else if (_continuous.kind == HapticContinuousKind::ShipBoost) {
        constexpr double sampleRate = 48000.0;
        const double targetGain = static_cast<double>(_continuous.gain);
        const double targetLevel = static_cast<double>(_continuous.level);

        for (auto& frame : output) {
            const double gainTau = targetGain >= _shipSmoothedGain ? 0.022 : 0.060;
            const double levelTau = targetLevel >= _shipSmoothedLevel ? 0.025 : 0.060;
            const double gainStep = 1.0 - std::exp(-1.0 / (sampleRate * gainTau));
            const double levelStep = 1.0 - std::exp(-1.0 / (sampleRate * levelTau));
            _shipSmoothedGain += (targetGain - _shipSmoothedGain) * gainStep;
            _shipSmoothedLevel += (targetLevel - _shipSmoothedLevel) * levelStep;

            const double u = std::clamp(_shipSmoothedLevel, 0.0, 1.0);
            const double gain = std::clamp(_shipSmoothedGain, 0.0, 1.0);
            const double bodyHz = 64.0 + 18.0 * u;
            const double jetHz = 150.0 + 90.0 * u;
            const double amplitude = gain * (0.40 + 0.18 * u);
            const double signal =
                (0.68 * std::sin(_bodyPhase) + 0.30 * std::sin(_sparkPhase)) *
                amplitude;

            frame[2] += static_cast<float>(signal);
            frame[3] += static_cast<float>(signal);

            _bodyPhase = std::fmod(
                _bodyPhase + 2.0 * std::numbers::pi * bodyHz / sampleRate,
                2.0 * std::numbers::pi);
            _sparkPhase = std::fmod(
                _sparkPhase + 2.0 * std::numbers::pi * jetHz / sampleRate,
                2.0 * std::numbers::pi);
            ++_continuousFrames;
        }
    } else if (_continuous.kind == HapticContinuousKind::LandVehicleChassis) {
        constexpr double sampleRate = 48000.0;
        constexpr double chassisBodyHz = 46.0;
        constexpr double chassisTextureHz = 92.0;
        const double gain = std::clamp(static_cast<double>(_continuous.gain), 0.0, 1.0);
        const double texture = std::clamp(static_cast<double>(_continuous.level), 0.0, 1.0);
        const double amplitude = gain * (0.24 + 0.18 * texture);

        for (auto& frame : output) {
            const double attackFrames = 0.018 * sampleRate;
            const double attackGain = (std::min)(
                1.0,
                static_cast<double>(_continuousFrames + 1) / attackFrames);
            const double signal =
                (0.78 * std::sin(_bodyPhase) + 0.22 * texture * std::sin(_sparkPhase)) *
                amplitude * attackGain;

            frame[2] += static_cast<float>(signal);
            frame[3] += static_cast<float>(signal);

            _bodyPhase = std::fmod(
                _bodyPhase + 2.0 * std::numbers::pi * chassisBodyHz / sampleRate,
                2.0 * std::numbers::pi);
            _sparkPhase = std::fmod(
                _sparkPhase + 2.0 * std::numbers::pi * chassisTextureHz / sampleRate,
                2.0 * std::numbers::pi);
            ++_continuousFrames;
        }
    } else if (_continuous.kind == HapticContinuousKind::NovablastCharge) {
        constexpr double sampleRate = 48000.0;
        const double u = static_cast<double>(_continuous.level);
        const double gain = static_cast<double>(_continuous.gain);
        const double bodyHz = 70.0 + 70.0 * u;
        const double sparkHz = 180.0 + 140.0 * u;
        const double amplitude = gain * (0.18 + 0.72 * u);

        for (auto& frame : output) {
            const double attackFrames = 0.004 * sampleRate;
            const double attackGain = (std::min)(
                1.0,
                static_cast<double>(_continuousFrames + 1) / attackFrames);
            const double signal =
                (0.70 * std::sin(_bodyPhase) + 0.25 * std::sin(_sparkPhase)) *
                amplitude * attackGain;

            frame[2] += static_cast<float>(signal);
            frame[3] += static_cast<float>(signal);

            _bodyPhase = std::fmod(
                _bodyPhase + 2.0 * std::numbers::pi * bodyHz / sampleRate,
                2.0 * std::numbers::pi);
            _sparkPhase = std::fmod(
                _sparkPhase + 2.0 * std::numbers::pi * sparkHz / sampleRate,
                2.0 * std::numbers::pi);
            ++_continuousFrames;
        }
    } else if (_continuous.kind == HapticContinuousKind::PenumbraStress) {
        constexpr double sampleRate = 48000.0;
        const double gain = static_cast<double>(_continuous.gain);
        constexpr double bodyHz = 72.0;
        constexpr double sparkHz = 245.0;
        const double amplitude = gain * 0.34;

        for (auto& frame : output) {
            const double attackFrames = 0.006 * sampleRate;
            const double attackGain = (std::min)(
                1.0,
                static_cast<double>(_continuousFrames + 1) / attackFrames);
            const double signal =
                (0.68 * std::sin(_bodyPhase) + 0.32 * std::sin(_sparkPhase)) *
                amplitude * attackGain;

            frame[2] += static_cast<float>(signal);
            frame[3] += static_cast<float>(signal);

            _bodyPhase = std::fmod(
                _bodyPhase + 2.0 * std::numbers::pi * bodyHz / sampleRate,
                2.0 * std::numbers::pi);
            _sparkPhase = std::fmod(
                _sparkPhase + 2.0 * std::numbers::pi * sparkHz / sampleRate,
                2.0 * std::numbers::pi);
            ++_continuousFrames;
        }
    } else if (_continuous.kind == HapticContinuousKind::MagsniperCharge) {
        constexpr double sampleRate = 48000.0;
        const double gain = static_cast<double>(_continuous.gain);
        constexpr double bodyHz = 118.0;
        constexpr double coilHz = 330.0;
        const double amplitude = gain * 0.30;

        for (auto& frame : output) {
            const double attackFrames = 0.006 * sampleRate;
            const double attackGain = (std::min)(
                1.0,
                static_cast<double>(_continuousFrames + 1) / attackFrames);
            const double signal =
                (0.38 * std::sin(_bodyPhase) + 0.62 * std::sin(_sparkPhase)) *
                amplitude * attackGain;

            frame[2] += static_cast<float>(signal);
            frame[3] += static_cast<float>(signal);

            _bodyPhase = std::fmod(
                _bodyPhase + 2.0 * std::numbers::pi * bodyHz / sampleRate,
                2.0 * std::numbers::pi);
            _sparkPhase = std::fmod(
                _sparkPhase + 2.0 * std::numbers::pi * coilHz / sampleRate,
                2.0 * std::numbers::pi);
            ++_continuousFrames;
        }
    } else if (_continuous.kind == HapticContinuousKind::ArcWelderArc) {
        constexpr double sampleRate = 48000.0;
        const double gain = static_cast<double>(_continuous.gain);
        constexpr double bodyHz = 105.0;
        constexpr double arcHz = 305.0;
        const double amplitude = gain * 0.45;

        for (auto& frame : output) {
            const double attackFrames = 0.006 * sampleRate;
            const double attackGain = (std::min)(
                1.0,
                static_cast<double>(_continuousFrames + 1) / attackFrames);
            const double signal =
                (0.45 * std::sin(_bodyPhase) + 0.55 * std::sin(_sparkPhase)) *
                amplitude * attackGain;

            frame[2] += static_cast<float>(signal);
            frame[3] += static_cast<float>(signal);

            _bodyPhase = std::fmod(
                _bodyPhase + 2.0 * std::numbers::pi * bodyHz / sampleRate,
                2.0 * std::numbers::pi);
            _sparkPhase = std::fmod(
                _sparkPhase + 2.0 * std::numbers::pi * arcHz / sampleRate,
                2.0 * std::numbers::pi);
            ++_continuousFrames;
        }
    } else if (_continuous.kind == HapticContinuousKind::AutoRivetTension) {
        constexpr double sampleRate = 48000.0;
        const double u = static_cast<double>(_continuous.level);
        const double gain = static_cast<double>(_continuous.gain);
        const double bodyHz = 82.0 + 18.0 * u;
        const double motorHz = 220.0 + 80.0 * u;
        const double amplitude = gain * (0.06 + 0.12 * u);

        for (auto& frame : output) {
            const double attackFrames = 0.008 * sampleRate;
            const double attackGain = (std::min)(
                1.0,
                static_cast<double>(_continuousFrames + 1) / attackFrames);
            const double signal =
                (0.58 * std::sin(_bodyPhase) + 0.42 * std::sin(_sparkPhase)) *
                amplitude * attackGain;

            frame[2] += static_cast<float>(signal);
            frame[3] += static_cast<float>(signal);

            _bodyPhase = std::fmod(
                _bodyPhase + 2.0 * std::numbers::pi * bodyHz / sampleRate,
                2.0 * std::numbers::pi);
            _sparkPhase = std::fmod(
                _sparkPhase + 2.0 * std::numbers::pi * motorHz / sampleRate,
                2.0 * std::numbers::pi);
            ++_continuousFrames;
        }
    } else if (_continuous.kind == HapticContinuousKind::CutterBeam) {
        constexpr double sampleRate = 48000.0;
        const double u = static_cast<double>(_continuous.level);
        const double gain = static_cast<double>(_continuous.gain);
        const double bodyHz = 75.0 + 40.0 * u;
        const double gritHz = 190.0 + 70.0 * u;
        const double amplitude = gain * (0.30 + 0.65 * u);

        for (auto& frame : output) {
            const double attackFrames = 0.006 * sampleRate;
            const double attackGain = (std::min)(
                1.0,
                static_cast<double>(_continuousFrames + 1) / attackFrames);
            const double signal =
                (0.72 * std::sin(_bodyPhase) + 0.28 * std::sin(_sparkPhase)) *
                amplitude * attackGain;

            frame[2] += static_cast<float>(signal);
            frame[3] += static_cast<float>(signal);

            _bodyPhase = std::fmod(
                _bodyPhase + 2.0 * std::numbers::pi * bodyHz / sampleRate,
                2.0 * std::numbers::pi);
            _sparkPhase = std::fmod(
                _sparkPhase + 2.0 * std::numbers::pi * gritHz / sampleRate,
                2.0 * std::numbers::pi);
            ++_continuousFrames;
        }
    }


    if (_continuous.shipLaserGain > 0.0F) {
        constexpr double sampleRate = 48000.0;
        constexpr double bodyHz = 114.0;
        constexpr double edgeHz = 330.0;
        const double gain = static_cast<double>(_continuous.shipLaserGain);
        const double amplitude = gain * 0.30;

        for (auto& frame : output) {
            const double attackFrames = 0.008 * sampleRate;
            const double attackGain = (std::min)(
                1.0,
                static_cast<double>(_shipLaserFrames + 1) / attackFrames);
            const double signal =
                (0.72 * std::sin(_shipLaserBodyPhase) +
                 0.28 * std::sin(_shipLaserEdgePhase)) *
                amplitude * attackGain;

            frame[2] += static_cast<float>(signal);
            frame[3] += static_cast<float>(signal);

            _shipLaserBodyPhase = std::fmod(
                _shipLaserBodyPhase + 2.0 * std::numbers::pi * bodyHz / sampleRate,
                2.0 * std::numbers::pi);
            _shipLaserEdgePhase = std::fmod(
                _shipLaserEdgePhase + 2.0 * std::numbers::pi * edgeHz / sampleRate,
                2.0 * std::numbers::pi);
            ++_shipLaserFrames;
        }
    }

    for (auto& frame : output) {
        frame[0] = 0.0F;
        frame[1] = 0.0F;
        frame[2] = std::clamp(frame[2], -1.0F, 1.0F);
        frame[3] = std::clamp(frame[3], -1.0F, 1.0F);
    }

    std::erase_if(_voices, [](const Voice& voice) {
        return voice.cursor >= voice.waveform.size();
    });
}
