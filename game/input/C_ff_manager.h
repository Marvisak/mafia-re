#pragma once

#include "Main.h"
#include "tools.h"

#include <vector>

#include "ijoy.h"

// The force feedback (PS2 C_ff_manager, C_ff_manager.cpp; the PC's drives the joysticks' effects through IJoy), 0x30
// bytes (Main.cpp's ff_manager, the next global 0x30 after it). The humans and the cars create their effects by id
// and start them; an effect started moves to started_effects, and one that can't start for the effects of a priority
// as high waits in effects, which Tick and DeleteEffect try again.
class C_ff_manager
{
public:
    // What an effect keeps for IJoyEffect::SetEnvelope (the name is a guess: the effect's only parameters IJoy takes that
    // it has no other member for); nothing sets it but its constructor
    struct S_envelope
    {
        float attack_level;             // 0x0
        uint attack_time;               // 0x4
        float fade_level;               // 0x8
        uint fade_time;                 // 0xc

        S_envelope();
    };

    // An effect, 0x38 bytes (PS2's name, whose S_effect is another design; the members' names are IJoy's): the base of
    // the four kinds below, which CreateEffect creates by its type
    // VTABLE: GAME 0x623d58
    struct S_effect
    {
        S_envelope envelope;            // 0x4
        int type;                       // 0x14, JOY_EFFECT_*: JOY_EFFECT_NONE where a kind's constructor refused it
        float direction;                // 0x18
        float gain;                     // 0x1c
        uint duration;                  // 0x20, in microseconds, 0 for endless: only an endless effect waits
        uint priority;                  // 0x24: a started effect of a lower one gives way
        bool human;                     // 0x28, whose joystick: the human's (true) or the car's
        bool waiting;                   // 0x29, set where it couldn't start, for StartWaiting
        IJoy *joy;                      // 0x2c, the joystick that created effect
        int id;                         // 0x30, which the manager's functions take
        IJoyEffect *effect;             // 0x34, NULL if the joystick couldn't create it

        S_effect(int type, float direction, uint duration, float gain, uint priority, bool human);

        // Sets the joystick effect's parameters (the base's direction, gain and duration; the kinds' their forces)
        virtual void Update();
        // Whether the joystick plays it
        bool IsPlaying();
        // Sets the joystick effect's type with the effect's parameters
        void SetType();

        // The name is ours: CreateEffect's return reads the id through an inline call, which leaves the push_back
        // before it insert out of line as the original
        int GetId()
        {
            return id;
        }
    };

    // JOY_EFFECT_CONSTANT_FORCE
    // VTABLE: GAME 0x623d68
    struct S_constant_effect: public S_effect
    {
        float magnitude;                // 0x38

        S_constant_effect(float direction, uint duration, float gain, ushort priority, bool human):
            S_effect(JOY_EFFECT_CONSTANT_FORCE, direction, duration, gain, priority, human)
        {
            magnitude = 0.0f;
        }
        void Update();
    };

    // JOY_EFFECT_RAMP_FORCE
    // VTABLE: GAME 0x623d64
    struct S_ramp_effect: public S_effect
    {
        float start;                    // 0x38
        float end;                      // 0x3c

        S_ramp_effect(float direction, uint duration, float gain, ushort priority, bool human):
            S_effect(JOY_EFFECT_RAMP_FORCE, direction, duration, gain, priority, human)
        {
            start = 0.0f;
            end = 0.0f;
        }
        void Update();
    };

    // JOY_EFFECT_SQUARE to JOY_EFFECT_SAWTOOTH_DOWN
    // VTABLE: GAME 0x623d5c
    struct S_periodic_effect: public S_effect
    {
        float magnitude;                // 0x38
        float offset;                   // 0x3c
        float invert;                   // 0x40, which IJoy turns into a phase
        uint period;                    // 0x44, in microseconds

        S_periodic_effect(int type, float direction, uint duration, float gain, ushort priority, bool human);
        void Update();
    };

    // JOY_EFFECT_SPRING to JOY_EFFECT_FRICTION
    // VTABLE: GAME 0x623d60
    struct S_condition_effect: public S_effect
    {
        S_joy_condition *condition_0;   // 0x38, SetCondition's copies
        S_joy_condition *condition_1;   // 0x3c

        S_condition_effect(int type, float direction, uint duration, float gain, ushort priority, bool human):
            S_effect(type, direction, duration, gain, priority, human)
        {
            condition_0 = NULL;
            condition_1 = NULL;
        }
        void Update();
    };

    // The effects not started, which GameDone releases, and those started, which it stops first (the names are ours)
    std::vector<S_effect *> effects;    // 0x0
    std::vector<S_effect *> started_effects;    // 0x10
    int next_id;                        // 0x20, the id CreateEffect gives next (the name is ours)
    // 0x24, its period 1000 from the constructor: Tick tries the waiting effects each second (the name is ours)
    S_timer wait_timer;
    // 0x2c and 0x2d, which of the effects C_game::Init creates (the names are ours): GameInit sets them where the
    // player's (forward's secondary binding) and the car's controls (steer left's) are on a joystick with force feedback
    bool human_effects;
    bool car_effects;

    C_ff_manager();
    ~C_ff_manager();

    // The name is ours. InitSystem calls it once the joysticks are there.
    void Reset();
    // The name is ours. CloseSystem calls it before it closes the joysticks; it does nothing.
    void Close();
    // PS2 names: what C_game's Init and Done call
    void GameInit();
    void GameDone();
    // An effect of the type, JOY_EFFECT_*, on the joystick of the human's controls or the car's: its id (the names are
    // ours, as C_human::InitFFEffects shows them; the duration in microseconds)
    int CreateEffect(int type, float direction, uint duration, float gain, ushort priority, bool human);
    // The effect of the id, started or not, NULL for none (the name is ours)
    S_effect *GetEffect(int id);
    // The effects' forces by kind, which a playing effect takes at once (the names are ours): a constant force's, a
    // ramp's, a periodic one's, a condition's (copied)
    void SetConstantForce(int id, float magnitude);
    void SetRampForce(int id, float start, float end);
    void SetPeriodicForce(int id, float magnitude, float offset, float invert, uint period);
    void SetCondition(int id, const S_joy_condition *condition_0, const S_joy_condition *condition_1);
    // The effect's gain, direction and duration (in microseconds), set on its joystick effect (the names are ours)
    void SetGain(int id, float gain);
    void SetDirection(int id, float direction);
    void SetDuration(int id, uint duration);
    // Whether the effect's joystick is a wheel (the name is ours)
    bool IsWheel(int id);
    // Starts the effect of the id, moved to started_effects: false if it doesn't start, where another one of a
    // priority as high plays (it then waits) (the name is ours)
    bool Start(int id);
    // Deletes the effect, then starts the waiting ones (the name is ours)
    void DeleteEffect(int id);
    // Stops the started effect of the id, moved back to effects (the name is ours)
    void Stop(int id);
    // The started effect to stop for another: one that has stopped playing first, then the lowest priority (the name
    // is ours)
    S_effect *GetLowest();
    // The waiting effect of the highest priority, NULL for none (the name is ours)
    S_effect *GetWaiting();
    // The effects by the time passed, which C_game::Tick calls (PS2 name)
    void Tick(uint time);
    // Starts the waiting effects, the highest priority first, until one can't start (the name is ours)
    void StartWaiting();

    // Whether the human's or the car's effects are created (the name is ours): C_human::Shooting calls the copy
    // FUNCTION: GAME 0x585b20
    bool HasEffects(bool human)
    {
        return human ? human_effects : car_effects;
    }
};

extern C_ff_manager ff_manager;

// The empty functions of the out-of-line kind the linker folded into one (0x4aabe0): Close above, C_web_path::
// DrawConnections, C_human::HideLegs and ~C_animations_machine_de_luxe, named after the copy our linker keeps (foldfix)
// SYNTHETIC: GAME 0x4aabe0
// ?Death@C_actor@@UAEXXZ

// The destructor, folded with C_sledovacka's, named after the copy our linker keeps (foldfix)
// FUNCTION: GAME 0x444670
// ??1C_sledovacka@@QAE@XZ
