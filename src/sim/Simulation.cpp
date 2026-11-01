#include "sim/Simulation.h"
#include "sim/Integrator.h"

namespace sim {

std::vector<MergeEvent> Simulation::Step(float dt) {
  if (params_.integrator == IntegratorType::SemiImplicitEuler) {
    // New velocity first, then move with it.
    ComputeAccelerations(bodies_, params_.gravity, accelerations_);
    Kick(bodies_, accelerations_, dt);
    Drift(bodies_, dt);
  } else {
    // Velocity Verlet, written as "kick-drift-kick" (also called leapfrog):
    // half a kick with the forces here, a full drift, then half a kick with the
    // forces at the new positions. Averaging the old and new forces like this
    // is what makes it second-order accurate.
    //
    // Forces are computed twice per step. The second set could be reused as the
    // first set of the next step, but the HUD sliders can change masses between
    // steps, so recomputing keeps it simple and always correct. At 10 bodies
    // this costs nothing measurable (see docs/MEASUREMENTS.md).
    ComputeAccelerations(bodies_, params_.gravity, accelerations_);
    Kick(bodies_, accelerations_, 0.5f * dt);
    Drift(bodies_, dt);
    ComputeAccelerations(bodies_, params_.gravity, accelerations_);
    Kick(bodies_, accelerations_, 0.5f * dt);
  }
  return ResolveCollisions(bodies_, params_.collisions);
}

} // namespace sim
