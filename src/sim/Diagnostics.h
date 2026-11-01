#pragma once
#include "sim/Body.h"
#include "sim/Gravity.h"
#include <vector>

namespace sim {

// Quantities that physics says must stay constant in a closed system. Measuring
// them is how the tests (and you) check that the simulation is behaving.

// Sum of mass * velocity over live bodies. Gravity alone never changes it.
Vec3 TotalMomentum(const std::vector<Body> &bodies);

// Mass-weighted average position of live bodies (the barycentre).
Vec3 CenterOfMass(const std::vector<Body> &bodies);

// Kinetic energy (sum of 1/2 m v^2) plus gravitational potential energy
// (sum over pairs of -G m1 m2 / r). Gravity alone keeps this constant; a good
// integrator keeps the numerical value close to constant. Merges lose energy
// (they are inelastic), so only compare values between merges.
double TotalEnergy(const std::vector<Body> &bodies, const GravityParams &params);

} // namespace sim
