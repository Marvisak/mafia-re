// The cars with dynamic collisions (PS2 C_car_ex.cpp; the PC's file is the one at 0x42e240-0x4307cc,
// docs/game-files.md), which starts with the constructor. Not written yet: the constructor, which
// C_mission::CreateActor calls, is a stub.

#include "vehicles/C_car_ex.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x42e240
C_car_ex::C_car_ex()
{
    StubLog("C_car_ex::C_car_ex");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x42e300
C_car_ex::~C_car_ex()
{
    StubLog("C_car_ex::~C_car_ex");
}

// STUB: GAME 0x42e3a0
bool C_car_ex::Init(I3D_frame *frm)
{
    StubLog("C_car_ex::Init");
    return C_car::Init(frm);
}

// STUB: GAME 0x42e3c0
void C_car_ex::GameInit()
{
    StubLog("C_car_ex::GameInit");
    C_car::GameInit();
}

// STUB: GAME 0x42ee30
void C_car_ex::GameDone()
{
    StubLog("C_car_ex::GameDone");
    C_car::GameDone();
}

// STUB: GAME 0x42f110
void C_car_ex::LoadData(C_chunk *chunk)
{
    StubLog("C_car_ex::LoadData");
    C_car::LoadData(chunk);
}

// STUB: GAME 0x42f130
void C_car_ex::SaveData(C_chunk *chunk)
{
    StubLog("C_car_ex::SaveData");
    C_car::SaveData(chunk);
}

// STUB: GAME 0x42f390
void C_car_ex::Update(uint time)
{
    StubLog("C_car_ex::Update");
    C_car::Update(time);
}

#pragma auto_inline(on)
