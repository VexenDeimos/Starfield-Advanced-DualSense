from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
capture = (root / "src/starfield/StarfieldAudioCapture.cpp").read_text(encoding="utf-8")
probe = (root / "src/core/UiAudioDiscoveryProbe.cpp").read_text(encoding="utf-8")

assert "const bool uiAudioDiscoveryEnabled = config.debugLogging;" in plugin
assert "g_uiAudioDiscovery = std::make_unique<sds::UiAudioDiscoveryProbe>();" in plugin
assert "UI audio discovery: ACTIVE diagnostic-only" in plugin
assert "unpromotedOnly=yes" in plugin
assert "existingPromotedPlayback=" in plugin
assert "!sds::isPromotedUiSpeakerEvent(eventId)" in capture
assert "isPromotedUiSpeakerEvent(observation.eventId)" in probe

print("PASS v0.3.58 bounded unpromoted UI discovery activation retained through v0.3.60")
