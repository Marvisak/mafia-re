#pragma once

#include "actors/C_actor.h"

class C_car;

// The traffic's cars (PS2 C_traffic_car, C_trf_c.cpp), 0x26c bytes (PS2's 0x290). Only the constructor so far, which
// C_mission::CreateActor calls for ACTOR_TRAFFIC_CAR.
// VTABLE: GAME 0x624c98
class C_traffic_car : public C_actor
{
public:
    C_traffic_car();
    virtual ~C_traffic_car();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual void AI(uint time);
    virtual void Update(uint time);
    virtual void NetDirect(uint time);
    virtual bool Init(I3D_frame *frm);
    virtual void SaveData(C_chunk *chunk);
    virtual void LoadData(C_chunk *chunk);
    virtual void DuplicateFrom(C_actor *act);
    virtual int SaveGameGetSize();
    virtual bool SaveGameSave(void **buf);
    virtual bool SaveGameLoad(void **buf);
    virtual void GameInit();
    virtual void GameDone();
    virtual void ChangeState();
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);
    virtual bool IsPossibleColl(C_actor *act, S_vector v1, S_vector v2, float f, float &dist, C_actor **coll);
    // A virtual function of its own (the original's last slot)
    virtual bool vfunc_38(int a, int b, int c, int d, int e, int f, int g, int h, int i, int j, int k, int l);

    // PS2 name (Czech, roughly "so now I buy them all from you"; PS2's takes a bool)
    void TakATedSiVasKupim_Vsecky();
    // PS2 name (Czech, roughly "he'll beat the car up")
    void VypraskeeeKaaaru(C_actor *act);

    // A car of the traffic's lists (PS2 C_traffic_car::S_ELEMENT; the names are ours)
    struct S_ELEMENT
    {
        uchar unknown_0[4];
        I3D_frame *frame;               // 0x4
        uchar unknown_8[4];
        C_car *car;                     // 0xc
    };

    // The parked cars (PS2 names), which C_traffic_generator::TryGoToAssHole sends pedestrians to
    static std::vector<S_ELEMENT *> &GetParkingCars();

    uchar unknown_70[0x1fc];            // 0x70

    static std::vector<S_ELEMENT *> v_elems_parking;

    // C_game::DoneIntern sets both to 1 (the names are ours)
    static float unknown_63d7fc;
    static float unknown_63d804;
};

// SYNTHETIC: GAME 0x4aa810
// C_traffic_car::`scalar deleting destructor'
