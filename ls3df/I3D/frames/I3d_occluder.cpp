// The occluders (PS2 I3d_occluder.cpp), at 0x1003a430-0x1003cbb0 in the original: I3D_occluder in PS2's order from its
// constructor, then the occlusion: UOL_EnumProc and OCCL_UpdateOcclLists, the header's zero vector's $E (0x1003c050,
// where ours comes too), ProcessOccluders with the static CullOcclsEachOther and POIS_EnumProc after it,
// OCCL_PrepareSector2 and LoadImage, then XSBTree's out-of-line functions. Names are PS2's unless noted.
// - TestVertListW and ProcessOccludersInSector have no copy of their own on PC: VC6 inlines them everywhere. The second
//   is declared inline: an out-of-line copy before ProcessOccluders would bring the statics it calls up to it.
//
// Not matching yet:
// - UpdateV: the inline budget for ComputePlane's Dot, see the function.
// - UpdateShadow, OCCL_PrepareSector2: the order of a few products' operands and of the terms of their sums, see the
//   functions.
// - CullOcclsEachOther (and its copy inlined in ProcessOccluders): VC6 keeps a different one of its two floats on the
//   x87 stack (see the function).
// - POIS_EnumProc: a jump target, see the function.
#include "I3D/I3D.h"
#include "I3D/frames/XSBTree.h"
#include "dta.h"

#include <math.h>
#include <string.h>

// <new> declares operator delete throw(), so VC6 sets no EH state before the tree's destructor in Build
#include <new>

// An edge of a face, for XSBTree (PS2 type, 8 bytes; the member names are guesses)
struct TRISIDE2 {
    uint key;                           // the edge's vertex indices, the smaller in the high word
    ushort face;
    ushort side;                        // which of the face's edges: 0 to 2
};

// OCCL_UpdateOcclLists' sector and the last occluder of its list (PS2 names, static there too), and POIS_EnumProc's
// sector (PC only; the name is a guess, chosen for the original's .bss order, see tools/bsshash.py)
// GLOBAL: LS3DF 0x101b43bc
// GLOBAL: LS3DF11 0x101b14dc
static I3D_sector *PO_pSector;

// GLOBAL: LS3DF 0x101b43c0
// GLOBAL: LS3DF11 0x101b14e0
static I3D_sector *pPOISector;

// Nothing references it (PC only; the name is a guess, chosen for its place in .bss). An unreferenced static would get
// no space: VC6 leaves it out.
// GLOBAL: LS3DF 0x101b43c4
// GLOBAL: LS3DF11 0x101b14e4
int OccludedVisuals;

// GLOBAL: LS3DF 0x101b43c8
// GLOBAL: LS3DF11 0x101b14e8
static I3D_occluder *pLastOccl;

// The flags after the pointers, as PS2 has them: set first, VC6 changes them in memory, where the original loads and
// stores them
// FUNCTION: LS3DF 0x1003a430
// FUNCTION: LS3DF11 0x10038970
I3D_occluder::I3D_occluder()
{
    num_side_planes = 0;
    num_face_planes = 0;
    occl_flags = 0;
    next_in_scene = NULL;
    prev_in_scene = NULL;
    num_verts = 0;
    num_faces = 0;
    next_in_sector = NULL;
    sector = NULL;
    face_flags = NULL;
    verts_w = NULL;
    verts = NULL;
    faces = NULL;
    edge_sides = NULL;
    edge_faces = NULL;
    planes = NULL;
    scene = NULL;
    m_FrmFlags |= FRMFLAGS_HR_OCCLUDERS;
    m_FrmFlags |= FRMFLAGS_UNKNOWN_8000000;
    m_Type = FRAME_OCCLUDER;
    center = ZeroSVector;
    radius = 0.0f;
    radius_2 = 0.0f;
    dist = 0.0f;
}

// FUNCTION: LS3DF 0x1003a520
// FUNCTION: LS3DF11 0x10038a60
I3D_occluder::~I3D_occluder()
{
    if (face_flags)
        delete[] face_flags;
    if (verts_w)
        delete[] verts_w;
    if (verts)
        delete[] verts;
    if (faces)
        delete[] faces;
    if (edge_sides)
        delete[] edge_sides;
    if (edge_faces)
        delete[] edge_faces;
    if (planes)
        delete[] planes;
    if (sector) {
        sector->occluder = NULL;
        sector = NULL;
    }
    AddToRemoveFromSceneRing(NULL);
    if (radius == 0.0f)
        dbgPrintf("!! Destructed occluder was not Builded!!! (this:0x%x  %s)", this, GetName());
}

// Moves the occluder from its scene's ring of occluders into the other scene's
// FUNCTION: LS3DF 0x1003a650
// FUNCTION: LS3DF11 0x10038b90
void I3D_occluder::AddToRemoveFromSceneRing(I3D_scene *new_scene)
{
    if (scene && prev_in_scene) {
        if (scene->occluders == this)
            scene->occluders = next_in_scene;
        if (prev_in_scene == this)
            scene->occluders = NULL;
        else {
            prev_in_scene->next_in_scene = next_in_scene;
            next_in_scene->prev_in_scene = prev_in_scene;
        }
        scene->num_occluders--;
        scene->occluders_changed = true;
        scene = NULL;
    }
    if (!new_scene)
        return;
    scene = new_scene;
    if (!scene->occluders) {
        prev_in_scene = this;
        next_in_scene = this;
        scene->occluders = this;
    } else {
        scene->occluders->prev_in_scene->next_in_scene = this;
        prev_in_scene = scene->occluders->prev_in_scene;
        next_in_scene = scene->occluders;
        scene->occluders->prev_in_scene = this;
    }
    scene->num_occluders++;
    scene->unknown_274 = true;
    scene->occluders_changed = true;
}

// The occluder goes into the scene of its new parent: the scene that owns the parent, or the one of the sector the
// parent is in
// FUNCTION: LS3DF 0x1003a770
// FUNCTION: LS3DF11 0x10038cb0
LS3D_RESULT I3D_occluder::LinkTo(I3D_frame *frame, uint flags)
{
    if (frame == m_pParent)
        return I3D_OK;
    AddToRemoveFromSceneRing(NULL);
    if (frame) {
        I3D_scene *scn;
        if (frame->m_pOwner && frame->m_pOwner->GetType() == FRAME_SCENE)
            scn = (I3D_scene *)frame->m_pOwner;
        else {
            scn = NULL;
            for (I3D_frame *f = frame; f; f = f->m_pParent) {
                if (f->GetType() == FRAME_SECTOR) {
                    scn = ((I3D_sector *)f)->scene;
                    break;
                }
            }
        }
        if (scn)
            AddToRemoveFromSceneRing(scn);
    }
    return I3D_frame::LinkTo(frame, flags);
}

// The vertices in world space, the faces' planes through S_plane's constructor (I3d_math.h), as the area's UpdateV, and
// the sphere around the vertices' box.
// - The early return tests the member and the count is read after it: tested through n, VC6 knows the count isn't 0
//   and drops the transform loop's guard, which the original has (it reads the count again from n's slot after
//   GetMatrix's call and counts with a copy).
// - The matrix through a reference local, which takes ebx in the transform loop as the original's &matrix does
//   (UpdateWMatrix() and matrix give the count ebx and compute the address in each pass).
// - The three vertices through references declared from the last on, and the plane a temporary: the order VC6 then
//   sums the normal's squares (x, y, z) and Dot's products (y, z, x) in. A named plane, the vertices in the call or
//   the references declared from the first on sum them in other orders.
// - The box loop's count assigned before its pointer: the original computes it first, in edx.
// Open: the inline budget. The original inlines all of ComputePlane but GetNormal's two operator- calls; ours also
// calls the Dot of its d. 3 to 19 dead statements (if (0) radius = radius * radius_2 + dist;) at the top give the
// original's code exactly, and so does ComputePlane declared __forceinline (see I3d_math.h). No spelling of the
// radius, the box loop, the transform loop or the plane (ComputePlane called on a named plane inlines the Dot but sums
// the normal's squares in another order) adds that much.
// FUNCTION: LS3DF 0x1003a800
// FUNCTION: LS3DF11 0x10038d40
void I3D_occluder::UpdateV()
{
    if (!num_verts)
        return;
    int n = num_verts;
    const S_vector *v = verts;
    S_vector *vw = verts_w;
    const S_matrix &m = GetMatrix();
    int i;
    for (i = n; i--; )
        *vw++ = *v++ * m;
    const ushort *fc = faces;
    S_plane *p = planes;
    for (i = num_faces; i--; ) {
        const S_vector &v2 = verts_w[fc[2]], &v1 = verts_w[fc[1]], &v0 = verts_w[fc[0]];
        *p++ = S_plane(v0, v1, v2);
        fc += 3;
    }
    S_vector vmin = verts_w[0];
    S_vector vmax = verts_w[0];
    for (i = num_verts - 1, v = verts_w + 1; i--; v++) {
        if (vmin.x > v->x)
            vmin.x = v->x;
        if (vmin.y > v->y)
            vmin.y = v->y;
        if (vmin.z > v->z)
            vmin.z = v->z;
        if (vmax.x < v->x)
            vmax.x = v->x;
        if (vmax.y < v->y)
            vmax.y = v->y;
        if (vmax.z < v->z)
            vmax.z = v->z;
    }
    center = (vmin + vmax) * .5f;
    S_vector half = (vmax - vmin) * .5f;
    radius = (float)sqrt(half.x * half.x + half.y * half.y + half.z * half.z);
    radius_2 = radius * radius;
}

// The other occluder's faces are built into this one
// FUNCTION: LS3DF 0x1003ac00
// FUNCTION: LS3DF11 0x10039140
LS3D_RESULT I3D_occluder::Duplicate(I3D_frame *frame)
{
    if (!frame)
        return I3DERR_INVALIDPARAMS;
    LS3D_RESULT result = I3D_frame::Duplicate(frame);
    if (frame->GetType() != FRAME_OCCLUDER)
        return result;
    I3D_occluder *occl = (I3D_occluder *)frame;
    Build(occl->verts, occl->num_verts, occl->faces, occl->num_faces, sizeof(S_vector));
    return result;
}

// FUNCTION: LS3DF 0x1003ac60
// FUNCTION: LS3DF11 0x100391a0
LS3D_RESULT I3D_occluder::Build(const S_vector *vp, uint nv, const ushort *fp, uint nf, uint stride)
{
    if (verts)
        delete[] verts;
    if (verts_w)
        delete[] verts_w;
    if (faces)
        delete[] faces;
    verts = new S_vector[nv];
    verts_w = new S_vector[nv];
    faces = new ushort[nf * 3];
    if (!verts || !verts_w || !faces) {
        dbgPrintf("!! Out of memory !!    i3d_occluder::Build [1]");
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
    return Build();
}

// The faces across the edges: each edge goes into the tree under its two vertex indices, so the two faces that share
// it end in one ring. The count of the nodes comes through the tree's NumNodes: an inline call after the faces' loop
// leaves the third Add's Add_r out of line, as the original's. The node's fields go into locals in the order PS2 reads
// them, which puts VC6's walking pointer on next.
// FUNCTION: LS3DF 0x1003ad90
// FUNCTION: LS3DF11 0x100392d0
LS3D_RESULT I3D_occluder::Build()
{
    XSBTree<TRISIDE2> tree(num_faces * 3);
    edge_faces = new ushort[num_faces * 3];
    edge_sides = new uchar[num_faces * 3];
    planes = new S_plane[num_faces];
    face_flags = new ushort[num_faces];
    if (!tree.ok || !edge_faces || !edge_sides || !planes || !face_flags) {
        dbgPrintf("!! Out of memory !!    i3d_occluder::Build [3]");
        return I3DERR_OUTOFMEM;
    }
    const ushort *fc = faces;
    uint i;
    for (i = 0; i < num_faces; i++) {
        TRISIDE2 ts;
        ts.face = i;
        ts.side = 0;
        ts.key = fc[0] < fc[1] ? (fc[0] << 16) | fc[1] : (fc[1] << 16) | fc[0];
        tree.Add(ts);
        ts.side = 1;
        ts.key = fc[1] < fc[2] ? (fc[1] << 16) | fc[2] : (fc[2] << 16) | fc[1];
        tree.Add(ts);
        ts.side = 2;
        ts.key = fc[2] < fc[0] ? (fc[2] << 16) | fc[0] : (fc[0] << 16) | fc[2];
        tree.Add(ts);
        fc += 3;
    }
    XSBTree<TRISIDE2>::_NODE *nd = tree.root;
    for (int j = tree.NumNodes(); j--; nd++) {
        ushort f = nd->data.face;
        XSBTree<TRISIDE2>::_NODE *nx = nd->next;
        ushort s = nd->data.side;
        ushort ns = nx->data.side;
        edge_faces[f * 3 + s] = nx->data.face;
        edge_sides[f * 3 + s] = (uchar)ns;
    }
    ushort *ff = face_flags;
    for (i = num_faces; i--; )
        *ff++ = 0;
    radius = 1.0f;
    return I3D_OK;
}

// The volume the occluder hides for the current camera: the planes of the faces turned to it, the planes through the
// camera and each silhouette edge (an edge between a face turned to it and one turned away), and depth_plane. Gives
// up when there would be more planes than the arrays hold. The planes come through S_plane's constructors; the three
// edges are written out, as PS2's, and the third's first operator- is inlined in the original as in ours.
// - The face flags' pointer steps in both branches, as PS2's: stepped after the if/else, VC6 swaps the registers of
//   the plane and the flags.
// - depth_plane is assigned and read through a reference and changed directly: the original keeps its address in edi
//   for the loop's depth tests and copies the temporary plane through a register.
// - One vector for the three edges, at the top of the function: the original keeps the three in one stack area (and
//   ours otherwise has a frame 24 bytes larger).
// Open: the order of some products' operands and of some sums' terms: the first loop's and depth_plane's Dot (the
// original loads the plane's component first, ours the camera's), the view direction's squares (y, z, x in the
// original), and the planes' squares and Dot. The orders follow VC6's internal numbering; the spellings tried (Dot's
// object and argument swapped, the camera's position through a reference, the plane through a pointer, the vertices
// through references in any order, the direction constructed or assigned) don't move them, except that the camera's
// position read into a reference before the plane gives the first loop's order but steps the plane pointer after the
// call.
// FUNCTION: LS3DF 0x1003b130
// FUNCTION: LS3DF11 0x10039670
void I3D_occluder::UpdateShadow()
{
    occl_flags &= ~(OCCLF_ACTIVE | OCCLF_PORTAL);
    if (!num_verts)
        return;
    if (!scene) {
        I3D_scene *scn = NULL;
        for (I3D_frame *f = m_pParent; f; f = f->m_pParent) {
            if (f->GetType() == FRAME_SECTOR) {
                scn = ((I3D_sector *)f)->scene;
                break;
            }
        }
        if (scn)
            AddToRemoveFromSceneRing(scn);
        else
            dbgPrintf("!! Occluder not in scene  i3d_occluder::UpdateShadow() this:0x%x  %s", this, GetName());
        return;
    }
    if (!(m_FrmFlags & FRMFLAGS_UNKNOWN_10000) || !(m_FrmFlags & FRMFLAGS_ON))
        return;
    if (m_pParent && !(m_pParent->m_FrmFlags & FRMFLAGS_ON))
        return;

    S_vector v;
    const S_plane *pl = planes;
    ushort *ff = face_flags;
    for (int i = num_faces; i--; ) {
        const S_plane &p = *pl++;
        if (p.normal.Dot(pCurrentCamera->GetWorldPos()) + p.d > 0.0f)
            *ff++ |= 1;
        else
            *ff++ &= ~1;
    }

    ushort *ef = edge_faces;
    num_side_planes = 0;
    num_face_planes = 0;
    int n = num_faces;
    S_plane *sp = side_planes;
    S_plane *fp = face_planes;
    const S_vector &cpos = pCurrentCamera->GetWorldPos();
    int j = 0;
    S_vector dir = pCurrentCamera->GetMatrix()(2);
    dir.Normalize();
    S_plane &dp = depth_plane;
    dp = S_plane(dir, 0.0f);
    depth_plane.d = -(pCurrentCamera->GetWorldPos().Dot(dp.normal) + dp.d);

    while (n--) {
        if (face_flags[j] & 1) {
            int k = ef - edge_faces;
            *fp++ = S_plane(verts_w[faces[k + 1]], verts_w[faces[k]], verts_w[faces[k + 2]]);
            num_face_planes++;
            if (!(face_flags[*ef] & 1)) {
                k = ef - edge_faces;
                v = verts_w[faces[k + 1]];
                *sp++ = S_plane(cpos, v, verts_w[faces[k]]);
                num_side_planes++;
                float d = v.Dot(dp.normal) + dp.d;
                if (d > 0.0f)
                    depth_plane.d -= d;
            }
            ef++;
            if (!(face_flags[*ef] & 1)) {
                k = ef - edge_faces;
                v = verts_w[faces[k + 1]];
                *sp++ = S_plane(cpos, v, verts_w[faces[k]]);
                num_side_planes++;
                float d = v.Dot(dp.normal) + dp.d;
                if (d > 0.0f)
                    depth_plane.d -= d;
            }
            ef++;
            if (!(face_flags[*ef] & 1)) {
                k = ef - edge_faces;
                v = verts_w[faces[k - 2]];
                *sp++ = S_plane(cpos, v, verts_w[faces[k]]);
                num_side_planes++;
                float d = v.Dot(dp.normal) + dp.d;
                if (d > 0.0f)
                    depth_plane.d -= d;
            }
            ef++;
        } else
            ef += 3;
        j++;
        if (num_side_planes > 32 || num_face_planes > 32)
            return;
    }
    if (num_side_planes)
        occl_flags |= OCCLF_ACTIVE;
}

// Whether the volume hides all the points (world space). depth_plane's normal is Dot's object, the points' the side
// planes' argument: the order VC6 then gives the products in the first copy of PrepareSector2's.
inline bool I3D_occluder::TestVertListW(int num, S_vector *v)
{
    for (; num--; v++) {
        if (depth_plane.normal.Dot(*v) + depth_plane.d < .01)
            return false;
        const S_plane *p = side_planes;
        for (int i = num_side_planes; i--; p++) {
            if (v->Dot(p->normal) + p->d < 0.0f)
                return false;
        }
    }
    return true;
}

// Each nearest occluder takes the volumes behind it out of the work, so that the others test fewer. The plane tests
// are written out, as PS2's (no Dot).
// Open: VC6 keeps last on the x87 stack in the original (loaded before the prologue) and best in memory, cached on the
// stack only in the first loop; ours keeps best on the stack for the whole loop and last in memory. No spelling of the
// loops (do-while, last assigned in a for's step or at the end), the declarations, the assignment of last, the second
// loop's compare (best or last) or best's address taken changed that; last as a double goes on the stack but best is
// then never cached.
static void CullOcclsEachOther(I3D_sector *sct)
{
    float last = 0.0f;
    while (true) {
        I3D_occluder *first = sct->occluders;
        float best = 1e12f;
        I3D_occluder *nearest = NULL;
        I3D_occluder *o;
        for (o = first; o; o = o->next_in_sector) {
            if (o->occl_flags & OCCLF_ACTIVE) {
                if (o->dist > last && o->dist < best) {
                    best = o->dist;
                    nearest = o;
                }
            }
        }
        last = best;
        if (!nearest)
            return;
        for (o = first; o; o = o->next_in_sector) {
            if ((o->occl_flags & OCCLF_ACTIVE) && o->dist > best) {
                S_vector pos = o->center;
                bool vis = false;
                const S_plane *p;
                int i;
                const S_plane &dp = nearest->depth_plane;
                if (!(dp.d + dp.normal.x * pos.x + dp.normal.y * pos.y + dp.normal.z * pos.z > o->radius)) {
                    p = nearest->face_planes;
                    for (i = nearest->num_face_planes; i--; p++) {
                        if (p->d + p->normal.x * pos.x + p->normal.y * pos.y + p->normal.z * pos.z < o->radius) {
                            vis = true;
                            break;
                        }
                    }
                }
                if (!vis) {
                    p = nearest->side_planes;
                    for (i = nearest->num_side_planes; i--; p++) {
                        if (p->d + p->normal.x * pos.x + p->normal.y * pos.y + p->normal.z * pos.z < o->radius) {
                            vis = true;
                            break;
                        }
                    }
                }
                if (!vis)
                    o->occl_flags &= ~OCCLF_ACTIVE;
            }
        }
    }
}

// The visuals to render from frm on, along their chain, that an occluder of pPOISector hides aren't rendered: their
// render number goes back. A sphere is visible past an occluder when it reaches in front of a face plane (if it
// reaches past depth_plane) or of a side plane. The tests are written out, as PS2's (no Dot), and the render number to
// go back to is read before the test, as the original keeps it in a register.
// Open: the original sets vis again on a face plane's hit, where ours jumps on with it set. A face hit that jumps into
// the side planes' hit (goto) gives the original's code exactly, but costs less: ProcessOccluders then inlines this
// function in its first sector loop, which one more statement's worth of source (dead code) here prevents.
static void POIS_EnumProc(I3D_frame *frm)
{
    if (!pPOISector->occluders)
        return;
    for (I3D_frame *f = frm; f; f = f->m_pNextVisual) {
        int rn = RenderNumber - 1;
        if (f->m_RenderNumber != RenderNumber)
            continue;
        bool vis = true;
        for (I3D_occluder *o = pPOISector->occluders; o; o = o->next_in_sector) {
            if (!vis)
                break;
            if ((o->occl_flags & OCCLF_ACTIVE) == OCCLF_ACTIVE) {
                const I3D_bvolume &bv = f->GetHRBoundVolumeW();
                S_vector pos = bv.bsphere.pos;
                float r = bv.bsphere.radius;
                const S_plane *p;
                int i;
                const S_plane &dp = o->depth_plane;
                bool face_hit = false;
                if (!(dp.d + dp.normal.x * pos.x + dp.normal.y * pos.y + dp.normal.z * pos.z > r)) {
                    p = o->face_planes;
                    for (i = o->num_face_planes; i--; p++) {
                        if (p->d + p->normal.x * pos.x + p->normal.y * pos.y + p->normal.z * pos.z < r) {
                            face_hit = true;
                            break;
                        }
                    }
                }
                if (!face_hit) {
                    vis = false;
                    p = o->side_planes;
                    for (i = o->num_side_planes; i--; p++) {
                        if (p->d + p->normal.x * pos.x + p->normal.y * pos.y + p->normal.z * pos.z < r) {
                            vis = true;
                            break;
                        }
                    }
                }
            }
        }
        if (!vis)
            f->m_RenderNumber = rn;
    }
}

// The occluders made of a hull the sector's flag turns off, then the rest hide the sector's visuals
inline void ProcessOccludersInSector(I3D_sector *sct)
{
    for (I3D_occluder *o = sct->occluders; o; o = o->next_in_sector) {
        if (o->sector && (o->sector->sct_flags & SCTF_UNKNOWN_80))
            o->occl_flags &= ~OCCLF_ACTIVE;
    }
    CullOcclsEachOther(sct);
    pPOISector = sct;
    POIS_EnumProc(sct);
}

// Adds the occluders below frm to PO_pSector's list, without going into cameras and other sectors
// FUNCTION: LS3DF 0x1003bec0
// FUNCTION: LS3DF11 0x1003a400
void UOL_EnumProc(I3D_frame *frm)
{
    for (I3D_frame *f = frm->m_pChild; f; f = f->m_pNext) {
        if (f->m_Type == FRAME_CAMERA || f->m_Type == FRAME_SECTOR)
            continue;
        if (f->m_Type == FRAME_OCCLUDER) {
            if (!PO_pSector->occluders)
                PO_pSector->occluders = (I3D_occluder *)f;
            else
                pLastOccl->next_in_sector = (I3D_occluder *)f;
            pLastOccl = (I3D_occluder *)f;
            pLastOccl->next_in_sector = NULL;
        }
        UOL_EnumProc(f);
    }
}

// FUNCTION: LS3DF 0x1003bf30
// FUNCTION: LS3DF11 0x1003a470
void OCCL_UpdateOcclLists(I3D_scene *scene)
{
    if (!scene->occluders_changed)
        return;
    I3D_sector *sct = scene->sectors;
    for (int i = scene->num_sectors; i--; sct = sct->next_in_scene) {
        PO_pSector = sct;
        sct->occluders = NULL;
        UOL_EnumProc(sct);
    }
    sct = scene->primary_sector;
    PO_pSector = sct;
    sct->occluders = NULL;
    UOL_EnumProc(sct);
    scene->occluders_changed = false;
}

// The rendering's: the occluders hide the visuals of the sectors on (RS_USE_OCCLUSION)
// FUNCTION: LS3DF 0x1003c070
// FUNCTION: LS3DF11 0x1003a5b0
void ProcessOccluders(I3D_scene *scene)
{
    if (!renderState[RS_USE_OCCLUSION] || !scene)
        return;
    I3D_sector *sct = scene->sectors;
    for (int i = scene->num_sectors; i--; sct = sct->next_in_scene) {
        if ((sct->sct_flags & SCTF_VISIBLE) && sct->occluders)
            ProcessOccludersInSector(sct);
    }
    sct = scene->primary_sector;
    if (sct->sct_flags & SCTF_VISIBLE)
        ProcessOccludersInSector(sct);
}

// The volumes of the sector's occluders for the camera, and which of the sector's portals they hide. An occluder made
// of a hull whose portals the camera sees from both sides hides no portal. The portals' planes take the camera's
// position as Dot's argument, the first loop's through a reference. The loop over the portals into the sector has a
// counter and an occluder variable of its own: with those of the loop before, TestVertListW's second copy sums its
// depth test's products in another order.
// Open: two products' operands come in the other order, the x product of the first copy's depth test and the last side
// plane product of the second copy's. Of the variable choices of the four loops, the Dot spellings of the portal tests
// and TestVertListW's (1024 combinations), none gives both copies; each best one leaves two products swapped.
// FUNCTION: LS3DF 0x1003c610
// FUNCTION: LS3DF11 0x1003ab50
void OCCL_PrepareSector2(I3D_sector *sct)
{
    I3D_occluder *o;
    for (o = sct->occluders; o; o = o->next_in_sector)
        o->UpdateShadow();
    S_vector cpos = pCurrentCamera->GetWorldPos();
    for (o = sct->occluders; o; o = o->next_in_sector) {
        I3D_sector *s = o->sector;
        if (!s)
            continue;
        bool front = false, back = false;
        I3D_portal **pp = &s->portals[0];
        for (int i = s->portals.size(); i--; pp++) {
            I3D_portal *p = *pp;
            const S_plane &pl = p->plane;
            if (pl.normal.Dot(cpos) + pl.d < 0.0f)
                back = true;
            else
                front = true;
        }
        if (front && back)
            o->occl_flags |= OCCLF_PORTAL;
    }
    I3D_portal **pp = &sct->portals[0];
    for (int i = sct->portals.size(); i--; pp++) {
        I3D_portal *p = *pp;
        if (p->plane.normal.Dot(cpos) + p->plane.d > 0.0f) {
            p->portal_flags &= ~PORTALF_OCCLUDED;
            for (o = sct->occluders; o; o = o->next_in_sector) {
                if ((o->occl_flags & OCCLF_ACTIVE) && !(o->occl_flags & OCCLF_PORTAL) &&
                    o->TestVertListW(p->num_verts, (S_vector *)p->verts_w)) {
                    p->portal_flags |= PORTALF_OCCLUDED;
                    break;
                }
            }
        }
    }
    pp = &sct->portals_in[0];
    for (int j = sct->portals_in.size(); j--; pp++) {
        I3D_portal *p = *pp;
        if (p->plane.normal.Dot(cpos) + p->plane.d < 0.0f) {
            p->portal_flags &= ~PORTALF_OCCLUDED;
            if (!p->twin) {
                for (I3D_occluder *oi = sct->occluders; oi; oi = oi->next_in_sector) {
                    if ((oi->occl_flags & OCCLF_ACTIVE) && !(oi->occl_flags & OCCLF_PORTAL) &&
                        oi->TestVertListW(p->num_verts, (S_vector *)p->verts_w)) {
                        p->portal_flags |= PORTALF_OCCLUDED;
                        break;
                    }
                }
            }
        }
    }
}

// The counts, the vertices and the faces
// FUNCTION: LS3DF 0x1003c9b0
// FUNCTION: LS3DF11 0x1003aef0
void I3D_occluder::LoadImage()
{
    I3D_frame::LoadImage();
    dtaRead(LoadImageHandle, &num_verts, 8);
    verts = new S_vector[num_verts];
    verts_w = new S_vector[num_verts];
    faces = new ushort[num_faces * 3];
    if (!verts || !faces || !verts_w) {
        delete[] verts;
        verts = NULL;
        num_verts = 0;
        delete[] verts_w;
        verts_w = NULL;
        delete[] faces;
        faces = NULL;
        num_faces = 0;
        dbgPrintf("!! Out of memory  I3D_occluder::LoadImage() %s  %s", GetName(), ImageFileName);
        ImageErrFlg = I3DERR_OUTOFMEM;
        return;
    }
    dtaRead(LoadImageHandle, verts, num_verts * sizeof(S_vector));
    dtaRead(LoadImageHandle, faces, num_faces * 3 * sizeof(ushort));
    LS3D_RESULT r = Build();
    if (r)
        ImageErrFlg = r;
}
