from pathlib import Path

root = Path(__file__).resolve().parents[1]
plugin = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
catalog = (root / "include/StarfieldDualSense/UiAudioCandidateCatalog.h").read_text(encoding="utf-8")
pipeline = (root / "include/StarfieldDualSense/WeaponAudioPipeline.h").read_text(encoding="utf-8")
backend = (root / "src/core/WeaponAudioPipelineBackend.cpp").read_text(encoding="utf-8")

assert '0.3.60-main-menu-controller-speaker-startup' in plugin or '0.3.61-ship-pilot-context' in plugin or '0.3.62-propulsion-' in plugin or '0.3.63-ship-ballistic-haptics' in plugin or '0.3.64-ship-laser-recon' in plugin or ('0.3.65-ship-laser-haptics' in plugin or '0.3.65-r2-ship-laser-tactile-retune' in plugin) or ('0.3.66-ship-particle-recon' in plugin or '0.3.67-ship-particle-haptics' in plugin or '0.3.68-ship-missile-recon' in plugin or ('0.3.69-ship-missile-haptics' in plugin or ('0.3.70-ship-em-recon' in plugin or ('0.3.71-ship-em-haptics' in plugin or '0.3.72-ship-launch-landing-recon' in plugin))))
assert 'mainMenuUiSpeakerCueDefinitions' in catalog
assert 'prepareMainMenuUiOnly' in pipeline
assert 'options_.prepareMainMenuUiOnly' in backend
assert 'g_mainMenuUiAudioPipeline' in plugin
assert 'startEarlyMainMenuUiPreparation' in plugin

load_start = plugin.index('SFSE_PLUGIN_LOAD')
init_at = plugin.index('SFSE::Init(sfse', load_start)
early_at = plugin.index('startEarlyMainMenuUiPreparation();', init_at)
listener_at = plugin.index('RegisterListener(onSfseMessage)', early_at)
assert init_at < early_at < listener_at

start_fn = plugin.index('void startEarlyMainMenuUiPreparation()')
start_end = plugin.index('\n    void ', start_fn + 10)
start_body = plugin[start_fn:start_end]
assert 'speakerCategoryEnabled(config, sds::SpeakerCategory::ScannerUI)' in start_body
assert '.prepareUi = true' in start_body
assert '.prepareMainMenuUiOnly = true' in start_body
assert '.prepareWeapons = false' in start_body
assert '.resolveUiDiagnostics = false' in start_body
assert 'g_uiSpeakerPreparedCache = std::make_shared<sds::UiSpeakerPreparedCache>()' in start_body
assert 'g_mainMenuUiAudioPipeline->start();' in start_body

runtime_cache = plugin.index('if (uiSpeakerPlaybackEnabled)')
runtime_window = plugin[runtime_cache:runtime_cache + 1400]
assert 'if (!g_uiSpeakerPreparedCache)' in runtime_window

shutdown = plugin.index('void shutdownRuntime()')
shutdown_window = plugin[shutdown:shutdown + 5000]
assert 'g_mainMenuUiAudioPipeline->stop();' in shutdown_window

assert 'inline constexpr std::array<UiSpeakerCueDefinition, 10> kUiSpeakerCueDefinitions' in catalog
assert 'UIMenuMonocleOpen' in catalog and 'UIMenuMonocleClose' in catalog

print('PASS v0.3.60 main-menu generic controller-speaker startup wiring retained through v0.3.71')
