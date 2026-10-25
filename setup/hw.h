#pragma once

#include "main.h"

#include <d3d8.h>

// From the DirectX SDK's dxguid.lib, the only one of its GUIDs setup uses
// GLOBAL: SETUP 0x40d37c
// GLOBAL: SETUP10 0x40c36c
// GLOBAL: SETUPCZ 0x40d37c
// GLOBAL: SETUPPL 0x40d37c
// GLOBAL: SETUPSTEAM 0x40414c
// IID_IDirectDraw7

struct S_adapter_mode
{
    uint width;
    uint height;
    uint flags;
};

// The most modes LoadAdapterModes keeps in adapter_modes
#define MAX_ADAPTER_MODES 256

int GetTotalVRAM(GUID *guid);
bool LoadAdapterRefreshRates();
bool LoadAdapterModes();
// adapter: the resolution box's entry, or -1 for the settings' resolution
bool LoadAdapterData(int adapter);
bool LoadD3D8();
void FreeD3D8();

// The Czech setup's dynamic initializers (hw.cpp's dialog_state and settings): built without optimization, each
// entry of the initializer table calls the constructor's code instead of jumping to it, so reccmp doesn't pair
// them through the table
// SYNTHETIC: SETUPCZ 0x402c70
// $E2

// SYNTHETIC: SETUPCZ 0x402c80
// $E1

// SYNTHETIC: SETUPCZ 0x402cf0
// $E5

// SYNTHETIC: SETUPCZ 0x402d00
// $E4
