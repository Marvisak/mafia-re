#include "types.h"

#define SETTINGS_MAGIC 0x14

#pragma pack(push, 1)
struct S_settings
{
    uchar magic;
    char adapter;
    int width;
    int height;
    int bit_depth;
    int refresh_rate;
    char antialiasing;
    bool multipass_rendering;
    bool clip_always;
    bool fullscreen;
    bool hardware_transform;
    bool vsync;
    bool texture_low_detail;
    bool triple_buffering;
    bool lightmap_true_color;
    bool texture_compressed;
    bool texture_true_color;
    bool disable_sound;
    bool disable_eax;
    bool suspend_when_inactive;
    bool software_mixing_only;
    uchar sound_type;
    int language;
    bool w_buffer;

    // These values are just initialized but never actually set
    float unk;
    float unk2;
    int unk3;
    int unk4;
    int unk5;
    int unk6;

    // The defaults, in declaration order. setup.exe 1.2 constructs its global S_settings and every local
    // copy with them: the global has a dynamic initializer, and no InitSettings function exists.
    // FUNCTION: SETUPSTEAM 0x401000
    S_settings()
    {
        magic = SETTINGS_MAGIC;
        adapter = 0;
        width = 800;
        height = 600;
        bit_depth = 32;
        refresh_rate = 0;
        antialiasing = 0;
        multipass_rendering = false;
        clip_always = false;
        fullscreen = false;
        hardware_transform = true;
        vsync = false;
        texture_low_detail = false;
        triple_buffering = false;
        lightmap_true_color = true;
        texture_compressed = true;
        texture_true_color = false;
        disable_sound = false;
        disable_eax = true;
        suspend_when_inactive = true;
        software_mixing_only = false;
        sound_type = 1;
        language = 0;
        w_buffer = true;
        unk = 1.0;
        unk2 = 1.0;
        unk3 = 2;
        unk4 = 2;
        unk5 = 3;
        unk6 = 2;
    }
};
#pragma pack(pop)