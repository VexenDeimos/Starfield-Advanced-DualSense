#include <StarfieldDualSense/WwiseBankHircResolver.h>
#include <cassert>
#include <cstdint>
#include <vector>
namespace{
void p32(std::vector<unsigned char>&b,std::uint32_t v){for(int i=0;i<4;++i)b.push_back((v>>(i*8))&0xff);}void sec(std::vector<unsigned char>&b,const char id[5],const std::vector<unsigned char>&p){b.insert(b.end(),id,id+4);p32(b,p.size());b.insert(b.end(),p.begin(),p.end());}
void obj(std::vector<unsigned char>&h,std::uint8_t t,std::uint32_t id,const std::vector<unsigned char>&body){h.push_back(t);p32(h,body.size()+4);p32(h,id);h.insert(h.end(),body.begin(),body.end());}
std::vector<unsigned char> bank(bool container=false){std::vector<unsigned char>b,bk; p32(bk,140);sec(b,"BKHD",bk); std::vector<unsigned char> h; p32(h,container?5:3);
 std::vector<unsigned char> ev{1};p32(ev,100);obj(h,4,10,ev);
 std::vector<unsigned char> act{0,4};p32(act,container?200:300);obj(h,3,100,act);
 if(container){std::vector<unsigned char> c;p32(c,2);p32(c,300);p32(c,301);obj(h,5,200,c);std::vector<unsigned char>s1;p32(s1,0);p32(s1,0);p32(s1,1111);p32(s1,0);obj(h,2,300,s1);std::vector<unsigned char>s2;p32(s2,0);p32(s2,0);p32(s2,2222);p32(s2,0);obj(h,2,301,s2);} else {std::vector<unsigned char>s;p32(s,0);p32(s,0);p32(s,1111);p32(s,0);obj(h,2,300,s);} sec(b,"HIRC",h);return b;}
}
int main(){auto r=sds::resolveWwiseBankEventMedia(bank(false),10);assert(r.bankValid&&r.bankVersion==140&&r.eventFound);assert(r.actionIds.size()==1&&r.actionIds[0]==100);assert(r.mediaIds==std::vector<std::uint32_t>({1111}));auto c=sds::resolveWwiseBankEventMedia(bank(true),10);assert(c.mediaIds.size()==2&&c.mediaIds[0]==1111&&c.mediaIds[1]==2222);auto miss=sds::resolveWwiseBankEventMedia(bank(false),999);assert(miss.bankValid&&!miss.eventFound);auto bad=bank(false);bad.resize(10);auto x=sds::resolveWwiseBankEventMedia(bad,10);assert(!x.bankValid);}
