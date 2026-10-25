#pragma once

#include "Main.h"

// The force feedback (PS2 C_ff_manager, C_ff_manager.cpp; the PC's drives the joysticks' effects), 0xbc bytes. The
// cars call most of its methods. Only what InitSystem and CloseSystem use so far.
class C_ff_manager
{
public:
    uchar unknown_0[0x20];
    int unknown_20;                     // 0x20
    uchar unknown_24[0x98];

    // The name is ours. InitSystem calls it once the joysticks are there.
    void Reset();
    // The name is ours. CloseSystem calls it before it closes the joysticks; it does nothing.
    void Close();
};

extern C_ff_manager ff_manager;
