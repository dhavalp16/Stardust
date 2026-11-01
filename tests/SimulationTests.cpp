// Tests for whole simulation steps, using the real starting scenario.
#include "sim/FixedTimestep.h"
#include "sim/Simulation.h"
#include "sim/SolarSystem.h"
#include <gtest/gtest.h>

using sim::Body;
using sim::Simulation;

namespace {

// Drives a simulation the way the app does: frames of `frameSeconds` fed
// through a fixed-timestep clock, for `frames` frames.
void RunFrames(Simulation &simulation, sim::FixedTimestep &clock, double frameSeconds,
               int frames) {
  for (int f = 0; f < frames; f++) {
    int steps = clock.Advance(frameSeconds);
    for (int s = 0; s < steps; s++) simulation.Step(static_cast<float>(clock.StepSeconds()));
  }
}

} // namespace

TEST(Simulation, ResultDoesNotDependOnTheFrameRate) {
  // The same 2 seconds of play, delivered at 64 frames/s and at 128 frames/s,
  // must give bit-for-bit identical planets, because both run the same 1024
  // physics steps. The old frame-coupled loop could not pass this.
  Simulation slow(sim::BuildBodies(sim::SolarSystemSpecs()));
  Simulation fast(sim::BuildBodies(sim::SolarSystemSpecs()));
  sim::FixedTimestep slowClock(1.0 / 512.0), fastClock(1.0 / 512.0);
  RunFrames(slow, slowClock, 1.0 / 64.0, 128);
  RunFrames(fast, fastClock, 1.0 / 128.0, 256);

  for (size_t i = 0; i < slow.Bodies().size(); i++) {
    EXPECT_EQ(slow.Bodies()[i].position.x, fast.Bodies()[i].position.x) << i;
    EXPECT_EQ(slow.Bodies()[i].position.z, fast.Bodies()[i].position.z) << i;
    EXPECT_EQ(slow.Bodies()[i].isAlive, fast.Bodies()[i].isAlive) << i;
  }
}

TEST(Simulation, ALongStallDoesNotLoseTheMoon) {
  // Regression test for the bug in docs/AUDIT.md: one 0.5 s frame used to be
  // applied as a single giant step. With the clamp it is just 0.1 s of normal
  // steps. Run 20 s at 144 FPS with a stall at t = 2 s.
  Simulation simulation(sim::BuildBodies(sim::SolarSystemSpecs()));
  sim::FixedTimestep clock(1.0 / 600.0);
  RunFrames(simulation, clock, 1.0 / 144.0, 288);
  RunFrames(simulation, clock, 0.5, 1);
  RunFrames(simulation, clock, 1.0 / 144.0, 144 * 18);

  for (const Body &body : simulation.Bodies()) EXPECT_TRUE(body.isAlive);
}

TEST(Simulation, StepMovesBodies) {
  Simulation simulation(sim::BuildBodies(sim::SolarSystemSpecs()));
  sim::Vec3 before = simulation.Bodies()[3].position;
  simulation.Step(0.01f);
  sim::Vec3 after = simulation.Bodies()[3].position;
  EXPECT_GT(sim::Length(after - before), 0.0f);
}

TEST(Simulation, StepReportsMerges) {
  Body a, b;
  a.mass = 1000.0f; // the anchor
  b.position = {0.5f, 0, 0};
  Simulation simulation({a, b});
  auto events = simulation.Step(0.001f);
  ASSERT_EQ(events.size(), 1u);
  EXPECT_EQ(events[0].victim, 1u);
}
