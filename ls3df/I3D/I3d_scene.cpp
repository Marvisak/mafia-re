// The scene (PS2 I3d_scene.cpp), at 0x10048a00-0x1004e9b0 in the original: I3D_scene in PS2's order, with the PC's
// shadow and mirror lists after Close. After the last function come the copies of the C_list functions of the weather's
// structures, and of the scene's inline functions, the linker kept. Names are the PS2 build's unless noted.
// - /YX (tools/autopch.py): the header's zero vector has its $E after SetBackdropRange (0x1004a860), ours after Close,
//   and the copies of the class's in-class functions come at the file's end, ours after m_UpdateSoundProperties: the
//   original's headers have more inline code before them.
// - .bss: the function-local statics' names are guesses that don't hash into the original's order (see I3D.h).
#include "I3D/I3D.h"
#include "I3D/IBManager.h"
#include "I3D/render.h"
#include "I3D/VBManager.h"
#include "IGraph/IGraph.h"
#include "ISound/ISound.h"
#include "ISound/eax.h"
#include <mmsystem.h>

// The point the transformations give, with 1 / w (PS2 name)
struct S_vector4 : public S_vector {
    float w;
};

// A weather particle's vertex, 12 to a particle (the name is a guess)
struct WS_VERTEX {
    S_vector pos;
    uint color;
    float u, v;
};

// The loader (Load.cpp, not written yet)
LS3D_RESULT LoadHierarchy(const char *file_name, uint flags, void *cb, void *context, I3D_frame *root);

// ITexture.cpp
void UnloadManagedTextures();

// I3d_driver.cpp
extern int driverTickTime;

// IGraph.cpp's fog render states
extern int m_CurrFogType;
extern float m_CurrFogStart;
extern float m_CurrFogEnd;

// render.cpp: the areas the camera is in, and the sectors it was last in
extern C_list<I3D_area *> CurrentActiveAreaList;
extern I3D_sector *pLastCameraSector;
extern I3D_sector *pPrevLastCameraSector;

// The sectors near the camera, and the weather regions near it
// GLOBAL: LS3DF 0x101b4e4c
C_list<I3D_sector *> WSNearSectorList;

// GLOBAL: LS3DF 0x101b4e60
C_list<DUMMY_STRUCT *> WSNearDummyList;

// The sound environment outside every sector: m_UpdateSoundProperties blends from it without a previous sector
// GLOBAL: LS3DF 0x101b4e78
S_sector_sound_properties InitialSoundProp;

// SetSpace's parameter, for sceneSS
// GLOBAL: LS3DF 0x101b4e5c
bool SetSpaceParam;

// The weather system's state the scenes share. VC6 lays out zero-initialized globals in declaration order after the
// ones its hash places, and these come in PS2's .sbss order: the particles' vertex buffer, the scene whose colors it
// holds, the number of particles it has room for, the scenes, the particles in it and their texture.
// GLOBAL: LS3DF 0x101b4efc
COMPLETE_VB_STRUCT WSComplVBStr = {NULL, NULL, 0};

// GLOBAL: LS3DF 0x101b4f08
I3D_scene *WSLastScene = NULL;

// GLOBAL: LS3DF 0x101b4f0c
int MaxWECnt = 0;

// GLOBAL: LS3DF 0x101b4f10
int WSRefCnt = 0;

// GLOBAL: LS3DF 0x101b4f14
uint CurrentWECnt = 0;

// GLOBAL: LS3DF 0x101b4f18
ITexture *pSnowTexture = NULL;

// FUNCTION: LS3DF 0x10048ab0
I3D_scene::I3D_scene()
{
    type = FRAME_SCENE;
    ipol_set = NULL;
    cb_context = 0;
    callback = NULL;
    unknown_220 = S_vector(0.0f, .5f, .4f);
    backdrop_ncp = 1.0f;
    backdrop_fcp = 10000.0f;
    current_camera = NULL;
    num_portals = 0;
    portals = NULL;
    unknown_25c = 0;
    num_sectors = 0;
    sectors = NULL;
    unknown_268 = 0;
    unknown_26c = 0;
    unknown_270 = 0;
    unknown_274 = false;
    unknown_275 = false;
    num_areas = 0;
    areas = NULL;
    areas_changed = false;
    listener = NULL;
    frm_flags |= FRMFLAGS_LOCKED;

    static I3D_bbox bigBox(S_vector(-10000.0f, -10000.0f, -10000.0f), S_vector(10000.0f, 10000.0f, 10000.0f));

    primary_sector = (I3D_sector *)pI3DDriver->CreateFrame(FRAME_SECTOR);
    primary_sector->I3D_frame::SetName("Primary sector");
    primary_sector->scene = this;
    primary_sector->sct_flags |= SCTF_PRIMARY;
    primary_sector->frm_flags |= FRMFLAGS_LOCKED;
    primary_sector->SetBoundVolume(bigBox);
    backdrop_sector = (I3D_sector *)pI3DDriver->CreateFrame(FRAME_SECTOR);
    backdrop_sector->I3D_frame::SetName("Backdrop sector");
    backdrop_sector->scene = this;
    backdrop_sector->sct_flags |= SCTF_BACKDROP;
    backdrop_sector->frm_flags |= FRMFLAGS_LOCKED;
    backdrop_sector->SetBoundVolume(bigBox);
    omni_sector = (I3D_sector *)pI3DDriver->CreateFrame(FRAME_SECTOR);
    omni_sector->I3D_frame::SetName("Omni sector");
    omni_sector->scene = this;
    omni_sector->sct_flags |= SCTF_OMNI;
    omni_sector->frm_flags |= FRMFLAGS_LOCKED;
    near_sector = (I3D_sector *)pI3DDriver->CreateFrame(FRAME_SECTOR);
    near_sector->I3D_frame::SetName("Near sector");
    near_sector->scene = this;
    near_sector->sct_flags |= SCTF_NEAR;
    near_sector->frm_flags |= FRMFLAGS_LOCKED;
    SetRect(&viewport, 0, 0, pGlobalIGraph->Scrn_sx(), pGlobalIGraph->Scrn_sy());

    WSRefCnt++;
    ws_flags = 0;
    WeatherSystemReset();
    if (!pSnowTexture)
        pGlobalIGraph->CreateITexture("Snow.tga", NULL, 2, &pSnowTexture);
    InitialSoundProp.SetupDefaults();
}

// FUNCTION: LS3DF 0x10048ed0
I3D_scene::~I3D_scene()
{
    Close();
    if (current_camera) {
        current_camera->scene = NULL;
        current_camera->sector = NULL;
        current_camera->Release();
        current_camera = NULL;
    }
    if (primary_sector)
        primary_sector->Release();
    if (backdrop_sector)
        backdrop_sector->Release();
    if (omni_sector)
        omni_sector->Release();
    if (near_sector)
        near_sector->Release();
    delete listener;
    if (--WSRefCnt <= 0) {
        if (pSnowTexture) {
            pSnowTexture->Release();
            pSnowTexture = NULL;
        }
        if (WSComplVBStr.vb) {
            cVBMan.m_Free(WSComplVBStr);
            memset(&WSComplVBStr, 0, sizeof(WSComplVBStr));
        }
    }
    pI3DDriver->scenes.remove(this);
}

// FUNCTION: LS3DF 0x10049120
SCN_CALLBACK *I3D_scene::SetCallback(SCN_CALLBACK *cb, uint context)
{
    if (!cb)
        cb_context = 0;
    else
        cb_context = context;
    SCN_CALLBACK *old = callback;
    callback = cb;
    return old;
}

// FUNCTION: LS3DF 0x10049160
LS3D_RESULT I3D_scene::Render()
{
    return pI3DDriver->Render(this);
}

// The primary sector's frames, then the other sectors' the flags ask for
// FUNCTION: LS3DF 0x10049180
LS3D_RESULT I3D_scene::EnumFrames(I3D_ENUMPROC *proc, uint user, uint flags, const char *mask) const
{
    LS3D_RESULT ret = primary_sector->EnumFrames(proc, user, flags, mask);
    if (ret < 0)
        return ret;
    if (flags & ENUMF_BACKDROP) {
        ret = backdrop_sector->EnumFrames(proc, user, flags, mask);
        if (ret < 0)
            return ret;
    }
    if (flags & ENUMF_OMNI) {
        ret = omni_sector->EnumFrames(proc, user, flags, mask);
        if (ret < 0)
            return ret;
    }
    if (flags & ENUMF_NEAR)
        return near_sector->EnumFrames(proc, user, flags, mask);
    return ret;
}

// FUNCTION: LS3DF 0x10049210
I3D_frame *I3D_scene::FindFrame(const char *name, uint flags) const
{
    I3D_frame *frm = primary_sector->GetChildFrame(name, flags);
    if (frm)
        return frm;
    if (flags & ENUMF_BACKDROP) {
        frm = backdrop_sector->GetChildFrame(name, flags);
        if (frm)
            return frm;
    }
    if (flags & ENUMF_OMNI) {
        frm = omni_sector->GetChildFrame(name, flags);
        if (frm)
            return frm;
    }
    if (flags & ENUMF_NEAR) {
        frm = near_sector->GetChildFrame(name, flags);
        if (frm)
            return frm;
    }
    return NULL;
}

// The scene takes a reference; an emitter is ticked
// FUNCTION: LS3DF 0x10049290
void I3D_scene::AddFrame(I3D_frame *frame)
{
    if (frame->owner == frame)
        return;
    frame->ref++;
    frame->DeleteFromOwner();
    frame->owner = this;
    owned_frames.push_back(frame);
    if (frame->GetType() != FRAME_EMITOR)
        return;
    I3D_frame **p = &tick_frames[0];
    for (int i = tick_frames.size(); i--; p++) {
        if (*p == frame)
            return;
    }
    tick_frames.push_back(frame);
    frame->frm_flags |= FRMFLAGS_TICK;
}

// FUNCTION: LS3DF 0x10049400
LS3D_RESULT I3D_scene::DeleteFrame(I3D_frame *frame)
{
    if (owned_frames.remove(frame)) {
        frame->owner = NULL;
        if (frame->frm_flags & FRMFLAGS_TICK) {
            frame->frm_flags &= ~FRMFLAGS_TICK;
            tick_frames.remove(frame);
        }
        if (frame->ref > 0)
            frame->Release();
        return I3D_OK;
    }
    return I3DERR_OBJECTNOTFOUND;
}

// FUNCTION: LS3DF 0x10049520
void I3D_scene::SetGlobalColor(const S_vector &color)
{
    int i = primary_sector->lights.size();
    I3D_light **lights = &primary_sector->lights[0];
    while (i--)
        lights[i]->SetDiffuse(color);
}

// The lights' brightness at pos: the colors' luminance, faded over a point light's range and a spot's cone, the ambient
// lights' added in; with I3D_LIGHTNESS_TESTCOL only the lights a ray from pos reaches
// FUNCTION: LS3DF 0x10049560
float I3D_scene::GetLightness(const S_vector &pos, I3D_sector *sector, uint flags, I3D_frame *frame)
{
    if (!sector) {
        sector = GetSector(pos, NULL);
        if (!sector)
            return 0.0f;
    }
    float ambient = 0.0f;
    float lightness = 0.0f;
    I3D_light **lights = &sector->lights[0];
    for (int i = sector->lights.size(); i--; ) {
        I3D_light *lt = lights[i];
        if (!(lt->frm_flags & FRMFLAGS_ON))
            continue;
        if (!(lt->mode & I3DLIGHTMODE_LIGHTNESS))
            continue;
        float f = lt->color.x * .3f + lt->color.y * .6f + lt->color.z * .1f;
        switch (lt->ltype) {
        case I3DLIGHT_AMBIENT:
            ambient += f;
            continue;
        case I3DLIGHT_SPOT: {
            S_vector dir = pos - lt->w_pos;
            dir.Normalize();
            float a = dir.Dot(lt->w_dir);
            if (a <= lt->outer_cos)
                continue;
            if (a <= lt->inner_cos)
                f *= (a - lt->outer_cos) * lt->cone_scale;
        }
            // fall through
        case I3DLIGHT_POINT: {
            float dist = (pos - lt->w_pos).Magnitude();
            if (dist >= lt->far_range)
                continue;
            if (dist > lt->near_range)
                f *= (lt->far_range - dist) / (lt->far_range - lt->near_range);
        } break;
        case I3DLIGHT_DIRECTIONAL:
            break;
        case I3DLIGHT_NULL:
        case I3DLIGHT_FOG:
        case I3DLIGHT_POINTAMBIENT:
        case I3DLIGHT_POINTFOG:
            continue;
        }
        if (flags & I3D_LIGHTNESS_TESTCOL) {
            uint col_flags = 0x21000000;
            S_vector dir;
            if (lt->ltype == I3DLIGHT_DIRECTIONAL) {
                dir = -lt->w_dir;
                col_flags = 0x21040000;
            } else
                dir = lt->w_pos - pos;
            if (TestColGeo(pos, dir, col_flags, NULL, NULL, NULL, NULL, NULL, frame, 0))
                continue;
        }
        lightness += f;
    }
    float ret = lightness + ambient;
    if (ret < 0.0f)
        return 0.0f;
    return ret < 1.0 ? ret : 1.0;
}

// FUNCTION: LS3DF 0x100499a0
void I3D_scene::SetActiveCamera(I3D_camera *cam)
{
    if (current_camera == cam)
        return;
    if (current_camera) {
        current_camera->scene = NULL;
        current_camera->sector = NULL;
        current_camera->Release();
    }
    current_camera = cam;
    if (cam) {
        cam->ref++;
        current_camera->scene = this;
        current_camera->sector = GetSector(current_camera->GetMatrix()(3), NULL);
    }
}

// The world position and direction of a ray through the screen point, from the active camera
// FUNCTION: LS3DF 0x10049a30
LS3D_RESULT I3D_scene::UnmapScreenPoint(int x, int y, S_vector &pos, S_vector &dir) const
{
    if (!current_camera)
        return I3DERR_NOTINITIALIZED;
    float half_x = pGlobalIGraph->Scrn_sx() * .5f;
    float half_y = pGlobalIGraph->Scrn_sy() * .5f;
    I3D_camera *cam = current_camera;
    float fx = (x - half_x) / half_x;
    float fy = (y - half_y) / half_y;
    if (cam->orthogonal) {
        float s = 1.0f / cam->ortho_scale;
        pos = S_vector(s * fx, -(fy / current_camera->aspect_ratio * s), 0.0f) * cam->GetMatrix();
        S_vector v = current_camera->GetMatrix()(2);
        v.Normalize();
        dir = v;
    } else {
        pos = cam->GetMatrix()(3);
        S_vector v(fx * current_camera->ncp_x, -(fy * current_camera->ncp_y), current_camera->ncp);
        S_matrix m = current_camera->view_inv;
        dir = v.RotateByMatrix(m);
        dir.Normalize();
    }
    return I3D_OK;
}

// The points on the screen, with 1 / w
// FUNCTION: LS3DF 0x10049e60
LS3D_RESULT I3D_scene::TransformPoints(const S_vector *in, S_vector4 *out, uint num) const
{
    if (!current_camera)
        return I3DERR_NOTINITIALIZED;
    const S_matrix &vp = current_camera->m_view_proj;
    float half_x = pGlobalIGraph->Scrn_sx() * .5;
    float half_y = pGlobalIGraph->Scrn_sy() * .5;
    while (num--) {
        const S_matrix &m = *pViewProjMatrix;
        float w = m(0, 3) * in->x + m(1, 3) * in->y + m(2, 3) * in->z + m(3, 3);
        if (IsAbsMrgZero(w))
            out->w = 1e16f;
        else
            out->w = 1.0 / w;
        (S_vector &)*out = *in * vp;
        out->x = (out->x * out->w + 1.0f) * half_x;
        out->y = (1.0f - out->y * out->w) * half_y;
        in++;
        out++;
    }
    return I3D_OK;
}

// Open: the original reads the count once, into a register of its own that push_back's test reuses (so the pointer
// takes edi, saved at the start); ours reloads it. Loop forms, a count local and a flag compile the same.
// FUNCTION: LS3DF 0x10049f90
void I3D_scene::AddTickFrame(I3D_frame *frame)
{
    I3D_frame **p = &tick_frames[0];
    for (int i = tick_frames.size(); i--; p++) {
        if (*p == frame)
            goto found;
    }
    tick_frames.push_back(frame);
found:
    frame->frm_flags |= FRMFLAGS_TICK;
}

// FUNCTION: LS3DF 0x1004a050
LS3D_RESULT I3D_scene::DeleteTickFrame(I3D_frame *frame)
{
    if (tick_frames.remove(frame)) {
        frame->frm_flags &= ~FRMFLAGS_TICK;
        return I3D_OK;
    }
    return I3DERR_OBJECTNOTFOUND;
}

// A frame may take itself out of the list when it is ticked
// FUNCTION: LS3DF 0x1004a0e0
void I3D_scene::Tick(int time)
{
    for (int i = 0; i < tick_frames.size(); i++) {
        I3D_frame *frm = tick_frames[i];
        frm->Tick(time);
        if (frm != tick_frames[i])
            i--;
    }
    if (ipol_set)
        ipol_set->Tick(time);
}

// FUNCTION: LS3DF 0x1004a140
I3D_sector *I3D_scene::CreateSector()
{
    I3D_sector *sct = new I3D_sector;
    if (!sct)
        return NULL;
    sct->AddToRemoveFromSceneRing(this);
    return sct;
}

// FUNCTION: LS3DF 0x1004a1d0
I3D_sector *I3D_scene::GetSector(const S_vector &pos, I3D_sector *hint) const
{
    if (hint && !(hint->sct_flags & SCTF_PRIMARY) && hint->CheckPoint(pos))
        return hint;
    I3D_sector *sct = sectors;
    for (int i = num_sectors; i--; sct = sct->next_in_scene) {
        if (sct->CheckPoint(pos))
            return sct;
    }
    return primary_sector;
}

// FUNCTION: LS3DF 0x1004a240
I3D_sector *I3D_scene::GetSector(const S_vector &pos, I3D_sector **list, int num) const
{
    while (num--) {
        if (list[num]->CheckPoint(pos))
            return list[num];
    }
    return primary_sector;
}

// Links the frame to the sector at pos, unless it is in the backdrop or omni sector
// FUNCTION: LS3DF 0x1004a280
LS3D_RESULT I3D_scene::SetFrameSectorPos(I3D_frame *frame, const S_vector &pos) const
{
    I3D_frame *frm;
    for (frm = frame->parent; frm; frm = frm->parent) {
        if (frm->GetType() == FRAME_SECTOR) {
            if (((I3D_sector *)frm)->sct_flags & SCTF_BACKDROP)
                return I3D_OK;
            if (((I3D_sector *)frm)->sct_flags & SCTF_OMNI)
                return I3D_OK;
            break;
        }
    }
    I3D_sector *sct = GetSector(pos, NULL);
    if (frm == sct)
        return I3D_OK;
    return frame->LinkTo(sct, I3DLINK_UPDMATRIX);
}

// FUNCTION: LS3DF 0x1004a300
LS3D_RESULT I3D_scene::Open(const char *file_name, uint flags, void *cb, void *context)
{
    Close();
    if (!file_name)
        return I3D_OK;
    LS3D_RESULT ret = LoadHierarchy(file_name, flags, cb, context, primary_sector);
    if (current_camera)
        current_camera->Update();
    MakeReadySectors();
    return ret;
}

// FUNCTION: LS3DF 0x1004a390
void I3D_scene::Close()
{
    UnloadManagedTextures();
    primary_sector->unknown_130 = 0;
    backdrop_sector->unknown_130 = 0;
    omni_sector->unknown_130 = 0;
    near_sector->unknown_130 = 0;
    CurrentActiveAreaList.clear();
    for (int i = primary_sector->sounds.size(); i--; )
        primary_sector->DeleteSound(primary_sector->sounds[0]);
    for (int j = list_288.size(); j--; )
        list_288[j]->Release();
    list_288.clear();
    pI3DDriver->sectors.clear();
    tick_frames.clear();
    delete ipol_set;
    ipol_set = NULL;
    I3D_frame **p = &owned_frames[0];
    for (int k = owned_frames.size(); k--; p++) {
        (*p)->owner = NULL;
        (*p)->Release();
    }
    owned_frames.reset();
    if (current_camera)
        current_camera->sector = NULL;
    pCurrentCamera = NULL;
    primary_sector->fog_light = NULL;
    backdrop_sector->fog_light = NULL;
    pLastCameraSector = NULL;
    pPrevLastCameraSector = NULL;
    primary_sector->sct_flags |= SCTF_SOUND_CHANGED;
    primary_sector->unknown_1e8 = 0;
    primary_sector->unknown_1ec = 0;
    primary_sector->sound_props.SetupDefaults();
    m_pD3DDevice8->ResourceManagerDiscardBytes(0);
    cVBMan.m_FreeAllUnusedVBuffers(NULL);
    cIBMan.m_FreeAllUnusedIBuffers();
    I3D_shadow **s = &shadows[0];
    for (int l = shadows.size(); l--; s++)
        (*s)->scene = NULL;
    shadows.reset();
}

// FUNCTION: LS3DF 0x1004a5a0
void I3D_scene::AddShadow(I3D_shadow *shadow)
{
    if (shadow->scene == this)
        return;
    shadows.push_back(shadow);
    shadow->scene = this;
}

// FUNCTION: LS3DF 0x1004a640
LS3D_RESULT I3D_scene::DeleteShadow(I3D_shadow *shadow)
{
    if (shadows.remove(shadow)) {
        shadow->scene = NULL;
        return I3D_OK;
    }
    dbgPrintf("!! frame not found in the list  pS:0x%x   I3D_scene::DeleteShadow(", shadow);
    return I3DERR_OBJECTNOTFOUND;
}

// FUNCTION: LS3DF 0x1004a6f0
void I3D_scene::AddMirror(I3D_mirror *mirror)
{
    if (mirror->scene == this)
        return;
    mirrors.push_back(mirror);
    mirror->scene = this;
}

// FUNCTION: LS3DF 0x1004a790
LS3D_RESULT I3D_scene::DeleteMirror(I3D_mirror *mirror)
{
    if (mirrors.remove(mirror)) {
        mirror->scene = NULL;
        return I3D_OK;
    }
    dbgPrintf("!! frame not found in the list  pM:0x%x   I3D_scene::DeleteMirror(", mirror);
    return I3DERR_OBJECTNOTFOUND;
}

// FUNCTION: LS3DF 0x1004a840
void I3D_scene::SetBackdropRange(float near1, float far1)
{
    backdrop_ncp = near1;
    backdrop_fcp = far1;
}

// FUNCTION: LS3DF 0x1004a880
void I3D_scene::MakeReadySectors()
{
    struct EnumHelp {
        // FUNCTION: LS3DF 0x1004a8c0
        static I3DENUMRET __stdcall I3D_EnumProc(I3D_frame *frm, uint context)
        {
            ((I3D_sector *)frm)->MakeReady();
            return I3DENUMRET_OK;
        }
    };
    EnumFrames(EnumHelp::I3D_EnumProc, 0, ENUMF_SECTOR, NULL);
    int i = num_portals;
    I3D_portal *portal = portals;
    while (i--) {
        portal->UpdatePortal();
        portal = portal->next_in_scene;
    }
}

static void sceneSS(I3D_frame *frm);

// FUNCTION: LS3DF 0x1004a8d0
void I3D_scene::SetSpace(bool world)
{
    SetSpaceParam = world;
    sceneSS(primary_sector);
}

// The objects and lightmapped objects of the hierarchy, not into models and volumes
// FUNCTION: LS3DF 0x1004a950
static void sceneSS(I3D_frame *frm)
{
    if (frm->frm_flags & (FRMFLAGS_TARGET | 0x10000000))
        return;
    I3D_FRAME_TYPE type = frm->GetType();
    if (type == FRAME_VOLUME || type == FRAME_MODEL)
        return;
    if ((type == FRAME_VISUAL && ((I3D_visual *)frm)->visual_type == I3D_VISUAL_OBJECT) ||
        ((I3D_visual *)frm)->visual_type == I3D_VISUAL_LIT_OBJECT)
        ((I3D_visual *)frm)->SetSpace(SetSpaceParam);
    for (I3D_frame *f = frm->child; f; f = f->next)
        sceneSS(f);
}

// FUNCTION: LS3DF 0x1004a9c0
LS3D_RESULT I3D_scene::SetViewport(uint x, uint y, uint sx, uint sy)
{
    viewport.left = x;
    viewport.top = y;
    viewport.right = sx;
    viewport.bottom = sy;
    viewport_size_half[0] = (sx - x) * .5f;
    viewport_size_half[1] = -(viewport.bottom - viewport.top) * .5f;
    viewport_center[0] = viewport.left + viewport_size_half[0];
    viewport_center[1] = viewport.top - viewport_size_half[1];
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1004aa50
void I3D_scene::SetViewProjMatrix(const S_matrix &m)
{
    m_view_proj_screen = m;
    for (int i = 0; i < 4; i++) {
        m_view_proj_screen.m[i][0] = viewport_size_half[0] * m.m[i][0];
        m_view_proj_screen.m[i][1] = m.m[i][1] * viewport_size_half[1];
    }
}

// Gives the listener the sound environment of the sector, blended from the previous sector's, as EAX 3.0 listener
// properties: at most every 100 ms while blending
// FUNCTION: LS3DF 0x1004aaa0
void I3D_scene::m_UpdateSoundProperties(I3D_sector *sector, I3D_sector *prev_sector, float blend)
{
    if (!listener)
        return;
    const S_sector_sound_properties &sp = sector->sound_props;
    bool done = false;
    S_sector_sound_properties props;
    if (IsAbsMrgZero(1.0 - blend)) {
        done = true;
        props = sp;
    } else if (!prev_sector)
        props.MakeBlend(InitialSoundProp, sp, blend, true);
    else
        props.MakeBlend(prev_sector->sound_props, sp, blend, true);
    if (pISndDriver->create_flags & ISNDINIT_EAX30) {
        static uint last_time = timeGetTime();
        uint time = timeGetTime();
        if (done || time - last_time > 100) {
            EAXLISTENERPROPERTIES p;
            p.ulEnvironment = EAX_ENVIRONMENT_UNDEFINED;
            p.ulFlags = EAXLISTENERFLAGS_DEFAULT;
            p.flEnvironmentSize = 7.5f;
            p.lRoomLF = (long)(props.room_lf * 10000.0f - 10000.0f);
            if (p.lRoomLF < -10000)
                p.lRoomLF = -10000;
            else if (p.lRoomLF > 0)
                p.lRoomLF = 0;
            p.lRoomHF = (long)(props.room_hf * 10000.0f - 10000.0f);
            if (p.lRoomHF < -10000)
                p.lRoomHF = -10000;
            else if (p.lRoomHF > 0)
                p.lRoomHF = 0;
            p.flDecayTime = props.decay_time * .001f;
            if (p.flDecayTime < .1f)
                p.flDecayTime = .1f;
            else if (p.flDecayTime > 20.0f)
                p.flDecayTime = 20.0f;
            p.flDecayHFRatio = props.decay_hf_ratio * 1.9f + .1f;
            if (p.flDecayHFRatio < .1f)
                p.flDecayHFRatio = .1f;
            else if (p.flDecayHFRatio > 2.0f)
                p.flDecayHFRatio = 2.0f;
            p.lReflections = (long)(props.reflections * 11000.0f - 10000.0f);
            if (p.lReflections < -10000)
                p.lReflections = -10000;
            else if (p.lReflections > 1000)
                p.lReflections = 1000;
            p.flReflectionsDelay = props.reflections_delay * .001f;
            if (p.flReflectionsDelay < 0.0f)
                p.flReflectionsDelay = 0.0f;
            else if (p.flReflectionsDelay > .3f)
                p.flReflectionsDelay = .3f;
            p.lReverb = (long)(props.reverb * 12000.0f - 10000.0f);
            if (p.lReverb < -10000)
                p.lReverb = -10000;
            else if (p.lReverb > 2000)
                p.lReverb = 2000;
            p.flReverbDelay = props.reverb_delay * .001f;
            if (p.flReverbDelay < 0.0f)
                p.flReverbDelay = 0.0f;
            else if (p.flReverbDelay > .1f)
                p.flReverbDelay = .1f;
            p.flEnvironmentDiffusion = props.environment_diffusion;
            if (p.flEnvironmentDiffusion < 0.0f)
                p.flEnvironmentDiffusion = 0.0f;
            else if (p.flEnvironmentDiffusion > 1.0f)
                p.flEnvironmentDiffusion = 1.0f;
            p.flDecayLFRatio = props.decay_lf_ratio * 1.9f + .1f;
            if (p.flDecayLFRatio < .1f)
                p.flDecayLFRatio = .1f;
            else if (p.flDecayLFRatio > 2.0f)
                p.flDecayLFRatio = 2.0f;
            p.lRoom = (long)(props.room * 10000.0f - 10000.0f);
            if (p.lRoom < -10000)
                p.lRoom = -10000;
            else if (p.lRoom > 0)
                p.lRoom = 0;
            p.flEchoTime = props.echo_time * .001f;
            if (p.flEchoTime < .075f)
                p.flEchoTime = .075f;
            else if (p.flEchoTime > .25f)
                p.flEchoTime = .25f;
            p.flEchoDepth = props.echo_depth;
            if (p.flEchoDepth < 0.0f)
                p.flEchoDepth = 0.0f;
            else if (p.flEchoDepth > 1.0f)
                p.flEchoDepth = 1.0f;
            p.flModulationTime = props.modulation_time * .001f;
            if (p.flModulationTime < .04f)
                p.flModulationTime = .04f;
            else if (p.flModulationTime > 4.0f)
                p.flModulationTime = 4.0f;
            p.flModulationDepth = props.modulation_depth;
            if (p.flModulationDepth < 0.0f)
                p.flModulationDepth = 0.0f;
            else if (p.flModulationDepth > 1.0f)
                p.flModulationDepth = 1.0f;
            p.flRoomRolloffFactor = props.room_rolloff_factor * 10.0f;
            if (p.flRoomRolloffFactor < 0.0f)
                p.flRoomRolloffFactor = 0.0f;
            else if (p.flRoomRolloffFactor > 10.0f)
                p.flRoomRolloffFactor = 10.0f;
            p.flAirAbsorptionHF = props.air_absorption_hf * 100.0f - 100.0f;
            if (p.flAirAbsorptionHF < -100.0f)
                p.flAirAbsorptionHF = -100.0f;
            else if (p.flAirAbsorptionHF > 0.0f)
                p.flAirAbsorptionHF = 0.0f;
            p.flHFReference = props.hf_reference * 19000.0f + 1000.0f;
            if (p.flHFReference < 1000.0f)
                p.flHFReference = 1000.0f;
            else if (p.flHFReference > 20000.0f)
                p.flHFReference = 20000.0f;
            p.flLFReference = props.lf_reference * 980.0f + 20.0f;
            if (p.flLFReference < 20.0f)
                p.flLFReference = 20.0f;
            else if (p.flLFReference > 1000.0f)
                p.flLFReference = 1000.0f;
            p.vReflectionsPan.x = props.reflections_pan.x;
            p.vReflectionsPan.y = props.reflections_pan.y;
            p.vReflectionsPan.z = props.reflections_pan.z;
            p.vReverbPan.x = props.reverb_pan.x;
            p.vReverbPan.y = props.reverb_pan.y;
            p.vReverbPan.z = props.reverb_pan.z;
            listener->SetEAX3Property(DSPROPERTY_EAXLISTENER_ALLPARAMETERS, &p, sizeof(p));
            last_time = time;
        }
    }
    if (prev_sector)
        prev_sector->sct_flags &= ~SCTF_SOUND_CHANGED;
    sector->sct_flags &= ~SCTF_SOUND_CHANGED;
}

// FUNCTION: LS3DF 0x1004b0f0
LS3D_RESULT I3D_scene::SetAnimation(I3D_animation_set *set, uint index, float blend, uint options)
{
    if (index >= 8)
        return I3DERR_INVALIDPARAMS;
    if (!ipol_set) {
        ipol_set = new I3D_IPolSet(this);
        if (!ipol_set) {
            dbgPrintf("!! Out of memory  I3D_scene::SetAnimation()");
            return I3DERR_OUTOFMEM;
        }
    }
    return ipol_set->SetAnimation(set, index, blend, options);
}

// FUNCTION: LS3DF 0x1004b1b0
I3D_animation_set *I3D_scene::GetAnimationSet(uint index) const
{
    if (index >= 8)
        return NULL;
    if (!ipol_set)
        return NULL;
    return ipol_set->anims[index].set;
}

// FUNCTION: LS3DF 0x1004b1e0
void I3D_scene::StopAnimation(uint index)
{
    if (index >= 8)
        return;
    if (!ipol_set)
        return;
    ipol_set->anims[index].on = false;
}

// FUNCTION: LS3DF 0x1004b210
LS3D_RESULT I3D_scene::SetAnimBlend(uint index, float blend)
{
    if (index >= 8)
        return I3DERR_INVALIDPARAMS;
    I3D_IPolSet *ips = ipol_set;
    if (!ips)
        return I3DERR_OBJECTNOTFOUND;
    float b = blend;
    if (blend < 0.0f)
        b = 0.0f;
    else if (blend > 1.0f)
        b = 1.0f;
    ips->anims[index].blend = b;
    ips->changed = true;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1004b290
float I3D_scene::GetAnimBlend(uint index) const
{
    if (index >= 8)
        return 0.0f;
    if (!ipol_set)
        return 0.0f;
    return ipol_set->anims[index].blend;
}

// FUNCTION: LS3DF 0x1004b2d0
LS3D_RESULT I3D_scene::SetAnimTime(uint index, int time)
{
    if (index >= 8)
        return I3DERR_INVALIDPARAMS;
    I3D_IPolSet *ips = ipol_set;
    if (!ips)
        return I3DERR_OBJECTNOTFOUND;
    ips->anims[index].time = time;
    ips->anims[index].last_time = time;
    ips->changed = true;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1004b310
int I3D_scene::GetAnimTime(uint index) const
{
    if (index >= 8)
        return 0;
    if (!ipol_set)
        return 0;
    return ipol_set->anims[index].time;
}

// FUNCTION: LS3DF 0x1004b340
bool I3D_scene::AnimIsOn(uint index) const
{
    if (index >= 8)
        return false;
    if (!ipol_set)
        return false;
    return ipol_set->anims[index].on;
}

// FUNCTION: LS3DF 0x1004b370
int I3D_scene::NumInterpolators() const
{
    if (!ipol_set)
        return 0;
    return ipol_set->interpolators.size();
}

// FUNCTION: LS3DF 0x1004b390
I3D_interpolator **I3D_scene::GetInterpolators() const
{
    if (!ipol_set)
        return NULL;
    return &ipol_set->interpolators[0];
}

// Rain, or snow with WSF_SNOW; the other flags but WSF_ON are reset
// FUNCTION: LS3DF 0x1004b3b0
void I3D_scene::WeatherSystemReset()
{
    uint on = ws_flags & WSF_ON;
    if (ws_flags & WSF_SNOW) {
        ws_flags = WSF_SNOW;
        ws_density = .002f;
        ws_size = .3f;
        ws_speed = .15f;
    } else {
        ws_flags = 0;
        ws_density = .025f;
        ws_size = 1.5f;
        ws_speed = .03f;
    }
    ws_flags |= on | WSF_FLAG_2 | WSF_FLAG_4;
    ws_range_x = 25.0f;
    ws_range_y = 12.0f;
    ws_count = 1000;
    ws_unknown_2c4 = 3000;
    ws_wind = S_vector(0.0f, -1.0f, 0.0f) * RotateZMatrix(.2f);
    ws_color = 0x80606060;
    ws_color_2 = 0x606060;
    ws_dir = ws_wind;
    ws_matrix.SetDir(ws_dir);
    ws_range = (float)sqrt(ws_range_x * ws_range_x + ws_range_y * ws_range_y);
    ws_elements.reset();
}

// The dummy's box in world space: its center and radius, three of its faces as planes and its edges' lengths. A free
// function despite its name, as PS2's signature has it (m_PrepareDummyStruct__FP12DUMMY_STRUCTP9I3D_dummy). Each face
// is made in one plane local and copied, as in the original (Insanity3D's S_plane::ComputePlane has the same three
// steps). Open: the original inlines Normalize all three times but calls GetNormal and Dot, which no amount of source
// gives with them inline (VC6 takes GetNormal before Normalize, and Dot whenever Normalize fits); a forced inline
// Normalize does, but costs S_quat::Make and the camera. The original has no out-of-line copy of Normalize anywhere;
// ours comes from camera code that doesn't inline it.
// FUNCTION: LS3DF 0x1004b510
void m_PrepareDummyStruct(DUMMY_STRUCT *ds, I3D_dummy *dummy)
{
    S_matrix m = dummy->GetMatrix();
    ds->dummy = dummy;
    ds->update_count = dummy->update_count;
    const I3D_bbox &bb = dummy->bbox;
    S_vector v_max = bb.max * m;
    S_vector v_min = bb.min * m;
    S_vector v1, v2, v3;
    v1 = S_vector(bb.min.x, bb.max.y, bb.min.z) * m;
    v2 = S_vector(bb.max.x, bb.max.y, bb.min.z) * m;
    v3 = S_vector(bb.max.x, bb.min.y, bb.min.z) * m;
    S_vector half = (v_max - v_min) * .5f;
    ds->center = v_min + half;
    ds->radius_2 = half.x * half.x + half.y * half.y + half.z * half.z;
    S_plane pl;
    pl.normal.GetNormal(v3, v2, v1);
    pl.normal.Normalize();
    pl.d = -pl.normal.Dot(v3);
    ds->planes[0] = pl;
    pl.normal.GetNormal(v3, v_max, v2);
    pl.normal.Normalize();
    pl.d = -pl.normal.Dot(v3);
    ds->planes[1] = pl;
    pl.normal.GetNormal(v2, v_max, v1);
    pl.normal.Normalize();
    pl.d = -pl.normal.Dot(v2);
    ds->planes[2] = pl;
    ds->size[0] = (v2 - v_max).Magnitude();
    ds->size[1] = (v1 - v2).Magnitude();
    ds->size[2] = (v2 - v3).Magnitude();
}

// FUNCTION: LS3DF 0x1004bbc0
void I3D_scene::WeatherSystemAddDummy(I3D_dummy *dummy)
{
    if (dummy->frm_flags & FRMFLAGS_WEATHER)
        return;
    for (int i = ws_dummies.size(); i--; ) {
        if (ws_dummies[i].dummy == dummy)
            return;
    }
    DUMMY_STRUCT ds;
    m_PrepareDummyStruct(&ds, dummy);
    ws_dummies.push_back(ds);
    dummy->frm_flags |= FRMFLAGS_WEATHER;
}

// FUNCTION: LS3DF 0x1004bc30
void I3D_scene::WeatherSystemDelDummy(I3D_dummy *dummy)
{
    for (int i = ws_dummies.size(); i--; ) {
        if (ws_dummies[i].dummy == dummy) {
            ws_dummies.erase(i);
            break;
        }
    }
    dummy->frm_flags &= ~FRMFLAGS_WEATHER;
}

// FUNCTION: LS3DF 0x1004bcc0
int I3D_scene::WeatherSystemGetDummyCnt()
{
    return ws_dummies.size();
}

// FUNCTION: LS3DF 0x1004bcd0
DUMMY_STRUCT *I3D_scene::WeatherSystemGetDumies()
{
    return &ws_dummies[0];
}

// The floats come as their bits. A new color is written into the particles' vertices (doubled where the color op can't
// modulate by 2), a new size, speed or kind gives every particle its shape again.
// Not matching yet: this sits in another register (the original's esi is our edi, its edi our esi).
// FUNCTION: LS3DF 0x1004bce0
void I3D_scene::SetWeatherSystemParam(WS_PARAM param, uint value)
{
    int update;
    switch (param) {
    case WS_PARAM_ON:
        if (value)
            ws_flags |= WSF_ON;
        else
            ws_flags &= ~WSF_ON;
        return;
    case WS_PARAM_FLAG_2:
        if (value)
            ws_flags |= WSF_FLAG_2;
        else
            ws_flags &= ~WSF_FLAG_2;
        return;
    case WS_PARAM_FLAG_4:
        if (value)
            ws_flags |= WSF_FLAG_4;
        else
            ws_flags &= ~WSF_FLAG_4;
        return;
    case WS_PARAM_COLOR:
        update = 1;
        ws_color = value;
        break;
    case WS_PARAM_COLOR_2:
        update = 1;
        ws_color_2 = value;
        break;
    case WS_PARAM_DENSITY:
        ws_density = I3DIntAsFloat(value) * .001f;
        return;
    case WS_PARAM_SIZE:
        ws_size = I3DIntAsFloat(value);
        update = 2;
        break;
    case WS_PARAM_RANGE_X:
        ws_range_x = I3DIntAsFloat(value);
        ws_range = (float)sqrt(ws_range_x * ws_range_x + ws_range_y * ws_range_y);
        return;
    case WS_PARAM_RANGE_Y:
        ws_range_y = I3DIntAsFloat(value);
        ws_range = (float)sqrt(ws_range_x * ws_range_x + ws_range_y * ws_range_y);
        return;
    case WS_PARAM_SPEED:
        ws_speed = I3DIntAsFloat(value) * .5f;
        update = 2;
        break;
    case WS_PARAM_COUNT:
        ws_count = value;
        if (MaxWECnt < ws_count) {
            MaxWECnt = ws_count;
            cVBMan.m_Free(WSComplVBStr);
            memset(&WSComplVBStr, 0, sizeof(WSComplVBStr));
        }
        if (ws_elements.size() > ws_count)
            ws_elements.used_size = ws_count;
        return;
    case WS_PARAM_WIND_X:
        ws_wind.x = I3DIntAsFloat(value);
        return;
    case WS_PARAM_WIND_Y:
        ws_wind.y = I3DIntAsFloat(value);
        return;
    case WS_PARAM_WIND_Z:
        ws_wind.z = I3DIntAsFloat(value);
        return;
    case WS_PARAM_SNOW:
        if (value == 1)
            ws_flags |= WSF_SNOW;
        else
            ws_flags &= ~WSF_SNOW;
        update = 2;
        break;
    case WS_PARAM_MODE:
        if (value == 2)
            ws_flags |= WSF_MODE_2;
        else if (value == 1)
            ws_flags |= WSF_MODE_1;
        else
            ws_flags &= ~(WSF_MODE_1 | WSF_MODE_2);
        return;
    default:
        return;
    }
    switch (update) {
    case 2: {
        WE_STRUCT *e = &ws_elements[0];
        for (int i = ws_elements.size(); i--; e++) {
            S_matrix m = RotateZMatrix(e->angle);
            e->v[1] = S_vector(-ws_speed, 0.0f, 0.0f) * m;
            e->v[2] = S_vector(ws_speed, 0.0f, 0.0f) * m;
            if (ws_flags & WSF_SNOW) {
                e->v[0] = S_vector(0.0f, 0.0f, -ws_size / 2.0f) * m;
                e->v[3] = S_vector(0.0f, 0.0f, ws_size / 2.0f) * m;
            } else {
                e->v[0] = S_vector(0.0f, 0.0f, -ws_size) * m;
                e->v[3] = S_vector(0.0f, 0.0f, ws_size / 6.0f) * m;
            }
        }
    } break;
    case 1:
        if (WSComplVBStr.vb) {
            uint color, color_2;
            if (!(m_ProblemFlags & 4)) {
                color = ws_color;
                color_2 = ws_color_2;
            } else {
                uint r = (ws_color >> 15) & 0x1fe;
                if (r > 0xff)
                    r = 0xff;
                uint g = (ws_color >> 7) & 0x1fe;
                if (g > 0xff)
                    g = 0xff;
                uint b = (ws_color & 0xff) << 1;
                if (b > 0xff)
                    b = 0xff;
                color = (((r << 8) | g) << 8) | b | (ws_color & 0xff000000);
                r = (ws_color_2 >> 15) & 0x1fe;
                if (r > 0xff)
                    r = 0xff;
                g = (ws_color_2 >> 7) & 0x1fe;
                if (g > 0xff)
                    g = 0xff;
                b = (ws_color_2 & 0xff) << 1;
                if (b > 0xff)
                    b = 0xff;
                color_2 = (((r << 8) | g) << 8) | b | (ws_color_2 & 0xff000000);
            }
            WS_VERTEX *v = (WS_VERTEX *)WSComplVBStr.buffer->Lock(WSComplVBStr.base, 0);
            for (uint i = ws_count; i--; v += 12) {
                v[0].color = color_2;
                v[1].color = color_2;
                v[2].color = color;
                v[3].color = color_2;
                v[4].color = color_2;
                v[5].color = color;
                v[6].color = color_2;
                v[7].color = color_2;
                v[8].color = color;
                v[9].color = color_2;
                v[10].color = color_2;
                v[11].color = color;
            }
            WSLastScene = this;
        }
        break;
    }
}

// The floats are returned as their bits
// FUNCTION: LS3DF 0x1004c4d0
uint I3D_scene::GetWeatherSystemParam(WS_PARAM param)
{
    switch (param) {
    case WS_PARAM_ON:
        return ws_flags & WSF_ON;
    case WS_PARAM_FLAG_2:
        return ws_flags & WSF_FLAG_2;
    case WS_PARAM_FLAG_4:
        return ws_flags & WSF_FLAG_4;
    case WS_PARAM_COLOR:
        return ws_color;
    case WS_PARAM_COLOR_2:
        return ws_color_2;
    case WS_PARAM_DENSITY:
        return I3DFloatAsInt(ws_density * 1000.0f);
    case WS_PARAM_SIZE:
        return I3DFloatAsInt(ws_size);
    case WS_PARAM_RANGE_X:
        return I3DFloatAsInt(ws_range_x);
    case WS_PARAM_RANGE_Y:
        return I3DFloatAsInt(ws_range_y);
    case WS_PARAM_SPEED:
        return I3DFloatAsInt(ws_speed * 2.0f);
    case WS_PARAM_COUNT:
        return ws_count;
    case WS_PARAM_WIND_X:
        return I3DFloatAsInt(ws_wind.x);
    case WS_PARAM_WIND_Y:
        return I3DFloatAsInt(ws_wind.y);
    case WS_PARAM_WIND_Z:
        return I3DFloatAsInt(ws_wind.z);
    case WS_PARAM_SNOW:
        return (ws_flags & WSF_SNOW) != 0;
    case WS_PARAM_MODE:
        if (ws_flags & WSF_MODE_1)
            return 1;
        if (ws_flags & WSF_MODE_2)
            return 2;
        return 0;
    }
    return (uint)-1;
}

// The particles in a box around the camera: they fall with the wind (rain, turned by the camera's movement) or sway
// (snow), come back at a random place in the box when they leave it, and are written into the vertex buffer where
// they are in view, outside the near sectors and the dummies' regions. Spellings that matched: the snow's rotation
// through a matrix local, the loops' bounds (for (int i = ws_count; i > num; i--)), the alpha masked before the
// doubled color, the time converted inside the particle loop (VC6 hoists it), the sway's double factors, the regions'
// f < 0.0f || f > size and d.Square() >= range * range.
// Open: the rain's blend computes ws_dir * (1 - t) first in the original, with both factors on the FPU stack, which
// dir * t + ws_dir * (1 - t) gives (and PS2's call order agrees), but that form moves most of the frame's slots; the
// vertex buffer lock's early return computes the vertex's address apart (the index in ecx); the corners' pushes are
// scheduled differently.
// FUNCTION: LS3DF 0x1004c660
void I3D_scene::m_UpdateWeatherSystem()
{
    if (!(ws_flags & WSF_ON) || !(primary_sector->sct_flags & 1) || !ws_count) {
        CurrentWECnt = 0;
        return;
    }
    static int LastUpdateTime = driverTickTime;
    uint time = driverTickTime - LastUpdateTime;
    LastUpdateTime = driverTickTime;
    CurrentWECnt = 0;
    float cos_fov = (float)cos((float)sqrt(pCurrentCamera->aspect_ratio * pCurrentCamera->aspect_ratio + 1.0f) *
        pCurrentCamera->fov_a * .5f);
    S_vector cam_pos = pCurrentCamera->GetMatrix()(3);
    S_vector cam_dir = pCurrentCamera->GetMatrix()(2);
    cam_dir.Normalize();
    static S_vector CamOldPos = pCurrentCamera->GetMatrix()(3);
    S_vector cam_move = cam_pos - CamOldPos;
    S_vector move = CamOldPos - cam_pos;
    move.y = 0.0f;
    cam_move.Normalize();
    CamOldPos = cam_pos;
    if (ws_flags & WSF_SNOW) {
        S_matrix m = RotateYMatrix(driverTickTime * .0007f);
        ws_dir = ws_wind * m;
    } else {
        S_vector dir;
        if (!IsMrgZeroLess(move.Square())) {
            dir = ws_wind * (float)time * ws_density + move;
            dir.Normalize();
        } else
            dir = ws_wind;
        if (!IsMrgZeroLess((ws_dir - dir).Square())) {
            float t = time * .01f;
            if (1.0f < t)
                t = 1.0f;
            ws_dir = ws_dir * (1.0f - t) + dir * t;
        }
    }
    S_vector tail = -ws_dir * ws_size;
    ws_matrix.SetDir(ws_dir);
    if (!WSComplVBStr.vb || ws_count > MaxWECnt) {
        MaxWECnt = ws_count;
        if (WSComplVBStr.vb) {
            cVBMan.m_Free(WSComplVBStr);
            memset(&WSComplVBStr, 0, sizeof(WSComplVBStr));
        }
        if (!cVBMan.m_Alloc(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DUSAGE_WRITEONLY, sizeof(WS_VERTEX),
                ws_count * 12, WSComplVBStr, VF_LVERTEX, false)) {
            dbgPrintf("WeatherSystem ERROR !!! VB creating !");
            return;
        }
        WSLastScene = NULL;
    }
    int num = ws_elements.size();
    if (num < ws_count) {
        ws_elements.reserve(ws_count);
        for (int i = ws_count; i > num; i--) {
            WE_STRUCT e;
            e.pos = cam_pos - S_vector(ws_range_x, ws_range_y, ws_range_x);
            e.pos.x += RandFloat() * ws_range_x * 2.0f;
            e.pos.y += RandFloat() * ws_range_y * 2.0f;
            e.pos.z += RandFloat() * ws_range_x * 2.0f;
            e.min_y = cam_pos.y - ws_range_y;
            e.angle = RandFloat() * PI;
            S_matrix m = RotateZMatrix(e.angle);
            e.v[1] = S_vector(-ws_speed, 0.0f, 0.0f) * m;
            e.v[2] = S_vector(ws_speed, 0.0f, 0.0f) * m;
            if (ws_flags & WSF_SNOW) {
                e.v[0] = S_vector(0.0f, 0.0f, -ws_size / 2.0f) * m;
                e.v[3] = S_vector(0.0f, 0.0f, ws_size / 2.0f) * m;
            } else {
                e.v[0] = S_vector(0.0f, 0.0f, -ws_size) * m;
                e.v[3] = S_vector(0.0f, 0.0f, ws_size / 6.0f) * m;
            }
            e.phase = (uint)(RandFloat() * 4294967295.0f);
            ws_elements.push_back(e);
        }
    }
    WS_VERTEX *v = (WS_VERTEX *)WSComplVBStr.buffer->Lock(WSComplVBStr.base, 0);
    if (WSLastScene != this) {
        uint color, color_2;
        if (m_ProblemFlags & 4) {
            uint a = ws_color & 0xff000000;
            uint r = (ws_color >> 15) & 0x1fe;
            if (r > 0xff)
                r = 0xff;
            uint g = (ws_color >> 7) & 0x1fe;
            if (g > 0xff)
                g = 0xff;
            uint b = (ws_color & 0xff) << 1;
            if (b > 0xff)
                b = 0xff;
            color = (((r << 8) | g) << 8) | b | a;
            a = ws_color_2 & 0xff000000;
            r = (ws_color_2 >> 15) & 0x1fe;
            if (r > 0xff)
                r = 0xff;
            g = (ws_color_2 >> 7) & 0x1fe;
            if (g > 0xff)
                g = 0xff;
            b = (ws_color_2 & 0xff) << 1;
            if (b > 0xff)
                b = 0xff;
            color_2 = (((r << 8) | g) << 8) | b | a;
        } else {
            color = ws_color;
            color_2 = ws_color_2;
        }
        WS_VERTEX *p = v;
        for (uint i = ws_count; i--; p += 12) {
            p[0].color = color_2;
            p[1].color = color_2;
            p[2].color = color;
            p[3].color = color_2;
            p[4].color = color_2;
            p[5].color = color;
            p[6].color = color_2;
            p[7].color = color_2;
            p[8].color = color;
            p[9].color = color_2;
            p[10].color = color_2;
            p[11].color = color;
            p[0].u = 0.0f;
            p[0].v = 0.0f;
            p[1].u = 1.0f;
            p[1].v = 0.0f;
            p[2].u = .5f;
            p[2].v = .5f;
            p[3].u = 1.0f;
            p[3].v = 0.0f;
            p[4].u = 1.0f;
            p[4].v = 1.0f;
            p[5].u = .5f;
            p[5].v = .5f;
            p[6].u = 1.0f;
            p[6].v = 1.0f;
            p[7].u = 0.0f;
            p[7].v = 1.0f;
            p[8].u = .5f;
            p[8].v = .5f;
            p[9].u = 0.0f;
            p[9].v = 1.0f;
            p[10].u = 0.0f;
            p[10].v = 0.0f;
            p[11].u = .5f;
            p[11].v = .5f;
        }
        WSLastScene = this;
    }
    float density = ws_density;
    WSNearSectorList.reset();
    if (ws_flags & WSF_FLAG_2) {
        WSNearSectorList.reserve(num_sectors);
        I3D_sector *sct = sectors;
        for (int i = num_sectors; i--; sct = sct->next_in_scene) {
            if (!(sct->frm_flags & FRMFLAGS_HR_BOUND_W_VALID))
                sct->UpdateHRBoundVolW();
            I3D_bsphere bs = sct->bound_w.bsphere;
            if ((cam_pos - bs.pos).Square() < (bs.radius + ws_range) * (bs.radius + ws_range))
                WSNearSectorList.push_back(sct);
        }
    }
    WE_STRUCT *e = &ws_elements[0];
    for (uint i = ws_count; i--; e++) {
        S_vector pos = e->pos + ws_wind * (float)time * density;
        S_vector d = pos - cam_pos;
        if (pos.y <= e->min_y || d.Square() >= ws_range * ws_range) {
            pos = cam_pos - S_vector(ws_range_x, ws_range_y, ws_range_x);
            pos.x += RandFloat() * ws_range_x * 2.0f;
            pos.y += RandFloat() * ws_range_y * 2.0f;
            pos.z += RandFloat() * ws_range_x * 2.0f;
            e->min_y = cam_pos.y - ws_range_y;
            d = pos - cam_pos;
        }
        if (ws_flags & WSF_SNOW) {
            pos.x -= sin((e->phase + driverTickTime) * .002f) * .004f;
            pos.z += sin((e->phase + driverTickTime + 500) * .002f) * .004f;
        }
        e->pos = pos;
        if (cam_dir.Dot(d) <= 0.0f)
            continue;
        d.Normalize();
        if (cam_dir.Dot(d) <= cos_fov)
            continue;
        S_vector pos2 = pos + tail;
        if (WSNearSectorList.size() > 0) {
            if (GetSector(pos, &WSNearSectorList[0], WSNearSectorList.size()) != primary_sector) {
                e->min_y = 1e6f;
                continue;
            }
            if (GetSector(pos2, &WSNearSectorList[0], WSNearSectorList.size()) != primary_sector) {
                e->min_y = 1e6f;
                continue;
            }
        }
        if (ws_flags & WSF_FLAG_4) {
            int j;
            for (j = ws_dummies.size(); j--; ) {
                DUMMY_STRUCT &ds = ws_dummies[j];
                if (ds.update_count != ds.dummy->update_count)
                    m_PrepareDummyStruct(&ds, ds.dummy);
                if ((ds.center - pos).Square() > ds.radius_2 && (ds.center - pos2).Square() > ds.radius_2)
                    continue;
                float f = ds.planes[0].normal.Dot(pos) + ds.planes[0].d;
                if (!(f < 0.0f || f > ds.size[0])) {
                    f = ds.planes[1].normal.Dot(pos) + ds.planes[1].d;
                    if (!(f < 0.0f || f > ds.size[1])) {
                        f = ds.planes[2].normal.Dot(pos) + ds.planes[2].d;
                        if (!(f < 0.0f || f > ds.size[2]))
                            break;
                    }
                }
                f = ds.planes[0].normal.Dot(pos2) + ds.planes[0].d;
                if (!(f < 0.0f || f > ds.size[0])) {
                    f = ds.planes[1].normal.Dot(pos2) + ds.planes[1].d;
                    if (!(f < 0.0f || f > ds.size[1])) {
                        f = ds.planes[2].normal.Dot(pos2) + ds.planes[2].d;
                        if (!(f < 0.0f || f > ds.size[2]))
                            break;
                    }
                }
            }
            if (j >= 0) {
                e->min_y = 1e6f;
                continue;
            }
        }
        S_vector c0 = e->v[0] * ws_matrix + pos;
        S_vector c1 = e->v[1] * ws_matrix + pos;
        S_vector c2 = e->v[2] * ws_matrix + pos;
        S_vector c3 = e->v[3] * ws_matrix + pos;
        v[0].pos = c1;
        v[1].pos = c0;
        v[2].pos = pos;
        v[3].pos = c0;
        v[4].pos = c2;
        v[5].pos = pos;
        v[6].pos = c2;
        v[7].pos = c3;
        v[8].pos = pos;
        v[9].pos = c3;
        v[10].pos = c1;
        v[11].pos = pos;
        v += 12;
        CurrentWECnt++;
    }
}

inline void SetFog(uint state, uint type, float start, float end, uint color);

// The particles m_UpdateWeatherSystem wrote, in the primary sector's fog, untextured rain or textured snow
// FUNCTION: LS3DF 0x1004ddd0
void I3D_scene::m_RenderWeatherSystem()
{
    if (!CurrentWECnt || !(ws_flags & WSF_ON))
        return;
    I3D_light *fog = primary_sector->fog_light;
    if (fog && renderState[RS_FOG]) {
        switch (fog->ltype) {
        case I3DLIGHT_FOG: {
            float end = pCurrentCamera->fcp * fog->range_f;
            float start = end * fog->range_n;
            SetFog(TRUE, 0, start, end, ((((uint)-1 << 8 | FloatToInt(fog->diffuse.x * 255.0f)) << 8 |
                FloatToInt(fog->diffuse.y * 255.0f)) << 8) | FloatToInt(fog->diffuse.z * 255.0f));
        } break;
        case I3DLIGHT_POINTAMBIENT:
            SetFog(TRUE, 1, fog->near_range, 0.0f, ((((uint)-1 << 8 | FloatToInt(fog->diffuse.x * 255.0f)) << 8 |
                FloatToInt(fog->diffuse.y * 255.0f)) << 8) | FloatToInt(fog->diffuse.z * 255.0f));
            break;
        case I3DLIGHT_POINTFOG:
            SetFog(TRUE, 2, fog->near_range, 0.0f, ((((uint)-1 << 8 | FloatToInt(fog->diffuse.x * 255.0f)) << 8 |
                FloatToInt(fog->diffuse.y * 255.0f)) << 8) | FloatToInt(fog->diffuse.z * 255.0f));
            break;
        }
    }
    SetColorOp(m_Modulate2X);
    SetAlphaOp(D3DTOP_MODULATE);
    SetColorOp1(D3DTOP_DISABLE);
    SetClipping(TRUE);
    SetAlphaState(1);
    SetBlendState(0);
    if (ws_flags & WSF_SNOW && pSnowTexture)
        m_pD3DDevice8->SetTexture(0, pSnowTexture->texture);
    else
        m_pD3DDevice8->SetTexture(0, NULL);
    m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    SetCullState(D3DCULL_NONE);
    pGlobalIGraph->SetWorldMatrix(IdentityMatrix);
    SetVertexFormat(0);
    m_pD3DDevice8->SetStreamSource(0, WSComplVBStr.vb, sizeof(WS_VERTEX));
    m_pD3DDevice8->DrawPrimitive(D3DPT_TRIANGLELIST, WSComplVBStr.base, CurrentWECnt * 4);
    m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

// The fog render states, set where they differ from IGraph's (PC only, the name is a guess): linear from start to end
// (type 0), or exponential (1) or squared (2) with start as the density
// FUNCTION: LS3DF 0x1004e390
inline void SetFog(uint state, uint type, float start, float end, uint color)
{
    if (m_CurrFogState != state) {
        m_CurrFogState = state;
        if (state == TRUE)
            m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, TRUE);
        else {
            m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
            return;
        }
    }
    if (!state)
        return;
    if (m_CurrFogType != type) {
        m_CurrFogType = type;
        switch (type) {
        case 0:
            m_pD3DDevice8->SetRenderState(m_HardwareCaps & 4 ? D3DRS_FOGTABLEMODE : D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            break;
        case 1:
            m_pD3DDevice8->SetRenderState(m_HardwareCaps & 4 ? D3DRS_FOGTABLEMODE : D3DRS_FOGVERTEXMODE, D3DFOG_EXP);
            break;
        case 2:
            m_pD3DDevice8->SetRenderState(m_HardwareCaps & 4 ? D3DRS_FOGTABLEMODE : D3DRS_FOGVERTEXMODE, D3DFOG_EXP2);
            break;
        }
    }
    switch (m_CurrFogType) {
    case 0:
        if (!IsAbsMrgZero(m_CurrFogEnd - end)) {
            m_CurrFogEnd = end;
            m_pD3DDevice8->SetRenderState(D3DRS_FOGEND, I3DFloatAsInt(end));
        }
        if (!IsAbsMrgZero(m_CurrFogStart - start)) {
            m_CurrFogStart = start;
            m_pD3DDevice8->SetRenderState(D3DRS_FOGSTART, I3DFloatAsInt(start));
        }
        break;
    case 1:
    case 2:
        if (!IsAbsMrgZero(m_CurrFogStart - start)) {
            m_CurrFogStart = start;
            m_pD3DDevice8->SetRenderState(D3DRS_FOGDENSITY, I3DFloatAsInt(start));
        }
        break;
    }
    if (m_CurrFogColor != color) {
        m_CurrFogColor = color;
        m_pD3DDevice8->SetRenderState(D3DRS_FOGCOLOR, color);
    }
}

// Whether the frame's bounding sphere is inside the camera's view
// FUNCTION: LS3DF 0x1004e550
bool I3D_scene::TestVisibility(I3D_frame *frame, I3D_camera *cam)
{
    if (!frame || !cam)
        return false;
    const I3D_bvolume *bvol;
    if (frame->GetType() == FRAME_VISUAL)
        bvol = &((I3D_visual *)frame)->GetWBoundVolume();
    else {
        if (!(frame->frm_flags & FRMFLAGS_HR_BOUND_W_VALID))
            frame->UpdateHRBoundVolW();
        bvol = &frame->bound_w;
    }
    const S_vector &pos = bvol->bsphere.pos;
    float r = bvol->bsphere.radius;
    const S_frustum &f = cam->frustums[0];
    float d = f.normal[0].x * pos.x + f.normal[0].y * pos.y + f.normal[0].z * pos.z + f.d[0];
    if (d + r < cam->ncp)
        return false;
    if (d - r > cam->fcp)
        return false;
    r = -r;
    if (f.normal[1].x * pos.x + f.normal[1].y * pos.y + f.normal[1].z * pos.z + f.d[1] < r)
        return false;
    if (f.normal[2].x * pos.x + f.normal[2].y * pos.y + f.normal[2].z * pos.z + f.d[2] < r)
        return false;
    if (f.normal[3].x * pos.x + f.normal[3].y * pos.y + f.normal[3].z * pos.z + f.d[3] < r)
        return false;
    if (f.normal[4].x * pos.x + f.normal[4].y * pos.y + f.normal[4].z * pos.z + f.d[4] < r)
        return false;
    return true;
}

// The functions of other files the scene calls, until those are written. They come after the scene's functions: VC6
// knows that a function compiled earlier in the file can't throw (see I3d_driver.cpp). Their bodies differ, or the
// linker would fold them; auto_inline(off) keeps VC6 from inlining them here. The markers of the ones below the scene's
// addresses are in I3D.h, by name, as the linter checks the order of a file's markers.
#pragma auto_inline(off)

void I3D_portal::UpdatePortal()
{
    renderState[11]++;
}

// STUB: LS3DF 0x100603e0
LS3D_RESULT LoadHierarchy(const char *file_name, uint flags, void *cb, void *context, I3D_frame *root)
{
    return (LS3D_RESULT)renderState[14];
}

#pragma auto_inline(on)
