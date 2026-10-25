#pragma once

#include "Main.h"

#include <vector>

// PS2's C_tshift and S_tshift_elem (tshift.cpp): elements ticked with the game's time, 0x1c bytes each, the size the
// PC's Tick (0x560510) walks them by. Only what C_mission's Init and UnInit use so far.
struct S_tshift_elem
{
    uchar unknown_0[0x1c];
};

class C_tshift
{
public:
    std::vector<S_tshift_elem> elems;   // 0x0

    C_tshift();
    ~C_tshift();
};
