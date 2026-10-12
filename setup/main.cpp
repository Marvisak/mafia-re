#include "main.h"
#include "hw.h"
#include "settings.h"
#include "res/resource.h"

#include "commctrl.h"

// This file's only globals in 1.2, the start of its .data. The rest are in hw.cpp.
// Initialized to true in the binary (it lives in .data, not .bss), overwritten in WinMain anyway
// GLOBAL: SETUP 0x40e040
// GLOBAL: SETUPSTEAM 0x405019
bool settings_loaded = true;
// 1.2 reads and writes it as a dword, Steam's rebuild as a byte
#ifdef SETUP_STEAM
// GLOBAL: SETUPSTEAM 0x40505b
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
    // An if/else, not value = ... == BST_CHECKED: with that, 1.2 folds the sound checkbox's negation into the compare
    if (SendDlgItemMessageA(app_state.windows.main, uid, BM_GETCHECK, NULL, NULL) == BST_CHECKED)
    {
        value = true;
    }
    else
    {
        value = false;
    }
}

// FUNCTION: SETUP 0x401220
// FUNCTION: SETUPSTEAM 0x401370
void ToggleFields()
{
    // The handles go through a local: 1.2 calls GetDlgItem before pushing TRUE or FALSE
    HWND window;
    if (settings.fullscreen)
    {
        window = GetDlgItem(app_state.windows.main, IDC_CHECK_VSYNC);
        EnableWindow(window, TRUE);
        window = GetDlgItem(app_state.windows.main, IDC_COMBO_REFRESHRATE);
        EnableWindow(window, TRUE);
        window = GetDlgItem(app_state.windows.main, IDC_COMBO_BITDEPTH);
        EnableWindow(window, TRUE);
    }
    else
    {
        window = GetDlgItem(app_state.windows.main, IDC_CHECK_VSYNC);
        EnableWindow(window, FALSE);
        window = GetDlgItem(app_state.windows.main, IDC_COMBO_REFRESHRATE);
        EnableWindow(window, FALSE);
        window = GetDlgItem(app_state.windows.main, IDC_COMBO_BITDEPTH);
        EnableWindow(window, FALSE);
    }

    if (!settings.disable_sound)
    {
        window = GetDlgItem(app_state.windows.main, IDC_COMBO_SOUND);
        EnableWindow(window, TRUE);
        window = GetDlgItem(app_state.windows.main, IDC_CHECK_SOFTWAREMIXINGONLY);
        EnableWindow(window, TRUE);
        window = GetDlgItem(app_state.windows.main, IDC_CHECK_DISABLEEAX);
        EnableWindow(window, TRUE);
    }
    else
    {
        window = GetDlgItem(app_state.windows.main, IDC_COMBO_SOUND);
        EnableWindow(window, FALSE);
        window = GetDlgItem(app_state.windows.main, IDC_CHECK_SOFTWAREMIXINGONLY);
        EnableWindow(window, FALSE);
        window = GetDlgItem(app_state.windows.main, IDC_CHECK_DISABLEEAX);
        EnableWindow(window, FALSE);
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
    switch (Msg)
    {
    case WM_COMMAND:
        // The cases in Steam's order: VS2008 lays them out in source order, VC6 by the dispatch tree, except for the
        // jump table's targets, which 1.2 has in this order too
        switch (LOWORD(wParam))
        {
        case IDC_BUTTON_EXIT:
        {
            S_settings saved_settings;
            LoadSettings(saved_settings);

            if (memcmp(&saved_settings, &settings, sizeof(S_settings)) != 0)
            {
                char buffer[256];
                if (LoadStringA(NULL, IDS_DISCARDCHANGES, buffer, sizeof(buffer)) == 0)
                {
                    // STRING: SETUP 0x40e124
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
        case IDC_BUTTON_SAVE:
            SaveSettings(settings);
            EndDialog(hWnd, 0);
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
            // Compared with true, not negated with !: both builds compare the flag with 1
            settings.disable_sound = settings.disable_sound != true;
            HandleComboField(IDC_CHECK_SOFTWAREMIXINGONLY, settings.software_mixing_only);
            HandleComboField(IDC_CHECK_DISABLEEAX, settings.disable_eax);
            HandleComboField(IDC_CHECK_SUSPENDWHENINACTIVE, settings.suspend_when_inactive);
            HandleComboField(IDC_CHECK_CLIPALWAYS, settings.clip_always);
            HandleComboField(IDC_CHECK_MULTIPASSRENDERING, settings.multipass_rendering);
            HandleComboField(IDC_CHECK_WBUFFER, settings.w_buffer);
            ToggleFields();
            if (settings.texture_true_color && LOWORD(wParam) == IDC_CHECK_TEXTURETRUECOLOR && app_state.vram_size < 64)
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
                    // STRING: SETUP 0x40e11c
                    strcpy(caption, "Warning");
                }

                if (MessageBoxA(hWnd, buffer, caption, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
                {
                    settings.texture_true_color = false;
                    break;
                }
            }

            // The controls are only refreshed when one of these two fires
            if (settings.hardware_transform && LOWORD(wParam) != IDC_CHECK_MULTIPASSRENDERING)
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
        case IDC_COMBO_SOUND:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                settings.sound_type = SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_GETCURSEL, NULL, NULL);
                ToggleFields();
            }
            return 0;
        case IDC_COMBO_DISPLAY:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                // The selection is cut to a byte into an int: both builds extend it once, a uchar local at its use
                int pos = static_cast<uchar>(SendMessageA(app_state.windows.display, CB_GETCURSEL, NULL, NULL));
                settings.adapter = SendMessageA(app_state.windows.display, CB_GETITEMDATA, pos, NULL);
                LoadAdapterModes();
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
        case IDC_COMBO_ANTIALIASING:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int pos = static_cast<uchar>(SendMessageA(app_state.windows.antialiasing, CB_GETCURSEL, NULL, NULL));
                settings.antialiasing = SendMessageA(app_state.windows.antialiasing, CB_GETITEMDATA, pos, NULL);
            }
            return 0;
        case IDC_COMBO_REFRESHRATE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int pos = static_cast<uchar>(SendMessageA(app_state.windows.refresh_rate, CB_GETCURSEL, NULL, NULL));
                // Cut to a byte as well, this is what the original does
                settings.refresh_rate = static_cast<uchar>(SendMessageA(app_state.windows.refresh_rate, CB_GETITEMDATA, pos, NULL));
            }
            return 0;
        case IDC_COMBO_BITDEPTH:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int pos = static_cast<uchar>(SendMessageA(app_state.windows.bit_depth, CB_GETCURSEL, NULL, NULL));
                settings.bit_depth = SendMessageA(app_state.windows.bit_depth, CB_GETITEMDATA, pos, NULL);
            }
            return 0;
        case IDC_BUTTON_EXPERTSETTINGS:
            ToggleExpertSettings();
            return 0;
        case IDC_BUTTON_AUTODETECT:
            // Both builds have the autodetection once, in WM_INITDIALOG, entered from there and from this button
            goto autodetect;
        default:
            return 0;
        }
        break;
    case WM_INITDIALOG:
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
            app_state.vram_size = GetTotalVRAM(NULL);
        autodetect:
            // Only the best of the best, true beasts can run Mafia with true color textures
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
            if (app_state.vram_size <= 8)
            {
                settings.width = 640;
                settings.height = 480;
                settings.bit_depth = 16;
                settings.lightmap_true_color = false;
                settings.texture_true_color = false;
                settings.texture_low_detail = true;
                settings.texture_compressed = true;
            }
            else
            {
                if (app_state.vram_size <= 16)
                {
                    settings.width = 800;
                    settings.height = 600;
                    settings.bit_depth = 16;
                    settings.texture_true_color = false;
                    settings.texture_low_detail = true;
                    settings.texture_compressed = true;
                }
                else
                {
                    // bit_depth and texture_low_detail in each branch: the compilers merge the stores, but the three
                    // copies make 1.2 keep 32 in a register from the function's start
                    if (app_state.vram_size <= 32)
                    {
                        settings.width = 800;
                        settings.height = 600;
                        settings.texture_true_color = false;
                        settings.texture_compressed = true;
                        settings.bit_depth = 32;
                        settings.texture_low_detail = false;
                    }
                    else if (app_state.vram_size <= 64)
                    {
                        settings.width = 1024;
                        settings.height = 768;
                        settings.texture_true_color = false;
                        settings.texture_compressed = true;
                        settings.bit_depth = 32;
                        settings.texture_low_detail = false;
                    }
                    else
                    {
                        settings.width = 1280;
                        settings.height = 1024;
                        settings.texture_true_color = true;
                        settings.texture_compressed = false;
                        settings.bit_depth = 32;
                        settings.texture_low_detail = false;
                    }
                }
                settings.lightmap_true_color = true;
            }
        }

        if (!LoadD3D8())
        {
            char buffer[256];
            if (LoadStringA(NULL, IDS_SETUPERROR, buffer, sizeof(buffer)) == 0)
            {
                // This loads in different error message than it reads from resources
                // STRING: SETUP 0x40e0e8
                strcpy(buffer, "DirectX : Display devices could not be enumerated.");
            }
            Error(buffer);
        }
        break;
    default:
        return 0;
    }

    // Both builds test each flag (setne): a bool passed as it is would be zero-extended
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_FULLSCREEN, BM_SETCHECK, settings.fullscreen ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_TRIPLEBUFFERING, BM_SETCHECK, settings.triple_buffering ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_HARDWARETRANSFORM, BM_SETCHECK, settings.hardware_transform ? BST_CHECKED : BST_UNCHECKED, NULL);
    if (settings.hardware_transform)
    {
        settings.multipass_rendering = false;
    }
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_TEXTURETRUECOLOR, BM_SETCHECK, settings.texture_true_color ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_TEXTURELOWDETAIL, BM_SETCHECK, settings.texture_low_detail ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_VSYNC, BM_SETCHECK, settings.vsync ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_LIGHTMAPTRUECOLOR, BM_SETCHECK, settings.lightmap_true_color ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_TEXTURECOMPRESSED, BM_SETCHECK, settings.texture_compressed ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_SOUND, BM_SETCHECK, settings.disable_sound ? BST_UNCHECKED : BST_CHECKED, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_SOFTWAREMIXINGONLY, BM_SETCHECK, settings.software_mixing_only ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_DISABLEEAX, BM_SETCHECK, settings.disable_eax ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_SUSPENDWHENINACTIVE, BM_SETCHECK, settings.suspend_when_inactive ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_CLIPALWAYS, BM_SETCHECK, settings.clip_always ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_MULTIPASSRENDERING, BM_SETCHECK, settings.multipass_rendering ? BST_CHECKED : BST_UNCHECKED, NULL);
    if (!supports_w_buffer)
    {
        settings.w_buffer = false;
    }
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_WBUFFER, BM_SETCHECK, settings.w_buffer ? BST_CHECKED : BST_UNCHECKED, NULL);
    EnableWindow(GetDlgItem(hWnd, IDC_CHECK_WBUFFER), supports_w_buffer);

    SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_RESETCONTENT, NULL, NULL);

    // A block of its own: 1.2 gives this buffer the stack slot of the cases' captions, which a buffer in the function's
    // scope can't share
    {
        char buffer[256];
        if (LoadStringA(NULL, IDS_STEREOHEADPHONES, buffer, sizeof(buffer)) == 0)
        {
            // STRING: SETUP 0x40e0d4
            strcpy(buffer, "Stereo headphones");
        }
        SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(buffer));

        if (LoadStringA(NULL, IDS_STEREOSPEAKERS, buffer, sizeof(buffer)) == 0)
        {
            // STRING: SETUP 0x40e0c4
            strcpy(buffer, "Stereo speakers");
        }
        SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(buffer));

        if (LoadStringA(NULL, IDS_QUADROPHONICSPEAKERS, buffer, sizeof(buffer)) == 0)
        {
            // STRING: SETUP 0x40e0ac
            // STRING: SETUPSTEAM 0x4041f8
            strcpy(buffer, "Quadraphonic speakers");
        }
        SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(buffer));

        if (LoadStringA(NULL, IDS_SURROUNDSPEAKERS, buffer, sizeof(buffer)) == 0)
        {
            // STRING: SETUP 0x40e090
            strcpy(buffer, "5.1 surround sound speakers");
        }
        SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>(buffer));
        SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_SETCURSEL, settings.sound_type, NULL);
    }
    LoadAdapterData(-1);
    ToggleFields();

    // GetDlgItem for every message, as both builds call it
    SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_RESETCONTENT, NULL, NULL);
    // STRING: SETUP 0x40e088
    int pos = SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>("English"));
    SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_SETITEMDATA, pos, 0);
    // The typo is in the binary
    // STRING: SETUP 0x40e080
    pos = SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>("Deutch"));
    SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_SETITEMDATA, pos, 2);
    // STRING: SETUP 0x40e078
    pos = SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>("Cesky"));
    SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_SETITEMDATA, pos, 1);

    int language_count = SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_GETCOUNT, NULL, NULL);
    for (int i = 0; i < language_count; i++)
    {
        if (SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_GETITEMDATA, i, NULL) == settings.language)
        {
            SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_SETCURSEL, i, NULL);
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
        // STRING: SETUP 0x40e138
        strcpy(buffer, "Setup error");
    }
    MessageBoxA(app_state.windows.main, msg, buffer, MB_OK);
    EndDialog(app_state.windows.main, 0);
}
