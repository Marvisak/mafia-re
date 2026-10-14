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

// 0x5c bytes, created by Globals.cpp's BreBre. Its 41 virtual functions (vtable 0x1009b830), in PS2's order where the
// names are known.
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
    virtual void __stdcall vfunc_16();
    virtual void __stdcall vfunc_17();
    virtual void __stdcall vfunc_18();
    virtual void __stdcall vfunc_19();
    virtual void __stdcall vfunc_20();
    virtual void __stdcall vfunc_21();
    virtual void __stdcall vfunc_22();
    virtual void __stdcall vfunc_23();
    virtual void __stdcall vfunc_24();
    virtual void __stdcall vfunc_25();
    virtual void __stdcall vfunc_26();
    virtual void __stdcall vfunc_27();
    virtual void __stdcall vfunc_28();
    virtual void __stdcall vfunc_29();
    virtual void __stdcall vfunc_30();
    virtual void __stdcall vfunc_31();
    virtual void __stdcall vfunc_32();
    virtual void __stdcall vfunc_33();
    virtual void __stdcall vfunc_34();
    virtual void __stdcall vfunc_35();
    virtual void __stdcall vfunc_36();
    virtual void __stdcall vfunc_37();
    virtual void __stdcall vfunc_38();
    virtual void __stdcall vfunc_39();
    // PS2 name. Empty on PS2 and PC: the original's copy is folded with an empty function of the DirectShow base
    // classes (0x10087ec0). The buffer managers call it whenever they create or release a buffer.
    virtual void __stdcall m_RecomputeUsedVRAM();

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
