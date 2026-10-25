#pragma once

#include "Main.h"

// A car (PS2 C_car, C_car.cpp). Only what InitSystem uses so far: its tables, which InitSystem loads once.
class C_car
{
public:
    static void LoadMaterials();
    static void LoadParticles();
    static void LoadTyres();
};
