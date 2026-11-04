#pragma once

#include "actors/C_actor.h"

// The rails' turnouts (PS2 C_turnout, C_turnout.cpp), 0x1a0 bytes (PS2's 0x1d0). The constructor, which
// C_mission::CreateActor calls for ACTOR_TURNOUT, is a stub; written: SaveGameGetSize.
// VTABLE: GAME 0x624d58
class C_turnout : public C_actor
{
public:
    C_turnout();
    virtual ~C_turnout();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk);
    virtual bool LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();

    // Switches the turnout (PS2 name), which C_human::Use_Actor calls
    void Start();

    uchar unknown_70[0x130];            // 0x70
};

// SYNTHETIC: GAME 0x4bc470
// C_turnout::`scalar deleting destructor'
