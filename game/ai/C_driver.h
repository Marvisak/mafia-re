#pragma once

#include "Main.h"

#include <vector>

// A path's crossing the driver keeps (PS2 S_LOCKCROSS, C_driver's std::vector of them): what it holds and its PC size
// aren't known yet
struct S_LOCKCROSS
{
    int unknown_0;                      // 0x0
};

// The AI that drives a car (PS2 C_driver, C_driver.cpp; the PC's file is the one at 0x43bd90-0x4435ac, which starts
// with its constructor), at least 0x29c bytes: its constructor sets members up to 0x298. Only what its destructor uses
// so far; PS2's members are in another order, its offsets in the comments.
class C_driver
{
public:
    // A part of the driver's path (the name is ours), 0x70 bytes (PS2's 0x90)
    struct S_unknown_160
    {
        uchar unknown_0[0x70];          // 0x0
    };

    std::vector<uint> unknown_0;        // 0x0 (PS2 0x120, a gif::vector; the elements' type isn't known yet)
    uchar unknown_10[0x150];            // 0x10
    std::vector<S_unknown_160> unknown_160;  // 0x160 (PS2 0x1bc, a gif::vector)
    uchar unknown_170[0x3c];            // 0x170
    std::vector<S_LOCKCROSS> lock_crosses;  // 0x1ac (PS2 0x1f8; the name is ours)
    uchar unknown_1bc[0xe0];            // 0x1bc

    // Not virtual: the chases delete their drivers through it
    ~C_driver();
};
