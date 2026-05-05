#include "HUD.h"
#include "raymath.h"
#include <cstdio>
#include "raygui.h"

void DrawSelectionReticle(Planet *selectedPlanet, Camera3D &camera) {
  if (selectedPlanet == nullptr) return;

  Vector2 p = GetWorldToScreen(selectedPlanet->position, camera);
  Vector3 edgeWorld = Vector3Add(selectedPlanet->position,
                                 Vector3Scale(camera.up, selectedPlanet->radius * 1.5f));
  Vector2 edgeScreen = GetWorldToScreen(edgeWorld, camera);
  float circleRadius = Vector2Distance(p, edgeScreen);
  if (circleRadius < 25.0f) circleRadius = 25.0f;
  if (circleRadius > 300.0f) circleRadius = 300.0f;

  float rot = GetTime() * 80.0f;
  Color hudColor = {0, 220, 255, 210};

  DrawRing(p, circleRadius, circleRadius + 2.0f, rot, rot + 45.0f, 16, hudColor);
  DrawRing(p, circleRadius, circleRadius + 2.0f, rot + 90.0f, rot + 135.0f, 16, hudColor);
  DrawRing(p, circleRadius, circleRadius + 2.0f, rot + 180.0f, rot + 225.0f, 16, hudColor);
  DrawRing(p, circleRadius, circleRadius + 2.0f, rot + 270.0f, rot + 315.0f, 16, hudColor);

  DrawLineEx({p.x - circleRadius - 15, p.y}, {p.x - circleRadius - 5, p.y}, 2.0f, hudColor);
  DrawLineEx({p.x + circleRadius + 5, p.y}, {p.x + circleRadius + 15, p.y}, 2.0f, hudColor);
  DrawLineEx({p.x, p.y - circleRadius - 15}, {p.x, p.y - circleRadius - 5}, 2.0f, hudColor);
  DrawLineEx({p.x, p.y + circleRadius + 5}, {p.x, p.y + circleRadius + 15}, 2.0f, hudColor);

  DrawText("TARGET LOCKED", (int)(p.x + circleRadius + 15), (int)(p.y - 10), 12, hudColor);
  DrawText(selectedPlanet->name.c_str(), (int)(p.x + circleRadius + 15), (int)(p.y + 4), 18, WHITE);
}

void DrawDebugOverlay(int screenWidth, int screenHeight,
                      Planet *selectedPlanet, EngineState currentState,
                      const std::vector<Planet> &activePlanets,
                      bool &isTracking) {
  // Top-left: Selected Planet Info
  if (selectedPlanet != nullptr) {
    DrawRectangleRounded({10, 10, 300, 200}, 0.1f, 8, Color{10, 10, 10, 200});
    DrawText(selectedPlanet->name.c_str(), 20, 20, 20, YELLOW);
    
    char massText[64];
    snprintf(massText, sizeof(massText), "Mass: %.2f", selectedPlanet->mass);
    DrawText(massText, 20, 50, 16, WHITE);

    float velMag = Vector3Length(selectedPlanet->velocity);
    char velText[64];
    snprintf(velText, sizeof(velText), "Velocity: %.2f", velMag);
    DrawText(velText, 20, 70, 16, WHITE);

    GuiSlider({70, 100, 200, 20}, "Mass", TextFormat("%.2f", selectedPlanet->mass), 
              &selectedPlanet->mass, selectedPlanet->massMin, selectedPlanet->massMax);
    GuiSlider({70, 130, 200, 20}, "Radius", TextFormat("%.2f", selectedPlanet->radius), 
              &selectedPlanet->radius, selectedPlanet->radiusMin, selectedPlanet->radiusMax);

    if (GuiButton({20, 160, 260, 30}, isTracking ? "STOP TRACKING" : "TRACK PLANET")) {
      isTracking = !isTracking;
    }
  } else {
    DrawRectangleRounded({10, 10, 300, 50}, 0.1f, 8, Color{10, 10, 10, 200});
    DrawText("No planet selected", 20, 25, 20, LIGHTGRAY);
  }

  // Top-right: General Engine State
  DrawRectangleRounded({(float)screenWidth - 250, 10, 240, 100}, 0.1f, 8, Color{10, 10, 10, 200});
  DrawText(TextFormat("FPS: %d", GetFPS()), screenWidth - 230, 20, 20, GREEN);
  
  int aliveCount = 0;
  for (const auto& p : activePlanets) {
    if (p.isAlive) aliveCount++;
  }
  DrawText(TextFormat("Planets: %d", aliveCount), screenWidth - 230, 50, 16, WHITE);
  
  const char* stateText = (currentState == PLAYING) ? "PLAYING" : "PAUSED";
  Color stateColor = (currentState == PLAYING) ? GREEN : RED;
  DrawText(stateText, screenWidth - 230, 70, 16, stateColor);
}

void DrawHelpBar(int screenHeight) {
  DrawText("SPACE: Play/Pause | WASD+RMB: Camera | LMB: Select | R: Reset",
           10, screenHeight - 28, 15, Color{150, 150, 150, 255});
}
