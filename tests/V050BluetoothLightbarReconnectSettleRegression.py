from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

backend = (
    root / "src/windows/NativeBluetoothBackend.cpp"
).read_text(encoding="utf-8")

checks = [
    (
        "Bluetooth connection time is retained",
        "connectionEstablishedAt" in backend,
    ),
    (
        "connection timestamp resets on disconnect",
        "connectionEstablishedAt = {};" in backend,
    ),
    (
        "successful Bluetooth connection records host time",
        "_impl->connectionEstablishedAt =" in backend
        and "std::chrono::steady_clock::now();" in backend,
    ),
    (
        "LED takeover requires two-second host settle",
        "kHostLedSettleDelay" in backend
        and "std::chrono::milliseconds(2000)" in backend,
    ),
    (
        "controller timestamp readiness gate remains",
        "kLedStartupCompleteTimestamp" in backend
        and "timestamp < kLedStartupCompleteTimestamp" in backend,
    ),
    (
        "host settle occurs before LED-ready latch",
        backend.find("kHostLedSettleDelay")
        < backend.find("lightbarStartupReady = true;"),
    ),
    (
        "startup telemetry exposes host settle timing",
        "hostElapsedMs=" in backend,
    ),
    (
        "legacy retry burst has been removed",
        "lightbarTakeoverRetriesRemaining" not in backend
        and "armLightbarTakeoverFailsafe" not in backend
        and "serviceLightbarTakeoverFailsafe" not in backend
        and "Bluetooth LED takeover failsafe:" not in backend,
    ),
]

failed = []

for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print("FAILED:", ", ".join(failed))
    sys.exit(1)

print("PASS v0.5.0 Bluetooth lightbar reconnect settle contract")