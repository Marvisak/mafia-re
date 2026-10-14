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

// STUB: LS3DF 0x100182f0
void I3D_driver::vfunc_16()
{
    unknown_04[18] = 0x7d;
}

// STUB: LS3DF 0x100186a0
void I3D_driver::vfunc_17()
{
    unknown_04[19] = 0x7d;
}

// STUB: LS3DF 0x10018cc0
void I3D_driver::vfunc_18()
{
    unknown_04[20] = 0x7d;
}

// STUB: LS3DF 0x10018d80
void I3D_driver::vfunc_19()
{
    unknown_04[21] = 0x7d;
}

// STUB: LS3DF 0x10019090
void I3D_driver::vfunc_21()
{
    unknown_04[22] = 0x7d;
}

// STUB: LS3DF 0x10019100
void I3D_driver::vfunc_22()
{
    unknown_04[23] = 0x7d;
}

// STUB: LS3DF 0x10019180
void I3D_driver::vfunc_23()
{
    unknown_04[24] = 0x7d;
}

// STUB: LS3DF 0x100191a0
void I3D_driver::vfunc_24()
{
    unknown_04[25] = 0x7d;
}

// STUB: LS3DF 0x10019200
void I3D_driver::vfunc_25()
{
    unknown_04[26] = 0x7d;
}

// STUB: LS3DF 0x10019260
void I3D_driver::vfunc_26()
{
    unknown_04[27] = 0x7d;
}

// STUB: LS3DF 0x100192c0
void I3D_driver::vfunc_27()
{
    unknown_04[28] = 0x7d;
}

// STUB: LS3DF 0x10019320
void I3D_driver::vfunc_28()
{
    unknown_04[29] = 0x7d;
}

// STUB: LS3DF 0x10019380
void I3D_driver::vfunc_29()
{
    unknown_04[30] = 0x7d;
}

// STUB: LS3DF 0x100193e0
void I3D_driver::vfunc_20()
{
    unknown_04[31] = 0x7d;
}

// STUB: LS3DF 0x10019880
void I3D_driver::vfunc_32()
{
    unknown_04[32] = 0x7d;
}

// STUB: LS3DF 0x10019a10
void I3D_driver::vfunc_30()
{
    unknown_04[33] = 0x7d;
}

// STUB: LS3DF 0x10019a20
void I3D_driver::vfunc_31()
{
    unknown_04[34] = 0x7d;
}

// STUB: LS3DF 0x10019b10
void I3D_driver::vfunc_33()
{
    unknown_04[35] = 0x7d;
}

// STUB: LS3DF 0x10019d30
void I3D_driver::vfunc_34()
{
    unknown_04[36] = 0x7d;
}

// STUB: LS3DF 0x10019fd0
void I3D_driver::vfunc_35()
{
    unknown_04[37] = 0x7d;
}

// STUB: LS3DF 0x1001a0a0
void I3D_driver::vfunc_36()
{
    unknown_04[38] = 0x7d;
}

// STUB: LS3DF 0x1001a0e0
void I3D_driver::vfunc_37()
{
    unknown_04[39] = 0x7d;
}

// STUB: LS3DF 0x1001a0f0
void I3D_driver::vfunc_38()
{
    unknown_04[40] = 0x7d;
}

// STUB: LS3DF 0x10062030
void I3D_driver::vfunc_07(int a)
{
    unknown_04[17] = 0x7d;
}

// STUB: LS3DF 0x100658f0
void I3D_driver::vfunc_39()
{
    unknown_04[41] = 0x7d;
}

// Empty, as on PS2. The original's copy is folded with an empty function of the DirectShow base classes
// (0x10087ec0), so it has no marker.
void I3D_driver::m_RecomputeUsedVRAM()
{
}
