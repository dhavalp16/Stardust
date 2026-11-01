# 4. Numerical integration: moving the world forward in time

Chapter 3 gave the rules (gravity gives acceleration). This chapter is about how a
computer turns those rules into motion, one small step at a time. It is the most
important chapter for interviews: it is where most of the overhaul's bugs and fixes
were.

## 4.1 The problem

We know acceleration at any moment (from positions, via gravity). We want positions
over time. In continuous maths you would **integrate**: velocity is the integral of
acceleration, position is the integral of velocity. A computer cannot do continuous
time; it takes **steps** of size `dt` (also written `h`, the **time step**) and
approximates. A method for that is a **numerical integrator**.

Every integrator makes some error per step. Two things matter:

- **Order**: how the error shrinks when you shrink the step. A *first-order*
  method's error halves when `dt` halves; *second-order* quarters it (error ∝ dt²).
- **Long-term behaviour**: does energy wander away over thousands of steps (orbits
  spiral out or in), or stay close to the true value?

## 4.2 Explicit Euler (the naive one)

```
x_new = x + v · dt
v_new = v + a(x) · dt
```
Uses the old velocity to move. On an orbit it steadily *adds* energy: planets spiral
outwards. Not used in Stardust; it is the method to recognise and avoid.

## 4.3 Semi-implicit (symplectic) Euler

Swap the order: update velocity first, then move with the *new* velocity.

```
v_new = v + a(x) · dt        // "kick"
x_new = x + v_new · dt       // "drift"
```
Still first order, but it is **symplectic**: a property that, roughly, means it
preserves the geometry of the motion. Energy does not drift away; it wobbles
around the right value. Orbits stay closed. This is what the original Stardust
effectively did. In the code it is `IntegratorType::SemiImplicitEuler`
(`src/sim/Simulation.cpp:7-11`).

## 4.4 Velocity Verlet (kick-drift-kick, also called leapfrog)

```
v_half = v + a(x) · dt/2         // half kick
x_new  = x + v_half · dt         // drift
v_new  = v_half + a(x_new) · dt/2 // half kick with the new forces
```
Second order and symplectic. Same idea as before, but it uses the average of the
forces at the start and end of the step. It is the standard integrator for
gravitational N-body problems and molecular dynamics. In the code it is the default
(`src/sim/Simulation.cpp:22-26`): `Kick`, `Drift`, `ComputeAccelerations`, `Kick`.

Why not **RK4** (Runge-Kutta 4th order), which is more accurate per step? It is not
symplectic, so over very long runs its energy drifts; it also needs four force
evaluations per step. For orbits run indefinitely, a symplectic method usually wins.

### Measured here

`stardust_bench` section 1 runs a tiny planet round a circular orbit (radius 10,
period 2π) for ten orbits and measures the distance from the exact answer
(`docs/MEASUREMENTS.md` section 1):

| Steps per second | Semi-implicit Euler | Velocity Verlet |
|---|---|---|
| 50 | 1.78e-01 | 8.38e-02 |
| 100 | 4.45e-02 | 2.09e-02 |
| 600 (the app) | 1.23e-03 | 5.80e-04 |

Verlet's error drops 4× when the step halves (second order;
`Integrator.VerletErrorShrinksWithTheSquareOfTheStep` tests the ratio is between 3.5
and 4.5). On this symmetric test semi-implicit Euler also happens to fall about 4×
but is about twice as far off. The bigger difference is in energy: at 50 steps/s,
Verlet's worst energy error is under a tenth of Euler's
(`Integrator.VerletConservesEnergyFarBetterThanEulerAtLargeSteps`).

Be precise about this in an interview: "Verlet is second order and symplectic; on my
test orbit it was about twice as accurate in position and more than ten times better
in energy at large steps." Do not say "Euler was unstable"; semi-implicit Euler is
stable, just less accurate.

## 4.5 The time step and the frame loop

A game runs a **frame loop**: read input, update, draw, repeat. Each frame takes a
different amount of time: about 16.7 ms at 60 Hz, 6.9 ms at 144 Hz, and sometimes
hundreds of milliseconds when Windows stalls the program (dragging the window, a
breakpoint, saving a screenshot).

**The original bug.** Physics used the frame's duration directly as `dt`:

```cpp
float dt = GetFrameTime();
// 10 kicks of dt/10 with positions frozen, then one drift of dt
```

Three problems, all measured (`docs/AUDIT.md`, `docs/MEASUREMENTS.md` section 2):

1. The "10 sub-steps" changed velocity ten times but moved positions only once, so
   all ten kicks used the same positions: mathematically one Euler step.
2. Results depended on the monitor: 60 Hz and 144 Hz gave different orbits.
3. One long frame became one giant step. A single 0.1 s frame (instead of 0.007 s)
   was enough to knock the Moon into Earth.

## 4.6 The fix: a fixed timestep with an accumulator

The standard solution (Glenn Fiedler, "Fix Your Timestep!"): physics always advances
in **equal** steps (here 1/600 s), and the frame's real time is fed into an
**accumulator** that pays out whole steps:

```cpp
int Advance(double frameSeconds) {
  if (frameSeconds > maxFrame_) frameSeconds = maxFrame_;  // clamp: 0.1 s
  accumulator_ += frameSeconds;
  int steps = 0;
  while (accumulator_ >= step_) { accumulator_ -= step_; steps++; }
  return steps;
}
```
(`src/sim/FixedTimestep.h:22`). The app then runs that many steps
(`src/app/main.cpp:253`). Leftover time waits for the next frame.

- At 60 Hz each frame runs 10 steps; at 144 Hz it alternates 4 and 5. Same physics.
- The **clamp** means a 5-second stall runs only 0.1 s of physics (60 steps), not
  3000 steps. Otherwise a slow frame causes more steps, which make the next frame
  slower, which causes even more steps: the **spiral of death**. The cost is that the
  simulation briefly runs slower than real time after a stall. That is the right
  trade.
- The accumulator is a `double`: a `float` would slowly lose the leftover fractions.

Tested by `Simulation.ResultDoesNotDependOnTheFrameRate` (two frame rates give
bit-identical planets) and `Simulation.ALongStallDoesNotLoseTheMoon` (the original bug
as a regression test).

What is not done: **interpolation**. The drawn position is the last whole step's,
up to 1/600 s old. With fewer steps per second you would blend the last two states
(see `docs/ROADMAP.md` item 3).

## 4.7 Floating point and why the state is double

Computers store decimals as **floating point**: a sign, a set of significant digits
(the **mantissa**) and an **exponent** (like scientific notation, 1.234 × 10⁵ but in
base 2). A `float` keeps about 7 significant decimal digits; a `double` about 16.

Consequence: adding a small number to a big one loses the small one's last digits.
A planet at x = 10 moving 0.017 per step: `10.000000 + 0.016667` keeps only 7 digits
in total, so the last few digits of the step are rounded away. Every step. Over tens
of thousands of steps that rounding adds up.

Measured (`docs/MEASUREMENTS.md`, "Float versus double"): with `float` positions,
Verlet at 600 steps/s ended 1.4e-2 from the exact orbit after ten orbits, and 2400
steps/s barely helped (1.2e-2). With `double` it is 5.8e-4 and keeps improving with
smaller steps. So `sim::Vec3` is double (`src/sim/Vec3.h`), and rendering converts to
float at the end (`src/app/Convert.h`), because the GPU only needs enough precision
to place pixels. See `docs/decisions/0004-double-precision-state.md`.

General lessons:

- Never compare floating-point results with `==` unless you know they are computed
  identically; use a tolerance (`EXPECT_NEAR` in the tests).
- Precision problems look like "the method is inaccurate". Test with double to
  separate the two.

## 4.8 Collisions inside the step

Collisions are checked once per step after moving (`Simulation::Step`, last line).
At 600 steps/s the fastest body (Mercury, about 16 units/s) moves 0.027 per step,
far less than any planet's size, so nothing can jump through another. A much faster
or smaller body could; the fix is **continuous collision detection** (roadmap 12).

## Exercises

1. On paper, run explicit Euler and semi-implicit Euler for 3 steps of a body at
   x = 1, v = 0, with a = -x (a spring), dt = 0.5. Which one's energy `½v² + ½x²` grows?
2. In `src/sim/Simulation.cpp`, switch the default integrator to `SemiImplicitEuler`
   (in `Simulation.h`), rebuild, run the tests. Which tests fail and why? Switch back.
3. In `src/app/main.cpp`, change the physics step from `1.0 / 600.0` to `1.0 / 30.0`.
   Run the app for a minute. What happens to the Moon? (Measure: watch for
   "Planets: 9".) Change it back.
4. Write a test that feeds a `FixedTimestep(1.0/512.0)` 1000 frames of random
   lengths between 0 and 0.05 s and checks that the total number of steps equals
   `floor(total time × 512)` give or take one.
5. Explain the spiral of death to a friend in three sentences.
