#include <StarfieldDualSense/WwiseResolvedMediaExtractor.h>
#include <cassert>
#include <filesystem>
#include <vector>
int main(){
 auto root=std::filesystem::temp_directory_path()/"sds_v0320_extract";std::filesystem::remove_all(root);
 std::vector<unsigned char>a{1,2,3},b{4,5,6};
 auto r=sds::writeResolvedWemDiagnostic(root,"fire",0xE7814E8Eu,123456,"bolt_release.wav",a);assert(r.status=="written");assert(r.path.filename()=="123456__bolt_release.wem");
 auto pathQualified=sds::writeResolvedWemDiagnostic(root,"fire",0xE7814E8Eu,654321,"WPN\\Hand\\Rifle\\Maelstrom\\Fire\\WPN_Hand_Rifle_Maelstrom_Fire_PC_V3_03.wav",a);assert(pathQualified.status=="written");assert(pathQualified.path.filename()=="654321__WPN_Hand_Rifle_Maelstrom_Fire_WPN_Hand_Rifle_Maelstrom_Fire_PC_V3_03.wem");
 auto r2=sds::writeResolvedWemDiagnostic(root,"fire",0xE7814E8Eu,123456,"bolt_release.wav",a);assert(r2.status=="exists-same");
 auto r3=sds::writeResolvedWemDiagnostic(root,"fire",0xE7814E8Eu,123456,"bolt_release.wav",b);assert(r3.status=="written-duplicate");
 auto bad=sds::writeResolvedWemDiagnostic(root,"fire",0xE7814E8Eu,9,"../evil.wem",a);assert(bad.status=="rejected");assert(bad.error.find("traversal")!=std::string::npos);
 auto over=sds::writeResolvedWemDiagnostic(root,"reload",0x7F65DE86u,10,"x.wem",a,2);assert(over.status=="rejected");
 auto draw=sds::writeResolvedWemDiagnostic(root,"draw",0xFFDDC978u,600001,"draw.wav",a);assert(draw.status=="written");assert(draw.path.parent_path().filename()=="FFDDC978");assert(draw.path.parent_path().parent_path().filename()=="draw");
 auto holster=sds::writeResolvedWemDiagnostic(root,"holster",0x5A51678Fu,600002,"holster.wav",a);assert(holster.status=="written");assert(holster.path.parent_path().filename()=="5A51678F");assert(holster.path.parent_path().parent_path().filename()=="holster");
 auto unknown=sds::writeResolvedWemDiagnostic(root,"unknown",0x12345678u,11,"x.wem",a);assert(unknown.status=="rejected");assert(unknown.error=="invalid semantic");
 std::filesystem::remove_all(root);
}
