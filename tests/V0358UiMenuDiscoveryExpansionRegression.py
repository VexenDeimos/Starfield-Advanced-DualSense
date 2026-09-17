from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
catalog = (root / "include/StarfieldDualSense/UiAudioCandidateCatalog.h").read_text(encoding="utf-8")
playback = (root / "src/core/UiSpeakerPlayback.cpp").read_text(encoding="utf-8")
transport = (root / "src/core/DualSenseAudioRenderBlock.cpp").read_text(encoding="utf-8")

assert "0.3.60-main-menu-controller-speaker-startup" in plugin
assert xmake.count('set_version("0.3.60")') == 2
assert 'target("sds-v0358-ui-discovery-expansion-tests"' in xmake

production = catalog[catalog.index("kUiSpeakerCueDefinitions"):catalog.index("uiSpeakerCueDefinitions()")]
for event_id, name in [
    ("0x05234A32u", "UIMenuGeneralFocus"),
    ("0x5C8034FCu", "UIMenuGeneralOK"),
    ("0x7956E9B0u", "UIMenuGeneralCancel"),
    ("0x12D8B183u", "UIMenuMonocleOpen"),
    ("0x1F770B61u", "UIMenuMonocleClose"),
]:
    assert event_id in production and name in production
assert production.count("requiredGameObjectId") == 0 or production.count("0x3u") == 10
assert "0x06D80D5Eu" not in production
for promoted in ["0x7470A961u", "0xC7F9CACCu", "0xF488F841u", "0x0976086Cu", "0xB32B4C8Eu"]:
    assert promoted in production

for forbidden_tuning in ["debounce", "30ms", "milliseconds(30)", "replaceExisting=true"]:
    assert forbidden_tuning not in playback
assert "return { 0.0F, mono };" in transport

assert "unpromotedOnly=yes" in plugin
assert "existingPromotedPlayback=" in plugin
assert "playback=production-promoted" in plugin
assert "normalGameAudio=untouched" in plugin

print("PASS v0.3.58 discovery architecture and accepted base cues retained through v0.3.60")
