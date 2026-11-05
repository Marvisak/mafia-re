#pragma once

#include "Main.h"

#include <vector>

class I3D_visual;
class I3D_mesh_object;

// PS2's C_tshift and S_tshift_elem (tshift.cpp): elements ticked with the game's time, 0x1c bytes each, the size the
// PC's Tick (0x560510) walks them by: the meshes of lightmapped and plain objects, which the mission's scene2.bin lists
// (C_mission::OpenBin). Each moves its mesh's texture coordinates: the shift grows by its speed and stays between -1
// and 1, and Tick sets it as the mesh's offsets (PS2's adds the shift's change to the vertices' coordinates instead).
// The members' names are ours.
struct S_tshift_elem
{
    I3D_mesh_object *mesh;              // 0x0
    I3D_visual *visual;                 // 0x4
    float u_speed, v_speed;             // 0x8, per second
    float u, v;                         // 0x10, the shift
    ushort flags;                       // 0x18, TSHIFT_*

    // Fills an element (the name is ours): not a constructor, the original's keeps this in ecx and returns nothing.
    // Open: VC6 orders the stores otherwise, whatever the statements' order.
    // FUNCTION: GAME 0x545250
    void Init(I3D_visual *vis, I3D_mesh_object *m, float us, float vs, ushort f)
    {
        mesh = m;
        visual = vis;
        u_speed = us;
        v_speed = vs;
        u = 0.0f;
        v = 0.0f;
        flags = f;
    }
    // Moves the shift by t seconds (PS2 name and parameter)
    void Tick(float t);
};

// S_tshift_elem::flags: which of the mesh's offsets the shift sets, the texture's (u_ofs, v_ofs) or the environment
// map's (the names are guesses)
#define TSHIFT_TEXTURE 1
#define TSHIFT_ENVIRONMENT 2

class C_tshift
{
public:
    std::vector<S_tshift_elem> elems;   // 0x0

    C_tshift();
    ~C_tshift();

    // Moves every element by the game's time, in milliseconds (PS2 name and parameter)
    void Tick(uint time);

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
