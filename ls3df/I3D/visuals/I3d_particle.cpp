// The particles (PS2 I3d_particle.cpp), at 0x1003cbb0-0x10041800 in the original (1.1: 0x1003b0f0-0x1003f880, without
// the pointer lists' push_back): the particles' renderer with InitParticle and CloseParticle, the emitter (I3D_emitor)
// from its constructor, the header's zero vector's $E (0x1003eaa0) after its Reset, the rest of the emitter, then the
// particles (I3D_part_element, then I3D_part_base), each from its constructor, and the out-of-line template and
// in-class functions. PS2's file has an emitter of another design (PS2's I3D_emitor draws its own elements, the Xbox
// build's too), so the PC's names are guesses but I3D_emitor, SetParam, GetParam, Reset and KillElements.
// 1.1's code is 1.2's but for the buffers' locking (VBManager.h, IBManager.h). The renderer's m_RenderFGroup (the
// zero-initialized locals first, the quad indices counted down) and the particles' m_AddFGroups (ProcessVertices in
// each clip branch, lv first) are spelled as 1.1's original shows them, which gives 1.2's too.
//
// Not matching yet:
// - The emitter's Tick: the scale's QUAT_VALID test (the original reads the flags' byte into al ahead of the FPU's
//   pop) and life_max's two loads, in the other order (no spelling of the sum moves them). 1.1's differ the same way.
// - The element's Tick: the scale's copy (one store scheduled after the QUAT_VALID test in the original, as in the
//   emitter's Tick, so most likely SetScale's or S_vector's spelling in the headers). 1.1's original is 1.2's.
// - The particles' m_AddFGroups in 1.2: l reloaded once more in the original.
// - SetSector and GetSector, in the class: ours come after the element's Tick, the original's at the file's end (its
//   headers had more inline code before them, see tools/autopch.py). The pointer lists' push_back the original keeps
//   is this file's (0x10041700), ours another's.
#include "I3D/I3D.h"
#include "I3D/render/render.h"
#include "I3D/render/VBManager.h"
#include "I3D/render/IBManager.h"
#include "IGraph/IGraph.h"

#include <string.h>

// The particles' renderer (PC only, the name is a guess; pParticleVisual): renders the particles the rendering queued
// in ParticleElementsList (PARTF_RENDERER), sorted, a group of one sort_group with one draw from a buffer of its own,
// their meshes transformed into it (PARTF_MESH) or quads
// VTABLE: LS3DF 0x1009c4c8 I3D_frame
// VTABLE: LS3DF11 0x100994c8 I3D_frame
// VTABLE: LS3DF 0x1009c4b8 I3D_void_interface
// VTABLE: LS3DF11 0x100994b8 I3D_void_interface
class I3D_part_renderer : public I3D_visual {
public:
    I3D_part_renderer();
    ~I3D_part_renderer();

    virtual void m_AddFGroups();
    virtual void m_RenderFGroup(RENDER_FG_HELP_STRUCT *help);

    uint unknown_1dc;
    COMPLETE_VB_STRUCT mesh_vb;         // 0x1e0, the meshes' vertices, transformed
    COMPLETE_IB_STRUCT mesh_ib;         // 0x1ec
    COMPLETE_VB_STRUCT quad_vb;         // 0x1f8, 0x800 quads
    COMPLETE_IB_STRUCT quad_ib;         // 0x204
};

// GLOBAL: LS3DF 0x101b43d8
// GLOBAL: LS3DF11 0x101b14f8
I3D_visual *pParticleVisual = NULL;

// FUNCTION: LS3DF 0x1003cbb0
// FUNCTION: LS3DF11 0x1003b0f0
LS3D_RESULT InitParticle()
{
    pParticleVisual = new I3D_part_renderer;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x1003cc70
// FUNCTION: LS3DF11 0x1003b1b0
void CloseParticle()
{
    if (pParticleVisual)
        pParticleVisual->Release();
}

// Out of line after CloseParticle, though InitParticle inlines it: its scalar deleting destructor follows its copy,
// which the linker drops
I3D_part_renderer::I3D_part_renderer()
{
    memset(&mesh_vb, 0, sizeof(mesh_vb));
    memset(&mesh_ib, 0, sizeof(mesh_ib));
    memset(&quad_vb, 0, sizeof(quad_vb));
    memset(&quad_ib, 0, sizeof(quad_ib));
}

// FUNCTION: LS3DF 0x1003cca0
// FUNCTION: LS3DF11 0x1003b1e0
I3D_part_renderer::~I3D_part_renderer()
{
    cVBMan.m_Free(mesh_vb);
    cIBMan.m_Free(mesh_ib);
    cVBMan.m_Free(quad_vb);
    cIBMan.m_Free(quad_ib);
}

// Sorts the queued particles and queues a face group for each group of them: the particles of one sort_group next to
// each other in the sorted order, transparent at their mean distance. The last one alone is opaque with PARTF_OPAQUE
// while not faded, else transparent; its branch returns, so its push_back isn't merged with the other one's.
// FUNCTION: LS3DF 0x1003cd40
// FUNCTION: LS3DF11 0x1003b290
void I3D_part_renderer::m_AddFGroups()
{
    int i = ParticleElementsList.visuals.size();
    if (!i)
        return;
    ParticleElementsList.sort.Reserve(i);
    while (i--)
        ParticleElementsList.sort.Add(i, ParticleElementsList.visuals[i].sort_value);
    ParticleElementsList.sort.Sort();
    int j = ParticleElementsList.visuals.size() - 1;
    if (j < 0)
        return;
    RENDER_FG_HELP_STRUCT h;
    h.clip = 1;
    h.vis = this;
    do {
        I3D_part_base *part = (I3D_part_base *)ParticleElementsList.visuals[ParticleElementsList.sort[j]].vis;
        h.sector = part->GetSector() ? part->GetSector() : AddFGHlpStruct.sector;
        I3D_face_group *fg = part->mesh->levels[0]->fgroups;
        I3D_material *mat = fg->material;
        h.first = j;
        h.num = 1;
        h.fg = fg;
        h.texture = mat && mat->flags & MATF_TEXTURE ? mat->GetTexture() : NULL;
        if (--j >= 0) {
            float dist = part->sort_dist;
            int group = part->sort_group;
            while (((I3D_part_base *)ParticleElementsList.visuals[ParticleElementsList.sort[j]].vis)->sort_group ==
                    group) {
                dist += ((I3D_part_base *)ParticleElementsList.visuals[ParticleElementsList.sort[j]].vis)->sort_dist;
                h.num++;
                if (--j < 0)
                    break;
            }
            h.sort_value = (FloatToInt(dist) / h.num & 0x3fffffff) | 0x80000000;
            AddFGHlpStruct.num_transparent++;
            AddFGHlpStruct.fgroups.push_back(h);
        } else {
            if (part->part_flags & PARTF_OPAQUE && part->fade * part->transparency >= 1.0f) {
                h.sort_value = 0;
                if (h.texture)
                    h.sort_value = h.texture->m_UID & 0x7fff;
                AddFGHlpStruct.num_opaque++;
            } else {
                h.sort_value = (FloatToInt(part->sort_dist) & 0x3fffffff) | 0x80000000;
                AddFGHlpStruct.num_transparent++;
            }
            AddFGHlpStruct.fgroups.push_back(h);
            return;
        }
    } while (j >= 0);
}

// A particle's color with its alpha, for its vertices (the name is a guess). VC6 computes the operands left to right,
// and the original's order is blue, green, red, alpha.
#define PART_COLOR(part) \
    (FloatToInt(vertex_light_mult * (part)->color.z) | FloatToInt(vertex_light_mult * (part)->color.y) << 8 | \
        FloatToInt(vertex_light_mult * (part)->color.x) << 16 | \
        FloatToInt((part)->fade * (part)->transparency * 255.0f) << 24)

// Draws a group m_AddFGroups queued: the meshes of its particles (their first levels' first face group's vertices and
// faces, transformed, in the group's first particle's flags) or a quad for each, untransformed, with the first face
// group's material, depth writes off when blended
// FUNCTION: LS3DF 0x1003d0e0
// FUNCTION: LS3DF11 0x1003b630
void I3D_part_renderer::m_RenderFGroup(RENDER_FG_HELP_STRUCT *h)
{
    // Declared first, their zero gets esi from the function's start
    ushort num_verts = 0;
    LVERTEX *verts = NULL;
    int num_faces = 0;
    int first = h->first;
    uint num = h->num;
    int last = first - num + 1;
    uint flags = ((I3D_part_base *)ParticleElementsList.visuals[ParticleElementsList.sort[first]].vis)->part_flags;
    if (flags & PARTF_MESH) {
        if (!mesh_vb.vb && !cVBMan.m_Alloc(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1,
                D3DUSAGE_DYNAMIC | D3DUSAGE_SOFTWAREPROCESSING | D3DUSAGE_WRITEONLY, sizeof(LVERTEX), 0x2000, mesh_vb,
                VF_LVERTEX, false))
            return;
        if (!mesh_ib.ib && !cIBMan.m_Alloc(D3DUSAGE_DYNAMIC | D3DUSAGE_SOFTWAREPROCESSING, 0x3000, mesh_ib))
            return;
        verts = (LVERTEX *)mesh_vb.buffer->Lock(mesh_vb.base, BUFFER_LOCK_FLAGS);
        ushort *indices = (ushort *)mesh_ib.buffer->Lock(mesh_ib.base, BUFFER_LOCK_FLAGS);
        for (int i = first; i >= last; i--) {
            I3D_part_base *part = (I3D_part_base *)ParticleElementsList.visuals[ParticleElementsList.sort[i]].vis;
            I3D_mesh_level *level = part->mesh->levels[0];
            S_matrix m;
            if (part->billboard) {
                m = part->GetMatrix() * AddFGHlpStruct.camera->view_inv;
                m(3) = part->GetWorldPos();
            } else
                m = part->GetRenderMatrix();
            uint color = PART_COLOR(part);
            const I3D_vertex *v = level->LockVertices(0);
            for (int n = level->num_vertices; n; n--) {
                *(S_vector *)&verts->x = v->pos * m;
                verts->color = color;
                verts->u = v->tex.u;
                verts->v = v->tex.v;
                verts++;
                v++;
            }
            I3D_face_group *fg = level->fgroups;
            num_faces += fg->num_faces;
            const I3D_triface *faces = fg->GetFaces();
            for (int k = fg->num_faces; k; k--) {
                indices[0] = faces->vi[0] + num_verts;
                indices[1] = faces->vi[1] + num_verts;
                indices[2] = faces->vi[2] + num_verts;
                indices += 3;
                faces++;
            }
            fg->UnlockFaces();
            num_verts += level->num_vertices;
        }
        mesh_vb.buffer->Unlock();
        mesh_ib.buffer->Unlock();
    } else {
        if (!quad_vb.vb) {
            if (!cVBMan.m_Alloc(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1,
                    D3DUSAGE_DYNAMIC | D3DUSAGE_SOFTWAREPROCESSING | D3DUSAGE_WRITEONLY, sizeof(LVERTEX), 0x2000,
                    quad_vb, VF_LVERTEX, true))
                return;
            verts = (LVERTEX *)quad_vb.buffer->Lock(quad_vb.base, D3DLOCK_DISCARD | D3DLOCK_NOSYSLOCK);
            LVERTEX *v = verts;
            for (int k = 0x800; k; k--) {
                v[0].u = 0.0f;
                v[0].v = 1.0f;
                v[1].u = 1.0f;
                v[1].v = 1.0f;
                v[2].u = 1.0f;
                v[2].v = 0.0f;
                v[3].u = 0.0f;
                v[3].v = 0.0f;
                v += 4;
            }
        }
        if (!quad_ib.ib) {
            if (!cIBMan.m_Alloc(D3DUSAGE_SOFTWAREPROCESSING, 0x3000, quad_ib))
                return;
            ushort *indices = (ushort *)quad_ib.buffer->Lock(quad_ib.base, BUFFER_LOCK_FLAGS);
            // Counted down, the vertex and the index with counters of their own (the original's loop walks the
            // indices from the third, its count in a stack slot)
            ushort base = 0;
            int n = 0;
            for (int k = 0x800; k; k--) {
                indices[n] = base + 2;
                indices[n + 1] = base;
                indices[n + 2] = base + 3;
                indices[n + 3] = base + 1;
                indices[n + 4] = base;
                indices[n + 5] = base + 2;
                n += 6;
                base += 4;
            }
            quad_ib.buffer->Unlock();
        }
        if (!verts)
            verts = (LVERTEX *)quad_vb.buffer->Lock(quad_vb.base, BUFFER_LOCK_FLAGS);
        for (int i = first; i >= last; i--) {
            I3D_part_base *part = (I3D_part_base *)ParticleElementsList.visuals[ParticleElementsList.sort[i]].vis;
            S_matrix m;
            if (part->billboard) {
                m = part->GetMatrix() * AddFGHlpStruct.camera->view_inv;
                m(3) = part->GetWorldPos();
            } else
                m = part->GetRenderMatrix();
            uint color = PART_COLOR(part);
            *(S_vector *)&verts[0].x = S_vector(-.5f, -.5f, 0.0f) * m;
            verts[0].color = color;
            *(S_vector *)&verts[1].x = S_vector(.5f, -.5f, 0.0f) * m;
            verts[1].color = color;
            *(S_vector *)&verts[2].x = S_vector(.5f, .5f, 0.0f) * m;
            verts[2].color = color;
            *(S_vector *)&verts[3].x = S_vector(-.5f, .5f, 0.0f) * m;
            verts[3].color = color;
            verts += 4;
        }
        quad_vb.buffer->Unlock();
    }

    SetColorOp(m_Modulate2X);
    SetAlphaOp(D3DTOP_MODULATE);
    SetColorOp1(D3DTOP_DISABLE);
    I3D_material *mat = h->fg->material;
    int zwrite = 0;
    if (!mat) {
        SetAlphaState(0);
        m_pD3DDevice8->SetTexture(0, NULL);
    } else {
        if (flags & 3) {
            m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            zwrite = 1;
            SetAlphaState(1);
            SetBlendState(0);
        } else {
            m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            zwrite = 1;
            SetAlphaState(1);
            SetBlendState(1);
        }
        if (h->texture)
            m_pD3DDevice8->SetTexture(0, h->texture->m_pD3DTexture8);
        else
            m_pD3DDevice8->SetTexture(0, NULL);
    }
    pGlobalIGraph->SetWorldMatrix(IdentityMatrix);
    SetVertexFormat(VF_LVERTEX);
    if (flags & PARTF_MESH) {
        m_pD3DDevice8->SetStreamSource(0, mesh_vb.vb, sizeof(LVERTEX));
        m_pD3DDevice8->SetIndices(mesh_ib.ib, mesh_vb.base);
        m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, mesh_ib.base, num_faces);
    } else {
        m_pD3DDevice8->SetStreamSource(0, quad_vb.vb, sizeof(LVERTEX));
        m_pD3DDevice8->SetIndices(quad_ib.ib, quad_vb.base);
        // The vertices' count is the indices'
        m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num * 6, quad_ib.base, num * 2);
    }
    if (zwrite > 0)
        m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}

// FUNCTION: LS3DF 0x1003de50
// FUNCTION: LS3DF11 0x1003c140
I3D_emitor::I3D_emitor() : elements(0x20)
{
    m_Type = FRAME_EMITOR;
    mesh = NULL;
    Reset();
    time = 0;
    emit_time = 0;
    num_emitted = 0;
    sector = NULL;
    I3D_driver *drv = pI3DDriver;
    ushort n = drv->unknown_58++;
    id = n;
}

// FUNCTION: LS3DF 0x1003df20
// FUNCTION: LS3DF11 0x1003c210
I3D_emitor::~I3D_emitor()
{
    I3D_part_element **p = &elements[0];
    for (int i = elements.size(); i--; p++)
        (*p)->emitor = NULL;
    if (mesh)
        mesh->Release();
}

// FUNCTION: LS3DF 0x1003dfc0
// FUNCTION: LS3DF11 0x1003c2b0
void I3D_emitor::SetMesh(I3D_mesh_object *m)
{
    if (m == mesh)
        return;
    if (mesh)
        mesh->Release();
    if (m)
        m->AddRef();
    mesh = m;
}

// The floats come as their bits. The cases are in the order of the members (the jump table's targets), not of the
// values.
// FUNCTION: LS3DF 0x1003dff0
// FUNCTION: LS3DF11 0x1003c2e0
LS3D_RESULT I3D_emitor::SetParam(PARTICLE_PARAM param, uint value)
{
    if (param >= PP_LAST) {
        dbgPrintf("!! Invalid index:%u  I3D_emitor::SetParam()", param);
        return I3DERR_INVALIDPARAMS;
    }
    switch (param) {
    case PP_EMIT_TIME:
        p_emit_time = value;
        emit_time = 0;
        break;
    case PP_EMIT_TIME_RND:
        p_emit_time_rnd = value;
        emit_time = (uint)(RandFloat() * p_emit_time_rnd + p_emit_time);
        break;
    case PP_LIFE_TIME:
        p_life_time = value;
        break;
    case PP_LIFE_TIME_RND:
        p_life_time_rnd = value;
        break;
    case PP_SCALE:
        p_scale = I3DIntAsFloat(value);
        break;
    case PP_GROW_1:
        p_grow_1 = I3DIntAsFloat(value);
        break;
    case PP_GROW_2:
        p_grow_2 = I3DIntAsFloat(value);
        break;
    case PP_POS_RND:
        p_pos_rnd = I3DIntAsFloat(value);
        break;
    case PP_DIR_RND:
        p_dir_rnd = I3DIntAsFloat(value);
        break;
    case PP_FADE_IN:
        p_fade_in = I3DIntAsFloat(value);
        break;
    case PP_FADE_OUT:
        p_fade_out = I3DIntAsFloat(value);
        break;
    case PP_DIR_TIME:
        p_dir_time = value;
        break;
    case PP_DIR_TIME_RND:
        p_dir_time_rnd = value;
        break;
    case PP_DIR_TIME_STRAY:
        p_dir_time_stray = I3DIntAsFloat(value);
        break;
    case PP_VELOCITY_X:
        p_velocity.x = I3DIntAsFloat(value);
        break;
    case PP_VELOCITY_Y:
        p_velocity.y = I3DIntAsFloat(value);
        break;
    case PP_VELOCITY_Z:
        p_velocity.z = I3DIntAsFloat(value);
        break;
    case PP_VELOCITY_END_X:
        p_velocity_end.x = I3DIntAsFloat(value);
        break;
    case PP_VELOCITY_END_Y:
        p_velocity_end.y = I3DIntAsFloat(value);
        break;
    case PP_VELOCITY_END_Z:
        p_velocity_end.z = I3DIntAsFloat(value);
        break;
    case PP_MAX_PARTICLES:
        p_max_particles = value;
        SetOn(true);
        break;
    case PP_VELOCITY_WORLD:
        p_velocity_world = value;
        break;
    case PP_VELOCITY_END_WORLD:
        p_velocity_end_world = value;
        break;
    case PP_ROT_SPEED_1:
        p_rot_speed_1 = I3DIntAsFloat(value);
        break;
    case PP_ROT_SPEED_2:
        p_rot_speed_2 = I3DIntAsFloat(value);
        break;
    case PP_PRE_TICK:
        p_pre_tick = value;
        if (value)
            Tick(value);
        break;
    case PP_BILLBOARD:
        p_billboard = value != 0;
        break;
    case PP_ROT_AXIS:
        p_rot_axis = value;
        break;
    case PP_AXIS_MODE:
        p_axis_mode = value;
        break;
    case PP_COLOR:
        if (!value)
            return I3DERR_INVALIDPARAMS;
        p_color = *(const S_vector *)value;
        if (p_color.x < 0.0f)
            p_color.x = 0.0f;
        else if (p_color.x > 1.0f)
            p_color.x = 1.0f;
        if (p_color.y < 0.0f)
            p_color.y = 0.0f;
        else if (p_color.y > 1.0f)
            p_color.y = 1.0f;
        if (p_color.z < 0.0f)
            p_color.z = 0.0f;
        else if (p_color.z > 1.0f)
            p_color.z = 1.0f;
        break;
    case PP_COLOR_END:
        if (!value)
            return I3DERR_INVALIDPARAMS;
        p_color_end = *(const S_vector *)value;
        if (p_color_end.x < 0.0f)
            p_color_end.x = 0.0f;
        else if (p_color_end.x > 1.0f)
            p_color_end.x = 1.0f;
        if (p_color_end.y < 0.0f)
            p_color_end.y = 0.0f;
        else if (p_color_end.y > 1.0f)
            p_color_end.y = 1.0f;
        if (p_color_end.z < 0.0f)
            p_color_end.z = 0.0f;
        else if (p_color_end.z > 1.0f)
            p_color_end.z = 1.0f;
        break;
    case PP_OMNI_SECTOR:
        p_omni_sector = value != 0;
        break;
    case PP_FLAGS:
        part_flags = value;
        break;
    }
    return I3D_OK;
}

// A byte parameter as an int, the way I3DFloatAsInt gives a float's (I3D.h): copied to a local and read back as a
// dword, masked. The original stores the byte to the stack and reloads it; a plain return gives movzx.
inline uint I3DByteAsInt(const uchar &b)
{
    uchar t = b;
    return *(uint *)&t & 0xff;
}

// The floats as their bits, the colors as pointers to them, the bytes through I3DByteAsInt. The cases are in
// SetParam's order; PP_UNUSED_27 gives PP_BILLBOARD's.
// FUNCTION: LS3DF 0x1003e660
// FUNCTION: LS3DF11 0x1003c950
uint I3D_emitor::GetParam(PARTICLE_PARAM param) const
{
    if (param >= PP_LAST) {
        dbgPrintf("!! Invalid index:%u  I3D_emitor::GetParam()", param);
        return 0;
    }
    switch (param) {
    case PP_EMIT_TIME:
        return p_emit_time;
    case PP_EMIT_TIME_RND:
        return p_emit_time_rnd;
    case PP_LIFE_TIME:
        return p_life_time;
    case PP_LIFE_TIME_RND:
        return p_life_time_rnd;
    case PP_SCALE:
        return I3DFloatAsInt(p_scale);
    case PP_GROW_1:
        return I3DFloatAsInt(p_grow_1);
    case PP_GROW_2:
        return I3DFloatAsInt(p_grow_2);
    case PP_POS_RND:
        return I3DFloatAsInt(p_pos_rnd);
    case PP_DIR_RND:
        return I3DFloatAsInt(p_dir_rnd);
    case PP_FADE_IN:
        return I3DFloatAsInt(p_fade_in);
    case PP_FADE_OUT:
        return I3DFloatAsInt(p_fade_out);
    case PP_DIR_TIME:
        return p_dir_time;
    case PP_DIR_TIME_RND:
        return p_dir_time_rnd;
    case PP_DIR_TIME_STRAY:
        return I3DFloatAsInt(p_dir_time_stray);
    case PP_VELOCITY_X:
        return I3DFloatAsInt(p_velocity.x);
    case PP_VELOCITY_Y:
        return I3DFloatAsInt(p_velocity.y);
    case PP_VELOCITY_Z:
        return I3DFloatAsInt(p_velocity.z);
    case PP_VELOCITY_END_X:
        return I3DFloatAsInt(p_velocity_end.x);
    case PP_VELOCITY_END_Y:
        return I3DFloatAsInt(p_velocity_end.y);
    case PP_VELOCITY_END_Z:
        return I3DFloatAsInt(p_velocity_end.z);
    case PP_MAX_PARTICLES:
        return p_max_particles;
    case PP_VELOCITY_WORLD:
        return I3DByteAsInt(p_velocity_world);
    case PP_VELOCITY_END_WORLD:
        return I3DByteAsInt(p_velocity_end_world);
    case PP_ROT_SPEED_1:
        return I3DFloatAsInt(p_rot_speed_1);
    case PP_ROT_SPEED_2:
        return I3DFloatAsInt(p_rot_speed_2);
    case PP_PRE_TICK:
        return p_pre_tick;
    case PP_UNUSED_27:
    case PP_BILLBOARD:
        return p_billboard;
    case PP_ROT_AXIS:
        return p_rot_axis;
    case PP_AXIS_MODE:
        return p_axis_mode;
    case PP_COLOR:
        return (uint)&p_color;
    case PP_COLOR_END:
        return (uint)&p_color_end;
    case PP_OMNI_SECTOR:
        return p_omni_sector;
    case PP_FLAGS:
        return part_flags;
    }
    return 0;
}

// In the members' order but the billboards' axis and mode; VC6 moves the 0.1 stores and the velocities first
// FUNCTION: LS3DF 0x1003e930
// FUNCTION: LS3DF11 0x1003cc20
void I3D_emitor::Reset()
{
    p_emit_time = 100;
    p_emit_time_rnd = 60;
    p_life_time = 1800;
    p_life_time_rnd = 600;
    p_scale = 1.0f;
    p_grow_1 = -.25f;
    p_grow_2 = .6f;
    p_pos_rnd = 0.0f;
    p_dir_rnd = 0.0f;
    p_fade_in = .1f;
    p_fade_out = .1f;
    p_dir_time = 0;
    p_dir_time_rnd = 0;
    p_dir_time_stray = .1f;
    p_velocity = S_vector(0.0f, 2.0f, 0.0f);
    p_velocity_end = S_vector(2.0f, 0.0f, 0.0f);
    p_max_particles = 0;
    p_velocity_world = false;
    p_velocity_end_world = false;
    p_rot_speed_1 = -PI * .4f;
    p_rot_speed_2 = PI * .4f;
    p_pre_tick = 0;
    p_billboard = true;
    p_rot_axis = 0;
    p_axis_mode = 3;
    p_color = UnitVector;
    p_color_end = UnitVector;
    p_omni_sector = false;
    part_flags = 0x104;
}

// Switched on, it starts again. The frame's SetOn written out, then a test VC6 can't fold into it.
// FUNCTION: LS3DF 0x1003eac0
// FUNCTION: LS3DF11 0x1003cdb0
void I3D_emitor::SetOn(bool on)
{
    if (on)
        m_FrmFlags |= FRMFLAGS_ON;
    else
        m_FrmFlags &= ~FRMFLAGS_ON;
    if (on == true) {
        num_emitted = 0;
        emit_time = 0;
        time = 0;
    }
}

// A move looks for its sector again
// FUNCTION: LS3DF 0x1003eb10
// FUNCTION: LS3DF11 0x1003ce00
void I3D_emitor::UpdateV()
{
    if (m_FrmFlags & (FRMFLAGS_UPDATE | FRMFLAGS_CHANGED))
        sector = NULL;
}

// Creates the particles due: each in the box around the emitter, with its parameters, in the emitter's sector (or the
// scene's omni sector) and its scene's tick list, then ticked for the time since it was due
// FUNCTION: LS3DF 0x1003eb30
// FUNCTION: LS3DF11 0x1003ce20
void I3D_emitor::Tick(int t)
{
    if (!(m_FrmFlags & FRMFLAGS_ON))
        return;
    time += t;
    while (time >= emit_time) {
        time -= emit_time;
        num_emitted++;
        if (p_max_particles && num_emitted > p_max_particles) {
            SetOn(false);
            return;
        }
        if (!sector) {
            for (I3D_frame *frm = m_pParent; frm; frm = frm->m_pParent) {
                if (frm->m_Type == FRAME_SECTOR) {
                    sector = (I3D_sector *)frm;
                    break;
                }
            }
            if (!sector)
                return;
        }
        I3D_part_element *e = new I3D_part_element;
        if (!e)
            return;
        e->SetMesh(mesh, false);
        e->emitor = this;
        elements.push_back(e);
        e->billboard = p_billboard;
        e->vis_flags |= VISF_COLOR_CHANGED;
        e->rot_axis = p_rot_axis;
        e->axis_mode = p_axis_mode;
        e->SetSector(sector);
        if (part_flags & PARTF_RENDERER) {
            e->part_flags &= ~PARTF_RENDERER;
            e->part_flags |= part_flags & PARTF_RENDERER;
            e->sort_group = id;
        }
        e->part_flags &= ~(PARTF_OPAQUE | PARTF_ALPHA | 4);
        e->part_flags |= part_flags & (PARTF_OPAQUE | PARTF_ALPHA | 4);
        float sc = p_scale;
        S_vector s = GetScale();
        e->SetScale(s * sc);
        S_vector pos = GetWorldPos();
        pos.x += RandFloat() * p_pos_rnd + (-p_pos_rnd) * .5;
        pos.y += RandFloat() * p_pos_rnd + (-p_pos_rnd) * .5;
        pos.z += RandFloat() * p_pos_rnd + (-p_pos_rnd) * .5;
        e->SetPos(pos);
        I3D_scene *scene = sector->scene;
        if (!scene)
            return;
        if (p_omni_sector)
            e->LinkTo(scene->omni_sector, 0);
        else
            e->LinkTo(sector, I3DLINK_UPDMATRIX);
        scene->AddTickFrame(e);
        scene->AddFrame(e);
        e->Release();

        S_matrix m = GetMatrix();
        if (p_velocity_world)
            e->dir_start = p_velocity;
        else
            e->dir_start = p_velocity.RotateByMatrix(m);
        if (p_velocity_end_world)
            e->dir_end = p_velocity_end;
        else
            e->dir_end = p_velocity_end.RotateByMatrix(m);
        e->speed = e->speed_start = p_velocity.Magnitude();
        e->speed_end = p_velocity_end.Magnitude();
        e->dir_start.Normalize();
        e->dir_end.Normalize();
        e->dir_prev = e->dir_start;
        e->dir = e->dir_end;
        e->dir.x += p_dir_rnd * .5 - RandFloat() * p_dir_rnd;
        e->dir.y += p_dir_rnd * .5 - RandFloat() * p_dir_rnd;
        e->dir.z += p_dir_rnd * .5 - RandFloat() * p_dir_rnd;
        e->dir.Normalize();
        e->Update();
        e->life = (uint)(RandFloat() * p_life_time_rnd + p_life_time);
        e->life_max = p_life_time_rnd + p_life_time;
        e->dir_time = p_dir_time;
        e->dir_time_rnd = p_dir_time_rnd;
        e->dir_period = (int)(RandFloat() * p_dir_time_rnd + p_dir_time);
        e->dir_time_stray = p_dir_time_stray;
        e->fade_in = (uint)(e->life * p_fade_in);
        e->fade_out = (uint)(e->life - e->life * p_fade_out);
        e->color = p_color;
        e->vis_flags |= VISF_COLOR_CHANGED;
        e->color_start = p_color;
        e->color_end = p_color_end;
        e->rot_speed = (RandFloat() * (p_rot_speed_2 - p_rot_speed_1) + p_rot_speed_1) * .001;
        float grow_min = min(p_grow_1, p_grow_2);
        float grow_max = max(p_grow_1, p_grow_2);
        e->grow = (RandFloat() * (grow_max - grow_min) + grow_min) * .001;
        uint emit_base = p_emit_time;
        emit_time = (uint)(RandFloat() * p_emit_time_rnd + emit_base);
        if (time > emit_time) {
            int late = time - emit_base;
            if (late > 0)
                e->Tick(late);
        }
    }
}

// FUNCTION: LS3DF 0x1003f6b0
// FUNCTION: LS3DF11 0x1003d9a0
LS3D_RESULT I3D_emitor::Duplicate(I3D_frame *frame)
{
    if (!frame || frame == this)
        return I3DERR_INVALIDPARAMS;
    I3D_frame::Duplicate(frame);
    if (frame->m_Type == FRAME_EMITOR) {
        I3D_emitor *e = (I3D_emitor *)frame;
        SetMesh(e->mesh);
        p_emit_time = e->p_emit_time;
        p_emit_time_rnd = e->p_emit_time_rnd;
        p_life_time = e->p_life_time;
        p_life_time_rnd = e->p_life_time_rnd;
        p_scale = e->p_scale;
        p_grow_1 = e->p_grow_1;
        p_grow_2 = e->p_grow_2;
        p_pos_rnd = e->p_pos_rnd;
        p_dir_rnd = e->p_dir_rnd;
        p_fade_in = e->p_fade_in;
        p_fade_out = e->p_fade_out;
        p_dir_time = e->p_dir_time;
        p_dir_time_rnd = e->p_dir_time_rnd;
        p_dir_time_stray = e->p_dir_time_stray;
        p_velocity = e->p_velocity;
        p_velocity_end = e->p_velocity_end;
        part_flags = e->part_flags;
        p_max_particles = e->p_max_particles;
        p_velocity_world = e->p_velocity_world;
        p_velocity_end_world = e->p_velocity_end_world;
        p_rot_speed_1 = e->p_rot_speed_1;
        p_rot_speed_2 = e->p_rot_speed_2;
        p_billboard = e->p_billboard;
        p_rot_axis = e->p_rot_axis;
        p_axis_mode = e->p_axis_mode;
        p_pre_tick = e->p_pre_tick;
        p_color = e->p_color;
        p_color_end = e->p_color_end;
        p_omni_sector = e->p_omni_sector;
        return I3D_OK;
    }
    if (frame->m_Type != FRAME_VISUAL)
        return I3D_OK;
    p_billboard = false;
    switch (((I3D_visual *)frame)->GetVisualType()) {
    case I3D_VISUAL_BILLBOARD:
        p_billboard = true;
    case I3D_VISUAL_OBJECT:
    case I3D_VISUAL_LIT_OBJECT:
    case I3D_VISUAL_SINGLEMESH:
    case I3D_VISUAL_LENSF:
    case I3D_VISUAL_MORPH:
    case I3D_VISUAL_SINGLEMORPH:
        SetMesh(((I3D_object *)frame)->mesh);
        return I3D_OK;
    }
    return I3DERR_GENERIC;
}

// FUNCTION: LS3DF 0x1003f910
// FUNCTION: LS3DF11 0x1003dc00
void I3D_emitor::KillElements()
{
    I3D_part_element **p = &elements[0];
    for (int i = elements.size(); i--; p++) {
        (*p)->emitor = NULL;
        (*p)->LinkTo(NULL, 0);
        (*p)->Release();
    }
    elements.reset();
}

// A particle that dies takes itself out of the list, which moves the last one into its place
// FUNCTION: LS3DF 0x1003f960
// FUNCTION: LS3DF11 0x1003dc50
void I3D_emitor::TickElements(int t)
{
    I3D_part_element **p = &elements[0];
    int i = elements.size();
    while (i--) {
        I3D_part_element *e = *p;
        e->Tick(t);
        if (e != *p) {
            if (i < 2)
                break;
            i--;
        } else
            p++;
    }
}

// FUNCTION: LS3DF 0x1003f9b0
// FUNCTION: LS3DF11 0x1003dca0
void I3D_emitor::SetElementsOn(bool on)
{
    I3D_part_element **p = &elements[0];
    for (int i = elements.size(); i--; p++)
        (*p)->SetOn(on);
}

// FUNCTION: LS3DF 0x1003f9f0
// FUNCTION: LS3DF11 0x1003dce0
void I3D_emitor::LoadImage()
{
    I3D_frame::LoadImage();
    I3D_mesh_object *m = LoadMeshImage();
    SetMesh(m);
    if (m)
        m->Release();
}

// FUNCTION: LS3DF 0x1003fa20
// FUNCTION: LS3DF11 0x1003dd10
I3D_part_element::I3D_part_element()
{
    visual_type = I3D_VISUAL_PART_ELEMENT;
    emitor = NULL;
    life = 0;
    life_max = 0;
    unknown_290 = 0;
    dir_start = S_vector(0.0f, 0.0f, 0.0f);
    dir_end = S_vector(0.0f, 0.0f, 0.0f);
    dir = S_vector(0.0f, 0.0f, 0.0f);
    dir_prev = S_vector(0.0f, 0.0f, 0.0f);
    dir_time = 0;
    dir_time_stray = 0.0f;
    speed_start = 0.0f;
    speed_end = 0.0f;
    grow = 0.0f;
    color_start = S_vector(1.0f, 1.0f, 1.0f);
    color_end = S_vector(1.0f, 1.0f, 1.0f);
    age = 0;
    speed = 0.0f;
    dir_period = 0;
    dir_counter = 0;
}

// FUNCTION: LS3DF 0x1003fc00
// FUNCTION: LS3DF11 0x1003def0
I3D_part_element::~I3D_part_element()
{
    if (emitor)
        emitor->elements.remove(this);
}

// FUNCTION: LS3DF 0x1003fca0
// FUNCTION: LS3DF11 0x1003df90
LS3D_RESULT I3D_part_element::Duplicate(I3D_frame *frame)
{
    LS3D_RESULT r = I3D_part_base::Duplicate(frame);
    if (r < I3D_OK)
        return r;
    if (frame->m_Type == FRAME_VISUAL && ((I3D_visual *)frame)->GetVisualType() == I3D_VISUAL_PART_ELEMENT) {
        I3D_part_element *e = (I3D_part_element *)frame;
        life = e->life;
        life_max = e->life_max;
        unknown_290 = e->unknown_290;
        dir_start = e->dir_start;
        dir_end = e->dir_end;
        dir = e->dir;
        dir_prev = e->dir_prev;
        dir_time = e->dir_time;
        dir_time_stray = e->dir_time_stray;
        speed_start = e->speed_start;
        speed_end = e->speed_end;
        grow = e->grow;
        color_start = e->color_start;
        color_end = e->color_end;
        age = e->age;
        speed = e->speed;
        dir_period = e->dir_period;
        dir_counter = e->dir_counter;
        lightness = e->lightness;
        transparency = e->transparency;
        fade = e->fade;
    }
    return I3D_OK;
}

// Moves, turns and grows the particle in steps of at most 10 ms, heading from dir_prev to dir (which strays again
// every dir_period), at a speed and in a color from their start to their end, faded in and out. At the end of its life
// it leaves its tick list, its parent, its emitter and its scene.
// The step is set to 10 first and replaced (one store after the join). The rotation and the growth convert dt in their
// products, which VC6 does once into a temporary; the speed's step is a double product (fld speed; fimul dt), which
// a float product would take from that temporary.
// FUNCTION: LS3DF 0x1003fe40
// FUNCTION: LS3DF11 0x1003e130
void I3D_part_element::Tick(int time)
{
    if (!(m_FrmFlags & FRMFLAGS_ON))
        return;
    age += time;
    if ((uint)age >= life) {
        pI3DDriver->DeleteFromSceneTickList(this);
        LinkTo(NULL, 0);
        if (emitor) {
            emitor->elements.remove(this);
            emitor = NULL;
        }
        DeleteFromOwner();
        return;
    }
    while (time > 0) {
        int dt = time >= 10 ? 10 : time;
        time -= 10;
        if (dir_period > 0) {
            dir_counter += dt;
            if (dir_counter >= dir_period) {
                dir_counter = 0;
                dir_period = (int)(RandFloat() * dir_time_rnd + dir_time);
                dir_prev = dir;
                dir = dir_end;
                dir.x += dir_time_stray * .5 - RandFloat() * dir_time_stray;
                dir.y += dir_time_stray * .5 - RandFloat() * dir_time_stray;
                dir.z += dir_time_stray * .5 - RandFloat() * dir_time_stray;
                dir.Normalize();
            }
        }
        S_quat q;
        q.Make(S_vector(0.0f, 0.0f, 1.0f), dt * rot_speed);
        if (!(m_FrmFlags & FRMFLAGS_QUAT_VALID)) {
            m_FrmFlags |= FRMFLAGS_QUAT_VALID;
            m_Rot.Make(m_LocalMatrix);
        }
        m_Rot = m_Rot * q;
        m_Rot.Normalize();
        m_FrmFlags = (m_FrmFlags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID | FRMFLAGS_MAT_VALID |
            FRMFLAGS_ROT_VALID)) | FRMFLAGS_UPDATE | FRMFLAGS_QUAT_VALID;
        SetScale(GetScale() * (float)(dt * grow + 1.0));

        float alpha;
        int a = age;
        if (a < fade_in)
            alpha = (float)a / fade_in;
        else if (a > life - fade_out)
            alpha = (float)(life - a) / fade_out;
        else
            alpha = 1.0f;
        if (!IsAbsMrgZero(alpha - transparency)) {
            transparency = alpha;
            vis_flags |= VISF_COLOR_CHANGED;
        }
        speed = (speed_end - speed_start) / life * age + speed_start;
        S_vector c = (color_end - color_start) / life * age + color_start;
        if (!IsAbsMrgZero(c.x - color.x) || !IsAbsMrgZero(c.y - color.y) || !IsAbsMrgZero(c.z - color.z)) {
            color = c;
            vis_flags |= VISF_COLOR_CHANGED;
        }

        float f;
        if (dir_period > 0) {
            f = (float)dir_counter / min(dir_period, life_max - age);
        } else
            f = (float)age / life_max;
        S_vector d = dir * f + dir_prev * (1.0f - f);
        SetPos(m_LocalMatrix(3) + d * (float)(dt * .001 * speed));
    }
    Update();
}

// FUNCTION: LS3DF 0x100406c0
// FUNCTION: LS3DF11 0x1003e9b0
I3D_part_base::I3D_part_base()
{
    visual_type = I3D_VISUAL_PART_BASE;
    color = UnitVector;
    billboard = true;
    rot_axis = 0;
    axis_mode = 3;
    sector = NULL;
    sort_group = 0xffff;
    part_flags = PARTF_OPAQUE | 0x100;
}

// The mesh as the object's, without the billboard's
// FUNCTION: LS3DF 0x10040770
// FUNCTION: LS3DF11 0x1003ea60
LS3D_RESULT I3D_part_base::Duplicate(I3D_frame *frame)
{
    LS3D_RESULT r = I3D_object::Duplicate(frame);
    if (r < I3D_OK)
        return r;
    sector = NULL;
    if (frame->m_Type == FRAME_VISUAL) {
        switch (((I3D_visual *)frame)->GetVisualType()) {
        case I3D_VISUAL_PART_BASE:
        case I3D_VISUAL_PART_ELEMENT: {
            I3D_part_base *p = (I3D_part_base *)frame;
            color = p->color;
            billboard = p->billboard;
            rot_axis = p->rot_axis;
            axis_mode = p->axis_mode;
            sector = p->sector;
            sort_group = p->sort_group;
            part_flags = p->part_flags;
        } break;
        }
    }
    return I3D_OK;
}

// Like the billboard's, with the color written into the lod's vertices where it changed and no lighting or
// environment map: the matrix turned to the camera only with billboard set, the lod's sector the particle's own if it
// has one, and transparent unless PARTF_OPAQUE and not faded. The face groups are walked through their GetNext and
// GetMaterial (as the single meshes'): the inline call pending after the first Lock keeps its push_back out of line,
// as the original has it.
// FUNCTION: LS3DF 0x10040820
// FUNCTION: LS3DF11 0x1003eb10
void I3D_part_base::m_AddFGroups()
{
    if (!mesh)
        return;
    if (vis_flags & VISF_PROXY) {
        I3D_part_base *vis = (I3D_part_base *)m_UserData;
        part_flags = vis->part_flags;
        if (vis->GetSector())
            AddFGHlpStruct.sector = vis->GetSector();
        else {
            I3D_frame *frm = vis->m_pParent;
            while (true) {
                if (!frm) {
                    AddFGHlpStruct.sector = pCurrentScene->primary_sector;
                    break;
                }
                if (frm->m_Type == FRAME_SECTOR) {
                    AddFGHlpStruct.sector = (I3D_sector *)frm;
                    break;
                }
                frm = frm->m_pParent;
            }
        }
        int first = AddFGHlpStruct.fgroups.size();
        vis->m_AddFGroups();
        for (int i = first; i < AddFGHlpStruct.fgroups.size(); i++)
            AddFGHlpStruct.fgroups[i].vis = this;
        if (vis->billboard)
            ProductBillMatrix(AddFGHlpStruct.cam_pos, &g_matrix);
        return;
    }
    const I3D_bvolume &bvol = GetWBoundVolume();
    float dist_2 = (bvol.bsphere.pos - AddFGHlpStruct.cam_pos).Square();
    int lod_index = GetObjectLOD(this, dist_2);
    I3D_mesh_level *level = mesh->levels[lod_index];
    if (!level || !level->fgroups)
        return;
    int num_verts = level->num_vertices;
    if (!num_verts)
        return;
    I3D_vertex *verts = (I3D_vertex *)level->vb.buffer->Lock(level->vb.base, BUFFER_LOCK_FLAGS);
    lod *l = lods[lod_index];
    if (!l || l->dirty || !l->vb.vb) {
        if (!l) {
            l = new lod;
            if (!l) {
                dbgPrintf("!! Out of memory    renderObject()");
                ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                return;
            }
        }
        if (!l->Create(1 | 0x300, level->num_vertices, &verts->pos, sizeof(I3D_vertex), &verts->tex,
                sizeof(I3D_vertex), NULL, 0, mesh->u_ofs, mesh->v_ofs)) {
            dbgPrintf("!! Out of memory    renderObject()");
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return;
        }
        lods[lod_index] = l;
        l->lights_changed = (uint)-1;
        l->light_key = (uint)-1;
    }

    if (lod_index == mesh->num_lods - 1 && level->lod_dist > 0.0f) {
        float f = level->lod_dist * 0.5f;
        if (mesh->num_lods > 1)
            f += mesh->levels[lod_index - 1]->lod_dist;
        float s = RSLodQuality * m_WorldScale;
        float d = 1.0f - (dist_2 / (s * s) - f) / (level->lod_dist - f);
        if (d > 1.0f)
            d = 1.0f;
        SetFade(d);
    } else
        SetFade(1.0f);

    bool moved;
    if (l->update_count != m_UpdateCount) {
        moved = true;
        l->update_count = m_UpdateCount;
    } else
        moved = false;
    uchar *lv = NULL;                   // first, it is zeroed ahead of the u_ofs compare
    int base = l->vb.base;
    IDirect3DVertexBuffer8 *dest = l->vb.vb;
    if (!IsAbsMrgZero(mesh->u_ofs - l->u_ofs) || !IsAbsMrgZero(mesh->v_ofs - l->v_ofs))
        l->SetUV(1, level->num_vertices, &verts->tex, sizeof(I3D_vertex), mesh->u_ofs, mesh->v_ofs);
    const S_matrix *m;
    if (billboard) {
        ProductBillMatrix(AddFGHlpStruct.cam_pos, &g_matrix);
        m = &g_matrix;
    } else
        m = &GetRenderMatrix();

    if (vis_flags & VISF_COLOR_CHANGED || moved || l->alpha_dirty) {
        l->alpha_dirty = false;
        vis_flags &= ~VISF_COLOR_CHANGED;
        lv = l->vb.buffer->Lock(base, BUFFER_LOCK_FLAGS);
        uint c = PART_COLOR(this);
        if (m_HardwareCaps & HWCAPS_HWTL) {
            LVERTEX *v = (LVERTEX *)lv;
            for (int n = num_verts; n; n--) {
                v->color = c;
                v++;
            }
        } else {
            TLVERTEX *v = (TLVERTEX *)lv;
            for (int n = num_verts; n; n--) {
                v->color = c;
                v++;
            }
        }
    }

    if ((renderCameraChanged || m_RenderNumber != RenderNumberMinus1 || moved) && !(m_HardwareCaps & HWCAPS_HWTL)) {
        if (m_FrmFlags & FRMFLAGS_CLIP) {
            m_pD3DDevice8->SetStreamSource(0, level->vb.vb, level->vb.buffer->stride);
            m_pD3DDevice8->SetTransform(D3DTS_WORLD, m);
            level->vb.buffer->Unlock();
            SetVertexFormat(VF_VERTEX);
            SetClipping(TRUE);
            // In each branch: the TRUE one keeps its own SetRenderState call (1.1's original shows it)
            m_pD3DDevice8->ProcessVertices(level->vb.base, base, num_verts, dest, D3DPV_DONOTCOPYDATA);
        } else {
            m_pD3DDevice8->SetStreamSource(0, level->vb.vb, level->vb.buffer->stride);
            m_pD3DDevice8->SetTransform(D3DTS_WORLD, m);
            level->vb.buffer->Unlock();
            SetVertexFormat(VF_VERTEX);
            SetClipping(FALSE);
            m_pD3DDevice8->ProcessVertices(level->vb.base, base, num_verts, dest, D3DPV_DONOTCOPYDATA);
        }
        if (!(m_HardwareCaps & HWCAPS_TABLEFOG)) {
            if (!lv)
                lv = l->vb.buffer->Lock(base, BUFFER_LOCK_FLAGS);
            FogProcess((uchar *)&((TLVERTEX *)lv)->specular + 3, &((TLVERTEX *)lv)->z, sizeof(TLVERTEX), num_verts,
                AddFGHlpStruct.sector->fog_light, 0);
        }
    }

    RENDER_FG_HELP_STRUCT h;
    h.clip = m_FrmFlags & FRMFLAGS_CLIP;
    h.vis = this;
    h.sector = sector ? sector : AddFGHlpStruct.sector;
    for (I3D_face_group *fg = level->fgroups; fg; fg = fg->GetNext()) {
        I3D_material *mat = fg->GetMaterial();
        h.fg = fg;
        h.texture = mat && mat->flags & MATF_TEXTURE ? mat->GetTexture() : NULL;
        if (part_flags & PARTF_OPAQUE && fade * transparency >= 1.0f) {
            h.sort_value = 0;
            if (h.texture)
                h.sort_value = h.texture->m_UID & 0x7fff;
            AddFGHlpStruct.num_opaque++;
        } else {
            h.sort_value = (FloatToInt(sort_dist) & 0x3fffffff) | 0x80000000;
            AddFGHlpStruct.num_transparent++;
        }
        AddFGHlpStruct.fgroups.push_back(h);
    }
}

// The face group with the particle's lod: opaque with PARTF_OPAQUE while not faded, else blended (by alpha with
// PARTF_ALPHA, else added) without depth writes; untextured, transparent while faded. Both cull modes go through
// SetCullState: the proxy's written out leaves the inline budget for one more SetAlphaState than the original has.
// FUNCTION: LS3DF 0x10041320
// FUNCTION: LS3DF11 0x1003f410
void I3D_part_base::m_RenderFGroup(RENDER_FG_HELP_STRUCT *h)
{
    I3D_face_group *fg = h->fg;
    I3D_material *mat = fg->material;
    lod *l;
    int num_verts;
    if (vis_flags & VISF_PROXY) {
        I3D_object *obj = (I3D_object *)m_UserData;
        l = obj->lods[obj->cur_lod];
        SetCullState(D3DCULL_NONE);
        num_verts = mesh->levels[obj->cur_lod]->num_vertices;
    } else {
        l = lods[cur_lod];
        SetCullState(mat && mat->flags & MATF_TWO_SIDED ? D3DCULL_NONE : D3DCULL_CCW);
        num_verts = mesh->levels[cur_lod]->num_vertices;
    }
    SetColorOp(m_Modulate2X);
    SetAlphaOp(D3DTOP_MODULATE);
    SetColorOp1(D3DTOP_DISABLE);
    IDirect3DVertexBuffer8 *vb = l->vb.vb;
    int zwrite = 0;
    if (!mat) {
        if (fade * transparency < 1.0f) {
            m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            zwrite = 1;
            SetAlphaState(1);
            SetBlendState(0);
        } else
            SetAlphaState(0);
        m_pD3DDevice8->SetTexture(0, NULL);
    } else {
        if (part_flags & PARTF_OPAQUE && fade * transparency >= 1.0f)
            SetAlphaState(0);
        else if (part_flags & 3) {
            m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            zwrite = 1;
            SetAlphaState(1);
            SetBlendState(0);
        } else {
            m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            zwrite = 1;
            SetAlphaState(1);
            SetBlendState(1);
        }
        if (h->texture)
            m_pD3DDevice8->SetTexture(0, h->texture->m_pD3DTexture8);
        else
            m_pD3DDevice8->SetTexture(0, NULL);
    }
    if (m_HardwareCaps & HWCAPS_HWTL)
        m_pD3DDevice8->SetTransform(D3DTS_WORLD, billboard ? &g_matrix : &GetRenderMatrix());
    SetVertexFormat(l->vb.buffer->vertex_format);
    m_pD3DDevice8->SetStreamSource(0, vb, l->vb.buffer->stride);
    m_pD3DDevice8->SetIndices(fg->ib.ib, l->vb.base);
    m_pD3DDevice8->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, num_verts, fg->ib.base, fg->num_faces);
    if (zwrite > 0)
        m_pD3DDevice8->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
}
