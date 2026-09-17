#include <StarfieldDualSense/ShipWeaponSemanticCatalog.h>

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
    int failures = 0;

    void check(bool condition, const char* name)
    {
        if (condition) {
            std::cout << "PASS " << name << '\n';
        } else {
            std::cout << "FAIL " << name << '\n';
            ++failures;
        }
    }

    sds::WwiseSoundBanksInfoIndex fixture()
    {
        sds::WwiseSoundBanksInfoIndex index{};
        index.mediaById.emplace(1001u, sds::WwiseMediaMetadata{
            1001u,
            R"(WPN\Ship\BallisticLightScythe\WPN_Ship_Ballistic_C_LightScythe_Fire_PC_Semi_3rd_01.wav)",
            R"(SFX\WPN\Ship\BallisticLightScythe\WPN_Ship_Ballistic_C_LightScythe_Fire_PC_Semi_3rd_01.wem)" });
        index.mediaById.emplace(1002u, sds::WwiseMediaMetadata{
            1002u,
            R"(WPN\Ship\BallisticLightScythe\WPN_Ship_Ballistic_C_LightScythe_Fire_PC_Semi_3rd_02.wav)",
            R"(SFX\WPN\Ship\BallisticLightScythe\WPN_Ship_Ballistic_C_LightScythe_Fire_PC_Semi_3rd_02.wem)" });
        index.mediaById.emplace(2001u, sds::WwiseMediaMetadata{
            2001u,
            R"(WPN\Ship\LaserLightScythe\WPN_Ship_Laser_C_LightScythe_Fire_PC_Semi_3rd_03.wav)",
            R"(SFX\WPN\Ship\LaserLightScythe\WPN_Ship_Laser_C_LightScythe_Fire_PC_Semi_3rd_03.wem)" });
        index.mediaById.emplace(3001u, sds::WwiseMediaMetadata{
            3001u,
            R"(WPN\Ship\BallisticLightScythe\WPN_Ship_Ballistic_C_LightScythe_Reload_PC_3rd_01.wav)",
            R"(SFX\WPN\Ship\BallisticLightScythe\WPN_Ship_Ballistic_C_LightScythe_Reload_PC_3rd_01.wem)" });
        index.mediaById.emplace(4001u, sds::WwiseMediaMetadata{
            4001u,
            R"(WPN\Ship\BallisticLightScythe\WPN_Ship_Ballistic_C_LightScythe_Fire_NPC_Semi_3rd_01.wav)",
            R"(SFX\WPN\Ship\BallisticLightScythe\WPN_Ship_Ballistic_C_LightScythe_Fire_NPC_Semi_3rd_01.wem)" });

        index.eventsById[0x11111111u].push_back({ 0x11111111u, "Play_WPN_Ship_Ballistic_Fire", "Starfield_Weapons", { 1001u, 1002u } });
        index.eventsById[0x22222222u].push_back({ 0x22222222u, "Play_WPN_Ship_Laser_Fire", "Starfield_Weapons", { 2001u } });
        index.eventsById[0x33333333u].push_back({ 0x33333333u, "Play_WPN_Ship_Ballistic_Reload", "Starfield_Weapons", { 3001u } });
        index.eventsById[0x44444444u].push_back({ 0x44444444u, "Play_WPN_Ship_Ballistic_Fire", "Starfield_Weapons", { 4001u } });
        index.eventsById[0x55555555u].push_back({ 0x55555555u, "Play_WPN_Ship_Mixed_Fire", "Starfield_Weapons", { 1001u, 2001u } });
        return index;
    }
}

int main()
{
    const auto index = fixture();
    const auto catalog = sds::buildShipWeaponSemanticCatalog(index);

    const auto ballistic = catalog.find(0x11111111u);
    check(ballistic.has_value(), "explicit player ship ballistic fire event is cataloged");
    check(ballistic && ballistic->family == sds::ShipWeaponFamily::Ballistic,
        "cataloged ballistic fire keeps ballistic family identity");
    check(ballistic && ballistic->action == sds::ShipWeaponAction::Fire,
        "cataloged ballistic event is a fire action");
    check(ballistic && ballistic->playerVariant,
        "cataloged ballistic fire requires player-specific media evidence");

    check(!catalog.find(0x22222222u).has_value(),
        "laser ship fire remains inert in ballistic-family slice");
    check(!catalog.find(0x33333333u).has_value(),
        "ballistic reload/mechanical audio cannot fabricate a shot");
    check(!catalog.find(0x44444444u).has_value(),
        "NPC-only ballistic fire cannot authorize player recoil");
    check(!catalog.find(0x55555555u).has_value(),
        "mixed-family Wwise ambiguity fails closed");
    check(!catalog.find(0xDEADBEEFu).has_value(),
        "unknown Wwise event fails closed");

    sds::ShipWeaponSemanticCache cache;
    check(!cache.ready(), "runtime semantic cache starts unready");
    check(!cache.isPlayerBallisticFire(0x11111111u),
        "unready semantic cache cannot authorize ballistic fire");
    cache.publish(catalog);
    check(cache.ready(), "published semantic cache becomes ready");
    check(cache.isPlayerBallisticFire(0x11111111u),
        "published cache authorizes known player ballistic fire");
    check(!cache.isPlayerBallisticFire(0x22222222u),
        "published cache keeps laser fire inert");
    cache.clear();
    check(!cache.ready() && !cache.isPlayerBallisticFire(0x11111111u),
        "cache clear revokes all ship-fire authority");

    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
