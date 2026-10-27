#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class I3D_frame;

// A piece of a broken pane (PS2 S_DICE_OBJ, most likely), 0x34 bytes (PS2's 0x40, its frame at 0x30): only its frame is
// known so far, which Done releases
struct S_DICE_OBJ
{
    I3D_frame *frame;                   // 0x0
    uchar unknown_4[0x30];              // 0x4
};

// A broken pane's pieces (PS2 S_DICE, VE_BreakageGlass.cpp), 0x18 bytes: the frames Done unlinks and releases, and the
// pieces (PS2 keeps both as arrays with their counts, 0x10 bytes). Its constructor is the compiler's: the vector's.
struct S_DICE
{
    std::vector<I3D_frame *> frames;    // 0x0 (PS2 0x0, with its count at 0x4)
    S_DICE_OBJ *objs;                   // 0x10 (PS2 0x8)
    int num_objs;                       // 0x14 (PS2 0xc)
};

// The breaking of plain glass (PS2 C_Break_plain_glass, VE_BreakageGlass.cpp; the PC's file is the one at
// 0x5605c0-0x56488c, which has its methods), the game's 0x208, 0x38 bytes. PS2's has its gravity before the vectors.
// Its constructor and destructor are the compiler's. The members' names are ours.
class C_Break_plain_glass
{
public:
    // PS2 names: C_game's Init and Done call them. Init takes how many panes the pool holds (PS2's takes it unused)
    bool Init(int num);
    // Releases the frames and the pieces and forgets them
    bool Done();

    std::vector<I3D_frame *> frames;    // 0x0 (PS2 0x10, gif::vector<I3D_frame *>)
    std::vector<S_DICE> dices;          // 0x10 (PS2 0x1c)
    S_DICE *pool;                       // 0x20, Init's array (PS2 0x28, which Init clears)
    int unknown_24;                     // 0x24, Init sets it to the pool's size (PS2 0x2c)
    int pool_size;                      // 0x28 (PS2 0x30)
    S_vector gravity;                   // 0x2c (PS2 0x0), downwards, set by Init
};

// The pieces' compiler-generated constructor, which Init's new calls, and assignment, which Done's clear calls (Init's
// inlines it, calling the frames' assignment, this file's copy, which the linker folded with the other vectors of
// pointers')
// SYNTHETIC: GAME 0x560f20
// S_DICE::S_DICE

// SYNTHETIC: GAME 0x562000
// S_DICE::operator=

// TEMPLATE: GAME 0x563b40
// std::vector<C_actor *,std::allocator<C_actor *> >::operator=

// Its destructor's copy, which the game's destructor and its constructor's unwinding call (Mission.cpp's range)
// SYNTHETIC: GAME 0x5483d0
// ??1C_Break_plain_glass@@QAE@XZ

// The frames' allocator's destroy, which that destructor calls out of line: an empty ret 4, folded, the original's the
// copy of the range at 0x401030 (docs/game-files.md)
// TEMPLATE: GAME 0x403db0
// std::allocator<S_cache_model_one>::destroy
