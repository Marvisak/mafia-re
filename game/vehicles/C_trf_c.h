#pragma once

#include "actors/C_actor.h"

class C_car;

// The size of each of C_traffic_car::names (the name is a guess)
#define TRAFFIC_NAME_SIZE 0x20

// The traffic's cars (PS2 C_traffic_car, C_trf_c.cpp), 0x26c bytes (PS2's 0x290). The constructor, which
// C_mission::CreateActor calls for ACTOR_TRAFFIC_CAR, is a stub; written: GetParkingCars, SaveGameGetSize and
// DuplicateFrom.
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
    // A count of the traffic's down by one, at least 0 (PS2 name, Czech slang; static: C_shvestky::TickDoRiciActors
    // calls it with nothing in ecx, PS2's with nothing in a0)
    static void ChlupatejVystrik();

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

    uchar unknown_70[0x1ac];            // 0x70
    // DuplicateFrom's fields
    int unknown_21c;                    // 0x21c
    uchar unknown_220[4];               // 0x220
    int unknown_224;                    // 0x224
    int unknown_228;                    // 0x228
    int unknown_22c;                    // 0x22c
    uchar unknown_230[4];               // 0x230
    // The names PS2's LoadData reads and DuplicateFrom copies, TRAFFIC_NAME_SIZE bytes each (the names are ours)
    char *names;                        // 0x234
    int num_names;                      // 0x238
    uchar unknown_23c[0x30];            // 0x23c

    static std::vector<S_ELEMENT *> v_elems_parking;
    // The traffic's cars (PS2 name; the third of the file's six vectors, 0x63d844)
    static std::vector<S_ELEMENT *> v_elems;
    // Its accessor (the name is ours), whose copy is among C_sledovacka.cpp's
    // FUNCTION: GAME 0x4a9ca0
    static std::vector<S_ELEMENT *> &GetElems()
    {
        return v_elems;
    }

    // Two of the class's three coefficients (PS2 names, most likely: C_game::DoneIntern sets both to 1 after the
    // pedestrians' two ratios, as PS2's sets elems_coefff and police_coefff; PS2's GameInit reads them, its
    // G_LoadSaveClass::FreerideSetupFromScript sets them). The third, PS2's mafian_coefff, is most likely the original's
    // 0x63d800, between them.
    static float elems_coefff;
    static float police_coefff;
};

// SYNTHETIC: GAME 0x4aa810
// C_traffic_car::`scalar deleting destructor'
