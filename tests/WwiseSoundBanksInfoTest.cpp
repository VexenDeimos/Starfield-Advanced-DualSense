#include <StarfieldDualSense/WwiseSoundBanksInfo.h>
#include <cassert>
#include <string>
int main(){
    const std::string j=R"({"SoundBanksInfo":{"StreamedFiles":[{"Id":"123456","ShortName":"bolt.wav","Path":"WPN\\Maelstrom\\bolt.wav"},{"Id":123456,"ShortName":"","Path":""}],"SoundBanks":[{"ShortName":"Weapons","IncludedEvents":[{"Id":"3884019342","Name":"Play_Maelstrom_Fire","ReferencedStreamedFiles":[{"Id":"123456"}]}]}]}})";
    auto r=sds::parseWwiseSoundBanksInfo(j,"fixture"); assert(r.ok); assert(r.index.mediaById.count(123456)==1); auto m=r.index.mediaById.at(123456); assert(m.shortName=="bolt.wav"); assert(m.originalPath=="WPN\\Maelstrom\\bolt.wav");
    auto it=r.index.eventsById.find(3884019342u); assert(it!=r.index.eventsById.end()); assert(it->second.size()==1); assert(it->second[0].eventName=="Play_Maelstrom_Fire"); assert(it->second[0].bankName=="Weapons"); assert(it->second[0].referencedMediaIds.size()==1 && it->second[0].referencedMediaIds[0]==123456u);
    auto partial=sds::parseWwiseSoundBanksInfo(R"({"SoundBanksInfo":{"StreamedFiles":[{"Id":42}],"SoundBanks":[]}})","partial"); assert(partial.ok && partial.index.mediaById.count(42));
    auto bad=sds::parseWwiseSoundBanksInfo("{broken","bad"); assert(!bad.ok);
}
