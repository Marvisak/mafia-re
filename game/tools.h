#pragma once

#include "Main.h"
#include "actors/g_inventory.h"

#include <vector>

#include "I3D/I3d_math.h"

class I3D_frame;
class I3D_sound;
class C_actor;

// A period ticked with the game's time (the name is ours; PS2 has it inline, without a symbol), 8 bytes: the game, the
// police (C_shvestky) and the force feedback have them, 200 ms unless set otherwise. PS2's C_shvestky::Tick adds the
// time and, once it is past the period, subtracts the period and does the work.
struct S_timer
{
    uint time;                          // 0x0
    uint period;                        // 0x4

    S_timer() : time(0), period(200) {}

    // Starts again with another period (the name is ours): C_shvestky::GameInit sets its timers through it, as the
    // inline calls its inline budget needs show
    void Reset(uint p)
    {
        time = 0;
        period = p;
    }

    // Adds the time: true once it is past the period, which is then subtracted (the name is ours)
    bool Tick(uint t)
    {
        time += t;
        if (time <= period)
            return false;
        time -= period;
        return true;
    }
};

// C_using_object::S_using_object::flags (the names are ours): what ClearObjects frees
#define USINGF_DELETE 0x1               // the object, which C_using_object allocated
#define USINGF_FRAME 0x2                // its frame, which the game forgets, unlinks and releases

// The special actions' ids (S_GameItem::ammo_loaded of a GAMEITEM_SPECIAL_ACTION's using object), which
// C_using_object's invalid items hold: those C_human::Do_FuckingBox and Do_DeadBody turn off while the player
// carries something, and the dead body's two, which C_human::ChangeState gives its using objects (the names are
// ours, after the labels the text database has for the same ids)
#define USING_ACTION_OPEN_DOOR 0x7d0             // "Open door"
#define USING_ACTION_CLOSE_DOOR 0x7d1            // "Close door"
#define USING_ACTION_USE_SWITCH 0x7d2            // "Use switch"
#define USING_ACTION_GET_IN_FRONT 0x7d3          // "Get in the front"
#define USING_ACTION_GET_IN_BACK 0x7d4           // "Get in the back"
#define USING_ACTION_BOARD_TRAIN 0x7d7           // "Board train"
#define USING_ACTION_EXIT_TRAIN 0x7d8            // "Exit train"
#define USING_ACTION_HIJACK_CAR 0x7d9            // "Hijack car"
#define USING_ACTION_PICK_UP_BOX 0x7da           // "Pick up box"
#define USING_ACTION_DROP_BOX 0x7db              // "Drop box"
#define USING_ACTION_SEARCH_DEAD_BODY 0x7dc      // "Search dead body"
#define USING_ACTION_MOVE_DEAD_BODY 0x7dd        // "Move dead body"
#define USING_ACTION_TALK 0x7de                  // "Talk to the pedestrian"
#define USING_ACTION_GET_IN_AS_DRIVER 0x7df      // "Get in the car as the driver"
#define USING_ACTION_BOARD_TROLLEY 0x7e0         // "Board Trolley Car"
#define USING_ACTION_EXIT_TROLLEY 0x7e1          // "Exit Trolley Car"

// The objects the player can use (PS2 C_using_object, tools.cpp), the game's 0x29ec, which the game's inline
// constructor constructs (PS2's also clears its 0x18 and 0x1c).
class C_using_object
{
public:
    // An object the player can use (PS2 C_using_object::S_using_object, whose members are 4 bytes further, 0x40 bytes),
    // 0x38 bytes: C_detector has one at 0xe0, and the scripts' CREATEWEAPONFROMFRAME creates one for a weapon's copy,
    // which gives the members (the names are ours but PS2's frame and flags). Its constructor is C_detector's stores
    // (PS2's clears its flags too), the item's memset last.
    struct S_using_object
    {
        S_vector pos;                   // 0x0, the frame's world position, 1 higher
        // 0xc, the square of the horizontal distance within which the player reaches the object: 0.5 for a weapon
        float radius_2;
        // 0x10, the space pos is in, transformed by it where it isn't NULL (every user of the PC's sets NULL; the name
        // is ours)
        const S_matrix *matrix;
        uint flags;                     // 0x14, USINGF_*
        I3D_frame *frame;               // 0x18 (PS2 0x1c)
        C_actor *actor;                 // 0x1c, the dead human whose body it is, NULL for a weapon (the name is ours)
        I3D_frame *sector;              // 0x20, the frame's nearest FRAME_SECTOR parent
        // 0x24, set for CREATEWEAPONFROMFRAME's weapons, which SaveGameSave leaves out (the name is a guess)
        bool scripted;
        S_GameItem item;                // 0x28, the item the player gets, whose using_object is the object itself

        // FUNCTION: GAME 0x47aef0
        S_using_object() : matrix(NULL), frame(NULL), actor(NULL), sector(NULL), scripted(false) {}
    };

    // A put down weapon in a saved game (the name is ours), 0x30 bytes: SaveGameSave's record of the objects' and the
    // dropped weapons' (C_drop_in_weapon), from which SaveGameLoad makes the objects again
    struct S_using_object_savegame
    {
        S_GameItem item;                // 0x0
        S_vector pos;                   // 0x10, the frame's world position
        S_quat rot;                     // 0x1c, its world rotation
        float radius_2;                 // 0x2c
    };

    std::vector<S_using_object *> objects;  // 0x0 (PS2's first, destroyed last)
    // 0x10, the special actions the player can't use, AddInvalidItem's (PS2's vector<unsigned int>; the name is ours)
    std::vector<uint> invalid_items;

    // PS2 names
    // An object for the player to use, once
    void AddObject(S_using_object *obj);
    // Forgets the object, its frame and itself freed as ClearObjects' (from the last object)
    void DelObject(S_using_object *obj);
    // The objects near pos, their items into items, the nearest first; an object of another sector than sector
    // is left out (PS2 name and parameters; dir isn't read)
    void FindNearObjects(const S_vector &pos, const S_vector &dir, std::vector<S_GameItem> &items,
        I3D_frame *sector);
    void ClearObjects();
    void ClearInvalidItems();
    // The special actions (S_GameItem::ammo_loaded of a GAMEITEM_SPECIAL_ACTION) the player can't use, added and
    // taken out again, and whether one is (PS2 names)
    void AddInvalidItem(uint id);
    void DelInvalidItem(uint id);
    bool IsInvalidItem(uint id);
    // Whether a valid object in the frame's sector (or in none) is within reach of pos: 1.5 m up or down, and
    // horizontally within its radius (PS2 name; dir isn't read)
    bool IsAnyObjectNear(const S_vector &pos, const S_vector &dir, I3D_frame *sector);
    // The objects in a saved game (PS2 names), which C_game's SaveGame functions call: the save and the load
    // return true
    int SaveGameGetSize();
    bool SaveGameSave(void **buf);
    bool SaveGameLoad(void **buf);
};

// The compass of the game's HUD (PS2 C_compass, tools.cpp), the game's 0x26c, 0x14 bytes. Its destructor is declared: the
// game's constructor's unwinding calls it, an empty function (the folded 0x404990).
class C_compass
{
public:
    I3D_frame *frame;                   // 0x0, where it points, NULL when off (the name is ours)
    // Activate sets them from the screen's size: a twentyfifth, a fortieth, a fifteenth and a twentieth of its width
    // or height, most likely the compass's place and size on the screen
    int unknown_4;                      // 0x4, width / 15
    int unknown_8;                      // 0x8, height / 20
    int unknown_c;                      // 0xc, width / 25
    int unknown_10;                     // 0x10, height / 40

    C_compass() : frame(NULL) {}
    ~C_compass() {}

    // Points the compass at a frame, NULL turning it off (PS2 name)
    void Activate(I3D_frame *frm);
};

// The cinema's bars (PS2 C_silver_screen, its Tick in tools.cpp; Main.cpp has the one object, PS2's silver_screen),
// 0x24 bytes as PS2's: while moving, Tick slides amount towards 1 or 0 by direction, speed a second, stops at either
// end and turns them off at 0. Nothing on the PC sets them moving: the constructor leaves them off, and only
// GameLoopSingle and the racing menu tick them (the members' names are ours).
class C_silver_screen
{
public:
    bool active;                        // 0x0, ticked while set
    bool moving;                        // 0x1
    float speed;                        // 0x4, 1.0
    float amount;                       // 0x8, 0 to 1
    int direction;                      // 0xc, 1 or -1
    float ratio;                        // 0x10, 1 / 1.85, the cinema's (most likely the picture's between the bars)
    uchar unknown_14[0x10];             // 0x14

    C_silver_screen() : active(false), speed(1.0f), amount(0.0f), ratio(1.0f / 1.85f) {}
    ~C_silver_screen() {}

    void Tick(uint time);
};

extern C_silver_screen silver_screen;

class C_entity;

// How many sounds soundArena keeps (the name is ours)
#define NUM_ARENA_SOUNDS 16

// The sounds the humans hear (PS2 soundArena, tools.cpp), the game's 0x2b3c, 0x208 bytes: the last 16 of them (the
// members' names are ours), which an entity's GetSounds gives its senses
class soundArena
{
public:
    // A sound (the names are ours), 0x20 bytes (PS2's 0x30, the vector 16 bytes)
    struct S_sound
    {
        S_vector pos;                   // 0x0
        uint id;                        // 0xc, the caller's kind of sound, 0 for none
        uint time;                      // 0x10, the game's time it was emitted at
        float loudness;                 // 0x14, what it has at pos, falling to 0 at range
        float range;                    // 0x18
        C_actor *act;                   // 0x1c, who made it
    };

    S_sound sounds[NUM_ARENA_SOUNDS];   // 0x0
    uint last;                          // 0x200, the sound written last; the next wraps round
    uint next_clear;                    // 0x204, the game's time after which SoundEmit forgets the old sounds

    // Forgets every sound (PS2 name)
    void Init();
    // A sound of the actor's the humans hear at pos (PS2 name; the parameters' names are ours): human_notify_callback's
    // steps. An actor's sound of the same id is moved there, otherwise it takes the place after the last; the sounds
    // older than 2 s are forgotten every 2 s.
    void SoundEmit(S_vector &pos, float range, float loudness, uint id, C_actor *act);
    // The sounds the entity hears given to its senses, those it doesn't hear taken out of them (PS2 name)
    void GetSounds(C_entity *ent);
    // Forgets an actor's sounds (PS2 name), which C_game::InvalidateActor calls
    void InvalidateActor(C_actor *act);
};

// The effects' particle and the sound table's sound of a fire, which C_fire::Init starts (the names are ours)
#define PARTICLE_FIRE 0x15
#define SOUND_FIRE_BURNING 0x14

// A fire (PS2 C_fire, tools.cpp), 0x30 bytes (C_game::NewFire's new), which the game keeps (its 0x314) and deletes at
// Done. The members' names are ours.
class C_fire
{
public:
    C_fire();
    ~C_fire();
    // Starts the fire at at, in frm's space where frm isn't NULL, for time ms (PS2 name; the parameters' names are
    // ours): its particle, and its sound if it burns for 2 s or longer. With hurt set it hurts the actors within
    // range, by dmg at its start and less as it burns out, which C_game::NewFire calls.
    void Init(S_vector &at, I3D_frame *frm, int time, float range, float dmg, bool hurt);

    // The fire by the time passed, which C_game::Tick calls (PS2 name): false when it has burnt out, and C_game::Tick
    // deletes it
    bool Tick(uint time);

    S_vector pos;                       // 0x0
    // 0xc (PS2 0x10), the frame it burns on: C_game::InvalidateActor deletes the fires of an actor's frame
    I3D_frame *frame;
    int life;                           // 0x10, the time left (PS2 0x14)
    float range_2;                      // 0x14, the square of the range it hurts within
    float damage;                       // 0x18
    S_timer timer;                      // 0x1c, when it hurts, every 450 to 550 ms (PS2 0x20)
    I3D_sound *sound;                   // 0x24 (PS2 0x28)
    bool hurts;                         // 0x28 (PS2 0x2c)
    int particle;                       // 0x2c, the effects' particle, -1 without one
};

// A group of the sound table's sounds (PS2 S_sound_group), 0x24 bytes: the editor's, which the game skips in the file
struct S_sound_group
{
    char name[0x20];                    // 0x0
    int id;                             // 0x20, what the sounds' group is
};

// The sound groups (PS2 name)
extern std::vector<S_sound_group> sound_group;

// A sound's type in the sound table (C_sound_table::S_sound_param::type; the names are ours, after I3D_sound's types,
// whose message names the last one): ConstructSound gives the sound I3D_SOUNDTYPE's type of the name, but for the
// volume, which the PC has none for
#define SOUNDPARAM_AMBIENT 0
#define SOUNDPARAM_POINT 1
#define SOUNDPARAM_SPOT 2
#define SOUNDPARAM_VOLUME 3

// The game's sounds by number (PS2 C_sound_table, tools.cpp), from tables\sounds.dat, 0x1c bytes. The members' names
// are ours. The virtual functions but the last two are the editor's.
// VTABLE: GAME 0x625410
class C_sound_table
{
public:
    // How a sound is set up when it's constructed, 0x24 bytes (the name is ours): its type and I3D_sound's parameters
    struct S_sound_param
    {
        uchar type;                     // 0x0, SOUNDPARAM_*
        float near_range;               // 0x4
        float far_range;                // 0x8
        float mid;                      // 0xc
        float mid_vol;                  // 0x10
        float in_angle;                 // 0x14
        float out_angle;                // 0x18
        float volume;                   // 0x1c
        float out_vol;                  // 0x20
    };

    // A sound (PS2 S_sound_table), 0xd0 bytes. PS2's is 0xe8, six more dwords before the parameters
    struct S_sound_table
    {
        char *name;                     // 0x0, the editor's: the game skips it in the file
        uint id;                        // 0x4, the number the game asks for
        int group;                      // 0x8, its S_sound_group's id
        char *file_names[6];            // 0xc
        int file_probability[6];        // 0x24, the files' cumulative chances in percent
        uchar num_files;                // 0x3c
        uchar num_params;               // 0x3d
        bool visible;                   // 0x3e, cleared when the sound fails to construct, ClearInvisibleFlag sets it again
        S_sound_param params[4];        // 0x40
    };

    int next_group_id;                  // 0x4
    uint next_sound_id;                 // 0x8
    std::vector<S_sound_table> tables;  // 0xc

    C_sound_table();
    // Not virtual: the vtable doesn't have it
    ~C_sound_table();

    void FreeTables();
    void LoadTables(const char *name);
    // Sets a sound up from the table of an id: false without one (PS2 name)
    bool ConstructSound(I3D_sound *sound, uint id);

    // PS2's virtual functions in PS2's order (the parameters each one's ret takes)
    virtual void SaveTables(const char *name);
    // A group of the next group id
    virtual void AddGroup(const char *name);
    // The group and its sounds; its place taken by the last group
    virtual void DelGroup(int index);
    // A sound of the next sound id, cleared
    virtual void AddSound(const char *name, uint group);
    virtual void DelSound(uint id);
    // A file's name, a copy of file_name, NULL for none
    virtual void SetFileName(S_sound_table *table, int index, const char *file_name);
    // The files sorted by their chances, the lowest first
    virtual void OptimalizeTable(S_sound_table *table);
    // The sound of an id, NULL without one
    virtual S_sound_table *GetTableFromIDX(uint id);

    // Its copy comes among the file's templates
    // FUNCTION: GAME 0x55fd90
    virtual uint GetNumSounds() { return tables.size(); }

    // The original kept another file's copy of its folded group (by name below the class)
    virtual S_sound_table *GetSounds() { return tables.begin(); }

    virtual bool TestConstructSound(I3D_sound *sound, uint id);
    virtual void ClearInvisibleFlag();
};

extern C_sound_table sound_table;

// GetSounds' folded group (mov eax, [ecx + 0x10]; ret, with C_actor::GetType, C_mission::GetScene and
// GM_Component::GetText), whose copy the original kept from another file: named after the copy our linker keeps
// (foldfix)
// SYNTHETIC: GAME 0x5d92d0
// ?GetSounds@C_sound_table@@UAEPAUS_sound_table@1@XZ

// The groups' push_back's insert(P, n, x), which AddGroup calls out of line
// TEMPLATE: GAME 0x55fdc0
// ?insert@?$vector@US_sound_group@@V?$allocator@US_sound_group@@@std@@@std@@QAEXPAUS_sound_group@@IABU3@@Z

// The vectors' helpers, which LoadTables calls out of line: the sounds' clear's erase and push_back's insert(P, n, x)
// TEMPLATE: GAME 0x55fd40
// ?erase@?$vector@US_sound_table@C_sound_table@@V?$allocator@US_sound_table@C_sound_table@@@std@@@std@@QAEPAUS_sound_table@C_sound_table@@PAU34@0@Z

// TEMPLATE: GAME 0x5600c0
// ?insert@?$vector@US_sound_table@C_sound_table@@V?$allocator@US_sound_table@C_sound_table@@@std@@@std@@QAEXPAUS_sound_table@C_sound_table@@IABU34@@Z

// And the groups' clear's copy: the original kept another file's copy of it, or of its folded group
// TEMPLATE: GAME 0x499020
// ?copy@std@@YIPAUS_sound_group@@PAU2@00@Z

// Plays one of the logo videos at the start until a key is pressed or it ends: the name without its extension, and the
// video's size on an 800x600 screen, whose ratio sets its height. False if no file was found or none opened. PC only,
// the name is ours.
bool PlayLogo(const char *name, float sx, float sy);
