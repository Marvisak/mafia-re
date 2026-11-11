#pragma once

#include "Main.h"
#include "I3D/I3d_math.h"

#include <vector>

class C_actor;
class C_car;
class C_entity;
class C_human;

// The AI's groups, its following and its fights (PS2 ai_groups, ai_follow_manager and ai_fight_manager, members of
// AIManager; PS2's jp_ai_groups.cpp, most likely the PC's file at 0x530390-0x53857c). The classes' and the structs'
// names are PS2's, the members' are ours: those whose meaning isn't known are named by their offset. The members are
// what the functions written so far and the copies of the implicit functions the original kept show (the elements'
// operator= and destructors, AIManager's constructor and destructor).

// A member of a group (PS2 ai_group_member), 8 bytes; PS2's ai_group::Add takes an entity and a number
struct ai_group_member
{
    C_entity *entity;                   // 0x0
    // 0x4, Add's number: PS2's canCast is false for a member of 1, canReceive for one of 2 (the name is ours)
    int role;
};

// An event of a group (PS2 ai_group_event), 0x14 bytes: PS2's ai_group::UnderAttack adds one of type 1, which
// ProcessEvents passes on to the members once its time has come (the PC's the same). The names are ours.
struct ai_group_event
{
    int type;                           // 0x0
    int time;                           // 0x4, the game's time it is processed at (UnderAttack's 1 s ahead)
    float unknown_8;                    // 0x8, UnderAttack's, whose address the members' Event gets
    C_entity *entity;                   // 0xc, the member attacked
    C_actor *attacker;                  // 0x10
};

// A group (PS2 ai_group), 0x60 bytes (PS2's 0x4c, its vectors 0xc bytes). Its operator= (0x537a50) and destructor
// (0x531060) are implicit: the original's copies inline what they call but the events' and the last two vectors'
// operator=.
struct ai_group
{
    int id;                             // 0x0, the group's number (PS2's NewGroup; the name is ours)
    int unknown_4;                      // 0x4
    int unknown_8;                      // 0x8
    int unknown_c;                      // 0xc
    std::vector<ai_group_member> members;   // 0x10
    // The vectors' names are ours: 0x20 (PS2 0x1c), the attackers ProcessEvents has told the members of, each once
    std::vector<C_actor *> attackers;
    std::vector<ai_group_event> events; // 0x30
    // 0x40 (PS2 0x34), the group's cars, which GetCarClosestAndUsable searches (PS2's) and GROUP_DISBAND frees
    std::vector<C_car *> cars;
    // 0x50 (PS2 0x40), GetCarFactor's: the one at a car's index in cars, 1 without one
    std::vector<float> car_factors;

    // The group's events by the game's time, which ai_groups::Tick calls (PS2 name)
    void ProcessEvents(uint time);
    // PS2 names, which the fights' Tick calls: how much the car counts for the group, and the group's car closest to
    // the position that its members can use, NULL for none (the second vector and the flag what else it found)
    float GetCarFactor(C_car *car);
    C_car *GetCarClosestAndUsable(S_vector *pos, S_vector *unknown, uint *unknown2);
};

class ai_groups
{
public:
    // 0x0, the game's time at which Tick unlocks the player's controls, -1 for none, as after Init and Destroy (unsigned:
    // Tick compares it so, as PS2's). The names are ours.
    uint unlock_time;
    int last_id;                        // 0x4, the last group's number (PS2's NewGroup counts it up)
    std::vector<ai_group> groups;       // 0x8
    // The actors the groups let go (PS2's second vector, a gif::vector<C_actor *, 2> at 0x14)
    std::vector<C_actor *> free_actors; // 0x18

    // PS2 names
    void Init();
    void Destroy();
    // The groups' frame, which C_game::Tick calls (PS2 name)
    void Tick();
    // The group of that number, NULL for none (PS2 name)
    ai_group *GetGroup(uint id);
    // Forgets an actor (PS2 name), which C_game::InvalidateActor calls
    void InvalidateActor(C_actor *act);
    // The saved game's record of the groups, its size and the record (PS2 names)
    uint GetSize();
    bool Save(void **buf);
    // The groups from a saved game's record (PS2 name; the result's type is a guess)
    bool Load(void **buf);

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours): its copy
    // inlines push_back's insert
    // FUNCTION: GAME 0x47b870
    void AddFreeActor(C_actor *act)
    {
        free_actors.push_back(act);
    }
};

// ai_follow_group::state (the names are ours, by value): what Update does with the group. PS2's AddFollower gives a
// new group FOLLOWSTATE_0 where the follower sits in the leader's car, else FOLLOWSTATE_5.
#define FOLLOWSTATE_0 0                 // the leader in a car, the followers getting in
#define FOLLOWSTATE_1 1                 // the leader without a car, the followers getting out
#define FOLLOWSTATE_2 2                 // the car driving the group to its place (AddCarHunt or AddCarMoveTo)
#define FOLLOWSTATE_3 3                 // all in the leader's car, which the AI doesn't drive for them
#define FOLLOWSTATE_4 4                 // the car stopped until stop_end (PS2's Event 4 stops a driving one for 7 s)
#define FOLLOWSTATE_5 5                 // the followers with the leader, in the same car or none

// ai_follow_group::flags (the names are ours)
// Set by PS2's AddFollower that takes a position, with pos: a car an entity drives takes the group there
#define FOLLOWF_HAS_POS 0x1
#define FOLLOWF_DRIVE_DONE 0x10         // set where the car's chase had C_sledovacka::STATUS_DONE
// Set with car where the chase ended otherwise, cleared with it: PS2's mustLeave gets the followers out while it's set
#define FOLLOWF_MUSTLEAVE 0x20

// A group that follows an actor (PS2 ai_follow_group), 0x38 bytes (PS2's 0x40, its position first)
struct ai_follow_group
{
    std::vector<C_actor *> followers;   // 0x0 (PS2 0x10)
    S_vector pos;                       // 0x10 (PS2 0x0), where the car takes the group (PS2's AddFollower's)
    float unknown_1c;                   // 0x1c
    uint flags;                         // 0x20, FOLLOWF_*
    int state;                          // 0x24, FOLLOWSTATE_*
    uint stop_end;                      // 0x28, a time in ms: the end of FOLLOWSTATE_4, 0 out of it (the name is ours)
    uint unknown_2c;                    // 0x2c, a time in ms: 6 s after the car set out
    C_actor *leader;                    // 0x30, the actor the group follows (PS2's AddFollower's second parameter)
    C_car *car;                         // 0x34, the car whose chase ended (FOLLOWF_MUSTLEAVE's)
};

class ai_follow_manager
{
public:
    std::vector<ai_follow_group> groups;    // 0x0
    // 0x10, a time in ms: Update's next look at the followers' jobs, cleared by Init and Destroy (the name is ours)
    uint check_time;

    // PS2 names
    void Init();
    void Destroy();
    // The following's frame, which C_game::Tick calls (PS2 name)
    void Update();
    // An actor's event for his followers (PS2 name; the player's Do_AB_OwnerCar gives 4 for a passenger's action
    // button)
    void Event(uint event, C_actor *actor);
    // Forgets an actor (PS2 name), which C_game::InvalidateActor calls
    void Invalidate(C_actor *act);
    // The saved game's record of the following, its size and the record (PS2 names)
    uint GetSize();
    bool Save(void **buf);
    // The following from a saved game's record (PS2 name; the result's type is a guess)
    bool Load(void **buf);
};

// A member's state in its party (ai_fight_member::state; the names are ours, by value): the fights' Tick moves the
// members through them, and locks the entity's substate with the state's number in each but 2
#define FIGHTSTATE_0 0
#define FIGHTSTATE_1 1
#define FIGHTSTATE_2 2
#define FIGHTSTATE_3 3
#define FIGHTSTATE_4 4                  // on the way to the car's seat
#define FIGHTSTATE_5 5                  // at the seat
#define FIGHTSTATE_6 6                  // out of the car
#define FIGHTSTATE_7 7                  // in the car, the driver
#define FIGHTSTATE_8 8                  // in the car
#define FIGHTSTATE_9 9
#define FIGHTSTATE_10 10
#define FIGHTSTATE_11 11                // at a seat someone else has

// A member of a fighting party (PS2 ai_fight_member), 0x14 bytes
struct ai_fight_member
{
    C_entity *entity;                   // 0x0
    int state;                          // 0x4, FIGHTSTATE_*
    int unknown_8;                      // 0x8
    // 0xc, 1 where the entity is of the commander's group, else 0 (PS2's SetCommander; the name is ours)
    int same_group;
    int unknown_10;                     // 0x10
};

// ai_fight_followship::type (the names are guesses): the party on foot or in its car
#define FIGHTPARTY_ON_FOOT 0
#define FIGHTPARTY_IN_CAR 1
// What ai_fight_followship::stand_time is set to once it reaches 10 s and the hunt is given up (the name is ours)
#define FIGHT_HUNT_ENDED 1000000

// A fighting party (PS2 ai_fight_followship), 0x48 bytes (PS2's 0x60). Its destructor's copy the linker kept is
// AIManager's file's (0x538c30, folded with the police actions'). The members' names are ours.
struct ai_fight_followship
{
    S_vector pos;                       // 0x0, the party's position (ComputePosition's)
    S_vector unknown_c;                 // 0xc, set from the second vector of ai_group::GetCarClosestAndUsable
    std::vector<ai_fight_member> members;   // 0x18 (PS2 0x20)
    // 0x28 (PS2 0x2c), the largest square of a member's distance from pos (PS2's ComputePosition)
    float max_dist_sq;
    // 0x2c (PS2 0x30), the game's time at which Tick looks at the party again (unsigned: Tick compares it so)
    uint check_time;
    int type;                           // 0x30 (PS2 0x34), FIGHTPARTY_*
    int unknown_34;                     // 0x34 (PS2 0x38), 0, 1 or 2
    // 0x38 (PS2 0x3c), while the car hunts: 2 s more at each of Tick's looks that finds it standing with all the
    // members in, else 4 s; FIGHT_HUNT_ENDED once it reaches 10 s (unsigned: Tick compares it so)
    uint stand_time;
    // 0x3c (PS2 0x40), the member of a group with cars, or else of a group, or else the first (PS2's SetCommander):
    // its group gives the cars
    C_entity *commander;
    C_human *target;                    // 0x40 (PS2 0x44)
    C_car *car;                         // 0x44 (PS2 0x48), the party's car, GROUP_DISBAND's

    // PS2 names: the party's position, whether its car hunts the target, and a new party of the members that take the
    // car (NULL for none), which Tick copies into the parties and deletes
    void ComputePosition();
    bool mustCarHunt();
    ai_fight_followship *TakeCar(C_car *car);
};

// PS2's has its parties only, as the PC's: AIManager's next member is PS2's sight manager too
class ai_fight_manager
{
public:
    std::vector<ai_fight_followship> parties;   // 0x0

    // PS2 names
    void Init();
    void Destroy();
    // The fights' frame, which C_game::Tick calls (PS2 name)
    void Tick();
    // The entity, or else the actor, leaves its party
    void LeaveParty(C_entity *e, C_actor *act);
    // The party of the entity, or else of the actor, NULL for none; the third parameter receives a number of it
    ai_fight_followship *GetParty(C_entity *e, C_actor *act, uint *unknown);
    // The party in a car goes apart (PS2 name)
    void DisbandCarParty(ai_fight_followship *party, bool unknown);
    // Forgets an actor (PS2 name), which C_game::InvalidateActor calls
    void Invalidate(C_actor *act);
};

// The copies of the implicit functions and the vectors' helpers that the managers' Init and Destroy call out of line:
// this file's (0x530390-0x53857c), but the folded ones the original kept elsewhere
// SYNTHETIC: GAME 0x531060
// ??1ai_group@@QAE@XZ

// TEMPLATE: GAME 0x536460
// ?_Destroy@?$vector@Uai_group@@V?$allocator@Uai_group@@@std@@@std@@IAEXPAUai_group@@0@Z

// What the fights' Tick calls out of line: the parties' _Destroy (its erase's) and implicit operator= (the erase's
// copy); the original's push_back calls insert(P, x) out of line too (0x536490, with insert(P, 1, x) inlined in it),
// which ours inlines
// TEMPLATE: GAME 0x536690
// ?_Destroy@?$vector@Uai_fight_followship@@V?$allocator@Uai_fight_followship@@@std@@@std@@IAEXPAUai_fight_followship@@0@Z

// The helpers of the vectors of 0x14-byte elements, folded (the parties' members, the groups' events, the game's
// streams, whose C_game::CreateIShow calls them, the semaphores): the copies the original kept are this file's
// TEMPLATE: GAME 0x537050
// ?_Ucopy@?$vector@Uai_fight_member@@V?$allocator@Uai_fight_member@@@std@@@std@@IAEPAUai_fight_member@@PBU3@0PAU3@@Z

// TEMPLATE: GAME 0x537090
// ?_Ufill@?$vector@US_i_show@C_game@@V?$allocator@US_i_show@C_game@@@std@@@std@@IAEXPAUS_i_show@C_game@@IABU34@@Z

// TEMPLATE: GAME 0x537130
// ?copy@std@@YIPAUS_stop_sounds@C_game@@PAU23@00@Z

// TEMPLATE: GAME 0x537910
// ?_Construct@std@@YIXPAUS_i_show@C_game@@ABU23@@Z

// SYNTHETIC: GAME 0x537a50
// ??4ai_group@@QAEAAU0@ABU0@@Z

// SYNTHETIC: GAME 0x537e10
// ??4ai_fight_followship@@QAEAAU0@ABU0@@Z

// The parties' members' operator=, folded with the groups' events' (both elements 0x14 bytes)
// TEMPLATE: GAME 0x538280
// ??4?$vector@Uai_group_event@@V?$allocator@Uai_group_event@@@std@@@std@@QAEAAV01@ABV01@@Z

// The parties' destructor, folded with the police actions' (both a vector at 0x18): the original kept AIManager's
// file's copy
// SYNTHETIC: GAME 0x538c30
// ??1ai_fight_followship@@QAE@XZ

// A copy of 4-byte elements, folded with every other one: the original kept a later file's copy
// TEMPLATE: GAME 0x564750
// ?copy@std@@YIPAPADPAPAD00@Z
