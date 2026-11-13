#pragma once

#include "actors/C_PhThings.h"
#include "actors/g_inventory.h"
#include "tools.h"

// A weapon put down, a physical thing the player can pick up (PS2 C_drop_in_weapon, C_drop_in_weapon.cpp), 0x3d4
// bytes (PS2's 0x690): the script's UNLINKFROMHAND's and CREATEWEAPONFROMFRAME's, C_human's dropped weapons and the
// boxes the player puts down (GAMEITEM_NONE).
// VTABLE: GAME 0x623c28
class C_drop_in_weapon : public C_PhThing
{
public:
    C_drop_in_weapon();
    virtual ~C_drop_in_weapon();
    // The vtable's other overrides (the slots' names are C_actor's)
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

    // 0x38c (PS2 0x640), the weapon, or a box's GAMEITEM_NONE with its number in ammo_loaded (the name is ours)
    S_GameItem item;
    // 0x39c, a weapon's for the player while the actor is in the game, GameInit's (the name is ours)
    C_using_object::S_using_object using_object;
};

// SYNTHETIC: GAME 0x443620
// C_drop_in_weapon::`scalar deleting destructor'
