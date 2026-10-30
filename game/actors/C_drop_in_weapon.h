#pragma once

#include "actors/C_PhThings.h"
#include "actors/g_inventory.h"

// A weapon put down, a physical thing the player can pick up (PS2 C_drop_in_weapon, C_drop_in_weapon.cpp), 0x3d4
// bytes (PS2's 0x690). Only the constructor so far, which the script's UNLINKFROMHAND calls, as a stub. Its constructor
// clears the items and the members between them, then sets the type to ACTOR_DROP_IN_WEAPON.
// VTABLE: GAME 0x623c28
class C_drop_in_weapon : public C_PhThing
{
public:
    C_drop_in_weapon();
    virtual ~C_drop_in_weapon();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    // The base's: the original's jumps there
    // FUNCTION: GAME 0x443650 FOLDED
    virtual void AI(uint time)
    {
        C_PhThing::AI(time);
    }
    virtual void Update(uint time);
    // The base's: the original's jumps there, a copy the linker folded
    // FUNCTION: GAME 0x443860 FOLDED
    virtual void NetDirect(uint time)
    {
        C_PhThing::NetDirect(time);
    }
    // The base's: the original's jumps there
    // FUNCTION: GAME 0x443850 FOLDED
    virtual bool Init(I3D_frame *frm)
    {
        return C_PhThing::Init(frm);
    }
    // The base's: the original's jumps there, a copy the linker folded
    // FUNCTION: GAME 0x443860 FOLDED
    virtual void DuplicateFrom(C_actor *act)
    {
        C_PhThing::DuplicateFrom(act);
    }
    virtual void GameInit();
    virtual void GameDone();
    virtual void ChangeState();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm)
    {
        return false;
    }
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm)
    {
        return false;
    }

    S_GameItem item;                    // 0x38c (PS2 0x640), the weapon (the name is ours)
    uchar unknown_39c[0x38];            // 0x39c
};

// SYNTHETIC: GAME 0x443620
// C_drop_in_weapon::`scalar deleting destructor'
