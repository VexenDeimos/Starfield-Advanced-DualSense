#include "StarfieldDualSense/TESHitSourceDiscovery.h"

#include <iostream>

int main()
{
    if (!sds::tesHitUnregistrationVerified(8, 7, true, false)) {
        std::cerr << "FAIL v0.3.53 observed TESHit shutdown must accept an exact one-count drop even when another sink joined after registration\n";
        return 1;
    }
    if (sds::tesHitUnregistrationVerified(8, 8, true, false)) {
        std::cerr << "FAIL v0.3.53 TESHit shutdown must reject an unchanged sink count\n";
        return 1;
    }
    if (sds::tesHitUnregistrationVerified(8, 7, true, true)) {
        std::cerr << "FAIL v0.3.53 TESHit shutdown must reject our exact sink remaining present\n";
        return 1;
    }

    if (sds::tesHitUnregistrationVerified(0, UINT32_MAX, true, false)) {
        std::cerr << "FAIL v0.3.53 TESHit shutdown count-drop verification must not wrap at zero\n";
        return 1;
    }

    std::cout << "PASS v0.3.53 TESHit shutdown local unregistration verification\n";
    return 0;
}
