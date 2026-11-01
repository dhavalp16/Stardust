// Tests for Newton's law of gravitation as implemented in ComputeAccelerations.
#include "sim/Gravity.h"
#include <gtest/gtest.h>

using sim::Body;
using sim::Vec3;

namespace {

Body MakeBody(Vec3 position, float mass) {
  Body b;
  b.position = position;
  b.mass = mass;
  return b;
}

} // namespace

TEST(Gravity, AccelerationFollowsTheInverseSquareLaw) {
  // A mass of 100 at the origin and a test body 10 units away along x.
  // a = G * M / r^2 = 1 * 100 / 100 = 1, pointing back at the origin.
  std::vector<Body> bodies = {MakeBody({0, 0, 0}, 100.0f), MakeBody({10, 0, 0}, 1.0f)};
  std::vector<Vec3> acc;
  sim::ComputeAccelerations(bodies, sim::GravityParams{}, acc);

  ASSERT_EQ(acc.size(), 2u);
  EXPECT_NEAR(acc[1].x, -1.0f, 1e-5f);
  EXPECT_NEAR(acc[1].y, 0.0f, 1e-6f);
  EXPECT_NEAR(acc[1].z, 0.0f, 1e-6f);
}

TEST(Gravity, DoublingTheDistanceQuartersTheAcceleration) {
  std::vector<Body> near = {MakeBody({0, 0, 0}, 50.0f), MakeBody({5, 0, 0}, 1.0f)};
  std::vector<Body> far = {MakeBody({0, 0, 0}, 50.0f), MakeBody({10, 0, 0}, 1.0f)};
  std::vector<Vec3> accNear, accFar;
  sim::ComputeAccelerations(near, sim::GravityParams{}, accNear);
  sim::ComputeAccelerations(far, sim::GravityParams{}, accFar);
  EXPECT_NEAR(accNear[1].x / accFar[1].x, 4.0f, 1e-4f);
}

TEST(Gravity, ForcesAreEqualAndOpposite) {
  // Newton's third law: m1 * a1 = -m2 * a2. This is why momentum is conserved.
  std::vector<Body> bodies = {MakeBody({1, 2, 3}, 7.0f), MakeBody({-4, 0, 2}, 3.0f)};
  std::vector<Vec3> acc;
  sim::ComputeAccelerations(bodies, sim::GravityParams{}, acc);
  Vec3 f1 = acc[0] * bodies[0].mass;
  Vec3 f2 = acc[1] * bodies[1].mass;
  EXPECT_NEAR(f1.x + f2.x, 0.0f, 1e-5f);
  EXPECT_NEAR(f1.y + f2.y, 0.0f, 1e-5f);
  EXPECT_NEAR(f1.z + f2.z, 0.0f, 1e-5f);
}

TEST(Gravity, ScalesWithTheGravitationalConstant) {
  std::vector<Body> bodies = {MakeBody({0, 0, 0}, 100.0f), MakeBody({10, 0, 0}, 1.0f)};
  std::vector<Vec3> acc;
  sim::GravityParams params;
  params.G = 3.0f;
  sim::ComputeAccelerations(bodies, params, acc);
  EXPECT_NEAR(acc[1].x, -3.0f, 1e-5f);
}

TEST(Gravity, DeadBodiesNeitherPullNorArePulled) {
  std::vector<Body> bodies = {MakeBody({0, 0, 0}, 100.0f), MakeBody({10, 0, 0}, 1.0f)};
  bodies[0].isAlive = false;
  std::vector<Vec3> acc;
  sim::ComputeAccelerations(bodies, sim::GravityParams{}, acc);
  EXPECT_EQ(acc[0].x, 0.0f);
  EXPECT_EQ(acc[1].x, 0.0f);
}

TEST(Gravity, BodiesOnTopOfEachOtherDoNotProduceInfinity) {
  std::vector<Body> bodies = {MakeBody({0, 0, 0}, 100.0f), MakeBody({0, 0, 0}, 1.0f)};
  std::vector<Vec3> acc;
  sim::ComputeAccelerations(bodies, sim::GravityParams{}, acc);
  EXPECT_TRUE(std::isfinite(acc[0].x));
  EXPECT_TRUE(std::isfinite(acc[1].x));
}

TEST(Gravity, SofteningGivesZeroPullAtZeroDistance) {
  // With softening the force smoothly falls to zero at the centre instead of
  // jumping to infinity (or, as the old code did, switching off suddenly).
  std::vector<Body> bodies = {MakeBody({0, 0, 0}, 100.0f), MakeBody({0, 0, 0}, 1.0f)};
  std::vector<Vec3> acc;
  sim::ComputeAccelerations(bodies, sim::GravityParams{}, acc);
  EXPECT_EQ(acc[1].x, 0.0);
}

TEST(Gravity, SofteningIsNegligibleAtPlanetaryDistances) {
  // At r = 1 with softening 0.01 the pull is r^3 / (r^2 + 0.0001)^1.5 of the
  // pure Newtonian value, about 0.99985.
  std::vector<Body> bodies = {MakeBody({0, 0, 0}, 1.0f), MakeBody({1, 0, 0}, 1.0f)};
  std::vector<Vec3> acc;
  sim::ComputeAccelerations(bodies, sim::GravityParams{}, acc);
  EXPECT_NEAR(acc[1].x, -0.99985, 1e-5);
}

TEST(Gravity, SofteningKeepsThePullContinuous) {
  // The old guard switched gravity off below r = 0.01. Just inside and just
  // outside that distance the softened pull must be nearly the same (here they
  // differ by about 1%; the old code jumped from full strength to zero).
  std::vector<Body> inside = {MakeBody({0, 0, 0}, 1.0f), MakeBody({0.0099, 0, 0}, 1.0f)};
  std::vector<Body> outside = {MakeBody({0, 0, 0}, 1.0f), MakeBody({0.0101, 0, 0}, 1.0f)};
  std::vector<Vec3> accIn, accOut;
  sim::ComputeAccelerations(inside, sim::GravityParams{}, accIn);
  sim::ComputeAccelerations(outside, sim::GravityParams{}, accOut);
  EXPECT_NEAR(accIn[1].x / accOut[1].x, 1.0, 0.02);
}
