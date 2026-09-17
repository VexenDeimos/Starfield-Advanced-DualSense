#include <StarfieldDualSense/Config.h>
#include <StarfieldDualSense/ControllerSpeakerManager.h>
#include <StarfieldDualSense/IControllerSpeakerBackend.h>
#include <StarfieldDualSense/SpeakerMixer.h>
#include <StarfieldDualSense/WwiseRemoteVoMirrorGate.h>

#include <array>
#include <iostream>
#include <memory>
#include <string_view>

namespace
{
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

    sds::RemoteVoMirrorCandidate qualifyingCandidate()
    {
        return {
            .eventId = sds::kRemoteCommsVoEventId,
            .externalCookie = 0x24DB9834u,
            .externalCount = 1,
            .codecId = 4,
            .fileId = 0,
            .memorySize = 0,
            .hasMemory = false,
            .hasFilePath = true,
            .dialogueMenuActive = false,
            .filePathLength = 32,
            .originalPlayingId = 42,
        };
    }

    class RecordingBackend final : public sds::IControllerSpeakerBackend
    {
    public:
        void start() override { started = true; }
        void stop() noexcept override { started = false; }
        bool enqueue(const sds::SpeakerCommand&) noexcept override { return started; }
        bool enqueuePreparedPcm(const sds::PreparedSpeakerPcm&) noexcept override
        {
            ++queued;
            return started;
        }
        bool replacePreparedPcm(sds::PreparedSpeakerPcm) noexcept override
        {
            ++replaced;
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
        std::size_t queued{ 0 };
        std::size_t replaced{ 0 };
    };
}

int main()
{
    const auto candidate = qualifyingCandidate();
    expect(sds::qualifiesRemoteVoCandidate(candidate, sds::kRemoteCommsVoMirrorProfile),
        "remote VO qualification is reusable for continuous source handling");
    expect(sds::qualifiesRemoteVoCandidate(candidate, sds::kRemoteCommsVoMirrorProfile),
        "continuous qualification remains true on a second line");

    sds::OneShotRemoteVoMirrorGate mirrorGate{};
    expect(mirrorGate.tryClaim(candidate), "legacy Wwise mirror gate still accepts the first qualifying line");
    expect(!mirrorGate.tryClaim(candidate), "legacy Wwise mirror gate remains one-shot");

    sds::SpeakerMixer preparedOnlyMixer(1.0F);
    sds::PreparedSpeakerPcm preparedOnly{};
    preparedOnly.frames.assign(64, sds::StereoSpeakerFrame{ 0.5F, 0.5F });
    expect(preparedOnlyMixer.addPrepared(preparedOnly), "prepared-only remote VO accepted");
    preparedOnlyMixer.clearPrepared();
    expect(preparedOnlyMixer.empty(), "clearing prepared PCM removes the old remote VO voice");

    sds::SpeakerMixer mixer(1.0F);
    sds::SpeakerCommand generated{};
    generated.frequencyHz = 440.0F;
    generated.duration = std::chrono::milliseconds(20);
    generated.gain = 0.25F;
    expect(mixer.add(generated), "generated speaker cue accepted before replacement");

    sds::PreparedSpeakerPcm oldVoice{};
    oldVoice.frames.assign(128, sds::StereoSpeakerFrame{ 0.5F, 0.5F });
    expect(mixer.addPrepared(oldVoice), "old prepared remote VO accepted");
    mixer.clearPrepared();
    expect(!mixer.empty(), "clearing remote VO preserves generated speaker cues");

    std::array<sds::StereoSpeakerFrame, 64> rendered{};
    mixer.render(rendered);
    bool generatedStillAudible = false;
    for (const auto& frame : rendered) {
        generatedStillAudible |= frame.left != 0.0F || frame.right != 0.0F;
    }
    expect(generatedStillAudible, "generated speaker cue still renders after remote VO replacement");

    auto backend = std::make_unique<RecordingBackend>();
    auto* recording = backend.get();
    sds::ControllerSpeakerManager manager(sds::Config::defaults(), std::move(backend));
    manager.start();

    sds::PreparedSpeakerPcm first{};
    first.frames.assign(32, sds::StereoSpeakerFrame{ 0.1F, 0.1F });
    sds::CapturedSoundIdentity identity{ .id = 1, .controllerOnlySafe = false };
    expect(manager.submitCaptured(first, sds::SpeakerCategory::Comms, identity, false),
        "ordinary captured PCM still queues additively");
    expect(recording->queued == 1 && recording->replaced == 0,
        "ordinary captured PCM uses enqueue path");

    sds::PreparedSpeakerPcm second{};
    second.frames.assign(32, sds::StereoSpeakerFrame{ 0.2F, 0.2F });
    expect(manager.submitCaptured(second, sds::SpeakerCategory::Comms, identity, true),
        "continuous remote VO replacement is accepted");
    expect(recording->queued == 1 && recording->replaced == 1,
        "continuous remote VO uses replace path");

    return failures == 0 ? 0 : 1;
}
