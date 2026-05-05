#include "Input.h"
#include "raygui.h"
#include <cmath>

void ProcessMobileInput(int screenWidth, int screenHeight,
                        Camera3D &camera, MobileInputState &input,
                        float effectiveCameraSpeed, bool &isTracking,
                        bool isCameraActive, float dt) {
  if (isCameraActive) return;

  const float JS_BASE_RADIUS = 156.0f;
  const float JS_THUMB_RADIUS = 54.0f;
  const float JS_DEAD_ZONE = 0.10f;
  const float MARGIN = 96.0f;

  Vector2 jsBase = {MARGIN + JS_BASE_RADIUS,
                    (float)screenHeight - MARGIN - JS_BASE_RADIUS};

  // Elevation control
  Rectangle btnUp = {(float)screenWidth - 192.0f,
                     (float)screenHeight - 432.0f, 144.0f, 120.0f};
  Rectangle btnDown = {(float)screenWidth - 192.0f,
                       (float)screenHeight - 264.0f, 144.0f, 120.0f};

  Vector3 camForward =
      Vector3Normalize(Vector3Subtract(camera.target, camera.position));
  Vector3 camRight =
      Vector3Normalize(Vector3CrossProduct(camForward, camera.up));
  Vector3 worldUp = {0.0f, 1.0f, 0.0f};
  float moveAmount = effectiveCameraSpeed * dt;

  int tc = GetTouchPointCount();

  auto isRectPressed = [&](Rectangle rect) -> bool {
    if (tc > 0) {
      for (int t = 0; t < tc; t++)
        if (CheckCollisionPointRec(GetTouchPosition(t), rect))
          return true;
      return false;
    }
    return IsMouseButtonDown(MOUSE_LEFT_BUTTON) &&
           CheckCollisionPointRec(GetMousePosition(), rect);
  };

  if (isRectPressed(btnUp)) {
    isTracking = false;
    camera.position = Vector3Add(camera.position, Vector3Scale(worldUp, moveAmount));
    camera.target = Vector3Add(camera.target, Vector3Scale(worldUp, moveAmount));
  }
  if (isRectPressed(btnDown)) {
    isTracking = false;
    camera.position = Vector3Subtract(camera.position, Vector3Scale(worldUp, moveAmount));
    camera.target = Vector3Subtract(camera.target, Vector3Scale(worldUp, moveAmount));
  }

  bool currentJoystickHeld = false;
  bool currentLookHeld = false;

  // Exclusion Zones
  Rectangle leftPanelRec = {10.0f, 10.0f, 539.0f, 320.0f};
  Rectangle rightPanelRec = {(float)screenWidth - 516.0f, 10.0f, 506.0f, 146.0f};
  Rectangle playPauseRec = {(float)screenWidth / 2.0f - 140.0f,
                            (float)screenHeight - 245.0f, 280.0f, 130.0f};

  if (tc > 0) {
    for (int t = 0; t < tc; t++) {
      Vector2 tp = GetTouchPosition(t);
      int tid = GetTouchPointId(t);

      if (CheckCollisionPointRec(tp, btnUp) ||
          CheckCollisionPointRec(tp, btnDown) ||
          CheckCollisionPointRec(tp, leftPanelRec) ||
          CheckCollisionPointRec(tp, rightPanelRec) ||
          CheckCollisionPointRec(tp, playPauseRec))
        continue;

      if (input.joystickActive && tid == input.joystickTouchPointId) {
        currentJoystickHeld = true;
        float dx = tp.x - input.joystickCenter.x;
        float dy = tp.y - input.joystickCenter.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > JS_BASE_RADIUS) {
          dx = (dx / dist) * JS_BASE_RADIUS;
          dy = (dy / dist) * JS_BASE_RADIUS;
        }
        input.joystickThumb = {input.joystickCenter.x + dx, input.joystickCenter.y + dy};
      } else if (input.lookActive && tid == input.lookTouchPointId) {
        currentLookHeld = true;
        isTracking = false;
        Vector2 delta = {tp.x - input.lastLookPos.x, tp.y - input.lastLookPos.y};
        input.lastLookPos = tp;

        camForward = Vector3RotateByAxisAngle(camForward, worldUp, -delta.x * 0.004f);
        camRight = Vector3Normalize(Vector3CrossProduct(camForward, worldUp));
        Vector3 newForward = Vector3RotateByAxisAngle(camForward, camRight, -delta.y * 0.004f);
        if (Vector3DotProduct(newForward, worldUp) < 0.95f &&
            Vector3DotProduct(newForward, worldUp) > -0.95f)
          camForward = newForward;
        camera.target = Vector3Add(camera.position, camForward);
      } else if (!input.joystickActive && tp.x < screenWidth / 2.5f &&
                 tp.y > screenHeight / 2.0f) {
        input.joystickActive = true;
        input.joystickTouchPointId = tid;
        input.joystickCenter = tp;
        input.joystickThumb = tp;
        currentJoystickHeld = true;
      } else if (!input.lookActive && tp.x >= screenWidth / 2.5f) {
        input.lookActive = true;
        input.lookTouchPointId = tid;
        input.lastLookPos = tp;
        currentLookHeld = true;
      }
    }
  } else if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
    Vector2 mp = GetMousePosition();
    if (!CheckCollisionPointRec(mp, leftPanelRec) &&
        !CheckCollisionPointRec(mp, rightPanelRec) &&
        !CheckCollisionPointRec(mp, playPauseRec) &&
        !CheckCollisionPointRec(mp, btnUp) &&
        !CheckCollisionPointRec(mp, btnDown)) {

      if (mp.x < screenWidth / 2.5f && mp.y > screenHeight / 2.0f) {
        if (!input.joystickActive) {
          input.joystickActive = true;
          currentJoystickHeld = true;
          input.joystickCenter = mp;
        } else {
          currentJoystickHeld = true;
        }
        float dx = mp.x - input.joystickCenter.x;
        float dy = mp.y - input.joystickCenter.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > JS_BASE_RADIUS) {
          dx = (dx / dist) * JS_BASE_RADIUS;
          dy = (dy / dist) * JS_BASE_RADIUS;
        }
        input.joystickThumb = {input.joystickCenter.x + dx, input.joystickCenter.y + dy};
      }
    }
  }

  if (!currentJoystickHeld) {
    input.joystickActive = false;
    input.joystickTouchPointId = -1;
    input.joystickThumb = jsBase;
  }
  if (!currentLookHeld) {
    input.lookActive = false;
    input.lookTouchPointId = -1;
  }

  if (input.joystickActive) {
    float normX = (input.joystickThumb.x - input.joystickCenter.x) / JS_BASE_RADIUS;
    float normY = (input.joystickThumb.y - input.joystickCenter.y) / JS_BASE_RADIUS;
    if (fabsf(normX) > JS_DEAD_ZONE) {
      isTracking = false;
      camera.position = Vector3Add(camera.position, Vector3Scale(camRight, normX * moveAmount));
      camera.target = Vector3Add(camera.target, Vector3Scale(camRight, normX * moveAmount));
    }
    if (fabsf(normY) > JS_DEAD_ZONE) {
      isTracking = false;
      camera.position = Vector3Subtract(camera.position, Vector3Scale(camForward, normY * moveAmount));
      camera.target = Vector3Subtract(camera.target, Vector3Scale(camForward, normY * moveAmount));
    }
  }

  // Draw joystick visuals
  DrawCircleV(jsBase, JS_BASE_RADIUS, {50, 50, 50, 120});
  DrawCircleLinesV(jsBase, JS_BASE_RADIUS, {255, 255, 255, 160});
  DrawCircleV(input.joystickActive ? input.joystickThumb : jsBase, JS_THUMB_RADIUS,
              input.joystickActive ? Color{255, 255, 255, 220} : Color{160, 160, 160, 170});
  DrawText("MOVE", (int)(jsBase.x - 36), (int)(jsBase.y - JS_BASE_RADIUS - 42), 29, LIGHTGRAY);

  GuiSetStyle(DEFAULT, TEXT_SIZE, 28);
  GuiSetAlpha(0.75f);
  GuiButton(btnUp, "^ FLY UP");
  GuiButton(btnDown, "v FLY DN");
  GuiSetAlpha(1.0f);
  GuiSetStyle(DEFAULT, TEXT_SIZE, 10);

  if (!input.lookActive && tc == 0)
    DrawText("[ Drag Right Half to Look Around ]", screenWidth - 420, screenHeight / 2, 22, {255, 255, 255, 100});
}
