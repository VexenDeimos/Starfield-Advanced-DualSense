#include <StarfieldDualSense/DualModeHapticsBackend.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

namespace
{
    struct BluetoothPulseSpec
    {
        std::uint8_t left{ 0 };
        std::uint8_t right{ 0 };
        std::chrono::milliseconds duration{ 0 };
        const char* name{ "Unknown" };
    };

    [[nodiscard]] BluetoothPulseSpec finitePulseSpec(
        sds::HapticEffectKind kind) noexcept
    {
        using K = sds::HapticEffectKind;
        using namespace std::chrono_literals;

        switch (kind) {
        case K::EonSnap:
            return { 120, 210, 55ms, "EonSnap" };
        case K::BridgerConcussion:
            return { 255, 220, 125ms, "BridgerConcussion" };
        case K::MicrogunKick:
            return { 135, 190, 40ms, "MicrogunKick" };
        case K::BallisticHandgunKick:
            return { 155, 210, 55ms, "BallisticHandgunKick" };
        case K::BallisticRapidKick:
            return { 125, 185, 42ms, "BallisticRapidKick" };
        case K::BallisticRifleKick:
            return { 180, 220, 60ms, "BallisticRifleKick" };
        case K::PrecisionBallisticKick:
            return { 215, 235, 75ms, "PrecisionBallisticKick" };
        case K::LauncherConcussion:
            return { 255, 225, 130ms, "LauncherConcussion" };
        case K::ParticleLauncherConcussion:
            return { 245, 235, 120ms, "ParticleLauncherConcussion" };
        case K::MagneticPulse:
            return { 120, 230, 55ms, "MagneticPulse" };
        case K::MagneticRapid:
            return { 105, 220, 42ms, "MagneticRapid" };
        case K::MagneticPrecision:
            return { 170, 245, 70ms, "MagneticPrecision" };
        case K::ShotgunBlast:
            return { 240, 215, 95ms, "ShotgunBlast" };
        case K::LaserPulse:
            return { 75, 220, 45ms, "LaserPulse" };
        case K::ParticlePulse:
            return { 145, 235, 60ms, "ParticlePulse" };
        case K::NovablastDischarge:
            return { 180, 245, 80ms, "NovablastDischarge" };
        case K::MeleeLightSwing:
            return { 60, 120, 45ms, "MeleeLightSwing" };
        case K::MeleeHeavySwing:
            return { 100, 170, 60ms, "MeleeHeavySwing" };
        case K::MeleeVeryHeavySwing:
            return { 145, 205, 75ms, "MeleeVeryHeavySwing" };
        case K::MeleeLightImpact:
            return { 100, 155, 55ms, "MeleeLightImpact" };
        case K::MeleeHeavyImpact:
            return { 170, 200, 75ms, "MeleeHeavyImpact" };
        case K::MeleeVeryHeavyImpact:
            return { 230, 220, 100ms, "MeleeVeryHeavyImpact" };
        case K::IncomingDamageImpact:
            return { 180, 195, 85ms, "IncomingDamageImpact" };
        case K::ShipBallisticCannonKick:
            return { 220, 225, 75ms, "ShipBallisticCannonKick" };
        case K::ShipLaserPulseCrest:
            return { 80, 225, 45ms, "ShipLaserPulseCrest" };
        case K::ShipParticlePulse:
            return { 150, 235, 60ms, "ShipParticlePulse" };
        case K::ShipMissileLaunchThump:
            return { 255, 215, 105ms, "ShipMissileLaunchThump" };
        case K::ShipEMPulse:
            return { 110, 245, 75ms, "ShipEMPulse" };
        case K::ShipTouchdownThump:
            return { 255, 185, 100ms, "ShipTouchdownThump" };
        case K::BoostpackIgnition:
            return { 185, 210, 95ms, "BoostpackIgnition" };
        case K::DigipickRotateTick:
            return { 40, 125, 24ms, "DigipickRotateTick" };
        case K::DigipickSelectClick:
            return { 55, 150, 30ms, "DigipickSelectClick" };
        case K::DigipickInsertClunk:
            return { 110, 150, 45ms, "DigipickInsertClunk" };
        case K::DigipickSuccess:
            return { 80, 180, 55ms, "DigipickSuccess" };
        case K::LandVehicleBoostKick:
            return { 210, 215, 90ms, "LandVehicleBoostKick" };
        case K::LandVehicleTouchdownThump:
            return { 255, 180, 100ms, "LandVehicleTouchdownThump" };
        case K::LandVehicleGunRecoil:
            return { 175, 215, 60ms, "LandVehicleGunRecoil" };
        }

        return { 160, 190, 60ms, "Unknown" };
    }

    [[nodiscard]] std::uint8_t scaledMotor(
        std::uint8_t base,
        float gain) noexcept
    {
        const float clamped =
            std::clamp(
                gain,
                0.0F,
                1.0F);

        if (base == 0 || clamped <= 0.0F) {
            return 0;
        }

        int value =
            static_cast<int>(
                std::lround(
                    static_cast<float>(base) *
                    clamped));

        if (value > 0) {
            value = std::max(value, 16);
        }

        return static_cast<std::uint8_t>(
            std::clamp(
                value,
                0,
                255));
    }
}

namespace
{
    [[nodiscard]] std::uint8_t continuousMotor(
        std::uint8_t base,
        float intensity) noexcept
    {
        const float clamped =
            std::clamp(
                intensity,
                0.0F,
                1.0F);

        if (base == 0 ||
            clamped <= 0.0F) {
            return 0;
        }

        const int raw =
            static_cast<int>(
                std::lround(
                    static_cast<float>(base) *
                    clamped));

        if (raw <= 0) {
            return 0;
        }

        const unsigned quantized =
            ((static_cast<unsigned>(raw) + 4U) / 8U) * 8U;

        return static_cast<std::uint8_t>(
            std::min(
                quantized,
                255U));
    }

    [[nodiscard]] std::uint8_t blendContinuousMotor(
        std::uint8_t base,
        std::uint8_t overlay) noexcept
    {
        const unsigned b = base;
        const unsigned o = overlay;

        return static_cast<std::uint8_t>(
            b + o -
            ((b * o + 127U) / 255U));
    }

    struct BluetoothContinuousSpec
    {
        std::uint8_t left{ 0 };
        std::uint8_t right{ 0 };
    };

    [[nodiscard]] BluetoothContinuousSpec continuousSpec(
        sds::HapticContinuousState state) noexcept
    {
        using K = sds::HapticContinuousKind;

        const float gain =
            std::clamp(
                state.gain,
                0.0F,
                1.0F);

        const float level =
            std::clamp(
                state.level,
                0.0F,
                1.0F);

        std::uint8_t leftBase = 0;
        std::uint8_t rightBase = 0;
        float intensity = 0.0F;

        switch (state.kind) {
        case K::None:
            break;

        case K::NovablastCharge:
            leftBase = 145;
            rightBase = 220;
            intensity =
                gain *
                (0.25F + 0.75F * level);
            break;

        case K::CutterBeam:
            leftBase = 135;
            rightBase = 210;
            intensity =
                gain *
                (0.40F + 0.60F * level);
            break;

        case K::PenumbraStress:
            leftBase = 145;
            rightBase = 185;
            intensity = gain;
            break;

        case K::MagsniperCharge:
            leftBase = 115;
            rightBase = 220;
            intensity = gain;
            break;

        case K::ArcWelderArc:
            leftBase = 130;
            rightBase = 230;
            intensity = gain;
            break;

        case K::AutoRivetTension:
            leftBase = 95;
            rightBase = 170;
            intensity =
                gain *
                (0.35F + 0.65F * level);
            break;

        case K::BoostpackThrust:
            leftBase = 190;
            rightBase = 145;
            intensity =
                gain *
                (0.70F + 0.30F * level);
            break;

        case K::ShipPropulsion:
        {
            leftBase = 175;
            rightBase = 135;

            const float throttleIntensity =
                std::clamp(
                    gain,
                    0.0F,
                    1.0F);

            intensity =
                0.75F *
                throttleIntensity;

            break;
        }
        case K::ShipBoost:
            leftBase = 235;
            rightBase = 190;
            intensity =
                gain *
                (0.75F + 0.25F * level);
            break;

        case K::LandVehicleChassis:
        {
            leftBase = 145;
            rightBase = 90;

            const float terrainTexture =
                std::clamp(
                    (level - 0.08F) / 0.92F,
                    0.0F,
                    1.0F);

            intensity =
                gain *
                terrainTexture;

            break;
        }
        }

        auto left =
            continuousMotor(
                leftBase,
                intensity);

        auto right =
            continuousMotor(
                rightBase,
                intensity);

        // A moving ship should fade to the smallest compatible-rumble
        // step instead of crossing an artificial low-speed cutoff.
        if (state.kind == K::ShipPropulsion &&
            gain > 0.0001F) {

            left =
                std::max(
                    left,
                    static_cast<std::uint8_t>(8));

            right =
                std::max(
                    right,
                    static_cast<std::uint8_t>(8));
        }

        const float laserGain =
            std::clamp(
                state.shipLaserGain,
                0.0F,
                1.0F);

        if (laserGain > 0.0F) {
            left =
                blendContinuousMotor(
                    left,
                    continuousMotor(
                        80,
                        laserGain));

            right =
                blendContinuousMotor(
                    right,
                    continuousMotor(
                        220,
                        laserGain));
        }

        return {
            left,
            right
        };
    }
}
sds::DualModeHapticsBackend::DualModeHapticsBackend(
    std::unique_ptr<IHapticsBackend> wiredBackend,
    BluetoothActiveCallback bluetoothActive,
    BluetoothPulseCallback bluetoothPulse,
    BluetoothContinuousCallback bluetoothContinuous,
    LogCallback log) :
    _wiredBackend(std::move(wiredBackend)),
    _bluetoothActive(std::move(bluetoothActive)),
    _bluetoothPulse(std::move(bluetoothPulse)),
    _bluetoothContinuous(std::move(bluetoothContinuous)),
    _log(std::move(log))
{}

sds::DualModeHapticsBackend::~DualModeHapticsBackend()
{
    stop();
}

void sds::DualModeHapticsBackend::start()
{
    if (_started) {
        return;
    }

    _started = true;

    if (_wiredBackend) {
        _wiredBackend->start();
    }
}

void sds::DualModeHapticsBackend::stop() noexcept
{
    if (!_started) {
        return;
    }

    try {
        if (_bluetoothContinuous) {
            (void)_bluetoothContinuous(
                0,
                0);
        }
    } catch (...) {
    }

    if (_wiredBackend) {
        (void)_wiredBackend->setContinuous({});
        _wiredBackend->stop();
    }

    _started = false;
}

bool sds::DualModeHapticsBackend::enqueue(
    HapticCommand command) noexcept
{
    try {
        if (!_started) {
            return false;
        }

        const bool bluetooth =
            _bluetoothActive &&
            _bluetoothActive();

        if (!bluetooth) {
            return _wiredBackend &&
                _wiredBackend->enqueue(
                    std::move(command));
        }

        const auto spec =
            finitePulseSpec(
                command.kind);

        const auto left =
            scaledMotor(
                spec.left,
                command.gain);

        const auto right =
            scaledMotor(
                spec.right,
                command.gain);

        if (left == 0 &&
            right == 0) {

            return true;
        }

        const bool submitted =
            _bluetoothPulse &&
            _bluetoothPulse(
                left,
                right,
                spec.duration);

        char diagnostic[256]{};

        std::snprintf(
            diagnostic,
            sizeof(diagnostic),
            "Bluetooth finite haptic: kind=%s gain=%.3f left=%u right=%u durationMs=%lld submitted=%s",
            spec.name,
            static_cast<double>(command.gain),
            static_cast<unsigned>(left),
            static_cast<unsigned>(right),
            static_cast<long long>(spec.duration.count()),
            submitted ? "yes" : "no");

        log(diagnostic);

        return submitted;
    } catch (...) {
        log(
            "Bluetooth finite haptic: "
            "translation/submit exception");
        return false;
    }
}

bool sds::DualModeHapticsBackend::setContinuous(
    HapticContinuousState state) noexcept
{
    try {
        if (!_started) {
            return false;
        }

        const bool bluetooth =
            _bluetoothActive &&
            _bluetoothActive();

        if (!bluetooth) {
            if (_bluetoothContinuous) {
                (void)_bluetoothContinuous(
                    0,
                    0);
            }

            return _wiredBackend &&
                _wiredBackend->setContinuous(
                    state);
        }

        const auto spec =
            continuousSpec(
                state);

        return _bluetoothContinuous &&
            _bluetoothContinuous(
                spec.left,
                spec.right);
    } catch (...) {
        log(
            "Bluetooth continuous haptic: "
            "translation/submit exception");
        return false;
    }
}

bool sds::DualModeHapticsBackend::active() const noexcept
{
    try {
        if (!_started) {
            return false;
        }

        if (_bluetoothActive &&
            _bluetoothActive()) {

            return true;
        }

        return _wiredBackend &&
            _wiredBackend->active();
    } catch (...) {
        return false;
    }
}

void sds::DualModeHapticsBackend::log(
    std::string_view message) const noexcept
{
    try {
        if (_log) {
            _log(message);
        }
    } catch (...) {
    }
}