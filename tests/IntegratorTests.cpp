// Tests for the Kick and Drift building blocks.
#include "sim/Integrator.h"
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
