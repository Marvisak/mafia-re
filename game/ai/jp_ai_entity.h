#pragma once

#include "actors/C_human.h"

// The AI's humans (PS2 C_entity, jp_ai_entity.cpp), whose constructor constructs a C_program of their own, 0x128c
// bytes (PS2's 0x14a0). Only the constructor so far, which C_mission::CreateActor calls for ACTOR_ENTITY, and what
// C_game::EntityBlock calls.
class C_entity : public C_human
{
public:
    C_entity();
    // PS2 names, Czech for "sit on your backside and watch the movie" and "wake up, the credits are on": the entity
    // blocked for a cutscene and woken after it (C_game::EntityBlock)
    void SedniNaPrdelASledujKino(bool unknown);
    void VzbudSeUzSouTitulky();

    uchar unknown_a9c[0x7f0];           // 0xa9c
};
