// The player's controls (PC only, the name is ours; the file at 0x4efb40-0x4f1b4c, which starts with the class's
// constructor, docs/game-files.md), written whole. Open: GetLastInput's registers (where flags and &last_input are
// kept in the joysticks' loop, and two stack slots swapped).

#include "input/controls.h"
#include "ui/g_textdb.h"

#include <math.h>
#include <string.h>

#include "IGraph/IGraph.h"

// FUNCTION: GAME 0x4efb40
C_controls::C_controls()
{
    mouse_scale[MOUSE_AXIS_X] = 20.0f;
    mouse_scale[MOUSE_AXIS_Y] = 20.0f;
    mouse_scale[MOUSE_AXIS_Z] = 20.0f;
    igraph = NULL;
}

// FUNCTION: GAME 0x4efbd0
C_controls::~C_controls()
{
    Close();
}

// FUNCTION: GAME 0x4efc80
void C_controls::Close()
{
    controls.clear();
    values.clear();
    pressed.clear();
    for (int i = joys.size(); i--; )
        if (joys[i].joy)
            joys[i].joy->Release();
    joys.clear();
}

// FUNCTION: GAME 0x4efdb0
bool C_controls::SetMouseScale(int x, int y, int z)
{
    mouse_scale[MOUSE_AXIS_X] = (float)x;
    mouse_scale[MOUSE_AXIS_Y] = (float)y;
    mouse_scale[MOUSE_AXIS_Z] = (float)z;
    return true;
}

// FUNCTION: GAME 0x4efde0
bool C_controls::Init(IGraph *graph, IJoy **joy_list, int num_joys)
{
    if (!graph)
        return false;
    int i;
    for (i = num_joys; i--; )
        if (!joy_list[i])
            return false;
    memset(keys, 0, sizeof(keys));
    memset(key_refs, 0, sizeof(key_refs));
    memset(mouse_refs, 0, sizeof(mouse_refs));
    igraph = graph;
    for (i = 0; i < num_joys; i++) {
        S_joy j;
        memset(&j, 0, sizeof(j));
        j.joy = joy_list[i];
        j.joy->AddRef();
        joys.push_back(j);
    }
    key_mask = (ulong)-1;
    return true;
}

// FUNCTION: GAME 0x4f00f0
void C_controls::ClearControls()
{
    controls.clear();
    values.clear();
    pressed.clear();
    memset(keys, 0, sizeof(keys));
    memset(key_refs, 0, sizeof(key_refs));
    memset(mouse_refs, 0, sizeof(mouse_refs));
}

// FUNCTION: GAME 0x4f01a0
int C_controls::GetValues(float *&list, uint flags)
{
    if (flags & CONTROLS_PRESSED)
    {
        if (pressed.size() < 1)
            return -1;
        list = pressed.begin();
        return pressed.size();
    }
    if (values.size() < 1)
        return -1;
    list = values.begin();
    return values.size();
}

// One control's value, or with CONTROLS_PRESSED its value at the update it was pressed in; 0 for an index outside the
// table (the name is ours)
// FUNCTION: GAME 0x4f0240
float C_controls::GetValue(int index, uint flags)
{
    if (flags & CONTROLS_PRESSED)
    {
        if (index < pressed.size() && index >= 0)
            return pressed[index];
    }
    else if (index < values.size() && index >= 0)
        return values[index];
    return 0.0f;
}

// FUNCTION: GAME 0x4f02b0
bool C_controls::SetControl(const S_control *binding, int index)
{
    if (index < 0)
        return false;
    if (index >= controls.size())
    {
        S_control none;
        memset(&none, 0, sizeof(none));
        while (controls.size() <= index)
            controls.push_back(none);
        while (values.size() <= index)
            values.push_back(0.0f);
        while (pressed.size() <= index)
            pressed.push_back(0.0f);
    }
    // The old binding's key and mouse button lose a user
    int i;
    if (controls[index].device == CONTROL_KEY)
    {
        for (i = 0; i < MAX_CONTROL_KEYS; i++)
            if (keys[i] == (uchar)controls[index].code)
                break;
        if (i < MAX_CONTROL_KEYS && !--key_refs[i])
        {
            keys[i] = 0;
            igraph->DefineKeys(keys, MAX_CONTROL_KEYS);
        }
    }
    if (controls[index].device == CONTROL_MOUSE)
    {
        switch (controls[index].code)
        {
        case MBUT_LEFT:
            i = 0;
            break;
        case MBUT_RIGHT:
            i = 1;
            break;
        case MBUT_MIDDLE:
            i = 2;
            break;
        default:
            i = -1;
        }
        if (i >= 0)
            mouse_refs[i]--;
    }
    if (binding)
    {
        if (binding->device == CONTROL_KEY)
        {
            for (i = 0; i < MAX_CONTROL_KEYS; i++)
                if (keys[i] == (uchar)binding->code)
                    break;
            if (i < MAX_CONTROL_KEYS)
                key_refs[i]++;
            else
            {
                // A free slot for the new key
                int j;
                for (j = 0; j < MAX_CONTROL_KEYS; j++)
                    if (!keys[j])
                        break;
                if (j < MAX_CONTROL_KEYS)
                {
                    keys[j] = (uchar)binding->code;
                    igraph->DefineKeys(keys, MAX_CONTROL_KEYS);
                    key_refs[j]++;
                }
                else
                    return false;
            }
        }
        else if (binding->device == CONTROL_MOUSE)
        {
            switch (binding->code)
            {
            case MBUT_LEFT:
                i = 0;
                break;
            case MBUT_RIGHT:
                i = 1;
                break;
            case MBUT_MIDDLE:
                i = 2;
                break;
            default:
                i = -1;
            }
            if (i >= 0)
                mouse_refs[i]++;
        }
        else if (binding->device == CONTROL_JOY)
        {
            if (joys.size() > (uint)binding->joy)
            {
                if (binding->half == CONTROL_AXIS_PEDAL && binding->code < 0xff)
                    joys[binding->joy].joy->SetAxisMode(binding->code, JOY_AXIS_POSITIVE);
                else
                    joys[binding->joy].joy->SetAxisMode(binding->code, 0);
            }
            else
                return false;
        }
        controls[index] = *binding;
    }
    else
        memset(&controls[index], 0, sizeof(S_control));
    return true;
}

// An index outside the table returns early: written as the copy's condition, VC6 pushes esi after the first test
// FUNCTION: GAME 0x4f05b0
bool C_controls::GetControl(S_control *binding, int index)
{
    if (!binding)
        return false;
    if (index < 0 || index >= controls.size())
        return true;
    *binding = controls[index];
    return true;
}

// A POV's direction bits from its angle (the name is ours), Update's: a diagonal outside a small window around each
// direction. It returns an int: as a uint the second test's mask comes out after the and, not before it.
static inline int GetPOVDirection(float angle)
{
    int direction;
    if (angle > 3.16f)
    {
        if (angle > 4.72f)
            direction = CONTROL_POV_LEFT | CONTROL_POV_UP;
        else if (angle < 4.69f)
            direction = CONTROL_POV_DOWN | CONTROL_POV_LEFT;
        else
            direction = CONTROL_POV_LEFT;
    }
    else if (angle < 3.12f)
    {
        if (angle < 1.5f)
        {
            if (angle > .1f)
                direction = CONTROL_POV_UP | CONTROL_POV_RIGHT;
            else
                direction = CONTROL_POV_UP;
        }
        else if (angle > 1.6f)
            direction = CONTROL_POV_RIGHT | CONTROL_POV_DOWN;
        else
            direction = CONTROL_POV_RIGHT;
    }
    else
        direction = CONTROL_POV_DOWN;
    return direction;
}

// A control's press is reported once: a key's or a mouse button's mask bit is set while it is up and cleared once
// every control bound to it has seen the press (the copies of the users' counts count them down). A joystick's press
// is its GetStateChanges.
// - The mouse's refs are copied before its mask is updated, then the keys': another order schedules the copies and the
//   masks' stores elsewhere.
// - The joystick's code goes through an int local, which gives the original's 32-bit signed compares (the member
//   compares as a word) and the POV number's sar; the button's bit reads the member, through the local the first POV
//   test's direction lands in edi.
// FUNCTION: GAME 0x4f0620
void C_controls::Update(bool update_input)
{
    int i;
    for (i = joys.size(); i--;)
    {
        if (update_input)
            joys[i].joy->Update(JOY_UPDATE_RESET);
        joys[i].joy->GetState(&joys[i].state);
        joys[i].joy->GetStateChanges(&joys[i].changes);
    }
    if (update_input)
    {
        igraph->ProcessWinMessages();
        igraph->UpdateMouseData();
        igraph->UpdateKeyboardData();
    }
    ulong keys_down = igraph->ReadKeys();
    uint buttons = igraph->GetMouseButtons();
    float mx = (float)igraph->Mouse_rx();
    float my = (float)igraph->Mouse_ry();
    float mz = (float)igraph->Mouse_rz();
    uchar mrefs[4];
    uchar krefs[MAX_CONTROL_KEYS];
    memcpy(mrefs, mouse_refs, sizeof(mrefs));
    mouse_mask &= buttons;
    mouse_mask = ~mouse_mask;
    mouse_mask ^= buttons;
    memcpy(krefs, key_refs, sizeof(krefs));
    key_mask &= keys_down;
    key_mask = ~key_mask;
    key_mask ^= keys_down;
    // The mouse's moves, clamped to the scale, as -1..1
    if (fabs(mx) > mouse_scale[MOUSE_AXIS_X])
        mx = mx > 0.0f ? mouse_scale[MOUSE_AXIS_X] : -mouse_scale[MOUSE_AXIS_X];
    mx /= mouse_scale[MOUSE_AXIS_X];
    if (fabs(my) > mouse_scale[MOUSE_AXIS_Y])
        my = my > 0.0f ? mouse_scale[MOUSE_AXIS_Y] : -mouse_scale[MOUSE_AXIS_Y];
    my /= mouse_scale[MOUSE_AXIS_Y];
    if (fabs(mz) > mouse_scale[MOUSE_AXIS_Z])
        mz = mz > 0.0f ? mouse_scale[MOUSE_AXIS_Z] : -mouse_scale[MOUSE_AXIS_Z];
    mz /= mouse_scale[MOUSE_AXIS_Z];
    int button;
    for (i = controls.size(); i--;)
    {
        switch (controls[i].device)
        {
        case CONTROL_KEY:
            {
                int k = 0;
                while ((uchar)controls[i].code != keys[k] && k < MAX_CONTROL_KEYS)
                    k++;
                ulong bit = (ulong)1 << k;
                if (keys_down & bit)
                    values[i] = 1.0f;
                else
                    values[i] = 0.0f;
                if ((keys_down & bit) && (key_mask & bit))
                {
                    if (--krefs[k] < 1)
                        key_mask &= ~bit;
                    pressed[i] = 1.0f;
                }
                else
                    pressed[i] = 0.0f;
            }
            break;
        case CONTROL_MOUSE:
            values[i] = 0.0f;
            switch (controls[i].code)
            {
            case MBUT_LEFT:
            case MBUT_RIGHT:
            case MBUT_MIDDLE:
                {
                    switch (controls[i].code)
                    {
                    case MBUT_LEFT:
                        button = 0;
                        break;
                    case MBUT_RIGHT:
                        button = 1;
                        break;
                    case MBUT_MIDDLE:
                        button = 2;
                        break;
                    }
                    uint bit = 1 << button;
                    if (buttons & bit)
                        values[i] = 1.0f;
                    else
                        values[i] = 0.0f;
                    if ((buttons & bit) && (mouse_mask & bit))
                    {
                        if (--mrefs[button] < 1)
                            mouse_mask &= ~bit;
                        pressed[i] = 1.0f;
                    }
                    else
                        pressed[i] = 0.0f;
                }
                break;
            case CONTROL_MOUSE_X:
                switch (controls[i].half)
                {
                case CONTROL_AXIS_POSITIVE:
                    if (mx > 0.0f)
                        values[i] = mx;
                    break;
                case CONTROL_AXIS_NEGATIVE:
                    if (mx < 0.0f)
                        values[i] = -mx;
                    break;
                default:
                    values[i] = mx;
                }
                pressed[i] = values[i];
                break;
            case CONTROL_MOUSE_Y:
                switch (controls[i].half)
                {
                case CONTROL_AXIS_POSITIVE:
                    if (my > 0.0f)
                        values[i] = my;
                    break;
                case CONTROL_AXIS_NEGATIVE:
                    if (my < 0.0f)
                        values[i] = -my;
                    break;
                default:
                    values[i] = my;
                }
                pressed[i] = values[i];
                break;
            case CONTROL_MOUSE_Z:
                switch (controls[i].half)
                {
                case CONTROL_AXIS_POSITIVE:
                    if (mz > 0.0f)
                        values[i] = mz;
                    break;
                case CONTROL_AXIS_NEGATIVE:
                    if (mz < 0.0f)
                        values[i] = -mz;
                    break;
                default:
                    values[i] = mz;
                }
                pressed[i] = values[i];
                break;
            }
            break;
        case CONTROL_JOY:
            values[i] = 0.0f;
            pressed[i] = 0.0f;
            if (joys.size() > controls[i].joy)
            {
                int code = controls[i].code;
                if (code > 0xff && code < CONTROL_JOY_POV)
                {
                    // Only the first 32 buttons
                    uint bit = 1 << (controls[i].code & 0xff);
                    if (joys[controls[i].joy].state.buttons[0] & bit)
                        values[i] = 1.0f;
                    if (joys[controls[i].joy].changes.buttons[0] & bit)
                        pressed[i] = 1.0f;
                }
                else if (code >= CONTROL_JOY_POV)
                {
                    int pov = (code >> 4) & 0xf;
                    if (!joys[controls[i].joy].state.pov_centered[pov] &&
                        GetPOVDirection(joys[controls[i].joy].state.pov[pov]) & controls[i].code & 0xf)
                        values[i] = 1.0f;
                    if (!joys[controls[i].joy].changes.pov_centered[pov] &&
                        GetPOVDirection(joys[controls[i].joy].changes.pov[pov]) & controls[i].code & 0xf)
                        pressed[i] = 1.0f;
                }
                else
                {
                    switch (controls[i].half)
                    {
                    case CONTROL_AXIS_POSITIVE:
                        switch (code)
                        {
                        case JOY_X:
                            if (joys[controls[i].joy].state.x > 0.0f)
                                values[i] = joys[controls[i].joy].state.x;
                            break;
                        case JOY_Y:
                            if (joys[controls[i].joy].state.y > 0.0f)
                                values[i] = joys[controls[i].joy].state.y;
                            break;
                        case JOY_Z:
                            if (joys[controls[i].joy].state.z > 0.0f)
                                values[i] = joys[controls[i].joy].state.z;
                            break;
                        case JOY_RX:
                            if (joys[controls[i].joy].state.rx > 0.0f)
                                values[i] = joys[controls[i].joy].state.rx;
                            break;
                        case JOY_RY:
                            if (joys[controls[i].joy].state.ry > 0.0f)
                                values[i] = joys[controls[i].joy].state.ry;
                            break;
                        case JOY_RZ:
                            if (joys[controls[i].joy].state.rz > 0.0f)
                                values[i] = joys[controls[i].joy].state.rz;
                            break;
                        case JOY_SLIDER0:
                            if (joys[controls[i].joy].state.slider[0] > 0.0f)
                                values[i] = joys[controls[i].joy].state.slider[0];
                            break;
                        case JOY_SLIDER1:
                            if (joys[controls[i].joy].state.slider[1] > 0.0f)
                                values[i] = joys[controls[i].joy].state.slider[1];
                            break;
                        }
                        break;
                    case CONTROL_AXIS_NEGATIVE:
                        switch (code)
                        {
                        case JOY_X:
                            if (joys[controls[i].joy].state.x < 0.0f)
                                values[i] = -joys[controls[i].joy].state.x;
                            break;
                        case JOY_Y:
                            if (joys[controls[i].joy].state.y < 0.0f)
                                values[i] = -joys[controls[i].joy].state.y;
                            break;
                        case JOY_Z:
                            if (joys[controls[i].joy].state.z < 0.0f)
                                values[i] = -joys[controls[i].joy].state.z;
                            break;
                        case JOY_RX:
                            if (joys[controls[i].joy].state.rx < 0.0f)
                                values[i] = -joys[controls[i].joy].state.rx;
                            break;
                        case JOY_RY:
                            if (joys[controls[i].joy].state.ry < 0.0f)
                                values[i] = -joys[controls[i].joy].state.ry;
                            break;
                        case JOY_RZ:
                            if (joys[controls[i].joy].state.rz < 0.0f)
                                values[i] = -joys[controls[i].joy].state.rz;
                            break;
                        case JOY_SLIDER0:
                            if (joys[controls[i].joy].state.slider[0] < 0.0f)
                                values[i] = -joys[controls[i].joy].state.slider[0];
                            break;
                        case JOY_SLIDER1:
                            if (joys[controls[i].joy].state.slider[1] < 0.0f)
                                values[i] = -joys[controls[i].joy].state.slider[1];
                            break;
                        }
                        break;
                    default:
                        switch (code)
                        {
                        case JOY_X:
                            values[i] = joys[controls[i].joy].state.x;
                            break;
                        case JOY_Y:
                            values[i] = joys[controls[i].joy].state.y;
                            break;
                        case JOY_Z:
                            values[i] = joys[controls[i].joy].state.z;
                            break;
                        case JOY_RX:
                            values[i] = joys[controls[i].joy].state.rx;
                            break;
                        case JOY_RY:
                            values[i] = joys[controls[i].joy].state.ry;
                            break;
                        case JOY_RZ:
                            values[i] = joys[controls[i].joy].state.rz;
                            break;
                        case JOY_SLIDER0:
                            values[i] = joys[controls[i].joy].state.slider[0];
                            break;
                        case JOY_SLIDER1:
                            values[i] = joys[controls[i].joy].state.slider[1];
                            break;
                        }
                    }
                    pressed[i] = values[i];
                }
            }
            break;
        }
    }
}

// FUNCTION: GAME 0x4f1180
void C_controls::SaveJoyStates()
{
    for (int i = joys.size(); i--;)
    {
        joys[i].joy->Update(JOY_UPDATE_RESET);
        joys[i].joy->GetState(&joys[i].changes);
    }
}

// The binding is last_input, cleared first; each kind of input sets what it has. A joystick's button or POV leaves
// the half as it is.
// FUNCTION: GAME 0x4f11f0
S_control *C_controls::GetLastInput(uint flags, bool update)
{
    last_input.device = CONTROL_NONE;
    last_input.code = 0;
    last_input.joy = 0;
    last_input.half = CONTROL_AXIS_FULL;
    int i;
    for (i = joys.size(); i--;)
    {
        if (update)
            joys[i].joy->Update(JOY_UPDATE_RESET);
        joys[i].joy->GetState(&joys[i].state);
    }
    if (update)
    {
        igraph->ProcessWinMessages();
        igraph->UpdateMouseData();
        igraph->UpdateKeyboardData();
    }
    uint shift_keys = igraph->GetShiftKeys(false);
    uchar key = igraph->ReadKey();
    uint buttons = igraph->GetMouseButtons();
    int rx = igraph->Mouse_rx();
    int ry = igraph->Mouse_ry();
    int rz = igraph->Mouse_rz();
    if (!(flags & LASTINPUT_NO_KEYS) && (shift_keys || key))
    {
        last_input.device = CONTROL_KEY;
        if (shift_keys)
        {
            if (shift_keys & SKEY_LSHIFT)
                last_input.code = DIK_LSHIFT;
            else if (shift_keys & SKEY_RSHIFT)
                last_input.code = DIK_RSHIFT;
            else if (shift_keys & SKEY_LCTRL)
                last_input.code = DIK_LCONTROL;
            else if (shift_keys & SKEY_RCTRL)
                last_input.code = DIK_RCONTROL;
            else if (shift_keys & SKEY_LALT)
                last_input.code = DIK_LMENU;
            else if (shift_keys & SKEY_RALT)
                last_input.code = DIK_RMENU;
        }
        else
            last_input.code = key;
        last_input.joy = 0;
        last_input.half = CONTROL_AXIS_FULL;
        return &last_input;
    }
    if (!(flags & LASTINPUT_NO_MOUSE_BUTTONS))
    {
        if (buttons & MBUT_LEFT)
        {
            last_input.device = CONTROL_MOUSE;
            last_input.code = MBUT_LEFT;
            last_input.joy = 0;
            last_input.half = CONTROL_AXIS_FULL;
            return &last_input;
        }
        if (buttons & MBUT_RIGHT)
        {
            last_input.device = CONTROL_MOUSE;
            last_input.code = MBUT_RIGHT;
            last_input.joy = 0;
            last_input.half = CONTROL_AXIS_FULL;
            return &last_input;
        }
        if (buttons & MBUT_MIDDLE)
        {
            last_input.device = CONTROL_MOUSE;
            last_input.code = MBUT_MIDDLE;
            last_input.joy = 0;
            last_input.half = CONTROL_AXIS_FULL;
            return &last_input;
        }
    }
    if (!(flags & LASTINPUT_NO_MOUSE_AXES))
    {
        if (rx > 10)
        {
            last_input.device = CONTROL_MOUSE;
            last_input.code = CONTROL_MOUSE_X;
            last_input.joy = 0;
            last_input.half = CONTROL_AXIS_POSITIVE;
            return &last_input;
        }
        if (rx < -10)
        {
            last_input.device = CONTROL_MOUSE;
            last_input.code = CONTROL_MOUSE_X;
            last_input.joy = 0;
            last_input.half = CONTROL_AXIS_NEGATIVE;
            return &last_input;
        }
        if (ry > 10)
        {
            last_input.device = CONTROL_MOUSE;
            last_input.code = CONTROL_MOUSE_Y;
            last_input.joy = 0;
            last_input.half = CONTROL_AXIS_POSITIVE;
            return &last_input;
        }
        if (ry < -10)
        {
            last_input.device = CONTROL_MOUSE;
            last_input.code = CONTROL_MOUSE_Y;
            last_input.joy = 0;
            last_input.half = CONTROL_AXIS_NEGATIVE;
            return &last_input;
        }
        if (rz > 10)
        {
            last_input.device = CONTROL_MOUSE;
            last_input.code = CONTROL_MOUSE_Z;
            last_input.joy = 0;
            last_input.half = CONTROL_AXIS_POSITIVE;
            return &last_input;
        }
        if (rz < -10)
        {
            last_input.device = CONTROL_MOUSE;
            last_input.code = CONTROL_MOUSE_Z;
            last_input.joy = 0;
            last_input.half = CONTROL_AXIS_NEGATIVE;
            return &last_input;
        }
    }
    for (i = joys.size(); i--;)
    {
        int n = 0;
        if (!(flags & LASTINPUT_NO_JOY_BUTTONS))
        {
            uint bits = joys[i].state.buttons[0];
            while (!(bits & 1) && n < 32)
            {
                bits >>= 1;
                n++;
            }
            if (n < 32)
            {
                last_input.device = CONTROL_JOY;
                last_input.code = CONTROL_JOY_BUTTON + n;
                last_input.joy = i;
                return &last_input;
            }
        }
        if (!(flags & LASTINPUT_NO_POVS))
        {
            for (n = 0; n < 4; n++)
            {
                if (!joys[i].state.pov_centered[n])
                {
                    float angle = joys[i].state.pov[n];
                    uint direction;
                    if (angle > 4.69f)
                        direction = CONTROL_POV_LEFT;
                    else if (angle > 3.12f)
                        direction = CONTROL_POV_DOWN;
                    else if (angle > 1.5f)
                        direction = CONTROL_POV_RIGHT;
                    else
                        direction = CONTROL_POV_UP;
                    last_input.device = CONTROL_JOY;
                    last_input.code = (n + 0x20) * 16 + direction;
                    last_input.joy = i;
                    return &last_input;
                }
            }
        }
        if (!(flags & LASTINPUT_NO_JOY_AXES))
        {
            for (int axis = 0; axis < JOY_NUM_AXES; axis++)
            {
                float value = joys[i].joy->GetAxis(axis);
                float start;
                switch (axis)
                {
                case JOY_X:
                    start = joys[i].changes.x;
                    break;
                case JOY_Y:
                    start = joys[i].changes.y;
                    break;
                case JOY_Z:
                    start = joys[i].changes.z;
                    break;
                case JOY_RX:
                    start = joys[i].changes.rx;
                    break;
                case JOY_RY:
                    start = joys[i].changes.ry;
                    break;
                case JOY_RZ:
                    start = joys[i].changes.rz;
                    break;
                case JOY_SLIDER0:
                    start = joys[i].changes.slider[0];
                    break;
                case JOY_SLIDER1:
                    start = joys[i].changes.slider[1];
                    break;
                }
                uint mode = joys[i].joy->GetAxisMode(axis);
                if (!(mode & JOY_AXIS_RAW))
                {
                    if (mode & JOY_AXIS_RELATIVE)
                    {
                        if (mode & JOY_AXIS_POSITIVE ? fabs(value) > .3f : fabs(value) > .5f)
                        {
                            last_input.device = CONTROL_JOY;
                            last_input.code = axis;
                            last_input.joy = i;
                            if (value > 0.0f)
                                last_input.half = CONTROL_AXIS_POSITIVE;
                            else
                                last_input.half = CONTROL_AXIS_NEGATIVE;
                            return &last_input;
                        }
                    }
                    else if (mode & JOY_AXIS_POSITIVE ? fabs(value - start) > .5f : fabs(value - start) > .7f)
                    {
                        last_input.device = CONTROL_JOY;
                        last_input.code = axis;
                        last_input.joy = i;
                        if (value > start)
                            last_input.half = CONTROL_AXIS_POSITIVE;
                        else
                            last_input.half = CONTROL_AXIS_NEGATIVE;
                        return &last_input;
                    }
                }
                else if (!(flags & LASTINPUT_NO_RAW_AXES) && value != start)
                {
                    last_input.device = CONTROL_JOY;
                    last_input.code = axis;
                    last_input.joy = i;
                    if (value > 0.0f)
                        last_input.half = CONTROL_AXIS_POSITIVE;
                    else
                        last_input.half = CONTROL_AXIS_NEGATIVE;
                    return &last_input;
                }
            }
        }
    }
    return NULL;
}

static void WideToChar(char *dst, const wchar_t *src, int size);

// The texts of the mouse's names (the names are ours, after the Czech database's texts: the English one has none, so
// GetText gives its " ")
#define TEXT_MOUSE_LEFT 0x1ceb
#define TEXT_MOUSE_RIGHT 0x1cec
#define TEXT_MOUSE_MIDDLE 0x1ced
#define TEXT_MOUSE_X 0x1cee
#define TEXT_MOUSE_Y 0x1cef
#define TEXT_MOUSE_Z 0x1cf0
#define TEXT_JOY_NOT_CONNECTED 0x1cf1

// A joystick's axes have codes below 0xff, its buttons above it (CONTROL_JOY_BUTTON), its POVs from CONTROL_JOY_POV.
// The key's code goes through a byte local (the original's stores it and reads it back), the POV's number through a
// masked local: written into the call, VC6 shifts first and masks then.
// FUNCTION: GAME 0x4f1830
const char *C_controls::GetControlName(const S_control *control)
{
    if (!control)
        return NULL;
    memset(name, 0, sizeof(name));
    switch (control->device)
    {
    case CONTROL_NONE:
        return NULL;
    case CONTROL_KEY:
        {
            uchar key = (uchar)control->code;
            WideToChar(name, (const wchar_t *)igraph->GetKeyName(key), sizeof(name) - 1);
        }
        return name;
    case CONTROL_MOUSE:
        switch (control->code)
        {
        case MBUT_LEFT:
            return textDatabase.GetText(TEXT_MOUSE_LEFT);
        case MBUT_RIGHT:
            return textDatabase.GetText(TEXT_MOUSE_RIGHT);
        case MBUT_MIDDLE:
            return textDatabase.GetText(TEXT_MOUSE_MIDDLE);
        case CONTROL_MOUSE_X:
            return textDatabase.GetText(TEXT_MOUSE_X);
        case CONTROL_MOUSE_Y:
            return textDatabase.GetText(TEXT_MOUSE_Y);
        case CONTROL_MOUSE_Z:
            return textDatabase.GetText(TEXT_MOUSE_Z);
        }
        break;
    case CONTROL_JOY:
        if (joys.size() > control->joy)
        {
            if (control->code < 0xff)
                return joys[control->joy].joy->GetAxisName(control->code);
            if (control->code > 0xff && control->code < CONTROL_JOY_POV)
                return joys[control->joy].joy->GetButtonName(control->code & 0xff);
            if (control->code >= CONTROL_JOY_POV)
            {
                uint pov = control->code & 0xf0;
                return joys[control->joy].joy->GetPOVName(pov >> 4);
            }
        }
        else
            return textDatabase.GetText(TEXT_JOY_NOT_CONNECTED);
        break;
    }
    return NULL;
}

// A wide text into dst, at most size characters and the zero (the name is ours)
// FUNCTION: GAME 0x4f1a30
static void WideToChar(char *dst, const wchar_t *src, int size)
{
    if (dst && src)
    {
        int len = wcslen(src);
        if (len > size)
            len = size;
        WideCharToMultiByte(CP_ACP, 0, src, -1, dst, len, NULL, NULL);
        dst[len] = 0;
    }
}
