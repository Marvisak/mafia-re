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

// The vehicles' shared part (PS2 C_Vehicle, C_Vehicle.cpp): so far the tables InitSystem loads through C_car, which
// CloseSystem frees through this class, as PS2's
class C_Vehicle
{
public:
    // The scene the vehicles are in, which C_mission::Open sets (PS2 name)
    static I3D_scene *scene;
    // The tyres, the "user" one first (PS2 name)
    static std::vector<S_TYRE> v_tyre;
    // The materials (PS2 name), and the index of each material's entry in it (PS2's is an int array)
    static std::vector<S_VEH_MATERIAL> v_materials;
    static uchar s_material_conv_tbl[0x100];
    // The particle emitters (PS2 name)
    static std::vector<S_VEH_PARTICLE> v_particle;

    // Copies the tyres, each with its own copy of its surfaces
    static bool SetTyres(const std::vector<S_TYRE> *tyres);
    static void CloseTyres();
    static bool SetVehicleMaterials(const std::vector<S_VEH_MATERIAL> *materials);
    static void CloseVehicleMaterials();
    // Takes the emitters (PC only, the name is ours; PS2 adds one at a time through AddParticle)
    static bool SetParticles(const std::vector<S_VEH_PARTICLE> *particles);
    // Unlinks and releases the emitters
    static void CloseParticles();
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
// ?_Ucopy@?$vector@US_VEH_PARTICLE@@V?$allocator@US_VEH_PARTICLE@@@std@@@std@@IAEPAUS_VEH_PARTICLE@@PBU3@0PAU3@@Z

// TEMPLATE: GAME 0x604da0
// ?_Ufill@?$vector@US_VEH_PARTICLE@@V?$allocator@US_VEH_PARTICLE@@@std@@@std@@IAEXPAUS_VEH_PARTICLE@@IABU3@@Z
