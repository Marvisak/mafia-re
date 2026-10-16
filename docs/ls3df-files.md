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
    `tools/autopch.py`.
- **Reading the table.** Each such `$E` closes a file's code. A zero-vector entry whose code lies past the current
  file's marker starts the next file, and so does any other entry past it (a file without the header).
  - Files that define no globals and don't include the header don't appear in the table. They have to be found from
    their code.
  - `Globals.cpp`'s own `ZeroVector` has the same shape, but its code sits before the file's marker.
- **Names.** Strings in each range name the classes (`!! Out of memory  I3D_frame::SetName()`), and two files name
  themselves through `__FILE__`. The PS2 build (`SLUS_206.71`) names its files through `__sinit_<file>.cpp`. Its
  engine files are linked in folder groups (I3D, IGraph, ISound), alphabetically within each, and so are the PC's.

PS2 has 51 engine files: the 49 from `I3d_animation.cpp` to `IS_source.cpp`, and `VBManager_PS2.cpp` and
`IBManager_PS2.cpp`, the PS2's counterparts of `VBManager.cpp` and `IBManager.cpp`. The PC's Rich header lists 61 C++
objects and one C object of the engine's own.

## The map

The *range* runs from the end of the previous file's marker to the end of this one's. Trailing out-of-line inline
functions fall into the next range: `ISND_sound::GetSource` at `0x1007b680` belongs to `IS_3Dsound.cpp`, and
`ISND_source::GetFileName` at `0x10059370` to `I3d_sound.cpp`. The *other `$E`s* are the file's own globals.

| Range | Marker `$E` | Other `$E`s | File | Evidence |
| --- | --- | --- | --- | --- |
| `10001000–100013e0` | `100013c0` | `10001000` (empty) | clipping (PC only) | `ReserveClipArrays()` |
| `100013e0–10004870` | `10004850` | | clipping (PC only) | messages cite `clipping.cpp` and `clipper.cpp` (`ClipFacegroups()`) |
| `10004870–10004890` | `10004870` | | ? | no code, only the marker |
| `10004890–10005180` | `10005160` | | `I3d_animation.cpp` | `I3D_animation_set` |
| `10005180–100051a0` | `10005180` | | ? | no code, only the marker |
| `100051a0–100083b0` | `10008390` | | `I3d_area.cpp` | `I3D_area::Build()` |
| `100083b0–1000de70` | `1000de50` | | `I3d_billboard.cpp`, `I3d_camera.cpp` | `renderBillboard()`, `I3D_lensflare` (in PS2's `I3d_billboard.cpp`) from `100083b0`, `I3D_camera` from about `1000b600`. One of the two has no marker. |
| `1000de70–1000dec0` | `1000dea0` | `1000de70`, `1000de80`, `1000de90` (empty) | ? | no functions, three globals with empty constructors |
| `1000dec0–` about `10015530` | none | `10011d20`–`10011d50` (empty) | `i3d_collision.cpp` | collision tests; PS2 has it between the camera and the driver |
| `10015530–1001a1e0` | `10016fd0` | `10015530`, `10015580`, `100155d0`, `100188a0` | `I3d_driver.cpp` | one file with its marker after `DrawLines`, as `I3d_math.cpp`: `I3D_driver` in PS2's order with the PC's icons and shadow rasterizer after `GetState`, `VolumeBall`'s `$E` before `DrawSphere`, then two `C_list` reserves; written |
| `1001a1e0–1001a6e0` | `1001a6c0` | | `I3d_dummy.cpp` | `I3D_dummy` in PS2's order (the constructor `CreateFrame` calls, `Duplicate`, `Normalize`, the load of its box), with the PC's `UpdateV` after `Duplicate`; written |
| `1001a6e0–1001d2f0` | `1001c280` | `1001c780` | `I3d_frame.cpp` | one file with its marker after `EnumSubtree`, as `I3d_math.cpp`: `I3D_frame` from the constructor (`1001a7f0`, after the dummy file's copy of `UpdateMatrix`) to `LoadImage`, in PS2's order; `UpdateTargetList`'s `$E` after `SetProperty`; written |
| `1001d2f0–1001e860` | `1001e840` | | ? | `I3d_interpolator.cpp` and `I3d_joint.cpp` are due here |
| `1001e860–1001ef60` | `1001ef40` | | ? | no strings |
| `1001ef60–100201d0` | `100201b0` | `1001ef60` | landscape (PC only) | `LandTex0.tga`, `Creating landscape patch` |
| `100201d0–10021570` | `10021550` | | `I3d_light.cpp`? | no strings |
| `10021570–10026920` | `10026900` | | `I3d_lit_object.cpp` | `I3D_lit_object::Construct()` |
| `10026920–1002c8c0` | `1002c8a0` | | `I3d_lit_object.cpp` (second file) | `I3D_lit_object::Load()`, lightmaps |
| `1002c8c0–10030560` | `1002ea90` | | `I3d_math.cpp` | written. The math from `RandFloat` (`1002cda0`) to `LineLineIntersect`, one file as on PS2; its marker sits between `S_vector::RotateByMatrix` and `RotateByNormMatrix`, not at the end |
| `10030560–10031820` | `10031800` | | `I3d_mesh.cpp` | `I3D_face_group`, `I3D_mesh_level`, `I3D_mesh_object` |
| `10031820–100333d0` | `100333b0` | `100324b0` | `I3d_mirror.cpp`? | no strings |
| `100333d0–10034350` | `10034330` | `10033b50`, `10033ba0` | `I3d_model.cpp`? | two lists; PS2's `__sinit_I3d_model.cpp` constructs more than the header's objects too |
| `10034350–10036540` | `10036520` | | `I3d_morph.cpp` | `morphBase`, `I3D_morph` |
| `10036540–10039280` | `10039260` | | `I3d_object.cpp` | `I3D_object::DetachMesh()`, `lod::Create()` |
| `10039280–1003c070` | `1003c050` | | `I3d_occluder.cpp` | `i3d_occluder::Build`, `UpdateShadow` |
| `1003c070–1003eac0` | `1003eaa0` | | `I3d_particle.cpp` | `I3D_emitor` |
| `1003eac0–100437f0` | `100437d0` | `10041de0` (empty) | ? | `renderObject()` |
| `100437f0–10045e30` | `10045e10` | | `I3d_portal.cpp`, `I3d_pose.cpp` | portal parsing, `I3D_pose`, `I3D_pose_set`; one of the two has no marker |
| `10045e30–10048060` | `10048040` | `10046730` | `I3d_projector.cpp` | `I3D_projector::ProcessRecList()` |
| `10048060–1004a880` | `1004a860` | `10048a00`, `10048a50`, `10048aa0` (empty) | `I3d_scene.cpp` | sector names, `Snow.tga`, `I3D_scene::Delete...` |
| `1004a880–100508b0` | `10050890` | | `I3d_sector.cpp` | `I3D_sector::MakeReady()` |
| `100508b0–100529c0` | `100529a0` | `10050900`, `10050d30` | `I3d_shadow.cpp` | `I3D_shadow` constructor, `ShadowDef` |
| `100529c0–` | none | `10052f30` | ? | |
| `–10056860` | `10056840` | | ? (PC only?) | `I3D_projector::ProcessShadow()`, `I3D_shadow::ProcessShadow()` |
| `10056860–10057650` | `10057630` | | `I3d_singlemesh.cpp`, `I3d_singlemorph.cpp` | one of the two has no marker |
| `10057650–10059250` | `10059230` | | `I3d_sound.cpp` | `I3D_sound::Open()` |
| `10059250–1005a140` | `1005a120` | `10059440` | `I3d_target.cpp` | PS2 order |
| `1005a140–1005a200` | `1005a1e0` | | `I3d_user.cpp` | PS2 order |
| `1005a200–1005a7a0` | `1005a780` | | `I3d_visual.cpp` | `I3D_visual::SetSpace()`, exported `UpdateWBoundProc` |
| `1005a7a0–1005aad0` | `1005aab0` | | `I3d_volume.cpp` | PS2 order |
| `1005aad0–1005c760` | `1005c740` | `1005aad0` | `IBManager.cpp` | `__FILE__` `C:\MyProjects\engine8\I3D\IBManager.cpp`; written: `C_IBuffer`, `C_IBManager` and the multisets' out-of-line functions |
| `1005c760–1005e3a0` | `1005e380` | `1005d010` | `lightmap.cpp`? | no strings; up to `1005d010` the rest of `IBManager.cpp`'s multiset functions |
| `1005e3a0–10060c20` | `10060c00` | | `Load.cpp` | `LightMap::m_CreateLM()`, `load.cpp` messages, `.4ds`, `.5ds`, `.6ds` |
| `10060c20–10060c40` | `10060c20` | | ? | no code, only the marker |
| `10060c40–10060c60` | `10060c40` | | ? | no code, only the marker |
| `10060c60–` | none | `10060c60` | ? | |
| `–10061990` | `10061970` | | `load4ds.cpp`? | the 4ds loader's `LoadImage()` messages |
| `10061990–100619b0` | `10061990` | | ? | no code, only the marker |
| `100619b0–100658f0` | `100658d0` | `10061cd0`, `10061d20`, `10061d70`, `10061dd0` | `render.cpp` | `I3D_driver::Render()` |
| `100658f0–10067d20` | `10067d00` | `10065a10`, `10065a30` | `T&l.cpp` | PS2 order |
| `10067d20–10068880` | none | none | ? | six `__fastcall` engine helpers taking an `S_matrix` and an `I3D_frame`, called from all over I3D: the end of `T&l.cpp` or a file of their own |
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
- The `DirectSoundCreate8` thunk (`DSOUND` ordinal 11) at `0x10086a10`.
- The DirectShow base classes, `0x10086a20–0x1008d3e0`: seven objects of the DirectX 8.1 SDK's `BaseClasses`
  (`renbase`, `amfilter`, `ctlutil`, `mtype`, `combase`, `wxutil`, `wxlist`), compiled with VC6 SP5 without the
  Processor Pack (the Rich header's `Utc12_CPP` 8966 ×7). Every function in the range compiles to the same bytes from
  `ls3df/BaseClasses/`, and linked through `IShow.cpp`'s texture renderer they come in the original's order. Only
  where the linker folded identical functions (empty ones, `E_NOTIMPL`s, `CBaseInputPin`'s `IUnknown` members) the copy
  it kept is elsewhere, which depends on the engine files not written yet.

## Open questions

- **Files with only a marker.** Several ranges hold nothing but a marker (`10004870`, `10005180`, `10060c20`,
  `10060c40`, `10061990`). Each is either a file whose code is compiled out on PC, or the second zero vector of the
  file before it.
  - Counted as separate files, the table has more files than the Rich header's 61 C++ objects. So some of them
    probably belong to the file before.
  - PS2 constructs two header objects per file, a zero vector and an identity quaternion. PC constructs only the
    vector.
- **Files without a marker.** Their boundaries come from their code only: the billboard/camera, portal/pose and
  singlemesh/singlemorph pairs and the ranges marked `none`.
- **`.bss` between `IGraph.cpp` and `IShow.cpp`.** `IGraph.cpp`'s ends at `0x101c5ad9`, the first of
  `IShow.cpp`'s (`m_CreateCTextureRenderer`'s statics) is at `0x101c5be8`. Nothing references the 0x108 bytes in
  between, so they are either zero-initialized globals at the end of `IGraph.cpp` or `IShow.cpp` globals whose
  names hash to the lowest buckets.
- **The marker's place in the long files.** `/YX` puts it where a file's own source outweighs the inline code before
  the vector in the precompiled headers (`tools/autopch.py`). In our long files it comes a few functions earlier than
  in the original, so the original's headers had more inline functions before the vector than ours.
