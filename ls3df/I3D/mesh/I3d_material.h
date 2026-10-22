#pragma once

#include "I3D/I3D.h"

#include <vector>

// The animated material (PS2 I3D_anim_material, 0x88 bytes on PC): a texture and an environment map, each a series of
// frames shown in turn, every frame_delay ticks of the driver, from loop_frame on once past the last (-1: frames at
// random). PS2 keeps the frames in C_lists, the PC in std::vectors, so its own header: only I3d_material.cpp and the
// files that create one (I3d_driver.cpp) or convert one (I3d_mesh.cpp) include it. The member names are guesses.
// VTABLE: LS3DF 0x1009bef0
// VTABLE: LS3DF11 0x10098ef0
class I3D_anim_material : public I3D_material {
public:
    I3D_anim_material();
    virtual ~I3D_anim_material();

    virtual LS3D_RESULT __stdcall Duplicate(I3D_material *mat);
    virtual void __stdcall UpdateTexture();
    virtual void __stdcall UpdateEnvTexture();
    // Shows a frame, or (negative) restarts the animation; true
    virtual bool __stdcall SetCurrentFrame(int frame);
    // The same for the environment map; false where there is no such frame
    virtual bool __stdcall SetEnvCurrentFrame(int frame);

    // The material's part of the 4ds file, with the frames' count and timing: frame n of an image "nameXX.ext" is
    // "nameNN.ext"
    void LoadImage();

    std::vector<ITexture *> textures;   // 0x40, the frames
    int last_time;                      // 0x50, driverTickTime at the last change (0: none yet)
    int elapsed;                        // 0x54, the time since the last frame change
    int frame;                          // 0x58, the current frame
    std::vector<ITexture *> env_textures; // 0x5c
    int env_last_time;                  // 0x6c
    int env_elapsed;                    // 0x70
    int env_frame;                      // 0x74
    int loop_frame;                     // 0x78, -1: frames at random
    int frame_delay;                    // 0x7c, 0: the first frame only
    int env_loop_frame;                 // 0x80
    int env_frame_delay;                // 0x84
};

// SYNTHETIC: LS3DF 0x1002b430
// SYNTHETIC: LS3DF11 0x1002a730
// I3D_anim_material::`scalar deleting destructor'

// std::vector's out-of-line functions for the frames, at the end of I3d_material.cpp
// TEMPLATE: LS3DF 0x1002c8c0
// TEMPLATE: LS3DF11 0x1002bbc0
// ??1?$vector@PAVITexture@@V?$allocator@PAVITexture@@@std@@@std@@QAE@XZ

// TEMPLATE: LS3DF 0x1002c8e0
// TEMPLATE: LS3DF11 0x1002bbe0
// ?size@?$vector@PAVITexture@@V?$allocator@PAVITexture@@@std@@@std@@QBEIXZ

// TEMPLATE: LS3DF 0x1002c900
// TEMPLATE: LS3DF11 0x1002bc00
// ?insert@?$vector@PAVITexture@@V?$allocator@PAVITexture@@@std@@@std@@QAEPAPAVITexture@@PAPAV3@ABQAV3@@Z

// TEMPLATE: LS3DF 0x1002caa0
// TEMPLATE: LS3DF11 0x1002bda0
// ?erase@?$vector@PAVITexture@@V?$allocator@PAVITexture@@@std@@@std@@QAEPAPAVITexture@@PAPAV3@0@Z

// TEMPLATE: LS3DF 0x1002cad0
// TEMPLATE: LS3DF11 0x1002bdd0
// ?insert@?$vector@PAVITexture@@V?$allocator@PAVITexture@@@std@@@std@@QAEXPAPAVITexture@@IABQAV3@@Z

// TEMPLATE: LS3DF 0x1002ccf0
// TEMPLATE: LS3DF11 0x1002bff0
// ?_Ucopy@?$vector@PAVITexture@@V?$allocator@PAVITexture@@@std@@@std@@IAEPAPAVITexture@@PBQAV3@0PAPAV3@@Z

// TEMPLATE: LS3DF 0x1002cd20
// TEMPLATE: LS3DF11 0x1002c020
// ?_Ufill@?$vector@PAVITexture@@V?$allocator@PAVITexture@@@std@@@std@@IAEXPAPAVITexture@@IABQAV3@@Z

// TEMPLATE: LS3DF 0x1002cd50
// TEMPLATE: LS3DF11 0x1002c050
// ?copy@std@@YIPAPAVITexture@@PAPAV2@00@Z
