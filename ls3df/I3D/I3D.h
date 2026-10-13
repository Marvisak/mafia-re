#pragma once

#include "ls3df.h"

#include <d3d8.h>

// The 3D engine's classes, as far as the files written so far use them. Their layouts and virtual functions aren't
// recovered yet: placeholders keep the known members at their offsets and the known virtual functions in their
// vtable slots, and will make way for the real declarations.

// 15 virtual functions on PC, at 0x1009cf58 in I3D_sound's vtable
class I3D_frame {
public:
    virtual void __stdcall vfunc_00() = 0;
    virtual void __stdcall vfunc_01() = 0;
    virtual void __stdcall vfunc_02() = 0;
    virtual void __stdcall vfunc_03() = 0;
    virtual void __stdcall vfunc_04() = 0;
    virtual void __stdcall vfunc_05() = 0;
    virtual void __stdcall vfunc_06() = 0;
    virtual void __stdcall vfunc_07() = 0;
    virtual void __stdcall vfunc_08() = 0;
    virtual void __stdcall vfunc_09() = 0;
    virtual void __stdcall vfunc_10() = 0;
    virtual void __stdcall vfunc_11() = 0;
    virtual void __stdcall vfunc_12() = 0;
    virtual void __stdcall vfunc_13() = 0;
    virtual void __stdcall vfunc_14() = 0;
};

class I3D_sound : public I3D_frame {
public:
    virtual void __stdcall vfunc_15() = 0;
    virtual void __stdcall vfunc_16() = 0;
    virtual void __stdcall vfunc_17() = 0;
    virtual void __stdcall vfunc_18() = 0;
    virtual void __stdcall vfunc_19() = 0;
    virtual void __stdcall vfunc_20() = 0;
    // PS2 name, 0x100597f0
    virtual bool __stdcall IsPlaying(bool unknown) const = 0;

    uchar unknown_04[0x13c];
    uint sound_flags;                       // 0x140
};

// The driver's render states (PS2 type; the value names aren't known yet)
enum I3D_RENDERSTATE {
    RS_UNKNOWN_0,
};

// 0x5c bytes, created by Globals.cpp's BreBre. Its first 16 virtual functions (of at least 41), in PS2's order where
// the names are known.
class I3D_driver {
public:
    I3D_driver();
    ~I3D_driver();

    virtual void __stdcall vfunc_00(int a);
    virtual void __stdcall vfunc_01();
    virtual void __stdcall vfunc_02(int a);
    virtual void __stdcall vfunc_03();
    virtual void __stdcall vfunc_04(int a, int b);
    virtual LS3D_RESULT __stdcall SetState(I3D_RENDERSTATE state, int value);
    virtual void __stdcall vfunc_06(int a);
    virtual void __stdcall vfunc_07(int a);
    virtual void __stdcall vfunc_08(int a, int b, int c);
    virtual void __stdcall vfunc_09(int a, int b, int c, int d);
    virtual void __stdcall vfunc_10(int a, int b, int c, int d);
    virtual void __stdcall vfunc_11(int a, int b, int c, int d, int e, int f, int g);
    virtual void __stdcall vfunc_12(int a, int b, int c, int d, int e, int f, int g, int h);
    virtual void __stdcall vfunc_13(int a, int b, int c, int d, int e);
    virtual void __stdcall vfunc_14(int a, int b, int c, int d);
    virtual void __stdcall DrawText2D(int x, int y, const char *text, uchar unknown, float size);

private:
    uchar unknown_04[0x34];

public:
    C_list<I3D_sound *> sounds;             // 0x38

private:
    uchar unknown_48[0x14];
};

// The values the driver set last, by I3D_RENDERSTATE (PS2 name; PS2 has 70)
extern int renderState[];

// Whether the driver is initialised (set by its Init, cleared by its Close; the name is a guess)
extern bool i3d_driver_init;

// The driver's statistics, which its GetStats copies out and IGraph::Init clears (the name is a guess)
extern uint i3d_stats[8];

// A scale the I3D code applies to lighting, 128 or 255 without MODULATE2X (the name is a guess)
extern float i3d_light_scale;

// A part of one of the vertex buffers C_VBManager shares out (PS2 names; the members are guesses)
struct COMPLETE_VB_STRUCT {
    uint unknown_00;
    IDirect3DVertexBuffer8 *vb;
    int base;                           // the part's first vertex
};

// The same for index buffers (PS2 names)
struct COMPLETE_IB_STRUCT {
    uint unknown_00;
    IDirect3DIndexBuffer8 *ib;
    int base;                           // the part's first index
};

// The vertex buffers (PS2 VBManager.cpp). Not written yet: only what other files need.
class C_VBManager {
    uchar unknown_00[0x18];

public:
    bool m_Alloc(uint fvf, uint usage, uint stride, uint count, COMPLETE_VB_STRUCT &vb, int unknown6, bool unknown7);
    void m_Free(COMPLETE_VB_STRUCT &vb);
    void m_FreeAllUnusedVBuffers(void *unknown);
    void m_FreeAllVBuffers();
    void m_FreeAllVRAMVBuffers();
};

// The index buffers (PS2 IBManager.cpp)
class C_IBManager {
    uchar unknown_00[0x18];

public:
    bool m_Alloc(uint usage, uint count, COMPLETE_IB_STRUCT &ib);
    void m_Free(COMPLETE_IB_STRUCT &ib);
    void m_FreeAllUnusedIBuffers();
    void m_FreeAllIBuffers();
    void m_FreeAllVRAMIBuffers();
};

// PS2 names
extern C_VBManager cVBMan;
extern C_IBManager cIBMan;
