// The effects of tables\effects.tbl, PC only: the file at 0x448f00-0x44b4cc (docs/game-files.md), which starts with
// C_effects' constructor; the name is ours. Written so far: the constructor, the destructor, Load and Clear, which
// C_mission's Init and UnInit run, and with Load the effects' classes (C_effects.h).

#include "C_effects.h"

// FUNCTION: GAME 0x449080
C_effects::C_effects()
{
    Load();
    unknown_30 = -1;
}

// FUNCTION: GAME 0x449100
C_effects::~C_effects()
{
    Clear();
}

// The table is a list of effect chunks; anything else ends it
// FUNCTION: GAME 0x449380
void C_effects::Load()
{
    Clear();
    C_chunk chunk;
    // STRING: GAME 0x632590
    if (!chunk.ROpen("tables\\effects.tbl"))
        return;
    ushort type;
    while (chunk.RAscend(type) && type == CT_EFFECT)
    {
        C_effect *effect = new C_effect;
        effect->Load(chunk);
        chunk.Descend();
        effects.push_back(effect);
    }
    chunk.Close();
}

// FUNCTION: GAME 0x44a350
void C_effects::Clear()
{
    for (int i = 0; i < effects.size(); i++)
        delete effects[i];
    effects.clear();
}
