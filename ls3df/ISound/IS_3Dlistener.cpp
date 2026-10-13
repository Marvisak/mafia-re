// The 3D listener (PS2 IS_3Dlistener.cpp). Not written yet: only what other files need to link.
#include "isound.h"

// Without its vtable and reference count
// STUB: LS3DF 0x1007b060
ISND_3Dlistener::ISND_3Dlistener()
{
    memset(&params, 0, sizeof(params));
    params.dwSize = sizeof(params);
    params.flDistanceFactor = 1.0f;
    params.flRolloffFactor = 1.0f;
    params.flDopplerFactor = 1.0f;
}
