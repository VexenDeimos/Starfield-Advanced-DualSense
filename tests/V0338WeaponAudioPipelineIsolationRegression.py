from pathlib import Path

root = Path(__file__).resolve().parents[1]
profile_cpp = (root / "src/core/WeaponSpeakerProfile.cpp").read_text(encoding="utf-8")
resolver = (root / "src/core/WwiseEventMediaResolver.cpp").read_text(encoding="utf-8")

assert '"XM-2311"' in profile_cpp
assert '"Old Earth Pistol"' in profile_cpp
for forbidden in ["inferAudioFamily", "inferSharedMedia", "autoAliasByEvent", "autoAliasByMedia"]:
    assert forbidden not in profile_cpp

# Resolver ownership must be explicit-family based rather than relying on first-match behavior.
assert "speakerAudioFamily(profile) != profile.weaponIdentity" in resolver
assert "findWeaponSpeakerAudioFamilyProfile" in resolver

backend_path = root / "src/core/WeaponAudioPipelineBackend.cpp"
assert backend_path.exists(), "real weapon-audio backend source must exist"
backend = backend_path.read_text(encoding="utf-8")
assert "makeRealWeaponAudioPipelineBackend" in backend
assert "decodeWeaponSpeakerWemToSpeakerPcm" in backend
assert "prepareWeaponSpeakerPcm" in backend
assert "WeaponSfxMediaCorrelation" in backend

plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

assert "WeaponAudioPipeline" in plugin
assert "g_weaponAudioPipeline" in plugin
assert "mode=batch-any-weapon" not in plugin
assert "workerResolution=background" in plugin
assert "discoveryQueue=64" in plugin
assert "profile=XM-2311 family=Old Earth Pistol mode=explicit-shared" in plugin

runtime_tick = plugin[plugin.index("    void runtimeTick()") : plugin.index("    void initializeRuntime()")]
for forbidden in [
    "WwiseEventMediaResolver>(",
    "->prepare(true)",
    "->run(true)",
    "decodeWwisePcmWemToSpeakerPcm",
    "prepareWeaponSpeakerPcm",
    "g_weaponSfxMediaCorrelation->correlate",
]:
    assert forbidden not in runtime_tick, forbidden

for required in [
    "g_haptics->tick",
    "g_controller->tryPopInputAction",
    "pollNativeInputInjection",
    "pollHealth",
    "g_audioCapture->drainDiagnostics",
    "tryEnqueueDiscovery",
    "tryTakeDiagnostics",
]:
    assert required in runtime_tick, required

assert "sds::HidWriteTrace::stop" in plugin
assert "submitCaptured" in plugin
assert "g_weaponAudioPipeline->stop()" in plugin

shutdown = plugin[plugin.index("    void shutdownRuntime()") : plugin.index("    void runtimeTick()")]
assert shutdown.index("g_audioCapture->stop()") < shutdown.index("g_fireMarkerBridge->stop()")
assert shutdown.index("g_fireMarkerBridge->stop()") < shutdown.index("g_gameState->unregisterSinks()")
assert shutdown.index("g_gameState->unregisterSinks()") < shutdown.index("g_weaponAudioPipeline->stop()")
assert shutdown.index("g_weaponAudioPipeline->stop()") < shutdown.index("g_haptics->handle")
