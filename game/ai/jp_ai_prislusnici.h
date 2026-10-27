#pragma once

#include "Main.h"
#include "I3D/I3d_math.h"

#include <vector>

class C_actor;
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

// An action the police carry out (PS2 ai_police_action), 0x6c bytes (PS2's 0x80). Its destructor's copy the linker
// kept is AIManager's file's (0x538c30, folded with the fighting parties').
struct ai_police_action
{
    S_vector unknown_0;                 // 0x0
    S_vector unknown_c;                 // 0xc
    std::vector<C_actor *> unknown_18;  // 0x18 (PS2 0x20)
    int unknown_28;                     // 0x28
    int unknown_2c;                     // 0x2c
    int unknown_30;                     // 0x30
    int unknown_34;                     // 0x34
    int unknown_38;                     // 0x38
    int unknown_3c;                     // 0x3c
    int unknown_40;                     // 0x40
    int unknown_44;                     // 0x44
    int unknown_48;                     // 0x48
    int unknown_4c;                     // 0x4c
    int unknown_50;                     // 0x50
    int unknown_54;                     // 0x54
    int unknown_58;                     // 0x58
    C_actor *unknown_5c;                // 0x5c, an actor the shift's end stops (0x53d7b0)
    int unknown_60;                     // 0x60
    int unknown_64;                     // 0x64
    bool unknown_68;                    // 0x68
};

// The police's notice board (PS2 ai_police_nastenka, "nastenka" Czech for a notice board), a member of AIManager, 0x34
// bytes (PS2's 0x2c). Its methods' names are PS2's, Czech.
class ai_police_nastenka : public ai_animitem_manager
{
public:
    std::vector<ai_police_action> actions;  // 0x10 (PS2 0xc)
    int unknown_20;                     // 0x20, the actions' numbers (PS2's JaChcuCislo, "I want a number", counts it up)
    int unknown_24;                     // 0x24
    float unknown_28;                   // 0x28
    bool unknown_2c;                    // 0x2c
    int unknown_30;                     // 0x30

    // The shift's start
    void ZacatekSmeny();
    // An action done ("the action is finished, colonel")
    void AkceDokoncenaPlukovniku(ai_police_action *action);
    // The shift's end
    void KonecSmeny();
};
