from pathlib import Path
import sys

root = Path(__file__).resolve().parents[1]

def read(rel):
    return (root / rel).read_text(encoding="utf-8")

interface = read("include/StarfieldDualSense/IControllerSpeakerBackend.h")
usb_h = read("include/StarfieldDualSense/DualSenseAudioSpeakerClient.h")
usb_cpp = read("src/windows/DualSenseAudioSpeakerClient.cpp")
manager_h = read("include/StarfieldDualSense/ControllerSpeakerManager.h")
manager_cpp = read("src/core/ControllerSpeakerManager.cpp")
bt_h = read("include/StarfieldDualSense/BluetoothSpeakerBackend.h")
bt_cpp = read("src/windows/BluetoothSpeakerBackend.cpp")
dual_h = read("include/StarfieldDualSense/DualModeSpeakerBackend.h")
dual_cpp = read("src/windows/DualModeSpeakerBackend.cpp")
plugin = read("src/starfield/Plugin.cpp")

checks = [
    (
        "shared speaker backend exposes master volume",
        "virtual void setSpeakerVolume(float volume) noexcept" in interface,
    ),
    (
        "USB speaker backend exposes master volume",
        "void setSpeakerVolume(float volume) noexcept override;" in usb_h
        and "_transport->setSpeakerVolume(volume);" in usb_cpp,
    ),
    (
        "manager owns transport-neutral master volume",
        "void setSpeakerVolume(float volume) noexcept;" in manager_h
        and "_backend->setSpeakerVolume(" in manager_cpp,
    ),
    (
        "Bluetooth backend exposes master volume",
        "void setSpeakerVolume(float volume) noexcept override;" in bt_h,
    ),
    (
        "Bluetooth live volume updates existing mixer",
        "_impl->mixer.setGlobalVolume(" in bt_cpp
        and "_impl->mixerMutex" in bt_cpp,
    ),
    (
        "dual-mode backend exposes master volume",
        "void setSpeakerVolume(float volume) noexcept override;" in dual_h,
    ),
    (
        "dual-mode volume reaches both transports",
        "_wiredBackend->setSpeakerVolume(" in dual_cpp
        and "_bluetoothBackend->setSpeakerVolume(" in dual_cpp,
    ),
    (
        "Plugin live setting uses transport-neutral manager",
        "g_speakerManager->setSpeakerVolume(" in plugin
        and "live.speakerVolume" in plugin,
    ),
]

failed = []

for label, ok in checks:
    print(("PASS" if ok else "FAIL"), label)
    if not ok:
        failed.append(label)

if failed:
    print("FAILED checks:", ", ".join(failed))
    sys.exit(1)

print("PASS Bluetooth controller-speaker live master volume contract")