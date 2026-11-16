// The physical objects (PS2 C_box_new.cpp; the PC's file is the one at 0x5674f0-0x567cec, docs/game-files.md), which
// starts with the constructor: physical things of the scene, each a box or a wheel by its data.

#include "actors/C_box_new.h"

#include "C_chunk.h"

// FUNCTION: GAME 0x5674f0
C_box_new::C_box_new()
{
    type = ACTOR_BOX_NEW;
}

// Declared, empty: it stores the vtable again before the base's
// FUNCTION: GAME 0x567530
C_box_new::~C_box_new()
{
}

// The data's defaults, the weight from the size of the frame's bound box. The size scaled by the frame's world scale
// is never used (PS2's the same), so only the scale's GetScale call is left of it.
// FUNCTION: GAME 0x567540
bool C_box_new::Init(I3D_frame *frm)
{
    if (!C_PhThing::Init(frm))
        return false;
    is_box = true;
    radius = 1.0f;
    unknown_394 = 1.0f;
    weight = 10.0f;
    unknown_39c = .5f;
    unknown_3a0 = .01f;
    I3D_bvolume bvol = frame->GetHRBoundVolume();
    S_vector size = bvol.bbox.max - bvol.bbox.min;
    float volume = size.x * size.y * size.z;
    size = frame->GetWorldScale() * size;
    if (volume > .01f && volume < 1000.0f)
        weight = volume * 50.0f;
    return true;
}

// The thing set up as a box or a wheel from the data, at rest. The structs are filled in PS2's order, the velocities
// one member at a time (an S_vector assigned goes through a temporary, which VC6 builds once before the branch): the
// values both read first are loaded once before it.
// FUNCTION: GAME 0x567650
void C_box_new::GameInit()
{
    gravity_force = S_vector(0.0f, -1.0f, 0.0f);
    if (is_box)
    {
        S_THING_BOX box;
        box.unknown_10 = unknown_39c;
        box.velocity.x = 0.0f;
        box.velocity.y = 0.0f;
        box.velocity.z = 0.0f;
        box.weight = weight;
        InitBox(&box);
    }
    else
    {
        S_THING_WHEEL wheel;
        wheel.unknown_18 = unknown_39c;
        wheel.weight = weight;
        wheel.radius = radius;
        wheel.unknown_1c = unknown_3a0;
        wheel.velocity.x = 0.0f;
        wheel.velocity.y = 0.0f;
        wheel.velocity.z = 0.0f;
        wheel.unknown_4 = unknown_394;
        InitWheel(&wheel);
    }
    C_PhThing::GameInit();
    hit = false;
}

// The base's: the original's jumps there, from its place in the file (PS2's the same)
// FUNCTION: GAME 0x567720
void C_box_new::GameDone()
{
    C_PhThing::GameDone();
}

// The versions SaveData has written: 0 had the box's and the wheel's values, 1 added the hit sound, 2 its restart and
// the material; true for any version (PS2's the same)
// FUNCTION: GAME 0x567730
bool C_box_new::LoadData(C_chunk *chunk)
{
    char version;
    chunk->Read(&version, sizeof(version));
    switch (version)
    {
    case 0:
        chunk->Read(&is_box, sizeof(is_box));
        chunk->Read(&radius, sizeof(radius));
        chunk->Read(&unknown_394, sizeof(unknown_394));
        chunk->Read(&weight, sizeof(weight));
        chunk->Read(&unknown_39c, sizeof(unknown_39c));
        chunk->Read(&unknown_3a0, sizeof(unknown_3a0));
        break;
    case 1:
        chunk->Read(&is_box, sizeof(is_box));
        chunk->Read(&radius, sizeof(radius));
        chunk->Read(&unknown_394, sizeof(unknown_394));
        chunk->Read(&weight, sizeof(weight));
        chunk->Read(&unknown_39c, sizeof(unknown_39c));
        chunk->Read(&unknown_3a0, sizeof(unknown_3a0));
        chunk->Read(&hit_sound, sizeof(hit_sound));
        break;
    case BOX_NEW_DATA_VERSION:
        chunk->Read(&is_box, sizeof(is_box));
        chunk->Read(&radius, sizeof(radius));
        chunk->Read(&unknown_394, sizeof(unknown_394));
        chunk->Read(&weight, sizeof(weight));
        chunk->Read(&unknown_39c, sizeof(unknown_39c));
        chunk->Read(&unknown_3a0, sizeof(unknown_3a0));
        chunk->Read(&hit_sound, sizeof(hit_sound));
        chunk->Read(&hit_sound_restart, sizeof(hit_sound_restart));
        chunk->Read(&material, sizeof(material));
        break;
    }
    if (weight > 100000.0f)
        weight = 100000.0f;
    if (weight < .01f)
        weight = .01f;
    return true;
}

// FUNCTION: GAME 0x567ab0
bool C_box_new::SaveData(C_chunk *chunk)
{
    char version = BOX_NEW_DATA_VERSION;
    chunk->Write(&version, sizeof(version));
    chunk->Write(&is_box, sizeof(is_box));
    chunk->Write(&radius, sizeof(radius));
    chunk->Write(&unknown_394, sizeof(unknown_394));
    chunk->Write(&weight, sizeof(weight));
    chunk->Write(&unknown_39c, sizeof(unknown_39c));
    chunk->Write(&unknown_3a0, sizeof(unknown_3a0));
    chunk->Write(&hit_sound, sizeof(hit_sound));
    chunk->Write(&hit_sound_restart, sizeof(hit_sound_restart));
    chunk->Write(&material, sizeof(material));
    return true;
}

// The base's (C_actor's empty one), then the data
// FUNCTION: GAME 0x567c20
void C_box_new::DuplicateFrom(C_actor *act)
{
    C_PhThing::DuplicateFrom(act);
    C_box_new *box = (C_box_new *)act;
    is_box = box->is_box;
    radius = box->radius;
    unknown_394 = box->unknown_394;
    weight = box->weight;
    unknown_39c = box->unknown_39c;
    unknown_3a0 = box->unknown_3a0;
}

// FUNCTION: GAME 0x567c80
bool C_box_new::Collision(C_actor *act, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f,
    I3D_frame *frm)
{
    hit = true;
    return C_PhThing::Collision(act, v1, v2, v3, f, frm);
}

// FUNCTION: GAME 0x567ca0
bool C_box_new::Hit(int type, const S_vector &v1, const S_vector &v2, const S_vector &v3, float f, C_actor *act,
    uint u, I3D_frame *frm)
{
    hit = true;
    return C_PhThing::Hit(type, v1, v2, v3, f, act, u, frm);
}
