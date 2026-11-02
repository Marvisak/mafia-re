// The force feedback (PS2 C_ff_manager.cpp; the PC's file is most likely the one at 0x444430-0x44525c, which starts
// with its constructor's helpers, docs/game-files.md). Written so far: the constructor, the destructor, Reset, GameInit,
// GameDone and Close, and Tick, which C_game::Tick calls, with StartWaiting and GetWaiting, which Tick runs; Start,
// which StartWaiting calls for a waiting effect (none in the main menu), is a stub.

#include "input/C_ff_manager.h"

#include "input/controls.h"
#include "ui/g_loadsave.h"
#include "stub.h"

// The timer's constructor's 200 stored before the period
// FUNCTION: GAME 0x444630
C_ff_manager::C_ff_manager()
{
    wait_timer.Reset(1000);
}

// The same code as C_sledovacka's destructor (two vectors' destructors): the linker kept this file's copy, ours keeps
// either, so the marker is by name (C_ff_manager.h)
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

// Of the effects not started; one that isn't waiting, or has unknown_20 set, isn't taken
// FUNCTION: GAME 0x4451a0
C_ff_manager::S_effect *C_ff_manager::GetWaiting()
{
    uint best = 0;
    S_effect *found = NULL;
    for (uint i = 0; i < effects.size(); i++)
    {
        S_effect *e = effects[i];
        if (e->priority > best && e->waiting && !e->unknown_20)
        {
            best = e->priority;
            found = e;
        }
    }
    return found;
}

// The timer is advanced by a second whatever the time passed, so the waiting effects are tried from the second tick on
// FUNCTION: GAME 0x4451e0
void C_ff_manager::Tick(uint time)
{
    if (wait_timer.Tick(1000))
        StartWaiting();
}

// FUNCTION: GAME 0x445200
void C_ff_manager::StartWaiting()
{
    S_effect *e = GetWaiting();
    bool started = true;
    while (e && started)
    {
        started = Start(e->id);
        e = GetWaiting();
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x444d20
bool C_ff_manager::Start(int id)
{
    StubLog("C_ff_manager::Start");
    return false;
}

#pragma auto_inline(on)
