#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class I3D_frame;

// The debris and the flashes of explosions (PS2 S_DEBRIS and S_FLASH, VE_Debris.cpp), 0x1c and 0x44 bytes (PS2's 0x20
// and 0x60), which the file's copy loops move. Only the debris' frame is known so far (PS2's at 0x10), which Done
// unlinks and releases.
struct S_DEBRIS
{
    I3D_frame *frame;                   // 0x0
    uchar unknown_4[0x18];              // 0x4
};

struct S_FLASH
{
    uchar unknown_0[0x44];              // 0x0
};

// The explosions' debris (PS2 C_Debris, VE_Debris.cpp; the PC's file is the one at 0x564890-0x56615c, which has its
// methods), the game's 0x240, 0x2c bytes. PS2's has its gravity before the vectors. Its constructor and destructor are
// the compiler's.
class C_Debris
{
public:
    // PS2 names: C_game's Init and Done call them. Init takes the gravity's acceleration (the parameter's name is ours)
    bool Init(float gravity);
    // Releases the debris' frames and forgets the debris and the flashes
    bool Done();

    std::vector<S_DEBRIS> debris;       // 0x0 (PS2 0x10, gif::vector<S_DEBRIS>)
    std::vector<S_FLASH> flashes;       // 0x10 (PS2 0x1c)
    S_vector gravity;                   // 0x20 (PS2 0x0), downwards, set by Init (the name is ours)
};

// Its destructor's copy, which the game's destructor and its constructor's unwinding call (Mission.cpp's range), the
// same code as C_using_object's (tools.h), folded
// SYNTHETIC: GAME 0x5479e0
// ??1C_Debris@@QAE@XZ
