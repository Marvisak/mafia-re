#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3d_math.h"

class C_ParticleType;
class I3D_part_base;

// One particle of the effects' emitters (the PC's own; the name is PS2's C_Particle, after the file, whose class the
// PC's need not be). C_effects' particle_pool keeps the free ones, which C_ParticleType's destructor and Reset give back
// to it. 0x64 bytes (C_ParticleEmitter::CreateParticle's new).
class C_Particle
{
public:
    I3D_part_base *visual;              // 0x0, the particle's (Init makes it once), released by the destructor
    C_ParticleType *type;               // 0x4, the emitter's part it belongs to
    int unknown_8;                      // 0x8, the part's subpart it is at (SetPhase)
    S_vector unknown_c;                 // 0xc, its direction times its speed, per second
    S_quat unknown_18;                  // 0x18, its direction when the subpart started, as a rotation
    S_quat unknown_28;                  // 0x28, the subpart's direction, which Tick turns it to
    bool unknown_38;                    // 0x38, it turns from unknown_18 to unknown_28
    int unknown_3c;                     // 0x3c, the time the subpart has left, in ms
    uchar unknown_40[4];                // 0x40
    int unknown_44;                     // 0x44, the part's flags (PARTF_*)
    uchar unknown_48[4];                // 0x48
    int unknown_4c;                     // 0x4c, the subpart's time
    S_vector unknown_50;                // 0x50, how the visual's scale changes, per ms
    float unknown_5c;                   // 0x5c, how its transparency changes, per ms
    float unknown_60;                   // 0x60, how the speed changes (a fraction of it), per second

    C_Particle()
    {
        visual = NULL;
        type = NULL;
        unknown_8 = 0;
        unknown_3c = 0;
        unknown_44 = 0;
        unknown_60 = 0.0f;
        unknown_5c = 0.0f;
    }

    ~C_Particle();
    // The particle of the emitter's part at the position and in the direction, its visual made the first time: false
    // if it can't be (no visual, no part or no subparts). The id is the emitter's, for the visual; time is what the
    // particle has already lived (the name is ours).
    bool Init(ushort id, const S_vector &pos, const S_vector &dir, C_ParticleType *part, int time);
    // Unlinks the visual and clears the particle's state, for particle_pool (the name is ours)
    void Reset();
    // Goes on with the part's subpart phase, taking its values at random within their ranges (the name is ours)
    void SetPhase(int phase);
    // The particle by the time passed (PS2's name): it moves, turns, grows and fades by the subpart's values
    void Tick(int time);
};

// The free particles (C_effects.cpp's, the name is ours)
extern std::vector<C_Particle *> particle_pool;

// The engine's S_vector Cross and operator+= (I3d_math.h), which Tick calls out of line: the original kept the copies
// of C_airplane.cpp's file (docs/game-files.md)
// LIBRARY: GAME 0x40ba00
// ??YS_vector@@QAEAAU0@ABU0@@Z

// LIBRARY: GAME 0x40ba30
// ?Cross@S_vector@@QBE?AU1@ABU1@@Z
