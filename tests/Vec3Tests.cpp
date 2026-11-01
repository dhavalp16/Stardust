// Tests for the small vector type the simulation is built on.
#include "sim/Vec3.h"
#include <gtest/gtest.h>

using sim::Vec3;

TEST(Vec3, AddsAndSubtractsComponentWise) {
  Vec3 a{1, 2, 3}, b{4, 5, 6};
  Vec3 sum = a + b, diff = b - a;
  EXPECT_FLOAT_EQ(sum.x, 5);
  EXPECT_FLOAT_EQ(sum.y, 7);
  EXPECT_FLOAT_EQ(sum.z, 9);
  EXPECT_FLOAT_EQ(diff.x, 3);
  EXPECT_FLOAT_EQ(diff.y, 3);
  EXPECT_FLOAT_EQ(diff.z, 3);
}

TEST(Vec3, ScalesByANumberOnEitherSide) {
  Vec3 a{1, -2, 3};
  Vec3 left = 2.0f * a, right = a * 2.0f, half = a / 2.0f;
  EXPECT_FLOAT_EQ(left.y, -4);
  EXPECT_FLOAT_EQ(right.z, 6);
  EXPECT_FLOAT_EQ(half.x, 0.5f);
}

TEST(Vec3, CompoundAssignmentChangesTheLeftSide) {
  Vec3 a{1, 1, 1};
  a += Vec3{1, 2, 3};
  a -= Vec3{0, 1, 0};
  a *= 2.0f;
  EXPECT_FLOAT_EQ(a.x, 4);
  EXPECT_FLOAT_EQ(a.y, 4);
  EXPECT_FLOAT_EQ(a.z, 8);
}

TEST(Vec3, DotProductAndLength) {
  EXPECT_FLOAT_EQ(sim::Dot(Vec3{1, 2, 3}, Vec3{4, -5, 6}), 4 - 10 + 18);
  // A 3-4-5 triangle: the length of (3, 4, 0) is exactly 5.
  EXPECT_FLOAT_EQ(sim::Length(Vec3{3, 4, 0}), 5);
  EXPECT_FLOAT_EQ(sim::LengthSquared(Vec3{3, 4, 0}), 25);
}
