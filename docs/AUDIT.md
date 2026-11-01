# Stardust audit (2 November 2026)

This is the list of problems found in `release/sd-winApp` at commit `4acb9f0`,
ranked by how much they matter. Each entry says what is wrong, how it was found,
and where it was fixed (or why it was not). "Fixed in" names the branch; the
progress log (`docs/PROGRESS_LOG.md`) has the commit-by-commit detail.

Severity scale:

- **S1 Critical**: wrong results or broken behaviour a user can hit in normal use.
- **S2 Major**: wrong physics or design that will bite as soon as the project grows,
  or that an interviewer would flag immediately.
- **S3 Minor**: correctness or quality issues with small visible impact.
- **S4 Hygiene**: tidiness, tooling, documentation.

## How the evidence was gathered

- The untouched code was built with CMake 4.4.5, Ninja 1.13.2 and clang 23.1.4
  (llvm-mingw 20261020) against raylib 5.5 on this laptop (Windows 11, NVIDIA GPU,
  1920x1080 display at 125% scaling, 144 Hz).
- The app was run with a temporary, uncommitted patch that took screenshots at
  fixed frame numbers and then exited. That run showed the Moon merging into Earth
  a few seconds after pressing play ("Planets: 9", Earth mass 10.01).
- A scratch harness then replayed the original physics loop (`Physics.cpp` and
  `Collision.cpp`, unmodified, same loop shape as `main.cpp`) without a window,
  for 60 simulated seconds at fixed frame rates, and once with a single long frame
  injected at t = 2 s. Results (minimum Earth-Moon distance; a merge happens below
  0.56):

  | Run | Moon lost? | Closest approach |
  |---|---|---|
  | 30 FPS, steady | no | 0.629 |
  | 60 FPS, steady | no | 0.646 |
  | 144 FPS, steady | no | 0.669 |
  | 240 FPS, steady | no | 0.670 |
  | 144 FPS + one 0.05 s frame | no | 0.624 |
  | 144 FPS + one 0.10 s frame | **yes, at t = 5.9 s** | 0.555 |
  | 144 FPS + one 0.50 s frame | **yes, at t = 21.2 s** | 0.818 |
  | Reference: double precision velocity Verlet, h = 1e-5, no collisions | no | 0.682 |

  The trajectory depends on the frame rate, and one stalled frame (the kind
  a screenshot, window drag or breakpoint causes) is enough to lose the Moon. The
  in-app merge seen in the screenshot run matches this: saving a PNG stalls a frame.

## S1 Critical

1. **The simulation step is the raw frame time and is never clamped.**
   `main.cpp:167` reads `GetFrameTime()` and feeds it straight into physics.
   Results differ between a 60 Hz and a 144 Hz monitor, and any hitch delivers one
   huge step (measured above: one 0.1 s frame loses the Moon).
   *Fix:* fixed-timestep loop with an accumulator and a clamp on frame time.
   Fixed in `feature/simulation-core`.

2. **The "10 sub-steps" do nothing useful.** `main.cpp:219-233` kicks velocities
   ten times while positions stay frozen, then moves positions once with the full
   frame time. Because positions don't change, all ten kicks compute the same force,
   so this is exactly one semi-implicit Euler step that costs ten times as much.
   The README admits this. *Fix:* positions move inside each sub-step, and the
   integrator is velocity Verlet (second order, symplectic). Fixed in
   `feature/simulation-core`.

3. **The window does not fit the screen it was built on.** The window is a fixed
   1920x1080 (`main.cpp:25-29`). raylib reports this laptop's display as 1920x1080,
   so with the title bar and taskbar the bottom of the window (the help bar) is
   off-screen. *Fix:* resizable window sized to fit the monitor, HUD laid out from
   the live window size. Fixed in `refactor/app-loop`.

## S2 Major

4. **Physics, rendering, UI and input are tangled together.** `Planet` mixes
   physics state with a model path, colour, name and slider ranges; `main.cpp`
   advances planet spin and fragment motion inside the draw loop
   (`main.cpp:255-285`); collision code clears UI selection
   (`Collision.cpp:36-39`) and calls raylib's random generator. None of the
   physics can be tested without opening a window. *Fix:* a `stardust_sim`
   library with no raylib dependency and its own small `Vec3` type; the app layer
   converts at the boundary. Fixed in `feature/simulation-core`.

5. **No tests at all.** *Fix:* GoogleTest suite run by CTest, covering gravity,
   the integrator (energy and orbit checks), collisions (mass, momentum, volume),
   the fixed-timestep clock, initial conditions and the HDR decoder. Added in
   `feature/simulation-core` and `refactor/app-loop`.

6. **Selection is a raw pointer into a `std::vector`.** `main.cpp:158`,
   `Input.cpp:141`. It works today only because the vector never reallocates and
   reset reuses the same storage; any `push_back` or `erase` would leave it
   dangling. *Fix:* select by index. Fixed in `refactor/app-loop`.

7. **Momentum is not conserved when something hits the Sun.**
   `Collision.cpp:41-47` skips the momentum update when the Sun survives, which
   breaks the "momentum is conserved" claim in the README and lets the barycentre
   drift. The Sun is 2000 units of mass, so conserving momentum barely moves it
   anyway. Merges also left the survivor at its old position instead of the
   centre of mass. *Fix:* every merge conserves momentum and centre of mass; the Sun
   still always survives. Fixed in `feature/simulation-core`.

8. **Hard singularity cutoff.** `Physics.cpp:19` switches gravity off completely
   when two bodies are closer than 0.01 units, which is a discontinuity.
   *Fix:* Plummer softening (`r² + ε²`), the standard N-body technique.
   Fixed in `feature/simulation-core`.

9. **Only the Visual Studio project builds it, and only from one working
   directory.** `Stardust.vcxproj` needs NuGet restore and a matching toolset
   (`v145`); assets load relative to the current directory, with one hard-coded
   fallback (`main.cpp:46-48`). Running the `.exe` by double-click shows a magenta
   sky and missing shaders. *Fix:* CMake build with pinned dependencies, and the
   app finds its asset folder itself. Fixed in `chore/cmake-build`.

10. **Gravity is computed twice per pair.** `main.cpp:223-224` calls
    `ApplyGravity` for i→j and j→i, recomputing the square root. Newton's third
    law gives both from one evaluation. *Fix:* one evaluation per pair.
    Fixed in `feature/simulation-core`.

## S3 Minor

11. **Click-through gap under the planet panel.** The panel is drawn to y = 230
    (`HUD.cpp:41`) but clicks are blocked only above y = 220 (`Input.cpp:122`);
    the two numbers live in different files. *Fix:* one set of layout rectangles
    shared by HUD and input. Fixed in `refactor/app-loop`.
12. **Light stays at the origin while the Sun moves** (`main.cpp:35`).
    *Fix:* light follows the Sun every frame. Fixed in `refactor/app-loop`.
13. **The Sun's shader is swapped twice every frame** (`main.cpp:259-271`) when it
    could simply never be given the lit shader. Fixed in `refactor/app-loop`.
14. **Camera smoothing depends on frame rate** (`main.cpp:208-209`): `lerp(a, b,
    dt * 3.5)` overshoots when `dt > 0.29` and feels different at 60 vs 144 Hz.
    *Fix:* exponential smoothing `1 - exp(-k·dt)`. Fixed in `refactor/app-loop`.
15. **Duplicated camera-speed code** (`Input.cpp:25-39` and `68-82`).
    Fixed in `refactor/app-loop`.
16. **Missing music file is loaded anyway** (`main.cpp:152`): warning on every
    clean clone. *Fix:* only load if the file exists. Fixed in `refactor/app-loop`.
17. **HDR loader is a `static` function in a header** (`hdr_loader.h:12`), calls
    `powf` three times per pixel on a 50-million-pixel image, and does not check
    the file signature. *Fix:* moved to a `.cpp`, signature check, a lookup
    table for the tone curve (same output, measured speed-up), unit tests.
    Fixed in `refactor/app-loop`.
18. **Initial velocities are typed-in decimals** (`15.811`, `11.952`, ...) instead
    of being computed from `sqrt(G·M/r)`. *Fix:* computed. Fixed in
    `feature/simulation-core`.
19. **Unused `SUMMARY` state** (`HUD.h:6`). Removed; noted on the roadmap.
20. **`HUD.cpp` uses `std::remove_if` without including `<algorithm>`**; it only
    compiles because another header happens to include it. Fixed in
    `refactor/app-loop`.

## S4 Hygiene

21. **Git history and branches are confusing.** `main` holds the Android line
    while the README calls `release/sd-winApp` the main product; `desktop` and
    `release/sd-winApp` differ only in the README; PR #4 merged the Android `main`
    into the Windows release branch; there is a "dummy commit" PR; there are no
    tags. Nothing was rewritten (that would break your links). See
    `docs/GIT_GUIDE.md` for the recommended structure and the steps for you.
22. **93 MB HDR and ~55 MB of models live in normal git history**, so every clone
    is about 100 MB. Moving them to Git LFS needs a history rewrite and force-push,
    which is your decision; the steps are in the roadmap.
23. **Dead files on the desktop branch:** `resources/shaders/glsl100/*` (Android
    only) and `assets/glb_paths.txt` (absolute paths from another PC,
    `d:\Coding\...`). Removed in `chore/cmake-build`; still in history and on the
    Android branches.
24. **No formatting config, warnings at `/W3` only, no CI.** Added `.clang-format`,
    `.editorconfig`, higher warning levels for project code, and a GitHub Actions
    workflow. Done in `chore/cmake-build`.
25. **Asset sources and licences are not recorded.** The `.glb` models and the HDR
    have no credit or licence note. I could not find out where they came from, so
    this is left for you: see the roadmap.

## Things checked and found fine

- The circular-orbit maths: every planet's speed matches `sqrt(G·M/r)` with
  `G = 1, M = 2000` to the 3 decimals typed in, and the Moon's `3.333` matches
  `sqrt(10/0.9)`.
- The Sun's starting velocity correctly cancels the planets' total momentum.
- The Moon orbits in the same direction as Earth (prograde), at about 38% of
  Earth's Hill radius, as the README says.
- `lighting.vs` multiplies the normal by `matNormal` with `w = 1.0`. That looks
  wrong (normals usually use `w = 0`), but raylib's `matNormal` is the
  inverse-transpose of the model matrix, whose fourth column is zero, so the
  result is the same. The docs explain why.
