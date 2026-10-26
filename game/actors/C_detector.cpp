// The script objects (PS2 C_detector.cpp; the PC's file is the one at 0x432870-0x433fdc, docs/game-files.md), whose
// constructor (0x432880) comes first. Written so far: the constructor and Init, which C_mission::CreateActor and
// OpenBin call, and the destructor. Not written: the rest, from DuplicateFrom (0x4329c0) on; LoadData, which OpenBin
// calls next, is a stub.

#include "actors/C_detector.h"

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

// STUB: GAME 0x432b20
void C_detector::LoadData(C_chunk *chunk)
{
    StubLog("C_detector::LoadData");
}

#pragma auto_inline(on)
