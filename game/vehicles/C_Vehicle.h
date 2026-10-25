#pragma once

#include "Main.h"

// The vehicles' shared part (PS2 C_Vehicle, C_Vehicle.cpp). Only what CloseSystem uses so far: the tables InitSystem
// loads through C_car, which CloseSystem frees through this class, as PS2's.
class C_Vehicle
{
public:
    static void CloseTyres();
    static void CloseVehicleMaterials();
    static void CloseParticles();
};
