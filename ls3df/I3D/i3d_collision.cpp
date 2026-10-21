// The collision tests (PS2 i3d_collision.cpp), at 0x1000de70-0x10015530 in the original: rays and moving spheres
// against the visuals' faces. The tests keep their state in the file's globals (PS2's names), which the recursive walks
// of the hierarchy (testSubtree, sphTestSubtree) and the visuals' face tests (testVisual, sphTestVisual) read. In PS2's
// order: the first three globals' empty $Es, the header's zero vector's (0x1000dea0, due at once: the static functions
// defined before TestColGeo count as source though they come after it), the ray tests with their static helpers after
// their first callers, TestColHierarchy's static's atexit thunk, the sphere tests' globals' empty $Es and the sphere
// tests. The copies of the header's inline functions the linker kept follow their first callers: S_vector's
// operator* and operator+, Square, SetNormalized, I3D_bbox::Absorb, and the file's SphereIntersect. The markers that
// would be out of address order here are in I3D.h, by name. Names are the PS2 build's unless noted. The texel test of
// testVisual inlines C_VBuffer::Lock (VBManager.h).
#include "I3D/I3D.h"
#include "I3D/render/VBManager.h"
#include "IGraph/IGraph.h"

// GLOBAL: LS3DF 0x100af82c
static S_vector normala;                // the normal of the face hit, in the visual's space

// GLOBAL: LS3DF 0x100af8a0
static S_vector lineBase;               // the ray's start

// GLOBAL: LS3DF 0x100af8c0
static S_vector lineDir;                // its direction, normalized

// GLOBAL: LS3DF 0x100af854
static uint colFlags;

// The callbacks and the frames to leave out, zero-initialized, so after the other globals in .bss
// GLOBAL: LS3DF 0x100af8d4
static I3D_COLLISION_CALLBACK *lpCallbackGeo = NULL;

// GLOBAL: LS3DF 0x100af8d8
static I3D_TEXEL_CALLBACK *lpCallbackGeoTex = NULL;

// GLOBAL: LS3DF 0x100af824
static int cbResult;                    // the callback's last answer (COLL_ENUMRET)

// GLOBAL: LS3DF 0x100af8cc
static void *cbContext;

// GLOBAL: LS3DF 0x100af89c
static float vzdalenost2;               // the nearest hit's distance, squared (Czech "distance")

// GLOBAL: LS3DF 0x100af84c
static I3D_frame *lpNearestObj;

// GLOBAL: LS3DF 0x100af888
static S_vector *GetNormal;             // where the normal goes, NULL if it isn't wanted

// GLOBAL: LS3DF 0x100af8ac
static const I3D_scene *pThisScene;

// GLOBAL: LS3DF 0x100af8dc
static I3D_frame *pWFrame = NULL;       // a frame left out

// GLOBAL: LS3DF 0x100af8e0
static I3D_frame *pWSubTree = NULL;     // a subtree left out

// GLOBAL: LS3DF 0x100af8e4
static I3D_frame *pSFrame = NULL;       // the only frame tested

// GLOBAL: LS3DF 0x100af898
static int specLod;                     // the level of detail to test pSFrame at

// GLOBAL: LS3DF 0x100af844
static int colLod;                      // the level of detail to test the others at

// GLOBAL: LS3DF 0x100af828
static bool breakTests;

// GLOBAL: LS3DF 0x100af870
static int v0;

// GLOBAL: LS3DF 0x100af874
static int v1;

// GLOBAL: LS3DF 0x100af878
static int v2;                          // the face hit's vertices

// GLOBAL: LS3DF 0x100af850
static I3D_face_group *pFGroupC;        // and its group

// GLOBAL: LS3DF 0x100af8bc
static float lineSize;                  // the ray's length

// GLOBAL: LS3DF 0x100af848
static float lineSize2;                 // squared

// GLOBAL: LS3DF 0x100af8d0
static float radiusA;                   // the moving sphere's

// The ray from base along dir against a box in the same space (the name is a guess; PS2 inlines it too): 0 from inside
// the box, else the ray's parameter where it enters the nearest side it crosses, 1e17 if none, with that side's normal
// (always the positive axis). The sides go min z, y, x, then max x, y, z; the bounds the original keeps in locals are
// locals here. Its vector calls are nested inline decisions, which testVisual's budget leaves out of line, as the
// original does.
inline float LineBoxIntersect(const S_vector &base, const S_vector &dir, const I3D_bbox &bb, S_vector &normal)
{
    if (base.x >= bb.min.x && base.x <= bb.max.x && base.y >= bb.min.y && base.y <= bb.max.y &&
        base.z >= bb.min.z && base.z <= bb.max.z) {
        normal = S_vector(0.0f, 0.0f, 1.0f);
        return 0.0f;
    }
    float y = bb.min.y;
    float x = bb.min.x;
    float t = 1e17f;
    if (dir.z != 0.0f) {
        float f = (bb.min.z - base.z) / dir.z;
        if (f >= 0.0f) {
            S_vector p = base + dir * f;
            if (p.x >= bb.min.x && p.x <= bb.max.x && p.y >= bb.min.y && p.y <= bb.max.y) {
                t = f;
                normal = S_vector(0.0f, 0.0f, 1.0f);
            }
        }
    }
    if (dir.y != 0.0f) {
        float f = (y - base.y) / dir.y;
        if (f >= 0.0f) {
            S_vector p = base + dir * f;
            if (p.z >= bb.min.z && p.z <= bb.max.z && p.x >= bb.min.x && p.x <= bb.max.x && t > f) {
                t = f;
                normal = S_vector(0.0f, 1.0f, 0.0f);
            }
        }
    }
    if (dir.x != 0.0f) {
        float f = (x - base.x) / dir.x;
        if (f >= 0.0f) {
            S_vector p = base + dir * f;
            if (p.z >= bb.min.z && p.z <= bb.max.z && p.y >= bb.min.y && p.y <= bb.max.y && t > f) {
                t = f;
                normal = S_vector(1.0f, 0.0f, 0.0f);
            }
        }
    }
    y = bb.max.y;
    float z = bb.max.z;
    if (dir.x != 0.0f) {
        float f = (bb.max.x - base.x) / dir.x;
        if (f >= 0.0f) {
            S_vector p = base + dir * f;
            if (p.z >= bb.min.z && p.z <= bb.max.z && p.y >= bb.min.y && p.y <= bb.max.y && t > f) {
                t = f;
                normal = S_vector(1.0f, 0.0f, 0.0f);
            }
        }
    }
    if (dir.y != 0.0f) {
        float f = (y - base.y) / dir.y;
        if (f >= 0.0f) {
            S_vector p = base + dir * f;
            if (p.z >= bb.min.z && p.z <= bb.max.z && p.x >= bb.min.x && p.x <= bb.max.x && t > f) {
                t = f;
                normal = S_vector(0.0f, 1.0f, 0.0f);
            }
        }
    }
    if (dir.z != 0.0f) {
        float f = (z - base.z) / dir.z;
        if (f >= 0.0f) {
            S_vector p = base + dir * f;
            if (p.x >= bb.min.x && p.x <= bb.max.x && p.y >= bb.min.y && p.y <= bb.max.y && t > f) {
                t = f;
                normal = S_vector(0.0f, 0.0f, 1.0f);
            }
        }
    }
    return t;
}

// A hit the texel test lets through, or that needs none: with COLF_ACCEPT_ANY the context's vector is cleared and
// the texel callback dropped for the rest of the test; true if COLF_FIRST_HIT stops the test at the first hit (the
// name is a guess). As a function it spends less of testVisual's inline budget than its seven copies written out.
inline bool AcceptHit(I3D_visual *vis)
{
    if (colFlags & COLF_ACCEPT_ANY) {
        if (cbContext)
            *(S_vector *)cbContext = S_vector(0.0f, 0.0f, 0.0f);
        lpCallbackGeoTex = NULL;
    }
    if (colFlags & COLF_FIRST_HIT) {
        lpNearestObj = vis;
        breakTests = true;
        return true;
    }
    return false;
}

// The faces of a visual against the ray, in the visual's space: the ray goes through the inverse matrix (a billboard
// turned to the ray in modes 2 and 3, to the camera in the others, as ProductBillMatrix does), then against the
// visual's box. With COLF_BOX the box is the hit; else a mirror's faces or the face groups of the mesh level to test,
// each face by Moller and Trumbore's ray-triangle test (culling for one-sided groups), the texel callback asked about
// it, and the nearest is kept. Its distance is measured back in world space; the collision callback then decides.
// The original's oddities stay: the box's distance is scaled by dir's square only in world space, the box hit passes
// its normal as the callback's point, and the texel callback gets a point never set (PS2's do the same).
// The billboard modes invert through temporaries: with a named copy VC6 merges the ends of cases 2 and 3, which the
// original keeps apart. Mode 2's scale comes from GetWorldScale, which constructs either branch's vector in s.
// Each test that rejects a face steps to the next face itself (PS2 has the increments there too): with the step in the
// for statement VC6 jumps to it directly and lays out the loop's end after the rest.
// Not matching yet: the frame (our three temporary matrices take a slot each, the original's one), a few nested
// inline decisions (the mirror's GetNormal leaves both operator- calls out of line, the two-sided normal its first
// operator* and second operator-), and the register allocation (the original keeps I3D_VISUAL_BILLBOARD in ebx and
// zero in edi); 96.5% with registers and stack offsets masked.
static void testVisual(I3D_visual *vis)
{
    S_vector dir, base;
    const I3D_bbox *bb;
    S_matrix m, bill_matrix;
    if (vis->vis_flags & VISF_WORLD_SPACE) {
        dir = lineDir;
        base = lineBase;
        bb = &vis->GetWBoundVolume().bbox;
    } else {
        if (vis->GetVisualType() == I3D_VISUAL_BILLBOARD) {
            I3D_billboard *bill = (I3D_billboard *)vis;
            switch (bill->axis_mode) {
            case 3: {
                m.Identity();
                m.SetDir3(-lineDir, S_vector(0.0f, 1.0f, 0.0f));
                S_vector r1 = m(1);
                m(0) *= vis->scale.x;
                m(1) = m(2) * vis->scale.y;
                m(2) = r1 * vis->scale.z;
                m(3) = vis->GetMatrix()(3);
                bill_matrix = m;
                m.Inverse(S_matrix(m));
                break;
            }
            case 2: {
                m.Identity();
                m.SetDir3(-lineDir, S_vector(0.0f, 1.0f, 0.0f));
                S_vector s = vis->GetWorldScale();
                S_vector r1 = m(1);
                m(0) *= s.x;
                m(1) = m(2) * s.y;
                m(2) = r1 * s.z;
                m(3) = vis->GetMatrix()(3);
                bill_matrix = m;
                m.Inverse(S_matrix(m));
                break;
            }
            default: {
                bill->ProductBillMatrix(lineBase, &m);
                bill_matrix = m;
                m.Inverse(S_matrix(m));
            }
            }
        } else
            m = vis->GetInvMatrix();
        base = lineBase * m;
        dir = lineDir.RotateByMatrix(m);
        bb = &vis->GetBoundVolume().bbox;
    }

    S_vector normal;
    float d = LineBoxIntersect(base, dir, *bb, normal);
    float dist2 = d * d;
    if (vis->vis_flags & VISF_WORLD_SPACE)
        dist2 *= dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
    if (!(colFlags & COLF_INFINITE) && lineSize2 < dist2)
        return;
    if (!(colFlags & COLF_ACCEPT_ANY) && vzdalenost2 < dist2)
        return;

    if (colFlags & COLF_BOX) {
        if (lpCallbackGeo) {
            cbResult = lpCallbackGeo(normal, (float)sqrt(dist2), vis, cbContext);
            if (cbResult >= COLL_IGNORE)
                return;
        }
        if (vzdalenost2 > dist2) {
            vzdalenost2 = dist2;
            lpNearestObj = vis;
            v0 = v1 = v2 = 0;
            pFGroupC = NULL;
            if (vis->vis_flags & VISF_WORLD_SPACE)
                normala = normal;
            else
                normala = normal.RotateByMatrix(vis->GetMatrix());
        }
        return;
    }

    const S_matrix *mat = vis->GetVisualType() != I3D_VISUAL_BILLBOARD ? &vis->GetRenderMatrix() : &bill_matrix;
    float best_dist2 = 1e16f;
    bool front = true;
    const I3D_triface *best_face = NULL;
    I3D_face_group *best_fg = NULL;
    if (vis->GetVisualType() == I3D_VISUAL_MIRROR) {
        I3D_mirror *mir = (I3D_mirror *)vis;
        if (mir->num_verts && mir->num_faces && mir->verts) {
            const S_vector *verts = mir->verts;
            int num = mir->num_faces;
            const I3D_triface *faces = mir->faces;
            for (; num > 0; num--) {
                const S_vector &vert0 = verts[faces->vi[0]];
                const S_vector &vert1 = verts[faces->vi[1]];
                const S_vector &vert2 = verts[faces->vi[2]];
                S_vector edge1 = vert1 - vert0;
                S_vector edge2 = vert2 - vert0;
                S_vector pvec = dir.Cross(edge2);
                float det = edge1.Dot(pvec);
                if (det < 1e-6) {
                    faces++;
                    continue;
                }
                S_vector tvec = base - vert0;
                float u = tvec.Dot(pvec);
                if (u < 0.0 || u > det) {
                    faces++;
                    continue;
                }
                S_vector qvec = tvec.Cross(edge1);
                float v = qvec.Dot(dir);
                if (v < 0.0 || u + v > det) {
                    faces++;
                    continue;
                }
                float t = edge2.Dot(qvec);
                if (t < 0.0f) {
                    faces++;
                    continue;
                }
                float inv_det = 1.0f / det;
                t *= inv_det;
                S_vector hit = (base + dir * t - base).RotateByMatrix(*mat);
                float hit_dist2 = hit.Square();
                if (!(colFlags & COLF_INFINITE) && lineSize2 < hit_dist2) {
                    faces++;
                    continue;
                }
                if (lpCallbackGeoTex) {
                    if (AcceptHit(vis))
                        return;
                } else if (colFlags & COLF_FIRST_HIT) {
                    lpNearestObj = vis;
                    breakTests = true;
                    return;
                }
                if (best_dist2 > hit_dist2) {
                    best_dist2 = hit_dist2;
                    if (GetNormal || lpCallbackGeo)
                        normal.GetNormal(vert0, vert1, vert2);
                    best_face = faces;
                    if (colFlags & COLF_FIRST_FACE)
                        break;
                }
                faces++;
            }
            if (best_dist2 >= 1e16)
                return;
            if (!(colFlags & COLF_INFINITE) && lineSize2 < best_dist2)
                return;
            if (lpCallbackGeo) {
                cbResult = lpCallbackGeo(normal.RotateByMatrix(*mat), (float)sqrt(best_dist2), vis, cbContext);
                if (cbResult >= COLL_IGNORE)
                    return;
            }
            if (vzdalenost2 > best_dist2) {
                vzdalenost2 = best_dist2;
                lpNearestObj = vis;
                normala = normal;
                v0 = best_face->vi[0];
                v1 = best_face->vi[1];
                v2 = best_face->vi[2];
                pFGroupC = NULL;
            }
        }
    } else {
        I3D_mesh_object *mesh = ((I3D_object *)vis)->GetMesh();
        if (!mesh)
            return;
        I3D_mesh_level *level = mesh->levels[vis == pSFrame ? specLod : colLod];
        if (!level)
            return;
        const uchar *verts;
        uint stride;
        if (vis->GetVisualType() != I3D_VISUAL_SINGLEMESH && vis->GetVisualType() != I3D_VISUAL_SINGLEMORPH) {
            verts = (const uchar *)level->LockVertices(1);
            stride = sizeof(I3D_vertex);
        } else {
            verts = (const uchar *)((I3D_singlemesh *)vis)->GetVertexPos(vis == pSFrame ? specLod : colLod);
            stride = sizeof(S_clip_sm_vertex);
        }
        if (!verts)
            return;
        for (I3D_face_group *fg = level->fgroups; fg; fg = fg->next) {
            if (!(colFlags & COLF_TWO_SIDED) && (!fg->material || !(fg->material->flags & MATF_TWO_SIDED))) {
                // One-sided: the faces turned away from the ray are left out
                int num = fg->num_faces;
                const I3D_triface *faces = fg->GetFaces();
                for (; num > 0; num--) {
                    const S_vector *vert0 = (const S_vector *)(verts + faces->vi[0] * stride);
                    const S_vector *vert1 = (const S_vector *)(verts + faces->vi[1] * stride);
                    const S_vector *vert2 = (const S_vector *)(verts + faces->vi[2] * stride);
                    S_vector edge1 = *vert1 - *vert0;
                    S_vector edge2 = *vert2 - *vert0;
                    S_vector pvec = dir.Cross(edge2);
                    float det = edge1.Dot(pvec);
                    if (det < 1e-6) {
                        faces++;
                        continue;
                    }
                    S_vector tvec = base - *vert0;
                    float u = tvec.Dot(pvec);
                    if (u < 0.0 || u > det) {
                        faces++;
                        continue;
                    }
                    S_vector qvec = tvec.Cross(edge1);
                    float v = qvec.Dot(dir);
                    if (v < 0.0 || u + v > det) {
                        faces++;
                        continue;
                    }
                    float t = edge2.Dot(qvec);
                    if (t < 0.0f) {
                        faces++;
                        continue;
                    }
                    float inv_det = 1.0f / det;
                    t *= inv_det;
                    S_vector hit = (base + dir * t - base).RotateByMatrix(*mat);
                    float hit_dist2 = hit.Square();
                    if (!(colFlags & COLF_INFINITE) && lineSize2 < hit_dist2) {
                        faces++;
                        continue;
                    }
                    if (lpCallbackGeoTex) {
                        S_vector pos;           // never set
                        if (!fg->material ||
                            (!fg->material->texture || !fg->material->texture->pixels) &&
                                IsAbsMrgZero(fg->material->alpha - 1.0f) ||
                            fg->material->texture && fg->material->flags <= MATF_TWO_SIDED &&
                                IsAbsMrgZero(fg->material->alpha - 1.0f)) {
                            if (AcceptHit(vis))
                                return;
                        } else if (fg->material->texture) {
                            // The texel: the point's coordinates from the level's vertices, through where the line
                            // from vert0 through it meets the opposite edge. The coordinates are vectors with a zero z.
                            const I3D_vertex *uv_verts = (const I3D_vertex *)level->vb.buffer->Lock(level->vb.base, 0);
                            S_vector uv0(uv_verts[faces->vi[0]].tex.u, uv_verts[faces->vi[0]].tex.v, 0.0f);
                            S_vector uv1(uv_verts[faces->vi[1]].tex.u, uv_verts[faces->vi[1]].tex.v, 0.0f);
                            S_vector uv2(uv_verts[faces->vi[2]].tex.u, uv_verts[faces->vi[2]].tex.v, 0.0f);
                            S_vector dir2 = *vert2 - *vert1;
                            S_vector dir1 = base + dir * t - *vert0;
                            float t1, t2;
                            LineLineIntersect(*vert0, dir1, *vert1, dir2, t1, t2);
                            if (!IsAbsMrgZero(t1)) {
                                S_vector pt = *vert0 + dir1 * t1;
                                float sum = dir2.Sum();
                                if (!IsAbsMrgZero(sum)) {
                                    t2 = (pt - *vert1).Sum() / sum;
                                    S_texel_info ti;
                                    ti.u = uv1.x + (uv2.x - uv1.x) * t2;
                                    ti.v = uv1.y + (uv2.y - uv1.y) * t2;
                                    t1 = 1.0f / t1;
                                    ti.u = uv0.x + (ti.u - uv0.x) * t1;
                                    ti.v = uv0.y + (ti.v - uv0.y) * t1;
                                    ti.u = (float)fmod(ti.u, 1.0);
                                    ti.v = (float)fmod(ti.v, 1.0);
                                    if (ti.u < 0.0f)
                                        ti.u += 1.0f;
                                    if (ti.v < 0.0f)
                                        ti.v += 1.0f;
                                    ti.mat = fg->material;
                                    switch (lpCallbackGeoTex(pos, t, vis, cbContext, &ti)) {
                                    case 0:
                                        if (AcceptHit(vis))
                                            return;
                                        break;
                                    case 2:
                                        faces++;
                                        continue;
                                    }
                                }
                            }
                        } else {
                            S_texel_info ti;
                            ti.mat = fg->material;
                            switch (lpCallbackGeoTex(pos, t, vis, cbContext, &ti)) {
                            case 0:
                                if (AcceptHit(vis))
                                    return;
                                break;
                            case 2:
                                faces++;
                                continue;
                            }
                        }
                    } else if (colFlags & COLF_FIRST_HIT) {
                        lpNearestObj = vis;
                        breakTests = true;
                        return;
                    }
                    if (best_dist2 > hit_dist2) {
                        best_dist2 = hit_dist2;
                        if (GetNormal || lpCallbackGeo) {
                            // The edges scaled up, for the precision of a small face's normal
                            S_vector a = (*vert2 - *vert1) * 10000.0f;
                            S_vector b = (*vert0 - *vert1) * 10000.0f;
                            normal.x = a.y * b.z - a.z * b.y;
                            normal.y = a.z * b.x - a.x * b.z;
                            normal.z = a.x * b.y - a.y * b.x;
                        }
                        best_face = faces;
                        best_fg = fg;
                        if (colFlags & COLF_FIRST_FACE)
                            goto done;
                    }
                    faces++;
                }
            } else {
                // Two-sided: the faces turned away count too
                int num = fg->num_faces;
                const I3D_triface *faces = fg->GetFaces();
                for (; num > 0; num--) {
                    const S_vector *vert0 = (const S_vector *)(verts + faces->vi[0] * stride);
                    const S_vector *vert1 = (const S_vector *)(verts + faces->vi[1] * stride);
                    const S_vector *vert2 = (const S_vector *)(verts + faces->vi[2] * stride);
                    S_vector edge1 = *vert1 - *vert0;
                    S_vector edge2 = *vert2 - *vert0;
                    S_vector pvec = dir.Cross(edge2);
                    float det = edge1.Dot(pvec);
                    if (det > -1e-6 && det < 1e-6) {
                        faces++;
                        continue;
                    }
                    float inv_det = 1.0f / det;
                    S_vector tvec = base - *vert0;
                    float u = tvec.Dot(pvec) * inv_det;
                    if (u < 0.0 || u > 1.0) {
                        faces++;
                        continue;
                    }
                    S_vector qvec = tvec.Cross(edge1);
                    float v = qvec.Dot(dir) * inv_det;
                    if (v < 0.0 || u + v > 1.0) {
                        faces++;
                        continue;
                    }
                    float t = edge2.Dot(qvec) * inv_det;
                    if (t < 0.0f) {
                        faces++;
                        continue;
                    }
                    S_vector hit = (base + dir * t - base).RotateByMatrix(*mat);
                    float hit_dist2 = hit.Square();
                    if (!(colFlags & COLF_INFINITE) && lineSize2 < hit_dist2) {
                        faces++;
                        continue;
                    }
                    if (lpCallbackGeoTex) {
                        S_vector pos;           // never set
                        if (!fg->material ||
                            (!fg->material->texture || !fg->material->texture->pixels) &&
                                IsAbsMrgZero(fg->material->alpha - 1.0f) ||
                            fg->material->texture && fg->material->flags <= MATF_TWO_SIDED &&
                                IsAbsMrgZero(fg->material->alpha - 1.0f)) {
                            if (AcceptHit(vis))
                                return;
                        } else if (fg->material->texture) {
                            // The texel: the point's coordinates from the level's vertices, through where the line
                            // from vert0 through it meets the opposite edge. The coordinates are vectors with a zero z.
                            const I3D_vertex *uv_verts = (const I3D_vertex *)level->vb.buffer->Lock(level->vb.base, 0);
                            S_vector uv0(uv_verts[faces->vi[0]].tex.u, uv_verts[faces->vi[0]].tex.v, 0.0f);
                            S_vector uv1(uv_verts[faces->vi[1]].tex.u, uv_verts[faces->vi[1]].tex.v, 0.0f);
                            S_vector uv2(uv_verts[faces->vi[2]].tex.u, uv_verts[faces->vi[2]].tex.v, 0.0f);
                            S_vector dir2 = *vert2 - *vert1;
                            S_vector dir1 = base + dir * t - *vert0;
                            float t1, t2;
                            LineLineIntersect(*vert0, dir1, *vert1, dir2, t1, t2);
                            if (!IsAbsMrgZero(t1)) {
                                S_vector pt = *vert0 + dir1 * t1;
                                float sum = dir2.Sum();
                                if (!IsAbsMrgZero(sum)) {
                                    t2 = (pt - *vert1).Sum() / sum;
                                    S_texel_info ti;
                                    ti.u = uv1.x + (uv2.x - uv1.x) * t2;
                                    ti.v = uv1.y + (uv2.y - uv1.y) * t2;
                                    t1 = 1.0f / t1;
                                    ti.u = uv0.x + (ti.u - uv0.x) * t1;
                                    ti.v = uv0.y + (ti.v - uv0.y) * t1;
                                    ti.u = (float)fmod(ti.u, 1.0);
                                    ti.v = (float)fmod(ti.v, 1.0);
                                    if (ti.u < 0.0f)
                                        ti.u += 1.0f;
                                    if (ti.v < 0.0f)
                                        ti.v += 1.0f;
                                    ti.mat = fg->material;
                                    switch (lpCallbackGeoTex(pos, t, vis, cbContext, &ti)) {
                                    case 0:
                                        if (AcceptHit(vis))
                                            return;
                                        break;
                                    case 2:
                                        faces++;
                                        continue;
                                    }
                                }
                            }
                        } else {
                            S_texel_info ti;
                            ti.mat = fg->material;
                            switch (lpCallbackGeoTex(pos, t, vis, cbContext, &ti)) {
                            case 0:
                                if (AcceptHit(vis))
                                    return;
                                break;
                            case 2:
                                faces++;
                                continue;
                            }
                        }
                    } else if (colFlags & COLF_FIRST_HIT) {
                        lpNearestObj = vis;
                        breakTests = true;
                        return;
                    }
                    if (best_dist2 > hit_dist2) {
                        if (inv_det < 0.0f)
                            front = false;
                        best_dist2 = hit_dist2;
                        if (GetNormal || lpCallbackGeo) {
                            // The edges scaled up, for the precision of a small face's normal
                            S_vector a = (*vert2 - *vert1) * 10000.0f;
                            S_vector b = (*vert0 - *vert1) * 10000.0f;
                            normal.x = a.y * b.z - a.z * b.y;
                            normal.y = a.z * b.x - a.x * b.z;
                            normal.z = a.x * b.y - a.y * b.x;
                            if (inv_det < 0.0f && fg->material && fg->material->flags & MATF_TWO_SIDED) {
                                normal.x = -normal.x;
                                normal.y = -normal.y;
                                normal.z = -normal.z;
                            }
                        }
                        best_face = faces;
                        best_fg = fg;
                        if (colFlags & COLF_FIRST_FACE)
                            goto done;
                    }
                    faces++;
                }
            }
        }
    done:
        level->UnlockVertices();
        if (best_dist2 >= 1e16)
            return;
        if (!(colFlags & COLF_INFINITE) && lineSize2 < best_dist2)
            return;
        if (lpCallbackGeo) {
            cbResult = lpCallbackGeo(normal.RotateByMatrix(*mat), (float)sqrt(best_dist2), vis, cbContext);
            if (cbResult >= COLL_IGNORE)
                return;
        }
        if (vzdalenost2 > best_dist2) {
            if (!front)
                vzdalenost2 = best_dist2 + .001f;
            else
                vzdalenost2 = best_dist2;
            lpNearestObj = vis;
            normala = normal;
            v0 = best_face->vi[0];
            v1 = best_face->vi[1];
            v2 = best_face->vi[2];
            pFGroupC = best_fg;
        }
    }
}

// The ray against a box (the name is a guess; PS2 inlines it too): lineBase inside it, or the box's center's distance
// from the line against the box's extent along the axes crossed with the line. The corners are separate references
// (the original keeps max in a register of its own), and the dot product is written out: through Dot, a call of its
// own, VC6 leaves it out of line here.
inline bool LineIntersectBox(const S_vector &min, const S_vector &max)
{
    if (lineBase.x >= min.x && lineBase.x <= max.x && lineBase.y >= min.y && lineBase.y <= max.y &&
        lineBase.z >= min.z && lineBase.z <= max.z)
        return true;
    S_vector center = (min + max) * .5f;
    S_vector d = center - lineBase;
    float t = lineDir.x * d.x + lineDir.y * d.y + lineDir.z * d.z;
    S_vector p = lineBase + lineDir * t;
    S_vector w = center - p;
    S_vector e = (max - min) * .5f;
    float ay = (float)fabs(lineDir.y);
    float az = (float)fabs(lineDir.z);
    if (fabs(lineDir.z * w.y - w.z * lineDir.y) > az * e.y + ay * e.z)
        return false;
    float ax = (float)fabs(lineDir.x);
    if (fabs(w.z * lineDir.x - w.x * lineDir.z) > az * e.x + ax * e.z)
        return false;
    if (fabs(w.x * lineDir.y - lineDir.x * w.y) > ay * e.x + ax * e.y)
        return false;
    return true;
}

// The ray against a bound sphere (the name is a guess; PS2 inlines it): the point of the line nearest to the center,
// within the ray unless it is infinite (COLF_INFINITE). The last test is an if: returned, the compare tests the other
// flags (test ah, 5 for test ah, 1).
inline bool LineIntersectSphere(const I3D_bsphere &bs)
{
    float d = (bs.pos - lineBase).Dot(lineDir);
    if (d < -bs.radius)
        return false;
    if (!(colFlags & COLF_INFINITE) && d > lineSize + bs.radius)
        return false;
    S_vector p = lineBase + lineDir * d;
    S_vector v = p - bs.pos;
    float dd = v.Dot(v);
    if (dd >= bs.radius * bs.radius)
        return false;
    return true;
}

// The frame's hierarchy against the ray: its box (a sector without one always), then a visual's sphere and faces,
// then its children until a callback stops the test.
// Not matching yet: in the box's tests VC6 loads the other operand of three products first (lineDir.x before w.z, the
// extents before the absolute values); the operands' order in the source doesn't move them.
// FUNCTION: LS3DF 0x1000e370
static void testSubtree(I3D_frame *frm)
{
    if (frm == pWSubTree)
        return;
    if (!(frm->frm_flags & FRMFLAGS_ON) && !(colFlags & COLF_HIDDEN))
        return;
    const I3D_bbox &bb = frm->GetHRBoundVolumeW().bbox;
    if (bb.IsValid()) {
        if (!LineIntersectBox(bb.min, bb.max))
            return;
    } else if (frm->GetType() != FRAME_SECTOR)
        return;
    if (frm->GetType() == FRAME_VISUAL && frm != pWFrame) {
        I3D_visual *vis = (I3D_visual *)frm;
        if (LineIntersectSphere(vis->GetWBoundVolume().bsphere) &&
            !(vis->vis_flags & VISF_UNKNOWN_1000 && colFlags & COLF_UNKNOWN_4000000)) {
            switch (vis->GetVisualType()) {
            case I3D_VISUAL_OBJECT:
            case I3D_VISUAL_LIT_OBJECT:
            case I3D_VISUAL_BILLBOARD:
            case I3D_VISUAL_SINGLEMESH:
            case I3D_VISUAL_MORPH:
            case I3D_VISUAL_SINGLEMORPH:
            case I3D_VISUAL_MIRROR:
                testVisual(vis);
                if (cbResult == COLL_IGNORE_CHILDREN)
                    return;
            }
        }
    }
    for (I3D_frame *f = frm->child; f; f = f->next) {
        if (breakTests)
            break;
        if (cbResult == COLL_ACCEPT_STOP)
            break;
        testSubtree(f);
    }
}

// The texel callback without one of the caller's: 2 for a transparent texel (alpha below 0.1), 1 else. It takes the
// alpha as T&l.cpp's TexelCallback does, the texel's index computed before the branches and the color's channels in
// a vector.
// Not matching yet: the original keeps mat in a stack slot and loads texture->pixels again after the __ftol calls,
// where ours keeps the test's load (TexelCallback has the same open difference).
// FUNCTION: LS3DF 0x10011300
static int __stdcall DefaultTexelCallback(const S_vector &pos, float dist, I3D_frame *frm, void *context, void *texel)
{
    S_texel_info *ti = (S_texel_info *)texel;
    I3D_material *mat = ti->mat;
    ITexture *texture = mat->texture;
    if (texture && texture->pixels) {
        float u = ti->u;
        float v = ti->v;
        uint x = min((uint)(int)(texture->width * u), (uint)(texture->width - 1));
        uint y = min((uint)(int)(texture->height * v), (uint)(texture->height - 1));
        uint i = y * texture->width + x;
        float a;
        if (mat->flags & MATF_ADD_MODE) {
            const uchar *p = &texture->pixels[i * 4];
            S_vector c;
            c.x = p[1] * (1.0f / 255.0f);
            c.y = p[2] * (1.0f / 255.0f);
            c.z = p[3] * (1.0f / 255.0f);
            a = c.x * 0.3f + c.y * 0.6f + c.z * 0.1f;
        } else
            a = texture->pixels[i * 4] * (1.0f / 255.0f);
        if (a * mat->alpha < 0.1)
            return 2;
    } else if (mat->alpha < 0.1f)
        return 2;
    return 1;
}

I3D_frame *I3D_scene::TestColGeo(const S_vector &from, const S_vector &dir, uint flags, float *dist, S_vector *normal,
    I3D_COLLISION_CALLBACK *cb, void *cb_context, I3D_TEXEL_CALLBACK *texel_cb, I3D_frame *frm, int lod) const
{
    breakTests = false;
    lineBase = from;
    lineDir.SetNormalized(dir);
    lineSize2 = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
    lineSize = (float)sqrt(lineSize2);
    cbContext = cb_context;
    lpCallbackGeo = cb;
    cbResult = COLL_ACCEPT;
    if (flags & COLF_COLORKEY) {
        if (texel_cb)
            lpCallbackGeoTex = texel_cb;
        else
            lpCallbackGeoTex = DefaultTexelCallback;
    } else
        lpCallbackGeoTex = NULL;
    colFlags = flags;
    pThisScene = this;
    if (flags & COLF_EXCLUDE_SUBTREE) {
        specLod = lod;
        colLod = lod;
        pWSubTree = frm;
        pSFrame = NULL;
    } else if (flags & COLF_FRAME_LOD) {
        pWSubTree = NULL;
        specLod = lod;
        colLod = 0;
        pSFrame = frm;
        pWFrame = NULL;
    } else {
        pWSubTree = NULL;
        specLod = lod;
        colLod = lod;
        pWFrame = frm;
        pSFrame = NULL;
    }
    GetNormal = normal;
    lpNearestObj = NULL;
    vzdalenost2 = 1e16f;
    if (flags & COLF_NEAR_SECTOR)
        testSubtree(near_sector);
    testSubtree(primary_sector);
    if (flags & COLF_OMNI_SECTOR)
        testSubtree(omni_sector);
    if (lpNearestObj) {
        if (dist)
            *dist = (float)sqrt(vzdalenost2);
        if (normal) {
            if (lpNearestObj->GetType() != FRAME_VISUAL ||
                !(((I3D_visual *)lpNearestObj)->vis_flags & VISF_WORLD_SPACE))
                normala = normala.RotateByMatrix(lpNearestObj->GetMatrix());
            normala.Normalize();
            *normal = normala;
        }
    }
    return lpNearestObj;
}

// TestColGeo with the hit's point instead of its distance, and no frame left out
// FUNCTION: LS3DF 0x10011430
I3D_frame *I3D_scene::TestColGeo1(const S_vector &from, const S_vector &dir, uint flags, S_vector *pos,
    S_vector *normal, I3D_COLLISION_CALLBACK *cb, void *cb_context, I3D_TEXEL_CALLBACK *texel_cb, int lod) const
{
    breakTests = false;
    lineBase = from;
    lineDir.SetNormalized(dir);
    lineSize2 = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
    lineSize = (float)sqrt(lineSize2);
    cbContext = cb_context;
    lpCallbackGeo = cb;
    cbResult = COLL_ACCEPT;
    if (flags & COLF_COLORKEY) {
        if (texel_cb)
            lpCallbackGeoTex = texel_cb;
        else
            lpCallbackGeoTex = DefaultTexelCallback;
    } else
        lpCallbackGeoTex = NULL;
    colFlags = flags;
    pThisScene = this;
    specLod = lod;
    colLod = lod;
    pSFrame = NULL;
    pWFrame = NULL;
    pWSubTree = NULL;
    GetNormal = normal;
    lpNearestObj = NULL;
    vzdalenost2 = 1e16f;
    if (flags & COLF_NEAR_SECTOR)
        testSubtree(near_sector);
    testSubtree(primary_sector);
    if (flags & COLF_OMNI_SECTOR)
        testSubtree(omni_sector);
    if (lpNearestObj) {
        if (pos)
            *pos = from + lineDir * (float)sqrt(vzdalenost2);
        if (normal) {
            if (lpNearestObj->GetType() != FRAME_VISUAL ||
                !(((I3D_visual *)lpNearestObj)->vis_flags & VISF_WORLD_SPACE))
                normala = normala.RotateByMatrix(lpNearestObj->GetMatrix());
            normala.Normalize();
            *normal = normala;
        }
    }
    return lpNearestObj;
}

// The ray against root's hierarchy only; the face hit goes to col. GetNormal points at a static of its own (PS2's
// retNormal, whose guard PS2 names init$2610), as nothing reads the normal it gets. The third branch's stores are in
// TestColGeo's order: in PS2's, VC6 gives the normal's square another parameter's stack slot.
// Not matching yet: retNormal and its guard, which VC6 hashes by their decorated names, come elsewhere in .bss. The
// original's put retNormal first and the guard right after breakTests (tools/bsshash.py), which no block number of
// ours fits with the guard's $S17 and these parameter types: the original's function had more blocks before the
// static, or other numbers before the guard.
// FUNCTION: LS3DF 0x100118e0
I3D_frame *I3D_scene::TestColHierarchy(const S_vector &from, const S_vector &dir, I3D_frame *root, I3D_COLLISION &col,
    uint flags, I3D_frame *frm, int lod, I3D_COLLISION_CALLBACK *cb) const
{
    if (!root)
        return NULL;
    breakTests = false;
    lineBase = from;
    lineDir.SetNormalized(dir);
    lineSize2 = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
    lineSize = (float)sqrt(lineSize2);
    cbContext = NULL;
    lpCallbackGeo = cb;
    cbResult = COLL_ACCEPT;
    if (flags & COLF_COLORKEY)
        lpCallbackGeoTex = DefaultTexelCallback;
    else
        lpCallbackGeoTex = NULL;
    colFlags = flags;
    pThisScene = this;
    if (flags & COLF_EXCLUDE_SUBTREE) {
        pWSubTree = frm;
        specLod = lod;
        colLod = lod;
        pSFrame = NULL;
    } else if (flags & COLF_FRAME_LOD) {
        pWSubTree = NULL;
        specLod = lod;
        colLod = 0;
        pSFrame = frm;
        pWFrame = NULL;
    } else {
        pWSubTree = NULL;
        specLod = lod;
        colLod = lod;
        pWFrame = frm;
        pSFrame = NULL;
    }
    static S_vector retNormal;
    GetNormal = &retNormal;
    lpNearestObj = NULL;
    vzdalenost2 = 1e16f;
    pFGroupC = NULL;
    testSubtree(root);
    if (lpNearestObj) {
        col.dist = (float)sqrt(vzdalenost2);
        col.normal = normala.RotateByMatrix(lpNearestObj->GetMatrix());
        col.normal.Normalize();
        col.v[0] = v0;
        col.v[1] = v1;
        col.v[2] = v2;
        col.fgroup = pFGroupC;
    }
    return lpNearestObj;
}

// The box of the sphere's move
// GLOBAL: LS3DF 0x100af858
static I3D_bbox sphBox;

// The move's end
// GLOBAL: LS3DF 0x100af8b0
static S_vector lineDest;

// The nearest hit's point
// GLOBAL: LS3DF 0x100af88c
static S_vector colPointSph;

// A hit's point (Czech "collision point")
// GLOBAL: LS3DF 0x100af87c
static S_vector kolBod;

// Whether two boxes overlap (the name is a guess; PS2 inlines it). Written out in TestColSphereVisual, VC6 inlines
// SphereIntersect and S_vector's constructors there, which the original calls: the inline call is one more pending.
inline bool BoxIntersect(const I3D_bbox &a, const I3D_bbox &b)
{
    return a.min.x <= b.max.x && b.min.x <= a.max.x && a.min.y <= b.max.y && b.min.y <= a.max.y &&
        a.min.z <= b.max.z && b.min.z <= a.max.z;
}

// The sphere moving from lineBase against the line through a bound sphere (the name is a guess; PS2 inlines it). The
// test of the far end tests too little: only a sphere both behind the start and beyond the end fails it.
inline bool SphereIntersect(const I3D_bsphere &bs)
{
    float r = radiusA + bs.radius;
    float d = lineDir.Dot(bs.pos - lineBase);
    if (d < -r && d > lineSize + r)
        return false;
    S_vector p = lineBase + lineDir * d;
    return (p - bs.pos).Square() < r * r;
}

// The faces of a visual against the moving sphere, in the visual's space: a face the sphere touches at the start
// (the point of the plane nearest to the start in the face's box, then in the face or near one of its edges) gives
// the start, one the sphere crosses during the move the point where it reaches the plane, the nearest of them goes to
// kolBod. A billboard's space is the one it has turned to the start in.
// Not matching yet: the register allocation and scheduling all through (95% with registers and stack offsets masked),
// and one nested inline decision: after the first edge's SetNormalized the original inlines the projection's
// operator- and Dot (calling S_vector's constructor), ours calls both.
static void sphTestVisual(I3D_visual *vis)
{
    S_vector lb, ld;
    float r;
    const S_matrix *mat;
    S_matrix m;
    if (vis->vis_flags & VISF_WORLD_SPACE) {
        lb = lineBase;
        ld = lineDest;
        r = radiusA;
    } else {
        if (vis->GetVisualType() == I3D_VISUAL_BILLBOARD) {
            switch (((I3D_billboard *)vis)->axis_mode) {
            case 3: {
                m.Identity();
                m.SetDir3(-lineDir, S_vector(0.0f, 1.0f, 0.0f));
                S_vector r1 = m(1);
                m(0) *= vis->scale.x;
                m(1) = m(2) * vis->scale.y;
                m(2) = r1 * vis->scale.z;
                m(3) = vis->GetMatrix()(3);
                break;
            }
            case 2: {
                m.Identity();
                m.SetDir3(-lineDir, S_vector(0.0f, 1.0f, 0.0f));
                S_vector s = vis->GetWorldScale();
                S_vector r1 = m(1);
                m(0) *= s.x;
                m(1) = m(2) * s.y;
                m(2) = r1 * s.z;
                m(3) = vis->GetMatrix()(3);
                break;
            }
            default:
                ((I3D_billboard *)vis)->ProductBillMatrix(lineBase, &m);
            }
            m.Inverse(S_matrix(m));
            mat = &m;
        } else
            mat = &vis->GetInvMatrix();
        lb = lineBase * *mat;
        ld = lineDest * *mat;
        S_vector dir;
        dir.SetNormalized(ld - lb);
        r = radiusA / mat->GetUScale();
    }
    float r2 = r * r;
    I3D_mesh_object *mesh = ((I3D_object *)vis)->GetMesh();
    if (!mesh)
        return;
    I3D_mesh_level *level = mesh->levels[vis == pSFrame ? specLod : colLod];
    if (!level)
        return;
    const uchar *verts;
    uint stride;
    if (vis->GetVisualType() != I3D_VISUAL_SINGLEMESH && vis->GetVisualType() != I3D_VISUAL_SINGLEMORPH) {
        verts = (const uchar *)level->LockVertices(1);
        stride = sizeof(I3D_vertex);
    } else {
        verts = (const uchar *)((I3D_singlemesh *)vis)->GetVertexPos(vis == pSFrame ? specLod : colLod);
        stride = sizeof(S_clip_sm_vertex);
    }
    if (!verts)
        return;
    float best = 1e16f;
    for (I3D_face_group *fg = level->fgroups; fg; fg = fg->next) {
        int num = fg->num_faces;
        const I3D_triface *faces = fg->GetFaces();
        for (; num > 0; faces++, num--) {
            const S_vector &v0 = *(const S_vector *)(verts + faces->vi[0] * stride);
            const S_vector &v1 = *(const S_vector *)(verts + faces->vi[1] * stride);
            const S_vector &v2 = *(const S_vector *)(verts + faces->vi[2] * stride);
            S_vector n;
            n.GetNormal(v0, v1, v2);
            n.Normalize();
            S_vector a = lb - v0, b = ld - v0;
            float d0 = a.x * n.x + a.y * n.y + a.z * n.z;
            float d1 = b.x * n.x + b.y * n.y + b.z * n.z;
            if (fabs(d0) <= r) {
                // The plane's point nearest to the start
                S_vector p;
                if (d0 >= 0.0f)
                    p = lb - n * d0;
                else
                    p = lb + n * (float)fabs(d0);
                S_vector mn, mx;
                if (v0.x < v1.x)
                    mn.x = v0.x;
                else
                    mn.x = v1.x;
                if (mn.x > v2.x)
                    mn.x = v2.x;
                if (v0.y < v1.y)
                    mn.y = v0.y;
                else
                    mn.y = v1.y;
                if (mn.y > v2.y)
                    mn.y = v2.y;
                if (v0.z < v1.z)
                    mn.z = v0.z;
                else
                    mn.z = v1.z;
                if (mn.z > v2.z)
                    mn.z = v2.z;
                if (v0.x > v1.x)
                    mx.x = v0.x;
                else
                    mx.x = v1.x;
                if (mx.x < v2.x)
                    mx.x = v2.x;
                if (v0.y > v1.y)
                    mx.y = v0.y;
                else
                    mx.y = v1.y;
                if (mx.y < v2.y)
                    mx.y = v2.y;
                if (v0.z > v1.z)
                    mx.z = v0.z;
                else
                    mx.z = v1.z;
                if (mx.z < v2.z)
                    mx.z = v2.z;
                mn.x -= r;
                mn.y -= r;
                mn.z -= r;
                mx.x += r;
                mx.y += r;
                mx.z += r;
                if (p.x >= mn.x && p.x <= mx.x && p.y >= mn.y && p.y <= mx.y && p.z >= mn.z && p.z <= mx.z) {
                    S_vector e1 = v1 - v0, e2 = v2 - v0;
                    float d11 = e1.Dot(e1), d12 = e1.Dot(e2), d22 = e2.Dot(e2);
                    float d1w = (p - v0).Dot(e1), d2w = (p - v0).Dot(e2);
                    float u = d1w * d22 - d2w * d12;
                    float v = d2w * d11 - d1w * d12;
                    if (u >= 0.0 && v >= 0.0 && u + v <= (float)fabs(d11 * d22 - d12 * d12)) {
                        kolBod = p;
                        best = 0.0f;
                        if (colFlags & COLF_FIRST_FACE)
                            goto done;
                    } else {
                        S_vector dir;
                        dir.SetNormalized(v1 - v0);
                        float proj = dir.Dot(p - v0);
                        S_vector c = v0 + dir * proj;
                        S_vector d = p - c;
                        if ((v1 - v0).Square() >= proj * proj && d.Square() <= r2) {
                            kolBod = c;
                            best = 0.0f;
                            if (colFlags & COLF_FIRST_FACE)
                                goto done;
                        } else {
                            dir.SetNormalized(v1 - v2);
                            proj = dir.Dot(p - v2);
                            c = v2 + dir * proj;
                            S_vector d = p - c;
                            if ((v1 - v2).Square() >= proj * proj && d.Square() <= r2) {
                                kolBod = c;
                                best = 0.0f;
                                if (colFlags & COLF_FIRST_FACE)
                                    goto done;
                            } else {
                                dir.SetNormalized(v2 - v0);
                                proj = dir.Dot(p - v0);
                                c = v0 + dir * proj;
                                S_vector d = p - c;
                                if ((v0 - v2).Square() >= proj * proj && d.Square() <= r2) {
                                    kolBod = c;
                                    best = 0.0f;
                                    if (colFlags & COLF_FIRST_FACE)
                                        goto done;
                                }
                            }
                        }
                    }
                }
            }
            if (d0 > r && d1 < r) {
                // Where the sphere reaches the plane
                float t = (d0 - r) / (d0 - d1);
                S_vector p = lb * (1.0f - t) + ld * t;
                S_vector q = p - n * r;
                S_vector mn, mx;
                if (v0.x < v1.x)
                    mn.x = v0.x;
                else
                    mn.x = v1.x;
                if (mn.x > v2.x)
                    mn.x = v2.x;
                if (v0.y < v1.y)
                    mn.y = v0.y;
                else
                    mn.y = v1.y;
                if (mn.y > v2.y)
                    mn.y = v2.y;
                if (v0.z < v1.z)
                    mn.z = v0.z;
                else
                    mn.z = v1.z;
                if (mn.z > v2.z)
                    mn.z = v2.z;
                if (v0.x > v1.x)
                    mx.x = v0.x;
                else
                    mx.x = v1.x;
                if (mx.x < v2.x)
                    mx.x = v2.x;
                if (v0.y > v1.y)
                    mx.y = v0.y;
                else
                    mx.y = v1.y;
                if (mx.y < v2.y)
                    mx.y = v2.y;
                if (v0.z > v1.z)
                    mx.z = v0.z;
                else
                    mx.z = v1.z;
                if (mx.z < v2.z)
                    mx.z = v2.z;
                mn.x -= r;
                mn.y -= r;
                mn.z -= r;
                mx.x += r;
                mx.y += r;
                mx.z += r;
                if (q.x >= mn.x && q.x <= mx.x && q.y >= mn.y && q.y <= mx.y && q.z >= mn.z && q.z <= mx.z) {
                    S_vector e1 = v1 - v0, e2 = v2 - v0;
                    float d11 = e1.Dot(e1), d12 = e1.Dot(e2), d22 = e2.Dot(e2);
                    float d1w = (q - v0).Dot(e1), d2w = (q - v0).Dot(e2);
                    float u = d1w * d22 - d2w * d12;
                    float v = d2w * d11 - d1w * d12;
                    if (u >= 0.0 && v >= 0.0 && u + v <= (float)fabs(d11 * d22 - d12 * d12)) {
                        if (best > t) {
                            best = t;
                            kolBod = q;
                            if (colFlags & COLF_FIRST_FACE)
                                goto done;
                        }
                    } else {
                        S_vector dir;
                        dir.SetNormalized(v1 - v0);
                        float proj = dir.Dot(q - v0);
                        S_vector c = v0 + dir * proj;
                        S_vector d = q - c;
                        if ((v1 - v0).Square() >= proj * proj && d.Square() <= r2) {
                            if (best > t) {
                                best = t;
                                kolBod = c;
                                if (colFlags & COLF_FIRST_FACE)
                                    goto done;
                            }
                        } else {
                            dir.SetNormalized(v1 - v2);
                            proj = dir.Dot(q - v2);
                            c = v2 + dir * proj;
                            float dist2 = (q - c).Square();
                            if ((v1 - v2).Square() >= proj * proj && dist2 <= r2) {
                                if (best > t) {
                                    best = t;
                                    kolBod = c;
                                    if (colFlags & COLF_FIRST_FACE)
                                        goto done;
                                }
                            } else {
                                dir.SetNormalized(v2 - v0);
                                proj = dir.Dot(q - v0);
                                c = v0 + dir * proj;
                                float dist2 = (q - c).Square();
                                if ((v0 - v2).Square() >= proj * proj && dist2 <= r2) {
                                    if (best > t) {
                                        best = t;
                                        kolBod = c;
                                        if (colFlags & COLF_FIRST_FACE)
                                            goto done;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
done:
    level->UnlockVertices();
    if (best >= 1e16f)
        return;
    if (!(vis->vis_flags & VISF_WORLD_SPACE))
        kolBod = kolBod * *mat;
    float d2 = (kolBod - lineBase).Square();
    if (lpCallbackGeo) {
        cbResult = lpCallbackGeo(ZeroSVector, (float)sqrt(d2), vis, cbContext);
        if (cbResult >= COLL_IGNORE)
            return;
    }
    if (vzdalenost2 > d2) {
        vzdalenost2 = d2;
        colPointSph = kolBod;
        lpNearestObj = vis;
    }
}

// The frame's hierarchy against the moving sphere: its bound sphere (a sector without one always), then a visual's
// box and sphere and faces, then its children until a callback stops the test. The visual's type is read as the
// member: through GetVisualType, one more inline call to come, VC6 leaves more of the second SphereIntersect's calls
// out of line than the original.
// Not matching yet: the stack slots of the first SphereIntersect's temporaries.
// FUNCTION: LS3DF 0x10012140
static void sphTestSubtree(I3D_frame *frm)
{
    if (frm == pWSubTree)
        return;
    if (!(frm->frm_flags & FRMFLAGS_ON) && !(colFlags & COLF_HIDDEN))
        return;
    const I3D_bsphere &bs = frm->GetHRBoundVolumeW().bsphere;
    if (bs.radius > 0.0f) {
        if (!SphereIntersect(bs))
            return;
    } else if (frm->GetType() != FRAME_SECTOR)
        return;
    if (frm->GetType() == FRAME_VISUAL && frm != pWFrame) {
        I3D_visual *vis = (I3D_visual *)frm;
        const I3D_bvolume &bv = vis->GetWBoundVolume();
        if (BoxIntersect(sphBox, bv.bbox) && SphereIntersect(bv.bsphere) &&
            !(vis->vis_flags & VISF_UNKNOWN_1000 && colFlags & COLF_UNKNOWN_4000000)) {
            switch (vis->visual_type) {
            case I3D_VISUAL_OBJECT:
            case I3D_VISUAL_LIT_OBJECT:
            case I3D_VISUAL_BILLBOARD:
            case I3D_VISUAL_SINGLEMESH:
            case I3D_VISUAL_MORPH:
            case I3D_VISUAL_SINGLEMORPH:
            case I3D_VISUAL_MIRROR:
                cbResult = COLL_ACCEPT;
                sphTestVisual(vis);
                if (cbResult == COLL_IGNORE_CHILDREN)
                    return;
            }
        }
    }
    for (I3D_frame *f = frm->child; f; f = f->next) {
        if (breakTests)
            break;
        if (cbResult == COLL_ACCEPT_STOP)
            break;
        sphTestSubtree(f);
    }
}

I3D_frame *I3D_scene::TestColSphere(const S_vector &from, const S_vector &dir, float radius, uint flags, S_vector *pos,
    I3D_frame *frm, I3D_COLLISION_CALLBACK *cb) const
{
    lineBase = from;
    lineDir.SetNormalized(dir);
    lineSize2 = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
    lineSize = (float)sqrt(lineSize2);
    lineDest = from + dir;
    sphBox.min = from;
    sphBox.max = sphBox.min;
    sphBox.Absorb(lineDest);
    sphBox.min.x -= radius;
    sphBox.min.y -= radius;
    sphBox.min.z -= radius;
    sphBox.max.x += radius;
    sphBox.max.y += radius;
    sphBox.max.z += radius;
    radiusA = radius;
    lpNearestObj = NULL;
    vzdalenost2 = 1e16f;
    colFlags = flags;
    lpCallbackGeo = cb;
    cbResult = COLL_ACCEPT;
    cbContext = NULL;
    pThisScene = this;
    if (flags & COLF_EXCLUDE_SUBTREE)
        pWSubTree = frm;
    else {
        pWFrame = frm;
        pWSubTree = NULL;
    }
    sphTestSubtree(primary_sector);
    if (flags & COLF_OMNI_SECTOR)
        sphTestSubtree(omni_sector);
    if (flags & COLF_NEAR_SECTOR)
        sphTestSubtree(near_sector);
    if (lpNearestObj && pos)
        *pos = colPointSph;
    return lpNearestObj;
}

// The box of the move is the boxes of the sphere at both ends
// FUNCTION: LS3DF 0x10014aa0
I3D_frame *I3D_scene::TestColSphereHierarchy(const S_vector &from, const S_vector &dir, float radius, I3D_frame *root,
    uint flags, S_vector *pos, I3D_frame *frm, I3D_COLLISION_CALLBACK *cb) const
{
    lineBase = from;
    lineDir.SetNormalized(dir);
    lineSize2 = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
    lineSize = (float)sqrt(lineSize2);
    lineDest = from + dir;
    sphBox.min = from - lineDir * radius;
    sphBox.max = sphBox.min;
    sphBox.Absorb(lineDest + lineDir * radius);
    radiusA = radius;
    lpNearestObj = NULL;
    vzdalenost2 = 1e16f;
    colFlags = flags;
    lpCallbackGeo = cb;
    cbResult = COLL_ACCEPT;
    cbContext = NULL;
    pThisScene = this;
    if (flags & COLF_EXCLUDE_SUBTREE)
        pWSubTree = frm;
    else {
        pWFrame = frm;
        pWSubTree = NULL;
    }
    sphTestSubtree(root);
    if (lpNearestObj && pos)
        *pos = colPointSph;
    return lpNearestObj;
}

// The moving sphere against one visual. Its SphereIntersect, written after more inline calls than
// TestColSphereHierarchy's code, is the call whose out-of-line copy the linker kept (0x100152a0).
// Not matching yet: one sum's operand order in lineDest + lineDir * radius (the allocator's live-range ids).
// FUNCTION: LS3DF 0x10014e50
I3D_frame *I3D_scene::TestColSphereVisual(const S_vector &from, const S_vector &dir, float radius, I3D_visual *vis,
    uint flags, S_vector *pos) const
{
    lineBase = from;
    lineDir.SetNormalized(dir);
    lineSize2 = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
    lineSize = (float)sqrt(lineSize2);
    lineDest = from + dir;
    sphBox.min = from - lineDir * radius;
    sphBox.max = sphBox.min;
    sphBox.Absorb(lineDest + lineDir * radius);
    radiusA = radius;
    lpNearestObj = NULL;
    vzdalenost2 = 1e16f;
    colFlags = flags;
    lpCallbackGeo = NULL;
    cbResult = COLL_ACCEPT;
    cbContext = NULL;
    pThisScene = this;
    if (!(vis->frm_flags & FRMFLAGS_ON) && !(colFlags & COLF_HIDDEN))
        return NULL;
    const I3D_bvolume &bv = vis->GetWBoundVolume();
    if (BoxIntersect(sphBox, bv.bbox) && SphereIntersect(bv.bsphere) &&
        !(vis->vis_flags & VISF_UNKNOWN_1000 && colFlags & COLF_UNKNOWN_4000000)) {
        switch (vis->GetVisualType()) {
        case I3D_VISUAL_OBJECT:
        case I3D_VISUAL_LIT_OBJECT:
        case I3D_VISUAL_BILLBOARD:
        case I3D_VISUAL_SINGLEMESH:
        case I3D_VISUAL_MORPH:
        case I3D_VISUAL_SINGLEMORPH:
        case I3D_VISUAL_MIRROR:
            break;
        default:
            return NULL;
        }
        sphTestVisual(vis);
    }
    if (lpNearestObj && pos)
        *pos = colPointSph;
    return lpNearestObj;
}

// The relative move's quadratic: the fraction of the moves at which the spheres are radius1 + radius2 apart, the
// smaller root if it is within the moves
// FUNCTION: LS3DF 0x100153b0
float I3D_scene::SphereToSphereD(float radius1, const S_vector &from1, const S_vector &to1, float radius2,
    const S_vector &from2, const S_vector &to2) const
{
    S_vector d1 = to1 - from1;
    S_vector d2 = to2 - from2;
    S_vector d = from2 - from1;
    S_vector dd = d2 - d1;
    float r = radius1 + radius2;
    float a = dd.Dot(dd);
    float rr = r * r;
    if (a <= rr)
        return 0.0f;
    float b = d.Dot(dd) * 2.0f;
    float c = d.Dot(d) - rr;
    float disc = b * b - 4.0f * a * c;
    // Negated: disc >= 0.0f tests the other flag (test ah, 1)
    if (!(disc < 0.0f)) {
        float s = (float)sqrt(disc);
        float f = 0.5f / a;
        float t1 = (s - b) * f;
        float t2 = (-b - s) * f;
        if (t1 > t2)
            t1 = t2;
        if (t1 <= 1.0f && t1 >= 0.0f)
            return t1;
    }
    return -1.0f;
}
