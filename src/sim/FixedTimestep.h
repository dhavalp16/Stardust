#pragma once

namespace sim {

// Turns variable frame times into a whole number of equal physics steps.
//
// Frames take different amounts of time (16.7 ms at 60 Hz, 6.9 ms at 144 Hz,
// 500 ms if Windows stalls), but physics should always advance in identical
// steps, so that the result does not depend on the monitor or on hiccups.
// Each frame's time goes into an "accumulator"; while the accumulator holds at
// least one step, we take a step out of it. Leftover time waits for the next
// frame. This is the classic "Fix Your Timestep!" loop (Glenn Fiedler, 2004).
class FixedTimestep {
public:
  // stepSeconds: the size of one physics step.
  // maxFrameSeconds: longer frames are cut down to this, so one stall can't
  // demand hundreds of steps at once (the "spiral of death").
  explicit FixedTimestep(double stepSeconds, double maxFrameSeconds = 0.1)
      : step_(stepSeconds), maxFrame_(maxFrameSeconds) {}

  // Adds one frame's elapsed time. Returns how many steps to run now.
  int Advance(double frameSeconds) {
    if (frameSeconds < 0.0) frameSeconds = 0.0;
    if (frameSeconds > maxFrame_) frameSeconds = maxFrame_;
    accumulator_ += frameSeconds;

    int steps = 0;
    while (accumulator_ >= step_) {
      accumulator_ -= step_;
      steps++;
    }
    return steps;
  }

  // Forget any leftover time (used on reset).
  void Reset() { accumulator_ = 0.0; }

  double StepSeconds() const { return step_; }

private:
  double step_;
  double maxFrame_;
  double accumulator_ = 0.0; // double: float would slowly lose the leftover fractions
};

} // namespace sim
