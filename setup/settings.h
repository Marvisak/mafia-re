#pragma once

#include "main.h"

// Neither has an out-of-line copy in 1.2, which /OPT:NOREF would have kept: they are inlined into
// WinMain and DialogFunc

#ifdef SETUP_STEAM
// Steam's rebuild keeps the settings per user, in the 32-bit view of the registry
#define SETTINGS_ROOT HKEY_CURRENT_USER
#define SETTINGS_ACCESS (KEY_WRITE | KEY_WOW64_32KEY)
#else
#define SETTINGS_ROOT HKEY_LOCAL_MACHINE
#define SETTINGS_ACCESS KEY_ALL_ACCESS
#endif

// FUNCTION: SETUPSTEAM 0x401080
inline bool LoadSettings(S_settings &settings)
{
    HKEY key;
    // STRING: SETUP 0x40e054
    if (RegOpenKeyA(SETTINGS_ROOT, "Software\\Illusion Softworks\\Mafia", &key) != ERROR_SUCCESS)
    {
        return false;
    }

    DWORD type;
    DWORD size = sizeof(S_settings);
    S_settings buffer;
    // STRING: SETUP 0x40e048
    if (RegQueryValueExA(key, "LS3D_setup", NULL, &type, reinterpret_cast<LPBYTE>(&buffer), &size))
    {
        size = 0;
    }
    RegCloseKey(key);

    // Two checks, not one with ||: 1.2 keeps each failure return inline after its check, which an || would
    // merge into one shared exit.
    // The magic of the passed settings is checked, not the one read from the registry, this is what the original does
    if (size != sizeof(S_settings))
    {
        return false;
    }
    if (settings.magic != SETTINGS_MAGIC)
    {
        return false;
    }

    memcpy(&settings, &buffer, sizeof(S_settings));
    return true;
}

// FUNCTION: SETUPSTEAM 0x401120
inline bool SaveSettings(S_settings &settings)
{
    HKEY key;
    DWORD deposition;
    if (RegCreateKeyExA(SETTINGS_ROOT, "Software\\Illusion Softworks\\Mafia", NULL, NULL, NULL, SETTINGS_ACCESS, NULL, &key, &deposition) != ERROR_SUCCESS)
    {
        return false;
    }

    if (RegSetValueExA(key, "LS3D_setup", NULL, REG_BINARY, reinterpret_cast<BYTE *>(&settings), sizeof(S_settings)) != ERROR_SUCCESS)
    {
        RegCloseKey(key);
        return false;
    }
    RegCloseKey(key);
    return true;
}
