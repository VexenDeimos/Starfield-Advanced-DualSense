#include <StarfieldDualSense/WwiseSpatialProbeGate.h>

#include <cassert>

int main()
{
    using sds::SpatialProbeCandidate;
    using sds::qualifiesSpatialProbeCandidate;

    // v0.2.98 identifies the Eon fire event by event identity, not by a
    // session-specific Wwise game-object ID. Different nonzero emitters must
    // therefore qualify when the event itself is the observed Eon event.
    const SpatialProbeCandidate eonOnSessionEmitterA{
        .eventId = 0xE7205CE1u,
        .gameObjectId = 0x13u,
        .externalCount = 0u,
        .originalPlayingId = 286u,
    };
    assert(qualifiesSpatialProbeCandidate(eonOnSessionEmitterA));

    const SpatialProbeCandidate eonOnSessionEmitterB{
        .eventId = 0xE7205CE1u,
        .gameObjectId = 0x42u,
        .externalCount = 0u,
        .originalPlayingId = 900u,
    };
    assert(qualifiesSpatialProbeCandidate(eonOnSessionEmitterB));

    // Fail closed on an invalid emitter, unrelated events, external-source
    // events, or an original PostEvent that Wwise rejected.
    const SpatialProbeCandidate zeroEmitter{
        .eventId = 0xE7205CE1u,
        .gameObjectId = 0u,
        .externalCount = 0u,
        .originalPlayingId = 286u,
    };
    assert(!qualifiesSpatialProbeCandidate(zeroEmitter));

    const SpatialProbeCandidate unrelatedInternal{
        .eventId = 0x58D15B40u,
        .gameObjectId = 0x85u,
        .externalCount = 0u,
        .originalPlayingId = 287u,
    };
    assert(!qualifiesSpatialProbeCandidate(unrelatedInternal));

    const SpatialProbeCandidate externalEon{
        .eventId = 0xE7205CE1u,
        .gameObjectId = 0x13u,
        .externalCount = 1u,
        .originalPlayingId = 286u,
    };
    assert(!qualifiesSpatialProbeCandidate(externalEon));

    const SpatialProbeCandidate rejectedEon{
        .eventId = 0xE7205CE1u,
        .gameObjectId = 0x13u,
        .externalCount = 0u,
        .originalPlayingId = 0u,
    };
    assert(!qualifiesSpatialProbeCandidate(rejectedEon));

    return 0;
}
