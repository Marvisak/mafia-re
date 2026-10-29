// The player (PS2 C_player.cpp; the PC's file is the one at 0x58f160-0x5961fc, docs/game-files.md), whose
// constructor (0x58f170) comes first. Written so far: FadeReset, which the camera calls, and SetPlayerNaviPoint; the
// constructor, which C_mission::CreateActor calls, UpdateControls, SetSniperMode and SetAlpha, and the functions the
// script calls are stubs.

#include "actors/C_player.h"

#include "stub.h"

// GLOBAL: GAME 0x6478b0
bool C_player::unknown_6478b0;
// GLOBAL: GAME 0x6478b4
I3D_frame *C_player::unknown_6478b4;

#pragma auto_inline(off)

#pragma auto_inline(on)

// FUNCTION: GAME 0x58f160
void SetPlayerNaviPoint(I3D_frame *frm)
{
    C_player::unknown_6478b4 = frm;
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
    unknown_ab8 = 1.0f;
    unknown_abc = 1.0f;
    SetAlpha(unknown_ab8);
}
