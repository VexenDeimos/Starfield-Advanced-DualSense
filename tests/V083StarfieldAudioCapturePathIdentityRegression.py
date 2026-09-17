from pathlib import Path

root = Path(__file__).resolve().parents[1]
cpp = (root / 'src/starfield/StarfieldAudioCapture.cpp').read_text(encoding='utf-8')
hdr = (root / 'include/StarfieldDualSense/StarfieldAudioCapture.h').read_text(encoding='utf-8')
plugin = (root / 'src/starfield/Plugin.cpp').read_text(encoding='utf-8')
xmake = (root / 'xmake.lua').read_text(encoding='utf-8')

checks = {
    'bounded path buffer': 'kMaxFilePathCharacters' in cpp,
    'record snapshots dialogue state': 'dialogueMenuActive' in cpp,
    'record owns copied path': 'filePath' in cpp and 'filePathLength' in cpp,
    'public dialogue state setter': 'setDialogueMenuActive(bool active)' in hdr,
    'runtime updates dialogue state': 'setDialogueMenuActive' in plugin and 'DialogueMenu' in plugin,
    'diagnostic formats dialogue state': 'dialogueMenu=' in cpp,
    'diagnostic formats file path': 'filePath=' in cpp,
    'still forbids loopback': 'AUDCLNT_STREAMFLAGS_LOOPBACK' not in cpp,
    'v083 version': '0.3.01-voice-archive-manifest-probe' in plugin and 'set_version("0.3.1")' in xmake,
}

failed = [name for name, ok in checks.items() if not ok]
for name, ok in checks.items():
    print(('PASS' if ok else 'FAIL') + ': ' + name)
raise SystemExit(1 if failed else 0)
