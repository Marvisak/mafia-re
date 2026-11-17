#pragma once

#include "actors/C_actor.h"
#include "collision/g_colls.h"

// A physical thing's box (PS2 S_THING_BOX, InitBox's; the members' names are ours), 0x14 bytes, PS2's in another
// order. Its constructor is the implicit one, which S_vector's empty constructor makes the original call (the folded
// 0x401c60).
struct S_THING_BOX
{
    float weight;                       // 0x0, the script's CREATE_PHYSICALOBJECT gives at least 1 (InitBox's 0x210)
    S_vector velocity;                  // 0x4, (0, -speed, 0) from the scripts (InitBox's 0x220)
    float friction;                     // 0x10, .45 from the scripts (InitBox's 0x238)
};

// A physical thing's wheel (PS2 S_THING_WHEEL, InitWheel's; the members' names are ours), 0x20 bytes, PS2's with the
// velocity first. C_box_new's GameInit fills it from its scene data.
struct S_THING_WHEEL
{
    float radius;                       // 0x0, InitWheel's 0x260, whose square times the weight it divides by
    float width;                        // 0x4, InitWheel's 0x25c
    float weight;                       // 0x8, InitWheel's 0x210
    S_vector velocity;                  // 0xc, InitWheel's 0x220
    float friction;                     // 0x18, InitWheel's 0x238, as the box's
    float rolling_friction;             // 0x1c, InitWheel's 0x264
};

// C_PhThing::thing_type (the names are ours): AI moves a wheel by MoveWheel and anything else by MoveBox
#define PHTHING_WHEEL 1                 // InitWheel's
#define PHTHING_BOX 2                   // InitBox's

// The points of a thing's box (C_PhThing::box_points; the name is ours): its eight corners and the six faces' centres
#define PHTHING_BOX_POINTS 14

// A saved physical thing, 0xd3 bytes after the actor's (the names are ours, after the members): PS2's has the same
// fields but the matrix, in other places
#pragma pack(push, 1)
struct S_phthing_savegame
{
    ushort version;                     // 0x0, PHTHING_SAVEGAME_VERSION
    S_vector pos;                       // 0x2, the frame's position
    S_vector center_offset;             // 0xe
    S_quat rot;                         // 0x1a, the frame's rotation
    int thing_type;                     // 0x2a
    bool unknown_2e;                    // 0x2e, always true
    float max_step;                     // 0x2f
    float weight;                       // 0x33
    S_vector inv_inertia;               // 0x37
    S_matrix unknown_43;                // 0x43, SaveGameSave writes a local it never sets, SaveGameLoad doesn't read it
    S_vector velocity;                  // 0x83
    S_vector rot_velocity;              // 0x8f
    float friction;                     // 0x9b
    float bounce;                       // 0x9f
    float contact_reach;                // 0xa3, which SaveGameLoad also takes for contact_dist
    float stiffness;                    // 0xa7
    float unknown_258;                  // 0xab
    float width;                        // 0xaf
    float radius;                       // 0xb3
    float rolling_friction;             // 0xb7
    float spring_force;                 // 0xbb
    S_vector contact_pos;               // 0xbf
    float unknown_cb;                   // 0xcb, contact_reach again, which SaveGameLoad doesn't read
    float rest_time;                    // 0xcf
};
#pragma pack(pop)

// The version of the saved game's data SaveGameSave writes, the only one SaveGameLoad takes (the name is ours)
#define PHTHING_SAVEGAME_VERSION 4

// The physical things (PS2 C_PhThing, C_PhThings.cpp), the base of the public physics, of C_box_new, of C_crash_obj
// and of C_drop_in_weapon, 0x38c bytes: a box or a wheel, a rigid body that AI moves in steps of at most max_step
// (MoveBox, MoveWheel) and that comes to rest after a while still.
// VTABLE: GAME 0x624088
class C_PhThing : public C_actor
{
public:
    // An actor the thing touched (PS2 C_PhThing::S_SURR_ACT; the members' names are ours), 0x14 bytes: MoveBox adds
    // one with the time 1000 or sets an existing one's time back to it, Update counts the times down and wakes the
    // thing at rest where one of them moved, InvalidateActor takes one out
    struct S_SURR_ACT
    {
        S_vector pos;                   // 0x0, the actor's world position when it was added
        C_actor *act;                   // 0xc
        int time;                       // 0x10
    };

    C_PhThing();
    virtual ~C_PhThing();
    // The vtable's other overrides (the slots' names are C_actor's)
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();
    virtual void ChangeState();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);

    // PS2 names, the types PS2's; the parameters' names are ours
    // Runs a step of the thing at rest to find the actors it touches, and wakes it up where it touches any
    void FindActors();
    // A step of the physics, by the thing's type
    void MoveWheel(float time);
    void MoveBox(float time);
    // Turns the thing by its rotation velocity over the time and moves it by move
    void MoveCount(float time, const S_vector &move);
    // A contact's response: the velocity changes (into vel and rot_vel) of the point's impulse along the normal and
    // of its friction; time and act aren't used
    void CollisionCount(S_vector &vel, S_vector &rot_vel, S_vector &normal, S_vector &point, float time, ushort index,
        C_actor *act);
    // The thing's box, its state set by the velocity
    void InitBox(S_THING_BOX *box);
    // The same for a wheel (C_box_new's)
    void InitWheel(S_THING_WHEEL *wheel);
    // A world point's position from the centre, its velocity, the weight and the rotation an impulse along axis there
    // makes (another file's, 0x425189)
    void GetRVelMassJ(const S_vector &point, const S_vector &axis, S_vector *rel, S_vector *point_vel, float *mass,
        S_vector *rot_response);
    // An impulse at a world point
    void ExternalImpuls(const S_vector &impuls, const S_vector &point);
    // Forgets an actor (PS2 name), which C_game::InvalidateActor calls
    void InvalidateActor(C_actor *act);
    // A line test's hit (PS2's parameter types): its collision, NULL for the drowning water (MAT_WATER_DEATH), which
    // the things fall through; car says whether it is a moving object a car owns
    g_collision_header *CollisionFilter(uint *obj, S_vector &point, S_vector &normal, bool &car);
    // The matrices, the centre and the axes taken again from the frame
    void UpdateImportantVariables();

    int thing_type;                     // 0x70, PHTHING_*, 0 from the constructor (the name is ours)
    // 0x74, MoveBox's: every point is tested, not only those moving away from the centre, every other step and while
    // the thing is slow (the name is a guess)
    bool test_all_points;
    // 0x75, ChangeState starts the rest again on a wake while it is set; FindActors clears it, so the thing it wakes
    // stays at rest (the name is ours)
    bool wake_resets_rest;
    // 0x76, false from the constructor, set by InitBox: center_offset is the box's centre in the frame's space (the
    // name is ours)
    bool has_center_offset;
    S_vector center_offset;             // 0x78, the frame's space (the name is ours)
    S_vector center;                    // 0x84, center_offset in the world, or pos without one (the name is ours)
    // 0x90, where the thing has stayed within .2 of, and for how long (ms): over 4000 it is at rest (the names are
    // ours)
    S_vector rest_pos;
    uint still_time;
    // 0xa0, the world matrix's first and second rows (the third is dir; the names are ours): a wheel's axle and up
    S_vector dir_x;
    S_vector dir_y;
    // 0xb8, the thing's collision (DYNCOLL_UNKNOWN_40, its box the frame's), which GameInit puts in and GameDone takes
    // out (the name is ours)
    tDynamicCollObject coll;
    // 0x100, the normal of the surface the last line test hit, (0, 1, 0) from the constructor (the name is ours)
    S_vector contact_normal;
    // 0x10c, the frame's world matrix, and its inverse; the same without the translation (the names are ours)
    S_matrix matrix;
    S_matrix inv_matrix;                // 0x14c
    S_matrix rot_matrix;                // 0x18c
    S_matrix inv_rot_matrix;            // 0x1cc
    // 0x20c, AI's longest step of the physics, in seconds: .028 from the constructor (the name is ours)
    float max_step;
    float weight;                       // 0x210, InitBox's and InitWheel's (the name is ours)
    // 0x214, the inertia's reciprocal per axis, in the thing's space: a wheel's 1 / (weight * radius² / 2) (the name
    // is ours)
    S_vector inv_inertia;
    S_vector velocity;                  // 0x220, InitBox's and InitWheel's (the name is ours)
    S_vector rot_velocity;              // 0x22c, in the world's axes, 0 from the constructor (the name is ours)
    // The contacts' response (the names are ours): the friction (S_THING_BOX's, S_THING_WHEEL's), 1 + the bounce's
    // restitution (a box's 1.01, a wheel's 1.05), how far from a surface counts as touching (.04, .03) and the
    // spring's stiffness against going nearer (the weight's force over twice that distance)
    float friction;                     // 0x238
    float bounce;                       // 0x23c
    float contact_reach;                // 0x240
    float stiffness;                    // 0x244
    // 0x248, the weight's force, (0, -weight, 0) times gravity from InitBox and InitWheel; C_box_new's GameInit sets
    // (0, -1, 0) before them (the name is ours)
    S_vector gravity_force;
    float gravity;                      // 0x254, 10 from the constructor (the name is ours)
    float unknown_258;                  // 0x258, a box's 4 times its weight
    // 0x25c, a wheel's (S_THING_WHEEL's; the names are ours): half the width moves its contact along the axle, the
    // rolling friction times the weight's force over the radius brakes it
    float width;
    float radius;                       // 0x260
    float rolling_friction;             // 0x264
    S_vector box_points[PHTHING_BOX_POINTS];    // 0x268, InitBox's, in the frame's space (the name is ours)
    ushort contact_mask;                // 0x310, MoveBox's: a bit for each of box_points that went in (the name is ours)
    // 0x314, a wheel's spring force from the last step's contact, 0 without one, the contact point in the world and
    // its distance from the surface (contact_reach for none; MoveBox's last point's) (the names are ours)
    float spring_force;
    S_vector contact_pos;               // 0x318
    float contact_dist;                 // 0x324
    uchar unknown_328[4];               // 0x328
    // 0x32c, how long the thing has been slow, in seconds: AI moves it while that is at most rest_limit (the names
    // are ours)
    float rest_time;
    float rest_limit;                   // 0x330, .6 from GameInit
    // 0x334, 1 from GameInit; AI sets it to its time's share above 83 ms (the name is ours)
    float time_scale;
    I3D_sound *sound;                   // 0x338, the hit sound's, GameInit's, which GameDone releases (the name is ours)
    // 0x33c, the point of the step's strongest impulse, where the hit sound plays (the name is ours)
    S_vector hit_point;
    // 0x348, the sound of the thing hitting a surface: an index of its material's sounds from S_material_def's 0x130
    // (grenade_sound the first), -1 for none; the items C_human drops by their kind, 8 for a clip (the name is ours)
    int hit_sound;
    int material;                       // 0x34c, an index of materialList, the shots' sound's (the name is ours)
    // 0x350, the surface's material each contact touched (a box's by its points, a wheel's the first), cleared by
    // GameInit; the hit sound takes that of the step's strongest impulse (the names are ours)
    ushort contact_materials[PHTHING_BOX_POINTS];
    ushort hit_index;                   // 0x36c, its contact
    float hit_impulse;                  // 0x370, its size, which Update clears
    uint hit_sound_time;                // 0x374, since the hit sound started, in ms: a sound restarts after 150
    // 0x378, true from the constructor: a hit cuts its sound short to play it again; false lets it finish (the name is
    // ours)
    bool hit_sound_restart;
    // 0x37c, the actors the thing touched (PS2's gif::vector<S_SURR_ACT, 8>; the name is ours)
    std::vector<S_SURR_ACT> surr_acts;
};

// SYNTHETIC: GAME 0x459f70
// C_PhThing::`scalar deleting destructor'
