// The collisions' sphere tests (PS2 g_c_sph.cpp, the fourth of PS2's collision files; the PC's file is the one at
// 0x5cafb0-0x5d58fc, docs/game-files.md), written whole, PS2's functions in PS2's order: the moving objects' test, an
// edge's, a face's, an axis-aligned box's, the volumes that aren't faces, a cell's volumes, the sphere moved through the
// grid's cells (_TestSphereB), then the sliding test (PC only), the racing's curves and the vehicles' tests. Every test
// fills a tCSphereParam and keeps the nearest hit in it. PS2's SphereInAACapsule tests a volume type the PC's level
// doesn't have (as LineInAACapsule), so it has no PC counterpart.
// No precompiled header (#pragma hdrstop before the includes): the vector helpers' out-of-line copies then follow
// their first caller, SphereInDynamics, so they are compiled before the other tests, which VC6 needs to give their
// temporaries the original's slots (SphereInEdge's frame is 0x4c only so) and to propagate SphereInAABox's face
// normals. Open: SphereInFace's, SphereInAABox's and SphereInNonface's frames and an inline decision each in the last
// two (see their comments).

#pragma hdrstop

#include "collision/g_c_glob.h"

#include <float.h>

#include "actors/C_scatter.h"

// SphereInNonface's oriented box, a rectangle at y = 0 in its own space (the names are ours): its edges, a bit each,
// the one at the lower z (its corners MIN_MIN to MAX_MIN), the higher x, the higher z and the lower x, which the move
// may leave the rectangle through, and its corners, which SphereInEdge reports when the nearest point of an edge lies
// past one of its ends (x first, then z)
#define OBB_EDGE_MIN_Z 1
#define OBB_EDGE_MAX_X 2
#define OBB_EDGE_MAX_Z 4
#define OBB_EDGE_MIN_X 8
#define OBB_EDGES (OBB_EDGE_MIN_Z | OBB_EDGE_MAX_X | OBB_EDGE_MAX_Z | OBB_EDGE_MIN_X)
#define OBB_CORNER_MIN_MIN 1
#define OBB_CORNER_MAX_MIN 2
#define OBB_CORNER_MAX_MAX 4
#define OBB_CORNER_MIN_MAX 8

// Where a line meets the plane: VE_BreakageGlass.cpp's (Insanity3D's Intersection with its IntersectionPosition
// written in), which _TestSphereSlide inlines for the plane each pass hit, the line from where the sphere would end up
// along the plane's normal
inline bool S_plane::Intersection(const S_vector &pos, const S_vector &dir, S_vector &ret) const
{
    float f = normal.Dot(dir);
    if (fabs(f) < MRG_ZERO)
        return false;
    f = -(normal.Dot(pos) + d) / f;
    ret = pos + dir * f;
    return true;
}

// GLOBAL: GAME 0x648240
float lastSphereDistance;

// The moving objects in the grid's cells under the move's bounds and a cell around them, but the next test's owners
// and those with one of the move's flags, whose bounding sphere the move passes within both radii and overlaps in x and
// z: a scattered object (0x47) goes to its owner's LineHit while push_scatter is set, the spheres (0x42, 0x44) by where
// the sphere touches them along dir_n (0x42 also from inside, pushed out horizontally), a box (0x41) by SphereInAABox,
// an oriented box (0x40, and 0x46 with the radius scaled) the same in its own space (the move's ends moved into it by
// the inverse matrix, the hit and the normal turned back by the matrix), an upright cylinder (0x43) around the sphere's
// position by the move's horizontal part. True where one was hit nearer than p's hit, which then goes into p.
// FUNCTION: GAME 0x5cafb0
bool g_collision::SphereInDynamics(tCSphereParam &p)
{
    bool hit = false;
    if (!dyn_grid)
        return false;
    int x0 = (int)((p.min_x - dyn_grid->min_x) / dyn_grid->cell_size_x - 1.0f);
    int x1 = (int)((p.max_x - dyn_grid->min_x) / dyn_grid->cell_size_x + 2.0f);
    int z0 = (int)((p.min_z - dyn_grid->min_z) / dyn_grid->cell_size_z - 1.0f);
    int z1 = (int)((p.max_z - dyn_grid->min_z) / dyn_grid->cell_size_z + 2.0f);
    if (x0 < 0)
        x0 = 0;
    if (z0 < 0)
        z0 = 0;
    if ((uint)x1 > dyn_grid->width)
        x1 = dyn_grid->width;
    if ((uint)z1 > dyn_grid->height)
        z1 = dyn_grid->height;
    for (int z = z0; z < z1; z++)
    {
        for (int x = x0; x < x1; x++)
        {
            tDynamicPolicko *cell = dyn_grid->cells[x + z * dyn_grid->width];
            if (!cell)
                continue;
            for (tDynamicCollObject **it = cell->objects.begin(); it < cell->objects.end(); it++)
            {
                bool owner = false;
                for (uint i = num_next_coll_owners; i--;)
                {
                    if (*it == next_coll_owners[i])
                    {
                        owner = true;
                        break;
                    }
                }
                if (owner)
                    continue;
                if ((*it)->flags & p.flags)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.DistanceToLine(p.from, p.dir) >
                    p.radius + ((I3D_bsphere &)(*it)->bsphere).radius)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.x + ((I3D_bsphere &)(*it)->bsphere).radius < p.min_x)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.x - ((I3D_bsphere &)(*it)->bsphere).radius > p.max_x)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.z + ((I3D_bsphere &)(*it)->bsphere).radius < p.min_z)
                    continue;
                if (((I3D_bsphere &)(*it)->bsphere).pos.z - ((I3D_bsphere &)(*it)->bsphere).radius > p.max_z)
                    continue;
                switch ((*it)->type & 0xff)
                {
                case DYNCOLL_UNKNOWN_47:
                    if (push_scatter && (*it)->owner->type == ACTOR_SCATTER)
                        ((C_scatter *)(*it)->owner)->LineHit(*it, (tCLineParam &)p);
                    break;
                case DYNCOLL_UNKNOWN_42:
                {
                    S_vector d = ((I3D_bsphere &)(*it)->bsphere).pos - p.from;
                    float sq = d.Square();
                    float r = p.radius + ((I3D_bsphere &)(*it)->bsphere).radius;
                    float b = d.Dot(p.dir_n);
                    if (b <= 0.0f)
                        break;
                    float r2 = r * r;
                    // Started inside: the hit is at the move's start, on the sphere's side toward from horizontally,
                    // the normal that horizontal direction, raised a little
                    if (sq < r2)
                    {
                        d.y = 0.0f;
                        float m = d.Magnitude();
                        if (!(m < MRG_ZERO))
                        {
                            p.pos = ((I3D_bsphere &)(*it)->bsphere).pos - d * (r / m);
                            p.pos.y = p.from.y;
                            p.normal = d;
                            if (p.normal.y < 0.01f)
                                p.normal.y = 0.01f;
                            p.dist_sq = 0.0f;
                            p.hit = (g_collision_header *)*it;
                            hit = true;
                            break;
                        }
                    }
                    float disc = r2 - (sq - b * b);
                    if (disc < 0.0f)
                        break;
                    float t = b - sqrt(disc);
                    if (t < -p.radius)
                        break;
                    float t2 = t * t;
                    if (t2 >= p.dist_sq)
                        break;
                    p.pos = p.from + p.dir_n * t;
                    p.normal = ((I3D_bsphere &)(*it)->bsphere).pos - p.pos;
                    if (p.normal.y < 0.0f)
                        p.normal.y = 0.0f;
                    p.dist_sq = t2;
                    hit = true;
                    p.hit = (g_collision_header *)*it;
                    break;
                }
                case DYNCOLL_UNKNOWN_44:
                {
                    S_vector d = ((I3D_bsphere &)(*it)->bsphere).pos - p.from;
                    float b = d.Dot(p.dir_n);
                    if (b <= 0.0f)
                        break;
                    float r = p.radius + ((I3D_bsphere &)(*it)->bsphere).radius;
                    float disc = r * r - (d.Square() - b * b);
                    if (disc < 0.0f)
                        break;
                    float t = b - sqrt(disc);
                    if (t < -p.radius)
                        break;
                    float t2 = t * t;
                    if (t2 >= p.dist_sq)
                        break;
                    p.pos = p.from + p.dir_n * t;
                    p.normal = ((I3D_bsphere &)(*it)->bsphere).pos - p.pos;
                    p.dist_sq = t2;
                    hit = true;
                    p.hit = (g_collision_header *)*it;
                    break;
                }
                case DYNCOLL_UNKNOWN_41:
                    if (SphereInAABox(((I3D_bbox &)(*it)->bbox).min, ((I3D_bbox &)(*it)->bbox).max, p))
                    {
                        hit = true;
                        p.hit = (g_collision_header *)*it;
                    }
                    break;
                case DYNCOLL_UNKNOWN_40:
                {
                    tCSphereParam sp = p;
                    sp.from *= *(*it)->inv_matrix;
                    sp.to *= *(*it)->inv_matrix;
                    sp.dir = sp.to - sp.from;
                    sp.dir_n = sp.dir;
                    sp.dir_n.Normalize();
                    if (SphereInAABox(((I3D_bbox &)(*it)->bbox).min, ((I3D_bbox &)(*it)->bbox).max, sp))
                    {
                        p.pos = sp.pos;
                        p.pos *= *(*it)->matrix;
                        p.dist_sq = sp.dist_sq;
                        p.normal = sp.normal.RotateByMatrix(*(*it)->matrix);
                        hit = true;
                        p.hit = (g_collision_header *)*it;
                    }
                    break;
                }
                case DYNCOLL_UNKNOWN_46:
                {
                    tCSphereParam sp = p;
                    sp.from *= *(*it)->inv_matrix;
                    sp.to *= *(*it)->inv_matrix;
                    sp.radius *= (*it)->inv_matrix->GetUScale();
                    sp.dir = sp.to - sp.from;
                    sp.dir_n = sp.dir;
                    sp.dir_n.Normalize();
                    if (SphereInAABox(((I3D_bbox &)(*it)->bbox).min, ((I3D_bbox &)(*it)->bbox).max, sp))
                    {
                        p.pos = sp.pos;
                        p.pos *= *(*it)->matrix;
                        p.dist_sq = sp.dist_sq;
                        p.normal = sp.normal.RotateByNormMatrix(*(*it)->matrix);
                        hit = true;
                        p.hit = (g_collision_header *)*it;
                    }
                    break;
                }
                case DYNCOLL_UNKNOWN_43:
                {
                    // In x and z only, along the move's horizontal direction n, t then scaled back to the whole move
                    // by f. Its failures return false from the whole test, the hits found before lost
                    // (SphereInVehicle's cylinder the same)
                    S_vector c = ((I3D_bsphere &)(*it)->bsphere).pos;
                    c -= p.from;
                    c.y = 0.0f;
                    S_vector n = p.dir_n;
                    n.y = 0.0f;
                    float m = n.Magnitude();
                    if (fabs(m) < MRG_ZERO)
                        return false;
                    float f = 1.0f / m;
                    n *= f;
                    float b = c.Dot(n);
                    if (b <= 0.0f)
                        break;
                    float r = (*it)->cylinder_radius + p.radius;
                    float disc = r * r - (c.Square() - b * b);
                    if (disc <= 0.0f)
                        return false;
                    // Two statements: as one expression VC6 keeps disc on the FPU stack, where the original keeps f
                    float t = b - sqrt(disc);
                    t *= f;
                    if (t < -p.radius)
                        return false;
                    float t2 = t * t;
                    if (t2 >= p.dist_sq)
                        return false;
                    p.pos = p.from + p.dir_n * t;
                    p.normal = ((I3D_bsphere &)(*it)->bsphere).pos - p.pos;
                    p.normal.y = 0.0f;
                    p.dist_sq = t2;
                    hit = true;
                    p.hit = (g_collision_header *)*it;
                    break;
                }
                }
            }
        }
    }
    return hit;
}

// The sphere moving along p.dir_n against the edge v0-v1 (PS2's): t, the distance along the move at which the center
// comes within the radius of the edge's line (from the edge's cross products with the move and with v0 - from), and s,
// the place on the edge of the point it touches (0 at v0, 1 at v1). A point before v0 or past v1 puts that end's code
// (vertex0, vertex1) into vertex and fails, so that the caller tests the end as a point (SphereInFace). A hit ahead of
// from nearer than p's puts t's square, the center's place and the normal (from it to the point on the edge) into p;
// one the sphere already overlaps at from (t below 0, the edge within its reach and the move heading into it) the same
// with -1.0 for the square. p.hit is the caller's. The two inner cross products are temporaries (PS2's frame has them
// among its temporaries, its named vectors first).
// FUNCTION: GAME 0x5cbed0
bool g_collision::SphereInEdge(const S_vector &v0, const S_vector &v1, tCSphereParam &p, uint &vertex, uint vertex0,
    uint vertex1)
{
    S_vector from = p.from;
    S_vector dir = p.dir_n;
    float radius = p.radius;
    S_vector a = v0 - from;
    S_vector edge = v1 - v0;
    S_vector n = edge.Cross(dir);
    float n_sq = n.Dot(n);
    if (fabs(n_sq) > MRG_ZERO) {
        float dot1 = dir.Cross(a).Dot(edge);
        float edge_sq = edge.Dot(edge);
        float disc = (n_sq * radius * radius - dot1 * dot1) * edge_sq;
        if (disc >= 0.0f) {
            float dot2 = edge.Cross(a).Dot(n);
            float sq = (float)sqrt(disc);
            if (dot2 < -sq)
                return false;
            float t = (dot2 - sq) / n_sq;
            if (t < -radius)
                return false;
            float ea = edge.Dot(a);
            float ed = edge.Dot(dir);
            float s = (t * ed - ea) / edge_sq;
            if (s < 0.0f) {
                vertex = vertex0;
                return false;
            }
            if (s > 1.0f) {
                vertex = vertex1;
                return false;
            }
            if (t < 0.0f) {
                if (ea > 0.0f) {
                    vertex = vertex0;
                    return false;
                }
                if (edge.Dot(v1 - from) < 0.0f) {
                    vertex = vertex1;
                    return false;
                }
                S_vector pe = edge * s + v0;
                S_vector pos = p.from + p.dir_n * t;
                S_vector normal = pe - pos;
                if (t < -normal.Dot(dir))
                    return false;
                p.normal = normal;
                p.dist_sq = -1.0f;
                p.pos = pos;
                return true;
            }
            float t_sq = t * t;
            if (t_sq >= p.dist_sq)
                return false;
            S_vector pe = edge * s + v0;
            p.dist_sq = t_sq;
            p.pos = p.from + p.dir_n * t;
            p.normal = pe - p.pos;
            return true;
        }
    }
    return false;
}

// The sphere moving along p.dir against a face (PS2's, which builds the face's plane from its vertices, the PC's level
// has it stored): none where the move doesn't head into the face (dir_n against its normal, which points into the
// solid) or the center is in front of its plane. A sphere already touching the plane at from (the center within the
// radius behind it) whose center lies inside the face's edges (the three edge tests at or below zero, or for a
// VOLF_REVERSED face at or above it) is a hit at distance 0, its place from pushed back until the sphere only touches.
// Otherwise t, where the center reaches the plane moved back by the radius: inside the edges (each edge's side of that
// line, a FACE_EDGE_* bit of side each, which starts with all of them for a reversed face) a hit ahead of from nearer
// than p's; outside them the edges it lies beyond go to SphereInEdge, and an end one of them names (vertex, by the bit
// of the edge opposite it) is tested as a point the sphere hits (where the line along dir_n passes within the radius).
// A hit puts the distance's square, the center's place and the normal into p and the face into p.hit. unknown is never
// read or written (PS2 neither; SphereInTreeLevel passes a local). fabsf (math.h's inline one) for t's test: it is the
// inline call the original's budget has there (with fabs the touch hit's position and the second vertex case inline
// more). Open: the frame (0x9c for the original's 0x84: the moving case's c and q keep areas of their own, s1, s2 and
// this sit elsewhere) and the operand order of several products (the first plane distance, the crosses).
// FUNCTION: GAME 0x5cc470
bool g_collision::SphereInFace(g_volume_triface *vol, tCSphereParam &p, int &unknown)
{
    uint side = 0;
    if (vol->flags & VOLF_REVERSED)
        side = FACE_EDGES;
    if (vol->normal.Dot(p.dir_n) < 0.0f)
        return false;
    S_vector rn = vol->normal * p.radius;
    S_vector q = *vol->vertices[0] - rn;
    float d = -q.Dot(vol->normal);
    float dist = vol->normal.Dot(p.from) + vol->d;
    if (dist > 0.0f)
        return false;
    if (dist > -p.radius) {
        S_vector c = vol->normal.Cross(p.from);
        float w0 = vol->normal.Dot(vol->vertices[0]->Cross(*vol->vertices[1]));
        float e0 = (*vol->vertices[0] - *vol->vertices[1]).Dot(c);
        float s0 = w0 + e0;
        float w1 = vol->normal.Dot(vol->vertices[1]->Cross(*vol->vertices[2]));
        float e1 = (*vol->vertices[1] - *vol->vertices[2]).Dot(c);
        float s1 = w1 + e1;
        float w2 = vol->normal.Dot(vol->vertices[2]->Cross(*vol->vertices[0]));
        float e2 = (*vol->vertices[2] - *vol->vertices[0]).Dot(c);
        float s2 = w2 + e2;
        if (side == 0 && s0 <= 0.0f && s1 <= 0.0f && s2 <= 0.0f ||
            side == FACE_EDGES && s0 >= 0.0f && s1 >= 0.0f && s2 >= 0.0f) {
            p.dist_sq = 0.0f;
            p.pos = p.from + vol->normal * (-dist - p.radius);
            p.normal = vol->normal;
            p.hit = (g_collision_header *)vol;
            return true;
        }
    }
    S_vector c = p.dir.Cross(p.from + vol->normal * p.radius);
    if (vol->vertices[0]->Cross(*vol->vertices[1]).Dot(p.dir) + (*vol->vertices[0] - *vol->vertices[1]).Dot(c) > 0.0f)
        side ^= FACE_EDGE_01;
    if (vol->vertices[1]->Cross(*vol->vertices[2]).Dot(p.dir) + (*vol->vertices[1] - *vol->vertices[2]).Dot(c) > 0.0f)
        side ^= FACE_EDGE_12;
    if (vol->vertices[2]->Cross(*vol->vertices[0]).Dot(p.dir) + (*vol->vertices[2] - *vol->vertices[0]).Dot(c) > 0.0f)
        side ^= FACE_EDGE_20;
    float t = vol->normal.Dot(p.dir);
    if (fabsf(t) < MRG_ZERO)
        return false;
    t = -(vol->normal.Dot(p.from) + d) / t;
    S_vector hp = p.from + p.dir * t;
    if (!side) {
        if (t < 0.0f)
            return false;
        S_vector v;
        v = hp - p.from;
        float dist_sq = v.x * v.x + v.y * v.y + v.z * v.z;
        if (dist_sq >= p.dist_sq)
            return false;
        p.dist_sq = dist_sq;
        p.pos = hp;
        p.normal = vol->normal;
        p.hit = (g_collision_header *)vol;
        return true;
    }
    uint vertex = 0;
    if (side & FACE_EDGE_20 &&
        SphereInEdge(*vol->vertices[2], *vol->vertices[0], p, vertex, FACE_EDGE_01, FACE_EDGE_12)) {
        p.hit = (g_collision_header *)vol;
        return true;
    }
    if (side & FACE_EDGE_12 &&
        SphereInEdge(*vol->vertices[1], *vol->vertices[2], p, vertex, FACE_EDGE_20, FACE_EDGE_01)) {
        p.hit = (g_collision_header *)vol;
        return true;
    }
    if (side & FACE_EDGE_01 &&
        SphereInEdge(*vol->vertices[0], *vol->vertices[1], p, vertex, FACE_EDGE_12, FACE_EDGE_20)) {
        p.hit = (g_collision_header *)vol;
        return true;
    }
    switch (vertex)
    {
    case FACE_EDGE_01:
    {
        S_vector a = *vol->vertices[2] - p.from;
        float dot = a.Dot(p.dir_n);
        float disc = p.radius * p.radius - (a.Dot(a) - dot * dot);
        if (disc < 0.0f)
            return false;
        float t = dot - (float)sqrt(disc);
        if (t * t >= p.dist_sq)
            return false;
        p.pos = p.from + p.dir_n * t;
        p.normal = *vol->vertices[2] - p.pos;
        p.dist_sq = t * t;
        p.hit = (g_collision_header *)vol;
        return true;
    }
    case FACE_EDGE_12:
    {
        S_vector a = *vol->vertices[0] - p.from;
        float dot = a.Dot(p.dir_n);
        float disc = p.radius * p.radius - (a.Dot(a) - dot * dot);
        if (disc < 0.0f)
            return false;
        float t = dot - (float)sqrt(disc);
        if (t * t >= p.dist_sq)
            return false;
        p.pos = p.from + p.dir_n * t;
        p.normal = *vol->vertices[0] - p.pos;
        p.dist_sq = t * t;
        p.hit = (g_collision_header *)vol;
        return true;
    }
    case FACE_EDGE_20:
    {
        S_vector a = *vol->vertices[1] - p.from;
        float dot = a.Dot(p.dir_n);
        float disc = p.radius * p.radius - (a.Dot(a) - dot * dot);
        if (disc < 0.0f)
            return false;
        float t = dot - (float)sqrt(disc);
        if (t * t >= p.dist_sq)
            return false;
        p.pos = p.from + p.dir_n * t;
        p.normal = *vol->vertices[1] - p.pos;
        p.dist_sq = t * t;
        p.hit = (g_collision_header *)vol;
        return true;
    }
    }
    return false;
}

// The sphere moved along dir against an axis-aligned box (min, max): nothing where the move's bounds, widened by the
// radius, miss the box. Then the faces the sphere moves into, one per axis (x, y, z; min's moving up the axis, max's
// down), each with its axis as the normal (the way the sphere moves, as LineInAABox's): a sphere already in the face's
// plane within its edges is pushed back out of it (dist_sq 0); otherwise the leading point of the sphere is moved to
// the face's plane, and a hit inside the face's edges nearer than p's goes into p. An edge the move passes outside of
// is marked in a mask (four bits per face), and a later axis's face is tested only where an edge it shares was
// marked. Then each marked edge by SphereInEdge, and where an edge's nearest point lies past one of its ends, that
// corner as a point. True where a hit nearer than p's went into p (pos, normal and dist_sq; the caller sets hit). The
// min faces of x and y take t from the face's coordinate, the others through the normal (PS2's the same).
// FUNCTION: GAME 0x5cd040
bool g_collision::SphereInAABox(const S_vector &min, const S_vector &max, tCSphereParam &p)
{
    {
        S_vector to = p.from + p.dir;
        S_vector from = p.from;
        S_vector lo = min - S_vector(p.radius, p.radius, p.radius);
        S_vector hi = max + S_vector(p.radius, p.radius, p.radius);
        if (to.x > from.x) {
            if (to.x < lo.x || from.x > hi.x)
                return false;
        } else {
            if (from.x < lo.x || to.x > hi.x)
                return false;
        }
        if (to.z > from.z) {
            if (to.z < lo.z || from.z > hi.z)
                return false;
        } else {
            if (from.z < lo.z || to.z > hi.z)
                return false;
        }
        if (to.y > from.y) {
            if (to.y < lo.y || from.y > hi.y)
                return false;
        } else {
            if (from.y < lo.y || to.y > hi.y)
                return false;
        }
    }
    uint mask = 0;
    S_vector normal;
    S_vector pos, v;
    if (p.dir_n.x >= 0.0f) {
        normal = S_vector(1.0f, 0.0f, 0.0f);
        S_vector nf;
        float d = (p.from - min).Dot(normal);
        if (d <= 0.0f && d > -p.radius) {
            nf = normal.Cross(p.from);
            float n0 = normal.Dot(min.Cross(S_vector(min.x, min.y, max.z)));
            float s0 = n0 + ((min - S_vector(min.x, min.y, max.z)).Dot(nf));
            float n1 = normal.Dot(S_vector(min.x, min.y, max.z).Cross(S_vector(min.x, max.y, max.z)));
            float s1 = n1 + ((S_vector(min.x, min.y, max.z) - S_vector(min.x, max.y, max.z)).Dot(nf));
            float n2 = normal.Dot(S_vector(min.x, max.y, max.z).Cross(S_vector(min.x, max.y, min.z)));
            float s2 = n2 + ((S_vector(min.x, max.y, max.z) - S_vector(min.x, max.y, min.z)).Dot(nf));
            float n3 = normal.Dot(S_vector(min.x, max.y, min.z).Cross(min));
            float s3 = n3 + ((S_vector(min.x, max.y, min.z) - min).Dot(nf));
            if (s0 <= 0.0f && s1 <= 0.0f && s2 <= 0.0f && s3 <= 0.0f) {
                p.dist_sq = 0.0f;
                p.pos = p.from + normal * (-d - p.radius);
                p.normal = normal;
                return true;
            }
        }
        nf = p.dir.Cross(p.from + normal * p.radius);
        float k0 = p.dir.Dot(min.Cross(S_vector(min.x, min.y, max.z)));
        float m0 = (min - S_vector(min.x, min.y, max.z)).Dot(nf);
        if (k0 + m0 > 0.0f)
            mask |= 0x1;
        float k1 = p.dir.Dot(S_vector(min.x, min.y, max.z).Cross(S_vector(min.x, max.y, max.z)));
        float m1 = (S_vector(min.x, min.y, max.z) - S_vector(min.x, max.y, max.z)).Dot(nf);
        if (k1 + m1 > 0.0f)
            mask |= 0x2;
        float k2 = p.dir.Dot(S_vector(min.x, max.y, max.z).Cross(S_vector(min.x, max.y, min.z)));
        float m2 = (S_vector(min.x, max.y, max.z) - S_vector(min.x, max.y, min.z)).Dot(nf);
        if (k2 + m2 > 0.0f)
            mask |= 0x4;
        float k3 = p.dir.Dot(S_vector(min.x, max.y, min.z).Cross(min));
        float m3 = (S_vector(min.x, max.y, min.z) - min).Dot(nf);
        if (k3 + m3 > 0.0f)
            mask |= 0x8;
        if (!(mask & 0xf)) {
            if (p.dir.x >= MRG_ZERO) {
                float t = (p.from.x + (p.radius - min.x)) * (-1.0f / p.dir.x);
                if (t >= 0.0f) {
                    pos = p.from + p.dir * t;
                    v = pos - p.from;
                    float sq = v.Dot(v);
                    if (sq < p.dist_sq) {
                        p.dist_sq = sq;
                        p.pos = pos;
                        p.normal = normal;
                        return true;
                    }
                }
            }
        }
    } else {
        normal = S_vector(-1.0f, 0.0f, 0.0f);
        S_vector nf;
        float d = (p.from - max).Dot(normal);
        if (d <= 0.0f && d > -p.radius) {
            nf = normal.Cross(p.from);
            float n0 = normal.Dot(S_vector(max.x, min.y, min.z).Cross(S_vector(max.x, max.y, min.z)));
            float s0 = n0 + ((S_vector(max.x, min.y, min.z) - S_vector(max.x, max.y, min.z)).Dot(nf));
            float n1 = normal.Dot(S_vector(max.x, max.y, min.z).Cross(S_vector(max.x, max.y, max.z)));
            float s1 = n1 + ((S_vector(max.x, max.y, min.z) - S_vector(max.x, max.y, max.z)).Dot(nf));
            float n2 = normal.Dot(S_vector(max.x, max.y, max.z).Cross(S_vector(max.x, min.y, max.z)));
            float s2 = n2 + ((S_vector(max.x, max.y, max.z) - S_vector(max.x, min.y, max.z)).Dot(nf));
            float n3 = normal.Dot(S_vector(max.x, min.y, max.z).Cross(S_vector(max.x, min.y, min.z)));
            float s3 = n3 + ((S_vector(max.x, min.y, max.z) - S_vector(max.x, min.y, min.z)).Dot(nf));
            if (s0 <= 0.0f && s1 <= 0.0f && s2 <= 0.0f && s3 <= 0.0f) {
                p.dist_sq = 0.0f;
                p.pos = p.from + normal * (-d - p.radius);
                p.normal = normal;
                return true;
            }
        }
        nf = p.dir.Cross(p.from + normal * p.radius);
        float k0 = p.dir.Dot(S_vector(max.x, min.y, min.z).Cross(S_vector(max.x, max.y, min.z)));
        float m0 = (S_vector(max.x, min.y, min.z) - S_vector(max.x, max.y, min.z)).Dot(nf);
        if (k0 + m0 > 0.0f)
            mask |= 0x10;
        float k1 = p.dir.Dot(S_vector(max.x, max.y, min.z).Cross(S_vector(max.x, max.y, max.z)));
        float m1 = (S_vector(max.x, max.y, min.z) - S_vector(max.x, max.y, max.z)).Dot(nf);
        if (k1 + m1 > 0.0f)
            mask |= 0x20;
        float k2 = p.dir.Dot(S_vector(max.x, max.y, max.z).Cross(S_vector(max.x, min.y, max.z)));
        float m2 = (S_vector(max.x, max.y, max.z) - S_vector(max.x, min.y, max.z)).Dot(nf);
        if (k2 + m2 > 0.0f)
            mask |= 0x40;
        float k3 = p.dir.Dot(S_vector(max.x, min.y, max.z).Cross(S_vector(max.x, min.y, min.z)));
        float m3 = (S_vector(max.x, min.y, max.z) - S_vector(max.x, min.y, min.z)).Dot(nf);
        if (k3 + m3 > 0.0f)
            mask |= 0x80;
        if (!(mask & 0xf0)) {
            float dn = normal.Dot(p.dir);
            if (fabs(dn) >= MRG_ZERO) {
                float pd = -normal.Dot(max - normal * p.radius);
                float t = (pd + normal.Dot(p.from)) * (-1.0f / dn);
                pos = p.from + p.dir * t;
                if (t >= 0.0f) {
                    v = pos - p.from;
                    float sq = v.Dot(v);
                    if (sq < p.dist_sq) {
                        p.dist_sq = sq;
                        p.pos = pos;
                        p.normal = normal;
                        return true;
                    }
                }
            }
        }
    }
    if (p.dir_n.y >= 0.0f) {
        if (mask & 0x81) {
            normal = S_vector(0.0f, 1.0f, 0.0f);
            S_vector nf;
            float d = (p.from - min).Dot(normal);
            if (d <= 0.0f && d > -p.radius) {
                nf = normal.Cross(p.from);
                float n0 = normal.Dot(S_vector(min.x, min.y, min.z).Cross(S_vector(max.x, min.y, min.z)));
                float s0 = n0 + ((S_vector(min.x, min.y, min.z) - S_vector(max.x, min.y, min.z)).Dot(nf));
                float n1 = normal.Dot(S_vector(max.x, min.y, min.z).Cross(S_vector(max.x, min.y, max.z)));
                float s1 = n1 + ((S_vector(max.x, min.y, min.z) - S_vector(max.x, min.y, max.z)).Dot(nf));
                float n2 = normal.Dot(S_vector(max.x, min.y, max.z).Cross(S_vector(min.x, min.y, max.z)));
                float s2 = n2 + ((S_vector(max.x, min.y, max.z) - S_vector(min.x, min.y, max.z)).Dot(nf));
                float n3 = normal.Dot(S_vector(min.x, min.y, max.z).Cross(S_vector(min.x, min.y, min.z)));
                float s3 = n3 + ((S_vector(min.x, min.y, max.z) - S_vector(min.x, min.y, min.z)).Dot(nf));
                if (s0 <= 0.0f && s1 <= 0.0f && s2 <= 0.0f && s3 <= 0.0f) {
                    p.dist_sq = 0.0f;
                    p.pos = p.from + normal * (-d - p.radius);
                    p.normal = normal;
                    return true;
                }
            }
            nf = p.dir.Cross(p.from + normal * p.radius);
            float k0 = p.dir.Dot(S_vector(min.x, min.y, min.z).Cross(S_vector(max.x, min.y, min.z)));
            float m0 = (S_vector(min.x, min.y, min.z) - S_vector(max.x, min.y, min.z)).Dot(nf);
            if (k0 + m0 > 0.0f)
                mask |= 0x1000;
            float k1 = p.dir.Dot(S_vector(max.x, min.y, min.z).Cross(S_vector(max.x, min.y, max.z)));
            float m1 = (S_vector(max.x, min.y, min.z) - S_vector(max.x, min.y, max.z)).Dot(nf);
            if (k1 + m1 > 0.0f)
                mask |= 0x2000;
            float k2 = p.dir.Dot(S_vector(max.x, min.y, max.z).Cross(S_vector(min.x, min.y, max.z)));
            float m2 = (S_vector(max.x, min.y, max.z) - S_vector(min.x, min.y, max.z)).Dot(nf);
            if (k2 + m2 > 0.0f)
                mask |= 0x4000;
            float k3 = p.dir.Dot(S_vector(min.x, min.y, max.z).Cross(S_vector(min.x, min.y, min.z)));
            float m3 = (S_vector(min.x, min.y, max.z) - S_vector(min.x, min.y, min.z)).Dot(nf);
            if (k3 + m3 > 0.0f)
                mask |= 0x8000;
            if (!(mask & 0xf000)) {
                if (p.dir.y >= MRG_ZERO) {
                    float t = (p.from.y + (p.radius - min.y)) * (-1.0f / p.dir.y);
                    if (t >= 0.0f) {
                        pos = p.from + p.dir * t;
                        v = pos - p.from;
                        float sq = v.Dot(v);
                        if (sq < p.dist_sq) {
                            p.dist_sq = sq;
                            p.pos = pos;
                            p.normal = normal;
                            return true;
                        }
                    }
                }
            }
        }
    } else {
        if (mask & 0x24) {
            normal = S_vector(0.0f, -1.0f, 0.0f);
            S_vector nf;
            float d = (p.from - max).Dot(normal);
            if (d <= 0.0f && d > -p.radius) {
                nf = normal.Cross(p.from);
                float n0 = normal.Dot(S_vector(min.x, max.y, min.z).Cross(S_vector(min.x, max.y, max.z)));
                float s0 = n0 + ((S_vector(min.x, max.y, min.z) - S_vector(min.x, max.y, max.z)).Dot(nf));
                float n1 = normal.Dot(S_vector(min.x, max.y, max.z).Cross(S_vector(max.x, max.y, max.z)));
                float s1 = n1 + ((S_vector(min.x, max.y, max.z) - S_vector(max.x, max.y, max.z)).Dot(nf));
                float n2 = normal.Dot(S_vector(max.x, max.y, max.z).Cross(S_vector(max.x, max.y, min.z)));
                float s2 = n2 + ((S_vector(max.x, max.y, max.z) - S_vector(max.x, max.y, min.z)).Dot(nf));
                float n3 = normal.Dot(S_vector(max.x, max.y, min.z).Cross(S_vector(min.x, max.y, min.z)));
                float s3 = n3 + ((S_vector(max.x, max.y, min.z) - S_vector(min.x, max.y, min.z)).Dot(nf));
                if (s0 <= 0.0f && s1 <= 0.0f && s2 <= 0.0f && s3 <= 0.0f) {
                    p.dist_sq = 0.0f;
                    p.pos = p.from + normal * (-d - p.radius);
                    p.normal = normal;
                    return true;
                }
            }
            nf = p.dir.Cross(p.from + normal * p.radius);
            float k0 = p.dir.Dot(S_vector(min.x, max.y, min.z).Cross(S_vector(min.x, max.y, max.z)));
            float m0 = (S_vector(min.x, max.y, min.z) - S_vector(min.x, max.y, max.z)).Dot(nf);
            if (k0 + m0 > 0.0f)
                mask |= 0x100;
            float k1 = p.dir.Dot(S_vector(min.x, max.y, max.z).Cross(S_vector(max.x, max.y, max.z)));
            float m1 = (S_vector(min.x, max.y, max.z) - S_vector(max.x, max.y, max.z)).Dot(nf);
            if (k1 + m1 > 0.0f)
                mask |= 0x200;
            float k2 = p.dir.Dot(S_vector(max.x, max.y, max.z).Cross(S_vector(max.x, max.y, min.z)));
            float m2 = (S_vector(max.x, max.y, max.z) - S_vector(max.x, max.y, min.z)).Dot(nf);
            if (k2 + m2 > 0.0f)
                mask |= 0x400;
            float k3 = p.dir.Dot(S_vector(max.x, max.y, min.z).Cross(S_vector(min.x, max.y, min.z)));
            float m3 = (S_vector(max.x, max.y, min.z) - S_vector(min.x, max.y, min.z)).Dot(nf);
            if (k3 + m3 > 0.0f)
                mask |= 0x800;
            if (!(mask & 0xf00)) {
                float dn = normal.Dot(p.dir);
                if (fabs(dn) >= MRG_ZERO) {
                    float pd = -normal.Dot(max - normal * p.radius);
                    float t = (pd + normal.Dot(p.from)) * (-1.0f / dn);
                    pos = p.from + p.dir * t;
                    if (t >= 0.0f) {
                        v = pos - p.from;
                        float sq = v.Dot(v);
                        if (sq < p.dist_sq) {
                            p.dist_sq = sq;
                            p.pos = pos;
                            p.normal = normal;
                            return true;
                        }
                    }
                }
            }
        }
    }
    if (p.dir_n.z >= 0.0f) {
        if (mask & 0x1818) {
            normal = S_vector(0.0f, 0.0f, 1.0f);
            S_vector nf;
            float d = (p.from - min).Dot(normal);
            if (d <= 0.0f && d > -p.radius) {
                nf = normal.Cross(p.from);
                float n0 = normal.Dot(S_vector(min.x, min.y, min.z).Cross(S_vector(min.x, max.y, min.z)));
                float s0 = n0 + ((S_vector(min.x, min.y, min.z) - S_vector(min.x, max.y, min.z)).Dot(nf));
                float n1 = normal.Dot(S_vector(min.x, max.y, min.z).Cross(S_vector(max.x, max.y, min.z)));
                float s1 = n1 + ((S_vector(min.x, max.y, min.z) - S_vector(max.x, max.y, min.z)).Dot(nf));
                float n2 = normal.Dot(S_vector(max.x, max.y, min.z).Cross(S_vector(max.x, min.y, min.z)));
                float s2 = n2 + ((S_vector(max.x, max.y, min.z) - S_vector(max.x, min.y, min.z)).Dot(nf));
                float n3 = normal.Dot(S_vector(max.x, min.y, min.z).Cross(S_vector(min.x, min.y, min.z)));
                float s3 = n3 + ((S_vector(max.x, min.y, min.z) - S_vector(min.x, min.y, min.z)).Dot(nf));
                if (s0 <= 0.0f && s1 <= 0.0f && s2 <= 0.0f && s3 <= 0.0f) {
                    p.dist_sq = 0.0f;
                    p.pos = p.from + normal * (-d - p.radius);
                    p.normal = normal;
                    return true;
                }
            }
            nf = p.dir.Cross(p.from + normal * p.radius);
            float k0 = p.dir.Dot(S_vector(min.x, min.y, min.z).Cross(S_vector(min.x, max.y, min.z)));
            float m0 = (S_vector(min.x, min.y, min.z) - S_vector(min.x, max.y, min.z)).Dot(nf);
            if (k0 + m0 > 0.0f)
                mask |= 0x10000;
            float k1 = p.dir.Dot(S_vector(min.x, max.y, min.z).Cross(S_vector(max.x, max.y, min.z)));
            float m1 = (S_vector(min.x, max.y, min.z) - S_vector(max.x, max.y, min.z)).Dot(nf);
            if (k1 + m1 > 0.0f)
                mask |= 0x20000;
            float k2 = p.dir.Dot(S_vector(max.x, max.y, min.z).Cross(S_vector(max.x, min.y, min.z)));
            float m2 = (S_vector(max.x, max.y, min.z) - S_vector(max.x, min.y, min.z)).Dot(nf);
            if (k2 + m2 > 0.0f)
                mask |= 0x40000;
            float k3 = p.dir.Dot(S_vector(max.x, min.y, min.z).Cross(S_vector(min.x, min.y, min.z)));
            float m3 = (S_vector(max.x, min.y, min.z) - S_vector(min.x, min.y, min.z)).Dot(nf);
            if (k3 + m3 > 0.0f)
                mask |= 0x80000;
            if (!(mask & 0xf0000)) {
                float dn = normal.Dot(p.dir);
                if (fabs(dn) >= MRG_ZERO) {
                    float pd = -normal.Dot(min - normal * p.radius);
                    float t = (pd + normal.Dot(p.from)) * (-1.0f / dn);
                    pos = p.from + p.dir * t;
                    if (t >= 0.0f) {
                        v = pos - p.from;
                        float sq = v.Dot(v);
                        if (sq < p.dist_sq) {
                            p.dist_sq = sq;
                            p.pos = pos;
                            p.normal = normal;
                            return true;
                        }
                    }
                }
            }
        }
    } else {
        if (mask & 0x4242) {
            normal = S_vector(0.0f, 0.0f, -1.0f);
            S_vector nf;
            float d = (p.from - max).Dot(normal);
            if (d <= 0.0f && d > -p.radius) {
                nf = normal.Cross(p.from);
                float n0 = normal.Dot(S_vector(min.x, min.y, max.z).Cross(S_vector(max.x, min.y, max.z)));
                float s0 = n0 + ((S_vector(min.x, min.y, max.z) - S_vector(max.x, min.y, max.z)).Dot(nf));
                float n1 = normal.Dot(S_vector(max.x, min.y, max.z).Cross(S_vector(max.x, max.y, max.z)));
                float s1 = n1 + ((S_vector(max.x, min.y, max.z) - S_vector(max.x, max.y, max.z)).Dot(nf));
                float n2 = normal.Dot(S_vector(max.x, max.y, max.z).Cross(S_vector(min.x, max.y, max.z)));
                float s2 = n2 + ((S_vector(max.x, max.y, max.z) - S_vector(min.x, max.y, max.z)).Dot(nf));
                float n3 = normal.Dot(S_vector(min.x, max.y, max.z).Cross(S_vector(min.x, min.y, max.z)));
                float s3 = n3 + ((S_vector(min.x, max.y, max.z) - S_vector(min.x, min.y, max.z)).Dot(nf));
                if (s0 <= 0.0f && s1 <= 0.0f && s2 <= 0.0f && s3 <= 0.0f) {
                    p.dist_sq = 0.0f;
                    p.pos = p.from + normal * (-d - p.radius);
                    p.normal = normal;
                    return true;
                }
            }
            nf = p.dir.Cross(p.from + normal * p.radius);
            float k0 = p.dir.Dot(S_vector(min.x, min.y, max.z).Cross(S_vector(max.x, min.y, max.z)));
            float m0 = (S_vector(min.x, min.y, max.z) - S_vector(max.x, min.y, max.z)).Dot(nf);
            if (k0 + m0 > 0.0f)
                mask |= 0x100000;
            float k1 = p.dir.Dot(S_vector(max.x, min.y, max.z).Cross(S_vector(max.x, max.y, max.z)));
            float m1 = (S_vector(max.x, min.y, max.z) - S_vector(max.x, max.y, max.z)).Dot(nf);
            if (k1 + m1 > 0.0f)
                mask |= 0x200000;
            float k2 = p.dir.Dot(S_vector(max.x, max.y, max.z).Cross(S_vector(min.x, max.y, max.z)));
            float m2 = (S_vector(max.x, max.y, max.z) - S_vector(min.x, max.y, max.z)).Dot(nf);
            if (k2 + m2 > 0.0f)
                mask |= 0x400000;
            float k3 = p.dir.Dot(S_vector(min.x, max.y, max.z).Cross(S_vector(min.x, min.y, max.z)));
            float m3 = (S_vector(min.x, max.y, max.z) - S_vector(min.x, min.y, max.z)).Dot(nf);
            if (k3 + m3 > 0.0f)
                mask |= 0x800000;
            if (!(mask & 0xf00000)) {
                float dn = normal.Dot(p.dir);
                if (fabs(dn) >= MRG_ZERO) {
                    float pd = -normal.Dot(max - normal * p.radius);
                    float t = (pd + normal.Dot(p.from)) * (-1.0f / dn);
                    pos = p.from + p.dir * t;
                    if (t >= 0.0f) {
                        v = pos - p.from;
                        float sq = v.Dot(v);
                        if (sq < p.dist_sq) {
                            p.dist_sq = sq;
                            p.pos = pos;
                            p.normal = normal;
                            return true;
                        }
                    }
                }
            }
        }
    }
    uint vertex = 0;
    if ((mask & 0x8001) && SphereInEdge(min, S_vector(min.x, min.y, max.z), p, vertex, 0x1, 0x2))
        return true;
    if ((mask & 0x10008) && SphereInEdge(min, S_vector(min.x, max.y, min.z), p, vertex, 0x1, 0x4))
        return true;
    if ((mask & 0x81000) && SphereInEdge(min, S_vector(max.x, min.y, min.z), p, vertex, 0x1, 0x8))
        return true;
    if ((mask & 0x40010) && SphereInEdge(S_vector(max.x, min.y, min.z), S_vector(max.x, max.y, min.z), p, vertex, 0x8, 0x10))
        return true;
    if ((mask & 0x2080) && SphereInEdge(S_vector(max.x, min.y, min.z), S_vector(max.x, min.y, max.z), p, vertex, 0x8, 0x20))
        return true;
    if ((mask & 0x104000) && SphereInEdge(S_vector(min.x, min.y, max.z), S_vector(max.x, min.y, max.z), p, vertex, 0x2, 0x20))
        return true;
    if ((mask & 0x800002) && SphereInEdge(S_vector(min.x, min.y, max.z), S_vector(min.x, max.y, max.z), p, vertex, 0x2, 0x40))
        return true;
    if ((mask & 0x200040) && SphereInEdge(S_vector(max.x, min.y, max.z), S_vector(max.x, max.y, max.z), p, vertex, 0x20, 0x80))
        return true;
    if ((mask & 0x20800) && SphereInEdge(S_vector(min.x, max.y, min.z), S_vector(max.x, max.y, min.z), p, vertex, 0x4, 0x10))
        return true;
    if ((mask & 0x420) && SphereInEdge(S_vector(max.x, max.y, min.z), S_vector(max.x, max.y, max.z), p, vertex, 0x10, 0x80))
        return true;
    if ((mask & 0x400200) && SphereInEdge(S_vector(max.x, max.y, max.z), S_vector(min.x, max.y, max.z), p, vertex, 0x80, 0x40))
        return true;
    if ((mask & 0x104) && SphereInEdge(S_vector(min.x, max.y, max.z), S_vector(min.x, max.y, min.z), p, vertex, 0x40, 0x4))
        return true;
    if (!vertex)
        return false;
    S_vector corner;
    switch (vertex) {
    case 0x1:
        corner = min;
        break;
    case 0x2:
        corner = S_vector(min.x, min.y, max.z);
        break;
    case 0x4:
        corner = S_vector(min.x, max.y, min.z);
        break;
    case 0x8:
        corner = S_vector(max.x, min.y, min.z);
        break;
    case 0x10:
        corner = S_vector(max.x, max.y, min.z);
        break;
    case 0x20:
        corner = S_vector(max.x, min.y, max.z);
        break;
    case 0x40:
        corner = S_vector(min.x, max.y, max.z);
        break;
    case 0x80:
        corner = max;
        break;
    }
    S_vector e = corner - p.from;
    float a = e.Dot(p.dir_n);
    if (a < 0.0f)
        return false;
    float disc = p.radius * p.radius - (e.Dot(e) - a * a);
    if (disc < 0.0f)
        return false;
    float t = a - (float)sqrt(disc);
    float sq = t * t;
    if (sq >= p.dist_sq)
        return false;
    p.pos = p.from + p.dir_n * t;
    p.normal = corner - p.pos;
    p.dist_sq = sq;
    return true;
}

// A volume that isn't a face, by its type (the cell's entries' type byte, as LineInNonface tells them apart), and where
// the sphere moving along p's move hits it nearer than p's hit, the square of the distance along the move, the place,
// the normal and the volume into p:
// - an upright cylinder (no ends, its axis at x, z) by the move's horizontal part, the normal from the hit to the axis
//   (the line test's arithmetic, with the radii added);
// - an oriented box, a rectangle at y = 0 in its own space, the move moved into it by the inverse matrix and the hit
//   turned back out by the matrix. Nothing where the move's bounds, widened by the radius, miss its bounds (the test
//   SphereInAABox begins with). A move upward meets the rectangle's underside, a move downward its top: where the
//   sphere already overlaps the plane over the rectangle, it is pushed back out (dist_sq 0); otherwise the edges the
//   move passes outside of are marked (upward from where the sphere's top meets the plane, downward from the move's
//   sides against each edge, PS2's the same), and with none marked the sphere's hit on the plane is taken. A
//   horizontal move within the radius of the plane marks edges by its direction alone (the bits there look turned by
//   a quarter against the edges they name; PS2's the same). Then the marked edges by SphereInEdge, and a corner one of
//   them names as a point;
// - a sphere, the normal from the hit to its center;
// - an axis-aligned box by SphereInAABox;
// - the box with an oriented box in it, that box by SphereInAABox in its own space.
// The normals point into the volume (the cylinder's, the sphere's and a corner's not normalized). Where the sphere
// starts inside a cylinder or a sphere, the hit lies behind from (t down to -radius), dist_sq still t's square.
// Written as the original: the corners are fresh temporaries at each use (PS2 constructs them anew too), the face
// tests' sums keep their two dot products apart (parenthesized), the downward move's contact point, its sides' product
// and its hit are declared and then assigned, which leaves them the slots of the out-of-line constructors' temporaries
// (the original's frame), and the corner test is a block of its own (its vector shares the spilled temporaries' slot).
// Open: one inline decision (the corner switch's second constructor is inlined, the original calls it), the FPU
// operand order of the upward hit's `sp.dir * t` and of the XTOBB's Normalize, and the frame (0x348 to the
// original's 0x354; the single temporaries above sp in another order).
// FUNCTION: GAME 0x5d15a0
bool g_collision::SphereInNonface(g_volume_triface *vol, tCSphereParam &p)
{
    switch (vol->flags & 0xff)
    {
    case VOLTYPE_CYLINDER:
    {
        g_volume_cylinder *cyl = (g_volume_cylinder *)vol;
        S_vector c(cyl->x, 0.0f, cyl->z);
        S_vector a(c.x - p.from.x, 0.0f, c.z - p.from.z);
        S_vector d = p.dir_n;
        d.y = 0.0f;
        float len = d.Magnitude();
        if (fabs(len) < MRG_ZERO)
            return false;
        float inv = 1.0f / len;
        d *= inv;
        float dot = a.Dot(d);
        if (dot <= 0.0f)
            return false;
        float r = cyl->radius + p.radius;
        float disc = r * r - (a.Dot(a) - dot * dot);
        if (disc <= 0.0f)
            return false;
        float t = dot - sqrt(disc);
        t *= inv;
        if (t < -p.radius)
            return false;
        if (t >= 0.0f && t * t >= p.dist_sq)
            return false;
        p.pos = p.from + p.dir_n * t;
        p.normal = S_vector(cyl->x, 0.0f, cyl->z) - p.pos;
        p.normal.y = 0.0f;
        p.dist_sq = t * t;
        p.hit = (g_collision_header *)vol;
        return true;
    }
    case VOLTYPE_OBB:
    {
        g_volume_obb *obb = (g_volume_obb *)vol;
        tCSphereParam sp = p;
        sp.from *= obb->inv_matrix;
        sp.to *= obb->inv_matrix;
        sp.dir = sp.to - sp.from;
        I3D_bbox bbox(obb->extents.min, obb->extents.max);
        {
            S_vector to = sp.from + sp.dir;
            S_vector from = sp.from;
            S_vector lo = bbox.min - S_vector(p.radius, p.radius, p.radius);
            S_vector hi = bbox.max + S_vector(p.radius, p.radius, p.radius);
            if (to.x > from.x) {
                if (to.x < lo.x || from.x > hi.x)
                    return false;
            } else {
                if (from.x < lo.x || to.x > hi.x)
                    return false;
            }
            if (to.z > from.z) {
                if (to.z < lo.z || from.z > hi.z)
                    return false;
            } else {
                if (from.z < lo.z || to.z > hi.z)
                    return false;
            }
            if (to.y > from.y) {
                if (to.y < lo.y || from.y > hi.y)
                    return false;
            } else {
                if (from.y < lo.y || to.y > hi.y)
                    return false;
            }
        }
        uint vertex = 0;
        uint flags = 0;
        sp.dir_n = sp.dir;
        sp.dir_n.Normalize();
        if (fabs(sp.dir.y) < MRG_ZERO) {
            if (fabs(sp.from.y) < sp.radius) {
                if (sp.dir.x > 0.0f)
                    flags |= OBB_EDGE_MIN_Z;
                if (sp.dir.z < 0.0f)
                    flags |= OBB_EDGE_MAX_X;
                if (sp.dir.x < 0.0f)
                    flags |= OBB_EDGE_MAX_Z;
                if (sp.dir.z > 0.0f)
                    flags |= OBB_EDGE_MIN_X;
            }
        } else if (sp.dir_n.y >= 0.0f) {
            S_vector n(0.0f, 1.0f, 0.0f);
            float dist = (sp.from - S_vector(bbox.min.x, 0.0f, bbox.min.z)).Dot(n);
            if (dist <= 0.0f && dist > -sp.radius) {
                S_vector nxp = n.Cross(sp.from);
                float d0 = (n.Dot(S_vector(bbox.min.x, 0.0f, bbox.min.z).Cross(
                    S_vector(bbox.max.x, 0.0f, bbox.min.z)))) +
                    ((S_vector(bbox.min.x, 0.0f, bbox.min.z) - S_vector(bbox.max.x, 0.0f, bbox.min.z)).Dot(nxp));
                float d1 = (n.Dot(S_vector(bbox.max.x, 0.0f, bbox.min.z).Cross(
                    S_vector(bbox.max.x, 0.0f, bbox.max.z)))) +
                    ((S_vector(bbox.max.x, 0.0f, bbox.min.z) - S_vector(bbox.max.x, 0.0f, bbox.max.z)).Dot(nxp));
                float d2 = (n.Dot(S_vector(bbox.max.x, 0.0f, bbox.max.z).Cross(
                    S_vector(bbox.min.x, 0.0f, bbox.max.z)))) +
                    ((S_vector(bbox.max.x, 0.0f, bbox.max.z) - S_vector(bbox.min.x, 0.0f, bbox.max.z)).Dot(nxp));
                float d3 = (n.Dot(S_vector(bbox.min.x, 0.0f, bbox.max.z).Cross(
                    S_vector(bbox.min.x, 0.0f, bbox.min.z)))) +
                    ((S_vector(bbox.min.x, 0.0f, bbox.max.z) - S_vector(bbox.min.x, 0.0f, bbox.min.z)).Dot(nxp));
                if (d0 <= 0.0f && d1 <= 0.0f && d2 <= 0.0f && d3 <= 0.0f) {
                    p.dist_sq = 0.0f;
                    p.pos = sp.from + n * (-dist - sp.radius);
                    p.pos *= obb->matrix;
                    p.normal = n.RotateByMatrix(obb->matrix);
                    p.hit = (g_collision_header *)vol;
                    return true;
                }
            }
            S_vector q = sp.from + n * sp.radius;
            float x = q.x - q.y * sp.dir_n.x / sp.dir_n.y;
            float z = q.z - q.y * sp.dir_n.z / sp.dir_n.y;
            if (x < bbox.min.x)
                flags |= OBB_EDGE_MIN_X;
            if (x > bbox.max.x)
                flags |= OBB_EDGE_MAX_X;
            if (z < bbox.min.z)
                flags |= OBB_EDGE_MIN_Z;
            if (z > bbox.max.z)
                flags |= OBB_EDGE_MAX_Z;
            if (!(flags & OBB_EDGES)) {
                float t = (sp.from.y + sp.radius) * (-1.0f / sp.dir.y);
                if (t >= 0.0f) {
                    S_vector hit = sp.from + sp.dir * t;
                    float sq = (hit - sp.from).Square();
                    if (sq < sp.dist_sq) {
                        p.dist_sq = sq;
                        p.pos = hit;
                        p.pos *= obb->matrix;
                        p.normal = n.RotateByMatrix(obb->matrix);
                        p.hit = (g_collision_header *)vol;
                        return true;
                    }
                }
            }
        } else {
            S_vector n(0.0f, -1.0f, 0.0f);
            float dist = (sp.from - S_vector(bbox.min.x, 0.0f, bbox.min.z)).Dot(n);
            if (dist <= 0.0f && dist > -sp.radius) {
                S_vector nxp = n.Cross(sp.from);
                float d0 = (n.Dot(S_vector(bbox.min.x, 0.0f, bbox.min.z).Cross(
                    S_vector(bbox.min.x, 0.0f, bbox.max.z)))) +
                    ((S_vector(bbox.min.x, 0.0f, bbox.min.z) - S_vector(bbox.min.x, 0.0f, bbox.max.z)).Dot(nxp));
                float d1 = (n.Dot(S_vector(bbox.min.x, 0.0f, bbox.max.z).Cross(
                    S_vector(bbox.max.x, 0.0f, bbox.max.z)))) +
                    ((S_vector(bbox.min.x, 0.0f, bbox.max.z) - S_vector(bbox.max.x, 0.0f, bbox.max.z)).Dot(nxp));
                float d2 = (n.Dot(S_vector(bbox.max.x, 0.0f, bbox.max.z).Cross(
                    S_vector(bbox.max.x, 0.0f, bbox.min.z)))) +
                    ((S_vector(bbox.max.x, 0.0f, bbox.max.z) - S_vector(bbox.max.x, 0.0f, bbox.min.z)).Dot(nxp));
                float d3 = (n.Dot(S_vector(bbox.max.x, 0.0f, bbox.min.z).Cross(
                    S_vector(bbox.min.x, 0.0f, bbox.min.z)))) +
                    ((S_vector(bbox.max.x, 0.0f, bbox.min.z) - S_vector(bbox.min.x, 0.0f, bbox.min.z)).Dot(nxp));
                if (d0 <= 0.0f && d1 <= 0.0f && d2 <= 0.0f && d3 <= 0.0f) {
                    p.dist_sq = 0.0f;
                    p.pos = sp.from + n * (-dist - sp.radius);
                    p.pos *= obb->matrix;
                    p.normal = n.RotateByMatrix(obb->matrix);
                    p.hit = (g_collision_header *)vol;
                    return true;
                }
            }
            S_vector q, m, hit;
            q = sp.from + n * sp.radius;
            m = sp.dir.Cross(q);
            if ((sp.dir.Dot(S_vector(bbox.min.x, 0.0f, bbox.min.z).Cross(S_vector(bbox.min.x, 0.0f, bbox.max.z)))) +
                (m.Dot(S_vector(bbox.min.x, 0.0f, bbox.min.z) - S_vector(bbox.min.x, 0.0f, bbox.max.z))) > 0.0f)
                flags |= OBB_EDGE_MIN_X;
            if ((sp.dir.Dot(S_vector(bbox.min.x, 0.0f, bbox.max.z).Cross(S_vector(bbox.max.x, 0.0f, bbox.max.z)))) +
                (m.Dot(S_vector(bbox.min.x, 0.0f, bbox.max.z) - S_vector(bbox.max.x, 0.0f, bbox.max.z))) > 0.0f)
                flags |= OBB_EDGE_MAX_Z;
            if ((sp.dir.Dot(S_vector(bbox.max.x, 0.0f, bbox.max.z).Cross(S_vector(bbox.max.x, 0.0f, bbox.min.z)))) +
                (m.Dot(S_vector(bbox.max.x, 0.0f, bbox.max.z) - S_vector(bbox.max.x, 0.0f, bbox.min.z))) > 0.0f)
                flags |= OBB_EDGE_MAX_X;
            if ((sp.dir.Dot(S_vector(bbox.max.x, 0.0f, bbox.min.z).Cross(S_vector(bbox.min.x, 0.0f, bbox.min.z)))) +
                (m.Dot(S_vector(bbox.max.x, 0.0f, bbox.min.z) - S_vector(bbox.min.x, 0.0f, bbox.min.z))) > 0.0f)
                flags |= OBB_EDGE_MIN_Z;
            if (!(flags & OBB_EDGES)) {
                float t = sp.dir.Dot(n);
                if (fabs(t) >= MRG_ZERO) {
                    float d = -n.Dot(S_vector(bbox.max.x, 0.0f, bbox.max.z) - n * sp.radius);
                    t = -(n.Dot(sp.from) + d) / t;
                    hit = sp.from + sp.dir * t;
                    if (t >= 0.0f) {
                        float sq = (hit - sp.from).Square();
                        if (sq < sp.dist_sq) {
                            p.dist_sq = sq;
                            p.pos = hit;
                            p.pos *= obb->matrix;
                            p.normal = n.RotateByMatrix(obb->matrix);
                            p.hit = (g_collision_header *)vol;
                            return true;
                        }
                    }
                }
            }
        }
        if (flags & OBB_EDGE_MIN_Z && SphereInEdge(S_vector(bbox.min.x, 0.0f, bbox.min.z),
            S_vector(bbox.max.x, 0.0f, bbox.min.z), sp, vertex, OBB_CORNER_MIN_MIN, OBB_CORNER_MAX_MIN)) {
            p.pos = sp.pos;
            p.dist_sq = sp.dist_sq;
            p.pos *= obb->matrix;
            p.normal = sp.normal.RotateByMatrix(obb->matrix);
            p.hit = (g_collision_header *)vol;
            return true;
        }
        if (flags & OBB_EDGE_MAX_X && SphereInEdge(S_vector(bbox.max.x, 0.0f, bbox.min.z),
            S_vector(bbox.max.x, 0.0f, bbox.max.z), sp, vertex, OBB_CORNER_MAX_MIN, OBB_CORNER_MAX_MAX)) {
            p.pos = sp.pos;
            p.dist_sq = sp.dist_sq;
            p.pos *= obb->matrix;
            p.normal = sp.normal.RotateByMatrix(obb->matrix);
            p.hit = (g_collision_header *)vol;
            return true;
        }
        if (flags & OBB_EDGE_MAX_Z && SphereInEdge(S_vector(bbox.max.x, 0.0f, bbox.max.z),
            S_vector(bbox.min.x, 0.0f, bbox.max.z), sp, vertex, OBB_CORNER_MAX_MAX, OBB_CORNER_MIN_MAX)) {
            p.pos = sp.pos;
            p.dist_sq = sp.dist_sq;
            p.pos *= obb->matrix;
            p.normal = sp.normal.RotateByMatrix(obb->matrix);
            p.hit = (g_collision_header *)vol;
            return true;
        }
        if (flags & OBB_EDGE_MIN_X && SphereInEdge(S_vector(bbox.min.x, 0.0f, bbox.max.z),
            S_vector(bbox.min.x, 0.0f, bbox.min.z), sp, vertex, OBB_CORNER_MIN_MAX, OBB_CORNER_MIN_MIN)) {
            p.pos = sp.pos;
            p.dist_sq = sp.dist_sq;
            p.pos *= obb->matrix;
            p.normal = sp.normal.RotateByMatrix(obb->matrix);
            p.hit = (g_collision_header *)vol;
            return true;
        }
        if (vertex) {
            S_vector corner;
            switch (vertex)
            {
            case OBB_CORNER_MIN_MIN:
                corner = S_vector(bbox.min.x, 0.0f, bbox.min.z);
                break;
            case OBB_CORNER_MAX_MIN:
                corner = S_vector(bbox.max.x, 0.0f, bbox.min.z);
                break;
            case OBB_CORNER_MAX_MAX:
                corner = S_vector(bbox.max.x, 0.0f, bbox.max.z);
                break;
            case OBB_CORNER_MIN_MAX:
                corner = S_vector(bbox.min.x, 0.0f, bbox.max.z);
                break;
            }
            S_vector d = corner - sp.from;
            float dot = d.Dot(sp.dir_n);
            if (dot <= 0.0f)
                return false;
            float disc = sp.radius * sp.radius - (d.Dot(d) - dot * dot);
            if (disc < 0.0f)
                return false;
            float t = dot - sqrt(disc);
            if (t * t >= sp.dist_sq)
                return false;
            p.pos = sp.from + sp.dir_n * t;
            p.dist_sq = t * t;
            p.normal = (corner - p.pos).RotateByMatrix(obb->matrix);
            p.pos *= obb->matrix;
            p.hit = (g_collision_header *)vol;
            return true;
        }
        return false;
    }
    case VOLTYPE_SPHERE:
    {
        g_volume_sphere *sph = (g_volume_sphere *)vol;
        S_vector a = sph->pos - p.from;
        float dot = a.Dot(p.dir_n);
        if (dot <= 0.0f)
            return false;
        float r = sph->radius + p.radius;
        float disc = r * r - (a.Dot(a) - dot * dot);
        if (disc < 0.0f)
            return false;
        float t = dot - sqrt(disc);
        if (t < -p.radius)
            return false;
        if (t >= 0.0f && t * t >= p.dist_sq)
            return false;
        p.pos = p.from + p.dir_n * t;
        p.normal = sph->pos - p.pos;
        p.dist_sq = t * t;
        p.hit = (g_collision_header *)vol;
        return true;
    }
    case VOLTYPE_AABB:
    {
        g_volume_aabb *box = (g_volume_aabb *)vol;
        if (SphereInAABox(box->bbox.min, box->bbox.max, p)) {
            p.hit = (g_collision_header *)vol;
            return true;
        }
        return false;
    }
    case VOLTYPE_XTOBB:
    {
        g_volume_xtobb *box = (g_volume_xtobb *)vol;
        tCSphereParam sp = p;
        sp.from *= box->inv_matrix;
        sp.to *= box->inv_matrix;
        sp.dir = sp.to - sp.from;
        sp.dir_n = sp.dir;
        sp.dir_n.Normalize();
        if (SphereInAABox(box->extents.min, box->extents.max, sp)) {
            p.pos = sp.pos;
            p.pos *= box->matrix;
            p.dist_sq = sp.dist_sq;
            p.normal = sp.normal.RotateByMatrix(box->matrix);
            p.hit = (g_collision_header *)vol;
            return true;
        }
        return false;
    }
    }
    return false;
}

// A cell's volumes (PS2 name), as LineInTreeLevel: the nonfaces by SphereInNonface, the faces whose bounds overlap the
// move's by SphereInFace; the faces lie in x order (their first vertex's), so the walk ends at the first beyond the
// move. True where one was hit nearer than p's hit, which then goes into p.
// FUNCTION: GAME 0x5d3870
bool g_collision::SphereInTreeLevel(tCollTreeLevel *cell, tCSphereParam &p)
{
    bool hit = false;
    int i = cell->num_volumes;
    g_volume_triface **vols = cell->volumes;
    for (; i > 0; i--)
    {
        g_volume_triface *f = *vols++;
        if (f->flags & p.flags2)
            continue;
        if (f->flags & (VOLF_NONFACE | VOLF_DISABLED))
        {
            hit |= SphereInNonface(f, p);
            continue;
        }
        if (f->vertices[2]->x < p.min_x)
            continue;
        if (f->vertices[0]->x > p.max_x)
            break;
        float min_z = f->vertices[0]->z, max_z = min_z;
        float z = f->vertices[2]->z;
        if (z < min_z)
            min_z = z;
        else if (z > max_z)
            max_z = z;
        z = f->vertices[1]->z;
        if (z < min_z)
            min_z = z;
        else if (z > max_z)
            max_z = z;
        if (!(min_z <= p.max_z))
            continue;
        if (!(max_z >= p.min_z))
            continue;
        int unknown;
        hit |= SphereInFace(f, p, unknown);
    }
    return hit;
}

// A sphere of that radius moved from from along dir against the level and the moving objects (PS2 name), as _TestLineV
// with the line: p filled in, the moving objects tested first (a hit there shortens the move to it), then the grid's
// cells under the move's bounds whose volumes reach above its lowest point. The collision hit, NULL for none; where
// it hit into pos, the surface's normal into normal, the square of the distance moved into lastSphereDistance.
// FUNCTION: GAME 0x5d39a0
int g_collision::_TestSphereB(const S_vector &from, const S_vector &dir, float radius, S_vector *pos,
    S_vector *normal, uint flags, uint flags2)
{
    if (!level)
        return 0;
    tCSphereParam p;
    p.hit = NULL;
    p.dir = dir;
    p.dir_n = dir;
    p.dir_n.Normalize();
    p.unknown_84 = false;
    p.from = from;
    p.to = from + dir;
    p.radius = radius;
    p.flags = flags;
    p.flags2 = flags2 | VOLF_DISABLED;
    p.min_x = Min(from.x, p.to.x) - radius;
    p.min_z = Min(from.z, p.to.z) - radius;
    p.max_x = radius + Max(from.x, p.to.x);
    p.max_z = radius + Max(from.z, p.to.z);
    bool hit = false;
    p.dist_sq = dir.Square();
    if (flags != COLF_ALL && dyn_grid)
    {
        if (SphereInDynamics(p))
        {
            // The move ends where it hit. Through a reference: read directly, the pos - from below squares its
            // terms for the Normalize in another order (VC6 orders a sum's terms by its operands' numbering).
            const S_vector &end = p.pos;
            p.min_x = Min(from.x, end.x) - radius;
            p.min_z = Min(from.z, end.z) - radius;
            p.max_x = radius + Max(from.x, end.x);
            p.max_z = radius + Max(from.z, end.z);
            p.dir = end - p.from;
            p.dir_n = p.dir;
            p.dir_n.Normalize();
            p.to = end;
            p.min_x = Min(from.x, p.to.x) - radius;
            p.min_z = Min(from.z, p.to.z) - radius;
            p.max_x = radius + Max(from.x, p.to.x);
            p.max_z = radius + Max(from.z, p.to.z);
            hit = true;
            p.dist_sq = p.dir.Square();
        }
    }
    if (flags2 != COLF_ALL)
    {
        float min_y = Min(from.y, p.to.y) - radius;
        p.side = S_vector(0.0f, 1.0f, 0.0f).Cross(p.dir_n);
        if (p.side.Square() < .001)
            p.side = S_vector(1.0f, 0.0f, 0.0f);
        p.side.Normalize();
        // The planes along the move through the sphere's two sides. The product twice, as PS2 computes it: VC6
        // shares it, its copies' homes are the products' `fst` slots, and the four vectors' temporaries share two
        // areas of the frame (a side_r local gets a frame larger by a vector and other x87 code).
        float d0 = -p.side.Dot(from + p.side * radius);
        float d1 = -p.side.Dot(from - p.side * radius);
        if (d0 > d1)
        {
            p.side_d_min = d1;
            p.side_d_max = d0;
        }
        else
        {
            p.side_d_min = d0;
            p.side_d_max = d1;
        }
        float f = 1.0f / level->cell_size_x;
        int x0 = (int)((p.min_x - level->min_x) * f);
        int x1 = (int)((p.max_x - level->min_x) * f + 1.0f);
        f = 1.0f / level->cell_size_z;
        int z0 = (int)((p.min_z - level->min_z) * f);
        int z1 = (int)((p.max_z - level->min_z) * f + 1.0f);
        if (x0 < 0)
            x0 = 0;
        if (z0 < 0)
            z0 = 0;
        if (x1 > level->width)
            x1 = level->width;
        if (z1 > level->height)
            z1 = level->height;
        for (int z = z0; z < z1; z++)
        {
            for (int x = x0; x < x1; x++)
            {
                tCollTreeLevel *cell = level->cells[x + z * level->width];
                if (cell->max_y >= min_y)
                    hit |= SphereInTreeLevel(cell, p);
                last_x = x;
                last_cell = cell;
                last_z = z;
            }
        }
    }
    if (!hit)
        return 0;
    lastSphereDistance = p.dist_sq;
    // No code: it numbers the normal's x before the Normalize's reciprocal (tools/c2trace.py --x87 shows the ids),
    // which makes its x *= f, with x kept on the FPU stack, the original's fld st(0); fmulp st(2), not fxch st(1);
    // fmul st(1); fxch st(1). No spelling of the end gave it.
    (void)p.normal.x;
    p.normal.Normalize();
    if (pos)
        *pos = p.pos;
    if (!normal)
        return (int)p.hit;
    *normal = -p.normal;
    return (int)p.hit;
}

// A sphere slid along what it hits (PC only, the name is ours; TestSphereSlide's, for C_dyn_object::AI): from a
// hundredth above from, the move two hundredths lower, up to ten passes of _TestSphereB. Each hit moves the sphere to
// where it touched and turns the rest of the move along the plane it hit (to the move's end taken along the normal onto
// the plane, the end itself for a null normal), then shortens it to nineteen twentieths of the way to each earlier
// contact's plane it would cross; the plane, the touch point and the collision (where its flags have
// COLLHF_UNKNOWN_40, else NULL) go into contacts. The last collision hit, 0 for none; where the sphere ended up into
// pos, the last normal (up at first) into normal. The starting vectors' sums with constant vectors (VC6 folds the
// zeros away) and the plane's constructor spend the inline budget that leaves the later constructors and the final
// sum out of line, as in the original; hp.Dot(cp.normal) gives the loop's dot products the original's term order, and
// i and ret set before the vectors give the start's schedule and its one zero register.
// FUNCTION: GAME 0x5d4570
int g_collision::_TestSphereSlide(const S_vector &from, const S_vector &dir, float radius, S_vector *pos,
    S_vector *normal, uint flags, uint flags2)
{
    S_vector n, hp, dst, f, d;
    int i = 10;
    int ret = 0;
    n = S_vector(0.0f, 1.0f, 0.0f);
    f = from + S_vector(0.0f, .01f, 0.0f);
    d = dir - S_vector(0.0f, .02f, 0.0f);
    num_contacts = 0;
    do
    {
        i--;
        int hit = _TestSphereB(f, d, radius, &hp, &n, flags, flags2);
        if (!hit)
            break;
        ret = hit;
        float dist = -n.Dot(hp);
        S_plane pl(n, dist);
        if (!pl.Intersection(f + d, n, dst))
            dst = f + d;
        f = hp;
        d = dst - hp;
        for (uint j = 0; j < num_contacts; j++)
        {
            const S_plane &cp = (const S_plane &)contacts[j].plane;
            float dd = cp.normal.Dot(d);
            if (fabs(dd) < MRG_ZERO)
                continue;
            float t = -(hp.Dot(cp.normal) + cp.d) / dd;
            if (t > 0.0f && t < 1.0f)
                d = d * t * .95f;
        }
        S_slide_contact *c = &contacts[num_contacts++];
        if (((g_collision_header *)hit)->flags & COLLHF_UNKNOWN_40)
            c->coll = (g_collision_header *)hit;
        else
            c->coll = NULL;
        (S_vector &)c->pos = hp;
        ((S_plane &)c->plane).d = dist;
        ((S_plane &)c->plane).normal = n;
    } while (i);
    *pos = f + d;
    if (normal)
        *normal = n;
    return ret;
}

// Where two curves of spheres come nearest in time: their segments walked together in time order, the earlier end
// first, each pair of segments moved as spheres of radius r0 and r1 by TestSphereSphere at the segments' speeds; the
// earliest time within both segments, FLT_MAX for none (or where either curve has fewer than two points). The walk
// stops at the first time that comes no earlier than the best one.
// FUNCTION: GAME 0x5d4a00
float g_collision::TestCurveCurve(const std::vector<gcol_CurveSphereStruct> *c0, float r0,
    const std::vector<gcol_CurveSphereStruct> *c1, float r1)
{
    float best = FLT_MAX;
    if (c0->size() < 2)
        return best;
    if (c1->size() < 2)
        return best;
    const gcol_CurveSphereStruct *a1 = c0->begin();
    const gcol_CurveSphereStruct *a = a1++;
    const gcol_CurveSphereStruct *b1 = c1->begin();
    const gcol_CurveSphereStruct *b = b1++;
    while (a1 < c0->end() && b1 < c1->end())
    {
        float f0 = 1.0f / (a1->t - a->t);
        float f1 = 1.0f / (b1->t - b->t);
        // Constructed and then scaled in place: as (a1->pos - a->pos) * f0, the differences take temporaries of their
        // own
        S_vector d0 = a1->pos - a->pos;
        d0 *= f0;
        S_vector d1 = b1->pos - b->pos;
        d1 *= f1;
        S_vector p0 = a->pos - d0 * a->t;
        S_vector p1 = b->pos - d1 * b->t;
        float t = TestSphereSphere(p0, d0, r0, p1, d1, r1);
        if (t >= a->t && t >= b->t)
        {
            if (t <= a1->t && t <= b1->t)
            {
                if (t < best)
                    best = t;
                else
                    break;
            }
            else if (t > best)
                break;
        }
        if (a1->t < b1->t)
            a = a1++;
        else
            b = b1++;
    }
    return best;
}

// The moved sphere against a vehicle's collisions, but those with one of the sphere's flags and the next test's
// owners: the spheres (0x42, 0x44) by where the sphere first touches them, a box (0x41) by SphereInAABox, an oriented
// box (0x40) the same in its own space (the move's ends moved into it by the inverse matrix, the hit turned back by the
// matrix), an upright cylinder (0x84) by its circle in x and z. True where one was hit nearer than p's hit, which then
// goes into p; a cylinder ends the test, true where it was hit nearer, false where it wasn't.
// FUNCTION: GAME 0x5d4c30
bool g_collision::SphereInVehicle(std::vector<tDynamicCollObject *> *colls, tCSphereParam &p)
{
    bool hit = false;
    for (uint i = 0; i < colls->size(); i++)
    {
        tDynamicCollObject *obj = (*colls)[i];
        if (obj->flags & p.flags)
            continue;
        bool owner = false;
        for (uint j = num_next_coll_owners; j--;)
        {
            if (obj == next_coll_owners[j])
            {
                owner = true;
                break;
            }
        }
        if (owner)
            continue;
        switch (obj->type & 0xff)
        {
        case DYNCOLL_UNKNOWN_42:
        {
            const I3D_bsphere &bs = (I3D_bsphere &)obj->bsphere;
            S_vector d = bs.pos - p.from;
            float t = d.Dot(p.dir_n);
            float r = bs.radius + p.radius;
            float disc = r * r - (d.Square() - t * t);
            if (disc < 0.0f)
                continue;
            t -= sqrt(disc);
            if (t < -p.radius)
                continue;
            if (t * t >= p.dist_sq)
                continue;
            p.pos = p.from + p.dir_n * t;
            p.normal = bs.pos - p.pos;
            if (p.normal.y < 0.0f)
                p.normal.y = 0.0f;
            p.hit = (g_collision_header *)obj;
            p.dist_sq = t * t;
            hit = true;
            break;
        }
        case DYNCOLL_UNKNOWN_44:
        {
            const I3D_bsphere &bs = (I3D_bsphere &)obj->bsphere;
            S_vector d = bs.pos - p.from;
            float t = d.Dot(p.dir_n);
            float r = bs.radius + p.radius;
            float disc = r * r - (d.Square() - t * t);
            if (disc < 0.0f)
                continue;
            t -= sqrt(disc);
            if (t < -p.radius)
                continue;
            if (t * t >= p.dist_sq)
                continue;
            hit = true;
            p.pos = p.from + p.dir_n * t;
            p.normal = bs.pos - p.pos;
            p.hit = (g_collision_header *)obj;
            p.dist_sq = t * t;
            break;
        }
        case DYNCOLL_UNKNOWN_41:
            if (SphereInAABox(((I3D_bbox &)obj->bbox).min, ((I3D_bbox &)obj->bbox).max, p))
            {
                p.hit = (g_collision_header *)obj;
                hit = true;
            }
            break;
        case DYNCOLL_UNKNOWN_40:
        {
            tCSphereParam sp = p;
            sp.from *= *obj->inv_matrix;
            sp.to *= *obj->inv_matrix;
            sp.dir = sp.to - sp.from;
            sp.dir_n = sp.dir;
            sp.dir_n.Normalize();
            if (SphereInAABox(((I3D_bbox &)obj->bbox).min, ((I3D_bbox &)obj->bbox).max, sp))
            {
                p.pos = sp.pos;
                p.pos *= *obj->matrix;
                p.dist_sq = sp.dist_sq;
                p.normal = sp.normal.RotateByMatrix(*obj->matrix);
                hit = true;
                p.hit = (g_collision_header *)obj;
            }
            break;
        }
        case DYNCOLL_CYLINDER:
        {
            // The cylinder's axis from the sphere's start and the move's direction in x and z. The difference through
            // operator-= (its y left dead): written member by member, VC6 sums the dot product and d's square z first
            // and takes n.z off the FPU stack in the dot, where the original sums them x first.
            S_vector d = ((I3D_bsphere &)obj->bsphere).pos;
            d -= p.from;
            d.y = 0.0f;
            S_vector n = p.dir_n;
            n.y = 0.0f;
            float len = n.Magnitude();
            if (fabs(len) < MRG_ZERO)
                return false;
            float inv = 1.0f / len;
            n *= inv;
            float b = d.Dot(n);
            float r = obj->cylinder_radius + p.radius;
            float disc = r * r - (d.Square() - b * b);
            if (disc <= 0.0f)
                return false;
            // Two statements: as (b - sqrt(disc)) * inv, VC6 keeps disc on the FPU stack instead of inv
            float t = b - sqrt(disc);
            t *= inv;
            if (t < -p.radius)
                return false;
            if (t * t >= p.dist_sq)
                return false;
            p.pos = p.from + p.dir_n * t;
            p.normal = ((I3D_bsphere &)obj->bsphere).pos - p.pos;
            p.hit = (g_collision_header *)obj;
            p.dist_sq = t * t;
            p.normal.y = 0.0f;
            return true;
        }
        }
    }
    return hit;
}

// A sphere of that radius moved from from along dir against the collisions of a vehicle a human is in, but those
// with one of flags (SphereInVehicle): the collision hit, NULL for none. On a hit lastSphereDistance gets the square of
// its distance along the move, the next test's callback is asked where the hit is a moving object, and its place goes
// into vehicle_hit_pos and pos and its normal, normalized and negated, into normal where they aren't NULL. PS2's adds a
// flag of its own to flags2.
// FUNCTION: GAME 0x5d5490
int g_collision::TestSphereInVehicle(std::vector<tDynamicCollObject *> *colls, const S_vector &from,
    const S_vector &dir, float radius, S_vector *pos, S_vector *normal, uint flags, uint flags2)
{
    tCSphereParam p;
    p.hit = NULL;
    p.dir = dir;
    p.dir_n = dir;
    p.dir_n.Normalize();
    p.from = from;
    p.to = from + dir;
    p.radius = radius;
    p.flags = flags;
    p.flags2 = flags2;
    p.dist_sq = dir.Square();
    if (!SphereInVehicle(colls, p))
        return 0;
    lastSphereDistance = p.dist_sq;
    p.normal.Normalize();
    if (next_coll_callback && p.hit && ((tDynamicCollObject *)p.hit)->type & DYNCOLL_DYNAMIC)
        next_coll_callback((tDynamicCollObject *)p.hit, &p);
    (S_vector &)vehicle_hit_pos = p.pos;
    if (pos)
        *pos = p.pos;
    if (normal)
        *normal = -p.normal;
    return (int)p.hit;
}
