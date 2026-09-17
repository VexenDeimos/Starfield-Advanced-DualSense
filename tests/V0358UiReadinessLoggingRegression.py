from pathlib import Path

root = Path(__file__).resolve().parents[1]
pipeline = (root / "src/core/WeaponAudioPipeline.cpp").read_text(encoding="utf-8")

assert 'eventName=\\\"" << cue.eventName << \'"\'' in pipeline
assert 'eventName="" << cue.eventName << """' not in pipeline

print("PASS v0.3.58 UI readiness log fix retained through v0.3.60")
