#include <StarfieldDualSense/ShipWeaponSemanticCatalog.h>
#include <StarfieldDualSense/WeaponAudioPipeline.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>

namespace
{
    int failures = 0;
    void check(bool condition, const char* name)
    {
        std::cout << (condition ? "PASS " : "FAIL ") << name << '\n';
        if (!condition) ++failures;
    }

    sds::ShipWeaponSemanticCatalog ballisticCatalog()
    {
        sds::WwiseSoundBanksInfoIndex index{};
        index.mediaById.emplace(1u, sds::WwiseMediaMetadata{
            1u,
            R"(WPN\Ship\BallisticTest\WPN_Ship_Ballistic_A_Test_Fire_PC_Auto_3rd_01.wav)",
            R"(SFX\WPN\Ship\BallisticTest\WPN_Ship_Ballistic_A_Test_Fire_PC_Auto_3rd_01.wem)" });
        index.eventsById[0xAABBCCDDu].push_back({
            0xAABBCCDDu, "Play_WPN_Ship_Ballistic_Fire", "Starfield_Weapons", { 1u } });
        return sds::buildShipWeaponSemanticCatalog(index);
    }

    class Backend final : public sds::IWeaponAudioPipelineBackend
    {
    public:
        sds::WeaponAudioStartupBatch resolveStartup() override
        {
            sds::WeaponAudioStartupBatch batch{};
            batch.weaponPreparationRequested = false;
            batch.shipWeaponCatalog = ballisticCatalog();
            return batch;
        }

        bool prepareVariant(const sds::WwisePcmWeaponVariantCandidate&,
            const sds::WeaponSpeakerPcmPreparation&,
            sds::PreparedWeaponSpeakerVariant&,
            std::string&) override
        {
            return false;
        }

        sds::MusicReconResolvedEvent resolveMusicRecon(
            const sds::MusicReconResolveRequest&) override
        {
            return {};
        }

        sds::WeaponAudioDiscoveryBatch resolveDiscovery(const sds::WeaponSfxDiscoveryReport&) override
        {
            return {};
        }
    };
}

int main()
{
    auto shipCache = std::make_shared<sds::ShipWeaponSemanticCache>();
    sds::WeaponAudioPipeline pipeline({}, std::make_unique<Backend>(), {}, shipCache);
    check(!shipCache->ready(), "ship weapon semantic cache is unready before worker startup");
    pipeline.start();

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!pipeline.stats().startupSettled && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    check(pipeline.stats().startupSettled, "audio worker settles after publishing ship semantics");
    check(shipCache->ready(), "audio worker publishes ship semantic cache");
    check(shipCache->isPlayerBallisticFire(0xAABBCCDDu),
        "published worker catalog authorizes known ballistic fire event");
    pipeline.stop();
    check(shipCache->ready(), "normal pipeline stop does not erase immutable startup semantics");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
