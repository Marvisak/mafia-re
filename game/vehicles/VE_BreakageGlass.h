#pragma once

#include "Main.h"

#include <vector>

class I3D_frame;

// A broken pane's pieces (PS2 S_DICE, VE_BreakageGlass.cpp), 0x18 bytes: the game's destructor of the glass walks the
// vector at its start
struct S_DICE
{
    std::vector<uint> unknown_0;        // 0x0, 4-byte values (PS2's S_DICE_OBJ, most likely)
    uchar unknown_10[8];                // 0x10
};

// The breaking of plain glass (PS2 C_Break_plain_glass, VE_BreakageGlass.cpp; the PC's file is the one at
// 0x5605c0-0x56488c, which has its methods), the game's 0x208, 0x38 bytes. PS2's has 0x10 bytes more before the vectors.
// Only what the game's constructor and destructor use so far: its constructor and destructor are the compiler's.
class C_Break_plain_glass
{
public:
    std::vector<I3D_frame *> frames;    // 0x0 (PS2 0x10, gif::vector<I3D_frame *>)
    std::vector<S_DICE> dices;          // 0x10 (PS2 0x1c)
    uchar unknown_20[0x18];             // 0x20
};

// Its destructor's copy, which the game's destructor and its constructor's unwinding call (Mission.cpp's range)
// SYNTHETIC: GAME 0x5483d0
// ??1C_Break_plain_glass@@QAE@XZ

// The pieces' allocator's destroy, which that destructor calls out of line: an empty ret 4, folded, the original's the
// copy of the range at 0x401030 (docs/game-files.md)
// TEMPLATE: GAME 0x403db0
// ?destroy@?$allocator@I@std@@QAEXPAI@Z
