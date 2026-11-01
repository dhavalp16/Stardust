#pragma once
#include "sim/Body.h"
#include "raylib.h"
#include <vector>

void ProcessDesktopInput(Camera3D &camera, float &cameraSpeed,
                         const std::vector<sim::Body> &bodies,
                         int &selected, bool &isTracking, float dt);
