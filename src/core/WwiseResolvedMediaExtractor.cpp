#include <StarfieldDualSense/WwiseResolvedMediaExtractor.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>
namespace {
[[maybe_unused]] constexpr std::string_view kDiagnosticRelativePath = "Data/SFSE/Plugins/StarfieldDualSenseDiagnostics/MaelstromWwise";
std::string sanitize(std::string_view in){std::string s(in);auto dot=s.find_last_of('.');if(dot!=std::string::npos)s.resize(dot);std::string o;for(unsigned char c:s){char x=(std::isalnum(c)||c=='_'||c=='-'||c=='.')?(char)c:'_';if(x=='_'&&!o.empty()&&o.back()=='_')continue;o.push_back(x);}while(!o.empty()&&o.front()=='.')o.erase(o.begin());if(o.empty())o="media";if(o.size()>96)o.resize(96);return o;}
std::string unsafeReason(std::string_view s){
    if(s.empty())return "unsafe original name: empty";
    if(s.find('\0')!=std::string_view::npos)return "unsafe original name: embedded NUL";
    if(s.find(':')!=std::string_view::npos)return "unsafe original name: colon";
    std::size_t start=0;
    while(start<=s.size()){
        auto end=s.find_first_of("/\\",start);
        auto part=s.substr(start,end==std::string_view::npos?s.size()-start:end-start);
        if(part=="..")return "unsafe original name: traversal segment";
        if(end==std::string_view::npos)break;
        start=end+1;
    }
    return {};
}
std::string eventHex(std::uint32_t v){std::ostringstream o;o<<std::uppercase<<std::hex<<std::setw(8)<<std::setfill('0')<<v;return o.str();}
std::vector<unsigned char> readAll(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f)return{};auto n=f.tellg();if(n<0)return{};std::vector<unsigned char>b((std::size_t)n);f.seekg(0);if(!b.empty()&&!f.read((char*)b.data(),n))return{};return b;}
}
sds::WwiseResolvedMediaWriteResult sds::writeResolvedWemDiagnostic(const std::filesystem::path& root,std::string_view semantic,std::uint32_t eventId,std::uint32_t mediaId,std::string_view originalName,std::span<const unsigned char> bytes,std::size_t maxBytes){
    WwiseResolvedMediaWriteResult r;
    if(semantic!="fire"&&semantic!="reload"&&semantic!="draw"&&semantic!="holster"){r.status="rejected";r.error="invalid semantic";return r;}
    if(bytes.size()>maxBytes){r.status="rejected";r.error="payload exceeds maxBytes";return r;}
    if(auto reason=unsafeReason(originalName);!reason.empty()){r.status="rejected";r.error=std::move(reason);return r;}
    std::error_code ec;
    std::filesystem::create_directories(root,ec);
    if(ec){r.status="error";r.error=ec.message();return r;}
    auto base=std::filesystem::weakly_canonical(root,ec);
    if(ec){r.status="error";r.error=ec.message();return r;}
    auto dir=base/std::string(semantic)/eventHex(eventId);
    std::filesystem::create_directories(dir,ec);
    if(ec){r.status="error";r.error=ec.message();return r;}
    auto name=std::to_string(mediaId)+"__"+sanitize(originalName)+".wem";
    auto target=dir/name;
    auto canonParent=std::filesystem::weakly_canonical(target.parent_path(),ec);
    if(ec||canonParent.string().rfind(base.string(),0)!=0){r.status="rejected";r.error="target escapes diagnostic root";return r;}
    if(std::filesystem::exists(target)){
        auto old=readAll(target);
        if(old.size()==bytes.size()&&std::equal(old.begin(),old.end(),bytes.begin())){r.path=target;r.status="exists-same";return r;}
        for(unsigned i=1;i<10000;++i){auto d=dir/(std::to_string(mediaId)+"__"+sanitize(originalName)+"__dup"+std::to_string(i)+".wem");if(!std::filesystem::exists(d)){target=d;r.status="written-duplicate";break;}}
    }
    if (r.status.empty()) {
        r.status = "written";
    }
    std::ofstream f(target,std::ios::binary|std::ios::trunc);
    if(!f){r.status="error";r.error="open output failed";return r;}
    if(!bytes.empty())f.write((const char*)bytes.data(),(std::streamsize)bytes.size());
    if(!f){r.status="error";r.error="write failed";return r;}
    r.path=target;
    return r;
}
