// The dubbing (PS2 g_dabing.cpp; the PC's file is most likely the one at 0x5f4b60-0x5f6a1c, between g_camera.cpp and
// g_indicators.cpp as the names sort, docs/game-files.md). Written so far: the globals, DabingInit and DabingDone;
// DabingReset (PC only), which C_cutscene::Close calls, is a stub.
//
// Open: the original's .bss has G_DabingMorphActive (0x6bdb40) before dabingChanel (0x6bdb50), the reverse of PS2's
// definition order, where their buckets (tools/bsshash.py: 885 and 135) give the other order; as g_indicators.cpp's,
// what an object compiled with another file's precompiled header (/Yu) gives.

#include "ui/g_dabing.h"

#include <string.h>

#include "I3D/I3D.h"
#include "stub.h"

// PS2 names, in PS2's order; the vector's initializer starts the file
// GLOBAL: GAME 0x6bdb50
S_dabing_chanel dabingChanel[NUM_DABING_CHANELS];
// GLOBAL: GAME 0x6bdb40
std::vector<G_DabingMorphObj> G_DabingMorphActive;

// FUNCTION: GAME 0x5f4bc0
void DabingInit()
{
    memset(dabingChanel, 0, sizeof(dabingChanel));
    G_DabingMorphActive.clear();
}

// PS2's frees no data
// FUNCTION: GAME 0x5f4c10
void DabingDone()
{
    for (int i = 0; i < NUM_DABING_CHANELS; i++)
    {
        if (dabingChanel[i].sound && dabingChanel[i].state != DABING_STATE_UNKNOWN_5)
            dabingChanel[i].sound->Release();
        dabingChanel[i].sound = NULL;
        if (dabingChanel[i].data)
            delete dabingChanel[i].data;
    }
    memset(dabingChanel, 0, sizeof(dabingChanel));
    G_DabingMorphActive.clear();
}

#pragma auto_inline(off)

// STUB: GAME 0x5f5060
void DabingReset()
{
    StubLog("DabingReset");
}

#pragma auto_inline(on)
