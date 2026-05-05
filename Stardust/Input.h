#pragma once
#include "HUD.h"
#include "raylib.h"
#include "raymath.h"

// Mobile touch input state
struct MobileInputState {
  bool joystickActive = false;
  Vector2 joystickCenter = {0.0f, 0.0f};
  Vector2 joystickThumb = {0.0f, 0.0f};
  int joystickTouchPointId = -1;

  bool lookActive = false;
  Vector2 lastLookPos = {0.0f, 0.0f};
  int lookTouchPointId = -1;
};

// Process and render mobile joystick + look controls + elevation buttons
void ProcessMobileInput(int screenWidth, int screenHeight,
                        Camera3D &camera, MobileInputState &input,
                        float effectiveCameraSpeed, bool &isTracking,
                        bool isCameraActive, float dt);
