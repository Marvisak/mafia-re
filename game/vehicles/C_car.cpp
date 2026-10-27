// The cars (PS2 C_car.cpp; the PC's file is most likely the one at 0x41bb50-0x42e23c, docs/game-files.md). Written so
// far: the tables InitSystem loads, in PS2's order (PS2's Load_Particle and Change_Particle after LoadParticles are
// PS2's own), which the loaders hand to C_Vehicle in a local vector, and the PC's force feedback scales, which the
// profiles set, GetInvModelMatrix and GetActualLevel. The constructor, which C_mission::CreateActor calls, and the
// other virtual functions (GetOwner, SetActualLevel, the force feedback effects, Reset, IsCarUsable) are stubs.

#include "vehicles/C_car.h"
#include "vehicles/C_Vehicle.h"
#include "g_material.h"
#include "I3D_cache.h"

#include <string.h>

#include "rw_data.h"

#include "stub.h"

// Initialized data, in the order SetForceFeedback stores them
// GLOBAL: GAME 0x6322dc
float C_car::ff_volume = 1.0f;
// GLOBAL: GAME 0x6322e0
float C_car::ff_tire_grip = 1.0f;
// GLOBAL: GAME 0x6322e4
float C_car::ff_collisions = 1.0f;
// GLOBAL: GAME 0x6322e8
float C_car::ff_suspension = 1.0f;
// GLOBAL: GAME 0x6322ec
float C_car::ff_revs = 1.0f;

// A bool parameter's bits, the way I3DFloatAsInt gives a float's (I3D.h): the original stores the byte to a slot of its
// own for each call and pushes the whole dword. The name is ours.
inline uint I3DBoolAsInt(const bool &b)
{
    bool t = b;
    return *(uint *)&t;
}

#pragma auto_inline(off)

// STUB: GAME 0x41bb80
C_car::C_car()
{
    StubLog("C_car::C_car");
}

#pragma auto_inline(on)

// Materials 1 to 4 (asphalt, paving, pavement, concrete), 9, 10 and 38 (rock, rails and brick wall) get the two flags.
// Without the table (InitSystem runs before G_LoadSaveClass::Init loads it) one default material.
// FUNCTION: GAME 0x41c7f0
bool C_car::LoadMaterials()
{
    std::vector<S_VEH_MATERIAL> materials;
    if (numOfMaterials < 1) {
        S_VEH_MATERIAL m;
        memset(&m, 0, sizeof(m));
        m.id = 0;
        m.unknown_18 = .35f;
        m.unknown_1c = 1.0f;
        m.unknown_08 = 100.0f;
        m.unknown_04 = 100.0f;
        m.unknown_10 = 100.0f;
        m.unknown_0c = 100.0f;
        m.unknown_14 = 0.0f;
        m.unknown_20 = 0;
        m.unknown_24 = 0;
        m.unknown_28 = 0;
        materials.push_back(m);
    } else {
        for (int i = 0; i < numOfMaterials; i++) {
            S_VEH_MATERIAL m;
            memset(&m, 0, sizeof(m));
            m.id = i;
            m.unknown_18 = materialList[i].unknown_108;
            m.unknown_1c = materialList[i].unknown_10c;
            m.unknown_08 = materialList[i].unknown_110;
            m.unknown_04 = materialList[i].unknown_114;
            m.unknown_10 = materialList[i].unknown_118;
            m.unknown_0c = materialList[i].unknown_11c;
            m.unknown_14 = materialList[i].unknown_120;
            m.unknown_20 = materialList[i].unknown_124;
            m.unknown_24 = materialList[i].unknown_128;
            m.unknown_28 = materialList[i].unknown_12c;
            if (m.unknown_18 < 0.0f)
                m.unknown_18 = 0.0f;
            else if (m.unknown_18 > 1.0f)
                m.unknown_18 = 1.0f;
            if (m.unknown_1c < 1.0f)
                m.unknown_1c = 1.0f;
            else if (m.unknown_1c > 2.0f)
                m.unknown_1c = 2.0f;
            if (m.unknown_04 > m.unknown_08)
                m.unknown_04 = m.unknown_08;
            if (m.unknown_0c > m.unknown_10)
                m.unknown_0c = m.unknown_10;
            if (m.unknown_14 < 0.0f)
                m.unknown_14 = 0.0f;
            else if (m.unknown_14 > .2f)
                m.unknown_14 = .2f;
            if (i > 0 && i < 5 || i == 9 || i == 10 || i == 38) {
                m.unknown_2c = true;
                m.unknown_2d = true;
            }
            materials.push_back(m);
        }
    }
    C_Vehicle::SetVehicleMaterials(&materials);
    materials.clear();
    return true;
}

// An emitter of each table entry's model, linked to the scene's primary sector with the entry's parameters. An entry
// whose model doesn't load still gets its place, without an emitter. The model cache's Open is inlined, as in the
// original (I3D_cache.h). PP_DIR_RND gets the float converted to an integer, where the emitter takes a float's bits.
// Open: the original gives each float parameter's copy a stack slot of its own (frame 0xc4, ours 0x88), as compiler
// temporaries get; ours share one, with every helper spelling tried (by value, by reference, forced, a template, a
// union, casts). The registers differ with it, and the child and no-children failures share their emitter's release
// with the outer one's in the original.
// FUNCTION: GAME 0x41ca80
bool C_car::LoadParticles()
{
    std::vector<S_VEH_PARTICLE> particles;
    if (numOfGameParticles < 1) {
        S_VEH_PARTICLE p;
        memset(&p, 0, sizeof(p));
        particles.push_back(p);
    } else {
        for (uint i = 0; i < numOfGameParticles; i++) {
            S_VEH_PARTICLE p;
            memset(&p, 0, sizeof(p));
            p.emitor = (I3D_emitor *)driver->CreateFrame(FRAME_EMITOR);
            I3D_model *model = (I3D_model *)driver->CreateFrame(FRAME_MODEL);
            if (model && p.emitor) {
                if (model_cache.Open(model, gameParticleTable[i].name, 0, NULL, NULL, NULL) == I3D_OK) {
                    if (model->NumChildren()) {
                        I3D_frame *frm = model->GetChild(0);
                        if (frm && frm->GetType() == FRAME_VISUAL) {
                            S_particle_def &def = gameParticleTable[i];
                            p.emitor->Duplicate(frm);
                            // STRING: GAME 0x632348
                            p.emitor->SetName("Car_particle");
                            p.emitor->LinkTo(C_Vehicle::scene->primary_sector, 0);
                            p.emitor->SetScale(S_vector(1.0f, 1.0f, 1.0f));
                            p.emitor->SetDir(S_vector(0.0f, 0.0f, 1.0f), 0.0f);
                            p.emitor->Update();
                            p.emitor->Reset();
                            p.emitor->SetParam(PP_MAX_PARTICLES, gameParticleTable[i].max_particles);
                            p.emitor->SetParam(PP_FLAGS, gameParticleTable[i].flags);
                            p.emitor->SetParam(PP_EMIT_TIME, gameParticleTable[i].emit_time);
                            p.emitor->SetParam(PP_EMIT_TIME_RND, gameParticleTable[i].emit_time_rnd);
                            p.emitor->SetParam(PP_LIFE_TIME, gameParticleTable[i].life_time);
                            p.emitor->SetParam(PP_LIFE_TIME_RND, gameParticleTable[i].life_time_rnd);
                            p.emitor->SetParam(PP_GROW_1, I3DFloatAsInt(gameParticleTable[i].grow_1));
                            p.emitor->SetParam(PP_GROW_2, I3DFloatAsInt(gameParticleTable[i].grow_2));
                            p.emitor->SetParam(PP_SCALE, I3DFloatAsInt(gameParticleTable[i].scale));
                            p.emitor->SetParam(PP_FADE_IN, I3DFloatAsInt(gameParticleTable[i].fade_in));
                            p.emitor->SetParam(PP_FADE_OUT, I3DFloatAsInt(gameParticleTable[i].fade_out));
                            p.emitor->SetParam(PP_POS_RND, I3DFloatAsInt(gameParticleTable[i].pos_rnd));
                            p.emitor->SetParam(PP_DIR_RND, gameParticleTable[i].dir_rnd);
                            p.emitor->SetParam(PP_VELOCITY_X, I3DFloatAsInt(gameParticleTable[i].velocity[0]));
                            p.emitor->SetParam(PP_VELOCITY_Y, I3DFloatAsInt(gameParticleTable[i].velocity[1]));
                            p.emitor->SetParam(PP_VELOCITY_Z, I3DFloatAsInt(gameParticleTable[i].velocity[2]));
                            p.emitor->SetParam(PP_VELOCITY_WORLD, I3DBoolAsInt(true));
                            p.velocity = S_vector(gameParticleTable[i].velocity[0], gameParticleTable[i].velocity[1],
                                gameParticleTable[i].velocity[2]);
                            p.emitor->SetParam(PP_VELOCITY_END_X, I3DFloatAsInt(gameParticleTable[i].velocity_end[0]));
                            p.emitor->SetParam(PP_VELOCITY_END_Y, I3DFloatAsInt(gameParticleTable[i].velocity_end[1]));
                            p.emitor->SetParam(PP_VELOCITY_END_Z, I3DFloatAsInt(gameParticleTable[i].velocity_end[2]));
                            p.emitor->SetParam(PP_VELOCITY_END_WORLD, I3DBoolAsInt(true));
                            p.velocity_end = S_vector(gameParticleTable[i].velocity_end[0],
                                gameParticleTable[i].velocity_end[1], gameParticleTable[i].velocity_end[2]);
                            p.emitor->SetParam(PP_ROT_SPEED_1, I3DFloatAsInt(gameParticleTable[i].rot_speed_1));
                            p.emitor->SetParam(PP_ROT_SPEED_2, I3DFloatAsInt(gameParticleTable[i].rot_speed_2));
                            p.emitor->SetParam(PP_DIR_TIME, gameParticleTable[i].dir_time);
                            p.emitor->SetParam(PP_DIR_TIME_RND, gameParticleTable[i].dir_time_rnd);
                            p.emitor->SetParam(PP_DIR_TIME_STRAY, I3DFloatAsInt(gameParticleTable[i].dir_time_stray));
                            p.emitor->SetParam(PP_COLOR, (uint)gameParticleTable[i].color);
                            p.emitor->SetParam(PP_COLOR_END, (uint)gameParticleTable[i].color_end);
                            p.emitor->Update();
                            model->Release();
                        } else {
                            model->Release();
                            p.emitor->Release();
                            p.emitor = NULL;
                        }
                    } else {
                        model->Release();
                        p.emitor->Release();
                        p.emitor = NULL;
                    }
                } else {
                    model->Release();
                    p.emitor->Release();
                    p.emitor = NULL;
                }
            } else {
                if (model)
                    model->Release();
                if (p.emitor) {
                    p.emitor->Release();
                    p.emitor = NULL;
                }
            }
            particles.push_back(p);
        }
    }
    C_Vehicle::SetParticles(&particles);
    particles.clear();
    return true;
}

// Version 1. Each tyre has its size (skipped), name, id and surfaces, each surface its material and its curves' size;
// C_Vehicle copies the tyres, so the loaded ones are freed again. buffer is never set: each exit deletes it, NULL, and
// each surface's data copies none of it (PS2's memcpy(data, NULL, 0); VC6 leaves the setup's xor esi, esi and xor ecx,
// ecx of the inlined copy).
// FUNCTION: GAME 0x41d480
bool C_car::LoadTyres()
{
    std::vector<S_TYRE> tyres;
    char *buffer = NULL;
    C_Vehicle::SetTyres(&tyres);
    // STRING: GAME 0x632358
    int h = dtaOpen("tables\\tyres.bin", false);
    if (h == DTA_INVALID_HANDLE) {
        delete[] buffer;
        return false;
    }
    int version;
    dtaRead(h, &version, 4);
    if (version != 1) {
        dtaClose(h);
        delete[] buffer;
        return false;
    }
    int num_tyres;
    dtaRead(h, &num_tyres, 4);
    while (num_tyres--) {
        S_TYRE tyre;
        memset(&tyre, 0, sizeof(tyre));
        dtaRead(h, &version, 4);
        dtaRead(h, tyre.name, sizeof(tyre.name));
        dtaRead(h, &tyre.id, 4);
        int num_srfc;
        dtaRead(h, &num_srfc, 4);
        while (num_srfc--) {
            S_SRFC srfc;
            memset(&srfc, 0, sizeof(srfc));
            if (!tyre.surfaces)
                tyre.surfaces = new std::vector<S_SRFC>;
            dtaRead(h, &srfc.material, 4);
            int size;
            dtaRead(h, &size, 4);
            char *data = new char[size];
            if (!data) {
                delete[] buffer;
                return false;
            }
            memcpy(data, buffer, 0);
            dtaRead(h, data, size);
            ConvertTyres((S_TSRFC *)data, &srfc);
            tyre.surfaces->push_back(srfc);
            delete[] data;
        }
        tyres.push_back(tyre);
    }
    dtaClose(h);
    delete[] buffer;
    C_Vehicle::SetTyres(&tyres);
    num_tyres = tyres.size();
    while (num_tyres--) {
        if (tyres[num_tyres].surfaces)
            delete tyres[num_tyres].surfaces;
    }
    tyres.clear();
    return true;
}

// The curves' first values times 1000, their angles in radians, and their ranges
// FUNCTION: GAME 0x41d770
void C_car::ConvertTyres(S_TSRFC *src, S_SRFC *dst)
{
    dst->curve[0].unknown_00 = src->curve[0].unknown_00 * 1000.0f;
    dst->curve[0].angle = src->curve[0].angle * 3.14159f / 180.0f;
    dst->curve[0].unknown_08 = src->curve[0].unknown_08;
    dst->curve[0].unknown_0c = src->curve[0].unknown_0c;
    dst->curve[0].min_1 = src->curve[0].min_1;
    dst->curve[0].max_1 = src->curve[0].max_1;
    dst->curve[0].range_1 = dst->curve[0].max_1 - dst->curve[0].min_1;
    dst->curve[0].min_2 = src->curve[0].min_2;
    dst->curve[0].max_2 = src->curve[0].max_2;
    dst->curve[0].range_2 = dst->curve[0].max_2 - dst->curve[0].min_2;
    dst->curve[0].unknown_28 = src->curve[0].unknown_20;
    dst->curve[1].unknown_00 = src->curve[1].unknown_00 * 1000.0f;
    dst->curve[1].angle = src->curve[1].angle * 3.14159f / 180.0f;
    dst->curve[1].unknown_08 = src->curve[1].unknown_08;
    dst->curve[1].unknown_0c = src->curve[1].unknown_0c;
    dst->curve[1].min_1 = src->curve[1].min_1;
    dst->curve[1].max_1 = src->curve[1].max_1;
    dst->curve[1].range_1 = dst->curve[1].max_1 - dst->curve[1].min_1;
    dst->curve[1].min_2 = src->curve[1].min_2;
    dst->curve[1].max_2 = src->curve[1].max_2;
    dst->curve[1].range_2 = dst->curve[1].max_2 - dst->curve[1].min_2;
    dst->curve[1].unknown_28 = src->curve[1].unknown_20;
}

#pragma auto_inline(off)

// STUB: GAME 0x41dec0
C_human *C_car::GetOwner(int seat)
{
    StubLog("C_car::GetOwner %d", seat);
    return NULL;
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x41e9f0
int C_car::GetActualLevel()
{
    return actual_level;
}

#pragma auto_inline(off)

// STUB: GAME 0x41ea00
bool C_car::SetActualLevel(int level)
{
    StubLog("C_car::SetActualLevel %d", level);
    return false;
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x41eb00
void C_car::InitFFEffects()
{
    StubLog("C_car::InitFFEffects");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x41eca0
void C_car::DoneFFEffects()
{
    StubLog("C_car::DoneFFEffects");
}

#pragma auto_inline(on)

#pragma auto_inline(off)

// STUB: GAME 0x422be0
void C_car::Reset(float unknown, bool unknown2)
{
    StubLog("C_car::Reset");
}

#pragma auto_inline(on)

// PS2's order: the frame's world matrix into the model matrix, that into the inverse, inverted from a copy
// FUNCTION: GAME 0x4293e0
const S_matrix &C_car::GetInvModelMatrix(bool update)
{
    if (update)
    {
        model_matrix = model_frame->GetMatrix();
        inv_model_matrix = model_matrix;
        inv_model_matrix.Inverse(S_matrix(inv_model_matrix));
    }
    return inv_model_matrix;
}

#pragma auto_inline(off)

// STUB: GAME 0x429850
bool C_car::IsCarUsable(uint flags)
{
    StubLog("C_car::IsCarUsable %u", flags);
    return false;
}

#pragma auto_inline(on)

// FUNCTION: GAME 0x42bb90
void C_car::SetForceFeedback(float volume, float tire_grip, float collisions, float suspension, float revs)
{
    ff_volume = volume;
    ff_tire_grip = tire_grip;
    ff_collisions = collisions;
    ff_suspension = suspension;
    ff_revs = revs;
}
