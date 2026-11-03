#pragma once

#include "actors/C_actor.h"

// The rail vehicles' generator (PS2 C_rail_generator, C_rail_generator.cpp), 0x84 bytes (PS2's 0xa0). Written: the
// constructor, which C_mission::CreateActor calls for ACTOR_RAIL_GENERATOR, Init, SaveData and LoadData.
// VTABLE: GAME 0x6259e8
class C_rail_generator : public C_actor
{
public:
    C_rail_generator();
    virtual ~C_rail_generator();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk);
    virtual bool LoadData(C_chunk *chunk);
    virtual void GameInit();
    virtual void GameDone();

    // The constructor's values (the names are ours)
    int unknown_70;                     // 0x70
    int unknown_74;                     // 0x74, 5000
    float unknown_78;                   // 0x78, 130
    int unknown_7c;                     // 0x7c
    // The generator's count of the game's (the name is ours): its constructor sets 10
    static int unknown_636ed4;
};

// SYNTHETIC: GAME 0x596660
// C_rail_generator::`scalar deleting destructor'
