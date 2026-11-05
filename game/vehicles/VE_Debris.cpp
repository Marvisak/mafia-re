// The debris (PS2 VE_Debris.cpp; the PC's file is most likely the one at 0x564890-0x56615c, docs/game-files.md).
// Written so far: Init and Done, which C_game's Init and Done call, and Tick, which C_game::Tick calls.

#include "vehicles/VE_Debris.h"

#include "g_lines.h"

#include "I3D/I3D.h"

#include "stub.h"

// The gravity goes through a temporary, as PS2's
// FUNCTION: GAME 0x564890
bool C_Debris::Init(float gravity)
{
    this->gravity = S_vector(0.0f, -gravity, 0.0f);
    debris.clear();
    flashes.clear();
    return true;
}

// FUNCTION: GAME 0x564920
bool C_Debris::Done()
{
    for (int i = debris.size(); i--;)
    {
        debris[i].frame->LinkTo(NULL, 0);
        debris[i].frame->Release();
    }
    debris.clear();
    flashes.clear();
    return true;
}

// The debris and the flashes by the time passed, clamped to 1-83 ms: a piece of debris falls under the gravity, turns
// by its roll and goes out once it falls faster than 20 m/s, when it is unlinked, released and replaced by the last
// one; a flash is a line whose head flies off at once and whose tail follows after 0.05 s, both drifting, drawn
// through AddLine (PC only) until its time is up. The two removals erase the last element, the debris' out of line
// (0x5660e0), the flashes' inlined.
// FUNCTION: GAME 0x5649d0
void C_Debris::Tick(uint time)
{
    if (time < 1)
        time = 1;
    else if (time > 83)
        time = 83;
    float t = time * .001f;
    int i;
    for (i = debris.size(); i--;)
    {
        S_DEBRIS &d = debris[i];
        S_vector pos = d.frame->GetPos();
        d.velocity += gravity * t;
        pos += d.velocity * t;
        d.frame->SetPos(pos);
        d.roll += d.roll_speed * t;
        d.frame->SetDir(d.frame->GetDir(), d.roll);
        if (d.roll > 6.2831855f)
            d.roll -= 6.2831855f;
        if (d.roll < 0.0f)
            d.roll += 6.2831855f;
        d.frame->Update();
        if (d.velocity.y < -20.0f)
            d.alive = false;
    }
    for (i = debris.size(); i--;)
    {
        if (!debris[i].alive)
        {
            debris[i].frame->LinkTo(NULL, 0);
            debris[i].frame->Release();
            debris[i] = debris.back();
            debris.pop_back();
        }
    }
    for (i = flashes.size(); i--;)
    {
        S_FLASH &f = flashes[i];
        f.head_speed += gravity * t;
        f.head += f.head_speed * t;
        f.time += t;
        if (f.time >= .05f)
        {
            f.tail_speed += gravity * t;
            f.tail += f.tail_speed * t;
        }
        S_vector drift = f.drift * t;
        f.head += drift;
        f.tail += drift;
        AddLine(f.head, f.tail, 0xbfffffae, 0x9fffffae);
    }
    for (i = flashes.size(); i--;)
    {
        if (flashes[i].time > flashes[i].life_time)
        {
            flashes[i] = flashes.back();
            flashes.pop_back();
        }
    }
}

// STUB: GAME 0x565060
void C_Debris::GenerateDebris(const S_vector &pos, int num, float size, float speed, float time, const S_vector *dir)
{
    StubLog("C_Debris::GenerateDebris");
}
