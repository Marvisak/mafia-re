// The cars with dynamic collisions (PS2 C_car_ex.cpp; the PC's file is the one at 0x42e240-0x4307cc,
// docs/game-files.md), which starts with the constructor. Written: the constructor, destructor and Init; what the
// scripts read.

#include "vehicles/C_car_ex.h"

#include "stub.h"

#pragma auto_inline(off)

// The four vectors default-construct with the car (PS2 name)
// FUNCTION: GAME 0x42e240
C_car_ex::C_car_ex()
{
    subtype = 3;
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// The four vectors' destructors follow the base's (PS2 name)
// FUNCTION: GAME 0x42e300
C_car_ex::~C_car_ex()
{
}

// FUNCTION: GAME 0x42e3a0
bool C_car_ex::Init(I3D_frame *frm)
{
    return C_car::Init(frm) ? true : false;
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

// The car's data, normalized (PS2 name)
// FUNCTION: GAME 0x42f110
bool C_car_ex::LoadData(C_chunk *chunk)
{
    return C_car::LoadData(chunk) ? true : false;
}

// The car's data, normalized (PS2 name)
// FUNCTION: GAME 0x42f130
bool C_car_ex::SaveData(C_chunk *chunk)
{
    return C_car::SaveData(chunk) ? true : false;
}

// STUB: GAME 0x42f390
void C_car_ex::Update(uint time)
{
    StubLog("C_car_ex::Update");
    C_car::Update(time);
}

#pragma auto_inline(on)
