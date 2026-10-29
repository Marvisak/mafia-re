#pragma once

#include "actors/C_PhThings.h"
#include "actors/g_inventory.h"

// A weapon put down, a physical thing the player can pick up (PS2 C_drop_in_weapon, C_drop_in_weapon.cpp), 0x3d4
// bytes (PS2's 0x690). Only the constructor so far, which the script's UNLINKFROMHAND calls, as a stub. Its constructor
// clears the items and the members between them, then sets the type to ACTOR_DROP_IN_WEAPON.
class C_drop_in_weapon : public C_PhThing
{
public:
    C_drop_in_weapon();

    S_GameItem item;                    // 0x38c (PS2 0x640), the weapon (the name is ours)
    uchar unknown_39c[0x38];            // 0x39c
};
