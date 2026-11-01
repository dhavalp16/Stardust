# 10. Rebuild it yourself

Reading code is not the same as being able to write it. These exercises have you
rebuild the important parts of Stardust yourself, with the existing tests as the
judge. Do them after the chapter they depend on.

## How every exercise works

1. Make a practice branch so you can't damage the real code:
   ```
   git switch -c practice/<name>
   ```
2. Delete the body of the function named in the exercise and replace it with the
   simplest thing that compiles (for example `return {};`). Do **not** look at the
   original while you write your version. (`git show HEAD:<file>` brings it back if
   you are truly stuck; try for at least an hour first.)
3. Build and run the named tests:
   ```
   cmake --build --preset debug
   ctest --preset debug -R <TestNamePattern>
   ```
   They should fail with your empty body, then pass with your version.
4. Run **all** the tests (`ctest --preset debug`). Your version should pass every one.
5. Compare with the original (`git diff HEAD`). Note anything you did differently and
   whether it matters.
6. Throw the branch away (`git switch docs/learning-guide`, then
   `git branch -D practice/<name>`) or keep it as a record.

Difficulty: ★ an hour, ★★ an evening, ★★★ a weekend.

---

## E1. Vec3 operators ★ (after chapters 1-2)

**Rebuild:** every operator and `Dot`, `LengthSquared`, `Length` in `src/sim/Vec3.h`.
**Check:** `ctest --preset debug -R Vec3`, then everything (all physics uses them).
**Then:** add `Cross(a, b)` with your own test using `(1,0,0) × (0,1,0) = (0,0,1)`.

## E2. Gravity ★★ (after chapter 3)

**Rebuild:** `ComputeAccelerations` in `src/sim/Gravity.cpp`, including softening and
the "each pair once" trick.
**Check:** `ctest --preset debug -R Gravity`, then `-R Simulation` (the stability test
will fail if anything is subtly wrong).
**Then:** write it the "simple" way (for each i, loop over every j ≠ i). Time both
with `stardust_bench` section 3. How much faster is the pair trick at 1000 bodies?

## E3. Kick, Drift and the two integrators ★★ (after chapter 4)

**Rebuild:** `Kick` and `Drift` (`src/sim/Integrator.cpp`) and both branches of
`Simulation::Step` (`src/sim/Simulation.cpp`).
**Check:** `ctest --preset debug -R "Integrator|Simulation"`.
**Then:** add a third option, explicit Euler (move first with the *old* velocity), and
a test showing its energy grows over ten orbits while the others don't.

## E4. The fixed-timestep clock ★ (after chapter 4)

**Rebuild:** `FixedTimestep::Advance` (`src/sim/FixedTimestep.h`).
**Check:** `ctest --preset debug -R "FixedTimestep|Simulation"`.
**Then:** add `double Alpha() const` returning `accumulator / step`, with a test. You
will need it for roadmap item 3 (interpolation).

## E5. Merging bodies ★★ (after chapter 3)

**Rebuild:** `ResolveCollisions` (`src/sim/Collision.cpp`): threshold, survivor rule,
momentum, centre of mass, volume, event.
**Check:** `ctest --preset debug -R Collision`.
**Then:** run the app, push two planets together with the radius slider, and confirm
the debris and the HUD behave as before.

## E6. Starting conditions ★ (after chapter 3)

**Rebuild:** `CircularOrbitSpeed` and `BuildBodies` (`src/sim/SolarSystem.cpp`),
including the Sun's momentum-cancelling velocity.
**Check:** `ctest --preset debug -R "SolarSystem|Simulation"`.
**Then:** add Pluto at radius 170 (mass 0.002, radius 0.2, model `assets/pluto.glb`).
Which test needs updating, and why?

## E7. Energy diagnostics ★ (after chapter 3)

**Rebuild:** `TotalEnergy` (`src/sim/Diagnostics.cpp`).
**Check:** `ctest --preset debug -R "Integrator|Simulation"`. If you forget the
softening in the potential, which test notices? (Try it. If none does, write
the test that would: what situation makes the difference big?)

## E8. The HDR decoder ★★★ (after chapter 5)

**Rebuild:** `ReadScanline` and the main loop of `LoadHdr` (`src/io/HdrLoader.cpp`).
Keep `ToneMapChannel` and the table.
**Check:** `ctest --preset debug -R HdrLoader`, then run the app and confirm the sky
appears, then run `stardust_bench` section 4 and compare the time.
**Then:** try reading with `fgetc` one byte at a time instead of from memory, and
measure the difference yourself.

## E9. Picking ★★ (after chapter 5)

**Rebuild:** the left-click block in `ProcessDesktopInput` (`src/app/Input.cpp`, from
`if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)`), using raylib's `GetMouseRay` but
writing your *own* ray-sphere intersection instead of `GetRayCollisionSphere`.
**Check:** by eye: steps 16-19 of `docs/CHECK_BY_EYE.md`.
**Maths hint:** a point on the ray is `o + t·d`; it is on the sphere when
`|o + t·d - c|² = r²`; that is a quadratic in `t`.

## E10. A whole N-body simulation from an empty file ★★★ (after chapters 1-4)

Without looking at Stardust, write a single `main.cpp` that:

1. defines a vector struct with the operators you need;
2. sets up a Sun (mass 1000) and one planet at radius 10 with circular speed;
3. integrates with velocity Verlet at 600 steps per second for 10 orbits (about
   62.8 s);
4. prints the distance from the exact position `(10 cos t, 0, 10 sin t)` and the
   relative energy change.

**Check:** with double precision and no softening, your distance should be close to
the 600 steps/s Verlet row of `docs/MEASUREMENTS.md` section 1 (5.80e-04), and your
energy change should be tiny. If you get something much bigger, check: are you
updating velocity with the *new* forces in the second half-kick?

This is the exercise to be able to do in an interview on a whiteboard.

## E11. RAII wrappers for raylib resources ★★ (after chapter 1)

**Build:** a small class `ModelHandle` that loads a model in its constructor, unloads
it in its destructor, and cannot be copied (but can be moved). Use it for the planet
models in `main.cpp` and delete the manual `UnloadModel` loop.
**Check:** the app runs and closes without errors in the console; run the screenshot
script to confirm the planets still appear.

## E12. A test you write from scratch ★ (after chapter 7)

Write a new test file `tests/DiagnosticsTests.cpp` for `CenterOfMass` and
`TotalMomentum` (two or three cases each), add it to `CMakeLists.txt` in the
`stardust_tests` list, and get it passing.
