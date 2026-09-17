#include <StarfieldDualSense/WeaponAudioPipeline.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

namespace
{
    void require(bool c, std::string_view m){if(!c){std::cerr<<"FAIL: "<<m<<'\n';std::exit(1);}}
    void p16(std::vector<unsigned char>&b,std::uint16_t v){b.push_back(v&255);b.push_back((v>>8)&255);} void p32(std::vector<unsigned char>&b,std::uint32_t v){for(int i=0;i<4;++i)b.push_back((v>>(8*i))&255);} void patch32(std::vector<unsigned char>&b,std::size_t o,std::uint32_t v){for(int i=0;i<4;++i)b[o+i]=(v>>(8*i))&255;}
    void chunk(std::vector<unsigned char>&b,const char*id,const std::vector<unsigned char>&p){b.insert(b.end(),id,id+4);p32(b,(std::uint32_t)p.size());b.insert(b.end(),p.begin(),p.end());if(p.size()&1)b.push_back(0);}
    std::vector<unsigned char> wem(){
        std::vector<unsigned char>b{'R','I','F','F',0,0,0,0,'W','A','V','E'},fmt; p16(fmt,0xFFFE);p16(fmt,1);p32(fmt,48000);p32(fmt,96000);p16(fmt,2);p16(fmt,16);p16(fmt,6);fmt.insert(fmt.end(),{0,0,1,0x41,0,0});chunk(b,"fmt ",fmt);
        std::vector<unsigned char>smpl;for(int i=0;i<7;++i)p32(smpl,0);p32(smpl,1);p32(smpl,0);p32(smpl,0);p32(smpl,0);p32(smpl,100);p32(smpl,399);p32(smpl,0);p32(smpl,0);chunk(b,"smpl",smpl);
        std::vector<unsigned char>data;for(int i=0;i<1000;++i)p16(data,(std::uint16_t)((i%100)*200));chunk(b,"data",data);patch32(b,4,(std::uint32_t)b.size()-8);return b;
    }
}
int main(){
    auto backend=sds::makeRealWeaponAudioPipelineBackend(std::filesystem::temp_directory_path());
    sds::WwisePcmWeaponVariantCandidate c{};c.weaponIdentity="Va'ruun Starstorm";c.action="sustained-loop";c.eventId=0xB6E1A82E;c.variant=1;c.mediaId=963157375;c.wemPayload=wem();
    sds::PreparedWeaponSpeakerVariant out{};std::string diagnostic;
    const sds::WeaponSpeakerPcmPreparation prep{0.35F,0u,0u,10u,true};
    require(backend->prepareVariant(c,prep,out,diagnostic),"backend prepares authored loop");
    require(out.pcm.frames.size()==400u,"authored loop trims post-loop tail");
    require(out.loopResumeFrame==110u,"authored loop resumes after crossfaded head");
    require(diagnostic.find("authoredLoop=yes")!=std::string::npos,"diagnostic marks authored loop");
    std::cout<<"PASS v0.3.48 authored loop backend\n";
}
