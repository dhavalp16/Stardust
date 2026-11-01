#pragma once
#include "sim/Body.h"
#include <vector>

namespace sim {

// The two building blocks every integrator here is made of.
//
//   Kick:  change velocities using accelerations     v += a * dt
//   Drift: change positions using velocities         x += v * dt
//
// Dead bodies are skipped by both.
void Kick(std::vector<Body> &bodies, const std::vector<Vec3> &accelerations, float dt);
void Drift(std::vector<Body> &bodies, float dt);

} // namespace sim
