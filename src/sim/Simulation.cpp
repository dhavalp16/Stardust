#include "sim/Simulation.h"
#include "sim/Integrator.h"

namespace sim {

std::vector<MergeEvent> Simulation::Step(float dt) {
  // Semi-implicit (symplectic) Euler: new velocity first, then move with it.
  ComputeAccelerations(bodies_, params_.gravity, accelerations_);
  Kick(bodies_, accelerations_, dt);
  Drift(bodies_, dt);
  return ResolveCollisions(bodies_, params_.collisions);
}

} // namespace sim
