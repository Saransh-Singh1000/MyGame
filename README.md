# MyGame

## Gameplay

The 8x8 cube field is stationary and has solid collisions. Use **WASD** to walk,
**Space** to jump, **Shift** to run, and click to capture the mouse for looking
around. Press **Esc** to release the mouse and open renderer settings. Gravity
pulls the player onto the cubes; falling below Y = -50 returns the player to the
starting cube.

Choose **1** for Vulkan or **2** for OpenGL; the menu shows the active renderer.
Press **Esc** again to close settings.

## Vulkan shaders

The GLSL sources are in `Common/shaders`. CMake uses `glslc`, `glslangValidator`, or
`glslang` when one is available; otherwise it embeds the checked-in SPIR-V files, so
build machines do not need a shader compiler installed.

After editing a shader, regenerate its SPIR-V file with Glslang, for example:

```powershell
glslang -V Common/shaders/scene.vert -o Common/shaders/scene.vert.spv
glslang -V Common/shaders/scene.frag -o Common/shaders/scene.frag.spv
```