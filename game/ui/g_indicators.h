#pragma once

#include "Main.h"

// The HUD (PS2 G_IndicatorsClass, g_indicators.cpp; mafiaIndicators is 0x4e38 bytes there). Only what main uses so far.
class G_IndicatorsClass
{
public:
    // PC only, the name is ours. Stores the flag at 0x46f4 and, with Matrox Parhelia's two or three screens (the global
    // at 0x6c4508, 1 or 2), confines the scene's viewport and camera to the middle one while it is set; main sets it
    // around the car encyclopedia.
    void ParheliaSetCenter(bool center);
};

extern G_IndicatorsClass mafiaIndicators;
