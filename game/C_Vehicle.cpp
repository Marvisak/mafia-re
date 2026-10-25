// The vehicles (PS2 C_Vehicle.cpp, which frees the cars' tables; the PC's file is most likely the one at
// 0x4bd650-0x4d34ac, which has the three, docs/game-files.md). Not written yet: only what CloseSystem needs to link.

#include "C_Vehicle.h"

#include "stub.h"

#pragma auto_inline(off)

// STUB: GAME 0x4c5b80
void C_Vehicle::CloseTyres()
{
    StubLog("C_Vehicle::CloseTyres");
}

// STUB: GAME 0x4c5c80
void C_Vehicle::CloseVehicleMaterials()
{
    StubLog("C_Vehicle::CloseVehicleMaterials");
}

// STUB: GAME 0x4c5f80
void C_Vehicle::CloseParticles()
{
    StubLog("C_Vehicle::CloseParticles");
}

#pragma auto_inline(on)
