#include "main.h"
#include "hw.h"
#include "settings.h"
#include "res/resource.h"

#include "commctrl.h"

// This file's only globals in 1.2, the start of its .data. The rest are in hw.cpp.
// Initialized to true in the binary (it lives in .data, not .bss), overwritten in WinMain anyway
// GLOBAL: SETUP 0x40e040
bool settings_loaded = true;
// 1.2 reads and writes it as a dword, Steam's rebuild as a byte
#ifdef SETUP_STEAM
bool expert_settings_enabled = true;
#else
// GLOBAL: SETUP 0x40e044
BOOL expert_settings_enabled = TRUE;
#endif

INT_PTR __stdcall DialogFunc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
void Error(const char *msg);

// GetWindowHandles and HandleComboField have no out-of-line copy in 1.2, which /OPT:NOREF would have kept:
// they are inlined into DialogFunc
// FUNCTION: SETUPSTEAM 0x401190
inline void GetWindowHandles(S_state &state, HWND main_window)
{
    state.windows.main = main_window;
    state.windows.display = GetDlgItem(main_window, IDC_COMBO_DISPLAY);
    state.windows.resolution = GetDlgItem(main_window, IDC_COMBO_RESOLUTION);
    state.windows.bit_depth = GetDlgItem(main_window, IDC_COMBO_BITDEPTH);
    state.windows.antialiasing = GetDlgItem(main_window, IDC_COMBO_ANTIALIASING);
    state.windows.driver_version = GetDlgItem(main_window, IDC_DRIVERVERSION);
    state.windows.vram_size = GetDlgItem(main_window, IDC_VRAMSIZE);
    state.windows.refresh_rate = GetDlgItem(main_window, IDC_COMBO_REFRESHRATE);
}

// FUNCTION: SETUP 0x401000
// FUNCTION: SETUPSTEAM 0x401270
bool SetText(HWND window, uint uid)
{
    char buffer[1024];
    if (LoadStringA(NULL, uid, buffer, sizeof(buffer)))
    {
        SetWindowTextA(window, buffer);
        return true;
    }
    return false;
}

// Inlined into DialogFunc in Steam's rebuild
// FUNCTION: SETUP 0x401050
bool SetDialogText(HWND main_window, int dlg, uint uid)
{
    char buffer[1024];
    if (LoadStringA(NULL, uid, buffer, sizeof(buffer)))
    {
        SetWindowTextA(GetDlgItem(main_window, dlg), buffer);
        return true;
    }
    return false;
}

// FUNCTION: SETUP 0x4010b0
// FUNCTION: SETUPSTEAM 0x4012e0
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    app_state.hinst = hInstance;
    InitCommonControls();
    settings_loaded = LoadSettings(settings);
    DialogBoxParamA(hInstance, MAKEINTRESOURCEA(IDD_MAIN), NULL, DialogFunc, NULL);
    FreeD3D8();
    return 0;
}

// FUNCTION: SETUPSTEAM 0x401340
inline void HandleComboField(uint uid, bool &value)
{
    value = SendDlgItemMessageA(app_state.windows.main, uid, BM_GETCHECK, NULL, NULL) == 1;
}

// FUNCTION: SETUP 0x401220
// FUNCTION: SETUPSTEAM 0x401370
void ToggleFields()
{
    if (!settings.fullscreen)
    {
        EnableWindow(GetDlgItem(app_state.windows.main, IDC_CHECK_VSYNC), FALSE);
        EnableWindow(GetDlgItem(app_state.windows.main, IDC_COMBO_REFRESHRATE), FALSE);
        EnableWindow(GetDlgItem(app_state.windows.main, IDC_COMBO_BITDEPTH), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(app_state.windows.main, IDC_CHECK_VSYNC), TRUE);
        EnableWindow(GetDlgItem(app_state.windows.main, IDC_COMBO_REFRESHRATE), TRUE);
        EnableWindow(GetDlgItem(app_state.windows.main, IDC_COMBO_BITDEPTH), TRUE);
    }

    if (settings.disable_sound)
    {
        EnableWindow(GetDlgItem(app_state.windows.main, IDC_COMBO_SOUND), FALSE);
        EnableWindow(GetDlgItem(app_state.windows.main, IDC_CHECK_SOFTWAREMIXINGONLY), FALSE);
        EnableWindow(GetDlgItem(app_state.windows.main, IDC_CHECK_DISABLEEAX), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(app_state.windows.main, IDC_COMBO_SOUND), TRUE);
        EnableWindow(GetDlgItem(app_state.windows.main, IDC_CHECK_SOFTWAREMIXINGONLY), TRUE);
        EnableWindow(GetDlgItem(app_state.windows.main, IDC_CHECK_DISABLEEAX), TRUE);
    }
}

// FUNCTION: SETUP 0x401320
// FUNCTION: SETUPSTEAM 0x401470
void ToggleExpertSettings()
{
    RECT window_rect;
    GetWindowRect(app_state.windows.main, &window_rect);

    // The button's handle goes through a local: 1.2 calls GetDlgItem before pushing the string id, which keeps
    // the two branches from sharing their tails
    if (expert_settings_enabled)
    {
        SetWindowPos(app_state.windows.main, HWND_TOP, window_rect.left, window_rect.top, window_rect.right - window_rect.left, window_rect.bottom - window_rect.top - 75, SWP_SHOWWINDOW);
        expert_settings_enabled = false;
        HWND button = GetDlgItem(app_state.windows.main, IDC_BUTTON_EXPERTSETTINGS);
        SetText(button, IDS_OPENEXPERTSETTINGS);
    }
    else
    {
        SetWindowPos(app_state.windows.main, HWND_TOP, window_rect.left, window_rect.top, window_rect.right - window_rect.left, window_rect.bottom - window_rect.top + 75, SWP_SHOWWINDOW);
        expert_settings_enabled = true;
        HWND button = GetDlgItem(app_state.windows.main, IDC_BUTTON_EXPERTSETTINGS);
        SetText(button, IDS_CLOSEEXPERTSETTINGS);
    }
}

// FUNCTION: SETUP 0x4013e0
// FUNCTION: SETUPSTEAM 0x401550
INT_PTR __stdcall DialogFunc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Msg == WM_INITDIALOG)
    {
        SetText(hWnd, IDS_SETUP);
        SetDialogText(hWnd, IDC_BUTTON_AUTODETECT, IDS_AUTODETECT);
        SetDialogText(hWnd, IDC_BUTTON_EXIT, IDS_EXIT);
        SetDialogText(hWnd, IDC_BUTTON_SAVE, IDS_SAVE);
        SetDialogText(hWnd, IDC_CHECK_HARDWARETRANSFORM, IDS_HARDWARETRANSFORM);
        SetDialogText(hWnd, IDC_CHECK_SUSPENDWHENINACTIVE, IDS_SUSPENDWHENINACTIVE);
        SetDialogText(hWnd, IDC_CHECK_TRIPLEBUFFERING, IDS_TRIPLEBUFFERING);
        SetDialogText(hWnd, IDC_CHECK_VSYNC, IDS_VSYNC);
        SetDialogText(hWnd, IDC_CHECK_MULTIPASSRENDERING, IDS_MULTIPASSRENDERING);
        SetDialogText(hWnd, IDC_CHECK_CLIPALWAYS, IDS_CLIPALWAYS);
        SetDialogText(hWnd, IDC_CHECK_WBUFFER, IDS_WBUFFER);
        SetDialogText(hWnd, IDC_CHECK_TEXTURETRUECOLOR, IDS_TRUECOLOR);
        SetDialogText(hWnd, IDC_CHECK_TEXTURELOWDETAIL, IDS_LOWDETAIL);
        SetDialogText(hWnd, IDC_CHECK_TEXTURECOMPRESSED, IDS_COMPRESSED);
        SetDialogText(hWnd, IDC_CHECK_LIGHTMAPTRUECOLOR, IDS_TRUECOLOR);
        SetDialogText(hWnd, IDC_CHECK_SOFTWAREMIXINGONLY, IDS_SOFTWAREMIXINGONLY);
        SetDialogText(hWnd, IDC_CHECK_DISABLEEAX, IDS_DISABLEEAX);
        SetDialogText(hWnd, IDC_DISPLAY, IDS_DISPLAY);
        SetDialogText(hWnd, IDC_RESOLUTION, IDS_RESOLUTION);
        SetDialogText(hWnd, IDC_ANTIALIASING, IDS_ANTIALIASING);
        SetDialogText(hWnd, IDC_EXPERTSETTINGS, IDS_EXPERTSETTINGS);
        SetDialogText(hWnd, IDC_FULLSCREEN, IDS_FULLSCREEN);
        SetDialogText(hWnd, IDC_REFRESHRATE, IDS_REFRESHRATE);
        SetDialogText(hWnd, IDC_BITDEPTH, IDS_BITDEPTH);
        SetDialogText(hWnd, IDC_TEXTUREQUALITY, IDS_TEXTUREQUALITY);
        SetDialogText(hWnd, IDC_LIGHTMAPQUALITY, IDS_LIGHTMAPQUALITY);
        SetDialogText(hWnd, IDC_SOUND, IDS_SOUND);

        GetWindowHandles(app_state, hWnd);
        ToggleExpertSettings();

        if (!settings_loaded)
        {
            app_state.vram_size = GetTotalVRAM();
            AutodetectSettings();
        }

        if (!LoadD3D8())
        {
            char buffer[256];
            if (LoadStringA(NULL, IDS_SETUPERROR, buffer, sizeof(buffer)) == 0)
            {
                // This loads in different error message than it reads from resources
                strcpy(buffer, "DirectX : Display devices could not be enumerated.");
            }
            Error(buffer);
        }
    }
    else if (Msg == WM_COMMAND)
    {
        WORD id = LOWORD(wParam);
        switch (id)
        {
        case IDC_COMBO_SOUND:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                settings.sound_type = SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_GETCURSEL, NULL, NULL);
                ToggleFields();
            }
            return 0;
        case IDC_COMBO_RESOLUTION:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int pos = SendMessageA(app_state.windows.resolution, CB_GETCURSEL, NULL, NULL);
                S_adapter_mode *adapter_data = reinterpret_cast<S_adapter_mode *>(SendMessageA(app_state.windows.resolution, CB_GETITEMDATA, pos, NULL));
                if (adapter_data && adapter_data != reinterpret_cast<S_adapter_mode *>(CB_ERR))
                {
                    settings.width = adapter_data->width;
                    settings.height = adapter_data->height;
                    LoadAdapterRefreshRates();
                    LoadAdapterData(pos);
                }
            }
            return 0;
        case IDC_COMBO_REFRESHRATE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                uchar pos = static_cast<uchar>(SendMessageA(app_state.windows.refresh_rate, CB_GETCURSEL, NULL, NULL));
                settings.refresh_rate = SendMessageA(app_state.windows.refresh_rate, CB_GETITEMDATA, pos, NULL);
            }
            return 0;
        case IDC_CHECK_FULLSCREEN:
        case IDC_CHECK_TRIPLEBUFFERING:
        case IDC_CHECK_SOUND:
        case IDC_CHECK_TEXTURELOWDETAIL:
        case IDC_CHECK_VSYNC:
        case IDC_CHECK_TEXTURETRUECOLOR:
        case IDC_CHECK_LIGHTMAPTRUECOLOR:
        case IDC_CHECK_TEXTURECOMPRESSED:
        case IDC_CHECK_SUSPENDWHENINACTIVE:
        case IDC_CHECK_HARDWARETRANSFORM:
        case IDC_CHECK_DISABLEEAX:
        case IDC_CHECK_SOFTWAREMIXINGONLY:
        case IDC_CHECK_CLIPALWAYS:
        case IDC_CHECK_MULTIPASSRENDERING:
        case IDC_CHECK_WBUFFER:
            HandleComboField(IDC_CHECK_FULLSCREEN, settings.fullscreen);
            HandleComboField(IDC_CHECK_TRIPLEBUFFERING, settings.triple_buffering);
            HandleComboField(IDC_CHECK_HARDWARETRANSFORM, settings.hardware_transform);
            HandleComboField(IDC_CHECK_TEXTURETRUECOLOR, settings.texture_true_color);
            HandleComboField(IDC_CHECK_TEXTURELOWDETAIL, settings.texture_low_detail);
            HandleComboField(IDC_CHECK_VSYNC, settings.vsync);
            HandleComboField(IDC_CHECK_LIGHTMAPTRUECOLOR, settings.lightmap_true_color);
            HandleComboField(IDC_CHECK_TEXTURECOMPRESSED, settings.texture_compressed);
            HandleComboField(IDC_CHECK_SOUND, settings.disable_sound);
            settings.disable_sound = !settings.disable_sound;
            HandleComboField(IDC_CHECK_SOFTWAREMIXINGONLY, settings.software_mixing_only);
            HandleComboField(IDC_CHECK_DISABLEEAX, settings.disable_eax);
            HandleComboField(IDC_CHECK_SUSPENDWHENINACTIVE, settings.suspend_when_inactive);
            HandleComboField(IDC_CHECK_CLIPALWAYS, settings.clip_always);
            HandleComboField(IDC_CHECK_MULTIPASSRENDERING, settings.multipass_rendering);
            HandleComboField(IDC_CHECK_WBUFFER, settings.w_buffer);
            ToggleFields();
            if (settings.texture_true_color && id == IDC_CHECK_TEXTURETRUECOLOR && app_state.vram_size < 64)
            {
                char buffer[256];
                if (LoadStringA(NULL, IDS_LOWVRAMWARNING, buffer, sizeof(buffer)) == 0)
                {
                    // Loads the wrong string again
                    strcpy(buffer, "DirectX : Display devices could not be enumerated.");
                }
                char caption[128];
                if (LoadStringA(NULL, IDS_WARNING, caption, sizeof(caption)) == 0)
                {
                    strcpy(caption, "Warning");
                }

                if (MessageBoxA(hWnd, buffer, caption, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
                {
                    settings.texture_true_color = false;
                    break;
                }
            }

            // The controls are only refreshed when one of these two fires
            if (settings.hardware_transform && id != IDC_CHECK_MULTIPASSRENDERING)
            {
                settings.multipass_rendering = false;
                break;
            }
            if (settings.multipass_rendering)
            {
                settings.hardware_transform = false;
                break;
            }
            return 0;
        case IDC_COMBO_BITDEPTH:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                uchar pos = static_cast<uchar>(SendMessageA(app_state.windows.bit_depth, CB_GETCURSEL, NULL, NULL));
                settings.bit_depth = SendMessageA(app_state.windows.bit_depth, CB_GETITEMDATA, pos, NULL);
            }
            return 0;
        case IDC_COMBO_DISPLAY:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                uchar pos = static_cast<uchar>(SendMessageA(app_state.windows.display, CB_GETCURSEL, NULL, NULL));
                settings.adapter = SendMessageA(app_state.windows.display, CB_GETITEMDATA, pos, NULL);
                LoadAdapterModes();
            }
            return 0;
        case IDC_BUTTON_AUTODETECT:
            AutodetectSettings();
            // The D3D8 object is created again without releasing the old one, this is what the original does
            if (!LoadD3D8())
            {
                char buffer[256];
                if (LoadStringA(NULL, IDS_SETUPERROR, buffer, sizeof(buffer)) == 0)
                {
                    strcpy(buffer, "DirectX : Display devices could not be enumerated.");
                }
                Error(buffer);
            }
            break;
        case IDC_BUTTON_EXPERTSETTINGS:
            ToggleExpertSettings();
            return 0;
        case IDC_COMBO_ANTIALIASING:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                uchar pos = static_cast<uchar>(SendMessageA(app_state.windows.antialiasing, CB_GETCURSEL, NULL, NULL));
                settings.antialiasing = SendMessageA(app_state.windows.antialiasing, CB_GETITEMDATA, pos, NULL);
            }
            return 0;
        case IDC_COMBO_LANGUAGE:
        {
            int pos = SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_GETCURSEL, NULL, NULL);
            if (pos != CB_ERR)
            {
                settings.language = SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_GETITEMDATA, pos, NULL);
            }
            return 0;
        }
        case IDC_BUTTON_SAVE:
            SaveSettings(settings);
            EndDialog(hWnd, 0);
            return 0;
        case IDC_BUTTON_EXIT:
        {
            S_settings saved_settings;
            LoadSettings(saved_settings);

            if (memcmp(&saved_settings, &settings, sizeof(S_settings)) != 0)
            {
                char buffer[256];
                if (LoadStringA(NULL, IDS_DISCARDCHANGES, buffer, sizeof(buffer)) == 0)
                {
                    strcpy(buffer, "Discard changes ?!");
                }

                char caption[128];
                if (LoadStringA(NULL, IDS_WARNING, caption, sizeof(caption)) == 0)
                {
                    strcpy(caption, "Warning");
                }
                if (MessageBoxA(hWnd, buffer, caption, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
                {
                    return 0;
                }
            }

            EndDialog(hWnd, -1);
            return 0;
        }
        default:
            return 0;
        }
    }
    else
    {
        return 0;
    }

    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_FULLSCREEN, BM_SETCHECK, settings.fullscreen, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_TRIPLEBUFFERING, BM_SETCHECK, settings.triple_buffering, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_HARDWARETRANSFORM, BM_SETCHECK, settings.hardware_transform, NULL);
    if (settings.hardware_transform)
    {
        settings.multipass_rendering = false;
    }
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_TEXTURETRUECOLOR, BM_SETCHECK, settings.texture_true_color, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_TEXTURELOWDETAIL, BM_SETCHECK, settings.texture_low_detail, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_VSYNC, BM_SETCHECK, settings.vsync, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_LIGHTMAPTRUECOLOR, BM_SETCHECK, settings.lightmap_true_color, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_TEXTURECOMPRESSED, BM_SETCHECK, settings.texture_compressed, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_SOUND, BM_SETCHECK, !settings.disable_sound, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_SOFTWAREMIXINGONLY, BM_SETCHECK, settings.software_mixing_only, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_DISABLEEAX, BM_SETCHECK, settings.disable_eax, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_SUSPENDWHENINACTIVE, BM_SETCHECK, settings.suspend_when_inactive, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_CLIPALWAYS, BM_SETCHECK, settings.clip_always, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_MULTIPASSRENDERING, BM_SETCHECK, settings.multipass_rendering, NULL);
    if (!supports_w_buffer)
    {
        settings.w_buffer = false;
    }
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_WBUFFER, BM_SETCHECK, settings.w_buffer, NULL);
    EnableWindow(GetDlgItem(hWnd, IDC_CHECK_WBUFFER), supports_w_buffer);

    SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_RESETCONTENT, NULL, NULL);

    char buffer[256];
    if (LoadStringA(NULL, IDS_STEREOHEADPHONES, buffer, sizeof(buffer)) == 0)
    {
        strcpy(buffer, "Stereo headphones");
    }
    SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(buffer));

    if (LoadStringA(NULL, IDS_STEREOSPEAKERS, buffer, sizeof(buffer)) == 0)
    {
        strcpy(buffer, "Stereo speakers");
    }
    SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(buffer));

    if (LoadStringA(NULL, IDS_QUADROPHONICSPEAKERS, buffer, sizeof(buffer)) == 0)
    {
        strcpy(buffer, "Quadraphonic speakers");
    }
    SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(buffer));

    if (LoadStringA(NULL, IDS_SURROUNDSPEAKERS, buffer, sizeof(buffer)) == 0)
    {
        strcpy(buffer, "5.1 surround sound speakers");
    }
    SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(buffer));
    SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_SETCURSEL, settings.sound_type, NULL);
    LoadAdapterData(-1);
    ToggleFields();

    HWND language = GetDlgItem(hWnd, IDC_COMBO_LANGUAGE);
    SendMessageA(language, CB_RESETCONTENT, NULL, NULL);
    int pos = SendMessageA(language, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>("English"));
    SendMessageA(language, CB_SETITEMDATA, pos, 0);
    // The typo is in the binary
    pos = SendMessageA(language, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>("Deutch"));
    SendMessageA(language, CB_SETITEMDATA, pos, 2);
    pos = SendMessageA(language, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>("Cesky"));
    SendMessageA(language, CB_SETITEMDATA, pos, 1);

    int language_count = SendMessageA(language, CB_GETCOUNT, NULL, NULL);
    for (int i = 0; i < language_count; i++)
    {
        if (SendMessageA(language, CB_GETITEMDATA, i, NULL) == settings.language)
        {
            SendMessageA(language, CB_SETCURSEL, i, NULL);
            break;
        }
    }

    return 0;
}

// FUNCTION: SETUP 0x402550
void Error(const char *msg)
{
    char buffer[128];
    if (LoadStringA(NULL, IDS_SETUPERROR, buffer, sizeof(buffer)) == 0)
    {
        strcpy(buffer, "Setup error");
    }
    MessageBoxA(app_state.windows.main, msg, buffer, MB_OK);
    EndDialog(app_state.windows.main, 0);
}
