#pragma once

// The engine's math types (PS2 file I3d_math.cpp), as far as the files written so far use them. LS3DF exports their
// members (__stdcall, see the export table), which are still to be written.

struct S_vector {
    float x, y, z;

    // PS2 has it out of line (__ct__8S_vectorFv)
    S_vector() {}

    S_vector(float x, float y, float z) : x(x), y(y), z(z) {}
};

// An axis-aligned box (PS2 I3D_bbox)
struct I3D_bbox {
    S_vector min;
    S_vector max;

    I3D_bbox() {}

    // In the body: windows.h's min and max macros would take an initializer list's min(...)
    I3D_bbox(const S_vector &a, const S_vector &b)
    {
        min = a;
        max = b;
    }
};

// Globals.cpp (PS2 names)
extern I3D_bbox UnitBBox;
extern S_vector UnitVector;
extern S_vector ZeroVector;

// The box's eight corners, in a buffer of its own
const S_vector *BBoxVertices(const I3D_bbox &bbox);
