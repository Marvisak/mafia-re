#pragma once

#include "ls3df.h"

// The engine's math types (PS2 file I3d_math.cpp). LS3DF exports their out-of-line members one by one, __stdcall
// (QAG/QBG in the decorated names): a dllexport on the struct would export the inline members too.
#define I3D_MATH_API __declspec(dllexport)

#define PI 3.1415927f

struct S_matrix;
struct S_quat;

struct S_vector2 {
    float x, y;

    float Dot(const S_vector2 &v) const
    {
        return x * v.x + y * v.y;
    }

    float Square() const
    {
        return x * x + y * y;
    }

    I3D_MATH_API float __stdcall AngleTo(const S_vector2 &v) const;
};

struct S_vector {
    float x, y, z;

    // PS2 has it out of line (__ct__8S_vectorFv)
    S_vector() {}

    // The out-of-line copies of this constructor and Dot the linker kept are in camera code
    // FUNCTION: LS3DF 0x1000dd60
    S_vector(float x, float y, float z) : x(x), y(y), z(z) {}

    // PS2 has these out of line (Dot__8S_vectorCFRC8S_vector, Cross__8S_vectorCFRC8S_vector)
    // FUNCTION: LS3DF 0x1000ddb0
    float Dot(const S_vector &v) const
    {
        return x * v.x + y * v.y + z * v.z;
    }

    // Sets the vector to v normalized (the name is a guess). The out-of-line copy the linker kept is in collision code.
    // FUNCTION: LS3DF 0x10014970
    void SetNormalized(const S_vector &v)
    {
        x = v.x;
        y = v.y;
        z = v.z;
        float sq = v.Dot(v);
        if (fabs(sq - 1.0f) < MRG_ZERO) {
            x = v.x;
            y = v.y;
            z = v.z;
            return;
        }
        if (sq < MRG_ZERO) {
            if (v.x != 0.0f)
                x = v.x < 0.0f ? -1.0f : 1.0f;
            else if (v.z != 0.0f)
                z = v.z < 0.0f ? -1.0f : 1.0f;
            else
                y = v.y < 0.0f ? -1.0f : 1.0f;
            return;
        }
        float f = 1.0f / (float)sqrt(sq);
        x = v.x * f;
        y = v.y * f;
        z = v.z * f;
    }

    // FUNCTION: LS3DF 0x1002e360
    S_vector Cross(const S_vector &v) const
    {
        return S_vector(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }

    float Square() const
    {
        return x * x + y * y + z * z;
    }

    // PS2 Magnitude__8S_vectorCFv
    // FUNCTION: LS3DF 0x1002f9f0
    float Magnitude() const
    {
        float sq = Square();
        if (IsMrgZeroLess(sq))
            return 0.0f;
        if (fabs(sq - 1.0f) < MRG_ZERO)
            return 1.0f;
        return (float)sqrt(sq);
    }

    // PS2 Normalize__8S_vectorFv. A null vector becomes the unit vector along its first non-zero axis.
    void Normalize()
    {
        float sq = Square();
        if (fabs(sq - 1.0f) < MRG_ZERO)
            return;
        if (sq < MRG_ZERO) {
            if (x != 0.0f)
                x = x < 0.0f ? -1.0f : 1.0f;
            else if (z != 0.0f)
                z = z < 0.0f ? -1.0f : 1.0f;
            else
                y = y < 0.0f ? -1.0f : 1.0f;
            return;
        }
        float f = 1.0f / (float)sqrt(sq);
        x *= f;
        y *= f;
        z *= f;
    }

    // PS2 __pl__8S_vectorCFRC8S_vector
    S_vector operator+(const S_vector &v) const
    {
        return S_vector(x + v.x, y + v.y, z + v.z);
    }

    S_vector operator-(const S_vector &v) const
    {
        return S_vector(x - v.x, y - v.y, z - v.z);
    }

    S_vector operator-() const
    {
        return S_vector(-x, -y, -z);
    }

    // PS2 __ml__8S_vectorCFRCf: the factor by reference, so it is read again for each component
    S_vector operator*(const float &f) const
    {
        return S_vector(x * f, y * f, z * f);
    }

    I3D_MATH_API float __stdcall CosAngleTo(const S_vector &v) const;
    I3D_MATH_API float __stdcall AngleTo(const S_vector &v) const;
    I3D_MATH_API S_vector &__stdcall operator*=(const S_matrix &m);
    I3D_MATH_API S_vector __stdcall operator*(const S_matrix &m) const;
    I3D_MATH_API S_vector __stdcall RotateByMatrix(const S_matrix &m) const;
    I3D_MATH_API S_vector __stdcall RotateByNormMatrix(const S_matrix &m) const;
};

// A 4x4 matrix whose rows are the axes and the position (row vectors: v * m)
struct S_matrix {
    float m[4][4];

    // A row as a vector (PS2 __cl__8S_matrixFi)
    S_vector &operator()(int i)
    {
        return *(S_vector *)m[i];
    }

    const S_vector &operator()(int i) const
    {
        return *(const S_vector *)m[i];
    }

    I3D_MATH_API void __stdcall Identity();
    I3D_MATH_API S_matrix __stdcall RotateByMatrix(const S_matrix &mat) const;
    I3D_MATH_API bool __stdcall Inverse(const S_matrix &mat);
    I3D_MATH_API S_matrix &__stdcall operator*=(const S_matrix &mat);
    I3D_MATH_API S_matrix __stdcall operator*(const S_matrix &mat) const;
    I3D_MATH_API S_matrix __stdcall Mult4X4(const S_matrix &mat) const;
    // PS2 has it in S_matrix4 (Make4X4__9S_matrix4FRC9S_matrix4RC9S_matrix4)
    S_matrix &Make4X4(const S_matrix &a, const S_matrix &b);
    I3D_MATH_API void __stdcall SetDir(const S_vector &dir);
    I3D_MATH_API void __stdcall SetDir3(const S_vector &dir, const S_vector &up);
    I3D_MATH_API void __stdcall SetDir(const S_vector &dir, const S_vector &up);
    I3D_MATH_API void __stdcall SetRot3(const S_quat &q);
    I3D_MATH_API void __stdcall SetRot(const S_quat &q);
    I3D_MATH_API void __stdcall SetRot(const S_matrix &mat);
    I3D_MATH_API S_vector __stdcall GetScale() const;
    I3D_MATH_API S_vector __stdcall GetScale2() const;
    I3D_MATH_API float __stdcall GetUScale() const;
};

// PS2 has a copy in every file that uses it (a local IdentityMatrix in .data); here it is constant data
// GLOBAL: LS3DF 0x1009bf48
static const S_matrix IdentityMatrix = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};

// A rotation: s is the cosine of half the angle, v the axis scaled by its sine (PS2 __ct__6S_quatFfRC8S_vector)
struct S_quat {
    float s;
    S_vector v;

    S_quat() {}

    S_quat(float s, const S_vector &v) : s(s), v(v) {}

    S_quat operator*(const S_quat &q) const;

    I3D_MATH_API S_matrix __stdcall RotationMatrix() const;
    I3D_MATH_API S_vector __stdcall GetDir() const;
    I3D_MATH_API void __stdcall Make(const S_vector &axis, float angle);
    I3D_MATH_API void __stdcall Make(const S_matrix &m);
    I3D_MATH_API void __stdcall SetDir(const S_vector &dir, float roll);
    I3D_MATH_API void __stdcall Inverse(S_vector &axis, float &angle) const;
    I3D_MATH_API S_quat __stdcall Slerp(const S_quat &q, float t, bool shortest) const;
    I3D_MATH_API S_quat __stdcall RotateByMatrix(const S_matrix &m) const;
    I3D_MATH_API void __stdcall Normalize();
};

struct S_plane {
    S_vector normal;
    float d;

    I3D_MATH_API bool __stdcall IntersectionLine(const S_plane &p, S_vector &pos, S_vector &dir) const;
};

LS3D_EXPORT float __cdecl RandFloat();
I3D_MATH_API S_matrix __stdcall RotateXMatrix(float angle);
I3D_MATH_API S_matrix __stdcall RotateYMatrix(float angle);
I3D_MATH_API S_matrix __stdcall RotateZMatrix(float angle);
I3D_MATH_API bool __stdcall LineLineIntersect(const S_vector &p1, const S_vector &dir1, const S_vector &p2,
    const S_vector &dir2, float &t1, float &t2);

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
