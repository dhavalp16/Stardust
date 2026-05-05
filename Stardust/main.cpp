// Mobile+Android build. For clean desktop engine, see desktop branch.
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
#include "JNIBridge.h"
#include "Physics.h"
#include "Planet.h"

#if defined(PLATFORM_ANDROID)
#include "raymob.h"
#endif

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

int main() {
  const int screenWidth = 1920;
  const int screenHeight = 1080;

  InitWindow(screenWidth, screenHeight, "Stardust");

  // Load 3D lighting shaders
#if defined(PLATFORM_ANDROID)
  Shader lightShader = LoadShader("resources/shaders/glsl100/lighting.vs",
                                  "resources/shaders/glsl100/lighting.fs");
#else
  Shader lightShader = LoadShader("resources/shaders/glsl330/lighting.vs",
                                  "resources/shaders/glsl330/lighting.fs");
#endif

  int viewPosLoc = GetShaderLocation(lightShader, "viewPos");

  // Create point light at the Sun's position
  Light sun = CreateLight(LIGHT_POINT, Vector3{0.0f, 0.0f, 0.0f}, Vector3Zero(),
                          WHITE, lightShader);

  Camera3D camera = {};
  camera.position = Vector3{0.0f, 160.0f, 200.0f};
  camera.target = Vector3{0.0f, 0.0f, 0.0f};
  camera.up = Vector3{0.0f, 1.0f, 0.0f};
  camera.fovy = 45.0f;
  camera.projection = CAMERA_PERSPECTIVE;

  // ── Planet Storage ─────────────────────────────────────────────────
  std::vector<Planet> initialPlanets;
  std::vector<Planet> activePlanets;
  std::vector<Model> planetModels;

  initialPlanets.reserve(16);
  activePlanets.reserve(16);
  planetModels.reserve(16);

  // Fragment pool for collision effects
  std::vector<Fragment> activeFragments;
  activeFragments.reserve(1024);

  // ── Solar System Initialization ────────────────────────────────────
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

  // Conservation of momentum to fix system drift
  Vector3 totalMomentum = {0.0f, 0.0f, 0.0f};
  for (size_t i = 1; i < initialPlanets.size(); i++) {
    totalMomentum.x += initialPlanets[i].mass * initialPlanets[i].velocity.x;
    totalMomentum.y += initialPlanets[i].mass * initialPlanets[i].velocity.y;
    totalMomentum.z += initialPlanets[i].mass * initialPlanets[i].velocity.z;
  }
  initialPlanets[0].velocity.x = -totalMomentum.x / initialPlanets[0].mass;
  initialPlanets[0].velocity.y = -totalMomentum.y / initialPlanets[0].mass;
  initialPlanets[0].velocity.z = -totalMomentum.z / initialPlanets[0].mass;

  // Load Planet Models
  for (size_t i = 0; i < initialPlanets.size(); i++) {
    planetModels.push_back(LoadModel(initialPlanets[i].modelPath.c_str()));
  }

  Shader defaultSunShader = planetModels[0].materials[0].shader;

  // Setup generic lighting for planets
  for (size_t i = 0; i < planetModels.size(); i++) {
    for (int m = 0; m < planetModels[i].materialCount; m++) {
      planetModels[i].materials[m].shader = lightShader;
    }
  }

  activePlanets = initialPlanets;
  SetTargetFPS(180);

  // ── Ambient space music ────────────────────────────────────────────
  InitAudioDevice();
  Music ambientMusic = LoadMusicStream("assets/ambient_space.mp3");
  ambientMusic.looping = true;
  SetMusicVolume(ambientMusic, 0.025f);
  PlayMusicStream(ambientMusic);

  // ── Engine State ───────────────────────────────────────────────────
  EngineState currentState = PAUSED;
  Planet *selectedPlanet = nullptr;
  bool isCameraActive = false;
  float cameraSpeed = 20.0f;
  bool isTracking = false;
  float settledMass = -1.0f;
  bool welcomeSent = false;

  MobileInputState mobileInput;
  Planet *prevSelectedPlanet = nullptr;

  // Toast notification state
  float toastTimer = 0.0f;
  char toastText[192] = {0};
  float prevSliderMass = -1.0f;
  float prevSliderRadius = -1.0f;

  const float G = 1.0f;

  // ══════════════════════════════════════════════════════════════════
  //  MAIN GAME LOOP
  // ══════════════════════════════════════════════════════════════════
  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    UpdateMusicStream(ambientMusic);
    PollAINarration();

    // ── Welcome narration on first frame ────────────────────────────
    if (!welcomeSent) {
      welcomeSent = true;
      RequestAINarration(
          "The simulation just started. There are 10 celestial bodies: Sun, "
          "Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, Neptune, and "
          "Moon. "
          "This is a scaled simulation where 1 minute equals roughly 5 Earth "
          "years. "
          "Planets orbit faster than reality for educational purposes. "
          "Welcome the student and briefly explain the time scale in a fun "
          "way. Keep it to 2 sentences max.");
    }

    // ── Dynamic Camera Speed ─────────────────────────────────────────
    float distanceToNearest = 99999.0f;
    for (size_t i = 0; i < activePlanets.size(); i++) {
      if (!activePlanets[i].isAlive)
        continue;
      float d = Vector3Distance(camera.position, activePlanets[i].position);
      if (d < distanceToNearest)
        distanceToNearest = d;
    }

    float speedMultiplier = 1.0f;
    if (distanceToNearest > 50.0f) {
      speedMultiplier = distanceToNearest / 50.0f;
    } else if (distanceToNearest < 20.0f) {
      speedMultiplier = distanceToNearest / 20.0f;
      if (speedMultiplier < 0.1f)
        speedMultiplier = 0.1f;
    }
    float effectiveCameraSpeed = cameraSpeed * speedMultiplier;

    // ── Planet Tracking ──────────────────────────────────────────────
    if (isTracking && selectedPlanet != nullptr && selectedPlanet->isAlive) {
      Vector3 desiredTarget = selectedPlanet->position;
      Vector3 currentOffset =
          Vector3Subtract(camera.position, selectedPlanet->position);
      float targetDist = selectedPlanet->radius * 8.0f;
      if (targetDist < 15.0f)
        targetDist = 15.0f;

      Vector3 dir = Vector3Normalize(currentOffset);
      if (Vector3Length(currentOffset) < 0.1f)
        dir = Vector3{0.0f, 0.5f, 0.8f};

      Vector3 desiredPosition =
          Vector3Add(selectedPlanet->position, Vector3Scale(dir, targetDist));
      camera.position =
          Vector3Lerp(camera.position, desiredPosition, dt * 3.5f);
      camera.target = Vector3Lerp(camera.target, desiredTarget, dt * 5.0f);
    } else if (isTracking) {
      isTracking = false;
    }

    // ── Desktop Camera Controls ──────────────────────────────────────
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      if (!isCameraActive) {
        DisableCursor();
        isCameraActive = true;
      }
      UpdateCamera(&camera, CAMERA_FREE);

      Vector3 forward =
          Vector3Normalize(Vector3Subtract(camera.target, camera.position));
      Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
      float moveAmount = effectiveCameraSpeed * dt;

      if (IsKeyDown(KEY_W)) {
        isTracking = false;
        camera.position =
            Vector3Add(camera.position, Vector3Scale(forward, moveAmount));
        camera.target =
            Vector3Add(camera.target, Vector3Scale(forward, moveAmount));
      }
      if (IsKeyDown(KEY_S)) {
        isTracking = false;
        camera.position =
            Vector3Subtract(camera.position, Vector3Scale(forward, moveAmount));
        camera.target =
            Vector3Subtract(camera.target, Vector3Scale(forward, moveAmount));
      }
      if (IsKeyDown(KEY_D)) {
        isTracking = false;
        camera.position =
            Vector3Add(camera.position, Vector3Scale(right, moveAmount));
        camera.target =
            Vector3Add(camera.target, Vector3Scale(right, moveAmount));
      }
      if (IsKeyDown(KEY_A)) {
        isTracking = false;
        camera.position =
            Vector3Subtract(camera.position, Vector3Scale(right, moveAmount));
        camera.target =
            Vector3Subtract(camera.target, Vector3Scale(right, moveAmount));
      }

      float wheel = GetMouseWheelMove();
      if (wheel != 0.0f) {
        cameraSpeed += wheel * 1.0f;
        if (cameraSpeed < 0.5f)
          cameraSpeed = 0.5f;
        if (cameraSpeed > 50.0f)
          cameraSpeed = 50.0f;
      }
    } else {
      if (isCameraActive) {
        EnableCursor();
        isCameraActive = false;
      }
    }

    // ── Toggle Simulation ────────────────────────────────────────────
    if (IsKeyPressed(KEY_SPACE)) {
      if (currentState == PAUSED)
        currentState = PLAYING;
      else if (currentState == PLAYING)
        currentState = PAUSED;
    }

    // ── Mouse Picking ────────────────────────────────────────────────
    {
      Rectangle pickGuardLeft = {10.0f, 10.0f, 539.0f, 320.0f};
      Rectangle pickGuardRight = {(float)screenWidth - 516.0f, 10.0f, 506.0f,
                                  146.0f};
      Vector2 mpos = GetMousePosition();
      bool overUI = CheckCollisionPointRec(mpos, pickGuardLeft) ||
                    CheckCollisionPointRec(mpos, pickGuardRight);

      if (!isCameraActive && !overUI &&
          IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && GetMouseY() > 120 &&
          GetMouseY() < (screenHeight - 280)) {
        Ray mouseRay = GetMouseRay(mpos, camera);
        float closestDistance = 999999.0f;
        int closestIndex = -1;

        for (size_t i = 0; i < activePlanets.size(); i++) {
          RayCollision collision = GetRayCollisionSphere(
              mouseRay, activePlanets[i].position, activePlanets[i].radius);
          if (collision.hit && collision.distance < closestDistance) {
            closestDistance = collision.distance;
            closestIndex = (int)i;
          }
        }

        if (closestIndex >= 0)
          selectedPlanet = &activePlanets[closestIndex];
        else
          selectedPlanet = nullptr;
      }
    }

    // ── Physics Pipeline ─────────────────────────────────────────────
    if (currentState == PLAYING) {
      const int SUB_STEPS = 10;
      float subDt = dt / SUB_STEPS;

      for (int step = 0; step < SUB_STEPS; step++) {
        // N-Body Gravity
        for (size_t i = 0; i < activePlanets.size(); i++) {
          for (size_t j = i + 1; j < activePlanets.size(); j++) {
            if (!activePlanets[i].isAlive || !activePlanets[j].isAlive)
              continue;
            ApplyGravity(activePlanets[i], activePlanets[i].mass,
                         activePlanets[j], G, subDt);
            ApplyGravity(activePlanets[j], activePlanets[j].mass,
                         activePlanets[i], G, subDt);
          }
        }

        // Collision Detection & Resolution
        ProcessCollisions(activePlanets, activeFragments, selectedPlanet,
                          isTracking);
      }

      // Position Update
      for (size_t i = 0; i < activePlanets.size(); i++) {
        if (!activePlanets[i].isAlive)
          continue;
        UpdatePosition(activePlanets[i], dt);
      }
    }

    // ── Update light shader ──────────────────────────────────────────
    float camPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(lightShader, viewPosLoc, camPos, SHADER_UNIFORM_VEC3);

    // ══════════════════════════════════════════════════════════════════
    //  RENDERING
    // ══════════════════════════════════════════════════════════════════
    BeginDrawing();
    ClearBackground(BLACK);
    BeginMode3D(camera);

    // 3D Planet Rendering
    for (size_t i = 0; i < activePlanets.size(); i++) {
      if (!activePlanets[i].isAlive)
        continue;

      Vector3 modelScale = {activePlanets[i].radius, activePlanets[i].radius,
                            activePlanets[i].radius};

      if (currentState == PLAYING) {
        activePlanets[i].rotationAngle +=
            activePlanets[i].rotationSpeed * dt * RAD2DEG;
      }

      // Emissive Sun Trick
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

    // Fragment Rendering
    for (size_t f = 0; f < activeFragments.size(); f++) {
      if (!activeFragments[f].isAlive)
        continue;

      if (currentState == PLAYING) {
        activeFragments[f].position =
            Vector3Add(activeFragments[f].position,
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
                Vector3{activeFragments[f].size, activeFragments[f].size,
                        activeFragments[f].size},
                renderColor);
    }

    // Pool Cleanup
    activeFragments.erase(
        std::remove_if(activeFragments.begin(), activeFragments.end(),
                       [](const Fragment &f) { return !f.isAlive; }),
        activeFragments.end());

    EndMode3D();

    // ── 2D HUD ───────────────────────────────────────────────────────
    DrawSelectionReticle(selectedPlanet, camera);
    DrawControlsPanel(screenWidth, screenHeight, selectedPlanet, isTracking,
                      currentState, activePlanets, initialPlanets,
                      activeFragments, prevSelectedPlanet, prevSliderMass,
                      prevSliderRadius, settledMass, toastTimer);
    DrawStatusPanel(screenWidth, currentState, cameraSpeed);

    ProcessToastAndNarration(screenWidth, screenHeight, selectedPlanet,
                             prevSelectedPlanet, activePlanets, prevSliderMass,
                             prevSliderRadius, settledMass, toastTimer,
                             toastText, sizeof(toastText), dt);

    DrawToast(screenWidth, toastTimer, toastText, dt);
    DrawHelpBar(screenHeight);
    DrawCaptions(screenWidth, screenHeight, dt);
    DrawKillFeed(screenWidth, dt);

    // Mobile Controls
    ProcessMobileInput(screenWidth, screenHeight, camera, mobileInput,
                       effectiveCameraSpeed, isTracking, isCameraActive, dt);

    DrawPlayPauseButton(screenWidth, screenHeight, currentState);

    EndDrawing();
  }

  // ── Cleanup ────────────────────────────────────────────────────────
  EnableCursor();
  UnloadMusicStream(ambientMusic);
  CloseAudioDevice();
  for (size_t i = 0; i < planetModels.size(); i++)
    UnloadModel(planetModels[i]);
  UnloadShader(lightShader);
  CloseWindow();
  return 0;
}
