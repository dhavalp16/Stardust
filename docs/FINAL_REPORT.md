# Final report: the Stardust overhaul (night of 1-2 November 2026)

Start with this page. It summarises what was found, what changed, what was measured,
what you must check by eye, what comes next, and the order to study everything in.

## 1. The situation found

- **The machine:** no Visual Studio, no C++ compiler, no CMake and no `gh` were
  installed, although the plan assumed them. Nothing was installed system-wide;
  portable CMake 4.4.5, Ninja 1.13.2 and clang 23.1.4 (llvm-mingw) were used from
  `C:\Users\Optimus\stardust-build-tools\` (safe to delete once you install Visual
  Studio). MSVC builds were checked by GitHub Actions instead. The first copy of these
  tools, in the Temp folder, was deleted mid-session by something on the machine and
  had to be downloaded again.
- **The code** (`release/sd-winApp`, `4acb9f0`): worked, but its physics depended on
  the frame rate, physics/rendering/UI were tangled, there were no tests, and it only
  built in one Visual Studio setup. Full ranked list: `docs/AUDIT.md` (25 items).
- **The most important finding**, reproduced headlessly against the original source
  files: one slow frame (0.1 s) is enough to knock the Moon into Earth, because the raw
  frame time was used as the physics step, and the "10 sub-steps" were really one
  Euler step.
- **Git:** `main` is the Android line while the portfolio is the desktop line; four
  branches hold two real lines; the Android line was merged into the Windows release;
  no tags; ~140 MB of assets in history. Nothing was rewritten.

## 2. What changed

Four new branches, stacked, each with a pull request into `release/sd-winApp` still to
be opened (see `docs/GIT_GUIDE.md`). 32 commits in total; `docs/learn/09-what-changed-and-why.md`
explains every one.

| Branch | In one line |
|---|---|
| `chore/cmake-build` | CMake + presets with pinned raylib, standard folder layout, assets found from any folder, strict warnings, CI on MSVC |
| `feature/simulation-core` | Raylib-free physics library; fixed 1/600 s timestep with clamp; velocity Verlet; double precision; softening; momentum/centre-of-mass conserving merges; computed initial speeds; GoogleTest; benchmark; CI on Linux too |
| `refactor/app-loop` | Scripted screenshots; update/draw split; Sun shader and light; window fits the screen; shared HUD rectangles (click-through fix); frame-rate-independent camera; optional music; HDR loader 11x faster with identical output; CI warnings as errors + MSVC annotations |
| `docs/learning-guide` | README, decision records, roadmap, check-by-eye list, learning path (chapters 0-11, glossary, exercises, interview questions), git guide, resume claims, this report |

Existing branches were not merged into, renamed or deleted. Every existing branch head
is also tagged `archive/2026-11-02/<branch>` on GitHub.

## 3. What was measured

All with the method stated in `docs/MEASUREMENTS.md`; Release builds, clang, this
laptop.

| Measurement | Result |
|---|---|
| One 0.1 s stalled frame, 60 s at 144 FPS | original loop: Moon merges into Earth at t = 5.9 s; new loop: no merge, same trajectory as with no stall |
| Distance from an exact test orbit after 10 orbits, 600 steps/s, double | semi-implicit Euler 1.23e-3; velocity Verlet 5.80e-4 |
| Same test, Verlet, float vs double state | float 1.42e-2; double 5.82e-4 |
| Full system for 300 s at 600 steps/s | no merges; worst relative energy error 1.57e-8; momentum below 1e-11 |
| Cost of one physics step | 10 bodies 0.36 µs; 100 bodies 37 µs; 1000 bodies about 4 ms (O(N²)) |
| Decoding the 93 MB HDR sky | original 3344-3398 ms; new 298-344 ms; output byte-identical |
| Unit tests | 0 before; 52 now |

## 4. Verification

- **Every commit** was checked out in a separate worktree, configured, built (Debug,
  clang) and its tests run. Results are in the table below.
- **CI** (GitHub Actions) on the final commit of each code branch: Windows/MSVC
  (warnings as errors, build, tests) and Linux/GCC (library + tests) all **passed**.
  Exception, stated plainly: on `refactor/app-loop`, the four commits `5706810` to
  `5744021` fail CI's MSVC *warnings-as-errors* build because of one C4244 warning
  (`double` to `float` at `src/app/HUD.cpp:24`) that clang's default warnings did not
  show. The code compiles; the warning was promoted to an error by the CI setting
  introduced in `5706810`. Fixed in `6c3cdda`.
- **Rendering** changes were compared with screenshot runs before and after.
- **Not verified:** mouse, keyboard and slider interaction by a person, and any build
  with MSVC on this laptop. That is what section 5 is for.

Per-commit results (Debug, clang 23.1.4, each commit checked out in one separate
worktree and built incrementally, then its tests run with CTest):

| Commit | Build | Tests |
|---|---|---|
| 64fb184 docs: add ranked audit of release/sd-winApp and a progress log | not applicable: documentation only (the build system arrives in the next commit) | none |
| b74fd9a build: add a CMake build with raylib 5.5 pinned by hash | OK | no tests yet |
| 6c8adab build: move to a standard folder layout and find assets from any folder | OK | no tests yet |
| 78211fa build: strict warnings for our code, formatting config | OK | no tests yet |
| c1bdbb2 ci: build with MSVC on GitHub Actions for every push and PR | OK | no tests yet |
| 4150fcb refactor: extract a raylib-free simulation library and add unit tests | OK | 100% tests passed out of 24 |
| 975619b fix: advance physics in fixed 1/600 s steps and clamp long frames | OK | 100% tests passed out of 34 |
| 7395327 fix: integrate with velocity Verlet (kick-drift-kick) | OK | 100% tests passed out of 36 |
| d3c0978 fix: store positions and velocities in double precision | OK | 100% tests passed out of 38 |
| 20b552d fix: replace the gravity cutoff with Plummer softening | OK | 100% tests passed out of 41 |
| 98278e5 fix: conserve momentum and centre of mass in every merge | OK | 100% tests passed out of 43 |
| 105eb03 refactor: compute starting orbit speeds from v = sqrt(G*M/r) | OK | 100% tests passed out of 46 |
| dfa1201 bench: add stardust_bench and record measured results | OK | 100% tests passed out of 46 |
| 6f2516e ci: run the tests on Windows (MSVC) and on Linux (GCC, no app) | OK | 100% tests passed out of 46 |
| 00e08a3 test: add a scripted screenshot run (Stardust.exe --screenshots <folder>) | OK | 100% tests passed out of 46 |
| 7de18fe refactor: update planet spin and debris before drawing, not while drawing | OK | 100% tests passed out of 46 |
| 83c7781 fix: give the Sun its unlit shader once and make the light follow it | OK | 100% tests passed out of 46 |
| 1504c50 fix: fit the window to the screen and share HUD rectangles with input | OK | 100% tests passed out of 46 |
| b616f1c fix: frame-rate independent camera easing, optional music, one speed rule | OK | 100% tests passed out of 46 |
| 96e36e0 perf: rewrite the HDR sky loader as a tested module, 11x faster | OK | 100% tests passed out of 52 |
| 5706810 ci: fail the build on any compiler warning in our code | OK | 100% tests passed out of 52 |
| b86b199 refactor: give main() named sections and pull out three helper functions | OK | 100% tests passed out of 52 |
| 67488bb ci: show MSVC warnings and errors as annotations | OK | 100% tests passed out of 52 |
| 5744021 ci: fix the MSVC problem matcher's JSON escaping | OK | 100% tests passed out of 52 |
| 6c3cdda fix: explicit double-to-float conversion in the selection reticle | OK | 100% tests passed out of 52 |
| 2011063 docs: rewrite the README for the current code | OK | 100% tests passed out of 52 |
| df5e363 docs: add decision records, roadmap and a check-by-eye list; tidy the audit | OK | 100% tests passed out of 52 |
| 2030ce5 docs: learning path chapters 0-8 | OK | 100% tests passed out of 52 |
| 5e2517b docs: learning path chapters 9-11 and glossary | OK | 100% tests passed out of 52 |
| 92389f6 docs: add the git guide and the resume-claims guide | OK | 100% tests passed out of 52 |

The documentation commits after `92389f6` change no code. The final
code was also built from scratch with `-DSTARDUST_WARNINGS_AS_ERRORS=ON` locally, and
by CI with MSVC `/WX` and GCC `-Werror`: no warnings.


## 5. What you must check by eye

`docs/CHECK_BY_EYE.md`: 30 checks, about 15 minutes. The most important:

1. The window fits your screen with the help bar visible (step 1).
2. Drag the window by its title bar for a few seconds while playing: the Moon survives
   (step 9).
3. Clicking just below the TRACK PLANET button inside the panel does not change the
   selection (step 18).
4. Sliders, tracking and merges behave as before (steps 20-25).

## 6. What is next

1. Morning: `docs/GIT_GUIDE.md` steps 1-4 (check, open PRs, merge in order, update),
   then tag `v1.0.0`.
2. Make `release/sd-winApp` the default branch on GitHub when you are ready.
3. Find and record the asset sources and licences (`docs/ROADMAP.md` item 10).
4. Then the roadmap, in order; items 1-3 are small and teach a lot.

## 7. The order to study everything

1. `docs/learn/README.md`: how to use the material.
2. `docs/learn/00-setup.md`: install, build, run, test. **Do this first, hands on.**
3. `docs/CHECK_BY_EYE.md`: get to know the app.
4. `docs/learn/01-cpp.md` alongside a C++ course (weeks).
5. `docs/learn/02-vectors-and-maths.md`, then `03-physics.md`.
6. `docs/learn/04-numerical-integration.md` with `docs/AUDIT.md` S1 and
   `docs/MEASUREMENTS.md` open. The most important chapter for interviews.
7. `docs/learn/05-graphics.md` alongside learnopengl.com.
8. `docs/learn/06-architecture.md` and `docs/decisions/`.
9. `docs/learn/07-build-test-tools.md`.
10. `docs/learn/08-git.md`, then actually do `docs/GIT_GUIDE.md` if not done yet.
11. `docs/learn/09-what-changed-and-why.md` with `git show` for each commit.
12. `docs/learn/10-exercises.md`: E1 to E12, in order. E10 is the one to be able to do
    on a whiteboard.
13. `docs/learn/11-interview-questions.md`: practise out loud.
14. `docs/RESUME_CLAIMS.md`: add each claim only after passing its gate.
15. `docs/ROADMAP.md`: build the next features yourself.
