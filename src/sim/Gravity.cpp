#include "sim/Gravity.h"

namespace sim {

void ComputeAccelerations(const std::vector<Body> &bodies, const GravityParams &params,
                          std::vector<Vec3> &out) {
  out.assign(bodies.size(), Vec3{});
  const double eps2 = double{params.softening} * params.softening;

  // Visit each pair once (j starts after i). Newton's third law says the pull
  // of i on j is equal and opposite to the pull of j on i, so one distance
  // calculation serves both bodies.
  for (size_t i = 0; i < bodies.size(); i++) {
    if (!bodies[i].isAlive) continue;
    for (size_t j = i + 1; j < bodies.size(); j++) {
      if (!bodies[j].isAlive) continue;

      Vec3 d = bodies[j].position - bodies[i].position; // points from i to j
      double r2 = LengthSquared(d) + eps2;              // softened distance squared
      // a = G * m / r^2 in the direction of d. Dividing d by r^3 does both at once:
      // d / r is the unit direction, and the remaining 1 / r^2 is the inverse square.
      Vec3 pull = d * (params.G / (r2 * std::sqrt(r2)));
      out[i] += pull * bodies[j].mass; // i is pulled towards j
      out[j] -= pull * bodies[i].mass; // j is pulled towards i
    }
  }
}

} // namespace sim
