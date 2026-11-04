#pragma once

#include "actors/C_actor.h"

// A shot actor's record (C_actor_shot::AddToCutscene gives its size; the names are ours), 0x10 bytes
struct S_actor_shot_record
{
    int time;                           // 0x0, S_cutscene_record's
    int type;                           // 0x4
    // 0x8, the effect a particle starts with (C_effects::CreateParticle), or the item SetWeapon takes
    int id;
    uint flags;                         // 0xc, SHOT_REC_*
};

// S_actor_shot_record::flags (the names are ours): a particle started or stopped (and nothing else), or the
// weapon set, the shooting turned on (C_actor_shot::firing) and its shots fired (shoots)
#define SHOT_REC_PARTICLE_START 0x1
#define SHOT_REC_PARTICLE_STOP 0x2
#define SHOT_REC_FIRING 0x1000
#define SHOT_REC_WEAPON 0x2000
#define SHOT_REC_SHOOTS 0x8000

// PS2's C_actor_shot (C_actor_shot.cpp), 0x8c bytes (PS2's 0xa0). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_ACTOR_SHOT.
// VTABLE: GAME 0x625458
class C_actor_shot : public C_actor
{
public:
    C_actor_shot();
    virtual ~C_actor_shot();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void PlayRecord(int a);
    virtual void PlayStep(int a) {}
    virtual bool PlaceAtTime(int a);
    virtual S_vector GetRecordPos(int a);
    virtual S_quat GetRecordRot(int a);
    virtual bool AddToCutscene();
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf)
    {
        return true;
    }
    virtual bool SaveGameLoad(void **buf)
    {
        return true;
    }
    virtual void GameInit();
    virtual void GameDone();
    // The weapon the shots are of, an item (gameItemTable): its sound, and the frame's name's count (PS2 name)
    void SetWeapon(int weapon);

    uchar unknown_70[8];                // 0x70
    // 0x78, -1 (GameInit, PlayRecord) has Update start a shot, the item's times summed (the name is a guess)
    int shot_time;
    bool firing;                        // 0x7c, Update does nothing without it (the name is a guess)
    uchar unknown_7d[7];                // 0x7d
    bool shoots;                        // 0x84, Update fires C_game::NewShoot (the name is a guess)
    uchar unknown_85[3];                // 0x85
    // 0x88, the record's particle (CreateParticle's number), -1 from Init and for none (the name is ours)
    int particle;
};

// SYNTHETIC: GAME 0x566f60
// C_actor_shot::`scalar deleting destructor'
