from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

def read(path: str) -> str:
    p = root / path
    return p.read_text(encoding="utf-8") if p.exists() else ""

plugin = read("src/starfield/Plugin.cpp")
haptics = read("src/core/HapticsManager.cpp")
waveforms = read("src/core/HapticWaveforms.cpp")
effects = read("src/core/EffectsEngine.cpp")
xmake = read("xmake.lua")

checks = {
    "r1 tactile baseline remains active in r1/r2": any(marker in plugin for marker in [
        "0.3.71-ship-em-haptics-r1-tactile-reliability-retune",
        "0.3.71-ship-em-haptics-r2-trigger-rearm",
        "0.3.72-ship-launch-landing-recon",
    ]),
    "file version remains 0.3.71": (xmake.count('set_version("0.3.71")') == 2 or xmake.count('set_version("0.3.72")') == 2),
    "authority identity unchanged": "event=0x7A1A570C" in plugin,
    "EM pulse gain retuned to 0.70": "0.70F * _config.hapticStrength" in haptics,
    "EM pulse duration retuned to 75 ms": "case HapticEffectKind::ShipEMPulse:" in waveforms and "duration = 0.075F;" in waveforms,
    "EM trigger starts earlier at 72": "effect.startPosition = 72;" in effects,
    "EM trigger stronger electrical envelope": all(x in effects for x in [
        "scaledByte(132, _config.triggerStrength)",
        "scaledByte(196, _config.triggerStrength)",
        "scaledByte(112, _config.triggerStrength)",
        "effect.frequency = 92;",
    ]),
    "EM trigger lifetime retuned to 90 ms": "std::chrono::milliseconds(90)" in effects,
    "startup advertises r1 tactile contract": all(x in plugin for x in [
        "pulseMs=75", "pulseGain=0.70", "triggerMs=90", "tactileRetune=r1"
    ]),
    "no synthetic EM cadence": "syntheticEM" not in plugin and "emSynthetic" not in plugin,
}

failed = [name for name, ok in checks.items() if not ok]
if failed:
    print("FAIL v0.3.71-r1 EM tactile reliability retune regression: " + ", ".join(failed))
    sys.exit(1)
print("PASS v0.3.71-r1 EM tactile reliability retune regression")
