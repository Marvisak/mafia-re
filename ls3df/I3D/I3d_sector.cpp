// The sectors (PS2 I3d_sector.cpp), at 0x1004e9b0-0x100508b0 in the original: the sound properties' functions, then
// I3D_sector in PS2's order, without PS2's SetBBox, Clear and CheckPointDepth. The header's zero vector has its $E at
// the end (0x10050890), after it come SetGroupBits (an in-class function the vtable needs) and C_list::find. Names are
// the PS2 build's unless noted.
// Open: the original's .data has two 4-byte items of this file that nothing references and ours lacks, an int 1 after
// SetName's buffer (0x100a8720) and a zero among Duplicate's messages (0x100a883c, where an empty string would go).
#include "I3D/I3D.h"
#include "dta.h"

// <new> declares operator delete throw(), so VC6 sets no EH state before the lists' destructors in the sector's
#include <new>

// Count the sectors' and the portals' changes, for those who keep what they found in them (PS2 names; the portals'
// file counts portalVersion). Uninitialized, so .bss has them in their hash buckets' order, after the header's zero
// vector.
// GLOBAL: LS3DF 0x101b4f28
int portalVersion;

// GLOBAL: LS3DF 0x101b4f2c
int sectorVersion;

// The listener's defaults: EAX 3.0's generic environment, scaled as the members are, without the room, and half a
// second to blend into it. PS2's only clears the struct and sets the blend time.
// FUNCTION: LS3DF 0x1004e9b0
void S_sector_sound_properties::SetupDefaults()
{
    memset(this, 0, sizeof(S_sector_sound_properties));
    room_lf = 1.0f;                     // 0 mB
    room_hf = .99f;                     // -100 mB
    decay_time = 1490;
    decay_hf_ratio = .38421053f;        // 0.83
    reflections = .67254543f;           // -2602 mB
    reflections_delay = 7;
    reverb = .85f;                      // 200 mB
    reverb_delay = 10;
    environment_diffusion = 1.0f;
    decay_lf_ratio = .47368422f;        // 1.0
    room = 0.0f;
    reflections_pan.Zero();
    reverb_pan.Zero();
    echo_time = 250;
    echo_depth = 0.0f;
    modulation_time = 250;
    modulation_depth = 0.0f;
    room_rolloff_factor = 0.0f;
    air_absorption_hf = .95f;           // -5 mB
    hf_reference = .21052632f;          // 5000 Hz
    lf_reference = .23469388f;          // 250 Hz
    blend_time = 500;
}

// Without all, only the blend time is blended. Each pan is copied into a temporary before it is multiplied, and the
// copies and the sum share one stack area, so the products are named locals.
// Open: a few instructions are scheduled otherwise: t1's subtraction before the flag's load, and the copies' stores
// among the multiplications.
// FUNCTION: LS3DF 0x1004ea50
void S_sector_sound_properties::MakeBlend(const S_sector_sound_properties &a, const S_sector_sound_properties &b,
    float t, bool all)
{
    float t1 = 1.0f - t;
    if (all) {
        room_lf = a.room_lf * t1 + b.room_lf * t;
        room_hf = a.room_hf * t1 + b.room_hf * t;
        decay_time = (int)(a.decay_time * t1 + b.decay_time * t);
        decay_hf_ratio = a.decay_hf_ratio * t1 + b.decay_hf_ratio * t;
        reflections = a.reflections * t1 + b.reflections * t;
        reflections_delay = (int)(a.reflections_delay * t1 + b.reflections_delay * t);
        reverb = a.reverb * t1 + b.reverb * t;
        reverb_delay = (int)(a.reverb_delay * t1 + b.reverb_delay * t);
        environment_diffusion = a.environment_diffusion * t1 + b.environment_diffusion * t;
        decay_lf_ratio = a.decay_lf_ratio * t1 + b.decay_lf_ratio * t;
        room = a.room * t1 + b.room * t;
        {
            S_vector vb = S_vector(b.reflections_pan) * t;
            S_vector va = S_vector(a.reflections_pan) * t1;
            reflections_pan = va + vb;
        }
        {
            S_vector vb = S_vector(b.reverb_pan) * t;
            S_vector va = S_vector(a.reverb_pan) * t1;
            reverb_pan = va + vb;
        }
        echo_time = (int)(a.echo_time * t1 + b.echo_time * t);
        echo_depth = a.echo_depth * t1 + b.echo_depth * t;
        modulation_time = (int)(a.modulation_time * t1 + b.modulation_time * t);
        modulation_depth = a.modulation_depth * t1 + b.modulation_depth * t;
        room_rolloff_factor = a.room_rolloff_factor * t1 + b.room_rolloff_factor * t;
        air_absorption_hf = a.air_absorption_hf * t1 + b.air_absorption_hf * t;
        hf_reference = a.hf_reference * t1 + b.hf_reference * t;
        lf_reference = a.lf_reference * t1 + b.lf_reference * t;
    }
    blend_time = (int)(a.blend_time * t1 + b.blend_time * t);
}

// FUNCTION: LS3DF 0x1004ed30
I3D_sector::I3D_sector()
{
    type = FRAME_SECTOR;
    scene = NULL;
    num_verts = 0;
    verts = NULL;
    num_faces = 0;
    faces = NULL;
    sct_flags = SCTF_UNKNOWN_1 | SCTF_SOUND_CHANGED;
    num_planes = 0;
    planes = NULL;
    lights_changed = 0;
    fog_light = NULL;
    num_fog_lights = 0;
    prev_in_scene = NULL;
    next_in_scene = NULL;
    occluders = NULL;
    occluder = NULL;
    frm_flags |= FRMFLAGS_HR_SECTORS;
    unknown_1e8 = 0;
    unknown_1ec = 0;
    sound_props.SetupDefaults();
    frm_flags |= FRMFLAGS_UNKNOWN_8000000;
    group_bits = 0;
    sectorVersion++;
}

// Takes the sector out of its lights, sounds and portals. The sounds go through DeleteSound, which VC6 inlines here
// with its find and remove calls out of line, and PS2's Clear is written out (no counts or pointers set back).
// FUNCTION: LS3DF 0x1004ee80
I3D_sector::~I3D_sector()
{
    sectorVersion++;
    AddToRemoveFromSceneRing(NULL);
    if (occluder) {
        occluder->sector = NULL;
        occluder = NULL;
    }
    I3D_light **lp = &lights[0];
    for (int i = lights.size(); i--; lp++)
        (*lp)->sectors.remove(this);
    lights.clear();
    for (i = sounds.size(); i--; )
        DeleteSound(sounds[0]);
    sounds.clear();
    delete[] planes;
    I3D_portal **it;
    for (it = portals.begin(); it != portals.end(); it++) {
        I3D_portal *p = *it;
        if (p->connect_sector)
            p->connect_sector->portals_in.remove(p);
        delete p;
    }
    portals.clear();
    for (it = portals_in.begin(); it != portals_in.end(); it++) {
        I3D_portal *p = *it;
        p->connect_sector = NULL;
    }
    portals_in.clear();
    delete[] verts;
    delete[] faces;
}

// A sector's name starts with "sector "
// FUNCTION: LS3DF 0x1004f230
LS3D_RESULT I3D_sector::SetName(const char *new_name)
{
    static char name[64] = "sector ";
    if (!new_name)
        return I3D_frame::SetName(new_name);
    strncpy(name + 7, new_name, 54);
    return I3D_frame::SetName(name);
}

// Moves the sector from its scene's ring of sectors into the other scene's, with its occluder and portals
// FUNCTION: LS3DF 0x1004f260
void I3D_sector::AddToRemoveFromSceneRing(I3D_scene *new_scene)
{
    if (scene == new_scene)
        return;
    if (scene && prev_in_scene) {
        scene->num_sectors--;
        if (scene->sectors == this)
            scene->sectors = next_in_scene;
        if (prev_in_scene == this)
            scene->sectors = NULL;
        else {
            prev_in_scene->next_in_scene = next_in_scene;
            next_in_scene->prev_in_scene = prev_in_scene;
        }
        scene->unknown_25c = 0;
        scene = NULL;
    }
    scene = new_scene;
    if (new_scene) {
        if (!new_scene->sectors) {
            prev_in_scene = this;
            next_in_scene = this;
            new_scene->sectors = this;
        } else {
            new_scene->sectors->prev_in_scene->next_in_scene = this;
            prev_in_scene = new_scene->sectors->prev_in_scene;
            next_in_scene = new_scene->sectors;
            new_scene->sectors->prev_in_scene = this;
        }
        new_scene->num_sectors++;
        scene->unknown_25c = 0;
        if (occluder && occluder->parent == this)
            occluder->LinkTo(scene->primary_sector, I3DLINK_UPDMATRIX);
    }
    sectorVersion++;
    for (I3D_portal **it = portals.begin(); it != portals.end(); it++)
        (*it)->AddToSceneRing(scene);
}

// A fog light that is on becomes the sector's fog light. PS2 also marks the sector for an ambient light.
// FUNCTION: LS3DF 0x1004f3d0
void I3D_sector::AddLight(I3D_light *light)
{
    if (lights.find(light))
        return;
    switch (light->ltype) {
    case I3DLIGHT_FOG:
    case I3DLIGHT_POINTAMBIENT:
    case I3DLIGHT_POINTFOG:
        num_fog_lights++;
        if (light->frm_flags & FRMFLAGS_ON)
            fog_light = light;
        break;
    }
    lights_changed++;
    lights.push_back(light);
    if (!light->sectors.find(this))
        light->sectors.push_back(this);
}

// A fog light taken out gives the place to another fog light that is on
// FUNCTION: LS3DF 0x1004f550
LS3D_RESULT I3D_sector::DeleteLight(I3D_light *light)
{
    if (!lights.find(light))
        return I3DERR_OBJECTNOTFOUND;
    switch (light->ltype) {
    case I3DLIGHT_FOG:
    case I3DLIGHT_POINTAMBIENT:
    case I3DLIGHT_POINTFOG:
        num_fog_lights--;
        fog_light = NULL;
        if (num_fog_lights) {
            for (int i = 0; i < lights.size(); i++) {
                if ((lights[i]->frm_flags & FRMFLAGS_ON) && (lights[i]->ltype == I3DLIGHT_FOG ||
                    lights[i]->ltype == I3DLIGHT_POINTAMBIENT || lights[i]->ltype == I3DLIGHT_POINTFOG)) {
                    fog_light = lights[i];
                    break;
                }
            }
        }
        break;
    }
    lights_changed++;
    light->sectors.remove(this);
    lights.remove(light);
    return I3D_OK;
}

// PS2's also puts a sector without a scene into the scene of the sector it is in
// FUNCTION: LS3DF 0x1004f6d0
void I3D_sector::UpdateV()
{
    UpdateCHPlanesW();
    I3D_portal **it = portals.begin();
    for (int i = portals.end() - portals.begin(); i--; it++)
        (*it)->UpdatePortal();
}

// The planes of the hull's faces in world space, each once, facing out.
// Open: the inline budget. The original calls GetNormal out of line and inlines Normalize after it, as in the camera's
// first frustums; ours inlines GetNormal, with its operator- calls out of line. No budget gives that pair: our Normalize
// costs more than GetNormal, so padding inlines GetNormal first (also through Insanity3D's S_plane::ComputePlane).
// GetNormal alone nested in an inline function of its own (a plane constructor taking the three points) gives the
// original's calls, and then Normalize's null-vector branches store y's and z's constants through the FPU, where the
// original stores them as integers; written as if/else statements they do here, but that breaks Normalize's other
// callers (the light's UpdateV, S_quat::Inverse, GetLightness).
// FUNCTION: LS3DF 0x1004f710
void I3D_sector::UpdateCHPlanesW()
{
    if (!(sct_flags & SCTF_PREPARED))
        return;
    if (planes)
        delete[] planes;
    C_list<S_plane> list(8);
    const ushort *fc = faces;
    int i = num_faces;
    UpdateWMatrix();
    for (; i; i--) {
        S_vector v0, v1, v2;
        v0 = verts[*fc++] * matrix;
        v1 = verts[*fc++] * matrix;
        v2 = verts[*fc++] * matrix;
        S_plane pl;
        pl.normal.GetNormal(v0, v1, v2);
        pl.normal.Normalize();
        pl.d = -pl.normal.Dot(v0);
        bool add = true;
        for (S_plane *p = list.begin(); p < list.end(); p++) {
            if (fabs(p->d - pl.d) < .001 && fabs(p->normal.x - pl.normal.x) < .001 &&
                fabs(p->normal.y - pl.normal.y) < .001 && fabs(p->normal.z - pl.normal.z) < .001) {
                add = false;
                break;
            }
        }
        if (add)
            list.push_back(pl);
    }
    num_planes = list.size();
    planes = new S_plane[num_planes];
    if (!planes) {
        dbgPrintf(" !! Out of memory !!  I3D_sector::UpdateCHPlanesW() [1]");
        return;
    }
    S_plane *dst = planes;
    for (S_plane *p = list.begin(); p < list.end(); p++, dst++)
        *dst = *p;
}

// Prepares the sector once: its hull's planes, and the occluder of the hull where it is to have one. The occluder's
// failure is an else, with UpdateV once at the end: a return of its own makes VC6 save ebp before the first test.
// FUNCTION: LS3DF 0x1004fb40
void I3D_sector::MakeReady()
{
    if (sct_flags & SCTF_PREPARED)
        return;
    sct_flags |= SCTF_PREPARED;
    if (!num_verts)
        return;
    if (sct_flags & SCTF_OCCLUDER) {
        I3D_occluder *occl = (I3D_occluder *)pI3DDriver->CreateFrame(FRAME_OCCLUDER);
        if (!occl)
            dbgPrintf(" !! Out of memory !!  I3D_sector::MakeReady() [2]");
        else {
            occl->Build(verts, num_verts, faces, num_faces, 0xc);
            if (!owner)
                dbgPrintf("!!+ Sector has no owner. I3D_sector::MakeReady() this:0x%x %s", this, GetName());
            else {
                if (owner->GetType() == FRAME_SCENE)
                    ((I3D_scene *)owner)->AddFrame(occl);
                else
                    ((I3D_model *)owner)->AddFrame(occl);
                occl->Release();
            }
            if (scene) {
                occl->SetLocalMatrix(GetMatrix());
                occl->LinkTo(scene->primary_sector, 0);
            } else
                occl->LinkTo(this, 0);
            occl->Update();
            occluder = occl;
            occl->sector = this;
        }
    }
    UpdateV();
}

// The box through a reference and IsValid, and the count before the planes: otherwise VC6 sums the plane's dot product
// in another order
// FUNCTION: LS3DF 0x1004fcc0
bool I3D_sector::CheckPoint(const S_vector &pos)
{
    if (!(frm_flags & FRMFLAGS_HR_BOUND_W_VALID))
        UpdateHRBoundVolW();
    const I3D_bbox &bb = bound_w.bbox;
    if (!bb.IsValid())
        return false;
    if (bb.min.x > pos.x || bb.max.x < pos.x)
        return false;
    if (bb.min.y > pos.y || bb.max.y < pos.y)
        return false;
    if (bb.min.z > pos.z || bb.max.z < pos.z)
        return false;
    int i = num_planes;
    const S_plane *p = planes;
    for (; i--; p++) {
        if (pos.Dot(p->normal) + p->d < -MRG_ZERO)
            return false;
    }
    return true;
}

// PS2's stores the data as given. The PC's clamps the values to their ranges (bounds first: the original compares the
// bound with the value), and takes a pan as a vector, which it shortens to 1 at most (through a reciprocal of its own:
// with operator*= VC6 sums the squares in another order).
// FUNCTION: LS3DF 0x1004fdb0
LS3D_RESULT I3D_sector::SetSoundData(I3D_SECTOR_SOUND_DATA_INDEX index, uint data)
{
    if (index >= SSD_LAST)
        return I3DERR_INVALIDPARAMS;
    switch (index) {
    case SSD_ROOM_LF:
    case SSD_ROOM_HF:
    case SSD_DECAY_HF_RATIO:
    case SSD_REFLECTIONS:
    case SSD_REVERB:
    case SSD_ENVIRONMENT_DIFFUSION:
    case SSD_DECAY_LF_RATIO:
    case SSD_ROOM:
    case SSD_ECHO_DEPTH:
    case SSD_MODULATION_DEPTH:
    case SSD_ROOM_ROLLOFF_FACTOR:
    case SSD_AIR_ABSORPTION_HF:
    case SSD_HF_REFERENCE:
    case SSD_LF_REFERENCE:
        data = I3DFloatAsInt(Min(1.0f, Max(0.0f, I3DIntAsFloat(data))));
        break;
    case SSD_DECAY_TIME:
        data = Max(100, Min(20000, (int)data));
        break;
    case SSD_REFLECTIONS_DELAY:
        data = Max(0, Min(300, (int)data));
        break;
    case SSD_REVERB_DELAY:
        data = Max(0, Min(100, (int)data));
        break;
    case SSD_ECHO_TIME:
        data = Max(75, Min(250, (int)data));
        break;
    case SSD_MODULATION_TIME:
        data = Max(39, Min(4000, (int)data));
        break;
    case SSD_REFLECTIONS_PAN:
        {
            S_vector v = *(const S_vector *)data;
            float m = v.Magnitude();
            if (m > 1.0f) {
                float r = 1.0f / m;
                v.x *= r;
                v.y *= r;
                v.z *= r;
            }
            sound_props.reflections_pan = v;
            sct_flags |= SCTF_SOUND_CHANGED;
        }
        return I3D_OK;
    case SSD_REVERB_PAN:
        {
            S_vector v = *(const S_vector *)data;
            float m = v.Magnitude();
            if (m > 1.0f) {
                float r = 1.0f / m;
                v.x *= r;
                v.y *= r;
                v.z *= r;
            }
            sound_props.reverb_pan = v;
            sct_flags |= SCTF_SOUND_CHANGED;
        }
        return I3D_OK;
    }
    ((uint *)&sound_props)[index] = data;
    sct_flags |= SCTF_SOUND_CHANGED;
    return I3D_OK;
}

// A pan's address
// FUNCTION: LS3DF 0x10050140
uint I3D_sector::GetSoundData(I3D_SECTOR_SOUND_DATA_INDEX index) const
{
    switch (index) {
    case SSD_REFLECTIONS_PAN:
        return (uint)&sound_props.reflections_pan;
    case SSD_REVERB_PAN:
        return (uint)&sound_props.reverb_pan;
    }
    return ((const uint *)&sound_props)[index];
}

// A sound that is on plays on in the sector
// FUNCTION: LS3DF 0x10050180
void I3D_sector::AddSound(I3D_sound *sound)
{
    if (sounds.find(sound))
        return;
    sounds.push_back(sound);
    if (!sound->sectors.find(this))
        sound->sectors.push_back(this);
    if (sound->frm_flags & FRMFLAGS_ON)
        sound->SetOn(true);
}

// FUNCTION: LS3DF 0x100502f0
LS3D_RESULT I3D_sector::DeleteSound(I3D_sound *sound)
{
    if (!sounds.find(sound))
        return I3DERR_OBJECTNOTFOUND;
    if (sound->sectors.remove(this) && scene && scene->current_camera && scene->current_camera->sector == this &&
        sound->sectors.size() > 0)
        sound->SetOn(false);
    sounds.remove(sound);
    return I3D_OK;
}

// Copies the hull, which a prepared sector can't take any more (the message: "explain to dlouhy@..."). PS2's copies the
// portals too.
// FUNCTION: LS3DF 0x10050430
LS3D_RESULT I3D_sector::Duplicate(I3D_frame *frame)
{
    if (!frame)
        return I3DERR_INVALIDPARAMS;
    I3D_frame::Duplicate(frame);
    I3D_sector *sct = (I3D_sector *)frame;
    if (sct->sct_flags & SCTF_PREPARED)
        dbgPrintf("!! Duplikace pripraveneho sectoru - vysvetli dlouhy@illusionsoftworks.cz");
    else {
        delete[] verts;
        verts = NULL;
        delete[] faces;
        num_verts = 0;
        faces = NULL;
        num_faces = 0;
        num_verts = sct->num_verts;
        if (num_verts) {
            verts = new S_vector[num_verts];
            if (!verts) {
                dbgPrintf(" !! Out of memory !!  I3D_sector::DuplicateSector()");
                num_verts = 0;
                return I3DERR_OUTOFMEM;
            }
            S_vector *dst = verts;
            const S_vector *src = sct->verts;
            for (int i = num_verts; i--; )
                *dst++ = *src++;
            num_faces = sct->num_faces;
            if (num_faces) {
                faces = new ushort[num_faces * 3];
                if (!faces) {
                    dbgPrintf(" !! Out of memory !!  I3D_sector::DuplicateSector() [2]");
                    num_faces = 0;
                    return I3DERR_OUTOFMEM;
                }
                ushort *fd = faces;
                const ushort *fs = sct->faces;
                for (i = num_faces * 3; i--; )
                    *fd++ = *fs++;
            }
        }
    }
    sct_flags = (sct->sct_flags & (SCTF_OCCLUDER | SCTF_UNKNOWN_100)) | SCTF_SOUND_CHANGED;
    group_bits = sct->group_bits;
    return I3D_OK;
}

// The flags, group bits, the hull, the box and the portals. PS2's reads the box into the sector and makes the planes'
// array here. The failures have a message each (VC6 merges their ends).
// FUNCTION: LS3DF 0x100505c0
void I3D_sector::LoadImage()
{
    I3D_frame::LoadImage();
    dtaRead(LoadImageHandle, &sct_flags, 0x10);
    verts = new S_vector[num_verts];
    faces = new ushort[num_faces * 3];
    if (!verts || !faces) {
        delete[] verts;
        verts = NULL;
        delete[] faces;
        faces = NULL;
        dbgPrintf("!! Out of memory  I3D_sector::LoadImage() %s  %s", GetName(), ImageFileName);
        ImageErrFlg = I3DERR_OUTOFMEM;
        return;
    }
    dtaRead(LoadImageHandle, verts, num_verts * sizeof(S_vector));
    dtaRead(LoadImageHandle, faces, num_faces * 3 * sizeof(ushort));
    {
        I3D_bbox bbox;
        dtaRead(LoadImageHandle, &bbox, sizeof(bbox));
        SetBoundVolume(bbox);
    }
    {
        int num = 0;
        dtaRead(LoadImageHandle, &num, 1);
        portals.reserve(num);
        while (num--) {
            I3D_portal *p = new I3D_portal;
            if (!p) {
                dbgPrintf("!! Out of memory  I3D_sector::LoadImage() %s  %s", GetName(), ImageFileName);
                ImageErrFlg = I3DERR_OUTOFMEM;
                return;
            }
            p->sector = this;
            p->LoadImage();
            portals.push_back(p);
        }
    }
}

// The functions of other files the sectors call, until those are written. They come after the file's functions (see
// I3d_driver.cpp); their markers are in I3D.h, by name, as their addresses come before the file's.
#pragma auto_inline(off)

I3D_portal::I3D_portal()
{
    renderState[6]++;
}

I3D_portal::~I3D_portal()
{
    renderState[7]++;
}

void I3D_portal::AddToSceneRing(I3D_scene *scene)
{
    renderState[8]++;
}

void I3D_portal::LoadImage()
{
    renderState[9]++;
}

#pragma auto_inline(on)
