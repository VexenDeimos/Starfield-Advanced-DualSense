#include <StarfieldDualSense/WwiseBankHircResolver.h>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
namespace {
std::uint32_t u32(const unsigned char*p){return std::uint32_t(p[0])|(std::uint32_t(p[1])<<8)|(std::uint32_t(p[2])<<16)|(std::uint32_t(p[3])<<24);}
bool cc(std::span<const unsigned char>s,std::size_t o,const char*x){return o+4<=s.size()&&std::equal(x,x+4,s.begin()+o);}
struct Obj{std::uint8_t t{};std::uint32_t id{};std::span<const unsigned char>b;};
bool vlq(std::span<const unsigned char>b,std::size_t&pos,std::uint32_t&v){v=0;std::uint32_t shift=0;for(int i=0;i<5&&pos<b.size();++i){auto x=b[pos++];v|=(std::uint32_t(x&0x7f)<<shift);if((x&0x80)==0)return true;shift+=7;}return false;}
}
sds::WwiseHircResolveResult sds::resolveWwiseBankEventMedia(std::span<const unsigned char> bank,std::uint32_t eventId,std::size_t maxDepth){
 WwiseHircResolveResult r; std::span<const unsigned char> hirc; std::size_t p=0; bool bk=false;
 while(p+8<=bank.size()){auto sz=u32(bank.data()+p+4);if(sz>bank.size()-(p+8)){r.error="section exceeds bank";return r;}auto payload=bank.subspan(p+8,sz);if(cc(bank,p,"BKHD")){if(sz<4){r.error="BKHD too short";return r;}r.bankVersion=u32(payload.data());bk=true;}else if(cc(bank,p,"HIRC"))hirc=payload;p+=8+sz;}
 if(p!=bank.size()){r.error="trailing/truncated bank bytes";return r;}if(!bk||hirc.empty()){r.error="missing BKHD/HIRC";return r;}r.bankValid=true;if(r.bankVersion!=140){r.error="unsupported bank version";return r;}if(hirc.size()<4){r.bankValid=false;r.error="HIRC too short";return r;}
 std::unordered_map<std::uint32_t,Obj> objs; std::unordered_set<std::uint32_t> ambiguous; auto count=u32(hirc.data());std::size_t q=4;
 for(std::uint32_t i=0;i<count;++i){if(q+9>hirc.size()){r.bankValid=false;r.error="truncated HIRC object";return r;}auto t=hirc[q];auto len=u32(hirc.data()+q+1);if(len<4||q+5ull+len>hirc.size()){r.bankValid=false;r.error="bad HIRC object length";return r;}auto id=u32(hirc.data()+q+5);Obj o{t,id,hirc.subspan(q+9,len-4)};if(objs.contains(id))ambiguous.insert(id);else objs.emplace(id,o);q+=5+len;}
 if(q!=hirc.size()){r.bankValid=false;r.error="HIRC trailing bytes";return r;}auto eit=objs.find(eventId);if(eit==objs.end()||eit->second.t!=4)return r;r.eventFound=true;if(ambiguous.contains(eventId)){r.unsupported.push_back({eventId,4,"ambiguous-id"});return r;}
 std::size_t ep=0;std::uint32_t ac{};if(!vlq(eit->second.b,ep,ac)||ep+4ull*ac>eit->second.b.size()){r.error="invalid Event action list";return r;}for(std::uint32_t i=0;i<ac;++i){auto id=u32(eit->second.b.data()+ep);ep+=4;r.actionIds.push_back(id);}std::unordered_set<std::uint32_t> mediaSeen,visiting;
 auto walk=[&](auto&& self,std::uint32_t id,std::size_t depth,std::string rel)->void{
   auto it=objs.find(id);if(it==objs.end()){r.unsupported.push_back({id,0,"missing-target:"+rel});return;}const auto&o=it->second;r.traversed.push_back({id,o.t,rel});if(depth>maxDepth){r.unsupported.push_back({id,o.t,"max-depth"});return;}if(!visiting.insert(id).second){r.unsupported.push_back({id,o.t,"cycle"});return;}auto done=[&]{visiting.erase(id);};
   if(o.t==3){if(o.b.size()<6){r.unsupported.push_back({id,o.t,"action-too-short"});done();return;}auto at=o.b[1];auto target=u32(o.b.data()+2);if(at==4||at==0x12||at==0x1A)self(self,target,depth+1,"action-target");else r.unsupported.push_back({id,o.t,"non-play-action"});done();return;}
   if(o.t==2){if(o.b.size()<16){r.unsupported.push_back({id,o.t,"sound-too-short"});done();return;}auto mid=u32(o.b.data()+8);if(mid&&mediaSeen.insert(mid).second)r.mediaIds.push_back(mid);done();return;}
   if(o.t==5||o.t==6||o.t==7||o.t==9){if(o.b.size()<4){r.unsupported.push_back({id,o.t,"unsupported-boundary"});done();return;}auto n=u32(o.b.data());if(4ull+4ull*n!=o.b.size()||n>4096){r.unsupported.push_back({id,o.t,"unsupported-boundary"});done();return;}bool valid=true;for(std::uint32_t k=0;k<n;++k){auto cid=u32(o.b.data()+4+4*k);auto ci=objs.find(cid);if(ci==objs.end()||!(ci->second.t==2||ci->second.t==5||ci->second.t==6||ci->second.t==7||ci->second.t==9)){valid=false;break;}}if(!valid){r.unsupported.push_back({id,o.t,"unsupported-boundary"});done();return;}for(std::uint32_t k=0;k<n;++k)self(self,u32(o.b.data()+4+4*k),depth+1,"container-child");done();return;}
   r.unsupported.push_back({id,o.t,"unsupported-type"});done();
 };
 for(auto aid:r.actionIds) { walk(walk,aid,0,"event-action"); }
 return r;
}
