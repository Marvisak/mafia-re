#pragma once

#include "Main.h"

#include <vector>

// The debris and the flashes of explosions (PS2 S_DEBRIS and S_FLASH, VE_Debris.cpp). Their PC layouts aren't known
// yet: the file's copy loops move elements of 0x1c and 0x44 bytes, one size each.
struct S_DEBRIS
{
};

struct S_FLASH
{
};

// The explosions' debris (PS2 C_Debris, VE_Debris.cpp; the PC's file is the one at 0x564890-0x56615c, which has its
// methods), the game's 0x240, 0x2c bytes. PS2's has 0x10 bytes more before the vectors. Only what the game's
// constructor and destructor use so far: its constructor and destructor are the compiler's.
class C_Debris
{
public:
    std::vector<S_DEBRIS> debris;       // 0x0 (PS2 0x10, gif::vector<S_DEBRIS>)
    std::vector<S_FLASH> flashes;       // 0x10 (PS2 0x1c)
    uchar unknown_20[0xc];              // 0x20
};

// Its destructor's copy, which the game's destructor and its constructor's unwinding call (Mission.cpp's range), the
// same code as C_using_object's (tools.h), folded
// SYNTHETIC: GAME 0x5479e0
// ??1C_using_object@@QAE@XZ
