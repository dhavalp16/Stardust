// Stardust desktop app: window, input, rendering and the frame loop.
// The physics lives in src/sim/ and knows nothing about raylib.
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <vector>

#include "hdr_loader.h"
#include "rlgl.h"
#include "rlights.h"

#include "AssetPaths.h"
#include "Convert.h"
#include "Debris.h"
#include "HUD.h"
#include "Input.h"
#include "PlanetInfo.h"
#include "sim/FixedTimestep.h"
#include "sim/Simulation.h"
#include "sim/SolarSystem.h"

int main() {
  const int screenWidth = 1920;
  const int screenHeight = 1080;

  ChangeToAssetRoot();

  SetConfigFlags(FLAG_VSYNC_HINT);
  InitWindow(screenWidth, screenHeight, "Stardust");

  Shader lightShader = LoadShader("resources/shaders/glsl330/lighting.vs",
                                  "resources/shaders/glsl330/lighting.fs");
  int viewPosLoc = GetShaderLocation(lightShader, "viewPos");

  Light sun = CreateLight(LIGHT_POINT, Vector3{0.0f, 0.0f, 0.0f}, Vector3Zero(),
                          WHITE, lightShader);

  Camera3D camera = {};
  camera.position = Vector3{0.0f, 160.0f, 200.0f};
  camera.target = Vector3{0.0f, 0.0f, 0.0f};
  camera.up = Vector3{0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  // Load HDR with custom loader (Raylib's stb_image fails on this 93MB file)
  Image skyImg = LoadHDRManual("assets/HDR_multi_nebulae_1.hdr");
  bool hdrLoaded = (skyImg.data != NULL);

  if (!hdrLoaded) {
    skyImg = GenImageChecked(512, 256, 32, 32, MAGENTA, DARKBLUE);
  }

  // Cap texture size at 8192 for GPU compatibility
  if (skyImg.width > 8192 || skyImg.height > 8192) {
    int newW = skyImg.width, newH = skyImg.height;
    if (newW > 8192) { newH = newH * 8192 / newW; newW = 8192; }
    if (newH > 8192) { newW = newW * 8192 / newH; newH = 8192; }
    ImageResize(&skyImg, newW, newH);
  }

  Texture2D skyboxTexture = LoadTextureFromImage(skyImg);
  UnloadImage(skyImg);
  GenTextureMipmaps(&skyboxTexture);
  SetTextureFilter(skyboxTexture, TEXTURE_FILTER_BILINEAR);

  Shader skyboxShader = LoadShader("resources/shaders/glsl330/skybox.vs",
                                    "resources/shaders/glsl330/skybox.fs");
  Model skybox = LoadModelFromMesh(GenMeshSphere(1.0f, 64, 64));
  skybox.materials[0].shader = skyboxShader;
  skybox.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = skyboxTexture;

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

  std::vector<Fragment> activeFragments;
  activeFragments.reserve(1024);

  Shader defaultSunShader = planetModels[0].materials[0].shader;

  for (size_t i = 0; i < planetModels.size(); i++) {
    for (int m = 0; m < planetModels[i].materialCount; m++) {
      planetModels[i].materials[m].shader = lightShader;
    }
  }

  InitAudioDevice();
  Music ambientMusic = LoadMusicStream("assets/ambient_space.mp3");
  ambientMusic.looping = true;
  SetMusicVolume(ambientMusic, 0.025f);
  PlayMusicStream(ambientMusic);

  EngineState currentState = PAUSED;
  int selected = -1; // index into bodies, or -1 for none
  float cameraSpeed = 20.0f;
  bool isTracking = false;
  // Physics always advances in steps of exactly 1/600 s (ten per frame at
  // 60 FPS), however long each frame takes. See src/sim/FixedTimestep.h.
  sim::FixedTimestep physicsClock(1.0 / 600.0);

  float trackingZoom = 1.0f;
  int lastSelectedForZoom = -1;

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    UpdateMusicStream(ambientMusic);

    // Controls
    if (IsKeyPressed(KEY_SPACE)) {
      currentState = (currentState == PLAYING) ? PAUSED : PLAYING;
    }
    if (IsKeyPressed(KEY_R)) {
      selected = -1;
      isTracking = false;
      currentState = PAUSED;
      bodies = sim::BuildBodies(specs);
      physicsClock.Reset();
      planetInfo = MakePlanetInfo(specs);
      activeFragments.clear();
    }

    if (selected != lastSelectedForZoom) {
      trackingZoom = 1.0f;
      lastSelectedForZoom = selected;
    }

    ProcessDesktopInput(camera, cameraSpeed, bodies, selected, isTracking, dt);

    if (isTracking && selected >= 0 && bodies[selected].isAlive) {
      const sim::Body &target = bodies[selected];
      Vector3 targetPos = ToRaylib(target.position);

      // Zoom while tracking
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
      camera.position = Vector3Lerp(camera.position, desiredPosition, dt * 3.5f);
      camera.target = Vector3Lerp(camera.target, targetPos, dt * 5.0f);
    } else if (isTracking) {
      isTracking = false;
    }

    // Physics Pipeline
    if (currentState == PLAYING) {
      int steps = physicsClock.Advance(dt);
      for (int step = 0; step < steps; step++) {
        float h = static_cast<float>(physicsClock.StepSeconds());
        for (const sim::MergeEvent &event : simulation.Step(h)) {
          SpawnDebris(event, planetInfo[event.victim].tint, activeFragments);
          if (selected == (int)event.victim) {
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
    }

    float camPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(lightShader, viewPosLoc, camPos, SHADER_UNIFORM_VEC3);

    // Rendering
    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D(camera);

    rlDisableBackfaceCulling();
    rlDisableDepthMask();
    DrawModelEx(skybox, camera.position, Vector3{1.0f, 0.0f, 0.0f}, 90.0f, Vector3{500.0f, 500.0f, 500.0f}, WHITE);
    rlEnableBackfaceCulling();
    rlEnableDepthMask();

    for (size_t i = 0; i < bodies.size(); i++) {
      if (!bodies[i].isAlive) continue;

      float r = bodies[i].radius;
      Vector3 modelScale = {r, r, r};

      if (currentState == PLAYING) {
        planetInfo[i].spinAngle += planetInfo[i].spinRate * dt * RAD2DEG;
      }

      if (i == 0) {
        for (int m = 0; m < planetModels[i].materialCount; m++)
          planetModels[i].materials[m].shader = defaultSunShader;
      }

      DrawModelEx(planetModels[i], ToRaylib(bodies[i].position), Vector3{0.0f, 1.0f, 0.0f},
                  planetInfo[i].spinAngle, modelScale, WHITE);

      if (i == 0) {
        for (int m = 0; m < planetModels[i].materialCount; m++)
          planetModels[i].materials[m].shader = lightShader;
      }
    }

    for (size_t f = 0; f < activeFragments.size(); f++) {
      if (!activeFragments[f].isAlive) continue;

      if (currentState == PLAYING) {
        activeFragments[f].position = Vector3Add(activeFragments[f].position,
                                                 Vector3Scale(activeFragments[f].velocity, dt));
        activeFragments[f].life -= dt * 0.5f;
        if (activeFragments[f].life <= 0.0f) {
          activeFragments[f].isAlive = false;
          continue;
        }
      }

      Color renderColor = activeFragments[f].color;
      renderColor.a = (unsigned char)(activeFragments[f].life * 255.0f);
      DrawCubeV(activeFragments[f].position,
                Vector3{activeFragments[f].size, activeFragments[f].size, activeFragments[f].size},
                renderColor);
    }

    activeFragments.erase(
        std::remove_if(activeFragments.begin(), activeFragments.end(),
                       [](const Fragment &f) { return !f.isAlive; }),
        activeFragments.end());

    EndMode3D();

    // Sun bloom effect (screen-space, additive blending)
    if (bodies[0].isAlive) {
      Vector3 sunPos = ToRaylib(bodies[0].position);
      Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
      Vector3 toSun = Vector3Subtract(sunPos, camera.position);
      float sunDist = Vector3Length(toSun);

      // Only draw bloom if sun is in front of camera
      if (sunDist > 0.1f && Vector3DotProduct(camForward, Vector3Normalize(toSun)) > 0.0f) {
        Vector2 sunScreen = GetWorldToScreen(sunPos, camera);
        float bloomBase = (bodies[0].radius * 800.0f) / sunDist;
        if (bloomBase < 8.0f) bloomBase = 8.0f;

        BeginBlendMode(BLEND_ADDITIVE);
        // Outer soft glow
        DrawCircleGradient((int)sunScreen.x, (int)sunScreen.y, bloomBase * 8.0f,
                           Color{255, 120, 20, 25}, Color{255, 80, 10, 0});
        // Mid glow
        DrawCircleGradient((int)sunScreen.x, (int)sunScreen.y, bloomBase * 4.0f,
                           Color{255, 170, 50, 50}, Color{255, 120, 30, 0});
        // Inner bright glow
        DrawCircleGradient((int)sunScreen.x, (int)sunScreen.y, bloomBase * 2.0f,
                           Color{255, 210, 100, 80}, Color{255, 160, 50, 0});
        // Core hot spot
        DrawCircleGradient((int)sunScreen.x, (int)sunScreen.y, bloomBase * 1.0f,
                           Color{255, 240, 180, 100}, Color{255, 200, 80, 0});
        EndBlendMode();
      }
    }

    DrawSelectionReticle(selected, bodies, planetInfo, camera);
    DrawDebugOverlay(screenWidth, screenHeight, selected, bodies, planetInfo, currentState,
                     isTracking);
    DrawHelpBar(screenHeight);
    DrawKeyPressOverlay(screenWidth, screenHeight);

    EndDrawing();
  }

  // Cleanup
  UnloadModel(skybox);
  UnloadShader(skyboxShader);
  UnloadMusicStream(ambientMusic);
  CloseAudioDevice();
  for (size_t i = 0; i < planetModels.size(); i++) UnloadModel(planetModels[i]);
  UnloadShader(lightShader);
  CloseWindow();
  return 0;
}
