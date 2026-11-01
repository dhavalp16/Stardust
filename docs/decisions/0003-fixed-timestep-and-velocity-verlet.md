# 0003: Fixed 1/600 s physics steps with velocity Verlet

*Status: accepted, 2 November 2026*

## Context

Physics advanced once per rendered frame by however long the frame took. The
"10 sub-steps" kicked velocities ten times while positions stayed frozen, which
is mathematically one semi-implicit Euler step. Consequences, all measured (see
`docs/MEASUREMENTS.md`):

- the trajectory depended on the monitor's refresh rate;
- one long frame (0.1 s or 0.5 s) caused the Moon to merge into Earth.

## Options considered

1. **Clamp `dt` only.** Stops the worst explosions but results still depend on
   frame rate.
2. **Fixed timestep with accumulator + clamp** (Glenn Fiedler, "Fix Your
   Timestep!"). Same steps whatever the frame rate; standard in game engines.
3. **Adaptive step size** (smaller steps when bodies are close). More accurate
   for close encounters but harder to reason about and to make deterministic.

For the integrator: semi-implicit Euler (first order), velocity Verlet (second
order, symplectic), RK4 (fourth order but not symplectic, so energy drifts over
long runs, and four force evaluations per step).

## Decision

- Option 2 with a step of 1/600 s (the same count as the old 10 sub-steps at
  60 FPS) and a maximum of 0.1 s of simulated time per frame.
- Velocity Verlet in kick-drift-kick form. Semi-implicit Euler stays available as
  `IntegratorType::SemiImplicitEuler` so tests and the benchmark can compare.
- Forces are computed twice per step (start and end) rather than cached between
  steps, because the HUD sliders can change masses between steps.

## Consequences

- Good: bit-identical results at different frame rates (tested); a stalled frame
  just pauses the simulation briefly (tested).
- Good: energy error over 300 s of the full system is about 1.6e-8 (measured).
- Cost: if a frame takes longer than 0.1 s, the simulation runs slower than real
  time for that frame. That is the intended trade.
- Cost: the drawn position can lag the true one by up to one step (1/600 s);
  interpolating between the last two states would remove that (see roadmap).
- Cost: 2 force evaluations per step instead of 1. At 10 bodies a step costs
  about 0.36 µs (measured), so this does not matter.
