#pragma once

#include "actors/C_actor.h"

// The objects of dynamic parts (PC only, the name is ours: it takes its model's frames named dynA, dynB and dynS,
// a part of 0xc8 bytes each from 0x130 on), 0xc9c bytes. Only the constructor so far, which C_mission::CreateActor
// calls for ACTOR_DYN_OBJECT.
// VTABLE: GAME 0x6234f8
class C_dyn_object : public C_actor
{
public:
    C_dyn_object();
    virtual ~C_dyn_object();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual int vfunc_0(int a);
    virtual void vfunc_1(int a);
    virtual void vfunc_2(int a);
    virtual S_vector vfunc_4(int a);
    virtual S_vector vfunc_5(int a);
    virtual bool vfunc_9();
    virtual void AI(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual void SaveData(C_chunk *chunk);
    virtual void LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act) {}
    virtual void GameInit();
    virtual void GameDone();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);

    uchar unknown_70[0xc2c];            // 0x70
};

// SYNTHETIC: GAME 0x4189b0
// C_dyn_object::`scalar deleting destructor'
