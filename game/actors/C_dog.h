#pragma once

#include "actors/C_actor.h"

// The dogs (PS2 C_dog, C_dog.cpp), 0x310 bytes (PS2's 0x350). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_DOG.
// VTABLE: GAME 0x623a70
class C_dog : public C_actor
{
public:
    C_dog();
    virtual ~C_dog();
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
    virtual void Death();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);
    virtual void cbCollTester(tDynamicCollObject *obj);

    // The dogs' animations' tracks (PS2 name)
    static void LoadTCK();

    uchar unknown_70[0x2a0];            // 0x70
};

// SYNTHETIC: GAME 0x434230
// C_dog::`scalar deleting destructor'
