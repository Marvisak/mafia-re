#pragma once

#include "actors/C_actor.h"

// C_recorder::state (the names are ours, by value but the stopped one)
#define CUTSCENE_STATE_0 0
#define CUTSCENE_STATE_1 1
#define CUTSCENE_STATE_2 2
#define CUTSCENE_STOPPED 4

// A track of timed items (the names are ours), 0xc bytes: Tick takes each item once the cutscene's time (scaled_time)
// reaches the time it starts with. The dubbing's track has 0x24-byte items, the subtitles 0xc-byte ones, which it
// passes to the HUD (0x5f8d30, a text database text by the id at 0x8), the morphs' 0x2c-byte ones.
struct S_cutscene_track
{
    uint num_items;                     // 0x0
    uint next;                          // 0x4, the next item to take
    uchar *items;                       // 0x8, new[]
};

// The names of the cutscene's animations (the names are ours), 0x10 bytes: an array of 0x38-byte items, each an id,
// its animation's index in C_recorder::anims (0x0), a name (0x4) and a value (0x34); its vtable loads and saves them
// VTABLE: GAME 0x625ac0
class C_cutscene_anim_names
{
public:
    C_cutscene_anim_names();
    // The vtable (the names are ours): the array loaded from an archive's file (dtaRead), from a file (fread) and from
    // memory, saved to a file (fwrite) and to memory, and the size it saves
    virtual void LoadDta(int handle);
    virtual void LoadFile(FILE *file);
    virtual void LoadBuffer(void *buf);
    virtual void SaveFile(FILE *file);
    virtual void SaveBuffer(void *buf);
    virtual int GetSaveSize();
    // The name and the value of the item of that id, NULL and 0 if there is none, and the value set for each item of
    // that id (the names are ours)
    const char *GetName(int id);
    int GetValue(int id);
    void SetValue(int id, int value);
    // Frees the array
    void Clear();

    int num_items;                      // 0x4
    uchar *items;                       // 0x8, new[], one more for each item C_recorder::AddAnim adds (0x59b620)
    int unknown_c;                      // 0xc, 0x100 from the constructor
};

// What C_recorder::vfunc_5 creates for an actor of a cutscene (the names are ours), 0x5c bytes: the actor holds it
// (C_actor::records), and so does its entry in the cutscene's recording. Records in blocks of 0x1800 bytes (0x5c2700
// makes one) that end in 0xeeeeeeee, each a dword the walks compare with unknown_14, its type and as many bytes as
// record_sizes gives for the type; C_actor's vfunc_10 and vfunc_11 walk them. Its name is the actor type's prefix and
// a count ("act%d", "hum%d", "car%d", ...). Its functions are in the range of the file at 0x5c25b0-0x5c283c, with
// C_actor's vfunc_6, vfunc_7, vfunc_8, vfunc_10 and vfunc_11.
class C_cutscene_records
{
public:
    // Drops the records (first_block), nothing for NULL (the name is ours)
    void Reset();

    uchar unknown_0[0x14];              // 0x0
    int unknown_14;                     // 0x14, -666 from vfunc_5
    uchar *first_block;                 // 0x18
    int unknown_1c;                     // 0x1c
    char name[0x24];                    // 0x20
    int record_sizes[4];                // 0x44, by the records' type: 8, then vfunc_5's first three arguments
    uchar unknown_54[8];                // 0x54
};

// An actor of the cutscene's recording (the names are ours), 0x58 bytes
struct S_cutscene_actor
{
    int unknown_0;                      // 0x0, while 0 the records are reset where the entry lets them go
    int type;                           // 0x4, the actor's type (ACTOR_*), which AttachActor's actor must have
    char frame_name[0x24];              // 0x8, the actor's frame's name, "<NONE>" once DetachActor took the actor off
    char name[0x24];                    // 0x2c, the records' name, which AttachActor finds the entry by
    C_actor *actor;                     // 0x50
    C_cutscene_records *records;        // 0x54, the actor's (C_actor::records)
};

// An actor RecAddActor names for the recording (the name is ours), 0x28 bytes: the name (0x0) and the actor (0x24)
struct S_cutscene_rec_actor
{
    uchar unknown_0[0x28];
};

// The cutscene's recording (the names are ours), 0x24 bytes. Its constructor (0x59cf70) isn't written yet.
class C_cutscene_recording
{
public:
    // Takes the actor's entry out of the vector of actors; with the flag it also resets its records (unless
    // unknown_0 is set) and takes them from the entry and from the actor
    void RemoveActor(C_actor *actor, bool unknown);
    // Resets the actors' records (those whose entry's unknown_0 is 0) and takes their first blocks from the actors,
    // empties the vector of actors, then ReleaseUnknown0
    void Clear();
    // Frees unknown_0 unless an actor's entry has unknown_0 1
    void ReleaseUnknown0();
    // What the cutscene's recordings pass on to (the names are ours, Unknown59d3c0's by address; see C_recorder)
    void RecClear();
    void RecAddActor(const char *name, C_actor *actor);
    void RecLoad(const char *name);
    void Unknown59d3c0();
    void DetachActor(int unknown);
    void AttachActor(C_actor *actor, int unknown);

    uchar *unknown_0;                   // 0x0, NULL from the constructor, new
    std::vector<S_cutscene_actor> actors;   // 0x4
    std::vector<S_cutscene_rec_actor> rec_actors;  // 0x14, RecAddActor's
};

// What an animation's .tck file (anims\<name> with the extension .tck) gives (the names are ours), 0x44 bytes: the
// cutscene's LoadTCK (0x59bdf0) loads it: C_animations.h's S_track without the name, has_directions last (0x40)
struct S_cutscene_tck
{
    int length;                         // 0x0, as C_animations_machine_de_luxe::S_track's, 0 where it isn't loaded
    uchar unknown_4[0x1c];              // 0x4
    std::vector<uint> positions;        // 0x20, the keys', 0xc bytes each (LoadTCK), as S_track's
    std::vector<uint> directions;       // 0x30, the keys', 0xc bytes each (LoadTCK), as S_track's
    uchar unknown_40[4];                // 0x40
};

// An animation of the cutscene (the names are ours), 0x48 bytes: the cutscene's CreateAnim (0x59bc20) creates it,
// with an animation set of anims\<name> and, if asked, its .tck file. Its destructor is the implicit one, out of line
// after ReleaseAnims (0x59c550).
struct S_cutscene_anim
{
    I3D_animation_set *anim_set;        // 0x0
    S_cutscene_tck tck;                 // 0x4
};

// SYNTHETIC: GAME 0x59c550
// S_cutscene_anim::~S_cutscene_anim

// The in-engine cutscenes (PC only: PS2 plays its cutscenes as videos, C_vid, which its GameLoopSingle updates where
// the PC's ticks this object; the class's name is LoadTCK's messages', the members' names are ours). Its Tick
// (0x599610) plays two tracks of keys, 0x40 and 0x38 bytes, until a key whose time is 666.0; GameLoopSingle stops
// it on Space or Enter, and on Escape in the intro mission. Only what C_mission::Close, C_actor's destructor and
// C_game's Done call so far, and their members. Open: the original's constructor (0x59cc40, after Close), which sets
// the state CUTSCENE_STOPPED and clears the members, isn't written: ours is the implicit one, in the initializer.
// VTABLE: GAME 0x625ad8
class C_recorder
{
public:
    // The vtable has 54 functions, the unknown ones named by their slots (the names are ours). From CUTSCENE_STATE_0
    // on, CUTSCENE_STATE_1 and CUTSCENE_STATE_2 set the two lists' actors' ACTF_CUTSCENE and C_actor::vfunc_6 and
    // vfunc_7 by the state, CUTSCENE_STATE_0 clears them.
    virtual void SetState(int new_state);
    virtual int GetState();
    // Sets cur_time from the argument (through 0x605560), and scaled_time and time_speed from it
    virtual void SetTime(int unknown);
    virtual int GetCurTime();
    virtual int GetScaledTime();
    // Creates an actor's records (C_cutscene_records), their sizes of types 1 to 3 from the first three arguments,
    // named by the actor type
    virtual void *vfunc_5(int a, int b, int c, int type);
    // Clears a 0x20-byte struct, its member at 0x14 to -666
    virtual void vfunc_6(void *p);
    virtual void vfunc_7(int unknown);
    // Adds the actor to the cutscene's lists, which RemoveActor takes it from
    virtual bool vfunc_8(C_actor *actor, int a, int b, int c);
    // Takes the actor from the cutscene's lists (the recording's, which takes the flag too, actors and actors_2), from
    // actors with its ACTF_CUTSCENE: true if one of those two held it (C_actor's destructor calls it)
    virtual bool RemoveActor(C_actor *actor, bool unknown);
    // Takes every actor out of the two lists where unknown_ec is 1, through RemoveActor with the flag (which skips every
    // other one, the lists shrinking under the loops); otherwise frees each actor's records and clears its
    // ACTF_CUTSCENE, keeping the lists, and clears the recording
    virtual void RemoveActors();
    virtual void vfunc_11();
    // Whether the actor is in either list, and whether it is in the second one only (the names are ours)
    virtual bool HasActor(C_actor *actor);
    virtual void vfunc_13(C_actor *actor, int unknown);
    virtual bool HasActor2Only(C_actor *actor);
    virtual void AddAnim(const char *name, int unknown);
    virtual S_cutscene_anim *CreateAnim(const char *name, bool tck);
    // AddAnim: the id of the name's item of anim_names, or a new animation (CreateAnim's) into anims and its item. The
    // animation of an item of anim_names by its name and by its index, NULL if there is none (the names are ours)
    virtual S_cutscene_anim *GetAnim(const char *name);
    virtual S_cutscene_anim *GetAnimByIndex(int index);
    // anim_names's (the original's copies of 19 and 35 are folded into one)
    virtual const char *vfunc_19(int id);
    virtual void LoadTCK(const char *name, int unknown);
    virtual void vfunc_21(int unknown);
    virtual void vfunc_22();
    virtual void ResetSpeedKeys();
    // Starts the actor's records over from their first block
    virtual void vfunc_24(C_actor *actor);
    // The scripts' recordings (RECLOADFULL, RECCLEAR, RECADDACTOR, RECLOAD; the names are ours): RecLoadFull reads the
    // mission's actors, the other three pass on to the recording's
    virtual void RecLoadFull(const char *name);
    virtual void vfunc_26(const char *name);
    // The names are ours
    virtual void SetSpeedKeysOn(bool on);
    virtual bool GetSpeedKeysOn();
    virtual void RecClear();
    virtual void RecAddActor(const char *name, C_actor *actor);
    virtual void RecLoad(const char *name);
    // The recording's (an actor off its entry, an actor onto the entry of a name), the last one with vfunc_24 after it
    virtual void DetachActor(int unknown);
    virtual void AttachActor(C_actor *actor, int unknown);
    virtual int vfunc_34(const char *name);
    // anim_names's
    virtual const char *vfunc_35(int id);
    virtual int vfunc_36(int id);
    virtual void vfunc_37(int id, int value);
    // The recording's
    virtual void vfunc_38();
    // Unloads the cutscene and gives the HUD and the player's controls back (C_mission::Close, C_game::Done, the racing
    // menu and GameLoopSingle call it)
    virtual void Close();
    virtual void vfunc_40();
    virtual void vfunc_41(int unknown);
    virtual void vfunc_42(int unknown);
    virtual void vfunc_43(const char *name);
    virtual void vfunc_44(const char *name);
    virtual void vfunc_45(int unknown);
    virtual int vfunc_46();
    virtual void vfunc_47(int a, int b, int c, int d);
    virtual void vfunc_48(int a, int b, int c);
    // The dubbing's tick (DabingTick)
    virtual void vfunc_49(uint time);
    // The show (an IShow at 0x647a50): opened by name, its start in the cutscene's time, kept in step (names ours)
    virtual void OpenShow(int unknown);
    virtual void SetShowStart(int unknown);
    virtual void TickShow(int unknown);
    // The linker's folded return of 0 (0x4aa430)
    virtual int vfunc_53();
    // Whether a cutscene is playing (the name is ours): while cutscene_play_ticks counts down, and in a state but
    // CUTSCENE_STATE_0 and CUTSCENE_STOPPED
    bool IsPlaying();
    // Releases the animations of anims and empties it (the name is ours)
    void ReleaseAnims();

    // The names are ours
    std::vector<C_actor *> actors;      // 0x4
    std::vector<C_actor *> actors_2;    // 0x14
    std::vector<uint> unknown_24;       // 0x24
    // 0x34, an animation for each of anim_names' items, NULL where an item has none (the vtable's 23rd function,
    // 0x59c2f0), AddAnim's
    std::vector<S_cutscene_anim *> anims;
    // Close clears the rest from unknown_44 to state, and frees the tracks' items first
    uchar unknown_44[0x18];             // 0x44
    uchar unknown_5c[0xc];              // 0x5c
    int speed_keys[3];                  // 0x68, the speed keys (0xc bytes each), their count and the current one
    // 0x74, the dubbing's, whose items Tick passes to 0x5f6670 (g_dabing.cpp's range): a speech of a channel
    // (DabingSpeak), or the channel's human or model taken from the frame named in the item (the name is ours)
    S_cutscene_track dabing;
    C_cutscene_anim_names anim_names;   // 0x80
    C_cutscene_recording recording;     // 0x90
    S_cutscene_track subtitles;         // 0xb4
    S_cutscene_track programs;          // 0xc0, Tick runs the global program named at its items' 0x4 (the name is ours)
    uchar unknown_cc[0xc];              // 0xcc
    // 0xd8, whose items Tick passes to 0x5f6760 (g_dabing.cpp's range), which adds the morph of the frame named in the
    // item to G_DabingMorphActive (the name is ours)
    S_cutscene_track morphs;
    int state;                          // 0xe4, CUTSCENE_*
    int prev_state;                     // 0xe8, SetState's previous one
    int unknown_ec;                     // 0xec, 1 from the constructor
    uchar *unknown_f0;                  // 0xf0, new[]: Close deletes it, as the two below
    uchar *unknown_f4;                  // 0xf4
    uchar *unknown_f8;                  // 0xf8
    bool unknown_fc;                    // 0xfc
    uchar unknown_fd[0x1f];             // 0xfd
    int cur_time;                       // 0x11c, SetTime's, which Tick plays the keys by
    int scaled_time;                    // 0x120, cur_time through the speed keys, which the tracks' items wait for
    float time_speed;                   // 0x124, the current speed key's (a float), 1.0 without them
    bool unknown_128;                   // 0x128, cleared by SetState(CUTSCENE_STATE_0) and Tick, set by Close
    bool speed_keys_on;                 // 0x129, SetTime goes by the speed keys (each a time, its scaled_time, a speed)
    uchar unknown_12a[2];               // 0x12a
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
extern C_recorder cutscene;
// Ticks the cutscene still counts as playing (the name is ours): 5 from RecLoadFull, Tick counts it down to 0
extern int cutscene_play_ticks;
// The placements RecLoadFull makes and Close puts back and frees (the name is ours)
extern std::vector<S_cutscene_placement> *saved_placements;

// The seagulls (PC only, the name is ours: they load racek.i3d, Czech for a seagull), 0x244 bytes. Their file is the
// cutscenes': the constructor (0x59e7e0) is in its range, after Close. Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_SEAGULL.
// VTABLE: GAME 0x625bb0
class C_seagull : public C_actor
{
public:
    C_seagull();
    virtual ~C_seagull();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk) { return true; }
    virtual bool LoadData(C_chunk *chunk) { return true; }
    virtual int SaveGameGetSize()
    {
        return 0;
    }
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);
    // A virtual function of its own (the original's last slot)
    virtual void vfunc_38(int a, int b);

    uchar unknown_70[0x1d4];            // 0x70
};

// SYNTHETIC: GAME 0x59e830
// C_seagull::`scalar deleting destructor'
