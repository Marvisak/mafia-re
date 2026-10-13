// The 3D driver (PS2 I3d_driver.cpp). Not written yet: only what other files need to link. The stubs of the virtual
// functions after slot 6 are in other files of the original.
#include "I3D/I3D.h"

// GLOBAL: LS3DF 0x100bfca0
int renderState[70];

// GLOBAL: LS3DF 0x100c0009
bool i3d_driver_init;

// The z-buffer type of IGraph's DXS_ZENABLE (a D3DZBUFFERTYPE; the name is a guess). It is in .data from an I3D
// file before this one.
// GLOBAL: LS3DF 0x100a69d8
uint i3d_zbuffer_type = 1;

// In .data from another I3D file
// GLOBAL: LS3DF 0x100a9470
float i3d_light_scale = 128.0f;

// GLOBAL: LS3DF 0x100afb84
uint i3d_stats[8];

// STUB: LS3DF 0x10015620
I3D_driver::I3D_driver()
{
    unknown_04[0] = 0x7d;
}

// STUB: LS3DF 0x10015670
I3D_driver::~I3D_driver()
{
    unknown_04[1] = 0x7d;
}

// STUB: LS3DF 0x10015730
void I3D_driver::vfunc_02(int a)
{
    unknown_04[2] = 0x7d;
}

// STUB: LS3DF 0x10015750
void I3D_driver::vfunc_03()
{
    unknown_04[3] = 0x7d;
}

// STUB: LS3DF 0x10015760
void I3D_driver::vfunc_00(int a)
{
    unknown_04[4] = 0x7d;
}

// STUB: LS3DF 0x10015930
void I3D_driver::vfunc_01()
{
    unknown_04[5] = 0x7d;
}

// STUB: LS3DF 0x100159b0
void I3D_driver::vfunc_04(int a, int b)
{
    unknown_04[6] = 0x7d;
}

// STUB: LS3DF 0x10015a20
LS3D_RESULT I3D_driver::SetState(I3D_RENDERSTATE state, int value)
{
    return (LS3D_RESULT)7;
}

// STUB: LS3DF 0x10015e50
void I3D_driver::vfunc_06(int a)
{
    unknown_04[8] = 0x7d;
}

// STUB: LS3DF 0x100166c0
void I3D_driver::vfunc_08(int a, int b, int c)
{
    unknown_04[9] = 0x7d;
}

// STUB: LS3DF 0x100168f0
void I3D_driver::vfunc_09(int a, int b, int c, int d)
{
    unknown_04[10] = 0x7d;
}

// STUB: LS3DF 0x10016b20
void I3D_driver::vfunc_10(int a, int b, int c, int d)
{
    unknown_04[11] = 0x7d;
}

// STUB: LS3DF 0x10016d70
void I3D_driver::vfunc_11(int a, int b, int c, int d, int e, int f, int g)
{
    unknown_04[12] = 0x7d;
}

// STUB: LS3DF 0x10016ff0
void I3D_driver::vfunc_12(int a, int b, int c, int d, int e, int f, int g, int h)
{
    unknown_04[13] = 0x7d;
}

// STUB: LS3DF 0x10017580
void I3D_driver::vfunc_13(int a, int b, int c, int d, int e)
{
    unknown_04[14] = 0x7d;
}

// STUB: LS3DF 0x10017890
void I3D_driver::vfunc_14(int a, int b, int c, int d)
{
    unknown_04[15] = 0x7d;
}

// STUB: LS3DF 0x10017d40
void I3D_driver::DrawText2D(int x, int y, const char *text, uchar unknown, float size)
{
    unknown_04[16] = 0x7d;
}

// STUB: LS3DF 0x10062030
void I3D_driver::vfunc_07(int a)
{
    unknown_04[17] = 0x7d;
}
