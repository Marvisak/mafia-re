#pragma once

#include "actors/C_actor.h"
#include "vehicles/C_Vehicle.h"

struct S_SRFC;
class C_human;

// A surface's grip curve as tables\tyres.bin has it (S_SRFC_CURVE before ConvertTyres), 0x24 bytes; the names are ours
struct S_TSRFC_CURVE
{
    float unknown_00;                   // 0x0
    float angle;                        // 0x4, in degrees
    float unknown_08;                   // 0x8
    float unknown_0c;                   // 0xc
    float min_1;                        // 0x10
    float max_1;                        // 0x14
    float min_2;                        // 0x18
    float max_2;                        // 0x1c
    float unknown_20;                   // 0x20
};

// A tyre on one material as the file has it (PS2 S_TSRFC), after the material: 0x4c bytes in the file, of which
// ConvertTyres reads 0x48
struct S_TSRFC
{
    S_TSRFC_CURVE curve[2];             // 0x0
};

// A car (PS2 C_car, C_car.cpp), 0x221c bytes (PS2's 0x2490), an actor with a C_Vehicle at 0x70 (PS2's constructor
// constructs both, the PC's stores a second vtable there, 0x6236b0). Only what InitSystem, the profiles and
// C_mission::CreateActor use so far: its tables, which InitSystem loads once (C_game::Init loads them again, with the
// game's tables loaded), and which C_Vehicle keeps, the force feedback's scales, and the constructor, a stub.
class C_car : public C_actor, public C_Vehicle
{
public:
    C_car();

    // The profile's force feedback options, which the cars' effects scale by (PC only, the names are ours, after the
    // options), 1 until G_LoadSaveClass::Refresh sets them
    static float ff_volume;
    static float ff_tire_grip;
    static float ff_collisions;
    static float ff_suspension;
    static float ff_revs;

    // From materialList, one default material without it
    static bool LoadMaterials();
    // An emitter for each of gameParticleTable's, one without an emitter without it
    static bool LoadParticles();
    // From tables\tyres.bin
    static bool LoadTyres();
    static void ConvertTyres(S_TSRFC *src, S_SRFC *dst);
    // Sets the force feedback's scales (PC only, the name is ours)
    static void SetForceFeedback(float volume, float tire_grip, float collisions, float suspension, float revs);

    // The model matrix's inverse, made from the frame's world matrix first with update (PS2 name)
    const S_matrix &GetInvModelMatrix(bool update);
    // PS2 name and parameters, which C_mission::FrmActBuff_IsActiveActor calls with 0 and true
    void Reset(float unknown, bool unknown2);
    // The car's tuning level (PS2 names): G_CarDatabase::GetCarParams' level to set, false for one the car hasn't
    int GetActualLevel();
    bool SetActualLevel(int level);
    // The human in a seat (PS2 name), NULL for none
    C_human *GetOwner(int seat);
    // Whether the car can be driven (PS2 name and parameter): G_LoadSaveClass::GarageGameDone asks with 1
    bool IsCarUsable(uint flags);
    // The cars' force feedback effects, which C_game's Init and Done create and free when the profile has them (PC
    // only, the names are ours)
    static void InitFFEffects();
    static void DoneFFEffects();

    uchar unknown_20b0[2];              // 0x20b0
    // 0x20b2 (PS2 0x246a): a car G_LoadSaveClass::GarageGameDone doesn't take into the garage
    bool unknown_20b2;
    uchar unknown_20b3[0x66];
    // 0x2119 (PS2 0x2479), set with the camera's CAMPF_UNKNOWN_2 (G_Camera::Begin), cleared by G_Camera::End
    bool unknown_2119;
    uchar unknown_211a[0x72];
    // 0x218c (PS2 0x240c), the model's name, which the constructor clears and C_traffic_generator::PlayerInTaxi
    // compares with the taxis' (the name is ours)
    char model_name[32];
    int actual_level;                   // 0x21ac, GetActualLevel's (the name is ours)
    uchar unknown_21b0[0x5c];           // 0x21b0
    bool unknown_220c;                 // 0x220c, set for the cars the player left in the garage
    uchar unknown_220d[0xf];            // 0x220d
};
