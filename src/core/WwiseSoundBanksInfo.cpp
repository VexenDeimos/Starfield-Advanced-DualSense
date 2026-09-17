#include <StarfieldDualSense/WwiseSoundBanksInfo.h>
#include <charconv>
#include <cctype>
#include <map>
#include <optional>
#include <string>
#include <variant>

namespace {
struct J { using Obj=std::map<std::string,J>; using Arr=std::vector<J>; std::variant<std::nullptr_t,bool,std::uint64_t,std::string,Obj,Arr> v{nullptr}; };
class P {
public: explicit P(std::string_view s):s_(s){} std::optional<J> parse(std::string& e){auto x=val(0,e);ws();if(!x||i_!=s_.size()){if(e.empty())e="trailing JSON";return std::nullopt;}return x;}
private:
 std::string_view s_; std::size_t i_{}; void ws(){while(i_<s_.size()&&std::isspace((unsigned char)s_[i_]))++i_;}
 std::optional<std::string> str(std::string& e){ws();if(i_>=s_.size()||s_[i_]!='"'){e="expected string";return{};}++i_;std::string o;while(i_<s_.size()){char c=s_[i_++];if(c=='"')return o;if(c=='\\'){if(i_>=s_.size()){e="bad escape";return{};}char q=s_[i_++];switch(q){case'"':case'\\':case'/':o.push_back(q);break;case'b':o.push_back('\b');break;case'f':o.push_back('\f');break;case'n':o.push_back('\n');break;case'r':o.push_back('\r');break;case't':o.push_back('\t');break;case'u': if(i_+4>s_.size()){e="bad unicode";return{};} o.push_back('?'); i_+=4; break;default:e="bad escape";return{};}}else{o.push_back(c);}if(o.size()>1024*1024){e="string too long";return{};}}e="unterminated string";return{};}
 std::optional<J> val(int d,std::string& e){if(d>64){e="nesting too deep";return{};}ws();if(i_>=s_.size()){e="unexpected end";return{};}char c=s_[i_];if(c=='{')return obj(d,e);if(c=='[')return arr(d,e);if(c=='"'){auto x=str(e);if(!x)return{};J j;j.v=*x;return j;}if(std::isdigit((unsigned char)c)){std::size_t b=i_;while(i_<s_.size()&&std::isdigit((unsigned char)s_[i_]))++i_;std::uint64_t n{};auto r=std::from_chars(s_.data()+b,s_.data()+i_,n);if(r.ec!=std::errc{}){e="bad number";return{};}J j;j.v=n;return j;}if(s_.substr(i_,4)=="true"){i_+=4;J j;j.v=true;return j;}if(s_.substr(i_,5)=="false"){i_+=5;J j;j.v=false;return j;}if(s_.substr(i_,4)=="null"){i_+=4;return J{};}e="unexpected token";return{};}
 std::optional<J> obj(int d,std::string& e){++i_;J::Obj o;ws();if(i_<s_.size()&&s_[i_]=='}'){++i_;J j;j.v=std::move(o);return j;}while(true){auto k=str(e);if(!k)return{};ws();if(i_>=s_.size()||s_[i_++]!=':'){e="expected colon";return{};}auto x=val(d+1,e);if(!x)return{};o[*k]=std::move(*x);ws();if(i_>=s_.size()){e="unterminated object";return{};}char c=s_[i_++];if(c=='}')break;if(c!=','){e="expected comma";return{};}}J j;j.v=std::move(o);return j;}
 std::optional<J> arr(int d,std::string& e){++i_;J::Arr a;ws();if(i_<s_.size()&&s_[i_]==']'){++i_;J j;j.v=std::move(a);return j;}while(true){auto x=val(d+1,e);if(!x)return{};a.push_back(std::move(*x));ws();if(i_>=s_.size()){e="unterminated array";return{};}char c=s_[i_++];if(c==']')break;if(c!=','){e="expected comma";return{};}}J j;j.v=std::move(a);return j;}
};
const J* get(const J& j,std::string_view k){auto* o=std::get_if<J::Obj>(&j.v);if(!o)return nullptr;auto it=o->find(std::string(k));return it==o->end()?nullptr:&it->second;}
std::string text(const J* j){if(!j)return{};if(auto*s=std::get_if<std::string>(&j->v))return*s;if(auto*n=std::get_if<std::uint64_t>(&j->v))return std::to_string(*n);return{};}
std::optional<std::uint32_t> id(const J* j){auto s=text(j);if(s.empty())return{};std::uint64_t n{};auto r=std::from_chars(s.data(),s.data()+s.size(),n);if(r.ec!=std::errc{}||r.ptr!=s.data()+s.size()||n>0xffffffffull)return{};return(std::uint32_t)n;}
void walk(const J& j,sds::WwiseSoundBanksInfoIndex& out){
 if(auto* o=std::get_if<J::Obj>(&j.v)){
   if(auto it=o->find("StreamedFiles");it!=o->end())if(auto*a=std::get_if<J::Arr>(&it->second.v))for(auto&x:*a){auto mid=id(get(x,"Id"));if(!mid)continue;auto& m=out.mediaById[*mid];m.mediaId=*mid;auto sn=text(get(x,"ShortName"));auto p=text(get(x,"Path"));if(m.shortName.empty()&&!sn.empty())m.shortName=sn;if(m.originalPath.empty()&&!p.empty())m.originalPath=p;}
   if(auto it=o->find("SoundBanks");it!=o->end())if(auto*a=std::get_if<J::Arr>(&it->second.v))for(auto&b:*a){auto bn=text(get(b,"ShortName"));auto* ev=get(b,"IncludedEvents");if(auto*ea=ev?std::get_if<J::Arr>(&ev->v):nullptr)for(auto&e:*ea){auto eid=id(get(e,"Id"));if(!eid)continue;sds::WwiseEventMetadata m;m.eventId=*eid;m.eventName=text(get(e,"Name"));m.bankName=bn;auto*rf=get(e,"ReferencedStreamedFiles");if(auto*ra=rf?std::get_if<J::Arr>(&rf->v):nullptr)for(auto&r:*ra)if(auto rid=id(get(r,"Id")))m.referencedMediaIds.push_back(*rid);out.eventsById[*eid].push_back(std::move(m));}}
   for(auto& [k,v]:*o){(void)k;walk(v,out);}
 } else if(auto*a=std::get_if<J::Arr>(&j.v)) for(auto&x:*a)walk(x,out);
}
}
sds::WwiseSoundBanksInfoParseResult sds::parseWwiseSoundBanksInfo(std::string_view json,std::string_view sourceName){WwiseSoundBanksInfoParseResult r;std::string e;auto root=P(json).parse(e);if(!root){r.error=std::string(sourceName)+": "+e;return r;}walk(*root,r.index);r.ok=true;return r;}
