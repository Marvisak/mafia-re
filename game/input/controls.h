#pragma once

#include "Main.h"

#include <vector>

#include "ijoy.h"

// The keys the controls can use at once: IGraph::DefineKeys takes at most 64, and ReadKeys returns a bit for each
// (the name is ours)
#define MAX_CONTROL_KEYS 64

// S_control::half, the part of an axis a control reads (the names are ours)
#define CONTROL_AXIS_FULL 0
#define CONTROL_AXIS_POSITIVE 1
#define CONTROL_AXIS_NEGATIVE 2

// S_control::device (the names are ours)
#define CONTROL_KEY 1
#define CONTROL_MOUSE 2
#define CONTROL_JOY 3

// The mouse's axes, the indices of C_controls::mouse_scale (the names are ours)
enum {
    MOUSE_AXIS_X,
    MOUSE_AXIS_Y,
    MOUSE_AXIS_Z,                       // the wheel
    NUM_MOUSE_AXES,
};

// S_control::code of a mouse axis (the names are ours); a mouse button's is its MBUT_* bit
#define CONTROL_MOUSE_X 0x100
#define CONTROL_MOUSE_Y 0x200
#define CONTROL_MOUSE_Z 0x300           // the wheel

// S_control::code of a joystick's button or POV, added to the button's number or to 16 times the POV's number and its
// direction bits; an axis's is its JOY_* (the names are ours)
#define CONTROL_JOY_BUTTON 0x100
#define CONTROL_JOY_POV 0x200

// A control's binding (PC only, the names are ours): what the controls' table gives each of the game's 60 controls, and
// how GetLastInput (0x4f11f0) describes the last input it saw
struct S_control {
    int half;                           // of an axis, CONTROL_AXIS_*
    ushort device;                      // CONTROL_KEY, CONTROL_MOUSE, CONTROL_JOY
    ushort code;                        // a key's DIK code, a mouse button's MBUT_*, CONTROL_MOUSE_*, a joystick's JOY_*,
                                        // CONTROL_JOY_BUTTON or CONTROL_JOY_POV
    int joy;                            // the joystick's index
};

// A joystick and the state Update (0x4f0620) reads from it
struct S_joy {
    IJoy *joy;
    S_joy_state state;                  // IJoy::GetState
    S_joy_state changes;                // IJoy::GetStateChanges
};

// The player's controls (PC only, the names are ours): the keyboard, the mouse and the joysticks, which the menus and
// the game read. Main.cpp has the one object. Open: nothing references the 0x40 bytes between this class's end and the
// next global (anim_cache, 0x647d80), which are more members no code reaches or a global of their own.
class C_controls
{
    IGraph *igraph;
    std::vector<S_joy> joys;
    S_control last_input;               // what GetLastInput returns
    uchar keys[MAX_CONTROL_KEYS];       // the keys defined with IGraph::DefineKeys, 0 a free slot
    uchar key_refs[MAX_CONTROL_KEYS];   // how many controls use each of them
    uchar mouse_refs[4];                // the same for the mouse's buttons, MBUT_LEFT, MBUT_RIGHT and MBUT_MIDDLE's
    float mouse_scale[NUM_MOUSE_AXES];  // the movement that makes an axis's full value (0x4efdb0 sets them)
    std::vector<S_control> controls;
    std::vector<float> values;          // each control's value
    std::vector<float> pressed;         // each control's value at the update it was pressed in
    ulong key_mask;                     // ReadKeys' bits of the keys whose press can still be reported: set while up
    uint mouse_mask;                    // the same for the mouse's buttons
    char name[32];                      // what GetControlName (0x4f1830) returns
public:
    C_controls();
    ~C_controls();
    // Releases the joysticks and forgets the controls
    void Close();
    // Takes IGraph and the joysticks InitJoy found, each with an AddRef; fails if one of them is missing
    bool Init(IGraph *graph, IJoy **joy_list, int num_joys);
};

extern C_controls controls;

// The joysticks' vector's helpers, which Init's inlined push_back calls
// TEMPLATE: GAME 0x4f1a80
// ?_Ucopy@?$vector@US_joy@@V?$allocator@US_joy@@@std@@@std@@IAEPAUS_joy@@PBU3@0PAU3@@Z

// TEMPLATE: GAME 0x4f1ac0
// ?_Ufill@?$vector@US_joy@@V?$allocator@US_joy@@@std@@@std@@IAEXPAUS_joy@@IABU3@@Z

// TEMPLATE: GAME 0x4f1b00
// ?_Construct@std@@YIXPAUS_joy@@ABU2@@Z
