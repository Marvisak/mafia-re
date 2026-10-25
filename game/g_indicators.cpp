// The HUD (PS2 g_indicators.cpp; the PC's file is the one at 0x5f6a20-0x604e5c, docs/game-files.md). Not written yet:
// only what main needs to link.

#include "g_indicators.h"

#include "stub.h"

// GLOBAL: GAME 0x6bf980
G_IndicatorsClass mafiaIndicators;

#pragma auto_inline(off)

// STUB: GAME 0x604a80
void G_IndicatorsClass::ParheliaSetCenter(bool center)
{
    StubLog("G_IndicatorsClass::ParheliaSetCenter");
}

#pragma auto_inline(on)
