from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
adapter_h = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
adapter_cpp = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")

assert "HapticsManager" in plugin
assert "DualSenseAudioHapticsClient" in plugin
assert "DualSenseAudioTransport" in plugin
assert "RuntimeEventRouter" in plugin
assert "using EmitCallback = std::function<bool(GameEvent)>" in adapter_h
assert "ControllerManager& _controller" not in adapter_h
assert "_emit(std::move(event))" in adapter_cpp
assert "g_eventRouter" in plugin
assert "g_haptics->stop()" in plugin
print("PASS v0.2.43 runtime haptics event-routing source regression")

xmake = (root / "xmake.lua").read_text(encoding="utf-8")
effects = (root / "src/core/EffectsEngine.cpp").read_text(encoding="utf-8")
native_usb = (root / "src/windows/NativeUsbBackend.cpp").read_text(encoding="utf-8")
hid_trace = (root / "src/windows/HidWriteTrace.cpp").read_text(encoding="utf-8")
audio_backend = (root / "src/windows/DualSenseAudioTransport.cpp").read_text(encoding="utf-8")

assert '0.3.01-voice-archive-manifest-probe' in plugin
assert xmake.count('set_version("0.3.1")') >= 2
assert "DualSenseAudioTransport.cpp" in xmake
assert "DualSenseAudioHapticsClient.cpp" in xmake
assert 'add_syslinks("hid", "setupapi", "ole32", "uuid")' in xmake
assert "DualSenseAudioHapticsBackend" not in native_usb
assert "IAudioClient" not in native_usb
assert "IAudioClient" not in hid_trace
assert "GetDefaultAudioEndpoint" not in audio_backend
assert "#include <propkeydef.h>" in audio_backend
assert audio_backend.index("#include <propkeydef.h>") < audio_backend.index("#include <functiondiscoverykeys_devpkey.h>")
assert "EnumAudioEndpoints" in audio_backend
assert "AUDCLNT_STREAMFLAGS_EVENTCALLBACK" in audio_backend
assert "beginForce = scaledByte(195" in effects
assert "middleForce = scaledByte(255" in effects
print("PASS v0.2.43 USB haptics transport proof source regression")
