// The player's controls (PC only, the name is ours; the file at 0x4efb40-0x4f1b4c, which starts with the class's
// constructor, docs/game-files.md). Written so far: the constructor, the destructor, Close, SetMouseScale, Init,
// ClearControls, GetValues and SetControl; Update is a stub.

#include "input/controls.h"

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

#pragma auto_inline(off)

// STUB: GAME 0x4f0620
void C_controls::Update(bool update_input)
{
    StubLog("C_controls::Update");
}

#pragma auto_inline(on)
