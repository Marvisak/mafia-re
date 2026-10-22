// The clipper's entry points (PC only; clipping.cpp is the name the overflow message gives), at 0x10003b40-0x10004870
// in the original: a mesh's vertices into clip space, then its face groups' faces through clipper2.cpp's clipping and
// their vertices into the result. clipper.h describes the clipper. The header's zero vector has its $E at the end
// (0x10004850). ClipFacegroups is static: it comes after its first caller, ClipMesh, wherever it is defined.
// Shapes the original needs: ClipMesh and ClipSingleMesh walk a pointer of their own through the vertices; the
// world-space ones index the clip vertices (the pointer VC6 walks them with is on clip), with clip_verts read in each
// branch after the loop's guard, which an if around the loop gives; ClipFacegroups' result loop indexes a copy of
// clip_verts up to a count of its own.
// The face overflow check is a helper of its own (CheckClipFaces, the name is a guess), which ClipFacegroups inlines:
// the original's data has its message first, then ClipFacegroups' other two in reverse order of use. VC6 emits each
// function's literals after its code, so the message came from a function compiled before ClipFacegroups; the helper
// isn't static, so VC6 also compiles a copy of it where it is defined, which nothing calls and the linker drops. The
// message cites clipping.cpp, the other two clipper.cpp.
// ClipFacegroups assigns clip_two_sided once after the blend test, and VC6 copies it into both branches; written in
// each branch, the copies and the face groups' calls took other registers.
// Open: the world-space ones' three temporaries before and at the loop's top: the original's clip_verts copy, matrix
// and result address take ecx, edx and eax, ours eax, ecx and edx (a rotation, the same in both functions and both
// loops, and with the perspective branch removed). Dead locals, (void) uses, the positions of i, v and cv, references
// for the matrix, the vertex or the clip vertex, and the loop's other forms don't move it.
#include "I3D/I3D.h"
#include "I3D/render/clipper.h"

#include <string.h>

static bool ClipFacegroups(I3D_face_group *fg);

// The opaque faces still fit their block (ClipFacegroups, after it has added them)
bool CheckClipFaces()
{
    if (num_clip_faces > clip_faces_alloc) {
        dbgPrintf("!! Too many faces of projector/shadow (clipping.cpp)");
        return false;
    }
    return true;
}

// A mesh's vertices into clip space through m and the volume's matrix, then its face groups
// FUNCTION: LS3DF 0x10003b40
// FUNCTION: LS3DF11 0x10003b40
bool ClipMesh(const I3D_vertex *verts, int num_verts, I3D_face_group *fgroups, int num_faces, uint transp,
    const S_matrix &m)
{
    if (!ReserveClipArrays(num_verts, num_faces))
        return false;
    clip_with_alpha = transp;
    S_clip_vertex *cv = clip_verts;
    int i;
    const I3D_vertex *v = verts;
    if (clip_is_ortho) {
        S_matrix mm = m * *clip_matrix;
        for (i = num_verts; i; i--, cv++, v++) {
            cv->pos = v->pos * mm;
            if (cv->pos.x < -1.0f)
                cv->clip = CLIP_LEFT;
            else if (cv->pos.x > 1.0f)
                cv->clip = CLIP_RIGHT;
            else
                cv->clip = 0;
            if (cv->pos.y < -1.0f)
                cv->clip |= CLIP_BOTTOM;
            else if (cv->pos.y > 1.0f)
                cv->clip |= CLIP_TOP;
            if (cv->pos.z < 0.0f)
                cv->clip |= CLIP_NEAR;
            else if (cv->pos.z > 1.0f)
                cv->clip |= CLIP_FAR;
            cv->index = -1;
        }
    } else {
        S_matrix mm = m * *clip_matrix;
        for (i = num_verts; i; i--, cv++, v++) {
            cv->pos = v->pos * mm;
            if (cv->pos.z < 0.0f) {
                float z = -cv->pos.z;
                cv->clip = CLIP_NEAR;
                if (fabs(cv->pos.x) > z) {
                    if (cv->pos.x < 0.0f)
                        cv->clip |= CLIP_LEFT;
                    else
                        cv->clip |= CLIP_RIGHT;
                }
                if (fabs(cv->pos.y) > z) {
                    if (cv->pos.y < 0.0f)
                        cv->clip |= CLIP_BOTTOM;
                    else
                        cv->clip |= CLIP_TOP;
                }
            } else {
                if (cv->pos.z > 1.0f)
                    cv->clip = CLIP_FAR;
                else
                    cv->clip = 0;
                if (fabs(cv->pos.x) > cv->pos.z) {
                    if (cv->pos.x < 0.0f)
                        cv->clip |= CLIP_LEFT;
                    else
                        cv->clip |= CLIP_RIGHT;
                }
                if (fabs(cv->pos.y) > cv->pos.z) {
                    if (cv->pos.y < 0.0f)
                        cv->clip |= CLIP_BOTTOM;
                    else
                        cv->clip |= CLIP_TOP;
                }
            }
            cv->index = -1;
        }
    }
    clip_vert_count = num_verts;
    return ClipFacegroups(fgroups);
}

// The face groups' faces into the two lists, and their vertices into the result
// FUNCTION: LS3DF 0x10003dc0
// FUNCTION: LS3DF11 0x10003dc0
static bool ClipFacegroups(I3D_face_group *fg)
{
    clip_vert_end = clip_verts + clip_vert_count;
    clip_next_face = clip_faces + num_clip_faces;
    clip_next_face_t = clip_faces_t + num_clip_faces_t;
    clip_added_faces = 0;
    clip_added_transp = 0;
    for (; fg; fg = fg->next) {
        I3D_material *mat = fg->material;
        if (mat) {
            if (mat->flags & (MATF_CKEY_ALPHA | MATF_TXT_ALPHA | MATF_ADD_MODE) || mat->alpha < 1.0f) {
                clip_fg_blend = true;
                if (!clip_with_alpha)
                    continue;
            } else
                clip_fg_blend = false;
            clip_two_sided = mat->flags & MATF_TWO_SIDED;
        } else {
            clip_two_sided = 0;
            clip_fg_blend = 0;
        }
        if (clip_backfaces)
            clip_two_sided = 1;
        if (clip_is_ortho)
            ClipFacesOrtho(fg->GetFaces(), fg->num_faces);
        else if (!ClipFacesPersp(fg->GetFaces(), fg->num_faces))
            return false;
    }
    if (!clip_added_faces && !clip_added_transp)
        return false;
    // The faces' vertices get their result indices
    short index = num_clip_results;
    int i;
    if (clip_added_faces) {
        ushort *p = clip_faces[num_clip_faces].vi;
        num_clip_faces += clip_added_faces;
        if (!CheckClipFaces())
            return false;
        for (i = clip_added_faces * 3; i; i--, p++) {
            S_clip_vertex *v = &clip_verts[*p];
            if (v->index < 0) {
                v->index = index;
                *p = index;
                index++;
            } else
                *p = v->index;
        }
    }
    if (clip_added_transp) {
        ushort *p = clip_faces_t[num_clip_faces_t].vi;
        num_clip_faces_t += clip_added_transp;
        for (i = clip_added_transp * 3; i; i--, p++) {
            S_clip_vertex *v = &clip_verts[*p];
            if (v->index < 0) {
                v->index = index;
                *p = index;
                index++;
                clip_center_t += v->pos;
            } else
                *p = v->index;
        }
    }
    int num = index;
    if (num > 65000) {
        dbgPrintf("!! Too many result vertices:%d  ClipFacegroups()  clipper.cpp", num);
        num_clip_faces -= clip_added_faces;
        num_clip_faces_t -= clip_added_transp;
        return false;
    }
    if (clip_result_alloc < num) {
        if (num < 512)
            num = 512;
        S_vector *result = new S_vector[num];
        if (!result) {
            dbgPrintf("!! Out of memory  ClipFacegroups()  result vertices:%d  clipper.cpp", num);
            return false;
        }
        if (num_clip_results)
            memcpy(result, clip_results, num_clip_results * sizeof(S_vector));
        delete[] clip_results;
        clip_result_alloc = num;
        clip_results = result;
    }
    int n = 0;
    S_clip_vertex *v = clip_verts;
    int count = clip_vert_count;
    for (i = 0; i != count; i++) {
        if (v[i].index >= 0) {
            clip_results[v[i].index] = v[i].pos;
            n++;
        }
    }
    num_clip_results += n;
    return true;
}

// The same with the vertices in world space
// FUNCTION: LS3DF 0x10004110
// FUNCTION: LS3DF11 0x10004110
bool ClipMeshW(const I3D_vertex *verts, int num_verts, I3D_face_group *fgroups, int num_faces, uint transp)
{
    if (!ReserveClipArrays(num_verts, num_faces))
        return false;
    clip_with_alpha = transp;
    int i;
    const I3D_vertex *v = verts;
    if (clip_is_ortho) {
        if (num_verts) {
            S_clip_vertex *cv = clip_verts;
            for (i = 0; i != num_verts; i++, v++) {
                cv[i].pos = v->pos * *clip_matrix;
                if (cv[i].pos.x < -1.0f)
                    cv[i].clip = CLIP_LEFT;
                else if (cv[i].pos.x > 1.0f)
                    cv[i].clip = CLIP_RIGHT;
                else
                    cv[i].clip = 0;
                if (cv[i].pos.y < -1.0f)
                    cv[i].clip |= CLIP_BOTTOM;
                else if (cv[i].pos.y > 1.0f)
                    cv[i].clip |= CLIP_TOP;
                if (cv[i].pos.z < 0.0f)
                    cv[i].clip |= CLIP_NEAR;
                else if (cv[i].pos.z > 1.0f)
                    cv[i].clip |= CLIP_FAR;
                cv[i].index = -1;
            }
        }
    } else {
        if (num_verts) {
            S_clip_vertex *cv = clip_verts;
            for (i = 0; i != num_verts; i++, v++) {
                cv[i].pos = v->pos * *clip_matrix;
                if (cv[i].pos.z < 0.0f) {
                    float z = -cv[i].pos.z;
                    cv[i].clip = CLIP_NEAR;
                    if (fabs(cv[i].pos.x) > z) {
                        if (cv[i].pos.x < 0.0f)
                            cv[i].clip |= CLIP_LEFT;
                        else
                            cv[i].clip |= CLIP_RIGHT;
                    }
                    if (fabs(cv[i].pos.y) > z) {
                        if (cv[i].pos.y < 0.0f)
                            cv[i].clip |= CLIP_BOTTOM;
                        else
                            cv[i].clip |= CLIP_TOP;
                    }
                } else {
                    if (cv[i].pos.z > 1.0f)
                        cv[i].clip = CLIP_FAR;
                    else
                        cv[i].clip = 0;
                    if (fabs(cv[i].pos.x) > cv[i].pos.z) {
                        if (cv[i].pos.x < 0.0f)
                            cv[i].clip |= CLIP_LEFT;
                        else
                            cv[i].clip |= CLIP_RIGHT;
                    }
                    if (fabs(cv[i].pos.y) > cv[i].pos.z) {
                        if (cv[i].pos.y < 0.0f)
                            cv[i].clip |= CLIP_BOTTOM;
                        else
                            cv[i].clip |= CLIP_TOP;
                    }
                }
                cv[i].index = -1;
            }
        }
    }
    clip_vert_count = num_verts;
    return ClipFacegroups(fgroups);
}

// ClipMesh for a single mesh's or single morph's vertices
// FUNCTION: LS3DF 0x10004370
// FUNCTION: LS3DF11 0x10004370
bool ClipSingleMesh(const S_clip_sm_vertex *verts, int num_verts, I3D_face_group *fgroups, int num_faces, uint transp,
    const S_matrix &m)
{
    if (!ReserveClipArrays(num_verts, num_faces))
        return false;
    clip_with_alpha = transp;
    S_clip_vertex *cv = clip_verts;
    int i;
    const S_clip_sm_vertex *v = verts;
    if (clip_is_ortho) {
        S_matrix mm = m * *clip_matrix;
        for (i = num_verts; i; i--, cv++, v++) {
            cv->pos = v->pos * mm;
            if (cv->pos.x < -1.0f)
                cv->clip = CLIP_LEFT;
            else if (cv->pos.x > 1.0f)
                cv->clip = CLIP_RIGHT;
            else
                cv->clip = 0;
            if (cv->pos.y < -1.0f)
                cv->clip |= CLIP_BOTTOM;
            else if (cv->pos.y > 1.0f)
                cv->clip |= CLIP_TOP;
            if (cv->pos.z < 0.0f)
                cv->clip |= CLIP_NEAR;
            else if (cv->pos.z > 1.0f)
                cv->clip |= CLIP_FAR;
            cv->index = -1;
        }
    } else {
        S_matrix mm = m * *clip_matrix;
        for (i = num_verts; i; i--, cv++, v++) {
            cv->pos = v->pos * mm;
            if (cv->pos.z < 0.0f) {
                float z = -cv->pos.z;
                cv->clip = CLIP_NEAR;
                if (fabs(cv->pos.x) > z) {
                    if (cv->pos.x < 0.0f)
                        cv->clip |= CLIP_LEFT;
                    else
                        cv->clip |= CLIP_RIGHT;
                }
                if (fabs(cv->pos.y) > z) {
                    if (cv->pos.y < 0.0f)
                        cv->clip |= CLIP_BOTTOM;
                    else
                        cv->clip |= CLIP_TOP;
                }
            } else {
                if (cv->pos.z > 1.0f)
                    cv->clip = CLIP_FAR;
                else
                    cv->clip = 0;
                if (fabs(cv->pos.x) > cv->pos.z) {
                    if (cv->pos.x < 0.0f)
                        cv->clip |= CLIP_LEFT;
                    else
                        cv->clip |= CLIP_RIGHT;
                }
                if (fabs(cv->pos.y) > cv->pos.z) {
                    if (cv->pos.y < 0.0f)
                        cv->clip |= CLIP_BOTTOM;
                    else
                        cv->clip |= CLIP_TOP;
                }
            }
            cv->index = -1;
        }
    }
    clip_vert_count = num_verts;
    return ClipFacegroups(fgroups);
}

// ClipMeshW for a single mesh's or single morph's vertices
// FUNCTION: LS3DF 0x100045f0
// FUNCTION: LS3DF11 0x100045f0
bool ClipSingleMeshW(const S_clip_sm_vertex *verts, int num_verts, I3D_face_group *fgroups, int num_faces,
    uint transp)
{
    if (!ReserveClipArrays(num_verts, num_faces))
        return false;
    clip_with_alpha = transp;
    int i;
    const S_clip_sm_vertex *v = verts;
    if (clip_is_ortho) {
        if (num_verts) {
            S_clip_vertex *cv = clip_verts;
            for (i = 0; i != num_verts; i++, v++) {
                cv[i].pos = v->pos * *clip_matrix;
                if (cv[i].pos.x < -1.0f)
                    cv[i].clip = CLIP_LEFT;
                else if (cv[i].pos.x > 1.0f)
                    cv[i].clip = CLIP_RIGHT;
                else
                    cv[i].clip = 0;
                if (cv[i].pos.y < -1.0f)
                    cv[i].clip |= CLIP_BOTTOM;
                else if (cv[i].pos.y > 1.0f)
                    cv[i].clip |= CLIP_TOP;
                if (cv[i].pos.z < 0.0f)
                    cv[i].clip |= CLIP_NEAR;
                else if (cv[i].pos.z > 1.0f)
                    cv[i].clip |= CLIP_FAR;
                cv[i].index = -1;
            }
        }
    } else {
        if (num_verts) {
            S_clip_vertex *cv = clip_verts;
            for (i = 0; i != num_verts; i++, v++) {
                cv[i].pos = v->pos * *clip_matrix;
                if (cv[i].pos.z < 0.0f) {
                    float z = -cv[i].pos.z;
                    cv[i].clip = CLIP_NEAR;
                    if (fabs(cv[i].pos.x) > z) {
                        if (cv[i].pos.x < 0.0f)
                            cv[i].clip |= CLIP_LEFT;
                        else
                            cv[i].clip |= CLIP_RIGHT;
                    }
                    if (fabs(cv[i].pos.y) > z) {
                        if (cv[i].pos.y < 0.0f)
                            cv[i].clip |= CLIP_BOTTOM;
                        else
                            cv[i].clip |= CLIP_TOP;
                    }
                } else {
                    if (cv[i].pos.z > 1.0f)
                        cv[i].clip = CLIP_FAR;
                    else
                        cv[i].clip = 0;
                    if (fabs(cv[i].pos.x) > cv[i].pos.z) {
                        if (cv[i].pos.x < 0.0f)
                            cv[i].clip |= CLIP_LEFT;
                        else
                            cv[i].clip |= CLIP_RIGHT;
                    }
                    if (fabs(cv[i].pos.y) > cv[i].pos.z) {
                        if (cv[i].pos.y < 0.0f)
                            cv[i].clip |= CLIP_BOTTOM;
                        else
                            cv[i].clip |= CLIP_TOP;
                    }
                }
                cv[i].index = -1;
            }
        }
    }
    clip_vert_count = num_verts;
    return ClipFacegroups(fgroups);
}
