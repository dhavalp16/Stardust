# 5. Graphics: how a frame gets on screen

This chapter explains what happens between "the planets are at these positions" and
"pixels on your screen", using what Stardust actually does. raylib hides a lot of
detail; this chapter tells you what it is hiding, because a graphics or engine
interview will ask.

## 5.1 The GPU and the graphics API

The **GPU** (graphics card; here an NVIDIA RTX 4050) is a processor built to do the
same small calculation on millions of pixels at once. The CPU talks to it through a
**graphics API**. Stardust uses **OpenGL 3.3** through raylib (raylib's `rlgl.h`
layer wraps the OpenGL calls). Other APIs you will hear about: Direct3D 11/12,
Vulkan, Metal. The concepts below are shared by all of them.

The CPU's job each frame: decide *what* to draw and *where*, and send the GPU
**draw calls**: "draw this mesh, with this shader, these textures, this transform".

## 5.2 Meshes, models and the .glb files

A **mesh** is a 3D shape made of triangles. Each corner (**vertex**) has a position,
a **normal** (the direction the surface faces there), and **texture coordinates**
(**UVs**: where on the image this corner takes its colour from).

A **model** is one or more meshes plus **materials** (which shader and textures to
use). The planets are `.glb` files (binary **glTF**, a standard 3D file format),
loaded with `LoadModel` (`src/app/main.cpp:173`). They are unit spheres, so the
planet's radius is used directly as the drawing scale.

## 5.3 Transforms: from model to screen

To draw a planet, every vertex goes through a chain of **transforms**, each a 4x4
**matrix** (a grid of numbers that can rotate, scale and move points):

1. **Model matrix**: places the mesh in the world (scale by radius, spin around
   y, move to the planet's position). `DrawModelEx(model, position, axis, angle,
   scale, tint)` builds it (`src/app/main.cpp:301`).
2. **View matrix**: moves the whole world so the camera sits at the origin looking
   down an axis. Built from `camera.position`, `camera.target`, `camera.up`.
3. **Projection matrix**: perspective: far things get smaller. Built from
   `camera.fovy = 45°` (field of view) and the window's width/height ratio. Anything
   farther than 1000 units (raylib's default **far plane**) is cut off.

Multiplied together this is the **MVP** matrix. `BeginMode3D(camera)` sets up view
and projection. In the shaders below, `mvp` is that combined matrix.

## 5.4 Shaders

A **shader** is a small program that runs on the GPU, written here in **GLSL**
(OpenGL Shading Language). Two kinds:

- The **vertex shader** runs once per vertex: transforms its position with the MVP
  matrix and passes data on.
- The **fragment shader** (also called **pixel shader**) runs once per pixel the
  triangle covers: decides that pixel's colour.

Between them the GPU **rasterises**: works out which pixels each triangle covers and
**interpolates** the vertex data across the triangle.

Inputs that are the same for a whole draw call are **uniforms** (the camera position,
the light). The CPU sets them, e.g. `SetShaderValue(lightShader, viewPosLoc, ...)`
(`src/app/main.cpp:278`).

Stardust's shaders are in `resources/shaders/glsl330/`.

## 5.5 Lighting the planets

The two lighting shaders are adapted from raylib's own lighting example (you can
tell from comments like "NOTE: Add your custom variables here"); the sky shaders
are minimal pass-throughs. Say so if asked: knowing every line is what matters, not
having typed them.

`resources/shaders/glsl330/lighting.vs` sends the world-space position and normal
on. `lighting.fs` computes, per pixel:

- **Diffuse** light (Lambert's law): a surface facing the light gets full light, one
  at 90° gets none. `NdotL = max(dot(normal, lightDirection), 0.0)` (line 64). That is
  the dot product from chapter 2.
- **Specular** highlight (Phong): a shiny spot where the light's reflection points at
  the camera, `pow(max(0, dot(viewDir, reflect(-light, normal))), 16.0)` (line 68).
  The 16 is "shininess".
- **Gamma**: `pow(finalColor, vec4(1.0/2.2))` (line 77), see 5.9.

There is one **point light** (light shining in all directions from a point), created
with `CreateLight` from `third_party/rlights.h` (`src/app/main.cpp:148`). Each frame
its position is set to the Sun's and sent to the shader with `UpdateLightValues`
(`src/app/main.cpp:279-280`). Before the overhaul the light stayed at the origin while
the Sun moved.

The **Sun itself** must not be lit by itself. It simply keeps raylib's default
**unlit** shader (texture colour as-is), because the loop that gives models the
lighting shader starts at index 1 (`src/app/main.cpp`, "Every planet uses the
lighting shader"). The old code swapped the Sun's shader back and forth twice a frame
to get the same effect.

Note: there is no ambient light, so the night side is black, and no shadows.

## 5.6 Textures, sampling and mipmaps

A **texture** is an image on the GPU. The fragment shader reads (**samples**) it at
the pixel's UV coordinate: `texture(texture0, fragTexCoord)`.

When a texture is shown smaller than its real size, many texels (texture pixels)
fall into one screen pixel and sampling just one of them shimmers. **Mipmaps** are
pre-shrunk copies (half size, quarter size, ...); the GPU picks the right one.
`GenTextureMipmaps` makes them for the sky (`LoadSkyTexture` in
`src/app/main.cpp`). **Bilinear filtering** blends the four nearest texels.

GPUs limit texture size; 8192 is a safe maximum, so the 10000x5000 sky is shrunk to
8192x4096 before upload.

## 5.7 The sky

The background is a large sphere (`GenMeshSphere`, radius 1 scaled by 500) with the
nebula texture on its **inside**, centred on the camera every frame so you can never
reach it (`src/app/main.cpp`, "The sky sphere is centred on the camera"):

- **Back-face culling** normally skips triangles facing away from the camera (you
  never see the inside of a closed object). We *are* inside, so it is switched off
  for the sky (`rlDisableBackfaceCulling`).
- The **depth buffer** stores, per pixel, how far away the nearest thing drawn so far
  is, so nearer objects hide farther ones regardless of drawing order. The sky is
  drawn first with **depth writes off** (`rlDisableDepthMask`), so it never hides
  anything drawn after it.

The sky image is **equirectangular** (longitude across, latitude down, like a world
map). Putting it on a UV sphere causes a visible seam and pinching at the poles; the
roadmap (item 4) describes sampling it by view direction instead.

## 5.8 HDR images and tone mapping

Normal images store 0-255 per channel. A real sky has light far brighter than
"white". **HDR** (high dynamic range) images store real brightness values. The
**Radiance .hdr** format stores each pixel as **RGBE**: three 8-bit **mantissas** and
one shared 8-bit **exponent**, value = `mantissa · 2^(exponent - 136)`. Most of the
file is **run-length encoded** (RLE): "repeat this byte 87 times" instead of writing
it 87 times (`ReadScanline`, `src/io/HdrLoader.cpp:59`).

A screen can only show 0-255, so HDR values must be squeezed: **tone mapping**.
Stardust uses (`ToneMapChannel`, `src/io/HdrLoader.cpp:10`):

1. **Exposure**: multiply by 10 (brighten the dim nebula).
2. **Reinhard** curve: `v / (v + 1)`: 0 stays 0, huge values approach 1, never
   exceeding it.
3. A power of `2/2.2`: gamma encoding (5.9) combined with a contrast boost.
4. Times 255.

**Why a custom loader:** raylib's loader first converts the whole image to 32-bit
floats, three per pixel: about 600 MB for 50 million pixels. Ours keeps one row of
RGBE at a time and writes 8-bit output.

**Why a lookup table:** there are only 256 × 256 possible (mantissa, exponent) pairs,
so instead of calling `pow` 150 million times, it computes the curve once for each
pair into a 64 KB table and then just reads it. Together with reading the file in
one go instead of byte by byte, loading went from about 3.4 s to 0.3 s with
byte-identical output (`docs/MEASUREMENTS.md` section 5). This is a classic
optimisation pattern: **precompute when the input space is small**.

## 5.9 Gamma and colour spaces

Screens do not show brightness linearly: a pixel value of 128 is much less than half
as bright as 255. Image files are stored **gamma-encoded** (in the **sRGB** colour
space) to match. Lighting maths (adding light, multiplying by `NdotL`) is only
physically correct on **linear** values. The correct pipeline is: decode textures to
linear, light in linear, encode to sRGB once at the end.

`lighting.fs` encodes at the end but never decodes the texture first, so colours come
out brighter and less saturated than they should, and it also applies the power to
alpha. It looks acceptable, so it was left alone (changing it changes the look), and
written up as roadmap item 5. Knowing this is a good interview point.

## 5.10 Blending: the sun glow and the debris

**Blending** decides how a new pixel combines with what is already there.

- **Additive** blending: new + old. Overlapping glows get brighter, like light.
  The Sun's glow is four soft circles drawn additively on top of the 3D image
  (`DrawSunBloom` in `src/app/main.cpp`). It is a cheap imitation of **bloom**, a
  post-processing effect that blurs the brightest parts of the image.
- **Alpha** blending: `new·alpha + old·(1-alpha)`, for see-through things. Debris cubes
  fade out by lowering alpha (`DrawDebris`, `src/app/Debris.cpp`). Correct
  transparency normally needs sorting far-to-near; with small fading cubes the error
  is not visible.

## 5.11 From 3D to the screen and back

- **World to screen**: `GetWorldToScreen(position, camera)` runs the MVP transform on
  the CPU to find where a 3D point lands in pixels. The selection reticle
  (`src/app/HUD.cpp:16`) and the sun glow use it.
- **Screen to world (picking)**: clicking a planet. `GetMouseRay` turns the mouse
  position into a **ray** (a start point and a direction) from the camera into the
  scene; `GetRayCollisionSphere` tests it against each planet's sphere; the nearest
  hit wins (`src/app/Input.cpp:115-130`). Clicks on HUD panels are ignored first
  (`IsOverHud`, `src/app/HudLayout.h`).

## 5.12 2D on top: the HUD

After `EndMode3D()`, drawing is in 2D screen pixels. The HUD uses **raygui**
(`third_party/raygui.h`), an **immediate-mode GUI**: you call `GuiSlider(...)` every
frame and it both draws the slider and returns/changes the value; there are no
persistent widget objects. That is why the slider can write straight into
`body.mass` (`src/app/HUD.cpp:60`).

raylib collects 2D drawing into a **batch** and sends it to the GPU in one go at
`EndDrawing()` (fewer draw calls is faster). That detail mattered for the screenshot
tool: it must flush the batch (`rlDrawRenderBatchActive`) before reading the screen,
or the HUD is missing (`src/app/ScreenshotScript.cpp`).

## 5.13 Presenting the frame: vsync

`EndDrawing()` shows the finished image. With **vsync** (`FLAG_VSYNC_HINT`,
`src/app/main.cpp:137`) it waits for the monitor's next refresh, which avoids
**tearing** (half of one frame and half of the next on screen) and caps the frame
rate at the refresh rate (144 FPS on this laptop).

## Exercises

1. In `resources/shaders/glsl330/lighting.fs`, change the shininess from 16.0 to
   64.0. Run the screenshot script before and after and compare the highlights.
2. Draw the Sun with the lighting shader too (start that loop at 0). What does it
   look like and why?
3. In `DrawSunBloom`, remove the "Sun is in front of the camera" check, fly so the
   Sun is behind you, and explain what you see.
4. Explain to a friend what the depth buffer does, and what would go wrong if the
   sky were drawn *last* with depth writes on.
5. Compute by hand the tone-mapped value for mantissa 128, exponent 129 (value 1.0).
   Check your answer against `HdrLoader.ToneCurveMatchesTheFormula`.
