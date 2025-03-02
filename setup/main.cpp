#include "main.h"
#include "hw.h"
#include "settings.h"
#include "res/resource.h"

#include "commctrl.h"

S_state app_state;
bool expert_settings_enabled = true;

bool SetText(uint uid, HWND window)
{
    char buffer[1024];
    if (LoadStringA(NULL, uid, buffer, sizeof(buffer)) == 0)
    {
        return false;
    }
    SetWindowTextA(window, buffer);
    return true;
}

bool SetDialogText(HWND main_window, uint uid, int dlg)
{
    char buffer[1024];
    if (LoadStringA(NULL, uid, buffer, sizeof(buffer)) == 0)
    {
        return false;
    }
    HWND window = GetDlgItem(main_window, dlg);
    SetWindowTextA(window, buffer);
    return true;
}

void Error(const char *msg)
{
    char buffer[128];
    if (LoadStringA(NULL, IDS_SETUPERROR, buffer, sizeof(buffer)))
    {
        strcpy(buffer, "Setup error");
    }
    MessageBoxA(app_state.windows.main, msg, buffer, MB_OK);
    EndDialog(app_state.windows.main, 0);
}

void GetWindowHandles(S_state &state, HWND main_window)
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

void HandleComboField(uint uid, bool &value)
{
    value = SendDlgItemMessage(app_state.windows.main, uid, BM_GETCHECK, NULL, NULL) == 1;
}

void ToggleExpertSettings()
{
    RECT window_rect;
    GetWindowRect(app_state.windows.main, &window_rect);

    LONG height = window_rect.bottom - window_rect.top;
    if (expert_settings_enabled)
    {
        SetWindowPos(app_state.windows.main, HWND_TOP, window_rect.left, window_rect.top, window_rect.right - window_rect.left, height - 75, SWP_SHOWWINDOW);
        expert_settings_enabled = false;
        HWND dlg = GetDlgItem(app_state.windows.main, IDC_BUTTON_EXPERTSETTINGS);
        SetText(IDS_OPENEXPERTSETTINGS, dlg);
    }
    else
    {
        SetWindowPos(app_state.windows.main, HWND_TOP, window_rect.left, window_rect.top, window_rect.right - window_rect.left, height + 75, SWP_SHOWWINDOW);
        expert_settings_enabled = true;
        HWND dlg = GetDlgItem(app_state.windows.main, IDC_BUTTON_EXPERTSETTINGS);
        SetText(IDS_CLOSEEXPERTSETTINGS, dlg);
    }
}

void ToggleFields()
{
    if (app_state.settings.fullscreen)
    {
        HWND vsync = GetDlgItem(app_state.windows.main, IDC_CHECK_VSYNC);
        EnableWindow(vsync, true);
        HWND refresh_rate = GetDlgItem(app_state.windows.main, IDC_COMBO_REFRESHRATE);
        EnableWindow(refresh_rate, true);
        HWND bit_depth = GetDlgItem(app_state.windows.main, IDC_COMBO_BITDEPTH);
        EnableWindow(bit_depth, true);
    }
    else
    {
        HWND vsync = GetDlgItem(app_state.windows.main, IDC_CHECK_VSYNC);
        EnableWindow(vsync, false);
        HWND refresh_rate = GetDlgItem(app_state.windows.main, IDC_COMBO_REFRESHRATE);
        EnableWindow(refresh_rate, false);
        HWND bit_depth = GetDlgItem(app_state.windows.main, IDC_COMBO_BITDEPTH);
        EnableWindow(bit_depth, false);
    }

    if (app_state.settings.disable_sound)
    {
        HWND sound = GetDlgItem(app_state.windows.main, IDC_COMBO_SOUND);
        EnableWindow(sound, false);
        HWND software_mixing_only = GetDlgItem(app_state.windows.main, IDC_CHECK_SOFTWAREMIXINGONLY);
        EnableWindow(software_mixing_only, false);
        HWND disable_eax = GetDlgItem(app_state.windows.main, IDC_CHECK_DISABLEEAX);
        EnableWindow(disable_eax, false);
    }
    else
    {
        HWND sound = GetDlgItem(app_state.windows.main, IDC_COMBO_SOUND);
        EnableWindow(sound, true);
        HWND software_mixing_only = GetDlgItem(app_state.windows.main, IDC_CHECK_SOFTWAREMIXINGONLY);
        EnableWindow(software_mixing_only, true);
        HWND disable_eax = GetDlgItem(app_state.windows.main, IDC_CHECK_DISABLEEAX);
        EnableWindow(disable_eax, true);
    }
}

INT_PTR __stdcall DialogFunc(HWND hWnd, UINT Msg, WPARAM Param1, LPARAM Param2)
{
    if (Msg == WM_INITDIALOG)
    {
        SetText(IDS_SETUP, hWnd);
        SetDialogText(hWnd, IDS_AUTODETECT, IDC_BUTTON_AUTODETECT);
        SetDialogText(hWnd, IDS_EXIT, IDC_BUTTON_EXIT);
        SetDialogText(hWnd, IDS_SAVE, IDC_BUTTON_SAVE);
        SetDialogText(hWnd, IDS_HARDWARETRANSFORM, IDC_CHECK_HARDWARETRANSFORM);
        SetDialogText(hWnd, IDS_SUSPENDWHENINACTIVE, IDC_CHECK_SUSPENDWHENINACTIVE);
        SetDialogText(hWnd, IDS_TRIPLEBUFFERING, IDC_CHECK_TRIPLEBUFFERING);
        SetDialogText(hWnd, IDS_VSYNC, IDC_CHECK_VSYNC);
        SetDialogText(hWnd, IDS_MULTIPASSRENDERING, IDC_CHECK_MULTIPASSRENDERING);
        SetDialogText(hWnd, IDS_CLIPALWAYS, IDC_CHECK_CLIPALWAYS);
        SetDialogText(hWnd, IDS_WBUFFER, IDC_CHECK_WBUFFER);
        SetDialogText(hWnd, IDS_TRUECOLOR, IDC_CHECK_TEXTURETRUECOLOR);
        SetDialogText(hWnd, IDS_LOWDETAIL, IDC_CHECK_TEXTURELOWDETAIL);
        SetDialogText(hWnd, IDS_COMPRESSED, IDC_CHECK_TEXTURECOMPRESSED);
        SetDialogText(hWnd, IDS_TRUECOLOR, IDC_CHECK_LIGHTMAPTRUECOLOR);
        SetDialogText(hWnd, IDS_SOFTWAREMIXINGONLY, IDC_CHECK_SOFTWAREMIXINGONLY);
        SetDialogText(hWnd, IDS_DISABLEEAX, IDC_CHECK_DISABLEEAX);
        SetDialogText(hWnd, IDS_DISPLAY, IDC_DISPLAY);
        SetDialogText(hWnd, IDS_RESOLUTION, IDC_RESOLUTION);
        SetDialogText(hWnd, IDS_ANTIALIASING, IDC_ANTIALIASING);
        SetDialogText(hWnd, IDS_EXPERTSETTINGS, IDC_EXPERTSETTINGS);
        SetDialogText(hWnd, IDS_FULLSCREEN, IDC_FULLSCREEN);
        SetDialogText(hWnd, IDS_REFRESHRATE, IDC_REFRESHRATE);
        SetDialogText(hWnd, IDS_BITDEPTH, IDC_BITDEPTH);
        SetDialogText(hWnd, IDS_TEXTUREQUALITY, IDC_TEXTUREQUALITY);
        SetDialogText(hWnd, IDS_LIGHTMAPQUALITY, IDC_LIGHTMAPQUALITY);
        SetDialogText(hWnd, IDS_SOUND, IDC_SOUND);

        GetWindowHandles(app_state, hWnd);
        ToggleExpertSettings();

        if (!app_state.settings_loaded)
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
        switch (Param1 & 0xFFFF)
        {
        case IDC_COMBO_SOUND:
            if ((Param1 >> 16) == 1)
            {
                app_state.settings.sound_type = SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_GETCURSEL, NULL, NULL);
                ToggleFields();
            }
            return 0;
        case IDC_COMBO_RESOLUTION:
            if ((Param1 >> 16) == 1)
            {
                int pos = SendMessageA(app_state.windows.resolution, CB_GETCURSEL, NULL, NULL);
                S_adapter_mode *adapter_data = reinterpret_cast<S_adapter_mode *>(SendMessageA(app_state.windows.resolution, CB_GETITEMDATA, pos, NULL));
                if (adapter_data)
                {
                    app_state.settings.width = adapter_data->width;
                    app_state.settings.height = adapter_data->height;
                    LoadAdapterRefreshRates();
                    LoadAdapterData(pos);
                }
            }
            return 0;
        case IDC_COMBO_REFRESHRATE:
            if ((Param1 >> 16) == 1)
            {
                int pos = SendMessageA(app_state.windows.refresh_rate, CB_GETCURSEL, NULL, NULL);
                app_state.settings.refresh_rate = SendMessageA(app_state.windows.refresh_rate, CB_GETITEMDATA, pos, NULL);
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
            HandleComboField(IDC_CHECK_FULLSCREEN, app_state.settings.fullscreen);
            HandleComboField(IDC_CHECK_TRIPLEBUFFERING, app_state.settings.triple_buffering);
            HandleComboField(IDC_CHECK_HARDWARETRANSFORM, app_state.settings.hardware_transform);
            HandleComboField(IDC_CHECK_TEXTURETRUECOLOR, app_state.settings.texture_true_color);
            HandleComboField(IDC_CHECK_TEXTURELOWDETAIL, app_state.settings.texture_low_detail);
            HandleComboField(IDC_CHECK_VSYNC, app_state.settings.vsync);
            HandleComboField(IDC_CHECK_LIGHTMAPTRUECOLOR, app_state.settings.lightmap_true_color);
            HandleComboField(IDC_CHECK_TEXTURECOMPRESSED, app_state.settings.texture_compressed);
            HandleComboField(IDC_CHECK_SOUND, app_state.settings.disable_sound);
            app_state.settings.disable_sound = !app_state.settings.disable_sound;
            HandleComboField(IDC_CHECK_SOFTWAREMIXINGONLY, app_state.settings.software_mixing_only);
            HandleComboField(IDC_CHECK_DISABLEEAX, app_state.settings.disable_eax);
            HandleComboField(IDC_CHECK_SUSPENDWHENINACTIVE, app_state.settings.suspend_when_inactive);
            HandleComboField(IDC_CHECK_CLIPALWAYS, app_state.settings.clip_always);
            HandleComboField(IDC_CHECK_MULTIPASSRENDERING, app_state.settings.multipass_rendering);
            HandleComboField(IDC_CHECK_WBUFFER, app_state.settings.w_buffer);
            ToggleFields();
            if (app_state.settings.texture_true_color && Param1 == 1010 && app_state.vram_size < 64)
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
                    app_state.settings.texture_true_color = false;
                }
            }
            else if (app_state.settings.hardware_transform && Param1 != IDC_CHECK_MULTIPASSRENDERING)
            {
                app_state.settings.multipass_rendering = false;
            }
            else if (app_state.settings.multipass_rendering)
            {
                app_state.settings.hardware_transform = false;
            }
            else
            {
                return 0;
            }
            break;
        case IDC_COMBO_BITDEPTH:
            if ((Param1 >> 16) == 1)
            {
                int pos = SendMessageA(app_state.windows.bit_depth, CB_GETCURSEL, NULL, NULL);
                app_state.settings.bit_depth = SendMessageA(app_state.windows.bit_depth, CB_GETITEMDATA, pos, NULL);
            }
            return 0;
        case IDC_COMBO_DISPLAY:
            if ((Param1 >> 16) == 1)
            {
                int pos = SendMessageA(app_state.windows.display, CB_GETCURSEL, NULL, NULL);
                app_state.settings.adapter = SendMessageA(app_state.windows.display, CB_GETITEMDATA, pos, NULL);
                LoadAdapterModes();
            }
            return 0;
        case IDC_BUTTON_AUTODETECT:
            AutodetectSettings();
            break;
        case IDC_BUTTON_EXPERTSETTINGS:
            ToggleExpertSettings();
            return 0;
        case IDC_COMBO_ANTIALIASING:
            if ((Param1 >> 16) == 1)
            {
                int pos = SendMessageA(app_state.windows.antialiasing, CB_GETCURSEL, NULL, NULL);
                app_state.settings.antialiasing = SendMessageA(app_state.windows.antialiasing, CB_GETITEMDATA, pos, NULL);
            }
            return 0;
        case IDC_COMBO_LANGUAGE:
        {
            HWND language = GetDlgItem(hWnd, IDC_COMBO_LANGUAGE);
            int pos = SendMessageA(language, CB_GETCURSEL, NULL, NULL);
            if (pos != -1)
            {
                app_state.settings.language = SendMessageA(language, CB_GETITEMDATA, pos, NULL);
            }
            break;
        }
        case IDC_BUTTON_SAVE:
            SaveSettings(app_state.settings);
            EndDialog(hWnd, 0);
            return 0;
        case IDC_BUTTON_EXIT:
            S_settings empty_settings;
            InitSettings(empty_settings);
            LoadSettings(empty_settings);

            if (memcmp(&empty_settings, &app_state.settings, sizeof(S_settings)) != 0)
            {
                char buffer[256];
                if (LoadStringA(NULL, IDS_DISCARDCHANGES, buffer, sizeof(buffer)) == 0)
                {
                    strcpy(buffer, "Discard changes ?!");
                }

                char caption[128];
                if (LoadStringA(NULL, IDS_WARNING, caption, sizeof(caption)) == 0)
                {
                    strcpy(buffer, "Warning");
                }
                if (MessageBoxA(hWnd, buffer, caption, MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
                {
                    return 0;
                }
            }

            EndDialog(hWnd, 0);
            return 0;
        default:
            return 0;
        }
    }
    else
    {
        return 0;
    }

    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_FULLSCREEN, BM_SETCHECK, app_state.settings.fullscreen, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_TRIPLEBUFFERING, BM_SETCHECK, app_state.settings.triple_buffering, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_HARDWARETRANSFORM, BM_SETCHECK, app_state.settings.hardware_transform, NULL);
    if (app_state.settings.hardware_transform)
    {
        app_state.settings.multipass_rendering = false;
    }
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_TEXTURETRUECOLOR, BM_SETCHECK, app_state.settings.texture_true_color, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_TEXTURELOWDETAIL, BM_SETCHECK, app_state.settings.texture_low_detail, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_VSYNC, BM_SETCHECK, app_state.settings.vsync, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_LIGHTMAPTRUECOLOR, BM_SETCHECK, app_state.settings.lightmap_true_color, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_TEXTURECOMPRESSED, BM_SETCHECK, app_state.settings.texture_compressed, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_SOUND, BM_SETCHECK, !app_state.settings.disable_sound, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_SOFTWAREMIXINGONLY, BM_SETCHECK, app_state.settings.software_mixing_only, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_DISABLEEAX, BM_SETCHECK, app_state.settings.disable_eax, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_SUSPENDWHENINACTIVE, BM_SETCHECK, app_state.settings.suspend_when_inactive, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_CLIPALWAYS, BM_SETCHECK, app_state.settings.clip_always, NULL);
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_MULTIPASSRENDERING, BM_SETCHECK, app_state.settings.multipass_rendering, NULL);
    if (!app_state.supports_w_buffer)
    {
        app_state.settings.w_buffer = false;
    }
    SendDlgItemMessageA(app_state.windows.main, IDC_CHECK_WBUFFER, BM_SETCHECK, app_state.settings.w_buffer, NULL);

    HWND w_buffer_window = GetDlgItem(app_state.windows.main, IDC_CHECK_WBUFFER);
    EnableWindow(w_buffer_window, app_state.supports_w_buffer);

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
    SendDlgItemMessageA(app_state.windows.main, IDC_COMBO_SOUND, CB_SETCURSEL, app_state.settings.sound_type, NULL);
    LoadAdapterData(-1);
    ToggleFields();

    HWND language = GetDlgItem(app_state.windows.main, IDC_COMBO_LANGUAGE);
    SendMessageA(language, CB_RESETCONTENT, NULL, NULL);
    int pos = SendMessageA(language, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>("English"));
    SendMessageA(language, CB_SETITEMDATA, pos, 0);
    pos = SendMessageA(language, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>("Deutsch"));
    SendMessageA(language, CB_SETITEMDATA, pos, 2);
    pos = SendMessageA(language, CB_ADDSTRING, NULL, reinterpret_cast<LPARAM>("Cesky"));
    SendMessageA(language, CB_SETITEMDATA, pos, 1);

    int language_count = SendMessageA(language, CB_GETCOUNT, NULL, NULL);
    if (language_count > 0)
    {
        int i = 0;
        while (true)
        {
            if (SendMessageA(language, CB_GETITEMDATA, i, NULL) == app_state.settings.language)
            {
                break;
            }
            if (++i >= language_count)
            {
                return 0;
            }
        }
        SendMessageA(language, CB_SETCURSEL, i, NULL);
    }

    return 0;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    app_state.hinst = hInstance;
    InitCommonControls();
    app_state.settings_loaded = LoadSettings(app_state.settings);
    DialogBoxParamA(hInstance, MAKEINTRESOURCEA(IDD_MAIN), NULL, DialogFunc, NULL);
    FreeD3D8();
    return 0;
}