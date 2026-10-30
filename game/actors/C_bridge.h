#pragma once

#include "actors/C_actor.h"

// The bridges (PS2 C_bridge, C_bridge.cpp), 0x194 bytes (PS2's 0x1a0). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_BRIDGE, and ShutDown, which the script calls, as stubs.
// VTABLE: GAME 0x625588
class C_bridge : public C_actor
{
public:
    C_bridge();
    virtual ~C_bridge();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual void SaveData(C_chunk *chunk) {}
    virtual void LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act) {}
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf)
    {
        return true;
    }
    virtual void GameInit();
    virtual void GameDone();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);

    // The bridge shut down or not (PS2 name): the flag stored, the bridge's collision taken from the game's
    void ShutDown(bool shut);

    uchar unknown_70[0x124];            // 0x70
};

// SYNTHETIC: GAME 0x567e00
// C_bridge::`scalar deleting destructor'
