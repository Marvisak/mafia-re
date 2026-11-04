// PS2's C_actor_shot (PS2 C_actor_shot.cpp; the PC's file is the one at 0x566f40-0x5674ec, docs/game-files.md),
// which starts with the constructor. Not written yet: the constructor, which C_mission::CreateActor calls, is a
// stub.

#include "actors/C_actor_shot.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x566f40
C_actor_shot::C_actor_shot()
{
    StubLog("C_actor_shot::C_actor_shot");
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x566f80
C_actor_shot::~C_actor_shot()
{
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x404e20
bool C_actor_shot::AddToCutscene()
{
    StubLog("C_actor_shot::AddToCutscene");
    return C_actor::AddToCutscene();
}

// STUB: GAME 0x404e40
void C_actor_shot::PlayRecord(int a)
{
    StubLog("C_actor_shot::PlayRecord");
    C_actor::PlayRecord(a);
}

// The original's is C_dyn_object's, folded: it logs that name, so ours folds too
// STUB: GAME 0x41bae0 FOLDED
S_vector C_actor_shot::GetRecordPos(int a)
{
    StubLog("C_dyn_object::GetRecordPos");
    return C_actor::GetRecordPos(a);
}

// The original's is C_dyn_object's, folded: it logs that name, so ours folds too
// STUB: GAME 0x41bb00 FOLDED
S_quat C_actor_shot::GetRecordRot(int a)
{
    StubLog("C_dyn_object::GetRecordRot");
    return C_actor::GetRecordRot(a);
}

// The linker's folded copy (0x4aa490)
bool C_actor_shot::PlaceAtTime(int a)
{
    return true;
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_actor_shot::AI(uint time)
{
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_actor_shot::NetDirect(uint time)
{
}

// STUB: GAME 0x566f90
bool C_actor_shot::Init(I3D_frame *frm)
{
    StubLog("C_actor_shot::Init");
    return C_actor::Init(frm);
}

// STUB: GAME 0x566fd0
void C_actor_shot::Update(uint time)
{
    StubLog("C_actor_shot::Update");
    C_actor::Update(time);
}

// STUB: GAME 0x567330
void C_actor_shot::GameInit()
{
    StubLog("C_actor_shot::GameInit");
    C_actor::GameInit();
}

// STUB: GAME 0x5673f0
void C_actor_shot::GameDone()
{
    StubLog("C_actor_shot::GameDone");
    C_actor::GameDone();
}

// The linker's folded copy of the out-of-line kind (0x4aa430)
int C_actor_shot::SaveGameGetSize()
{
    return 0;
}

#pragma auto_inline(on)
