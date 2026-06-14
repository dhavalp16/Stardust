#pragma once
#include "Planet.h"
#include "raylib.h"
#include <vector>

enum EngineState { PAUSED, PLAYING, SUMMARY };

void DrawSelectionReticle(Planet *selectedPlanet, Camera3D &camera);

void DrawDebugOverlay(int screenWidth, int screenHeight,
                      Planet *selectedPlanet, EngineState currentState,
                      const std::vector<Planet> &activePlanets,
                      bool &isTracking);

void DrawHelpBar(int screenHeight);

void DrawKeyPressOverlay(int screenWidth, int screenHeight);
