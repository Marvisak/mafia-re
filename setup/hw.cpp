#include "hw.h"
#include "res/resource.h"

#include <ddraw.h>
#include <cstdio>

// A default library, which LINK searches after the CRT's: both builds have the Direct3DCreate8 thunk after the
// CRT's thunks (1.2 with KERNEL32's, Steam's after MSVCR90's), not with the import libraries of the command line
#pragma comment(lib, "d3d8.lib")

// The shapes the comments credit to "the Czech setup" come from Steam's Czech setup, built without optimization
// (main.cpp)

#if SETUP_DATE >= SETUP_DATE_1_2
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
// GLOBAL: SETUPCZ 0x410d48
// GLOBAL: SETUPPL 0x410d48
// GLOBAL: SETUPSTEAM 0x405fc0
char message_buffer[256];
// dialog_state and settings are constructed at startup: their dynamic initializers start this file's code. VC6 emits
// them where the globals are defined, so defined at the end of main.cpp they would end that file's code instead, in
// the same place: the layout doesn't tell which file had them.
// GLOBAL: SETUP 0x410e48
// GLOBAL: SETUP10 0x40fee8
// GLOBAL: SETUPCZ 0x410e48
// GLOBAL: SETUPPL 0x410e48
// GLOBAL: SETUPSTEAM 0x4060c0
S_state dialog_state;
// GLOBAL: SETUP 0x410e70
// GLOBAL: SETUP10 0x40ff10
// GLOBAL: SETUPCZ 0x410e70
// GLOBAL: SETUPPL 0x410e70
// GLOBAL: SETUPSTEAM 0x4053b8
IDirect3D8 *direct3d8;
// GLOBAL: SETUP 0x410e74
// GLOBAL: SETUP10 0x40ff14
// GLOBAL: SETUPCZ 0x410e78
// GLOBAL: SETUPPL 0x410e74
// GLOBAL: SETUPSTEAM 0x40501c
S_settings settings;
// GLOBAL: SETUP 0x410eb8
// GLOBAL: SETUP10 0x40ff58
// GLOBAL: SETUPCZ 0x410eb8
// GLOBAL: SETUPPL 0x410eb8
// GLOBAL: SETUPSTEAM 0x4053c0
S_adapter_mode adapter_modes[256];

// The numbers are in megabytes, the result is rounded up to a power of two
// Requires DirectDraw7 which was already deprecated at the time
// FUNCTION: SETUP 0x4026f0
// FUNCTION: SETUP10 0x4022e0
// FUNCTION: SETUPCZ 0x402e00
// FUNCTION: SETUPPL 0x4026c0
// FUNCTION: SETUPSTEAM 0x4026d0
int GetTotalVRAM(GUID *guid)
{
    IDirectDraw7 *dd;
    HRESULT rc = DirectDrawCreateEx(guid, (LPVOID *)&dd, IID_IDirectDraw7, NULL);
    if (FAILED(rc))
    {
        return 0;
    }
    // Cleared whole and then dwCaps set: 1.2 stores all four dwords, an initializer leaves dwCaps out
    DDSCAPS2 caps;
    memset(&caps, 0, sizeof(caps));
    caps.dwCaps = DDSCAPS_LOCALVIDMEM;

    DWORD vram;
    DWORD available_vram;
    rc = dd->GetAvailableVidMem(&caps, &vram, &available_vram);
    if (FAILED(rc))
    {
        return 0;
    }
    dd->Release();

    // In place, and an empty for, as in the Czech setup
    vram /= 0x100000;
    int size;
    for (size = 1; vram > (uint)size; size <<= 1)
    {
    }
    return size;
}

// FUNCTION: SETUP 0x402780
// FUNCTION: SETUP10 0x402370
// FUNCTION: SETUPCZ 0x402ea0
// FUNCTION: SETUPPL 0x402750
// FUNCTION: SETUPSTEAM 0x402750
bool LoadAdapterRefreshRates()
{
#if SETUP_DATE < SETUP_DATE_CZECH
    if (dialog_state.windows.refresh_rate)
    {
        SendMessageA(dialog_state.windows.refresh_rate, CB_RESETCONTENT, NULL, NULL);
        // STRING: SETUP10 0x40d1f8
        int item = SendMessageA(dialog_state.windows.refresh_rate, CB_INSERTSTRING, -1, (LPARAM)"Default");
#else
    if (dialog_state.windows.refresh_rate)
    {
        char buffer[128];
        if (LoadStringA(NULL, IDS_DEFAULT, buffer, sizeof(buffer)) == 0)
        {
            // STRING: SETUP 0x40e150
            // STRING: SETUPCZ 0x40e14c
            // STRING: SETUPPL 0x40e14c
            strcpy(buffer, "Default");
        }
        SendMessageA(dialog_state.windows.refresh_rate, CB_RESETCONTENT, NULL, NULL);
        int item = SendMessageA(dialog_state.windows.refresh_rate, CB_INSERTSTRING, -1, (LPARAM)buffer);
#endif
        SendMessageA(dialog_state.windows.refresh_rate, CB_SETITEMDATA, item, NULL);
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
    int number_of_modes = direct3d8->GetAdapterModeCount(settings.adapter);
    D3DDISPLAYMODE display_mode;
    for (int i = 0; i < number_of_modes; ++i)
    {
        HRESULT result = direct3d8->EnumAdapterModes(settings.adapter, i, &display_mode);
        if (result != D3D_OK)
        {
            continue;
        }

        switch (display_mode.Format)
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

        if (display_mode.Width != settings.width)
        {
            continue;
        }
        if (display_mode.Height != settings.height)
        {
            continue;
        }
        if (display_mode.RefreshRate == 0)
        {
            continue;
        }

        // STRING: SETUP 0x40e148
        // STRING: SETUP10 0x40d1f0
        // STRING: SETUPCZ 0x40e144
        // STRING: SETUPPL 0x40e144
        sprintf(message_buffer, "%d Hz", display_mode.RefreshRate);
        int pos = SendMessageA(dialog_state.windows.refresh_rate, CB_INSERTSTRING, -1, (LPARAM)message_buffer);
        SendMessageA(dialog_state.windows.refresh_rate, CB_SETITEMDATA, pos, display_mode.RefreshRate);
        if (settings.refresh_rate == display_mode.RefreshRate)
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
// FUNCTION: SETUPCZ 0x403100
// FUNCTION: SETUPPL 0x402910
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

    D3DADAPTER_IDENTIFIER8 adapter_info;
    HRESULT result = direct3d8->GetAdapterIdentifier(settings.adapter, 0, &adapter_info);
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
#if SETUP_DATE < SETUP_DATE_CZECH
            sprintf(
                message_buffer,
                // STRING: SETUP10 0x40d218
                "Driver version: %d.%02d.%02d.%04d",
                HIWORD(adapter_info.DriverVersion.HighPart),
                LOWORD(adapter_info.DriverVersion.HighPart),
                HIWORD(adapter_info.DriverVersion.LowPart),
                LOWORD(adapter_info.DriverVersion.LowPart));
#else
            char buffer[256];
            if (LoadStringA(NULL, IDS_DRIVERVERSION, buffer, sizeof(buffer)) == 0)
            {
                // STRING: SETUP 0x40e170
                // STRING: SETUPCZ 0x40e16c
                // STRING: SETUPPL 0x40e16c
                strcpy(buffer, "Driver version: %d.%02d.%02d.%04d");
            }
            sprintf(
                message_buffer,
                buffer,
                HIWORD(adapter_info.DriverVersion.HighPart),
                LOWORD(adapter_info.DriverVersion.HighPart),
                HIWORD(adapter_info.DriverVersion.LowPart),
                LOWORD(adapter_info.DriverVersion.LowPart));
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
        // STRING: SETUPCZ 0x40e160
        // STRING: SETUPPL 0x40e160
        sprintf(message_buffer, "VRAM: %iMB", dialog_state.vram_size);
        SetWindowTextA(dialog_state.windows.vram_size, message_buffer);
    }

    if (!dialog_state.windows.resolution)
    {
        return true;
    }

    // The flags initialized before the loop, the result locals, the continues and the listing loop reusing new_entry and
    // index, as in the Czech setup
    int number_of_modes = direct3d8->GetAdapterModeCount(settings.adapter);
    int cached_modes = 0;
    int bit_depth_flags = 0;
    S_adapter_mode *new_entry = adapter_modes;
    D3DDISPLAYMODE display_mode;
    int index;
    for (index = 0; index < number_of_modes; ++index)
    {
        HRESULT result = direct3d8->EnumAdapterModes(settings.adapter, index, &display_mode);
        if (result != D3D_OK)
        {
            continue;
        }

        switch (display_mode.Format)
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

#if SETUP_DATE >= SETUP_DATE_1_2
        // The multisample flags are collected directly in the next free slot, even when the display_mode turns out to be a
        // duplicate. 1.0 checks them only for a new display_mode, below.
        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, display_mode.Format, TRUE, D3DMULTISAMPLE_2_SAMPLES) >= D3D_OK)
        {
            new_entry->flags |= MULTISAMPLE_2X_WINDOWED;
        }

        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, display_mode.Format, TRUE, D3DMULTISAMPLE_4_SAMPLES) >= D3D_OK)
        {
            new_entry->flags |= MULTISAMPLE_4X_WINDOWED;
        }

        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, display_mode.Format, FALSE, D3DMULTISAMPLE_2_SAMPLES) >= D3D_OK)
        {
            new_entry->flags |= MULTISAMPLE_2X_FULLSCREEN;
        }

        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, display_mode.Format, FALSE, D3DMULTISAMPLE_4_SAMPLES) >= D3D_OK)
        {
            new_entry->flags |= MULTISAMPLE_4X_FULLSCREEN;
        }
#endif

        // Both loops walk a pointer assigned before them: the builds load adapter_modes ahead of the loop's guard test
        S_adapter_mode *cached_mode = adapter_modes;
        for (int j = 0; j < cached_modes; ++j, ++cached_mode)
        {
            if (cached_mode->width != display_mode.Width)
            {
                continue;
            }
            if (cached_mode->height != display_mode.Height)
            {
                continue;
            }
#if SETUP_DATE >= SETUP_DATE_1_2
            cached_mode->flags |= new_entry->flags & MULTISAMPLE_MASK;
#endif
            if ((cached_mode->flags & bit_depth_flags) == 0)
            {
                cached_mode->flags |= bit_depth_flags;
            }
            // A found flag tested after the loop is threaded away into the same jump, but moves the outer loop's
            // counter and bound into other registers and hoists the SendMessageA load out of the listing loop
            goto next_mode;
        }

        // In 1.2 the multisample flags collected above are lost for a new display_mode, this is what the original does. 1.0
        // keeps those of the first format a resolution comes in.
        new_entry->width = display_mode.Width;
        new_entry->height = display_mode.Height;
        new_entry->flags = bit_depth_flags;
#if SETUP_DATE < SETUP_DATE_1_2
        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, display_mode.Format, TRUE, D3DMULTISAMPLE_2_SAMPLES) >= D3D_OK)
        {
            new_entry->flags |= MULTISAMPLE_2X_WINDOWED;
        }

        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, display_mode.Format, TRUE, D3DMULTISAMPLE_4_SAMPLES) >= D3D_OK)
        {
            new_entry->flags |= MULTISAMPLE_4X_WINDOWED;
        }

        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, display_mode.Format, FALSE, D3DMULTISAMPLE_2_SAMPLES) >= D3D_OK)
        {
            new_entry->flags |= MULTISAMPLE_2X_FULLSCREEN;
        }

        if (direct3d8->CheckDeviceMultiSampleType(settings.adapter, D3DDEVTYPE_HAL, display_mode.Format, FALSE, D3DMULTISAMPLE_4_SAMPLES) >= D3D_OK)
        {
            new_entry->flags |= MULTISAMPLE_4X_FULLSCREEN;
        }
#endif

        new_entry++;
        cached_modes++;
        if (cached_modes >= 256)
        {
            break;
        }
    next_mode:;
    }

    new_entry = adapter_modes;
    for (index = 0; index < cached_modes; ++index)
    {
        // STRING: SETUP 0x40e158
        // STRING: SETUP10 0x40d200
        // STRING: SETUPCZ 0x40e154
        // STRING: SETUPPL 0x40e154
        sprintf(message_buffer, "%4d x %4d", new_entry->width, new_entry->height);
        int pos = SendMessageA(dialog_state.windows.resolution, CB_ADDSTRING, -1, (LPARAM)message_buffer);
        SendMessageA(dialog_state.windows.resolution, CB_SETITEMDATA, pos, (LPARAM)new_entry);
        if (new_entry->width == settings.width && new_entry->height == settings.height)
        {
            SendMessageA(dialog_state.windows.resolution, CB_SETCURSEL, pos, NULL);
        }
        new_entry++;
    }
    LoadAdapterRefreshRates();

    return true;
}

// FUNCTION: SETUP 0x402c80
// FUNCTION: SETUP10 0x402800
// FUNCTION: SETUPCZ 0x4035f0
// FUNCTION: SETUPPL 0x402c50
// FUNCTION: SETUPSTEAM 0x402ca0
bool LoadAdapterData(int adapter)
{
#if SETUP_DATE >= SETUP_DATE_CZECH
    char buffer[128];
    if (LoadStringA(NULL, IDS_OFF, buffer, sizeof(buffer)) == 0)
    {
        // STRING: SETUP 0x40e1a4
        // STRING: SETUPCZ 0x40e1a0
        // STRING: SETUPPL 0x40e1a0
        strcpy(buffer, " OFF");
    }
#endif

    SendMessageA(dialog_state.windows.bit_depth, CB_RESETCONTENT, NULL, NULL);
    SendMessageA(dialog_state.windows.antialiasing, CB_RESETCONTENT, NULL, NULL);
#if SETUP_DATE < SETUP_DATE_CZECH
    // STRING: SETUP10 0x40d24c
    int sel = SendMessageA(dialog_state.windows.antialiasing, CB_INSERTSTRING, -1, (LPARAM)" OFF");
#else
    int sel = SendMessageA(dialog_state.windows.antialiasing, CB_INSERTSTRING, -1, (LPARAM)buffer);
#endif
    SendMessageA(dialog_state.windows.antialiasing, CB_SETITEMDATA, sel, NULL);
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
        // STRING: SETUPCZ 0x40e19c
        // STRING: SETUPPL 0x40e19c
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
        // STRING: SETUPCZ 0x40e198
        // STRING: SETUPPL 0x40e198
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
            // STRING: SETUPCZ 0x40e194
            // STRING: SETUPPL 0x40e194
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
            // STRING: SETUPCZ 0x40e190
            // STRING: SETUPPL 0x40e190
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
// FUNCTION: SETUPCZ 0x403ac0
// FUNCTION: SETUPPL 0x402f60
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

    int adapters = direct3d8->GetAdapterCount();
    // Two checks, which VC6 merges into one store: an || gives the same code but other registers, in both builds (the
    // locals' declaration order and place change nothing)
    if (settings.adapter >= adapters)
    {
        settings.adapter = 0;
    }
    if (settings.adapter < 0)
    {
        settings.adapter = 0;
    }

    int current_adapter = 0;
    for (int i = 0; i < adapters; ++i)
    {
        D3DADAPTER_IDENTIFIER8 identifier;
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

#if SETUP_DATE >= SETUP_DATE_1_2
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
// FUNCTION: SETUPCZ 0x403c40
// FUNCTION: SETUPPL 0x403070
void FreeD3D8()
{
    if (direct3d8)
    {
        direct3d8->Release();
        direct3d8 = NULL;
    }
}
