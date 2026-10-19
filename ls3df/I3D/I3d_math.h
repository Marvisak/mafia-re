#pragma once

#include "ls3df.h"

#include <d3d8types.h>

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
    // The sum is written out, as in Magnitude: through Dot, the math's callers leave that call out of line.
    // FUNCTION: LS3DF 0x10014970
    void SetNormalized(const S_vector &v)
    {
        x = v.x;
        y = v.y;
        z = v.z;
        float sq = v.x * v.x + v.y * v.y + v.z * v.z;
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

    // PS2 Magnitude__8S_vectorCFv. The sum is written out: a nested Square() call would take inline budget of its
    // own, which S_quat::Make(const S_matrix &) doesn't have where the original inlines the whole computation.
    // FUNCTION: LS3DF 0x1002f9f0
    float Magnitude() const
    {
        float sq = x * x + y * y + z * z;
        if (IsMrgZeroLess(sq))
            return 0.0f;
        if (fabs(sq - 1.0f) < MRG_ZERO)
            return 1.0f;
        return (float)sqrt(sq);
    }

    // PS2 Normalize__8S_vectorFv. A null vector becomes the unit vector along its first non-zero axis. The sum is
    // written out, as in Magnitude.
    // Open: the original has no out-of-line copy, and inlines it where ours runs out of inline budget: nested in
    // GetDir and GetWorldDir (the light's constructor, I3D_target::UpdateFrames, the mirror's UpdateVisibility and
    // PrepareReflection) and after the camera's starved GetNormal calls. Declared __forceinline it gives those four
    // exactly, but then S_quat::Make(const S_matrix &) inlines S_quat::operator* (the original calls it) and leaves
    // later constructors, operator+ and Dot out of line (it charges the budget), so that isn't the whole answer.
    // Neither Square() for the sum, *this *= for the products, an else for the second return, nor if/else statements
    // for the signs inline it in those four, and each scores worse elsewhere.
    void Normalize()
    {
        float sq = x * x + y * y + z * z;
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
    // FUNCTION: LS3DF 0x100112d0
    S_vector operator+(const S_vector &v) const
    {
        return S_vector(x + v.x, y + v.y, z + v.z);
    }

    // PS2 __apl__8S_vectorFRC8S_vector
    S_vector &operator+=(const S_vector &v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }

    // The out-of-line copy the linker kept is in camera code
    // FUNCTION: LS3DF 0x1000dd80
    S_vector operator-(const S_vector &v) const
    {
        return S_vector(x - v.x, y - v.y, z - v.z);
    }

    S_vector operator-() const
    {
        return S_vector(-x, -y, -z);
    }

    // PS2 __ml__8S_vectorCFRCf: the factor by reference, so it is read again for each component
    // FUNCTION: LS3DF 0x100112a0
    S_vector operator*(const float &f) const
    {
        return S_vector(x * f, y * f, z * f);
    }

    // Insanity3D's. Through these the members are addressed from the vector, not from the struct that holds it
    // (S_quat::Make(const S_vector &, float)).
    S_vector &operator*=(const float &f)
    {
        x *= f;
        y *= f;
        z *= f;
        return *this;
    }

    // Insanity3D's, component by component
    S_vector operator*(const S_vector &v) const
    {
        return S_vector(x * v.x, y * v.y, z * v.z);
    }

    // Insanity3D's, component by component
    S_vector &operator*=(const S_vector &v)
    {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        return *this;
    }

    // Insanity3D's: the reciprocal goes into a local, which keeps a stack slot where VC6 inlines it
    // (VertexLightingProcess)
    S_vector operator/(float f) const
    {
        float r = 1.0f / f;
        return S_vector(x * r, y * r, z * r);
    }

    // Insanity3D's
    float Sum() const
    {
        return x + y + z;
    }

    void Zero()
    {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }

    // The normal of the triangle u1, v1, w1: (w1 - v1) x (u1 - v1), not normalized (PS2
    // GetNormal__8S_vectorFRC8S_vectorRC8S_vectorRC8S_vector; Insanity3D has the same arithmetic with six floats). The
    // differences are vectors: the copy the linker kept in camera code keeps one in a frame of its own, and where the
    // camera inlines it late, the two operator- calls stay out of line.
    // FUNCTION: LS3DF 0x1000ddd0
    void GetNormal(const S_vector &u1, const S_vector &v1, const S_vector &w1)
    {
        S_vector u = w1 - v1, v = u1 - v1;
        x = u.y * v.z - u.z * v.y;
        y = u.z * v.x - u.x * v.z;
        z = u.x * v.y - u.y * v.x;
    }

    I3D_MATH_API float __stdcall CosAngleTo(const S_vector &v) const;
    I3D_MATH_API float __stdcall AngleTo(const S_vector &v) const;
    I3D_MATH_API S_vector &__stdcall operator*=(const S_matrix &m);
    I3D_MATH_API S_vector __stdcall operator*(const S_matrix &m) const;
    I3D_MATH_API S_vector __stdcall RotateByMatrix(const S_matrix &m) const;
    I3D_MATH_API S_vector __stdcall RotateByNormMatrix(const S_matrix &m) const;
};

// A 4x4 matrix whose rows are the axes and the position (row vectors: v * m)
// On D3DMATRIX (m[4][4] is its), as D3DX's D3DXMATRIX: with a base class the matrix is no POD, so VC6 constructs a
// returned matrix where it goes (DrawCircle2D, SetWorldDir use it in place, where it copies a POD first), while
// assignment stays its own block copy, which it schedules with the code around (UpdateWMatrixProc), and a static
// matrix needs no guard (DrawSphere). A constructor would give both of the latter.
struct S_matrix : D3DMATRIX {

    // A row as a vector (PS2 __cl__8S_matrixFi)
    S_vector &operator()(int i)
    {
        return *(S_vector *)m[i];
    }

    const S_vector &operator()(int i) const
    {
        return *(const S_vector *)m[i];
    }

    // An element (Insanity3D's S_matrix has the same pair). Its address is computed from the matrix, where a row's
    // member goes through the row's address: m(1, 1) is [m + 0x14], m(1).y is [row + 4].
    float &operator()(int r, int c)
    {
        return m[r][c];
    }

    const float &operator()(int r, int c) const
    {
        return m[r][c];
    }

    I3D_MATH_API void __stdcall Identity();
    I3D_MATH_API S_matrix __stdcall RotateByMatrix(const S_matrix &mat) const;
    I3D_MATH_API bool __stdcall Inverse(const S_matrix &mat);
    // this = a * b without the last column (affine matrices)
    S_matrix &Make(const S_matrix &m1, const S_matrix &m2);
    I3D_MATH_API S_matrix &__stdcall operator*=(const S_matrix &mat);
    I3D_MATH_API S_matrix __stdcall operator*(const S_matrix &mat) const;
    I3D_MATH_API S_matrix __stdcall Mult4X4(const S_matrix &mat) const;
    // PS2 has it in S_matrix4 (Make4X4__9S_matrix4FRC9S_matrix4RC9S_matrix4)
    S_matrix &Make4X4(const S_matrix &m1, const S_matrix &m2);
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

// A rotation: s is the cosine of half the angle, v the axis scaled by its sine (PS2 __ct__6S_quatFfRC8S_vector). PS2's
// layout has the axis first, the PC's s.
struct S_quat {
    float s;
    S_vector v;

    S_quat() {}

    S_quat(float s, const S_vector &v) : s(s), v(v) {}

    // PS2 __ct__6S_quatFffff
    S_quat(float s, float x, float y, float z) : s(s)
    {
        v.x = x;
        v.y = y;
        v.z = z;
    }

    // Insanity3D's name, s first as there. The axis as a chain (the constructors store z, y, x); Zero() is three
    // statements. Through the inline function the pose's constructor sets its zero register where the original does,
    // the statements written out set it at the start; the frame's constructor compiles the same either way.
    void Identity()
    {
        s = 1.0f;
        v.x = v.y = v.z = 0.0f;
    }

    // Out of line after S_quat::Make(const S_matrix &), its only caller that doesn't inline it
    // FUNCTION: LS3DF 0x1002fa70
    S_quat operator*(const S_quat &q) const
    {
        return S_quat(s * q.s - v.Dot(q.v), q.v * s + v * q.s + v.Cross(q.v));
    }

    I3D_MATH_API S_matrix __stdcall RotationMatrix() const;
    I3D_MATH_API S_vector __stdcall GetDir() const;
    I3D_MATH_API void __stdcall Make(const S_vector &axis, float angle);
    I3D_MATH_API void __stdcall Make(const S_matrix &m);
    I3D_MATH_API void __stdcall SetDir(const S_vector &dir, float roll);
    I3D_MATH_API void __stdcall Inverse(S_vector &axis, float &angle) const;
    I3D_MATH_API S_quat __stdcall Slerp(const S_quat &q, float t, bool shorten) const;
    I3D_MATH_API S_quat __stdcall RotateByMatrix(const S_matrix &m) const;
    I3D_MATH_API void __stdcall Normalize();
};

struct S_plane {
    S_vector normal;
    float d;

    // Insanity3D's constructors and ComputePlane: the plane through three points, which the area's UpdateV builds
    // for each face. Defined after the zero vector, as inline code before it moves the files' $Es (see ZeroSVector).
    S_plane() {}
    S_plane(const S_vector &v0, const S_vector &v1, const S_vector &v2);
    void ComputePlane(const S_vector &v0, const S_vector &v1, const S_vector &v2);

    I3D_MATH_API bool __stdcall IntersectionLine(const S_plane &pl, S_vector &xpt, S_vector &xdir) const;
};

// The vectors' empty constructor, out of line for the vector constructor iterator (the scene's weather particles and
// regions): the scene file's copy, folded with S_plane's (our PDB keeps either name, S_plane's until I3d_area.cpp and
// again from clipping.cpp's CheckClipFaces to I3d_pose.cpp)
// FUNCTION: LS3DF 0x1004e960
// ??0S_vector@@QAE@XZ

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

    // Insanity3D's name
    bool IsValid() const { return min.x <= max.x; }

    // An empty box (PS2 name)
    void Invalidate()
    {
        min.x = min.y = min.z = 1e16f;
        max.x = max.y = max.z = -1e16f;
    }

    // Grows the box to hold the point (PS2 Absorb__8I3D_bboxFRC8S_vector)
    void Absorb(const S_vector &v)
    {
        if (min.x > v.x)
            min.x = v.x;
        if (max.x < v.x)
            max.x = v.x;
        if (min.y > v.y)
            min.y = v.y;
        if (max.y < v.y)
            max.y = v.y;
        if (min.z > v.z)
            min.z = v.z;
        if (max.z < v.z)
            max.z = v.z;
    }
};

// A triangle's vertex indices (PS2 I3D_triface). Here, not in I3D.h: the clipper's arrays (clipper.cpp, which
// includes only this header) hold them.
struct I3D_triface {
    ushort vi[3];
};

// Every file that includes the header gets its own (PS2 name). Its $E isn't where it is defined: the files are
// compiled with /YX (see tools/autopch.py). Nothing but its $E references it. PS2's files also construct a ZeroSQuat;
// the PC's construct only the vector.
static S_vector ZeroSVector(0.0f, 0.0f, 0.0f);

// PS2 has a copy in every file that uses it (a local IdentityMatrix in .data, after the zero vector); here it is
// constant data, which an S_matrix can't be (no initializer list with a base class), so the D3DMATRIX. I3d_math.cpp's
// copy; the scene file's is annotated in I3D.h.
// Every object of the original that includes the header has a copy, first in its .rdata (58; clipper.cpp's at
// 0x1009b2c0), though only 12 read theirs. VC6 puts one into an object only where the source references it, inline
// functions nobody calls included, so the header has one every file sees: I3DGetIdentityMatrix (Insanity3D's name,
// which has it out of line), never called. After the zero vector: its body before it would move the files' $Es (the
// driver's one function late, tools/autopch.py).
// GLOBAL: LS3DF 0x1009bf48
static const D3DMATRIX IdentityMatrixData = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};
#define IdentityMatrix (*(const S_matrix *)&IdentityMatrixData)
inline const S_matrix &I3DGetIdentityMatrix() { return IdentityMatrix; }

// Through the constructor VC6 inlines ComputePlane's calls as the area's UpdateV does; the three calls written at the
// call site sum the normal's squares and scale it in another order, and ComputePlane called there in yet another.
inline S_plane::S_plane(const S_vector &v0, const S_vector &v1, const S_vector &v2)
{
    ComputePlane(v0, v1, v2);
}

inline void S_plane::ComputePlane(const S_vector &v0, const S_vector &v1, const S_vector &v2)
{
    normal.GetNormal(v0, v1, v2);
    normal.Normalize();
    d = -normal.Dot(v0);
}

// Globals.cpp (PS2 names)
extern I3D_bbox UnitBBox;
extern S_vector UnitVector;
extern S_vector ZeroVector;

// The box's eight corners, in a buffer of its own
const S_vector *BBoxVertices(const I3D_bbox &bbox);

// The guard of BBoxVertices' static buffer, and the function its first call registers with atexit (it has nothing
// to destroy). VC6 numbers both per file, after the zero vector's three numbers, so the names hold only while no
// other file has them. The scene's file and IGraph.cpp have a $E13 too: reccmp pairs the annotations of a name with our
// functions of that name in address order, so this one holds with the scene's annotated (I3D.h) and IGraph.cpp's not.
// GLOBAL: LS3DF 0x101c5290
// $S12
// SYNTHETIC: LS3DF 0x1006bc90
// $E13
