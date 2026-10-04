from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

wave = (root / "src/core/HapticWaveforms.cpp").read_text(encoding="utf-8")
bt = (root / "src/windows/DualModeHapticsBackend.cpp").read_text(encoding="utf-8")
tests = (root / "tests/HapticsTest.cpp").read_text(encoding="utf-8")
changelog = (root / "CHANGELOG.md").read_text(encoding="utf-8")

checks = [
    (
        "Eon USB haptic lasts 70 ms",
        "case HapticEffectKind::EonSnap:\n        duration = 0.070F;" in wave,
    ),
    (
        "generic ballistic handgun USB haptic lasts 70 ms",
        "case HapticEffectKind::BallisticHandgunKick:\n        duration = 0.070F;" in wave,
    ),
    (
        "Eon uses sustained low-frequency buzz body",
        "const float buzz = 0.92F * oscillator(112.0F, t)" in wave
        and "std::exp(-t / 0.050F)" in wave,
    ),
    (
        "generic handgun uses sustained buzz body",
        "const float buzz = 0.84F * oscillator(108.0F, t)" in wave,
    ),
    (
        "old amplitude-only handgun multiplier is removed",
        "kBallisticHandgunUsbHapticGain" not in wave,
    ),
    (
        "Eon Bluetooth reference pulse remains unchanged at 55 ms",
        'return { 120, 210, 55ms, "EonSnap" };' in bt,
    ),
    (
        "generic handgun Bluetooth reference pulse remains unchanged at 55 ms",
        'return { 155, 210, 55ms, "BallisticHandgunKick" };' in bt,
    ),
    (
        "HapticsTest expects Eon 70 ms buzz",
        'eonWave.size() == 3360' in tests
        and "Eon USB waveform is exactly 70 ms at 48 kHz" in tests,
    ),
    (
        "HapticsTest expects generic handgun 70 ms buzz",
        'handgunWave.size() == 3360' in tests
        and "BallisticHandgunKick USB waveform is exactly 70 ms at 48 kHz" in tests,
    ),
    (
        "HapticsTest requires sustained late-window Eon energy",
        'actuatorRmsWindow(eonWave, 25.0F, 45.0F) > 0.07F' in tests,
    ),
    (
        "changelog describes final buzz retune",
        "70 ms sustained buzz-style pulse" in changelog,
    ),
]

failed = []
for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print()
    print("FAILED v0.6.1 handgun USB buzz checks:")
    for label in failed:
        print(" -", label)
    sys.exit(1)

print()
print("PASS v0.6.1 USB handgun Bluetooth-parity buzz contract")
