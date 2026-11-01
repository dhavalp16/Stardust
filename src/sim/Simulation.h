#pragma once
#include "sim/Body.h"
#include "sim/Collision.h"
#include "sim/Gravity.h"
#include <utility>
#include <vector>

namespace sim {

struct SimulationParams {
  GravityParams gravity;
  CollisionParams collisions;
};

// Owns the bodies and advances them one fixed step at a time.
class Simulation {
public:
  explicit Simulation(std::vector<Body> bodies, SimulationParams params = {})
      : bodies_(std::move(bodies)), params_(params) {}

  // Advances every body by `dt` seconds, then merges any that now overlap.
  // Returns the merges that happened during this step.
  std::vector<MergeEvent> Step(float dt);

  // Direct access, so the HUD sliders can edit mass and radius.
  std::vector<Body> &Bodies() { return bodies_; }
  const std::vector<Body> &Bodies() const { return bodies_; }

  const SimulationParams &Params() const { return params_; }

private:
  std::vector<Body> bodies_;
  SimulationParams params_;
  std::vector<Vec3> accelerations_; // reused every step to avoid reallocating
};

} // namespace sim
