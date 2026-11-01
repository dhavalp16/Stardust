#pragma once
#include "sim/Vec3.h"

namespace sim {

// The physical state of one celestial body. Only what the physics needs lives
// here; names, models and colours belong to the app (src/app/PlanetInfo.h).
struct Body {
  Vec3 position;
  Vec3 velocity;
  float mass = 1.0f;
  float radius = 1.0f;
  // Bodies absorbed in a merge are flagged dead rather than erased, so that
  // index i always means the same planet (the app pairs index i with model i).
  bool isAlive = true;
};

} // namespace sim
