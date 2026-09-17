#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/SpeakerTypes.h>
#include <StarfieldDualSense/SpeakerPcm.h>
#include <StarfieldDualSense/SpeakerMixer.h>
#include <StarfieldDualSense/SpeakerEventClassifier.h>
#include <StarfieldDualSense/ControllerSpeakerManager.h>
#include <StarfieldDualSense/CommsProcessor.h>
#include <StarfieldDualSense/IControllerSpeakerBackend.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <iostream>
#include <string_view>

namespace {
int failures = 0;
void expect(bool condition, std::string_view name) {
    if (condition) std::cout << "PASS " << name << '\n';
    else { std::cerr << "FAIL " << name << '\n'; ++failures; }
}
}

namespace {
class RecordingSpeakerBackend final : public sds::IControllerSpeakerBackend {
public:
    void start() override { started = true; }
    void stop() noexcept override { started = false; }
    bool enqueue(const sds::SpeakerCommand& command) noexcept override {
        if (!started || reject) return false;
        last = command;
        ++commands;
        return true;
    }
    bool enqueuePreparedPcm(const sds::PreparedSpeakerPcm& pcm) noexcept override {
        if (!started || reject) return false;
        lastPrepared = pcm;
        ++preparedCommands;
        return true;
    }
    bool replacePreparedPcm(sds::PreparedSpeakerPcm pcm) noexcept override {
        if (!started || reject) return false;
        lastPrepared = std::move(pcm);
        ++preparedCommands;
        return true;
    }
    bool setPersistentPreparedPcm(sds::PersistentPreparedSpeakerPcm voice) noexcept override {
        if (!started || reject || !voice.pcm) return false;
        lastPersistent = std::move(voice);
        ++persistentSetCommands;
        return true;
    }
    bool clearPersistentPreparedPcm(std::uint64_t owner, bool force = false) noexcept override {
        if (!started || reject) return false;
        lastPersistentClearOwner = owner;
        lastPersistentClearForce = force;
        ++persistentClearCommands;
        return true;
    }
    [[nodiscard]] bool active() const noexcept override { return started; }
    bool started{false};
    bool reject{false};
    std::size_t commands{0};
    std::size_t preparedCommands{0};
    std::size_t persistentSetCommands{0};
    std::size_t persistentClearCommands{0};
    std::uint64_t lastPersistentClearOwner{0};
    bool lastPersistentClearForce{false};
    sds::SpeakerCommand last{};
    sds::PreparedSpeakerPcm lastPrepared{};
    sds::PersistentPreparedSpeakerPcm lastPersistent{};
};

sds::GameEvent makeSpeakerEvent(sds::GameEventType type, const char* text = "") {
    sds::GameEvent event{};
    event.type = type;
    std::strncpy(event.text.data(), text, event.text.size() - 1);
    return event;
}
}

int main() {
    const auto defaults = sds::Config::defaults();
    expect(defaults.controllerSpeaker, "speaker enabled by default");
    expect(std::fabs(defaults.speakerVolume - 0.8F) < 0.0001F, "speaker volume defaults to 0.8");
    expect(defaults.speakerOutputMode == sds::SpeakerOutputMode::Both, "speaker output defaults to Both");
    expect(std::fabs(defaults.speakerWeaponsVolume - 1.0F) < 0.0001F, "weapon speaker volume defaults to 1.0");
    expect(defaults.speakerComms && defaults.speakerScannerUI && defaults.speakerWeapons &&
           defaults.speakerDigipick && defaults.speakerCrafting && defaults.speakerShipSystems,
           "all speaker categories default enabled");

    const auto parsed = sds::loadConfig(R"(
SpeakerVolume = 2.0
SpeakerOutputMode = "ControllerOnly"
SpeakerComms = false
SpeakerScannerUI = false
SpeakerWeapons = false
SpeakerWeaponsVolume = 0.6
SpeakerDigipick = false
SpeakerCrafting = false
SpeakerShipSystems = false
)");
    expect(parsed.speakerVolume == 1.0F, "speaker volume clamps high values");
    expect(parsed.speakerOutputMode == sds::SpeakerOutputMode::ControllerOnly, "ControllerOnly parses");
    expect(!sds::speakerCategoryEnabled(parsed, sds::SpeakerCategory::Comms), "comms category toggle applies");
    expect(std::fabs(parsed.speakerWeaponsVolume - 0.6F) < 0.0001F, "weapon speaker volume parses independently");
    expect(!sds::speakerCategoryEnabled(parsed, sds::SpeakerCategory::ShipSystems), "ship category toggle applies");

    const auto invalid = sds::loadConfig("SpeakerOutputMode = \"SomethingElse\"\n");
    expect(invalid.speakerOutputMode == sds::SpeakerOutputMode::Both, "unknown output mode keeps default Both");


    const std::array<float, 2> mono{{0.25F, -0.5F}};
    const auto monoPrepared = sds::prepareSpeakerPcm(mono.data(), mono.size(), sds::SpeakerSampleFormat::Float32, 48000, 1);
    expect(monoPrepared && monoPrepared->frames.size() == 2 &&
           monoPrepared->frames[0].left == 0.25F && monoPrepared->frames[0].right == 0.25F &&
           monoPrepared->frames[1].left == -0.5F && monoPrepared->frames[1].right == -0.5F,
           "mono PCM duplicates to stereo");

    const std::array<float, 4> stereo{{0.1F, 0.2F, -0.3F, -0.4F}};
    const auto stereoPrepared = sds::prepareSpeakerPcm(stereo.data(), stereo.size(), sds::SpeakerSampleFormat::Float32, 48000, 2);
    expect(stereoPrepared && stereoPrepared->frames.size() == 2 &&
           std::fabs(stereoPrepared->frames[1].left + 0.3F) < 0.0001F &&
           std::fabs(stereoPrepared->frames[1].right + 0.4F) < 0.0001F,
           "stereo PCM stays stereo");

    const std::array<float, 2> mono24{{0.0F, 1.0F}};
    const auto upsampled = sds::prepareSpeakerPcm(mono24.data(), mono24.size(), sds::SpeakerSampleFormat::Float32, 24000, 1);
    expect(upsampled && upsampled->frames.size() == 4, "24 kHz PCM resamples to double 48 kHz frame count");

    const std::array<std::int16_t, 2> pcm16{{-32768, 32767}};
    const auto pcm16Prepared = sds::prepareSpeakerPcm(pcm16.data(), pcm16.size(), sds::SpeakerSampleFormat::Pcm16, 48000, 1);
    expect(pcm16Prepared && pcm16Prepared->frames.front().left <= -0.999F && pcm16Prepared->frames.back().left >= 0.999F,
           "PCM16 endpoints normalize into unit range");
    expect(!sds::prepareSpeakerPcm(mono.data(), mono.size(), sds::SpeakerSampleFormat::Float32, 0, 1), "zero sample rate rejected");
    expect(!sds::prepareSpeakerPcm(stereo.data(), stereo.size(), sds::SpeakerSampleFormat::Float32, 48000, 3), "unsupported channel count rejected");

    sds::SpeakerMixer mixer(0.5F);
    sds::SpeakerCommand tone{};
    tone.gain = 0.5F;
    tone.frequencyHz = 440.0F;
    tone.duration = std::chrono::milliseconds(20);
    tone.routing = sds::SpeakerRoutingMode::Both;
    expect(mixer.add(tone), "generated speaker tone accepted");
    std::array<sds::StereoSpeakerFrame, 480> mixBlock{};
    mixer.render(mixBlock);
    float maxL = 0.0F, maxR = 0.0F;
    for (const auto& f : mixBlock) { maxL = (std::max)(maxL, std::fabs(f.left)); maxR = (std::max)(maxR, std::fabs(f.right)); }
    expect(maxL > 0.0F && maxL <= 0.313F && std::fabs(maxL-maxR) < 0.001F, "mixer applies command gain and global volume");

    {
        sds::PreparedSpeakerPcm scaleProbe{};
        scaleProbe.gain = 1.0F;
        scaleProbe.frames.assign(1, sds::StereoSpeakerFrame{0.4F, 0.4F});

        sds::SpeakerMixer currentMaxAtNewDefault(0.8F);
        expect(currentMaxAtNewDefault.addPrepared(scaleProbe), "0.8 speaker volume scale probe accepted");
        std::array<sds::StereoSpeakerFrame, 1> currentMaxBlock{};
        currentMaxAtNewDefault.render(currentMaxBlock);
        expect(std::fabs(currentMaxBlock[0].left - 0.4F) < 0.0001F,
               "SpeakerVolume 0.8 equals the v0.3.10 maximum output gain");

        sds::SpeakerMixer newMax(1.0F);
        expect(newMax.addPrepared(scaleProbe), "1.0 speaker volume scale probe accepted");
        std::array<sds::StereoSpeakerFrame, 1> newMaxBlock{};
        newMax.render(newMaxBlock);
        expect(std::fabs(newMaxBlock[0].left - 0.5F) < 0.0001F &&
               newMaxBlock[0].left > currentMaxBlock[0].left,
               "SpeakerVolume 1.0 provides 25 percent headroom above the v0.3.10 maximum");
    }

    sds::SpeakerMixer routeMixer(1.0F);
    tone.routing = sds::SpeakerRoutingMode::Channel1;
    expect(routeMixer.add(tone), "channel1 tone accepted");
    std::array<sds::StereoSpeakerFrame, 240> routeBlock{};
    routeMixer.render(routeBlock);
    bool leftEnergy=false, rightEnergy=false;
    for (const auto& f: routeBlock) { leftEnergy |= std::fabs(f.left) > 0.0001F; rightEnergy |= std::fabs(f.right) > 0.0001F; }
    expect(leftEnergy && !rightEnergy, "Channel1 routing produces left-only speaker energy");

    sds::PreparedSpeakerPcm loud{};
    loud.gain = 2.0F;
    loud.frames.assign(16, sds::StereoSpeakerFrame{1.0F, -1.0F});
    sds::SpeakerMixer clipMixer(1.0F);
    expect(clipMixer.addPrepared(loud), "prepared speaker PCM accepted");
    std::array<sds::StereoSpeakerFrame,16> clipBlock{};
    clipMixer.render(clipBlock);
    expect(clipBlock[0].left == 1.0F && clipBlock[0].right == -1.0F, "speaker mixer clips each channel independently");


    {
        sds::PreparedSpeakerPcm speech{};
        speech.gain = 0.85F;
        speech.frames.resize(4800);
        for (std::size_t i = 0; i < speech.frames.size(); ++i) {
            const float t = static_cast<float>(i) / 48000.0F;
            const float base = 0.20F * std::sin(2.0F * 3.14159265358979323846F * 700.0F * t);
            const float transient = (i == 1200) ? 0.95F : 0.0F;
            speech.frames[i] = { base + transient, base + transient };
        }

        const auto processed = sds::processCommsPcm(speech);
        expect(processed.frames.size() == speech.frames.size(),
               "comms processing preserves prepared PCM frame count");

        float maxAbs = 0.0F;
        double energy = 0.0;
        double inputEnergy = 0.0;
        for (std::size_t i = 0; i < processed.frames.size(); ++i) {
            const auto& frame = processed.frames[i];
            maxAbs = (std::max)(maxAbs, (std::max)(std::fabs(frame.left), std::fabs(frame.right)));
            energy += static_cast<double>(frame.left) * frame.left + static_cast<double>(frame.right) * frame.right;
            inputEnergy += static_cast<double>(speech.frames[i].left) * speech.frames[i].left +
                static_cast<double>(speech.frames[i].right) * speech.frames[i].right;
        }
        expect(maxAbs <= 1.0F && energy > 0.0,
               "comms processing stays bounded and preserves audible energy");
        expect(energy > inputEnergy * 1.20,
               "comms processing adds meaningful post-compression makeup gain");
        expect(std::fabs(processed.frames[1200].left) < std::fabs(speech.frames[1200].left),
               "comms compressor reduces a strong transient peak");

        sds::PreparedSpeakerPcm silence{};
        silence.frames.assign(64, { 0.0F, 0.0F });
        const auto processedSilence = sds::processCommsPcm(silence);
        bool silent = true;
        for (const auto& frame : processedSilence.frames) {
            silent = silent && frame.left == 0.0F && frame.right == 0.0F;
        }
        expect(silent, "comms processing keeps silence silent");
    }

    sds::SpeakerMixer bounded(1.0F);
    std::size_t accepted = 0;
    for (std::size_t i=0; i<sds::kMaxSpeakerVoices+1; ++i) accepted += bounded.add(tone) ? 1U : 0U;
    expect(accepted == sds::kMaxSpeakerVoices && bounded.droppedSubmissions() == 1, "speaker mixer rejects newest voice at capacity");




    {
        auto cfg = sds::Config::defaults();
        const std::array<sds::GameEvent, 12> syntheticCandidates{{
            makeSpeakerEvent(sds::GameEventType::WeaponFired, "WeaponFire"),
            makeSpeakerEvent(sds::GameEventType::ReloadCompleted, "ReloadComplete"),
            makeSpeakerEvent(sds::GameEventType::WeaponEquipped, "Eon"),
            makeSpeakerEvent(sds::GameEventType::MeleeSwing),
            makeSpeakerEvent(sds::GameEventType::MenuOpened, "MonocleMenu"),
            makeSpeakerEvent(sds::GameEventType::MenuClosed, "MonocleMenu"),
            makeSpeakerEvent(sds::GameEventType::MenuOpened, "DataMenu"),
            makeSpeakerEvent(sds::GameEventType::MenuClosed, "DataMenu"),
            makeSpeakerEvent(sds::GameEventType::MenuOpened, "SecurityMenu"),
            makeSpeakerEvent(sds::GameEventType::MenuOpened, "WeaponsCraftingMenu"),
            makeSpeakerEvent(sds::GameEventType::MenuOpened, "ShipRefuelMenu"),
            makeSpeakerEvent(sds::GameEventType::PlayerHealthChanged)
        }};

        for (auto event : syntheticCandidates) {
            if (event.type == sds::GameEventType::PlayerHealthChanged) {
                event.value = 0.20F;
            }
            expect(!sds::classifySpeakerEvent(event, cfg),
                   "semantic event no longer classifies into synthesized speaker cue");
        }
    }

    {
        auto backend = std::make_unique<RecordingSpeakerBackend>();
        auto* recording = backend.get();
        sds::ControllerSpeakerManager manager(sds::Config::defaults(), std::move(backend));
        manager.start();
        expect(recording->started, "speaker manager starts backend");
        expect(manager.handle(makeSpeakerEvent(sds::GameEventType::WeaponFired, "WeaponFire")) &&
               recording->commands == 0,
               "speaker manager ignores semantic generated cue after beep removal");

        sds::PreparedSpeakerPcm captured{};
        captured.frames.assign(16, sds::StereoSpeakerFrame{0.25F, 0.25F});
        expect(manager.submitCaptured(
                   captured,
                   sds::SpeakerCategory::Comms,
                   sds::CapturedSoundIdentity{0x1234, true},
                   true),
               "speaker manager still accepts captured Comms PCM");
        manager.stop();
        expect(!recording->started, "speaker manager stops backend");
    }

    {
        auto backend = std::make_unique<RecordingSpeakerBackend>();
        auto* recording = backend.get();
        const auto cfg = sds::loadConfig("SpeakerWeaponsVolume = 0.6\n");
        sds::ControllerSpeakerManager manager(cfg, std::move(backend));
        manager.start();

        sds::PreparedSpeakerPcm weapon{};
        weapon.gain = 0.35F;
        weapon.frames.assign(8, sds::StereoSpeakerFrame{0.25F, 0.25F});
        expect(manager.submitCaptured(
                   weapon,
                   sds::SpeakerCategory::Weapons,
                   sds::CapturedSoundIdentity{0x5678, false},
                   false),
               "weapon PCM accepted with independent category volume");
        expect(recording->preparedCommands == 1 &&
                   std::fabs(recording->lastPrepared.gain - 0.21F) < 0.0001F,
               "SpeakerWeaponsVolume scales weapon PCM without changing tuned source gain");

        sds::PreparedSpeakerPcm comms{};
        comms.gain = 0.35F;
        comms.frames.assign(8, sds::StereoSpeakerFrame{0.25F, 0.25F});
        expect(manager.submitCaptured(
                   comms,
                   sds::SpeakerCategory::Comms,
                   sds::CapturedSoundIdentity{0x9ABC, true},
                   false),
               "non-weapon PCM accepted with weapon category volume configured");
        expect(std::fabs(recording->lastPrepared.gain - 0.35F) < 0.0001F,
               "SpeakerWeaponsVolume does not change non-weapon PCM");
    }

    {
        auto backend = std::make_unique<RecordingSpeakerBackend>();
        auto* recording = backend.get();
        const auto cfg = sds::loadConfig("SpeakerWeapons = false\nSpeakerWeaponsVolume = 0.6\n");
        sds::ControllerSpeakerManager manager(cfg, std::move(backend));
        manager.start();
        sds::PreparedSpeakerPcm weapon{};
        weapon.gain = 0.35F;
        weapon.frames.assign(8, sds::StereoSpeakerFrame{0.25F, 0.25F});
        expect(!manager.submitCaptured(
                   weapon,
                   sds::SpeakerCategory::Weapons,
                   sds::CapturedSoundIdentity{0xDEF0, false},
                   false) && recording->preparedCommands == 0,
               "SpeakerWeapons false fully disables captured weapon PCM");
    }

    return failures == 0 ? 0 : 1;
}
