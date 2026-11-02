#pragma once

#include "actors/C_human.h"
#include "mission/C_program.h"
#include "mission/Mission.h"
#include "mission/game.h"
#include "tools.h"

#include <vector>

// The AI's parts of an entity (PS2's classes, the members' names ours): only what the script's commands use so far.

// ai_action::type of the hostile job, and its unknown_20->unknown_38 there, which ENEMY_GETHOSTILESTATE tests (the
// names are ours, by value; PS2 the same)
#define AIACTION_UNKNOWN_1E 0x1e
#define AIACTION20_UNKNOWN_A 0xa
// The jobs ai_follow_manager::Update asks a following entity for: without the first or with the second it leaves its
// group (the names are ours, by value); C_entity::IsInDanger looks for the second among others
#define AIACTION_UNKNOWN_1 1
#define AIACTION_UNKNOWN_6 6

// What a job's unknown_20 points to (the name is ours): only the value ENEMY_GETHOSTILESTATE compares (PS2's 0x44)
struct ai_action_unknown_20
{
    uchar unknown_0[0x38];              // 0x0
    int unknown_38;                     // 0x38, AIACTION20_*
};

// A job of an entity (PS2 ai_action), whose size isn't known
struct ai_action
{
    int type;                           // 0x0, AIACTION_*
    uchar unknown_4[8];                 // 0x4
    int unknown_c;                      // 0xc, GetBaseJob takes the last job where it is 0 (most likely a parent)
    uchar unknown_10[0x10];             // 0x10
    ai_action_unknown_20 *unknown_20;   // 0x20 (PS2 0x20)
};

// The entity's jobs (PS2 ai_action_manager), C_entity's 0x1164: three vectors of jobs first (the constructor's and
// Clear's), whose size isn't known
class ai_action_manager
{
public:
    std::vector<ai_action *> unknown_0;     // 0x0, the jobs GetBaseJob walks from the last
    std::vector<ai_action *> unknown_10;    // 0x10
    std::vector<ai_action *> unknown_20;    // 0x20

    // PS2 names: the jobs ended, and the last job whose unknown_c is 0, NULL for none
    void Clear(bool unknown);
    ai_action *GetBaseJob();
    // Whether the entity has a job of the type, which the second parameter receives where it isn't NULL (PS2 name)
    bool hasAction(uint type, ai_action **action, bool unknown);
};

// What the entity has seen (PS2 ai_vis_logs), C_entity's 0xb04, 0xb0 bytes: Init clears its eight entries and keeps
// its owner last
class ai_vis_logs
{
public:
    uchar unknown_0[0xac];              // 0x0
    void *owner;                        // 0xac

    // PS2 name
    void Init(void *owner);
};

// The entity's senses (PS2 ai_sensors), C_entity's 0xbb4, 0x2d4 bytes: Init clears its contacts, three arrays of eight,
// and keeps its owner before the last twelve dwords
class ai_sensors
{
public:
    uchar unknown_0[0x2a0];             // 0x0
    void *owner;                        // 0x2a0
    uchar unknown_2a4[0x30];            // 0x2a4

    // PS2 name
    void Init(void *owner);
};

// A log of another character (PS2 ai_character_log), 0x54 bytes: what it holds isn't known yet
struct ai_character_log
{
    uchar unknown_0[0x24];              // 0x0
    uint unknown_24;                    // 0x24, a game's time: the fights' Tick counts the 3 s after it
    uchar unknown_28[0xc];              // 0x28
    uint unknown_34;                    // 0x34, a game's time: the fights' Tick counts the 2 s after it
    uchar unknown_38[0x1c];             // 0x38
};

// What the entity remembers of the other characters (PS2 ai_logs), C_entity's 0xe88, whose size isn't known
class ai_logs
{
public:
    uchar unknown_0[0x20];              // 0x0
    std::vector<ai_character_log> logs; // 0x20

    // The log of the actor, NULL for none (PS2 name)
    ai_character_log *GetLog(C_actor *act);

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours): PS2 clears
    // the vector itself
    // FUNCTION: GAME 0x47ba20
    void Clear()
    {
        logs.clear();
    }
};

// C_entity::unknown_f50's bits (the names are ours, by value): HUMAN_RETURNFROMPANIC clears the first with a nonzero
// parameter, GROUP_DISBAND sets the second for the members without a car and clears it when they go free
#define ENTITYF_UNKNOWN_1 0x1
#define ENTITYF_UNKNOWN_2 0x2

// What C_entity::ai_state adds to the state (the names are ours, by value): GetAIState and GETENEMYAISTATE take them
// off, PS2's hasLockedState compares the state with 666
#define ENTITY_STATE_10000 10000
#define ENTITY_STATE_666 666

// C_entity::Event's events (the names are ours, by value): the fights' Tick sends ENTITY_EVENT_UNKNOWN_B with
// an ai_blob, the target of the entity's fighting party, which the entity deletes; C_shvestky::Tick sends
// ENTITY_EVENT_UNKNOWN_13 to the entities of a road block that ends
#define ENTITY_EVENT_UNKNOWN_B 0xb
#define ENTITY_EVENT_UNKNOWN_13 0x13

// The AI's humans (PS2 C_entity, jp_ai_entity.cpp), 0x128c bytes (PS2's 0x14a0): C_program is the second base (at
// 0xa9c, PS2's 0xbf0, whose this adjustors PS2 names @3056@), the entity's own script, as C_detector's. Only the
// constructor so far, which C_mission::CreateActor calls for ACTOR_ENTITY, what C_game::EntityBlock calls, and what
// the script's commands use.
// VTABLE: GAME 0x625064 C_program
// VTABLE: GAME 0x625090 C_human
class C_entity : public C_human, public C_program
{
public:
    C_entity();
    virtual ~C_entity();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual void SaveData(C_chunk *chunk);
    virtual void LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();
    virtual void Death();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);
    virtual std::vector<C_program::S_instruction> *GetProgram();
    // C_program's: the entity's commands (stubs)
    virtual bool DecodeParams(char *params, S_instruction &ins);
    virtual int DecodeInstruction(char *line, int &len);
    virtual void Process(uint time);
    virtual void DecompileCommand(int i, char *s);
    virtual void CallBack(E_cb_action action);
    // PS2 names, Czech for "sit on your backside and watch the movie" and "wake up, the credits are on": the entity
    // blocked for a cutscene and woken after it (C_game::EntityBlock)
    void SedniNaPrdelASledujKino(bool unknown);
    void VzbudSeUzSouTitulky();
    // Gives the entity its script back (PS2 name; PS2's takes two bools, the PC's one), which ENEMY_FORCESCRIPT asks
    // with false
    void ForceScript(bool unknown);
    // Takes the weapon of that id in its hand (PS2 name)
    void WeaponChoose(uint id);
    // Gives the game its action button object (PS2 name)
    void AddUsingObject();
    // PS2 names: the fights' Tick locks a substate with its member's state and passes the target's ai_blob through
    // the event ENTITY_EVENT_UNKNOWN_B, which the entity deletes
    void LockSubstate(uint unknown, uint unknown2, float unknown3, uint unknown4, bool unknown5);
    void Event(C_actor *act, uint id, void *data);
    // Whether one of the game's entities has one of five jobs aimed at this one, AIACTION_UNKNOWN_1E among them (PS2
    // name), which ai_follow_manager::Update asks
    bool IsInDanger();

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours, after PS2's
    // types)
    // FUNCTION: GAME 0x47ba50
    ai_vis_logs *GetVisLogs()
    {
        return &vis_logs;
    }
    // FUNCTION: GAME 0x47ba60
    ai_sensors *GetSensors()
    {
        return &sensors;
    }
    // FUNCTION: GAME 0x47ba70
    ai_logs *GetLogs()
    {
        return &logs;
    }
    // The state without what was added to it (after GETENEMYAISTATE, PS2 inlines the same): VC6 counts the loop's
    // subtractions by a division
    // FUNCTION: GAME 0x47ba80
    uint GetAIState() const
    {
        uint state = ai_state;
        if (state >= ENTITY_STATE_10000)
            state -= ENTITY_STATE_10000;
        while (state >= ENTITY_STATE_666)
            state -= ENTITY_STATE_666;
        return state;
    }
    // After HUMAN_RETURNFROMPANIC (PS2 inlines it the same way)
    // FUNCTION: GAME 0x47bac0
    void ReturnFromPanic(bool on)
    {
        if (on)
            unknown_f50 &= ~ENTITYF_UNKNOWN_1;
        else
            unknown_f50 |= ENTITYF_UNKNOWN_1;
    }
    // FUNCTION: GAME 0x47baf0
    void SetUnknownF50Bit2(bool on)
    {
        if (on)
            unknown_f50 |= ENTITYF_UNKNOWN_2;
        else
            unknown_f50 &= ~ENTITYF_UNKNOWN_2;
    }
    // Takes the entity's action button object from the game's (PS2 inlines the call)
    // FUNCTION: GAME 0x47bb20
    void DelUsingObject()
    {
        act_mission->GetGame()->using_object.DelObject(&using_object);
    }

    ai_vis_logs vis_logs;               // 0xb04 (PS2 0xc50)
    ai_sensors sensors;                 // 0xbb4 (PS2 0xd00)
    ai_logs logs;                       // 0xe88 (PS2 0x1060)
    uchar unknown_eb8[0x88];            // 0xeb8
    uint ai_state;                      // 0xf40 (PS2 0x1128), ENTITY_STATE_* added
    uchar unknown_f44[0xc];             // 0xf44
    uint unknown_f50;                   // 0xf50 (PS2 0x1138), ENTITYF_*
    uchar unknown_f54[0x210];           // 0xf54
    ai_action_manager action_manager;   // 0x1164 (PS2 0x1348)
    uchar unknown_1194[0x18];           // 0x1194
    uint unknown_11ac;                  // 0x11ac (PS2 0x1380), the number of the entity's group (ai_groups::GetGroup)
    uchar unknown_11b0[0x80];           // 0x11b0
    // The entity's action button object, which the game's objects hold while it has one (PS2 0x1420; the name is ours)
    C_using_object::S_using_object using_object;  // 0x1230
    uchar unknown_1268[0x24];           // 0x1268, a vector first (the constructor's)
};

// SYNTHETIC: GAME 0x5072f0
// C_entity::`scalar deleting destructor'

// The adjustor thunks of the C_program vtable (this - 0xa9c), to the destructor and the save functions
// SYNTHETIC: GAME 0x530390
// ??_EC_entity@@WKJM@AEPAXI@Z

// SYNTHETIC: GAME 0x5303a0
// ?SaveGameGetSize@C_entity@@WKJM@AEHXZ

// SYNTHETIC: GAME 0x5303b0
// ?SaveGameSave@C_entity@@WKJM@AE_NPAPAX@Z

// SYNTHETIC: GAME 0x5303c0
// ?SaveGameLoad@C_entity@@WKJM@AE_NPAPAX@Z
