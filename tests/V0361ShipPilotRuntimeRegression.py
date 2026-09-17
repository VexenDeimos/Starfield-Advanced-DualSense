from pathlib import Path

root = Path(__file__).resolve().parents[1]

types_h = (root / "include/StarfieldDualSense/Types.h").read_text(encoding="utf-8")
adapter_h = (root / "include/StarfieldDualSense/GameStateAdapter.h").read_text(encoding="utf-8")
adapter_cpp = (root / "src/starfield/GameStateAdapter.cpp").read_text(encoding="utf-8")
plugin_cpp = (root / "src/starfield/Plugin.cpp").read_text(encoding="utf-8")
xmake = (root / "xmake.lua").read_text(encoding="utf-8")

for token in (
    "ShipPilotEntered",
    "ShipPilotExited",
    "ShipPilotInvalidated",
    "ShipPilotResumed",
):
    assert token in types_h

assert "ShipPilotContext" in adapter_h
assert '"SpaceshipHudMenu"' in adapter_cpp
assert '"LoadingMenu"' in adapter_cpp
assert "ShipPilotTransition::Entered" in adapter_cpp
assert "ShipPilotTransition::Exited" in adapter_cpp
assert "ShipPilotTransition::Invalidated" in adapter_cpp
assert "ShipPilotTransition::Resumed" in adapter_cpp
assert "ShipPilotTransition::ExitedAfterLoad" in adapter_cpp

# The old normalized menu stream remains present.
assert "GameEventType::MenuOpened" in adapter_cpp
assert "GameEventType::MenuClosed" in adapter_cpp

# v0.3.61 must not introduce guessed pilot authority.
for forbidden in (
    "GetFurnitureUsing",
    "currentFurniture",
    "occupiedFurniture",
    "ShipHasActorInPilotSeat",
    "GetActorInShipPilotSeat",
    "GetPilot(",
):
    assert forbidden not in adapter_cpp

# No placeholder ship presentation in the first slice.
for forbidden in (
    "ShipBoost",
    "ShipWeaponFired",
    "ShipShieldHit",
    "ShipHullHit",
    "ShipCollision",
    "ShipMissileWarning",
    "ShipGravJump",
):
    assert forbidden not in types_h

# Final v0.3.61 integration identity and clean-exit refresh path.
assert xmake.count('set_version("0.3.61")') >= 2 or xmake.count('set_version("0.3.62")') >= 2 or xmake.count('set_version("0.3.63")') >= 2 or xmake.count('set_version("0.3.64")') >= 2 or xmake.count('set_version("0.3.65")') >= 2 or xmake.count('set_version("0.3.66")') >= 2 or xmake.count('set_version("0.3.67")') >= 2 or xmake.count('set_version("0.3.68")') >= 2 or (xmake.count('set_version("0.3.69")') >= 2 or (xmake.count('set_version("0.3.70")') >= 2 or (xmake.count('set_version("0.3.71")') >= 2 or xmake.count('set_version("0.3.72")') >= 2)))
assert "0.3.61-ship-pilot-context-r2" in plugin_cpp or "0.3.62-propulsion-" in plugin_cpp or "0.3.63-ship-ballistic-haptics" in plugin_cpp or "0.3.64-ship-laser-recon" in plugin_cpp or ("0.3.65-ship-laser-haptics" in plugin_cpp or "0.3.65-r2-ship-laser-tactile-retune" in plugin_cpp) or ("0.3.66-ship-particle-recon" in plugin_cpp or "0.3.67-ship-particle-haptics" in plugin_cpp or "0.3.68-ship-missile-recon" in plugin_cpp or ("0.3.69-ship-missile-haptics" in plugin_cpp or ("0.3.70-ship-em-recon" in plugin_cpp or ("0.3.71-ship-em-haptics" in plugin_cpp or "0.3.72-ship-launch-landing-recon" in plugin_cpp))))
assert "g_onFootRefreshPending" in plugin_cpp
assert "g_shipPilotActive" in plugin_cpp
assert "refreshCurrentEquippedWeaponState" in plugin_cpp
assert "refreshPlayerHealth" in adapter_h
assert "refreshPlayerHealth" in adapter_cpp

for marker in (
    "Ship pilot context: ENTER source=SpaceshipHudMenu authority=menu",
    "Ship pilot context: EXIT source=SpaceshipHudMenu refresh=fresh-on-foot",
    "Ship pilot context: INVALIDATE source=LoadingMenu refresh=deferred-until-fresh-context",
    "Ship pilot context: RESUME source=LoadingMenu-close evidence=SpaceshipHudMenu-still-open",
    "Ship pilot context: EXIT source=LoadingMenu-close reason=SpaceshipHudMenu-closed-during-load refresh=fresh-on-foot",
    "Ship pilot context: ON-FOOT-REFRESH",
):
    assert marker in plugin_cpp

# Clean exit refresh runs from runtimeTick, not from the menu callback adapter.
assert "refreshCurrentEquippedWeaponState" not in adapter_cpp
assert "g_onFootRefreshPending.exchange" in plugin_cpp
assert 'IsMenuOpen(RE::BSFixedString("LoadingMenu"))' in plugin_cpp

# The first ship slice does not introduce the later subsystems yet.
for forbidden in (
    "ShipStateProvider.cpp",
    "ShipWeaponClassifier.cpp",
    "ShipEventBridge.cpp",
    "ShipEffectsEngine.cpp",
):
    assert forbidden not in xmake

# Frozen v0.3.60 UI startup/prewarm work remains wired.
assert "UiSpeakerPreparedCache" in xmake
assert "g_mainMenuUiAudioPipeline" in plugin_cpp
assert "Main-menu UI speaker prewarm: worker started" in plugin_cpp

print("PASS v0.3.61 ship pilot runtime integration regression retained through v0.3.71")
