// PS2's tshift.cpp (the PC's file is the one at 0x560410-0x5605bc, docs/game-files.md): S_tshift_elem::Tick
// (0x560410), C_tshift's constructor and C_tshift::Tick (0x560510), PS2's order with its destructor between the last two.
// Written: all of it.

#include "mission/tshift.h"

#include <math.h>

#include "I3D/I3D.h"

// The original copies the shift into the mesh through the FPU (fld, fstp), as an inline function's parameters are
// copied; stores written out copy it with integer moves (the name is ours)
static inline void SetOffsets(float &dst_u, float &dst_v, float u, float v)
{
    dst_u = u;
    dst_v = v;
}

// PS2's also has the shift's change after the wrap, which it adds to the vertices' coordinates. The rounded values in
// locals: subtracted directly, VC6 keeps a double copy of the shift for the subtraction.
// FUNCTION: GAME 0x560410
void S_tshift_elem::Tick(float t)
{
    float du = t * u_speed;
    float dv = t * v_speed;
    u += du;
    if (u > 1.0f)
    {
        float f = floor(u);
        u -= f;
    }
    else if (u < -1.0f)
    {
        float f = ceil(u);
        u -= f;
    }
    v += dv;
    if (v > 1.0f)
    {
        float f = floor(v);
        v -= f;
    }
    else if (v < -1.0f)
    {
        float f = ceil(v);
        v -= f;
    }
    if (flags & TSHIFT_TEXTURE)
        SetOffsets(mesh->u_ofs, mesh->v_ofs, u, v);
    if (flags & TSHIFT_ENVIRONMENT)
        SetOffsets(mesh->env_u_ofs, mesh->env_v_ofs, u, v);
}

// C_blood_manager's constructor (actors/C_blood.cpp) is the same code, folded into this one: the marker is in the header
C_tshift::C_tshift()
{
}

// The same code as C_blood_manager's destructor (actors/C_blood.cpp), whose copy the original kept: the marker is in
// actors/C_blood.h
C_tshift::~C_tshift()
{
}

// FUNCTION: GAME 0x560510
void C_tshift::Tick(uint time)
{
    float t = time * .001f;
    for (int i = elems.size(); i--;)
        elems[i].Tick(t);
}
