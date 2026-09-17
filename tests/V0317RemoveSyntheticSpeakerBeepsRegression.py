from pathlib import Path

root = Path(__file__).resolve().parents[1]
classifier = (root / "src/core/SpeakerEventClassifier.cpp").read_text(encoding="utf-8")
manager = (root / "src/core/ControllerSpeakerManager.cpp").read_text(encoding="utf-8")
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

assert "0.3.17-remove-synthetic-speaker-beeps" in plugin
assert 'set_version("0.3.17")' in xmake
assert "return std::nullopt;" in classifier
for frequency in ("900.0F", "1350.0F", "720.0F", "760.0F", "920.0F", "520.0F", "1040.0F", "680.0F", "480.0F"):
    assert frequency not in classifier, frequency
assert "submitCaptured" in manager
assert "enqueuePreparedPcm" in manager and "replacePreparedPcm" in manager
print("PASS v0.3.17 synthetic controller-speaker beeps removed while captured PCM path remains")
