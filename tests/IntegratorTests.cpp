// Tests for the Kick/Drift building blocks and for the two integrators.
#include "sim/Diagnostics.h"
#include "sim/Integrator.h"
#include "sim/Simulation.h"
#include <algorithm>
#include <cmath>
#include <gtest/gtest.h>

using sim::Body;
using sim::Vec3;

TEST(Integrator, KickChangesVelocityByAccelerationTimesDt) {
  std::vector<Body> bodies(1);
  bodies[0].velocity = {1, 0, 0};
  sim::Kick(bodies, {Vec3{0, -10, 0}}, 0.5f);
  EXPECT_FLOAT_EQ(bodies[0].velocity.x, 1.0f);
  EXPECT_FLOAT_EQ(bodies[0].velocity.y, -5.0f);
}

TEST(Integrator, DriftChangesPositionByVelocityTimesDt) {
  std::vector<Body> bodies(1);
  bodies[0].position = {1, 2, 3};
  bodies[0].velocity = {2, 0, -4};
  sim::Drift(bodies, 0.25f);
  EXPECT_FLOAT_EQ(bodies[0].position.x, 1.5f);
  EXPECT_FLOAT_EQ(bodies[0].position.y, 2.0f);
  EXPECT_FLOAT_EQ(bodies[0].position.z, 2.0f);
}

TEST(Integrator, DeadBodiesAreLeftAlone) {
  std::vector<Body> bodies(1);
  bodies[0].isAlive = false;
  bodies[0].velocity = {1, 1, 1};
  sim::Kick(bodies, {Vec3{5, 5, 5}}, 1.0f);
  sim::Drift(bodies, 1.0f);
  EXPECT_FLOAT_EQ(bodies[0].velocity.x, 1.0f);
  EXPECT_FLOAT_EQ(bodies[0].position.x, 0.0f);
}

// ---------------------------------------------------------------------------
// Whole-step integrator comparisons on a simple orbit with a known answer.
// ---------------------------------------------------------------------------
namespace {

// A tiny planet on a circular orbit of radius 10 around a mass of 1000.
// v = sqrt(G * M / r) = sqrt(1000 / 10) = 10, period = 2 * pi * r / v = 2 * pi.
std::vector<Body> CircularOrbit() {
  Body sun, planet;
  sun.mass = 1000.0f;
  sun.radius = 0.01f;
  planet.mass = 1e-6f;
  planet.radius = 0.01f;
  planet.position = {10, 0, 0};
  planet.velocity = {0, 0, 10};
  return {sun, planet};
}

// Largest relative change in total energy over `steps` steps of size h.
double MaxEnergyError(sim::IntegratorType type, float h, int steps) {
  sim::SimulationParams params;
  params.integrator = type;
  sim::Simulation simulation(CircularOrbit(), params);
  double e0 = sim::TotalEnergy(simulation.Bodies(), params.gravity), worst = 0.0;
  for (int i = 0; i < steps; i++) {
    simulation.Step(h);
    double e = sim::TotalEnergy(simulation.Bodies(), params.gravity);
    worst = std::max(worst, std::abs((e - e0) / e0));
  }
  return worst;
}

} // namespace

TEST(Integrator, VerletConservesEnergyFarBetterThanEulerAtLargeSteps) {
  // Ten orbits at 50 steps per second (about 314 steps per orbit).
  const float h = 1.0f / 50.0f;
  const int steps = 3142;
  double euler = MaxEnergyError(sim::IntegratorType::SemiImplicitEuler, h, steps);
  double verlet = MaxEnergyError(sim::IntegratorType::VelocityVerlet, h, steps);
  EXPECT_LT(verlet, euler / 10.0) << "euler " << euler << ", verlet " << verlet;
}

TEST(Integrator, VerletKeepsTheOrbitRadius) {
  sim::Simulation simulation(CircularOrbit());
  for (int i = 0; i < 3770; i++) simulation.Step(1.0f / 600.0f); // about one orbit
  float r = sim::Length(simulation.Bodies()[1].position - simulation.Bodies()[0].position);
  EXPECT_NEAR(r, 10.0f, 0.01f);
}
