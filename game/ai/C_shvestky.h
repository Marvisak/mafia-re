#pragma once

#include "Main.h"
#include "tools.h"

#include <vector>

class C_actor;
class C_car;
class C_human;

// The police (PS2 C_shvestky, C_shvestky.cpp), the game's 0x2da0, 0xb0 bytes. PS2's has the same members, its offsets in
// the comments. Only the constructor, the destructor, GameInit, GameDone and Tick are written so far, and the accessors
// the script's commands use.
// The records' flags (S_shvestky::flags; the names are ours, by value where the meaning isn't known yet)
#define SHVF_WEAPON 0x1                 // an item of 5 to 15 in the right hand
#define SHVF_FIRE 0x2                   // cleared once fire_time runs out
#define SHVF_SPEEDING 0x4               // the car faster than C_shvestky::unknown_c
#define SHVF_UNKNOWN_38 0x38            // cleared once unknown_18 runs out
#define SHVF_UNKNOWN_40 0x40            // counts unknown_14 up while the car is faster than 11
#define SHVF_UNKNOWN_200 0x200          // cleared once unknown_20 runs out
#define SHVF_RED_LIGHT 0x400            // TestOverRed's, cleared once unknown_24 runs out
#define SHVF_UNKNOWN_800 0x800          // cleared once unknown_c runs out
#define SHVF_UNKNOWN_1000 0x1000        // cleared with SHVF_RED_LIGHT
#define SHVF_UNKNOWN_2000 0x2000
#define SHVF_UNKNOWN_4000 0x4000
#define SHVF_UNKNOWN_8000 0x8000
// S_shvestky::flags2's (the name is ours): cleared once crash_time runs out
#define SHVF2_UNKNOWN_38 0x38
// The records' states, S_shvestky::state, JakPicusNaTomJest's (the names are ours, after the police's calls for them)
#define SHVSTATE_NONE 0
#define SHVSTATE_1 1
#define SHVSTATE_ARREST 2               // the police told to arrest him (PanoveZmenaZatkneteHoZmrda)
#define SHVSTATE_KILL 3                 // to kill him (PanoveZmenaZabijteHoZmrda)
// Why a record's state went up, S_shvestky::reason (the names are ours, by value where the meaning isn't known yet)
#define SHVREASON_WEAPON 0
#define SHVREASON_SPEEDING 2
#define SHVREASON_UNKNOWN_4 4
#define SHVREASON_RED_LIGHT 6
#define SHVREASON_UNKNOWN_7 7
#define SHVREASON_UNKNOWN_9 9
#define SHVREASON_UNKNOWN_A 0xa
// The items in a human's right hand the police mind, gameItemTable's ids (the names are ours): any of the first range
// on foot (SHVF_WEAPON), in a car the second, and in its seat 0 only up to SHVITEM_DRIVER_LAST
#define SHVITEM_FIRST 5
#define SHVITEM_LAST 15
#define SHVITEM_CAR_FIRST 6
#define SHVITEM_CAR_LAST 14
#define SHVITEM_DRIVER_LAST 9

class C_shvestky
{
public:
    // A human the police watch (PS2 name; PS2's DelNasrany is its), 0x80 bytes, the size AddPerson allocates (its
    // constructor, inline there, clears most of it). The names are ours.
    struct S_shvestky
    {
        C_human *human;                 // 0x0
        C_car *car;                     // 0x4, the human's car where it is ACTOR_CAR
        ushort flags;                   // 0x8, SHVF_*
        ushort flags2;                  // 0xa, SHVF2_*
        int unknown_c;                  // 0xc, SHVF_UNKNOWN_800's time
        int fire_time;                  // 0x10, GetFireTime's, SHVF_FIRE's
        short unknown_14;               // 0x14, SHVF_UNKNOWN_40's time
        int unknown_18;                 // 0x18, SHVF_UNKNOWN_38's time
        int crash_time;                 // 0x1c, GetCrashTime's, SHVF2_UNKNOWN_38's
        int unknown_20;                 // 0x20, SHVF_UNKNOWN_200's time
        int unknown_24;                 // 0x24, SHVF_RED_LIGHT's time: 4000 more for each red light, at most 10000
        int unknown_28;                 // 0x28
        bool unknown_2c;                // 0x2c
        uchar unknown_2d[7];            // 0x2d
        S_vector2 pos;                  // 0x34, the car's last position on the ground (x and z), TestOverRed's
        ushort state;                   // 0x3c, SHVSTATE_*
        ushort new_state;               // 0x3e, what CreateShvestkyTest makes state, cleared after
        int unknown_40;                 // 0x40, -1 from the constructor
        int unknown_44;                 // 0x44, a time left
        int unknown_48;                 // 0x48, unknown_44's start
        int unknown_4c;                 // 0x4c
        bool unknown_50;                // 0x50
        bool nevi;                      // 0x51, NeviDoKterePiceVlezl's
        uchar unknown_52[2];            // 0x52
        C_car *unknown_54;              // 0x54
        bool unknown_58;                // 0x58
        uchar unknown_59[3];            // 0x59
        int unknown_5c;                 // 0x5c, the more policemen the player's states 5 and 6 brought (1 or 2)
        int unknown_60;                 // 0x60
        int reason;                     // 0x64, SHVREASON_*
        int unknown_68;                 // 0x68
        bool unknown_6c;                // 0x6c
        uchar unknown_6d[3];            // 0x6d
        // 0x70, the actors angry at the human (PS2's AddNasrany's and DelNasrany's), those of C_actor::unknown_5e taken
        // out by Tick
        std::vector<C_actor *> nasrany;

        ~S_shvestky();
    };
    // The road blocks' cars, policemen and enemies (PS2's names), 8 bytes each: the actor and whether GameDone leaves it
    // in the game's lists (the members' names are ours)
    struct S_road_block_car
    {
        C_car *car;                     // 0x0
        bool unknown_4;                 // 0x4
    };
    struct S_road_block_iris
    {
        C_actor *actor;                 // 0x0
        bool unknown_4;                 // 0x4
    };
    struct S_road_block_enemy
    {
        C_actor *actor;                 // 0x0
        bool unknown_4;                 // 0x4
    };

    bool unknown_0;                     // 0x0, SetActive's; PM_SHOWSYMBOL and its group do nothing while it is set
    uchar unknown_1[3];
    S_timer unknown_4;                  // 0x4, Tick's period of the records
    // Two speeds, POLICEMANAGER_SETSPEED's (km/h made m/s)
    float unknown_c;                    // 0xc, 17 from GameInit (PS2's 27)
    float unknown_10;                   // 0x10, 28 from GameInit (PS2's 1000)
    S_timer unknown_14;                 // 0x14, Tick runs TickDoRiciActors at each of its periods
    S_timer unknown_1c;                 // 0x1c, Tick runs TestOverRed at each of its periods
    S_timer unknown_24;                 // 0x24
    int unknown_2c;                     // 0x2c
    int unknown_30;                     // 0x30
    uint unknown_34;                    // 0x34, the text of the failure's menu Tick runs with the player in the cooler
    bool unknown_38;                    // 0x38
    uchar unknown_39[3];                // 0x39
    std::vector<S_shvestky *> unknown_3c;  // 0x3c (PS2 0x40)
    std::vector<S_road_block_car> road_block_cars;  // 0x4c
    std::vector<S_road_block_iris> road_block_irises;  // 0x5c
    std::vector<S_road_block_enemy> road_block_enemies;  // 0x6c
    std::vector<C_actor *> unknown_7c;  // 0x7c (PS2 0x70)
    std::vector<C_car *> unknown_8c;    // 0x8c (PS2 0x7c), the police's temporary cars, on the radar
    std::vector<ushort> unknown_9c;     // 0x9c (PS2 0x88), nodes whose red lights TestOverRed leaves out
    bool unknown_ac;                    // 0xac, which C_game::Init sets
    uchar unknown_ad[3];

    C_shvestky();
    ~C_shvestky();

    // PS2 names: C_game's Init and Done call them
    void GameInit();
    void GameDone();
    // The police by the time passed, which C_game::Tick calls (PS2 name)
    void Tick(uint time);
    // PS2 names, which Tick calls: inline on the PC, PS2 has them out of line. The records' new states, and the police
    // cars' drivers who run a red light.
    inline void CreateShvestkyTest();
    inline void TestOverRed();
    // PS2 name (Czech, roughly "tick the actors in the face")
    void TickDoRiciActors(bool);
    // PS2 names, which Tick calls (the parameters' names are ours): Czech, roughly "he smoked me well, he can go home"
    // (the human let go) and "Vasek went boozing to the party"
    void DobreMiPokuril_MuzeJitDom(C_human *human, bool unknown);
    void VasekSelChlastatNaOslavu(C_human *human, const S_vector &pos, float range, bool unknown, bool unknown2);
    // A road block of the game's set up for the human (PC only, the name is ours), which Tick calls once a police car
    // comes near it
    void CreateRoadBlock(int index, C_human *human);

    // The police manager on or off (PS2 name; the parameters' names are ours): POLICEMANAGER_ON's
    void SetActive(bool active, int unknown, bool unknown2);
    // The police's people and their cars (PS2 names): POLICEMANAGER_ADD's and DEL's
    void AddPerson(C_human *human);
    void DelPerson(C_human *human);
    void DriveCar(C_human *human, C_car *car);
    // PS2 name: POLICEMANAGER_FORCEARREST's (the parameters' names are ours)
    void ForceArrest(C_human *human, bool a, bool b);
    // What the police know of a human (PS2 names), the GET_PM_ commands': its status bits, its record's state, the
    // times of its crash and of its fire, the number of people angry at it ("pocet nasranych") and of those alive
    // ("zivych"); JakPicusNaTomJest is Czech, rudely, for "how is the guy doing", 0 without a record
    uint GetHumanStatus(C_human *human);
    int JakPicusNaTomJest(C_human *human);
    uint GetCrashTime(C_human *human);
    uint GetFireTime(C_human *human);
    int PocetNasranych(C_human *human);
    int PocetNasranychZivych(C_human *human);
    // A flag of the human's record, while unknown_38 is clear (PS2 name, Czech, roughly "doesn't know which cop he ran
    // into")
    bool NeviDoKterePiceVlezl(C_human *human);
    // The actors TickDoRiciActors ticks (PS2 names): ACTOR_ADDDORICI's and DELDORICI's
    void AddDoRiciActor(C_actor *act);
    void DelDoRiciActor(C_actor *act);
    // More policemen for the human (PS2 name, Czech for "more guys to the party"): POLICE_SUPPORT's (the parameters'
    // names are ours)
    void DalsiChalpiDoPartie(C_human *human, uint unknown, float unknown2);

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours)
    // FUNCTION: GAME 0x47b4e0
    bool GetUnknown0() const
    {
        return unknown_0;
    }
    // PS2 writes the members directly
    // FUNCTION: GAME 0x47b4f0
    void SetSpeed(float speed1, float speed2)
    {
        unknown_c = speed1;
        unknown_10 = speed2;
    }
};

// The copies of the engine's math the original has right after Tick (0x49b7a0-0x49b853, I3d_math.h's): S_vector2's
// operator*(float) (whose marker is g_indicators.h's), operator+= and Normalize, and S_vector::IsNull
// LIBRARY: GAME 0x49b7c0
// ??YS_vector2@@QAEAAU0@ABU0@@Z

// LIBRARY: GAME 0x49b7e0
// ?Normalize@S_vector2@@QAEXXZ

// LIBRARY: GAME 0x49b820
// ?IsNull@S_vector@@QBE_NXZ

// The size of unknown_9c, which TestOverRed calls out of line: the copy the linker kept is among the accessors at the
// end of C_program.cpp's file
// TEMPLATE: GAME 0x47bc00
// ?size@?$vector@GV?$allocator@G@std@@@std@@QBEIXZ
