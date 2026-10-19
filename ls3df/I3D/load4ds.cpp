// The 4ds loader (PS2 load4ds.cpp), in the original from 0x10060c60 to its marker 0x10061970: LoadTargetList's $E, then
// LoadImage (0x10060cb0), LoadMeshImage, two more functions and LoadAnimation5ds (PS2 Load5ds.cpp's,
// LoadAnimation5ds__FPCcPPvPC9_FILETIME: const char *, void **, const FILETIME *, here in ecx, edx and on the stack,
// retn 4). Only LoadMeshImage is written yet. Names are the PS2 build's unless noted.
#include "I3D/I3D.h"
#include "dta.h"

// The loader's state, which the frames' LoadImage functions read from and set (Load.cpp's or its neighbours'; the
// original's .bss has them from 0x101b5160 on, among the zero vectors of two files without code). The frames and the
// materials loaded so far, by their 1-based index in the file.
// GLOBAL: LS3DF 0x101b5160
I3D_frame *LoadFramy[10000];

// GLOBAL: LS3DF 0x101beda8
I3D_material *LoadMaterialy[5000];

// GLOBAL: LS3DF 0x101c3bc8
int LoadMaterialu;

// GLOBAL: LS3DF 0x101c3be4
LS3D_RESULT ImageErrFlg;

// GLOBAL: LS3DF 0x101c3bfc
int LoadImageHandle;

// GLOBAL: LS3DF 0x101c3c04
const char *ImageFileName;

// The original's $E of this list is the file's first function (0x10060c60), but its entry in .CRT$XCU comes before
// the zero vector's, as if it were defined above the header's include
// GLOBAL: LS3DF 0x101c3c08
C_list<I3D_target *> LoadTargetList;

// An object's mesh from the file: a reference to an earlier object's (the frame's index), or its levels, each with its
// distance, vertices (0xffff: texture coordinates for the first level's vertices) and face groups. Errors set
// ImageErrFlg and return NULL; a bad material ends the level's face groups.
// FUNCTION: LS3DF 0x10061120
I3D_mesh_object *LoadMeshImage()
{
    int index = 0;
    dtaRead(LoadImageHandle, &index, 2);
    if (index) {
        I3D_mesh_object *mesh = ((I3D_object *)LoadFramy[index - 1])->mesh;
        mesh->AddRef();
        return mesh;
    }
    int num_lods = 0;
    dtaRead(LoadImageHandle, &num_lods, 1);
    if (!num_lods)
        return NULL;
    I3D_mesh_object *mesh = pI3DDriver->CreateMesh();
    if (!mesh) {
        dbgPrintf("!! Mesh not created  LoadMeshImage()  %s", ImageFileName);
        ImageErrFlg = I3DERR_OUTOFMEM;
        return NULL;
    }
    for (int i = 0; i < num_lods; i++) {
        I3D_mesh_level *level = pI3DDriver->CreateMeshLevel();
        if (!level) {
            dbgPrintf("!! Mesh_level not created  LoadMeshImage()  %s", ImageFileName);
            ImageErrFlg = I3DERR_OUTOFMEM;
            return NULL;
        }
        dtaRead(LoadImageHandle, &level->lod_dist, 4);
        int num_vertices = 0;
        dtaRead(LoadImageHandle, &num_vertices, 2);
        if (num_vertices == 0xffff) {
            // An older level's m_pUV, which the PC's level has its vertex buffer's pointer in place of
            I3D_text_coor *&uv = (I3D_text_coor *&)level->vb.buffer;
            num_vertices = mesh->levels[0]->num_vertices;
            uv = new I3D_text_coor[num_vertices];
            if (!uv) {
                dbgPrintf("!! m_pUV not created  LoadMeshImage()  %s", ImageFileName);
                ImageErrFlg = I3DERR_OUTOFMEM;
                return NULL;
            }
            dtaRead(LoadImageHandle, uv, num_vertices * sizeof(I3D_text_coor));
        } else if (num_vertices) {
            I3D_vertex *verts = level->CreateVertexMesh(num_vertices);
            dtaRead(LoadImageHandle, verts, num_vertices * sizeof(I3D_vertex));
            level->UpdateBBox();
        }
        int num_fgroups = 0;
        dtaRead(LoadImageHandle, &num_fgroups, 1);
        while (num_fgroups--) {
            I3D_face_group *fg = pI3DDriver->CreateFaceGroup();
            if (!fg) {
                dbgPrintf("!! Face_group not created  LoadMeshImage()  %s", ImageFileName);
                ImageErrFlg = I3DERR_OUTOFMEM;
                return NULL;
            }
            int num_faces = 0;
            dtaRead(LoadImageHandle, &num_faces, 2);
            if (num_faces) {
                I3D_triface *faces = fg->CreateFaces(num_faces);
                if (!faces) {
                    dbgPrintf("!! Faces not created  LoadMeshImage()  %s", ImageFileName);
                    ImageErrFlg = I3DERR_OUTOFMEM;
                    return NULL;
                }
                dtaRead(LoadImageHandle, faces, num_faces * sizeof(I3D_triface));
            }
            int mat = 0;
            dtaRead(LoadImageHandle, &mat, 2);
            if (mat) {
                if (mat > LoadMaterialu) {
                    dbgPrintf("!! Invalid material ID:%d  materialu:%d  LoadImage() %s", mat, LoadMaterialu,
                        ImageFileName);
                    ImageErrFlg = I3DERR_FILECORRUPTED;
                    break;
                }
                fg->SetMaterial(LoadMaterialy[mat - 1]);
            }
            level->AddFGroup(fg, true);
        }
        mesh->SetLOD(level, i);
        level->Release();
    }
    return mesh;
}
