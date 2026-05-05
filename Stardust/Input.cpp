#include "Input.h"
#include "raymath.h"
#include <cmath>

void ProcessDesktopInput(Camera3D &camera, float &cameraSpeed,
                         std::vector<Planet> &activePlanets,
                         Planet *&selectedPlanet, bool &isTracking, float dt) {
  // Camera zoom / speed modifier
  float wheel = GetMouseWheelMove();
  if (wheel != 0.0f) {
    cameraSpeed += wheel * 5.0f;
    if (cameraSpeed < 1.0f) cameraSpeed = 1.0f;
    if (cameraSpeed > 100.0f) cameraSpeed = 100.0f;
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

    // WASD Movement
    float moveAmount = cameraSpeed * dt;
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
  }

  // Mouse Picking Selection (LMB)
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
    Ray mouseRay = GetMouseRay(GetMousePosition(), camera);
    float closestDistance = 999999.0f;
    int closestIndex = -1;

    for (size_t i = 0; i < activePlanets.size(); i++) {
      if (!activePlanets[i].isAlive) continue;
      RayCollision collision = GetRayCollisionSphere(
          mouseRay, activePlanets[i].position, activePlanets[i].radius);
      if (collision.hit && collision.distance < closestDistance) {
        closestDistance = collision.distance;
        closestIndex = (int)i;
      }
    }

    if (closestIndex >= 0) {
      selectedPlanet = &activePlanets[closestIndex];
      isTracking = true;
    } else {
      selectedPlanet = nullptr;
      isTracking = false;
    }
  }
}
