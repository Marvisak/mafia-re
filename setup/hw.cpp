#include "hw.h"
#include "res/resource.h"

#include <ddraw.h>
#include <cstdio>

// A default library, which LINK searches after the CRT's: both builds have the Direct3DCreate8 thunk after the
// CRT's thunks (1.2 with KERNEL32's, Steam's after MSVCR90's), not with the import libraries of the command line
#pragma comment(lib, "d3d8.lib")

// The shapes the comments credit to "the Czech setup" come from Steam's Czech setup, built without optimization
// (main.cpp)

#ifndef SETUP_1_0
// The first initialized data of this file in 1.2. 1.0 has none: its literals start this file's data.
// GLOBAL: SETUP 0x40e144
// GLOBAL: SETUPSTEAM 0x405018
bool supports_w_buffer = true;
#endif

// The original names are unknown. Both compilers order uninitialized globals by a hash of their names (VS2008 puts
// constructed ones after the others), and these names give both builds' .bss order. So do many other sets of names:
// the layout only rules names out.
// GLOBAL: SETUP 0x410d48
// GLOBAL: SETUP10 0x40fde8
// GLOBAL: SETUPSTEAM 0x405fc0
char message_buffer[256];
// dialog_state and settings are constructed at startup: their dynamic initializers start this file's code. VC6 emits
// them where the globals are defined, so defined at the end of main.cpp they would end that file's code instead, in
// the same place: the layout doesn't tell which file had them.
// GLOBAL: SETUP 0x410e48
// GLOBAL: SETUP10 0x40fee8
// GLOBAL: SETUPSTEAM 0x4060c0
S_state dialog_state;
// GLOBAL: SETUP 0x410e70
// GLOBAL: SETUP10 0x40ff10
// GLOBAL: SETUPSTEAM 0x4053b8
IDirect3D8 *direct3d8;
// GLOBAL: SETUP 0x410e74
// GLOBAL: SETUP10 0x40ff14
// GLOBAL: SETUPSTEAM 0x40501c
S_settings settings;
// GLOBAL: SETUP 0x410eb8
// GLOBAL: SETUP10 0x40ff58
// GLOBAL: SETUPSTEAM 0x4053c0
S_adapter_mode adapter_modes[256];

// The numbers are in megabytes, the result is rounded up to a power of two
// Requires DirectDraw7 which was already deprecated at the time
// FUNCTION: SETUP 0x4026f0
// FUNCTION: SETUP10 0x4022e0
// FUNCTION: SETUPSTEAM 0x4026d0
int GetTotalVRAM(GUID *guid)
{
    IDirectDraw7 *dd;
    HRESULT result = DirectDrawCreateEx(guid, (LPVOID *)&dd, IID_IDirectDraw7, NULL);
    if (FAILED(result))
    {
        return 0;
    }
    // Cleared whole and then dwCaps set: 1.2 stores all four dwords, an initializer leaves dwCaps out
    DDSCAPS2 caps;
    memset(&caps, 0, sizeof(caps));
    caps.dwCaps = DDSCAPS_LOCALVIDMEM;

    DWORD vram;
    DWORD available_vram;
    result = dd->GetAvailableVidMem(&caps, &vram, &available_vram);
    if (FAILED(result))
    {
        return 0;
    }
    dd->Release();

    // In place, and an empty for, as in the Czech setup
    vram /= 0x100000;
    int vram_mb;
    for (vram_mb = 1; vram > (uint)vram_mb; vram_mb <<= 1)
    {
    }
    return vram_mb;
}

// FUNCTION: SETUP 0x402780
// FUNCTION: SETUP10 0x402370
// FUNCTION: SETUPSTEAM 0x402750
bool LoadAdapterRefreshRates()
{
#ifdef SETUP_1_0
    if (dialog_state.windows.refresh_rate)
    {
        SendMessageA(dialog_state.windows.refresh_rate, CB_RESETCONTENT, NULL, NULL);
        // STRING: SETUP10 0x40d1f8
        int pos = SendMessageA(dialog_state.windows.refresh_rate, CB_INSERTSTRING, -1, (LPARAM)"Default");
#else
    char buffer[128];
    if (dialog_state.windows.refresh_rate)
    {
        if (LoadStringA(NULL, IDS_DEFAULT, buffer, sizeof(buffer)) == 0)
        {
            // STRING: SETUP 0x40e150
            strcpy(buffer, "Default");
        }
        SendMessageA(dialog_state.windows.refresh_rate, CB_RESETCONTENT, NULL, NULL);
        int pos = SendMessageA(dialog_state.windows.refresh_rate, CB_INSERTSTRING, -1, (LPARAM)buffer);
#endif
        SendMessageA(dialog_state.windows.refresh_rate, CB_SETITEMDATA, pos, NULL);
    }

    if (!direct3d8)
    {
        return false;
    }

    // Early returns, a result local and a continue for each test, as in the Czech setup
    if (!dialog_state.windows.refresh_rate)
    {
        return true;
    }

    int selected = 0;
    int adapter_mode_count = direct3d8->GetAdapterModeCount(settings.adapter);
    for (int i = 0; i < adapter_mode_count; ++i)
    {
        D3DDISPLAYMODE mode;
        HRESULT result = direct3d8->EnumAdapterModes(settings.adapter, i, &mode);
        if (result != D3D_OK)
        {
            continue;
        }

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

        if (mode.Width != settings.width)
        {
            continue;
        }
        if (mode.Height != settings.height)
        {
            continue;
        }
        if (mode.RefreshRate == 0)
        {
            continue;
        }

        // STRING: SETUP 0x40e148
        // STRING: SETUP10 0x40d1f0
        sprintf(message_buffer, "%d Hz", mode.RefreshRate);
        int pos = SendMessageA(dialog_state.windows.refresh_rate, CB_INSERTSTRING, -1, (LPARAM)message_buffer);
        SendMessageA(dialog_state.windows.refresh_rate, CB_SETITEMDATA, pos, mode.RefreshRate);
        if (settings.refresh_rate == mode.RefreshRate)
        {
            selected = pos;
        }
    }

    SendMessageA(dialog_state.windows.refresh_rate, CB_SETCURSEL, selected, NULL);
    if (selected == 0)
    {
        settings.refresh_rate = 0;
    }
    return true;
}

// FUNCTION: SETUP 0x402940
// FUNCTION: SETUP10 0x4024f0
// FUNCTION: SETUPSTEAM 0x402930
bool LoadAdapterModes()
{
    if (dialog_state.windows.resolution)
    {
        SendMessageA(dialog_state.windows.resolution, CB_RESETCONTENT, NULL, NULL);
    }

    if (!direct3d8)
    {
        return false;
    }

#ifndef SETUP_1_0
    char buffer[256];
#endif
    D3DADAPTER_IDENTIFIER8 identifier;
    HRESULT result = direct3d8->GetAdapterIdentifier(settings.adapter, 0, &identifier);
    // The same call in both branches, as in the Czech setup
    if (result == D3D_OK)
    {
        dialog_state.vram_size = GetTotalVRAM(NULL);
    }
    else
    {
        dialog_state.vram_size = GetTotalVRAM(NULL);
    }
    if (dialog_state.windows.driver_version)
    {
        if (result == D3D_OK)
        {
#ifdef SETUP_1_0
            sprintf(
                message_buffer,
                // STRING: SETUP10 0x40d218
                "Driver version: %d.%02d.%02d.%04d",
                HIWORD(identifier.DriverVersion.HighPart),
                LOWORD(identifier.DriverVersion.HighPart),
                HIWORD(identifier.DriverVersion.LowPart),
                LOWORD(identifier.DriverVersion.LowPart));
#else
            if (LoadStringA(NULL, IDS_DRIVERVERSION, buffer, sizeof(buffer)) == 0)
            {
                // STRING: SETUP 0x40e170
                strcpy(buffer, "Driver version: %d.%02d.%02d.%04d");
            }
            sprintf(
                message_buffer,
                buffer,
                HIWORD(identifier.DriverVersion.HighPart),
                LOWORD(identifier.DriverVersion.HighPart),
                HIWORD(identifier.DriverVersion.LowPart),
                LOWORD(identifier.DriverVersion.LowPart));
#endif
        }
        else
        {
            message_buffer[0] = '\0';
        }
        SetWindowTextA(dialog_state.windows.driver_version, message_buffer);
    }
    if (dialog_state.windows.vram_size)
    {
        // STRING: SETUP 0x40e164
        // STRING: SETUP10 0x40d20c
        sprintf(message_buffer, "VRAM: %iMB", dialog_state.vram_size);
        SetWindowTextA(dialog_state.windows.vram_size, message_buffer);
    }

    if (!dialog_state.windows.resolution)
    {
        return true;
    }

    // The flags initialized before the loop, the result locals, the continues and the listing loop reusing new_mode and
    // i, as in the Czech setup
    int adapter_mode_count = direct3d8->GetAdapterModeCount(settings.adapter);
    int total_modes = 0;
    int bit_depth_flags = 0;
    S_adapter_mode *new_mode = adapter_modes;
    int i;
    for (i = 0; i < adapter_mode_count; ++i)
    {
        D3DDISPLAYMODE mode;
        HRESULT result = direct3d8->EnumAdapterModes(settings.adapter, i, &mode);
        if (result != D3D_OK)
        {
            continue;
        }

        switch (mode.Format)
        {
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
            bit_depth_flags = BIT_DEPTH_16;
            break;
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
            bit_depth_flags = BIT_DEPTH_32;
            break;
        default:
            continue;
        }

#ifndef SETUP_1_0
        // The multisample flags are collected directly in the next free slot, even when the mode turns out to be a
        // duplicate. 1.0 checks them only for a new mode, below.
        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, mode.Format, TRUE, D3DMULTISAMPLE_2_SAMPLES) >= D3D_OK)
        {
            new_mode->flags |= MULTISAMPLE_2X_WINDOWED;
        }

        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, mode.Format, TRUE, D3DMULTISAMPLE_4_SAMPLES) >= D3D_OK)
        {
            new_mode->flags |= MULTISAMPLE_4X_WINDOWED;
        }

        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, mode.Format, FALSE, D3DMULTISAMPLE_2_SAMPLES) >= D3D_OK)
        {
            new_mode->flags |= MULTISAMPLE_2X_FULLSCREEN;
        }

        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, mode.Format, FALSE, D3DMULTISAMPLE_4_SAMPLES) >= D3D_OK)
        {
            new_mode->flags |= MULTISAMPLE_4X_FULLSCREEN;
        }
#endif

        // Both loops walk a pointer assigned before them: the builds load adapter_modes ahead of the loop's guard test
        S_adapter_mode *cached_mode = adapter_modes;
        for (int j = 0; j < total_modes; ++j, ++cached_mode)
        {
            if (cached_mode->width != mode.Width)
            {
                continue;
            }
            if (cached_mode->height != mode.Height)
            {
                continue;
            }
#ifndef SETUP_1_0
            cached_mode->flags |= new_mode->flags & MULTISAMPLE_MASK;
#endif
            if ((cached_mode->flags & bit_depth_flags) == 0)
            {
                cached_mode->flags |= bit_depth_flags;
            }
            // A found flag tested after the loop is threaded away into the same jump, but moves the outer loop's
            // counter and bound into other registers and hoists the SendMessageA load out of the listing loop
            goto next_mode;
        }

        // In 1.2 the multisample flags collected above are lost for a new mode, this is what the original does. 1.0
        // keeps those of the first format a resolution comes in.
        new_mode->width = mode.Width;
        new_mode->height = mode.Height;
        new_mode->flags = bit_depth_flags;
#ifdef SETUP_1_0
        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, mode.Format, TRUE, D3DMULTISAMPLE_2_SAMPLES) >= D3D_OK)
        {
            new_mode->flags |= MULTISAMPLE_2X_WINDOWED;
        }

        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, mode.Format, TRUE, D3DMULTISAMPLE_4_SAMPLES) >= D3D_OK)
        {
            new_mode->flags |= MULTISAMPLE_4X_WINDOWED;
        }

        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, mode.Format, FALSE, D3DMULTISAMPLE_2_SAMPLES) >= D3D_OK)
        {
            new_mode->flags |= MULTISAMPLE_2X_FULLSCREEN;
        }

        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, mode.Format, FALSE, D3DMULTISAMPLE_4_SAMPLES) >= D3D_OK)
        {
            new_mode->flags |= MULTISAMPLE_4X_FULLSCREEN;
        }
#endif

        new_mode++;
        total_modes++;
        if (total_modes >= 256)
        {
            break;
        }
    next_mode:;
    }

    new_mode = adapter_modes;
    for (i = 0; i < total_modes; ++i)
    {
        // STRING: SETUP 0x40e158
        // STRING: SETUP10 0x40d200
        sprintf(message_buffer, "%4d x %4d", new_mode->width, new_mode->height);
        int pos = SendMessageA(dialog_state.windows.resolution, CB_ADDSTRING, -1, (LPARAM)message_buffer);
        SendMessageA(dialog_state.windows.resolution, CB_SETITEMDATA, pos, (LPARAM)new_mode);
        if (new_mode->width == settings.width && new_mode->height == settings.height)
        {
            SendMessageA(dialog_state.windows.resolution, CB_SETCURSEL, pos, NULL);
        }
        new_mode++;
    }
    LoadAdapterRefreshRates();

    return true;
}

// FUNCTION: SETUP 0x402c80
// FUNCTION: SETUP10 0x402800
// FUNCTION: SETUPSTEAM 0x402ca0
bool LoadAdapterData(int adapter)
{
#ifndef SETUP_1_0
    char buffer[128];
    if (LoadStringA(NULL, IDS_OFF, buffer, sizeof(buffer)) == 0)
    {
        // STRING: SETUP 0x40e1a4
        strcpy(buffer, " OFF");
    }
#endif

    SendMessageA(dialog_state.windows.bit_depth, CB_RESETCONTENT, NULL, NULL);
    SendMessageA(dialog_state.windows.antialiasing, CB_RESETCONTENT, NULL, NULL);
#ifdef SETUP_1_0
    // STRING: SETUP10 0x40d24c
    int pos = SendMessageA(dialog_state.windows.antialiasing, CB_INSERTSTRING, -1, (LPARAM)" OFF");
#else
    int pos = SendMessageA(dialog_state.windows.antialiasing, CB_INSERTSTRING, -1, (LPARAM)buffer);
#endif
    SendMessageA(dialog_state.windows.antialiasing, CB_SETITEMDATA, pos, NULL);
    if (adapter == -1)
    {
        // Uses the adapter mode count as the bound, not the number of cached modes, this is what the original does
        int adapter_mode_count = direct3d8->GetAdapterModeCount(settings.adapter);
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

    S_adapter_mode *adapter_data = (S_adapter_mode *)SendMessageA(dialog_state.windows.resolution, CB_GETITEMDATA, adapter, NULL);
    if (!settings.antialiasing
        || settings.fullscreen && (adapter_data->flags & MULTISAMPLE_FULLSCREEN) != MULTISAMPLE_FULLSCREEN
        || !settings.fullscreen && (adapter_data->flags & MULTISAMPLE_WINDOWED) != MULTISAMPLE_WINDOWED)
    {
        SendMessageA(dialog_state.windows.antialiasing, CB_SETCURSEL, NULL, NULL);
    }

    if (adapter_data->flags & BIT_DEPTH_16)
    {
        // STRING: SETUP 0x40e1a0
        // STRING: SETUP10 0x40d248
        int pos = SendMessageA(dialog_state.windows.bit_depth, CB_INSERTSTRING, -1, (LPARAM)"16");
        SendMessageA(dialog_state.windows.bit_depth, CB_SETITEMDATA, pos, 16);
        if ((adapter_data->flags & BIT_DEPTH_32) == 0 && settings.bit_depth == 32)
        {
            settings.bit_depth = 16;
        }
    }

    if (adapter_data->flags & BIT_DEPTH_32)
    {
        // STRING: SETUP 0x40e19c
        // STRING: SETUP10 0x40d244
        int pos = SendMessageA(dialog_state.windows.bit_depth, CB_INSERTSTRING, -1, (LPARAM)"32");
        SendMessageA(dialog_state.windows.bit_depth, CB_SETITEMDATA, pos, 32);
        if ((adapter_data->flags & BIT_DEPTH_16) == 0 && settings.bit_depth == 16)
        {
            settings.bit_depth = 32;
        }
    }

    if (settings.bit_depth == 16 || (adapter_data->flags & (BIT_DEPTH_16 | BIT_DEPTH_32)) != (BIT_DEPTH_16 | BIT_DEPTH_32))
    {
        SendMessageA(dialog_state.windows.bit_depth, CB_SETCURSEL, 0, NULL);
    }
    else
    {
        SendMessageA(dialog_state.windows.bit_depth, CB_SETCURSEL, 1, NULL);
    }

    if (settings.fullscreen)
    {
        if (adapter_data->flags & MULTISAMPLE_2X_FULLSCREEN)
        {
            int pos = SendMessageA(dialog_state.windows.antialiasing, CB_INSERTSTRING, -1, (LPARAM)" 2x");
            SendMessageA(dialog_state.windows.antialiasing, CB_SETITEMDATA, pos, 2);
            if (settings.antialiasing == 2)
            {
                SendMessageA(dialog_state.windows.antialiasing, CB_SETCURSEL, pos, NULL);
            }
        }
        if (adapter_data->flags & MULTISAMPLE_4X_FULLSCREEN)
        {
            int pos = SendMessageA(dialog_state.windows.antialiasing, CB_INSERTSTRING, -1, (LPARAM)" 4x");
            SendMessageA(dialog_state.windows.antialiasing, CB_SETITEMDATA, pos, 4);
            if (settings.antialiasing == 4)
            {
                SendMessageA(dialog_state.windows.antialiasing, CB_SETCURSEL, pos, NULL);
            }
        }
    }
    else
    {
        if (adapter_data->flags & MULTISAMPLE_2X_WINDOWED)
        {
            // STRING: SETUP 0x40e198
            // STRING: SETUP10 0x40d240
            int pos = SendMessageA(dialog_state.windows.antialiasing, CB_INSERTSTRING, -1, (LPARAM)" 2x");
            SendMessageA(dialog_state.windows.antialiasing, CB_SETITEMDATA, pos, 2);
            if (settings.antialiasing == 2)
            {
                SendMessageA(dialog_state.windows.antialiasing, CB_SETCURSEL, pos, NULL);
            }
        }
        if (adapter_data->flags & MULTISAMPLE_4X_WINDOWED)
        {
            // STRING: SETUP 0x40e194
            // STRING: SETUP10 0x40d23c
            int pos = SendMessageA(dialog_state.windows.antialiasing, CB_INSERTSTRING, -1, (LPARAM)" 4x");
            SendMessageA(dialog_state.windows.antialiasing, CB_SETITEMDATA, pos, 4);
            if (settings.antialiasing == 4)
            {
                SendMessageA(dialog_state.windows.antialiasing, CB_SETCURSEL, pos, NULL);
            }
        }
    }

    return true;
}

// FUNCTION: SETUP 0x402f90
// FUNCTION: SETUP10 0x402ac0
// FUNCTION: SETUPSTEAM 0x402fe0
bool LoadD3D8()
{
    if (!dialog_state.windows.display)
    {
        direct3d8 = Direct3DCreate8(D3D_SDK_VERSION);
        if (!direct3d8)
        {
            return false;
        }
        return true;
    }

    SendMessageA(dialog_state.windows.display, CB_RESETCONTENT, NULL, NULL);
    direct3d8 = Direct3DCreate8(D3D_SDK_VERSION);
    if (!direct3d8)
    {
        return false;
    }

    int adapter_count = direct3d8->GetAdapterCount();
    // Two checks, which VC6 merges into one store: an || gives the same code but other registers, in both builds (the
    // locals' declaration order and place change nothing)
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
        HRESULT result = direct3d8->GetAdapterIdentifier(i, 0, &identifier);
        if (result != D3D_OK)
        {
            continue;
        }

        int index = SendMessageA(dialog_state.windows.display, CB_INSERTSTRING, -1, (LPARAM)identifier.Description);
        SendMessageA(dialog_state.windows.display, CB_SETITEMDATA, index, i);
        if (settings.adapter == i)
        {
            current_adapter = SendMessageA(dialog_state.windows.display, CB_GETCOUNT, NULL, NULL) - 1;
        }
    }
    SendMessageA(dialog_state.windows.display, CB_SETCURSEL, current_adapter, NULL);
    LoadAdapterModes();

#ifndef SETUP_1_0
    D3DCAPS8 caps;
    memset(&caps, 0, sizeof(caps));
    if (direct3d8->GetDeviceCaps(settings.adapter, D3DDEVTYPE_HAL, &caps) == D3D_OK)
    {
        supports_w_buffer = (caps.RasterCaps & D3DPRASTERCAPS_WBUFFER) != 0;
    }
    else
    {
        supports_w_buffer = true;
    }
#endif

    return true;
}

// Inlined into WinMain in Steam's rebuild
// FUNCTION: SETUP 0x4030f0
// FUNCTION: SETUP10 0x402bd0
void FreeD3D8()
{
    if (direct3d8)
    {
        direct3d8->Release();
        direct3d8 = NULL;
    }
}
