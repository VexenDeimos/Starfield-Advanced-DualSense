#include <StarfieldDualSense/MusicSelectionRecon.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <vector>

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

    void dummyCallback(std::uint32_t, void*) {}

    void* seenForwardedCookie = nullptr;
    std::uint32_t seenForwardedType = 0;

    void recordingCallback(std::uint32_t callbackType, void* callbackInfo)
    {
        auto* base = static_cast<sds::WwiseCallbackInfoBase*>(callbackInfo);
        seenForwardedCookie = base ? base->pCookie : nullptr;
        seenForwardedType = callbackType;
    }
}

int main()
{
    using sds::MusicSelectionCallbackChainPool;
    using sds::MusicSelectionPostContext;

    const std::array<std::uint32_t, 1> targets{ 0x495792C0u };
    expect(sds::publishMusicSelectionReconTargets(targets), "runtime target catalog publishes test event");

    MusicSelectionPostContext chained{
        .armed = true,
        .eventId = 0x495792C0u,
        .externalCount = 0,
        .hasCallback = true,
        .hasCookie = true,
        .flags = 0x00100009u,
    };
    expect(sds::qualifiesMusicSelectionCallbackChain(chained), "existing AK_Duration+AK_EndOfEvent callback qualifies for chaining");

    auto missingDuration = chained;
    missingDuration.flags &= ~sds::kWwiseDurationCallback;
    expect(!sds::qualifiesMusicSelectionCallbackChain(missingDuration), "chain rejects callback without AK_Duration");

    auto missingEnd = chained;
    missingEnd.flags &= ~sds::kWwiseEndOfEventCallback;
    expect(!sds::qualifiesMusicSelectionCallbackChain(missingEnd), "chain rejects callback without AK_EndOfEvent retirement authority");

    auto missingCallback = chained;
    missingCallback.hasCallback = false;
    expect(!sds::qualifiesMusicSelectionCallbackChain(missingCallback), "chain rejects post without an existing callback");

    auto extraCallbackType = chained;
    extraCallbackType.flags |= 0x0002u;
    expect(!sds::qualifiesMusicSelectionCallbackChain(extraCallbackType),
        "chain rejects unproven additional callback types instead of assuming their info layout");

    auto external = chained;
    external.externalCount = 1;
    expect(!sds::qualifiesMusicSelectionCallbackChain(external), "chain rejects external-source post");

    auto nonTarget = chained;
    nonTarget.eventId = 0x6B110DA4u;
    expect(!sds::qualifiesMusicSelectionCallbackChain(nonTarget), "chain rejects event outside music target catalog");

    MusicSelectionCallbackChainPool pool;
    expect(pool.acquire(nullptr, nullptr, 0x495792C0u, 0x29u) == nullptr, "pool rejects null original callback");

    int cookieValue = 42;
    auto* context = pool.acquire(&recordingCallback, &cookieValue, 0x495792C0u, 0x29u);
    expect(context != nullptr, "pool acquires bounded callback context");
    expect(context && context->originalCallback == &recordingCallback, "context preserves original callback");
    expect(context && context->originalCookie == &cookieValue, "context preserves original cookie");
    expect(context && context->eventId == 0x495792C0u, "context preserves target event id");
    expect(context && context->gameObjectId == 0x29u, "context preserves game object id");
    expect(pool.callbackReady(context), "posting context is callback-readable after release publication");
    expect(pool.activeCount() == 1u, "posting context occupies one pool slot");

    expect(pool.completePost(context, 58u), "successful PostEvent binds returned playing id");
    expect(pool.inUse(context), "successful PostEvent keeps context active");
    expect(context && context->playingId.load() == 58u, "context retains returned playing id");

    sds::WwiseEventCallbackInfo callbackInfo{
        .pCookie = &cookieValue,
        .gameObjectId = 0x29u,
        .playingId = 58u,
        .eventId = 0x495792C0u,
    };
    expect(pool.findForCallback(callbackInfo) == context, "callback lookup resolves active context by original cookie and playing id");

    seenForwardedCookie = nullptr;
    seenForwardedType = 0;
    pool.forwardOriginalCallback(context, sds::kWwiseDurationCallback, &callbackInfo);
    expect(seenForwardedCookie == &cookieValue, "forwarded callback observes untouched Starfield cookie");
    expect(seenForwardedType == sds::kWwiseDurationCallback, "forwarded callback preserves callback type");
    expect(callbackInfo.pCookie == &cookieValue, "wrapper never substitutes Starfield callback cookie");
    expect(pool.inUse(context), "duration forwarding keeps callback-chain context active");

    pool.forwardOriginalCallback(context, sds::kWwiseEndOfEventCallback, &callbackInfo);
    expect(seenForwardedCookie == &cookieValue, "terminal callback also observes untouched Starfield cookie");
    expect(callbackInfo.pCookie == &cookieValue, "terminal forwarding leaves Starfield cookie untouched");
    expect(!pool.inUse(context), "terminal forwarding releases callback-chain context afterward");
    expect(!pool.callbackReady(context), "released context is not callback-readable");

    auto* synchronousEnd = pool.acquire(&dummyCallback, &cookieValue, 0x495792C0u, 0x29u);
    expect(synchronousEnd != nullptr, "pool reacquires context for callback-before-PostEvent-return fixture");
    sds::WwiseEventCallbackInfo synchronousInfo{
        .pCookie = &cookieValue,
        .gameObjectId = 0x29u,
        .playingId = 77u,
        .eventId = 0x495792C0u,
    };
    expect(pool.findForCallback(synchronousInfo) == synchronousEnd, "posting callback binds playing id before PostEvent returns");
    pool.forwardOriginalCallback(synchronousEnd, sds::kWwiseEndOfEventCallback, &synchronousInfo);
    expect(pool.inUse(synchronousEnd), "end callback during PostEvent does not free slot early");
    expect(pool.completePost(synchronousEnd, 77u), "PostEvent return agrees with callback-bound playing id");
    expect(!pool.inUse(synchronousEnd), "PostEvent return retires context ended during posting");

    auto* duplicateOwner = pool.acquire(&dummyCallback, &cookieValue, 0x495792C0u, 0x29u);
    expect(duplicateOwner != nullptr, "pool acquires duplicate-key owner fixture");
    expect(pool.completePost(duplicateOwner, 88u), "duplicate-key owner becomes active");
    expect(pool.acquire(&dummyCallback, &cookieValue, 0x495792C0u, 0x29u) == nullptr,
        "duplicate cookie/event/game-object key fails closed instead of becoming ambiguous");
    sds::WwiseEventCallbackInfo duplicateEnd{
        .pCookie = &cookieValue,
        .gameObjectId = 0x29u,
        .playingId = 88u,
        .eventId = 0x495792C0u,
    };
    pool.forwardOriginalCallback(duplicateOwner, sds::kWwiseEndOfEventCallback, &duplicateEnd);

    auto* failedPost = pool.acquire(&dummyCallback, &cookieValue, 0x495792C0u, 0x29u);
    expect(failedPost != nullptr, "pool acquires failed-post fixture");
    expect(pool.completePost(failedPost, 0u), "failed PostEvent cleanup is internally consistent");
    expect(!pool.inUse(failedPost), "failed PostEvent releases reserved context");

    std::vector<sds::MusicSelectionCallbackChainContext*> leases;
    leases.reserve(sds::kMusicSelectionCallbackChainCapacity);
    for (std::size_t i = 0; i < sds::kMusicSelectionCallbackChainCapacity; ++i) {
        if (auto* lease = pool.acquire(
                &dummyCallback,
                reinterpret_cast<void*>(0x1000u + i),
                static_cast<std::uint32_t>(i + 1u),
                static_cast<std::uint64_t>(0x2000u + i))) {
            leases.push_back(lease);
        }
    }
    expect(leases.size() == sds::kMusicSelectionCallbackChainCapacity, "pool exposes exactly fixed callback-chain capacity");
    expect(pool.acquire(&dummyCallback, reinterpret_cast<void*>(0xDEADu), 0xDEADBEEFu, 0xBEEFu) == nullptr,
        "full callback-chain pool fails closed");
    for (auto* lease : leases) {
        (void)pool.completePost(lease, 0u);
    }
    expect(pool.activeCount() == 0u, "failed-close fixture releases all callback-chain slots");

    MusicSelectionPostContext direct{
        .armed = true,
        .eventId = 0x495792C0u,
        .externalCount = 0,
        .hasCallback = false,
        .hasCookie = false,
        .flags = 0,
    };
    expect(sds::qualifiesMusicSelectionPost(direct), "legacy unowned direct-duration injection remains eligible");

    expect(sds::kMusicSelectionCallbackChainCapacity == 128u, "callback-chain pool capacity is fixed at 128");
    expect(sds::kWwiseEndOfEventCallback == 0x0001u, "AK_EndOfEvent callback value is fixed");
    expect(sds::kWwiseDurationCallback == 0x0008u, "AK_Duration callback value is fixed");
    expect(offsetof(sds::WwiseEventCallbackInfo, playingId) == 16u, "event callback playingId ABI offset is fixed");
    expect(offsetof(sds::WwiseEventCallbackInfo, eventId) == 20u, "event callback eventId ABI offset is fixed");

    return failures == 0 ? 0 : 1;
}
