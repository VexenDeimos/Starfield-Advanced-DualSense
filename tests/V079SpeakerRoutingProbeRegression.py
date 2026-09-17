from pathlib import Path
root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
reports = (root / 'src/core/DualSenseReports.cpp').read_text(encoding='utf-8')
native = (root / 'src/windows/NativeUsbBackend.cpp').read_text(encoding='utf-8')
config = (root / 'config/StarfieldDualSense.toml').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')

assert 'report[1] = static_cast<std::uint8_t>(report[1] | 0xA0U)' in reports
assert 'report[2] = static_cast<std::uint8_t>(report[2] | 0x80U)' in reports
assert 'report[8] = 0x30U' in reports
assert 'writeReport(report, "speaker-route", false)' in native
assert '.controllerSpeaker = _impl->speakerRoutingActive' in native
assert 'SpeakerProbeMode' not in config
assert 'SpeakerRoutingProbe' not in plugin
print('PASS v0.2.79 hardware-proven physical speaker route remains while temporary probe is removed')
