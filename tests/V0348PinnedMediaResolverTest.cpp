#include <StarfieldDualSense/WwiseEventMediaResolver.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
    void require(bool c, std::string_view m) { if (!c) { std::cerr << "FAIL: " << m << '\n'; std::exit(1); } }
    void p16(std::vector<unsigned char>& b, std::uint16_t v){b.push_back(v&255);b.push_back((v>>8)&255);}    
    void p32(std::vector<unsigned char>& b, std::uint32_t v){for(int i=0;i<4;++i)b.push_back((v>>(8*i))&255);}    
    void p64(std::vector<unsigned char>& b, std::uint64_t v){for(int i=0;i<8;++i)b.push_back((v>>(8*i))&255);}    
    void s32(std::vector<unsigned char>& b,std::size_t o,std::uint32_t v){for(int i=0;i<4;++i)b[o+i]=(v>>(8*i))&255;}    
    void s64(std::vector<unsigned char>& b,std::size_t o,std::uint64_t v){for(int i=0;i<8;++i)b[o+i]=(v>>(8*i))&255;}    
    std::vector<unsigned char> pcm(std::int16_t s){std::vector<unsigned char>b{'R','I','F','F'};p32(b,38);b.insert(b.end(),{'W','A','V','E','f','m','t',' '});p32(b,16);p16(b,1);p16(b,1);p32(b,48000);p32(b,96000);p16(b,2);p16(b,16);b.insert(b.end(),{'d','a','t','a'});p32(b,2);p16(b,(std::uint16_t)s);return b;}
    void ba2(const std::filesystem::path& path,const std::vector<std::pair<std::string,std::vector<unsigned char>>>& e){
        std::vector<unsigned char>b{'B','T','D','X'};p32(b,2);b.insert(b.end(),{'G','N','R','L'});p32(b,(std::uint32_t)e.size());p64(b,0);p64(b,0);
        std::vector<std::size_t> r;for(std::size_t i=0;i<e.size();++i){r.push_back(b.size());b.resize(b.size()+36);}
        for(std::size_t i=0;i<e.size();++i){auto off=b.size();b.insert(b.end(),e[i].second.begin(),e[i].second.end());auto ext=std::filesystem::path(e[i].first).extension().string();if(!ext.empty())ext.erase(0,1);s32(b,r[i],0x31u+i);for(std::size_t j=0;j<4;++j)b[r[i]+4+j]=j<ext.size()?ext[j]:0;s64(b,r[i]+16,off);s32(b,r[i]+28,(std::uint32_t)e[i].second.size());s32(b,r[i]+32,0xBAADF00D);}
        auto no=b.size();s64(b,16,no);for(auto&x:e){p16(b,(std::uint16_t)x.first.size());b.insert(b.end(),x.first.begin(),x.first.end());}
        std::ofstream(path,std::ios::binary).write((char*)b.data(),(std::streamsize)b.size());
    }
    std::vector<unsigned char> info(){
        const std::string j = R"({"SoundBanksInfo":{"StreamedFiles":[{"Id":"181682584","ShortName":"","Path":""},{"Id":"272400732","ShortName":"","Path":""},{"Id":"10032930","ShortName":"","Path":""},{"Id":"999999999","ShortName":"","Path":""}],"SoundBanks":[{"ShortName":"ShatteredSpace","IncludedEvents":[{"Id":"4184634111","Name":"SFBGS001_WPN_ParticleRocketLauncher_Semi_PC","ReferencedStreamedFiles":[{"Id":"181682584"},{"Id":"272400732"},{"Id":"10032930"},{"Id":"999999999"}]}]}]}})";
        return {j.begin(),j.end()};
    }
}
int main(){
    const auto root=std::filesystem::temp_directory_path()/"sds_v0348_media_pin";std::filesystem::remove_all(root);auto data=root/"Data";std::filesystem::create_directories(data);
    ba2(data/"ShatteredSpace - Main01.ba2",{{"soundbanksinfo.json",info()},{"181682584.wem",pcm(1)},{"272400732.wem",pcm(2)},{"10032930.wem",pcm(3)},{"999999999.wem",pcm(4)}});
    sds::WwiseEventMediaResolver resolver(data);auto prep=resolver.prepare(true);require(prep.ready,"resolver prepared");auto run=resolver.run(true);
    auto find=[&](std::string_view action){for(const auto&v:run.weaponVariants)if(v.weaponIdentity=="Va'ruun Penumbra"&&v.action==action&&v.variant==1u)return &v;return (const sds::WwisePcmWeaponVariantCandidate*)nullptr;};
    const auto* body=find("fire-body");const auto* low=find("fire-low");const auto* high=find("fire-high");
    require(body&&body->mediaId==181682584u,"body pinned by media id");require(low&&low->mediaId==272400732u,"low pinned by media id");require(high&&high->mediaId==10032930u,"high pinned by media id");
    for(const auto&v:run.weaponVariants)require(v.mediaId!=999999999u,"decoy media not promoted");
    std::filesystem::remove_all(root);std::cout<<"PASS v0.3.48 exact media-id resolver pinning\n";
}
