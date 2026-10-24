#pragma once

#include "Main.h"

// The racing championship (PS2 C_racing, racing.cpp), 0x688 bytes, which exists while its menu runs. Only what main
// uses so far.
class C_racing
{
public:
    C_racing();
    ~C_racing();

    uchar unknown_0[0x688];
};

extern C_racing *CRacing;

// The championship's menu, with CRacing created (PS2's file isn't known, perhaps g_m_race.cpp)
void ExecuteRacingMenu();
