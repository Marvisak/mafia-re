#include "types.h"

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
};
#pragma pack(pop)