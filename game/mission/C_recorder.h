#pragma once

#include "actors/C_actor.h"
#include "collision/g_colls.h"

class IShow;

// C_recorder::state (the names are ours, by value but the stopped one)
#define CUTSCENE_STATE_0 0
#define CUTSCENE_STATE_1 1
#define CUTSCENE_STATE_2 2
#define CUTSCENE_STATE_3 3
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

// An item of the names of the cutscene's animations (the names are ours), 0x38 bytes
struct S_cutscene_anim_name
{
    // 0x0, C_recorder::AddAnim's: its animation's index in C_recorder::anims; C_recorder::AddAnimName's: NewId's,
    // from 0x101 on
    int id;
    char name[0x30];                    // 0x4
    int value;                          // 0x34, 0 where added or loaded
};

// The names of the cutscene's animations (the names are ours), 0x10 bytes: an array of items, which its vtable loads
// and saves
// VTABLE: GAME 0x625ac0
class C_cutscene_anim_names
{
public:
    C_cutscene_anim_names();
    // The vtable (the names are ours): the array loaded from an archive's file (dtaRead), from a file (fread) and from
    // memory, saved to a file (fwrite) and to memory, and the size it saves
    virtual bool LoadDta(int handle);
    virtual bool LoadFile(FILE *file);
    virtual bool LoadBuffer(void *buf);
    virtual bool SaveFile(FILE *file);
    virtual bool SaveBuffer(void *buf);
    virtual int GetSaveSize();
    // The next id, the item of an id and a name added, the id of the name's item (-1 if there is none), the name and
    // the value of the item of that id (NULL and 0 if there is none), the value set for each item of that id, the
    // count (the names are ours)
    int NewId();
    bool AddItem(int id, const char *name);
    int FindId(const char *name);
    const char *GetName(int id);
    int GetValue(int id);
    void SetValue(int id, int value);
    int GetNumItems();
    // Frees the array where it has items (the recorder's constructor calls it; the name is ours)
    void Reset();
    // Frees the array
    void Clear();

    int num_items;                      // 0x4
    S_cutscene_anim_name *items;        // 0x8, new[], one more for each item AddItem adds
    int last_id;                        // 0xc, the last id NewId gave, 0x100 from the constructor
};

// What C_recorder::CreateRecords creates for an actor of a cutscene (the names are ours), 0x5c bytes: the actor holds it
// (C_actor::records), and so does its entry in the cutscene's recording. Records in blocks of 0x1800 bytes
// (C_actor::NewRecordBlock makes one: the next block, the end its records stay below, then the records) that end in
// RECORDS_BLOCK_END, each its time (S_cutscene_record), which the walks compare with last_key_time, its type and as
// many bytes as record_sizes gives for the type; C_actor's GetRecordsSize and GetBlockRecordsSize walk them. Its name
// is the actor type's prefix and a count ("act%d", "hum%d", "car%d", ...). Its functions are at 0x5c25b0-0x5c283c, with
// C_actor's SetCutsceneRecord, SetCutscenePlay, UpdateCutsceneFlags, GetRecordsSize and GetBlockRecordsSize.
class C_cutscene_records
{
public:
    // Drops the records (first_block), nothing for NULL (the name is ours)
    void Reset();
    // The size of the records up to the last key (the name is ours), as C_actor::GetRecordsSize
    int GetRecordsSize();

    // 0x0, the record the actor's PlayRecord plays, the last one C_actor::AddRecord added while recording (the name is
    // ours)
    uchar *cur;
    uchar *next;                        // 0x4, the next record to play (PlayStep's), AddRecord's last too
    // 0x8, RewindActor sets it to the first record, PlayActor to each record it plays whose type has bit 1
    uchar *unknown_8;
    uchar *unknown_c;                   // 0xc, NULL from RecLoadFull and LoadBuffer
    // 0x10, RewindActor sets it to the first record, PlayActor to each record it plays whose type has bit 1
    // (and bit 0 or 1)
    uchar *unknown_10;
    // 0x14, the time of the last key recorded, RECORDS_NO_KEY from CreateRecords (the name is the dump's,
    // C_human::DumpRecords)
    int last_key_time;
    uchar *first_block;                 // 0x18
    uchar *last_block;                  // 0x1c, AddRecord's (the name is ours)
    char name[0x24];                    // 0x20
    int record_sizes[4];                // 0x44, by the records' type: 8, then CreateRecords' first three arguments
    // 0x54, set by PlayActor once the cutscene's time has passed the last key, cleared by RewindActor (the name is
    // ours)
    int ended;
    C_actor *actor;                     // 0x58, the actor whose records they are (C_recorder::AddActor's)
};

// The last key's time of records without keys (the name is ours)
#define RECORDS_NO_KEY (-666)

// What every record starts with (the names are ours), 8 bytes: the time it was recorded at and its type (0 to 3), whose
// size C_cutscene_records::record_sizes gives
// The end of a block's records, where the next block's start (the name is ours)
#define RECORDS_BLOCK_END 0xeeeeeeee

struct S_cutscene_record
{
    int time;                           // 0x0
    int type;                           // 0x4
};

// An actor of the cutscene's recording (the names are ours), 0x58 bytes
struct S_cutscene_actor
{
    // 0x0, the records lie in the recording's data (C_cutscene_recording::data), which they don't own: while 0 they
    // are reset where the entry lets them go (the name is ours)
    int in_data;
    int type;                           // 0x4, the actor's type (ACTOR_*), which AttachActor's actor must have
    char frame_name[0x24];              // 0x8, the actor's frame's name, "<NONE>" once DetachActor took the actor off
    char name[0x24];                    // 0x2c, the records' name, which AttachActor finds the entry by
    C_actor *actor;                     // 0x50
    C_cutscene_records *records;        // 0x54, the actor's (C_actor::records)
};

// An actor RecAddActor names for the recording (the names are ours), 0x28 bytes
struct S_cutscene_rec_actor
{
    char name[0x24];                    // 0x0, the records' name in the file RecLoad loads
    C_actor *actor;                     // 0x24
};

// The cutscene's recording (the names are ours), 0x24 bytes
class C_cutscene_recording
{
public:
    C_cutscene_recording();
    // Whether there is data (the name is ours)
    bool HasData();
    // The entry of the records' name, and of the records' and the frame's names (the names are ours)
    bool FindActor(const char *name, S_cutscene_actor **entry);
    bool FindActor(const char *name, const char *frame_name, S_cutscene_actor **entry);
    // Takes the actor's entry out of the vector of actors; with the flag it also resets its records (unless
    // in_data is set) and takes them from the entry and from the actor
    void RemoveActor(C_actor *actor, bool unknown);
    // Resets the actors' records (those whose entry's in_data is 0) and takes their first blocks from the actors,
    // empties the vector of actors, then ReleaseData
    void Clear();
    // Frees the data unless an actor's entry has in_data 1
    void ReleaseData();
    // What the cutscene's recordings pass on to (the names are ours; see C_recorder)
    void RecClear();
    void RecAddActor(const char *name, C_actor *actor);
    bool RecLoad(const char *name);
    void UpdateActors();
    void DetachActor(C_actor *actor);
    void AttachActor(C_actor *actor, const char *name);
    // Takes the data the loaded records lie in as data, false for none or where an actor's entry still has
    // in_data 1 (0x59d000, the name is ours)
    bool SetData(uchar *data);
    // With add, the entry of the name gets the records (in_data from shared, the type), or a new entry is made
    // with the actor, whose records they become, which RewindActor starts (0x59d740, the names are ours)
    void AddEntry(bool add, bool shared, const char *name, C_actor *actor, C_cutscene_records *records,
        int type);
    // Adds an entry for the actor (0x59da60, C_recorder::AddActor's; the name is ours)
    void AddActor(C_actor *actor);

    // 0x0, the records' data RecLoadFull loaded (SetData's), new: the records of the entries with in_data lie in it
    // (the name is ours)
    uchar *data;
    std::vector<S_cutscene_actor> actors;   // 0x4
    std::vector<S_cutscene_rec_actor> rec_actors;  // 0x14, RecAddActor's
};

// The implicit destructor's copy, which the recorder's constructor's unwinding calls
// SYNTHETIC: GAME 0x59ce30
// C_cutscene_recording::~C_cutscene_recording

// The copies of the recording's vectors' templates
// TEMPLATE: GAME 0x59e5f0
// ?_Ucopy@?$vector@US_cutscene_actor@@V?$allocator@US_cutscene_actor@@@std@@@std@@IAEPAUS_cutscene_actor@@PBU3@0PAU3@@Z

// TEMPLATE: GAME 0x59e630
// ?_Ufill@?$vector@US_cutscene_actor@@V?$allocator@US_cutscene_actor@@@std@@@std@@IAEXPAUS_cutscene_actor@@IABU3@@Z

// TEMPLATE: GAME 0x59e660
// ?_Ucopy@?$vector@US_cutscene_rec_actor@@V?$allocator@US_cutscene_rec_actor@@@std@@@std@@IAEPAUS_cutscene_rec_actor@@PBU3@0PAU3@@Z

// TEMPLATE: GAME 0x59e6a0
// ?_Ufill@?$vector@US_cutscene_rec_actor@@V?$allocator@US_cutscene_rec_actor@@@std@@@std@@IAEXPAUS_cutscene_rec_actor@@IABU3@@Z

// TEMPLATE: GAME 0x59e7a0
// ?_Construct@std@@YIXPAUS_cutscene_actor@@ABU2@@Z

// TEMPLATE: GAME 0x59e7c0
// ?_Construct@std@@YIXPAUS_cutscene_rec_actor@@ABU2@@Z

// What an animation's .tck file (anims\<name> with the extension .tck) gives (the names are ours), 0x44 bytes: the
// cutscene's LoadTCK (0x59bdf0) loads it: C_animations.h's S_track without the name, has_directions last (0x40)
struct S_cutscene_tck
{
    uint length;                        // 0x0, as C_animations_machine_de_luxe::S_track's, 0 where it isn't loaded
    int key_time;                       // 0x4, as S_track's (C_human::GetRecordPos divides by it)
    S_vector start_pos;                 // 0x8, as S_track's
    S_vector end_pos;                   // 0x14, as S_track's
    std::vector<S_vector> positions;    // 0x20, the keys' (LoadTCK), as S_track's
    std::vector<S_vector> directions;   // 0x30, the keys' (LoadTCK), as S_track's
    bool has_directions;                // 0x40, as S_track's (C_human::GetRecordRot tests it)
    uchar unknown_41[3];                // 0x41
};

// The implicit destructor, out of line after C_recorder::HasTCK (0x59c170), whose track's unwinding calls it
// SYNTHETIC: GAME 0x59c2c0
// S_cutscene_tck::~S_cutscene_tck

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

// A key of the camera's track (the names are ours), 0x40 bytes: Tick moves the camera from one key to the next by
// cur_time, through 0x5984e0's curve (the positions with their tangents, the time eased), and turns it to the
// target's key of the time; a key whose fov is CUTSCENE_END_FOV stops the cutscene once its time has come
struct S_cutscene_camera_key
{
    uint time;                          // 0x0, cur_time's, which Tick looks the keys up by
    uint key_time;                      // 0x4, the interpolation's
    uint flags;                         // 0x8, CUTKEYF_*
    S_vector pos;                       // 0xc
    S_vector tangent_out;               // 0x18, the interpolation's from this key
    S_vector tangent_in;                // 0x24, the interpolation's into this key
    float roll;                         // 0x30
    float fov;                          // 0x34, G_IndicatorsClass::SetCameraFOV's
    float ease_out;                     // 0x38, Ease's from this key
    float ease_in;                      // 0x3c, Ease's into this key
};

// The fov of the key that ends the cutscene (the name is ours)
#define CUTSCENE_END_FOV 666.0f

// S_cutscene_camera_key::flags, S_cutscene_target_key::flags (the names are ours, by value): Tick goes from the last
// key with either bit whose time has come to the first one whose time hasn't; with CUTKEYF_UNKNOWN_2 the curve starts
// two keys after the first one and ends one key after the second one
#define CUTKEYF_UNKNOWN_1 0x1
#define CUTKEYF_UNKNOWN_2 0x2

// A key of the camera's target (the names are ours), 0x38 bytes: as the camera's, without roll and fov (0x598680)
struct S_cutscene_target_key
{
    uint time;                          // 0x0
    uint key_time;                      // 0x4
    uint flags;                         // 0x8, CUTKEYF_*
    S_vector pos;                       // 0xc
    S_vector tangent_out;               // 0x18
    S_vector tangent_in;                // 0x24
    float ease_out;                     // 0x30
    float ease_in;                      // 0x34
};

// The camera's track (the names are ours), 0x18 bytes: its keys, its target's, their counts and the keys Tick is at
struct S_cutscene_camera_track
{
    S_cutscene_camera_key *keys;        // 0x0
    S_cutscene_target_key *targets;     // 0x4
    uint num_keys;                      // 0x8
    uint num_targets;                   // 0xc
    uint cur_key;                       // 0x10
    uint cur_target;                    // 0x14
};

// A key of the overlay's track (the names are ours), 0x20 bytes: the HUD's fade and picture over the scene
// (G_IndicatorsClass::SetOverlayFade and SetOverlayTexture), full from the key's time to the next key's, or fading
struct S_cutscene_overlay_key
{
    uint flags;                         // 0x0, its time (OVERLAYF_TIME) and OVERLAYF_*
    float fade_out;                     // 0x4, the time it fades out over after its time (OVERLAYF_FADE_OUT)
    float fade_in;                      // 0x8, the time it fades in over before its time (OVERLAYF_FADE_IN)
    uint color;                         // 0xc, the fade's colour (0xrrggbb)
    char texture[0x10];                 // 0x10, the picture's name (OVERLAYF_TEXTURE)
};

// S_cutscene_overlay_key::flags (the names are ours)
#define OVERLAYF_TIME 0xffffff          // the key's time
#define OVERLAYF_OPAQUE 0x8000000       // SetOverlayTexture's opaque
#define OVERLAYF_FULL 0x10000000        // full until the next key, without fading
#define OVERLAYF_TEXTURE 0x20000000     // with a picture
#define OVERLAYF_FADE_IN 0x40000000
#define OVERLAYF_FADE_OUT 0x80000000

// The overlay's track (the names are ours), 0xc bytes
struct S_cutscene_overlay_track
{
    uint num_keys;                      // 0x0
    S_cutscene_overlay_key *keys;       // 0x4
    uint cur_key;                       // 0x8, the last key whose time has come
};

// A speed key (the names are ours), 0xc bytes: from its time on scaled_time runs at its speed
struct S_cutscene_speed_key
{
    uint time;                          // 0x0, cur_time's
    int scaled_time;                    // 0x4, scaled_time at its time
    float speed;                        // 0x8
};

// The speed keys (the names are ours), 0xc bytes
struct S_cutscene_speed_keys
{
    S_cutscene_speed_key *keys;         // 0x0
    uint num_keys;                      // 0x4
    uint cur_key;                       // 0x8, the last key cur_time has reached (ResetSpeedKeys clears it)
};

// A track of timed items Tick takes all of up to the time (the names are ours), 0xc bytes, as S_cutscene_track but
// counted from the last item taken
struct S_cutscene_events
{
    int num_items;                      // 0x0
    int last;                           // 0x4, the last item taken
    uchar *items;                       // 0x8, new[]
};

// The items of the tracks (the names are ours): a speech of the dubbing (0x24 bytes, 0x5f6670's), a subtitle (0xc
// bytes, 0x5f8d30's), a global program run (0x28 bytes), a sound frame switched on or off (0x28 bytes) and a morph
// of a face (0x2c bytes, 0x5f6760's), each starting with its time (scaled_time's)
struct S_cutscene_dabing
{
    uint time;                          // 0x0
    int chanel;                         // 0x4
    uint speech;                        // 0x8, a speech or DABING_CUTSCENE_* (DabingCutscene)
    char frame_name[0x18];              // 0xc
};

struct S_cutscene_subtitle
{
    uint time;                          // 0x0
    uint duration;                      // 0x4, 0 for as long as the next one doesn't come
    uint text_id;                       // 0x8, the text database's
};

struct S_cutscene_program
{
    uint time;                          // 0x0
    char name[0x24];                    // 0x4, C_mission::GlobalProgramRun's
};

// S_cutscene_sound::mode (the names are ours)
#define CUTSCENE_SOUND_ON 0
#define CUTSCENE_SOUND_OFF 1

struct S_cutscene_sound
{
    uint time;                          // 0x0
    int mode;                           // 0x4, CUTSCENE_SOUND_*
    char frame_name[0x20];              // 0x8, the scene's sound frame
};

struct S_cutscene_morph
{
    uint time;                          // 0x0
    int duration;                       // 0x4
    char frame_name[0x20];              // 0x8, the scene's visual
    uchar level[2][2];                  // 0x28, as G_DabingMorphObj's
};

// A recording file's header (RecLoadFull, RecSaveFull; the names are ours), 0x60 bytes: the sizes of what follows it,
// the animations' names (anim_names' save), the actors' entries (S_cutscene_file_actor), their records' data, the
// camera's keys and its target's, the lists (four sizes, then the speed keys, the overlay's keys, the programs and
// the sounds) and the tracks (C_recorder::LoadTracks')
#define CUTSCENE_FILE_MAGIC 0x1631

struct S_cutscene_file_header
{
    int magic;                          // 0x0, CUTSCENE_FILE_MAGIC
    int names_size;                     // 0x4
    uint data_size;                     // 0x8
    uint num_actors;                    // 0xc
    int unknown_10;                     // 0x10, neither written nor read
    uint num_keys;                      // 0x14, C_recorder::camera's
    uint num_targets;                   // 0x18
    uint lists_size;                    // 0x1c
    uint tracks_size;                   // 0x20
    int reserved[15];                   // 0x24, 0
};

// An actor's entry of a recording file (the names are ours), 0x6c bytes: the actor's frame's name, "<none>" for
// none, the records' name and what C_cutscene_records keeps of them
struct S_cutscene_file_actor
{
    char frame_name[0x24];              // 0x0
    char name[0x24];                    // 0x24
    int record_sizes[4];                // 0x48
    int ended;                          // 0x58
    int last_key_time;                  // 0x5c
    int type;                           // 0x60, S_cutscene_actor::type
    uint size;                          // 0x64, the records' bytes in the file's data, their block's header too
    uint offset;                        // 0x68, where they start in it
};

// The in-engine cutscenes (PC only: PS2 plays its cutscenes as videos, C_vid, which its GameLoopSingle updates where
// the PC's ticks this object; the class's name is LoadTCK's messages', the members' names are ours). Its Tick
// (0x599610) plays the camera's track (camera), until a camera key of CUTSCENE_END_FOV; GameLoopSingle stops
// it on Space or Enter, and on Escape in the intro mission.
// VTABLE: GAME 0x625ad8
class C_recorder
{
public:
    C_recorder();
    // The vtable has 54 functions, the unknown ones named by their slots (the names are ours). From CUTSCENE_STATE_0
    // on, CUTSCENE_STATE_1 and CUTSCENE_STATE_2 set the two lists' actors' ACTF_CUTSCENE and C_actor::SetCutsceneRecord
    // and SetCutscenePlay by the state, CUTSCENE_STATE_0 clears them.
    virtual void SetState(int new_state);
    virtual int GetState();
    // Jumps to a time: cur_time (CutsceneTime's), scaled_time and time_speed by the speed keys, the subtitles
    // past it skipped, and in CUTSCENE_STATE_0 and CUTSCENE_STATE_3 the actors placed there
    virtual void SetTime(int time);
    virtual int GetCurTime();
    virtual int GetScaledTime();
    // Creates an actor's records (C_cutscene_records), their sizes of types 1 to 3 from the first three arguments,
    // named by the actor type
    virtual C_cutscene_records *CreateRecords(int size1, int size2, int size3, int type);
    // Starts the records over (nothing recorded, no key yet), and adds an actor to the played ones without records
    // (the names are ours)
    virtual void ResetRecords(C_cutscene_records *records);
    virtual C_actor *AddPlayedActor(C_actor *actor);
    // Adds the actor to the cutscene's lists, which RemoveActor takes it from, with records of the three sizes
    // (CreateRecords'; the name is ours): the actor, NULL where they can't be made
    virtual C_actor *AddActor(C_actor *actor, int size1, int size2, int size3);
    // Takes the actor from the cutscene's lists (the recording's, which takes the flag too, actors and actors_2), from
    // actors with its ACTF_CUTSCENE: true if one of those two held it (C_actor's destructor calls it)
    virtual bool RemoveActor(C_actor *actor, bool unknown);
    // Takes every actor out of the two lists where unknown_ec is 1, through RemoveActor with the flag (which skips every
    // other one, the lists shrinking under the loops); otherwise frees each actor's records and clears its
    // ACTF_CUTSCENE, keeping the lists, and clears the recording
    virtual void RemoveActors();
    // Both lists' actors in one vector, a static of the function (the name is ours)
    virtual std::vector<C_actor *> *GetActors();
    // Whether the actor is in either list, and whether it is in the second one only (the names are ours)
    virtual bool HasActor(C_actor *actor);
    // An actor from the played ones to the recorded ones, or back (the name is ours)
    virtual void SetActorRecord(C_actor *actor, bool record);
    virtual bool HasActor2Only(C_actor *actor);
    virtual int AddAnim(const char *name, bool tck);
    virtual S_cutscene_anim *CreateAnim(const char *name, bool tck);
    // AddAnim: the id of the name's item of anim_names, or a new animation (CreateAnim's) into anims and its item. The
    // animation of an item of anim_names by its name and by its index, NULL if there is none (the names are ours)
    virtual S_cutscene_anim *GetAnim(const char *name);
    virtual S_cutscene_anim *GetAnimByIndex(int index);
    // anim_names's (the original's copies of 19 and 35 are folded into one)
    virtual const char *vfunc_19(int id);
    // An animation's track into tck (false where the file doesn't open), whether the id's item has one (HasTCK), and
    // the animations anew for the items (CreateAnims; the names are ours)
    virtual bool LoadTCK(const char *name, S_cutscene_tck *tck);
    virtual bool HasTCK(int id);
    virtual bool CreateAnims();
    virtual void ResetSpeedKeys();
    // Starts the actor's records over from their first block
    virtual void RewindActor(C_actor *actor);
    // The scripts' recordings (RECLOADFULL, RECCLEAR, RECADDACTOR, RECLOAD; the names are ours): RecLoadFull reads the
    // mission's actors, the other three pass on to the recording's
    virtual bool RecLoadFull(const char *name);
    // A recording file of the cutscene, which RecLoadFull loads (the name is ours)
    virtual bool RecSaveFull(const char *name);
    // The names are ours
    virtual void SetSpeedKeysOn(bool on);
    virtual bool GetSpeedKeysOn();
    virtual void RecClear();
    virtual void RecAddActor(const char *name, C_actor *actor);
    virtual void RecLoad(const char *name);
    // The recording's (an actor off its entry, an actor onto the entry of a name), the last one with RewindActor after it
    virtual void DetachActor(C_actor *actor);
    virtual void AttachActor(C_actor *actor, const char *name);
    // The id of the name's item of anim_names, added where it has none, -1 where that fails (the name is ours)
    virtual int AddAnimName(const char *name);
    // anim_names's (the names are ours): C_human::PlaceAtTime takes the value as the actor linked by the name
    virtual const char *GetAnimName(int id);
    virtual int GetAnimValue(int id);
    virtual void SetAnimValue(int id, int value);
    // The recording's UpdateActors (the name is ours)
    virtual void RecUpdateActors();
    // Unloads the cutscene and gives the HUD and the player's controls back (C_mission::Close, C_game::Done, the racing
    // menu and GameLoopSingle call it)
    virtual void Close();
    // The recording's actors without one to memory and to a file, as the animations' names' functions (the names
    // are ours, after theirs)
    virtual int GetSaveSize();
    virtual void SaveBuffer(void *buf);
    virtual void LoadBuffer(void *buf);
    virtual void SaveFile(const char *name);
    virtual void LoadFile(const char *name);
    // The end time of an animation of anim_names by its id (the name is ours)
    virtual int GetAnimEndTime(uint id);
    virtual int vfunc_46();
    // The HUD's overlay picture and its fade, a speech of a channel by an actor (the names are ours)
    virtual void SetOverlay(float fade, uint color, const char *name, bool opaque);
    virtual void Speak(int chanel, C_actor *actor, uint speech);
    // The dubbing's tick (DabingTick)
    virtual void vfunc_49(uint time);
    // The show (an IShow, movie): opened by name, its start in the cutscene's time, kept in step (names ours)
    virtual void OpenShow(const char *name);
    virtual void SetShowStart(int unknown);
    virtual void TickShow(int unknown);
    // The linker's folded return of 0 (0x4aa430)
    virtual int vfunc_53();
    // Whether a cutscene is playing (the name is ours): while cutscene_play_ticks counts down, and in a state but
    // CUTSCENE_STATE_0 and CUTSCENE_STOPPED
    bool IsPlaying();
    // GameLoopSingle's, every frame (the names are ours but Tick's): while the game is initialized the frame's time
    // advances cur_time and, by the speed keys, scaled_time, and is scaled by the current key's speed; Tick plays the
    // cutscene's tracks; Stop stops it, the next four frames rendered without being presented
    void ScaleTime(uint &time);
    void Tick(uint time);
    // Tick's (the names are ours): the camera's key and its target's at the time, interpolated between the two keys
    // given (0x5984e0, 0x598680, after Ease)
    void InterpolateCamera(S_cutscene_camera_key *out, const S_cutscene_camera_key *a, const S_cutscene_camera_key *b,
        uint time);
    void InterpolateTarget(S_cutscene_target_key *out, const S_cutscene_target_key *a, const S_cutscene_target_key *b,
        uint time);
    void Stop();
    // The actor's records played to the cutscene's time, from their start, and recorded (the names are ours)
    void PlayActor(C_actor *actor, uint time);
    void ReplayActor(C_actor *actor, uint time);
    void RecordActor(C_actor *actor, uint time);
    // Releases the animations of anims and empties it (the name is ours)
    void ReleaseAnims();
    // The dubbing's, the subtitles' and the morphs' tracks into a buffer, out of it, and the size they take there (the
    // names are ours)
    void SaveTracks(void *buf);
    void LoadTracks(void *buf);
    int GetTracksSize();

    // The names are ours
    std::vector<C_actor *> actors;      // 0x4
    std::vector<C_actor *> actors_2;    // 0x14
    std::vector<uint> unknown_24;       // 0x24
    // 0x34, an animation for each of anim_names' items, NULL where an item has none (CreateAnims), AddAnim's
    std::vector<S_cutscene_anim *> anims;
    // Close clears the rest from unknown_44 to state, and frees the tracks' items first
    S_cutscene_camera_track camera;     // 0x44
    S_cutscene_overlay_track overlay;   // 0x5c
    S_cutscene_speed_keys speed_keys;   // 0x68, ScaleTime's and SetTime's
    // 0x74, the dubbing's, whose items Tick passes to 0x5f6670 (g_dabing.cpp's range): a speech of a channel
    // (DabingSpeak), or the channel's human or model taken from the frame named in the item (the name is ours)
    S_cutscene_track dabing;
    C_cutscene_anim_names anim_names;   // 0x80
    C_cutscene_recording recording;     // 0x90
    S_cutscene_track subtitles;         // 0xb4
    S_cutscene_events programs;         // 0xc0, the global programs Tick runs (the name is ours)
    S_cutscene_events sounds;           // 0xcc, the sound frames Tick switches on and off (the name is ours)
    // 0xd8, whose items Tick passes to 0x5f6760 (g_dabing.cpp's range), which adds the morph of the frame named in the
    // item to G_DabingMorphActive (the name is ours)
    S_cutscene_track morphs;
    uint state;                         // 0xe4, CUTSCENE_*
    int prev_state;                     // 0xe8, SetState's previous one
    // The recording file's (RecLoadFull's; the names are ours): its records, which the recording keeps
    // (C_cutscene_recording::data; 1 from the constructor, which RemoveActors and RecLoadFull test for), the
    // keys, the lists and the tracks, new[]: Close deletes the last three
    uchar *records_data;                // 0xec
    uchar *keys_data;                   // 0xf0
    uchar *lists_data;                  // 0xf4
    uchar *tracks_data;                 // 0xf8
    // 0xfc, the camera's placement where C_actor::PlaceAtTime placed a camera's actor (ACTOR_REC_CAMERA):
    // the actor's frame's world position and rotation (the names are ours)
    bool camera_placed;
    uchar unknown_fd[3];                // 0xfd
    S_vector camera_pos;                // 0x100
    S_quat camera_rot;                  // 0x10c
    int cur_time;                       // 0x11c, SetTime's, which Tick plays the keys by
    uint scaled_time;                   // 0x120, cur_time through the speed keys, which the tracks' items wait for
    float time_speed;                   // 0x124, the current speed key's (a float), 1.0 without them
    bool unknown_128;                   // 0x128, cleared by SetState(CUTSCENE_STATE_0) and Tick, set by Close
    bool speed_keys_on;                 // 0x129, SetTime goes by the speed keys (each a time, its scaled_time, a speed)
    uchar unknown_12a[2];               // 0x12a
};

// The head of the tracks in a buffer, C_recorder::SaveTracks' (the names are ours): the counts of their items
struct S_cutscene_tracks_head
{
    uint num_dabing;
    uint num_subtitles;
    uint num_morphs;
};

// The implicit destructor
// SYNTHETIC: GAME 0x59ce80
// C_recorder::~C_recorder

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
// The ease of a key's interpolation, u from 0 to 1 eased by a (out of the first key) and b (into the next),
// as 3D Studio's keys have it (the name is ours, after 3D Studio's)
float Ease(float u, float a, float b);
// Ticks the cutscene still counts as playing (the name is ours): 5 from RecLoadFull, Tick counts it down to 0
extern int cutscene_play_ticks;
// The animations' ids from this on are anim_names' items' (GetAnimEndTime; the name is a guess)
#define CUTSCENE_ANIM_ID_BASE 0x400
// The placements RecLoadFull makes and Close puts back and frees (the name is ours)
extern std::vector<S_cutscene_placement> *saved_placements;
// The show the cutscene plays (OpenShow's)
extern IShow *movie;

// RecLoadFull's copies of the placements' vector's templates
// TEMPLATE: GAME 0x59e2f0
// ?insert@?$vector@US_cutscene_placement@@V?$allocator@US_cutscene_placement@@@std@@@std@@QAEXPAUS_cutscene_placement@@IABU3@@Z

// TEMPLATE: GAME 0x59e700
// ?size@?$vector@US_cutscene_placement@@V?$allocator@US_cutscene_placement@@@std@@@std@@QBEIXZ

// TEMPLATE: GAME 0x59e730
// ?_Ucopy@?$vector@US_cutscene_placement@@V?$allocator@US_cutscene_placement@@@std@@@std@@IAEPAUS_cutscene_placement@@PBU3@0PAU3@@Z

// TEMPLATE: GAME 0x59e770
// ?_Ufill@?$vector@US_cutscene_placement@@V?$allocator@US_cutscene_placement@@@std@@@std@@IAEXPAUS_cutscene_placement@@IABU3@@Z

// S_seagull::state (the names are ours): GameInit's seagulls fly, Hit makes one fall, AI puts a fallen one that hit
// the ground back into the air at a random place (respawn); none sets SEAGULL_STATE_0, which Hit skips (by value)
#define SEAGULL_STATE_0 0
#define SEAGULL_RESPAWN 1
#define SEAGULL_FLYING 2
#define SEAGULL_FALLING 4

// The number of planes C_seagull keeps (the name is ours): the floor and the ceiling of the box, then the walls along
// the ring of the web's nodes
#define SEAGULL_MAX_PLANES 18

// One seagull of the flock (the names are ours), 0x5c bytes. A flying one chases its target, a point that moves along
// dir at one unit a second and turns to next_dir where it meets the box's walls (turn_time); a falling one's dir is
// its speed.
struct S_seagull
{
    I3D_model *model;                   // 0x0, racek.i3d
    int state;                          // 0x4, SEAGULL_*
    S_vector dir;                       // 0x8
    S_vector target;                    // 0x14
    float turn_time;                    // 0x20, the time (in seconds) left till the target turns to next_dir
    S_vector next_dir;                  // 0x24
    S_vector unknown_30;                // 0x30, (1, 0, 0) from GameInit and the respawn
    uchar unknown_3c[0xc];              // 0x3c
    S_vector unknown_48;                // 0x48, (0, 0, 1) from GameInit and the respawn
    uchar unknown_54[4];                // 0x54
    // 0x58, 50 + 100 times its index from GameInit, 2000 from Hit (written through Hit's loop pointer, so into the
    // element after the last, not the one hit)
    int unknown_58;
};

// The seagulls (PC only, the name is ours: they load racek.i3d, Czech for a seagull), 0x244 bytes: a flock of five
// that fly about in a box over a ring of the walking paths' nodes (WEBF_UNKNOWN_8, the ring nearest the frame), fall
// when shot and explode where they hit the ground. Their file is the cutscenes': the constructor (0x59e7e0) is in its
// range, after Close; C_mission::CreateActor creates them for ACTOR_SEAGULL.
// VTABLE: GAME 0x625bb0
class C_seagull : public C_actor
{
public:
    C_seagull();
    virtual ~C_seagull();
    // The vtable's other overrides (the slots' names are C_actor's); some are the linker's folded constant functions
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk)
    {
        return true;
    }
    virtual bool LoadData(C_chunk *chunk)
    {
        return true;
    }
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

    // 0x70, a box's (DYNCOLL_UNKNOWN_40, the box and its matrix the seagulls'), which GameInit fills but nothing puts in
    tDynamicCollObject coll;
    S_matrix matrix;                    // 0xb8, the identity: the collision's matrix and its inverse
    I3D_bbox bbox;                      // 0xf8, the box around the ring's nodes
    std::vector<S_seagull> gulls;       // 0x110
    // 0x120, the box's floor and ceiling, then the walls through each pair of the ring's nodes (but GameInit puts the
    // walls from the first on, and the floor and the ceiling over the first two after them)
    S_plane planes[SEAGULL_MAX_PLANES];
    uint num_planes;                    // 0x240
};

// The game's copy of the engine's inline S_vector::GetNormal, which the seagulls' GameInit calls through
// S_plane::ComputePlane: by name, as a marker in ls3df/I3D/I3d_math.h moved LS3DF11's reccmp scores
// SYNTHETIC: GAME 0x5a06b0
// ?GetNormal@S_vector@@QAEXABU1@00@Z

// GetActors' static (its guard byte at 0x647a48). The destructor helper GetActors registers with atexit (0x5995f0,
// identical) is our $E24, the fourth of that name in link order: reccmp pairs a numbered name's annotations with
// our functions of the name in address order, so it can be annotated only with the three before it (C_effects.cpp's,
// tools.cpp's, VE_BreakageGlass.cpp's), which tools/enames.py can't place for the game (open)
// GLOBAL: GAME 0x647a38
// all_actors

// SYNTHETIC: GAME 0x59e830
// C_seagull::`scalar deleting destructor'
