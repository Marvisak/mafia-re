#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class C_Particle;
class I3D_frame;

// C_ParticleType::flags, a part's flags (the names are ours)
#define PARTF_BILLBOARD 0x1             // the particles turn to the camera (I3D_part_base::billboard)
#define PARTF_TURN 0x2                  // a particle is turned to its direction (C_Particle::Init)
#define PARTF_DIR 0x4                   // the particles start in the effect's direction, else in none
#define PARTF_LOCAL_DIR 0x8             // dir and dir_spread are turned by the parent's matrix
#define PARTF_LOCAL_POS 0x10            // offset and offset_spread are turned by it

// S_particle_subpart::flags, a subpart's flags (the names are ours)
#define SUBPARTF_GRAVITY 0x2            // the particles fall by gravity
#define SUBPARTF_OFF 0x8                // the particles' visuals are off in it
#define SUBPARTF_ROTATE 0x10            // the visuals turn about their axes by rot_speed
#define SUBPARTF_STILL 0x20             // the particles don't move

// A subpart's values in an emitter (the name is ours), 0x64 bytes: a copy of a C_effect_subpart's (C_effects.h, whose
// members have the same names), which Reset sets to the subpart's defaults. C_effects' particle_subparts keeps the free
// ones. The names are ours, and so are the setters', each a group of the values that CreateSParticle copies together:
// its floats go through the FPU, as through an inline function's parameters, and its first three vectors through
// temporaries, as by-value parameters.
struct S_particle_subpart
{
    // C_Particle::SetPhase and Tick's: a value with a range is spread by it at random, either way
    int duration;                       // 0x0, the subpart's time, in ms (the table's subpart's default: 1000)
    int duration_range;                 // 0x4
    int flags;                          // 0x8, SUBPARTF_* (the table's subpart's default: 1)
    S_vector dir;                       // 0xc, the particles' direction in it (null: they keep theirs)
    S_vector dir_spread;                // 0x18
    S_vector rot_speed;                 // 0x24, the visuals' turn about x, y and z, per second (SUBPARTF_ROTATE)
    uchar unknown_30[8];                // 0x30
    S_vector scale;                     // 0x38, the visuals' scale at its end
    S_vector scale_range;               // 0x44
    float transparency;                 // 0x50, their transparency at its end
    float transparency_range;           // 0x54
    float speed_change;                 // 0x58, how the particles' speed changes (C_Particle::speed_change)
    float speed_change_range;           // 0x5c
    float gravity;                      // 0x60, their fall (SUBPARTF_GRAVITY), per second (the default: 9.8)

    // Reset's values, written out: CreateSParticle inlines it, which a call of Reset would leave out of line
    S_particle_subpart()
    {
        duration = 0;
        duration_range = 0;
        flags = 1;
        scale = S_vector(1.0f, 1.0f, 1.0f);
        scale_range = S_vector(0.0f, 0.0f, 0.0f);
        transparency = 0.0f;
        transparency_range = 0.0f;
        speed_change = 0.0f;
        speed_change_range = 0.0f;
        gravity = 9.8f;
    }

    // Declared and empty: deleting one tests the pointer, which a structure without a destructor doesn't
    ~S_particle_subpart() {}

    // The defaults, which C_ParticleType::Reset inlines
    void Reset()
    {
        duration = 0;
        duration_range = 0;
        flags = 1;
        scale = S_vector(1.0f, 1.0f, 1.0f);
        scale_range = S_vector(0.0f, 0.0f, 0.0f);
        transparency = 0.0f;
        transparency_range = 0.0f;
        speed_change = 0.0f;
        speed_change_range = 0.0f;
        gravity = 9.8f;
    }

    // Whether the particles' visuals are on in the subpart (the name is ours). C_Particle::Init tests it through this:
    // written out, VC6 tests the byte (test byte ptr [ecx + 8], 8) where the original shifts the flags first.
    bool IsOn() const
    {
        return !(flags & SUBPARTF_OFF);
    }

    void SetValues(int v0, int v4, int v8, S_vector vc, S_vector v18, S_vector v24)
    {
        duration = v0;
        duration_range = v4;
        flags = v8;
        dir = vc;
        dir_spread = v18;
        rot_speed = v24;
    }

    void SetTransparency(float f50, float f54)
    {
        transparency = f50;
        transparency_range = f54;
    }

    void SetMotion(float f58, float f5c, float f60)
    {
        speed_change = f58;
        speed_change_range = f5c;
        gravity = f60;
    }
};

// An emitter's part, one of an effect's parts at work (the names are ours, after PS2's C_ParticleEmitter, which keeps a
// vector of C_ParticleType), 0xc0 bytes: the values of a C_effect_part (C_effects.h, whose members have the same
// names), its subparts' and its particles. C_effects' particle_parts keeps the free ones. The destructor and Reset give
// the subparts back to particle_subparts and the particles to particle_pool.
class C_ParticleType
{
public:
    uchar unknown_0[4];                 // 0x0
    std::vector<S_particle_subpart *> subparts;  // 0x4
    I3D_frame *frame;                   // 0x14, the part's frame of the effect's model, a reference of its own
    I3D_frame *parent;                  // 0x18, the frame the effect is at (LoadStaticParticles')
    std::vector<C_Particle *> particles;  // 0x1c
    int blend;                          // 0x2c, the visuals' blend (1 << it: PARTF_OPAQUE, PARTF_ALPHA)
    // C_Particle::Init's: each value with the range a particle's spreads it by, at random either way
    S_vector offset;                    // 0x30, the particles' offset from the position
    S_vector offset_spread;             // 0x3c
    S_vector dir;                       // 0x48, their direction, else the effect's (null)
    S_vector dir_spread;                // 0x54
    float speed;                        // 0x60, their speed
    float speed_spread;                 // 0x64
    S_vector scale;                     // 0x68, their visuals' scale
    S_vector scale_spread;              // 0x74
    float transparency;                 // 0x80, their visuals' transparency
    float transparency_spread;          // 0x84
    int burst;                          // 0x88, the particles the part makes at once when it (re)starts
    float rate;                         // 0x8c, the particles it makes per second, which falls after fall_delay
    float start_rate;                   // 0x90, rate's first value, which a restart sets again
    float rate_fall;                    // 0x94, how much rate falls per second
    float interval;                     // 0x98, 1000 / rate, the time between two particles in ms, 0 for 0
    float fall_interval;                // 0x9c, 1000 / rate_fall, 0 for 0
    int fall_delay;                     // 0xa0, the time at the full rate, in ms (an integer of the table's float)
    int run_time;                       // 0xa4, the time since the part (re)started, in ms
    int restart_delay;                  // 0xa8, the wait before a restart, in ms (an integer)
    int wait;                           // 0xac, the wait left before the part makes particles: -1 from Init
    int repeats;                        // 0xb0, how many times the part runs, -1 for ever (an integer)
    int emit_time;                      // 0xb4, the time since the last particle, in ms
    bool forever;                       // 0xb8, repeats is -1
    int flags;                          // 0xbc, PARTF_*

    C_ParticleType() : frame(NULL)
    {
        speed = 0.0f;
        speed_spread = 0.0f;
        scale = S_vector(1.0f, 1.0f, 1.0f);
        scale_spread = S_vector(0.0f, 0.0f, 0.0f);
        burst = 0;
        rate = 0.0f;
        start_rate = 0.0f;
        rate_fall = 0.0f;
        interval = 0.0f;
        fall_interval = 0.0f;
        restart_delay = 0;
        wait = 0;
        repeats = 0;
        emit_time = 0;
        forever = false;
        flags = 0;
    }

    ~C_ParticleType();
    // The part's values, its frame (with a reference) and the effect's frame
    void Init(I3D_frame *at, I3D_frame *frm, const S_vector &v30, const S_vector &v3c, int v88, float v8c,
        float v94, int va0, int vb0, int va8);
    // Gives the subparts and the particles back and clears the values, for particle_parts (the name is ours)
    void Reset();

    void SetSpeed(float f60, float f64)
    {
        speed = f60;
        speed_spread = f64;
    }

    void SetTransparency(float f80, float f84)
    {
        transparency = f80;
        transparency_spread = f84;
    }
};

// The free parts and subparts (C_effects.cpp's, the names are ours)
extern std::vector<C_ParticleType *> particle_parts;
extern std::vector<S_particle_subpart *> particle_subparts;
