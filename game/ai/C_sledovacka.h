#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class C_actor;
class C_car;
class C_driver;
class C_roads;

// The cars that chase, flee or follow (PS2 C_sledovacka, C_sledovacka.cpp), the game's 0x2b18. PS2's is 0x20 bytes, its
// offsets in the comments; the PC's size isn't known yet.
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
        C_actor *target;                // 0xc, AddCarHunt's
        S_vector target_pos;            // 0x10, the place without chased (AddCarHunt's and AddCarMoveTo's pos)
        uint flags;                     // 0x1c, SLEDF_*
        int status;                     // 0x20, 0 while the chase goes on
        uchar unknown_24[4];            // 0x24
        uint unknown_28;                // 0x28, a time in ms
        uint unknown_2c;                // 0x2c, a time in ms
        uchar unknown_30[8];            // 0x30
        float unknown_38;               // 0x38
        float unknown_3c;               // 0x3c
        int horn_time;                  // 0x40, a time in ms: Tick sounds the car's horn while it runs, 200 to 800
    };

    std::vector<S_BRIDGE> bridges;      // 0x0
    std::vector<S_ELEMENT> elements;    // 0x10 (PS2 0xc, a gif::vector)
    // 0x20 (PS2 0x18), the roads the cars find their paths on, the mission's from AddCarHunt (the name is ours: Init's
    // local is roads)
    C_roads *road_net;

    C_sledovacka();
    ~C_sledovacka();

    // PS2 names: C_game's Init and Done call them. Init finds the mission's bridges, Done deletes the drivers and
    // forgets the cars and the bridges.
    void Init();
    void Done();
    // PS2 names: the path of a car, by its index in elements (Tick calls them for a car whose path ran out)
    bool RecompilePath(int index);
    bool PreparePath(int index);
    // A car's driving, by its index in elements (PS2 name)
    void TickDriver(int index, uint time);
    // The cars by the time passed, which C_game::Tick calls (PS2 name)
    void Tick(uint time);
    // PS2 names: a car taken from the chases (CAR_REMOVE_DRIVER's), and a car's action
    void RemoveCar(C_car *car);
    // PS2 names: a car sent after the target or to the place, false without a car (PS2's AddCarMoveTo takes a float
    // more), and the status of its chase, STATUS_* (the element's, STATUS_DONE for a car without one; unsigned:
    // ai_follow_manager::Update's switch compares it so). Update calls them for a group's car.
    bool AddCarHunt(C_car *car, C_actor *target, uint unknown, uint unknown2, const S_vector &pos, float unknown3);
    bool AddCarMoveTo(C_car *car, const S_vector &pos, bool unknown, bool unknown2, uint unknown3);
    uint ActionStatus(C_car *car);
    void SetActionStatus(C_car *car, E_STATUS status);
};

// What Init's inlined push_back calls out of line: the bridges' _Ucopy and _Ufill, folded with other vectors' of 8-byte
// elements (the original kept g_indicators.cpp's copies)
// TEMPLATE: GAME 0x604dd0
// std::vector<C_sledovacka::S_BRIDGE,std::allocator<C_sledovacka::S_BRIDGE> >::_Ucopy

// TEMPLATE: GAME 0x604e00
// std::vector<C_sledovacka::S_BRIDGE,std::allocator<C_sledovacka::S_BRIDGE> >::_Ufill

// What Tick's second pass calls out of line, its inline budget spent: the elements' size and operator[]
// TEMPLATE: GAME 0x4a9940
// ?size@?$vector@US_ELEMENT@C_sledovacka@@V?$allocator@US_ELEMENT@C_sledovacka@@@std@@@std@@QBEIXZ

// TEMPLATE: GAME 0x4a9970
// ??A?$vector@US_ELEMENT@C_sledovacka@@V?$allocator@US_ELEMENT@C_sledovacka@@@std@@@std@@QAEAAUS_ELEMENT@C_sledovacka@@I@Z
