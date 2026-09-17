#include <StarfieldDualSense/MusicSelectionRecon.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <span>

namespace
{
    int failures = 0;

    void expect(bool condition, const char* label)
    {
        if (condition) {
            std::printf("PASS %s\n", label);
        } else {
            std::printf("FAIL %s\n", label);
            ++failures;
        }
    }
}

int main()
{
    using sds::MusicSelectionPostContext;
    using sds::qualifiesMusicSelectionPost;

    const std::array<std::uint32_t, 2> targets{ 0x9914DB09u, 0xFC745E47u };
    expect(sds::publishMusicSelectionReconTargets(std::span{ targets }), "catalog fixture publishes proven target set");

    MusicSelectionPostContext accepted{
        .armed = true,
        .eventId = targets[0],
        .externalCount = 0,
        .hasCallback = false,
        .hasCookie = false,
        .flags = 0,
    };

    expect(qualifiesMusicSelectionPost(accepted), "catalog target with unowned callback slot qualifies");

    auto otherEvent = accepted;
    otherEvent.eventId = 0xE59962DBu;
    expect(!qualifiesMusicSelectionPost(otherEvent), "event outside catalog is untouched");

    auto disarmed = accepted;
    disarmed.armed = false;
    expect(!qualifiesMusicSelectionPost(disarmed), "disarmed recon is untouched");

    auto external = accepted;
    external.externalCount = 1;
    expect(!qualifiesMusicSelectionPost(external), "external-source event is untouched");

    auto callbackOwned = accepted;
    callbackOwned.hasCallback = true;
    expect(!qualifiesMusicSelectionPost(callbackOwned), "existing callback ownership is preserved");

    auto cookieOwned = accepted;
    cookieOwned.hasCookie = true;
    expect(!qualifiesMusicSelectionPost(cookieOwned), "existing cookie ownership is preserved");

    auto existingDuration = accepted;
    existingDuration.flags = sds::kWwiseDurationCallback;
    expect(!qualifiesMusicSelectionPost(existingDuration), "existing duration callback bit is preserved");

    auto existingOtherCallback = accepted;
    existingOtherCallback.flags = 0x0001u;
    expect(!qualifiesMusicSelectionPost(existingOtherCallback), "any existing callback bit is preserved");

    auto nonCallbackCapability = accepted;
    nonCallbackCapability.flags = 0x00200000u;
    expect(qualifiesMusicSelectionPost(nonCallbackCapability), "non-callback Wwise capability flags remain eligible");

    expect(sds::kWwiseDurationCallback == 0x0008u, "AK_Duration callback value is fixed");
    expect(sizeof(sds::WwiseDurationCallbackInfo) == 48, "duration callback ABI size is fixed");
    expect(offsetof(sds::WwiseDurationCallbackInfo, mediaId) == 36, "duration callback mediaId offset is fixed");

    return failures == 0 ? 0 : 1;
}
