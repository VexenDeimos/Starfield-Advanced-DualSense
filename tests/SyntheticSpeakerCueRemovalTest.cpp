#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/ControllerSpeakerManager.h>
#include <StarfieldDualSense/IControllerSpeakerBackend.h>
#include <StarfieldDualSense/SpeakerEventClassifier.h>

#include <cstring>
#include <iostream>
#include <memory>
#include <string_view>
#include <vector>

namespace {
int failures = 0;
void expect(bool condition, std::string_view name)
{
    if (condition) {
        std::cout << "PASS " << name << '\n';
    } else {
        std::cerr << "FAIL " << name << '\n';
        ++failures;
    }
}

sds::GameEvent makeEvent(sds::GameEventType type, const char* text = "")
{
    sds::GameEvent event{};
    event.type = type;
    std::strncpy(event.text.data(), text, event.text.size() - 1);
    return event;
}

class RecordingBackend final : public sds::IControllerSpeakerBackend
{
public:
    void start() override { started = true; }
    void stop() noexcept override { started = false; }
    bool enqueue(const sds::SpeakerCommand&) noexcept override
    {
        ++generatedCommands;
        return started;
    }
    bool enqueuePreparedPcm(const sds::PreparedSpeakerPcm&) noexcept override
    {
        ++preparedCommands;
        return started;
    }
    bool replacePreparedPcm(sds::PreparedSpeakerPcm) noexcept override
    {
        ++replacedPreparedCommands;
        return started;
    }
    bool setPersistentPreparedPcm(sds::PersistentPreparedSpeakerPcm) noexcept override
    {
        return started;
    }
    bool clearPersistentPreparedPcm(std::uint64_t, bool = false) noexcept override
    {
        return started;
    }
    [[nodiscard]] bool active() const noexcept override { return started; }

    bool started{ false };
    std::size_t generatedCommands{ 0 };
    std::size_t preparedCommands{ 0 };
    std::size_t replacedPreparedCommands{ 0 };
};
}

int main()
{
    const auto cfg = sds::Config::defaults();

    const std::vector<sds::GameEvent> syntheticCandidates{
        makeEvent(sds::GameEventType::WeaponFired, "WeaponFire"),
        makeEvent(sds::GameEventType::ReloadCompleted, "ReloadComplete"),
        makeEvent(sds::GameEventType::WeaponEquipped, "Eon"),
        makeEvent(sds::GameEventType::MeleeSwing),
        makeEvent(sds::GameEventType::MenuOpened, "MonocleMenu"),
        makeEvent(sds::GameEventType::MenuClosed, "MonocleMenu"),
        makeEvent(sds::GameEventType::MenuOpened, "DataMenu"),
        makeEvent(sds::GameEventType::MenuClosed, "DataMenu"),
        makeEvent(sds::GameEventType::MenuOpened, "SecurityMenu"),
        makeEvent(sds::GameEventType::MenuOpened, "WeaponsCraftingMenu"),
        makeEvent(sds::GameEventType::MenuOpened, "ShipRefuelMenu")
    };

    for (const auto& event : syntheticCandidates) {
        expect(!sds::classifySpeakerEvent(event, cfg),
               "semantic game event does not generate synthesized controller-speaker tone");
    }

    auto lowHealth = makeEvent(sds::GameEventType::PlayerHealthChanged);
    lowHealth.value = 0.20F;
    expect(!sds::classifySpeakerEvent(lowHealth, cfg),
           "low health does not generate synthesized controller-speaker alert");

    auto backend = std::make_unique<RecordingBackend>();
    auto* recording = backend.get();
    sds::ControllerSpeakerManager manager(cfg, std::move(backend));
    manager.start();

    for (const auto& event : syntheticCandidates) {
        expect(manager.handle(event), "speaker manager harmlessly ignores semantic event");
    }
    expect(recording->generatedCommands == 0,
           "speaker manager sends zero synthesized commands after beep removal");

    sds::PreparedSpeakerPcm comms{};
    comms.frames.assign(32, sds::StereoSpeakerFrame{ 0.25F, 0.25F });
    comms.gain = 1.0F;
    expect(manager.submitCaptured(
               comms,
               sds::SpeakerCategory::Comms,
               sds::CapturedSoundIdentity{ 0x1234, true },
               true),
           "real captured Comms PCM remains accepted");
    expect(recording->replacedPreparedCommands == 1 && recording->preparedCommands == 0,
           "captured Comms PCM still takes prepared-audio path rather than synthesized-command path");

    manager.stop();
    return failures == 0 ? 0 : 1;
}
