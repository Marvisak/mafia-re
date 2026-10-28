// The force feedback (PS2 C_ff_manager.cpp; the PC's file is most likely the one at 0x444430-0x44525c, which starts
// with its constructor's helpers, docs/game-files.md). Written so far: the constructor, the destructor, Reset, GameInit,
// GameDone and Close.

#include "input/C_ff_manager.h"

#include "input/controls.h"
#include "ui/g_loadsave.h"

// The timer's constructor's 200 stored before the period
// FUNCTION: GAME 0x444630
C_ff_manager::C_ff_manager()
{
    unknown_24.Reset(1000);
}

// The same code as C_sledovacka's destructor (two vectors' destructors): the linker kept this file's copy
// FUNCTION: GAME 0x444670
C_ff_manager::~C_ff_manager()
{
}

// FUNCTION: GAME 0x4446a0
void C_ff_manager::Reset()
{
    unknown_20 = 0;
}

// The effects for the player where his forward control's secondary binding is on a joystick with force feedback, for
// the cars where steering left's is
// FUNCTION: GAME 0x4446b0
void C_ff_manager::GameInit()
{
    S_control binding;
    S_joy_device_info info;
    controls.GetControl(&binding, CTRL_FORWARD + CTRL_SECONDARY);
    if (binding.device == CONTROL_JOY)
    {
        joy[binding.joy]->GetInfo(&info);
        human_effects = info.force_feedback;
    }
    else
        human_effects = false;
    controls.GetControl(&binding, CTRL_STEER_LEFT + CTRL_SECONDARY);
    if (binding.device == CONTROL_JOY)
    {
        joy[binding.joy]->GetInfo(&info);
        car_effects = info.force_feedback;
    }
    else
        car_effects = false;
    unknown_20 = 0;
}

// Frees the effects, stopping the second list's first. The second list emptied by an erase of the whole, whose
// begin and end are inline calls still to come at the first clear: they leave its std::copy and _Destroy out of line,
// as the original (clear() inlines the copy)
// FUNCTION: GAME 0x444740
void C_ff_manager::GameDone()
{
    uint i;
    for (i = 0; i < effects.size(); i++)
    {
        if (effects[i]->effect)
            effects[i]->effect->Release();
        delete effects[i];
    }
    effects.clear();
    for (i = 0; i < effects_2.size(); i++)
    {
        if (effects_2[i]->effect)
        {
            effects_2[i]->effect->Stop(true);
            effects_2[i]->effect->Release();
        }
        delete effects_2[i];
    }
    effects_2.erase(effects_2.begin(), effects_2.end());
}

// Empty: CloseSystem's call goes to the empty function the linker folded the game's into (0x4aabe0, C_ff_manager.h)
void C_ff_manager::Close()
{
}
