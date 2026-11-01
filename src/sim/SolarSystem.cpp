#include "sim/SolarSystem.h"
#include "sim/Diagnostics.h"
#include <cmath>

namespace sim {

const std::vector<PlanetSpec> &SolarSystemSpecs() {
  // clang-format off
  static const std::vector<PlanetSpec> specs = {
    // name      parent radius phase  speed    mass     radius  model                 tint                  spin   mass range       radius range
    {"Sun",      -1,    0.0f, 0.0f,  0.000f, 2000.0f, 3.00f, "assets/sun.glb",     {253, 249,   0, 255},  0.10f, 500.0f, 5000.0f, 1.00f, 6.0f},
    {"Mercury",   0,    8.0f, 1.0f, 15.811f,  0.055f, 0.25f, "assets/mercury.glb", {130, 130, 130, 255},  0.02f, 0.01f,   1.0f,  0.10f, 1.0f},
    {"Venus",     0,   14.0f, 3.0f, 11.952f,  0.815f, 0.50f, "assets/venus.glb",   {255, 161,   0, 255}, -0.01f, 0.1f,    5.0f,  0.10f, 2.0f},
    {"Earth",     0,   20.0f, 5.0f, 10.000f, 10.000f, 0.55f, "assets/earth.glb",   {  0, 121, 241, 255},  0.50f, 1.0f,   20.0f,  0.20f, 3.0f},
    {"Mars",      0,   30.0f, 0.5f,  8.165f,  0.107f, 0.35f, "assets/mars.glb",    {230,  41,  55, 255},  0.48f, 0.01f,   2.0f,  0.10f, 1.5f},
    {"Jupiter",   0,   55.0f, 2.5f,  6.030f,  3.000f, 1.80f, "assets/jupiter.glb", {211, 176, 131, 255},  1.50f, 0.5f,   15.0f,  0.50f, 4.0f},
    {"Saturn",    0,   80.0f, 4.5f,  5.000f,  1.500f, 1.50f, "assets/saturn.glb",  {255, 203,   0, 255},  1.30f, 0.2f,   10.0f,  0.50f, 3.5f},
    {"Uranus",    0,  110.0f, 1.5f,  4.264f,  0.500f, 1.00f, "assets/uranus.glb",  {102, 191, 255, 255}, -0.80f, 0.1f,    5.0f,  0.30f, 2.5f},
    {"Neptune",   0,  140.0f, 3.5f,  3.780f,  0.600f, 0.95f, "assets/neptune.glb", {  0,  82, 172, 255},  0.90f, 0.1f,    5.0f,  0.30f, 2.5f},
    {"Moon",      3,    0.9f, 0.0f,  3.333f,  0.012f, 0.15f, "assets/moon.glb",    {200, 200, 200, 255},  0.05f, 0.001f,  0.5f,  0.05f, 0.5f},
  };
  // clang-format on
  return specs;
}

std::vector<Body> BuildBodies(const std::vector<PlanetSpec> &specs) {
  std::vector<Body> bodies;
  bodies.reserve(specs.size());

  for (const PlanetSpec &spec : specs) {
    Body body;
    body.mass = spec.mass;
    body.radius = spec.radius;
    if (spec.parent >= 0) {
      // Parents are listed before their children, so bodies[parent] exists.
      const Body &parent = bodies[static_cast<size_t>(spec.parent)];
      float c = std::cos(spec.phase), s = std::sin(spec.phase);
      body.position = parent.position + Vec3{spec.orbitRadius * c, 0.0f, spec.orbitRadius * s};
      // (-sin, 0, cos) is at right angles to (cos, 0, sin): a circular-orbit direction.
      body.velocity = parent.velocity + Vec3{-spec.orbitSpeed * s, 0.0f, spec.orbitSpeed * c};
    }
    bodies.push_back(body);
  }

  // Give the Sun the momentum that cancels everyone else's, so the centre of
  // mass stays still. Index 0 is the Sun and starts with zero velocity.
  bodies[0].velocity = -TotalMomentum(bodies) / bodies[0].mass;
  return bodies;
}

} // namespace sim
