// The police's officers (PS2 jp_ai_prislusnici.cpp; the PC's file is most likely the one at 0x538c80-0x53e4ac,
// docs/game-files.md), in PS2's order. Written so far: the shift's start and end, which C_game's Init and Done call,
// and the frames' Clear and Init, which they end with (tail calls), and two tests the script's commands make
// (PlukovnikuMasVChladkuPlajeraFrafera, PanoveHrajeteSiNaPoldy), and the board's Tick, which C_game::Tick calls; stubs
// of AkceDokoncenaPlukovniku, which the shift's end calls for each action, of the actions' Tick, which the board's
// calls, of what the script's commands call, and of the board's functions C_shvestky::Tick calls (the Panove... ones).

#include "ai/jp_ai_prislusnici.h"

#include "I3D/I3D.h"

#include "stub.h"

// FUNCTION: GAME 0x53ba50
void ai_police_nastenka::ZacatekSmeny()
{
    actions.clear();
    next_id = 0;
    unknown_24 = 0xfffffff;
    on_duty = 0;
    speed_factor = 1.0f;
    customer = NULL;
    Init();
}

// STUB: GAME 0x53c550
void ai_police_nastenka::AkceDokoncenaPlukovniku(ai_police_action *action)
{
    StubLog("ai_police_nastenka::AkceDokoncenaPlukovniku");
}

// From the last action, then the time counted down to 0 (PS2's the same)
// FUNCTION: GAME 0x53c890
void ai_police_nastenka::Tick(uint time)
{
    for (int i = actions.size() - 1; i >= 0; i--)
        actions[i].Tick(time);
    if (unknown_24 > time)
        unknown_24 -= time;
    else
        unknown_24 = 0;
}

// The PC's is in the file's range before AkceDokoncenaPlukovniku (PS2's order)
// STUB: GAME 0x538df0
void ai_police_action::Tick(uint time)
{
    StubLog("ai_police_action::Tick");
}

// FUNCTION: GAME 0x53c900
bool ai_police_nastenka::PlukovnikuMasVChladkuPlajeraFrafera()
{
    return unknown_24 <= 0 ? true : false;
}

// STUB: GAME 0x53cbf0
void ai_police_nastenka::PanoveZmenaZatkneteHoZmrda(C_actor *actor)
{
    StubLog("ai_police_nastenka::PanoveZmenaZatkneteHoZmrda");
}

// STUB: GAME 0x53cc90
void ai_police_nastenka::PanoveZmenaZabijteHoZmrda(C_actor *actor)
{
    StubLog("ai_police_nastenka::PanoveZmenaZabijteHoZmrda");
}

// STUB: GAME 0x53d220
bool ai_police_nastenka::PanoveViditeZmrdaNeboSteTupci(C_actor *actor)
{
    StubLog("ai_police_nastenka::PanoveViditeZmrdaNeboSteTupci");
    return false;
}

// STUB: GAME 0x53d290
void ai_police_nastenka::PanoveVytahneteBlokyAZapisteSiZmrda(C_actor *actor)
{
    StubLog("ai_police_nastenka::PanoveVytahneteBlokyAZapisteSiZmrda");
}

// STUB: GAME 0x53d300
bool ai_police_nastenka::PanoveMateOciNaZmrdoviNeboCumitePoBabach(C_actor *actor)
{
    StubLog("ai_police_nastenka::PanoveMateOciNaZmrdoviNeboCumitePoBabach");
    return false;
}

// STUB: GAME 0x53d3d0
bool ai_police_nastenka::PanoveJdeteNekdoPoZmrdovi(C_actor *actor)
{
    StubLog("ai_police_nastenka::PanoveJdeteNekdoPoZmrdovi");
    return false;
}

// FUNCTION: GAME 0x53d480
bool ai_police_nastenka::PanoveHrajeteSiNaPoldy()
{
    return on_duty;
}

// FUNCTION: GAME 0x53d750
void ai_police_nastenka::KonecSmeny()
{
    for (int i = actions.size() - 1; i >= 0; i--)
        AkceDokoncenaPlukovniku(&actions[i]);
    Clear();
}

// STUB: GAME 0x53d7b0
void ai_police_nastenka::ZastavMotorizovaneJednotky()
{
    StubLog("ai_police_nastenka::ZastavMotorizovaneJednotky");
}

// FUNCTION: GAME 0x53d8c0
void ai_animitem_manager::Clear()
{
    for (int i = items.size() - 1; i >= 0; i--) {
        items[i].frame->LinkTo(NULL, 0);
        items[i].frame->SetOn(false);
        items[i].frame->Release();
        items[i].frame = NULL;
        items.erase(&items[i]);
    }
    items.clear();
}

// FUNCTION: GAME 0x53d9c0
void ai_animitem_manager::Init()
{
    items.clear();
}

// STUB: GAME 0x53cd20
void ai_police_nastenka::PanoveKokotPremistujePrdel(C_actor *actor, C_actor *car, bool b)
{
    StubLog("ai_police_nastenka::PanoveKokotPremistujePrdel");
}
