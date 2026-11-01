#pragma once
#include "PlanetInfo.h"
#include "raylib.h"
#include "sim/Body.h"
#include <vector>

enum EngineState { PAUSED, PLAYING };

// `selected` is an index into bodies/info, or -1 when nothing is selected.
void DrawSelectionReticle(int selected, const std::vector<sim::Body> &bodies,
                          const std::vector<PlanetInfo> &info, Camera3D &camera);

// The sliders write straight into bodies[selected], which is why bodies is not const.
void DrawDebugOverlay(int screenWidth, int selected,
                      std::vector<sim::Body> &bodies, const std::vector<PlanetInfo> &info,
                      EngineState currentState, bool &isTracking);

void DrawHelpBar(int screenHeight);

void DrawKeyPressOverlay(int screenWidth, int screenHeight);
