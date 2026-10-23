// The clipper's faces (PC only; the name is a guess: the file sorts between clipper.cpp and clipping.cpp), at
// 0x100013e0-0x10003b40 in the original: the faces of an orthogonal and of a perspective volume, clipped to it.
// clipper.h describes the clipper. The file was compiled with clipper.cpp's precompiled header (/Yu): it has no $E for
// the header's zero vector, and the vector isn't in its .bss.
// A face outside one plane is dropped, and one turned away from the projector culled unless the reverse factor lets
// it through. The others are clipped as a polygon against the near and far planes, then x, then y, from one index
// buffer into the other, and fanned into triangles; the perspective volume's polygon also gets its edges that span
// more depth than RsNonOrthoTessellation split, and is triangulated from its nearest vertex. The clipping's new
// vertices then move along the face's normal (clip_lift) and away from the polygon's center (clip_spread). The
// orthogonal planes' copies of the near and far plane code are written out, as the original's differ: all but one
// compute y twice where z is constant. Both functions' spread step sets n.z where d.z is meant, in the original too.
// Shapes the original needs: the near plane's t as (0.0f - a->pos.z) / ..., like the far plane's (1.0f - a->pos.z)
// (VC6 makes -a->pos.z / ... into -(a->pos.z / ...); (-a->pos.z) / ... compiles the same as 0.0f -); the cull's
// threshold on the left of its compare.
// - Each pass's edge code reaches the a-inside case's two vertex blocks by goto: an edge leaving through one plane
//   jumps to the block of that plane, an edge crossing both writes its first vertex inline and jumps to the other's.
//   The original has only those four blocks per pass, and nothing short of a join explains it: Persp's both-planes
//   block and the jump target begin with three identical instructions, which VC6 would hoist above the branch were
//   the target reached from there alone, and the second vertex reloads clip_vert_count where a straight-line copy
//   keeps it in a register. Written out (nested if/else, one copy per case), each function grew by 60 to 125
//   instructions and every register moved.
// - The face count is a local copy (count); counting the parameter down gives it a register and moves the others.
// - After each pass the plane flags are collected through a pointer and a count of the remaining vertices (the
//   count-down loop); for (i = 1; i < n; i++) keeps the index.
// - The center is scaled through S_vector::operator*=: while it is summed, its members then sit on the FPU stack in
//   the original's order (y, z, x); a reciprocal multiplied into each member gives z, y, x.
// - Float sums whose term order VC6 picks itself: Ortho's spread sum is (d.z * d.z + d.x * d.x) + d.y * d.y, the
//   original's x, z, y (the parentheses count: unparenthesized, the same terms compile differently), and Persp's
//   lift sum n.x * n.x + (n.y * n.y + n.z * n.z) (with count, the plain sum squares x first); the other sums are
//   written plainly.
// - Ortho's last pass sets clip_poly_num before closing the polygon (the other two passes after it): its value is
//   then computed in a register of its own while out stays live.
// - Persp's triangulation: the first index after the nearest vertex is a local (first), which keeps VC6 from
//   reassociating n - (min_i + 1); the fan's i and j are declared before the first triangle's stores.
// Open: Persp's tessellation step (b->pos - a->pos) / dz. The original consumes dz in the reciprocal (fxch st(2);
// fdivr [1.0]), which VC6 gives only when S_vector::operator/ reuses its parameter (f = 1.0f / f; return S_vector(x *
// f, y * f, z * f)), a change to the math header; the header's local reciprocal keeps dz on the FPU stack (fld 1.0;
// fdiv st(3)). No spelling in this file gives it. The order of the float literals (0.0 before -1.0) follows the code.
#include <new>

#include "I3D/render/clipper.h"

#include <string.h>

// Its .bss is in the reverse order of the definitions: under /Yu VC6 doesn't order an object's uninitialized
// globals by the hash of their names (tools/bsshash.py), the last one defined comes first.

// The vertices of the mesh being clipped and the clipping's
// GLOBAL: LS3DF 0x100af7c8
// GLOBAL: LS3DF11 0x100ac8e8
int clip_vert_count;

// The polygon, every other plane
// GLOBAL: LS3DF 0x100af3c8
// GLOBAL: LS3DF11 0x100ac4e8
ushort clip_poly_buf1[512];

// The faces of an orthogonal volume, in the unit box
// FUNCTION: LS3DF 0x100013e0
// FUNCTION: LS3DF11 0x100013e0
void ClipFacesOrtho(const I3D_triface *faces, int num_faces)
{
    int first_vert = clip_vert_count;
    I3D_triface *fo;
    if (clip_fg_blend)
        fo = clip_next_face_t;
    else
        fo = clip_next_face;
    int count = num_faces;
    for (; count--; faces++) {
        S_clip_vertex *v0 = &clip_verts[faces->vi[0]];
        S_clip_vertex *v1 = &clip_verts[faces->vi[1]];
        S_clip_vertex *v2 = &clip_verts[faces->vi[2]];
        if (v0->clip & v1->clip & v2->clip)
            continue;
        if (!clip_two_sided) {
            S_vector n;
            n.GetNormal(v0->pos, v1->pos, v2->pos);
            if (n.z > 0.0f) {
                if (IsMrgZeroLess(clip_rev_factor))
                    continue;
                float x = clip_ratio_x * n.x, y = clip_ratio_y * n.y;
                if (clip_inv_reverse * n.z * n.z > x * x + y * y + n.z * n.z)
                    continue;
            }
        }
        ushort clip = v0->clip | v1->clip | v2->clip;
        if (!clip)
            *fo++ = *faces;
        else {
            clip_poly_buf1[0] = faces->vi[0];
            clip_poly_buf1[3] = faces->vi[0];
            clip_poly_buf1[1] = faces->vi[1];
            clip_poly_buf1[2] = faces->vi[2];
            ushort *in = clip_poly_buf1, *out = clip_poly_buf2;
            bool swap = false;
            clip_poly_num = 3;
            float t;
            if (clip & (CLIP_NEAR | CLIP_FAR)) {
                while (clip_poly_num) {
                    clip_poly_num--;
                    S_clip_vertex *a = &clip_verts[in[0]];
                    S_clip_vertex *b = &clip_verts[in[1]];
                    if (a->clip & CLIP_NEAR) {
                        if (!(b->clip & CLIP_NEAR)) {
                            if (b->clip & CLIP_FAR) {
                                *out++ = clip_vert_count;
                                clip_vert_end->pos.z = 0.0f;
                                t = (0.0f - a->pos.z) / (b->pos.z - a->pos.z);
                                clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                                clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                                if (clip_vert_end->pos.x < -1.0f)
                                    clip_vert_end->clip = CLIP_LEFT;
                                else if (clip_vert_end->pos.x > 1.0f)
                                    clip_vert_end->clip = CLIP_RIGHT;
                                else
                                    clip_vert_end->clip = 0;
                                if (clip_vert_end->pos.y < -1.0f)
                                    clip_vert_end->clip |= CLIP_BOTTOM;
                                else if (clip_vert_end->pos.y > 1.0f)
                                    clip_vert_end->clip |= CLIP_TOP;
                                clip_vert_end->index = -1;
                                clip_vert_end++;
                                clip_vert_count++;
                                goto far_vertex;
                            } else
                                goto near_vertex;
                        }
                    } else if (a->clip & CLIP_FAR) {
                        if (b->clip & CLIP_NEAR) {
                            *out++ = clip_vert_count;
                            clip_vert_end->pos.z = 1.0f;
                            t = (1.0f - a->pos.z) / (b->pos.z - a->pos.z);
                            clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                            clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                            clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                            if (clip_vert_end->pos.x < -1.0f)
                                clip_vert_end->clip = CLIP_LEFT;
                            else if (clip_vert_end->pos.x > 1.0f)
                                clip_vert_end->clip = CLIP_RIGHT;
                            else
                                clip_vert_end->clip = 0;
                            if (clip_vert_end->pos.y < -1.0f)
                                clip_vert_end->clip |= CLIP_BOTTOM;
                            else if (clip_vert_end->pos.y > 1.0f)
                                clip_vert_end->clip |= CLIP_TOP;
                            clip_vert_end->index = -1;
                            clip_vert_end++;
                            clip_vert_count++;
                            goto near_vertex;
                        } else if (b->clip & CLIP_FAR) {
                            // both beyond the far plane: no vertex
                        } else
                            goto far_vertex;
                    } else {
                        *out++ = in[0];
                        if (b->clip & CLIP_NEAR) {
                    near_vertex:
                            *out++ = clip_vert_count;
                            clip_vert_end->pos.z = 0.0f;
                            t = (0.0f - a->pos.z) / (b->pos.z - a->pos.z);
                            clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                            clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                            clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                            if (clip_vert_end->pos.x < -1.0f)
                                clip_vert_end->clip = CLIP_LEFT;
                            else if (clip_vert_end->pos.x > 1.0f)
                                clip_vert_end->clip = CLIP_RIGHT;
                            else
                                clip_vert_end->clip = 0;
                            if (clip_vert_end->pos.y < -1.0f)
                                clip_vert_end->clip |= CLIP_BOTTOM;
                            else if (clip_vert_end->pos.y > 1.0f)
                                clip_vert_end->clip |= CLIP_TOP;
                            clip_vert_end->index = -1;
                            clip_vert_end++;
                            clip_vert_count++;
                        } else if (b->clip & CLIP_FAR) {
                    far_vertex:
                            *out++ = clip_vert_count;
                            clip_vert_end->pos.z = 1.0f;
                            t = (1.0f - a->pos.z) / (b->pos.z - a->pos.z);
                            clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                            clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                            clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                            if (clip_vert_end->pos.x < -1.0f)
                                clip_vert_end->clip = CLIP_LEFT;
                            else if (clip_vert_end->pos.x > 1.0f)
                                clip_vert_end->clip = CLIP_RIGHT;
                            else
                                clip_vert_end->clip = 0;
                            if (clip_vert_end->pos.y < -1.0f)
                                clip_vert_end->clip |= CLIP_BOTTOM;
                            else if (clip_vert_end->pos.y > 1.0f)
                                clip_vert_end->clip |= CLIP_TOP;
                            clip_vert_end->index = -1;
                            clip_vert_end++;
                            clip_vert_count++;
                        }
                    }
                    in++;
                }
                // The polygon is in out: close it, and clip it from there
                if (swap) {
                    *out = clip_poly_buf1[0];
                    swap = false;
                    clip_poly_num = out - clip_poly_buf1;
                    in = clip_poly_buf1;
                    out = clip_poly_buf2;
                } else {
                    *out = clip_poly_buf2[0];
                    swap = true;
                    clip_poly_num = out - clip_poly_buf2;
                    in = clip_poly_buf2;
                    out = clip_poly_buf1;
                }
                if (clip_poly_num > 0) {
                    ushort *p = in;
                    clip = clip_verts[*p++].clip;
                    for (int i = clip_poly_num - 1; i; i--)
                        clip |= clip_verts[*p++].clip;
                } else
                    clip = 0;
            }
            if (clip & (CLIP_LEFT | CLIP_RIGHT)) {
                while (clip_poly_num) {
                    clip_poly_num--;
                    S_clip_vertex *a = &clip_verts[in[0]];
                    S_clip_vertex *b = &clip_verts[in[1]];
                    if (a->clip & CLIP_LEFT) {
                        if (!(b->clip & CLIP_LEFT)) {
                            if (b->clip & CLIP_RIGHT) {
                                *out++ = clip_vert_count;
                                clip_vert_end->pos.x = -1.0f;
                                t = (-1.0f - a->pos.x) / (b->pos.x - a->pos.x);
                                clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                                clip_vert_end->pos.z = (b->pos.z - a->pos.z) * t + a->pos.z;
                                if (clip_vert_end->pos.y < -1.0f)
                                    clip_vert_end->clip = CLIP_BOTTOM;
                                else if (clip_vert_end->pos.y > 1.0f)
                                    clip_vert_end->clip = CLIP_TOP;
                                else
                                    clip_vert_end->clip = 0;
                                clip_vert_end->index = -1;
                                clip_vert_end++;
                                clip_vert_count++;
                                goto right_vertex;
                            } else
                                goto left_vertex;
                        }
                    } else if (a->clip & CLIP_RIGHT) {
                        if (b->clip & CLIP_LEFT) {
                            *out++ = clip_vert_count;
                            clip_vert_end->pos.x = 1.0f;
                            t = (1.0f - a->pos.x) / (b->pos.x - a->pos.x);
                            clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                            clip_vert_end->pos.z = (b->pos.z - a->pos.z) * t + a->pos.z;
                            if (clip_vert_end->pos.y < -1.0f)
                                clip_vert_end->clip = CLIP_BOTTOM;
                            else if (clip_vert_end->pos.y > 1.0f)
                                clip_vert_end->clip = CLIP_TOP;
                            else
                                clip_vert_end->clip = 0;
                            clip_vert_end->index = -1;
                            clip_vert_end++;
                            clip_vert_count++;
                            goto left_vertex;
                        } else if (b->clip & CLIP_RIGHT) {
                            // both beyond the right plane: no vertex
                        } else
                            goto right_vertex;
                    } else {
                        *out++ = in[0];
                        if (b->clip & CLIP_LEFT) {
                    left_vertex:
                            *out++ = clip_vert_count;
                            clip_vert_end->pos.x = -1.0f;
                            t = (-1.0f - a->pos.x) / (b->pos.x - a->pos.x);
                            clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                            clip_vert_end->pos.z = (b->pos.z - a->pos.z) * t + a->pos.z;
                            if (clip_vert_end->pos.y < -1.0f)
                                clip_vert_end->clip = CLIP_BOTTOM;
                            else if (clip_vert_end->pos.y > 1.0f)
                                clip_vert_end->clip = CLIP_TOP;
                            else
                                clip_vert_end->clip = 0;
                            clip_vert_end->index = -1;
                            clip_vert_end++;
                            clip_vert_count++;
                        } else if (b->clip & CLIP_RIGHT) {
                    right_vertex:
                            *out++ = clip_vert_count;
                            clip_vert_end->pos.x = 1.0f;
                            t = (1.0f - a->pos.x) / (b->pos.x - a->pos.x);
                            clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                            clip_vert_end->pos.z = (b->pos.z - a->pos.z) * t + a->pos.z;
                            if (clip_vert_end->pos.y < -1.0f)
                                clip_vert_end->clip = CLIP_BOTTOM;
                            else if (clip_vert_end->pos.y > 1.0f)
                                clip_vert_end->clip = CLIP_TOP;
                            else
                                clip_vert_end->clip = 0;
                            clip_vert_end->index = -1;
                            clip_vert_end++;
                            clip_vert_count++;
                        }
                    }
                    in++;
                }
                // The polygon is in out: close it, and clip it from there
                if (swap) {
                    *out = clip_poly_buf1[0];
                    swap = false;
                    clip_poly_num = out - clip_poly_buf1;
                    in = clip_poly_buf1;
                    out = clip_poly_buf2;
                } else {
                    *out = clip_poly_buf2[0];
                    swap = true;
                    clip_poly_num = out - clip_poly_buf2;
                    in = clip_poly_buf2;
                    out = clip_poly_buf1;
                }
                if (clip_poly_num > 0) {
                    ushort *p = in;
                    clip = clip_verts[*p++].clip;
                    for (int i = clip_poly_num - 1; i; i--)
                        clip |= clip_verts[*p++].clip;
                } else
                    clip = 0;
            }
            if (clip & (CLIP_BOTTOM | CLIP_TOP)) {
                while (clip_poly_num) {
                    clip_poly_num--;
                    S_clip_vertex *a = &clip_verts[in[0]];
                    S_clip_vertex *b = &clip_verts[in[1]];
                    if (a->clip & CLIP_BOTTOM) {
                        if (!(b->clip & CLIP_BOTTOM)) {
                            if (b->clip & CLIP_TOP) {
                                *out++ = clip_vert_count;
                                clip_vert_end->pos.y = -1.0f;
                                t = (-1.0f - a->pos.y) / (b->pos.y - a->pos.y);
                                clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                                clip_vert_end->pos.z = (b->pos.z - a->pos.z) * t + a->pos.z;
                                clip_vert_end->index = -1;
                                clip_vert_end++;
                                clip_vert_count++;
                                goto top_vertex;
                            } else
                                goto bottom_vertex;
                        }
                    } else if (a->clip & CLIP_TOP) {
                        if (b->clip & CLIP_BOTTOM) {
                            *out++ = clip_vert_count;
                            clip_vert_end->pos.y = 1.0f;
                            t = (1.0f - a->pos.y) / (b->pos.y - a->pos.y);
                            clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                            clip_vert_end->pos.z = (b->pos.z - a->pos.z) * t + a->pos.z;
                            clip_vert_end->index = -1;
                            clip_vert_end++;
                            clip_vert_count++;
                            goto bottom_vertex;
                        } else if (b->clip & CLIP_TOP) {
                            // both beyond the top plane: no vertex
                        } else
                            goto top_vertex;
                    } else {
                        *out++ = in[0];
                        if (b->clip & CLIP_BOTTOM) {
                    bottom_vertex:
                            *out++ = clip_vert_count;
                            clip_vert_end->pos.y = -1.0f;
                            t = (-1.0f - a->pos.y) / (b->pos.y - a->pos.y);
                            clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                            clip_vert_end->pos.z = (b->pos.z - a->pos.z) * t + a->pos.z;
                            clip_vert_end->index = -1;
                            clip_vert_end++;
                            clip_vert_count++;
                        } else if (b->clip & CLIP_TOP) {
                    top_vertex:
                            *out++ = clip_vert_count;
                            clip_vert_end->pos.y = 1.0f;
                            t = (1.0f - a->pos.y) / (b->pos.y - a->pos.y);
                            clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                            clip_vert_end->pos.z = (b->pos.z - a->pos.z) * t + a->pos.z;
                            clip_vert_end->index = -1;
                            clip_vert_end++;
                            clip_vert_count++;
                        }
                    }
                    in++;
                }
                // The polygon is in out: close it, and clip it from there
                if (swap) {
                    clip_poly_num = out - clip_poly_buf1;
                    *out = clip_poly_buf1[0];
                    swap = false;
                    in = clip_poly_buf1;
                    out = clip_poly_buf2;
                } else {
                    clip_poly_num = out - clip_poly_buf2;
                    *out = clip_poly_buf2[0];
                    swap = true;
                    in = clip_poly_buf2;
                    out = clip_poly_buf1;
                }
                if (clip_poly_num > 0) {
                    ushort *p = in;
                    clip = clip_verts[*p++].clip;
                    for (int i = clip_poly_num - 1; i; i--)
                        clip |= clip_verts[*p++].clip;
                } else
                    clip = 0;
            }
            if (clip_poly_num > 2) {
                int i;
                for (i = 2; i < clip_poly_num; i++) {
                    fo->vi[0] = in[0];
                    fo->vi[1] = in[i - 1];
                    fo->vi[2] = in[i];
                    fo++;
                }
                // The new vertices move along the face's normal, and away from the polygon's center
                S_vector n;
                n.GetNormal(v0->pos, v1->pos, v2->pos);
                S_vector offs;
                float sq = n.x * n.x + n.y * n.y + n.z * n.z;
                if (sq < MRG_ZERO) {
                    if (n.x != 0.0f)
                        n.x = n.x < 0.0f ? -1.0f : 1.0f;
                    else if (n.z != 0.0f)
                        n.z = n.z < 0.0f ? -1.0f : 1.0f;
                    else
                        n.y = n.y < 0.0f ? -1.0f : 1.0f;
                    offs = n * clip_lift;
                } else
                    offs = n * (clip_lift / (float)sqrt(sq));
                S_vector c = clip_verts[in[0]].pos;
                for (i = 1; i < clip_poly_num; i++)
                    c += clip_verts[in[i]].pos;
                c *= 1.0f / clip_poly_num;
                for (i = 0; i < clip_poly_num; i++) {
                    int vi = in[i];
                    if (vi >= first_vert) {
                        clip_verts[vi].pos += offs;
                        if (clip_spread > 0.0f) {
                            S_vector d = clip_verts[vi].pos - c;
                            float sq = (d.z * d.z + d.x * d.x) + d.y * d.y;
                            if (sq < MRG_ZERO) {
                                if (d.x != 0.0f)
                                    d.x = d.x < 0.0f ? -1.0f : 1.0f;
                                // n.z, as in the original, so d.z is left as it is
                                else if (d.z != 0.0f)
                                    n.z = d.z < 0.0f ? -1.0f : 1.0f;
                                else
                                    d.y = d.y < 0.0f ? -1.0f : 1.0f;
                                d *= clip_spread;
                            } else
                                d *= clip_spread / (float)sqrt(sq);
                            clip_verts[vi].pos += d;
                        }
                    }
                }
            }
            if (clip_vert_count >= clip_verts_max)
                break;
        }
    }
    if (clip_fg_blend) {
        clip_added_transp += fo - clip_next_face_t;
        clip_next_face_t = fo;
    } else {
        clip_added_faces += fo - clip_next_face;
        clip_next_face = fo;
    }
}

// The faces of a perspective volume, in the frustum. The edges are split where they span more depth than
// RsNonOrthoTessellation, and the polygon is triangulated from its nearest vertex.
// FUNCTION: LS3DF 0x10002530
// FUNCTION: LS3DF11 0x10002530
bool ClipFacesPersp(const I3D_triface *faces, int num_faces)
{
    int first_vert = clip_vert_count;
    I3D_triface *fo;
    if (clip_fg_blend)
        fo = clip_next_face_t;
    else
        fo = clip_next_face;
    int count = num_faces;
    for (; count--; faces++) {
        S_clip_vertex *v0 = &clip_verts[faces->vi[0]];
        S_clip_vertex *v1 = &clip_verts[faces->vi[1]];
        S_clip_vertex *v2 = &clip_verts[faces->vi[2]];
        if (v0->clip & v1->clip & v2->clip)
            continue;
        if (!clip_two_sided) {
            S_vector n;
            n.GetNormal(v0->pos, v1->pos, v2->pos);
            if (n.z > 0.0f) {
                if (IsMrgZeroLess(clip_rev_factor))
                    continue;
                float x = clip_ratio_x * n.x, y = clip_ratio_y * n.y;
                if (clip_inv_reverse * n.z * n.z > x * x + y * y + n.z * n.z)
                    continue;
            }
        }
        ushort clip = v0->clip | v1->clip | v2->clip;
        clip_poly_buf1[0] = faces->vi[0];
        clip_poly_buf1[3] = faces->vi[0];
        clip_poly_buf1[1] = faces->vi[1];
        clip_poly_buf1[2] = faces->vi[2];
        ushort *in = clip_poly_buf1, *out = clip_poly_buf2;
        bool swap = false;
        clip_poly_num = 3;
        float t;
        if (clip & (CLIP_NEAR | CLIP_FAR)) {
            while (clip_poly_num) {
                clip_poly_num--;
                S_clip_vertex *a = &clip_verts[in[0]];
                S_clip_vertex *b = &clip_verts[in[1]];
                if (a->clip & CLIP_NEAR) {
                    if (!(b->clip & CLIP_NEAR)) {
                        if (b->clip & CLIP_FAR) {
                            *out++ = clip_vert_count;
                            clip_vert_end->pos.z = 0.0f;
                            t = (0.0f - a->pos.z) / (b->pos.z - a->pos.z);
                            clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                            clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                            if (clip_vert_end->pos.x < 0.0f)
                                clip_vert_end->clip = CLIP_LEFT;
                            else if (clip_vert_end->pos.x > 0.0f)
                                clip_vert_end->clip = CLIP_RIGHT;
                            else
                                clip_vert_end->clip = 0;
                            if (clip_vert_end->pos.y < 0.0f)
                                clip_vert_end->clip |= CLIP_BOTTOM;
                            else if (clip_vert_end->pos.y > 0.0f)
                                clip_vert_end->clip |= CLIP_TOP;
                            clip_vert_end->index = -1;
                            clip_vert_end++;
                            clip_vert_count++;
                            goto far_vertex;
                        } else
                            goto near_vertex;
                    }
                } else if (a->clip & CLIP_FAR) {
                    if (b->clip & CLIP_NEAR) {
                        *out++ = clip_vert_count;
                        clip_vert_end->pos.z = 1.0f;
                        t = (1.0f - a->pos.z) / (b->pos.z - a->pos.z);
                        clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                        clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                        if (clip_vert_end->pos.x < -1.0f)
                            clip_vert_end->clip = CLIP_LEFT;
                        else if (clip_vert_end->pos.x > 1.0f)
                            clip_vert_end->clip = CLIP_RIGHT;
                        else
                            clip_vert_end->clip = 0;
                        if (clip_vert_end->pos.y < -1.0f)
                            clip_vert_end->clip |= CLIP_BOTTOM;
                        else if (clip_vert_end->pos.y > 1.0f)
                            clip_vert_end->clip |= CLIP_TOP;
                        clip_vert_end->index = -1;
                        clip_vert_end++;
                        clip_vert_count++;
                        goto near_vertex;
                    } else if (b->clip & CLIP_FAR) {
                        // both beyond the far plane: no vertex
                    } else
                        goto far_vertex;
                } else {
                    *out++ = in[0];
                    if (b->clip & CLIP_NEAR) {
                near_vertex:
                        *out++ = clip_vert_count;
                        clip_vert_end->pos.z = 0.0f;
                        t = (0.0f - a->pos.z) / (b->pos.z - a->pos.z);
                        clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                        clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                        if (clip_vert_end->pos.x < 0.0f)
                            clip_vert_end->clip = CLIP_LEFT;
                        else if (clip_vert_end->pos.x > 0.0f)
                            clip_vert_end->clip = CLIP_RIGHT;
                        else
                            clip_vert_end->clip = 0;
                        if (clip_vert_end->pos.y < 0.0f)
                            clip_vert_end->clip |= CLIP_BOTTOM;
                        else if (clip_vert_end->pos.y > 0.0f)
                            clip_vert_end->clip |= CLIP_TOP;
                        clip_vert_end->index = -1;
                        clip_vert_end++;
                        clip_vert_count++;
                    } else if (b->clip & CLIP_FAR) {
                far_vertex:
                        *out++ = clip_vert_count;
                        clip_vert_end->pos.z = 1.0f;
                        t = (1.0f - a->pos.z) / (b->pos.z - a->pos.z);
                        clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                        clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                        if (clip_vert_end->pos.x < -1.0f)
                            clip_vert_end->clip = CLIP_LEFT;
                        else if (clip_vert_end->pos.x > 1.0f)
                            clip_vert_end->clip = CLIP_RIGHT;
                        else
                            clip_vert_end->clip = 0;
                        if (clip_vert_end->pos.y < -1.0f)
                            clip_vert_end->clip |= CLIP_BOTTOM;
                        else if (clip_vert_end->pos.y > 1.0f)
                            clip_vert_end->clip |= CLIP_TOP;
                        clip_vert_end->index = -1;
                        clip_vert_end++;
                        clip_vert_count++;
                    }
                }
                in++;
            }
            // The polygon is in out: close it, and clip it from there
            if (swap) {
                *out = clip_poly_buf1[0];
                swap = false;
                clip_poly_num = out - clip_poly_buf1;
                in = clip_poly_buf1;
                out = clip_poly_buf2;
            } else {
                *out = clip_poly_buf2[0];
                swap = true;
                clip_poly_num = out - clip_poly_buf2;
                in = clip_poly_buf2;
                out = clip_poly_buf1;
            }
            if (clip_poly_num > 0) {
                ushort *p = in;
                clip = clip_verts[*p++].clip;
                for (int i = clip_poly_num - 1; i; i--)
                    clip |= clip_verts[*p++].clip;
            } else
                clip = 0;
        }
        if (clip & (CLIP_LEFT | CLIP_RIGHT)) {
            while (clip_poly_num) {
                clip_poly_num--;
                S_clip_vertex *a = &clip_verts[in[0]];
                S_clip_vertex *b = &clip_verts[in[1]];
                if (a->clip & CLIP_LEFT) {
                    if (!(b->clip & CLIP_LEFT)) {
                        if (b->clip & CLIP_RIGHT) {
                            *out++ = clip_vert_count;
                            t = (a->pos.x + a->pos.z) / (a->pos.x + a->pos.z - b->pos.x - b->pos.z);
                            clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                            clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                            clip_vert_end->pos.z = -clip_vert_end->pos.x;
                            if (fabs(clip_vert_end->pos.y) > clip_vert_end->pos.z) {
                                if (clip_vert_end->pos.y < 0.0f)
                                    clip_vert_end->clip = CLIP_BOTTOM;
                                else
                                    clip_vert_end->clip = CLIP_TOP;
                            } else
                                clip_vert_end->clip = 0;
                            clip_vert_end->index = -1;
                            clip_vert_end++;
                            clip_vert_count++;
                            goto right_vertex;
                        } else
                            goto left_vertex;
                    }
                } else if (a->clip & CLIP_RIGHT) {
                    if (b->clip & CLIP_LEFT) {
                        *out++ = clip_vert_count;
                        t = (a->pos.z - a->pos.x) / (b->pos.x - a->pos.x - b->pos.z + a->pos.z);
                        clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                        clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                        clip_vert_end->pos.z = clip_vert_end->pos.x;
                        if (fabs(clip_vert_end->pos.y) > clip_vert_end->pos.z) {
                            if (clip_vert_end->pos.y < 0.0f)
                                clip_vert_end->clip = CLIP_BOTTOM;
                            else
                                clip_vert_end->clip = CLIP_TOP;
                        } else
                            clip_vert_end->clip = 0;
                        clip_vert_end->index = -1;
                        clip_vert_end++;
                        clip_vert_count++;
                        goto left_vertex;
                    } else if (b->clip & CLIP_RIGHT) {
                        // both beyond the right plane: no vertex
                    } else
                        goto right_vertex;
                } else {
                    *out++ = in[0];
                    if (b->clip & CLIP_LEFT) {
                left_vertex:
                        *out++ = clip_vert_count;
                        t = (a->pos.x + a->pos.z) / (a->pos.x + a->pos.z - b->pos.x - b->pos.z);
                        clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                        clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                        clip_vert_end->pos.z = -clip_vert_end->pos.x;
                        if (fabs(clip_vert_end->pos.y) > clip_vert_end->pos.z) {
                            if (clip_vert_end->pos.y < 0.0f)
                                clip_vert_end->clip = CLIP_BOTTOM;
                            else
                                clip_vert_end->clip = CLIP_TOP;
                        } else
                            clip_vert_end->clip = 0;
                        clip_vert_end->index = -1;
                        clip_vert_end++;
                        clip_vert_count++;
                    } else if (b->clip & CLIP_RIGHT) {
                right_vertex:
                        *out++ = clip_vert_count;
                        t = (a->pos.z - a->pos.x) / (b->pos.x - a->pos.x - b->pos.z + a->pos.z);
                        clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                        clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                        clip_vert_end->pos.z = clip_vert_end->pos.x;
                        if (fabs(clip_vert_end->pos.y) > clip_vert_end->pos.z) {
                            if (clip_vert_end->pos.y < 0.0f)
                                clip_vert_end->clip = CLIP_BOTTOM;
                            else
                                clip_vert_end->clip = CLIP_TOP;
                        } else
                            clip_vert_end->clip = 0;
                        clip_vert_end->index = -1;
                        clip_vert_end++;
                        clip_vert_count++;
                    }
                }
                in++;
            }
            // The polygon is in out: close it, and clip it from there
            if (swap) {
                *out = clip_poly_buf1[0];
                swap = false;
                clip_poly_num = out - clip_poly_buf1;
                in = clip_poly_buf1;
                out = clip_poly_buf2;
            } else {
                *out = clip_poly_buf2[0];
                swap = true;
                clip_poly_num = out - clip_poly_buf2;
                in = clip_poly_buf2;
                out = clip_poly_buf1;
            }
            if (clip_poly_num > 0) {
                ushort *p = in;
                clip = clip_verts[*p++].clip;
                for (int i = clip_poly_num - 1; i; i--)
                    clip |= clip_verts[*p++].clip;
            } else
                clip = 0;
        }
        if (clip & (CLIP_BOTTOM | CLIP_TOP)) {
            while (clip_poly_num) {
                clip_poly_num--;
                S_clip_vertex *a = &clip_verts[in[0]];
                S_clip_vertex *b = &clip_verts[in[1]];
                if (a->clip & CLIP_BOTTOM) {
                    if (!(b->clip & CLIP_BOTTOM)) {
                        if (b->clip & CLIP_TOP) {
                            *out++ = clip_vert_count;
                            t = (a->pos.y + a->pos.z) / (a->pos.y + a->pos.z - b->pos.y - b->pos.z);
                            clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                            clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                            clip_vert_end->pos.z = -clip_vert_end->pos.y;
                            clip_vert_end->index = -1;
                            clip_vert_end++;
                            clip_vert_count++;
                            goto top_vertex;
                        } else
                            goto bottom_vertex;
                    }
                } else if (a->clip & CLIP_TOP) {
                    if (b->clip & CLIP_BOTTOM) {
                        *out++ = clip_vert_count;
                        t = (a->pos.z - a->pos.y) / (b->pos.y - a->pos.y - b->pos.z + a->pos.z);
                        clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                        clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                        clip_vert_end->pos.z = clip_vert_end->pos.y;
                        clip_vert_end->index = -1;
                        clip_vert_end++;
                        clip_vert_count++;
                        goto bottom_vertex;
                    } else if (b->clip & CLIP_TOP) {
                        // both beyond the top plane: no vertex
                    } else
                        goto top_vertex;
                } else {
                    *out++ = in[0];
                    if (b->clip & CLIP_BOTTOM) {
                bottom_vertex:
                        *out++ = clip_vert_count;
                        t = (a->pos.y + a->pos.z) / (a->pos.y + a->pos.z - b->pos.y - b->pos.z);
                        clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                        clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                        clip_vert_end->pos.z = -clip_vert_end->pos.y;
                        clip_vert_end->index = -1;
                        clip_vert_end++;
                        clip_vert_count++;
                    } else if (b->clip & CLIP_TOP) {
                top_vertex:
                        *out++ = clip_vert_count;
                        t = (a->pos.z - a->pos.y) / (b->pos.y - a->pos.y - b->pos.z + a->pos.z);
                        clip_vert_end->pos.x = (b->pos.x - a->pos.x) * t + a->pos.x;
                        clip_vert_end->pos.y = (b->pos.y - a->pos.y) * t + a->pos.y;
                        clip_vert_end->pos.z = clip_vert_end->pos.y;
                        clip_vert_end->index = -1;
                        clip_vert_end++;
                        clip_vert_count++;
                    }
                }
                in++;
            }
            // The polygon is in out: close it, and clip it from there
            if (swap) {
                *out = clip_poly_buf1[0];
                swap = false;
                clip_poly_num = out - clip_poly_buf1;
                in = clip_poly_buf1;
                out = clip_poly_buf2;
            } else {
                *out = clip_poly_buf2[0];
                swap = true;
                clip_poly_num = out - clip_poly_buf2;
                in = clip_poly_buf2;
                out = clip_poly_buf1;
            }
            if (clip_poly_num > 0) {
                ushort *p = in;
                clip = clip_verts[*p++].clip;
                for (int i = clip_poly_num - 1; i; i--)
                    clip |= clip_verts[*p++].clip;
            } else
                clip = 0;
        }
        if (clip_poly_num > 2) {
            // The edges deeper than RsNonOrthoTessellation, split
            float min_z = 10.0f;
            int num = 0;
            int min_i;
            while (clip_poly_num) {
                clip_poly_num--;
                S_clip_vertex *a = &clip_verts[in[0]];
                if (a->pos.z < min_z) {
                    min_z = a->pos.z;
                    min_i = num;
                }
                *out++ = in[0];
                num++;
                S_clip_vertex *b = &clip_verts[in[1]];
                float dz = (float)fabs(a->pos.z - b->pos.z);
                if (dz > RsNonOrthoTessellation) {
                    int k = (int)(dz / RsNonOrthoTessellation);
                    num += k;
                    S_vector step = (b->pos - a->pos) / dz * RsNonOrthoTessellation;
                    S_vector p;
                    if (a->pos.z < b->pos.z)
                        p = a->pos;
                    else
                        p = b->pos - step * (float)k;
                    for (int j = k; j; j--) {
                        p += step;
                        *out++ = clip_vert_count;
                        clip_vert_end->pos = p;
                        clip_vert_end->index = -1;
                        clip_vert_end++;
                        clip_vert_count++;
                    }
                }
                in++;
            }
            clip_poly_num = num;
            if (swap) {
                in = clip_poly_buf1;
                out = clip_poly_buf2;
            } else {
                in = clip_poly_buf2;
                out = clip_poly_buf1;
            }
            // Triangulated from the nearest vertex, its neighbours first, then always the nearer of the next two
            if (clip_poly_num == 3) {
                fo->vi[0] = in[0];
                fo->vi[1] = in[1];
                fo->vi[2] = in[2];
                fo++;
            } else {
                int first = min_i + 1;
                if (first < clip_poly_num) {
                    memcpy(out, &in[first], (clip_poly_num - first) * sizeof(ushort));
                    if (min_i)
                        memcpy(&out[clip_poly_num - first], in, min_i * sizeof(ushort));
                } else
                    memcpy(out, in, min_i * sizeof(ushort));
                int hi = clip_poly_num - 2, lo = 0;
                int i = 1, j = clip_poly_num - 3;
                fo->vi[0] = out[hi];
                fo->vi[1] = in[min_i];
                fo->vi[2] = out[0];
                fo++;
                while (j > i) {
                    if (clip_verts[out[j]].pos.z < clip_verts[out[i]].pos.z) {
                        fo->vi[0] = out[hi];
                        fo->vi[1] = out[lo];
                        fo->vi[2] = out[j];
                        fo++;
                        hi = j;
                        j--;
                    } else {
                        fo->vi[0] = out[hi];
                        fo->vi[1] = out[lo];
                        fo->vi[2] = out[i];
                        fo++;
                        lo = i;
                        i++;
                    }
                }
                fo->vi[0] = out[hi];
                fo->vi[1] = out[lo];
                fo->vi[2] = out[j];
                fo++;
            }
            // The new vertices move along the face's normal, and away from the polygon's center
            S_vector n;
            n.GetNormal(v0->pos, v1->pos, v2->pos);
            S_vector offs;
            float sq = n.x * n.x + (n.y * n.y + n.z * n.z);
            if (sq < MRG_ZERO) {
                if (n.x != 0.0f)
                    n.x = n.x < 0.0f ? -1.0f : 1.0f;
                else if (n.z != 0.0f)
                    n.z = n.z < 0.0f ? -1.0f : 1.0f;
                else
                    n.y = n.y < 0.0f ? -1.0f : 1.0f;
                offs = n * clip_lift;
            } else
                offs = n * (clip_lift / (float)sqrt(sq));
            S_vector c = clip_verts[in[0]].pos;
            int i;
            for (i = 1; i < clip_poly_num; i++)
                c += clip_verts[in[i]].pos;
            c *= 1.0f / clip_poly_num;
            for (i = 0; i < clip_poly_num; i++) {
                int vi = in[i];
                if (vi >= first_vert) {
                    clip_verts[vi].pos += offs;
                    if (clip_spread > 0.0f) {
                        S_vector d = clip_verts[vi].pos - c;
                        float sq = d.x * d.x + d.y * d.y + d.z * d.z;
                        if (sq < MRG_ZERO) {
                            if (d.x != 0.0f)
                                d.x = d.x < 0.0f ? -1.0f : 1.0f;
                            // n.z, as in the original, so d.z is left as it is
                            else if (d.z != 0.0f)
                                n.z = d.z < 0.0f ? -1.0f : 1.0f;
                            else
                                d.y = d.y < 0.0f ? -1.0f : 1.0f;
                            d *= clip_spread;
                        } else
                            d *= clip_spread / (float)sqrt(sq);
                        clip_verts[vi].pos += d;
                    }
                }
            }
        }
        if (clip_vert_count >= clip_verts_max)
            break;
    }
    if (clip_fg_blend) {
        clip_added_transp += fo - clip_next_face_t;
        clip_next_face_t = fo;
    } else {
        clip_added_faces += fo - clip_next_face;
        clip_next_face = fo;
    }
    return true;
}

