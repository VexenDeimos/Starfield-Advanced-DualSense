#include <StarfieldDualSense/DualSenseAudioRenderBlock.h>
#include <StarfieldDualSense/HapticWaveforms.h>
#include <StarfieldDualSense/DualSenseAudioTransport.h>
#include <StarfieldDualSense/SpeakerMixer.h>

#include <array>
#include <iostream>
#include <string_view>

namespace { int failures=0; void expect(bool c,std::string_view n){ if(c) std::cout<<"PASS "<<n<<'\n'; else {std::cerr<<"FAIL "<<n<<'\n'; ++failures;}} }

int main(){
    const auto mappedSpeaker = sds::mapSpeakerToProvenUsbChannels(sds::StereoSpeakerFrame{0.25F, 0.75F});
    expect(mappedSpeaker.left == 0.0F && mappedSpeaker.right == 0.5F,
           "hardware-proven USB speaker mapping downmixes to Channel2 only");
    std::array<sds::StereoSpeakerFrame,2> speaker{{ {0.25F,-0.25F},{0.5F,-0.5F} }};
    std::array<sds::HapticFrame,2> haptics{{ {0.0F,0.0F,0.75F,-0.75F},{0.0F,0.0F,1.0F,-1.0F} }};
    std::array<sds::DualSenseAudioFrame,2> out{};
    sds::composeDualSenseAudioFrames(speaker,haptics,out);
    expect(out[0].ch1==0.0F && out[0].ch2==0.0F && out[0].ch3==0.75F && out[0].ch4==-0.75F,"physical mapping downmixes logical speaker pair onto proven Channel2 and keeps haptics isolated");
    std::array<sds::StereoSpeakerFrame,2> silent{};
    sds::composeDualSenseAudioFrames(silent,haptics,out);
    expect(out[0].ch1==0.0F && out[0].ch2==0.0F && out[0].ch3==haptics[0][2] && out[0].ch4==haptics[0][3],"silent speaker preserves haptic samples exactly");
    speaker[0]={2.0F,2.0F}; haptics[0][2]=2.0F; haptics[0][3]=-2.0F;
    sds::composeDualSenseAudioFrames(speaker,haptics,out);
    expect(out[0].ch1==0.0F && out[0].ch2==1.0F && out[0].ch3==1.0F && out[0].ch4==-1.0F,"speaker downmix and haptic pair clamp independently");
    sds::DualSenseAudioClientLifetime lifetime{};
    lifetime.startHaptics();
    expect(lifetime.activeClientCount()==1 && lifetime.transportShouldRun(), "haptics client starts transport lifetime");
    lifetime.startSpeaker();
    expect(lifetime.activeClientCount()==2, "speaker client shares active transport lifetime");
    lifetime.startSpeaker();
    expect(lifetime.activeClientCount()==2, "speaker start is idempotent");
    lifetime.stopHaptics();
    expect(lifetime.activeClientCount()==1 && lifetime.transportShouldRun(), "stopping haptics keeps speaker transport alive");
    lifetime.stopSpeaker();
    expect(lifetime.activeClientCount()==0 && !lifetime.transportShouldRun(), "last client stops transport lifetime");
    lifetime.stopSpeaker();
    expect(lifetime.activeClientCount()==0, "speaker stop is idempotent");

    sds::SpeakerMixer simultaneousMixer(1.0F);
    sds::SpeakerCommand simultaneousTone{};
    simultaneousTone.gain = 0.25F;
    simultaneousTone.duration = std::chrono::milliseconds(20);
    simultaneousTone.routing = sds::SpeakerRoutingMode::Both;
    expect(simultaneousMixer.add(simultaneousTone), "simultaneous speaker voice accepted");
    std::array<sds::StereoSpeakerFrame, 64> simultaneousSpeaker{};
    simultaneousMixer.render(simultaneousSpeaker);
    std::array<sds::HapticFrame, 64> simultaneousHaptics{};
    for (auto& f : simultaneousHaptics) { f[2] = 0.375F; f[3] = -0.625F; }
    std::array<sds::DualSenseAudioFrame, 64> simultaneousOut{};
    sds::composeDualSenseAudioFrames(simultaneousSpeaker, simultaneousHaptics, simultaneousOut);
    bool sawSpeaker = false;
    bool preservedHaptics = true;
    for (const auto& f : simultaneousOut) {
        sawSpeaker |= (f.ch1 != 0.0F || f.ch2 != 0.0F);
        preservedHaptics &= (f.ch3 == 0.375F && f.ch4 == -0.625F);
    }
    expect(sawSpeaker && preservedHaptics, "simultaneous speaker PCM leaves haptic lanes numerically unchanged");

    return failures?1:0;
}
