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

// C_car::unbreakable's value, which the scripts' CAR_UNBREAKABLE sets or clears (the name is ours; PS2's sets 0xc000)
#define CARF_UNBREAKABLE 0x4000

// The value the fights' Tick gives C_car::death_fade_time, 1332 ms of its 4000 (the name is ours)
#define CAR_DEATH_FADE_START 0x534

// A tuning level of the car (the name is ours): the vector's element, whose size the original's stride gives and
// whose members are unknown
struct S_car_level
{
    uchar unknown_0[0x113c];
};

// A seat of the car (the name is ours; GetOwner's members only)
struct S_car_seat
{
    uchar unknown_0[0xc];
    bool occupied;                      // 0xc
    uchar unknown_d[3];
    C_human *owner;                     // 0x10
    uchar unknown_14[0x8c];
};

// A car (PS2 C_car, C_car.cpp), 0x221c bytes (PS2's 0x2490), an actor with a C_Vehicle at 0x70 (PS2's constructor
// constructs both, the PC's stores a second vtable there, 0x6236b0). Only what InitSystem, the profiles,
// C_mission::CreateActor and the scripts (C_program::Process) use so far: its tables, which InitSystem loads once
// (C_game::Init loads them again, with the game's tables loaded), and which C_Vehicle keeps, the force feedback's
// scales, the constructor, a stub, and the scripts' functions and accessors (GetLevelsCount, GetOwner, SetEnterProtect1,
// GetCarWheelColl, SetCarWheelColl and GetActorSpeed are written).
// VTABLE: GAME 0x623618 C_actor
// VTABLE: GAME 0x6236b0 C_Vehicle
class C_car : public C_actor, public C_Vehicle
{
public:
    C_car();
    virtual ~C_car();
    // The vtable's other overrides (the slots' names are C_actor's): stubs that log and run the base's, but the
    // linker's folded constant functions and those that only call the base's
    virtual int vfunc_0(int a);
    virtual void vfunc_1(int a);
    virtual void vfunc_2(int a);
    virtual bool vfunc_3(int a);
    virtual S_vector vfunc_4(int a);
    virtual bool vfunc_9();
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
    virtual void ChangeState();
    virtual void GetActorSpeed(S_vector &speed);
    virtual bool Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
        I3D_frame *frm);
    virtual bool Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
        uint u, I3D_frame *frm);
    virtual int GetMaterialID(I3D_frame *frm);
    virtual bool IsPossibleColl(C_actor *act, S_vector v1, S_vector v2, float f, float &dist, C_actor **coll);
    virtual void SoundsChange(int i);

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
    // The car taken out of the game (PS2 name and parameter), which C_mission's FrmActBuff_StoreActor and
    // FrmActBuff_StoreActiveActor call with 2 for a kept car
    void DeactivateCar(uint unknown);
    // PS2 name and parameters, which C_mission::FrmActBuff_IsActiveActor calls with 0 and true
    void Reset(float unknown, bool unknown2);
    // PS2 name; the scripts' CAR_REPAIR passes true
    void _RepairPosition(bool unknown);
    // The number of tuning levels the car has (PS2 name)
    int GetLevelsCount();
    // The car's tuning level (PS2 names): G_CarDatabase::GetCarParams' level to set, false for one the car hasn't
    int GetActualLevel();
    bool SetActualLevel(int level);
    // The human in a seat (PS2 name), NULL for none
    C_human *GetOwner(int seat);
    // The actor into a seat, NULL to empty it (PS2 name and parameters; the names are ours): false for a seat the car
    // hasn't
    bool SetOwner(C_actor *act, int seat, float unknown);
    // The cars' force feedback effects, which C_game's Init and Done create and free when the profile has them (PC
    // only, the names are ours)
    static void InitFFEffects();
    static void DoneFFEffects();
    // Blows the car up (PS2 name); true
    bool CarExplosion(uint unknown);
    // PS2 name; the scripts call it after C_Vehicle::Deactivate(0), as PS2's
    void ResetCar();
    // The car's energy for the HUD (PS2 name)
    float ShowCarEnergy();
    // Keeps the humans from getting in or out (PS2 name)
    void SetEnterProtect(bool protect);
    // The same for one seat (PS2 name)
    void SetEnterProtect1(int seat, bool on);
    // Each wheel's collision flag into colls or out of it, at most num of them (PS2 names)
    void GetCarWheelColl(bool *colls, int num);
    void SetCarWheelColl(bool *colls, int num);
    // Whether the car can be driven (PS2 name and parameter): G_LoadSaveClass::GarageGameDone asks with 1
    bool IsCarUsable(uint flags);
    // PS2 names and parameters, which C_traffic_generator::TryGoToAssHole calls: a place by the car and its direction,
    // false for none; the four points around the car, in its model's space; and the time it has been parked, in ms
    bool GetPlacementFromTCK1(int unknown, S_vector &pos, S_vector &dir, bool unknown2);
    void GetNavigationPoints(float unknown, S_vector &p1, S_vector &p2, S_vector &p3, S_vector &p4);
    // A point of the car in world space: one of the model's two points (0x2080, 0x208c) times the model matrix,
    // kept in 0x2098 or 0x20a4 (PS2 name and parameter; which point the parameter picks isn't known)
    const S_vector &GetAIPos(bool b);
    uint GetTimeParking();

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours, most after
    // the scripts' commands)
    // FUNCTION: GAME 0x47b3a0
    void SetTankHitCount(int count)
    {
        tank_hits_left = tank_hit_count = count;
    }
    // FUNCTION: GAME 0x47b3c0
    void SetMustSteal(bool on)
    {
        must_steal = on;
    }
    // FUNCTION: GAME 0x47b3d0
    void SetDestroyMotor(bool destroy)
    {
        destroy_motor = destroy;
    }
    // FUNCTION: GAME 0x47b3e0
    void SetBreakMotor(bool on)
    {
        break_motor = on;
    }
    // FUNCTION: GAME 0x47b3f0
    char *GetModelName()
    {
        return model_name;
    }
    // FUNCTION: GAME 0x47b400
    void SetShowDamage(bool show)
    {
        show_damage = show;
    }
    // FUNCTION: GAME 0x47b410
    void Lock(bool lock)
    {
        if (lock)
            locks++;
        else
            locks--;
    }
    // FUNCTION: GAME 0x47b440
    void SetTemporary(bool b)
    {
        temporary = b;
    }
    // The old value
    // FUNCTION: GAME 0x47b450
    int SetShowEnergy(int show)
    {
        int old = show_energy;
        show_energy = show;
        return old;
    }
    // FUNCTION: GAME 0x47b470
    void SetUnknown217c(bool on)
    {
        unknown_217c = on;
    }
    // FUNCTION: GAME 0x47b480
    void SetAxis(int value)
    {
        axis = value;
    }
    // FUNCTION: GAME 0x47b490
    void SetUnbreakable(uint flags)
    {
        unbreakable = flags;
    }
    // The vehicle's dynamic collisions off while the car is invisible, its second flag kept
    // FUNCTION: GAME 0x47b4b0
    void SetInvisible(bool on)
    {
        SetDynamicCollsOn(!on, unknown_1fac);
    }

    uchar unknown_20b0[2];              // 0x20b0
    // 0x20b2 (PS2 0x246a; the name is ours): PS2's Init sets it for the models POLCAD, POLI, POLIMFOR and POLIMTUD;
    // G_LoadSaveClass::GarageGameDone doesn't take such a car into the garage
    bool police_model;
    uchar unknown_20b3[0x19];
    C_actor *unknown_20cc;              // 0x20cc
    int parking_time;                   // 0x20d0, the game's time GetTimeParking counts from (the name is ours)
    uchar unknown_20d4[0x1d];
    // The names of these are ours, after the scripts' commands
    bool show_damage;                   // 0x20f1, SHOWCARDAMAGE's
    uchar unknown_20f2[4];              // 0x20f2
    bool must_steal;                    // 0x20f6 (PS2 0x2472), CAR_MUSTSTEAL's
    uchar unknown_20f7;                 // 0x20f7
    int axis;                           // 0x20f8 (PS2 0x23c8), CAR_SETAXIS sets 1
    uint unbreakable;                   // 0x20fc (PS2 0x23cc), CARF_UNBREAKABLE or 0
    uchar unknown_2100[4];              // 0x2100
    // 0x2104 (PS2 0x2473), a car in the police's service (the name is ours): PS2's C_shvestky sets it for the cars it
    // sends and its PocetPolicejnichPlechovekVRici ("the number of police cans in the region") counts the cars with it;
    // a car leaving the police's faces with it set runs C_traffic_car::ChlupatejVystrik
    bool police_car;
    uchar unknown_2105[8];              // 0x2105
    bool break_motor;                   // 0x210d (PS2 0x2477), CAR_BREAKMOTOR's
    uchar unknown_210e[0xb];            // 0x210e
    // 0x2119 (PS2 0x2479), set with the camera's CAMPF_HIDE_CAR, which turns the car's frame off (G_Camera::Begin),
    // cleared by G_Camera::End (the name is ours)
    bool hidden_by_camera;
    uchar unknown_211a[0x3a];           // 0x211a
    // SetTankHitCount's (PS2 0x23f0 and 0x23f4, the names are ours): PS2's Hit counts the first down, GameInit and Reset
    // set it to the second again
    int tank_hits_left;                 // 0x2154
    int tank_hit_count;                 // 0x2158
    uchar unknown_215c[0x14];           // 0x215c
    // 0x2170 (PS2 0x2463), the car drives where it mustn't (the name is ours): PS2's AI clears it and sets it again
    // where it calls C_shvestky::CarIllegalPath, for a wheel on a surface of type 3; the police's SHVF_ILLEGAL_PATH
    // ends without it
    bool illegal_path;
    uchar unknown_2171[7];              // 0x2171
    int show_energy;                    // 0x2178, CAR_SWITCHSHOWENERGY's (the name is ours)
    bool unknown_217c;                  // 0x217c (PS2 0x2464), CARLIGHT_MAIN clears it with the lights on, sets it off
    uchar unknown_217d[0xf];            // 0x217d
    // 0x218c (PS2 0x240c), the model's name, which the constructor clears and C_traffic_generator::PlayerInTaxi
    // compares with the taxis' (the name is ours)
    char model_name[32];
    int actual_level;                   // 0x21ac, GetActualLevel's (the name is ours)
    // GetLevelsCount's (the name is ours): 0x113c bytes each
    std::vector<S_car_level> v_levels;  // 0x21b0
    int unknown_21c0;                   // 0x21c0
    // SetEnterProtect1's one bit a seat (the name is ours)
    uint enter_protect;                 // 0x21c4
    // GetOwner's (the name is ours): 0xa0 bytes each
    std::vector<S_car_seat> v_seats;    // 0x21c8
    uchar unknown_21d8[4];              // 0x21d8
    int locks;                          // 0x21dc, CAR_LOCK's count (Lock; the name is ours)
    uchar unknown_21e0[0x2a];           // 0x21e0
    bool destroy_motor;                 // 0x220a, CAR_SETDESTROYMOTOR's (the name is ours)
    uchar unknown_220b;                 // 0x220b
    // 0x220c (PS2 0x247f; the name is a guess), set for the cars the player left in the garage, and by the scripts'
    // DAN_INTERNAL_1; C_game::Done removes such cars (PS2's C_game::VyhjebejTemporaryho, "throw the temporary one
    // out", clears it, CarExplosion sets it)
    bool temporary;
    uchar unknown_220d[3];              // 0x220d
    // 0x2210 (PS2 0x23ac; the name is a guess), set to CAR_DEATH_FADE_START or 0 by the fights' Tick when its party's
    // hunt ends; in ms, which PS2's Update counts up with a flag of its (PS2 0x2480) set, to C_game::AddDeathHumanFade
    // at 4000
    int death_fade_time;
    bool unknown_2214;                  // 0x2214
    bool unknown_2215;                  // 0x2215
    uchar unknown_2216[3];              // 0x2216
    // The names are ours, after the scripts' commands
    bool in_water;                      // 0x2219 (PS2 0x248a), CAR_INWATER reads it
    bool invisible;                     // 0x221a, CAR_INVISIBLE sets it
    uchar unknown_221b;                 // 0x221b
};

// SYNTHETIC: GAME 0x41bc90
// C_car::`scalar deleting destructor'

// Set while the scripts' RECLOADFULL plays a cutscene's record; the cars read it (the names are ours)
extern bool record_playing;

// The flag's setter, whose copy is among the accessors at the end of the file at 0x460a50
// FUNCTION: GAME 0x47b4a0
inline void SetRecordPlaying(bool on)
{
    record_playing = on;
}
