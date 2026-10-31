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

// The AI that drives a car (PS2 C_driver, C_driver.cpp; the PC's file is the one at 0x43bd90-0x4435ac, which starts
// with its constructor), at least 0x29c bytes: its constructor sets members up to 0x298. Only what its destructor uses
// so far; PS2's members are in another order, its offsets in the comments.
class C_actor;

// A node of a driver's path (PS2 S_car_node), 0x70 bytes (PS2's 0x90): the chases clear it with memset
struct S_car_node
{
    S_vector pos;                       // 0x0
    uchar unknown_c[0x24];              // 0xc
    float unknown_30;                   // 0x30, the distance from the previous node most likely
    uchar unknown_34[0x10];             // 0x34
    bool unknown_44;                    // 0x44
    bool unknown_45;                    // 0x45
    bool unknown_46;                    // 0x46
    uchar unknown_47;                   // 0x47
    float unknown_48;                   // 0x48
    float unknown_4c;                   // 0x4c
    uchar unknown_50[4];                // 0x50
    bool unknown_54;                    // 0x54
    uchar unknown_55[0xb];              // 0x55
    ushort unknown_60;                  // 0x60, a node of C_roads
    uchar unknown_62[6];                // 0x62
    int *unknown_68;                    // 0x68
    uchar unknown_6c[4];                // 0x6c
};

class C_driver
{
public:
    std::vector<uint> unknown_0;        // 0x0 (PS2 0x120, a gif::vector; the elements' type isn't known yet)
    uchar unknown_10[0x150];            // 0x10
    std::vector<S_car_node> unknown_160;  // 0x160 (PS2 0x1bc, a gif::vector), the path
    uint unknown_170;                   // 0x170, the node of unknown_160 the car drives to
    uchar unknown_174[0x2c];            // 0x174
    int unknown_1a0;                    // 0x1a0
    uchar unknown_1a4[8];               // 0x1a4
    std::vector<S_LOCKCROSS> lock_crosses;  // 0x1ac (PS2 0x1f8; the name is ours)
    uchar unknown_1bc[0x20];            // 0x1bc
    uint unknown_1dc;                   // 0x1dc, flags
    uchar unknown_1e0[0x1c];            // 0x1e0
    C_actor *unknown_1fc;               // 0x1fc
    C_actor *unknown_200;               // 0x200
    C_actor *unknown_204;               // 0x204
    int unknown_208;                    // 0x208
    uchar unknown_20c[0x14];            // 0x20c
    uint unknown_220;                   // 0x220, a time in ms
    uchar unknown_224[0x74];            // 0x224
    bool unknown_298;                   // 0x298
    uchar unknown_299[3];               // 0x299

    // Not virtual: the chases delete their drivers through it
    ~C_driver();
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
