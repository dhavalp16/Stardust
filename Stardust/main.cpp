// Desktop branch — clean C++ engine, no Android/JNI dependencies
#define _CRT_SECURE_NO_WARNINGS
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <vector>

#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

#include "Collision.h"
#include "HUD.h"
#include "Input.h"
#include "Physics.h"
#include "Planet.h"

int main() {
  const int screenWidth = 1920;
  const int screenHeight = 1080;

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

  std::vector<Planet> initialPlanets;
  std::vector<Planet> activePlanets;
  std::vector<Model> planetModels;

  initialPlanets.reserve(16);
  activePlanets.reserve(16);
  planetModels.reserve(16);

  std::vector<Fragment> activeFragments;
  activeFragments.reserve(1024);

  initialPlanets = {
      Planet({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 2000.0f, 3.00f,
             "assets/sun.glb", YELLOW, 0.10f, "Sun", 500.0f, 5000.0f, 1.0f,
             6.0f),
      Planet({8.0f * cosf(1.0f), 0.0f, 8.0f * sinf(1.0f)},
             {-15.811f * sinf(1.0f), 0.0f, 15.811f * cosf(1.0f)}, 0.055f, 0.25f,
             "assets/mercury.glb", GRAY, 0.02f, "Mercury", 0.01f, 1.0f, 0.1f,
             1.0f),
      Planet({14.0f * cosf(3.0f), 0.0f, 14.0f * sinf(3.0f)},
             {-11.952f * sinf(3.0f), 0.0f, 11.952f * cosf(3.0f)}, 0.815f, 0.50f,
             "assets/venus.glb", ORANGE, -0.01f, "Venus", 0.1f, 5.0f, 0.1f,
             2.0f),
      Planet({20.0f * cosf(5.0f), 0.0f, 20.0f * sinf(5.0f)},
             {-10.000f * sinf(5.0f), 0.0f, 10.000f * cosf(5.0f)}, 10.00f, 0.55f,
             "assets/earth.glb", BLUE, 0.50f, "Earth", 1.0f, 20.0f, 0.2f, 3.0f),
      Planet({30.0f * cosf(0.5f), 0.0f, 30.0f * sinf(0.5f)},
             {-8.165f * sinf(0.5f), 0.0f, 8.165f * cosf(0.5f)}, 0.107f, 0.35f,
             "assets/mars.glb", RED, 0.48f, "Mars", 0.01f, 2.0f, 0.1f, 1.5f),
      Planet({55.0f * cosf(2.5f), 0.0f, 55.0f * sinf(2.5f)},
             {-6.030f * sinf(2.5f), 0.0f, 6.030f * cosf(2.5f)}, 3.00f, 1.80f,
             "assets/jupiter.glb", BEIGE, 1.50f, "Jupiter", 0.5f, 15.0f, 0.5f,
             4.0f),
      Planet({80.0f * cosf(4.5f), 0.0f, 80.0f * sinf(4.5f)},
             {-5.000f * sinf(4.5f), 0.0f, 5.000f * cosf(4.5f)}, 1.50f, 1.50f,
             "assets/saturn.glb", GOLD, 1.30f, "Saturn", 0.2f, 10.0f, 0.5f,
             3.5f),
      Planet({110.0f * cosf(1.5f), 0.0f, 110.0f * sinf(1.5f)},
             {-4.264f * sinf(1.5f), 0.0f, 4.264f * cosf(1.5f)}, 0.50f, 1.00f,
             "assets/uranus.glb", SKYBLUE, -0.80f, "Uranus", 0.1f, 5.0f, 0.3f,
             2.5f),
      Planet({140.0f * cosf(3.5f), 0.0f, 140.0f * sinf(3.5f)},
             {-3.780f * sinf(3.5f), 0.0f, 3.780f * cosf(3.5f)}, 0.60f, 0.95f,
             "assets/neptune.glb", DARKBLUE, 0.90f, "Neptune", 0.1f, 5.0f, 0.3f,
             2.5f),
      Planet({20.0f * cosf(5.0f) + 0.9f, 0.0f, 20.0f * sinf(5.0f)},
             {-10.000f * sinf(5.0f), 0.0f, 10.000f * cosf(5.0f) + 3.333f},
             0.012f, 0.15f, "assets/moon.glb", LIGHTGRAY, 0.05f, "Moon", 0.001f,
             0.5f, 0.05f, 0.5f),
  };

  Vector3 totalMomentum = {0.0f, 0.0f, 0.0f};
  for (size_t i = 1; i < initialPlanets.size(); i++) {
    totalMomentum.x += initialPlanets[i].mass * initialPlanets[i].velocity.x;
    totalMomentum.y += initialPlanets[i].mass * initialPlanets[i].velocity.y;
    totalMomentum.z += initialPlanets[i].mass * initialPlanets[i].velocity.z;
  }
  initialPlanets[0].velocity.x = -totalMomentum.x / initialPlanets[0].mass;
  initialPlanets[0].velocity.y = -totalMomentum.y / initialPlanets[0].mass;
  initialPlanets[0].velocity.z = -totalMomentum.z / initialPlanets[0].mass;

  for (size_t i = 0; i < initialPlanets.size(); i++) {
    planetModels.push_back(LoadModel(initialPlanets[i].modelPath.c_str()));
  }

  Shader defaultSunShader = planetModels[0].materials[0].shader;

  for (size_t i = 0; i < planetModels.size(); i++) {
    for (int m = 0; m < planetModels[i].materialCount; m++) {
      planetModels[i].materials[m].shader = lightShader;
    }
  }

  activePlanets = initialPlanets;
  SetTargetFPS(180);

  InitAudioDevice();
  Music ambientMusic = LoadMusicStream("assets/ambient_space.mp3");
  ambientMusic.looping = true;
  SetMusicVolume(ambientMusic, 0.025f);
  PlayMusicStream(ambientMusic);

  EngineState currentState = PAUSED;
  Planet *selectedPlanet = nullptr;
  float cameraSpeed = 20.0f;
  bool isTracking = false;
  const float G = 1.0f;

  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    UpdateMusicStream(ambientMusic);

    // Controls
    if (IsKeyPressed(KEY_SPACE)) {
      currentState = (currentState == PLAYING) ? PAUSED : PLAYING;
    }
    if (IsKeyPressed(KEY_R)) {
      selectedPlanet = nullptr;
      isTracking = false;
      currentState = PAUSED;
      activePlanets = initialPlanets;
      activeFragments.clear();
    }

    ProcessDesktopInput(camera, cameraSpeed, activePlanets, selectedPlanet, isTracking, dt);

    if (isTracking && selectedPlanet != nullptr && selectedPlanet->isAlive) {
      Vector3 desiredTarget = selectedPlanet->position;
      Vector3 currentOffset = Vector3Subtract(camera.position, selectedPlanet->position);
      float targetDist = selectedPlanet->radius * 8.0f;
      if (targetDist < 15.0f) targetDist = 15.0f;

      Vector3 dir = Vector3Normalize(currentOffset);
      if (Vector3Length(currentOffset) < 0.1f) dir = Vector3{0.0f, 0.5f, 0.8f};

      Vector3 desiredPosition = Vector3Add(selectedPlanet->position, Vector3Scale(dir, targetDist));
      camera.position = Vector3Lerp(camera.position, desiredPosition, dt * 3.5f);
      camera.target = Vector3Lerp(camera.target, desiredTarget, dt * 5.0f);
    } else if (isTracking) {
      isTracking = false;
    }

    // Physics Pipeline
    if (currentState == PLAYING) {
      const int SUB_STEPS = 10;
      float subDt = dt / SUB_STEPS;

      for (int step = 0; step < SUB_STEPS; step++) {
        for (size_t i = 0; i < activePlanets.size(); i++) {
          for (size_t j = i + 1; j < activePlanets.size(); j++) {
            if (!activePlanets[i].isAlive || !activePlanets[j].isAlive) continue;
            ApplyGravity(activePlanets[i], activePlanets[i].mass, activePlanets[j], G, subDt);
            ApplyGravity(activePlanets[j], activePlanets[j].mass, activePlanets[i], G, subDt);
          }
        }
        ProcessCollisions(activePlanets, activeFragments, selectedPlanet, isTracking);
      }

      for (size_t i = 0; i < activePlanets.size(); i++) {
        if (!activePlanets[i].isAlive) continue;
        UpdatePosition(activePlanets[i], dt);
      }
    }

    float camPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(lightShader, viewPosLoc, camPos, SHADER_UNIFORM_VEC3);

    // Rendering
    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D(camera);

    for (size_t i = 0; i < activePlanets.size(); i++) {
      if (!activePlanets[i].isAlive) continue;

      Vector3 modelScale = {activePlanets[i].radius, activePlanets[i].radius, activePlanets[i].radius};

      if (currentState == PLAYING) {
        activePlanets[i].rotationAngle += activePlanets[i].rotationSpeed * dt * RAD2DEG;
      }

      if (i == 0) {
        for (int m = 0; m < planetModels[i].materialCount; m++)
          planetModels[i].materials[m].shader = defaultSunShader;
      }

      DrawModelEx(planetModels[i], activePlanets[i].position,
                  Vector3{0.0f, 1.0f, 0.0f}, activePlanets[i].rotationAngle,
                  modelScale, WHITE);

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

    DrawSelectionReticle(selectedPlanet, camera);
    DrawDebugOverlay(screenWidth, screenHeight, selectedPlanet, currentState, activePlanets);
    DrawHelpBar(screenHeight);

    EndDrawing();
  }

  // Cleanup
  UnloadMusicStream(ambientMusic);
  CloseAudioDevice();
  for (size_t i = 0; i < planetModels.size(); i++) UnloadModel(planetModels[i]);
  UnloadShader(lightShader);
  CloseWindow();
  return 0;
}
