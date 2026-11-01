// Tests for merging bodies that overlap.
#include "sim/Collision.h"
#include "sim/Diagnostics.h"
#include <cmath>
#include <gtest/gtest.h>

using sim::Body;
using sim::Vec3;

namespace {

Body MakeBody(Vec3 position, Vec3 velocity, float mass, float radius) {
  Body b;
  b.position = position;
  b.velocity = velocity;
  b.mass = mass;
  b.radius = radius;
  return b;
}

// Index 0 is the anchor (the Sun) by default; these tests put a heavy anchor
// far away so it does not get involved unless a test wants it to.
std::vector<Body> WithFarAnchor(std::vector<Body> others) {
  std::vector<Body> bodies = {MakeBody({1000, 0, 0}, {}, 1000.0f, 1.0f)};
  bodies.insert(bodies.end(), others.begin(), others.end());
  return bodies;
}

} // namespace

TEST(Collision, SeparatedBodiesDoNotMerge) {
  // Radii 1 + 1, threshold 0.8 * 2 = 1.6; distance 1.7 is just outside.
  auto bodies = WithFarAnchor({MakeBody({0, 0, 0}, {}, 1, 1), MakeBody({1.7f, 0, 0}, {}, 1, 1)});
  EXPECT_TRUE(sim::ResolveCollisions(bodies, {}).empty());
  EXPECT_TRUE(bodies[1].isAlive);
  EXPECT_TRUE(bodies[2].isAlive);
}

TEST(Collision, OverlappingBodiesMergeIntoTheHeavierOne) {
  auto bodies = WithFarAnchor({MakeBody({0, 0, 0}, {}, 1, 1), MakeBody({1.5f, 0, 0}, {}, 4, 1)});
  auto events = sim::ResolveCollisions(bodies, {});

  ASSERT_EQ(events.size(), 1u);
  EXPECT_EQ(events[0].survivor, 2u);
  EXPECT_EQ(events[0].victim, 1u);
  EXPECT_FALSE(bodies[1].isAlive);
  EXPECT_TRUE(bodies[2].isAlive);
  EXPECT_FLOAT_EQ(bodies[2].mass, 5.0f);
}

TEST(Collision, MergedRadiusKeepsTheTotalVolume) {
  auto bodies = WithFarAnchor({MakeBody({0, 0, 0}, {}, 2, 1), MakeBody({1, 0, 0}, {}, 1, 1)});
  sim::ResolveCollisions(bodies, {});
  // Two unit spheres: r^3 = 1 + 1, so r = cube root of 2.
  EXPECT_NEAR(bodies[1].radius, std::cbrt(2.0f), 1e-6f);
}

TEST(Collision, MergeConservesMomentumBetweenPlanets) {
  auto bodies = WithFarAnchor(
      {MakeBody({0, 0, 0}, {2, 0, 0}, 3, 1), MakeBody({1, 0, 0}, {-1, 1, 0}, 1, 1)});
  Vec3 before = bodies[1].velocity * bodies[1].mass + bodies[2].velocity * bodies[2].mass;
  sim::ResolveCollisions(bodies, {});
  Vec3 after = bodies[1].velocity * bodies[1].mass;
  EXPECT_NEAR(after.x, before.x, 1e-5f);
  EXPECT_NEAR(after.y, before.y, 1e-5f);
  EXPECT_NEAR(after.z, before.z, 1e-5f);
}

TEST(Collision, TheAnchorSurvivesEvenWhenLighter) {
  std::vector<Body> bodies = {MakeBody({0, 0, 0}, {}, 1, 1), MakeBody({1, 0, 0}, {}, 50, 1)};
  auto events = sim::ResolveCollisions(bodies, {});
  ASSERT_EQ(events.size(), 1u);
  EXPECT_EQ(events[0].survivor, 0u);
  EXPECT_TRUE(bodies[0].isAlive);
  EXPECT_FALSE(bodies[1].isAlive);
  EXPECT_FLOAT_EQ(bodies[0].mass, 51.0f);
}

TEST(Collision, EventRecordsTheVictimAsItWasBeforeTheMerge) {
  auto bodies = WithFarAnchor(
      {MakeBody({0, 0, 0}, {}, 5, 1), MakeBody({1, 2, 0}, {3, 0, 0}, 1, 0.5f)});
  bodies[2].position = {1, 0, 0};
  auto events = sim::ResolveCollisions(bodies, {});
  ASSERT_EQ(events.size(), 1u);
  EXPECT_DOUBLE_EQ(events[0].victimPosition.x, 1.0f);
  EXPECT_DOUBLE_EQ(events[0].victimVelocity.x, 3.0f);
  EXPECT_FLOAT_EQ(events[0].victimRadius, 0.5f);
}

TEST(Collision, DeadBodiesAreIgnored) {
  auto bodies = WithFarAnchor({MakeBody({0, 0, 0}, {}, 1, 1), MakeBody({0.5f, 0, 0}, {}, 1, 1)});
  bodies[2].isAlive = false;
  EXPECT_TRUE(sim::ResolveCollisions(bodies, {}).empty());
  EXPECT_FLOAT_EQ(bodies[1].mass, 1.0f);
}

TEST(Collision, MergeIntoTheAnchorAlsoConservesMomentum) {
  // The original code let the Sun ignore impact momentum, so the system's
  // total momentum changed on every Sun impact.
  std::vector<Body> bodies = {MakeBody({0, 0, 0}, {0, 0, 0}, 1000, 2),
                              MakeBody({1, 0, 0}, {0, 0, 30}, 2, 0.5f)};
  sim::Vec3 before = sim::TotalMomentum(bodies);
  sim::ResolveCollisions(bodies, {});
  sim::Vec3 after = sim::TotalMomentum(bodies);
  EXPECT_NEAR(after.z, before.z, 1e-9);
  EXPECT_GT(bodies[0].velocity.z, 0.0); // the Sun is nudged, slightly
}

TEST(Collision, MergedBodySitsAtTheCentreOfMass) {
  auto bodies = WithFarAnchor({MakeBody({0, 0, 0}, {}, 3, 1), MakeBody({1, 0, 0}, {}, 1, 1)});
  sim::Vec3 before = sim::CenterOfMass(bodies);
  sim::ResolveCollisions(bodies, {});
  sim::Vec3 after = sim::CenterOfMass(bodies);
  EXPECT_NEAR(after.x, before.x, 1e-9);
  EXPECT_NEAR(bodies[1].position.x, 0.25, 1e-9); // (3 * 0 + 1 * 1) / 4
}
