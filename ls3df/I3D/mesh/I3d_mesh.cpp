// The meshes (PS2 I3d_mesh.cpp), at 0x10030560-0x10031820 in the original: I3D_face_group, I3D_mesh_level and
// I3D_mesh_object, each in PS2's order with the PC's additions. The header's zero vector has its $E at the end
// (0x10031800). Names are the PS2 build's unless noted.
#include "I3D/I3D.h"
#include "I3D/mesh/I3d_material.h"
#include "I3D/render/IBManager.h"
#include "I3D/render/VBManager.h"
#include "IGraph/IGraph.h"

#include <math.h>
#include <string.h>

// FUNCTION: LS3DF 0x10030560
I3D_face_group::I3D_face_group()
{
    next = NULL;
    material = NULL;
    num_faces = 0;
    memset(&ib, 0, sizeof(ib));
}

// FUNCTION: LS3DF 0x10030580
void I3D_face_group::SetMaterial(I3D_material *mat)
{
    if (mat != material) {
        if (material)
            material->Release();
        material = mat;
        if (mat)
            mat->AddRef();
    }
}

// FUNCTION: LS3DF 0x100305b0
void I3D_face_group::SetFaces(const I3D_triface *faces, int num)
{
    if (!faces) {
        cIBMan.m_Free(ib);
        memset(&ib, 0, sizeof(ib));
        num_faces = 0;
        return;
    }
    I3D_triface *dst = CreateFaces(num);
    memcpy(dst, faces, num * sizeof(I3D_triface));
    num_faces = num;
}

// FUNCTION: LS3DF 0x10030610
I3D_triface *I3D_face_group::CreateFaces(int num)
{
    cIBMan.m_Free(ib);
    memset(&ib, 0, sizeof(ib));
    if (!cIBMan.m_Alloc(D3DUSAGE_SOFTWAREPROCESSING, num * 3, ib)) {
        num_faces = 0;
        dbgPrintf("!! Out of memory  I3D_face_group::CreateFaces(%d)", num);
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return NULL;
    }
    num_faces = num;
    return GetFaces();
}

// FUNCTION: LS3DF 0x10030690
I3D_triface *I3D_face_group::GetFaces()
{
    if (!num_faces)
        return NULL;
    return (I3D_triface *)ib.buffer->Lock(ib.base, 0);
}

// FUNCTION: LS3DF 0x10030780
void I3D_face_group::UnlockFaces()
{
    ib.buffer->Unlock();
}

// FUNCTION: LS3DF 0x10030800
LS3D_RESULT I3D_face_group::Duplicate(I3D_face_group *fg)
{
    I3D_triface *faces = CreateFaces(fg->num_faces);
    memcpy(faces, fg->GetFaces(), fg->num_faces * sizeof(I3D_triface));
    num_faces = fg->num_faces;
    SetMaterial(fg->material);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10030860
LS3D_RESULT I3D_face_group::DetachMaterial()
{
    if (material && material->ref >= 2) {
        I3D_material *mat = material->flags & MATF_ANIMATED ? pI3DDriver->CreateAnimMaterial() :
            pI3DDriver->CreateMaterial();
        if (!mat) {
            dbgPrintf("!! Out of memory  I3D_face_group::DetachMaterial()");
            ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
            return I3DERR_OUTOFMEM;
        }
        LS3D_RESULT r = mat->Duplicate(material);
        if (r < 0) {
            mat->Release();
            dbgPrintf("!! Can't duplicate mesh  I3D_face_group::DetachMaterial()");
            return r;
        }
        material->Release();
        material = mat;
    }
    return I3D_OK;
}

// Inline: the scalar deleting destructor that a next face group needs has its code
inline I3D_face_group::~I3D_face_group()
{
    if (material)
        material->Release();
    cIBMan.m_Free(ib);
    delete next;
}

// FUNCTION: LS3DF 0x10030900
I3D_mesh_level::I3D_mesh_level()
{
    ref = 1;
    num_vertices = 0;
    memset(&vb, 0, sizeof(vb));
    num_fgroups = 0;
    fgroups = NULL;
    bbox.Invalidate();
    lod_dist = 0.0f;
    mesh_flags = 0;
    env_scale = sqrt(2.0) * 2;
}

// Release inlines it, and /OPT:REF drops its copy. A level without vertices frees vb.buffer as a plain block.
I3D_mesh_level::~I3D_mesh_level()
{
    delete fgroups;
    if (!num_vertices && vb.buffer)
        delete[] (uchar *)vb.buffer;
    else
        cVBMan.m_Free(vb);
}

// FUNCTION: LS3DF 0x100309b0
int I3D_mesh_level::Release()
{
    if (--ref)
        return ref;
    delete this;
    return 0;
}

// FUNCTION: LS3DF 0x10030a50
void I3D_mesh_level::UpdateBBox()
{
    const I3D_vertex *v = LockVertices(0);
    if (!v) {
        bbox.Invalidate();
        return;
    }
    int i = num_vertices - 1;
    bbox.min = bbox.max = v->pos;
    for (; i; i--) {
        v++;
        bbox.Absorb(v->pos);
    }
}

// FUNCTION: LS3DF 0x10030b40
LS3D_RESULT I3D_mesh_level::SetVertexMesh(const I3D_vertex *verts, int num)
{
    cVBMan.m_Free(vb);
    memset(&vb, 0, sizeof(vb));
    if (!verts) {
        num_vertices = 0;
        bbox.Invalidate();
        return I3D_OK;
    }
    if (!cVBMan.m_Alloc(I3D_VERTEX_FVF, D3DUSAGE_SOFTWAREPROCESSING, sizeof(I3D_vertex), num, vb, VF_VERTEX,
            false)) {
        num_vertices = 0;
        bbox.Invalidate();
        dbgPrintf("!! Out of memory    I3D_mesh_level::SetVertexMesh()");
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return I3DERR_OUTOFMEM;
    }
    memcpy(vb.buffer->Lock(vb.base, 0), verts, num * sizeof(I3D_vertex));
    num_vertices = num;
    UpdateBBox();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10030d00
I3D_vertex *I3D_mesh_level::CreateVertexMesh(int num)
{
    cVBMan.m_Free(vb);
    memset(&vb, 0, sizeof(vb));
    if (!num) {
        num_vertices = num;
        bbox.Invalidate();
        return NULL;
    }
    if (!cVBMan.m_Alloc(I3D_VERTEX_FVF, D3DUSAGE_SOFTWAREPROCESSING, sizeof(I3D_vertex), num, vb, VF_VERTEX,
            false)) {
        num_vertices = 0;
        bbox.Invalidate();
        dbgPrintf("!! Out of memory  I3D_mesh_level::CreateVertexMesh()");
        ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
        return NULL;
    }
    num_vertices = num;
    return (I3D_vertex *)vb.buffer->Lock(vb.base, 0);
}

// The original sets the read-only flag into a variable it never initialised (it lives in the dead this slot)
// FUNCTION: LS3DF 0x10030e80
I3D_vertex *I3D_mesh_level::LockVertices(uint flags)
{
    if (!vb.vb)
        return NULL;
    uint lock_flags;
    if (flags & 1)
        lock_flags |= D3DLOCK_READONLY;
    else
        lock_flags = 0;
    return (I3D_vertex *)vb.buffer->Lock(vb.base, lock_flags);
}

// FUNCTION: LS3DF 0x10030f90
void I3D_mesh_level::UnlockVertices()
{
    if (vb.vb)
        vb.buffer->Unlock();
}

// The face groups are deleted, but the chain is cleared only without a level: the original's (AddFGroup then appends
// to the deleted chain)
// FUNCTION: LS3DF 0x10031000
LS3D_RESULT I3D_mesh_level::Duplicate(I3D_mesh_level *level)
{
    if (level == this)
        return I3D_OK;
    delete fgroups;
    cVBMan.m_Free(vb);
    memset(&vb, 0, sizeof(vb));
    if (!level) {
        fgroups = NULL;
        num_vertices = 0;
        num_fgroups = 0;
        bbox.Invalidate();
        lod_dist = 0.0f;
        return I3D_OK;
    }
    mesh_flags = level->mesh_flags;
    env_scale = level->env_scale;
    lod_dist = level->lod_dist;
    LS3D_RESULT r = SetVertexMesh(level->LockVertices(0), level->num_vertices);
    if (r < 0)
        return r;
    for (I3D_face_group *fg = level->fgroups; fg; fg = fg->next) {
        I3D_face_group *fg1 = new I3D_face_group;
        r = fg1->Duplicate(fg);
        if (r < 0) {
            delete fg1;
            return r;
        }
        AddFGroup(fg1, true);
    }
    bbox = level->bbox;
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10031180
I3D_face_group *I3D_mesh_level::GetFGroup(int index)
{
    if ((uint)index >= num_fgroups) {
        dbgPrintf("!! Invalid index:%d > FaceGroupCount:%d )", index, num_fgroups);
        return NULL;
    }
    I3D_face_group *fg = fgroups;
    while (index-- > 0)
        fg = fg->next;
    return fg;
}

// FUNCTION: LS3DF 0x100311c0
int I3D_mesh_level::NumFaces() const
{
    int num = 0;
    for (I3D_face_group *fg = fgroups; fg; fg = fg->next)
        num += fg->num_faces;
    return num;
}

// FUNCTION: LS3DF 0x100311e0
LS3D_RESULT I3D_mesh_level::GetStats(I3D_mesh_stats &stats)
{
    stats.size = sizeof(I3D_mesh_level);
    stats.num_vertices = num_vertices;
    stats.vertex_bytes = num_vertices * sizeof(I3D_vertex);
    int faces = NumFaces();
    stats.num_faces = faces;
    stats.face_bytes = faces * sizeof(I3D_triface);
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10031220
void I3D_mesh_level::AddFGroup(I3D_face_group *fg, bool last)
{
    if (!fg)
        return;
    if (fg->material) {
        if (fg->material->flags & MATF_TEXTURE)
            mesh_flags |= MESHF_TEXTURE;
        if (fg->material->flags & MATF_KEEP_DIFFUSE)
            mesh_flags |= MESHF_KEEP_DIFFUSE;
        if (fg->material->flags & MATF_ENVMAP) {
            mesh_flags |= MESHF_ENVMAP;
            mesh_flags = (mesh_flags & ~MESHF_ENVMAP_MODE) | ((fg->material->flags >> 12) & MESHF_ENVMAP_MODE);
            env_scale = (float)(sqrt(2.0) * 2) / (fg->material->flags & 0xff);
        }
    }
    if (!fgroups) {
        fg->next = NULL;
        num_fgroups = 1;
        fgroups = fg;
        return;
    }
    num_fgroups++;
    if (fg->material && (fg->material->flags > 0x10000000 || fg->material->alpha < 1.0) || last) {
        I3D_face_group *p = fgroups;
        while (p->next)
            p = p->next;
        p->next = fg;
        fg->next = NULL;
    } else {
        fg->next = fgroups;
        fgroups = fg;
    }
}

// FUNCTION: LS3DF 0x10031320
I3D_mesh_object::I3D_mesh_object()
{
    num_lods = 0;
    ref = 1;
    memset(levels, 0, sizeof(levels));
    bvol.bbox.Invalidate();
    bvol.bsphere.pos = S_vector(0.0f, 0.0f, 0.0f);
    bvol.bsphere.radius = 0.0f;
    u_ofs = v_ofs = 0;
    env_u_ofs = env_v_ofs = 0;
}

// Release inlines it, and /OPT:REF drops its copy
I3D_mesh_object::~I3D_mesh_object()
{
    for (uint i = 0; i < num_lods; i++) {
        if (levels[i])
            levels[i]->Release();
    }
}

// FUNCTION: LS3DF 0x100313a0
int I3D_mesh_object::Release()
{
    if (--ref)
        return ref;
    delete this;
    return 0;
}

// FUNCTION: LS3DF 0x100313f0
I3D_mesh_level *I3D_mesh_object::GetLOD(int index) const
{
    return levels[index];
}

// FUNCTION: LS3DF 0x10031400
void I3D_mesh_object::UpdateBoundVolume()
{
    bvol.bbox.Invalidate();
    for (int i = num_lods - 1; i >= 0; i--) {
        I3D_mesh_level *level = levels[i];
        if (level && level->bbox.IsValid()) {
            if (bvol.bbox.min.x > level->bbox.min.x)
                bvol.bbox.min.x = level->bbox.min.x;
            if (bvol.bbox.max.x < level->bbox.max.x)
                bvol.bbox.max.x = level->bbox.max.x;
            if (bvol.bbox.min.y > level->bbox.min.y)
                bvol.bbox.min.y = level->bbox.min.y;
            if (bvol.bbox.max.y < level->bbox.max.y)
                bvol.bbox.max.y = level->bbox.max.y;
            if (bvol.bbox.min.z > level->bbox.min.z)
                bvol.bbox.min.z = level->bbox.min.z;
            if (bvol.bbox.max.z < level->bbox.max.z)
                bvol.bbox.max.z = level->bbox.max.z;
        }
    }
    S_vector half = (bvol.bbox.max - bvol.bbox.min) * 0.5f;
    bvol.bsphere.pos = bvol.bbox.min + half;
    bvol.bsphere.radius = (float)sqrt(half.Square());
}

// FUNCTION: LS3DF 0x10031550
LS3D_RESULT I3D_mesh_object::SetLOD(I3D_mesh_level *level, int index)
{
    if (index >= 10) {
        dbgPrintf("!!  Invalid index %d  I3D_mesh_object::SetLOD()", index);
        return I3DERR_INVALIDPARAMS;
    }
    if (levels[index] != level) {
        if (levels[index])
            levels[index]->Release();
        levels[index] = level;
        if (level) {
            level->ref++;
            if (num_lods <= (uint)index)
                num_lods = index + 1;
        } else if (num_lods > (uint)index)
            num_lods = index;
    } else if (!level)
        return I3D_OK;
    UpdateBoundVolume();
    return I3D_OK;
}

// FUNCTION: LS3DF 0x100315e0
LS3D_RESULT I3D_mesh_object::Duplicate(I3D_mesh_object *mesh)
{
    if (mesh == this)
        return I3D_OK;
    int i;
    for (i = 0; i < 10; i++) {
        if (levels[i])
            levels[i]->Release();
    }
    num_lods = 0;
    memset(levels, 0, sizeof(levels));
    if (!mesh) {
        bvol.bbox.Invalidate();
        bvol.bsphere.pos = S_vector(0.0f, 0.0f, 0.0f);
        bvol.bsphere.radius = 0.0f;
        u_ofs = v_ofs = 0;
        return I3D_OK;
    }
    for (i = 0; i < 10; i++) {
        if (mesh->levels[i]) {
            I3D_mesh_level *level = pI3DDriver->CreateMeshLevel();
            if (!level) {
                dbgPrintf("!! Out of memory  I3D_mesh_object::Duplicate()");
                ErrorHandler(I3DERR_OUTOFMEM, ERR_FATAL, NULL);
                return I3DERR_OUTOFMEM;
            }
            LS3D_RESULT r = level->Duplicate(mesh->levels[i]);
            if (r < 0)
                return r;
            SetLOD(level, i);
            level->Release();
        }
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10031700
LS3D_RESULT I3D_mesh_object::DetachMaterials()
{
    for (int i = 0; i < 10; i++) {
        if (levels[i]) {
            for (I3D_face_group *fg = levels[i]->fgroups; fg; fg = fg->next) {
                LS3D_RESULT r = fg->DetachMaterial();
                if (r < 0)
                    return r;
            }
        }
    }
    return I3D_OK;
}

// FUNCTION: LS3DF 0x10031750
bool I3D_mesh_object::IsCompatible(I3D_mesh_object *mesh)
{
    if (num_lods != mesh->num_lods)
        return false;
    for (int i = 0; i < 10; i++) {
        if (!levels[i]) {
            if (mesh->levels[i])
                return false;
        } else if (!mesh->levels[i] || levels[i]->num_vertices != mesh->levels[i]->num_vertices)
            return false;
    }
    return true;
}

// FUNCTION: LS3DF 0x100317a0
LS3D_RESULT I3D_mesh_object::GetStats(I3D_mesh_stats &stats)
{
    stats.size = sizeof(I3D_mesh_object);
    uint verts = 0;
    uint faces = 0;
    for (uint i = 0; i < num_lods; i++) {
        verts += levels[i]->num_vertices;
        faces += levels[i]->NumFaces();
    }
    stats.num_vertices = verts;
    stats.vertex_bytes = verts * sizeof(I3D_vertex);
    stats.num_faces = faces;
    stats.face_bytes = faces * sizeof(I3D_triface);
    return I3D_OK;
}
