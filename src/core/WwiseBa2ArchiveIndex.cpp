#include <StarfieldDualSense/WwiseBa2ArchiveIndex.h>
#include <zlib.h>
#include <algorithm>
#include <cctype>
#include <fstream>

namespace {
std::uint16_t u16(const unsigned char* p){return std::uint16_t(p[0]) | (std::uint16_t(p[1])<<8);}
std::uint32_t u32(const unsigned char* p){return std::uint32_t(p[0]) | (std::uint32_t(p[1])<<8) | (std::uint32_t(p[2])<<16) | (std::uint32_t(p[3])<<24);}
std::uint64_t u64(const unsigned char* p){std::uint64_t v=0;for(int i=7;i>=0;--i)v=(v<<8)|p[i];return v;}
bool readAt(std::ifstream& f,std::uint64_t off,unsigned char* out,std::size_t n){f.clear();f.seekg((std::streamoff)off,std::ios::beg);return !!f.read((char*)out,(std::streamsize)n);}
std::string basenameNorm(std::string_view s){auto n=sds::normalizeWwiseArchivePath(s);auto p=n.find_last_of('\\');return p==std::string::npos?n:n.substr(p+1);}
}
std::string sds::normalizeWwiseArchivePath(std::string_view value){
    std::string out; out.reserve(value.size());
    for(unsigned char c: value){char x=(char)c;if(x=='/')x='\\';out.push_back((char)std::tolower((unsigned char)x));}
    while(!out.empty() && (out.front()=='\\' || out.front()=='.')) { if(out.front()=='.' && out.size()>1 && out[1]!='\\') break; out.erase(out.begin()); }
    return out;
}
std::optional<sds::WwiseBa2ArchiveIndex> sds::WwiseBa2ArchiveIndex::open(const std::filesystem::path& path,std::string& error){
    error.clear(); std::ifstream f(path,std::ios::binary|std::ios::ate); if(!f){error="open failed";return std::nullopt;}
    auto end=f.tellg(); if(end<32){error="truncated header";return std::nullopt;} const std::uint64_t fs=(std::uint64_t)end;
    unsigned char h[32]{}; if(!readAt(f,0,h,sizeof h)){error="header read failed";return std::nullopt;}
    if(std::string((char*)h,4)!="BTDX"){error="bad magic";return std::nullopt;} if(u32(h+4)!=2){error="unsupported version";return std::nullopt;} if(std::string((char*)h+8,4)!="GNRL"){error="unsupported type";return std::nullopt;}
    const auto count=u32(h+12); const auto names=u64(h+16); constexpr std::uint64_t recBytes=36;
    if(count>10000000u){error="file count unreasonable";return std::nullopt;} if(32ull + recBytes*count > fs){error="truncated records";return std::nullopt;} if(names>fs){error="name table overrun";return std::nullopt;}
    WwiseBa2ArchiveIndex idx; idx.path_=path; idx.fileSize_=fs; idx.entries_.reserve(count);
    std::vector<unsigned char> rec(36); std::uint64_t np=names;
    for(std::uint32_t i=0;i<count;++i){
        if(!readAt(f,32ull+recBytes*i,rec.data(),rec.size())){error="record read failed";return std::nullopt;}
        unsigned char lenb[2]{}; if(np+2>fs || !readAt(f,np,lenb,2)){error="name table overrun";return std::nullopt;} np+=2; auto len=u16(lenb); if(np+len>fs){error="name table overrun";return std::nullopt;}
        std::string name(len,'\0'); if(len && !readAt(f,np,(unsigned char*)name.data(),len)){error="name read failed";return std::nullopt;} np+=len;
        WwiseBa2Entry e; e.name=name; e.normalizedName=normalizeWwiseArchivePath(name); e.nameHash=u32(rec.data()); e.extension=std::string((char*)rec.data()+4,4); while(!e.extension.empty() && e.extension.back()=='\0')e.extension.pop_back(); e.directoryHash=u32(rec.data()+8); e.flags=u32(rec.data()+12); e.dataOffset=u64(rec.data()+16); e.packedSize=u32(rec.data()+24); e.unpackedSize=u32(rec.data()+28); e.padding=u32(rec.data()+32);
        const std::uint64_t stored=e.packedSize?e.packedSize:e.unpackedSize; if(e.dataOffset>fs || stored>fs-e.dataOffset){error="payload range invalid";return std::nullopt;}
        idx.exact_.emplace(e.normalizedName,idx.entries_.size()); idx.entries_.push_back(std::move(e));
    }
    return idx;
}
const sds::WwiseBa2Entry* sds::WwiseBa2ArchiveIndex::findExact(std::string_view name) const noexcept {auto n=normalizeWwiseArchivePath(name);auto it=exact_.find(n);return it==exact_.end()?nullptr:&entries_[it->second];}
std::vector<const sds::WwiseBa2Entry*> sds::WwiseBa2ArchiveIndex::findFilename(std::string_view filename) const {std::vector<const WwiseBa2Entry*> out; auto want=basenameNorm(filename); for(auto& e:entries_)if(basenameNorm(e.normalizedName)==want)out.push_back(&e);return out;}
std::vector<const sds::WwiseBa2Entry*> sds::WwiseBa2ArchiveIndex::findSuffix(std::string_view suffix) const {std::vector<const WwiseBa2Entry*> out;auto s=normalizeWwiseArchivePath(suffix);for(auto& e:entries_)if(e.normalizedName.size()>=s.size() && e.normalizedName.compare(e.normalizedName.size()-s.size(),s.size(),s)==0)out.push_back(&e);return out;}
sds::WwiseBa2PayloadResult sds::WwiseBa2ArchiveIndex::readPayload(const WwiseBa2Entry& entry,std::size_t maxBytes) const {
    WwiseBa2PayloadResult r; r.compressed=entry.packedSize!=0; if(entry.unpackedSize>maxBytes){r.error="payload exceeds maxBytes";return r;} std::ifstream f(path_,std::ios::binary); if(!f){r.error="open failed";return r;}
    if(!r.compressed){r.bytes.resize(entry.unpackedSize);if(!readAt(f,entry.dataOffset,r.bytes.data(),r.bytes.size())){r.bytes.clear();r.error="payload read failed";return r;}r.ok=true;return r;}
    std::vector<unsigned char> packed(entry.packedSize); if(!readAt(f,entry.dataOffset,packed.data(),packed.size())){r.error="packed read failed";return r;} r.bytes.resize(entry.unpackedSize); uLongf out=(uLongf)r.bytes.size(); int z=uncompress(r.bytes.data(),&out,packed.data(),(uLong)packed.size()); if(z!=Z_OK || out!=entry.unpackedSize){r.bytes.clear();r.error="zlib decompress failed";return r;} r.ok=true; return r;
}
