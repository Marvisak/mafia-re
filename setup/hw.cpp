#include "hw.h"
#include "res/resource.h"

#include <ddraw.h>
#include <stdio.h>

IDirect3D8 *d3d8;

char global_buffer[256];
S_adapter_mode adapter_modes[256];

bool LoadD3D8()
{
    if (app_state.windows.display)
    {
        d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
        if (d3d8)
        {
            uint adapter_count = d3d8->GetAdapterCount();
            if (app_state.settings.adapter >= adapter_count || app_state.settings.adapter < 0)
            {
                app_state.settings.adapter = 0;
            }

            int current_adapter = 0;
            D3DADAPTER_IDENTIFIER8 identifier;
            for (int i = 0; i < adapter_count; ++i)
            {
                if (d3d8->GetAdapterIdentifier(i, 0, &identifier) == D3D_OK)
                {
                    int index = SendMessageA(app_state.windows.display, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(identifier.Description));
                    SendMessageA(app_state.windows.display, CB_SETITEMDATA, index, i);
                    if (app_state.settings.adapter == i)
                    {
                        current_adapter = SendMessageA(app_state.windows.display, CB_GETCOUNT, NULL, NULL) - 1;
                    }
                }
            }
            SendMessageA(app_state.windows.display, CB_SETCURSEL, current_adapter, NULL);
            LoadAdapterModes();
            D3DCAPS8 caps;
            if (d3d8->GetDeviceCaps(app_state.settings.adapter, D3DDEVTYPE_HAL, &caps) == D3D_OK)
            {
                app_state.supports_w_buffer = (caps.RasterCaps & D3DPRASTERCAPS_WBUFFER) != 0;
            }
            else
            {
                app_state.supports_w_buffer = true;
            }

            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
        return d3d8 != NULL;
    }
}

void FreeD3D8()
{
    if (d3d8)
    {
        d3d8->Release();
        d3d8 = NULL;
    }
}

bool LoadAdapterModes()
{
    if (app_state.windows.resolution)
    {
        SendMessageA(app_state.windows.resolution, CB_RESETCONTENT, NULL, NULL);
    }

    if (!d3d8)
    {
        return false;
    }

    char buffer[256];
    D3DADAPTER_IDENTIFIER8 identifier;
    HRESULT result = d3d8->GetAdapterIdentifier(app_state.settings.adapter, 0, &identifier);
    app_state.vram_size = GetTotalVRAM();
    if (app_state.windows.driver_version)
    {
        if (result != D3D_OK)
        {
            global_buffer[0] = '\0';
        }
        else
        {
            if (!LoadStringA(NULL, IDS_DRIVERVERSION, buffer, sizeof(buffer)))
            {
                strcpy(buffer, "Driver version: %d.%02d.%02d.%04d");
            }
            sprintf(
                global_buffer,
                buffer,
                identifier.DriverVersion.HighPart >> 16,
                identifier.DriverVersion.HighPart & 0xFFFF,
                identifier.DriverVersion.LowPart >> 16,
                identifier.DriverVersion.LowPart & 0xFFFF);
        }
        SetWindowTextA(app_state.windows.driver_version, global_buffer);
    }
    if (app_state.windows.vram_size)
    {
        sprintf(global_buffer, "VRAM: %iMB", app_state.vram_size);
        SetWindowTextA(app_state.windows.vram_size, global_buffer);
    }

    if (!app_state.windows.resolution)
    {
        return true;
    }

    uint adapter_mode_count = d3d8->GetAdapterModeCount(app_state.settings.adapter);
    int total_modes = 0;
    for (int i = 0; i < adapter_mode_count; ++i)
    {
        int bit_depth_flags = 0;
        D3DDISPLAYMODE mode;
        if (d3d8->EnumAdapterModes(app_state.settings.adapter, i, &mode) == D3D_OK)
        {
            switch (mode.Format)
            {
            case D3DFMT_A8R8G8B8:
            case D3DFMT_X8R8G8B8:
                bit_depth_flags = BIT_DEPTH_32;
                break;
            case D3DFMT_R5G6B5:
            case D3DFMT_X1R5G5B5:
                bit_depth_flags = BIT_DEPTH_16;
                break;
            default:
                continue;
            }

            for (int j = 0; j < total_modes; ++j)
            {
                S_adapter_mode &cached_mode = adapter_modes[j];
                if (cached_mode.width == mode.Width && cached_mode.height == mode.Height)
                {
                    if ((cached_mode.flags & bit_depth_flags) == 0)
                    {
                        cached_mode.flags |= bit_depth_flags;
                    }
                    goto found_duplicate;
                }
            }

            S_adapter_mode &cached_mode = adapter_modes[total_modes];
            cached_mode.width = mode.Width;
            cached_mode.height = mode.Height;
            cached_mode.flags = bit_depth_flags;
            if (d3d8->CheckDeviceMultiSampleType(app_state.settings.adapter, D3DDEVTYPE_HAL, mode.Format, true, D3DMULTISAMPLE_2_SAMPLES) >= D3D_OK)
            {
                cached_mode.flags |= MULTISAMPLE_2X_WINDOWED;
            }

            if (d3d8->CheckDeviceMultiSampleType(app_state.settings.adapter, D3DDEVTYPE_HAL, mode.Format, true, D3DMULTISAMPLE_4_SAMPLES) >= D3D_OK)
            {
                cached_mode.flags |= MULTISAMPLE_4X_WINDOWED;
            }

            if (d3d8->CheckDeviceMultiSampleType(app_state.settings.adapter, D3DDEVTYPE_HAL, mode.Format, false, D3DMULTISAMPLE_2_SAMPLES) >= D3D_OK)
            {
                cached_mode.flags |= MULTISAMPLE_2X_FULLSCREEN;
            }

            if (d3d8->CheckDeviceMultiSampleType(app_state.settings.adapter, D3DDEVTYPE_HAL, mode.Format, false, D3DMULTISAMPLE_4_SAMPLES) >= D3D_OK)
            {
                cached_mode.flags |= MULTISAMPLE_4X_FULLSCREEN;
            }

            if (++total_modes >= 256)
            {
                break;
            }
        }
    found_duplicate:;
    }

    for (int j = 0; j < total_modes; ++j)
    {
        S_adapter_mode &cached_mode = adapter_modes[j];
        sprintf(global_buffer, "%4d x %4d", cached_mode.width, cached_mode.height);
        int pos = SendMessageA(app_state.windows.resolution, CB_ADDSTRING, -1, reinterpret_cast<LPARAM>(global_buffer));
        SendMessageA(app_state.windows.resolution, CB_SETITEMDATA, pos, reinterpret_cast<LPARAM>(&cached_mode));
        if (cached_mode.width == app_state.settings.width && cached_mode.height == app_state.settings.height)
        {
            SendMessageA(app_state.windows.resolution, CB_SETCURSEL, pos, NULL);
        }
    }
    LoadAdapterRefreshRates();

    return true;
}

bool LoadAdapterRefreshRates()
{
    char buffer[128];
    if (app_state.windows.refresh_rate)
    {
        if (LoadStringA(NULL, IDS_DEFAULT, buffer, sizeof(buffer) == 0))
        {
            strcpy(buffer, "Default");
        }
        SendMessageA(app_state.windows.refresh_rate, CB_RESETCONTENT, NULL, NULL);
        int pos = SendMessageA(app_state.windows.refresh_rate, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(buffer));
        SendMessageA(app_state.windows.refresh_rate, CB_SETITEMDATA, pos, NULL);
    }

    if (!d3d8)
    {
        return false;
    }

    int selected = 0;
    if (app_state.windows.refresh_rate)
    {
        uint adapter_mode_count = d3d8->GetAdapterModeCount(app_state.settings.adapter);
        for (int i = 0; i < adapter_mode_count; ++i)
        {
            D3DDISPLAYMODE mode;
            if (d3d8->EnumAdapterModes(app_state.settings.adapter, i, &mode) == D3D_OK)
            {
                switch (mode.Format)
                {
                case D3DFMT_A8R8G8B8:
                case D3DFMT_X8R8G8B8:
                    if (app_state.settings.bit_depth != 32)
                    {
                        continue;
                    }
                    break;
                case D3DFMT_R5G6B5:
                case D3DFMT_X1R5G5B5:
                    if (app_state.settings.bit_depth != 16)
                    {
                        continue;
                    }
                    break;
                default:
                    continue;
                }

                if (mode.Width == app_state.settings.width && mode.Height == app_state.settings.height && mode.RefreshRate != 0)
                {
                    sprintf(global_buffer, "%d Hz", mode.RefreshRate);
                    int pos = SendMessageA(app_state.windows.refresh_rate, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(global_buffer));
                    SendMessageA(app_state.windows.refresh_rate, CB_SETITEMDATA, pos, mode.RefreshRate);
                    if (app_state.settings.refresh_rate == mode.RefreshRate)
                    {
                        selected = pos;
                    }
                }
            }
        }

        SendMessageA(app_state.windows.refresh_rate, CB_SETCURSEL, selected, NULL);
        if (selected == 0)
        {
            app_state.settings.refresh_rate = 0;
        }
    }
    return true;
}

bool LoadAdapterData(int adapter)
{
    char buffer[128];
    if (LoadStringA(NULL, IDS_OFF, buffer, sizeof(buffer)) == 0)
    {
        strcpy(buffer, " OFF");
    }

    SendMessageA(app_state.windows.bit_depth, CB_RESETCONTENT, NULL, NULL);
    SendMessageA(app_state.windows.antialiasing, CB_RESETCONTENT, NULL, NULL);
    int pos = SendMessageA(app_state.windows.antialiasing, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(buffer));
    SendMessageA(app_state.windows.antialiasing, CB_SETITEMDATA, pos, NULL);
    if (adapter == -1)
    {
        uint adapter_mode_count = d3d8->GetAdapterModeCount(app_state.settings.adapter);
        int i = 0;
        while (i < adapter_mode_count)
        {
            S_adapter_mode &cached_mode = adapter_modes[i];
            if (app_state.settings.width == cached_mode.width && app_state.settings.height == cached_mode.height)
            {
                break;
            }
            ++i;
        }

        if (i == adapter_mode_count)
        {
            return false;
        }
        adapter = i;
    }

    // This section looks this horrible because the control flow was so destroyed by the compiler that it has like 10 gotos
    S_adapter_mode *adapter_data = reinterpret_cast<S_adapter_mode *>(SendMessageA(app_state.windows.resolution, CB_GETITEMDATA, adapter, NULL));
    if (!app_state.settings.antialiasing)
    {
        SendMessageA(app_state.windows.antialiasing, CB_SETCURSEL, NULL, NULL);
    }
    else if (!app_state.settings.fullscreen)
    {
        if (adapter_data->flags & (MULTISAMPLE_2X_WINDOWED | MULTISAMPLE_4X_WINDOWED))
        {
            SendMessageA(app_state.windows.antialiasing, CB_SETCURSEL, NULL, NULL);
        }
    }
    else if ((adapter_data->flags & (MULTISAMPLE_2X_FULLSCREEN | MULTISAMPLE_4X_FULLSCREEN)) == (MULTISAMPLE_2X_FULLSCREEN | MULTISAMPLE_4X_FULLSCREEN))
    {
        SendMessageA(app_state.windows.antialiasing, CB_SETCURSEL, NULL, NULL);
    }

    if (adapter_data->flags & BIT_DEPTH_16)
    {
        pos = SendMessageA(app_state.windows.bit_depth, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>("16"));
        SendMessageA(app_state.windows.bit_depth, CB_SETITEMDATA, pos, 16);
        if ((adapter_data->flags & BIT_DEPTH_32) == 0 && app_state.settings.bit_depth == 32)
        {
            app_state.settings.bit_depth = 16;
        }
    }

    if (adapter_data->flags & BIT_DEPTH_32)
    {
        pos = SendMessageA(app_state.windows.bit_depth, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>("32"));
        SendMessageA(app_state.windows.bit_depth, CB_SETITEMDATA, pos, 32);
        if ((adapter_data->flags & BIT_DEPTH_16) == 0 && app_state.settings.bit_depth == 16)
        {
            app_state.settings.bit_depth = 32;
        }
    }

    if (((adapter_data->flags & (BIT_DEPTH_16 | BIT_DEPTH_32)) == (BIT_DEPTH_16 | BIT_DEPTH_32)) && app_state.settings.bit_depth == 32)
    {
        SendMessageA(app_state.windows.bit_depth, CB_SETCURSEL, 1, 0);
    }
    else
    {
        SendMessageA(app_state.windows.bit_depth, CB_SETCURSEL, 0, 0);
    }

    if (app_state.settings.fullscreen)
    {
        if (adapter_data->flags & MULTISAMPLE_2X_FULLSCREEN)
        {
            pos = SendMessageA(app_state.windows.antialiasing, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(" 2x"));
            SendMessageA(app_state.windows.antialiasing, CB_SETITEMDATA, pos, 2);
            if (app_state.settings.antialiasing == 2)
            {
                SendMessageA(app_state.windows.antialiasing, CB_SETCURSEL, pos, 0);
            }
        }
        if ((adapter_data->flags & MULTISAMPLE_4X_FULLSCREEN) == 0)
        {
            return true;
        }
    }
    else
    {
        if (adapter_data->flags & MULTISAMPLE_2X_WINDOWED)
        {
            pos = SendMessageA(app_state.windows.antialiasing, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(" 2x"));
            SendMessageA(app_state.windows.antialiasing, CB_SETITEMDATA, pos, 2);
            if (app_state.settings.antialiasing == 2)
            {
                SendMessageA(app_state.windows.antialiasing, CB_SETCURSEL, pos, 0);
            }
        }
        if ((adapter_data->flags & MULTISAMPLE_4X_WINDOWED) == 0)
        {
            return true;
        }
    }
    pos = SendMessageA(app_state.windows.antialiasing, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(" 4x"));
    SendMessageA(app_state.windows.antialiasing, CB_SETITEMDATA, pos, 4);
    if (app_state.settings.antialiasing == 4)
    {
        SendMessageA(app_state.windows.antialiasing, CB_SETCURSEL, pos, 0);
    }

    return true;
}

// The only thing I will add here is that the numbers are in megabytes
// Only the best of the best, true beasts can run Mafia with true color textures
void AutodetectSettings()
{
    app_state.settings.multipass_rendering = false;
    app_state.settings.suspend_when_inactive = true;
    app_state.settings.clip_always = false;
    app_state.settings.antialiasing = false;
    app_state.settings.fullscreen = true;
    app_state.settings.hardware_transform = true;
    app_state.settings.vsync = false;
    app_state.settings.refresh_rate = false;
    app_state.settings.disable_sound = false;
    app_state.settings.software_mixing_only = false;
    if (app_state.vram_size > 8)
    {
        if (app_state.vram_size > 16)
        {
            if (app_state.vram_size > 32)
            {
                if (app_state.vram_size > 64)
                {
                    app_state.settings.width = 1280;
                    app_state.settings.height = 1024;
                    app_state.settings.texture_true_color = true;
                    app_state.settings.texture_compressed = false;
                }
                else
                {
                    app_state.settings.width = 1024;
                    app_state.settings.height = 768;
                    app_state.settings.texture_true_color = false;
                    app_state.settings.texture_compressed = true;
                }
            }
            else
            {
                app_state.settings.width = 800;
                app_state.settings.height = 600;
                app_state.settings.texture_true_color = false;
                app_state.settings.texture_compressed = true;
            }
            app_state.settings.bit_depth = 32;
            app_state.settings.texture_low_detail = false;
        }
        else
        {
            app_state.settings.width = 800;
            app_state.settings.height = 600;
            app_state.settings.texture_true_color = false;
            app_state.settings.texture_low_detail = true;
            app_state.settings.texture_compressed = true;
        }
    }
    else
    {
        app_state.settings.width = 640;
        app_state.settings.height = 480;
        app_state.settings.lightmap_true_color = false;
        app_state.settings.texture_true_color = false;
        app_state.settings.texture_low_detail = true;
        app_state.settings.texture_compressed = true;
    }
}

// I have no clue how they came up with this abomination
// It doesn't work and requires DirectDraw7 which was already deprecated
uint GetTotalVRAM()
{
    IDirectDraw7 *dd;
    if (DirectDrawCreateEx(NULL, reinterpret_cast<LPVOID *>(&dd), IID_IDirectDraw7, NULL) != DD_OK)
    {
        return 0;
    }
    DDSCAPS2 caps = {0};
    caps.dwCaps = DDSCAPS_LOCALVIDMEM;

    DWORD vram;
    DWORD available_vram;
    if (dd->GetAvailableVidMem(&caps, &vram, &available_vram) < 0)
    {
        return 0;
    }
    dd->Release();

    uint vram_mb = 1;
    while ((vram / 0x100000) > vram_mb)
    {
        vram_mb *= 2;
    }
    return vram_mb;
}
