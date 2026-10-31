// The broken glass (PS2 VE_BreakageGlass.cpp; the PC's file is most likely the one at 0x5605c0-0x56488c, after
// tshift.cpp as on PS2, docs/game-files.md). Written so far: Init and Done, which C_game's Init and Done call, and
// Tick, which C_game::Tick calls. PrepareGlassSingleObject (0x5605c0), which Tick calls, is a stub.

#include "vehicles/VE_BreakageGlass.h"

#include "collision/g_colls.h"
#include "mission/Mission.h"

#include "I3D/I3D.h"
#include "stub.h"

// The game's S_vector and S_plane functions (I3d_math.h declares them for the game only): defined in
// I3d_math.h, after the zero vector, they move the /YX deferred items of seven game files (Main.cpp,
// C_effects.cpp, C_program.cpp, game.cpp, Mission.cpp, tools.cpp, g_menu.cpp).

// The distance from the plane through p with the normal n (Insanity3D's). Tick inlines the first Dot and calls
// the second.
inline float S_vector::DistanceToPlane(const S_vector &p, const S_vector &n) const
{
    return Dot(n) - p.Dot(n);
}

// Insanity3D's Intersection with its IntersectionPosition written in, f taking the position along the line: Tick keeps
// both in one slot. As Insanity3D's two functions, IntersectionPosition stays out of line there.
inline bool S_plane::Intersection(const S_vector &pos, const S_vector &dir, S_vector &ret) const
{
    float f = normal.Dot(dir);
    if (fabs(f) < MRG_ZERO)
        return false;
    f = -(normal.Dot(pos) + d) / f;
    ret = pos + dir * f;
    return true;
}

// The gravity goes through a temporary, as PS2's
// FUNCTION: GAME 0x560db0
bool C_Break_plain_glass::Init(int num)
{
    gravity = S_vector(0.0f, -8.0f, 0.0f);
    pool_size = num;
    pool_last = num;
    pool = new S_DICE[num];
    if (!pool)
        return false;
    memset(pool, 0, pool_size * sizeof(S_DICE));
    dices.clear();
    return true;
}

// The pool's panes and the broken ones alike, from the last one, as PS2's: the frames unlinked and released, the pieces
// released and freed. The broken ones are indexed through begin(), as the pool's pointer: the vector's operator[] adds
// the index's offset to the array the other way round.
// FUNCTION: GAME 0x560f40
bool C_Break_plain_glass::Done()
{
    if (pool)
    {
        for (int i = pool_size; i--;)
        {
            if (pool[i].frames.size())
            {
                for (int j = pool[i].frames.size(); j--;)
                {
                    pool[i].frames[j]->LinkTo(NULL, 0);
                    pool[i].frames[j]->Release();
                }
                if (pool[i].objs)
                    delete[] pool[i].objs;
            }
            else if (pool[i].objs)
            {
                for (int j = pool[i].num_objs; j--;)
                {
                    if (pool[i].objs[j].frame)
                        pool[i].objs[j].frame->Release();
                }
                delete[] pool[i].objs;
            }
        }
        delete[] pool;
    }
    int i;
    for (i = dices.size(); i--;)
    {
        if (dices.begin()[i].frames.size())
        {
            for (int j = dices.begin()[i].frames.size(); j--;)
            {
                dices.begin()[i].frames[j]->LinkTo(NULL, 0);
                dices.begin()[i].frames[j]->Release();
            }
            if (dices.begin()[i].objs)
                delete[] dices.begin()[i].objs;
        }
        else if (dices.begin()[i].objs)
        {
            for (int j = dices.begin()[i].num_objs; j--;)
            {
                if (dices.begin()[i].objs[j].frame)
                    dices.begin()[i].objs[j].frame->Release();
            }
            delete[] dices.begin()[i].objs;
        }
    }
    dices.clear();
    for (i = frames.size(); i--;)
    {
        frames[i]->SetOn(true);
        frames[i]->Release();
    }
    frames.clear();
    return true;
}

// The broken panes' pieces, by the time passed, clamped to 1-83 ms (as the debris'): a falling piece falls under the
// gravity, finds the ground below it once (a line down to TestLineV, its point raised 5 cm), bounces off it when it
// passes it (put back above the plane, its speed cut to 0.4 and its direction scattered around the normal; below 2 m/s
// it stops falling) and turns by its roll; a piece that stopped is laid on the ground along the normal and goes out.
// A pane whose pieces are all out is made a single object again (PrepareGlassSingleObject) and goes into the pool in
// place of the oldest one there, which is released, the last pane taking its place in dices. The PS2 build also fades
// the pieces out and has the distance to the ground as a constant 1.0f, so its pieces never bounce.
// The distance is Insanity3D's S_vector::DistanceToPlane: written out, or through a free helper, the bounce's sums load
// their terms in the other order. speed initialized before the test (PS2's too) is source the inline budget needs:
// without it the frames' vector assignment of the last pane's calls one more size() out of line. The original still
// had about two statements more than this: the last pane's assignment inlines one size() more there, and the erase of
// the last pane inlines S_DICE's destructor (the same instructions, calling the vector's destructor directly).
// FUNCTION: GAME 0x5611b0
void C_Break_plain_glass::Tick(uint time)
{
    if (time < 1)
        time = 1;
    else if (time > 83)
        time = 83;
    float t = time * .001f;
    for (int i = dices.size(); i--;)
    {
        S_DICE &dice = dices[i];
        int dead = 0;
        for (int j = dice.num_objs; j--;)
        {
            if (dice.objs[j].active)
            {
                if (dice.objs[j].falling)
                {
                    dice.objs[j].velocity += gravity * t;
                    if (dice.objs[j].velocity.y < -50.0f)
                    {
                        dice.objs[j].active = false;
                        continue;
                    }
                    S_vector move = dice.objs[j].velocity * t;
                    S_vector pos = dice.objs[j].frame->GetPos();
                    if (!dice.objs[j].ground_tested && dice.objs[j].velocity.y < 0.0f)
                    {
                        dice.objs[j].ground = pos + S_vector(0.0f, -10000.0f, 0.0f);
                        dice.objs[j].ground_normal = S_vector(0.0f, 1.0f, 0.0f);
                        gifCollision.TestLineV(pos, S_vector(0.0f, -500.0f, 0.0f), &dice.objs[j].ground,
                            &dice.objs[j].ground_normal, 0xffff, 0);
                        dice.objs[j].ground.y += .05f;
                        dice.objs[j].ground_tested = true;
                    }
                    pos += move;
                    float dist = pos.DistanceToPlane(dice.objs[j].ground, dice.objs[j].ground_normal);
                    float speed = 0.0f;
                    if (dist < 0.0f)
                    {
                        speed = dice.objs[j].velocity.Magnitude();
                        S_plane pl;
                        pl.normal = dice.objs[j].ground_normal;
                        pl.d = -dice.objs[j].ground.Dot(pl.normal);
                        S_vector from = pos + pl.normal * (dist * -2.0f);
                        S_vector dir = pl.normal * (dist * 3.0f);
                        pl.Intersection(from, dir, pos);
                        pos += pl.normal * .02f;
                        if (speed < 2.0f)
                        {
                            dice.objs[j].frame->SetPos(pos);
                            dice.objs[j].falling = false;
                            continue;
                        }
                        speed *= .4f;
                        // The normal turned two ways, each by a random -1 to 1, member by member: the vectors'
                        // constructor copies its x and y through the FPU, which the original doesn't
                        S_vector vel = dice.objs[j].ground_normal;
                        S_vector a, b;
                        a.x = vel.x;
                        a.y = vel.z;
                        a.z = -vel.y;
                        b.x = vel.z;
                        b.y = vel.y;
                        b.z = -vel.x;
                        a *= (rand() % 200 - 100.0f) * .01f;
                        b *= (rand() % 200 - 100.0f) * .01f;
                        vel += a + b;
                        vel.Normalize();
                        vel *= speed;
                        dice.objs[j].velocity = vel;
                        dice.objs[j].ground_tested = false;
                    }
                    dice.objs[j].roll += t * dice.objs[j].roll_speed;
                    dice.objs[j].frame->SetDir(dice.objs[j].frame->GetDir(), dice.objs[j].roll);
                    dice.objs[j].frame->SetPos(pos);
                    act_mission->GetScene()->SetFrameSector(dice.objs[j].frame);
                    dice.objs[j].frame->Update();
                }
                else
                {
                    dice.objs[j].ground = dice.objs[j].frame->GetPos() + S_vector(0.0f, -10000.0f, 0.0f);
                    gifCollision.TestLineV(dice.objs[j].frame->GetPos() + S_vector(0.0f, .1f, 0.0f),
                        S_vector(0.0f, -500.0f, 0.0f), &dice.objs[j].ground, &dice.objs[j].ground_normal, 0xffff,
                        0);
                    dice.objs[j].frame->SetPos(dice.objs[j].ground + S_vector(0.0f, .05f, 0.0f));
                    act_mission->GetScene()->SetFrameSector(dice.objs[j].frame);
                    dice.objs[j].frame->SetDir(dice.objs[j].ground_normal, 0.0f);
                    dice.objs[j].frame->Update();
                    dice.objs[j].active = false;
                }
            }
            else
                dead++;
        }
        // The pool's pane released as Done releases it, indexed again at each use
        if (dead == dice.num_objs)
        {
            PrepareGlassSingleObject(dice);
            if (++pool_last >= pool_size)
                pool_last = 0;
            if (pool[pool_last].frames.size())
            {
                for (int k = pool[pool_last].frames.size(); k--;)
                {
                    pool[pool_last].frames[k]->LinkTo(NULL, 0);
                    pool[pool_last].frames[k]->Release();
                }
                if (pool[pool_last].objs)
                    delete[] pool[pool_last].objs;
            }
            else if (pool[pool_last].objs)
            {
                for (int k = pool[pool_last].num_objs; k--;)
                {
                    if (pool[pool_last].objs[k].frame)
                        pool[pool_last].objs[k].frame->Release();
                }
                delete[] pool[pool_last].objs;
            }
            pool[pool_last] = dice;
            dices[i] = dices.back();
            dices.pop_back();
        }
    }
}

#pragma auto_inline(off)

// STUB: GAME 0x5605c0
void PrepareGlassSingleObject(S_DICE &dice)
{
    StubLog("PrepareGlassSingleObject");
}

// STUB: GAME 0x562170
void C_Break_plain_glass::BreakGlass(I3D_frame *frm, const S_vector *dir, const char *sound, bool unknown)
{
    StubLog("C_Break_plain_glass::BreakGlass");
}

#pragma auto_inline(on)
