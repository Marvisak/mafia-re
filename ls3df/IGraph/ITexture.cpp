// Textures (PS2 ITexture.cpp). Not written yet: only what other files need to link. ITexture's methods continue past
// the header's zero vector (0x10078170) in PS2's order, so the file probably runs to the image code before the
// sound files (0x1007b053).
#include "IGraph/IGraph.h"

// STUB: LS3DF 0x10075c10
ITexture::ITexture()
{
    unknown_0c[0] = 0;
}

// STUB: LS3DF 0x10075cd0
int ITexture::Release()
{
    return 0x2001;
}

// STUB: LS3DF 0x10075d10
void ITexture::vfunc_01(int a)
{
    unknown_0c[1] = 0;
}

// STUB: LS3DF 0x10075f30
void ITexture::vfunc_05()
{
    unknown_0c[2] = 0;
}

// STUB: LS3DF 0x10075f50
LS3D_RESULT ITexture::Create(const char *name, const char *alpha_name, uint flags, ushort sx, ushort sy,
    ushort unknown)
{
    return (LS3D_RESULT)0x2002;
}

// STUB: LS3DF 0x10078670
void ITexture::vfunc_03()
{
    unknown_0c[3] = 0;
}

// STUB: LS3DF 0x10078680
void ITexture::vfunc_04()
{
    unknown_0c[4] = 0;
}

// STUB: LS3DF 0x100787c0
void ITexture::vfunc_06(int a, int b, int c)
{
    unknown_0c[5] = 0;
}

// STUB: LS3DF 0x10078820
void ITexture::vfunc_07()
{
    unknown_0c[6] = 0;
}

// STUB: LS3DF 0x10078850
void ITexture::vfunc_08()
{
    unknown_0c[7] = 0;
}

// STUB: LS3DF 0x10078860
void ITexture::vfunc_09()
{
    unknown_0c[0] = 1;
}

// STUB: LS3DF 0x10078940
void ITexture::vfunc_10(int a, int b, int c)
{
    unknown_0c[1] = 1;
}

// STUB: LS3DF 0x10078b70
void ITexture::vfunc_12(int a, int b)
{
    unknown_0c[2] = 1;
}

// STUB: LS3DF 0x10078cf0
void ITexture::vfunc_11(int a, int b, int c, int d, int e, int f, int g, int h)
{
    unknown_0c[3] = 1;
}

// STUB: LS3DF 0x10078e40
void ITexture::vfunc_13(int a)
{
    unknown_0c[4] = 1;
}

// Releases the render target textures before a device reset (the name is a guess)
// STUB: LS3DF 0x1007a610
void ReleaseRenderTargets()
{
    static int stub;
    stub = 1;
}

// Recreates them after it (the name is a guess)
// STUB: LS3DF 0x1007a710
void RestoreRenderTargets(uint flags)
{
}

// STUB: LS3DF 0x1007a910
void ITexture::vfunc_14()
{
    unknown_0c[5] = 1;
}

// STUB: LS3DF 0x1007a9b0
DWORD WINAPI RenderThreadProc(void *param)
{
    return 0x2003;
}
