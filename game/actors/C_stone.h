#pragma once

#include "actors/C_bottle.h"

// The stones (PC only; the names are ours), ACTOR_STONE, 0x138 bytes: a bottle's physics. C_game::Init creates the game's
// from 2stone.i3d. Only the constructor so far.
// VTABLE: GAME 0x624c00
class C_stone : public C_bottle
{
public:
    C_stone();
    virtual ~C_stone();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    // The base's: the original's jumps there, a copy the linker folded
    // FUNCTION: GAME 0x443860 FOLDED
    virtual void NetDirect(uint time)
    {
        C_bottle::NetDirect(time);
    }
    virtual bool Init(I3D_frame *frm);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf)
    {
        return true;
    }
    virtual bool SaveGameLoad(void **buf)
    {
        return true;
    }
    virtual void GameInit();
    virtual void GameDone();
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
};

// SYNTHETIC: GAME 0x4aa2e0
// C_stone::`scalar deleting destructor'
