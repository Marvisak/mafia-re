#pragma once

#include "Main.h"

#include <string.h>
#include <vector>

// An item an actor carries (PS2 S_GameItem), 0x10 bytes (std::vector<S_GameItem>'s size divides by 16): the script's
// HUMAN_ADDWEAPON fills the first three, its CREATEWEAPONFROMFRAME all four (the names are ours but for id)
struct S_GameItem
{
    ushort id;                          // 0x0, the item's index in gameItemTable (g_material.h)
    int unknown_4;                      // 0x4
    int unknown_8;                      // 0x8
    void *unknown_c;                    // 0xc, the using object of a weapon put down, NULL in a hand

    // Cleared, as PS2's memset before an item is filled: G_Inventory's constructor constructs its arrays through it
    // FUNCTION: GAME 0x47aed0
    S_GameItem()
    {
        memset(this, 0, sizeof(*this));
    }
};

// An actor's inventory (PS2 G_Inventory, g_inventory.cpp), 0xe4 bytes, the size C_game::InventoryStackPush (0x5b1e10)
// allocates a copy of a human's (C_human's 0x480) with. PS2's starts with the items, a gif::vector. The functions the
// script calls are stubs, in g_loadsave.cpp's range on the PC (ui/g_loadsave.cpp).
class G_Inventory
{
public:
    // C_human's constructor inlines it, the items constructed and Clear called out of line
    G_Inventory()
    {
        Clear();
    }

    // PS2 names, which the script calls; the parameters' names are ours. The items the hands drop go into the vector.
    // Selects the item for the hand
    bool SelectByID(uint id, std::vector<S_GameItem> *dropped);
    // Whether the hand's item, or without in_hand any item, is of gameItemTable's flags
    bool HaveSomeWeapon(bool in_hand, uint flags);
    bool IsIn(uint id, bool unknown);
    bool HaveSpaceFor(uint id, bool unknown);
    bool Insert(S_GameItem *item, bool unknown);
    bool Remove(S_GameItem *item, std::vector<S_GameItem> *dropped);
    void SetCanHaveBigWeapon(bool can, std::vector<S_GameItem> *dropped);

    int unknown_0;                      // 0x0, 1 from Clear
    int unknown_4;                      // 0x4
    uint unknown_8;                     // 0x8, how many of unknown_90 hold items
    std::vector<S_GameItem *> items;    // 0xc, 4-byte elements (Clear's erase), pointers most likely
    uint unknown_1c;                    // 0x1c, 0xffffff from Clear
    S_GameItem right_hand;              // 0x20, the item in the right hand (the name is ours)
    S_GameItem unknown_30[5];           // 0x30
    S_GameItem unknown_80;              // 0x80
    S_GameItem unknown_90[4];           // 0x90
    S_GameItem unknown_d0;              // 0xd0
    bool unknown_e0;                    // 0xe0

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // Empties the inventory, the script's INVENTORY_CLEAR (PS2's goes on with an empty item and CyclePrepare)
    // FUNCTION: GAME 0x47b020
    void Clear()
    {
        unknown_0 = 1;
        unknown_4 = 0;
        unknown_8 = 0;
        unknown_1c = 0xffffff;
        memset(&right_hand, 0, sizeof(right_hand));
        memset(unknown_30, 0, sizeof(unknown_30));
        memset(&unknown_80, 0, sizeof(unknown_80));
        memset(unknown_90, 0, sizeof(unknown_90));
        memset(&unknown_d0, 0, sizeof(unknown_d0));
        items.clear();
        unknown_e0 = false;
    }
    // The right hand's item's id, and the item where asked for: a uint, which the copy widens (xor eax, eax; mov ax)
    // and the script's HUMAN_GETITEMINRHAND converts as unsigned (fild qword)
    // FUNCTION: GAME 0x47b0b0
    uint GetItemInRightHand(S_GameItem **item)
    {
        if (item)
            *item = &right_hand;
        return right_hand.id;
    }
};

// Its destructor's copy, which C_game::InventoryStackClear calls: the original kept the copy of the range at 0x53e4b0
// (docs/game-files.md) of its folded group
// SYNTHETIC: GAME 0x53ea20
// ??1check_seg@@QAE@XZ
