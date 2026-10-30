#pragma once

#include "actors/C_actor.h"

// The race cameras (PS2 C_racecam, c_racecam.cpp), 0x88 bytes (PS2's 0xa0). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_RACECAM.
// VTABLE: GAME 0x625950
class C_racecam : public C_actor
{
public:
    C_racecam();
    virtual ~C_racecam();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual bool Init(I3D_frame *frm);
    virtual void SaveData(C_chunk *chunk);
    virtual void LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize()
    {
        return 0;
    }
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();

    uchar unknown_70[0x18];             // 0x70
};

// SYNTHETIC: GAME 0x596220
// C_racecam::`scalar deleting destructor'
