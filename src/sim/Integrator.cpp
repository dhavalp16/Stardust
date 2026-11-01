#include "sim/Integrator.h"

namespace sim {

void Kick(std::vector<Body> &bodies, const std::vector<Vec3> &accelerations, float dt) {
  for (size_t i = 0; i < bodies.size(); i++) {
    if (bodies[i].isAlive) bodies[i].velocity += accelerations[i] * dt;
  }
}

void Drift(std::vector<Body> &bodies, float dt) {
  for (Body &body : bodies) {
    if (body.isAlive) body.position += body.velocity * dt;
  }
}

} // namespace sim
