#pragma once

#include "actors/C_actor.h"
#include "collision/g_colls.h"

// A bottle's saved game, after the actor's (the name is ours), 0x53 bytes: SaveGameSave writes the members in this
// order. PS2's has the same fields, 0x58 bytes: the version an int, the two bools at the end.
#pragma pack(push, 1)
struct S_bottle_savegame
{
    uchar version;                      // 0x0, BOTTLE_SAVEGAME_VERSION
    S_vector pos;                       // 0x1
    S_vector dir;                       // 0xd
    S_vector move_dir;                  // 0x19
    float speed;                        // 0x25
    float fall_speed;                   // 0x29
    bool set_states;                    // 0x2d
    S_vector hit_normal;                // 0x2e
    float radius;                       // 0x3a
    S_vector axis;                      // 0x3e
    int bounces;                        // 0x4a
    bool hit;                           // 0x4e
    int hit_material;                   // 0x4f
};
#pragma pack(pop)

// The version of the bottle's saved game, the only one SaveGameLoad takes (the name is ours)
#define BOTTLE_SAVEGAME_VERSION 1

// The bottles (PS2 C_bottle, C_bottle.cpp), 0x138 bytes (PS2's 0x190): a thrown object's physics, which the cartridge
// cases and the stones take over. C_mission::CreateActor creates one for ACTOR_BOTTLE.
// VTABLE: GAME 0x6233f0
class C_bottle : public C_actor
{
public:
    C_bottle();
    virtual ~C_bottle();
    // The vtable's other overrides (the slots' names are C_actor's), but the linker's folded constant functions and
    // those that only call the base's
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
    // A player, a bottle or an entity run into the bottle: its motion added to the bottle's (Collision's v2), v1 moved
    // off the last surface the bottle hit
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    // A shot, a stroke or an explosion: v1, made 15 times longer and its y upwards, added to the motion (true for every
    // type, as PS2's)
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);

    // The bottle thrown along v, as fast as v is long, and taken into the game's ticks (PS2 name), which C_game's
    // NewExplosion, NewCartidge and NewGrenade call
    void AbsoluteImpulse(S_vector &v);

    // The motion AI steps (the names are ours): along move_dir at speed and down at fall_speed. AI stops at rest
    // (both at most MRG_ZERO) and where the fall is faster than 50.
    S_vector move_dir;                  // 0x70
    float speed;                        // 0x7c
    float fall_speed;                   // 0x80
    // 0x84, true from GameInit (the cartridges' and the stones' clear it): AI sets the actor's state where it stops,
    // ACTSTATE_INACTIVE at rest and ACTSTATE_NONE for the fast fall, and clears moving where it isn't set (the name is a
    // guess)
    bool set_states;
    uchar unknown_85[3];                // 0x85
    // 0x88, the share of the speed a bounce keeps, .4 from GameInit (the shotgun shells' .2; the name is ours)
    float bounce;
    // 0x8c, the normal of the surface AI's test hit last, null from GameInit: Collision and Hit move the bottle 0.01
    // off it (the name is ours)
    S_vector hit_normal;
    // 0x98, the axis AI turns the bottle about, at its speed: random from GameInit, Collision and Hit (the name is ours)
    S_vector rot_axis;
    // 0xa4, GameInit's from the frame's bounding box (the cartridges' and the stones' .1), which scales the step AI
    // tests for collisions (the name is a guess)
    float radius;
    // 0xa8, the frame's up axis in the world from GameInit, (0, 1, 0) for the cartridges and the stones, which AI turns
    // with dir and Update makes the frame's (the name is a guess)
    S_vector axis;
    // Its constructor clears it before the vtable is stored (PS2's 0x60 bytes at 0x100)
    tDynamicCollObject coll;            // 0xb4 (the name is ours)
    // 0xfc, true from the constructor (the cartridges' and stones' clear it): GameInit fills coll and DynInserts it
    // only where it is set, Update moves it and GameDone DynErases it (the name is ours)
    bool use_coll;
    uchar unknown_fd[3];                // 0xfd
    // AI's collision test of each step (the names are ours): the hits so far (the cartridges stop after 15), whether
    // the last test hit, and the material it hit
    int bounces;                        // 0x100
    bool hit;                           // 0x104
    uchar unknown_105[3];               // 0x105
    // GameInit's copies of the start (the names are guesses): the position, the motion and the fall; nothing in the
    // bottles' files reads them
    S_vector start_pos;                 // 0x108
    S_vector start_motion;              // 0x114, move_dir times speed
    float start_fall_speed;             // 0x120
    int unknown_124;                    // 0x124, 0 from GameInit
    // 0x128, true from the constructor: AI clears it where the bottle stops and set_states isn't set (the name is a
    // guess)
    bool moving;
    uchar unknown_129[3];               // 0x129
    int hit_material;                   // 0x12c, materialList's index
    // 0x130, the actor whose collision AI's test hit, where the collision has COLLHF_UNKNOWN_40, NULL otherwise (the
    // name is ours)
    C_actor *hit_actor;
    // 0x134, false from GameInit (the cartridges' sets it): AI's TestLineV takes every collision where it is set,
    // COLF_UNKNOWN_2 otherwise (the name is a guess)
    bool test_all;
    uchar unknown_135[3];               // 0x135
};

// SYNTHETIC: GAME 0x412390
// C_bottle::`scalar deleting destructor'
