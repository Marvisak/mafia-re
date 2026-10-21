// The landscape (PC only), at 0x1001ef60-0x100201d0 in the original: Bryan Turner's ROAM demo in the engine's classes
// (see I3D.h), I3D_landpatch, then I3D_landscape, each in the demo's order from its constructor. The header's zero
// vector has its $E at the end (0x100201b0), then the copy of GetHeight. Names are the demo's where the roles agree.
// The file's name sorts between I3d_joint.cpp and I3d_light.cpp, but is a guess.
// The PC's patches always split down to node 0x800 and no deeper: the demo's variance test and the frame variance
// are gone, though the variance trees are still computed. A patch is drawn when its frame was rendered in the last
// frame, into the file's one vertex buffer, with the frame's matrix.
// Shapes the original needs: <new> (the landscape's destructor sets no EH state for its patches), the driver's list
// through a reference in the constructor, the patch number computed from the loop counters in Init.
// Still open: I3D_landpatch::Init (88%) keeps heightX in a register and stores m_HeightMap late, ours adds heightX
// from memory, which moves the loop's registers; no spelling that compiles to the same code changed it. Our linker
// keeps this file's copy of the empty m_RenderFGroup's fold group, the original the base classes' (I3D.h).
#include "I3D/I3D.h"
#include "I3D/render/render.h"
#include "IGraph/IGraph.h"
#include "dta.h"

#include <stdlib.h>
#include <string.h>

#include <new>

// The camera's position, which the patches' tessellation measures from (Reset's). Its $E is the file's first function.
// The .bss names are guesses that give the original's order: VC6's hash buckets put m_NextTriNode, gLandTextures,
// m_TriTreePool, gViewPos and gMaxNode in that order after the header's zero vector, and the three set to zero follow
// in declaration order.
// GLOBAL: LS3DF 0x101b42d0
S_vector gViewPos;

// The patches' texture coordinates: the base texture over a patch, the detail texture 40 times (the name is a guess)
// GLOBAL: LS3DF 0x100a710c
float gLandTexScale = 1.0f / PATCH_SIZE;

// LandTex0.tga to LandTex3.tga, then LandDetail.bmp, the detail texture
// GLOBAL: LS3DF 0x100c0078
ITexture *gLandTextures[5];

// The deepest node a patch would split to at its distance (Tessellate's; nothing reads it)
// GLOBAL: LS3DF 0x101b42dc
int gMaxNode;

// The vertex buffer all patches draw from, locked by each patch's Render, and its vertices
// GLOBAL: LS3DF 0x101b42e0
IDirect3DVertexBuffer8 *gLandVB = NULL;

// GLOBAL: LS3DF 0x101b42e4
LVERTEX2 *gLandVerts = NULL;

// GLOBAL: LS3DF 0x101b42e8
int gNumLandVerts = 0;

// FUNCTION: LS3DF 0x1001ef70
I3D_landpatch::I3D_landpatch()
{
    visual_type = I3D_VISUAL_LANDPATCH;
}

// FUNCTION: LS3DF 0x1001efc0
I3D_landpatch::~I3D_landpatch()
{
}

// A triangle's split: its base neighbor is split first unless the two form a diamond, so that the mesh has no cracks
// FUNCTION: LS3DF 0x1001efe0
void I3D_landpatch::Split(TriTreeNode *tri)
{
    if (tri->LeftChild)
        return;
    if (tri->BaseNeighbor && tri->BaseNeighbor->BaseNeighbor != tri)
        Split(tri->BaseNeighbor);
    tri->LeftChild = I3D_landscape::AllocateTri();
    tri->RightChild = I3D_landscape::AllocateTri();
    if (!tri->LeftChild)
        return;

    tri->LeftChild->BaseNeighbor = tri->LeftNeighbor;
    tri->LeftChild->LeftNeighbor = tri->RightChild;
    tri->RightChild->BaseNeighbor = tri->RightNeighbor;
    tri->RightChild->RightNeighbor = tri->LeftChild;

    if (tri->LeftNeighbor != NULL) {
        if (tri->LeftNeighbor->BaseNeighbor == tri)
            tri->LeftNeighbor->BaseNeighbor = tri->LeftChild;
        else if (tri->LeftNeighbor->LeftNeighbor == tri)
            tri->LeftNeighbor->LeftNeighbor = tri->LeftChild;
        else if (tri->LeftNeighbor->RightNeighbor == tri)
            tri->LeftNeighbor->RightNeighbor = tri->LeftChild;
    }
    if (tri->RightNeighbor != NULL) {
        if (tri->RightNeighbor->BaseNeighbor == tri)
            tri->RightNeighbor->BaseNeighbor = tri->RightChild;
        else if (tri->RightNeighbor->RightNeighbor == tri)
            tri->RightNeighbor->RightNeighbor = tri->RightChild;
        else if (tri->RightNeighbor->LeftNeighbor == tri)
            tri->RightNeighbor->LeftNeighbor = tri->RightChild;
    }
    if (tri->BaseNeighbor != NULL) {
        if (tri->BaseNeighbor->LeftChild) {
            tri->BaseNeighbor->LeftChild->RightNeighbor = tri->RightChild;
            tri->BaseNeighbor->RightChild->LeftNeighbor = tri->LeftChild;
            tri->LeftChild->RightNeighbor = tri->BaseNeighbor->RightChild;
            tri->RightChild->LeftNeighbor = tri->BaseNeighbor->LeftChild;
        } else
            Split(tri->BaseNeighbor);
    } else {
        tri->LeftChild->RightNeighbor = NULL;
        tri->RightChild->LeftNeighbor = NULL;
    }
}

// Splits every triangle down to node 0x800, two levels below the variance trees
// FUNCTION: LS3DF 0x1001f140
void I3D_landpatch::RecursTessellate(TriTreeNode *tri, int leftX, int leftY, int rightX, int rightY, int apexX,
    int apexY, int node)
{
    int centerX = (leftX + rightX) >> 1;
    int centerY = (leftY + rightY) >> 1;
    Split(tri);
    if (tri->LeftChild && node < 0x800) {
        RecursTessellate(tri->LeftChild, apexX, apexY, leftX, leftY, centerX, centerY, node << 1);
        RecursTessellate(tri->RightChild, rightX, rightY, apexX, apexY, centerX, centerY, 1 + (node << 1));
    }
}

// The leaves into the locked vertex buffer, a triangle each, white, with the map's heights for y
// FUNCTION: LS3DF 0x1001f1e0
void I3D_landpatch::RecursRender(TriTreeNode *tri, int leftX, int leftY, int rightX, int rightY, int apexX, int apexY)
{
    if (tri->LeftChild) {
        int centerX = (leftX + rightX) >> 1;
        int centerY = (leftY + rightY) >> 1;
        RecursRender(tri->LeftChild, apexX, apexY, leftX, leftY, centerX, centerY);
        RecursRender(tri->RightChild, rightX, rightY, apexX, apexY, centerX, centerY);
    } else {
        float leftZ = m_HeightMap[(leftY * MAP_SIZE) + leftX];
        float rightZ = m_HeightMap[(rightY * MAP_SIZE) + rightX];
        float apexZ = m_HeightMap[(apexY * MAP_SIZE) + apexX];

        gLandVerts[gNumLandVerts].x = (float)leftX;
        gLandVerts[gNumLandVerts].y = leftZ;
        gLandVerts[gNumLandVerts].z = (float)leftY;
        gLandVerts[gNumLandVerts].color = 0xffffffff;
        gLandVerts[gNumLandVerts].u = (float)leftX * gLandTexScale;
        gLandVerts[gNumLandVerts].v = (float)leftY * gLandTexScale;
        gLandVerts[gNumLandVerts].u2 = (float)(leftX * 40) * gLandTexScale;
        gLandVerts[gNumLandVerts].v2 = (float)(leftY * 40) * gLandTexScale;
        gNumLandVerts++;

        gLandVerts[gNumLandVerts].x = (float)rightX;
        gLandVerts[gNumLandVerts].y = rightZ;
        gLandVerts[gNumLandVerts].z = (float)rightY;
        gLandVerts[gNumLandVerts].color = 0xffffffff;
        gLandVerts[gNumLandVerts].u = (float)rightX * gLandTexScale;
        gLandVerts[gNumLandVerts].v = (float)rightY * gLandTexScale;
        gLandVerts[gNumLandVerts].u2 = (float)(rightX * 40) * gLandTexScale;
        gLandVerts[gNumLandVerts].v2 = (float)(rightY * 40) * gLandTexScale;
        gNumLandVerts++;

        gLandVerts[gNumLandVerts].x = (float)apexX;
        gLandVerts[gNumLandVerts].y = apexZ;
        gLandVerts[gNumLandVerts].z = (float)apexY;
        gLandVerts[gNumLandVerts].color = 0xffffffff;
        gLandVerts[gNumLandVerts].u = (float)apexX * gLandTexScale;
        gLandVerts[gNumLandVerts].v = (float)apexY * gLandTexScale;
        gLandVerts[gNumLandVerts].u2 = (float)(apexX * 40) * gLandTexScale;
        gLandVerts[gNumLandVerts].v2 = (float)(apexY * 40) * gLandTexScale;
        gNumLandVerts++;
    }
}

// A triangle's variance is how far the map at its hypotenuse's middle is from the edge's interpolated height, the
// largest of its own and its children's down to 8 units; the trees keep it plus one
// FUNCTION: LS3DF 0x1001f570
uchar I3D_landpatch::RecursComputeVariance(int leftX, int leftY, uchar leftZ, int rightX, int rightY, uchar rightZ,
    int apexX, int apexY, uchar apexZ, int node)
{
    int centerX = (leftX + rightX) >> 1;
    int centerY = (leftY + rightY) >> 1;
    uchar myVariance;
    uchar centerZ = m_HeightMap[(centerY * MAP_SIZE) + centerX];
    myVariance = abs((int)centerZ - (((int)leftZ + (int)rightZ) >> 1));
    if (abs(leftX - rightX) >= 8 || abs(leftY - rightY) >= 8) {
        uchar v = RecursComputeVariance(apexX, apexY, apexZ, leftX, leftY, leftZ, centerX, centerY, centerZ,
            node << 1);
        myVariance = max(myVariance, v);
        v = RecursComputeVariance(rightX, rightY, rightZ, apexX, apexY, apexZ, centerX, centerY, centerZ,
            1 + (node << 1));
        myVariance = max(myVariance, v);
    }
    if (node < (1 << VARIANCE_DEPTH))
        m_CurrentVariance[node] = 1 + myVariance;
    return myVariance;
}

// The demo's, then the PC's: the bound volume from the patch's heights, and the frame's position at its place in the
// map. The heights' loops run over the members, the box's corners are set one at a time through one temporary (the
// two-vector constructor keeps two), the sphere's center is a local computed before the box is copied, and the
// position is stored member by member (the frame's SetPos takes a vector, which goes through a temporary).
// FUNCTION: LS3DF 0x1001f690
void I3D_landpatch::Init(int heightX, int heightY, int worldX, int worldY, uchar *hMap)
{
    m_BaseLeft.RightNeighbor = m_BaseLeft.LeftNeighbor = m_BaseRight.RightNeighbor = m_BaseRight.LeftNeighbor =
        m_BaseLeft.LeftChild = m_BaseLeft.RightChild = m_BaseRight.LeftChild = m_BaseLeft.LeftChild = NULL;
    m_BaseLeft.BaseNeighbor = &m_BaseRight;
    m_BaseRight.BaseNeighbor = &m_BaseLeft;
    m_WorldX = worldX;
    m_WorldY = worldY;
    m_HeightMap = &hMap[heightY * MAP_SIZE + heightX];
    m_VarianceDirty = 1;

    float max_height = 0.0f;
    float min_height = 255.0f;
    for (int y = m_WorldY; y < m_WorldY + PATCH_SIZE; y++) {
        for (int x = m_WorldX; x < m_WorldX + PATCH_SIZE; x++) {
            float h = hMap[y * MAP_SIZE + x];
            if (h > max_height)
                max_height = h;
            if (h < min_height)
                min_height = h;
        }
    }
    texture = NULL;
    float half = (max_height - min_height) * 0.5f;
    I3D_bbox bb;
    bb.min = S_vector(0.0f, min_height, 0.0f);
    bb.max = S_vector(PATCH_SIZE, max_height, PATCH_SIZE);
    float center = min_height + half;
    vis_bound.bbox = bb;
    vis_bound.bsphere.pos = S_vector(PATCH_SIZE / 2, center, PATCH_SIZE / 2);
    vis_bound.bsphere.radius = S_vector(PATCH_SIZE / 2, half, PATCH_SIZE / 2).Magnitude();
    if (!(vis_flags & VISF_WORLD_SPACE))
        vis_flags &= ~VISF_WBOUND_VALID;
    frm_flags &= ~(FRMFLAGS_HR_BOUND_VALID | FRMFLAGS_HR_BOUND_W_VALID);
    if (!(frm_flags & FRMFLAGS_LOCKED)) {
        f_matrix.m[3][0] = (float)m_WorldX;
        f_matrix.m[3][1] = 0.0f;
        f_matrix.m[3][2] = (float)m_WorldY;
        frm_flags = (frm_flags & ~(FRMFLAGS_HR_BOUND_W_VALID | FRMFLAGS_WMAT_VALID)) | FRMFLAGS_UPDATE;
    }
}

// FUNCTION: LS3DF 0x1001f8e0
void I3D_landpatch::Reset()
{
    m_BaseLeft.LeftChild = m_BaseLeft.RightChild = m_BaseRight.LeftChild = m_BaseLeft.LeftChild = NULL;
    m_BaseLeft.BaseNeighbor = &m_BaseRight;
    m_BaseRight.BaseNeighbor = &m_BaseLeft;
    m_BaseLeft.RightNeighbor = m_BaseLeft.LeftNeighbor = m_BaseRight.RightNeighbor = m_BaseRight.LeftNeighbor = NULL;
}

// FUNCTION: LS3DF 0x1001f920
void I3D_landpatch::ComputeVariance()
{
    m_CurrentVariance = m_VarianceLeft;
    RecursComputeVariance(0, PATCH_SIZE, m_HeightMap[PATCH_SIZE * MAP_SIZE], PATCH_SIZE, 0, m_HeightMap[PATCH_SIZE],
        0, 0, m_HeightMap[0], 1);
    m_CurrentVariance = m_VarianceRight;
    RecursComputeVariance(PATCH_SIZE, 0, m_HeightMap[PATCH_SIZE], 0, PATCH_SIZE, m_HeightMap[PATCH_SIZE * MAP_SIZE],
        PATCH_SIZE, PATCH_SIZE, m_HeightMap[(PATCH_SIZE * MAP_SIZE) + PATCH_SIZE], 1);
    m_VarianceDirty = 0;
}

// The PC's measure of the distance first: from the camera to the bound sphere, along the ground, into gMaxNode
// FUNCTION: LS3DF 0x1001f9b0
void I3D_landpatch::Tessellate()
{
    const I3D_bvolume &bvol = GetWBoundVolume();
    float radius = bvol.bsphere.radius;
    S_vector pos = bvol.bsphere.pos;
    float dx = gViewPos.x - pos.x;
    float dz = gViewPos.z - pos.z;
    float dist = (float)sqrt(dx * dx + dz * dz);
    if (radius > dist)
        dist = radius;
    gMaxNode = (int)(512.0f / (1.0f + (dist - radius) / 300.0f));

    m_CurrentVariance = m_VarianceLeft;
    RecursTessellate(&m_BaseLeft, m_WorldX, m_WorldY + PATCH_SIZE, m_WorldX + PATCH_SIZE, m_WorldY, m_WorldX,
        m_WorldY, 1);
    m_CurrentVariance = m_VarianceRight;
    RecursTessellate(&m_BaseRight, m_WorldX + PATCH_SIZE, m_WorldY, m_WorldX, m_WorldY + PATCH_SIZE,
        m_WorldX + PATCH_SIZE, m_WorldY + PATCH_SIZE, 1);
}

// The patch's triangles through the shared vertex buffer, with the frame's matrix and the patch's texture
// FUNCTION: LS3DF 0x1001fac0
void I3D_landpatch::Render()
{
    gNumLandVerts = 0;
    if (FAILED(gLandVB->Lock(0, 0, (BYTE **)&gLandVerts, D3DLOCK_NOSYSLOCK)))
        dbgPrintf("nepodaril se LOCK !");
    RecursRender(&m_BaseLeft, 0, PATCH_SIZE, PATCH_SIZE, 0, 0, 0);
    RecursRender(&m_BaseRight, PATCH_SIZE, 0, 0, PATCH_SIZE, PATCH_SIZE, PATCH_SIZE);
    gLandVB->Unlock();
    pGlobalIGraph->SetWorldMatrix(GetMatrix());
    m_pD3DDevice8->SetTexture(0, texture ? texture->m_pD3DTexture8 : NULL);
    m_pD3DDevice8->DrawPrimitive(D3DPT_TRIANGLELIST, 0, gNumLandVerts / 3);
}

void I3D_landpatch::m_RenderFGroup(RENDER_FG_HELP_STRUCT *help)
{
}

// The tree nodes of all landscapes, and the first free one, which each frame's Reset frees all of
// GLOBAL: LS3DF 0x100c0074
int m_NextTriNode;

// GLOBAL: LS3DF 0x100c0090
TriTreeNode m_TriTreePool[POOL_SIZE];

// Inlined into Split, its only caller
TriTreeNode *I3D_landscape::AllocateTri()
{
    if (m_NextTriNode >= POOL_SIZE)
        return NULL;
    TriTreeNode *pTri = &m_TriTreePool[m_NextTriNode++];
    pTri->LeftChild = pTri->RightChild = NULL;
    return pTri;
}

// FUNCTION: LS3DF 0x1001fbb0
I3D_landscape::I3D_landscape()
{
    type = FRAME_LANDSCAPE;
    C_list<I3D_landscape *> &l = pI3DDriver->landscapes;
    if (!l.find(this))
        l.push_back(this);
}

// FUNCTION: LS3DF 0x1001fcc0
I3D_landscape::~I3D_landscape()
{
    pI3DDriver->landscapes.remove(this);
    if (m_HeightMap)
        delete[] m_HeightMap;
}

// The map is read only when dtaOpen returns handle 0 (the other files test for -1), and the size is that of a map of
// MAP_SIZE + 1 on a side, though the rows are MAP_SIZE apart. A failed allocation is tested after the memset. The
// textures are loaded but nothing gives them to the patches, except the detail texture, which Render sets. The patch
// number written out (not a counter of its own) is what VC6 keeps in ebx and tests for the outer loop.
// FUNCTION: LS3DF 0x1001fd90
void I3D_landscape::Init(const char *name, uint size)
{
    uchar *map;
    if (size > 0) {
        map = new uchar[(MAP_SIZE + 1) * (MAP_SIZE + 1)];
        memset(map, 0, (MAP_SIZE + 1) * (MAP_SIZE + 1));
        if (!map)
            return;
        int h = dtaOpen(name, false);
        if (!h) {
            dtaRead(h, map, (MAP_SIZE + 1) * (MAP_SIZE + 1));
            dtaClose(h);
        }
    } else
        map = (uchar *)name;
    m_HeightMap = map;

    pGlobalIGraph->CreateITexture("LandTex0.tga", NULL, ITC_TRUECOLOR | ITC_KEEP_PIXELS, &gLandTextures[0]);
    pGlobalIGraph->CreateITexture("LandTex1.tga", NULL, ITC_TRUECOLOR | ITC_KEEP_PIXELS, &gLandTextures[1]);
    pGlobalIGraph->CreateITexture("LandTex2.tga", NULL, ITC_TRUECOLOR | ITC_KEEP_PIXELS, &gLandTextures[2]);
    pGlobalIGraph->CreateITexture("LandTex3.tga", NULL, ITC_TRUECOLOR | ITC_KEEP_PIXELS, &gLandTextures[3]);
    pGlobalIGraph->CreateITexture("LandDetail.bmp", NULL, ITC_TRUECOLOR | ITC_KEEP_PIXELS, &gLandTextures[4]);

    for (int Y = 0; Y < NUM_PATCHES_PER_SIDE; Y++) {
        for (int X = 0; X < NUM_PATCHES_PER_SIDE; X++) {
            pGlobalIGraph->SetAppNameDebug("Creating landscape patch: %i/%i", Y * NUM_PATCHES_PER_SIDE + X + 1,
                NUM_PATCHES_PER_SIDE * NUM_PATCHES_PER_SIDE);
            I3D_landpatch *patch = &m_Patches[Y][X];
            patch->Init(X * PATCH_SIZE, Y * PATCH_SIZE, X * PATCH_SIZE, Y * PATCH_SIZE, m_HeightMap);
            patch->ComputeVariance();
            patch->LinkTo(this, 0);
        }
    }

    if (FAILED(m_pD3DDevice8->CreateVertexBuffer(30000 * sizeof(LVERTEX2),
            D3DUSAGE_WRITEONLY | D3DUSAGE_SOFTWAREPROCESSING, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2,
            D3DPOOL_MANAGED, &gLandVB)))
        dbgPrintf("nepodaril se vyrobit VB !");
}

// The demo's, with the camera's position for its view cone, and the patches rendered in the last frame for its visible
// ones
// FUNCTION: LS3DF 0x1001ff30
void I3D_landscape::Reset()
{
    m_NextTriNode = 0;
    gViewPos = pCurrentCamera->GetWorldPos();
    for (int Y = 0; Y < NUM_PATCHES_PER_SIDE; Y++) {
        for (int X = 0; X < NUM_PATCHES_PER_SIDE; X++) {
            I3D_landpatch *patch = &m_Patches[Y][X];
            patch->Reset();
            if (patch->isDirty())
                patch->ComputeVariance();
            if (RenderNumber == patch->render_number) {
                if (X > 0)
                    patch->GetBaseLeft()->LeftNeighbor = m_Patches[Y][X - 1].GetBaseRight();
                else
                    patch->GetBaseLeft()->LeftNeighbor = NULL;
                if (X < NUM_PATCHES_PER_SIDE - 1)
                    patch->GetBaseRight()->LeftNeighbor = m_Patches[Y][X + 1].GetBaseLeft();
                else
                    patch->GetBaseRight()->LeftNeighbor = NULL;
                if (Y > 0)
                    patch->GetBaseLeft()->RightNeighbor = m_Patches[Y - 1][X].GetBaseRight();
                else
                    patch->GetBaseLeft()->RightNeighbor = NULL;
                if (Y < NUM_PATCHES_PER_SIDE - 1)
                    patch->GetBaseRight()->RightNeighbor = m_Patches[Y + 1][X].GetBaseLeft();
                else
                    patch->GetBaseRight()->RightNeighbor = NULL;
            }
        }
    }
}

// FUNCTION: LS3DF 0x10020010
void I3D_landscape::Tessellate()
{
    int nCount;
    I3D_landpatch *patch = &m_Patches[0][0];
    for (nCount = 0; nCount < NUM_PATCHES_PER_SIDE * NUM_PATCHES_PER_SIDE; nCount++, patch++) {
        if (RenderNumber == patch->render_number)
            patch->Tessellate();
    }
}

// The render states for all patches: the vertex format with two texture coordinates, the detail texture modulating
// in the second stage, and the patch textures clamped
// FUNCTION: LS3DF 0x10020040
void I3D_landscape::Render()
{
    SetCullState(D3DCULL_CCW);
    SetVertexFormat(1);
    m_pD3DDevice8->SetStreamSource(0, gLandVB, sizeof(LVERTEX2));
    m_pD3DDevice8->SetTexture(1, gLandTextures[4] ? gLandTextures[4]->m_pD3DTexture8 : NULL);
    SetAlphaState(0);
    SetColorOp1(D3DTOP_MODULATE);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);

    int nCount;
    I3D_landpatch *patch = &m_Patches[0][0];
    for (nCount = 0; nCount < NUM_PATCHES_PER_SIDE * NUM_PATCHES_PER_SIDE; nCount++, patch++) {
        if (RenderNumber == patch->render_number)
            patch->Render();
    }

    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP);
    m_pD3DDevice8->SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP);
}
