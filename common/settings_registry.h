#pragma once

// The settings setup.exe saves and the game reads at its start and writes back when the player quits (main), in the
// registry as the binary value LS3D_setup. Both inline them: setup 1.2 has no out-of-line copy, nor has Game.exe.

#include <windows.h>
#include <string.h>

#include "settings.h"

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
    HKEY registry_key;
    // STRING: SETUP 0x40e054
    // STRING: SETUP10 0x40d054
    // STRING: SETUPCZ 0x40e054
    // STRING: SETUPPL 0x40e054
    // STRING: GAME 0x63797c
    if (RegOpenKeyA(SETTINGS_ROOT, "Software\\Illusion Softworks\\Mafia", &registry_key) != ERROR_SUCCESS)
    {
        return false;
    }

    DWORD type;
    DWORD value_size = sizeof(S_settings);
    S_settings buffer;
    // STRING: SETUP 0x40e048
    // STRING: SETUP10 0x40d048
    // STRING: SETUPCZ 0x40e048
    // STRING: SETUPPL 0x40e048
    // STRING: GAME 0x637970
    if (RegQueryValueExA(registry_key, "LS3D_setup", NULL, &type, (LPBYTE)&buffer, &value_size))
    {
        value_size = 0;
    }
    RegCloseKey(registry_key);

    // Two checks, not one with ||: 1.2 keeps each failure return inline after its check, which an || would
    // merge into one shared exit.
    // The magic of the passed settings is checked, not the one read from the registry, this is what the original does
    if (value_size != sizeof(S_settings))
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
    HKEY registry_key;
    DWORD action;
    if (RegCreateKeyExA(SETTINGS_ROOT, "Software\\Illusion Softworks\\Mafia", NULL, NULL, NULL, SETTINGS_ACCESS, NULL, &registry_key, &action) != ERROR_SUCCESS)
    {
        return false;
    }

    if (RegSetValueExA(registry_key, "LS3D_setup", NULL, REG_BINARY, (BYTE *)&settings, sizeof(S_settings)) != ERROR_SUCCESS)
    {
        RegCloseKey(registry_key);
        return false;
    }
    RegCloseKey(registry_key);
    return true;
}
