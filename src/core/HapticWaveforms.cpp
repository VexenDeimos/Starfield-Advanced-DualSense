#include <StarfieldDualSense/HapticWaveforms.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
    float attack(float t, float seconds) noexcept
    {
        return seconds <= 0.0F ? 1.0F : std::clamp(t / seconds, 0.0F, 1.0F);
    }

    float oscillator(float hz, float t) noexcept
    {
        return std::sin(2.0F * std::numbers::pi_v<float> * hz * t);
    }
}

sds::HapticWaveform sds::synthesizeHapticEffect(const HapticCommand& command, std::uint32_t sampleRate)
{
    if (sampleRate == 0) {
        return {};
    }

    const float gain = std::clamp(command.gain, 0.0F, 1.0F);
    float duration = 0.0F;
    switch (command.kind) {
    case HapticEffectKind::EonSnap:
        duration = 0.032F;
        break;
    case HapticEffectKind::BridgerConcussion:
        duration = 0.140F;
        break;
    case HapticEffectKind::MicrogunKick:
        duration = 0.014F;
        break;
    case HapticEffectKind::BallisticHandgunKick:
        duration = 0.028F;
        break;
    case HapticEffectKind::BallisticRapidKick:
        duration = 0.016F;
        break;
    case HapticEffectKind::BallisticRifleKick:
        duration = 0.036F;
        break;
    case HapticEffectKind::PrecisionBallisticKick:
        duration = 0.060F;
        break;
    case HapticEffectKind::LauncherConcussion:
        duration = 0.120F;
        break;
    case HapticEffectKind::ParticleLauncherConcussion:
        duration = 0.100F;
        break;
    case HapticEffectKind::MagneticPulse:
        duration = 0.042F;
        break;
    case HapticEffectKind::MagneticRapid:
        duration = 0.018F;
        break;
    case HapticEffectKind::MagneticPrecision:
        duration = 0.085F;
        break;
    case HapticEffectKind::ShotgunBlast:
        duration = 0.090F;
        break;
    case HapticEffectKind::LaserPulse:
        duration = 0.040F;
        break;
    case HapticEffectKind::ParticlePulse:
        duration = 0.065F;
        break;
    case HapticEffectKind::NovablastDischarge:
        duration = 0.060F;
        break;
    case HapticEffectKind::MeleeLightSwing:
        duration = 0.055F;
        break;
    case HapticEffectKind::MeleeHeavySwing:
        duration = 0.085F;
        break;
    case HapticEffectKind::MeleeVeryHeavySwing:
        duration = 0.115F;
        break;
    case HapticEffectKind::MeleeLightImpact:
        duration = 0.030F;
        break;
    case HapticEffectKind::MeleeHeavyImpact:
        duration = 0.055F;
        break;
    case HapticEffectKind::MeleeVeryHeavyImpact:
        duration = 0.120F;
        break;
    case HapticEffectKind::IncomingDamageImpact:
        duration = gain < 0.60F ? 0.032F : 0.030F + 0.070F * gain;
        break;
    case HapticEffectKind::ShipBallisticCannonKick:
        duration = 0.045F;
        break;
    case HapticEffectKind::ShipLaserPulseCrest:
        duration = 0.040F;
        break;
    case HapticEffectKind::ShipParticlePulse:
        duration = 0.060F;
        break;
    case HapticEffectKind::ShipMissileLaunchThump:
        duration = 0.105F;
        break;
    case HapticEffectKind::ShipEMPulse:
        duration = 0.075F;
        break;
    case HapticEffectKind::ShipTouchdownThump:
        duration = 0.100F;
        break;
    case HapticEffectKind::BoostpackIgnition:
        duration = 0.070F;
        break;
    case HapticEffectKind::DigipickRotateTick:
        duration = 0.020F;
        break;
    case HapticEffectKind::DigipickSelectClick:
        duration = 0.028F;
        break;
    case HapticEffectKind::DigipickInsertClunk:
        duration = 0.050F;
        break;
    case HapticEffectKind::DigipickSuccess:
        duration = 0.095F;
        break;
    case HapticEffectKind::LandVehicleBoostKick:
        duration = 0.090F;
        break;
    case HapticEffectKind::LandVehicleTouchdownThump:
        duration = 0.110F;
        break;
    case HapticEffectKind::LandVehicleGunRecoil:
        duration = 0.065F;
        break;
    }
    const auto frameCount = static_cast<std::size_t>(std::lround(duration * sampleRate));
    HapticWaveform frames(frameCount);

    for (std::size_t i = 0; i < frameCount; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(sampleRate);
        float sample = 0.0F;
        if (command.kind == HapticEffectKind::EonSnap) {
            const float envelope = attack(t, 0.0015F) * std::exp(-t / 0.0080F);
            sample = 0.95F * oscillator(190.0F, t) * envelope;
        } else if (command.kind == HapticEffectKind::BridgerConcussion) {
            const float transient = 0.35F * oscillator(190.0F, t) *
                attack(t, 0.0015F) * std::exp(-t / 0.010F);
            const float body = 0.85F * oscillator(65.0F, t) *
                attack(t, 0.0040F) * std::exp(-t / 0.055F);
            const float finalFade = std::clamp((duration - t) / 0.010F, 0.0F, 1.0F);
            sample = (transient + body) * finalFade;
        } else if (command.kind == HapticEffectKind::MicrogunKick) {
            const float body = 0.70F * oscillator(85.0F, t) *
                attack(t, 0.0010F) * std::exp(-t / 0.0060F);
            const float transient = 0.45F * oscillator(210.0F, t) *
                attack(t, 0.0008F) * std::exp(-t / 0.0035F);
            sample = body + transient;
        } else if (command.kind == HapticEffectKind::BallisticHandgunKick) {
            const float crack = 0.75F * oscillator(210.0F, t) *
                attack(t, 0.0008F) * std::exp(-t / 0.0055F);
            const float body = 0.45F * oscillator(110.0F, t) *
                attack(t, 0.0012F) * std::exp(-t / 0.012F);
            const float finalFade = std::clamp((duration - t) / 0.004F, 0.0F, 1.0F);
            sample = (crack + body) * finalFade;
        } else if (command.kind == HapticEffectKind::BallisticRapidKick) {
            const float crack = 0.60F * oscillator(220.0F, t) *
                attack(t, 0.0006F) * std::exp(-t / 0.0035F);
            const float body = 0.30F * oscillator(105.0F, t) *
                attack(t, 0.0009F) * std::exp(-t / 0.0060F);
            sample = crack + body;
        } else if (command.kind == HapticEffectKind::BallisticRifleKick) {
            const float crack = 0.70F * oscillator(190.0F, t) *
                attack(t, 0.0008F) * std::exp(-t / 0.0070F);
            const float body = 0.65F * oscillator(90.0F, t) *
                attack(t, 0.0016F) * std::exp(-t / 0.016F);
            const float finalFade = std::clamp((duration - t) / 0.005F, 0.0F, 1.0F);
            sample = (crack + body) * finalFade;
        } else if (command.kind == HapticEffectKind::PrecisionBallisticKick) {
            const float crack = 0.80F * oscillator(180.0F, t) *
                attack(t, 0.0009F) * std::exp(-t / 0.0080F);
            const float body = 0.90F * oscillator(75.0F, t) *
                attack(t, 0.0020F) * std::exp(-t / 0.028F);
            const float finalFade = std::clamp((duration - t) / 0.006F, 0.0F, 1.0F);
            sample = (crack + body) * finalFade;
        } else if (command.kind == HapticEffectKind::LauncherConcussion) {
            const float launchCrack = 0.52F * oscillator(185.0F, t) *
                attack(t, 0.0009F) * std::exp(-t / 0.0080F);
            const float concussion = 0.98F * oscillator(58.0F, t) *
                attack(t, 0.0025F) * std::exp(-t / 0.050F);
            const float mechanism = 0.28F * oscillator(105.0F, t) *
                attack(t, 0.0015F) * std::exp(-t / 0.022F);
            const float finalFade = std::clamp((duration - t) / 0.010F, 0.0F, 1.0F);
            sample = (launchCrack + concussion + mechanism) * finalFade;
        } else if (command.kind == HapticEffectKind::ParticleLauncherConcussion) {
            const float crack = 0.48F * oscillator(245.0F, t) *
                attack(t, 0.0007F) * std::exp(-t / 0.0070F);
            const float body = 0.82F * oscillator(68.0F, t) *
                attack(t, 0.0020F) * std::exp(-t / 0.040F);
            const float energy = 0.34F * oscillator(330.0F, t) *
                attack(t, 0.0006F) * std::exp(-t / 0.020F);
            const float finalFade = std::clamp((duration - t) / 0.008F, 0.0F, 1.0F);
            sample = (crack + body + energy) * finalFade;
        } else if (command.kind == HapticEffectKind::MagneticPulse) {
            const float snap = 0.72F * oscillator(285.0F, t) *
                attack(t, 0.0006F) * std::exp(-t / 0.0055F);
            const float railBody = 0.62F * oscillator(78.0F, t) *
                attack(t, 0.0012F) * std::exp(-t / 0.017F);
            const float coil = 0.25F * oscillator(420.0F, t) *
                attack(t, 0.0005F) * std::exp(-t / 0.0090F);
            const float finalFade = std::clamp((duration - t) / 0.005F, 0.0F, 1.0F);
            sample = (snap + railBody + coil) * finalFade;
        } else if (command.kind == HapticEffectKind::MagneticRapid) {
            const float snap = 0.66F * oscillator(305.0F, t) *
                attack(t, 0.0004F) * std::exp(-t / 0.0032F);
            const float body = 0.38F * oscillator(96.0F, t) *
                attack(t, 0.0007F) * std::exp(-t / 0.0065F);
            const float coil = 0.20F * oscillator(430.0F, t) *
                attack(t, 0.00035F) * std::exp(-t / 0.0045F);
            const float finalFade = std::clamp((duration - t) / 0.0025F, 0.0F, 1.0F);
            sample = (snap + body + coil) * finalFade;
        } else if (command.kind == HapticEffectKind::MagneticPrecision) {
            const float crack = 0.78F * oscillator(265.0F, t) *
                attack(t, 0.0007F) * std::exp(-t / 0.0075F);
            const float accelerator = 0.96F * oscillator(62.0F, t) *
                attack(t, 0.0018F) * std::exp(-t / 0.034F);
            const float coil = 0.30F * oscillator(390.0F, t) *
                attack(t, 0.0005F) * std::exp(-t / 0.017F);
            const float finalFade = std::clamp((duration - t) / 0.007F, 0.0F, 1.0F);
            sample = (crack + accelerator + coil) * finalFade;
        } else if (command.kind == HapticEffectKind::ShotgunBlast) {
            const float crack = 0.45F * oscillator(210.0F, t) *
                attack(t, 0.0008F) * std::exp(-t / 0.0060F);
            const float body = 0.95F * oscillator(72.0F, t) *
                attack(t, 0.0025F) * std::exp(-t / 0.034F);
            const float finalFade = std::clamp((duration - t) / 0.008F, 0.0F, 1.0F);
            sample = (crack + body) * finalFade;
        } else if (command.kind == HapticEffectKind::LaserPulse) {
            const float zap = 0.95F * oscillator(230.0F, t) *
                attack(t, 0.0006F) * std::exp(-t / 0.016F);
            const float body = 0.65F * oscillator(105.0F, t) *
                attack(t, 0.0012F) * std::exp(-t / 0.024F);
            const float shimmer = 0.18F * oscillator(360.0F, t) *
                attack(t, 0.0005F) * std::exp(-t / 0.012F);
            const float finalFade = std::clamp((duration - t) / 0.005F, 0.0F, 1.0F);
            sample = (zap + body + shimmer) * finalFade;
        } else if (command.kind == HapticEffectKind::NovablastDischarge) {
            const float snap = 0.90F * oscillator(320.0F, t) *
                attack(t, 0.0006F) * std::exp(-t / 0.006F);
            const float thump = 0.95F * oscillator(68.0F, t) *
                attack(t, 0.0018F) * std::exp(-t / 0.032F);
            const float buzz = 0.35F * oscillator(145.0F, t) *
                attack(t, 0.0010F) * std::exp(-t / 0.022F);
            const float finalFade = std::clamp((duration - t) / 0.008F, 0.0F, 1.0F);
            sample = (snap + thump + buzz) * finalFade;
        } else if (command.kind == HapticEffectKind::MeleeLightSwing) {
            const float motion = 0.55F * oscillator(138.0F, t) *
                attack(t, 0.0040F) * std::exp(-t / 0.030F);
            const float edge = 0.18F * oscillator(255.0F, t) *
                attack(t, 0.0030F) * std::exp(-t / 0.020F);
            const float finalFade = std::clamp((duration - t) / 0.010F, 0.0F, 1.0F);
            sample = (motion + edge) * finalFade;
        } else if (command.kind == HapticEffectKind::MeleeHeavySwing) {
            const float motion = 0.72F * oscillator(88.0F, t) *
                attack(t, 0.0060F) * std::exp(-t / 0.050F);
            const float edge = 0.22F * oscillator(172.0F, t) *
                attack(t, 0.0040F) * std::exp(-t / 0.032F);
            const float finalFade = std::clamp((duration - t) / 0.014F, 0.0F, 1.0F);
            sample = (motion + edge) * finalFade;
        } else if (command.kind == HapticEffectKind::MeleeVeryHeavySwing) {
            const float motion = 0.86F * oscillator(62.0F, t) *
                attack(t, 0.0080F) * std::exp(-t / 0.070F);
            const float edge = 0.24F * oscillator(132.0F, t) *
                attack(t, 0.0050F) * std::exp(-t / 0.040F);
            const float finalFade = std::clamp((duration - t) / 0.018F, 0.0F, 1.0F);
            sample = (motion + edge) * finalFade;
        } else if (command.kind == HapticEffectKind::MeleeLightImpact) {
            const float strike = 1.55F * oscillator(230.0F, t) *
                attack(t, 0.0002F) * std::exp(-t / 0.0045F);
            const float knock = 1.10F * oscillator(105.0F, t) *
                attack(t, 0.00035F) * std::exp(-t / 0.012F);
            const float finalFade = std::clamp((duration - t) / 0.004F, 0.0F, 1.0F);
            sample = (strike + knock) * finalFade;
        } else if (command.kind == HapticEffectKind::MeleeHeavyImpact) {
            const float strike = 1.20F * oscillator(210.0F, t) *
                attack(t, 0.00022F) * std::exp(-t / 0.0055F);
            const float thud = 1.55F * oscillator(90.0F, t) *
                attack(t, 0.00045F) * std::exp(-t / 0.025F);
            const float knock = 0.45F * oscillator(135.0F, t) *
                attack(t, 0.00030F) * std::exp(-t / 0.010F);
            const float finalFade = std::clamp((duration - t) / 0.007F, 0.0F, 1.0F);
            sample = (strike + thud + knock) * finalFade;
        } else if (command.kind == HapticEffectKind::MeleeVeryHeavyImpact) {
            // Diagnostic control: deliberately clone the already-proven LauncherConcussion
            // waveform exactly. Keep the melee effect kind and upstream 0.9 gain unchanged
            // so hardware testing isolates waveform reproduction from TESHit/routing behavior.
            const float launchCrack = 0.52F * oscillator(185.0F, t) *
                attack(t, 0.0009F) * std::exp(-t / 0.0080F);
            const float concussion = 0.98F * oscillator(58.0F, t) *
                attack(t, 0.0025F) * std::exp(-t / 0.050F);
            const float mechanism = 0.28F * oscillator(105.0F, t) *
                attack(t, 0.0015F) * std::exp(-t / 0.022F);
            const float finalFade = std::clamp((duration - t) / 0.010F, 0.0F, 1.0F);
            sample = (launchCrack + concussion + mechanism) * finalFade;
        } else if (command.kind == HapticEffectKind::ShipBallisticCannonKick) {
            const float crack = 0.82F * oscillator(185.0F, t) *
                attack(t, 0.0007F) * std::exp(-t / 0.0065F);
            const float cannonBody = 1.00F * oscillator(64.0F, t) *
                attack(t, 0.0014F) * std::exp(-t / 0.022F);
            const float mechanism = 0.34F * oscillator(112.0F, t) *
                attack(t, 0.0010F) * std::exp(-t / 0.013F);
            const float finalFade = std::clamp((duration - t) / 0.006F, 0.0F, 1.0F);
            sample = (crack + cannonBody + mechanism) * finalFade;
        } else if (command.kind == HapticEffectKind::ShipLaserPulseCrest) {
            const float edge = 0.48F * oscillator(248.0F, t) *
                attack(t, 0.0005F) * std::exp(-t / 0.009F);
            const float energy = 0.62F * oscillator(114.0F, t) *
                attack(t, 0.0009F) * std::exp(-t / 0.022F);
            const float shimmer = 0.16F * oscillator(365.0F, t) *
                attack(t, 0.0004F) * std::exp(-t / 0.012F);
            const float finalFade = std::clamp((duration - t) / 0.006F, 0.0F, 1.0F);
            sample = (edge + energy + shimmer) * finalFade;
        } else if (command.kind == HapticEffectKind::ShipParticlePulse) {
            // Proton Beam: a discrete energetic whip. Give it more tactile body
            // than the laser crest, but keep it decisively lighter than a
            // mechanical ballistic cannon kick.
            const float crack = 0.58F * oscillator(272.0F, t) *
                attack(t, 0.00045F) * std::exp(-t / 0.010F);
            const float particleBody = 0.82F * oscillator(92.0F, t) *
                attack(t, 0.0009F) * std::exp(-t / 0.030F);
            const float chargeEdge = 0.24F * oscillator(410.0F, t) *
                attack(t, 0.00035F) * std::exp(-t / 0.016F);
            const float finalFade = std::clamp((duration - t) / 0.008F, 0.0F, 1.0F);
            sample = (crack + particleBody + chargeEdge) * finalFade;
        } else if (command.kind == HapticEffectKind::ShipMissileLaunchThump) {
            // Missile launch: one hard ignition kick followed by a short, heavy
            // rocket-body tail. The native Wwise launch heartbeat owns cadence;
            // this waveform never creates sustain between launches.
            const float ignition = 0.78F * oscillator(176.0F, t) *
                attack(t, 0.00055F) * std::exp(-t / 0.010F);
            const float launchBody = 1.18F * oscillator(52.0F, t) *
                attack(t, 0.0015F) * std::exp(-t / 0.060F);
            const float motor = 0.46F * oscillator(82.0F, t) *
                attack(t, 0.0012F) * std::exp(-t / 0.048F);
            const float finalFade = std::clamp((duration - t) / 0.012F, 0.0F, 1.0F);
            sample = (ignition + launchBody + motor) * finalFade;
        } else if (command.kind == HapticEffectKind::ShipEMPulse) {
            // EM discharge: light electrical body with a dense high-frequency
            // crackle. Native Wwise heartbeats own cadence; there is no held-R2
            // sustain between discharges.
            const float zap = 0.60F * oscillator(338.0F, t) *
                attack(t, 0.00035F) * std::exp(-t / 0.014F);
            const float electricBody = 0.40F * oscillator(126.0F, t) *
                attack(t, 0.0008F) * std::exp(-t / 0.032F);
            const float crackle = 0.31F * oscillator(515.0F, t) *
                attack(t, 0.00025F) * std::exp(-t / 0.021F);
            const float finalFade = std::clamp((duration - t) / 0.009F, 0.0F, 1.0F);
            sample = (zap + electricBody + crackle) * finalFade;
        } else if (command.kind == HapticEffectKind::ShipTouchdownThump) {
            // Touchdown: one finite heavy body hit. The authoritative landed-state
            // transition owns timing; no continuous settle rumble is synthesized.
            const float impact = 1.20F * oscillator(58.0F, t) *
                attack(t, 0.0012F) * std::exp(-t / 0.042F);
            const float structure = 0.42F * oscillator(104.0F, t) *
                attack(t, 0.0008F) * std::exp(-t / 0.018F);
            const float finalFade = std::clamp((duration - t) / 0.010F, 0.0F, 1.0F);
            sample = (impact + structure) * finalFade;
        } else if (command.kind == HapticEffectKind::BoostpackIgnition) {
            const float ignition = 0.88F * oscillator(56.0F, t) *
                attack(t, 0.0010F) * std::exp(-t / 0.030F);
            const float jetEdge = 0.38F * oscillator(138.0F, t) *
                attack(t, 0.0007F) * std::exp(-t / 0.015F);
            const float finalFade = std::clamp((duration - t) / 0.008F, 0.0F, 1.0F);
            sample = (ignition + jetEdge) * finalFade;
        } else if (command.kind == HapticEffectKind::DigipickRotateTick) {
            const float body = 0.48F * oscillator(170.0F, t) *
                attack(t, 0.0006F) * std::exp(-t / 0.0065F);
            const float edge = 0.20F * oscillator(320.0F, t) *
                attack(t, 0.0004F) * std::exp(-t / 0.0032F);
            const float finalFade = std::clamp((duration - t) / 0.0035F, 0.0F, 1.0F);
            sample = (body + edge) * finalFade;
        } else if (command.kind == HapticEffectKind::DigipickSelectClick) {
            const float body = 0.58F * oscillator(110.0F, t) *
                attack(t, 0.0008F) * std::exp(-t / 0.010F);
            const float snap = 0.26F * oscillator(240.0F, t) *
                attack(t, 0.0005F) * std::exp(-t / 0.0048F);
            const float finalFade = std::clamp((duration - t) / 0.0045F, 0.0F, 1.0F);
            sample = (body + snap) * finalFade;
        } else if (command.kind == HapticEffectKind::DigipickInsertClunk) {
            const float clunk = 0.72F * oscillator(62.0F, t) *
                attack(t, 0.0012F) * std::exp(-t / 0.020F);
            const float edge = 0.24F * oscillator(126.0F, t) *
                attack(t, 0.0008F) * std::exp(-t / 0.0090F);
            const float finalFade = std::clamp((duration - t) / 0.0070F, 0.0F, 1.0F);
            sample = (clunk + edge) * finalFade;
        } else if (command.kind == HapticEffectKind::DigipickSuccess) {
            const float first = 0.62F * oscillator(74.0F, t) *
                attack(t, 0.0012F) * std::exp(-t / 0.018F);
            float settle = 0.0F;
            if (t >= 0.046F) {
                const float local = t - 0.046F;
                settle = 0.48F * oscillator(66.0F, local) *
                    attack(local, 0.0015F) * std::exp(-local / 0.020F);
            }
            const float finalFade = std::clamp((duration - t) / 0.010F, 0.0F, 1.0F);
            sample = (first + settle) * finalFade;
        } else if (command.kind == HapticEffectKind::LandVehicleBoostKick) {
            const float launch = 0.82F * oscillator(56.0F, t) *
                attack(t, 0.0012F) * std::exp(-t / 0.040F);
            const float structure = 0.34F * oscillator(98.0F, t) *
                attack(t, 0.0009F) * std::exp(-t / 0.022F);
            const float finalFade = std::clamp((duration - t) / 0.010F, 0.0F, 1.0F);
            sample = (launch + structure) * finalFade;
        } else if (command.kind == HapticEffectKind::LandVehicleTouchdownThump) {
            const float impact = 1.28F * oscillator(50.0F, t) *
                attack(t, 0.0010F) * std::exp(-t / 0.052F);
            const float structure = 0.50F * oscillator(88.0F, t) *
                attack(t, 0.0008F) * std::exp(-t / 0.028F);
            const float finalFade = std::clamp((duration - t) / 0.012F, 0.0F, 1.0F);
            sample = (impact + structure) * finalFade;
        } else if (command.kind == HapticEffectKind::LandVehicleGunRecoil) {
            const float crack = 0.46F * oscillator(182.0F, t) *
                attack(t, 0.00055F) * std::exp(-t / 0.0075F);
            const float body = 0.58F * oscillator(70.0F, t) *
                attack(t, 0.0010F) * std::exp(-t / 0.026F);
            const float mechanism = 0.22F * oscillator(112.0F, t) *
                attack(t, 0.0008F) * std::exp(-t / 0.014F);
            const float finalFade = std::clamp((duration - t) / 0.007F, 0.0F, 1.0F);
            sample = (crack + body + mechanism) * finalFade;
        } else if (command.kind == HapticEffectKind::IncomingDamageImpact) {
            if (gain < 0.60F) {
                const float strike = 1.65F * oscillator(235.0F, t) *
                    attack(t, 0.00018F) * std::exp(-t / 0.0032F);
                const float knock = 1.10F * oscillator(115.0F, t) *
                    attack(t, 0.00030F) * std::exp(-t / 0.0095F);
                const float finalFade = std::clamp((duration - t) / 0.004F, 0.0F, 1.0F);
                sample = (strike + knock) * finalFade;
            } else {
                const float bodyHz = 105.0F - 45.0F * gain;
                const float bodyDecay = 0.014F + 0.034F * gain;
                const float crack = 0.82F * oscillator(215.0F, t) *
                    attack(t, 0.00045F) * std::exp(-t / 0.0065F);
                const float body = 0.92F * oscillator(bodyHz, t) *
                    attack(t, 0.0012F) * std::exp(-t / bodyDecay);
                const float finalFade = std::clamp((duration - t) / 0.008F, 0.0F, 1.0F);
                sample = (crack + body) * finalFade;
            }
        } else {
            const float crack = 0.42F * oscillator(235.0F, t) *
                attack(t, 0.0007F) * std::exp(-t / 0.0070F);
            const float body = 0.72F * oscillator(105.0F, t) *
                attack(t, 0.0018F) * std::exp(-t / 0.024F);
            const float shimmer = 0.22F * oscillator(360.0F, t) *
                attack(t, 0.0005F) * std::exp(-t / 0.014F);
            const float finalFade = std::clamp((duration - t) / 0.006F, 0.0F, 1.0F);
            sample = (crack + body + shimmer) * finalFade;
        }
        sample = std::clamp(sample * gain, -1.0F, 1.0F);
        frames[i] = HapticFrame{ 0.0F, 0.0F, sample, sample };
    }

    if (!frames.empty()) {
        frames.back() = HapticFrame{};
    }
    return frames;
}
