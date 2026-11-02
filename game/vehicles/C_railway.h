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
    virtual void SaveData(C_chunk *chunk);
    virtual void LoadData(C_chunk *chunk);
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

    uchar unknown_70[0x138];            // 0x70
    // DuplicateFrom's two copies (the names are ours)
    int unknown_1a8;                    // 0x1a8
    uchar unknown_1ac[8];               // 0x1ac
    int unknown_1b4;                    // 0x1b4
    uchar unknown_1b8[0xf4];            // 0x1b8
    // GetMaterialID's (the name is ours)
    int unknown_2ac;                    // 0x2ac
    uchar unknown_2b0[0xc0];            // 0x2b0
};

// SYNTHETIC: GAME 0x486740
// C_railway::`scalar deleting destructor'
