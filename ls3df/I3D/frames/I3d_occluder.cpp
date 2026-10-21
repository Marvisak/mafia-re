// The occluders (PS2 I3d_occluder.cpp), at 0x1003a430-0x1003cbb0 in the original: I3D_occluder in PS2's order from its
// constructor, then the occlusion: UOL_EnumProc and OCCL_UpdateOcclLists, the header's zero vector's $E (0x1003c050,
// where ours comes too), ProcessOccluders with the static CullOcclsEachOther and POIS_EnumProc after it,
// OCCL_PrepareSector2 and LoadImage, then XSBTree's out-of-line functions. Names are PS2's unless noted.
// - TestVertListW and ProcessOccludersInSector have no copy of their own on PC: VC6 inlines them everywhere. The second
//   is declared inline: an out-of-line copy before ProcessOccluders would bring the statics it calls up to it.
//
// Not matching yet:
// - UpdateV and UpdateShadow: the original inlines S_vector::Normalize in the planes' S_plane constructor and in
//   UpdateShadow's view direction, where ours runs out of inline budget (the open question in I3d_math.h). With
//   Normalize __forceinline, UpdateShadow calls exactly what the original does.
// - CullOcclsEachOther (and its copy inlined in ProcessOccluders): VC6 keeps a different one of its two floats on the
//   x87 stack (see the function).
// - POIS_EnumProc and OCCL_PrepareSector2: a jump target and a term order, see the functions.
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
static I3D_sector *PO_pSector;

// GLOBAL: LS3DF 0x101b43c0
static I3D_sector *pPOISector;

// Nothing references it (PC only; the name is a guess, chosen for its place in .bss). An unreferenced static would get
// no space: VC6 leaves it out.
// GLOBAL: LS3DF 0x101b43c4
int OccludedVisuals;

// GLOBAL: LS3DF 0x101b43c8
static I3D_occluder *pLastOccl;

// The flags after the pointers, as PS2 has them: set first, VC6 changes them in memory, where the original loads and
// stores them
// FUNCTION: LS3DF 0x1003a430
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
    frm_flags |= FRMFLAGS_HR_OCCLUDERS;
    frm_flags |= FRMFLAGS_UNKNOWN_8000000;
    type = FRAME_OCCLUDER;
    center = ZeroSVector;
    radius = 0.0f;
    radius_2 = 0.0f;
    dist = 0.0f;
}

// FUNCTION: LS3DF 0x1003a520
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
LS3D_RESULT I3D_occluder::LinkTo(I3D_frame *frame, uint flags)
{
    if (frame == parent)
        return I3D_OK;
    AddToRemoveFromSceneRing(NULL);
    if (frame) {
        I3D_scene *scn;
        if (frame->owner && frame->owner->GetType() == FRAME_SCENE)
            scn = (I3D_scene *)frame->owner;
        else {
            scn = NULL;
            for (I3D_frame *f = frame; f; f = f->parent) {
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
// Open: besides Normalize (see the top), the original keeps the vertex count in a stack slot of its own and counts the
// transform loop with a copy; ours counts with the count itself.
// FUNCTION: LS3DF 0x1003a800
void I3D_occluder::UpdateV()
{
    int n = num_verts;
    if (!n)
        return;
    const S_vector *v = verts;
    S_vector *vw = verts_w;
    UpdateWMatrix();
    for (int i = n; i--; )
        *vw++ = *v++ * matrix;
    const ushort *fc = faces;
    S_plane *p = planes;
    for (i = num_faces; i--; ) {
        S_plane pl(verts_w[fc[0]], verts_w[fc[1]], verts_w[fc[2]]);
        fc += 3;
        *p++ = pl;
    }
    S_vector vmin = verts_w[0];
    S_vector vmax = verts_w[0];
    v = verts_w + 1;
    for (i = num_verts - 1; i--; v++) {
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
    for (uint i = 0; i < num_faces; i++) {
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
// FUNCTION: LS3DF 0x1003b130
void I3D_occluder::UpdateShadow()
{
    occl_flags &= ~(OCCLF_ACTIVE | OCCLF_PORTAL);
    if (!num_verts)
        return;
    if (!scene) {
        I3D_scene *scn = NULL;
        for (I3D_frame *f = parent; f; f = f->parent) {
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
    if (!(frm_flags & FRMFLAGS_UNKNOWN_10000) || !(frm_flags & FRMFLAGS_ON))
        return;
    if (parent && !(parent->frm_flags & FRMFLAGS_ON))
        return;

    const S_plane *pl = planes;
    ushort *ff = face_flags;
    for (int i = num_faces; i--; ) {
        const S_plane &p = *pl++;
        if (p.normal.Dot(pCurrentCamera->GetWorldPos()) + p.d > 0.0f)
            *ff |= 1;
        else
            *ff &= ~1;
        ff++;
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
    depth_plane = S_plane(dir, 0.0f);
    depth_plane.d = -(pCurrentCamera->GetWorldPos().Dot(depth_plane.normal) + depth_plane.d);

    while (n--) {
        if (face_flags[j] & 1) {
            int k = ef - edge_faces;
            *fp++ = S_plane(verts_w[faces[k + 1]], verts_w[faces[k]], verts_w[faces[k + 2]]);
            num_face_planes++;
            if (!(face_flags[*ef] & 1)) {
                k = ef - edge_faces;
                S_vector v = verts_w[faces[k + 1]];
                *sp++ = S_plane(cpos, v, verts_w[faces[k]]);
                num_side_planes++;
                float d = v.Dot(depth_plane.normal) + depth_plane.d;
                if (d > 0.0f)
                    depth_plane.d -= d;
            }
            ef++;
            if (!(face_flags[*ef] & 1)) {
                k = ef - edge_faces;
                S_vector v = verts_w[faces[k + 1]];
                *sp++ = S_plane(cpos, v, verts_w[faces[k]]);
                num_side_planes++;
                float d = v.Dot(depth_plane.normal) + depth_plane.d;
                if (d > 0.0f)
                    depth_plane.d -= d;
            }
            ef++;
            if (!(face_flags[*ef] & 1)) {
                k = ef - edge_faces;
                S_vector v = verts_w[faces[k - 2]];
                *sp++ = S_plane(cpos, v, verts_w[faces[k]]);
                num_side_planes++;
                float d = v.Dot(depth_plane.normal) + depth_plane.d;
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
// Open: VC6 keeps last on the x87 stack in the original and best in memory, ours the other way round; no spelling of
// the loops, the declarations or the assignment of last changed that.
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
    for (I3D_frame *f = frm; f; f = f->next_visual) {
        int rn = RenderNumber - 1;
        if (f->render_number != RenderNumber)
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
            f->render_number = rn;
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
void UOL_EnumProc(I3D_frame *frm)
{
    for (I3D_frame *f = frm->child; f; f = f->next) {
        if (f->type == FRAME_CAMERA || f->type == FRAME_SECTOR)
            continue;
        if (f->type == FRAME_OCCLUDER) {
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
// position as Dot's argument, the first loop's through a reference.
// Open: of the two copies of TestVertListW, the second (the portals into the sector) sums its products in another order
// than the original's; the first matches.
// FUNCTION: LS3DF 0x1003c610
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
    for (i = sct->portals_in.size(); i--; pp++) {
        I3D_portal *p = *pp;
        if (p->plane.normal.Dot(cpos) + p->plane.d < 0.0f) {
            p->portal_flags &= ~PORTALF_OCCLUDED;
            if (!p->twin) {
                for (o = sct->occluders; o; o = o->next_in_sector) {
                    if ((o->occl_flags & OCCLF_ACTIVE) && !(o->occl_flags & OCCLF_PORTAL) &&
                        o->TestVertListW(p->num_verts, (S_vector *)p->verts_w)) {
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
