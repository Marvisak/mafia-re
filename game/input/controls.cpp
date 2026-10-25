// The player's controls (PC only, the name is ours; the file at 0x4efb40-0x4f1b4c, which starts with the class's
// constructor, docs/game-files.md). Written so far: the constructor, the destructor, Close and Init.

#include "input/controls.h"

#include <string.h>

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
