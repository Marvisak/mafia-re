#pragma once

#include "actors/C_actor.h"

// PS2's C_irenka (C_irenka.cpp, a Czech name; what it is isn't known yet), 0x10c bytes (PS2's 0x140). Only the
// constructor so far, which C_mission::CreateActor calls for ACTOR_IRENKA.
// VTABLE: GAME 0x623e08
class C_irenka : public C_actor
{
public:
    C_irenka();
    virtual ~C_irenka();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
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

    uchar unknown_70[0x9c];             // 0x70
};

// SYNTHETIC: GAME 0x445880
// C_irenka::`scalar deleting destructor'
