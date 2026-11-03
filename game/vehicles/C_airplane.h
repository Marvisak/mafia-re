#pragma once

#include "actors/C_actor.h"

// The airplanes (PS2 C_airplane, C_airplane.cpp), 0x3f0 bytes (PS2's 0x400). Written: the destructor, which frees
// its two vectors, SaveGameGetSize, GetMaterialID, IsDestroyed, ShowDamageIndicator, SaveData and LoadData; the constructor, which
// C_mission::CreateActor calls for ACTOR_AIRPLANE, and the scripts' functions are stubs.
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
    virtual bool SaveData(C_chunk *chunk);
    virtual bool LoadData(C_chunk *chunk);
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

    // SaveData's values (the names are ours)
    int unknown_70;                     // 0x70
    int unknown_74;                     // 0x74
    int unknown_78;                     // 0x78
    int unknown_7c;                     // 0x7c
    int unknown_80;                     // 0x80
    int unknown_84;                     // 0x84
    int unknown_88;                     // 0x88
    int unknown_8c;                     // 0x8c
    S_vector unknown_90;                // 0x90
    int unknown_9c;                     // 0x9c
    int unknown_a0;                     // 0xa0
    int unknown_a4;                     // 0xa4
    float unknown_a8;                   // 0xa8
    int unknown_ac;                     // 0xac
    int unknown_b0;                     // 0xb0
    int unknown_b4;                     // 0xb4
    uchar unknown_b8[0x60];             // 0xb8
    // The two vectors the destructor frees (the names are ours)
    std::vector<uint> unknown_118;      // 0x118
    std::vector<uint> unknown_128;      // 0x128
    uchar unknown_138[0x32];            // 0x138
    // ShowDamageIndicator's, whether the HUD's bar is on (the name is ours)
    bool damage_indicator;              // 0x16a
    uchar unknown_16b[0x5d];            // 0x16b
    // The two engines' energies (PS2 0x37c and 0x380, the names are ours): PS2's Hit lowers them down to 0, its
    // UpdateParticles smokes each at GetEnginesPos' engine below half; the plane is destroyed with both under 0.01
    float engine_energy_1;              // 0x1c8
    float engine_energy_2;              // 0x1cc
    uchar unknown_1d0[0x220];           // 0x1d0
};

// SYNTHETIC: GAME 0x409500
// C_airplane::`scalar deleting destructor'
