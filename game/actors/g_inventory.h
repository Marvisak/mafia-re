#pragma once

#include "Main.h"

#include <vector>

// An item an actor carries (PS2 S_GameItem): what it holds isn't known yet
struct S_GameItem
{
};

// An actor's inventory (PS2 G_Inventory, g_inventory.cpp), 0xe4 bytes, the size C_game::InventoryStackPush (0x5b1e10)
// allocates a copy of a human's (C_human's 0x480) with. Only what the game's destructor uses so far; PS2's starts with
// the items, a gif::vector.
class G_Inventory
{
public:
    int unknown_0;                      // 0x0, 1 in a new copy
    int unknown_4;                      // 0x4
    int unknown_8;                      // 0x8
    std::vector<S_GameItem> items;      // 0xc
    uchar unknown_1c[0xc8];             // 0x1c
};

// Its destructor's copy, which C_game::InventoryStackClear calls: the original kept the copy of the range at 0x53e4b0
// (docs/game-files.md) of its folded group
// SYNTHETIC: GAME 0x53ea20
// ??1check_seg@@QAE@XZ
