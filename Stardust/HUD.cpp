#include "HUD.h"
#include "raymath.h"
#include <cstdio>
#include <string>
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

  DrawText("TARGET LOCKED", (int)(p.x + circleRadius + 15), (int)(p.y - 12), 14, hudColor);
  DrawText(selectedPlanet->name.c_str(), (int)(p.x + circleRadius + 15), (int)(p.y + 6), 20, WHITE);
}

void DrawDebugOverlay(int screenWidth, int screenHeight,
                      Planet *selectedPlanet, EngineState currentState,
                      const std::vector<Planet> &activePlanets,
                      bool &isTracking) {
  // Top-left: Selected Planet Info
  if (selectedPlanet != nullptr) {
    DrawRectangleRounded({10, 10, 310, 220}, 0.1f, 8, Color{10, 10, 10, 200});
    DrawText(selectedPlanet->name.c_str(), 20, 20, 22, YELLOW);
    
    char massText[64];
    snprintf(massText, sizeof(massText), "Mass: %.2f", selectedPlanet->mass);
    DrawText(massText, 20, 52, 18, WHITE);

    float velMag = Vector3Length(selectedPlanet->velocity);
    char velText[64];
    snprintf(velText, sizeof(velText), "Velocity: %.2f", velMag);
    DrawText(velText, 20, 76, 18, WHITE);

    GuiSlider({75, 108, 220, 22}, "Mass", TextFormat("%.2f", selectedPlanet->mass), 
              &selectedPlanet->mass, selectedPlanet->massMin, selectedPlanet->massMax);
    GuiSlider({75, 142, 220, 22}, "Radius", TextFormat("%.2f", selectedPlanet->radius), 
              &selectedPlanet->radius, selectedPlanet->radiusMin, selectedPlanet->radiusMax);

    if (GuiButton({20, 178, 280, 34}, isTracking ? "STOP TRACKING" : "TRACK PLANET")) {
      isTracking = !isTracking;
    }
  } else {
    DrawRectangleRounded({10, 10, 310, 55}, 0.1f, 8, Color{10, 10, 10, 200});
    DrawText("No planet selected", 20, 25, 22, LIGHTGRAY);
  }

  // Top-right: General Engine State
  DrawRectangleRounded({(float)screenWidth - 260, 10, 250, 110}, 0.1f, 8, Color{10, 10, 10, 200});
  DrawText(TextFormat("FPS: %d", GetFPS()), screenWidth - 240, 22, 22, GREEN);
  
  int aliveCount = 0;
  for (const auto& p : activePlanets) {
    if (p.isAlive) aliveCount++;
  }
  DrawText(TextFormat("Planets: %d", aliveCount), screenWidth - 240, 54, 18, WHITE);
  
  const char* stateText = (currentState == PLAYING) ? "PLAYING" : "PAUSED";
  Color stateColor = (currentState == PLAYING) ? GREEN : RED;
  DrawText(stateText, screenWidth - 240, 80, 18, stateColor);
}

void DrawHelpBar(int screenHeight) {
  DrawText("SPACE: Play/Pause | WASD+RMB: Fly | LMB: Select | R: Reset",
           10, screenHeight - 32, 17, Color{150, 150, 150, 255});
}

struct KeyEntry {
  std::string key;
  float life;
  bool held;
};

void DrawKeyPressOverlay(int screenWidth, int screenHeight) {
  static std::vector<KeyEntry> entries;
  static float prevScroll = 0.0f;

  float dt = GetFrameTime();

  auto setHeld = [&](const std::string &name, bool isHeld) {
    for (auto &e : entries) {
      if (e.key == name) {
        if (isHeld) {
          e.held = true;
          e.life = 0.0f;
        } else if (e.held) {
          e.held = false;
          e.life = 2.0f;
        }
        return;
      }
    }
    if (isHeld)
      entries.push_back({name, 0.0f, true});
  };

  setHeld("SPACE", IsKeyDown(KEY_SPACE));
  setHeld("W", IsKeyDown(KEY_W));
  setHeld("A", IsKeyDown(KEY_A));
  setHeld("S", IsKeyDown(KEY_S));
  setHeld("D", IsKeyDown(KEY_D));
  setHeld("Q", IsKeyDown(KEY_Q));
  setHeld("E", IsKeyDown(KEY_E));
  setHeld("R", IsKeyDown(KEY_R));
  setHeld("LMB", IsMouseButtonDown(MOUSE_BUTTON_LEFT));
  setHeld("RMB", IsMouseButtonDown(MOUSE_BUTTON_RIGHT));

  float scroll = GetMouseWheelMove();
  if (scroll != 0.0f && prevScroll == 0.0f) {
    bool found = false;
    for (auto &e : entries) {
      if (e.key == "SCROLL") { e.held = false; e.life = 2.0f; found = true; break; }
    }
    if (!found) entries.push_back({"SCROLL", 2.0f, false});
  }
  prevScroll = scroll;

  // Update lifetimes for released keys only
  for (auto &e : entries) {
    if (!e.held) e.life -= dt;
  }

  // Remove fully faded released keys
  entries.erase(
    std::remove_if(entries.begin(), entries.end(),
                    [](const KeyEntry &e) { return !e.held && e.life <= 0.0f; }),
    entries.end());

  if (entries.empty()) return;

  // Clamp to newest 12
  int startIdx = 0;
  int showCount = (int)entries.size();
  if (showCount > 12) { startIdx = showCount - 12; showCount = 12; }

  float entryH = 30.0f;
  float padX = 14.0f, padY = 10.0f;
  float totalH = showCount * entryH + padY * 2.0f;

  float maxW = 0.0f;
  for (int i = startIdx; i < (int)entries.size(); i++) {
    float tw = (float)MeasureText(entries[i].key.c_str(), 18);
    if (tw > maxW) maxW = tw;
  }
  float totalW = maxW + padX * 2.0f;

  float boxX = (float)screenWidth - totalW - 20.0f;
  float boxY = (float)screenHeight - totalH - 40.0f;

  DrawRectangleRounded({boxX, boxY, totalW, totalH}, 0.12f, 6, Color{0, 0, 0, 160});
  DrawRectangleRoundedLines({boxX, boxY, totalW, totalH}, 0.12f, 6, Color{80, 80, 100, 180});

  for (int i = startIdx; i < (int)entries.size(); i++) {
    auto &e = entries[i];
    float alpha = (e.held) ? 1.0f : ((e.life < 0.5f) ? (e.life / 0.5f) : 1.0f);
    unsigned char a = (unsigned char)(alpha * 255.0f);
    int idx = i - startIdx;
    float yPos = boxY + padY + idx * entryH;
    DrawText(e.key.c_str(), (int)(boxX + padX), (int)(yPos + 4), 18,
             Color{255, 255, 100, a});
  }
}
