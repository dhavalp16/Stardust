# 0004: Positions and velocities in double precision

*Status: accepted, 2 November 2026*

## Context

After switching to velocity Verlet, a test orbit (radius 10, ten orbits, 600
steps/s) still ended 1.4e-2 units from the exact answer, and smaller steps did
not help (1.2e-2 at 2400 steps/s). A plain double-precision implementation of the
same integrator ended 5.8e-4 away. A `float` holds about 7 significant digits:
adding a 0.017-unit move to a position of 10 every step loses digits each time,
and over tens of thousands of steps that rounding is bigger than the integrator's
own error.

## Decision

- `sim::Vec3` stores `double`. Time steps are `double`.
- `Body::mass` and `Body::radius` stay `float`: they are parameters, not
  accumulated step after step, and the raygui sliders edit them through a
  `float*`.
- The app converts to `float` only when handing positions to raylib for drawing.

## Consequences

- Good: the measured error at the app's step dropped from 1.4e-2 to 5.7e-4, and
  it falls with smaller steps as theory says (second order).
- Cost: 8 more bytes per vector and slightly slower maths. At 10 bodies the step
  still costs about 0.36 µs (measured).
- Cost: a conversion at the render boundary. Engines with huge worlds do the
  same thing for the same reason ("double on the CPU, float on the GPU",
  sometimes with camera-relative rendering).
