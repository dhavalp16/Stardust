// Tests for the starting scenario.
#include "sim/SolarSystem.h"
#include <cmath>
#include <gtest/gtest.h>
#include <string>

using sim::Body;

TEST(SolarSystem, HasTheSunEightPlanetsAndTheMoon) {
  const auto &specs = sim::SolarSystemSpecs();
  ASSERT_EQ(specs.size(), 10u);
  EXPECT_EQ(std::string(specs[0].name), "Sun");
  EXPECT_EQ(std::string(specs[9].name), "Moon");
  EXPECT_EQ(specs[9].parent, 3); // the Moon orbits Earth
}

TEST(SolarSystem, TotalMomentumStartsAtZero) {
  std::vector<Body> bodies = sim::BuildBodies(sim::SolarSystemSpecs());
  sim::Vec3 p = sim::TotalMomentum(bodies);
  // Each planet carries momentum of order 10-100; the sum should cancel.
  EXPECT_NEAR(p.x, 0.0f, 1e-3f);
  EXPECT_NEAR(p.y, 0.0f, 1e-3f);
  EXPECT_NEAR(p.z, 0.0f, 1e-3f);
}

TEST(SolarSystem, PlanetsStartAtTheirOrbitRadius) {
  const auto &specs = sim::SolarSystemSpecs();
  std::vector<Body> bodies = sim::BuildBodies(specs);
  for (size_t i = 1; i < specs.size(); i++) {
    const Body &parent = bodies[static_cast<size_t>(specs[i].parent)];
    float r = sim::Length(bodies[i].position - parent.position);
    EXPECT_NEAR(r, specs[i].orbitRadius, 1e-4f) << specs[i].name;
  }
}

TEST(SolarSystem, OrbitVelocityIsAtRightAnglesToTheParent) {
  // For a circular orbit, velocity relative to the parent is perpendicular to
  // the line joining them, so their dot product is zero.
  const auto &specs = sim::SolarSystemSpecs();
  std::vector<Body> bodies = sim::BuildBodies(specs);
  for (size_t i = 1; i < specs.size(); i++) {
    const Body &parent = bodies[static_cast<size_t>(specs[i].parent)];
    // The Sun was at rest when the planets were placed; its velocity is only
    // set afterwards (to cancel the total momentum), so measure against zero.
    sim::Vec3 parentVelocity = (specs[i].parent == 0) ? sim::Vec3{} : parent.velocity;
    sim::Vec3 offset = bodies[i].position - parent.position;
    sim::Vec3 relVel = bodies[i].velocity - parentVelocity;
    EXPECT_NEAR(sim::Dot(offset, relVel), 0.0f, 1e-3f) << specs[i].name;
    EXPECT_NEAR(sim::Length(relVel), specs[i].orbitSpeed, 1e-4f) << specs[i].name;
  }
}
