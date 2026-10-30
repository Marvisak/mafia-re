#pragma once

#include "actors/C_actor.h"
#include "collision/g_colls.h"

// The bottles (PS2 C_bottle, C_bottle.cpp), 0x138 bytes (PS2's 0x190). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_BOTTLE.
// VTABLE: GAME 0x6233f0
class C_bottle : public C_actor
{
public:
    C_bottle();
    virtual ~C_bottle();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    // The base's: the original's jumps there, a copy the linker folded
    // FUNCTION: GAME 0x443860 FOLDED
    virtual void DuplicateFrom(C_actor *act)
    {
        C_actor::DuplicateFrom(act);
    }
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);

    uchar unknown_70[0x44];             // 0x70
    // Its constructor clears it before the vtable is stored (PS2's 0x60 bytes at 0x100)
    tDynamicCollObject coll;            // 0xb4 (the name is ours)
    bool unknown_fc;                    // 0xfc, true from the constructor
    uchar unknown_fd[0x2b];             // 0xfd
    bool unknown_128;                   // 0x128, true from the constructor
    uchar unknown_129[0xf];             // 0x129
};

// SYNTHETIC: GAME 0x412390
// C_bottle::`scalar deleting destructor'
