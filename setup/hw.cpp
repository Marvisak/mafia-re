#include "hw.h"
#include "res/resource.h"

#include <ddraw.h>
#include <cstdio>

// A default library, which LINK searches after the CRT's: both builds have the Direct3DCreate8 thunk after the
// CRT's thunks (1.2 with KERNEL32's, Steam's after MSVCR90's), not with the import libraries of the command line
#pragma comment(lib, "d3d8.lib")

// The first initialized data of this file in 1.2
// GLOBAL: SETUP 0x40e144
// GLOBAL: SETUPSTEAM 0x405018
bool supports_w_buffer = true;

// 1.2 has this file's uninitialized globals in this order, which VC6's hash order of these names doesn't give,
// so the original names differ
// GLOBAL: SETUP 0x410d48
// GLOBAL: SETUPSTEAM 0x405fc0
char global_buffer[256];
// app_state and settings are constructed at startup: their dynamic initializers start this file's code
// GLOBAL: SETUP 0x410e48
// GLOBAL: SETUPSTEAM 0x4060c0
S_state app_state;
// GLOBAL: SETUP 0x410e70
// GLOBAL: SETUPSTEAM 0x4053b8
IDirect3D8 *d3d8;
// GLOBAL: SETUP 0x410e74
// GLOBAL: SETUPSTEAM 0x40501c
S_settings settings;
// GLOBAL: SETUP 0x410eb8
// GLOBAL: SETUPSTEAM 0x4053c0
S_adapter_mode adapter_modes[256];

// The numbers are in megabytes, the result is rounded up to a power of two
// Requires DirectDraw7 which was already deprecated at the time
// FUNCTION: SETUP 0x4026f0
// FUNCTION: SETUPSTEAM 0x4026d0
int GetTotalVRAM(GUID *guid)
{
    IDirectDraw7 *dd;
    if (FAILED(DirectDrawCreateEx(guid, reinterpret_cast<LPVOID *>(&dd), IID_IDirectDraw7, NULL)))
    {
        return 0;
    }
    // Cleared whole and then dwCaps set: 1.2 stores all four dwords, an initializer leaves dwCaps out
    DDSCAPS2 caps;
    memset(&caps, 0, sizeof(caps));
    caps.dwCaps = DDSCAPS_LOCALVIDMEM;

    DWORD vram;
    DWORD available_vram;
    if (FAILED(dd->GetAvailableVidMem(&caps, &vram, &available_vram)))
    {
        return 0;
    }
    dd->Release();

    uint total_mb = vram / 0x100000;
    int vram_mb = 1;
    while (total_mb > static_cast<uint>(vram_mb))
    {
        vram_mb <<= 1;
    }
    return vram_mb;
}

// FUNCTION: SETUP 0x402780
// FUNCTION: SETUPSTEAM 0x402750
bool LoadAdapterRefreshRates()
{
    char buffer[128];
    if (app_state.windows.refresh_rate)
    {
        if (LoadStringA(NULL, IDS_DEFAULT, buffer, sizeof(buffer)) == 0)
        {
            // STRING: SETUP 0x40e150
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

    if (app_state.windows.refresh_rate)
    {
        int selected = 0;
        int adapter_mode_count = d3d8->GetAdapterModeCount(settings.adapter);
        for (int i = 0; i < adapter_mode_count; ++i)
        {
            D3DDISPLAYMODE mode;
            if (d3d8->EnumAdapterModes(settings.adapter, i, &mode) == D3D_OK)
            {
                switch (mode.Format)
                {
                case D3DFMT_R5G6B5:
                case D3DFMT_X1R5G5B5:
                    if (settings.bit_depth != 16)
                    {
                        continue;
                    }
                    break;
                case D3DFMT_A8R8G8B8:
                case D3DFMT_X8R8G8B8:
                    if (settings.bit_depth != 32)
                    {
                        continue;
                    }
                    break;
                default:
                    continue;
                }

                if (mode.Width == settings.width && mode.Height == settings.height && mode.RefreshRate != 0)
                {
                    // STRING: SETUP 0x40e148
                    sprintf(global_buffer, "%d Hz", mode.RefreshRate);
                    int pos = SendMessageA(app_state.windows.refresh_rate, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(global_buffer));
                    SendMessageA(app_state.windows.refresh_rate, CB_SETITEMDATA, pos, mode.RefreshRate);
                    if (settings.refresh_rate == mode.RefreshRate)
                    {
                        selected = pos;
                    }
                }
            }
        }

        SendMessageA(app_state.windows.refresh_rate, CB_SETCURSEL, selected, NULL);
        if (selected == 0)
        {
            settings.refresh_rate = 0;
        }
    }
    return true;
}

// FUNCTION: SETUP 0x402940
// FUNCTION: SETUPSTEAM 0x402930
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
    HRESULT result = d3d8->GetAdapterIdentifier(settings.adapter, 0, &identifier);
    app_state.vram_size = GetTotalVRAM(NULL);
    if (app_state.windows.driver_version)
    {
        if (result != D3D_OK)
        {
            global_buffer[0] = '\0';
        }
        else
        {
            if (LoadStringA(NULL, IDS_DRIVERVERSION, buffer, sizeof(buffer)) == 0)
            {
                // STRING: SETUP 0x40e170
                strcpy(buffer, "Driver version: %d.%02d.%02d.%04d");
            }
            sprintf(
                global_buffer,
                buffer,
                HIWORD(identifier.DriverVersion.HighPart),
                LOWORD(identifier.DriverVersion.HighPart),
                HIWORD(identifier.DriverVersion.LowPart),
                LOWORD(identifier.DriverVersion.LowPart));
        }
        SetWindowTextA(app_state.windows.driver_version, global_buffer);
    }
    if (app_state.windows.vram_size)
    {
        // STRING: SETUP 0x40e164
        sprintf(global_buffer, "VRAM: %iMB", app_state.vram_size);
        SetWindowTextA(app_state.windows.vram_size, global_buffer);
    }

    if (!app_state.windows.resolution)
    {
        return true;
    }

    int adapter_mode_count = d3d8->GetAdapterModeCount(settings.adapter);
    int total_modes = 0;
    for (int i = 0; i < adapter_mode_count; ++i)
    {
        D3DDISPLAYMODE mode;
        if (d3d8->EnumAdapterModes(settings.adapter, i, &mode) != D3D_OK)
        {
            continue;
        }

        int bit_depth_flags;
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

        // The multisample flags are collected directly in the next free slot, even when the mode turns out to be a duplicate
        S_adapter_mode &new_mode = adapter_modes[total_modes];
        if (d3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, mode.Format, TRUE, D3DMULTISAMPLE_2_SAMPLES) >= D3D_OK)
        {
            new_mode.flags |= MULTISAMPLE_2X_WINDOWED;
        }

        if (d3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, mode.Format, TRUE, D3DMULTISAMPLE_4_SAMPLES) >= D3D_OK)
        {
            new_mode.flags |= MULTISAMPLE_4X_WINDOWED;
        }

        if (d3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, mode.Format, FALSE, D3DMULTISAMPLE_2_SAMPLES) >= D3D_OK)
        {
            new_mode.flags |= MULTISAMPLE_2X_FULLSCREEN;
        }

        if (d3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, mode.Format, FALSE, D3DMULTISAMPLE_4_SAMPLES) >= D3D_OK)
        {
            new_mode.flags |= MULTISAMPLE_4X_FULLSCREEN;
        }

        int j;
        for (j = 0; j < total_modes; ++j)
        {
            if (adapter_modes[j].width == mode.Width && adapter_modes[j].height == mode.Height)
            {
                break;
            }
        }

        if (j < total_modes)
        {
            S_adapter_mode &cached_mode = adapter_modes[j];
            cached_mode.flags |= new_mode.flags & MULTISAMPLE_MASK;
            if ((cached_mode.flags & bit_depth_flags) == 0)
            {
                cached_mode.flags |= bit_depth_flags;
            }
        }
        else
        {
            // The multisample flags collected above are lost for a new mode, this is what the original does
            new_mode.width = mode.Width;
            new_mode.height = mode.Height;
            new_mode.flags = bit_depth_flags;

            if (++total_modes >= 256)
            {
                break;
            }
        }
    }

    for (int j = 0; j < total_modes; ++j)
    {
        S_adapter_mode &cached_mode = adapter_modes[j];
        // STRING: SETUP 0x40e158
        sprintf(global_buffer, "%4d x %4d", cached_mode.width, cached_mode.height);
        int pos = SendMessageA(app_state.windows.resolution, CB_ADDSTRING, -1, reinterpret_cast<LPARAM>(global_buffer));
        SendMessageA(app_state.windows.resolution, CB_SETITEMDATA, pos, reinterpret_cast<LPARAM>(&cached_mode));
        if (cached_mode.width == settings.width && cached_mode.height == settings.height)
        {
            SendMessageA(app_state.windows.resolution, CB_SETCURSEL, pos, NULL);
        }
    }
    LoadAdapterRefreshRates();

    return true;
}

// FUNCTION: SETUP 0x402c80
// FUNCTION: SETUPSTEAM 0x402ca0
bool LoadAdapterData(int adapter)
{
    char buffer[128];
    if (LoadStringA(NULL, IDS_OFF, buffer, sizeof(buffer)) == 0)
    {
        // STRING: SETUP 0x40e1a4
        strcpy(buffer, " OFF");
    }

    SendMessageA(app_state.windows.bit_depth, CB_RESETCONTENT, NULL, NULL);
    SendMessageA(app_state.windows.antialiasing, CB_RESETCONTENT, NULL, NULL);
    int pos = SendMessageA(app_state.windows.antialiasing, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(buffer));
    SendMessageA(app_state.windows.antialiasing, CB_SETITEMDATA, pos, NULL);
    if (adapter == -1)
    {
        // Uses the adapter mode count as the bound, not the number of cached modes, this is what the original does
        int adapter_mode_count = d3d8->GetAdapterModeCount(settings.adapter);
        int i;
        for (i = 0; i < adapter_mode_count; ++i)
        {
            if (settings.width == adapter_modes[i].width && settings.height == adapter_modes[i].height)
            {
                break;
            }
        }

        if (i == adapter_mode_count)
        {
            return false;
        }
        adapter = i;
    }

    S_adapter_mode *adapter_data = reinterpret_cast<S_adapter_mode *>(SendMessageA(app_state.windows.resolution, CB_GETITEMDATA, adapter, NULL));
    if (!settings.antialiasing)
    {
        SendMessageA(app_state.windows.antialiasing, CB_SETCURSEL, NULL, NULL);
    }
    else if (!settings.fullscreen)
    {
        if ((adapter_data->flags & MULTISAMPLE_WINDOWED) != MULTISAMPLE_WINDOWED)
        {
            SendMessageA(app_state.windows.antialiasing, CB_SETCURSEL, NULL, NULL);
        }
    }
    else if ((adapter_data->flags & MULTISAMPLE_FULLSCREEN) != MULTISAMPLE_FULLSCREEN)
    {
        SendMessageA(app_state.windows.antialiasing, CB_SETCURSEL, NULL, NULL);
    }

    if (adapter_data->flags & BIT_DEPTH_16)
    {
        // STRING: SETUP 0x40e1a0
        pos = SendMessageA(app_state.windows.bit_depth, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>("16"));
        SendMessageA(app_state.windows.bit_depth, CB_SETITEMDATA, pos, 16);
        if ((adapter_data->flags & BIT_DEPTH_32) == 0 && settings.bit_depth == 32)
        {
            settings.bit_depth = 16;
        }
    }

    if (adapter_data->flags & BIT_DEPTH_32)
    {
        // STRING: SETUP 0x40e19c
        pos = SendMessageA(app_state.windows.bit_depth, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>("32"));
        SendMessageA(app_state.windows.bit_depth, CB_SETITEMDATA, pos, 32);
        if ((adapter_data->flags & BIT_DEPTH_16) == 0 && settings.bit_depth == 16)
        {
            settings.bit_depth = 32;
        }
    }

    if ((adapter_data->flags & (BIT_DEPTH_16 | BIT_DEPTH_32)) == (BIT_DEPTH_16 | BIT_DEPTH_32) && settings.bit_depth != 16)
    {
        SendMessageA(app_state.windows.bit_depth, CB_SETCURSEL, 1, NULL);
    }
    else
    {
        SendMessageA(app_state.windows.bit_depth, CB_SETCURSEL, 0, NULL);
    }

    if (!settings.fullscreen)
    {
        if (adapter_data->flags & MULTISAMPLE_2X_WINDOWED)
        {
            // STRING: SETUP 0x40e198
            pos = SendMessageA(app_state.windows.antialiasing, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(" 2x"));
            SendMessageA(app_state.windows.antialiasing, CB_SETITEMDATA, pos, 2);
            if (settings.antialiasing == 2)
            {
                SendMessageA(app_state.windows.antialiasing, CB_SETCURSEL, pos, NULL);
            }
        }
        if (adapter_data->flags & MULTISAMPLE_4X_WINDOWED)
        {
            // STRING: SETUP 0x40e194
            pos = SendMessageA(app_state.windows.antialiasing, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(" 4x"));
            SendMessageA(app_state.windows.antialiasing, CB_SETITEMDATA, pos, 4);
            if (settings.antialiasing == 4)
            {
                SendMessageA(app_state.windows.antialiasing, CB_SETCURSEL, pos, NULL);
            }
        }
    }
    else
    {
        if (adapter_data->flags & MULTISAMPLE_2X_FULLSCREEN)
        {
            pos = SendMessageA(app_state.windows.antialiasing, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(" 2x"));
            SendMessageA(app_state.windows.antialiasing, CB_SETITEMDATA, pos, 2);
            if (settings.antialiasing == 2)
            {
                SendMessageA(app_state.windows.antialiasing, CB_SETCURSEL, pos, NULL);
            }
        }
        if (adapter_data->flags & MULTISAMPLE_4X_FULLSCREEN)
        {
            pos = SendMessageA(app_state.windows.antialiasing, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(" 4x"));
            SendMessageA(app_state.windows.antialiasing, CB_SETITEMDATA, pos, 4);
            if (settings.antialiasing == 4)
            {
                SendMessageA(app_state.windows.antialiasing, CB_SETCURSEL, pos, NULL);
            }
        }
    }

    return true;
}

// FUNCTION: SETUP 0x402f90
// FUNCTION: SETUPSTEAM 0x402fe0
bool LoadD3D8()
{
    if (!app_state.windows.display)
    {
        d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
        return d3d8 ? true : false;
    }

    SendMessageA(app_state.windows.display, CB_RESETCONTENT, NULL, NULL);
    d3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    if (!d3d8)
    {
        return false;
    }

    int adapter_count = d3d8->GetAdapterCount();
    // Two checks, which VC6 merges into one store: an || gives the same code but other registers
    if (settings.adapter >= adapter_count)
    {
        settings.adapter = 0;
    }
    if (settings.adapter < 0)
    {
        settings.adapter = 0;
    }

    int current_adapter = 0;
    D3DADAPTER_IDENTIFIER8 identifier;
    for (int i = 0; i < adapter_count; ++i)
    {
        if (d3d8->GetAdapterIdentifier(i, 0, &identifier) == D3D_OK)
        {
            int index = SendMessageA(app_state.windows.display, CB_INSERTSTRING, -1, reinterpret_cast<LPARAM>(identifier.Description));
            SendMessageA(app_state.windows.display, CB_SETITEMDATA, index, i);
            if (settings.adapter == i)
            {
                current_adapter = SendMessageA(app_state.windows.display, CB_GETCOUNT, NULL, NULL) - 1;
            }
        }
    }
    SendMessageA(app_state.windows.display, CB_SETCURSEL, current_adapter, NULL);
    LoadAdapterModes();

    D3DCAPS8 caps;
    memset(&caps, 0, sizeof(caps));
    if (d3d8->GetDeviceCaps(settings.adapter, D3DDEVTYPE_HAL, &caps) == D3D_OK)
    {
        supports_w_buffer = (caps.RasterCaps & D3DPRASTERCAPS_WBUFFER) != 0;
    }
    else
    {
        supports_w_buffer = true;
    }

    return true;
}

// Inlined into WinMain in Steam's rebuild
// FUNCTION: SETUP 0x4030f0
void FreeD3D8()
{
    if (d3d8)
    {
        d3d8->Release();
        d3d8 = NULL;
    }
}
