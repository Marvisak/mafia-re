#pragma once

#include "actors/C_PhThings.h"

// The version of the scene's data SaveData writes; LoadData reads it and the two before it (the name is ours)
#define BOX_NEW_DATA_VERSION 2

// The physical objects (PS2 C_box_new, C_box_new.cpp), a physical thing that moves as a box or as a wheel, from the
// scene's data, 0x3a8 bytes (PS2's 0x660). C_mission::CreateActor creates them for ACTOR_BOX_NEW.
// VTABLE: GAME 0x6254f0
class C_box_new : public C_PhThing
{
public:
    C_box_new();
    virtual ~C_box_new();
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk);
    virtual bool LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual void GameInit();
    virtual void GameDone();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);

    // The scene's data, the members in the order SaveData writes them, Init's defaults (the names are ours)
    bool is_box;                        // 0x38c, a box (InitBox), else a wheel (InitWheel)
    float radius;                       // 0x390, the wheel's, 1
    float width;                        // 0x394, the wheel's, 1
    // 0x398, the box's and the wheel's: 10, or 50 for each cubic metre of the frame's bound box where that lies
    // between .01 and 1000; LoadData keeps it within .01 and 100000
    float weight;
    float friction;                     // 0x39c, the box's and the wheel's, .5
    float rolling_friction;             // 0x3a0, the wheel's, .01
    bool hit;                           // 0x3a4, set by Collision and Hit, cleared by GameInit (the name is ours)
};

// SYNTHETIC: GAME 0x567510
// C_box_new::`scalar deleting destructor'
