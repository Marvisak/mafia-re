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

// The constructor, folded with C_blood_manager's: the name our PDB keeps for the two changes with the build (foldfix)
// FUNCTION: GAME 0x5604f0
// ??0C_tshift@@QAE@XZ

// The elements' erase, which C_mission::Close's inlined clear calls (Mission.cpp's copy)
// TEMPLATE: GAME 0x547a10
// std::vector<S_tshift_elem,std::allocator<S_tshift_elem> >::erase
