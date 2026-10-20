# LS3DF.dll source files

A map of LS3DF.dll's code by source file, for planning the files of `ls3df/`. It comes from the original's
static initializer table and is checked against the PS2 build's file names. It is a working map: where a boundary or
a name is uncertain, the table says so.

## Method

- **The initializer table.** `.CRT$XCU` (`0x100a6004–0x100a61bc`) lists every dynamic initializer (`$E`) in link
  order. That is the order of the object files, and within a file the order the globals are defined.
- **The header's zero vector.** An engine header defines a zero `S_vector` with internal linkage, so every file that
  includes it gets its own copy and its own `$E`: three zero stores to consecutive dwords, then `ret`.
  - In the table it comes first in its file's group, because the header is included before the file defines its own
    globals.
  - Its code comes last in the file's code, after all the functions, followed only by out-of-line inline and
    template functions, or in the middle of a long file. That is VC6's automatic precompiled headers (`/YX`), see
    `tools/autopch.py`. A file without one has it first, before its functions (`I3d_area.cpp`).
- **Reading the table.** Each such `$E` closes a file's code. A zero-vector entry whose code lies past the current
  file's marker starts the next file, and so does any other entry past it (a file without the header).
  - Files that define no globals and don't include the header don't appear in the table. They have to be found from
    their code.
  - `Globals.cpp`'s own `ZeroVector` has the same shape, but its code sits before the file's marker.
- **The header's identity matrix.** Every object that includes the engine header also has its own copy of
  `IdentityMatrixData` (`I3d_math.h`, 0x40 bytes, never referenced) at the start of its `.rdata`, except one compiled
  with another file's precompiled header (`clipper2.cpp`). The original has 58. They mark the objects in `.rdata` as
  the zero vectors do in `.text` and `.bss`, and the first reference from code into the data after a copy names its
  file. Ours has them through an inline function nobody calls, as the original must (see `I3d_math.h`).
- **Names.** Strings in each range name the classes (`!! Out of memory  I3D_frame::SetName()`), and two files name
  themselves through `__FILE__`. The PS2 build (`SLUS_206.71`) names its files through `__sinit_<file>.cpp`. Its
  engine files are linked in folder groups (I3D, IGraph, ISound), alphabetically within each and case-insensitively, as
  a VC6 project lists its files, and so are the PC's: its exports run `I3D_frame` (`0x1001c7d0`), `I3d_math`
  (`0x1002cda0`), `I3D_visual` (`0x1005a510`), `Globals` (`0x1006b6d0`), and the source names in its `.data` strings
  `clipper.cpp`, `IBManager.cpp`, `load.cpp`, `load5ds.cpp`, `VBManager.cpp`, then `Globals.cpp`'s and `IGraph.cpp`'s.

PS2 has 51 engine files: the 49 from `I3d_animation.cpp` to `IS_source.cpp`, and `VBManager_PS2.cpp` and
`IBManager_PS2.cpp`, the PS2's counterparts of `VBManager.cpp` and `IBManager.cpp`. The PC's Rich header lists 61 C++
objects and one C object of the engine's own.

## The map

The *range* runs from the end of the previous file's marker to the end of this one's. Trailing out-of-line inline
functions fall into the next range: `ISND_sound::GetSource` at `0x1007b680` belongs to `IS_3Dsound.cpp`, and
`ISND_source::GetFileName` at `0x10059370` to `I3d_sound.cpp`. The *other `$E`s* are the file's own globals.

| Range | Marker `$E` | Other `$E`s | File | Evidence |
| --- | --- | --- | --- | --- |
| `10001000–100013e0` | `100013c0` | `10001000` (empty) | `clipper.cpp` (PC only) | the projectors' and shadows' clipper: its parameters and arrays, `ReserveClipArrays()` citing `clipper.cpp`; the first object, whose default library requests (`libcpmt` first, no `uuid`) start the link's; written |
| `100013e0–10003b40` | none | none | `clipper2.cpp`? (PC only) | the faces of an orthogonal and of a perspective volume, clipped; no strings, and no `$E` for the zero vector: compiled with `clipper.cpp`'s precompiled header (`/Yu`). The name, which sorts between its neighbours', is a guess; written |
| `10003b40–10004870` | `10004850` | | `clipping.cpp` (PC only) | the four entry points (a mesh's vertices into clip space) and `ClipFacegroups()`, whose overflow message cites `clipping.cpp`, the others `clipper.cpp`; the message comes first in the data, from a helper compiled before them that the linker drops; written |
| `10004870–10004890` | `10004870` | | ? | no code, only the marker; a file of its own, with its own identity matrix (`1009b360`) and zero vector (`100af7d8`) and nothing else left; the name sorts between `clipping.cpp` and `I3d_animation.cpp` |
| `10004890–10005180` | `10005160` | | `I3d_animation.cpp` | `I3D_animation_set` in PS2's order from its constructor (`10004890`), with the PC's `GetAnimLink` after `GetEndTime` and the rotation's, position's and scale's key functions before the notes'; written |
| `10005180–10005db0` | `10005180` | | `I3d_area.cpp` | compiled without a precompiled header, so the marker comes first: `I3D_area` in PS2's order from its constructor (`100051a0`), the in-class functions its vtable needs right after it, the PC's `SaveImage` and `LoadImage`, then `C_list::remove` (`10005d20`); written |
| `10005db0–1000b0c0` | `10008390` | | `I3d_billboard.cpp` | `I3D_billboard` and `I3D_lensflare` in PS2's order (`renderBillboard()`, `I3D_lensflare::LoadImage()`); the marker before the billboard's `m_RenderFGroup`, then the copies of the render state helpers and of `C_VBuffer`'s `Lock` and `Unlock` the linker kept; written |
| `1000b0c0–1000de70` | `1000de50` | | `I3d_camera.cpp` | `I3D_camera` in PS2's order, from its constructor (`1000b0c0`) to `UpdateV`; after it the copies of `S_vector`'s constructor, `operator-`, `Dot` and `GetNormal` the linker kept, then the marker; written |
| `1000de70–10015530` | `1000dea0` | `1000de70`, `1000de80`, `1000de90`, `10011d20`–`10011d50` (empty) | `i3d_collision.cpp` | the collision tests in PS2's order: the empty `$E`s of three globals, the marker (due before the first function, as PS2 defines the static helpers first), `TestColGeo` (`1000dec0`) with `testSubtree`, `testVisual` and `DefaultTexelCallback` after it, `TestColGeo1`, `TestColHierarchy` and its static's atexit thunk (`10011d10`), the sphere tests' globals' empty `$E`s, then `TestColSphere` to `SphereToSphereD`; the copies of `S_vector`'s `operator*` and `operator+`, `Square`, `SetNormalized`, `I3D_bbox::Absorb` and the file's `SphereIntersect` after their first callers; its identity matrix is at `1009b778`; written |
| `10015530–1001a1e0` | `10016fd0` | `10015530`, `10015580`, `100155d0`, `100188a0` | `I3d_driver.cpp` | one file with its marker after `DrawLines`, as `I3d_math.cpp`: `I3D_driver` in PS2's order with the PC's icons and shadow rasterizer after `GetState`, `VolumeBall`'s `$E` before `DrawSphere`, then two `C_list` reserves; written |
| `1001a1e0–1001a6e0` | `1001a6c0` | | `I3d_dummy.cpp` | `I3D_dummy` in PS2's order (the constructor `CreateFrame` calls, `Duplicate`, `Normalize`, the load of its box), with the PC's `UpdateV` after `Duplicate`; written |
| `1001a6e0–1001d2f0` | `1001c280` | `1001c780` | `I3d_frame.cpp` | one file with its marker after `EnumSubtree`, as `I3d_math.cpp`: `I3D_frame` from the constructor (`1001a7f0`, after the dummy file's copy of `UpdateMatrix`) to `LoadImage`, in PS2's order; `UpdateTargetList`'s `$E` after `SetProperty`; written |
| `1001d2f0–1001ed30` | `1001e840` | | `I3d_interpolator.cpp` | one file with its marker after `I3D_IPolSet::SetPose(const char *, I3D_pose *)`, as `I3d_math.cpp`: `I3D_interpolator`, then `I3D_IPolSet`, each in PS2's order; written |
| `1001ed30–1001ef60` | `1001ef40` | | `I3d_joint.cpp` | `I3D_joint` in PS2's order, from its constructor (`1001ed30`); written |
| `1001ef60–100201d0` | `100201b0` | `1001ef60` | `I3d_landscape.cpp`? (PC only) | Bryan Turner's ROAM demo (`LandTex0.tga`, `Creating landscape patch`): the view position's empty `$E`, `I3D_landpatch` (the demo's `Patch`) from its constructor (`1001ef70`), then `I3D_landscape` (its `Landscape`) from its constructor (`1001fbb0`), each in the demo's order; after the marker the vtable's `GetHeight` (`100201d0`). The name, which sorts between its neighbours', is a guess; written |
| `100201d0–10021570` | `10021550` | | `I3d_light.cpp` | after the landscape's out-of-line accessor of its last member (`100201d0`), `I3D_light` in PS2's order from its constructor (`100201f0`) without PS2's specular, ambient and attenuation setters; written |
| `10021570–1002ade0` | `10026900` | | `I3d_lit_object.cpp` | one file with its marker in the middle, as `I3d_math.cpp`, and one object (its identity matrix `1009bcf8`): `I3D_lit_object::Construct()` before the marker, `I3D_lit_object::Load()` and the lightmaps after it, then `C_list::push_back` (`1002ad50`); PS2 has one file |
| `1002ade0–1002cda0` | `1002c8a0` | | `I3d_material.cpp` | `I3D_material` from its constructor (`1002ade0`), whose vtable (`1009bea0`) follows the file's identity matrix (`1009be60`), then `I3D_anim_material` and the two `LoadImage` functions, as on PS2; after the marker `std::vector<ITexture *>`'s functions and the in-class `IsCkey` and `IsAnimated` (`1002cd80`, `1002cd90`); written |
| `1002cda0–10030560` | `1002ea90` | | `I3d_math.cpp` | written. The math from `RandFloat` (`1002cda0`) to `LineLineIntersect`, one file as on PS2; its marker sits between `S_vector::RotateByMatrix` and `RotateByNormMatrix`, not at the end |
| `10030560–10031820` | `10031800` | | `I3d_mesh.cpp` | `I3D_face_group`, `I3D_mesh_level`, `I3D_mesh_object`, each in PS2's order with the PC's additions; written |
| `10031820–100333d0` | `100333b0` | `100324b0` | `I3d_mirror.cpp` | no strings, but PS2's `I3D_mirror::SetColor`, `SetViewBox` and `LoadImage` among the PC's rendering functions, from the constructor (`10031820`, which stores the vtables after the file's identity matrix `1009c050`); the other `$E` is the list of the visuals it reflects; written |
| `100333d0–10034350` | `10034330` | `10033b50`, `10033ba0` | `I3d_model.cpp` | `I3D_model` in PS2's order from its constructor (`10033410`; before it the mirrors' file's out-of-line `SetClipping`); the two `$E`s are PS2's `DuplTargetList` and `DuplShadowList`, defined before `duplicateChildren`; written |
| `10034350–100367b0` | `10036520` | | `I3d_morph.cpp` | after the folded `C_list` destructor (`10034350`), `morphBase` from its constructor (`10034370`, PS2's `BogusConstructor`), then `I3D_morph` from its constructor (`10034e30`), in PS2's order with the PC's second `UpdateMorphLod`; after the marker, which follows `m_AddFGroups`, the destructor of what `morphBase` releases (`10036540`), `MorphDef`'s constructor and `Release`, `morph_region`'s functions and the morph's `LoadImage` (`10036750`); written |
| `100367b0–1003a430` | `10039260` | | `I3d_object.cpp` | `I3D_object` close to PS2's order (`I3D_object::DetachMesh()`) with the PC's `lod` class (`lod::Create()`) after `Duplicate`; the marker before `m_RenderFGroup` (`renderObject()`) and `LoadImage`; written |
| `1003a430–1003c070` | `1003c050` | | `I3d_occluder.cpp` | one file with its marker after `OCCL_UpdateOcclLists`, as `I3d_math.cpp`: `I3D_occluder` in PS2's order from its constructor (`1003a430`), whose vtable (`1009c420`) follows the file's identity matrix (`1009c3e0`), then `UOL_EnumProc` and `OCCL_UpdateOcclLists`; after the marker `ProcessOccluders` with the static `CullOcclsEachOther` and `POIS_EnumProc`, `OCCL_PrepareSector2`, `LoadImage` (`I3D_occluder::LoadImage()`) and `XSBTree<TRISIDE2>`'s destructor and `Add_r`, up to `InitParticle` (`1003cbb0`); written |
| `1003c070–10041800` | `1003eaa0` | | `I3d_particle.cpp` | from `InitParticle` (`1003cbb0`), `I3D_emitor`; after the marker `renderObject()` and the particles' constructors (`1003fa20`, `100406c0`), which store the vtables after the file's identity matrix (`1009c478`), then `C_list::push_back` (`10041700`) and small in-class functions, as at a file's end |
| `10041800–10045930` | `100437d0` | `10041de0` (empty) | `I3d_portal.cpp` | one file with its marker after `CTP_EnumProc`, as `I3d_math.cpp`: `I3D_portal` in PS2's order from its constructor (`10041800`), whose vtable (`1009c738`) is the first data after the file's identity matrix (`1009c6f8`), with the PC's `SetOn` and `IsOn` after `AddToSceneRing`; after `UpdatePortal` the empty `$E` of `CTP_VFnormals`, then the walk through the portals in PS2's order (`CTP_EnumProcFirst`, `CTP_EnumProc`, `CullThruPortal`, `ParsePortal`, `ParsePortalsInSector`, `UpdatePortalInterconnection`, `ProcessPortals`) and `LoadImage` (`100458f0`); written |
| `10045930–10045e30` | `10045e10` | | `I3d_pose.cpp` | `I3D_pose`, then `I3D_pose_set`, each from its constructor; its own identity matrix (`1009c750`) before their vtables; written |
| `10045e30–10048060` | `10048040` | `10046730` | `I3d_projector.cpp` | one file with its marker after `ProcessRecList` (`I3D_projector::ProcessRecList()`), as `I3d_math.cpp`: `I3D_projector` in PS2's order from its constructor (`10045e30`), whose vtables (`1009c808`) follow the file's identity matrix (`1009c7c8`), then the PC's receivers and their queue (the other `$E`); after the marker `ProcessShadow`, `MakeProjection` with its search, `m_RenderFGroup` and `LoadImage`, up to the scene's first `$E` (`10048a00`); the receivers' array (`101b4630`, the last of its `.bss`) is the shadows' visual's too; written |
| `10048060–1004e9b0` | `1004a860` | `10048a00`, `10048a50`, `10048aa0` (empty) | `I3d_scene.cpp` | one file with its marker after `SetBackdropRange`, as `I3d_math.cpp`: `I3D_scene` in PS2's order from its constructor (`10048ab0`), the PC's shadow and mirror lists after `Close`, the weather system and the fog states; after the last function the copies of the weather's `C_list` functions, `S_vector`'s empty constructor and the class's in-class functions; written |
| `1004e9b0–100508b0` | `10050890` | | `I3d_sector.cpp` | from the sound properties' `SetupDefaults` (`1004e9b0`) and `MakeBlend`, `I3D_sector` in PS2's order without its `SetBBox`, `Clear` and `CheckPointDepth`; after the marker the vtable's `SetGroupBits` (`100508b0`, an in-class function) and `C_list::find` (`100508d0`); written |
| `100508b0–10054240` | `100529a0` | `10050900`, `10050d30`, `10052f30` | `I3d_shadow.cpp` | one file with its marker after `UpdateLight`, as `I3d_math.cpp`: `I3D_shadow` in PS2's order from its constructor (`10050950`) with the PC's `UpdateVertices` and `UpdateLight`, then `RenderTexture`, `Prepare`, `ShadowDef`'s destructor and the shadows' visual (`I3D_VISUAL_SHADOW`, its constructor `10052ce0`) with its receivers (`10053280`, the projectors' code: `I3D_shadow::ProcessShadow()`, `I3D_projector::ProcessShadow()`). One object, though the visual's code has no identity matrix or zero vector of its own: the visuals' queue (`101b4f3c`, its `$E` `10052f30`) lies between the shadow file's zero vector and `globalShadowList` in `.bss`, in VC6's hash order, and the visual's constructor copies the file's zero vector; written |
| `10054240–10056e80` | `10056840` | | `I3d_singlemesh.cpp` | `I3D_singlemesh` in PS2's order from its constructor (`10054240`), whose vtables (`1009cc88`) follow the file's identity matrix (`1009cc48`), with the PC's lods (`SMeshLod`) and their vertex helpers; after the marker, which follows `m_AddFGroups`, `LoadImage`, the lod's scalar deleting destructor and destructor, then the out-of-line `I3D_joint::GetJointMatrix` (`10056ba0`) and `S_vector::operator*=` (`10056e60`); written |
| `10056e80–10057650` | `10057630` | | `I3d_singlemorph.cpp` | `I3D_singlemorph` in PS2's order from its constructor (`10056e80`), whose vtables (`1009cd80`) follow the file's identity matrix (`1009cd40`), with the PC's `UpdateLodVertices` (`10057590`) where PS2 has `GetVertexPos` and `m_AddFGroups` (the PC's are the single mesh's); its `UpdateBoundProc` jumps to the single mesh's; the marker last; written |
| `10057650–10059250` | `10059230` | | `I3d_sound.cpp` | `I3D_sound` in PS2's order from its constructor (`10057650`), without `m_Pause` (inlined everywhere); after the marker `Duplicate` (`10059250`) and `m_SetState` (`10059360`), its `LoadImage` folded into `I3D_area`'s, then the out-of-line `ISND_source::GetFileName` and `I3D_sound::SetOn(bool)` up to `10059440`; written |
| `10059250–1005a140` | `1005a120` | `10059440` | `I3d_target.cpp` | from `globalTargetList`'s `$E` (`10059440`) `I3D_target` in PS2's order; written |
| `1005a140–1005a200` | `1005a1e0` | | `I3d_user.cpp` | `I3D_user` in PS2's order; written |
| `1005a200–1005a7a0` | `1005a780` | | `I3d_visual.cpp` | `I3D_visual::SetSpace()`, exported `UpdateWBoundProc`; `I3D_visual` in PS2's order, then its second base `I3D_void_interface`'s functions; written |
| `1005a7a0–1005aad0` | `1005aab0` | | `I3d_volume.cpp` | `I3D_volume` in PS2's order; written |
| `1005aad0–1005c760` | `1005c740` | `1005aad0` | `IBManager.cpp` | `__FILE__` `C:\MyProjects\engine8\I3D\IBManager.cpp`; written: `C_IBuffer`, `C_IBManager` and the multisets' out-of-line functions |
| `1005c760–1005e3a0` | `1005e380` | `1005d010` | `lightmap.cpp`? | no strings; up to `1005d010` the rest of `IBManager.cpp`'s multiset functions |
| `1005e3a0–10060c20` | `10060c00` | | `Load.cpp` | `LightMap::m_CreateLM()`, `load.cpp` messages, `.4ds`, `.5ds`, `.6ds` |
| `10060c20–10060c40` | `10060c20` | | ? | no code, only the marker; a file of its own (identity matrix `1009d298`). The `.bss` around this file's and the next one's vectors (`101c3be4`–`101c3c04`, among it the load file's handle at `101c3bfc`) is read by `load4ds.cpp` and the `LoadImage` functions, not by `Load.cpp` |
| `10060c40–10060c60` | `10060c40` | | ? | no code, only the marker; a file of its own (identity matrix `1009d2d8`) |
| `10060c60–10061990` | `10061970` | `10060c60` | `load4ds.cpp` | the 4ds loader's `LoadImage()` and `LoadMeshImage()` (PS2 names, PS2 order), two more functions and `LoadAnimation5ds` (PS2's `Load5ds.cpp`); the table has `LoadTargetList`'s `$E` before the zero vector's, so either the file defines it above its include of the header or it belongs to a file of its own without code (the count of objects, see the open questions, leaves no room for one); `LoadTargetList` and `LoadMeshImage` written |
| `10061990–100619b0` | `10061990` | | ? | no code, only the marker; a file of its own (identity matrix `1009d358`); PS2's `Load5ds.cpp` sorts here |
| `100619b0–10061cd0` | none | none | `RadixSort.cpp` | the radix sort (`RadixSort32` with its passes inlined), which PS2 has at the start of `render.cpp`'s code: a file of its own without the engine header, neither marker nor identity matrix, its counts the first `.bss` of an object before `render.cpp`'s (`101c3c30`). The name is H&D Deluxe's; written |
| `10061cd0–10065a10` | `100658d0` | `10061cd0`, `10061d20`, `10061d70`, `10061dd0` | `render.cpp` | `I3D_driver::Render()` and what it calls, the sounds' areas and sectors among it; after the marker `I3D_driver::BuildAllVBuffers` (`100658f0`), the out-of-line constructors its `$E`s call (`100659a0`, `100659d0`) and a z-bias setter only it calls (`100659f0`); written |
| `10065a10–10068880` | `10067d00` | `10065a10`, `10065a30` | `T&l.cpp` (`T_l.cpp`) | one file with its marker after `VertexLightingProcess`, as `I3d_math.cpp`: the lighting in PS2's order, the PC's texel callback, then the vertex helpers the objects and billboards render with (`EnvProcess`, the transforms, the fog, the vertex byte swap) and the copy of `I3D_dummy::GetInvMatrix` they call; written |
| `10068880–10069e00` | `10069de0` | `10068880` | `VBManager.cpp` | `__FILE__` `C:\MyProjects\engine8\I3D\VBManager.cpp`; written: `C_VBuffer`, `C_VBManager`, the same code as `IBManager.cpp`'s |
| `10069e00–1006ad40` | none | none | `VBManager.cpp` | the rest of its multisets' out-of-line functions, after the marker; written |
| `1006ad40–1006b430` | none | none | `cpuid.c` (C) | the C object: Microsoft's CPUID sample (`IsCPUID`, `map_mname`, `_cpuid`) and `GetCPUSpeed`; written |
| `1006b430–1006bd40` | `1006bd20` | `1006b430`, `1006bb80`, `1006bca0`, `1006bcc0` | `Globals.cpp` | written |
| `1006bd40–100733a0` | `1006deb0` | `1006bd40`–`1006bf70` (8) | `IGraph.cpp` | one file with its marker in the middle, as `I3d_math.cpp`: the `C_str` functions after `Init` are COMDATs linked into it, and the first object's copy is the one kept; written |
| `100733a0–10075890` | `10075800` | | `IShow.cpp` | `IShow` in PS2's order, the DirectX SDK's texture renderer (`0x10074f40–0x10075770`) and its adjustor thunks, and, after the marker, the class's inline functions and the base classes' `IUnknown` members; compiled without `/Gr`; written |
| `10075890–10075c10` | `10075bf0` | `10075890` | `IShowStream.cpp`? | PC only: `IShow`'s music streams through the sound driver's table of functions; the name, which sorts between its neighbours', is a guess; written |
| `10075c10–1007b053` | `10078170` | `1007a570`, `1007a5c0` | `ITexture.cpp` | one file with its marker after `Create`, as `IGraph.cpp`: `ITexture` in PS2's order, then the PC's texture manager, whose lists are the two `$E`s; its strings run straight into `IS_3Dlistener.cpp`'s; written |
| `1007b060–1007b370` | `1007b350` | | `IS_3Dlistener.cpp` | written |
| `1007b370–1007b680` | `1007b660` | | `IS_3Dsound.cpp` | written |
| `1007b690–1007c2a0` | `1007c280` | | `IS_driver.cpp` | written |
| `1007c2a0–1007d430` | `1007d410` | `1007c2a0`, `1007c2f0` | `IS_sound.cpp` | written |
| `1007d430–1007d9d0` | `1007d9b0` | | `IS_source.cpp` | written |

After `IS_source.cpp` and before the CRT (`0x1008d404` on) come library objects:
- `DXGetErrorString8` from the DirectX SDK's `dxerr8.lib`, from `0x1007d9d0`.
- The `DirectInput8Create` thunk at `0x10080364`.
- S3's S3TC library at `0x100813e0–0x10086a09`, which `ITexture.cpp`'s `CompressTexture` calls (`S3TCencode` at
  `0x100813e0`; its arguments are two `DDSURFACEDESC`s, the encode type and S3's color weights 0.3086, 0.6094, 0.082).
  Two C objects, `s3tc.obj` (the API) and `S3_quant.obj` (the quantizer), compiled by S3 with VC6 SP3 (the Rich
  header's `Utc12_C` 8447 ×2) and linked prebuilt; the build links the original's, `ls3df/S3TC/s3tc.lib`.
- The `DirectSoundCreate8` thunk (`DSOUND` ordinal 11) at `0x10086a10`.
- The DirectShow base classes, `0x10086a20–0x1008d3e0`: seven objects of the DirectX 8.1 SDK's `BaseClasses`
  (`renbase`, `amfilter`, `ctlutil`, `mtype`, `combase`, `wxutil`, `wxlist`), compiled with VC6 SP5 without the
  Processor Pack (the Rich header's `Utc12_CPP` 8966 ×7). Every function in the range compiles to the same bytes from
  `ls3df/BaseClasses/`, and linked through `IShow.cpp`'s texture renderer they come in the original's order. Only
  where the linker folded identical functions (empty ones, `E_NOTIMPL`s, `CBaseInputPin`'s `IUnknown` members) the copy
  it kept is elsewhere, which depends on the engine files not written yet.

## Open questions

- **Files with only a marker.** The ranges that hold nothing but a marker (`10004870`, `10060c20`, `10060c40`,
  `10061990`) are files of their own: each has its own identity matrix besides its zero vector, which a second zero
  vector of the file before wouldn't bring. Nothing else of them is left, not even a string or a constant: those
  would be kept where the first object that has them is (the linker keeps the first copy of a COMDAT), as
  `clipping.cpp`'s helper keeps its message. So their functions were dropped by `/OPT:REF` or compiled out on PC.
- **The count.** The original's 58 identity matrices are 48 objects of the I3D group and 5 each of IGraph and ISound.
  The object without one: `clipper2.cpp`, without a marker too (a file using another's precompiled header); the
  shadows' visual is code of `I3d_shadow.cpp`'s object. The portal and pose files have one each (`1009c6f8`,
  `1009c750`), and so have the single mesh and single morph (`1009cc48`, `1009cd40`). With `RadixSort.cpp`, a file
  without the engine header, that makes 60 of the Rich header's 61 C++ objects, so one more object has no copy: a
  file without the engine header, or one more compiled with another's precompiled header. `LoadTargetList`'s `$E` is
  a candidate.
- **Files without a marker.** Their boundaries come from their code only: the billboard/camera pair and the ranges
  marked `none`. A file compiled with another's precompiled header (`/Yu`)
  has no marker and the header's statics aren't in its `.bss` (`clipper2.cpp`), so these may be such files.
- **`.bss` between `IGraph.cpp` and `IShow.cpp`.** `IGraph.cpp`'s ends at `0x101c5ad9`, the first of
  `IShow.cpp`'s (`m_CreateCTextureRenderer`'s statics) is at `0x101c5be8`. Nothing references the 0x108 bytes in
  between, so they are either zero-initialized globals at the end of `IGraph.cpp` or `IShow.cpp` globals whose
  names hash to the lowest buckets.
- **The marker's place in the long files.** `/YX` puts it where a file's own source outweighs the inline code before
  the vector in the precompiled headers (`tools/autopch.py`). In our long files it comes a few functions earlier than
  in the original, so the original's headers had more inline functions before the vector than ours.
