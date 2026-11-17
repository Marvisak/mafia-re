// The force feedback (PS2 C_ff_manager.cpp, another design there; the PC's file is most likely the one at
// 0x444430-0x44525c, docs/game-files.md): the effects' classes, then the manager. An effect is created on the joystick
// of the human's controls or the car's, kept in effects until it starts and in started_effects while it plays; one
// that can't start for the effects of a priority as high waits (only an endless one) until Tick or DeleteEffect tries
// it again.

#include "input/C_ff_manager.h"

#include "input/controls.h"
#include "ui/g_loadsave.h"

// FUNCTION: GAME 0x444430
C_ff_manager::S_envelope::S_envelope()
{
    attack_level = 0.0f;
    attack_time = 0;
    fade_level = 0.0f;
    fade_time = 0;
}

// VC6 stores the body's two zeros before the vtable and human after it; human as an initializer would come before it
// FUNCTION: GAME 0x444440
C_ff_manager::S_effect::S_effect(int type, float direction, uint duration, float gain, uint priority, bool human):
    type(type), direction(direction), gain(gain), duration(duration), priority(priority)
{
    effect = NULL;
    waiting = false;
    this->human = human;
}

// FUNCTION: GAME 0x444490
void C_ff_manager::S_effect::Update()
{
    if (effect)
    {
        effect->SetDirection(direction, true);
        effect->SetGain(gain, true);
        effect->SetDuration(duration, true);
    }
}

// FUNCTION: GAME 0x4444d0
bool C_ff_manager::S_effect::IsPlaying()
{
    uint status = 0;
    if (effect)
    {
        effect->GetStatus(&status);
        if (status == JOY_EFFECT_PLAYING)
            return true;
    }
    return false;
}

// FUNCTION: GAME 0x444500
void C_ff_manager::S_effect::SetType()
{
    if (effect)
        effect->SetType(type, direction, duration, gain, NULL, NULL);
}

// FUNCTION: GAME 0x444530
void C_ff_manager::S_constant_effect::Update()
{
    if (effect)
        effect->SetConstantForce(magnitude, true);
}

// FUNCTION: GAME 0x444550
void C_ff_manager::S_ramp_effect::Update()
{
    if (effect)
        effect->SetRampForce(start, end, true);
}

// A type that isn't periodic becomes JOY_EFFECT_NONE, which the manager's functions skip
// FUNCTION: GAME 0x444570
C_ff_manager::S_periodic_effect::S_periodic_effect(int type, float direction, uint duration, float gain,
    ushort priority, bool human): S_effect(type, direction, duration, gain, priority, human)
{
    if (this->type != JOY_EFFECT_SQUARE && this->type != JOY_EFFECT_SINE && this->type != JOY_EFFECT_TRIANGLE
        && this->type != JOY_EFFECT_SAWTOOTH_UP && this->type != JOY_EFFECT_SAWTOOTH_DOWN)
        this->type = JOY_EFFECT_NONE;
    magnitude = 0.0f;
    offset = 0.0f;
    invert = 0.0f;
    period = 0;
}

// FUNCTION: GAME 0x4445e0
void C_ff_manager::S_periodic_effect::Update()
{
    if (effect)
        effect->SetPeriodic(magnitude, offset, invert, period, true);
}

// FUNCTION: GAME 0x444610
void C_ff_manager::S_condition_effect::Update()
{
    if (effect)
        effect->SetCondition(condition_0, condition_1, true);
}

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
    next_id = 0;
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
    next_id = 0;
}

// Frees the effects, stopping the started ones first. The second list emptied by an erase of the whole, whose begin
// and end are inline calls still to come at the first clear: they leave its std::copy and _Destroy out of line, as the
// original (clear() inlines the copy)
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
    for (i = 0; i < started_effects.size(); i++)
    {
        if (started_effects[i]->effect)
        {
            started_effects[i]->effect->Stop(true);
            started_effects[i]->effect->Release();
        }
        delete started_effects[i];
    }
    started_effects.erase(started_effects.begin(), started_effects.end());
}

// Empty: CloseSystem's call goes to the empty function the linker folded the game's into (0x4aabe0, C_ff_manager.h)
void C_ff_manager::Close()
{
}

// An unknown type leaves e NULL, which the original writes through. The joystick effect gets no null test before
// its ApplyTypeSpecific either. The human's and the car's joysticks are written out each, as the original's two
// blocks show, joined by VC6 from the CreateEffect call on; joy_effect declared in each takes the dead human's slot.
// FUNCTION: GAME 0x444810
int C_ff_manager::CreateEffect(int type, float direction, uint duration, float gain, ushort priority, bool human)
{
    S_effect *e = NULL;
    switch (type)
    {
    case JOY_EFFECT_CONSTANT_FORCE:
        e = new S_constant_effect(direction, duration, gain, priority, human);
        e->id = next_id++;
        break;
    case JOY_EFFECT_RAMP_FORCE:
        e = new S_ramp_effect(direction, duration, gain, priority, human);
        e->id = next_id++;
        break;
    case JOY_EFFECT_SQUARE:
    case JOY_EFFECT_SINE:
    case JOY_EFFECT_TRIANGLE:
    case JOY_EFFECT_SAWTOOTH_UP:
    case JOY_EFFECT_SAWTOOTH_DOWN:
        e = new S_periodic_effect(type, direction, duration, gain, priority, human);
        e->id = next_id++;
        break;
    case JOY_EFFECT_SPRING:
    case JOY_EFFECT_DAMPER:
    case JOY_EFFECT_INERTIA:
    case JOY_EFFECT_FRICTION:
        e = new S_condition_effect(type, direction, duration, gain, priority, human);
        e->id = next_id++;
        break;
    }
    e->joy = NULL;
    S_control binding;
    if (e->human)
    {
        controls.GetControl(&binding, CTRL_FORWARD + CTRL_SECONDARY);
        if (binding.device == CONTROL_JOY)
        {
            IJoyEffect *joy_effect;
            if (joy[binding.joy]->CreateEffect(&joy_effect))
            {
                e->effect = joy_effect;
                e->joy = joy[binding.joy];
            }
        }
    }
    else
    {
        controls.GetControl(&binding, CTRL_STEER_LEFT + CTRL_SECONDARY);
        if (binding.device == CONTROL_JOY)
        {
            IJoyEffect *joy_effect;
            if (joy[binding.joy]->CreateEffect(&joy_effect))
            {
                e->effect = joy_effect;
                e->joy = joy[binding.joy];
            }
        }
    }
    effects.push_back(e);
    e->SetType();
    e->effect->ApplyTypeSpecific();
    return e->GetId();
}

// FUNCTION: GAME 0x444a70
C_ff_manager::S_effect *C_ff_manager::GetEffect(int id)
{
    uint i;
    for (i = 0; i < effects.size(); i++)
    {
        if (effects[i]->id == id)
            return effects[i];
    }
    for (i = 0; i < started_effects.size(); i++)
    {
        if (started_effects[i]->id == id)
            return started_effects[i];
    }
    return NULL;
}

// FUNCTION: GAME 0x444ae0
void C_ff_manager::SetConstantForce(int id, float magnitude)
{
    S_effect *e = GetEffect(id);
    if (e && e->type == JOY_EFFECT_CONSTANT_FORCE)
    {
        ((S_constant_effect *)e)->magnitude = magnitude;
        if (e->IsPlaying())
            e->Update();
    }
}

// FUNCTION: GAME 0x444b20
void C_ff_manager::SetRampForce(int id, float start, float end)
{
    S_effect *e = GetEffect(id);
    if (e && e->type == JOY_EFFECT_RAMP_FORCE)
    {
        ((S_ramp_effect *)e)->start = start;
        ((S_ramp_effect *)e)->end = end;
        if (e->IsPlaying())
            e->Update();
    }
}

// FUNCTION: GAME 0x444b60
void C_ff_manager::SetPeriodicForce(int id, float magnitude, float offset, float invert, uint period)
{
    S_effect *e = GetEffect(id);
    if (e && (e->type == JOY_EFFECT_SQUARE || e->type == JOY_EFFECT_SINE || e->type == JOY_EFFECT_TRIANGLE
        || e->type == JOY_EFFECT_SAWTOOTH_UP || e->type == JOY_EFFECT_SAWTOOTH_DOWN))
    {
        ((S_periodic_effect *)e)->magnitude = magnitude;
        ((S_periodic_effect *)e)->offset = offset;
        ((S_periodic_effect *)e)->invert = invert;
        ((S_periodic_effect *)e)->period = period;
        if (e->IsPlaying())
            e->Update();
    }
}

// The conditions are copied, the old copies left behind
// FUNCTION: GAME 0x444bc0
void C_ff_manager::SetCondition(int id, const S_joy_condition *condition_0, const S_joy_condition *condition_1)
{
    S_effect *e = GetEffect(id);
    if (e && (e->type == JOY_EFFECT_SPRING || e->type == JOY_EFFECT_DAMPER || e->type == JOY_EFFECT_INERTIA
        || e->type == JOY_EFFECT_FRICTION))
    {
        S_joy_condition *c0 = new S_joy_condition;
        S_joy_condition *c1 = new S_joy_condition;
        *c0 = *condition_0;
        *c1 = *condition_1;
        ((S_condition_effect *)e)->condition_0 = c0;
        ((S_condition_effect *)e)->condition_1 = c1;
        if (e->IsPlaying())
            e->Update();
    }
}

// FUNCTION: GAME 0x444c40
void C_ff_manager::SetGain(int id, float gain)
{
    S_effect *e = GetEffect(id);
    if (e && e->type != JOY_EFFECT_NONE)
    {
        e->gain = gain;
        if (e->effect)
            e->effect->SetGain(gain, true);
    }
}

// FUNCTION: GAME 0x444c70
void C_ff_manager::SetDirection(int id, float direction)
{
    S_effect *e = GetEffect(id);
    if (e && e->type != JOY_EFFECT_NONE)
    {
        e->direction = direction;
        if (e->effect)
            e->effect->SetDirection(direction, true);
    }
}

// FUNCTION: GAME 0x444ca0
void C_ff_manager::SetDuration(int id, uint duration)
{
    S_effect *e = GetEffect(id);
    if (e && e->type != JOY_EFFECT_NONE)
    {
        e->duration = duration;
        if (e->effect)
            e->effect->SetDuration(duration, true);
    }
}

// The id isn't tested
// FUNCTION: GAME 0x444ce0
bool C_ff_manager::IsWheel(int id)
{
    S_effect *e = GetEffect(id);
    S_joy_device_info info;
    if (e->joy)
    {
        e->joy->GetInfo(&info);
        if (info.type == DIDEVTYPEJOYSTICK_WHEEL)
            return true;
    }
    return false;
}

// A playing effect starts again from its start. Where the joystick has no room, the started effect GetLowest picks is
// stopped as long as its priority is lower or it has stopped playing, and the start tried again. Every exit reads
// started from its slot, the original's single return VC6 copied into them: the waiting ones jump to it.
// FUNCTION: GAME 0x444d20
bool C_ff_manager::Start(int id)
{
    bool started = false;
    S_effect *e = GetEffect(id);
    if (e && e->type != JOY_EFFECT_NONE)
    {
        if (e->IsPlaying() && e->effect)
            e->effect->Stop(true);
        if (!e->IsPlaying() && e->effect)
        {
            e->Update();
            while (true)
            {
                if (e->effect->Start(0, false))
                    break;
                S_effect *lowest = GetLowest();
                if (!lowest)
                {
                    e->waiting = true;
                    goto done;
                }
                if (lowest->priority >= e->priority && lowest->IsPlaying())
                {
                    e->waiting = true;
                    goto done;
                }
                Stop(lowest->id);
            }
            started = true;
            for (std::vector<S_effect *>::iterator it = effects.begin(); it != effects.end(); it++)
            {
                if (*it == e)
                {
                    effects.erase(it);
                    started_effects.push_back(e);
                    break;
                }
            }
        }
    }
done:
    return started;
}

// Then the waiting effects may start
// FUNCTION: GAME 0x444fb0
void C_ff_manager::DeleteEffect(int id)
{
    std::vector<S_effect *>::iterator it;
    for (it = effects.begin(); it != effects.end(); it++)
    {
        if ((*it)->id == id)
        {
            if ((*it)->effect)
                (*it)->effect->Release();
            delete *it;
            effects.erase(it);
            break;
        }
    }
    for (it = started_effects.begin(); it != started_effects.end(); it++)
    {
        if ((*it)->id == id)
        {
            if ((*it)->effect)
            {
                (*it)->effect->Stop(true);
                (*it)->effect->Release();
            }
            delete *it;
            started_effects.erase(it);
            break;
        }
    }
    StartWaiting();
}

// A started effect has a joystick effect, which isn't tested
// FUNCTION: GAME 0x445080
void C_ff_manager::Stop(int id)
{
    for (std::vector<S_effect *>::iterator it = started_effects.begin(); it != started_effects.end(); it++)
    {
        if ((*it)->id == id)
        {
            (*it)->effect->Stop(true);
            effects.push_back(*it);
            started_effects.erase(it);
            break;
        }
    }
}

// Of two effects both playing or both not, the one of the lower priority; else the one not playing
// FUNCTION: GAME 0x4450e0
C_ff_manager::S_effect *C_ff_manager::GetLowest()
{
    if (!started_effects.size())
        return NULL;
    S_effect *lowest = started_effects[0];
    uint priority = lowest->priority;
    for (uint i = 1; i < started_effects.size(); i++)
    {
        if (!started_effects[i]->IsPlaying() && !lowest->IsPlaying()
            || started_effects[i]->IsPlaying() && lowest->IsPlaying())
        {
            if (started_effects[i]->priority < priority)
            {
                priority = started_effects[i]->priority;
                lowest = started_effects[i];
            }
        }
        else if (!started_effects[i]->IsPlaying())
        {
            priority = started_effects[i]->priority;
            lowest = started_effects[i];
        }
    }
    return lowest;
}

// Of the effects not started; one that isn't waiting, or has a duration (isn't endless), isn't taken
// FUNCTION: GAME 0x4451a0
C_ff_manager::S_effect *C_ff_manager::GetWaiting()
{
    uint best = 0;
    S_effect *found = NULL;
    for (uint i = 0; i < effects.size(); i++)
    {
        S_effect *e = effects[i];
        if (e->priority > best && e->waiting && !e->duration)
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
