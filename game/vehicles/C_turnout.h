#pragma once

#include "actors/C_actor.h"

// The rails' turnouts (PS2 C_turnout, C_turnout.cpp), 0x1a0 bytes (PS2's 0x1d0). Written: SaveGameGetSize,
// SaveGameSave and DuplicateFrom; the constructor, which C_mission::CreateActor calls for ACTOR_TURNOUT, is a stub.
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
    virtual void SaveData(C_chunk *chunk);
    virtual void LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();

    // DuplicateFrom's values (the names are ours)
    uint unknown_70;                    // 0x70
    uint unknown_74;                    // 0x74
    uint unknown_78;                    // 0x78
    uint unknown_7c;                    // 0x7c
    uint unknown_80;                    // 0x80
    uint unknown_84;                    // 0x84
    char unknown_88[0x20];              // 0x88
    char unknown_a8[0x20];              // 0xa8
    bool unknown_c8;                    // 0xc8
    uchar unknown_c9[0x7b];             // 0xc9
    // SaveGameSave's two values (the names are ours)
    uint unknown_144;                   // 0x144
    uchar unknown_148[0x48];            // 0x148
    uint unknown_190;                   // 0x190
    uchar unknown_194[0xc];             // 0x194
};

// SYNTHETIC: GAME 0x4bc470
// C_turnout::`scalar deleting destructor'
