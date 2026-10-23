# MyGame

## Gameplay

The 1000x1000 cube platform has solid Jolt Physics collisions. Use **WASD** to walk,
**Space** to jump, **Shift** to run, and click to capture the mouse for looking
around. Press **Esc** to release the mouse and open renderer settings. Gravity
pulls the player onto the platform; falling below Y = -50 returns the player to
the starting position. Nearby platform tiles are rendered as individual cubes,
while a single equivalent Jolt box collider keeps the million-tile floor efficient.

The shared `physics/physics_world.hpp` API exposes `PhysicsWorld::createCube`
and `PhysicsWorld::createSphere` for adding positioned physics objects. For many
stationary cubes, `PhysicsWorld::createStaticCubeBatch` combines them into one
Jolt static body; all cubes in that batch share its `BodyHandle`.

Choose **1** for Vulkan or **2** for OpenGL; the menu shows the active renderer.
Press **Esc** again to close settings.

## Performance optimizations

- **Scene geometry:** The platform contains 1,000 x 1,000 tiles, but rendering
  considers only a 65 x 65 area around the player (at most 4,225 tiles). It
  rejects tiles behind the camera and outside the screen, and skips back-facing
  cube faces. Visible geometry is submitted in one indexed draw call.
- **Physics:** The million-tile floor uses one equivalent Jolt box collider.
  Static cube batches use one compound body and share a box shape. Jolt's
  worker pool uses available hardware threads while leaving one for other work
  when possible. Physics uses a fixed 60 Hz step and caps catch-up at six steps
  per update.
- **CPU-side geometry memory:** Scene vectors reserve capacity for the expected
  nearby geometry and are cleared and reused between frames, avoiding repeated
  allocations in typical frames. Their initial vertex/index reservations are
  sized for six faces per nearby tile rather than a larger unnecessary
  overestimate.
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
  not currently implemented. Vulkan FIFO presentation and OpenGL vertical sync
  pace rendering to the display where supported instead of continuously
  rendering uncapped frames.

## Windows launcher

The shared launcher page (HTML, CSS, and JavaScript) is embedded in
`Common/include/launcher/launcher_page.hpp` for platform-specific launcher hosts
to reuse. The Windows release ZIP includes the Ultralight runtime, and the
Windows release workflow downloads the 64-bit Ultralight 1.4.0 SDK while
building. For local Windows builds, download and extract the SDK from
[ultralig.ht/download](https://ultralig.ht/download), then configure CMake with
`-DULTRALIGHT_SDK_ROOT="C:\path\to\ultralight-sdk"` or set the
`ULTRALIGHT_SDK_ROOT` environment variable. Builds without the SDK use a native
renderer-selection dialog instead.

## Vulkan shaders

The GLSL sources are in `Common/shaders`. CMake uses `glslc`, `glslangValidator`, or
`glslang` when one is available; otherwise it embeds the checked-in SPIR-V files, so
build machines do not need a shader compiler installed.

After editing a shader, regenerate its SPIR-V file with Glslang, for example:

```powershell
glslang -V Common/shaders/scene.vert -o Common/shaders/scene.vert.spv
glslang -V Common/shaders/scene.frag -o Common/shaders/scene.frag.spv
```