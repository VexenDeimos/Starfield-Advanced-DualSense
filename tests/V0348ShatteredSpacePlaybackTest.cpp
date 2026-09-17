#include <StarfieldDualSense/WeaponSpeakerPlayback.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace
{
    void require(bool c, std::string_view m){if(!c){std::cerr<<"FAIL: "<<m<<'\n';std::exit(1);}}

    sds::PreparedWeaponSpeakerVariant preparedVariant(const sds::WeaponSpeakerVariant& v, bool loop)
    {
        sds::PreparedWeaponSpeakerVariant p{};
        p.variant=v.variant;
        p.mediaId=v.pinnedMediaId ? v.pinnedMediaId : static_cast<std::uint32_t>(100000u+v.variant);
        p.pcm.frames={{0.1F,0.1F},{0.2F,0.2F},{0.3F,0.3F},{0.4F,0.4F}};
        p.pcm.gain=0.35F;
        p.loopResumeFrame=loop?1u:0u;
        return p;
    }

    sds::PreparedWeaponSpeakerFamily makeFamily(const sds::WeaponSpeakerProfile& profile)
    {
        sds::PreparedWeaponSpeakerFamily f{};f.familyIdentity=std::string(profile.weaponIdentity);
        for(const auto& c:profile.cues){sds::PreparedWeaponSpeakerCue pc{};pc.action=std::string(c.action);pc.eventId=c.mediaEventId;for(const auto&v:c.variants){pc.variants.push_back(preparedVariant(v,false));++f.preparedVariantCount;}f.cues.push_back(std::move(pc));}
        if(profile.sustained){const auto&s=*profile.sustained;sds::PreparedWeaponSpeakerSustainedCue ps{};ps.startWwiseEventId=s.startWwiseEventId;ps.stopWwiseEventId=s.stopWwiseEventId;ps.requiredGameObjectId=s.requiredGameObjectId;ps.requireZeroExternalSources=s.requireZeroExternalSources;for(const auto&v:s.loopVariants){ps.loopVariants.push_back(preparedVariant(v,true));++f.preparedVariantCount;}for(const auto&v:s.startTransientVariants){ps.startTransientVariants.push_back(preparedVariant(v,false));++f.preparedVariantCount;}for(const auto&v:s.stopTransientVariants){ps.stopTransientVariants.push_back(preparedVariant(v,false));++f.preparedVariantCount;}f.sustained=std::move(ps);}return f;
    }

    sds::GameEvent game(sds::GameEventType type,std::string_view text)
    {
        sds::GameEvent e{};e.type=type;std::copy_n(text.data(),std::min(text.size(),e.text.size()-1),e.text.data());return e;
    }

    struct Submitted{std::string weapon;std::string action;std::uint32_t media{};};
}

int main()
{
    auto cache=std::make_shared<sds::WeaponSpeakerPreparedCache>();
    const auto* pen=sds::findWeaponSpeakerProfile("Va'ruun Penumbra");const auto* star=sds::findWeaponSpeakerProfile("Va'ruun Starstorm");
    require(pen&&star,"profiles");require(cache->publish(makeFamily(*pen)),"publish Penumbra");require(cache->publish(makeFamily(*star)),"publish Starstorm");

    std::vector<Submitted> submitted;std::vector<sds::PersistentPreparedSpeakerPcm> persistent;std::vector<std::uint64_t> clears;
    sds::WeaponSpeakerPlayback playback(
        [&](const sds::PreparedSpeakerPcm&,std::string_view w,std::string_view a,std::uint32_t,std::uint32_t m,std::uint8_t){submitted.push_back({std::string(w),std::string(a),m});return true;},
        [&](sds::PersistentPreparedSpeakerPcm p,std::string_view,std::uint32_t,std::uint32_t,std::uint8_t){persistent.push_back(std::move(p));return true;},
        [&](std::uint64_t o,bool){clears.push_back(o);return true;},
        {},cache,false);

    require(playback.observeGameEvent(game(sds::GameEventType::WeaponEquipped,"Va'ruun Penumbra")),"arm Penumbra");
    require(playback.observeGameEvent(game(sds::GameEventType::WeaponFired,"WeaponFire")),"Penumbra fire accepted");
    require(submitted.size()==3u,"Penumbra submits three layers per shot");
    require(submitted[0].action=="fire-body"&&submitted[0].media==181682584u,"Penumbra body layer");
    require(submitted[1].action=="fire-low"&&submitted[1].media==272400732u,"Penumbra low layer");
    require(submitted[2].action=="fire-high"&&submitted[2].media==10032930u,"Penumbra high layer");
    require(playback.observeGameEvent(game(sds::GameEventType::WeaponFired,"WeaponFire")),"second Penumbra shot");
    require(submitted[3].media==284040293u&&submitted[4].media==570040591u&&submitted[5].media==471580376u,"Penumbra groups advance independently");

    submitted.clear();
    require(playback.observeGameEvent(game(sds::GameEventType::WeaponEquipped,"Va'ruun Starstorm")),"arm Starstorm");
    sds::WeaponSfxWwiseObservation start{};start.eventId=0xB6E1A82Eu;start.gameObjectId=0x2u;start.externalCount=0;start.hasExternalSources=false;
    require(playback.observeWwise(start),"Starstorm start");
    require(persistent.size()==1u,"Starstorm persistent voice starts");
    require(persistent[0].layers.size()==1u,"Starstorm has one persistent loop layer");
    require(submitted.size()==1u&&submitted[0].action=="sustained-start"&&submitted[0].media==148515386u,"Starstorm rotating start accent");

    sds::WeaponSfxWwiseObservation stop{};stop.eventId=0xA7514E2Du;stop.gameObjectId=0x2u;stop.externalCount=0;stop.hasExternalSources=false;
    require(playback.observeWwise(stop),"Starstorm stop");
    require(clears.size()==1u,"Starstorm persistent loop clears");
    require(submitted.size()==2u&&submitted[1].action=="sustained-stop"&&submitted[1].media==480391941u,"Starstorm immediate stop component");

    sds::WeaponSfxWwiseObservation power{};power.eventId=0xFB7756F7u;power.gameObjectId=0x2u;power.externalCount=0;power.hasExternalSources=false;
    require(playback.observeWwise(power),"Starstorm power-down");
    require(submitted.size()==3u&&submitted[2].action=="power-down"&&submitted[2].media==920864464u,"Starstorm power-down component");

    std::cout<<"PASS v0.3.48 Penumbra layered shot and Starstorm sustained playback\n";
}
