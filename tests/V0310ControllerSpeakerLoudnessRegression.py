from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')
reports_h = (root / 'include/StarfieldDualSense/DualSenseReports.h').read_text(encoding='utf-8')
native_usb = (root / 'src/windows/NativeUsbBackend.cpp').read_text(encoding='utf-8')
comms = (root / 'src/core/CommsProcessor.cpp').read_text(encoding='utf-8')
remote = (root / 'src/core/RemoteVoSpeakerPlayback.cpp').read_text(encoding='utf-8')
remote_h = (root / 'include/StarfieldDualSense/RemoteVoSpeakerPlayback.h').read_text(encoding='utf-8')

assert '0.3.10-controller-speaker-loudness' in plugin
assert xmake.count('set_version("0.3.10")') >= 2
assert 'preampGain = 0x05' in reports_h
assert 'volume=0x64 preamp=0x05' in native_usb
assert 'kMakeupGain = 1.99526231F' in comms
assert 'kLimiterThreshold = 0.90F' in comms
assert 'softLimit' in comms
assert 'processCommsPcm(*prepared)' in remote
assert 'commsPrePeak' in remote_h and 'commsPostRms' in remote_h
assert 'commsPrePeak=' in remote and 'commsPostRms=' in remote
remote_target = xmake[xmake.index('target("sds-remote-vo-speaker-playback-tests"'):]
assert 'src/core/CommsProcessor.cpp' in remote_target[:1200]

print('PASS v0.3.10 controller speaker loudness regression')
