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

// Inlined into DialogFunc, in 1.2 too (it has no out-of-line copy)
// Only the best of the best, true beasts can run Mafia with true color textures
inline void AutodetectSettings()
{
    settings.multipass_rendering = false;
    settings.suspend_when_inactive = true;
    settings.clip_always = false;
    settings.antialiasing = 0;
    settings.fullscreen = true;
    settings.hardware_transform = true;
    settings.vsync = false;
    settings.refresh_rate = 0;
    settings.disable_sound = false;
    settings.software_mixing_only = false;
    if (app_state.vram_size > 8)
    {
        if (app_state.vram_size > 16)
        {
            if (app_state.vram_size <= 32)
            {
                settings.width = 800;
                settings.height = 600;
                settings.texture_true_color = false;
                settings.texture_compressed = true;
            }
            else if (app_state.vram_size > 64)
            {
                settings.width = 1280;
                settings.height = 1024;
                settings.texture_true_color = true;
                settings.texture_compressed = false;
            }
            else
            {
                settings.width = 1024;
                settings.height = 768;
                settings.texture_true_color = false;
                settings.texture_compressed = true;
            }
            settings.bit_depth = 32;
            settings.texture_low_detail = false;
        }
        else
        {
            settings.width = 800;
            settings.height = 600;
            settings.bit_depth = 16;
            settings.texture_true_color = false;
            settings.texture_low_detail = true;
            settings.texture_compressed = true;
        }
        settings.lightmap_true_color = true;
    }
    else
    {
        settings.width = 640;
        settings.height = 480;
        settings.bit_depth = 16;
        settings.lightmap_true_color = false;
        settings.texture_true_color = false;
        settings.texture_low_detail = true;
        settings.texture_compressed = true;
    }
}
