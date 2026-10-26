#pragma once

#include "actors/C_human.h"

// The AI's humans (PS2 C_entity, jp_ai_entity.cpp), whose constructor constructs a C_program of their own, 0x128c
// bytes (PS2's 0x14a0). Only the constructor so far, which C_mission::CreateActor calls for ACTOR_ENTITY.
class C_entity : public C_human
{
public:
    C_entity();

    uchar unknown_a9c[0x7f0];           // 0xa9c
};
