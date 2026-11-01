# Roadmap

Bigger ideas that were deliberately **not** built during the overhaul, so you can
build them yourself. Each has a goal, a suggested approach, how to know you are
done, and roughly how hard it is. Do them in roughly this order: each one teaches
something the next one uses.

Difficulty: ★ an evening, ★★ a weekend, ★★★ a week or more.

---

## 1. Energy and momentum readout on the HUD ★

**Goal:** show the total energy and its drift (as a percentage of the value at the
last reset) in the top-right panel, so you can *see* the integrator working.

**Approach:** `sim::TotalEnergy` already exists (`src/sim/Diagnostics.cpp`). Store
the energy at reset, compute it each frame, print `(E - E0) / E0`. Merges are
inelastic and change the energy, so either reset `E0` after each merge event or
show "energy lost in merges" separately.

**Done when:** a 5-minute run with no merges shows drift around 1e-6 % or less, and
switching `SimulationParams::integrator` to `SemiImplicitEuler` shows a visibly
larger wobble.

## 2. Orbit trails ★★

**Goal:** a fading line behind each planet showing where it has been.

**Approach:** per planet, a fixed-size ring buffer (`std::array<Vector3, 256>` plus
a write index) of recent positions, appended every N physics steps. Draw with
`DrawLine3D` between consecutive points, with alpha falling off with age. Keep it
in the app (it is presentation), not in `sim`.

**Done when:** trails are smooth at 60 and 144 Hz, cost no allocations per frame,
and clear on reset.

## 3. Interpolated rendering ★★

**Goal:** draw planets exactly where they would be at the current instant, not at
the last whole physics step.

**Approach:** keep the previous step's positions. `FixedTimestep` knows the leftover
time in its accumulator; expose `alpha = accumulator / step`. Draw
`lerp(previous, current, alpha)`. See the end of "Fix Your Timestep!" by Glenn
Fiedler.

**Done when:** you can explain why this matters more at 60 steps/s than at 600.

## 4. Sky sampled by direction ★★

**Goal:** remove the seam and pole pinching that a texture on a UV sphere has.

**Approach:** in `resources/shaders/glsl330/skybox.fs`, compute the view direction
per pixel and convert it to equirectangular coordinates:
`u = atan(d.z, d.x) / (2π) + 0.5`, `v = asin(d.y) / π + 0.5`. The vertex shader
passes the direction. You will need to match today's orientation (the sphere is
currently rotated 90° about X).

**Done when:** the screenshot script's pictures look the same as before except
where the seam used to be.

## 5. Linear-space lighting ★★

**Goal:** correct colour handling. Today `lighting.fs` treats texture colours as
linear and then applies `pow(1/2.2)`, which brightens and washes out colours, and
applies the power to alpha too.

**Approach:** decode textures from sRGB to linear (`pow(c, 2.2)`) before lighting,
encode once at the end, leave alpha alone. Or load textures as sRGB on the GPU.

**Done when:** you can explain gamma, sRGB and why lighting maths must happen in
linear space. Expect the picture to change; compare screenshots and decide.

## 6. Pluto and a scenario file ★★

**Goal:** add Pluto (the model is already in `assets/`) and load the starting
system from a text file (JSON or a simple CSV) instead of the table in
`src/sim/SolarSystem.cpp`.

**Done when:** you can add a body without recompiling, a malformed file gives a clear
error, and `SolarSystemTests` still pass for the default file.

## 7. Barnes-Hut gravity ★★★

**Goal:** thousands of bodies. Measured today: 100 bodies cost 37 µs per step,
1000 bodies about 4 ms, 3000 bodies about 35 ms (see `docs/MEASUREMENTS.md`); the
cost grows with N², so a 60 FPS frame of 10 steps runs out at a few hundred bodies.

**Approach:** build an octree each step; for a distant cell, use its total mass at
its centre of mass instead of every body in it (opening angle θ ≈ 0.5). Cost is
about N log N. Keep the brute-force version for testing: Barnes-Hut forces should
match brute force within a small tolerance.

**Done when:** `stardust_bench` shows the crossover point where Barnes-Hut beats
brute force, and a test compares the two.

## 8. Multithreading or SIMD for the force loop ★★★

**Goal:** speed up brute force on many bodies.

**Approach:** split the outer loop across threads (each thread writes only its own
accelerations, so use the "both directions" form instead of the pair trick), or
store positions as separate x/y/z arrays and use SIMD. Measure before and after
with `stardust_bench`.

## 9. Git LFS for the assets ★ (decision) / ★★ (doing it)

**Goal:** stop every clone downloading ~100 MB of history.

**Approach:** `git lfs track "*.glb" "*.hdr"` makes *future* changes use LFS. Making
the *past* small needs `git lfs migrate import`, which rewrites every commit and
needs a force push: every link to an old commit changes. Decide whether a smaller
clone is worth that. Read about it before doing it.

## 10. Record asset sources and licences ★

**Goal:** find where the `.glb` models and `HDR_multi_nebulae_1.hdr` came from and
add an `assets/CREDITS.md` with source, author and licence for each. A portfolio
reviewer may ask. If a licence does not allow redistribution, replace the asset.

## 11. Shadows / eclipses ★★★

**Goal:** planets block the Sun's light. A point light needs a cube shadow map
(render depth from the Sun in six directions). A good graphics-programming
exercise after #5.

## 12. Continuous collision detection ★★

**Goal:** catch fast small bodies that would jump past each other inside one step.
Solve for the time of closest approach within the step (a quadratic in t for two
linearly moving spheres).
