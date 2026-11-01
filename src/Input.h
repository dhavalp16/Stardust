#pragma once
#include "Planet.h"
#include "raylib.h"
#include <vector>

void ProcessDesktopInput(Camera3D &camera, float &cameraSpeed,
                         std::vector<Planet> &activePlanets,
                         Planet *&selectedPlanet, bool &isTracking, float dt);
