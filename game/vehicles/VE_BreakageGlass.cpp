// The broken glass (PS2 VE_BreakageGlass.cpp; the PC's file is most likely the one at 0x5605c0-0x56488c, after
// tshift.cpp as on PS2, docs/game-files.md). Written so far: Init and Done, which C_game's Init and Done call.

#include "vehicles/VE_BreakageGlass.h"

#include "I3D/I3D.h"

// The gravity goes through a temporary, as PS2's
// FUNCTION: GAME 0x560db0
bool C_Break_plain_glass::Init(int num)
{
    gravity = S_vector(0.0f, -8.0f, 0.0f);
    pool_size = num;
    unknown_24 = num;
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
