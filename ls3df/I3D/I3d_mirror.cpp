// The mirrors (PS2 I3d_mirror.cpp), at 0x10031820-0x100333d0 in the original: I3D_mirror from its constructor, with
// the rendering's functions (PC only) among PS2's. The header's zero vector has its $E at the end (0x100333b0), and
// after it the out-of-line copy of SetClipping (render.h). Names are the PS2 build's unless noted.
#include "I3D/I3D.h"
#include "I3D/render.h"
#include "IGraph/IGraph.h"
#include "dta.h"

#include <stddef.h>
#include <string.h>
// <new> declares operator delete throw(), so VC6 sets no EH state before the copies' list's destructor in the mirror's
#include <new>

// The draws' color, as the driver's (I3d_driver.cpp): the alpha byte is a transparency
static inline uint MakeColor(float a, const S_vector &color)
{
    return FloatToInt(a * 255.0f) << 24 | FloatToInt(color.x * 255.0f) << 16 | FloatToInt(color.y * 255.0f) << 8 |
        FloatToInt(color.z * 255.0f);
}

// FUNCTION: LS3DF 0x10031820
I3D_mirror::I3D_mirror()
{
    visual_type = I3D_VISUAL_MIRROR;
    scene = NULL;
    bsphere.radius = 0.0f;
    range = 100.0f;
    view_box.Identity();
    num_verts = 0;
    num_faces = 0;
    verts = NULL;
    faces = NULL;
    mirror_flags = 0;
    color.Zero();
}

// FUNCTION: LS3DF 0x10031910
I3D_mirror::~I3D_mirror()
{
    if (scene)
        scene->DeleteMirror(this);
    delete[] verts;
    delete[] faces;
    I3D_visual **p = &copies[0];
    for (int i = copies.size(); i--; p++) {
        ((I3D_frame *)(*p)->user_data)->Release();
        (*p)->Release();
    }
}

// Each component is clamped to 0 to 1
// FUNCTION: LS3DF 0x100319f0
void I3D_mirror::SetColor(const S_vector &c)
{
    color = c;
    if (color.x < 0.0f)
        color.x = 0.0f;
    else if (color.x > 1.0f)
        color.x = 1.0f;
    if (color.y < 0.0f)
        color.y = 0.0f;
    else if (color.y > 1.0f)
        color.y = 1.0f;
    if (color.z < 0.0f)
        color.z = 0.0f;
    else if (color.z > 1.0f)
        color.z = 1.0f;
}

// The copies are released, but stay in the list
// FUNCTION: LS3DF 0x10031ab0
LS3D_RESULT I3D_mirror::Duplicate(I3D_frame *frame)
{
    if (!frame || frame == this)
        return I3DERR_INVALIDPARAMS;
    I3D_visual **p = &copies[0];
    for (int i = copies.size(); i--; p++) {
        ((I3D_frame *)(*p)->user_data)->Release();
        (*p)->Release();
    }
    delete[] verts;
    verts = NULL;
    num_verts = 0;
    delete[] faces;
    faces = NULL;
    num_faces = 0;
    I3D_frame::Duplicate(frame);
    if (frame->GetType() != FRAME_VISUAL)
        return I3D_OK;
    if (((I3D_visual *)frame)->visual_type != I3D_VISUAL_MIRROR)
        return I3D_OK;
    I3D_mirror *mir = (I3D_mirror *)frame;
    view_box = mir->view_box;
    mirror_flags = (mirror_flags & ~MIRF_PREPARED) | MIRF_VIEW_BOX;
    verts = new S_vector[mir->num_verts];
    num_verts = mir->num_verts;
    memcpy(verts, mir->verts, num_verts * sizeof(S_vector));
    faces = new I3D_triface[mir->num_faces];
    num_faces = mir->num_faces;
    memcpy(faces, mir->faces, num_faces * sizeof(I3D_triface));
    vis_bound = mir->vis_bound;
    vis_flags = (vis_flags & ~VISF_WBOUND_VALID) | VISF_BOUND_VALID;
    range = mir->range;
    color = mir->color;
    return I3D_OK;
}

// The faces in world space, as I3D_driver::DrawTriangles draws (the rendering draws a reflecting mirror nearly
// transparent, the others in their color). The vertex list is reserved and sized, not resized: through resize() VC6
// leaves the nested reserve out of line and inlines SetCullState and SetClipping, the reverse of the original. The cull
// mode is an if/else of two calls, which VC6 merges into one call of a constant chosen by a branch.
// FUNCTION: LS3DF 0x10031c70
void I3D_mirror::Draw(const S_vector &c, uint flags)
{
    if (!verts || !faces)
        return;
    float a = (255 - (flags & I3DDRAW_TRANPARENCY_MASK)) * (1.0f / 255.0f);
    uint col = MakeColor(a, c);
    LVertList.reserve(num_verts);
    LVertList.used_size = num_verts;
    for (int i = 0; i < num_verts; i++) {
        LVertList[i].x = verts[i].x;
        LVertList[i].y = verts[i].y;
        LVertList[i].z = verts[i].z;
        LVertList[i].color = col;
    }
    SetColorOp(D3DTOP_MODULATE);
    SetAlphaOp(D3DTOP_MODULATE);
    if (IsAbsMrgZero(a - 1.0))
        SetAlphaState(0);
    else {
        SetAlphaState(1);
        SetBlendState(0);
    }
    if (flags & I3DDRAW_NOCULL)
        SetCullState(D3DCULL_NONE);
    else if (flags & I3DDRAW_CULL_CW)
        SetCullState(D3DCULL_CW);
    else
        SetCullState(D3DCULL_CCW);
    SetClipping(TRUE);
    pGlobalIGraph->SetWorldMatrix(GetMatrix());
    m_pD3DDevice8->SetTexture(0, NULL);
    pGlobalIGraph->DrawIndexedPrimitiveList(PT_TRIANGLELIST, num_verts, faces, num_faces, &LVertList[0],
        LS3D_STREAM_LVERTEX);
}

// The mirror goes into the scene of the sector its new parent is in. The search is a while (true) whose test of the
// type has an else: a loop over the parents VC6 rotates, putting the test of frm at the bottom.
// FUNCTION: LS3DF 0x10032000
LS3D_RESULT I3D_mirror::LinkTo(I3D_frame *frame, uint flags)
{
    if (scene)
        scene->DeleteMirror(this);
    I3D_frame *frm = frame;
    while (true) {
        if (!frm)
            break;
        if (frm->type != FRAME_SECTOR)
            frm = frm->parent;
        else {
            ((I3D_sector *)frm)->scene->AddMirror(this);
            break;
        }
    }
    LS3D_RESULT result = I3D_frame::LinkTo(frame, flags);
    UpdateV();
    return result;
}

// A move makes the copies take their matrices again
// FUNCTION: LS3DF 0x10032060
void I3D_mirror::UpdateV()
{
    if (frm_flags & (FRMFLAGS_UPDATE | FRMFLAGS_CHANGED)) {
        vis_flags &= ~(VISF_WBOUND_VALID | VISF_UNKNOWN_80000 | VISF_INV_MAT_VALID);
        mirror_flags &= ~MIRF_PREPARED;
        I3D_visual **p = &copies[0];
        for (int i = copies.size(); i--; p++)
            (*p)->update_count = -1;
    }
    if (!scene) {
        I3D_frame *frm = parent;
        while (true) {
            if (!frm)
                break;
            if (frm->type != FRAME_SECTOR)
                frm = frm->parent;
            else {
                ((I3D_sector *)frm)->scene->AddMirror(this);
                break;
            }
        }
    }
}

// The copies of visuals that are gone from the scene (no other reference, or no parent) are released
// FUNCTION: LS3DF 0x100320f0
void I3D_mirror::Tick(int time)
{
    I3D_visual **p = &copies[0];
    for (int i = copies.size(); i--; ) {
        I3D_frame *frm = (I3D_frame *)(*p)->user_data;
        if (frm->ref >= 2 && frm->parent)
            p++;
        else {
            frm->Release();
            (*p)->Release();
            copies.remove(*p);
        }
    }
}

// Before the scene is rendered: whether the camera sees the mirror's front (MIRF_VISIBLE), and whether it is near enough
// to render what it reflects (MIRF_REFLECT), which needs hardware T&L. The distance test is the clearing branch's: the
// other way round (d.Square() < dist * dist && ...) gives another flag test.
// Not matching yet: Normalize's inline budget, as in the light's constructor and I3D_target::UpdateFrames. The original
// inlines GetWorldDir's Normalize, ours calls it. A forced inline Normalize gives the original's code exactly, and so do
// 40 dead statements (if (0) mirror_flags = 0;) at the top, 20 don't (see I3d_math.h).
// FUNCTION: LS3DF 0x100321b0
void I3D_mirror::UpdateVisibility()
{
    if (render_number != RenderNumber) {
        mirror_flags &= ~MIRF_VISIBLE;
        return;
    }
    S_vector dir = GetWorldDir().RotateByMatrix(pCurrentCamera->m_view);
    if (dir.z > 0.0f && dir.z / dir.Magnitude() > cos(pCurrentCamera->fov_a * .5f)) {
        mirror_flags &= ~(MIRF_VISIBLE | MIRF_REFLECT);
        return;
    }
    mirror_flags |= MIRF_VISIBLE;
    float dist = FOVDistCorrection * range;
    S_vector d = GetWorldPos() - pCurrentCamera->GetWorldPos();
    if (d.Square() > dist * dist || !(m_HardwareCaps & 1) || !renderState[RS_DRAWMIRRORS])
        mirror_flags &= ~MIRF_REFLECT;
    else
        mirror_flags |= MIRF_REFLECT;
}

// What PrepareReflection finds: the visuals in the mirror's view box (the names are guesses)
// GLOBAL: LS3DF 0x101b433c
C_list<I3D_visual *> MirroredVisualList;

// GLOBAL: LS3DF 0x101b4358
I3D_mirror *pCurrentMirror;

// Only the frames whose bound sphere touches the view box's are searched, and only objects are taken: those whose box
// isn't wholly outside the view box
// FUNCTION: LS3DF 0x10032500
void I3D_mirror::CollectVisuals(I3D_frame *frm)
{
    if (!(frm->frm_flags & (FRMFLAGS_ON | FRMFLAGS_TICK)))
        return;
    if (!(frm->frm_flags & FRMFLAGS_HR_BOUND_W_VALID))
        frm->UpdateHRBoundVolW();
    S_vector d = frm->bound_w.bsphere.pos - pCurrentMirror->bsphere.pos;
    float r = pCurrentMirror->bsphere.radius + frm->bound_w.bsphere.radius;
    if (d.Square() > r * r)
        return;
    if (frm->type == FRAME_VISUAL) {
        I3D_visual *vis = (I3D_visual *)frm;
        if (!(vis->vis_flags & (VISF_PROXY | VISF_UNKNOWN_20)) && vis->vis_flags & VISF_UNKNOWN_800) {
            if (!(vis->vis_flags & VISF_BOUND_VALID))
                vis->UpdateBoundProc();
            const S_vector *v = BBoxVertices(vis->vis_bound.bbox);
            S_matrix m = vis->GetMatrix() * pCurrentMirror->inv_view_box;
            uint clip = 0;
            for (int i = 8; i--; v++) {
                S_vector p = *v * m;
                if (p.x < 1.0f)
                    clip |= 1;
                if (p.x > -1.0f)
                    clip |= 2;
                if (p.y < 1.0f)
                    clip |= 4;
                if (p.y > -1.0f)
                    clip |= 8;
                if (p.z < 1.0f)
                    clip |= 0x10;
                if (p.z > -1.0f)
                    clip |= 0x20;
            }
            if (clip == 0x3f)
                MirroredVisualList.push_back(vis);
        }
    }
    for (I3D_frame *f = frm->child; f; f = f->next)
        CollectVisuals(f);
}

// The reflection, before the rendering renders it: the view box in world space and the reflection matrix, once after a
// move; the visuals in the view box (from the scene's primary and omni sectors), the copies of them it takes or makes,
// queued for rendering. The mirror's faces are drawn at the camera's far plane, after the z-buffer is cleared to 0, so
// that only what is seen through the mirror is rendered.
// The search for a visual's copy is written out with a local the match sets, which VC6 threads: the loop running out
// jumps straight to the copy's creation. The source's bound volume goes through an inline accessor, which reads *p once.
// Not matching yet: Normalize's inline budget, as in UpdateVisibility; with a forced inline Normalize this is the
// original's code exactly.
// FUNCTION: LS3DF 0x10032730
int I3D_mirror::PrepareReflection()
{
    if (!(mirror_flags & MIRF_PREPARED)) {
        if (!(mirror_flags & MIRF_VIEW_BOX))
            return 0;
        mirror_flags |= MIRF_PREPARED;
        S_matrix m = view_box * GetMatrix();
        bsphere.pos = m(3);
        S_vector r = S_vector(1.0f, 1.0f, 1.0f).RotateByMatrix(m);
        radius_2 = r.x * r.x + r.y * r.y + r.z * r.z;
        bsphere.radius = (float)sqrt(radius_2);
        inv_view_box.Inverse(m);
        m.SetDir(GetWorldDir());
        m(3) = GetWorldPos();
        mirror_matrix.Inverse(m);
        S_matrix flip = m;
        flip(2) *= -1.0f;
        mirror_matrix = mirror_matrix * flip;
    }
    MirroredVisualList.reset();
    pCurrentMirror = this;
    CollectVisuals(scene->primary_sector);
    CollectVisuals(scene->omni_sector);
    if (!MirroredVisualList.size()) {
        mirror_flags &= ~MIRF_REFLECT;
        return 0;
    }

    m_pD3DDevice8->Clear(0, NULL, D3DCLEAR_ZBUFFER | (m_HardwareCaps & 0x10) >> 2, 0, 0.0f, 0);
    LVertList.reserve(num_verts);
    LVertList.used_size = num_verts;
    const S_vector &cam_pos = pCurrentCamera->GetWorldPos();
    float dist = pCurrentCamera->fcp * .99f;
    const S_matrix &m = GetMatrix();
    uint c = MakeColor(1.0f, color);
    LVERTEX *lv = &LVertList[0];
    const S_vector *v = verts;
    for (int i = num_verts; i--; ) {
        S_vector w = *v * m;
        S_vector dir = w - cam_pos;
        dir.Normalize();
        w = cam_pos + dir * dist;
        lv->x = w.x;
        lv->y = w.y;
        lv->z = w.z;
        lv->color = c;
        lv++;
        v++;
    }
    if (m_CurrZFunc) {
        m_CurrZFunc = 0;
        m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS);
    }
    if (m_CurrFogState) {
        m_CurrFogState = FALSE;
        m_pD3DDevice8->SetRenderState(D3DRS_FOGENABLE, FALSE);
    }
    SetColorOp(D3DTOP_MODULATE);
    SetAlphaState(0);
    m_CullMode = D3DCULL_CCW;
    SetCullState(D3DCULL_NONE);
    SetCullState(D3DCULL_CCW);
    SetClipping(TRUE);
    pGlobalIGraph->SetWorldMatrix(IdentityMatrix);
    m_pD3DDevice8->SetTexture(0, NULL);
    pGlobalIGraph->DrawIndexedPrimitiveList(PT_TRIANGLELIST, num_verts, faces, num_faces, &LVertList[0],
        LS3D_STREAM_LVERTEX);
    if (m_CurrZFunc != 1) {
        m_CurrZFunc = 1;
        m_pD3DDevice8->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    }

    I3D_visual **p = &MirroredVisualList[0];
    for (int k = MirroredVisualList.size(); k--; p++) {
        I3D_visual *vis = NULL;
        I3D_visual **pc = &copies[0];
        for (int j = copies.size(); j--; pc++) {
            if ((I3D_frame *)(*pc)->user_data == *p) {
                vis = *pc;
                break;
            }
        }
        if (!vis) {
            vis = pI3DDriver->CreateVisual((*p)->visual_type);
            copies.push_back(vis);
            vis->vis_flags = VISF_PROXY;
            vis->Duplicate(*p);
            vis->user_data = (uint)*p;
            (*p)->ref++;
            vis->vis_flags = (vis->vis_flags & ~VISF_WORLD_SPACE) | VISF_PROXY;
            vis->frm_flags |= FRMFLAGS_CLIP;
            vis->SetCallback(NULL, FRMFLAGS_CALLBACK_MASK);
        }
        if (vis->update_count != (*p)->update_count) {
            vis->SetLocalMatrix((*p)->GetRenderMatrix() * mirror_matrix);
            vis->Update();
            vis->SetBoundVolume((*p)->GetBoundVolume());
            vis->update_count = (*p)->update_count;
        }
        if (vis->vis_flags & VISF_UNKNOWN_800)
            ((I3D_object *)vis)->transparency = ((I3D_object *)*p)->transparency;
        vis->m_AddFGroups();
        vis->render_number = RenderNumber;
    }
    if (!copies.size())
        mirror_flags &= ~MIRF_REFLECT;
    return copies.size();
}

// FUNCTION: LS3DF 0x100331b0
void I3D_mirror::SetViewBox(const I3D_bbox &bbox, const S_matrix &m)
{
    view_box = m;
    view_box(0) *= (bbox.max.x - bbox.min.x) * .5f;
    view_box(1) *= (bbox.max.y - bbox.min.y) * .5f;
    view_box(2) *= (bbox.max.z - bbox.min.z) * .5f;
    view_box(3) = (bbox.max + bbox.min) * .5f * view_box;
    mirror_flags = (mirror_flags & ~MIRF_PREPARED) | MIRF_VIEW_BOX;
}

// FUNCTION: LS3DF 0x100332e0
void I3D_mirror::LoadImage()
{
    I3D_frame::LoadImage();
    dtaRead(LoadImageHandle, &vis_bound, sizeof(vis_bound));
    vis_flags = (vis_flags & ~VISF_WBOUND_VALID) | VISF_BOUND_VALID;
    dtaRead(LoadImageHandle, &view_box, offsetof(I3D_mirror, verts) - offsetof(I3D_mirror, view_box));
    mirror_flags = (mirror_flags & ~MIRF_PREPARED) | MIRF_VIEW_BOX;
    verts = new S_vector[num_verts];
    dtaRead(LoadImageHandle, verts, num_verts * sizeof(S_vector));
    faces = new I3D_triface[num_faces];
    dtaRead(LoadImageHandle, faces, num_faces * sizeof(I3D_triface));
}
