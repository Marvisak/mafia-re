// The script objects (PS2 C_detector.cpp; the PC's file is the one at 0x432870-0x433fdc, docs/game-files.md), whose
// constructor (0x432880) comes first. Written so far: the constructor and Init, which C_mission::CreateActor and
// OpenBin call, the destructor, and LoadData, which OpenBin calls next. Not written: the rest, from DuplicateFrom
// (0x4329c0) on; the vtable's functions from DuplicateFrom to GameDone are stubs.

#include "actors/C_detector.h"

#include "C_chunk.h"
#include "stub.h"

// FUNCTION: GAME 0x432880
C_detector::C_detector() :
    unknown_f0(0),
    unknown_f8(0),
    unknown_fc(0),
    unknown_100(0),
    unknown_104(false)
{
    type = ACTOR_DETECTOR;
}

// FUNCTION: GAME 0x432930
C_detector::~C_detector()
{
}

// FUNCTION: GAME 0x432990
bool C_detector::Init(I3D_frame *frm)
{
    if (!C_actor::Init(frm))
        return false;
    unknown_d8 = 0;
    actor = this;
    return true;
}

#pragma auto_inline(off)

// STUB: GAME 0x4329c0
void C_detector::DuplicateFrom(C_actor *act)
{
    StubLog("C_detector::DuplicateFrom");
}

// STUB: GAME 0x432a00
void C_detector::GameInit()
{
    StubLog("C_detector::GameInit");
}

// STUB: GAME 0x432a80
void C_detector::GameDone()
{
    StubLog("C_detector::GameDone");
}

#pragma auto_inline(on)

// A byte nothing reads, the dword at 0xd8, then the script, as PS2's
// FUNCTION: GAME 0x432b20
void C_detector::LoadData(C_chunk *chunk)
{
    uchar unknown;
    chunk->Read(&unknown, sizeof(unknown));
    chunk->Read(&unknown_d8, sizeof(unknown_d8));
    LoadSourceCode(chunk);
}

#pragma auto_inline(off)

// STUB: GAME 0x432c10
int C_detector::SaveGameGetSize()
{
    StubLog("C_detector::SaveGameGetSize");
    return 0;
}

// STUB: GAME 0x432c30
bool C_detector::SaveGameSave(void **buf)
{
    StubLog("C_detector::SaveGameSave");
    return false;
}

// STUB: GAME 0x432ca0
bool C_detector::SaveGameLoad(void **buf)
{
    StubLog("C_detector::SaveGameLoad");
    return false;
}

#pragma auto_inline(on)
