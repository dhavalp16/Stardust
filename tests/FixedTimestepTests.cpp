// Tests for the fixed-timestep accumulator.
//
// The step sizes here are powers of two (1/64, 1/512...) because those are
// exact in binary floating point, so the expected step counts are exact too.
#include "sim/FixedTimestep.h"
#include <gtest/gtest.h>

using sim::FixedTimestep;

TEST(FixedTimestep, AFrameOfEightStepsRunsEightSteps) {
  FixedTimestep clock(1.0 / 512.0);
  EXPECT_EQ(clock.Advance(1.0 / 64.0), 8);
}

TEST(FixedTimestep, LeftoverTimeCarriesIntoTheNextFrame) {
  FixedTimestep clock(1.0 / 64.0);
  EXPECT_EQ(clock.Advance(1.5 / 64.0), 1); // half a step left over
  EXPECT_EQ(clock.Advance(0.5 / 64.0), 1); // ...which completes a step now
  EXPECT_EQ(clock.Advance(0.25 / 64.0), 0);
}

TEST(FixedTimestep, LongFramesAreClampedToTheMaximum) {
  // A 5 second stall must not run 5 seconds of physics in one go.
  FixedTimestep clock(1.0 / 512.0, 0.125);
  EXPECT_EQ(clock.Advance(5.0), 64); // 0.125 s / (1/512 s) = 64 steps
}

TEST(FixedTimestep, NegativeTimeRunsNothing) {
  FixedTimestep clock(1.0 / 64.0);
  EXPECT_EQ(clock.Advance(-1.0), 0);
}

TEST(FixedTimestep, ResetDropsLeftoverTime) {
  FixedTimestep clock(1.0 / 64.0);
  clock.Advance(0.75 / 64.0);
  clock.Reset();
  EXPECT_EQ(clock.Advance(0.5 / 64.0), 0);
}

TEST(FixedTimestep, TotalStepsDependOnlyOnTotalTime) {
  // One second delivered as 60 Hz-like frames or as 128 Hz frames: same steps.
  FixedTimestep a(1.0 / 512.0), b(1.0 / 512.0);
  int stepsA = 0, stepsB = 0;
  for (int i = 0; i < 64; i++) stepsA += a.Advance(1.0 / 64.0);
  for (int i = 0; i < 128; i++) stepsB += b.Advance(1.0 / 128.0);
  EXPECT_EQ(stepsA, 512);
  EXPECT_EQ(stepsB, 512);
}
