#include "HUD.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "raygui.h"

// ── Selection Reticle ──────────────────────────────────────────────────
void DrawSelectionReticle(Planet *selectedPlanet, Camera3D &camera) {
  if (selectedPlanet == nullptr)
    return;

  Vector2 p = GetWorldToScreen(selectedPlanet->position, camera);

  Vector3 edgeWorld =
      Vector3Add(selectedPlanet->position,
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

// ── Controls Panel ─────────────────────────────────────────────────────
void DrawControlsPanel(int screenWidth, int screenHeight,
                       Planet *&selectedPlanet, bool &isTracking,
                       EngineState &currentState,
                       std::vector<Planet> &activePlanets,
                       std::vector<Planet> &initialPlanets,
                       std::vector<Fragment> &activeFragments,
                       Planet *&prevSelectedPlanet,
                       float &prevSliderMass, float &prevSliderRadius,
                       float &settledMass, float &toastTimer) {
  const float PNL_X = 10.0f, PNL_Y = 10.0f;
  const float PNL_W = 539.0f, PNL_H = 320.0f;
  const float PAD = 19.0f;
  const float SLD_X = PNL_X + 132.0f;
  const float SLD_W = PNL_W - 132.0f - PAD - 81.0f;
  const float ROW_H = 48.0f;

  GuiSetStyle(DEFAULT, TEXT_SIZE, 22);

  DrawRectangleRounded({PNL_X, PNL_Y, PNL_W, PNL_H}, 0.06f, 8, Color{5, 5, 15, 175});
  DrawRectangleRoundedLinesEx({PNL_X, PNL_Y, PNL_W, PNL_H}, 0.06f, 8, 1.5f, Color{55, 55, 85, 210});

  float r1Y = PNL_Y + PAD;
  if (selectedPlanet != nullptr) {
    DrawText(selectedPlanet->name.c_str(), (int)(PNL_X + PAD), (int)r1Y, 31, YELLOW);
    DrawText("SELECTED", (int)(PNL_X + PNL_W - 128.0f), (int)(r1Y + 5), 17, Color{200, 200, 70, 150});
  } else {
    DrawText("Tap a planet to select", (int)(PNL_X + PAD), (int)r1Y, 26, Color{100, 100, 100, 200});
  }

  DrawLineEx({PNL_X + PAD, r1Y + 42.0f}, {PNL_X + PNL_W - PAD, r1Y + 42.0f}, 1.0f, Color{55, 55, 80, 180});

  float r2Y = r1Y + 55.0f;
  if (selectedPlanet != nullptr) {
    GuiSlider({SLD_X, r2Y, SLD_W, ROW_H}, "Mass",
              TextFormat("%.2f", selectedPlanet->mass),
              &selectedPlanet->mass, selectedPlanet->massMin, selectedPlanet->massMax);
  } else {
    GuiSetState(STATE_DISABLED);
    float dummy = 0.5f;
    GuiSlider({SLD_X, r2Y, SLD_W, ROW_H}, "Mass", "--", &dummy, 0.0f, 1.0f);
    GuiSetState(STATE_NORMAL);
  }

  float r3Y = r2Y + ROW_H + 13.0f;
  if (selectedPlanet != nullptr) {
    GuiSlider({SLD_X, r3Y, SLD_W, ROW_H}, "Radius",
              TextFormat("%.2f", selectedPlanet->radius),
              &selectedPlanet->radius, selectedPlanet->radiusMin, selectedPlanet->radiusMax);
  } else {
    GuiSetState(STATE_DISABLED);
    float dummy = 0.5f;
    GuiSlider({SLD_X, r3Y, SLD_W, ROW_H}, "Radius", "--", &dummy, 0.0f, 1.0f);
    GuiSetState(STATE_NORMAL);
  }

  float r4Y = r3Y + ROW_H + 13.0f;
  if (selectedPlanet != nullptr) {
    if (GuiButton({PNL_X + PAD, r4Y, PNL_W - PAD * 2.0f, ROW_H},
                  isTracking ? "STOP TRACKING" : "TRACK PLANET")) {
      isTracking = !isTracking;
    }
  } else {
    GuiSetState(STATE_DISABLED);
    GuiButton({PNL_X + PAD, r4Y, PNL_W - PAD * 2.0f, ROW_H}, "TRACK PLANET");
    GuiSetState(STATE_NORMAL);
  }

  float r5Y = r4Y + ROW_H + 13.0f;
  if (GuiButton({PNL_X + PAD, r5Y, PNL_W - PAD * 2.0f, ROW_H}, "RESET SIMULATION")) {
    selectedPlanet = nullptr;
    prevSelectedPlanet = nullptr;
    prevSliderMass = -1.0f;
    prevSliderRadius = -1.0f;
    settledMass = -1.0f;
    toastTimer = 0.0f;
    isTracking = false;
    currentState = PAUSED;
    activePlanets = initialPlanets;
    activeFragments.clear();
  }

  GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
}

// ── Status Panel ───────────────────────────────────────────────────────
void DrawStatusPanel(int screenWidth, EngineState currentState, float &cameraSpeed) {
  const float RP_W = 506.0f;
  const float RP_X = (float)screenWidth - RP_W - 10.0f;
  const float RP_Y = 10.0f;
  const float RP_H = 146.0f;

  DrawRectangleRounded({RP_X, RP_Y, RP_W, RP_H}, 0.06f, 8, Color{5, 5, 15, 175});
  DrawRectangleRoundedLinesEx({RP_X, RP_Y, RP_W, RP_H}, 0.06f, 8, 1.5f, Color{55, 55, 85, 210});

  bool isPlaying = (currentState == PLAYING);
  DrawCircleV({RP_X + 30.0f, RP_Y + 36.0f}, 12.0f,
              isPlaying ? Color{60, 220, 80, 255} : Color{220, 60, 60, 255});
  DrawText(isPlaying ? "PLAYING" : "PAUSED", (int)(RP_X + 53.0f), (int)(RP_Y + 21.0f), 31,
           isPlaying ? GREEN : RED);

  DrawLineEx({RP_X + 17.0f, RP_Y + 70.0f}, {RP_X + RP_W - 17.0f, RP_Y + 70.0f}, 1.0f, Color{55, 55, 80, 180});

  GuiSetStyle(DEFAULT, TEXT_SIZE, 22);
  GuiSlider({RP_X + 139.0f, RP_Y + 81.0f, RP_W - 160.0f, 44.0f}, "Cam Spd",
            TextFormat("%.1f", cameraSpeed), &cameraSpeed, 0.5f, 50.0f);
  GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
}

// ── Toast & AI Narration on slider release ─────────────────────────────
void ProcessToastAndNarration(int screenWidth, int screenHeight,
                              Planet *selectedPlanet,
                              Planet *&prevSelectedPlanet,
                              std::vector<Planet> &activePlanets,
                              float &prevSliderMass, float &prevSliderRadius,
                              float &settledMass, float &toastTimer,
                              char *toastText, size_t toastTextSize,
                              float dt) {
  if (selectedPlanet != nullptr) {
    if (selectedPlanet != prevSelectedPlanet) {
      prevSelectedPlanet = selectedPlanet;
      prevSliderMass = selectedPlanet->mass;
      prevSliderRadius = selectedPlanet->radius;
      settledMass = selectedPlanet->mass;
    }

    // AI narration on mass slider release
    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && settledMass >= 0.0f &&
        settledMass != selectedPlanet->mass) {
      const double SIM_EARTH_MASS = 10.0;
      const double REAL_EARTH_MASS = 5.972e24;
      double earthRatio = (double)selectedPlanet->mass / SIM_EARTH_MASS;
      double realKg = earthRatio * REAL_EARTH_MASS;

      char aiCtx[512];
      snprintf(aiCtx, sizeof(aiCtx),
               "The user changed %s mass to %.1f times Earth mass (%.3e kg). "
               "Other active planets and distances from %s: ",
               selectedPlanet->name.c_str(), earthRatio, realKg,
               selectedPlanet->name.c_str());
      for (size_t i = 0; i < activePlanets.size(); i++) {
        if (activePlanets[i].isAlive && &activePlanets[i] != selectedPlanet) {
          float d = Vector3Distance(activePlanets[i].position, selectedPlanet->position);
          char buf[80];
          snprintf(buf, sizeof(buf), "%s(%.0f units away), ", activePlanets[i].name.c_str(), d);
          strncat(aiCtx, buf, sizeof(aiCtx) - strlen(aiCtx) - 1);
        }
      }
      strncat(aiCtx,
              ". Explain briefly how this mass change affects the nearby "
              "planets gravitationally. Be accurate.",
              sizeof(aiCtx) - strlen(aiCtx) - 1);
      RequestAINarration(aiCtx);

      settledMass = selectedPlanet->mass;
    }

    if (prevSliderMass != selectedPlanet->mass) {
      prevSliderMass = selectedPlanet->mass;
      toastTimer = 3.0f;

      const double SIM_EARTH_MASS = 10.0;
      const double REAL_EARTH_MASS = 5.972e24;
      double earthRatio = (double)selectedPlanet->mass / SIM_EARTH_MASS;
      double realKg = earthRatio * REAL_EARTH_MASS;

      snprintf(toastText, toastTextSize,
               "%s  |  %.3e kg  |  %.3gx Earth mass",
               selectedPlanet->name.c_str(), realKg, earthRatio);
    }

    if (prevSliderRadius != selectedPlanet->radius) {
      prevSliderRadius = selectedPlanet->radius;
      toastTimer = 3.0f;

      const double SIM_EARTH_RADIUS = 0.55;
      const double REAL_EARTH_RADIUS = 6371.0;
      double earthRadiusRatio = (double)selectedPlanet->radius / SIM_EARTH_RADIUS;
      double realKm = earthRadiusRatio * REAL_EARTH_RADIUS;

      snprintf(toastText, toastTextSize,
               "%s  |  %.0f km  |  %.3gx Earth radius",
               selectedPlanet->name.c_str(), realKm, earthRadiusRatio);
    }
  } else {
    prevSelectedPlanet = nullptr;
    prevSliderMass = -1.0f;
    prevSliderRadius = -1.0f;
    settledMass = -1.0f;
  }
}

// ── Draw Toast ─────────────────────────────────────────────────────────
void DrawToast(int screenWidth, float &toastTimer, char *toastText, float dt) {
  if (toastTimer <= 0.0f) return;
  toastTimer -= dt;
  if (toastTimer < 0.0f) toastTimer = 0.0f;

  unsigned char alpha = (toastTimer < 0.5f)
                            ? (unsigned char)((toastTimer / 0.5f) * 255.0f)
                            : 255;
  const int FS = 26;
  int tw = MeasureText(toastText, FS);
  int tx = screenWidth / 2 - tw / 2;
  int ty = 175;

  DrawRectangleRounded({(float)(tx - 22), (float)(ty - 10),
                        (float)(tw + 44), (float)(FS + 22)},
                       0.45f, 8, Color{0, 0, 0, (unsigned char)(alpha * 0.65f)});
  DrawText(toastText, tx, ty, FS, Color{255, 228, 110, alpha});
}

// ── AI Narrator Subtitle Captions ──────────────────────────────────────
void DrawCaptions(int screenWidth, int screenHeight, float dt) {
  if (ttsCaptionTimer <= 0.0f && !captionQueue.empty()) {
    CaptionEntry entry = captionQueue.front();
    captionQueue.pop();
    currentTTSCaption = entry.displayText;
    float estimatedDuration = (float)entry.wordCount / 2.2f + 1.0f;
    if (estimatedDuration < 3.0f) estimatedDuration = 3.0f;
    ttsCaptionTimer = estimatedDuration;
  }
  if (ttsCaptionTimer > 0.0f) {
    ttsCaptionTimer -= dt;
    unsigned char captionAlpha =
        (ttsCaptionTimer < 0.5f)
            ? (unsigned char)(ttsCaptionTimer / 0.5f * 255.0f)
            : 255;
    int fs = 24;
    int tw = MeasureText(currentTTSCaption.c_str(), fs);
    int tx = screenWidth / 2 - tw / 2;
    int ty = screenHeight - 90;
    DrawRectangleRounded(
        {(float)(tx - 20), (float)(ty - 10), (float)(tw + 40), (float)(fs + 20)},
        0.5f, 8, Color{0, 0, 0, (unsigned char)(captionAlpha * 0.82f)});
    DrawText(currentTTSCaption.c_str(), tx, ty, fs, Color{230, 230, 230, captionAlpha});
  }
}

// ── Kill Feed ──────────────────────────────────────────────────────────
void DrawKillFeed(int screenWidth, float dt) {
  int feedY = 80;
  int feedFS = 18;
  for (int k = (int)killFeed.size() - 1; k >= 0; k--) {
    KillFeedEntry &kf = killFeed[k];
    kf.timer -= dt;
    if (kf.timer <= 0.0f) {
      killFeed.erase(killFeed.begin() + k);
      continue;
    }
    unsigned char alpha = (kf.timer < 1.0f)
        ? (unsigned char)(kf.timer / 1.0f * 255.0f) : 255;

    char feedText[128];
    snprintf(feedText, sizeof(feedText), "%s  absorbed  %s",
             kf.survivorName.c_str(), kf.victimName.c_str());

    int ftw = MeasureText(feedText, feedFS);
    int ftx = screenWidth - ftw - 20;
    int fty = feedY;

    DrawRectangleRounded(
        {(float)(ftx - 10), (float)(fty - 4), (float)(ftw + 20), (float)(feedFS + 8)},
        0.4f, 6, Color{180, 40, 40, (unsigned char)(alpha * 0.7f)});
    DrawText(feedText, ftx, fty, feedFS, Color{255, 200, 200, alpha});

    feedY += feedFS + 14;
  }
}

// ── Play/Pause Button ──────────────────────────────────────────────────
void DrawPlayPauseButton(int screenWidth, int screenHeight, EngineState &currentState) {
  const float PP_W = 264.0f, PP_H = 105.0f;
  const float PP_X = (float)screenWidth / 2.0f - PP_W / 2.0f;
  const float PP_Y = (float)screenHeight - PP_H - 120.0f;

  DrawRectangleRounded(
      {PP_X - 5.0f, PP_Y - 5.0f, PP_W + 10.0f, PP_H + 10.0f}, 0.40f, 8,
      (currentState == PLAYING) ? Color{50, 180, 70, 80} : Color{200, 60, 60, 80});

  GuiSetStyle(DEFAULT, TEXT_SIZE, 33);
  GuiSetAlpha(0.92f);
  if (GuiButton({PP_X, PP_Y, PP_W, PP_H},
                (currentState == PLAYING) ? "|| PAUSE" : ">  PLAY")) {
    currentState = (currentState == PLAYING) ? PAUSED : PLAYING;
  }
  GuiSetAlpha(1.0f);
  GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
}

// ── Help Bar ───────────────────────────────────────────────────────────
void DrawHelpBar(int screenHeight) {
  DrawText("SPACE: Play/Pause  |  Hold RMB + WASD: Free Camera  |  LMB: "
           "Select Planet",
           10, screenHeight - 28, 15, Color{90, 90, 90, 200});
}
