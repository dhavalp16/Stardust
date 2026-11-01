#include "sim/Diagnostics.h"
#include <cmath>

namespace sim {

Vec3 TotalMomentum(const std::vector<Body> &bodies) {
  Vec3 total;
  for (const Body &body : bodies) {
    if (body.isAlive) total += body.velocity * body.mass;
  }
  return total;
}

Vec3 CenterOfMass(const std::vector<Body> &bodies) {
  Vec3 weighted;
  double totalMass = 0.0;
  for (const Body &body : bodies) {
    if (!body.isAlive) continue;
    weighted += body.position * body.mass;
    totalMass += body.mass;
  }
  return totalMass > 0.0 ? weighted / totalMass : Vec3{};
}

double TotalEnergy(const std::vector<Body> &bodies, const GravityParams &params) {
  // Summed in double precision: the total is a small difference of large terms.
  // The potential uses the same softening as the forces, or they would disagree.
  const double eps2 = double{params.softening} * params.softening;
  double kinetic = 0.0, potential = 0.0;
  for (size_t i = 0; i < bodies.size(); i++) {
    if (!bodies[i].isAlive) continue;
    kinetic += 0.5 * bodies[i].mass * LengthSquared(bodies[i].velocity);
    for (size_t j = i + 1; j < bodies.size(); j++) {
      if (!bodies[j].isAlive) continue;
      double r = std::sqrt(LengthSquared(bodies[j].position - bodies[i].position) + eps2);
      potential -= params.G * bodies[i].mass * bodies[j].mass / r;
    }
  }
  return kinetic + potential;
}

} // namespace sim
