#pragma once
#include "sim/Body.h"
#include <vector>

namespace sim {

struct GravityParams {
  float G = 1.0f; // gravitational constant, in this simulation's units
};

// Fills `out` with the gravitational acceleration of every body, caused by all
// the other live bodies (Newton's law of universal gravitation).
// `out` is resized to bodies.size(); dead bodies get zero acceleration.
void ComputeAccelerations(const std::vector<Body> &bodies, const GravityParams &params,
                          std::vector<Vec3> &out);

} // namespace sim
