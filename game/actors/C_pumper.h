#pragma once

#include "actors/C_actor.h"

class C_car;

// C_pumper::CanPump's results (the names are ours): 1 the car can be refuelled, 2 the attendant is busy (the action
// button does nothing)
#define PUMPER_CAN_PUMP 1
#define PUMPER_BUSY 2

// The petrol pump attendants (PS2 C_pumper, C_pumper.cpp), 0x2ac bytes (PS2's 0x2e0). Only the constructor so far,
// which C_mission::CreateActor calls for ACTOR_PUMPER, and ScriptCanPump, which the script calls, as stubs.
// VTABLE: GAME 0x624990
class C_pumper : public C_actor
{
public:
    C_pumper();
    virtual ~C_pumper();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual bool SaveData(C_chunk *chunk);
    virtual bool LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);

    // Whether the attendant can serve the car, PUMPER_* (PS2 name)
    int CanPump(C_car *car);
    // The attendant refuels the car (PS2 name)
    void Start(C_car *car);
    // Whether the mission's attendant of the id can pump (PS2 name): its int the script's float register gets
    static int ScriptCanPump(uint id);

    uchar unknown_70[0x23c];            // 0x70
};

// SYNTHETIC: GAME 0x47c6e0
// C_pumper::`scalar deleting destructor'
