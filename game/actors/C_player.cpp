// The player (PS2 C_player.cpp; the PC's file is the one at 0x58f160-0x5961fc, docs/game-files.md), whose
// constructor (0x58f170) comes first. Written so far: FadeReset, which the camera calls, and SetPlayerNaviPoint; the
// constructor, which C_mission::CreateActor calls, UpdateControls, SetSniperMode and SetAlpha, and the functions the
// script calls are stubs.

#include "actors/C_player.h"

#include "stub.h"

// GLOBAL: GAME 0x6478b0
bool C_player::can_car_damage;
// GLOBAL: GAME 0x6478b4
I3D_frame *C_player::navi_frame;

#pragma auto_inline(off)

#pragma auto_inline(on)

// FUNCTION: GAME 0x58f160
void SetPlayerNaviPoint(I3D_frame *frm)
{
    C_player::navi_frame = frm;
}

#pragma auto_inline(off)

// STUB: GAME 0x58f170
C_player::C_player()
{
    StubLog("C_player::C_player");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x5942c0
void C_player::UpdateControls()
{
    StubLog("C_player::UpdateControls");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x5952f0
void C_player::IndicatorSetAimed()
{
    StubLog("C_player::IndicatorSetAimed");
}

// STUB: GAME 0x595440
void C_player::SetSniperMode(bool on)
{
    StubLog("C_player::SetSniperMode");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x595e10
void C_player::SetAlpha(float alpha)
{
    StubLog("C_player::SetAlpha");
}

// STUB: GAME 0x595f00
void C_player::LockControls(bool lock)
{
    StubLog("C_player::LockControls");
}

#pragma auto_inline(on)

// The player fully visible again (PS2 name), which the camera's Begin calls for the cameras outside his eyes
// FUNCTION: GAME 0x5961b0
void C_player::FadeReset()
{
    fade_alpha = 1.0f;
    fade_target = 1.0f;
    SetAlpha(fade_alpha);
}

#pragma auto_inline(off)

// STUB: GAME 0x58f220
C_player::~C_player()
{
    StubLog("C_player::~C_player");
}

// STUB: GAME 0x58f260
bool C_player::Init(I3D_frame *frm)
{
    StubLog("C_player::Init");
    return C_human::Init(frm);
}

// STUB: GAME 0x58f2a0
bool C_player::SaveData(C_chunk *chunk)
{
    StubLog("C_player::SaveData");
    return C_human::SaveData(chunk);
}

// STUB: GAME 0x58f2c0
bool C_player::LoadData(C_chunk *chunk)
{
    StubLog("C_player::LoadData");
    return C_human::LoadData(chunk);
}

// STUB: GAME 0x58f2e0
int C_player::SaveGameGetSize()
{
    StubLog("C_player::SaveGameGetSize");
    return C_human::SaveGameGetSize();
}

// STUB: GAME 0x58f2f0
bool C_player::SaveGameSave(void **buf)
{
    StubLog("C_player::SaveGameSave");
    return C_human::SaveGameSave(buf);
}

// STUB: GAME 0x58f380
bool C_player::SaveGameLoad(void **buf)
{
    StubLog("C_player::SaveGameLoad");
    return C_human::SaveGameLoad(buf);
}

// STUB: GAME 0x5937b0
void C_player::AI(uint time)
{
    StubLog("C_player::AI");
    C_human::AI(time);
}

// STUB: GAME 0x594060
void C_player::GameInit()
{
    StubLog("C_player::GameInit");
    C_human::GameInit();
}

// STUB: GAME 0x594180
void C_player::GameDone()
{
    StubLog("C_player::GameDone");
    C_human::GameDone();
}

// STUB: GAME 0x5941c0
bool C_player::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    StubLog("C_player::Collision");
    return C_human::Collision(act, v1, v2, v3, f, frm);
}

// STUB: GAME 0x594210
bool C_player::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    StubLog("C_player::Hit");
    return C_human::Hit(type, v1, v2, v3, f, act, u, frm);
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x595570
void C_player::_intern_ChangeModel()
{
    StubLog("C_player::_intern_ChangeModel");
}

// STUB: GAME 0x595c30
void C_player::SetCarDamage(C_car *car)
{
    StubLog("C_player::SetCarDamage");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x595390
void C_player::ShootDeAimed(S_item_def *weapon)
{
    StubLog("C_player::ShootDeAimed");
}

// STUB: GAME 0x595cb0
void C_player::AddAlphaObject(I3D_frame *frm)
{
    StubLog("C_player::AddAlphaObject");
}

// STUB: GAME 0x595d70
void C_player::DelAlphaObject(I3D_frame *frm)
{
    StubLog("C_player::DelAlphaObject");
}

#pragma auto_inline(on)
