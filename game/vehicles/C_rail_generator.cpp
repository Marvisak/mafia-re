// The rail vehicles' generator (PS2 C_rail_generator.cpp; the PC's file is the one at 0x596420-0x5983dc,
// docs/game-files.md), whose constructor (0x596620) comes after functions of its own. Written: the constructor, which
// C_mission::CreateActor calls, and Init.

#include "vehicles/C_rail_generator.h"
#include "C_chunk.h"

#include "stub.h"

// GLOBAL: GAME 0x636ed4
int C_rail_generator::unknown_636ed4;

#pragma auto_inline(off)

// FUNCTION: GAME 0x596620
C_rail_generator::C_rail_generator()
{
    unknown_7c = 1;
    unknown_70 = 1;
    unknown_74 = 5000;
    type = ACTOR_RAIL_GENERATOR;
    unknown_78 = 130.0f;
    unknown_636ed4 = 10;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x596680
C_rail_generator::~C_rail_generator()
{
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_rail_generator::Update(uint time)
{
}

// FUNCTION: GAME 0x596690
bool C_rail_generator::Init(I3D_frame *frm)
{
    frame = frm;
    return true;
}

// The two values the constructor sets (PS2 name)
// FUNCTION: GAME 0x5966a0
bool C_rail_generator::SaveData(C_chunk *chunk)
{
    uchar zero = 0;
    chunk->Write(&zero, 1);
    chunk->Write(&unknown_70, 4);
    chunk->Write(&unknown_74, 4);
    return true;
}

// Reads the two values and the type byte (PS2 name)
// FUNCTION: GAME 0x596710
bool C_rail_generator::LoadData(C_chunk *chunk)
{
    uchar type;
    chunk->Read(&type, 1);
    chunk->Read(&unknown_70, 4);
    chunk->Read(&unknown_74, 4);
    return true;
}

// STUB: GAME 0x596860
void C_rail_generator::GameInit()
{
    StubLog("C_rail_generator::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x597a20
void C_rail_generator::GameDone()
{
    StubLog("C_rail_generator::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x597bf0
void C_rail_generator::AI(uint time)
{
    StubLog("C_rail_generator::AI");
    C_actor::AI(time);
}

#pragma auto_inline(on)
