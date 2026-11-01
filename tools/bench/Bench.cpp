// stardust_bench: reproduces every number quoted in docs/MEASUREMENTS.md.
//
// Build the Release preset and run build/release/stardust_bench.exe. It needs
// no window and no assets. Timings depend on the machine; the accuracy and
// stability numbers do not (they are deterministic).
#include "sim/Diagnostics.h"
#include "sim/FixedTimestep.h"
#include "sim/Simulation.h"
#include "sim/SolarSystem.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

namespace {

const double kTwoPi = 6.283185307179586;

// --- 1. Integrator accuracy -------------------------------------------------

// Distance from the exact circular orbit after ten orbits (see IntegratorTests).
double OrbitError(sim::IntegratorType type, double h) {
  sim::Body sun, planet;
  sun.mass = 1000.0f;
  planet.mass = 1e-6f;
  planet.position = {10, 0, 0};
  planet.velocity = {0, 0, 10};
  sim::SimulationParams params;
  params.integrator = type;
  params.gravity.softening = 0.0f;
  params.collisions.overlapFactor = 0.0f; // never merge
  sim::Simulation simulation({sun, planet}, params);
  int steps = static_cast<int>(std::lround(10 * kTwoPi / h));
  for (int i = 0; i < steps; i++) simulation.Step(h);
  double t = steps * h;
  return sim::Length(simulation.Bodies()[1].position - sim::Vec3{10 * std::cos(t), 0, 10 * std::sin(t)});
}

void IntegratorAccuracy() {
  std::printf("1. Integrator accuracy: distance from the exact orbit after 10 orbits\n");
  std::printf("   (radius 10 around mass 1000, double precision, no softening)\n\n");
  std::printf("   %-12s %-18s %-14s\n", "steps/s", "semi-impl. Euler", "velocity Verlet");
  for (double rate : {50.0, 100.0, 200.0, 600.0, 2400.0}) {
    std::printf("   %-12.0f %-18.2e %-14.2e\n", rate,
                OrbitError(sim::IntegratorType::SemiImplicitEuler, 1.0 / rate),
                OrbitError(sim::IntegratorType::VelocityVerlet, 1.0 / rate));
  }
  std::printf("\n");
}

// --- 2. A stalled frame -----------------------------------------------------

struct StallResult {
  int merges;
  double closestEarthMoon;
};

// 60 s at 144 FPS with one long frame at t = 2 s.
// frameCoupled = true reproduces the original loop: one semi-implicit Euler
// step per frame, as long as the frame took. Otherwise the current loop.
StallResult RunWithStall(bool frameCoupled, double stallSeconds) {
  sim::SimulationParams params;
  if (frameCoupled) params.integrator = sim::IntegratorType::SemiImplicitEuler;
  sim::Simulation simulation(sim::BuildBodies(sim::SolarSystemSpecs()), params);
  sim::FixedTimestep clock(1.0 / 600.0);
  StallResult result{0, 1e9};

  const int frames = 144 * 60;
  for (int f = 0; f < frames; f++) {
    double frame = (f == 288 && stallSeconds > 0.0) ? stallSeconds : 1.0 / 144.0;
    if (frameCoupled) {
      result.merges += static_cast<int>(simulation.Step(frame).size());
    } else {
      int steps = clock.Advance(frame);
      for (int s = 0; s < steps; s++)
        result.merges += static_cast<int>(simulation.Step(clock.StepSeconds()).size());
    }
    const auto &b = simulation.Bodies();
    if (b[3].isAlive && b[9].isAlive)
      result.closestEarthMoon =
          std::min(result.closestEarthMoon, sim::Length(b[9].position - b[3].position));
  }
  return result;
}

void StalledFrame() {
  std::printf("2. One stalled frame at t = 2 s, then 144 FPS until t = 60 s\n");
  std::printf("   (merges happen when Earth-Moon distance < 0.56)\n\n");
  std::printf("   %-8s %-34s %-34s\n", "stall", "frame-coupled (original loop)",
              "fixed 1/600 s Verlet + clamp");
  for (double stall : {0.0, 0.05, 0.1, 0.2, 0.5}) {
    StallResult a = RunWithStall(true, stall), b = RunWithStall(false, stall);
    std::printf("   %-8.2f merges=%d closest=%-19.3f merges=%d closest=%.3f\n", stall, a.merges,
                a.closestEarthMoon, b.merges, b.closestEarthMoon);
  }
  std::printf("\n");
}

// --- 3. Cost per physics step -------------------------------------------------

// Random bodies in a disc that never merge, so N stays fixed.
std::vector<sim::Body> RandomBodies(int n) {
  std::mt19937 rng(12345);
  std::uniform_real_distribution<double> pos(-100.0, 100.0);
  std::vector<sim::Body> bodies(static_cast<size_t>(n));
  for (sim::Body &b : bodies) {
    b.position = {pos(rng), 0.1 * pos(rng), pos(rng)};
    b.mass = 1.0f;
    b.radius = 0.001f;
  }
  return bodies;
}

// Median wall-clock microseconds for one Step, over several timed batches.
double MicrosecondsPerStep(std::vector<sim::Body> bodies, int stepsPerBatch) {
  sim::SimulationParams params;
  params.collisions.overlapFactor = 0.0f;
  sim::Simulation simulation(std::move(bodies), params);
  std::vector<double> samples;
  for (int batch = 0; batch < 7; batch++) {
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < stepsPerBatch; i++) simulation.Step(1.0 / 600.0);
    auto end = std::chrono::steady_clock::now();
    samples.push_back(std::chrono::duration<double, std::micro>(end - start).count() /
                      stepsPerBatch);
  }
  std::sort(samples.begin(), samples.end());
  return samples[samples.size() / 2];
}

void StepCost() {
  std::printf("3. Cost of one physics step (median of 7 batches, this machine)\n\n");
  std::printf("   %-8s %-14s %-16s\n", "bodies", "us per step", "ms per 60 FPS frame (10 steps)");
  double tenBodies = MicrosecondsPerStep(sim::BuildBodies(sim::SolarSystemSpecs()), 20000);
  std::printf("   %-8d %-14.3f %-16.4f\n", 10, tenBodies, tenBodies * 10 / 1000);
  for (int n : {100, 1000, 3000}) {
    int steps = n <= 100 ? 2000 : (n <= 1000 ? 20 : 3);
    double us = MicrosecondsPerStep(RandomBodies(n), steps);
    std::printf("   %-8d %-14.1f %-16.2f\n", n, us, us * 10 / 1000);
  }
  std::printf("\n");
}

} // namespace

int main() {
  std::printf("stardust_bench\n\n");
  IntegratorAccuracy();
  StalledFrame();
  StepCost();
  return 0;
}
