from pathlib import Path

root = Path(__file__).resolve().parents[1]
probe_h = root / "include/StarfieldDualSense/WwiseSpatialOutputProbe.h"
probe_cpp = root / "src/starfield/WwiseSpatialOutputProbe.cpp"
gate_h = root / "include/StarfieldDualSense/WwiseSpatialProbeGate.h"
gate_cpp = root / "src/core/WwiseSpatialProbeGate.cpp"
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
readme = (root / "README.md").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

assert probe_h.exists() and probe_cpp.exists()
assert gate_h.exists() and gate_cpp.exists()
probe = probe_cpp.read_text(encoding="utf-8")

# v0.2.93 remains historical evidence for the synthetic-output experiment.
# v0.2.99 keeps that implementation and portable gate in the tree, but the
# active runtime has pivoted to a remote-VO WEM source-readability probe.
assert "Wwise Eon dynamic identity proof:" in probe
assert "RE::BGSAudio::AkSoundEngine::PostEvent(" in probe
assert "Sleep(" not in probe and "sleep_for" not in probe
assert "loopback" not in probe.lower()

assert "observeRightTrigger" not in plugin
assert "g_wwiseSpatialProbe" not in plugin
assert "0.3.01-voice-archive-manifest-probe" in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert 'target("sds-wwise-spatial-probe-tests"' in xmake
assert '"src/starfield/WwiseSpatialOutputProbe.cpp"' in xmake
assert "Current test build: v0.3.01" in readme
assert "v0.2.93 Wwise spatial secondary-output canary" in readme
assert "## 0.2.93" in changelog

runtime = plugin[plugin.index("if (config.controllerSpeaker && config.debugLogging)"):plugin.index("if (const auto* tasks", plugin.index("if (config.controllerSpeaker && config.debugLogging)"))]
assert "WwiseRemoteVoMirror" not in runtime
assert "StarfieldAudioCapture" in runtime
assert "WwiseSpatialOutputProbe" not in runtime

print("PASS v0.2.93 spatial-canary history / v0.2.99 WEM-source successor regression")
