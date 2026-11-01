#pragma once
#include "sim/Body.h"
#include "sim/Gravity.h"
#include <cstddef>
#include <vector>

namespace sim {

// One row of the starting scenario. The physics only reads the orbit and body
// fields; the presentation fields (model, colour, spin, slider ranges) are plain
// data for the app, kept in the same row so the two can never get out of step.
struct PlanetSpec {
  const char *name;

  // Orbit: the body starts `orbitRadius` from its parent at angle `phase`
  // (radians, in the horizontal x-z plane), moving relative to the parent at
  // circular-orbit speed, at right angles to the line joining them.
  int parent; // index of the body it orbits, or -1 for none (the Sun)
  float orbitRadius;
  float phase;

  float mass;
  float radius;

  // Presentation (used by the app only).
  const char *modelPath;
  unsigned char tint[4]; // RGBA, used for debris
  float spinRate;        // radians per second; negative spins backwards
  float massMin, massMax;
  float radiusMin, radiusMax;
};

// The Sun, eight planets and the Moon. Index 0 is the Sun.
const std::vector<PlanetSpec> &SolarSystemSpecs();

// Turns specs into bodies. Afterwards the Sun's velocity is set so the total
// momentum of the system is zero, which keeps the system from drifting away.
std::vector<Body> BuildBodies(const std::vector<PlanetSpec> &specs, float G = GravityParams{}.G);

// The speed for a circular orbit of radius r around a mass M:  v = sqrt(G * M / r).
// It comes from setting gravity (G M m / r^2) equal to the force needed to keep
// something moving in a circle (m v^2 / r).
double CircularOrbitSpeed(double G, double parentMass, double r);

} // namespace sim
