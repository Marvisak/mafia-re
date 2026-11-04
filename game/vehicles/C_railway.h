#pragma once

#include "actors/C_actor.h"

// The rail vehicles (PS2 C_railway, with doors, windows and seats; C_railway.cpp, or C_rail_traffic.cpp, whose seats
// they use), 0x370 bytes (PS2's 0x390). The constructor, which C_mission::CreateActor calls for ACTOR_RAILWAY, is a
// stub; written: DuplicateFrom, SaveGameGetSize and GetMaterialID.
// VTABLE: GAME 0x624a70
class C_railway : public C_actor
{
public:
    C_railway();
    virtual ~C_railway();
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
    virtual void GetActorSpeed(S_vector &speed);
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);
    virtual int GetMaterialID(I3D_frame *frm);
    virtual bool IsPossibleColl(C_actor *act, S_vector v1, S_vector v2, float f, float &dist, C_actor **coll);
    virtual bool GetBoundPoints2D(std::vector<S_vector2> &points, I3D_frame *frm);
    // The player's sounds of the vehicle, inside and outside, as he gets in and out (PS2 names, C_human::Intern_UseRailway)
    void RemoveOuterEntries();
    void AddInnerEntries();
    void RemoveInnerEntries();
    void AddOuterEntries();
    // Counts lock_count down to 0 (the name is a guess: PS2's Intern_UseRailway calls ai_unlock_manager::Unlock at
    // its place)
    void Unlock();
    // PS2 name; the parameters' names are ours
    void SetRainDummy(bool in, I3D_frame *frm);
    // The wagon's frame, from 1 (C_human::wagon), the vehicle's own for 0, NULL past the last (PS2 name)
    I3D_frame *GetVagon(uint wagon);
    // The entry nearest the actor: its position and direction, and the wagon it leads into, from 1, 0 for the
    // vehicle itself (PS2 name; the parameters' names are ours); false for none
    bool GetEntry(C_actor *act, S_vector *pos, S_vector *dir, uint *wagon);
    // Counts lock_count up (PS2 name), which Unlock counts down
    void Lock();

    uchar unknown_70[0x54];             // 0x70
    // The wagons' frames, the first wagon's [0]: C_human::wagon 1 and up indexes it from 1 (the name is ours)
    I3D_frame **wagon_frames;           // 0xc4
    uchar unknown_c8[0x34];             // 0xc8
    // The dynamic collisions in the vehicle, the vehicle's own [0], then one a wagon, which C_human::Intern_UseRailway
    // puts the human's into (the name is ours)
    std::vector<tDynamicCollObject *> *dyn_colls;  // 0xfc
    uchar unknown_100[0xa8];            // 0x100
    // DuplicateFrom's two copies
    int unknown_1a8;                    // 0x1a8
    uchar unknown_1ac[8];               // 0x1ac
    int unknown_1b4;                    // 0x1b4
    uchar unknown_1b8[0x20];            // 0x1b8
    // 0x1d8 (PS2 0x368): set, C_human::Use_Actor walks the human in or out along the frame's direction (animation 6
    // for 800 ms), else plays animation 0x140 or 0x141
    bool unknown_1d8;
    uchar unknown_1d9[0xaf];            // 0x1d9
    int lock_count;                     // 0x288, Unlock's (the name is ours)
    uchar unknown_28c[0x20];            // 0x28c
    // GetMaterialID's, a materials.def index (materialList's; the name is ours)
    int material;                       // 0x2ac (PS2 0x330)
    uchar unknown_2b0[0xc0];            // 0x2b0
};

// SYNTHETIC: GAME 0x486740
// C_railway::`scalar deleting destructor'
