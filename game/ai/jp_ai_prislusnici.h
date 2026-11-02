#pragma once

#include "Main.h"
#include "I3D/I3d_math.h"

#include <vector>

class C_actor;
class C_entity;
class I3D_frame;

// The police (PS2 jp_ai_prislusnici.cpp, "prislusnici" the police officers, most likely the PC's file at
// 0x538c80-0x53e4ac). The classes' and the structs' names are PS2's, the members' are ours: those whose meaning isn't
// known are named by their offset.

// A frame the police show for an action (PS2 ai_anim_item), 0x10 bytes
struct ai_anim_item
{
    int unknown_0;                      // 0x0
    int unknown_4;                      // 0x4
    int unknown_8;                      // 0x8
    I3D_frame *frame;                   // 0xc, unlinked, turned off and released by Clear
};

// The police's frames (PS2 ai_animitem_manager), the notice board's base: PS2 calls its methods on the board itself
class ai_animitem_manager
{
public:
    std::vector<ai_anim_item> items;    // 0x0

    // PS2 names
    void Clear();
    void Init();
};

// An action the police carry out (PS2 ai_police_action), 0x6c bytes (PS2's 0x80, its members after the vector 4 bytes
// further on than the PC's). Its destructor's copy the linker kept is AIManager's file's (0x538c30, folded with the
// fighting parties'). The members' names are ours.
struct ai_police_action
{
    S_vector unknown_0;                 // 0x0
    S_vector unknown_c;                 // 0xc
    // 0x18 (PS2 0x20), the officers: PS2's CoVlastneRobimPlukovniku finds the action an actor is in (the PC's 0x53bb60
    // the same), KolikatejSemSefe the actor's place in them
    std::vector<C_actor *> members;
    int unknown_28;                     // 0x28
    int state;                          // 0x2c, 0 to 7: PS2's Tick switches on it, ZbavSeAutaSmrdi moves it on
    int id;                             // 0x30, the action's number (JaChcuCislo's), which MamCisloDejAkci finds
    int unknown_34;                     // 0x34
    int unknown_38;                     // 0x38
    int unknown_3c;                     // 0x3c
    int unknown_40;                     // 0x40
    int unknown_44;                     // 0x44
    int unknown_48;                     // 0x48
    int unknown_4c;                     // 0x4c
    int flags;                          // 0x50, bits PS2's Tick and ZbavSeAutaSmrdi set and test
    int unknown_54;                     // 0x54
    int unknown_58;                     // 0x58
    // 0x5c, the action's car, which the shift's end stops (0x53d7b0, PS2's ZastavMotorizovaneJednotky, which puts
    // away the members in it) and PS2's ZbavSeAutaSmrdi ("get rid of the car, stinker") clears
    C_actor *car;
    int unknown_60;                     // 0x60
    int unknown_64;                     // 0x64
    bool unknown_68;                    // 0x68

    // The action by the time passed, which the board's Tick calls (PS2 name)
    void Tick(uint time);
};

// The police's notice board (PS2 ai_police_nastenka, "nastenka" Czech for a notice board), a member of AIManager, 0x34
// bytes (PS2's 0x2c). Its methods' names are PS2's, Czech.
class ai_police_nastenka : public ai_animitem_manager
{
public:
    std::vector<ai_police_action> actions;  // 0x10 (PS2 0xc)
    // 0x20, the next action's number (JaChcuCislo, "I want a number", returns it and counts it up; the name is ours)
    int next_id;
    uint unknown_24;                    // 0x24
    // POLICE_SPEED_FACTOR's, 0 to 1 (the name is ours, after the command)
    float speed_factor;                 // 0x28
    // 0x2c, set by PS2's MakamSefe ("I'm working, boss"), which PanoveHrajeteSiNaPoldy returns (the name is ours)
    uchar on_duty;
    C_entity *customer;                 // 0x30, PS2's TuMasZakaznika's ("here's a customer for you")

    // The shift's start
    void ZacatekSmeny();
    // An action done ("the action is finished, colonel")
    void AkceDokoncenaPlukovniku(ai_police_action *action);
    // The actions by the time passed, which C_game::Tick calls (PS2 name)
    void Tick(uint time);
    // Whether unknown_24 is 0 ("colonel, you have the player in the cooler"), which CHANGE_MISSION asks
    bool PlukovnikuMasVChladkuPlajeraFrafera();
    // Whether an action after the actor has seen it lately ("gentlemen, do you have your eyes on the bastard or are you
    // staring at the girls")
    bool PanoveMateOciNaZmrdoviNeboCumitePoBabach(C_actor *actor);
    // Whether on_duty is set ("gentlemen, are you playing cops")
    bool PanoveHrajeteSiNaPoldy();
    // The police told of an actor's new state ("gentlemen, a change: arrest the bastard", "... kill the bastard") and
    // to write him down ("gentlemen, take out your notepads and write the bastard down")
    void PanoveZmenaZatkneteHoZmrda(C_actor *actor);
    void PanoveZmenaZabijteHoZmrda(C_actor *actor);
    void PanoveVytahneteBlokyAZapisteSiZmrda(C_actor *actor);
    // Whether someone goes after the actor ("gentlemen, is someone going after the bastard"; PS2's takes a bool more)
    // and whether they see him ("gentlemen, do you see the bastard or are you dumb")
    bool PanoveJdeteNekdoPoZmrdovi(C_actor *actor);
    bool PanoveViditeZmrdaNeboSteTupci(C_actor *actor);
    // The shift's end
    void KonecSmeny();
    // Stops the motorized units (PS2's name, Czech)
    void ZastavMotorizovaneJednotky();
};
