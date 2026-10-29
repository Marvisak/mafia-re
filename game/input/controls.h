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
#define CONTROL_AXIS_PEDAL 3            // the whole axis as 0..1, IJoy's JOY_AXIS_POSITIVE (a pedal's)

// S_control::device (the names are ours)
#define CONTROL_NONE 0                  // unbound
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

// A POV's direction bits in S_control::code (the names are guesses: the default look left and right bindings use 8 and
// 2)
#define CONTROL_POV_RIGHT 2
#define CONTROL_POV_LEFT 8

// C_controls::GetValues' flags (the name is ours)
#define CONTROLS_PRESSED 1

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
    // The movement that makes each mouse axis's full value
    bool SetMouseScale(int x, int y, int z);
    // Takes IGraph and the joysticks InitJoy found, each with an AddRef; fails if one of them is missing
    bool Init(IGraph *graph, IJoy **joy_list, int num_joys);
    // Forgets the controls and the keys they defined, keeping the joysticks
    void ClearControls();
    // The controls' values, or with CONTROLS_PRESSED their values at the update each was pressed in (the name is ours):
    // the first one through values and the count, -1 while there are none
    int GetValues(float *&list, uint flags);
    // One control's value, or with CONTROLS_PRESSED its value at the update it was pressed in, 0 for an index outside the
    // table (the name is ours)
    float GetValue(int index, uint flags);
    // Binds the control of that index (NULL unbinds it): defines its key with IGraph, counts the keys' and the mouse
    // buttons' users, sets a joystick axis's mode, and grows the tables to the index. False for a negative index, a key
    // with no free slot, or a joystick that isn't there.
    bool SetControl(const S_control *binding, int index);
    // Copies the binding of that index, leaving binding as it is for an index outside the table; false only without
    // binding (the name is ours)
    bool GetControl(S_control *binding, int index);
    // Reads the keyboard, the mouse and the joysticks into the controls' values; with update_input it has IGraph and
    // the joysticks update their state first, which the menus do themselves
    void Update(bool update_input);
};

extern C_controls controls;
// The joysticks InitJoy found (Main.cpp's)
extern std::vector<IJoy *> joy;

// The joysticks' vector's helpers, which Init's inlined push_back calls
// TEMPLATE: GAME 0x4f1a80
// ?_Ucopy@?$vector@US_joy@@V?$allocator@US_joy@@@std@@@std@@IAEPAUS_joy@@PBU3@0PAU3@@Z

// TEMPLATE: GAME 0x4f1ac0
// ?_Ufill@?$vector@US_joy@@V?$allocator@US_joy@@@std@@@std@@IAEXPAUS_joy@@IABU3@@Z

// TEMPLATE: GAME 0x4f1b00
// ?_Construct@std@@YIXPAUS_joy@@ABU2@@Z

// The tables' helpers, which SetControl's inlined push_back calls
// TEMPLATE: GAME 0x60f1f0
// ?insert@?$vector@US_control@@V?$allocator@US_control@@@std@@@std@@QAEXPAUS_control@@IABU3@@Z

// TEMPLATE: GAME 0x448cc0
// ?insert@?$vector@IV?$allocator@I@std@@@std@@QAEXPAIIABI@Z
