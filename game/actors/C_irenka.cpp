// PS2's C_irenka (PS2 C_irenka.cpp; the PC's file is the one at 0x445850-0x44602c, docs/game-files.md), which starts
// with the constructor. Not written yet: the constructor, which C_mission::CreateActor calls, is a stub.

#include "actors/C_irenka.h"

#include "stub.h"

// STUB: GAME 0x445850
C_irenka::C_irenka()
{
    StubLog("C_irenka::C_irenka");
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x4458a0
C_irenka::~C_irenka()
{
}

// STUB: GAME 0x4458b0
void C_irenka::AI(uint time)
{
    StubLog("C_irenka::AI");
    C_actor::AI(time);
}

// STUB: GAME 0x4458c0
void C_irenka::GameInit()
{
    StubLog("C_irenka::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x445b70
void C_irenka::GameDone()
{
    StubLog("C_irenka::GameDone");
    C_actor::GameDone();
}

// The original's is C_pumper's, folded: it logs that name, so ours folds too
// STUB: GAME 0x47c760 FOLDED
bool C_irenka::Init(I3D_frame *frm)
{
    StubLog("C_pumper::Init");
    return C_actor::Init(frm);
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_irenka::Update(uint time)
{
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_irenka::NetDirect(uint time)
{
}

// The linker's folded copy of the out-of-line kind (0x4aa430)
int C_irenka::SaveGameGetSize()
{
    return 0;
}
