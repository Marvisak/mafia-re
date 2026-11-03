#pragma once

#include "actors/C_actor.h"

// The airplanes (PS2 C_airplane, C_airplane.cpp), 0x3f0 bytes (PS2's 0x400). Written: the destructor, which frees
// its two vectors, SaveGameGetSize, GetMaterialID, IsDestroyed and ShowDamageIndicator; the constructor, which
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

    uchar unknown_70[0x38];             // 0x70
    // ShowDamageIndicator's divisor, the two engines' full energy (the name is ours)
    float unknown_a8;                   // 0xa8
    uchar unknown_ac[0x6c];             // 0xac
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
