#include "main.h"

void InitSettings(S_settings &settings)
{
    settings.unk = 1.0;
    settings.adapter = 0;
    settings.unk2 = 1.0;
    settings.refresh_rate = 0;
    settings.antialiasing = 0;
    settings.multipass_rendering = false;
    settings.clip_always = false;
    settings.fullscreen = false;
    settings.vsync = false;
    settings.texture_low_detail = false;
    settings.disable_sound = false;
    settings.software_mixing_only = false;
    settings.language = 0;
    settings.magic = 0x14;
    settings.width = 800;
    settings.height = 600;
    settings.bit_depth = 32;
    settings.hardware_transform = true;
    settings.lightmap_true_color = true;
    settings.texture_compressed = true;
    settings.disable_eax = true;
    settings.suspend_when_inactive = true;
    settings.sound_type = 1;
    settings.w_buffer = true;
    settings.unk3 = 2;
    settings.unk4 = 2;
    settings.unk5 = 3;
    settings.unk6 = 2;
}

bool LoadSettings(S_settings &settings)
{
    HKEY key;
    if (RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Illusion Softworks\\Mafia", &key) != ERROR_SUCCESS)
    {
        return false;
    }

    S_settings buffer;

    DWORD type;
    DWORD size = sizeof(S_settings);
    InitSettings(settings);
    if (RegQueryValueExA(key, "LS3D_setup", NULL, &type, reinterpret_cast<LPBYTE>(&buffer), &size))
    {
        size = 0;
    }
    RegCloseKey(key);

    if (size != sizeof(S_settings) || buffer.magic != SETTINGS_MAGIC)
    {
        return false;
    }

    memcpy(&settings, &buffer, sizeof(S_settings));
    return true;
}

bool SaveSettings(S_settings &settings)
{
    HKEY key;
    DWORD deposition;
    // The KEY_WOW64_32KEY value which we know today is not present in VS 6.0, I have no idea how they came up with this value
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Illusion Softworks\\Mafia", NULL, NULL, NULL, KEY_WRITE | 0x200, NULL, &key, &deposition) != ERROR_SUCCESS)
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
