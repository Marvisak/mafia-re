#pragma once

#include "actors/C_actor.h"

// C_cutscene::state (the names are ours, by value but the stopped one)
#define CUTSCENE_STATE_0 0
#define CUTSCENE_STATE_1 1
#define CUTSCENE_STATE_2 2
#define CUTSCENE_STOPPED 4

// A track of timed items (the names are ours), 0xc bytes: Tick takes each item once the cutscene's time (unknown_120)
// reaches the time it starts with. The track at 0x74 has 0x24-byte items, the one at 0xb4 0xc-byte ones, which it
// passes to the HUD (0x5f8d30), the one at 0xd8 0x2c-byte ones.
struct S_cutscene_track
{
    uint num_items;                     // 0x0
    uint next;                          // 0x4, the next item to take
    uchar *items;                       // 0x8, new[]
};

// The cutscene's object at 0x80 (the names are ours), 0x10 bytes: a class with a vtable of its own (0x625ac0, six
// functions) and a constructor (0x59b5f0), neither written yet, so the vtable's place is a placeholder
class C_cutscene_unknown_80
{
public:
    // Frees the array
    void Clear();

    uchar unknown_0[4];                 // 0x0, the vtable
    int unknown_4;                      // 0x4, the array's count
    uchar *unknown_8;                   // 0x8, new[]
    int unknown_c;                      // 0xc, 0x100 from the constructor
};

// What C_cutscene::vfunc_5 creates for an actor of a cutscene (the names are ours), 0x5c bytes: the actor holds it
// (C_actor::unknown_8), and so does its entry in the object at 0x90. Records in blocks of 0x1800 bytes (0x5c2700)
// that end in 0xeeeeeeee, each a dword the walks compare with unknown_14, its type and as many bytes as unknown_44
// gives for the type; C_actor's vfunc_10 and vfunc_11 walk them. Its name is the actor type's prefix and a count
// ("act%d", "hum%d", "car%d", ...). Its functions are in the range of the file at 0x5c25b0-0x5c283c, with C_actor's
// vfunc_6, vfunc_7, vfunc_8, vfunc_10 and vfunc_11.
class C_cutscene_records
{
public:
    // Drops the records (unknown_18), nothing for NULL (the name is ours)
    void Reset();

    uchar unknown_0[0x14];              // 0x0
    int unknown_14;                     // 0x14, -666 from vfunc_5
    uchar *unknown_18;                  // 0x18, the first block
    int unknown_1c;                     // 0x1c
    char name[0x24];                    // 0x20
    int unknown_44[4];                  // 0x44, the records' sizes by type: 8, then vfunc_5's first three arguments
    uchar unknown_54[8];                // 0x54
};

// An actor of the cutscene's object at 0x90 (the names are ours), 0x58 bytes
struct S_cutscene_actor
{
    int unknown_0;                      // 0x0, while 0 the records are reset where the entry lets them go
    int unknown_4;                      // 0x4
    char unknown_8[0x24];               // 0x8
    char unknown_2c[0x24];              // 0x2c
    C_actor *actor;                     // 0x50
    C_cutscene_records *records;        // 0x54, the actor's (C_actor::unknown_8)
};

// The elements of the vector at 0x14 of the cutscene's object at 0x90 (the name is ours), 0x28 bytes
struct S_cutscene_unknown_28
{
    uchar unknown_0[0x28];
};

// The cutscene's object at 0x90 (the names are ours), 0x24 bytes. Its constructor (0x59cf70) isn't written yet.
class C_cutscene_unknown_90
{
public:
    // Takes the actor's entry out of the vector of actors; with the flag it also resets its records (unless
    // unknown_0 is set) and takes them from the entry and from the actor
    void RemoveActor(C_actor *actor, bool unknown);
    // Releases the first vector's elements and empties it, then calls 0x59d350
    void Clear();

    int unknown_0;                      // 0x0, 0 from the constructor
    std::vector<S_cutscene_actor> actors;   // 0x4
    std::vector<S_cutscene_unknown_28> unknown_14;  // 0x14
};

// The in-engine cutscenes (PC only, the names are ours: PS2 plays its cutscenes as videos, C_vid, which its
// GameLoopSingle updates where the PC's ticks this object). Its Tick (0x599610) plays two tracks of keys, 0x40 and
// 0x38 bytes, until a key whose time is 666.0; GameLoopSingle stops it on Space or Enter, and on Escape in the intro
// mission. Only what C_mission::Close, C_actor's destructor and C_game's Done call so far, and the members they use.
// Open: the original's constructor (0x59cc40, after Close), which sets the state CUTSCENE_STOPPED and clears the
// members, isn't written, so ours is the implicit one, inline in the object's initializer.
class C_cutscene
{
public:
    // The vtable (0x625ad8) has 45 functions or more; the first eleven are declared so far, the unknown ones by their
    // slots (the names are ours). From CUTSCENE_STATE_0 on, CUTSCENE_STATE_1 and CUTSCENE_STATE_2 set the two lists'
    // actors' ACTF_UNKNOWN_10 and C_actor::vfunc_6 and vfunc_7 by the state, CUTSCENE_STATE_0 clears them.
    virtual void SetState(int new_state);
    virtual int GetState();
    // Sets unknown_11c from the argument (through 0x605560), and unknown_120 and unknown_124 from it
    virtual void vfunc_2(int unknown);
    virtual int GetUnknown11c();
    virtual int GetUnknown120();
    // Creates an actor's records (C_cutscene_records), their sizes of types 1 to 3 from the first three arguments,
    // named by the actor type
    virtual void *vfunc_5(int a, int b, int c, int type);
    // Clears a 0x20-byte struct, its member at 0x14 to -666
    virtual void vfunc_6(void *p);
    virtual void vfunc_7(int unknown);
    // Adds the actor to the cutscene's lists, which RemoveActor takes it from
    virtual bool vfunc_8(C_actor *actor, int a, int b, int c);
    // Takes the actor from the cutscene's lists (+0x90's, which takes the flag too, +0x4's and +0x14's), the first
    // list's with its ACTF_UNKNOWN_10: true if one of those two held it (C_actor's destructor calls it)
    virtual bool RemoveActor(C_actor *actor, bool unknown);
    // Takes every actor out of the two lists: through RemoveActor with the flag where unknown_ec is 1, itself otherwise
    virtual void RemoveActors();
    // Unloads the cutscene and gives the HUD and the player's controls back (C_mission::Close, C_game::Done, the racing
    // menu and GameLoopSingle call it). The vtable's 40th function, declared plain until the slots before it are.
    void Close();
    // Whether a cutscene is playing (the name is ours): while unknown_647a58 counts down, and in a state but
    // CUTSCENE_STATE_0 and CUTSCENE_STOPPED
    bool IsPlaying();
    // Releases the objects of unknown_34 and empties it (the name is ours)
    void ReleaseUnknown34();

    // The names are ours
    std::vector<C_actor *> actors;      // 0x4
    std::vector<C_actor *> actors_2;    // 0x14
    std::vector<uint> unknown_24;       // 0x24
    std::vector<uint> unknown_34;       // 0x34
    // Close clears the rest from unknown_44 to state, and frees the tracks' items first
    uchar unknown_44[0x18];             // 0x44
    uchar unknown_5c[0xc];              // 0x5c
    uchar unknown_68[0xc];              // 0x68
    S_cutscene_track unknown_74;        // 0x74
    C_cutscene_unknown_80 unknown_80;   // 0x80
    C_cutscene_unknown_90 unknown_90;   // 0x90
    S_cutscene_track unknown_b4;        // 0xb4
    uchar unknown_c0[0xc];              // 0xc0
    uchar unknown_cc[0xc];              // 0xcc
    S_cutscene_track unknown_d8;        // 0xd8
    int state;                          // 0xe4, CUTSCENE_*
    int prev_state;                     // 0xe8, SetState's previous one
    int unknown_ec;                     // 0xec, 1 from the constructor
    uchar *unknown_f0;                  // 0xf0, new[]: Close deletes it, as the two below
    uchar *unknown_f4;                  // 0xf4
    uchar *unknown_f8;                  // 0xf8
    bool unknown_fc;                    // 0xfc
    uchar unknown_fd[0x1f];             // 0xfd
    int unknown_11c;                    // 0x11c
    int unknown_120;                    // 0x120, the cutscene's time, which the tracks' items wait for
    float unknown_124;                  // 0x124, 1.0 from the constructor
    bool unknown_128;                   // 0x128, cleared by SetState(CUTSCENE_STATE_0), set by Close
    uchar unknown_129[3];               // 0x129
};

// An actor's frame as a cutscene found it, which Close puts back (the names are ours), 0x2c bytes
struct S_cutscene_placement
{
    S_vector pos;                       // 0x0
    S_quat rot;                         // 0xc
    C_actor *actor;                     // 0x1c
    I3D_frame *frame;                   // 0x20
    uchar unknown_24[8];                // 0x24
};

// The one object
extern C_cutscene cutscene;
// Frames the cutscene still counts as playing (the name is ours): Tick counts it down, 0x59e2a0 clears it
extern int unknown_647a58;
// The placements Close puts back and frees (the name is ours)
extern std::vector<S_cutscene_placement> *unknown_647a4c;

// The seagulls (PC only, the name is ours: they load racek.i3d, Czech for a seagull), 0x244 bytes. Their file is the
// cutscenes': the constructor (0x59e7e0) is in its range, after Close. Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_SEAGULL.
class C_seagull : public C_actor
{
public:
    C_seagull();

    uchar unknown_70[0x1d4];            // 0x70
};
