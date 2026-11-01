#include "Input.h"
#include "Convert.h"
#include "HudLayout.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>

namespace {

// Scales camera movement by distance to the nearest planet: fast in empty
// space (more than 50 units away), slow near a surface (under 20 units, down
// to a tenth of normal speed), normal in between.
float DistanceSpeedMultiplier(const Camera3D &camera, const std::vector<sim::Body> &bodies) {
  float distanceToNearest = 99999.0f;
  for (const sim::Body &body : bodies) {
    if (!body.isAlive) continue;
    float d = Vector3Distance(camera.position, ToRaylib(body.position));
    if (d < distanceToNearest) distanceToNearest = d;
  }

  if (distanceToNearest > 50.0f) return distanceToNearest / 50.0f;
  if (distanceToNearest < 20.0f) return std::max(distanceToNearest / 20.0f, 0.1f);
  return 1.0f;
}

} // namespace

void ProcessDesktopInput(Camera3D &camera, float &cameraSpeed,
                         const std::vector<sim::Body> &bodies,
                         int &selected, bool &isTracking, float dt) {
  int prevSelected = selected; // to detect a change of selection

  // Camera zoom / speed modifier
  float wheel = GetMouseWheelMove();
  if (wheel != 0.0f) {
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      // RMB + Scroll: Adjust movement speed (Unreal viewport style)
      cameraSpeed += wheel * 5.0f;
      if (cameraSpeed < 1.0f) cameraSpeed = 1.0f;
      if (cameraSpeed > 100.0f) cameraSpeed = 100.0f;
    } else {
      // Scroll only: Zoom along camera forward vector (Unity/Unreal style)
      if (!isTracking) {
        Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));

        float effectiveCameraSpeed = cameraSpeed * DistanceSpeedMultiplier(camera, bodies);

        float zoomAmount = wheel * effectiveCameraSpeed * 0.15f;
        camera.position = Vector3Add(camera.position, Vector3Scale(forward, zoomAmount));
        camera.target = Vector3Add(camera.target, Vector3Scale(forward, zoomAmount));
      }
    }
  }

  // Free Camera (Unreal-style: Hold RMB to Look & Fly)
  if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) DisableCursor();
  if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) EnableCursor();

  if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    Vector2 mouseDelta = GetMouseDelta();
    
    Vector3 forward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
    Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));
    
    // Mouse look
    forward = Vector3RotateByAxisAngle(forward, camera.up, -mouseDelta.x * 0.003f);
    Vector3 newForward = Vector3RotateByAxisAngle(forward, right, -mouseDelta.y * 0.003f);
    
    if (std::abs(Vector3DotProduct(newForward, camera.up)) < 0.95f) {
      forward = newForward;
    }
    camera.target = Vector3Add(camera.position, forward);

    float effectiveCameraSpeed = cameraSpeed * DistanceSpeedMultiplier(camera, bodies);

    // WASD Movement
    float moveAmount = effectiveCameraSpeed * dt;
    if (IsKeyDown(KEY_W)) {
      isTracking = false;
      camera.position = Vector3Add(camera.position, Vector3Scale(forward, moveAmount));
      camera.target = Vector3Add(camera.target, Vector3Scale(forward, moveAmount));
    }
    if (IsKeyDown(KEY_S)) {
      isTracking = false;
      camera.position = Vector3Subtract(camera.position, Vector3Scale(forward, moveAmount));
      camera.target = Vector3Subtract(camera.target, Vector3Scale(forward, moveAmount));
    }
    if (IsKeyDown(KEY_D)) {
      isTracking = false;
      camera.position = Vector3Add(camera.position, Vector3Scale(right, moveAmount));
      camera.target = Vector3Add(camera.target, Vector3Scale(right, moveAmount));
    }
    if (IsKeyDown(KEY_A)) {
      isTracking = false;
      camera.position = Vector3Subtract(camera.position, Vector3Scale(right, moveAmount));
      camera.target = Vector3Subtract(camera.target, Vector3Scale(right, moveAmount));
    }
    if (IsKeyDown(KEY_Q)) {
      isTracking = false;
      camera.position = Vector3Add(camera.position, Vector3Scale(camera.up, moveAmount));
      camera.target = Vector3Add(camera.target, Vector3Scale(camera.up, moveAmount));
    }
    if (IsKeyDown(KEY_E)) {
      isTracking = false;
      camera.position = Vector3Subtract(camera.position, Vector3Scale(camera.up, moveAmount));
      camera.target = Vector3Subtract(camera.target, Vector3Scale(camera.up, moveAmount));
    }
  }

  // Mouse Picking Selection (LMB)
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    Vector2 mpos = GetMousePosition();
    // Clicks on a HUD panel belong to the HUD, not to the planets behind it.
    if (!IsOverHud(mpos, selected >= 0, GetScreenWidth())) {
      Ray mouseRay = GetMouseRay(mpos, camera);
      float closestDistance = 999999.0f;
      int closestIndex = -1;

      for (size_t i = 0; i < bodies.size(); i++) {
        if (!bodies[i].isAlive) continue;
        RayCollision collision = GetRayCollisionSphere(
            mouseRay, ToRaylib(bodies[i].position), bodies[i].radius);
        if (collision.hit && collision.distance < closestDistance) {
          closestDistance = collision.distance;
          closestIndex = (int)i;
        }
      }

      if (closestIndex >= 0) {
        selected = closestIndex;
        // If a DIFFERENT planet was clicked, reset tracking
        if (selected != prevSelected) {
          isTracking = false;
        }
      } else {
        selected = -1;
        isTracking = false;
      }
    }
  }
}
