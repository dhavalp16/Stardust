# 9. Every change of the overhaul, and why

This chapter walks through every commit made on 2 November 2026, in order. For each:
what was wrong, what changed, and how it was checked. Read it with
`git show <hash>` open next to it. By the end you should be able to defend each
change in an interview, including what it cost.

The starting point was `release/sd-winApp` at `4acb9f0`, kept as tag
`archive/2026-11-02/release/sd-winApp`. The problems are ranked in `docs/AUDIT.md`.

## How the work was checked

- **Builds and tests:** every commit was built and its tests run (the result is in
  `docs/FINAL_REPORT.md`).
- **Physics:** unit tests, plus headless scratch programs that ran the *original*
  physics files side by side with the new code.
- **Rendering:** the screenshot script (from `00e08a3` onwards) before and after each
  visual change; before that, a temporary uncommitted patch did the same.
- **MSVC:** GitHub Actions, because the laptop had no MSVC.

---

## Branch 1: `chore/cmake-build` (build and tooling)

**`64fb184` docs: audit and progress log.** Wrote down every problem with evidence
before touching code, ranked by severity. The key evidence: a headless replay of the
original physics loop showed steady frame rates keep the Moon, but a single 0.1 s
frame loses it.

**`b74fd9a` build: CMake with raylib 5.5 pinned by hash.** The old build needed NuGet
and a specific Visual Studio version. CMake works with any IDE and CI; the hash makes
the dependency exact. *Checked:* the untouched code built with it.

**`6c8adab` build: standard folder layout; find assets from any folder.** `src/`,
`third_party/`, `assets/`, `resources/` at the top, moved with `git mv` so history
follows. `ChangeToAssetRoot` (`src/app/AssetPaths.cpp`) finds the asset folder, so
the app no longer needs a particular working directory. Removed the `.vcxproj`
(one build system, not two drifting ones; `docs/decisions/0001`), the Android-only
shaders, and `glb_paths.txt` (absolute paths from another PC). *Checked:* started the
app from an unrelated folder.

**`78211fa` build: strict warnings, formatting config.** `/W4` or `-Wall -Wextra
-Wpedantic` on our code only; third-party code compiled separately with warnings off
(`src/app/ThirdPartyImpl.cpp`). `.clang-format` and `.editorconfig`.

**`c1bdbb2` ci: MSVC build on GitHub Actions.** The only way to check MSVC from this
laptop. *Checked:* the first run succeeded.

## Branch 2: `feature/simulation-core` (physics)

**`4150fcb` refactor: raylib-free simulation library and unit tests.** The central
change. `Planet` split into `sim::Body` (physics) and `PlanetInfo` (presentation);
physics moved into `stardust_sim`; collisions return `MergeEvent`s instead of touching
the UI; selection by index instead of a dangling-prone pointer; gravity evaluated once
per pair. GoogleTest with 24 tests. Behaviour kept the same on purpose (same loop
shape). *Checked:* old and new code run side by side for 60 s at 144 FPS: both kept
all 10 bodies; positions differed by 0.0006 after 1 s, from floating-point operations
in a different order. *Why:* `docs/decisions/0002`.

**`975619b` fix: fixed 1/600 s steps and a clamp.** `sim::FixedTimestep` and
`sim::Simulation`. Physics no longer depends on the frame rate; a stall can't produce
a giant step. *Checked:* tests for bit-identical results at two frame rates and for the
stall bug. *Cost:* after a stall the simulation runs slower than real time briefly.

**`7395327` fix: velocity Verlet.** Second order and symplectic; semi-implicit Euler
kept as an option for comparison. Added `Diagnostics` (momentum, centre of mass,
energy). *Checked:* Verlet's energy error is under a tenth of Euler's at 50 steps/s.

**`d3c0978` fix: double precision.** After Verlet, a test orbit was still 1.4e-2 off,
and smaller steps didn't help: float rounding dominated. With double, 5.7e-4 and
falling with step size. *Why:* `docs/decisions/0004`.

**`20b552d` fix: Plummer softening.** Replaced the "switch gravity off below 0.01"
cutoff with `r² + ε²`. Smooth everywhere; negligible at planet distances.

**`98278e5` fix: conserve momentum and centre of mass in every merge.** Sun impacts
used to delete the planet's momentum; survivors stayed at their old position.
*Checked:* tests for both.

**`105eb03` refactor: compute starting speeds.** `v = sqrt(G·M/r)` instead of typed
decimals; a test confirms they match the old values to 0.0005. Added the five-minute
stability test of the whole system (no merges, energy within 1e-6, momentum within
1e-9).

**`dfa1201` bench: `stardust_bench` and `docs/MEASUREMENTS.md`.** One command
reproduces every number. Shows the stall experiment, integrator accuracy, and O(N²)
cost growth (10 bodies: 0.36 µs per step; 1000: about 4 ms).

**`6f2516e` ci: tests on Windows and Linux.** The Linux job builds without raylib,
which proves the physics library really is independent.

## Branch 3: `refactor/app-loop` (the application)

**`00e08a3` test: scripted screenshot run.** `--screenshots <folder>` so rendering can
be compared automatically. Two subtleties found while building it: raylib's 2D drawing
is batched (flush before reading the screen, or the HUD is missing), and raylib's own
screenshot multiplies by the Windows display scale (black bands at 125%), so it reads
the framebuffer size directly.

**`7de18fe` refactor: update before drawing.** Spin and debris used to change inside
the draw loop. Now: input → update → draw, and drawing only reads.

**`83c7781` fix: Sun shader once; light follows the Sun.** No more per-frame shader
swapping; the light is where the Sun is. *Checked:* screenshots identical in content.

**`1504c50` fix: window fits the screen; shared HUD rectangles.** On this 1920x1080
laptop the fixed 1920x1080 window could not fit with its title bar. Now resizable and
maximised on small monitors; the HUD reads the live size. `HudLayout.h` gives drawing
and click-blocking the same rectangles, fixing a 10-pixel strip where clicks went
through the panel. Removed the last warnings.

**`b616f1c` fix: camera easing, optional music, one speed rule.** `1 - e^(-k·dt)`
easing (frame-rate independent, can't overshoot); music only if the file exists;
duplicated speed code made into one function.

**`96e36e0` perf: HDR loader rewritten, 11× faster.** `src/io/HdrLoader`: a 64 KB
lookup table instead of 150 million `pow` calls, the whole file read at once instead
of byte by byte, signature/format checks, RAII file handling, six tests including all
65536 table entries. *Measured:* 3344-3398 ms → 298-344 ms on the real file,
byte-identical output.

**`5706810` ci: warnings are errors in CI.** So no new warning can slip in.

**`b86b199` refactor: named sections in `main()`.** `LoadSkyTexture`,
`UpdateTrackingCamera`, `DrawSunBloom` pulled out; the loop is visibly
input / update / draw.

**`67488bb`, `5744021` ci: MSVC problem matcher.** CI logs need admin access to read,
but annotations are public; the matcher turns MSVC warnings into annotations. The
first version had unescaped backslashes in its JSON; the second fixed it.

**`6c3cdda` fix: explicit double-to-float in the reticle.** MSVC (C4244) caught what
clang's default warnings did not: `float rot = GetTime() * 80.0f;` with `GetTime()`
returning double. A good example of why two compilers in CI is worth it.

## Branch 4: `docs/learning-guide`

README rewrite, decision records, roadmap, check-by-eye list, this learning path,
interview questions, git guide, resume claims and the final report.

---

## Things deliberately *not* changed

- **History was not rewritten** (no force pushes, nothing deleted), to keep your links
  and old clones working. The LFS question is on the roadmap.
- **The look of the lighting** (the gamma issue in `lighting.fs`): changing it would
  change the picture, so it is explained and left for you (roadmap 5).
- **The masses and distances of the scenario**: they are your design.
- **The 0.8 collision overlap factor**: part of the feel of the app.
- **raygui and rlights** in `third_party/`: other people's code, used as-is.

## Exercises

1. Pick three commits. For each, write two sentences an interviewer would accept:
   the problem, and how you know the fix works.
2. Check out `archive/2026-11-02/release/sd-winApp` on a scratch branch and find each
   problem from `docs/AUDIT.md` S1 and S2 in the original code yourself.
3. Which commit would you revert first if a player reported "the planets move
   differently from last week"? Why? (Several are candidates; argue for one.)
