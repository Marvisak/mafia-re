#pragma once

#include "Main.h"

#include <vector>

class I3D_visual;
class I3D_mesh_object;

// PS2's C_tshift and S_tshift_elem (tshift.cpp): elements ticked with the game's time, 0x1c bytes each, the size the
// PC's Tick (0x560510) walks them by: the meshes of lightmapped and plain objects, which the mission's scene2.bin lists
// (C_mission::OpenBin). The members' names are ours.
struct S_tshift_elem
{
    I3D_mesh_object *mesh;              // 0x0
    I3D_visual *visual;                 // 0x4
    int unknown_8;
    int unknown_c;
    int unknown_10;
    int unknown_14;
    ushort unknown_18;

    // Fills an element (the name is ours): not a constructor, the original's keeps this in ecx and returns nothing.
    // Open: VC6 orders the stores otherwise, whatever the statements' order.
    // FUNCTION: GAME 0x545250
    void Init(I3D_visual *v, I3D_mesh_object *m, int a, int b, ushort c)
    {
        mesh = m;
        visual = v;
        unknown_8 = a;
        unknown_c = b;
        unknown_10 = 0;
        unknown_14 = 0;
        unknown_18 = c;
    }
};

class C_tshift
{
public:
    std::vector<S_tshift_elem> elems;   // 0x0

    C_tshift();
    ~C_tshift();

    // The name is ours
    // FUNCTION: GAME 0x545280
    void Add(const S_tshift_elem &elem)
    {
        elems.push_back(elem);
    }
};

// The constructor, folded with C_blood_manager's: the name our PDB keeps for the two changes with the build (foldfix)
// FUNCTION: GAME 0x5604f0
// ??0C_blood_manager@@QAE@XZ

// The elements' erase, which C_mission::Close's inlined clear calls (Mission.cpp's copy)
// TEMPLATE: GAME 0x547a10
// std::vector<S_tshift_elem,std::allocator<S_tshift_elem> >::erase
