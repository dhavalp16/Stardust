#pragma once
#include "raylib.h"
#include "sim/Vec3.h"

// The simulation has its own double-precision vector type (sim::Vec3); raylib
// draws with float Vector3. Converting rounds each component to float, which
// is more than precise enough for putting pixels on screen.
inline Vector3 ToRaylib(sim::Vec3 v) {
  return Vector3{static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z)};
}
