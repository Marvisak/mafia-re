// The rail vehicles' generator (PS2 C_rail_generator.cpp; the PC's file is the one at 0x596420-0x5983dc,
// docs/game-files.md), whose constructor (0x596620) comes after functions of its own. Written: Init; the constructor,
// which C_mission::CreateActor calls, is a stub.

#include "vehicles/C_rail_generator.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x596620
C_rail_generator::C_rail_generator()
{
    StubLog("C_rail_generator::C_rail_generator");
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

// STUB: GAME 0x5966a0
void C_rail_generator::SaveData(C_chunk *chunk)
{
    StubLog("C_rail_generator::SaveData");
    C_actor::SaveData(chunk);
}

// STUB: GAME 0x596710
void C_rail_generator::LoadData(C_chunk *chunk)
{
    StubLog("C_rail_generator::LoadData");
    C_actor::LoadData(chunk);
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
