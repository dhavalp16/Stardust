#include "sim/Collision.h"
#include <cmath>

namespace sim {

std::vector<MergeEvent> ResolveCollisions(std::vector<Body> &bodies,
                                          const CollisionParams &params) {
  std::vector<MergeEvent> events;

  for (size_t i = 0; i < bodies.size(); i++) {
    for (size_t j = i + 1; j < bodies.size(); j++) {
      if (!bodies[i].isAlive || !bodies[j].isAlive) continue;

      float threshold = (bodies[i].radius + bodies[j].radius) * params.overlapFactor;
      // Compare squared distances: same answer, no square root needed.
      if (LengthSquared(bodies[i].position - bodies[j].position) >= threshold * threshold)
        continue;

      size_t survivorIndex, victimIndex;
      if (i == params.anchorIndex || j == params.anchorIndex) {
        survivorIndex = params.anchorIndex;
        victimIndex = (i == params.anchorIndex) ? j : i;
      } else if (bodies[i].mass >= bodies[j].mass) {
        survivorIndex = i;
        victimIndex = j;
      } else {
        survivorIndex = j;
        victimIndex = i;
      }

      Body &survivor = bodies[survivorIndex];
      Body &victim = bodies[victimIndex];
      events.push_back({survivorIndex, victimIndex, victim.position, victim.velocity,
                        victim.radius});

      victim.isAlive = false;

      // Perfectly inelastic merge. The merged body must carry the same total
      // momentum (mass * velocity) as the two did, and sit at their centre of
      // mass, so the system as a whole does not jump. This applies to the Sun
      // too: it is 2000 units of mass, so a planet barely nudges it.
      double m1 = survivor.mass, m2 = victim.mass, total = m1 + m2;
      survivor.velocity = (survivor.velocity * m1 + victim.velocity * m2) / total;
      survivor.position = (survivor.position * m1 + victim.position * m2) / total;
      survivor.mass += victim.mass;

      // Volumes add (4/3 pi r^3 each; the constants cancel), so the new radius
      // is the cube root of the summed cubes.
      float r1 = survivor.radius, r2 = victim.radius;
      survivor.radius = std::cbrt(r1 * r1 * r1 + r2 * r2 * r2);
    }
  }
  return events;
}

} // namespace sim
