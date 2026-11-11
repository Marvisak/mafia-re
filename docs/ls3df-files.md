# LS3DF.dll source files

A map of LS3DF.dll's code by source file, which the files in `ls3df/` follow. Every file is written; each one's header
comment gives its range, its order and what is open in it. Addresses are in `target/LS3DF.dll`.

## Method

- **The initializer table.** `.CRT$XCU` (`0x100a6004–0x100a61bc`) lists every dynamic initializer (`$E`) in link
  order: the object files in order, and within a file the globals in definition order.
- **The zero vector.** An engine header defines a zero `S_vector` with internal linkage, so every file that includes
  it gets its own copy and its own `$E` (three zero stores, then `ret`). It comes first in its file's group of the
  table. Its code, the file's *marker*, comes last in the file, followed only by out-of-line inline and template
  functions, or in the middle of a long file. That is VC6's automatic precompiled headers (`/YX`, see
  `tools/autopch.py`). A file compiled without one has the marker first (`I3d_area.cpp`, `load4ds.cpp`).
- **The identity matrix.** Every object that includes the engine header also has its own copy of
  `IdentityMatrixData` (0x40 bytes, never referenced) at the start of its `.rdata`. The original has 58. They mark the
  objects in `.rdata` as the zero vectors do in `.text` and `.bss`.
- **Names.** Strings name the classes (`!! Out of memory  I3D_frame::SetName()`), and three files name themselves
  through `__FILE__`. The PS2 build (`SLUS_206.71`) names its files through `__sinit_<file>.cpp`. Both builds link the
  engine in three folder groups (I3D, IGraph, ISound), each in alphabetical order, case-insensitively, as a VC6 project
  lists its files. The PC's exports and the source names in its `.data` run in that order too.

PS2 has 51 engine files. The PC's Rich header lists 61 C++ objects and one C object of the engine's own.

## The map

A *range* runs from the end of the previous file's marker to the end of this one's, so out-of-line inline functions
after a marker fall into the next range. The *other `$E`s* are the file's own globals. A `?` marks a guessed name,
usually one that sorts between its neighbours'.

| Range | Marker | Other `$E`s | File | Evidence |
| --- | --- | --- | --- | --- |
| `10001000–100013e0` | `100013c0` | `10001000` (empty) | `clipper.cpp` (PC only) | the clipper's parameters and arrays (`ReserveClipArrays()` cites `clipper.cpp`). The first object: its default-library requests start the link's |
| `100013e0–10003b40` | none | none | `clipper2.cpp`? (PC only) | the two clip routines; no strings, and no marker because it was compiled with `clipper.cpp`'s precompiled header (`/Yu`) |
| `10003b40–10004870` | `10004850` | | `clipping.cpp` (PC only) | the entry points and `ClipFacegroups()`, whose overflow message cites `clipping.cpp` |
| `10004870–10004890` | `10004870` | | `clipping2.cpp`? | only the marker, with its own identity matrix (`1009b360`) and zero vector |
| `10004890–10005180` | `10005160` | | `I3d_animation.cpp` | `I3D_animation_set`, from its constructor |
| `10005180–10005db0` | `10005180` | | `I3d_area.cpp` | `I3D_area`; compiled without a precompiled header, so the marker comes first |
| `10005db0–1000b0c0` | `10008390` | | `I3d_billboard.cpp` | `I3D_billboard` and `I3D_lensflare` (`renderBillboard()`); the marker before the billboard's `m_RenderFGroup` |
| `1000b0c0–1000de70` | `1000de50` | | `I3d_camera.cpp` | `I3D_camera`, from its constructor |
| `1000de70–10015530` | `1000dea0` | `1000de70`–`1000de90`, `10011d20`–`10011d50` (empty) | `i3d_collision.cpp` | the collision tests, `TestColGeo` to `SphereToSphereD`; the marker before the first function |
| `10015530–1001a1e0` | `10016fd0` | `10015530`, `10015580`, `100155d0`, `100188a0` | `I3d_driver.cpp` | `I3D_driver`; the marker after `DrawLines` |
| `1001a1e0–1001a6e0` | `1001a6c0` | | `I3d_dummy.cpp` | `I3D_dummy` |
| `1001a6e0–1001d2f0` | `1001c280` | `1001c780` | `I3d_frame.cpp` | `I3D_frame`; the marker after `EnumSubtree` |
| `1001d2f0–1001ed30` | `1001e840` | | `I3d_interpolator.cpp` | `I3D_interpolator` and `I3D_IPolSet` |
| `1001ed30–1001ef60` | `1001ef40` | | `I3d_joint.cpp` | `I3D_joint` |
| `1001ef60–100201d0` | `100201b0` | `1001ef60` | `I3d_landscape.cpp`? (PC only) | Bryan Turner's ROAM demo (`LandTex0.tga`): `I3D_landpatch` and `I3D_landscape` |
| `100201d0–10021570` | `10021550` | | `I3d_light.cpp` | `I3D_light` |
| `10021570–1002ade0` | `10026900` | | `I3d_lit_object.cpp` | `I3D_lit_object` and `litLod`; the marker after `Construct` |
| `1002ade0–1002cda0` | `1002c8a0` | | `I3d_material.cpp` | `I3D_material` and `I3D_anim_material` |
| `1002cda0–10030560` | `1002ea90` | | `I3d_math.cpp` | the math, `RandFloat` to `LineLineIntersect`; the marker between `S_vector::RotateByMatrix` and `RotateByNormMatrix` |
| `10030560–10031820` | `10031800` | | `I3d_mesh.cpp` | `I3D_face_group`, `I3D_mesh_level` and `I3D_mesh_object` |
| `10031820–100333d0` | `100333b0` | `100324b0` | `I3d_mirror.cpp` | `I3D_mirror` (no strings, but PS2's `SetColor`, `SetViewBox` and `LoadImage`) |
| `100333d0–10034350` | `10034330` | `10033b50`, `10033ba0` | `I3d_model.cpp` | `I3D_model` |
| `10034350–100367b0` | `10036520` | | `I3d_morph.cpp` | `morphBase` and `I3D_morph` |
| `100367b0–1003a430` | `10039260` | | `I3d_object.cpp` | `I3D_object` and the PC's `lod` |
| `1003a430–1003c070` | `1003c050` | | `I3d_occluder.cpp` | `I3D_occluder` and the occlusion pass |
| `1003c070–10041800` | `1003eaa0` | | `I3d_particle.cpp` | the particles' renderer (PC only), `I3D_emitor`, `I3D_part_element` and `I3D_part_base` |
| `10041800–10045930` | `100437d0` | `10041de0` (empty) | `I3d_portal.cpp` | `I3D_portal` and the walk through the portals |
| `10045930–10045e30` | `10045e10` | | `I3d_pose.cpp` | `I3D_pose` and `I3D_pose_set` |
| `10045e30–10048060` | `10048040` | `10046730` | `I3d_projector.cpp` | `I3D_projector` and its receivers |
| `10048060–1004e9b0` | `1004a860` | `10048a00`, `10048a50`, `10048aa0` (empty) | `I3d_scene.cpp` | `I3D_scene` |
| `1004e9b0–100508b0` | `10050890` | | `I3d_sector.cpp` | `I3D_sector`, after the sound properties' `SetupDefaults` and `MakeBlend` |
| `100508b0–10054240` | `100529a0` | `10050900`, `10050d30`, `10052f30` | `I3d_shadow.cpp` | `I3D_shadow` and the shadows' visual. One object: the visual's queue lies among the file's hashed `.bss` |
| `10054240–10056e80` | `10056840` | | `I3d_singlemesh.cpp` | `I3D_singlemesh` and its lods |
| `10056e80–10057650` | `10057630` | | `I3d_singlemorph.cpp` | `I3D_singlemorph` |
| `10057650–10059250` | `10059230` | | `I3d_sound.cpp` | `I3D_sound` |
| `10059250–1005a140` | `1005a120` | `10059440` | `I3d_target.cpp` | `I3D_target` |
| `1005a140–1005a200` | `1005a1e0` | | `I3d_user.cpp` | `I3D_user` |
| `1005a200–1005a7a0` | `1005a780` | | `I3d_visual.cpp` | `I3D_visual` (`I3D_visual::SetSpace()`) |
| `1005a7a0–1005aad0` | `1005aab0` | | `I3d_volume.cpp` | `I3D_volume` |
| `1005aad0–1005d010` | `1005c740` | `1005aad0` | `IBManager.cpp` | `__FILE__`: `C_IBuffer` and `C_IBManager` |
| `1005d010–10060390` | `1005e380` | `1005d010` | `lightmap.cpp` | PS2's `LightMap` (`LightMap::m_CreateLM()`); the marker mid-file |
| `10060390–10060c20` | `10060c00` | | `Load.cpp` | `LoadHierarchy`, `LoadAnimation` and the PC's `LoadShadow` (`load.cpp  ?::Open()`) |
| `10060c20–10060c40` | `10060c20` | | `Load3ds.cpp`? | only the marker (identity matrix `1009d298`) |
| `10060c40–10061800` | `10060c40` | `10060c60` | `load4ds.cpp` | `LoadImage()` and `LoadMeshImage()`; compiled without a precompiled header, so the marker comes first |
| `10061800–10061990` | `10061970` | | `Load5ds.cpp` | `LoadAnimation5ds` (`load5ds.cpp`) |
| `10061990–100619b0` | `10061990` | | `Load6ds.cpp`? | only the marker (identity matrix `1009d358`) |
| `100619b0–10061cd0` | none | none | `RadixSort.cpp` | the radix sort; no engine header, so neither a marker nor an identity matrix. The name is H&D Deluxe's |
| `10061cd0–10065a10` | `100658d0` | `10061cd0`, `10061d20`, `10061d70`, `10061dd0` | `render.cpp` | `I3D_driver::Render()` and what it calls |
| `10065a10–10068880` | `10067d00` | `10065a10`, `10065a30` | `T&l.cpp` (ours `T_l.cpp`) | the lighting and the vertex helpers; the marker after `VertexLightingProcess` |
| `10068880–1006ad40` | `10069de0` | `10068880` | `VBManager.cpp` | `__FILE__`: `C_VBuffer` and `C_VBManager`, then the rest of the multisets' functions after the marker |
| `1006ad40–1006b430` | none | none | `cpuid.c` (C) | Microsoft's CPUID sample and `GetCPUSpeed` |
| `1006b430–1006bd40` | `1006bd20` | `1006b430`, `1006bb80`, `1006bca0`, `1006bcc0` | `Globals.cpp` | the first IGraph file (exports from `0x1006b6d0`) |
| `1006bd40–100733a0` | `1006deb0` | `1006bd40`–`1006bf70` (8) | `IGraph.cpp` | `__FILE__`; the marker mid-file, the `C_str` COMDATs after `Init` |
| `100733a0–10075890` | `10075800` | | `IShow.cpp` | `IShow` and the DirectX SDK's texture renderer; compiled `/Gd` |
| `10075890–10075c10` | `10075bf0` | `10075890` | `IShowStream.cpp`? (PC only) | `IShow`'s music streams through the sound driver |
| `10075c10–1007b053` | `10078170` | `1007a570`, `1007a5c0` | `ITexture.cpp` | `ITexture` and the PC's texture manager |
| `1007b060–1007b370` | `1007b350` | | `IS_3Dlistener.cpp` | the ISound group, in alphabetical order |
| `1007b370–1007b680` | `1007b660` | | `IS_3Dsound.cpp` | |
| `1007b690–1007c2a0` | `1007c280` | | `IS_driver.cpp` | |
| `1007c2a0–1007d430` | `1007d410` | `1007c2a0`, `1007c2f0` | `IS_sound.cpp` | |
| `1007d430–1007d9d0` | `1007d9b0` | | `IS_source.cpp` | |

After the engine and before the CRT (from `0x1008d404`) come the library objects, in the order the linker pulled them:

- `d3d8`'s thunk and `DXGetErrorString8` from `dxerr8.lib`, from `0x1007d9d0`.
- `DINPUT8`'s thunk (`0x10080364`) and data formats.
- S3's S3TC library (`0x100813e0–0x10086a09`), which `ITexture.cpp` compresses textures with: two C objects S3
  compiled with VC6 SP3, linked prebuilt. The build links the original's objects, `ls3df/S3TC/s3tc.lib`.
- `DSOUND`'s thunk (`0x10086a10`).
- The DirectShow base classes (`0x10086a20–0x1008d3e0`): seven objects of the DirectX 8.1 SDK's `BaseClasses`,
  compiled with VC6 SP5 without the Processor Pack. `ls3df/BaseClasses/` compiles to the same bytes.
- `vorbisfile`'s thunks (`0x1008d3e0`).

## Open questions

- **Files with only a marker** (`clipping2.cpp`, `Load3ds.cpp`, `Load6ds.cpp`): each has its own identity matrix, so
  each is an object of its own whose functions were dropped by `/OPT:REF` or compiled out on PC. No binary names
  them, and PS2 has no file in their places.
- **The 61st object.** The 58 identity matrices are 48 objects of the I3D group and 5 each of IGraph and ISound.
  With `clipper2.cpp` and `RadixSort.cpp` that makes 60 of the Rich header's 61 C++ objects, so one more object has no
  copy: a file without the engine header, or one more compiled with another file's precompiled header.
- **`.bss` between `IGraph.cpp` and `IShow.cpp`.** Nothing references the 0x108 bytes from `0x101c5ad9` to
  `0x101c5be8`. They are either zero-initialized globals at the end of `IGraph.cpp` or `IShow.cpp` globals whose names
  hash to the lowest buckets.
- **The marker in the long files** comes a few functions earlier in ours than in the original, so the original's
  headers had more inline code before the zero vector than ours.
