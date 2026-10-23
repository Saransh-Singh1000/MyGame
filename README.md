# MyGame

## Gameplay

The game now generates a seeded, chunked voxel landscape with grassland, desert,
snow, caves, trees, water bodies, and clouds. Chunk meshes contain exposed block
faces and distant chunks are unloaded from memory while terrain edits remain
saved. Terrain has Jolt collision. A hostile creature follows nearby players.

- **WASD** move, **Space** jump, **Shift** sprint, and mouse movement looks.
  Click to capture the mouse; **Esc** releases it and opens renderer settings.
- **E** mines the block under the crosshair, **F** attacks a nearby enemy, and
  right-click places the selected block. **1–8** select the hotbar item.
- **C** crafts: one wood block into four planks, two planks into four sticks,
  then three stone plus two sticks into a stone pick.
- **F5** saves terrain edits and inventory; **F9** loads them.
- Touch controls use the left side to move, the right side to look, top-right
  tap zones to mine/attack/place, the bottom-right to jump, and the bottom
  hotbar area to select a block.

Choose **1** for Vulkan or **2** for OpenGL in renderer settings. Android uses
Vulkan. This is an original voxel-game implementation inspired by common
chunked-world techniques; it does not include vkCraft source.

## Performance optimizations

- **Scene geometry:** The world is divided into 16 x 16 x 40 voxel chunks.
  Cached chunk meshes contain only exposed block faces. The renderer limits
  visible chunks around the player, rejects back-facing/off-screen faces, sorts
  visible faces for painter-style depth, and submits indexed geometry in one
  draw call.
- **Physics:** Nearby terrain collision is represented by chunk-level Jolt
  compound bodies with shared box shapes; bodies outside the active area are
  removed. Jolt's worker pool uses available hardware threads while leaving one
  for other work when possible. Physics uses a fixed 60 Hz step and caps
  catch-up at six steps per update.
- **CPU-side geometry memory:** Scene vectors reserve capacity for the expected
  nearby geometry and are cleared and reused between frames, avoiding repeated
  allocations in typical frames.
- **Command buffers and multi-threading:** Vulkan has two reusable command
  buffers and two frames in flight, with per-frame synchronization. This allows
  CPU work for a later frame to overlap GPU work where synchronization permits.
  Jolt physics is multithreaded; Vulkan command recording and scene construction
  are currently performed on the calling thread.
- **GPU memory management:** Each Vulkan frame has persistently mapped,
  host-visible, coherent vertex and index buffers. Their capacity is retained
  and buffers grow geometrically only when a larger scene requires it, reducing
  reallocations when scene size changes.
- **Pipelines and shaders:** Vulkan creates a graphics pipeline from the
  checked-in SPIR-V shaders and reuses it across frames (recreating it only
  when the swapchain changes). The viewport and scissor are dynamic, and the
  viewport dimensions are passed as push constants.
- **Render passes and mobile tile-based GPUs:** Vulkan uses a single-subpass,
  single-color-attachment render pass, clears the attachment once, and renders
  directly to the swapchain image. This avoids extra render targets and
  depth/MSAA attachments in the current scene. There are no explicit
  tile-memory hints or transient attachments; the swapchain color image must
  be stored for presentation, so dedicated tile-based-GPU optimizations are
  not currently implemented.
- **Frame pacing:** VSync is enabled by default. Windows and Linux builds accept
  `--uncapped` to disable Vulkan/OpenGL synchronization when the driver supports
  it, for benchmarking. Uncapped mode can tear, consume more power, or still be
  limited by the display/driver. Chunk generation and meshing currently run on
  the game thread, and software projection/painter sorting are CPU costs.
  Consequently, 1,000–2,000 FPS is not guaranteed and is unrealistic on many
  systems; the uncapped switch only removes the application's VSync limit.

The game has a basic single-enemy loop, simple block recipes, static water/cloud
voxels, and saves modified blocks plus inventory. It does not yet include
advanced pathfinding, swimming, animated weather, crafting stations, multiplayer,
or a fully GPU-native 3D renderer.

## Desktop builds and release files

Windows and Linux start directly in the game; there is no separate launcher.
Each desktop release publishes its native executable as an individual file, not
a ZIP. The Windows release also publishes `vulkan-1.dll`; keep it beside the
executable when distributing it.

SDL and the C/C++ runtime are linked statically where supported. Graphics drivers
and operating-system windowing libraries cannot be bundled portably: the target
machine must provide a Vulkan driver/loader and the platform's required display
libraries. Linux builds therefore require compatible system Vulkan, OpenGL, and
SDL windowing dependencies.

## Vulkan shaders

The GLSL sources are in `Common/shaders`. CMake uses `glslc`, `glslangValidator`, or
`glslang` when one is available; otherwise it embeds the checked-in SPIR-V files, so
build machines do not need a shader compiler installed.

After editing a shader, regenerate its SPIR-V file with Glslang, for example:

```powershell
glslang -V Common/shaders/scene.vert -o Common/shaders/scene.vert.spv
glslang -V Common/shaders/scene.frag -o Common/shaders/scene.frag.spv
```