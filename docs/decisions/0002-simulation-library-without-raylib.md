# 0002: The simulation is a library that does not know raylib exists

*Status: accepted, 2 November 2026*

## Context

`Planet` held physics state (position, velocity, mass, radius) next to a model
path, a colour, a display name and slider ranges. The collision code cleared the
UI selection and used raylib's random numbers. Physics only ran inside the frame
loop of a windowed program. Nothing could be tested without opening a window, and
the physics could not be reused (a headless benchmark, a server, another renderer).

## Decision

- `src/sim/` is a static library, `stardust_sim`, that includes only the C++
  standard library. It has its own `Vec3` type.
- Physics state is `sim::Body` (position, velocity, mass, radius, alive).
  Everything else about a planet lives in the app's `PlanetInfo`. The app keeps
  `bodies[i]`, `planetInfo[i]` and `planetModels[i]` index-paired.
- Collisions report what happened (`MergeEvent`) instead of reaching into the UI.
  The app reacts: spawns debris, clears the selection, widens the slider range.
- The app converts `sim::Vec3` to raylib's `Vector3` at the boundary
  (`src/app/Convert.h`).

## Consequences

- Good: 52 unit tests run in about a second with no window; CI runs them on Linux
  with no graphics packages, which proves the boundary holds.
- Good: the benchmark (`tools/bench`) reuses the exact same code as the app.
- Good: the design maps to how engines separate simulation from presentation.
- Cost: one conversion per drawn object per frame (trivial at 10 bodies) and two
  vector types to know about.
- Cost: index pairing must be kept consistent. That is why merged bodies are
  flagged dead instead of erased.
