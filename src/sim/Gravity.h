#pragma once
#include "sim/Body.h"
#include <vector>

namespace sim {

struct GravityParams {
  float G = 1.0f; // gravitational constant, in this simulation's units

  // Plummer softening length. Real gravity, G*m/r^2, shoots to infinity as two
  // centres approach (r -> 0). Softened gravity uses r^2 + softening^2 instead
  // of r^2, so the pull stays finite and smooth. At r = 1 the pull is weaker by
  // about 0.015%; bodies merge long before they get close enough to notice.
  float softening = 0.01f;
};

// Fills `out` with the gravitational acceleration of every body, caused by all
// the other live bodies (Newton's law of universal gravitation, softened).
// `out` is resized to bodies.size(); dead bodies get zero acceleration.
void ComputeAccelerations(const std::vector<Body> &bodies, const GravityParams &params,
                          std::vector<Vec3> &out);

} // namespace sim
