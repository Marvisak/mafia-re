#pragma once

#include "actors/C_actor.h"

// PS2's C_actor_shot (C_actor_shot.cpp), 0x8c bytes (PS2's 0xa0). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_ACTOR_SHOT.
// VTABLE: GAME 0x625458
class C_actor_shot : public C_actor
{
public:
    C_actor_shot();
    virtual ~C_actor_shot();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void PlayRecord(int a);
    virtual void PlayStep(int a) {}
    virtual bool PlaceAtTime(int a);
    virtual S_vector GetRecordPos(int a);
    virtual S_quat GetRecordRot(int a);
    virtual bool AddToCutscene();
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
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

    uchar unknown_70[0x1c];             // 0x70
};

// SYNTHETIC: GAME 0x566f60
// C_actor_shot::`scalar deleting destructor'
