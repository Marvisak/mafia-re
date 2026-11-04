#pragma once

#include "actors/C_actor.h"

// The objects scattered around a frame (PC only, the name is ours): copies of up to seven of its frame's objects,
// the number of each from 0x84 on, put at random within a radius (0x80) on the ground below, 0xa0 bytes. Only the
// constructor so far, which C_mission::CreateActor calls for ACTOR_SCATTER.
// VTABLE: GAME 0x6257d0
class C_scatter : public C_actor
{
public:
    C_scatter();
    virtual ~C_scatter();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk);
    virtual bool LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize()
    {
        return 0;
    }
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();

    uchar unknown_70[0x30];             // 0x70
};

// SYNTHETIC: GAME 0x58e3a0
// C_scatter::`scalar deleting destructor'
