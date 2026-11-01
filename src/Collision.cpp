#include "Collision.h"
#include <cmath>

void ProcessCollisions(std::vector<Planet> &activePlanets,
                       std::vector<Fragment> &activeFragments,
                       Planet *&selectedPlanet, bool &isTracking) {
  for (size_t i = 0; i < activePlanets.size(); i++) {
    for (size_t j = i + 1; j < activePlanets.size(); j++) {
      if (!activePlanets[i].isAlive || !activePlanets[j].isAlive)
        continue;

      float distSqr = Vector3DistanceSqr(activePlanets[i].position,
                                         activePlanets[j].position);
      float combinedRadii = activePlanets[i].radius + activePlanets[j].radius;
      float collisionThreshold = combinedRadii * 0.8f;

      if (distSqr < (collisionThreshold * collisionThreshold)) {
        int survivorIndex = -1;
        int victimIndex = -1;

        if (i == 0 || j == 0) {
          survivorIndex = 0;
          victimIndex = (i == 0) ? j : i;
        } else if (activePlanets[i].mass >= activePlanets[j].mass) {
          survivorIndex = i;
          victimIndex = j;
        } else {
          survivorIndex = j;
          victimIndex = i;
        }

        Planet &survivor = activePlanets[survivorIndex];
        Planet &victim = activePlanets[victimIndex];

        victim.isAlive = false;
        if (selectedPlanet == &victim) {
          selectedPlanet = nullptr;
          isTracking = false;
        }

        if (survivorIndex != 0) {
          Vector3 p1 = Vector3Scale(survivor.velocity, survivor.mass);
          Vector3 p2 = Vector3Scale(victim.velocity, victim.mass);
          Vector3 totalMomentum = Vector3Add(p1, p2);
          float totalMass = survivor.mass + victim.mass;
          survivor.velocity = Vector3Scale(totalMomentum, 1.0f / totalMass);
        }

        survivor.mass += victim.mass;
        if (survivor.mass > survivor.massMax)
          survivor.massMax = survivor.mass * 1.5f;

        float newVolume =
            (survivor.radius * survivor.radius * survivor.radius) +
            (victim.radius * victim.radius * victim.radius);
        survivor.radius = std::cbrt(newVolume);
        if (survivor.radius > survivor.radiusMax)
          survivor.radiusMax = survivor.radius * 1.5f;

        int numFragments = 30 + GetRandomValue(0, 20);
        for (int f = 0; f < numFragments; f++) {
          Fragment frag;
          frag.position = victim.position;

          Vector3 randDir = {(float)GetRandomValue(-100, 100) / 100.0f,
                             (float)GetRandomValue(-100, 100) / 100.0f,
                             (float)GetRandomValue(-100, 100) / 100.0f};
          randDir = Vector3Normalize(randDir);

          float speed = (float)GetRandomValue(2, 8) +
                        (Vector3Length(victim.velocity) * 0.15f);

          Vector3 baseVelocity = Vector3Scale(victim.velocity, 0.4f);
          frag.velocity =
              Vector3Add(baseVelocity, Vector3Scale(randDir, speed));

          frag.size = victim.radius * ((float)GetRandomValue(10, 30) / 100.0f);
          frag.life = 1.0f;
          frag.color = victim.tint;
          frag.isAlive = true;
          activeFragments.push_back(frag);
        }
      }
    }
  }
}
