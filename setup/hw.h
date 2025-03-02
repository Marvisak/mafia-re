#pragma once

#include "main.h"

#include <d3d8.h>

struct S_adapter_mode
{
    uint width;
    uint height;
    uint flags;
};

bool LoadD3D8();
void FreeD3D8();

bool LoadAdapterModes();
bool LoadAdapterRefreshRates();
bool LoadAdapterData(int adapter);

void AutodetectSettings();
uint GetTotalVRAM();
