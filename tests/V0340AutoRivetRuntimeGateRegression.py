from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
engine = (root / 'src/core/HapticsEngine.cpp').read_text(encoding='utf-8')
manager = (root / 'src/core/HapticsManager.cpp').read_text(encoding='utf-8')
header = (root / 'include/StarfieldDualSense/AutoRivetChargeSemantics.h').read_text(encoding='utf-8')

xmake = (root / 'xmake.lua').read_text(encoding='utf-8')

assert 'sds-v0340-auto-rivet-gameplay-gate-tests' in xmake
assert 'sds-v0340-auto-rivet-runtime-safety-tests' in xmake
assert 'sds-v0340-auto-rivet-wwise-semantic-tests' in xmake

# Auto-Rivet charge semantics are a haptics dependency, not a speaker-feature
# dependency. The Wwise observer/capture gate must remain available when
# advanced haptics are enabled even if weapon speakers are disabled.
assert 'autoRivetChargeCaptureEnabled = config.advancedHaptics' in plugin
assert 'g_autoRivetChargeCaptureArmed' in plugin
assert 'remoteVoCaptureEnabled || weaponSpeakerCaptureEnabled || autoRivetChargeCaptureEnabled' in plugin
assert 'g_autoRivetChargeCaptureArmed.load' in plugin

assert '#include <StarfieldDualSense/AutoRivetChargeSemantics.h>' in plugin
assert 'routeAutoRivetChargeWwise' in plugin
assert 'autoRivetChargeMarker' in plugin
assert 'Auto-Rivet tension: charge-start' in plugin
assert 'Auto-Rivet tension: charge-stop' in plugin
assert 'g_haptics->handle' in plugin

assert 'AutoRivetChargeStart' in engine
assert 'AutoRivetChargeStop' in engine
assert '_autoRivetChargeAuthorized' in engine
assert 'DataMenu' in engine and 'PauseMenu' in engine and 'LoadingMenu' in engine
assert 'GameEventType::MenuOpened' in manager
assert 'GameEventType::MenuClosed' in manager

assert 'kAutoRivetChargeStartEventId = 0xB963BD33u' in header
assert 'kAutoRivetChargeStopEventId = 0xCDEE7F71u' in header
assert 'kAutoRivetPlayerGameObjectId = 0x2u' in header

print('v0.3.40 Auto-Rivet runtime gate regression PASS')
