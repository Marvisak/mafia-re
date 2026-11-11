#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

// A path's crossing the driver keeps (PS2 S_LOCKCROSS, C_driver's std::vector of them): what it holds and its PC size
// aren't known yet
struct S_LOCKCROSS
{
    int unknown_0;                      // 0x0
};

// C_driver::unknown_19c's bits (the names by value where the meaning isn't known)
#define DRIVER19C_UNKNOWN_10 0x10        // AddCarMoveTo's straight to the place
// A car is in the way (blocking_car): TickDriver drives around it; VypicujMiToyotu clears it where it forgets the
// car or unknown_1a4 (the name is ours)
#define DRIVER19C_BLOCKED 0x40000000

// The AI that drives a car (PS2 C_driver, C_driver.cpp; the PC's file is the one at 0x43bd90-0x4435ac, which starts
// with its constructor), at least 0x29c bytes: its constructor sets members up to 0x298. Only what its destructor uses
// so far; PS2's members are in another order, its offsets in the comments.
class C_actor;
class C_car;

struct S_road_lanes;

// A node of a driver's path (PS2 S_car_node), 0x70 bytes (PS2's 0x90): the chases clear it with memset. pos holds the
// node's position on each lane of its road it can be driven on (num_lanes of them; the first one also where the road
// has no lanes, and the only one the chases set), distance and lane the same lanes' (PS2's four vectors of 0x10 bytes,
// four floats and four bytes likewise)
struct S_car_node
{
    S_vector pos[4];                    // 0x0
    float distance[4];                  // 0x30, from the previous node's position on each lane (the name is ours)
    uchar lane[4];                      // 0x40, the road's lane of each position, an index of S_road_lanes::lane
    uchar cur_lane;                     // 0x44, an index of pos, the lane the car drives on (the name is ours)
    uchar num_lanes;                    // 0x45, the count of pos (the name is ours)
    uchar num_type2;                    // 0x46, of those the lanes of type 2 (the name is ours)
    uchar unknown_47;                   // 0x47
    float unknown_48;                   // 0x48
    float unknown_4c;                   // 0x4c
    uchar unknown_50[4];                // 0x50
    bool unknown_54;                    // 0x54
    uchar unknown_55[0xb];              // 0x55
    ushort road_node;                   // 0x60, a node of C_roads (the name is ours)
    uchar unknown_62[2];                // 0x62
    // 0x64, the road's lanes from the last road node before it towards the next (PreparePath's; the name is ours)
    S_road_lanes *lanes;
    // 0x68, the semaphore phase the node waits at, one of C_game's two or a bridge's (C_sledovacka::Tick takes a car
    // stopped at a light other than green for waiting there; the name is ours)
    int *semaphore_phase;
    // 0x6c, RecompilePath's: the positions are final (a link's, or a crossing's once moved; the name is ours)
    bool fixed;
    uchar unknown_6d[3];                // 0x6d
};

class C_driver
{
public:
    std::vector<uint> unknown_0;        // 0x0 (PS2 0x120, a gif::vector; the elements' type isn't known yet)
    uchar unknown_10[0x74];             // 0x10
    float unknown_84;                   // 0x84
    uchar unknown_88[0x9c];             // 0x88
    // From 0x124 to 0x218 what the chases' saved game keeps of the driver (C_sledovacka::SaveGameSave), each member by
    // its size there; the arrays are copied whole
    S_vector unknown_124;               // 0x124
    uchar unknown_130[0x1c];            // 0x130
    uchar unknown_14c[0x14];            // 0x14c
    // The names are ours
    std::vector<S_car_node> path;       // 0x160 (PS2 0x1bc, a gif::vector)
    uint next_node;                     // 0x170, the node of path the car drives to
    int unknown_174;                    // 0x174
    uchar unknown_178[4];               // 0x178
    int unknown_17c;                    // 0x17c
    int unknown_180;                    // 0x180
    int unknown_184;                    // 0x184
    ushort unknown_188;                 // 0x188
    ushort unknown_18a;                 // 0x18a
    ushort unknown_18c;                 // 0x18c
    uchar unknown_18e[2];               // 0x18e
    int unknown_190;                    // 0x190
    int unknown_194;                    // 0x194
    uchar unknown_198[4];               // 0x198
    uint unknown_19c;                   // 0x19c, DRIVER19C_* bits
    // 0x1a0, the car in the way (DRIVER19C_BLOCKED; the name is ours), and 0x1a4 an actor C_sledovacka::VypicujMiToyotu
    // forgets as it does the car (clearing DRIVER19C_BLOCKED)
    C_car *blocking_car;
    C_actor *unknown_1a4;
    uchar unknown_1a8[4];               // 0x1a8
    std::vector<S_LOCKCROSS> lock_crosses;  // 0x1ac (PS2 0x1f8; the name is ours)
    int unknown_1bc;                    // 0x1bc
    uchar unknown_1c0[4];               // 0x1c0
    int unknown_1c4;                    // 0x1c4
    uchar unknown_1c8[0x10];            // 0x1c8
    int unknown_1d8;                    // 0x1d8
    uint flags;                         // 0x1dc
    int unknown_1e0;                    // 0x1e0
    uchar unknown_1e4[0x18];            // 0x1e4
    // 0x1fc and 0x200, actors or 1 (the chases' saved game and VypicujMiToyotu take values above 1 for actors)
    C_actor *unknown_1fc;
    C_actor *unknown_200;
    C_actor *unknown_204;               // 0x204
    int unknown_208;                    // 0x208
    uchar unknown_20c[8];               // 0x20c
    bool unknown_214;                   // 0x214
    uchar unknown_215[3];               // 0x215
    float unknown_218;                  // 0x218
    uchar unknown_21c[4];               // 0x21c
    uint unknown_220;                   // 0x220, a time in ms
    uchar unknown_224[0x74];            // 0x224
    bool unknown_298;                   // 0x298
    uchar unknown_299[3];               // 0x299

    // PS2's (a stub so far), which the chases' AddCarHunt, AddCarMoveTo and SaveGameLoad call through new
    C_driver();
    // Not virtual: the chases delete their drivers through it
    ~C_driver();
    // The car it drives (PS2 name; a stub so far), which the chases call after new
    void SetCar(C_car *car);
    // PS2 name
    void Tick(uint time);
};

// The path's vector helpers C_sledovacka::Tick calls out of line (that file's copies but insert(P, n, x), this
// file's): operator[], push_back's insert(P, x), clear's erase and the copy of erase's inlined body
// TEMPLATE: GAME 0x4a99a0
// ??A?$vector@US_car_node@@V?$allocator@US_car_node@@@std@@@std@@QAEAAUS_car_node@@I@Z

// TEMPLATE: GAME 0x4a99c0
// ?insert@?$vector@US_car_node@@V?$allocator@US_car_node@@@std@@@std@@QAEPAUS_car_node@@PAU3@ABU3@@Z

// TEMPLATE: GAME 0x4a9ec0
// ?erase@?$vector@US_car_node@@V?$allocator@US_car_node@@@std@@@std@@QAEPAUS_car_node@@PAU3@0@Z

// TEMPLATE: GAME 0x4aa250
// ?copy@std@@YIPAUS_car_node@@PAU2@00@Z

// TEMPLATE: GAME 0x4431d0
// ?insert@?$vector@US_car_node@@V?$allocator@US_car_node@@@std@@@std@@QAEXPAUS_car_node@@IABU3@@Z
