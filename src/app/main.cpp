// Stardust desktop app: window, input, rendering and the frame loop.
// The physics lives in src/sim/ and knows nothing about raylib.
//
// Every frame does the same three things, in this order:
//   1. input   - read keys and mouse, move the camera, pick planets
//   2. update  - advance physics in fixed steps, spin planets, move debris
//   3. draw    - sky, planets, debris, sun glow, HUD (drawing never changes state)
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "rlights.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "AssetPaths.h"
#include "Convert.h"
#include "Debris.h"
#include "HUD.h"
#include "Input.h"
#include "PlanetInfo.h"
#include "ScreenshotScript.h"
#include "io/HdrLoader.h"
#include "sim/FixedTimestep.h"
#include "sim/Simulation.h"
#include "sim/SolarSystem.h"

namespace {

// Loads the nebula sky into a GPU texture. The 93 MB HDR file is decoded by our
// own loader (src/io/HdrLoader). If it cannot be loaded, a magenta checkerboard
// makes the problem obvious instead of failing silently.
Texture2D LoadSkyTexture(const char *path) {
  Image image = {};
  io::HdrImage hdr;
  std::string error;
  if (io::LoadHdr(path, hdr, &error)) {
    // raylib frees image data with its own allocator, so give it a copy it owns.
    image.data = RL_MALLOC(hdr.rgba.size());
    std::memcpy(image.data, hdr.rgba.data(), hdr.rgba.size());
    image.width = hdr.width;
    image.height = hdr.height;
    image.mipmaps = 1;
    image.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
  } else {
    TraceLog(LOG_WARNING, "STARDUST: Sky not loaded: %s", error.c_str());
    image = GenImageChecked(512, 256, 32, 32, MAGENTA, DARKBLUE);
  }

  // Most GPUs cannot hold textures wider or taller than 8192 pixels, so shrink
  // (keeping the shape) before uploading. The sky is 10000x5000 -> 8192x4096.
  if (image.width > 8192 || image.height > 8192) {
    int newW = image.width, newH = image.height;
    if (newW > 8192) { newH = newH * 8192 / newW; newW = 8192; }
    if (newH > 8192) { newW = newW * 8192 / newH; newH = 8192; }
    ImageResize(&image, newW, newH);
  }

  Texture2D texture = LoadTextureFromImage(image); // copies the pixels to the GPU
  UnloadImage(image);                              // so the CPU copy can go
  GenTextureMipmaps(&texture); // smaller copies for when the sky is seen at a distance
  SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
  return texture;
}

// Smoothly flies the camera to a point behind `target` and keeps looking at it.
// The mouse wheel changes the distance (trackingZoom, 0.2x to 5x).
void UpdateTrackingCamera(Camera3D &camera, const sim::Body &target, float &trackingZoom,
                          float dt) {
  Vector3 targetPos = ToRaylib(target.position);

  float wheel = GetMouseWheelMove();
  if (wheel != 0.0f && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    trackingZoom -= wheel * 0.1f;
    if (trackingZoom < 0.2f) trackingZoom = 0.2f; // prevent clipping into planet
    if (trackingZoom > 5.0f) trackingZoom = 5.0f; // limit max orbit distance
  }

  Vector3 currentOffset = Vector3Subtract(camera.position, targetPos);
  float defaultDist = target.radius * 8.0f;
  if (defaultDist < 15.0f) defaultDist = 15.0f;
  float targetDist = defaultDist * trackingZoom;

  Vector3 dir = Vector3Normalize(currentOffset);
  if (Vector3Length(currentOffset) < 0.1f) dir = Vector3{0.0f, 0.5f, 0.8f};

  Vector3 desiredPosition = Vector3Add(targetPos, Vector3Scale(dir, targetDist));
  // Ease towards the goal. Moving a fixed fraction (1 - e^(-k*dt)) of the
  // remaining distance each frame gives the same motion at any frame rate;
  // the old "dt * k" overshot when a frame was long (dt * 3.5 > 1).
  float positionBlend = 1.0f - std::exp(-3.5f * dt);
  float targetBlend = 1.0f - std::exp(-5.0f * dt);
  camera.position = Vector3Lerp(camera.position, desiredPosition, positionBlend);
  camera.target = Vector3Lerp(camera.target, targetPos, targetBlend);
}

// A cheap "bloom" glow around the Sun: four soft circles drawn on top of the
// finished 3D image with additive blending (colours add up, so overlaps get
// brighter). The size shrinks with distance like the Sun itself does.
void DrawSunBloom(const Camera3D &camera, const sim::Body &sun) {
  Vector3 sunPos = ToRaylib(sun.position);
  Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
  Vector3 toSun = Vector3Subtract(sunPos, camera.position);
  float sunDist = Vector3Length(toSun);

  // Only when the Sun is in front of the camera (dot product of the two
  // directions > 0); behind the camera it would project to a wrong spot.
  if (sunDist <= 0.1f || Vector3DotProduct(camForward, Vector3Normalize(toSun)) <= 0.0f) return;

  Vector2 sunScreen = GetWorldToScreen(sunPos, camera);
  float bloomBase = (sun.radius * 800.0f) / sunDist;
  if (bloomBase < 8.0f) bloomBase = 8.0f;
  int x = static_cast<int>(sunScreen.x), y = static_cast<int>(sunScreen.y);

  BeginBlendMode(BLEND_ADDITIVE);
  DrawCircleGradient(x, y, bloomBase * 8.0f, Color{255, 120, 20, 25}, Color{255, 80, 10, 0});
  DrawCircleGradient(x, y, bloomBase * 4.0f, Color{255, 170, 50, 50}, Color{255, 120, 30, 0});
  DrawCircleGradient(x, y, bloomBase * 2.0f, Color{255, 210, 100, 80}, Color{255, 160, 50, 0});
  DrawCircleGradient(x, y, bloomBase * 1.0f, Color{255, 240, 180, 100}, Color{255, 200, 80, 0});
  EndBlendMode();
}

} // namespace

int main(int argc, char **argv) {
  ScreenshotScript script = ParseScreenshotScript(argc, argv);

  ChangeToAssetRoot();

  // Preferred size 1920x1080. On a monitor that is not bigger than that (like a
  // 1080p laptop screen), a 1920x1080 window plus its title bar does not fit,
  // so maximise instead: Windows then sizes it to the space above the taskbar.
  // The window can be resized; the HUD reads the live size every frame.
  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
  InitWindow(1920, 1080, "Stardust");
  SetWindowMinSize(1024, 600);
  int monitor = GetCurrentMonitor();
  if (GetMonitorWidth(monitor) <= 1920 || GetMonitorHeight(monitor) <= 1080) MaximizeWindow();

  // ---- Load everything ------------------------------------------------------

  Shader lightShader = LoadShader("resources/shaders/glsl330/lighting.vs",
                                  "resources/shaders/glsl330/lighting.fs");
  int viewPosLoc = GetShaderLocation(lightShader, "viewPos");
  Light sunLight = CreateLight(LIGHT_POINT, Vector3Zero(), Vector3Zero(), WHITE, lightShader);

  Camera3D camera = {};
  camera.position = Vector3{0.0f, 160.0f, 200.0f};
  camera.target = Vector3{0.0f, 0.0f, 0.0f};
  camera.up = Vector3{0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  // The sky is a big sphere drawn around the camera with the texture on its inside.
  Shader skyboxShader = LoadShader("resources/shaders/glsl330/skybox.vs",
                                   "resources/shaders/glsl330/skybox.fs");
  Model skybox = LoadModelFromMesh(GenMeshSphere(1.0f, 64, 64));
  skybox.materials[0].shader = skyboxShader;
  skybox.materials[0].maps[MATERIAL_MAP_ALBEDO].texture =
      LoadSkyTexture("assets/HDR_multi_nebulae_1.hdr");

  // Three index-paired arrays: bodies[i] (physics), planetInfo[i] (name, slider
  // ranges, spin) and planetModels[i] (the 3D model) all describe planet i.
  const std::vector<sim::PlanetSpec> &specs = sim::SolarSystemSpecs();
  sim::Simulation simulation(sim::BuildBodies(specs));
  std::vector<sim::Body> &bodies = simulation.Bodies(); // stays valid across resets
  std::vector<PlanetInfo> planetInfo = MakePlanetInfo(specs);
  std::vector<Model> planetModels;
  for (const PlanetInfo &info : planetInfo) {
    planetModels.push_back(LoadModel(info.modelPath.c_str()));
  }

  // Every planet uses the lighting shader. The Sun (index 0) is skipped and
  // keeps raylib's default unlit shader, so it looks self-lit.
  for (size_t i = 1; i < planetModels.size(); i++) {
    for (int m = 0; m < planetModels[i].materialCount; m++) {
      planetModels[i].materials[m].shader = lightShader;
    }
  }

  std::vector<Fragment> activeFragments;
  activeFragments.reserve(1024);

  // Ambient music is optional: the file is not in the repository. Put any mp3
  // at assets/ambient_space.mp3 to hear it.
  InitAudioDevice();
  const char *musicPath = "assets/ambient_space.mp3";
  bool hasMusic = FileExists(musicPath);
  Music ambientMusic = {};
  if (hasMusic) {
    ambientMusic = LoadMusicStream(musicPath);
    ambientMusic.looping = true;
    SetMusicVolume(ambientMusic, 0.025f);
    PlayMusicStream(ambientMusic);
  }

  // ---- App state --------------------------------------------------------------

  EngineState currentState = PAUSED;
  int selected = -1; // index into bodies, or -1 for none
  float cameraSpeed = 20.0f;
  bool isTracking = false;
  float trackingZoom = 1.0f;
  int lastSelectedForZoom = -1;

  // Physics always advances in steps of exactly 1/600 s (ten per frame at
  // 60 FPS), however long each frame takes. See src/sim/FixedTimestep.h.
  sim::FixedTimestep physicsClock(1.0 / 600.0);
  double simulatedSeconds = 0.0; // total physics time since start or reset
  bool quit = false;

  // ---- The frame loop ---------------------------------------------------------

  while (!WindowShouldClose() && !quit) {
    float dt = GetFrameTime(); // seconds since the last frame
    if (hasMusic) UpdateMusicStream(ambientMusic);

    // 1. Input ------------------------------------------------------------------

    if (IsKeyPressed(KEY_SPACE)) {
      currentState = (currentState == PLAYING) ? PAUSED : PLAYING;
    }
    if (IsKeyPressed(KEY_R)) {
      selected = -1;
      isTracking = false;
      currentState = PAUSED;
      bodies = sim::BuildBodies(specs);
      physicsClock.Reset();
      simulatedSeconds = 0.0;
      planetInfo = MakePlanetInfo(specs);
      activeFragments.clear();
    }

    if (selected != lastSelectedForZoom) {
      trackingZoom = 1.0f;
      lastSelectedForZoom = selected;
    }

    ProcessDesktopInput(camera, cameraSpeed, bodies, selected, isTracking, dt);

    if (isTracking && selected >= 0 && bodies[selected].isAlive) {
      UpdateTrackingCamera(camera, bodies[selected], trackingZoom, dt);
    } else {
      isTracking = false;
    }

    // 2. Update -----------------------------------------------------------------

    if (currentState == PLAYING) {
      int steps = physicsClock.Advance(dt);
      for (int step = 0; step < steps; step++) {
        simulatedSeconds += physicsClock.StepSeconds();
        for (const sim::MergeEvent &event : simulation.Step(physicsClock.StepSeconds())) {
          SpawnDebris(event, planetInfo[event.victim].tint, activeFragments);
          if (selected == static_cast<int>(event.victim)) {
            selected = -1;
            isTracking = false;
          }
          // Let the sliders reach the merged values.
          PlanetInfo &survivor = planetInfo[event.survivor];
          const sim::Body &merged = bodies[event.survivor];
          if (merged.mass > survivor.massMax) survivor.massMax = merged.mass * 1.5f;
          if (merged.radius > survivor.radiusMax) survivor.radiusMax = merged.radius * 1.5f;
        }
      }

      // Visual-only motion. It belongs here with the other updates, not in the
      // drawing code: drawing should only look at state, never change it.
      for (PlanetInfo &info : planetInfo) info.spinAngle += info.spinRate * dt * RAD2DEG;
      UpdateDebris(activeFragments, dt);
    }

    // Tell the lighting shader where the camera and the Sun are this frame.
    float camPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(lightShader, viewPosLoc, camPos, SHADER_UNIFORM_VEC3);
    sunLight.position = ToRaylib(bodies[0].position);
    UpdateLightValues(lightShader, sunLight);

    // 3. Draw -------------------------------------------------------------------

    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D(camera);

    // The sky sphere is centred on the camera so it never gets closer. Back-face
    // culling is off because we see the inside of the sphere, and depth writes
    // are off so the sky never hides anything drawn after it.
    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    DrawModelEx(skybox, camera.position, Vector3{1.0f, 0.0f, 0.0f}, 90.0f,
                Vector3{500.0f, 500.0f, 500.0f}, WHITE);
    rlEnableBackfaceCulling();
    rlEnableDepthMask();

    for (size_t i = 0; i < bodies.size(); i++) {
      if (!bodies[i].isAlive) continue;
      float r = bodies[i].radius; // models are unit spheres, so scale = radius
      DrawModelEx(planetModels[i], ToRaylib(bodies[i].position), Vector3{0.0f, 1.0f, 0.0f},
                  planetInfo[i].spinAngle, Vector3{r, r, r}, WHITE);
    }

    DrawDebris(activeFragments);

    EndMode3D();

    // 2D on top of the 3D image.
    if (bodies[0].isAlive) DrawSunBloom(camera, bodies[0]);

    const int screenWidth = GetScreenWidth(), screenHeight = GetScreenHeight();
    DrawSelectionReticle(selected, bodies, planetInfo, camera);
    DrawDebugOverlay(screenWidth, selected, bodies, planetInfo, currentState, isTracking);
    DrawHelpBar(screenHeight);
    DrawKeyPressOverlay(screenWidth, screenHeight);

    // Scripted screenshot run (only with --screenshots <folder>).
    if (script.Active()) {
      script.framesInStage++;
      double inStage = simulatedSeconds - script.stageStartSimTime;
      bool advance = false;
      if (script.stage == 0 && script.framesInStage == 30) {
        SaveScreen(script, "1_paused.png");
        currentState = PLAYING;
        advance = true;
      } else if (script.stage == 1 && inStage >= 3.0) {
        SaveScreen(script, "2_playing.png");
        selected = 3; // Earth
        isTracking = true;
        advance = true;
      } else if (script.stage == 2 && inStage >= 3.0) {
        SaveScreen(script, "3_tracking_earth.png");
        int alive = 0;
        for (const sim::Body &b : bodies) alive += b.isAlive ? 1 : 0;
        TraceLog(LOG_INFO, "STARDUST: screenshot script done, %d bodies alive", alive);
        quit = true;
      }
      if (advance) {
        script.stage++;
        script.framesInStage = 0;
        script.stageStartSimTime = simulatedSeconds;
      }
    }

    EndDrawing(); // shows the finished frame (and waits for vsync)
  }

  // ---- Cleanup ----------------------------------------------------------------
  // UnloadModel also frees the model's materials and their textures (the sky).

  UnloadModel(skybox);
  UnloadShader(skyboxShader);
  if (hasMusic) UnloadMusicStream(ambientMusic);
  CloseAudioDevice();
  for (Model &model : planetModels) UnloadModel(model);
  UnloadShader(lightShader);
  CloseWindow();
  return 0;
}
