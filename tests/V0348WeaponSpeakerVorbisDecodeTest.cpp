#include <StarfieldDualSense/WeaponSpeakerWemDecode.h>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace
{
    struct Bits{std::vector<unsigned char>b;std::size_t n{};void put(std::uint64_t v,unsigned c){for(unsigned i=0;i<c;++i){auto bi=n/8,sh=n%8;if(bi==b.size())b.push_back(0);b[bi]|=((v>>i)&1u)<<sh;++n;}}};
    void p16(std::vector<unsigned char>&b,std::uint16_t v){b.push_back(v&255);b.push_back((v>>8)&255);} void p32(std::vector<unsigned char>&b,std::uint32_t v){for(int i=0;i<4;++i)b.push_back((v>>(8*i))&255);} void patch32(std::vector<unsigned char>&b,std::size_t o,std::uint32_t v){for(int i=0;i<4;++i)b[o+i]=(v>>(8*i))&255;}
    void chunk(std::vector<unsigned char>&b,const char*id,const std::vector<unsigned char>&p){b.insert(b.end(),id,id+4);p32(b,(std::uint32_t)p.size());b.insert(b.end(),p.begin(),p.end());if(p.size()&1)b.push_back(0);}
    std::vector<unsigned char> codebooks(){Bits x;x.put(1,4);x.put(1,14);x.put(1,1);x.put(0,5);x.put(1,1);x.put(0,1);auto r=x.b;p32(r,0);p32(r,4);return r;}
    std::vector<unsigned char> setup(){Bits b;b.put(0,8);b.put(0,10);b.put(0,6);b.put(0,5);b.put(0,3);b.put(0,2);b.put(0,8);b.put(0,2);b.put(0,4);b.put(0,6);b.put(0,2);b.put(0,24);b.put(0,24);b.put(0,24);b.put(0,6);b.put(0,8);b.put(0,3);b.put(0,1);b.put(0,6);b.put(0,1);b.put(0,1);b.put(0,2);b.put(0,8);b.put(0,8);b.put(0,8);b.put(1,6);b.put(0,1);b.put(0,8);b.put(1,1);b.put(0,8);return b.b;}
    std::vector<unsigned char> wem(){
        auto st=setup();std::vector<unsigned char>d;p16(d,(std::uint16_t)st.size());d.insert(d.end(),st.begin(),st.end());auto audio=(std::uint32_t)d.size();p16(d,2);d.push_back(0b10101010);d.push_back(0x55);p16(d,2);d.push_back(0b11001101);d.push_back(0x33);p16(d,2);d.push_back(0b01110000);d.push_back(0x77);
        std::vector<unsigned char>b{'R','I','F','F',0,0,0,0,'W','A','V','E'},f;p16(f,0xFFFF);p16(f,1);p32(f,44100);p32(f,7606);p16(f,0);p16(f,0);p16(f,0x30);p16(f,0);p32(f,0x4101);p32(f,12);p32(f,0);p32(f,(std::uint32_t)d.size());p16(f,0);p16(f,0);p32(f,0);p32(f,audio);p16(f,2);p16(f,0);p32(f,0);p32(f,0);p32(f,0);f.push_back(8);f.push_back(11);assert(f.size()==0x42);chunk(b,"fmt ",f);chunk(b,"data",d);patch32(b,4,(std::uint32_t)b.size()-8);return b;
    }
    bool fake(std::span<const unsigned char> ogg,std::uint16_t&ch,std::uint32_t&rate,std::vector<std::int16_t>&pcm,std::string&e){if(ogg.size()<4||std::string((const char*)ogg.data(),4)!="OggS"){e="bad ogg";return false;}ch=1;rate=44100;pcm={0,1000,-1000,2000,-2000,3000,-3000,2000,-2000,1000,-1000,0};return true;}
}
int main(){auto cb=codebooks();auto r=sds::decodeWeaponSpeakerWemToSpeakerPcmWithVorbisBackend(wem(),0.35F,cb,fake);if(!r.prepared){std::cerr<<"FAIL: "<<r.error<<'\n';return 1;}if(!r.usedVorbis||r.sampleRate!=44100u||r.sourceFrames!=12u||r.pcm.frames.empty()){std::cerr<<"FAIL: Vorbis path metadata\n";return 1;}std::cout<<"PASS v0.3.48 weapon speaker Wwise-Vorbis decode\n";}
