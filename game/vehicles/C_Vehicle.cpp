// The vehicles (PS2 C_Vehicle.cpp; the PC's file is most likely the one at 0x4bd650-0x4d34ac, whose first initializers
// construct the tables, docs/game-files.md). Written so far: the tables C_car's loaders fill and CloseSystem frees, in
// PS2's order (each table's Set before its Close), what the camera calls (GetWheelCamPos, GetHeadPos,
// IsVehicleUnstable, GetVehicleSize) and AddSectorForProjectorLights, which C_game::Init calls.
// Open: the file starts with an empty initializer (0x4bd650) and has two more vectors after v_particle (0x63d890,
// 0x63d880), most likely PS2's v_light_sector and v_sound_sector; the tables' destructors (0x4bd6a0 and on) test and set
// a guard byte (0x63d9c0) as the tracks' do (actors/C_animations.cpp), and the .bss order isn't the buckets' of these
// names.

#include "vehicles/C_Vehicle.h"

#include "stub.h"

#include <string.h>

// GLOBAL: GAME 0x63d9d4
I3D_scene *C_Vehicle::scene;
// GLOBAL: GAME 0x63d9c4
std::vector<S_TYRE> C_Vehicle::v_tyre;
// GLOBAL: GAME 0x63d9b0
std::vector<S_VEH_MATERIAL> C_Vehicle::v_materials;
// GLOBAL: GAME 0x63d8b0
uchar C_Vehicle::s_material_conv_tbl[0x100];
// GLOBAL: GAME 0x63d8a0
std::vector<S_VEH_PARTICLE> C_Vehicle::v_particle;
// GLOBAL: GAME 0x63d890
std::vector<I3D_sector *> C_Vehicle::v_light_sector;
// GLOBAL: GAME 0x63d880
std::vector<I3D_sector *> C_Vehicle::v_sound_sector;

#pragma auto_inline(off)

// STUB: GAME 0x4c31e0
void C_Vehicle::Deactivate(uint flags)
{
    StubLog("C_Vehicle::Deactivate %u", flags);
}

#pragma auto_inline(on)

// The user tyre first, then a copy of each, as PS2's
// FUNCTION: GAME 0x4c5820
bool C_Vehicle::SetTyres(const std::vector<S_TYRE> *tyres)
{
    CloseTyres();
    S_TYRE t;
    memset(&t, 0, sizeof(t));
    t.id = 0;
    // STRING: GAME 0x634c54
    strcpy(t.name, "user");
    v_tyre.push_back(t);
    for (uint i = 0; i < tyres->size(); i++) {
        v_tyre.push_back((*tyres)[i]);
        v_tyre[v_tyre.size() - 1].surfaces = new std::vector<S_SRFC>;
        for (uint j = 0; j < (*tyres)[i].surfaces->size(); j++)
            v_tyre[v_tyre.size() - 1].surfaces->push_back((*(*tyres)[i].surfaces)[j]);
    }
    return true;
}

// The surfaces from the last tyre, through a local: the global's element read again for the delete is read from memory
// twice. PS2 has the test too (C_car::LoadTyres reads its local vector's element twice).
// FUNCTION: GAME 0x4c5b80
void C_Vehicle::CloseTyres()
{
    for (int i = v_tyre.size(); i--;) {
        std::vector<S_SRFC> *s = v_tyre[i].surfaces;
        if (s)
            delete s;
    }
    v_tyre.clear();
}

// FUNCTION: GAME 0x4c5c10
bool C_Vehicle::SetVehicleMaterials(const std::vector<S_VEH_MATERIAL> *materials)
{
    CloseVehicleMaterials();
    int i = 0xff;
    while (i--)
        s_material_conv_tbl[i] = 0;
    for (uint j = 0; j < materials->size(); j++) {
        v_materials.push_back((*materials)[j]);
        s_material_conv_tbl[(*materials)[j].id] = j;
    }
    return true;
}

// FUNCTION: GAME 0x4c5c80
void C_Vehicle::CloseVehicleMaterials()
{
    v_materials.clear();
}

// FUNCTION: GAME 0x4c5cc0
bool C_Vehicle::SetParticles(const std::vector<S_VEH_PARTICLE> *particles)
{
    CloseParticles();
    for (uint i = 0; i < particles->size(); i++)
        v_particle.push_back((*particles)[i]);
    return true;
}

// FUNCTION: GAME 0x4c5f80
void C_Vehicle::CloseParticles()
{
    for (int i = v_particle.size(); i--;) {
        if (v_particle[i].emitor) {
            v_particle[i].emitor->LinkTo(NULL, 0);
            v_particle[i].emitor->Release();
        }
    }
    v_particle.clear();
}

// PS2's without its test of the wheel's number
// FUNCTION: GAME 0x4c6010
S_vector C_Vehicle::GetWheelCamPos(int wheel, const S_vector &offset)
{
    S_vector pos = offset;
    if (!(wheel % 2))
    {
        pos.x = -pos.x;
        pos.x -= wheels[wheel]->unknown_c;
    }
    else
        pos.x += wheels[wheel]->unknown_c;
    pos += wheels[wheel]->unknown_10;
    return pos;
}

#pragma auto_inline(off)

// STUB: GAME 0x4cb6e0
bool C_Vehicle::SetFuel(float fuel)
{
    StubLog("C_Vehicle::SetFuel");
    return true;
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x4cc620
const S_vector &C_Vehicle::GetHeadPos()
{
    return head_pos;
}

// FUNCTION: GAME 0x4cd5b0
bool C_Vehicle::IsVehicleUnstable()
{
    return unknown_1a4 & 8 ? true : false;
}

// FUNCTION: GAME 0x4cd5c0
const S_vector &C_Vehicle::GetVehicleSize(bool b)
{
    return b ? unknown_164 : unknown_158;
}

// Unless it is there already, searched from the last one, as PS2's
// FUNCTION: GAME 0x4ce560
void C_Vehicle::AddSectorForProjectorLights(I3D_sector *sector)
{
    int i = v_light_sector.size();
    while (i--)
    {
        if (v_light_sector[i] == sector)
            break;
    }
    if (i < 0)
        v_light_sector.push_back(sector);
}

#pragma auto_inline(off)

// STUB: GAME 0x4ce960
void C_Vehicle::CalmVehicle(uint time)
{
    StubLog("C_Vehicle::CalmVehicle");
}

#pragma auto_inline(on)


