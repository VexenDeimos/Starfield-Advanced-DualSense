#include <StarfieldDualSense/WwiseBa2ArchiveIndex.h>
#include <zlib.h>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {
void put16(std::vector<unsigned char>& b, std::uint16_t v){b.push_back(v&0xff);b.push_back((v>>8)&0xff);} 
void put32(std::vector<unsigned char>& b, std::uint32_t v){for(int i=0;i<4;++i)b.push_back((v>>(i*8))&0xff);} 
void put64(std::vector<unsigned char>& b, std::uint64_t v){for(int i=0;i<8;++i)b.push_back((v>>(i*8))&0xff);} 
void set32(std::vector<unsigned char>& b,std::size_t o,std::uint32_t v){for(int i=0;i<4;++i)b[o+i]=(v>>(i*8))&0xff;}
void set64(std::vector<unsigned char>& b,std::size_t o,std::uint64_t v){for(int i=0;i<8;++i)b[o+i]=(v>>(i*8))&0xff;}
std::filesystem::path makeArchive(){
    const std::vector<unsigned char> plain{1,2,3,4};
    const std::vector<unsigned char> raw{9,8,7,6,5,4,3,2,1};
    uLongf cap=compressBound(raw.size()); std::vector<unsigned char> packed(cap);
    assert(compress2(packed.data(),&cap,raw.data(),raw.size(),Z_BEST_SPEED)==Z_OK); packed.resize(cap);
    std::vector<unsigned char> b; b.insert(b.end(),{'B','T','D','X'}); put32(b,2); b.insert(b.end(),{'G','N','R','L'}); put32(b,2); put64(b,0); put64(b,0);
    const std::size_t rec0=b.size(); b.resize(b.size()+36,0); const std::size_t rec1=b.size(); b.resize(b.size()+36,0);
    const std::uint64_t off0=b.size(); b.insert(b.end(),plain.begin(),plain.end());
    const std::uint64_t off1=b.size(); b.insert(b.end(),packed.begin(),packed.end());
    const std::uint64_t names=b.size();
    const std::string n0="sound\\weapons\\maelstrom\\123.wem", n1="sound\\weapons\\maelstrom\\456.wem";
    put16(b,(std::uint16_t)n0.size()); b.insert(b.end(),n0.begin(),n0.end()); put16(b,(std::uint16_t)n1.size()); b.insert(b.end(),n1.begin(),n1.end());
    set64(b,16,names);
    auto writeRec=[&](std::size_t r,const char ext[4],std::uint64_t off,std::uint32_t ps,std::uint32_t us){
        set32(b,r+0,0x11111111u); for(int i=0;i<4;++i)b[r+4+i]=ext[i]; set32(b,r+8,0x22222222u); set32(b,r+12,0); set64(b,r+16,off); set32(b,r+24,ps); set32(b,r+28,us); set32(b,r+32,0xBAADF00Du);
    };
    writeRec(rec0,"wem\0",off0,0,plain.size()); writeRec(rec1,"wem\0",off1,packed.size(),raw.size());
    auto p=std::filesystem::temp_directory_path()/"sds_v0320_ba2_test.ba2"; std::ofstream(p,std::ios::binary).write((const char*)b.data(),b.size()); return p;
}
}
int main(){
    std::string err; auto p=makeArchive(); auto idx=sds::WwiseBa2ArchiveIndex::open(p,err); assert(idx); assert(idx->entries().size()==2);
    auto* e0=idx->findExact("SOUND/WEAPONS/MAELSTROM/123.WEM"); assert(e0); auto f=idx->findFilename("123.wem"); assert(f.size()==1 && f[0]->normalizedName=="sound\\weapons\\maelstrom\\123.wem"); assert(idx->findSuffix("maelstrom/123.wem").size()==1);
    auto a=idx->readPayload(*e0,1024); assert(a.ok && !a.compressed && a.bytes==std::vector<unsigned char>({1,2,3,4}));
    auto* e1=idx->findExact("sound\\weapons\\maelstrom\\456.wem"); assert(e1); auto z=idx->readPayload(*e1,1024); assert(z.ok && z.compressed && z.bytes==std::vector<unsigned char>({9,8,7,6,5,4,3,2,1}));
    auto small=idx->readPayload(*e1,2); assert(!small.ok);
    std::filesystem::remove(p);
}
