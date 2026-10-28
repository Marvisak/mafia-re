#pragma once

#include "Main.h"
#include "I3D/I3d_math.h"

#include <vector>

class C_actor;

// The AI's groups, its following and its fights (PS2 ai_groups, ai_follow_manager and ai_fight_manager, members of
// AIManager; PS2's jp_ai_groups.cpp, most likely the PC's file at 0x530390-0x53857c). The classes' and the structs'
// names are PS2's, the members' are ours: those whose meaning isn't known are named by their offset. The members are
// what the functions written so far and the copies of the implicit functions the original kept show (the elements'
// operator= and destructors, AIManager's constructor and destructor).

// A member of a group (PS2 ai_group_member), 8 bytes; PS2's ai_group::Add takes an entity and a number
struct ai_group_member
{
    int unknown_0;                      // 0x0
    int unknown_4;                      // 0x4
};

// An event of a group (PS2 ai_group_event), 0x14 bytes
struct ai_group_event
{
    int unknown_0;                      // 0x0
    int unknown_4;                      // 0x4
    int unknown_8;                      // 0x8
    int unknown_c;                      // 0xc
    int unknown_10;                     // 0x10
};

// A group (PS2 ai_group), 0x60 bytes (PS2's 0x4c, its vectors 0xc bytes). Its operator= (0x537a50) and destructor
// (0x531060) are implicit: the original's copies inline what they call but the events' and the last two vectors'
// operator=.
struct ai_group
{
    int unknown_0;                      // 0x0, the group's number (PS2's NewGroup)
    int unknown_4;                      // 0x4
    int unknown_8;                      // 0x8
    int unknown_c;                      // 0xc
    std::vector<ai_group_member> members;   // 0x10
    std::vector<uint> unknown_20;       // 0x20
    std::vector<ai_group_event> events; // 0x30
    std::vector<uint> unknown_40;       // 0x40
    std::vector<uint> unknown_50;       // 0x50
};

class ai_groups
{
public:
    int unknown_0;                      // 0x0, -1 after Init and Destroy
    int unknown_4;                      // 0x4, the last group's number (PS2's NewGroup counts it up)
    std::vector<ai_group> groups;       // 0x8
    std::vector<uint> unknown_18;       // 0x18

    // PS2 names
    void Init();
    void Destroy();
};

// A group that follows an actor (PS2 ai_follow_group), 0x38 bytes (PS2's 0x40, its position first)
struct ai_follow_group
{
    std::vector<C_actor *> followers;   // 0x0 (PS2 0x10)
    S_vector unknown_10;                // 0x10 (PS2 0x0)
    float unknown_1c;                   // 0x1c
    int unknown_20;                     // 0x20
    int unknown_24;                     // 0x24
    int unknown_28;                     // 0x28
    int unknown_2c;                     // 0x2c
    int unknown_30;                     // 0x30
    int unknown_34;                     // 0x34
};

class ai_follow_manager
{
public:
    std::vector<ai_follow_group> groups;    // 0x0
    int unknown_10;                     // 0x10, cleared by Init and Destroy

    // PS2 names
    void Init();
    void Destroy();
};

// A member of a fighting party (PS2 ai_fight_member), 0x14 bytes
struct ai_fight_member
{
    int unknown_0;                      // 0x0
    int unknown_4;                      // 0x4
    int unknown_8;                      // 0x8
    int unknown_c;                      // 0xc
    int unknown_10;                     // 0x10
};

// A fighting party (PS2 ai_fight_followship), 0x48 bytes (PS2's 0x60). Its destructor's copy the linker kept is
// AIManager's file's (0x538c30, folded with the police actions').
struct ai_fight_followship
{
    S_vector unknown_0;                 // 0x0
    S_vector unknown_c;                 // 0xc
    std::vector<ai_fight_member> members;   // 0x18 (PS2 0x20)
    float unknown_28;                   // 0x28 (PS2 0x2c)
    int unknown_2c;                     // 0x2c
    int unknown_30;                     // 0x30
    int unknown_34;                     // 0x34
    int unknown_38;                     // 0x38
    int unknown_3c;                     // 0x3c
    int unknown_40;                     // 0x40
    int unknown_44;                     // 0x44
};

// PS2's has its parties only, as the PC's: AIManager's next member is PS2's sight manager too
class ai_fight_manager
{
public:
    std::vector<ai_fight_followship> parties;   // 0x0

    // PS2 names
    void Init();
    void Destroy();
};

// The copies of the implicit functions and the vectors' helpers that the managers' Init and Destroy call out of line:
// this file's (0x530390-0x53857c), but the folded ones the original kept elsewhere
// SYNTHETIC: GAME 0x531060
// ??1ai_group@@QAE@XZ

// TEMPLATE: GAME 0x536460
// ?_Destroy@?$vector@Uai_group@@V?$allocator@Uai_group@@@std@@@std@@IAEXPAUai_group@@0@Z

// TEMPLATE: GAME 0x537130
// ?copy@std@@YIPAUai_group_event@@PAU2@00@Z

// SYNTHETIC: GAME 0x537a50
// ??4ai_group@@QAEAAU0@ABU0@@Z

// The parties' members' operator=, folded with the groups' events' (both elements 0x14 bytes)
// TEMPLATE: GAME 0x538280
// ??4?$vector@Uai_group_event@@V?$allocator@Uai_group_event@@@std@@@std@@QAEAAV01@ABV01@@Z

// The parties' destructor, folded with the police actions' (both a vector at 0x18): the original kept AIManager's
// file's copy
// SYNTHETIC: GAME 0x538c30
// ??1ai_police_action@@QAE@XZ

// A copy of 4-byte elements, folded with every other one: the original kept a later file's copy
// TEMPLATE: GAME 0x564750
// ?copy@std@@YIPAPAUS_effect@C_ff_manager@@PAPAU23@00@Z
