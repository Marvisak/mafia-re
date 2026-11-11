// The cars' AI drivers (PS2 C_driver.cpp; the PC's file is the one at 0x43bd90-0x4435ac, docs/game-files.md), which
// starts with the constructor (0x43bd90) and the destructor. Written so far: the destructor, which the chases' Done
// calls; the constructor, SetCar and Tick, which the chases call, are stubs.

#include "ai/C_driver.h"

#include "stub.h"

// STUB: GAME 0x43bd90
C_driver::C_driver()
{
    StubLog("C_driver::C_driver");
}

// FUNCTION: GAME 0x43bee0
C_driver::~C_driver()
{
    path.clear();
}

// STUB: GAME 0x43c350
void C_driver::SetCar(C_car *car)
{
    StubLog("C_driver::SetCar");
}

// STUB: GAME 0x43c4f0
void C_driver::Tick(uint time)
{
    StubLog("C_driver::Tick");
}
