#pragma once

#include "Main.h"

#include <vector>

// A camera state pushed and popped (PS2 G_CameraPushPop), 0x10 bytes by Reset's copy loop; what it holds isn't known yet
struct G_CameraPushPop
{
    uchar unknown_0[0x10];
};

// S_camera_params' flags (the names are ours, by value): the code copies 1 into G_Camera::unknown_22
#define CAMPF_UNKNOWN_1 1
#define CAMPF_UNKNOWN_2 2
#define CAMPF_UNKNOWN_4 4
#define CAMPF_UNKNOWN_8 8

// One of the cameras (an entry of PS2's cameraParams; the struct's and the members' names are ours), 0x44 bytes. PS2's
// entries are 0x38 bytes, without the name.
struct S_camera_params
{
    int id;                             // 0x0, the camera's number: G_Camera::unknown_10 is looked up by it
    int next;                           // 0x4, the number of the camera the player switches to (0: none)
    float values[10];                   // 0x8
    char name[16];                      // 0x30, "P Behind", "C Chase": P on foot, C in a car
    uint flags;                         // 0x40, CAMPF_*
};

// The game's camera (PS2 G_Camera, g_camera.cpp), the game's 0x4c, 0xa0 bytes. PS2's has the same members in the same
// order, at other offsets (in the comments). The members Reset sets are named by their offset; only the constructor, the
// destructor and Reset are written so far.
class G_Camera
{
public:
    int unknown_0;                      // 0x0
    int unknown_4;                      // 0x4
    int unknown_8;                      // 0x8
    int unknown_c;                      // 0xc
    // The number of the camera (S_camera_params::id), the profile's after Reset (PS2's Reset sets 1)
    uint unknown_10;                    // 0x10
    uint unknown_14;                    // 0x14, Reset sets it to unknown_10, the camera's code to the entry's next
    uint unknown_18;                    // 0x18, the camera's flags
    // The camera's values, the first camera's after Reset (PS2 0xd0)
    float *params;                      // 0x1c
    bool unknown_20;                    // 0x20 (PS2 0x1c)
    bool unknown_21;                    // 0x21
    bool unknown_22;                    // 0x22, the flags' 1
    float unknown_24;                   // 0x24, 5 after Reset (PS2 0x20)
    float unknown_28;                   // 0x28, 0.5 after Reset
    int unknown_2c;                     // 0x2c
    int unknown_30;                     // 0x30
    uchar unknown_34[0x24];             // 0x34
    bool unknown_58;                    // 0x58 (PS2 0x70)
    int unknown_5c;                     // 0x5c
    int unknown_60;                     // 0x60 (PS2 0x78)
    int unknown_64;                     // 0x64
    std::vector<G_CameraPushPop> push_pop;  // 0x68 (PS2 0x80)
    std::vector<uint> unknown_78;       // 0x78 (PS2 0x8c), 4-byte values (PS2's vector<unsigned int>)
    int unknown_88;                     // 0x88
    int unknown_8c;                     // 0x8c (PS2 0x9c)
    float unknown_90;                   // 0x90, 0.2 after Reset
    bool unknown_94;                    // 0x94
    uchar unknown_95[0xb];              // 0x95

    G_Camera();
    ~G_Camera();

    // Sets the camera's defaults; the constructor calls it
    void Reset();
};
