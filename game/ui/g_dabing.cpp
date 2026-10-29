// The dubbing (PS2 g_dabing.cpp; the PC's file is most likely the one at 0x5f4b60-0x5f6a1c, between g_camera.cpp and
// g_indicators.cpp as the names sort, docs/game-files.md). Written so far: the globals, DabingInit, DabingDone and
// DabingReset (PC only), which C_cutscene::Close calls.
//
// Open: the original's .bss has G_DabingMorphActive (0x6bdb40) before dabingChanel (0x6bdb50), the reverse of PS2's
// definition order, where their buckets (tools/bsshash.py: 885 and 135) give the other order; as g_indicators.cpp's,
// what an object compiled with another file's precompiled header (/Yu) gives.

#include "ui/g_dabing.h"

#include <string.h>

#include "I3D/I3D.h"

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

// FUNCTION: GAME 0x5f5060
void DabingReset()
{
    for (int i = 0; i < NUM_DABING_CHANELS; i++)
    {
        if (dabingChanel[i].face)
        {
            dabingChanel[i].face->SetWeight(0, 0, 1.0f);
            dabingChanel[i].face->SetWeight(0, 1, 0.0f);
            dabingChanel[i].face->SetWeight(0, 2, 0.0f);
            dabingChanel[i].face->SetWeight(0, 3, 0.0f);
            dabingChanel[i].face->SetWeight(0, 4, 0.0f);
            dabingChanel[i].face->SetWeight(1, 0, 1.0f);
            dabingChanel[i].face->SetWeight(1, 1, 0.0f);
            dabingChanel[i].face->SetWeight(1, 2, 0.0f);
            dabingChanel[i].face->SetWeight(1, 3, 0.0f);
            dabingChanel[i].face->SetWeight(1, 4, 0.0f);
            dabingChanel[i].face->SetWeight(2, 0, 1.0f);
            dabingChanel[i].face->SetWeight(2, 1, 0.0f);
            dabingChanel[i].face->SetWeight(2, 2, 0.0f);
            dabingChanel[i].face->SetWeight(2, 3, 0.0f);
            dabingChanel[i].face->SetWeight(2, 4, 0.0f);
        }
        if (dabingChanel[i].data)
            delete dabingChanel[i].data;
        if (dabingChanel[i].sound && dabingChanel[i].state != DABING_STATE_UNKNOWN_5)
            dabingChanel[i].sound->Release();
        dabingChanel[i].sound = NULL;
    }
    memset(dabingChanel, 0, sizeof(dabingChanel));
    G_DabingMorphActive.clear();
}
