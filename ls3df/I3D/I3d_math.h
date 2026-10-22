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

    S_vector2();
    S_vector2(float x, float y);

    // The lit object file's copy
    // FUNCTION: LS3DF 0x100268e0
    float Dot(const S_vector2 &v) const
    {
        return x * v.x + y * v.y;
    }

    float Square() const
    {
        return x * x + y * y;
    }

    float Magnitude() const;

    S_vector2 operator-(const S_vector2 &v) const;
    // Normalized unless null (the name is a guess)
    void Normalize();
    // The distance to the line through p along dir, positive on its left (Insanity3D's name)
    float DistanceToLine(const S_vector2 &p, const S_vector2 &dir) const;

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

    // The out-of-line copy the linker kept is in collision code
    // FUNCTION: LS3DF 0x10012500
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
    // __forceinline: the original has no out-of-line copy of it, while the linker kept one of every other inline of
    // the vector, and it is inlined wherever a nested call would be starved of inline budget (in GetDir and
    // GetWorldDir: the light's constructor, I3D_target::UpdateFrames, the mirror's UpdateVisibility and
    // PrepareReflection; after the camera's starved GetNormal calls; LightMap::m_ConvertRGB). Plain inline, those stay
    // out of line in ours, and no amount of source in the callers (dead code worth 70 statements in the mirror's
    // UpdateVisibility) or fewer later inline calls brings them in. Forced, its body still charges the budget:
    // S_quat::Make(const S_matrix &) then inlines S_quat::operator*, which the original calls (see there).
    __forceinline void Normalize()
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

    // PS2 __ml__8S_vectorCFRCf: the factor by reference, so it is read again for each component. Before operator+:
    // the collision file's out-of-line copies of the two, deferred items of the precompiled header, come in header
    // order.
    // FUNCTION: LS3DF 0x100112a0
    S_vector operator*(const float &f) const
    {
        return S_vector(x * f, y * f, z * f);
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

    S_vector &operator-=(const S_vector &v);

    S_vector operator-() const
    {
        return S_vector(-x, -y, -z);
    }

    // Insanity3D's. Through these the members are addressed from the vector, not from the struct that holds it
    // (S_quat::Make(const S_vector &, float)). The single meshes' file has the out-of-line copy, for its
    // I3D_joint::GetJointMatrix.
    // FUNCTION: LS3DF 0x10056e60
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
    // The reciprocal is computed into the parameter: a local for it kept the divisor on the FPU stack in
    // ClipFacesPersp's tessellation step (fld 1.0; fdiv st(3)), where the original consumes it (fxch st(2);
    // fdivr [1.0]); the parameter also gives I3D_part_element::Tick's color step, and changes nothing else in the DLL.
    S_vector operator/(float f) const
    {
        f = 1.0f / f;
        return S_vector(x * f, y * f, z * f);
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
    S_plane(const S_vector &n, float d1);
    S_plane(const S_vector &v0, const S_vector &v1, const S_vector &v2);
    void ComputePlane(const S_vector &v0, const S_vector &v1, const S_vector &v2);
    __forceinline void NormalizeNormal();

    I3D_MATH_API bool __stdcall IntersectionLine(const S_plane &pl, S_vector &xpt, S_vector &xdir) const;
};

// The vectors' empty constructor, out of line for the vector constructor iterator (the scene's weather particles and
// regions): the scene file's copy, folded with S_plane's (our PDB keeps either name, S_plane's until I3d_area.cpp,
// from clipping.cpp's CheckClipFaces to I3d_pose.cpp, from I3d_singlemesh.cpp to I3d_occluder.cpp and in
// I3d_particle.cpp, the lit object file's VERTEX_STRUCT's since I3d_lit_object.cpp)
// FUNCTION: LS3DF 0x1004e960
// ??0VERTEX_STRUCT@@QAE@XZ

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

    // Grows the box to hold the point (PS2 Absorb__8I3D_bboxFRC8S_vector). The out-of-line copy the linker kept is in
    // collision code.
    // FUNCTION: LS3DF 0x10014dd0
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
// compiled with /YX (see tools/autopch.py). Besides its $E, the collision tests, the occluder's, the projector's and
// the shadows' visual's constructors and render.cpp's mirrors read it. PS2's files also construct a ZeroSQuat; the
// PC's construct only the vector. Every file's copy is annotated at the end of this header.
static S_vector ZeroSVector(0.0f, 0.0f, 0.0f);

// PS2 has a copy in every file that uses it (a local IdentityMatrix in .data, after the zero vector); here it is
// constant data, which an S_matrix can't be (no initializer list with a base class), so the D3DMATRIX.
// Every object of the original that includes the header has a copy, first in its .rdata (58; clipper.cpp's at
// 0x1009b2c0), though only 12 read theirs. VC6 puts one into an object only where the source references it, inline
// functions nobody calls included, so the header has one every file sees: I3DGetIdentityMatrix (Insanity3D's name,
// which has it out of line), never called. After the zero vector: its body before it would move the files' $Es (the
// driver's one function late, tools/autopch.py). Every file's copy is annotated at the end of this header.
static const D3DMATRIX IdentityMatrixData = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
};
#define IdentityMatrix (*(const S_matrix *)&IdentityMatrixData)
inline const S_matrix &I3DGetIdentityMatrix() { return IdentityMatrix; }

// The lit objects' (Construct). Defined after the zero vector, as inline code before it moves the files' $Es (see
// ZeroSVector). Magnitude too, though the original's copy comes before Dot's and the lit object file's $E: defined
// in the class, it moves the driver's $E past DrawTriangles.
inline S_vector2::S_vector2() {}

// FUNCTION: LS3DF 0x100268c0
inline float S_vector2::Magnitude() const
{
    return (float)sqrt(x * x + y * y);
}

inline S_vector2::S_vector2(float x1, float y1) : x(x1), y(y1) {}

inline S_vector2 S_vector2::operator-(const S_vector2 &v) const
{
    return S_vector2(x - v.x, y - v.y);
}

inline void S_vector2::Normalize()
{
    float len = Magnitude();
    if (len > MRG_ZERO) {
        float r = 1.0f / len;
        x *= r;
        y *= r;
    }
}

inline float S_vector2::DistanceToLine(const S_vector2 &p, const S_vector2 &dir) const
{
    S_vector2 n(-dir.y, dir.x);
    n.Normalize();
    return Dot(n) - p.Dot(n);
}

// Insanity3D's. Defined after the zero vector, as inline code before it moves the files' $Es (see ZeroSVector): the
// portals' ParsePortal copies a vertex and subtracts the next in place, its z left dead.
inline S_vector &S_vector::operator-=(const S_vector &v)
{
    x -= v.x;
    y -= v.y;
    z -= v.z;
    return *this;
}

// Insanity3D's: the occluders' UpdateShadow assigns its plane along the view through a temporary of it
inline S_plane::S_plane(const S_vector &n, float d1) : normal(n), d(d1) {}

// Through the constructor VC6 inlines ComputePlane's calls as the area's UpdateV does; the three calls written at the
// call site sum the normal's squares and scale it in another order, and ComputePlane called there in yet another.
inline S_plane::S_plane(const S_vector &v0, const S_vector &v1, const S_vector &v2)
{
    ComputePlane(v0, v1, v2);
}

// Through the constructor VC6 inlines the three calls as the area's UpdateV does; the three calls written at the call
// site sum the normal's squares and scale it in another order, and ComputePlane called there in yet another.
inline void S_plane::ComputePlane(const S_vector &v0, const S_vector &v1, const S_vector &v2)
{
    normal.GetNormal(v0, v1, v2);
    NormalizeNormal();
    d = -normal.Dot(v0);
}

// The plane's own normalization, not S_vector::Normalize: where a null normal gets its unit vector, the planes built by
// ComputePlane (the area's and occluder's UpdateV, the occluder's UpdateShadow, the portal's UpdatePortal) store x's
// and y's -1 or 1 as integer constants, which these if/else statements give, while Normalize's ternaries store y
// through the FPU, as the vectors normalized directly need (S_quat::Inverse, the light's UpdateV, UnmapScreenPoint).
// __forceinline like Normalize: written out in ComputePlane it costs ComputePlane the budget that inlines GetNormal
// in the portal's UpdatePortal, and as a plain inline function it is starved there; ComputePlane itself forced
// inlines more than the original in the occluder's UpdateShadow. (The name is a guess: nothing names it.)
__forceinline void S_plane::NormalizeNormal()
{
    float sq = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;
    if (fabs(sq - 1.0f) < MRG_ZERO)
        return;
    if (sq < MRG_ZERO) {
        if (normal.x != 0.0f) {
            if (normal.x < 0.0f)
                normal.x = -1.0f;
            else
                normal.x = 1.0f;
        } else if (normal.z != 0.0f) {
            if (normal.z < 0.0f)
                normal.z = -1.0f;
            else
                normal.z = 1.0f;
        } else {
            if (normal.y < 0.0f)
                normal.y = -1.0f;
            else
                normal.y = 1.0f;
        }
        return;
    }
    float f = 1.0f / (float)sqrt(sq);
    normal.x *= f;
    normal.y *= f;
    normal.z *= f;
}

// Globals.cpp (PS2 names)
extern I3D_bbox UnitBBox;
extern S_vector UnitVector;
extern S_vector ZeroVector;

// The box's eight corners, in a buffer of its own
const S_vector *BBoxVertices(const I3D_bbox &bbox);

// The guard of BBoxVertices' static buffer, and the function its first call registers with atexit (it has nothing
// to destroy). VC6 numbers both per file, after the zero vector's three numbers, so the names hold only while no
// other file has them. Other files have a $E13 too: reccmp pairs the annotations of a name with our functions of that
// name in address order, so this one holds with every earlier file's annotated (the sixth, after render.cpp's, I3D.h).
// GLOBAL: LS3DF 0x101c5290
// $S12
// SYNTHETIC: LS3DF 0x1006bc90
// $E13

// More of Globals.cpp's $Es, annotated for later files' of the same names: the destructor StaticInstance's thunk
// registers (the ninth $E5 in link order, after T&l.cpp's), UnitBBox's initializer (the third $E10, after the driver's
// and the model file's) and ZeroVector's (the fifth $E18, after render.cpp's)
// SYNTHETIC: LS3DF 0x1006b660
// $E5

// SYNTHETIC: LS3DF 0x1006bb80
// $E10

// SYNTHETIC: LS3DF 0x1006bcc0
// $E18

// The smaller and the larger of two values, through references (PS2 Min<T>, which the sort lists' Sort calls): the
// clamps of S_vector::AngleTo and the sectors' SetSoundData take the addresses of their bounds. After the zero vector,
// with the other inline code that mustn't move the files' $Es.
template<class T>
inline const T &Min(const T &a, const T &b)
{
    return a < b ? a : b;
}

template<class T>
inline const T &Max(const T &a, const T &b)
{
    return a > b ? a : b;
}

// Every file's ZeroSVector and IdentityMatrixData in the original, in link order. reccmp pairs globals of one name
// with ours in address order (without a warning), so all of them are annotated: one left out would give each file
// after it the next file's. The files' $Es pair through the vectors too: reccmp matches the .CRT$XCU entries by the
// matched globals they write.

// clipper.cpp
// GLOBAL: LS3DF 0x100aef6c
// ZeroSVector
// GLOBAL: LS3DF 0x1009b2c0
// IdentityMatrixData

// clipping.cpp
// GLOBAL: LS3DF 0x100af7cc
// ZeroSVector
// GLOBAL: LS3DF 0x1009b320
// IdentityMatrixData

// clipping2.cpp
// GLOBAL: LS3DF 0x100af7d8
// ZeroSVector
// GLOBAL: LS3DF 0x1009b360
// IdentityMatrixData

// I3d_animation.cpp
// GLOBAL: LS3DF 0x100af7e4
// ZeroSVector
// GLOBAL: LS3DF 0x1009b3a0
// IdentityMatrixData

// I3d_area.cpp
// GLOBAL: LS3DF 0x100af7f0
// ZeroSVector
// GLOBAL: LS3DF 0x1009b420
// IdentityMatrixData

// I3d_billboard.cpp
// GLOBAL: LS3DF 0x100af800
// ZeroSVector
// GLOBAL: LS3DF 0x1009b4d0
// IdentityMatrixData

// I3d_camera.cpp
// GLOBAL: LS3DF 0x100af80c
// ZeroSVector
// GLOBAL: LS3DF 0x1009b690
// IdentityMatrixData

// i3d_collision.cpp
// GLOBAL: LS3DF 0x100af838
// ZeroSVector
// GLOBAL: LS3DF 0x1009b778
// IdentityMatrixData

// I3d_driver.cpp
// GLOBAL: LS3DF 0x100afbac
// ZeroSVector
// GLOBAL: LS3DF 0x1009b7f0
// IdentityMatrixData

// I3d_dummy.cpp
// GLOBAL: LS3DF 0x100c0010
// ZeroSVector
// GLOBAL: LS3DF 0x1009b8e0
// IdentityMatrixData

// I3d_frame.cpp
// GLOBAL: LS3DF 0x100c001c
// ZeroSVector
// GLOBAL: LS3DF 0x1009b978
// IdentityMatrixData

// I3d_interpolator.cpp
// GLOBAL: LS3DF 0x100c004c
// ZeroSVector
// GLOBAL: LS3DF 0x1009ba18
// IdentityMatrixData

// I3d_joint.cpp
// GLOBAL: LS3DF 0x100c0058
// ZeroSVector
// GLOBAL: LS3DF 0x1009ba68
// IdentityMatrixData

// I3d_landscape.cpp
// GLOBAL: LS3DF 0x100c0068
// ZeroSVector
// GLOBAL: LS3DF 0x1009baf8
// IdentityMatrixData

// I3d_light.cpp
// GLOBAL: LS3DF 0x101b42ec
// ZeroSVector
// GLOBAL: LS3DF 0x1009bc40
// IdentityMatrixData

// I3d_lit_object.cpp: GetInvMatrix and GetRenderMatrix use the matrix in world space
// GLOBAL: LS3DF 0x101b4308
// ZeroSVector
// GLOBAL: LS3DF 0x1009bcf8
// IdentityMatrixData

// I3d_material.cpp
// GLOBAL: LS3DF 0x101b4318
// ZeroSVector
// GLOBAL: LS3DF 0x1009be60
// IdentityMatrixData

// I3d_math.cpp
// GLOBAL: LS3DF 0x101b4324
// ZeroSVector
// GLOBAL: LS3DF 0x1009bf48
// IdentityMatrixData

// I3d_mesh.cpp
// GLOBAL: LS3DF 0x101b4330
// ZeroSVector
// GLOBAL: LS3DF 0x1009bfa0
// IdentityMatrixData

// I3d_mirror.cpp
// GLOBAL: LS3DF 0x101b434c
// ZeroSVector
// GLOBAL: LS3DF 0x1009c050
// IdentityMatrixData

// I3d_model.cpp
// GLOBAL: LS3DF 0x101b435c
// ZeroSVector
// GLOBAL: LS3DF 0x1009c120
// IdentityMatrixData

// I3d_morph.cpp
// GLOBAL: LS3DF 0x101b4398
// ZeroSVector
// GLOBAL: LS3DF 0x1009c218
// IdentityMatrixData

// I3d_object.cpp
// GLOBAL: LS3DF 0x101b43a4
// ZeroSVector
// GLOBAL: LS3DF 0x1009c2f8
// IdentityMatrixData

// I3d_occluder.cpp: the constructor copies the vector
// GLOBAL: LS3DF 0x101b43b0
// ZeroSVector
// GLOBAL: LS3DF 0x1009c3e0
// IdentityMatrixData

// I3d_particle.cpp: GetRenderMatrix uses the matrix in world space, the renderer as its world matrix
// GLOBAL: LS3DF 0x101b43cc
// ZeroSVector
// GLOBAL: LS3DF 0x1009c478
// IdentityMatrixData

// I3d_portal.cpp
// GLOBAL: LS3DF 0x101b43e0
// ZeroSVector
// GLOBAL: LS3DF 0x1009c6f8
// IdentityMatrixData

// I3d_pose.cpp
// GLOBAL: LS3DF 0x101b45ec
// ZeroSVector
// GLOBAL: LS3DF 0x1009c750
// IdentityMatrixData

// I3d_projector.cpp: the constructor copies the vector; GetInvMatrix and GetRenderMatrix use the matrix in world
// space
// GLOBAL: LS3DF 0x101b45fc
// ZeroSVector
// GLOBAL: LS3DF 0x1009c7c8
// IdentityMatrixData

// I3d_scene.cpp: the matrix is m_RenderWeatherSystem's world matrix
// GLOBAL: LS3DF 0x101b4e3c
// ZeroSVector
// GLOBAL: LS3DF 0x1009c8b8
// IdentityMatrixData

// I3d_sector.cpp
// GLOBAL: LS3DF 0x101b4f1c
// ZeroSVector
// GLOBAL: LS3DF 0x1009ca48
// IdentityMatrixData

// I3d_shadow.cpp: the visual's constructor copies the vector; GetInvMatrix and GetRenderMatrix use the matrix in
// world space
// GLOBAL: LS3DF 0x101b4f30
// ZeroSVector
// GLOBAL: LS3DF 0x1009cb00
// IdentityMatrixData

// I3d_singlemesh.cpp
// GLOBAL: LS3DF 0x101b5088
// ZeroSVector
// GLOBAL: LS3DF 0x1009cc48
// IdentityMatrixData

// I3d_singlemorph.cpp
// GLOBAL: LS3DF 0x101b5094
// ZeroSVector
// GLOBAL: LS3DF 0x1009cd40
// IdentityMatrixData

// I3d_sound.cpp
// GLOBAL: LS3DF 0x101b50a0
// ZeroSVector
// GLOBAL: LS3DF 0x1009ce30
// IdentityMatrixData

// I3d_target.cpp
// GLOBAL: LS3DF 0x101b50ac
// ZeroSVector
// GLOBAL: LS3DF 0x1009cf18
// IdentityMatrixData

// I3d_user.cpp
// GLOBAL: LS3DF 0x101b50c8
// ZeroSVector
// GLOBAL: LS3DF 0x1009cfb8
// IdentityMatrixData

// I3d_visual.cpp
// GLOBAL: LS3DF 0x101b50d4
// ZeroSVector
// GLOBAL: LS3DF 0x1009d048
// IdentityMatrixData

// I3d_volume.cpp
// GLOBAL: LS3DF 0x101b50e0
// ZeroSVector
// GLOBAL: LS3DF 0x1009d118
// IdentityMatrixData

// IBManager.cpp
// GLOBAL: LS3DF 0x101b50ec
// ZeroSVector
// GLOBAL: LS3DF 0x1009d1b0
// IdentityMatrixData

// lightmap.cpp
// GLOBAL: LS3DF 0x101b5138
// ZeroSVector
// GLOBAL: LS3DF 0x1009d1f0
// IdentityMatrixData

// Load.cpp: the vector is right before LoadFramy: the 1-based reads of the frames address 4 bytes before the
// array (load4ds.cpp), which falls in it
// GLOBAL: LS3DF 0x101b5154
// ZeroSVector
// GLOBAL: LS3DF 0x1009d250
// IdentityMatrixData

// Load3ds.cpp
// GLOBAL: LS3DF 0x101c3bd8
// ZeroSVector
// GLOBAL: LS3DF 0x1009d298
// IdentityMatrixData

// load4ds.cpp
// GLOBAL: LS3DF 0x101c3bf0
// ZeroSVector
// GLOBAL: LS3DF 0x1009d2d8
// IdentityMatrixData

// Load5ds.cpp
// GLOBAL: LS3DF 0x101c3c18
// ZeroSVector
// GLOBAL: LS3DF 0x1009d318
// IdentityMatrixData

// Load6ds.cpp
// GLOBAL: LS3DF 0x101c3c24
// ZeroSVector
// GLOBAL: LS3DF 0x1009d358
// IdentityMatrixData

// render.cpp: the mirrors' reflections are drawn with the vector
// GLOBAL: LS3DF 0x101c4c50
// ZeroSVector
// GLOBAL: LS3DF 0x1009d398
// IdentityMatrixData

// T&l.cpp
// GLOBAL: LS3DF 0x101c4d44
// ZeroSVector
// GLOBAL: LS3DF 0x1009d3e0
// IdentityMatrixData

// VBManager.cpp
// GLOBAL: LS3DF 0x101c5008
// ZeroSVector
// GLOBAL: LS3DF 0x1009d430
// IdentityMatrixData

// Globals.cpp
// GLOBAL: LS3DF 0x101c51c0
// ZeroSVector
// GLOBAL: LS3DF 0x1009d480
// IdentityMatrixData

// IGraph.cpp
// GLOBAL: LS3DF 0x101c5414
// ZeroSVector
// GLOBAL: LS3DF 0x1009d4c0
// IdentityMatrixData

// IShow.cpp
// GLOBAL: LS3DF 0x101c5c40
// ZeroSVector
// GLOBAL: LS3DF 0x1009d630
// IdentityMatrixData

// IShowStream.cpp
// GLOBAL: LS3DF 0x101c5dac
// ZeroSVector
// GLOBAL: LS3DF 0x1009d850
// IdentityMatrixData

// ITexture.cpp
// GLOBAL: LS3DF 0x101c61f8
// ZeroSVector
// GLOBAL: LS3DF 0x1009d890
// IdentityMatrixData

// IS_3Dlistener.cpp
// GLOBAL: LS3DF 0x101c62b4
// ZeroSVector
// GLOBAL: LS3DF 0x1009d928
// IdentityMatrixData

// IS_3Dsound.cpp
// GLOBAL: LS3DF 0x101c62c4
// ZeroSVector
// GLOBAL: LS3DF 0x1009d9a0
// IdentityMatrixData

// IS_driver.cpp
// GLOBAL: LS3DF 0x101c62d0
// ZeroSVector
// GLOBAL: LS3DF 0x1009da48
// IdentityMatrixData

// IS_sound.cpp
// GLOBAL: LS3DF 0x101c62f4
// ZeroSVector
// GLOBAL: LS3DF 0x1009dac8
// IdentityMatrixData

// IS_source.cpp
// GLOBAL: LS3DF 0x101c6310
// ZeroSVector
// GLOBAL: LS3DF 0x1009db50
// IdentityMatrixData
