// The player's controls (PC only, the name is ours; the file at 0x4efb40-0x4f1b4c, which starts with the class's
// constructor, docs/game-files.md). Written so far: the constructor, the destructor, Close, SetMouseScale, Init,
// ClearControls, GetValues, GetValue, SetControl, GetControl, SaveJoyStates, GetLastInput and GetControlName; Update is a
// stub.

#include "input/controls.h"
#include "ui/g_textdb.h"

#include <math.h>
#include <string.h>

#include "IGraph/IGraph.h"
#include "stub.h"

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

// STUB: GAME 0x4f0620
void C_controls::Update(bool update_input)
{
    StubLog("C_controls::Update");
}

// Between Update and SaveJoyStates the original has more of the bindings' functions

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
                        if (mode & JOY_AXIS_POSITIVE ? fabs(value) > .5f : fabs(value) > .3f)
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
                    else if (mode & JOY_AXIS_POSITIVE ? fabs(value - start) > .7f : fabs(value - start) > .5f)
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
