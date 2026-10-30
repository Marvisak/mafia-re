#pragma once

#include "actors/C_actor.h"
#include "mission/Mission.h"
#include "ai/pathweb.h"

class C_program;

// The wagons (PS2 C_wagon, C_wagon.cpp), 0x1a8 bytes (PS2's 0x1e0). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_WAGON, and what the scripts call.
// VTABLE: GAME 0x624e78
class C_wagon : public C_actor
{
public:
    C_wagon();
    virtual ~C_wagon();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual void SaveData(C_chunk *chunk) {}
    virtual void LoadData(C_chunk *chunk) {}
    // The base's: the original's jumps there, a copy the linker folded
    // FUNCTION: GAME 0x443860 FOLDED
    virtual void DuplicateFrom(C_actor *act)
    {
        C_actor::DuplicateFrom(act);
    }
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours, after the
    // scripts' WAGON_SETEVENT and WAGON_GETLASTNODE)
    // FUNCTION: GAME 0x47b6f0
    void SetEvent(C_program *program, int id)
    {
        event_program = program;
        event_id = id;
    }
    // The unknown_e of the path's first node, 0 without a path. The size is tested around the read: with an early
    // return for 0 VC6 reads the vector's array again. Either way it stores the size to a slot nothing reads, as the
    // original's copy does.
    // FUNCTION: GAME 0x47b710
    int GetLastNode()
    {
        if (path.size())
            return act_mission->web_path->nodes[path[0]].unknown_e;
        return 0;
    }

    uchar unknown_70[0xc];              // 0x70
    std::vector<uint> path;             // 0x7c (PS2 0x100), the web path's nodes (the name is ours)
    uchar unknown_8c[0x10c];            // 0x8c
    // The script and the event WAGON_SETEVENT gave the wagon (PS2 0x1cc and 0x1d0, the names are ours)
    C_program *event_program;           // 0x198
    int event_id;                       // 0x19c
    uchar unknown_1a0[8];               // 0x1a0
};

// SYNTHETIC: GAME 0x4f1c00
// C_wagon::`scalar deleting destructor'
