#pragma once

#include "Main.h"

#include <vector>

class I3D_frame;
class I3D_sound;

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

// The objects the player can use (PS2 C_using_object, tools.cpp): only its two vectors are known, which the game's
// inline constructor constructs (PS2's also clears its 0x18 and 0x1c). Its methods are in the PC's tools.cpp
// (0x55dd60, 0x55df10).
class C_using_object
{
public:
    // An object the player can use (PS2 C_using_object::S_using_object, whose members are 4 bytes further), 0x28 bytes:
    // C_detector has one at 0xe0, before its 0x108. Only its flags and frame are known so far, which ClearObjects and
    // DelObject use. Its constructor is C_detector's stores (PS2's clears its flags too).
    struct S_using_object
    {
        uchar unknown_0[0x10];          // 0x0
        int unknown_10;                 // 0x10
        uint flags;                     // 0x14, USINGF_*
        I3D_frame *frame;               // 0x18 (PS2 0x1c)
        int unknown_1c;                 // 0x1c
        int unknown_20;                 // 0x20
        bool unknown_24;                // 0x24

        S_using_object() : unknown_10(0), frame(NULL), unknown_1c(0), unknown_20(0), unknown_24(false) {}
    };

    std::vector<S_using_object *> objects;  // 0x0 (PS2's first, destroyed last)
    std::vector<uint> unknown_10;       // 0x10, 4-byte values (PS2's vector<unsigned int>)

    // PS2 names
    // Forgets the object, its frame and itself freed as ClearObjects' (from the last object)
    void DelObject(S_using_object *obj);
    void ClearObjects();
    void ClearInvalidItems();
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

// The sounds the humans hear (PS2 soundArena, tools.cpp), the game's 0x2b3c, 0x208 bytes; what it holds isn't known
// yet
class soundArena
{
public:
    uchar unknown_0[0x200];             // 0x0
    int unknown_200;                    // 0x200
    int unknown_204;                    // 0x204

    // Forgets every sound (PS2 name)
    void Init();
};

// A fire (PS2 C_fire, tools.cpp), which the game keeps (its 0x314) and deletes at Done; only the destructor is known
class C_fire
{
public:
    ~C_fire();
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
// are ours. Only what InitSystem and CloseSystem use is written so far.
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
    virtual void AddGroup(const char *name);
    virtual void DelGroup(int index);
    virtual void AddSound(const char *name, uint group);
    virtual void DelSound(uint id);
    virtual void SetFileName(S_sound_table *table, int index, const char *file_name);
    virtual void OptimalizeTable(S_sound_table *table);
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
// ?GetText@GM_Component@@UAEPAEXZ

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
