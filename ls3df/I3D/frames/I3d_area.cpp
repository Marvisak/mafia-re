// The areas (PS2 I3d_area.cpp), at 0x10005180-0x10005db0 in the original: I3D_area in PS2's order, with the PC's
// SaveImage and LoadImage at the end, then C_list::remove. Names are the PS2 build's unless noted.
// - No precompiled header: the header's zero vector has its $E first (0x10005180), and the in-class functions the
//   vtable needs (the frame's empty Tick, the fade time's) come right after the constructor. Under /YX
//   (tools/autopch.py) both wait for the file's end; #pragma hdrstop before the includes leaves the file without one.
// - The original keeps this file's copies of the empty Tick (0x10005240) and of C_list::remove (0x10005d20), which the
//   linker folds with other functions; ours are the scene's and the sector's (I3D.h, ls3df.h).
#pragma hdrstop
#include "I3D/I3D.h"
#include "IGraph/IGraph.h"

#include <string.h>

// <new> declares operator delete throw(), so VC6 sets no EH state before the sound list's destructor in the area's
#include <new>

// FUNCTION: LS3DF 0x100051a0
I3D_area::I3D_area()
{
    type = FRAME_AREA;
    next_in_scene = NULL;
    prev_in_scene = NULL;
    num_verts = 0;
    verts = NULL;
    verts_w = NULL;
    num_faces = 0;
    faces = NULL;
    planes = NULL;
    scene = NULL;
    frm_flags |= FRMFLAGS_HR_AREAS;
    unknown_164 = 0;
    switch_fade_time = 500;
}

// Takes the area out of its sounds and its scene. The sounds go through DeleteSound, which VC6 inlines here.
// FUNCTION: LS3DF 0x100052b0
I3D_area::~I3D_area()
{
    for (int i = sounds.size(); i--; )
        DeleteSound(sounds[0]);
    sounds.clear();
    delete[] verts;
    delete[] verts_w;
    delete[] faces;
    delete[] planes;
    AddToRemoveFromSceneRing(NULL);
}

// Moves the area from its scene's ring of areas into the other scene's
// FUNCTION: LS3DF 0x10005440
void I3D_area::AddToRemoveFromSceneRing(I3D_scene *new_scene)
{
    if (scene && prev_in_scene) {
        if (scene->areas == this)
            scene->areas = next_in_scene;
        if (prev_in_scene == this)
            scene->areas = NULL;
        else {
            prev_in_scene->next_in_scene = next_in_scene;
            next_in_scene->prev_in_scene = prev_in_scene;
        }
        scene->num_areas--;
        scene->areas_changed = true;
        scene = NULL;
    }
    if (!new_scene)
        return;
    scene = new_scene;
    if (!scene->areas) {
        prev_in_scene = this;
        next_in_scene = this;
        scene->areas = this;
    } else {
        scene->areas->prev_in_scene->next_in_scene = this;
        prev_in_scene = scene->areas->prev_in_scene;
        next_in_scene = scene->areas;
        scene->areas->prev_in_scene = this;
    }
    scene->num_areas++;
    scene->areas_changed = true;
}

// The area goes into the scene of its new parent: the scene that owns the parent, or the one of the sector the parent
// is in. The owner is tested as it is, then copied: a local tested instead swaps eax and ecx.
// FUNCTION: LS3DF 0x10005550
LS3D_RESULT I3D_area::LinkTo(I3D_frame *frame, uint flags)
{
    if (frame == parent)
        return I3D_OK;
    AddToRemoveFromSceneRing(NULL);
    if (frame) {
        I3D_scene *scn;
        if (frame->m_pOwner && frame->m_pOwner->GetType() == FRAME_SCENE)
            scn = (I3D_scene *)frame->m_pOwner;
        else {
            scn = NULL;
            for (I3D_frame *f = frame; !scn; ) {
                if (f->GetType() == FRAME_SECTOR)
                    scn = ((I3D_sector *)f)->scene;
                else
                    f = f->parent;
            }
        }
        AddToRemoveFromSceneRing(scn);
    }
    I3D_frame::LinkTo(frame, flags);
    return I3D_OK;
}

// The vertices in world space, and the faces' planes through S_plane's constructor (I3d_math.h), whose normalization
// stores the null normal's -1 or 1 as integer constants (S_plane::NormalizeNormal), where Normalize's would go through
// the FPU.
// FUNCTION: LS3DF 0x100055d0
void I3D_area::UpdateV()
{
    if (!num_verts)
        return;
    const S_vector *v = verts;
    S_vector *vw = verts_w;
    for (int i = num_verts; i--; )
        *vw++ = *v++ * GetMatrix();
    const ushort *fc = faces;
    S_plane *p = planes;
    for (i = num_faces; i--; ) {
        S_plane pl(verts_w[fc[0]], verts_w[fc[1]], verts_w[fc[2]]);
        fc += 3;
        *p++ = pl;
    }
}

// The other area's faces are built into this one. Two returns of the result, as PS2's: with the call in an if block,
// VC6 saves only esi before the test of frame.
// FUNCTION: LS3DF 0x10005820
LS3D_RESULT I3D_area::Duplicate(I3D_frame *frame)
{
    if (!frame)
        return I3DERR_INVALIDPARAMS;
    LS3D_RESULT result = I3D_frame::Duplicate(frame);
    if (frame->GetType() != FRAME_AREA)
        return result;
    I3D_area *area = (I3D_area *)frame;
    Build(area->verts, area->num_verts, area->faces, area->num_faces, sizeof(S_vector));
    return result;
}

// Copies the faces and the vertices, which are stride bytes apart. The vertices are read through a pointer of their
// own, which VC6 loads before the loop's guard.
// FUNCTION: LS3DF 0x10005880
LS3D_RESULT I3D_area::Build(const S_vector *vp, uint nv, const ushort *fp, uint nf, uint stride)
{
    delete[] verts;
    delete[] verts_w;
    delete[] faces;
    delete[] planes;
    verts = new S_vector[nv];
    verts_w = new S_vector[nv];
    faces = new ushort[nf * 3];
    planes = new S_plane[nf];
    if (!verts || !verts_w || !faces || !planes) {
        dbgPrintf("!! Out of memory  I3D_area::Build()");
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return I3DERR_OUTOFMEM;
    }
    num_verts = nv;
    const char *src = (const char *)vp;
    for (uint i = 0; i < nv; i++) {
        verts[i] = *(const S_vector *)src;
        src += stride;
    }
    num_faces = nf;
    memcpy(faces, fp, nf * 3 * sizeof(ushort));
    return I3D_OK;
}

// FUNCTION: LS3DF 0x100059d0
bool I3D_area::CheckPoint(const S_vector &pos)
{
    int i = num_faces;
    const S_plane *p = planes;
    for (; i--; p++) {
        if (pos.Dot(p->normal) + p->d > 0.0f)
            return false;
    }
    return true;
}

// A sound knows its areas as it knows its sectors
// FUNCTION: LS3DF 0x10005a30
void I3D_area::AddSound(I3D_sound *sound)
{
    unknown_164 = -1;
    if (sounds.find(sound))
        return;
    sounds.push_back(sound);
    if (sound->areas.find(this))
        return;
    sound->areas.push_back(this);
}

// FUNCTION: LS3DF 0x10005b90
LS3D_RESULT I3D_area::DeleteSound(I3D_sound *sound)
{
    unknown_164 = -1;
    if (!sounds.find(sound))
        return I3DERR_OBJECTNOTFOUND;
    sound->areas.remove(this);
    sounds.remove(sound);
    return I3D_OK;
}

// The areas aren't saved or loaded ("nepodporovany typ": unsupported type)
// FUNCTION: LS3DF 0x10005ca0
void I3D_area::SaveImage()
{
    dbgPrintf("** nepodporovany typ:%s  SaveImage()  %s", pI3DDriver->FrameType2String(type), GetName());
}

// Folded with I3D_sound::LoadImage, so the marker is in I3D.h, by name
void I3D_area::LoadImage()
{
    dbgPrintf("** nepodporovany typ:%s  LoadImage()  %s", pI3DDriver->FrameType2String(type), GetName());
}
