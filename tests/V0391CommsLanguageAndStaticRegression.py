from pathlib import Path

root = Path(__file__).resolve().parents[1]

config_h = (root / "include/StarfieldDualSense/Config.h").read_text(encoding="utf-8")
config_cpp = (root / "src/core/Config.cpp").read_text(encoding="utf-8")
config_toml = (root / "config/StarfieldDualSense.toml").read_text(encoding="utf-8")
manifest_h = (root / "include/StarfieldDualSense/WwiseVoiceArchiveManifestProbe.h").read_text(encoding="utf-8")
manifest_cpp = (root / "src/core/WwiseVoiceArchiveManifestProbe.cpp").read_text(encoding="utf-8")
remote_h = (root / "include/StarfieldDualSense/WwiseRemoteVoMirrorGate.h").read_text(encoding="utf-8")
remote_cpp = (root / "src/core/WwiseRemoteVoMirrorGate.cpp").read_text(encoding="utf-8")
catalog = (root / "include/StarfieldDualSense/UiAudioCandidateCatalog.h").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
audio_capture = (root / "src/starfield/StarfieldAudioCapture.cpp").read_text(encoding="utf-8")
weapon_backend = (root / "src/core/WeaponAudioPipelineBackend.cpp").read_text(encoding="utf-8")

# Language configuration defaults to Auto while explicit English remains supported.
assert "enum class SpeakerVoiceLanguage" in config_h
for language in ["English", "Auto", "French", "German", "Spanish", "Japanese"]:
    assert language in config_h, f"missing SpeakerVoiceLanguage value: {language}"

assert "SpeakerVoiceLanguage speakerVoiceLanguage{ SpeakerVoiceLanguage::Auto };" in config_h
assert 'SpeakerVoiceLanguage = "Auto"' in config_toml
assert "SpeakerVoiceLanguage" in config_cpp
assert "Auto" in config_cpp
assert "Spanish" in config_cpp

# Localized archive selection is language-aware and explicit choices do not
# silently fall back to English.
assert "sResourceEnglishVoiceList" in manifest_cpp
assert "sResourceLocaleVoiceList" in manifest_cpp
for suffix in ["Starfield_fr.ini", "Starfield_de.ini", "Starfield_es.ini", "Starfield_ja.ini"]:
    assert suffix in manifest_cpp, f"missing localized INI mapping: {suffix}"

assert "resolveAutoSpeakerVoiceLanguage" in manifest_h
assert "resolveAutoSpeakerVoiceLanguage" in manifest_cpp
assert "bUseLocaleVoices:Controls" in plugin
assert "sLanguage:General" in plugin
assert "resolvedSpeakerVoiceLanguage" in plugin

# Remote/radio VO continuation uses the runtime-proven dialogue event and
# accepts both the immediate post-answer closed state and later open state.
assert "0x06638D4E" in remote_h
assert "kRemoteCommsDialogueOpeningVoMirrorProfile" in remote_h
assert "kRemoteCommsDialogueVoMirrorProfile" in remote_cpp
assert "qualifiesRemoteCommsVoCandidate" in remote_cpp
assert "qualifiesRemoteCommsVoCandidate(candidate)" in audio_capture

# Exact authored ship-comms static event is routed as Comms with no fixed
# runtime game object.
assert '{ 0x27A3CE98u, "VOC_SFX_ShipComms_Static", 0x0u, SpeakerCategory::Comms }' in catalog

# ControllerOnly suppresses original PC static only after successful
# controller submission. Failed submission therefore remains fail-safe.
static_event_pos = plugin.index("observation.eventId == 0x27A3CE98u")
static_stop_region = plugin[static_event_pos - 200:static_event_pos + 500]

assert "if (submitted &&" in static_stop_region
assert "g_speakerManager->outputMode()" in static_stop_region
assert "sds::SpeakerOutputMode::ControllerOnly" in static_stop_region
assert "observation.returnedPlayingId != 0u" in static_stop_region
assert "sds::stopWwisePlayingId(" in static_stop_region
assert "observation.returnedPlayingId" in static_stop_region

# Startup description reflects the current mixed catalog.
assert "UI/digipick/crafting/comms speaker: ACTIVE cues=" in plugin
assert "categories=ScannerUI+Digipick+Crafting+Comms gameObject=per-cue" in plugin

# Temporary runtime-recon instrumentation must not ship.
combined = plugin + audio_capture + weapon_backend
for stale in [
    "Comms ancillary recon",
    "Comms ship-audio recon",
    "Comms static identity/media",
    "Comms static controller playback:",
]:
    assert stale not in combined, f"temporary recon string still present: {stale}"

print("PASS v0.3.91 comms language, continuation, and ship-static regression")