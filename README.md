# MyGame

## Gameplay

The 1000x1000 cube platform has solid Jolt Physics collisions. Use **WASD** to walk,
**Space** to jump, **Shift** to run, and click to capture the mouse for looking
around. Press **Esc** to release the mouse and open renderer settings. Gravity
pulls the player onto the platform; falling below Y = -50 returns the player to
the starting position. Nearby platform tiles are rendered as individual cubes,
while a single equivalent Jolt box collider keeps the million-tile floor efficient.

The shared `physics/physics_world.hpp` API exposes `PhysicsWorld::createCube`
and `PhysicsWorld::createSphere` for adding positioned physics objects.

Choose **1** for Vulkan or **2** for OpenGL; the menu shows the active renderer.
Press **Esc** again to close settings.

## Windows launcher

The Windows executable opens a renderer launcher before starting the game. To use
the HTML/CSS/JavaScript launcher, download and extract the 64-bit Ultralight SDK
from [ultralig.ht/download](https://ultralig.ht/download), then configure CMake
with `-DULTRALIGHT_SDK_ROOT="C:\path\to\ultralight-sdk"` or set the
`ULTRALIGHT_SDK_ROOT` environment variable. CMake stages the launcher assets,
Ultralight runtime binaries, and resources alongside the executable. If the SDK
is not installed, Windows builds use a native renderer-selection dialog instead.

## Vulkan shaders

The GLSL sources are in `Common/shaders`. CMake uses `glslc`, `glslangValidator`, or
`glslang` when one is available; otherwise it embeds the checked-in SPIR-V files, so
build machines do not need a shader compiler installed.

After editing a shader, regenerate its SPIR-V file with Glslang, for example:

```powershell
glslang -V Common/shaders/scene.vert -o Common/shaders/scene.vert.spv
glslang -V Common/shaders/scene.frag -o Common/shaders/scene.frag.spv
```