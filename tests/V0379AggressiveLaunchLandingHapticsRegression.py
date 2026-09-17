from pathlib import Path

root = Path(__file__).resolve().parents[1]
types_h = (root / "include/StarfieldDualSense/Types.h").read_text(encoding="utf-8")
effects_h = (root / "include/StarfieldDualSense/EffectsEngine.h").read_text(encoding="utf-8")
effects_cpp = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
haptics_cpp = (root / "src/core/HapticsManager.cpp").read_text(encoding="utf-8")
plugin_cpp = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

# Release identity.
assert xmake.count('set_version("0.3.79")') >= 2
assert '0.3.79-aggressive-launch-landing-haptics' in plugin_cpp

# Transition semantics must be explicit controller events, not direct USB writes.
assert "ShipLaunchLandingHapticsStarted" in types_h
assert "ShipLaunchLandingHapticsStopped" in types_h
assert "shipLaunchLandingTrigger" in effects_h
assert "shipLaunchLandingTrigger" in effects_cpp
assert "_shipLaunchLandingHapticsActive" in effects_h
assert "sds-v0379-ship-launch-landing-trigger-tests" in xmake
assert "sds-v0379-aggressive-launch-landing-haptics-tests" in xmake

# Aggressive trigger texture required by the approved design: sustained EffectEx on both triggers.
assert "effect.mode = TriggerEffectMode::EffectEx" in effects_cpp
assert "effect.keepEffect = true" in effects_cpp
assert "_state.output.leftTrigger = shipLaunchLandingTrigger();" in effects_cpp
assert "_state.output.rightTrigger = shipLaunchLandingTrigger();" in effects_cpp
assert "_state.output.leftTrigger = {};" in effects_cpp
assert "restorePersistentTrigger();" in effects_cpp
assert "_shipPilotActive || _shipLaunchLandingHapticsActive" in effects_cpp
assert "if (_shipPilotActive || _shipLaunchLandingHapticsActive)" not in effects_cpp

# Approved rumble retune: full ShipBoost body, not the old 0.30 ShipPropulsion bed.
assert "kShipLaunchLandingRumbleGain = 1.00F" in haptics_cpp
assert "kShipLaunchLandingRumbleLevel = 1.00F" in haptics_cpp
assert ".kind = HapticContinuousKind::ShipBoost" in haptics_cpp
assert '"ShipBoost"' in haptics_cpp
assert "_shipPilotActive || _shipLaunchLandingRumbleActive" in haptics_cpp
assert "if (_shipBlockingMenuMask != 0)" in haptics_cpp
assert "continuous = _shipLaunchLandingRumbleActive ?" in haptics_cpp
assert "} else if (!contextHandled &&\n                       (type == GameEventType::WeaponFired ||" in haptics_cpp

# Takeoff must survive landed true->false and the first fader close, then end on
# the next fader-open edge (or loading/timeout safety) rather than at liftoff.
assert "takeoff-boundary-observed" in plugin_cpp
assert 'eventIdentity(event) == "FaderMenu"' in plugin_cpp
assert "takeoff-first-fader-closed" in plugin_cpp
assert "takeoff-next-fader-open" in plugin_cpp
assert "takeoff-loading-fallback" in plugin_cpp
assert "takeoff-timeout" in plugin_cpp
assert 'stopShipLaunchLandingHaptics("takeoff-boundary")' not in plugin_cpp

# Landing continues through touchdown; thump remains exactly the accepted 0.90 path.
assert "landing-touchdown-observed" in plugin_cpp
assert "landing-sequence-end" in plugin_cpp
assert ".kind = HapticEffectKind::ShipTouchdownThump" in haptics_cpp
assert ".gain = std::clamp(0.90F * _config.hapticStrength" in haptics_cpp

# No new speaker/lightbar presentation is introduced by this slice.
assert "aggressiveLaunchLandingSpeaker" not in plugin_cpp
assert "aggressiveLaunchLandingLightbar" not in plugin_cpp

print("PASS v0.3.79 aggressive launch/landing haptics source regression")
