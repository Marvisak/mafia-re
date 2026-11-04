#pragma once

#include "actors/C_actor.h"
#include "collision/g_colls.h"

// The bottles (PS2 C_bottle, C_bottle.cpp), 0x138 bytes (PS2's 0x190). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_BOTTLE.
// VTABLE: GAME 0x6233f0
class C_bottle : public C_actor
{
public:
    C_bottle();
    virtual ~C_bottle();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    // The base's: the original's jumps there, a copy the linker folded
    // FUNCTION: GAME 0x443860 FOLDED
    virtual void DuplicateFrom(C_actor *act)
    {
        C_actor::DuplicateFrom(act);
    }
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);

    // The motion AI steps (the names are ours): along move_dir at speed and down at fall_speed. AI stops at rest
    // (both at most a small value) and where the fall is faster than 50.
    S_vector move_dir;                  // 0x70
    float speed;                        // 0x7c
    float fall_speed;                   // 0x80
    // 0x84, true from GameInit (the cartridges' clears it): AI sets the actor's state where it stops, ACTSTATE_INACTIVE
    // at rest and ACTSTATE_NONE for the fast fall (the name is a guess)
    bool set_states;
    uchar unknown_85[3];                // 0x85
    // 0x88, the share of the speed a bounce keeps, .4 from GameInit (the shotgun shells' .2; the name is ours)
    float bounce;
    uchar unknown_8c[0x18];             // 0x8c
    // 0xa4, GameInit's from the frame's bounding box (the cartridges' .1), which scales the step AI tests for
    // collisions (the name is a guess)
    float radius;
    // 0xa8, the frame's up axis in the world from GameInit, (0, 1, 0) for the cartridges (the name is a guess)
    S_vector axis;
    // Its constructor clears it before the vtable is stored (PS2's 0x60 bytes at 0x100)
    tDynamicCollObject coll;            // 0xb4 (the name is ours)
    // 0xfc, true from the constructor (the cartridges' and stones' clear it): GameInit fills coll and DynInserts it
    // only where it is set (the name is ours)
    bool use_coll;
    uchar unknown_fd[3];                // 0xfd
    // AI's collision test of each step (the names are ours): the hits so far (the cartridges stop after 15), whether
    // the last test hit, and the material it hit
    int bounces;                        // 0x100
    bool hit;                           // 0x104
    uchar unknown_105[0x23];            // 0x105
    bool unknown_128;                   // 0x128, true from the constructor
    uchar unknown_129[3];               // 0x129
    int hit_material;                   // 0x12c, materialList's index
    uchar unknown_130[4];               // 0x130
    // 0x134, false from GameInit (the cartridges' sets it): AI's TestLineV takes every collision where it is set, the
    // flags 2 otherwise (the name is a guess)
    bool test_all;
    uchar unknown_135[3];               // 0x135
};

// SYNTHETIC: GAME 0x412390
// C_bottle::`scalar deleting destructor'
