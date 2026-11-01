# Measurements

Every number in this project's documentation comes from this page, and every
number on this page was measured by running code, not estimated. Each section
says how to reproduce it.

## The machine

- Laptop running Windows 11 Home (build 26300)
- CPU: 13th Gen Intel Core i7-13650HX; 23.7 GB RAM
- GPU: NVIDIA GeForce RTX 4050 Laptop GPU (plus Intel UHD Graphics)
- Display: 1920x1080 at 125% scaling, 144 Hz
- Compiler for these runs: clang 23.1.4 (llvm-mingw 20261020), CMake `release`
  preset (`-O3`). MSVC was not available on this machine; GitHub Actions builds
  with MSVC but does not run the benchmark.

## How to reproduce

```
cmake --preset release
cmake --build --preset release
build/release/stardust_bench.exe
```

Sections 1 and 2 are deterministic: you should get exactly the same numbers on
any machine with the same compiler. Section 3 is wall-clock time and depends on
the machine and what else is running.

## 1. Integrator accuracy

A tiny planet on a circular orbit (radius 10, around a mass of 1000, so speed
10 and period 2π seconds), run for ten orbits. The number is the distance
between the simulated planet and where the exact solution says it should be.
Softening is off for this test because the exact solution assumes pure Newtonian
gravity.

| Steps per second | Semi-implicit Euler | Velocity Verlet |
|---|---|---|
| 50 | 1.78e-01 | 8.38e-02 |
| 100 | 4.45e-02 | 2.09e-02 |
| 200 | 1.11e-02 | 5.23e-03 |
| 600 (the app) | 1.23e-03 | 5.80e-04 |
| 2400 | 7.54e-05 | 3.45e-05 |

Reading it:

- Verlet's error drops about 4x each time the step halves (8.38e-2 → 2.09e-2 →
  5.23e-3): that is what "second order" means.
- On this particular test (a perfectly circular orbit, measured after whole
  orbits) semi-implicit Euler also happens to fall about 4x per halving, but it
  is consistently about twice as far off as Verlet. Its advantage shows more
  clearly in energy: `IntegratorTests.cpp` checks that at 50 steps/s Verlet's
  worst energy error over ten orbits is under a tenth of Euler's.

### Float versus double

Before commit `d3c0978` the simulation stored positions in `float`. Measured
then, with the same test (a scratch program, not part of the repo):

| Steps per second | Verlet, float state | Verlet, double state |
|---|---|---|
| 600 | 1.42e-02 | 5.82e-04 |
| 2400 | 1.22e-02 | 3.64e-05 |

With float, smaller steps stopped helping: rounding in `position += velocity * dt`
dominated. That measurement is why the simulation now uses `double`.

## 2. One stalled frame

The full solar system, 60 simulated seconds at 144 frames per second, with a
single long frame at t = 2 s (what a window drag, alt-tab or breakpoint causes).
"Frame-coupled" reproduces the original loop: one semi-implicit Euler step per
frame, as long as that frame took. The Moon merges into Earth when their centres
come closer than 0.56.

| Stall | Original loop: merges | closest Earth-Moon | Current loop: merges | closest Earth-Moon |
|---|---|---|---|---|
| none | 0 | 0.666 | 0 | 0.681 |
| 0.05 s | 0 | 0.623 | 0 | 0.681 |
| 0.10 s | **1** | 0.565 | 0 | 0.681 |
| 0.20 s | 0 | 0.789 | 0 | 0.681 |
| 0.50 s | **1** | 0.817 | 0 | 0.681 |

The current loop gives the same trajectory whatever the stall, because a long
frame is clamped to 0.1 s and then run as ordinary 1/600 s steps. (The "closest"
value for a run with a merge is measured only while both bodies were alive.)

The audit (`docs/AUDIT.md`) has the same experiment run against the untouched
original source files, with matching conclusions.

## 3. Long-run stability

From the test `Simulation.TheSolarSystemStaysStableForFiveMinutes` and a scratch
run extended to 600 s, at 600 steps per second with velocity Verlet:

| Simulated time | Merges | Closest Earth-Moon | Worst relative energy error | Largest total momentum |
|---|---|---|---|---|
| 60 s | 0 | 0.6813 | 1.28e-08 | 3.07e-12 |
| 300 s | 0 | 0.6706 | 1.57e-08 | 6.93e-12 |
| 600 s | 0 | 0.6673 | 1.63e-08 | 1.01e-11 |

## 4. Cost of one physics step

Median of 7 timed batches, Release build, this laptop. The 10-body case is the
real solar system; larger counts are random non-merging bodies, to show how the
cost grows.

| Bodies | Microseconds per step | Milliseconds per 60 FPS frame (10 steps) |
|---|---|---|
| 10 | 0.358 | 0.0036 |
| 100 | 37.0 | 0.37 |
| 1000 | 3961.7 | 39.6 |
| 3000 | 35038.0 | 350.4 |

Ten times the bodies costs about a hundred times as much (37 → 3962 µs), and three
times the bodies about nine times as much (3962 → 35038 µs): the pairwise loop is
O(N²). At 10 bodies physics is about 0.02% of a 16.7 ms frame. Somewhere between
100 and 1000 bodies it stops fitting in a frame, which is where Barnes-Hut (see
the roadmap) would come in.
