#pragma once

#include "ls3df.h"

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

class I3D_driver {
    uchar unknown_00[0x38];

public:
    C_list<I3D_sound *> sounds;             // 0x38
};
