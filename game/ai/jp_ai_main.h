#pragma once

#include "Main.h"
#include "ai/jp_ai_groups.h"
#include "ai/jp_ai_prislusnici.h"
#include "ai/jp_ai_segments.h"

#include <vector>

// What the AI sees (PS2 ai_sight_manager, jp_ai_main.cpp's), AIManager's last member, 0x20 bytes: PS2's has two vectors
// too. Nothing written uses it yet.
class ai_sight_manager
{
public:
    std::vector<uint> unknown_0;        // 0x0
    std::vector<uint> unknown_10;       // 0x10
};

// The AI's way finding (PS2 ai_wayfind, PS2's jp_ai_checkpoints.cpp, most likely the PC's file at 0x4f8490-0x5053fc,
// whose methods take AIManager's as their object), AIManager's member at 0xc (PS2's at 0x10), 0x84 bytes. Only what
// its absolute addresses in that file and C_dog's show; nothing written uses it yet. AIManager's constructor (0x538760)
// calls the police's first vector's constructor out of line only with about six S_vector constructors in this class
// before it (pending inline calls): the last three are a guess at the tail, which nothing addresses directly.
class ai_wayfind
{
public:
    S_vector unknown_0;                 // 0x0
    S_vector unknown_c;                 // 0xc
    S_vector unknown_18;                // 0x18
    S_vector unknown_24;                // 0x24
    int unknown_30;                     // 0x30
    int unknown_34;                     // 0x34
    float unknown_38;                   // 0x38
    int unknown_3c;                     // 0x3c, a mode (2 set by the file at 0x4f8490)
    int unknown_40;                     // 0x40
    int unknown_44;                     // 0x44
    bool unknown_48;                    // 0x48
    std::vector<uint> unknown_4c;       // 0x4c (PS2 0x80, which PS2's ai_manager::Init reserves 100 of)
    void *unknown_5c;                   // 0x5c, set by C_dog and C_entity before they call its methods
    S_vector unknown_60;                // 0x60
    S_vector unknown_6c;                // 0x6c
    S_vector unknown_78;                // 0x78
};

// The AI (PS2 ai_manager, AIManager; PS2's jp_ai_main.cpp, the PC's file at 0x538580-0x538c7c, whose initializer
// constructs it), 0x130 bytes (PS2's 0x250). PS2's Init and Destroy also start and end the sight, panic and other
// managers; the PC's game calls its members itself. Its constructor (0x538760) and destructor (0x5385c0) are implicit:
// the file has nothing else but the copies of the implicit functions they and its unwinding call. The members' names
// are ours.
class ai_manager
{
public:
    int unknown_0;                      // 0x0
    check_seg_clusters segments;        // 0x4
    ai_wayfind wayfind;                 // 0xc (PS2 0x10)
    ai_police_nastenka police;          // 0x90 (PS2 0xb0)
    ai_groups groups;                   // 0xc4 (PS2 0xdc)
    ai_follow_manager follow;           // 0xec (PS2 0xfc)
    ai_fight_manager fight;             // 0x100 (PS2 0x10c)
    ai_sight_manager sight;             // 0x110 (PS2 0x118)
};

extern ai_manager AIManager;

// AIManager's implicit constructor and destructor and what they and the constructor's unwinding call out of line, the
// rest of the file at 0x538580 (after its initializer)
// SYNTHETIC: GAME 0x5385c0
// ??1ai_manager@@QAE@XZ

// SYNTHETIC: GAME 0x538760
// ??0ai_manager@@QAE@XZ

// SYNTHETIC: GAME 0x538880
// ??1ai_wayfind@@QAE@XZ

// SYNTHETIC: GAME 0x5388a0
// ??1ai_police_nastenka@@QAE@XZ

// SYNTHETIC: GAME 0x538970
// ??1ai_groups@@QAE@XZ

// SYNTHETIC: GAME 0x538a80
// ??1ai_follow_manager@@QAE@XZ

// SYNTHETIC: GAME 0x538af0
// ??1ai_fight_manager@@QAE@XZ

// TEMPLATE: GAME 0x538b80
// ??1?$vector@Uai_group@@V?$allocator@Uai_group@@@std@@@std@@QAE@XZ

// TEMPLATE: GAME 0x538bc0
// ?_Destroy@?$vector@Uai_police_action@@V?$allocator@Uai_police_action@@@std@@@std@@IAEXPAUai_police_action@@0@Z

// SYNTHETIC: GAME 0x538bf0
// ??_Gai_follow_group@@QAEPAXI@Z

// SYNTHETIC: GAME 0x538c10
// ??_Gai_fight_followship@@QAEPAXI@Z
