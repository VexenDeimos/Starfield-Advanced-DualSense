from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
catalog = (root / "include/StarfieldDualSense/UiAudioCandidateCatalog.h").read_text(encoding="utf-8")
probe_h = (root / "include/StarfieldDualSense/UiAudioDiscoveryProbe.h").read_text(encoding="utf-8")
probe_cpp = (root / "src/core/UiAudioDiscoveryProbe.cpp").read_text(encoding="utf-8")
playback = (root / "src/core/UiSpeakerPlayback.cpp").read_text(encoding="utf-8")
transport = (root / "src/core/DualSenseAudioRenderBlock.cpp").read_text(encoding="utf-8")

assert "0.3.60-main-menu-controller-speaker-startup" in plugin or "0.3.61-ship-pilot-context" in plugin or "0.3.62-propulsion-" in plugin or "0.3.63-ship-ballistic-haptics" in plugin or "0.3.64-ship-laser-recon" in plugin or ("0.3.65-ship-laser-haptics" in plugin or "0.3.65-r2-ship-laser-tactile-retune" in plugin) or ("0.3.66-ship-particle-recon" in plugin or "0.3.67-ship-particle-haptics" in plugin or "0.3.68-ship-missile-recon" in plugin or ("0.3.69-ship-missile-haptics" in plugin or ("0.3.70-ship-em-recon" in plugin or ("0.3.71-ship-em-haptics" in plugin or "0.3.72-ship-launch-landing-recon" in plugin))))
assert xmake.count('set_version("0.3.60")') == 2 or xmake.count('set_version("0.3.61")') == 2 or xmake.count('set_version("0.3.62")') == 2 or xmake.count('set_version("0.3.63")') == 2 or xmake.count('set_version("0.3.64")') == 2 or xmake.count('set_version("0.3.65")') == 2 or xmake.count('set_version("0.3.66")') == 2 or xmake.count('set_version("0.3.67")') == 2 or xmake.count('set_version("0.3.68")') == 2 or (xmake.count('set_version("0.3.69")') == 2 or (xmake.count('set_version("0.3.70")') == 2 or (xmake.count('set_version("0.3.71")') == 2 or xmake.count('set_version("0.3.72")') == 2)))
assert 'target("sds-v0359-expanded-menu-promotion-tests"' in xmake
assert 'target("sds-v0359-ui-diagnostic-resolution-tests"' in xmake
assert 'target("sds-v0359-ui-diagnostic-backend-tests"' in xmake

production = catalog[catalog.index("kUiSpeakerCueDefinitions"):catalog.index("uiSpeakerCueDefinitions()")]
for event_id, name in [
    ("0x05234A32u", "UIMenuGeneralFocus"),
    ("0x5C8034FCu", "UIMenuGeneralOK"),
    ("0x7956E9B0u", "UIMenuGeneralCancel"),
    ("0x12D8B183u", "UIMenuMonocleOpen"),
    ("0x1F770B61u", "UIMenuMonocleClose"),
    ("0xF488F841u", "UIMenuSkillsSkillFocus"),
    ("0x7470A961u", "UIMenuStarmapRolloverFade"),
    ("0xC7F9CACCu", "UIMenuSurfaceMapRollover"),
    ("0x0976086Cu", "UIMenuMissionsMenuSelectionChange"),
    ("0xB32B4C8Eu", "UIMenuMissionsMenuSubtasksToggle"),
]:
    assert event_id in production and name in production
assert "0x06D80D5Eu" not in production
assert "std::array<UiSpeakerCueDefinition, 10>" in catalog
assert "std::array<UiAudioResolutionTarget, 33> kV0359UiAudioResolutionTargets" in catalog

assert "kUiAudioDiscoveryMaxDuration = std::chrono::seconds(300)" in probe_h
assert 'menuName == "DataMenu"' in probe_cpp
assert "maxDurationMs=300000" in plugin
assert "end=DataMenu-close" in plugin
assert "hudFollowupMs=60000" in plugin
assert "unpromotedOnly=yes" in plugin
assert "targetedResolution=" in plugin

# Frozen accepted behavior: no cadence/tuning or proven USB route changes.
for forbidden in ["debounce", "30ms", "milliseconds(30)", "replaceExisting=true"]:
    assert forbidden not in playback
assert "return { 0.0F, mono };" in transport
assert "captureUiCandidateWem" not in (root / "src/core/WeaponAudioPipelineBackend.cpp").read_text(encoding="utf-8")

print("PASS v0.3.59 expanded menu promotion, five-minute cap, targeted resolution, and frozen routing retained through v0.3.71")
