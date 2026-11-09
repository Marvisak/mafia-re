// The grenades (PS2 C_grenade.cpp; the PC's file is the one at 0x445260-0x44584c, docs/game-files.md, between
// C_ff_manager.cpp and C_irenka.cpp as the name sorts), in PS2's order: a bottle's physics that goes off once its fuse
// has burnt down or at its first hit, exploding or setting a fire. Not written yet: AI, a stub.

#include "actors/C_grenade.h"

#include "mission/Mission.h"
#include "mission/game.h"
#include "stub.h"

// FUNCTION: GAME 0x445260
C_grenade::C_grenade()
{
    type = ACTOR_GRENADE;
    unknown_6d = true;
    unknown_15c = true;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x4452b0
C_grenade::~C_grenade()
{
}

// The linker folded it with C_cartridge's, the same code
// FUNCTION: GAME 0x430830 FOLDED
bool C_grenade::Init(I3D_frame *frm)
{
    if (!frm)
        return false;
    return C_bottle::Init(frm) ? true : false;
}

// The bottle's step, with the hit material's sound near the camera (game sounds high), then the fuse: where its time
// is up or at its first hit the grenade goes off, the explosion, the fire (and its particles), then out of the game
// STUB: GAME 0x4452c0
void C_grenade::AI(uint time)
{
    StubLog("C_grenade::AI");
    C_bottle::AI(time);
}

// The base's: the original's jumps there, folded with C_cartridge's
// FUNCTION: GAME 0x430a60 FOLDED
void C_grenade::Update(uint time)
{
    C_bottle::Update(time);
}

// The fuse, the hits and the fire off
// FUNCTION: GAME 0x445620
void C_grenade::GameInit()
{
    C_bottle::GameInit();
    set_states = false;
    impact = false;
    timer = false;
    explode = false;
    fire = false;
}

// The frame unlinked and released: the grenade's model is its own
// FUNCTION: GAME 0x445650
void C_grenade::GameDone()
{
    C_bottle::GameDone();
    frame->LinkTo(NULL, 0);
    frame->Release();
}

// FUNCTION: GAME 0x445670
int C_grenade::SaveGameGetSize()
{
    return C_bottle::SaveGameGetSize() + sizeof(S_grenade_savegame);
}

// FUNCTION: GAME 0x445680
bool C_grenade::SaveGameSave(void **buf)
{
    if (!C_bottle::SaveGameSave(buf))
        return false;
    S_grenade_savegame *s = (S_grenade_savegame *)*buf;
    s->version = GRENADE_SAVEGAME_VERSION;
    s->timer = timer;
    s->fuse_time = fuse_time;
    s->impact = impact;
    s->explode = explode;
    s->explosion_radius = explosion_radius;
    s->explosion_power = explosion_power;
    s->fire = fire;
    s->fire_time = fire_time;
    s->fire_unknown1 = fire_unknown1;
    s->fire_unknown2 = fire_unknown2;
    *buf = (uchar *)*buf + sizeof(S_grenade_savegame);
    return true;
}

// FUNCTION: GAME 0x445710
bool C_grenade::SaveGameLoad(void **buf)
{
    if (!C_bottle::SaveGameLoad(buf))
        return false;
    S_grenade_savegame *s = (S_grenade_savegame *)*buf;
    if (s->version != GRENADE_SAVEGAME_VERSION)
        return false;
    timer = s->timer;
    fuse_time = s->fuse_time;
    impact = s->impact;
    explode = s->explode;
    explosion_radius = s->explosion_radius;
    explosion_power = s->explosion_power;
    fire = s->fire;
    fire_time = s->fire_time;
    fire_unknown1 = s->fire_unknown1;
    fire_unknown2 = s->fire_unknown2;
    *buf = (uchar *)*buf + sizeof(S_grenade_savegame);
    return true;
}

// FUNCTION: GAME 0x4457a0
bool C_grenade::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    if (impact)
        bounces++;
    if (act_state == ACTSTATE_NONE)
        return false;
    return C_bottle::Collision(act, v1, v2, v3, f, frm);
}

// FUNCTION: GAME 0x4457d0
bool C_grenade::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    if (impact)
        bounces++;
    if (act_state != ACTSTATE_NONE)
        C_bottle::Hit(type, v1, v2, v3, f, act, u, frm);
    return true;
}

// The linker's folded empty function of the out-of-line kind (0x4ab130)
void C_grenade::NetDirect(uint time)
{
}

// The linker's folded returns of true of the out-of-line kind (0x4aa490)
bool C_grenade::SaveData(C_chunk *chunk)
{
    return true;
}

bool C_grenade::LoadData(C_chunk *chunk)
{
    return true;
}
