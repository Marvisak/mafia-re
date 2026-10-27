#pragma once

#include "actors/C_actor.h"

// The traffic's cars (PS2 C_traffic_car, C_trf_c.cpp), 0x26c bytes (PS2's 0x290). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_TRAFFIC_CAR.
class C_traffic_car : public C_actor
{
public:
    C_traffic_car();

    // PS2 name (Czech, roughly "so now I buy them all from you"; PS2's takes a bool)
    void TakATedSiVasKupim_Vsecky();
    // PS2 name (Czech, roughly "he'll beat the car up")
    void VypraskeeeKaaaru(C_actor *act);

    uchar unknown_70[0x1fc];            // 0x70

    // C_game::DoneIntern sets both to 1 (the names are ours)
    static float unknown_63d7fc;
    static float unknown_63d804;
};
