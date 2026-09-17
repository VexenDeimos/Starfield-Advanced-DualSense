from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")
structure_source = (root / "src/core/WwiseWemStructureProbe.cpp").read_text(encoding="utf-8")
structure_test = (root / "tests/WwiseWemStructureProbeTest.cpp").read_text(encoding="utf-8")

required = [
    root / "include/StarfieldDualSense/RemoteVoSpeakerPlayback.h",
    root / "src/core/RemoteVoSpeakerPlayback.cpp",
    root / "tests/RemoteVoSpeakerPlaybackTest.cpp",
]
for path in required:
    assert path.exists(), f"v0.3.07 required file missing: {path.relative_to(root)}"

assert '0.3.07-remote-vo-controller-playback' in plugin
assert xmake.count('set_version("0.3.7")') >= 2
assert '#include <StarfieldDualSense/RemoteVoSpeakerPlayback.h>' in plugin
assert 'prepareRemoteVoControllerPlayback(decodeResult)' in plugin
assert 'SpeakerCategory::Comms' in plugin
assert 'g_speakerManager->submitCaptured' in plugin
assert 'Voice controller playback: submit=' in plugin
assert 'resample=44100-to-48000' in plugin
assert 'playback=one-shot-controller-speaker' in plugin
assert 'src/core/RemoteVoSpeakerPlayback.cpp' in xmake
assert 'target("sds-remote-vo-speaker-playback-tests"' in xmake
assert 'tests/RemoteVoSpeakerPlaybackTest.cpp' in xmake
assert 'payloadEnd == scanEnd' in structure_source
assert 'makeSyntheticWemWithUnpaddedOddFinalData' in structure_test
assert 'assert(oddFinal.scanComplete)' in structure_test

print("PASS v0.3.07 remote VO controller playback regression")
