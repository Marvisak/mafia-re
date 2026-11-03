// The rails' turnouts (PS2 C_turnout.cpp; the PC's file is the one at 0x4bc360-0x4bd64c, docs/game-files.md), which
// starts with the constructor. Written: SaveGameGetSize, SaveGameSave and DuplicateFrom; the constructor, which
// C_mission::CreateActor calls, is a stub.

#include "vehicles/C_turnout.h"
#include "C_chunk.h"

#include <string.h>

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x4bc360
C_turnout::C_turnout()
{
    StubLog("C_turnout::C_turnout");
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x4bc490
C_turnout::~C_turnout()
{
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// The original's is C_pumper's, folded: it logs that name, so ours folds too
// STUB: GAME 0x47c760 FOLDED
bool C_turnout::Init(I3D_frame *frm)
{
    StubLog("C_pumper::Init");
    return C_actor::Init(frm);
}

// Writes the type byte, the flag and the two names' values (PS2 name)
// FUNCTION: GAME 0x4bc4a0
bool C_turnout::SaveData(C_chunk *chunk)
{
    uchar zero = 0;
    chunk->Write(&zero, 1);
    chunk->Write(&unknown_c8, 1);
    chunk->Write(&unknown_70, 4);
    chunk->Write(&unknown_74, 4);
    chunk->Write(&unknown_78, 4);
    chunk->Write(&unknown_7c, 4);
    chunk->Write(&unknown_80, 4);
    chunk->Write(&unknown_84, 4);
    chunk->Write(unknown_88, 32);
    chunk->Write(unknown_a8, 32);
    return true;
}

// Reads the flag and the two names' values (PS2 name)
// FUNCTION: GAME 0x4bc600
bool C_turnout::LoadData(C_chunk *chunk)
{
    uchar type;
    chunk->Read(&type, 1);
    chunk->Read(&unknown_c8, 1);
    chunk->Read(&unknown_70, 4);
    chunk->Read(&unknown_74, 4);
    chunk->Read(&unknown_78, 4);
    chunk->Read(&unknown_7c, 4);
    chunk->Read(&unknown_80, 4);
    chunk->Read(&unknown_84, 4);
    chunk->Read(unknown_88, 32);
    chunk->Read(unknown_a8, 32);
    return true;
}

// Copies the turnout's values and its two names (PS2 name)
// FUNCTION: GAME 0x4bc830
void C_turnout::DuplicateFrom(C_actor *act)
{
    C_turnout *turnout = (C_turnout *)act;
    unknown_c8 = turnout->unknown_c8;
    unknown_70 = turnout->unknown_70;
    unknown_74 = turnout->unknown_74;
    unknown_78 = turnout->unknown_78;
    unknown_7c = turnout->unknown_7c;
    unknown_80 = turnout->unknown_80;
    unknown_84 = turnout->unknown_84;
    unknown_c8 = turnout->unknown_c8;
    strcpy(unknown_88, turnout->unknown_88);
    strcpy(unknown_a8, turnout->unknown_a8);
}

// STUB: GAME 0x4bc8c0
void C_turnout::GameInit()
{
    StubLog("C_turnout::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x4bcd40
void C_turnout::GameDone()
{
    StubLog("C_turnout::GameDone");
    C_actor::GameDone();
}

// STUB: GAME 0x4bce10
void C_turnout::AI(uint time)
{
    StubLog("C_turnout::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x4bcfe0
void C_turnout::Update(uint time)
{
    StubLog("C_turnout::Update");
    C_actor::Update(time);
}

// FUNCTION: GAME 0x4bd420
int C_turnout::SaveGameGetSize()
{
    return C_actor::SaveGameGetSize() + 16;
}

// FUNCTION: GAME 0x4bd430
bool C_turnout::SaveGameSave(void **buf)
{
    if (!C_actor::SaveGameSave(buf))
        return false;
    uchar *p = (uchar *)*buf;
    *p = 2;
    *(uint *)(p + 4) = unknown_190;
    *(uint *)(p + 0xc) = unknown_144;
    return true;
}

// STUB: GAME 0x4bd470
bool C_turnout::SaveGameLoad(void **buf)
{
    StubLog("C_turnout::SaveGameLoad");
    return C_actor::SaveGameLoad(buf);
}

#pragma auto_inline(on)
