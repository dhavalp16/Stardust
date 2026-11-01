# 6. Architecture: how the pieces fit

**Architecture** is the shape of a program: what the parts are, what each is
responsible for, and which parts are allowed to know about which. Good architecture
makes the program easy to test, change and explain.

## 6.1 The big picture

```
                 +--------------------------------------------+
                 |  Stardust.exe  (src/app/, uses raylib)      |
                 |  window, input, camera, HUD, drawing, debris|
                 +-----------+------------------+--------------+
                             | uses             | uses
                             v                  v
           +-----------------------+   +----------------------+
           | stardust_sim          |   | stardust_io          |
           | (src/sim/)            |   | (src/io/)            |
           | physics, no raylib    |   | HDR decoder, no raylib|
           +-----------+-----------+   +----------+-----------+
                       ^                          ^
                       | also used by             |
          +------------+-------------+------------+
          |                          |
  stardust_tests.exe          stardust_bench.exe
  (tests/, GoogleTest)        (tools/bench/)
```

The arrows only point one way. `sim` and `io` know nothing about the app, raylib,
windows or the GPU. That one rule is what makes them testable without a window and
reusable by the benchmark (`docs/decisions/0002-simulation-library-without-raylib.md`).
CI checks it for free: the Linux job builds `sim`, `io`, the tests and the
benchmark without raylib at all (`.github/workflows/ci.yml`).

## 6.2 The folders

| Folder | Responsibility | Depends on |
|---|---|---|
| `src/sim/` | The physics: `Vec3`, `Body`, gravity, integrator, collisions, fixed timestep, starting scenario, diagnostics | C++ standard library only |
| `src/io/` | Decoding the HDR sky | C++ standard library only |
| `src/app/` | The program: window, frame loop, input, camera, HUD, debris, screenshots | `sim`, `io`, raylib, `third_party/` |
| `third_party/` | `raygui.h`, `rlights.h`, code we did not write | raylib |
| `tests/` | Unit tests | `sim`, `io`, GoogleTest |
| `tools/bench/` | Benchmark | `sim`, `io` |
| `assets/`, `resources/` | Data: models, sky, shaders | (none) |

## 6.3 The sim library, file by file

| File | What it does |
|---|---|
| `Vec3.h` | 3D vector of doubles with `+ - * /`, `Dot`, `Length` |
| `Body.h` | Physical state of one body: position, velocity, mass, radius, alive |
| `Gravity.h/.cpp` | `ComputeAccelerations`: softened Newtonian gravity, each pair once |
| `Integrator.h/.cpp` | `Kick` (v += a·dt) and `Drift` (x += v·dt), the building blocks |
| `Collision.h/.cpp` | `ResolveCollisions`: merges overlapping bodies, returns `MergeEvent`s |
| `Simulation.h/.cpp` | `Simulation` class: owns bodies, `Step(dt)` = gravity + Verlet + collisions |
| `FixedTimestep.h` | Turns frame times into a whole number of 1/600 s steps |
| `SolarSystem.h/.cpp` | The starting table (`PlanetSpec`), `BuildBodies`, `CircularOrbitSpeed` |
| `Diagnostics.h/.cpp` | `TotalMomentum`, `CenterOfMass`, `TotalEnergy` for checking correctness |

Each file has one job. You can read `Simulation::Step` (`src/sim/Simulation.cpp`)
and understand a whole physics step from five lines, because the details live in
well-named functions.

## 6.4 The app, file by file

| File | What it does |
|---|---|
| `main.cpp` | Loads everything, holds app state, runs the frame loop (input → update → draw), cleans up |
| `Input.h/.cpp` | Free-fly camera, mouse-wheel zoom, planet picking |
| `HUD.h/.cpp` | Reticle, planet panel with sliders, stats panel, help bar, key overlay |
| `HudLayout.h` | Panel rectangles shared by HUD (drawing) and Input (click blocking) |
| `PlanetInfo.h` | Per-planet presentation data: name, model, colour, spin, slider ranges |
| `Debris.h/.cpp` | Spawning, updating and drawing collision debris |
| `Convert.h` | `sim::Vec3` → raylib `Vector3` |
| `AssetPaths.h/.cpp` | Finds the folder with `assets/` and switches to it |
| `ScreenshotScript.h/.cpp` | `--screenshots`: scripted run that saves pictures |
| `ThirdPartyImpl.cpp` | Compiles the raygui and rlights implementations, once |

## 6.5 One frame, step by step

From `src/app/main.cpp`, starting at the `while` loop (line 217):

1. **Input** (`// 1. Input`): Space toggles play/pause; R rebuilds the bodies and
   resets everything; `ProcessDesktopInput` moves the camera and handles clicks;
   if tracking, `UpdateTrackingCamera` eases the camera towards the planet.
2. **Update** (`// 2. Update`), only when playing:
   - `physicsClock.Advance(dt)` says how many 1/600 s steps to run;
   - each `simulation.Step(...)` advances physics and returns merge events;
   - for each event the app spawns debris, clears the selection if the victim was
     selected, and widens the survivor's slider ranges;
   - planet spin angles and debris advance.
   Then the light is moved to the Sun and the camera position is sent to the shader.
3. **Draw** (`// 3. Draw`): sky, planets, debris (3D), then the sun glow, reticle,
   panels, help bar, key overlay (2D), then `EndDrawing()` presents the frame.

Drawing never changes simulation state. Before the overhaul the draw loop also
advanced spin and moved debris, which mixes two jobs and makes bugs hard to find.

## 6.6 Index-paired arrays

The app keeps three arrays where index `i` always means the same planet
(`src/app/main.cpp`, "Three index-paired arrays"):

```
bodies[i]        physics        (sim::Body)
planetInfo[i]    presentation   (PlanetInfo)
planetModels[i]  3D model       (raylib Model)
```

This is a simple **data-oriented** layout: data used together lives together (the
physics loop only touches `bodies`, densely packed). The cost is a rule everyone must
follow: never erase or reorder one array without the others. That is why merged
bodies are marked `isAlive = false` instead of being erased, and why the selection is
an index.

## 6.7 Events instead of reaching across

When two bodies merge, the physics must not clear the HUD selection or spawn debris
itself: that would make `sim` depend on the app. Instead `ResolveCollisions` returns a
list of `MergeEvent { survivor, victim, victimPosition, victimVelocity, victimRadius }`
(`src/sim/Collision.h`), and the app decides what to do (`src/app/main.cpp`, the loop
over `simulation.Step(...)`). This is a lightweight form of the **observer** /
**event** pattern: the lower layer reports facts, the higher layer reacts.

## 6.8 Configuration as data

Tunable numbers live in small structs with defaults: `GravityParams { G, softening }`,
`CollisionParams { overlapFactor, anchorIndex }`, `SimulationParams { gravity,
collisions, integrator }`. Tests change them freely (for example, softening off for
the exact-orbit tests). The starting system is a table of `PlanetSpec` rows
(`src/sim/SolarSystem.cpp`), so adding a planet is adding a row.

## 6.9 What a professional team would still change

Being honest about the limits is part of explaining an architecture:

- `main()` still owns all app state as local variables. Grouping it into an `App`
  struct with `Update` and `Draw` functions would make the loop easier to test.
- raylib resources (`Model`, `Shader`) are freed by hand; RAII wrappers would make
  leaks impossible.
- `DrawKeyPressOverlay` keeps hidden `static` state.
- `PlanetSpec` mixes physics and presentation fields in one row. That was a
  deliberate trade (one row can't get out of step), but a purist would split it.

## Exercises

1. Draw the diagram in 6.1 from memory. Then add `stardust_tests` and
   `stardust_bench` and their arrows.
2. Try to `#include "raylib.h"` in `src/sim/Gravity.cpp` and use `DrawText`. What
   happens when you build the Linux CI configuration locally with
   `cmake -S . -B build/noapp -DSTARDUST_BUILD_APP=OFF`? Why is that failure a good thing?
3. Add a `planetCount` field to the top-right panel that counts only bodies with mass
   above 1. Which file(s) do you need to touch? Which must you *not* touch?
4. Write down every place that would break if `ResolveCollisions` erased the victim
   from `bodies` instead of marking it dead.
