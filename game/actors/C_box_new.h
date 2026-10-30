#pragma once

#include "actors/C_PhThings.h"

// The physical objects (PS2 C_box_new, C_box_new.cpp), a physical thing, 0x3a8 bytes (PS2's 0x660). Only the
// constructor so far, which C_mission::CreateActor calls for ACTOR_BOX_NEW.
// VTABLE: GAME 0x6254f0
class C_box_new : public C_PhThing
{
public:
    C_box_new();
    virtual ~C_box_new();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual bool Init(I3D_frame *frm);
    virtual void SaveData(C_chunk *chunk);
    virtual void LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual void GameInit();
    // The base's: the original's jumps there
    // FUNCTION: GAME 0x567720 FOLDED
    virtual void GameDone()
    {
        C_PhThing::GameDone();
    }
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);

    uchar unknown_38c[0x1c];            // 0x38c
};

// SYNTHETIC: GAME 0x567510
// C_box_new::`scalar deleting destructor'
