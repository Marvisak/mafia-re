#pragma once

#include "main.h"

#include <d3d8.h>

// From the DirectX SDK's dxguid.lib, the only one of its GUIDs setup uses
// GLOBAL: SETUP 0x40d37c
// GLOBAL: SETUP10 0x40c36c
// GLOBAL: SETUPSTEAM 0x40414c
// IID_IDirectDraw7

struct S_adapter_mode
{
    uint width;
    uint height;
    uint flags;
};

int GetTotalVRAM(GUID *guid);
bool LoadAdapterRefreshRates();
bool LoadAdapterModes();
bool LoadAdapterData(int adapter);
bool LoadD3D8();
void FreeD3D8();
