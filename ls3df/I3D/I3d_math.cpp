// The math types' functions, at 0x1002cda0-0x1003055f in the original. The PS2 build names the file and has these
// functions from RandFloat to LineLineIntersect. The PC adds RotateXMatrix, S_vector2::AngleTo, Mult4X4, SetRot,
// GetScale2, CosAngleTo and S_quat::GetDir, has S_matrix::RotateByMatrix earlier, and other SetDir and SetDir3
// algorithms. Make4X4 comes right after Mult4X4, which inlines it.
// The header's zero vector (PS2 ZeroSVector) has its dynamic initializer between S_vector::RotateByMatrix and
// RotateByNormMatrix (0x1002ea90), not at the end of the file as in the other engine files; it isn't here yet (see
// ISound/IS_driver.cpp).
// The matrix products are hand-written assembly: Make, Make4X4, S_matrix::RotateByMatrix and the vector-by-matrix
// products. The Xbox build has the same instructions, out of line where VC6 inlines them. Their tells: the inline
// assembler encodes a local's address with a 32-bit displacement (lea esi, r), and a block that reads this or a
// parameter by name keeps it in memory (this spilled to a stack slot).
#include "I3D/i3d_math.h"

#include <stdlib.h>

// The angle comparisons that need a constant other than MRG_ZERO
#define QUAT_ONE_LIMIT (1.0 - MRG_ZERO)
#define SLERP_DELTA 0.0001

// Min and max through references (the clamp in S_vector::AngleTo takes the address of its constants)
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

// FUNCTION: LS3DF 0x1002cda0
float __cdecl RandFloat()
{
    return rand() * (1.0f / RAND_MAX);
}

// FUNCTION: LS3DF 0x1002cdc0
S_matrix __stdcall RotateXMatrix(float angle)
{
    S_matrix m;
    m.Identity();
    float c = (float)cos(angle);
    float s = (float)sin(angle);
    m.m[1][1] = c;
    m.m[1][2] = s;
    m.m[2][1] = -s;
    m.m[2][2] = c;
    return m;
}

// FUNCTION: LS3DF 0x1002ce10
S_matrix __stdcall RotateYMatrix(float angle)
{
    S_matrix m;
    m.Identity();
    float c = (float)cos(angle);
    float s = (float)sin(angle);
    m.m[0][0] = c;
    m.m[0][2] = s;
    m.m[2][0] = -s;
    m.m[2][2] = c;
    return m;
}

// FUNCTION: LS3DF 0x1002ce60
S_matrix __stdcall RotateZMatrix(float angle)
{
    S_matrix m;
    m.Identity();
    float c = (float)cos(angle);
    float s = (float)sin(angle);
    m.m[0][0] = c;
    m.m[0][1] = s;
    m.m[1][0] = -s;
    m.m[1][1] = c;
    return m;
}

// FUNCTION: LS3DF 0x1002ceb0
float S_vector2::AngleTo(const S_vector2 &v) const
{
    float f = (float)sqrt(v.Square() * Square());
    if (f < MRG_ZERO)
        return 0.0f;
    return (float)acos(Dot(v) / f);
}

// FUNCTION: LS3DF 0x1002cf20
void S_matrix::Identity()
{
    *this = IdentityMatrix;
}

// The rotation part only: the position is rotated too, not moved
// FUNCTION: LS3DF 0x1002cf40
S_matrix S_matrix::RotateByMatrix(const S_matrix &mat) const
{
    S_matrix r;
    __asm {
        mov eax, this
        mov ebx, mat
        lea edi, r
        fld dword ptr [eax]
        mov dword ptr [edi + 0xc], 0
        fmul dword ptr [ebx]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [ebx + 0x10]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [ebx + 0x20]
        faddp st(1), st
        fstp dword ptr [edi]
        fld dword ptr [eax]
        fmul dword ptr [ebx + 0x4]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [ebx + 0x14]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [ebx + 0x24]
        faddp st(1), st
        fstp dword ptr [edi + 0x4]
        fld dword ptr [eax]
        fmul dword ptr [ebx + 0x8]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [ebx + 0x18]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [ebx + 0x28]
        faddp st(1), st
        fstp dword ptr [edi + 0x8]
        fld dword ptr [eax + 0x10]
        mov dword ptr [edi + 0x1c], 0
        fmul dword ptr [ebx]
        fld dword ptr [eax + 0x14]
        fmul dword ptr [ebx + 0x10]
        faddp st(1), st
        fld dword ptr [eax + 0x18]
        fmul dword ptr [ebx + 0x20]
        faddp st(1), st
        fstp dword ptr [edi + 0x10]
        fld dword ptr [eax + 0x10]
        fmul dword ptr [ebx + 0x4]
        fld dword ptr [eax + 0x14]
        fmul dword ptr [ebx + 0x14]
        faddp st(1), st
        fld dword ptr [eax + 0x18]
        fmul dword ptr [ebx + 0x24]
        faddp st(1), st
        fstp dword ptr [edi + 0x14]
        fld dword ptr [eax + 0x10]
        fmul dword ptr [ebx + 0x8]
        fld dword ptr [eax + 0x14]
        fmul dword ptr [ebx + 0x18]
        faddp st(1), st
        fld dword ptr [eax + 0x18]
        fmul dword ptr [ebx + 0x28]
        faddp st(1), st
        fstp dword ptr [edi + 0x18]
        fld dword ptr [eax + 0x20]
        mov dword ptr [edi + 0x2c], 0
        fmul dword ptr [ebx]
        fld dword ptr [eax + 0x24]
        fmul dword ptr [ebx + 0x10]
        faddp st(1), st
        fld dword ptr [eax + 0x28]
        fmul dword ptr [ebx + 0x20]
        faddp st(1), st
        fstp dword ptr [edi + 0x20]
        fld dword ptr [eax + 0x20]
        fmul dword ptr [ebx + 0x4]
        fld dword ptr [eax + 0x24]
        fmul dword ptr [ebx + 0x14]
        faddp st(1), st
        fld dword ptr [eax + 0x28]
        fmul dword ptr [ebx + 0x24]
        faddp st(1), st
        fstp dword ptr [edi + 0x24]
        fld dword ptr [eax + 0x20]
        fmul dword ptr [ebx + 0x8]
        fld dword ptr [eax + 0x24]
        fmul dword ptr [ebx + 0x18]
        faddp st(1), st
        fld dword ptr [eax + 0x28]
        fmul dword ptr [ebx + 0x28]
        faddp st(1), st
        fstp dword ptr [edi + 0x28]
        fld dword ptr [eax + 0x30]
        mov dword ptr [edi + 0x3c], 0x3f800000
        fmul dword ptr [ebx]
        fld dword ptr [eax + 0x34]
        fmul dword ptr [ebx + 0x10]
        faddp st(1), st
        fld dword ptr [eax + 0x38]
        fmul dword ptr [ebx + 0x20]
        faddp st(1), st
        fstp dword ptr [edi + 0x30]
        fld dword ptr [eax + 0x30]
        fmul dword ptr [ebx + 0x4]
        fld dword ptr [eax + 0x34]
        fmul dword ptr [ebx + 0x14]
        faddp st(1), st
        fld dword ptr [eax + 0x38]
        fmul dword ptr [ebx + 0x24]
        faddp st(1), st
        fstp dword ptr [edi + 0x34]
        fld dword ptr [eax + 0x30]
        fmul dword ptr [ebx + 0x8]
        fld dword ptr [eax + 0x34]
        fmul dword ptr [ebx + 0x18]
        faddp st(1), st
        fld dword ptr [eax + 0x38]
        fmul dword ptr [ebx + 0x28]
        faddp st(1), st
        fstp dword ptr [edi + 0x38]
    }
    return r;
}

// Sets the matrix to the inverse of mat, an affine matrix; a singular one gives the identity
// FUNCTION: LS3DF 0x1002d0b0
bool S_matrix::Inverse(const S_matrix &mat)
{
    m[0][0] = mat.m[1][1] * mat.m[2][2] - mat.m[2][1] * mat.m[1][2];
    m[1][0] = mat.m[2][2] * mat.m[1][0] - mat.m[2][0] * mat.m[1][2];
    m[2][0] = mat.m[2][1] * mat.m[1][0] - mat.m[1][1] * mat.m[2][0];
    float det = mat.m[0][0] * m[0][0] - m[1][0] * mat.m[0][1] + m[2][0] * mat.m[0][2];
    if (fabs(det) < MRG_ZERO) {
        Identity();
        return false;
    }
    det = 1.0f / det;
    m[0][0] = det * m[0][0];
    m[0][1] = -((mat.m[2][2] * mat.m[0][1] - mat.m[2][1] * mat.m[0][2]) * det);
    m[0][2] = (mat.m[1][2] * mat.m[0][1] - mat.m[0][2] * mat.m[1][1]) * det;
    m[0][3] = 0.0f;
    m[1][0] = -(det * m[1][0]);
    m[1][1] = (mat.m[0][0] * mat.m[2][2] - mat.m[0][2] * mat.m[2][0]) * det;
    m[1][2] = -((mat.m[0][0] * mat.m[1][2] - mat.m[0][2] * mat.m[1][0]) * det);
    m[1][3] = 0.0f;
    m[2][0] = det * m[2][0];
    m[2][1] = -((mat.m[2][1] * mat.m[0][0] - mat.m[2][0] * mat.m[0][1]) * det);
    m[2][2] = (mat.m[1][1] * mat.m[0][0] - mat.m[1][0] * mat.m[0][1]) * det;
    m[2][3] = 0.0f;
    m[3][0] = -(mat.m[3][0] * m[0][0] + mat.m[3][1] * m[1][0] + mat.m[3][2] * m[2][0]);
    m[3][1] = -(mat.m[3][0] * m[0][1] + mat.m[3][1] * m[1][1] + mat.m[3][2] * m[2][1]);
    m[3][2] = -(mat.m[3][0] * m[0][2] + mat.m[3][1] * m[1][2] + mat.m[3][2] * m[2][2]);
    m[3][3] = 1.0f;
    return true;
}

// this = a * b for affine matrices (Insanity3D's name, next to Make4X4). The PC inlines it into operator*= and
// operator*, so /OPT:REF drops it; the Xbox build keeps it out of line (0x20f590), as its compiler doesn't inline
// assembly.
S_matrix &S_matrix::Make(const S_matrix &a, const S_matrix &b)
{
    __asm {
        mov eax, a
        mov ebx, b
        mov edi, this
        fld dword ptr [eax]
        mov dword ptr [edi + 0xc], 0
        fmul dword ptr [ebx]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [ebx + 0x10]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [ebx + 0x20]
        faddp st(1), st
        fstp dword ptr [edi]
        fld dword ptr [eax]
        fmul dword ptr [ebx + 0x4]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [ebx + 0x14]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [ebx + 0x24]
        faddp st(1), st
        fstp dword ptr [edi + 0x4]
        fld dword ptr [eax]
        fmul dword ptr [ebx + 0x8]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [ebx + 0x18]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [ebx + 0x28]
        faddp st(1), st
        fstp dword ptr [edi + 0x8]
        fld dword ptr [eax + 0x10]
        mov dword ptr [edi + 0x1c], 0
        fmul dword ptr [ebx]
        fld dword ptr [eax + 0x14]
        fmul dword ptr [ebx + 0x10]
        faddp st(1), st
        fld dword ptr [eax + 0x18]
        fmul dword ptr [ebx + 0x20]
        faddp st(1), st
        fstp dword ptr [edi + 0x10]
        fld dword ptr [eax + 0x10]
        fmul dword ptr [ebx + 0x4]
        fld dword ptr [eax + 0x14]
        fmul dword ptr [ebx + 0x14]
        faddp st(1), st
        fld dword ptr [eax + 0x18]
        fmul dword ptr [ebx + 0x24]
        faddp st(1), st
        fstp dword ptr [edi + 0x14]
        fld dword ptr [eax + 0x10]
        fmul dword ptr [ebx + 0x8]
        fld dword ptr [eax + 0x14]
        fmul dword ptr [ebx + 0x18]
        faddp st(1), st
        fld dword ptr [eax + 0x18]
        fmul dword ptr [ebx + 0x28]
        faddp st(1), st
        fstp dword ptr [edi + 0x18]
        fld dword ptr [eax + 0x20]
        mov dword ptr [edi + 0x2c], 0
        fmul dword ptr [ebx]
        fld dword ptr [eax + 0x24]
        fmul dword ptr [ebx + 0x10]
        faddp st(1), st
        fld dword ptr [eax + 0x28]
        fmul dword ptr [ebx + 0x20]
        faddp st(1), st
        fstp dword ptr [edi + 0x20]
        fld dword ptr [eax + 0x20]
        fmul dword ptr [ebx + 0x4]
        fld dword ptr [eax + 0x24]
        fmul dword ptr [ebx + 0x14]
        faddp st(1), st
        fld dword ptr [eax + 0x28]
        fmul dword ptr [ebx + 0x24]
        faddp st(1), st
        fstp dword ptr [edi + 0x24]
        fld dword ptr [eax + 0x20]
        fmul dword ptr [ebx + 0x8]
        fld dword ptr [eax + 0x24]
        fmul dword ptr [ebx + 0x18]
        faddp st(1), st
        fld dword ptr [eax + 0x28]
        fmul dword ptr [ebx + 0x28]
        faddp st(1), st
        fstp dword ptr [edi + 0x28]
        fld dword ptr [eax + 0x30]
        mov dword ptr [edi + 0x3c], 0x3f800000
        fmul dword ptr [ebx]
        fadd dword ptr [ebx + 0x30]
        fld dword ptr [eax + 0x34]
        fmul dword ptr [ebx + 0x10]
        faddp st(1), st
        fld dword ptr [eax + 0x38]
        fmul dword ptr [ebx + 0x20]
        faddp st(1), st
        fstp dword ptr [edi + 0x30]
        fld dword ptr [eax + 0x30]
        fmul dword ptr [ebx + 0x4]
        fadd dword ptr [ebx + 0x34]
        fld dword ptr [eax + 0x34]
        fmul dword ptr [ebx + 0x14]
        faddp st(1), st
        fld dword ptr [eax + 0x38]
        fmul dword ptr [ebx + 0x24]
        faddp st(1), st
        fstp dword ptr [edi + 0x34]
        fld dword ptr [eax + 0x30]
        fmul dword ptr [ebx + 0x8]
        fadd dword ptr [ebx + 0x38]
        fld dword ptr [eax + 0x34]
        fmul dword ptr [ebx + 0x18]
        faddp st(1), st
        fld dword ptr [eax + 0x38]
        fmul dword ptr [ebx + 0x28]
        faddp st(1), st
        fstp dword ptr [edi + 0x38]
    }
    return *this;
}

// FUNCTION: LS3DF 0x1002d220
S_matrix &S_matrix::operator*=(const S_matrix &mat)
{
    S_matrix r;
    r.Make(*this, mat);
    *this = r;
    return *this;
}

// FUNCTION: LS3DF 0x1002d3a0
S_matrix S_matrix::operator*(const S_matrix &mat) const
{
    S_matrix r;
    r.Make(*this, mat);
    return r;
}

// FUNCTION: LS3DF 0x1002d520
S_matrix S_matrix::Mult4X4(const S_matrix &mat) const
{
    S_matrix r;
    r.Make4X4(*this, mat);
    return r;
}

// The full 4x4 product a * b
// FUNCTION: LS3DF 0x1002d780
S_matrix &S_matrix::Make4X4(const S_matrix &a, const S_matrix &b)
{
    __asm {
        mov esi, a
        mov edi, b
        mov eax, this
        fld dword ptr [esi]
        fmul dword ptr [edi]
        fld dword ptr [esi + 0x4]
        fmul dword ptr [edi + 0x10]
        fld dword ptr [esi + 0x8]
        fmul dword ptr [edi + 0x20]
        fld dword ptr [esi + 0xc]
        fmul dword ptr [edi + 0x30]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax]
        fld dword ptr [esi]
        fmul dword ptr [edi + 0x4]
        fld dword ptr [esi + 0x4]
        fmul dword ptr [edi + 0x14]
        fld dword ptr [esi + 0x8]
        fmul dword ptr [edi + 0x24]
        fld dword ptr [esi + 0xc]
        fmul dword ptr [edi + 0x34]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0x4]
        fld dword ptr [esi]
        fmul dword ptr [edi + 0x8]
        fld dword ptr [esi + 0x4]
        fmul dword ptr [edi + 0x18]
        fld dword ptr [esi + 0x8]
        fmul dword ptr [edi + 0x28]
        fld dword ptr [esi + 0xc]
        fmul dword ptr [edi + 0x38]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0x8]
        fld dword ptr [esi]
        fmul dword ptr [edi + 0xc]
        fld dword ptr [esi + 0x4]
        fmul dword ptr [edi + 0x1c]
        fld dword ptr [esi + 0x8]
        fmul dword ptr [edi + 0x2c]
        fld dword ptr [esi + 0xc]
        fmul dword ptr [edi + 0x3c]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0xc]
        fld dword ptr [esi + 0x10]
        fmul dword ptr [edi]
        fld dword ptr [esi + 0x14]
        fmul dword ptr [edi + 0x10]
        fld dword ptr [esi + 0x18]
        fmul dword ptr [edi + 0x20]
        fld dword ptr [esi + 0x1c]
        fmul dword ptr [edi + 0x30]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0x10]
        fld dword ptr [esi + 0x10]
        fmul dword ptr [edi + 0x4]
        fld dword ptr [esi + 0x14]
        fmul dword ptr [edi + 0x14]
        fld dword ptr [esi + 0x18]
        fmul dword ptr [edi + 0x24]
        fld dword ptr [esi + 0x1c]
        fmul dword ptr [edi + 0x34]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0x14]
        fld dword ptr [esi + 0x10]
        fmul dword ptr [edi + 0x8]
        fld dword ptr [esi + 0x14]
        fmul dword ptr [edi + 0x18]
        fld dword ptr [esi + 0x18]
        fmul dword ptr [edi + 0x28]
        fld dword ptr [esi + 0x1c]
        fmul dword ptr [edi + 0x38]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0x18]
        fld dword ptr [esi + 0x10]
        fmul dword ptr [edi + 0xc]
        fld dword ptr [esi + 0x14]
        fmul dword ptr [edi + 0x1c]
        fld dword ptr [esi + 0x18]
        fmul dword ptr [edi + 0x2c]
        fld dword ptr [esi + 0x1c]
        fmul dword ptr [edi + 0x3c]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0x1c]
        fld dword ptr [esi + 0x20]
        fmul dword ptr [edi]
        fld dword ptr [esi + 0x24]
        fmul dword ptr [edi + 0x10]
        fld dword ptr [esi + 0x28]
        fmul dword ptr [edi + 0x20]
        fld dword ptr [esi + 0x2c]
        fmul dword ptr [edi + 0x30]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0x20]
        fld dword ptr [esi + 0x20]
        fmul dword ptr [edi + 0x4]
        fld dword ptr [esi + 0x24]
        fmul dword ptr [edi + 0x14]
        fld dword ptr [esi + 0x28]
        fmul dword ptr [edi + 0x24]
        fld dword ptr [esi + 0x2c]
        fmul dword ptr [edi + 0x34]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0x24]
        fld dword ptr [esi + 0x20]
        fmul dword ptr [edi + 0x8]
        fld dword ptr [esi + 0x24]
        fmul dword ptr [edi + 0x18]
        fld dword ptr [esi + 0x28]
        fmul dword ptr [edi + 0x28]
        fld dword ptr [esi + 0x2c]
        fmul dword ptr [edi + 0x38]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0x28]
        fld dword ptr [esi + 0x20]
        fmul dword ptr [edi + 0xc]
        fld dword ptr [esi + 0x24]
        fmul dword ptr [edi + 0x1c]
        fld dword ptr [esi + 0x28]
        fmul dword ptr [edi + 0x2c]
        fld dword ptr [esi + 0x2c]
        fmul dword ptr [edi + 0x3c]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0x2c]
        fld dword ptr [esi + 0x30]
        fmul dword ptr [edi]
        fld dword ptr [esi + 0x34]
        fmul dword ptr [edi + 0x10]
        fld dword ptr [esi + 0x38]
        fmul dword ptr [edi + 0x20]
        fld dword ptr [esi + 0x3c]
        fmul dword ptr [edi + 0x30]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0x30]
        fld dword ptr [esi + 0x30]
        fmul dword ptr [edi + 0x4]
        fld dword ptr [esi + 0x34]
        fmul dword ptr [edi + 0x14]
        fld dword ptr [esi + 0x38]
        fmul dword ptr [edi + 0x24]
        fld dword ptr [esi + 0x3c]
        fmul dword ptr [edi + 0x34]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0x34]
        fld dword ptr [esi + 0x30]
        fmul dword ptr [edi + 0x8]
        fld dword ptr [esi + 0x34]
        fmul dword ptr [edi + 0x18]
        fld dword ptr [esi + 0x38]
        fmul dword ptr [edi + 0x28]
        fld dword ptr [esi + 0x3c]
        fmul dword ptr [edi + 0x38]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0x38]
        fld dword ptr [esi + 0x30]
        fmul dword ptr [edi + 0xc]
        fld dword ptr [esi + 0x34]
        fmul dword ptr [edi + 0x1c]
        fld dword ptr [esi + 0x38]
        fmul dword ptr [edi + 0x2c]
        fld dword ptr [esi + 0x3c]
        fmul dword ptr [edi + 0x3c]
        faddp st(1), st
        fxch st(2)
        faddp st(1), st
        faddp st(1), st
        fstp dword ptr [eax + 0x3c]
    }
    return *this;
}

// A rotation that turns the z axis into dir
// FUNCTION: LS3DF 0x1002d9d0
void S_matrix::SetDir(const S_vector &dir)
{
    Identity();
    (*this)(2).SetNormalized(dir);
    float sq = dir.z * dir.z + dir.x * dir.x;
    if (sq > MRG_ZERO) {
        if (fabs(sq - 1.0f) < MRG_ZERO) {
            m[0][0] = dir.z;
            m[0][2] = -dir.x;
        } else {
            float f = 1.0f / (float)sqrt(sq);
            m[0][0] = dir.z * f;
            m[0][2] = -(dir.x * f);
        }
    }
    if (fabs(dir.z) + fabs(dir.x) > MRG_ZERO)
        (*this)(0).SetNormalized(S_vector(dir.z, 0.0f, -dir.x));
    (*this)(1) = -(*this)(0).Cross(dir);
}

// The rotation part only: the z axis along dir, the y axis towards up
// FUNCTION: LS3DF 0x1002dcf0
void S_matrix::SetDir3(const S_vector &dir, const S_vector &up)
{
    (*this)(2).SetNormalized(dir);
    S_vector u = up - (*this)(2) * up.Dot((*this)(2));
    if (u.Dot(u) < MRG_ZERO) {
        u = S_vector(0.0f, 0.0f, 1.0f) - (*this)(2) * (*this)(2).z;
        if (u.Dot(u) < MRG_ZERO) {
            u = S_vector(0.0f, 1.0f, 0.0f) - (*this)(2) * (*this)(2).y;
            if (u.Dot(u) < MRG_ZERO)
                u = S_vector(0.0f, 1.0f, 0.0f);
        }
    }
    (*this)(1).SetNormalized(u);
    (*this)(0).SetNormalized(u.Cross(dir));
}

// FUNCTION: LS3DF 0x1002e030
void S_matrix::SetDir(const S_vector &dir, const S_vector &up)
{
    SetDir3(dir, up);
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

// FUNCTION: LS3DF 0x1002e070
void S_matrix::SetRot3(const S_quat &q)
{
    if (fabs(q.s) >= QUAT_ONE_LIMIT) {
        (*this)(0) = IdentityMatrix(0);
        (*this)(1) = IdentityMatrix(1);
        (*this)(2) = IdentityMatrix(2);
        return;
    }
    float x2 = q.v.x + q.v.x;
    float y2 = q.v.y + q.v.y;
    float z2 = q.v.z + q.v.z;
    float xx = q.v.x * x2;
    float xy = q.v.x * y2;
    float xz = q.v.x * z2;
    float yy = q.v.y * y2;
    float yz = q.v.y * z2;
    float zz = q.v.z * z2;
    float wx = q.s * x2;
    float wy = q.s * y2;
    float wz = q.s * z2;
    m[0][0] = 1.0f - (yy + zz);
    m[0][1] = xy - wz;
    m[0][2] = xz + wy;
    m[1][0] = xy + wz;
    m[1][1] = 1.0f - (xx + zz);
    m[1][2] = yz - wx;
    m[2][0] = xz - wy;
    m[2][1] = yz + wx;
    m[2][2] = 1.0f - (xx + yy);
}

// FUNCTION: LS3DF 0x1002e1c0
void S_matrix::SetRot(const S_quat &q)
{
    SetRot3(q);
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

// The rotation of mat without its scale
// FUNCTION: LS3DF 0x1002e1f0
void S_matrix::SetRot(const S_matrix &mat)
{
    (*this)(0) = mat(0);
    (*this)(1) = mat(1);
    (*this)(2) = mat(2);
    float sq = m[0][0] * m[0][0] + m[1][0] * m[1][0] + m[2][0] * m[2][0];
    if (fabs(sq - 1.0f) > MRG_ZERO) {
        float f = 1.0f / (float)sqrt(sq);
        m[0][0] *= f;
        m[1][0] *= f;
        m[2][0] *= f;
    }
    sq = m[0][1] * m[0][1] + m[1][1] * m[1][1] + m[2][1] * m[2][1];
    if (fabs(sq - 1.0f) > MRG_ZERO) {
        float f = 1.0f / (float)sqrt(sq);
        m[0][1] *= f;
        m[1][1] *= f;
        m[2][1] *= f;
    }
    sq = m[0][2] * m[0][2] + m[1][2] * m[1][2] + m[2][2] * m[2][2];
    if (fabs(sq - 1.0f) > MRG_ZERO) {
        float f = 1.0f / (float)sqrt(sq);
        m[0][2] *= f;
        m[1][2] *= f;
        m[2][2] *= f;
    }
    m[0][3] = 0.0f;
    m[1][3] = 0.0f;
    m[2][3] = 0.0f;
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

// FUNCTION: LS3DF 0x1002e3a0
S_vector S_matrix::GetScale() const
{
    return S_vector((*this)(0).Magnitude(), (*this)(1).Magnitude(), (*this)(2).Magnitude());
}

// FUNCTION: LS3DF 0x1002e510
S_vector S_matrix::GetScale2() const
{
    return S_vector((*this)(0).Square(), (*this)(1).Square(), (*this)(2).Square());
}

// The mean scale
// FUNCTION: LS3DF 0x1002e580
float S_matrix::GetUScale() const
{
    return ((*this)(0).Magnitude() + (*this)(1).Magnitude() + (*this)(2).Magnitude()) * (1.0f / 3.0f);
}

// The cosine of half the angle
// FUNCTION: LS3DF 0x1002e710
float S_vector::CosAngleTo(const S_vector &v) const
{
    float c = Dot(v) / sqrt(Square() * v.Square());
    return (float)sqrt((c + 1.0f) * 0.5f);
}

// FUNCTION: LS3DF 0x1002e7a0
float S_vector::AngleTo(const S_vector &v) const
{
    float f = Magnitude() * v.Magnitude();
    if (f < MRG_ZERO)
        return 0.0f;
    float c = Dot(v) / f;
    return (float)acos(Min(Max(c, -1.0f), 1.0f));
}

// FUNCTION: LS3DF 0x1002e920
S_vector &S_vector::operator*=(const S_matrix &m)
{
    float fx = x * m.m[0][0] + y * m.m[1][0] + z * m.m[2][0] + m.m[3][0];
    float fy = x * m.m[0][1] + y * m.m[1][1] + z * m.m[2][1] + m.m[3][1];
    float fz = x * m.m[0][2] + y * m.m[1][2] + z * m.m[2][2] + m.m[3][2];
    // The original stores z first
    z = fz;
    x = fx;
    y = fy;
    return *this;
}

// FUNCTION: LS3DF 0x1002e980
S_vector S_vector::operator*(const S_matrix &m) const
{
    S_vector r;
    __asm {
        mov eax, this
        mov edx, m
        lea esi, r
        fld dword ptr [eax]
        fmul dword ptr [edx]
        fadd dword ptr [edx + 0x30]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [edx + 0x10]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [edx + 0x20]
        faddp st(1), st
        fstp dword ptr [esi]
        fld dword ptr [eax]
        fmul dword ptr [edx + 0x4]
        fadd dword ptr [edx + 0x34]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [edx + 0x14]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [edx + 0x24]
        faddp st(1), st
        fstp dword ptr [esi + 0x4]
        fld dword ptr [eax]
        fmul dword ptr [edx + 0x8]
        fadd dword ptr [edx + 0x38]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [edx + 0x18]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [edx + 0x28]
        faddp st(1), st
        fstp dword ptr [esi + 0x8]
    }
    return r;
}

// FUNCTION: LS3DF 0x1002ea10
S_vector S_vector::RotateByMatrix(const S_matrix &m) const
{
    S_vector r;
    __asm {
        mov eax, this
        mov edx, m
        lea esi, r
        fld dword ptr [eax]
        fmul dword ptr [edx]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [edx + 0x10]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [edx + 0x20]
        faddp st(1), st
        fstp dword ptr [esi]
        fld dword ptr [eax]
        fmul dword ptr [edx + 0x4]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [edx + 0x14]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [edx + 0x24]
        faddp st(1), st
        fstp dword ptr [esi + 0x4]
        fld dword ptr [eax]
        fmul dword ptr [edx + 0x8]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [edx + 0x18]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [edx + 0x28]
        faddp st(1), st
        fstp dword ptr [esi + 0x8]
    }
    return r;
}

// RotateByMatrix with each axis of m normalized
// FUNCTION: LS3DF 0x1002eab0
S_vector S_vector::RotateByNormMatrix(const S_matrix &m) const
{
    S_vector r;
    __asm {
        mov eax, this
        mov edx, m
        lea esi, r
        fld dword ptr [eax]
        fmul dword ptr [edx]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [edx + 0x10]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [edx + 0x20]
        faddp st(1), st
        fstp dword ptr [esi]
        fld dword ptr [eax]
        fmul dword ptr [edx + 0x4]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [edx + 0x14]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [edx + 0x24]
        faddp st(1), st
        fstp dword ptr [esi + 0x4]
        fld dword ptr [eax]
        fmul dword ptr [edx + 0x8]
        fld dword ptr [eax + 0x4]
        fmul dword ptr [edx + 0x18]
        faddp st(1), st
        fld dword ptr [eax + 0x8]
        fmul dword ptr [edx + 0x28]
        faddp st(1), st
        fstp dword ptr [esi + 0x8]
    }
    float sq = m.m[0][0] * m.m[0][0] + m.m[1][0] * m.m[1][0] + m.m[2][0] * m.m[2][0];
    if (fabs(sq - 1.0f) > MRG_ZERO)
        r.x /= (float)sqrt(sq);
    sq = m.m[0][1] * m.m[0][1] + m.m[1][1] * m.m[1][1] + m.m[2][1] * m.m[2][1];
    if (fabs(sq - 1.0f) > MRG_ZERO)
        r.y /= (float)sqrt(sq);
    sq = m.m[0][2] * m.m[0][2] + m.m[1][2] * m.m[1][2] + m.m[2][2] * m.m[2][2];
    if (fabs(sq - 1.0f) > MRG_ZERO)
        r.z /= (float)sqrt(sq);
    return r;
}

// FUNCTION: LS3DF 0x1002ec00
S_matrix S_quat::RotationMatrix() const
{
    S_matrix m;
    if (fabs(s) >= 1.0) {
        m.Identity();
    } else {
        m.m[0][3] = 0.0f;
        m.m[1][3] = 0.0f;
        m.m[2][3] = 0.0f;
        m.m[3][0] = 0.0f;
        m.m[3][1] = 0.0f;
        m.m[3][2] = 0.0f;
        m.m[3][3] = 1.0f;
        float x2 = v.x + v.x;
        float y2 = v.y + v.y;
        float z2 = v.z + v.z;
        float xx = v.x * x2;
        float xy = v.x * y2;
        float xz = v.x * z2;
        float yy = v.y * y2;
        float yz = v.y * z2;
        float zz = v.z * z2;
        float wx = s * x2;
        float wy = s * y2;
        float wz = s * z2;
        m.m[0][0] = 1.0f - (yy + zz);
        m.m[0][1] = xy - wz;
        m.m[0][2] = xz + wy;
        m.m[1][0] = xy + wz;
        m.m[1][1] = 1.0f - (xx + zz);
        m.m[1][2] = yz - wx;
        m.m[2][0] = xz - wy;
        m.m[2][1] = yz + wx;
        m.m[2][2] = 1.0f - (xx + yy);
    }
    return m;
}

// The rotated z axis, the third row of RotationMatrix
// FUNCTION: LS3DF 0x1002ed60
S_vector S_quat::GetDir() const
{
    if (fabs(s) >= 1.0)
        return S_vector(0.0f, 0.0f, 1.0f);
    float x2 = v.x + v.x;
    float y2 = v.y + v.y;
    float z2 = v.z + v.z;
    float xx = v.x * x2;
    float xz = v.x * z2;
    float yy = v.y * y2;
    float yz = v.y * z2;
    float wx = s * x2;
    float wy = s * y2;
    return S_vector(xz - wy, yz + wx, 1.0f - (xx + yy));
}

// FUNCTION: LS3DF 0x1002ee00
void S_quat::Make(const S_vector &axis, float angle)
{
    s = (float)cos(angle * 0.5f);
    v = axis * (float)sin(angle * 0.5f);
    float sq = v.x * v.x + v.y * v.y + v.z * v.z + s * s;
    if (fabs(sq - 1.0f) < MRG_ZERO)
        return;
    if (sq < MRG_ZERO) {
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 0.0f;
        s = 1.0f;
        return;
    }
    float f = 1.0f / (float)sqrt(sq);
    v.x *= f;
    v.y *= f;
    v.z *= f;
    s *= f;
}

// FUNCTION: LS3DF 0x1002eef0
void S_quat::Make(const S_matrix &m)
{
    S_vector scale = m.GetScale();
    if (scale.x + scale.y + scale.z < MRG_ZERO) {
        s = 1.0f;
        v.z = 0.0f;
        v.y = 0.0f;
        v.x = 0.0f;
        return;
    }
    if (scale.x * scale.y * scale.z < MRG_ZERO) {
        // An axis is missing: rebuild it from the other two and try again
        S_matrix mm = m;
        if (scale.x < MRG_ZERO) {
            mm(0) = m(1).Cross(m(2));
            scale.x = mm(0).Magnitude();
            if (scale.x < MRG_ZERO) {
                mm(0) = S_vector(1.0f, 0.0f, 0.0f);
                scale.x = 1.0f;
            }
        }
        if (scale.y < MRG_ZERO) {
            mm(1) = m(0).Cross(m(2));
            scale.y = mm(1).Magnitude();
            if (scale.y < MRG_ZERO) {
                mm(1) = S_vector(0.0f, 1.0f, 0.0f);
                scale.y = 1.0f;
            }
        }
        if (scale.z < MRG_ZERO) {
            mm(2) = m(0).Cross(m(1));
            scale.z = mm(2).Magnitude();
            if (scale.z < MRG_ZERO) {
                mm(2) = S_vector(0.0f, 0.0f, 1.0f);
                scale.z = 1.0f;
            }
        }
        // VC6 merges the first exit into this one, so the first test jumps here
        S_vector scale2 = mm.GetScale();
        if (scale2.x * scale2.y * scale2.z < MRG_ZERO) {
            s = 1.0f;
            v.z = 0.0f;
            v.y = 0.0f;
            v.x = 0.0f;
            return;
        }
        S_quat q;
        q.Make(mm);
        v = q.v;
        s = q.s;
        return;
    }
    // Turn the z axis into the y row, then rotate around it to the x row. The rows are read element by element, which
    // addresses them from m (m(1).y would go through the address GetScale already took).
    float ix = 1.0f / scale.x;
    float iy = 1.0f / scale.y;
    S_vector y_axis, diag;
    y_axis = S_vector(m(1, 0) * iy, m(1, 1) * iy, m(1, 2) * iy);
    diag = y_axis;
    diag += S_vector(m(0, 0) * ix, m(0, 1) * ix, m(0, 2) * ix);
    S_quat q1;
    if (fabs(y_axis.y) >= QUAT_ONE_LIMIT) {
        if (y_axis.y > 0.0f)
            q1 = S_quat(1.0f, 0.0f, 0.0f, 0.0f);
        else
            q1 = S_quat(0.0f, 0.0f, 0.0f, 1.0f);
    } else {
        S_vector axis;
        axis = S_vector(y_axis.z, 0.0f, -y_axis.x);
        axis.Normalize();
        q1.Make(-axis, (float)acos(y_axis.y));
    }
    // (1, 1, 0) rotated by q1: the sum of the first two rows of its matrix
    S_vector rot;
    if (fabs(q1.s) >= 1.0)
        rot = S_vector(1.0f, 1.0f, 0.0f);
    else {
        float x2 = q1.v.x + q1.v.x;
        float y2 = q1.v.y + q1.v.y;
        float z2 = q1.v.z + q1.v.z;
        float xx = q1.v.x * x2;
        float xy = q1.v.x * y2;
        float xz = q1.v.x * z2;
        float yy = q1.v.y * y2;
        float yz = q1.v.y * z2;
        float zz = q1.v.z * z2;
        float wx = q1.s * x2;
        float wy = q1.s * y2;
        float wz = q1.s * z2;
        rot = S_vector(1.0f - (yy + zz) + xy + wz, xy - wz + 1.0f - (xx + zz), xz + wy + yz - wx);
    }
    S_vector d1 = rot - y_axis;
    S_vector d2 = diag - y_axis;
    float d = d1.Dot(d2);
    S_quat q2;
    if (fabs(d) >= QUAT_ONE_LIMIT) {
        if (y_axis.Cross(d1).Dot(diag) > 0.0f)
            q2.Make(-y_axis, d > 0.0f ? 0.0f : PI);
        else
            q2.Make(y_axis, d > 0.0f ? 0.0f : PI);
    } else {
        S_vector axis;
        axis = d1.Cross(d2);
        axis.Normalize();
        q2.Make(-axis, (float)acos(d));
    }
    *this = q1 * q2;
}

// FUNCTION: LS3DF 0x1002fa70
S_quat S_quat::operator*(const S_quat &q) const
{
    return S_quat(s * q.s - v.Dot(q.v), q.v * s + v * q.s + v.Cross(q.v));
}

// A rotation that turns the z axis into dir, then rolls around it
// FUNCTION: LS3DF 0x1002fb50
void S_quat::SetDir(const S_vector &dir, float roll)
{
    S_matrix m;
    m.SetDir(dir);
    if (fabs(roll) > MRG_ZERO)
        m = RotateZMatrix(roll) * m;
    Make(m);
}

// FUNCTION: LS3DF 0x1002fd70
void S_quat::Inverse(S_vector &axis, float &angle) const
{
    axis = v;
    axis.Normalize();
    if (s >= 1.0f)
        angle = 0.0f;
    else if (s <= -1.0f)
        angle = PI * 2.0f;
    else
        angle = (float)acos(s) * 2.0f;
}

// FUNCTION: LS3DF 0x1002fee0
S_quat S_quat::Slerp(const S_quat &q, float t, bool shortest) const
{
    float to1[4];
    float cosom = v.x * q.v.x + v.y * q.v.y + v.z * q.v.z + s * q.s;
    if (cosom < 0.0f) {
        cosom = -cosom;
        to1[0] = -q.v.x;
        to1[1] = -q.v.y;
        to1[2] = -q.v.z;
        to1[3] = -q.s;
    } else {
        to1[0] = q.v.x;
        to1[1] = q.v.y;
        to1[2] = q.v.z;
        to1[3] = q.s;
    }
    float scale0, scale1;
    if ((1.0f - cosom) > SLERP_DELTA) {
        float omega = (float)acos(cosom);
        float sinom = 1.0f / (float)sin(omega);
        scale0 = (float)sin((1.0f - t) * omega) * sinom;
        scale1 = (float)sin(t * omega) * sinom;
    } else {
        scale0 = 1.0f - t;
        scale1 = t;
    }
    S_quat r;
    r.v.x = scale0 * v.x + scale1 * to1[0];
    r.v.y = scale0 * v.y + scale1 * to1[1];
    r.v.z = scale0 * v.z + scale1 * to1[2];
    r.s = scale0 * s + scale1 * to1[3];
    return r;
}

// FUNCTION: LS3DF 0x10030040
S_quat S_quat::RotateByMatrix(const S_matrix &m) const
{
    S_quat q;
    q.Make(m);
    return q * *this;
}

// FUNCTION: LS3DF 0x10030140
void S_quat::Normalize()
{
    float sq = s * s + v.x * v.x + v.y * v.y + v.z * v.z;
    if (sq > MRG_ZERO) {
        if (fabs(sq - 1.0f) < MRG_ZERO)
            return;
        float f = 1.0f / (float)sqrt(sq);
        v.x *= f;
        v.y *= f;
        v.z *= f;
        s *= f;
        return;
    }
    if (v.x != 0.0f)
        v.x = v.x < 0.0f ? -1.0f : 1.0f;
    else if (v.z != 0.0f)
        v.z = v.z < 0.0f ? -1.0f : 1.0f;
    else
        v.y = v.y < 0.0f ? -1.0f : 1.0f;
    s = 0.0f;
}

// The line where the two planes meet: a point on it and its direction. Graphics Gems' method, which Insanity3D has
// with these names and the squares of the direction in a vector: three float locals compile differently.
// FUNCTION: LS3DF 0x10030290
bool S_plane::IntersectionLine(const S_plane &pl, S_vector &xpt, S_vector &xdir) const
{
    xdir = normal.Cross(pl.normal);
    S_vector dir2(xdir.x * xdir.x, xdir.y * xdir.y, xdir.z * xdir.z);
    float invdet;
    if (dir2.z > dir2.y && dir2.z > dir2.x && dir2.z > MRG_ZERO) {
        // a point on the xy plane
        invdet = 1.0f / xdir.z;
        xpt.x = normal.y * pl.d - pl.normal.y * d;
        xpt.y = pl.normal.x * d - normal.x * pl.d;
        xpt.z = 0.0f;
    } else if (dir2.y > dir2.x && dir2.y > MRG_ZERO) {
        // on the xz plane
        invdet = 1.0f / xdir.y;
        xpt.x = pl.normal.z * d - normal.z * pl.d;
        xpt.y = 0.0f;
        xpt.z = normal.x * pl.d - pl.normal.x * d;
    } else if (dir2.x > MRG_ZERO) {
        // on the yz plane
        invdet = 1.0f / xdir.x;
        xpt.x = 0.0f;
        xpt.y = normal.z * pl.d - pl.normal.z * d;
        xpt.z = pl.normal.y * d - normal.y * pl.d;
    } else
        return false;
    xpt.x *= invdet;
    xpt.y *= invdet;
    xpt.z *= invdet;
    float f = 1.0f / (float)sqrt(dir2.x + dir2.y + dir2.z);
    xdir.x *= f;
    xdir.y *= f;
    xdir.z *= f;
    return true;
}

// The closest points of two lines (p1 + t1 * dir1, p2 + t2 * dir2)
// FUNCTION: LS3DF 0x10030440
bool __stdcall LineLineIntersect(const S_vector &p1, const S_vector &dir1, const S_vector &p2, const S_vector &dir2,
    float &t1, float &t2)
{
    S_vector w = p1 - p2;
    float dd = dir2.Square();
    float bd = dir1.Dot(dir2);
    float det = dir1.Square() * dd - bd * bd;
    if (IsAbsMrgZero(det))
        return false;
    float wd = w.Dot(dir2);
    t1 = (bd * wd - w.Dot(dir1) * dd) / det;
    t2 = (t1 * bd + wd) / dd;
    return true;
}
