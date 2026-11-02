#pragma once

#include "actors/C_actor.h"

// The airplanes (PS2 C_airplane, C_airplane.cpp), 0x3f0 bytes (PS2's 0x400). The constructor, which
// C_mission::CreateActor calls for ACTOR_AIRPLANE, is a stub; written: SaveGameGetSize, GetMaterialID and IsDestroyed.
// VTABLE: GAME 0x623318
class C_airplane : public C_actor
{
public:
    C_airplane();
    virtual ~C_airplane();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual void SaveData(C_chunk *chunk);
    virtual void LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();
    virtual void ChangeState();
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);
    virtual int GetMaterialID(I3D_frame *frm);

    // Takes off (PS2 name): the scripts' AIRPLANE_START gives the speed in m/s
    void Start(uint unknown, float speed);
    // PS2 name, the scripts' AIRPLANE_START2
    void Start2();
    // PS2 name; the scripts' AIRPLANE_ISDESTROYED
    bool IsDestroyed();
    // Shows the damage on the HUD's bar or hides it (PS2 name); the scripts' AIRPLANESHOWDAMAGE
    void ShowDamageIndicator(bool show);

    uchar unknown_70[0x158];            // 0x70
    // IsDestroyed's two values, both under 0.01 when it is (the names are ours)
    float unknown_1c8;                  // 0x1c8
    float unknown_1cc;                  // 0x1cc
    uchar unknown_1d0[0x220];           // 0x1d0
};

// SYNTHETIC: GAME 0x409500
// C_airplane::`scalar deleting destructor'
