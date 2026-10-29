#pragma once

#include "actors/C_actor.h"

// The airplanes (PS2 C_airplane, C_airplane.cpp), 0x3f0 bytes (PS2's 0x400). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_AIRPLANE, and what the scripts call.
class C_airplane : public C_actor
{
public:
    C_airplane();

    // Takes off (PS2 name): the scripts' AIRPLANE_START gives the speed in m/s
    void Start(uint unknown, float speed);
    // PS2 name, the scripts' AIRPLANE_START2
    void Start2();
    // PS2 name; the scripts' AIRPLANE_ISDESTROYED
    bool IsDestroyed();
    // Shows the damage on the HUD's bar or hides it (PS2 name); the scripts' AIRPLANESHOWDAMAGE
    void ShowDamageIndicator(bool show);

    uchar unknown_70[0x380];            // 0x70
};
