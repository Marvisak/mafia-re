// setup.exe has two source files (the Rich header has two objects from the compiler), whose names are unknown. 1.2
// has this one's code at 0x401000-0x4025b0, in the order of this file apart from Error, and hw.cpp's after it; 1.0
// at 0x401000-0x4021b0, in the same order.
// 1.0 (SETUP_1_0) has its texts in the code and the dialog: 1.2 moved them to a string table, set at
// WM_INITDIALOG.
// Steam's Czech setup (2002-08-23, between 1.0 and 1.2) was built without optimization (VC6 SP5 without the Processor
// Pack, /Od /Ob1 /Oi /Gy /G6), which keeps the source's shape: every local and an inlined function's non-constant
// arguments in stack slots of their own, every statement where it is written, the cases in source order. Compiled the
// same way, this file and hw.cpp give its code but for what 1.2 added (the caption text, the W-buffer test, the
// multisampling order), and the shapes the comments credit to "the Czech setup" come from that comparison.
#include "main.h"
#include "hw.h"
#include "settings.h"
#include "res/resource.h"

#include "commctrl.h"

// This file's only global in 1.2, the start of its .data. The rest are in hw.cpp.
// Initialized to true in the binary (it lives in .data, not .bss), overwritten in WinMain anyway
// GLOBAL: SETUP 0x40e040
// GLOBAL: SETUP10 0x40d040
// GLOBAL: SETUPSTEAM 0x405019
bool settings_loaded = true;

INT_PTR __stdcall DialogFunc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

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

// static, which explains both builds' order: VC6 emits a static function after its first caller (1.2 has it after
// DialogFunc), VS2008's link-time code generation where it is defined. Not inline: VC6 inlines that into DialogFunc.
// FUNCTION: SETUPSTEAM 0x4011f0
static void Error(const char *msg)
{
#ifdef SETUP_1_0
    // STRING: SETUP10 0x40d1e4
    MessageBoxA(dialog_state.windows.main, msg, "Setup error", MB_OK);
#else
    char buffer[128];
    if (LoadStringA(NULL, IDS_SETUPERROR, buffer, sizeof(buffer)) == 0)
    {
        // STRING: SETUP 0x40e138
        strcpy(buffer, "Setup error");
    }
    MessageBoxA(dialog_state.windows.main, msg, buffer, MB_OK);
#endif
    EndDialog(dialog_state.windows.main, 0);
}

#ifndef SETUP_1_0
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
#endif

// FUNCTION: SETUP 0x4010b0
// FUNCTION: SETUP10 0x401000
// FUNCTION: SETUPSTEAM 0x4012e0
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    dialog_state.hinst = hInstance;
    InitCommonControls();
    settings_loaded = LoadSettings(settings);
    DialogBoxParamA(hInstance, MAKEINTRESOURCEA(IDD_MAIN), NULL, DialogFunc, NULL);
    FreeD3D8();
    return 0;
}

// FUNCTION: SETUPSTEAM 0x401340
inline void HandleComboField(int uid, bool &value)
{
    // The Czech setup's ?:, which gives 1.2's code only together with the sound toggle's conditional in DialogFunc:
    // with either alone, 1.2 folds the toggle into this compare
    value = SendDlgItemMessageA(dialog_state.windows.main, uid, BM_GETCHECK, NULL, NULL) == BST_CHECKED ? true : false;
}

// Every call of these four is inlined, in every build; their names are ours. The Czech setup shows three of them
// through the stack slots of their calls (EnableItem's id, which the conversion to uint makes one, and its handle;
// SetCheck's flag; SetSelection's index). AddString's arguments are constants and an address, which leave no trace
// there; it is the simplest helper that fits Steam's .reloc (setup/CMakeLists.txt), not a certain one.
inline void EnableItem(uint id, BOOL enable)
{
    HWND window = GetDlgItem(dialog_state.windows.main, id);
    EnableWindow(window, enable);
}

inline void AddString(int id, const char *text)
{
    SendDlgItemMessageA(dialog_state.windows.main, id, CB_ADDSTRING, NULL, (LPARAM)text);
}

inline void SetSelection(int id, int index)
{
    SendDlgItemMessageA(dialog_state.windows.main, id, CB_SETCURSEL, index, NULL);
}

inline void SetCheck(int id, bool checked)
{
    SendDlgItemMessageA(dialog_state.windows.main, id, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, NULL);
}

// FUNCTION: SETUP 0x401220
// FUNCTION: SETUP10 0x401170
// FUNCTION: SETUPSTEAM 0x401370
void ToggleFields()
{
    if (settings.fullscreen)
    {
        EnableItem(IDC_CHECK_VSYNC, TRUE);
        EnableItem(IDC_COMBO_REFRESHRATE, TRUE);
        EnableItem(IDC_COMBO_BITDEPTH, TRUE);
    }
    else
    {
        EnableItem(IDC_CHECK_VSYNC, FALSE);
        EnableItem(IDC_COMBO_REFRESHRATE, FALSE);
        EnableItem(IDC_COMBO_BITDEPTH, FALSE);
    }

    if (!settings.disable_sound)
    {
        EnableItem(IDC_COMBO_SOUND, TRUE);
        EnableItem(IDC_CHECK_SOFTWAREMIXINGONLY, TRUE);
        EnableItem(IDC_CHECK_DISABLEEAX, TRUE);
    }
    else
    {
        EnableItem(IDC_COMBO_SOUND, FALSE);
        EnableItem(IDC_CHECK_SOFTWAREMIXINGONLY, FALSE);
        EnableItem(IDC_CHECK_DISABLEEAX, FALSE);
    }
}

// FUNCTION: SETUP 0x401320
// FUNCTION: SETUP10 0x401270
// FUNCTION: SETUPSTEAM 0x401470
void ToggleExpertSettings()
{
    // A static of the only function that uses it: Steam's build places it after the settings, whose constructor
    // the compiler turned into initialized data, which a global's initializer would be placed before.
    // 1.0 and 1.2 read and write it as a dword, Steam's rebuild as a byte.
#ifdef SETUP_STEAM
    // GLOBAL: SETUPSTEAM 0x40505b
    static bool expert_settings_enabled = true;
#else
    // GLOBAL: SETUP 0x40e044
    // GLOBAL: SETUP10 0x40d044
    static BOOL expert_settings_enabled = TRUE;
#endif
    RECT window_rect;
    GetWindowRect(dialog_state.windows.main, &window_rect);
    // A const local: the Czech setup stores it and folds every use
    const int expert_height = 75;

    // The button's handle goes through a local: 1.2 calls GetDlgItem before pushing the string id, which keeps
    // the two branches from sharing their tails
    if (expert_settings_enabled)
    {
        SetWindowPos(dialog_state.windows.main, HWND_TOP, window_rect.left, window_rect.top, window_rect.right - window_rect.left, window_rect.bottom - window_rect.top - expert_height, SWP_SHOWWINDOW);
        expert_settings_enabled = false;
        HWND button = GetDlgItem(dialog_state.windows.main, IDC_BUTTON_EXPERTSETTINGS);
#ifdef SETUP_1_0
        // STRING: SETUP10 0x40d094
        SetWindowTextA(button, "Open expert settings...");
#else
        SetText(button, IDS_OPENEXPERTSETTINGS);
#endif
    }
    else
    {
        SetWindowPos(dialog_state.windows.main, HWND_TOP, window_rect.left, window_rect.top, window_rect.right - window_rect.left, window_rect.bottom - window_rect.top + expert_height, SWP_SHOWWINDOW);
        expert_settings_enabled = true;
        HWND button = GetDlgItem(dialog_state.windows.main, IDC_BUTTON_EXPERTSETTINGS);
#ifdef SETUP_1_0
        // STRING: SETUP10 0x40d078
        SetWindowTextA(button, "Close expert settings...");
#else
        SetText(button, IDS_CLOSEEXPERTSETTINGS);
#endif
    }
}

// FUNCTION: SETUP 0x4013e0
// FUNCTION: SETUP10 0x401330
// FUNCTION: SETUPSTEAM 0x401550
INT_PTR __stdcall DialogFunc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
#ifndef SETUP_1_0
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
#endif

        GetWindowHandles(dialog_state, hWnd);
        ToggleExpertSettings();

        if (!settings_loaded)
        {
            dialog_state.vram_size = GetTotalVRAM(NULL);
            goto autodetect;
        }
        // The autodetection comes back here (below, in IDC_BUTTON_AUTODETECT)
    autodetect_done:
        if (!LoadD3D8())
        {
#ifdef SETUP_1_0
            // The message 1.2's string table has as IDS_DIRECTXENUMERROR, which its code doesn't load
            // STRING: SETUP10 0x40d11c
            Error("DirectX : Display devices could not be enumerated.");
#else
            char buffer[256];
            if (LoadStringA(NULL, IDS_SETUPERROR, buffer, sizeof(buffer)) == 0)
            {
                // This loads in different error message than it reads from resources
                // STRING: SETUP 0x40e0e8
                strcpy(buffer, "DirectX : Display devices could not be enumerated.");
            }
            Error(buffer);
#endif
        }
        // The controls are refreshed here, after WM_INITDIALOG and for the command cases that jump back. Steam's build
        // has its strings before the command cases' (VS2008 keeps string literals in order of first use); the code is
        // the same either way.
    refresh:
        {
            // Both builds test each flag (setne): a bool passed as it is would be zero-extended
            SetCheck(IDC_CHECK_FULLSCREEN, settings.fullscreen);
            SetCheck(IDC_CHECK_TRIPLEBUFFERING, settings.triple_buffering);
            SetCheck(IDC_CHECK_HARDWARETRANSFORM, settings.hardware_transform);
            if (settings.hardware_transform)
            {
                settings.multipass_rendering = false;
            }
            SetCheck(IDC_CHECK_TEXTURETRUECOLOR, settings.texture_true_color);
            SetCheck(IDC_CHECK_TEXTURELOWDETAIL, settings.texture_low_detail);
            SetCheck(IDC_CHECK_VSYNC, settings.vsync);
            SetCheck(IDC_CHECK_LIGHTMAPTRUECOLOR, settings.lightmap_true_color);
            SetCheck(IDC_CHECK_TEXTURECOMPRESSED, settings.texture_compressed);
            SetCheck(IDC_CHECK_SOUND, !settings.disable_sound);
            SetCheck(IDC_CHECK_SOFTWAREMIXINGONLY, settings.software_mixing_only);
            SetCheck(IDC_CHECK_DISABLEEAX, settings.disable_eax);
            SetCheck(IDC_CHECK_SUSPENDWHENINACTIVE, settings.suspend_when_inactive);
            SetCheck(IDC_CHECK_CLIPALWAYS, settings.clip_always);
            SetCheck(IDC_CHECK_MULTIPASSRENDERING, settings.multipass_rendering);
#ifndef SETUP_1_0
            if (!supports_w_buffer)
            {
                settings.w_buffer = false;
            }
#endif
            SetCheck(IDC_CHECK_WBUFFER, settings.w_buffer);
#ifndef SETUP_1_0
            EnableWindow(GetDlgItem(hWnd, IDC_CHECK_WBUFFER), supports_w_buffer);
#endif

            SendDlgItemMessageA(dialog_state.windows.main, IDC_COMBO_SOUND, CB_RESETCONTENT, NULL, NULL);
#ifdef SETUP_1_0
            // STRING: SETUP10 0x40d108
            AddString(IDC_COMBO_SOUND, "Stereo headphones");
            // STRING: SETUP10 0x40d0f8
            AddString(IDC_COMBO_SOUND, "Stereo speakers");
            // STRING: SETUP10 0x40d0e0
            AddString(IDC_COMBO_SOUND, "Quadraphonic speakers");
            // STRING: SETUP10 0x40d0c4
            AddString(IDC_COMBO_SOUND, "5.1 surround sound speakers");
            SetSelection(IDC_COMBO_SOUND, settings.sound_type);
#else

            // A block of its own: 1.2 gives this buffer the stack slot of the cases' captions, which a buffer in the function's
            // scope can't share
            {
                char buffer[256];
                if (LoadStringA(NULL, IDS_STEREOHEADPHONES, buffer, sizeof(buffer)) == 0)
                {
                    // STRING: SETUP 0x40e0d4
                    strcpy(buffer, "Stereo headphones");
                }
                AddString(IDC_COMBO_SOUND, buffer);

                if (LoadStringA(NULL, IDS_STEREOSPEAKERS, buffer, sizeof(buffer)) == 0)
                {
                    // STRING: SETUP 0x40e0c4
                    strcpy(buffer, "Stereo speakers");
                }
                AddString(IDC_COMBO_SOUND, buffer);

                if (LoadStringA(NULL, IDS_QUADROPHONICSPEAKERS, buffer, sizeof(buffer)) == 0)
                {
                    // STRING: SETUP 0x40e0ac
                    // STRING: SETUPSTEAM 0x4041f8
                    strcpy(buffer, "Quadraphonic speakers");
                }
                AddString(IDC_COMBO_SOUND, buffer);

                if (LoadStringA(NULL, IDS_SURROUNDSPEAKERS, buffer, sizeof(buffer)) == 0)
                {
                    // STRING: SETUP 0x40e090
                    strcpy(buffer, "5.1 surround sound speakers");
                }
                AddString(IDC_COMBO_SOUND, buffer);
                SetSelection(IDC_COMBO_SOUND, settings.sound_type);
            }
#endif
            LoadAdapterData(-1);
            ToggleFields();

            // GetDlgItem for every message, as both builds call it
            SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_RESETCONTENT, NULL, NULL);
            // STRING: SETUP 0x40e088
            // STRING: SETUP10 0x40d0bc
            int pos = SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_ADDSTRING, NULL, (LPARAM)"English");
            SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_SETITEMDATA, pos, 0);
            // The typo is in the binary
            // STRING: SETUP 0x40e080
            // STRING: SETUP10 0x40d0b4
            pos = SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_ADDSTRING, NULL, (LPARAM)"Deutch");
            SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_SETITEMDATA, pos, 2);
            // STRING: SETUP 0x40e078
            // STRING: SETUP10 0x40d0ac
            pos = SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_ADDSTRING, NULL, (LPARAM)"Cesky");
            SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_SETITEMDATA, pos, 1);

            int language_count = SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_GETCOUNT, NULL, NULL);
            for (int i = 0; i < language_count; i++)
            {
                // The item data in pos, as the Czech setup has it
                pos = SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_GETITEMDATA, i, NULL);
                if (pos == settings.language)
                {
                    SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_SETCURSEL, i, NULL);
                    break;
                }
            }
        }
        break;
    case WM_COMMAND:
        // The cases in the Czech setup's order. The checkbox case comes before Exit: Steam's build has its "Warning"
        // first. VS2008 lays out the jump table's targets in source order, and so does VC6, which lays out the rest by
        // the dispatch tree. Every case breaks to the one return at the end, as in the Czech setup.
        switch (LOWORD(wParam))
        {
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
            // The Czech setup's conditional of two assignments (see HandleComboField)
            settings.disable_sound == true ? settings.disable_sound = false : settings.disable_sound = true;
            HandleComboField(IDC_CHECK_SOFTWAREMIXINGONLY, settings.software_mixing_only);
            HandleComboField(IDC_CHECK_DISABLEEAX, settings.disable_eax);
            HandleComboField(IDC_CHECK_SUSPENDWHENINACTIVE, settings.suspend_when_inactive);
            HandleComboField(IDC_CHECK_CLIPALWAYS, settings.clip_always);
            HandleComboField(IDC_CHECK_MULTIPASSRENDERING, settings.multipass_rendering);
            HandleComboField(IDC_CHECK_WBUFFER, settings.w_buffer);
            ToggleFields();
            if (settings.texture_true_color && LOWORD(wParam) == IDC_CHECK_TEXTURETRUECOLOR && dialog_state.vram_size < 64)
            {
#ifdef SETUP_1_0
                int result = MessageBoxA(
                        hWnd,
                        // STRING: SETUP10 0x40d150
                        "Selecting truecolor textures is only recommended\n\rfor video cards with 64MB video RAM or above.\n\rContinue?",
                        // STRING: SETUP10 0x40d1bc
                        "Warning",
                        MB_YESNO | MB_ICONEXCLAMATION);
#else
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

                int result = MessageBoxA(hWnd, buffer, caption, MB_YESNO | MB_ICONEXCLAMATION);
#endif
                // The result in a local, where the exit prompt compares the call: the Czech setup has both
                if (result == IDNO)
                {
                    settings.texture_true_color = false;
                    goto refresh;
                }
            }

            if (settings.hardware_transform && LOWORD(wParam) != IDC_CHECK_MULTIPASSRENDERING)
            {
                settings.multipass_rendering = false;
                goto refresh;
            }
            if (settings.multipass_rendering)
            {
                settings.hardware_transform = false;
                goto refresh;
            }
            break;
        case IDC_COMBO_SOUND:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                settings.sound_type = SendDlgItemMessageA(dialog_state.windows.main, IDC_COMBO_SOUND, CB_GETCURSEL, NULL, NULL);
                ToggleFields();
            }
            break;
        case IDC_COMBO_DISPLAY:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                // The selection is cut to a byte into an int: both builds extend it once, a uchar local at its use
                int pos = (uchar)SendMessageA(dialog_state.windows.display, CB_GETCURSEL, NULL, NULL);
                settings.adapter = SendMessageA(dialog_state.windows.display, CB_GETITEMDATA, pos, NULL);
                LoadAdapterModes();
            }
            break;
        case IDC_COMBO_RESOLUTION:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int pos = SendMessageA(dialog_state.windows.resolution, CB_GETCURSEL, NULL, NULL);
                // Two tests and a cast into a second local, as the Czech setup has them
                LRESULT data = SendMessageA(dialog_state.windows.resolution, CB_GETITEMDATA, pos, NULL);
                if (!data)
                {
                    break;
                }
                if (data == CB_ERR)
                {
                    break;
                }
                S_adapter_mode *adapter_data = (S_adapter_mode *)data;
                settings.width = adapter_data->width;
                settings.height = adapter_data->height;
                LoadAdapterRefreshRates();
                LoadAdapterData(pos);
            }
            break;
        case IDC_COMBO_ANTIALIASING:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int pos = (uchar)SendMessageA(dialog_state.windows.antialiasing, CB_GETCURSEL, NULL, NULL);
                settings.antialiasing = SendMessageA(dialog_state.windows.antialiasing, CB_GETITEMDATA, pos, NULL);
            }
            break;
        case IDC_COMBO_REFRESHRATE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int pos = (uchar)SendMessageA(dialog_state.windows.refresh_rate, CB_GETCURSEL, NULL, NULL);
                // Cut to a byte as well, this is what the original does
                settings.refresh_rate = (uchar)SendMessageA(dialog_state.windows.refresh_rate, CB_GETITEMDATA, pos, NULL);
            }
            break;
        case IDC_COMBO_BITDEPTH:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int pos = (uchar)SendMessageA(dialog_state.windows.bit_depth, CB_GETCURSEL, NULL, NULL);
                settings.bit_depth = SendMessageA(dialog_state.windows.bit_depth, CB_GETITEMDATA, pos, NULL);
            }
            break;
        case IDC_BUTTON_EXPERTSETTINGS:
            ToggleExpertSettings();
            break;
        case IDC_COMBO_LANGUAGE:
        {
            int pos = SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_GETCURSEL, NULL, NULL);
            if (pos != CB_ERR)
            {
                settings.language = SendMessageA(GetDlgItem(hWnd, IDC_COMBO_LANGUAGE), CB_GETITEMDATA, pos, NULL);
            }
            break;
        }
        case IDC_BUTTON_AUTODETECT:
            // The autodetection is here once, entered from WM_INITDIALOG too, and returns to it: the Czech setup has it
            // in this case, which WM_INITDIALOG reaches through VC6's jump stub for a goto into the switch, and both
            // optimized builds show the join (WM_INITDIALOG sets the constant register, mov edx, 0x20, and jumps past
            // this case's load of vram_size). Every branch sets all seven settings, as in the Czech setup.
        autodetect:
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
            if (dialog_state.vram_size <= 8)
            {
                settings.width = 640;
                settings.height = 480;
                settings.bit_depth = 16;
                settings.lightmap_true_color = false;
                settings.texture_true_color = false;
                settings.texture_low_detail = true;
                settings.texture_compressed = true;
            }
            else if (dialog_state.vram_size <= 16)
            {
                settings.width = 800;
                settings.height = 600;
                settings.bit_depth = 16;
                settings.lightmap_true_color = true;
                settings.texture_true_color = false;
                settings.texture_low_detail = true;
                settings.texture_compressed = true;
            }
            else if (dialog_state.vram_size <= 32)
            {
                settings.width = 800;
                settings.height = 600;
                settings.bit_depth = 32;
                settings.lightmap_true_color = true;
                settings.texture_true_color = false;
                settings.texture_low_detail = false;
                settings.texture_compressed = true;
            }
            else if (dialog_state.vram_size <= 64)
            {
                settings.width = 1024;
                settings.height = 768;
                settings.bit_depth = 32;
                settings.lightmap_true_color = true;
                settings.texture_true_color = false;
                settings.texture_low_detail = false;
                settings.texture_compressed = true;
            }
            else
            {
                settings.width = 1280;
                settings.height = 1024;
                settings.bit_depth = 32;
                settings.lightmap_true_color = true;
                settings.texture_true_color = true;
                settings.texture_low_detail = false;
                settings.texture_compressed = false;
            }
            goto autodetect_done;
        case IDC_BUTTON_SAVE:
            SaveSettings(settings);
            EndDialog(hWnd, 0);
            break;
        case IDC_BUTTON_EXIT:
        {
            S_settings saved_settings;
            LoadSettings(saved_settings);

            if (memcmp(&saved_settings, &settings, sizeof(S_settings)) != 0)
            {
#ifdef SETUP_1_0
                // 1.2 has "Warning" as the caption here too
                if (MessageBoxA(
                        hWnd,
                        // STRING: SETUP10 0x40d1c4
                        "Discard changes ?!",
                        // STRING: SETUP10 0x40d1d8
                        "WARNING...",
                        MB_YESNO | MB_ICONEXCLAMATION) == IDNO)
#else
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
#endif
                {
                    return 0;
                }
            }

            EndDialog(hWnd, -1);
            break;
        }
        }
        break;
    }
    return 0;
}
