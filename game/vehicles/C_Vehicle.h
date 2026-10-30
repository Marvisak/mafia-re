#pragma once

#include "Main.h"

#include <vector>

#include "I3D/I3D.h"

// A surface's grip curve of a tyre, as ConvertTyres makes it from the file's (S_TSRFC_CURVE), 0x2c bytes. The values'
// meanings aren't known: the first is scaled, the second turned from degrees into radians, and the two pairs of
// limits get their ranges. The names are ours.
struct S_SRFC_CURVE
{
    float unknown_00;                   // 0x0, the file's times 1000
    float angle;                        // 0x4, in radians
    float unknown_08;                   // 0x8
    float unknown_0c;                   // 0xc
    float min_1;                        // 0x10
    float max_1;                        // 0x14
    float range_1;                      // 0x18, max_1 - min_1
    float min_2;                        // 0x1c
    float max_2;                        // 0x20
    float range_2;                      // 0x24, max_2 - min_2
    float unknown_28;                   // 0x28
};

// A tyre on one material (PS2 S_SRFC), 0x5c bytes. PS2's is 0x34, of another design.
struct S_SRFC
{
    int material;                       // 0x0, an index into materialList
    S_SRFC_CURVE curve[2];              // 0x4
};

// A tyre (PS2 S_TYRE), 0x28 bytes: tables\tyres.bin's, or the "user" tyre SetTyres puts first
struct S_TYRE
{
    int id;                             // 0x0
    char name[0x20];                    // 0x4
    std::vector<S_SRFC> *surfaces;      // 0x24, NULL for the user tyre
};

// What a car's wheels do on a material (PS2 S_VEH_MATERIAL), 0x30 bytes, from materialList (C_car::LoadMaterials).
// The meanings aren't known; the comments give LoadMaterials' limits and defaults.
struct S_VEH_MATERIAL
{
    int id;                             // 0x0, its index in materialList
    float unknown_04;                   // 0x4, at most unknown_08; 100
    float unknown_08;                   // 0x8, 100
    float unknown_0c;                   // 0xc, at most unknown_10; 100
    float unknown_10;                   // 0x10, 100
    float unknown_14;                   // 0x14, 0 to 0.2 (asphalt 0.012, grass 0.07); 0
    float unknown_18;                   // 0x18, 0 to 1; 0.35
    float unknown_1c;                   // 0x1c, 1 to 2; 1
    int unknown_20;                     // 0x20
    int unknown_24;                     // 0x24
    int unknown_28;                     // 0x28
    bool unknown_2c;                    // 0x2c, set for asphalt, paving, pavement, concrete, rock, rails and brick wall
    bool unknown_2d;                    // 0x2d, the same
};

class I3D_emitor;

// A car's particle emitter (PS2 S_VEH_PARTICLE), 0x1c bytes, one for each of gameParticleTable's (C_car::
// LoadParticles). PS2's is 0x50, of another design (PS2 keeps a pool of them).
struct S_VEH_PARTICLE
{
    I3D_emitor *emitor;                 // 0x0, linked to the scene's primary sector; NULL if its model didn't load
    S_vector velocity;                  // 0x4, its PP_VELOCITY
    S_vector velocity_end;              // 0x10, PP_VELOCITY_END
};

// A vehicle's wheel (the name is ours; PS2's C_Vehicle has them through pointers too). Only what the camera reads.
struct S_wheel
{
    uchar unknown_0[0xc];
    // 0xc and 0x10 (PS2 0x15c and 0x30), what GetWheelCamPos places the camera by: the wheel's position and a distance
    // outwards, to the right for the odd wheels
    float unknown_c;
    S_vector unknown_10;
    uchar unknown_1c[0x60];
    S_vector unknown_7c;                // 0x7c, the camera "C In" tilts by their sum
};

// An element of C_Vehicle::unknown_ca0 (the name is ours; its members are unknown), 0x24 bytes
struct S_vehicle_unknown_ca0
{
    uchar unknown_0[0x24];
};

// C_Vehicle::lights' bits (the names are ours), which the setters at 0x47b220-0x47b380 and the scripts' CARLIGHT_*
// commands set: the lights themselves, then the projector lights' bits, each the projector of one light
#define VLIGHT_INDICATOR_L 0x1
#define VLIGHT_INDICATOR_R 0x2
#define VLIGHT_UNKNOWN_8 0x8            // the indicators' setters do nothing while it is set
#define VLIGHT_LIGHT 0x80
#define VLIGHT_MAIN 0x10000
#define VLIGHT_PROJECTOR_INDICATOR 0x100000
#define VLIGHT_PROJECTOR_LIGHT 0x200000
#define VLIGHT_PROJECTOR_MAIN 0x800000  // CAR_SETPROJECTOR sizes it
#define VLIGHT_PROJECTORS 0xff00000

// The vehicles' shared part (PS2 C_Vehicle, C_Vehicle.cpp and C_Vehicle_sounds.cpp), a base of C_car at 0x70 (the car
// offsets in the members' comments): the tables InitSystem loads through C_car, which CloseSystem frees through this
// class, as PS2's, the sounds' volume, what the camera reads and what the scripts (C_program::Process) call through
// C_car: the lights, the collisions, the doors. The size is a guess: the constructor (0x4bd8f0) sets members up to
// 0x2004, the car's own start at the car's 0x20b0 at the latest.
class C_Vehicle
{
public:
    // The scene the vehicles are in, which C_mission::Open sets (PS2 name)
    static I3D_scene *scene;
    // The name is ours. Open sets the scene through it: the inline call's cost, with the name copy's helper, gives the
    // budget that leaves Open's chunk destructors' calls out of line as the original's
    static void SetScene(I3D_scene *s)
    {
        scene = s;
    }
    // The vehicles' sounds' volume, the profile's for the cars relative to the sounds' (PC only, the name is ours): 1
    // until G_LoadSaveClass::Refresh sets it
    static float sound_volume;
    // The tyres, the "user" one first (PS2 name)
    static std::vector<S_TYRE> v_tyre;
    // The materials (PS2 name), and the index of each material's entry in it (PS2's is an int array)
    static std::vector<S_VEH_MATERIAL> v_materials;
    static uchar s_material_conv_tbl[0x100];
    // The particle emitters (PS2 name)
    static std::vector<S_VEH_PARTICLE> v_particle;
    // The sectors with projector lights and those with sounds (PS2 names), which C_game::Init fills
    static std::vector<I3D_sector *> v_light_sector;
    static std::vector<I3D_sector *> v_sound_sector;

    // Copies the tyres, each with its own copy of its surfaces
    static bool SetTyres(const std::vector<S_TYRE> *tyres);
    static void CloseTyres();
    static bool SetVehicleMaterials(const std::vector<S_VEH_MATERIAL> *materials);
    static void CloseVehicleMaterials();
    // Takes the emitters (PC only, the name is ours; PS2 adds one at a time through AddParticle)
    static bool SetParticles(const std::vector<S_VEH_PARTICLE> *particles);
    // Unlinks and releases the emitters
    static void CloseParticles();
    // Sets sound_volume (C_Vehicle_sounds.cpp; PC only, the name is ours)
    static void SetSoundVolume(float volume);
    // A sector for v_light_sector (PS2 name)
    static void AddSectorForProjectorLights(I3D_sector *sector);
    // A sector for v_sound_sector (PS2 name), the same way
    static void AddSectorForSounds(I3D_sector *sector);

    // Settles the vehicle at the game's start (PS2 name; PS2's on C_car takes three parameters)
    void CalmVehicle(uint time);
    // unknown_ca0's size, as an int (the name is ours: G_LoadSaveClass::GarageGameDone's loop over the seats compares
    // it signed, the scripts' CAR_SETDOOROPEN a door's number)
    // FUNCTION: GAME 0x47b1f0
    int GetNumUnknownCa0() const
    {
        return unknown_ca0.size();
    }
    // PS2 name and parameter, which G_LoadSaveClass::GarageGameInit calls with 0 for a car kept between missions
    void Deactivate(uint flags);
    // Fills the tank up to fuel, at most its size (PS2 name); true
    bool SetFuel(float fuel);
    // Counts the locks up or down, the vehicle locked while there are any (PS2 name)
    void LockVehicle(bool lock);
    // A projector light's size, VLIGHT_PROJECTOR_* (PS2 name)
    void SetProjectorSize(uint projector, const S_vector &size);
    // Opens a door so far (PS2 name)
    void OpenDoor(int door, float open, bool unknown);
    // Turns the special dynamic collisions on or off (PS2 name)
    bool SetSpecialDynamicColls(bool on);
    // Adds the dynamic collisions or takes them away (PS2 name)
    void SetDynamicCollsOn(bool on, bool unknown);

    // A camera's position at a wheel, offset from it (PS2 name): the offset's x mirrored for the even wheels, which
    // are on the left
    S_vector GetWheelCamPos(int wheel, const S_vector &offset);
    // The driver's head (PS2 name), where the camera "C In" sits
    const S_vector &GetHeadPos();
    // While it is set, the car cameras' faktorStability falls (PS2 name)
    bool IsVehicleUnstable();
    // One of the vehicle's two sizes (PS2 name)
    const S_vector &GetVehicleSize(bool b);
    // The pivot's position in the vehicle (PS2 name), the same way
    const S_vector &GetVehiclePivotPos(bool b);
    // The velocity, by value (the name is a guess): the camera copies it before each use, as for the actors' vectors
    S_vector GetVelocity() const
    {
        return velocity;
    }
    // The tank's size (the name is ours): C_mission::FrmActBuff_IsActiveActor passes it to SetFuel through the FPU, as
    // an inline function's float result, where the member read directly is pushed with integer moves
    float GetUnknown1fa0() const
    {
        return unknown_1fa0;
    }

    // The accessors whose copies are among those at the end of the file at 0x460a50 (the names are ours). The setters
    // wake the vehicle up through wake_up.
    // FUNCTION: GAME 0x47b0e0
    bool GetSpecialDynamicColls() const
    {
        return special_dynamic_colls;
    }
    // FUNCTION: GAME 0x47b0f0
    float GetSpeed() const
    {
        return speed;
    }
    // FUNCTION: GAME 0x47b100
    void SetVelocity(const S_vector &v)
    {
        velocity = v;
        speed = velocity.Magnitude();
        if (wake_up)
            wake_up(callback_context);
    }
    // FUNCTION: GAME 0x47b1b0
    void SetUnknown420(const S_vector &v)
    {
        unknown_420 = v;
        if (wake_up)
            wake_up(callback_context);
    }
    // Without a wake-up
    // FUNCTION: GAME 0x47b220
    void SetLightsUnknown8(bool on)
    {
        if (on)
            lights |= VLIGHT_UNKNOWN_8;
        else
            lights &= ~VLIGHT_UNKNOWN_8;
    }
    // FUNCTION: GAME 0x47b250
    void SetLight(bool on)
    {
        if (on)
            lights |= VLIGHT_LIGHT;
        else
            lights &= ~VLIGHT_LIGHT;
        if (wake_up)
            wake_up(callback_context);
    }
    // The indicators wake the vehicle up only when they go on
    // FUNCTION: GAME 0x47b280
    void SetIndicatorL(bool on)
    {
        if (lights & VLIGHT_UNKNOWN_8)
            return;
        if (on)
        {
            lights |= VLIGHT_INDICATOR_L;
            if (wake_up)
                wake_up(callback_context);
        }
        else
            lights &= ~VLIGHT_INDICATOR_L;
    }
    // FUNCTION: GAME 0x47b2c0
    void SetIndicatorR(bool on)
    {
        if (lights & VLIGHT_UNKNOWN_8)
            return;
        if (on)
        {
            lights |= VLIGHT_INDICATOR_R;
            if (wake_up)
                wake_up(callback_context);
        }
        else
            lights &= ~VLIGHT_INDICATOR_R;
    }
    // FUNCTION: GAME 0x47b300
    void SetMainLights(bool on)
    {
        if (on)
            lights |= VLIGHT_MAIN;
        else
            lights &= ~VLIGHT_MAIN;
        if (wake_up)
            wake_up(callback_context);
    }
    // Two statements, as PS2's: VC6 merges them into a masked xor that reads lights twice, as the original's copy does
    // (one expression reads it once)
    // FUNCTION: GAME 0x47b340
    void SetProjectors(uint projectors)
    {
        lights &= ~VLIGHT_PROJECTORS;
        lights |= projectors & VLIGHT_PROJECTORS;
        if (wake_up)
            wake_up(callback_context);
    }
    // FUNCTION: GAME 0x47b380
    uint GetProjectors() const
    {
        return lights & VLIGHT_PROJECTORS;
    }
    // FUNCTION: GAME 0x47b390
    int GetNumWheels() const
    {
        return num_wheels;
    }

    // Our names are guesses after the members' uses.
    // The vtable's place: PS2's C_Vehicle has one (one virtual function), the PC's car stores its second vtable here
    void *unknown_0;                    // 0x0 (car 0x70)
    uchar unknown_4[0x118];             // 0x4
    uint lights;                        // 0x11c (car 0x18c, PS2 0x578), VLIGHT_*
    uchar unknown_120[0x38];            // 0x120
    S_vector unknown_158;               // 0x158 (car 0x1c8), GetVehicleSize(false)'s
    S_vector unknown_164;               // 0x164 (car 0x1d4), GetVehicleSize(true)'s
    S_vector unknown_170;               // 0x170 (car 0x1e0), GetVehiclePivotPos(false)'s
    S_vector unknown_17c;               // 0x17c (car 0x1ec), GetVehiclePivotPos(true)'s
    uchar unknown_188[0x1c];            // 0x188
    uint unknown_1a4;                   // 0x1a4 (car 0x214), flags: 8 while the vehicle is unstable
    uchar unknown_1a8[0x12c];           // 0x1a8
    S_vector head_pos;                  // 0x2d4 (car 0x344), the driver's head (GetHeadPos)
    uchar unknown_2e0[0x28];            // 0x2e0
    // 0x308 (car 0x378), what wakes the vehicle up, given callback_context (PS2's SetCallbackWU sets one of this type,
    // C_car::Wake_Up)
    void (*wake_up)(const void *context);
    uchar unknown_30c[0x40];            // 0x30c
    I3D_frame *model_frame;            // 0x34c (car 0x3bc), the frame whose world matrix model_matrix is
    uchar unknown_350[0x48];            // 0x350
    S_vector unknown_398;               // 0x398 (car 0x408), the point the car cameras look at, the position most likely
    uchar unknown_3a4[0x7c];            // 0x3a4
    // 0x420 (car 0x490), most likely the angular velocity: the scripts' CAR_SETSPECCOLL zeroes it with the velocity
    S_vector unknown_420;
    uchar unknown_42c[0x98];            // 0x42c
    int num_wheels;                    // 0x4c4 (car 0x534)
    uchar unknown_4c8[0xd4];            // 0x4c8
    float speed;                        // 0x59c (car 0x60c), in m/s: velocity's magnitude (SetVelocity)
    uchar unknown_5a0[0x698];           // 0x5a0
    S_wheel **wheels;                   // 0xc38 (car 0xca8)
    uchar unknown_c3c[0x18];            // 0xc3c
    S_vector unknown_c54;               // 0xc54 (car 0xcc4), the car's up axis most likely
    uchar unknown_c60[4];               // 0xc60
    float unknown_c64;                  // 0xc64 (car 0xcd4), an angle: below pi/2 the car goes forward
    uchar unknown_c68[0xc];             // 0xc68
    const void *callback_context;       // 0xc74 (car 0xce4, PS2 0xf20), what the callbacks get
    uchar unknown_c78[0x28];            // 0xc78
    // 0xca0 (car 0xd10), as many as the car has seats most likely: G_LoadSaveClass::GarageGameDone asks C_car::GetOwner
    // for each
    std::vector<S_vehicle_unknown_ca0> unknown_ca0;
    uchar unknown_cb0[0x38];            // 0xcb0
    S_matrix model_matrix;              // 0xce8 (car 0xd58), C_car::GetInvModelMatrix's
    S_matrix inv_model_matrix;          // 0xd28 (car 0xd98)
    uchar unknown_d68[0x1228];          // 0xd68
    S_vector velocity;                  // 0x1f90 (car 0x2000), speed's vector (SetVelocity)
    uchar unknown_1f9c[4];              // 0x1f9c
    float unknown_1fa0;                 // 0x1fa0 (car 0x2010), the tank's size: SetFuel's upper bound
    uchar unknown_1fa4[6];              // 0x1fa4
    bool special_dynamic_colls;         // 0x1faa (car 0x201a, PS2 0x224b), SetSpecialDynamicColls'
    bool dynamic_colls_on;              // 0x1fab (car 0x201b), SetDynamicCollsOn's first flag
    bool unknown_1fac;                  // 0x1fac (car 0x201c), its second
    uchar unknown_1fad[0x93];           // 0x1fad, to the car's own members at its 0x20b0
};

// The tables' vector helpers the loaders and the Set functions call out of line: each push_back's insert(P, n, x) and the
// parts of it the inlined copies call. The size, copy, fill and construct helpers have a copy for each element size, so
// the linker folded them with other vectors' of that size; the original kept other files' copies of most.
// TEMPLATE: GAME 0x42d1f0
// ?insert@?$vector@US_VEH_PARTICLE@@V?$allocator@US_VEH_PARTICLE@@@std@@@std@@QAEXPAUS_VEH_PARTICLE@@IABU3@@Z

// TEMPLATE: GAME 0x42d510
// ?insert@?$vector@US_VEH_MATERIAL@@V?$allocator@US_VEH_MATERIAL@@@std@@@std@@QAEXPAUS_VEH_MATERIAL@@IABU3@@Z

// TEMPLATE: GAME 0x42d800
// ?insert@?$vector@US_TYRE@@V?$allocator@US_TYRE@@@std@@@std@@QAEXPAUS_TYRE@@IABU3@@Z

// TEMPLATE: GAME 0x42de10
// ?insert@?$vector@US_SRFC@@V?$allocator@US_SRFC@@@std@@@std@@QAEXPAUS_SRFC@@IABU3@@Z

// TEMPLATE: GAME 0x42e140
// ?size@?$vector@US_VEH_PARTICLE@@V?$allocator@US_VEH_PARTICLE@@@std@@@std@@QBEIXZ

// TEMPLATE: GAME 0x4d2010
// ?size@?$vector@US_SRFC@@V?$allocator@US_SRFC@@@std@@@std@@QBEIXZ

// TEMPLATE: GAME 0x4d2aa0
// ?_Ufill@?$vector@US_SRFC@@V?$allocator@US_SRFC@@@std@@@std@@IAEXPAUS_SRFC@@IABU3@@Z

// TEMPLATE: GAME 0x5490d0
// ?_Construct@std@@YIXPAUS_VEH_PARTICLE@@ABU2@@Z

// TEMPLATE: GAME 0x5bc780
// ?_Ucopy@?$vector@US_SRFC@@V?$allocator@US_SRFC@@@std@@@std@@IAEPAUS_SRFC@@PBU3@0PAU3@@Z

// TEMPLATE: GAME 0x5bc7c0
// ?_Ucopy@?$vector@US_tshift_elem@@V?$allocator@US_tshift_elem@@@std@@@std@@IAEPAUS_tshift_elem@@PBU3@0PAU3@@Z

// TEMPLATE: GAME 0x604da0
// ?_Ufill@?$vector@US_VEH_PARTICLE@@V?$allocator@US_VEH_PARTICLE@@@std@@@std@@IAEXPAUS_VEH_PARTICLE@@IABU3@@Z
