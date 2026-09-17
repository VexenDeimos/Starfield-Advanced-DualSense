#include <StarfieldDualSense/MusicSelectionRecon.h>

#include <array>
#include <cstddef>
#include <cstdio>

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
    using sds::MusicSelectionTargetCatalog;
    using sds::qualifiesMusicSelectionPost;

    MusicSelectionTargetCatalog catalog;
    expect(!catalog.ready(), "catalog starts unpublished");
    expect(!catalog.contains(0x9914DB09u), "unpublished catalog rejects all events");

    const std::array<std::uint32_t, 5> targets{
        0xFC745E47u,
        0x9914DB09u,
        0xFC745E47u,
        0x11111111u,
        0x22222222u,
    };
    expect(catalog.publish(targets), "first bounded target publication succeeds");
    expect(catalog.ready(), "catalog becomes ready after publication");
    expect(catalog.size() == 4u, "catalog sorts and deduplicates published targets");
    expect(catalog.contains(0x9914DB09u), "catalog contains proven Planet D event");
    expect(catalog.contains(0xFC745E47u), "catalog contains proven Planet E event");
    expect(!catalog.contains(0xE59962DBu), "catalog rejects non-target event");
    expect(!catalog.publish(targets), "catalog is immutable after first publication");

    MusicSelectionTargetCatalog oversizedCatalog;
    std::array<std::uint32_t, sds::kMusicSelectionReconMaxTargetEvents + 1u> oversizedTargets{};
    expect(!oversizedCatalog.publish(oversizedTargets), "oversized target set fails closed");
    expect(!oversizedCatalog.ready(), "rejected oversized target set remains unpublished");

    expect(sds::publishMusicSelectionReconTargets(targets), "global runtime target catalog publishes once");

    MusicSelectionPostContext accepted{
        .armed = true,
        .eventId = 0x9914DB09u,
        .externalCount = 0,
        .hasCallback = false,
        .hasCookie = false,
        .flags = 0,
    };
    expect(qualifiesMusicSelectionPost(accepted), "catalog target with unowned callback slot qualifies");

    auto notCatalogTarget = accepted;
    notCatalogTarget.eventId = 0xE59962DBu;
    expect(!qualifiesMusicSelectionPost(notCatalogTarget), "event outside catalog is untouched");

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

    expect(sds::kMusicSelectionReconMaxTargetEvents == 512u, "target catalog capacity is fixed at 512");
    expect(sds::kWwiseDurationCallback == 0x0008u, "AK_Duration callback value is fixed");
    expect(sizeof(sds::WwiseDurationCallbackInfo) == 48, "duration callback ABI size is fixed");
    expect(offsetof(sds::WwiseDurationCallbackInfo, mediaId) == 36, "duration callback mediaId offset is fixed");

    return failures == 0 ? 0 : 1;
}
