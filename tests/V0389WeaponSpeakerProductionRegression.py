from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

plugin = (ROOT / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
backend = (ROOT / "src/core/WeaponAudioPipelineBackend.cpp").read_text(encoding="utf-8")
resolver = (ROOT / "src/core/WwiseEventMediaResolver.cpp").read_text(encoding="utf-8")
header = (ROOT / "include/StarfieldDualSense/WwiseEventMediaResolver.h").read_text(encoding="utf-8")
extractor = (ROOT / "src/core/WwiseResolvedMediaExtractor.cpp").read_text(encoding="utf-8")

assert "g_weaponAudioPipelineEnabled = config.debugLogging;" not in plugin
assert (
    "g_weaponAudioPipelineEnabled =\n"
    "            config.operatingMode == sds::OperatingMode::Full;"
) in plugin

assert "resolver_.prepare(false);" in backend
assert "resolver_.run(false);" in backend
assert "resolver_.prepare(true);" not in backend
assert "resolver_.run(true);" not in backend

assert "run.diagnosticExtractionEnabled = debugLogging;" in resolver
assert 'mediaRecord.extraction.status = "disabled";' in resolver
assert (
    "if (debugLogging) {\n"
    "                mediaRecord.extraction = writeResolvedWemDiagnostic("
) in resolver

assert "bool diagnosticExtractionEnabled{ false };" in header

assert '"v0.3.21" / "MaelstromWwise"' not in resolver
assert '"v0.3.47" / "ShatteredSpaceWemCapture"' not in resolver
assert 'capturePathComponent(diagnosticVersion) / "UiWemCandidates"' not in resolver

assert "StarfieldDualSenseDiagnostics/v0.3.21/MaelstromWwise" not in extractor
assert (
    "StarfieldDualSenseDiagnostics/v0.3.47/ShatteredSpaceWemCapture"
    not in plugin
)

print("PASS v0.3.89 weapon speaker production pipeline is independent of DebugLogging")
print("PASS production resolver performs no diagnostic WEM extraction")
print("PASS diagnostic WEM directories are versionless")