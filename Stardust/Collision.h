#pragma once
#include "Planet.h"
#include "raylib.h"
#include "raymath.h"
#include <vector>

// Transient visual effects for collisions
struct Fragment {
  Vector3 position;
  Vector3 velocity;
  float size;
  float life; // 1.0 to 0.0
  Color color;
  bool isAlive;
};

// Process N-body collisions and spawn fragments
void ProcessCollisions(std::vector<Planet> &activePlanets,
                       std::vector<Fragment> &activeFragments,
                       Planet *&selectedPlanet, bool &isTracking);
