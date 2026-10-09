#pragma once

#include "main.h"

#include <d3d8.h>

struct S_adapter_mode
{
    uint width;
    uint height;
    uint flags;
};

int GetTotalVRAM();
bool LoadAdapterRefreshRates();
bool LoadAdapterModes();
bool LoadAdapterData(int adapter);
bool LoadD3D8();
void FreeD3D8();
void AutodetectSettings();
