#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class C_actor;
class C_car;
class C_driver;
class C_roads;

// C_sledovacka::S_ELEMENT::flags: the chase's modes (Tick's: along the roads after the target, straight after it,
// beside it; the names are ours) and the bits whose meaning isn't known yet, by value
#define SLEDF_ROADS 0x1
#define SLEDF_STRAIGHT 0x2
#define SLEDF_BESIDE 0x4
// AddCarEscape's: away from the target along the roads, braking where it is too far behind
#define SLEDF_ESCAPE 0x10
// AddCarMoveTo's: straight to the place (a path of two nodes), or along the roads
#define SLEDF_MOVE_DIRECT 0x20
#define SLEDF_MOVE_ROADS 0x40
#define SLEDF_UNKNOWN_100 0x100
#define SLEDF_UNKNOWN_200 0x200
#define SLEDF_UNKNOWN_400 0x400
#define SLEDF_UNKNOWN_800 0x800
#define SLEDF_UNKNOWN_1000 0x1000
#define SLEDF_UNKNOWN_100000 0x100000
#define SLEDF_UNKNOWN_200000 0x200000
#define SLEDF_UNKNOWN_1000000 0x1000000
#define SLEDF_UNKNOWN_2000000 0x2000000
#define SLEDF_UNKNOWN_4000000 0x4000000
#define SLEDF_UNKNOWN_8000000 0x8000000
#define SLEDF_UNKNOWN_20000000 0x20000000
#define SLEDF_UNKNOWN_40000000 0x40000000
#define SLEDF_UNKNOWN_80000000 0x80000000
// SLEDF_END

// The cars that chase, flee or follow (PS2 C_sledovacka, C_sledovacka.cpp), the game's 0x2b18. PS2's is 0x20 bytes, its
// offsets in the comments; the PC's is 0x24 bytes.
class C_sledovacka
{
public:
    // A car's action (PS2's type; the values' names are ours): ActionStatus's, STATUS_DONE for a car without a chase;
    // GROUP_DISBAND sets its fighting parties' cars to STATUS_FAILED. Tick ends a chase done at its end or near the
    // target (or with the driver stuck), failed where it lost the target, with a dead driver or a car without energy.
    enum E_STATUS
    {
        STATUS_DONE = 1,
        STATUS_FAILED = 2,
        STATUS_DRIVER_DEAD = 3,
        STATUS_NO_ENERGY = 4
    };

    // A bridge of the mission and the road node it is at (PS2 C_sledovacka::S_BRIDGE, whose members are the same; the
    // names are ours), 8 bytes
    struct S_BRIDGE
    {
        C_actor *actor;                 // 0x0
        ushort node;                    // 0x4, an index of C_roads::nodes
    };
    // A car the chases drive (PS2 C_sledovacka::S_ELEMENT, 0x50 bytes), 0x44 bytes
    struct S_ELEMENT
    {
        C_car *car;                     // 0x0
        C_driver *driver;               // 0x4, which Done deletes
        // The names are ours
        C_actor *chased;                // 0x8, target's car (C_human::car) where it has one, else target
        C_actor *target;                // 0xc, AddCarHunt's and AddCarEscape's
        S_vector target_pos;            // 0x10, the place without chased (AddCarHunt's pos without a target)
        uint flags;                     // 0x1c, SLEDF_*
        int status;                     // 0x20, 0 while the chase goes on
        // 0x24, GetVoleMode's (the name after PS2's getter): AddCarHunt's 1 after a target, 2 to a place,
        // AddCarEscape's 3, AddCarMoveTo's 4
        int vole_mode;
        // 0x28, a time in ms Tick counts by the mode: the target out of sight (after it along the roads or
        // straight, past the node limit2), the target's car standing (beside it), the car stopped or waiting
        // (SLEDF_MOVE_ROADS, SLEDF_UNKNOWN_100000); DevaaateeeZasNeznaCas returns it in vole mode 1 with a
        // chased actor
        uint timer;
        // 0x2c, a time in ms Tick counts too: to the next line test of the target (200 or 500 ms), and the car
        // on its roof
        uint timer2;
        uchar unknown_30[4];            // 0x30
        // 0x34, the car in the way, which TickDriver drives around while the driver's DRIVER19C_BLOCKED is set
        // (it takes the driver's blocking_car over; the name is ours)
        C_car *blocking_car;
        // 0x38 and 0x3c, the mode's limits: after the target, the time in ms it may be out of sight
        // (AddCarHunt's) and the path's node past which that time counts (Tick's: -1 at once, 10000 or 100000
        // never); escaping, the squared distance and the speed (m/s) beyond which the car brakes while the
        // target is behind (AddCarEscape's)
        float limit;
        float limit2;
        int horn_time;                  // 0x40, a time in ms: Tick sounds the car's horn while it runs, 200 to 800
    };

    std::vector<S_BRIDGE> bridges;      // 0x0
    std::vector<S_ELEMENT> elements;    // 0x10 (PS2 0xc, a gif::vector)
    // 0x20 (PS2 0x18), the roads the cars find their paths on, the mission's from AddCarHunt (the name is ours: Init's
    // local is roads)
    C_roads *road_net;

    // PS2's static, true from its .data: RemoveCar resets a removed car's level where it is set (or in vole modes 3
    // and 4); the AI's files set it through EnableChangeTables
    static bool change_tables;
    static void EnableChangeTables(bool enable);

    C_sledovacka();
    ~C_sledovacka();

    // PS2 names: C_game's Init and Done call them. Init finds the mission's bridges, Done deletes the drivers and
    // forgets the cars and the bridges.
    void Init();
    void Done();
    // Whether the car is in the chases (PS2 name)
    bool HoniMiHoNekdoZKanclu(C_car *car);
    // PS2 names: the path of a car, by its index in elements (Tick calls them for a car whose path ran out)
    bool RecompilePath(int index);
    bool PreparePath(int index);
    // A car's driving, by its index in elements (PS2 name)
    void TickDriver(int index, uint time);
    // The cars by the time passed, which C_game::Tick calls (PS2 name)
    void Tick(uint time);
    // PS2 names: a car taken from the chases (CAR_REMOVE_DRIVER's), and a car's action
    void RemoveCar(C_car *car);
    // A car gone, taken from the chases (PS2 name, Czech slang), which C_game::InvalidateActor calls for a car
    void VypicujMiToyotu(C_actor *act);
    // PS2 names: a car sent after the target (or to the place), away from it, or to the place, false without a car
    // (PS2's AddCarMoveTo takes a float more), and the status of its chase, STATUS_* (the element's, STATUS_DONE for
    // a car without one; unsigned: ai_follow_manager::Update's switch compares it so). Update calls AddCarHunt,
    // AddCarMoveTo and ActionStatus for a group's car; nothing written calls AddCarEscape yet.
    bool AddCarHunt(C_car *car, C_actor *target, uint flags, uint time, const S_vector &pos, float factor);
    bool AddCarEscape(C_car *car, C_actor *target, const S_vector &pos, bool unknown, float speed, float dist,
        float factor);
    bool AddCarMoveTo(C_car *car, const S_vector &pos, bool direct, bool unknown, uint flags);
    uint ActionStatus(C_car *car);
    // False for a car without a chase
    bool SetActionStatus(C_car *car, E_STATUS status);
    // PS2 names, which the AI's files call: a car's vole_mode, 0 for a car without a chase, and its timer in vole
    // mode 1 with a chased actor, without SLEDF_BESIDE (Czech, roughly "it doesn't know the time again")
    int GetVoleMode(C_car *car);
    uint DevaaateeeZasNeznaCas(C_car *car);
    // The chases in a saved game (PS2 names), which C_game's SaveGame functions call: the save and the load false
    // where they fail
    int SaveGameGetSize();
    bool SaveGameSave(void **buf);
    bool SaveGameLoad(void **buf);
};

// What Init's inlined push_back calls out of line: the bridges' _Ucopy and _Ufill, folded with other vectors' of 8-byte
// elements (the original kept g_indicators.cpp's copies)
// TEMPLATE: GAME 0x604dd0
// ?_Ucopy@?$vector@US_enemy_talk@C_game@@V?$allocator@US_enemy_talk@C_game@@@std@@@std@@IAEPAUS_enemy_talk@C_game@@PBU34@0PAU34@@Z

// TEMPLATE: GAME 0x604e00
// ?_Ufill@?$vector@US_enemy_talk@C_game@@V?$allocator@US_enemy_talk@C_game@@@std@@@std@@IAEXPAUS_enemy_talk@C_game@@IABU34@@Z

// What Tick's second pass calls out of line, its inline budget spent: the elements' size and operator[]
// TEMPLATE: GAME 0x4a9940
// ?size@?$vector@US_ELEMENT@C_sledovacka@@V?$allocator@US_ELEMENT@C_sledovacka@@@std@@@std@@QBEIXZ

// TEMPLATE: GAME 0x4a9970
// ??A?$vector@US_ELEMENT@C_sledovacka@@V?$allocator@US_ELEMENT@C_sledovacka@@@std@@@std@@QAEAAUS_ELEMENT@C_sledovacka@@I@Z

// What the AddCar functions call out of line: the elements' insert(P, x) (AddCarEscape's and AddCarMoveTo's push_back)
// and insert(P, n, x) (AddCarHunt's), and of the road nodes' vector its insert(P, x) (a push_back), insert(P, n, x), the
// erase(P) of pop_back and the copy of erase's body (the last two another file's copies, which the linker kept)
// TEMPLATE: GAME 0x4a9cb0
// ?insert@?$vector@US_ELEMENT@C_sledovacka@@V?$allocator@US_ELEMENT@C_sledovacka@@@std@@@std@@QAEPAUS_ELEMENT@C_sledovacka@@PAU34@ABU34@@Z

// TEMPLATE: GAME 0x4a9f10
// ?insert@?$vector@US_ELEMENT@C_sledovacka@@V?$allocator@US_ELEMENT@C_sledovacka@@@std@@@std@@QAEXPAUS_ELEMENT@C_sledovacka@@IABU34@@Z

// TEMPLATE: GAME 0x4a9750
// ?insert@?$vector@GV?$allocator@G@std@@@std@@QAEPAGPAGABG@Z

// TEMPLATE: GAME 0x4a98f0
// ?erase@?$vector@GV?$allocator@G@std@@@std@@QAEPAGPAG@Z

// TEMPLATE: GAME 0x47c020
// ?insert@?$vector@GV?$allocator@G@std@@@std@@QAEXPAGIABG@Z

// TEMPLATE: GAME 0x47c2c0
// ?copy@std@@YIPAGPAG00@Z

// The chases' saved game (C_sledovacka::SaveGameSave's; the names are ours): after the head, a record of 0xb8 bytes for
// each car whose chase can be saved, then the path nodes (S_car_node, whole)
#define SLEDOVACKA_SAVEGAME_VERSION 6
struct S_sledovacka_savegame_head
{
    ushort version;                     // 0x0, SLEDOVACKA_SAVEGAME_VERSION
    ushort num;                         // 0x2, the records
};
// A car's record: the element's and its driver's members (the driver's by their offsets), the actors by their numbers
// (C_game::GetActorFromGUID's), 0 for none
#pragma pack(push, 1)
struct S_sledovacka_savegame
{
    int car;                            // 0x0
    int chased;                         // 0x4
    int target;                         // 0x8
    uint flags;                         // 0xc
    int status;                         // 0x10
    int vole_mode;                      // 0x14
    uint unknown_28;                    // 0x18
    uint unknown_2c;                    // 0x1c
    S_vector driver_124;                // 0x20
    uchar driver_130[0x1c];             // 0x2c
    uchar driver_14c[0x14];             // 0x48
    uint next_node;                     // 0x5c
    int driver_174;                     // 0x60
    int driver_17c;                     // 0x64
    int driver_180;                     // 0x68
    int driver_184;                     // 0x6c
    ushort driver_188;                  // 0x70
    ushort driver_18a;                  // 0x72
    ushort driver_18c;                  // 0x74
    int driver_190;                     // 0x76
    int driver_194;                     // 0x7a
    uint driver_19c;                    // 0x7e
    int driver_blocking_car;            // 0x82
    int driver_1a4;                     // 0x86
    int driver_1bc;                     // 0x8a
    float driver_218;                   // 0x8e
    int driver_1c4;                     // 0x92
    int driver_1d8;                     // 0x96
    uint driver_flags;                  // 0x9a
    int driver_1e0;                     // 0x9e
    int driver_1fc;                     // 0xa2
    int driver_200;                     // 0xa6
    int driver_208;                     // 0xaa
    ushort num_nodes;                   // 0xae
    float unknown_38;                   // 0xb0
    float unknown_3c;                   // 0xb4
};
#pragma pack(pop)
