# Resume claims: what the project supports, and what you must know first

A resume line is a promise that you can talk about it for ten minutes under
questioning. Each claim below is backed by the code (with the evidence named), and
comes with a **gate**: what you must be able to explain or rebuild *before* you put it
on your resume. Until you pass the gate, leave the claim off. Being caught unable to
explain your own resume line costs more than a shorter resume.

## A word on honesty about how this was built

The original Stardust was yours. The overhaul (build system, physics fixes, tests,
tools, docs) was done by an AI assistant overnight, and you are studying it to own
it. That is a legitimate way to learn, and many interviewers will be fine with it,
**if you say so** when asked how you built it ("I built the first version; I then
used an AI assistant to help restructure and test it, and I studied every change
until I could rebuild it myself; ask me anything"). Do not present AI-written code as
solely your own. The gates below are what make the work genuinely yours.

## Claims the project supports

### 1. "Built a real-time 3D N-body gravity simulation in C++20 with raylib"

- **Evidence:** the app; `src/sim/`; README.
- **Gate:** explain every line of `src/sim/Gravity.cpp` and `src/sim/Simulation.cpp`;
  do exercise E10 (whole N-body program from an empty file) without notes.

### 2. "Implemented velocity Verlet integration with a fixed-timestep loop, making results independent of frame rate"

- **Evidence:** `src/sim/Simulation.cpp`, `src/sim/FixedTimestep.h`,
  `Simulation.ResultDoesNotDependOnTheFrameRate`.
- **Gate:** chapter 4 fully; exercises E3 and E4; explain symplectic, order, the
  accumulator, the clamp and the spiral of death on a whiteboard.

### 3. "Diagnosed and fixed a frame-time bug that destabilised orbits; added a regression test"

- **Evidence:** `docs/AUDIT.md` S1-1, `docs/MEASUREMENTS.md` section 2,
  `Simulation.ALongStallDoesNotLoseTheMoon`.
- **Gate:** tell the story (Q5 in chapter 11) with the numbers: 0.1 s frame, Moon lost;
  explain why the old "10 sub-steps" were really one Euler step.
- **Wording note:** the bug was in your original code and the diagnosis was done
  during the AI-assisted overhaul; say "diagnosed and fixed" only once you can
  reproduce the diagnosis yourself (run `stardust_bench` section 2 and explain it).

### 4. "Validated physics with conservation tests: energy within 1e-6 and momentum within 1e-9 over 5 simulated minutes"

- **Evidence:** `Simulation.TheSolarSystemStaysStableForFiveMinutes`; measured values
  in `docs/MEASUREMENTS.md` section 3 (energy error 1.6e-8 measured; 1e-6 is the test's
  threshold).
- **Gate:** chapter 3 sections 3.8-3.10; exercise E7; explain why merges break energy
  conservation.

### 5. "Separated the simulation into a renderer-independent library tested with GoogleTest (52 tests) and built on Windows/MSVC and Linux/GCC in CI"

- **Evidence:** `CMakeLists.txt`, `tests/`, `.github/workflows/ci.yml`,
  `docs/decisions/0002`.
- **Gate:** chapter 6 and 7; exercise E12; explain what the Linux job proves.
- **Keep the number current:** if you add tests, update it; never round up.

### 6. "Wrote a streaming Radiance HDR decoder; made it 11x faster (3.4 s to 0.3 s) with a lookup table and buffered parsing, with byte-identical output"

- **Evidence:** `src/io/HdrLoader.cpp`, `tests/HdrLoaderTests.cpp`,
  `docs/MEASUREMENTS.md` section 5 (measured on the author's laptop, Release, clang).
- **Gate:** chapter 5.8; exercise E8; explain RGBE, RLE, Reinhard, and *why* each
  optimisation helped.
- **Wording note:** the original loader was yours; the 11x rewrite was AI-assisted.
  "Optimised" is fair once you have done E8.

### 7. "Used double-precision simulation state after measuring float rounding errors about 24x larger than the integrator's own error"

- **Evidence:** `docs/decisions/0004`, `docs/MEASUREMENTS.md` "Float versus double"
  (1.42e-2 vs 5.82e-4).
- **Gate:** chapter 4.7; explain the "adding a small number to a big one" problem.

### 8. "Implemented mouse picking (ray-sphere), a free-fly and tracking camera, HUD with live physics editing, collision merging with debris"

- **Evidence:** `src/app/Input.cpp`, `src/app/main.cpp`, `src/app/HUD.cpp`,
  `src/sim/Collision.cpp`, `src/app/Debris.cpp`. These features were in your original
  version.
- **Gate:** exercise E9 (your own ray-sphere intersection); explain inelastic merges.

### 9. "Set up CMake with pinned dependencies and CI with warnings-as-errors"

- **Evidence:** `CMakeLists.txt`, `CMakePresets.json`, `.github/`.
- **Gate:** chapter 7.1-7.6; be able to add a new source file and a new test to the build
  unaided.

## Claims to avoid (not supported)

- **"Wrote OpenGL / a renderer from scratch."** raylib does the OpenGL work; the
  lighting shaders are adapted from raylib's example.
- **"Physically accurate Solar System."** The laws are real; the masses and distances
  are tuned, not real.
- **"Scales to thousands of bodies."** Measured: 1000 bodies take about 4 ms per step,
  40 ms per 60 FPS frame. It is O(N²); Barnes-Hut is on the roadmap, not built.
- **"Implemented Barnes-Hut / multithreading / SIMD / shadows / PBR."** Not built.
- **"Bloom post-processing."** The glow is four additive circles drawn in 2D, an
  imitation, not a blur-based post-process.
- **"HDR rendering."** The sky is *loaded* from an HDR file and tone-mapped to 8 bits at
  load time; rendering itself is not HDR.
- **"Cross-platform app."** The app is Windows-only as built; only the simulation
  library and tests are shown to build on Linux.
- **Any number not in `docs/MEASUREMENTS.md`** or not re-measured by you.

## Suggested resume bullets (once the gates are passed)

> **Stardust** — real-time N-body gravity simulation (C++20, raylib, CMake, GoogleTest)
> - Built a renderer-independent physics library using velocity Verlet at a fixed
>   600 Hz timestep; energy drift under 1e-6 over 5 simulated minutes, verified by tests.
> - Diagnosed a frame-time coupling bug that destabilised orbits after a single slow
>   frame; fixed it with an accumulator-based fixed timestep and added a regression test.
> - Rewrote the streaming Radiance HDR loader with a tone-mapping lookup table and
>   buffered parsing: 3.4 s → 0.3 s load, byte-identical output.
> - CMake build with pinned dependencies; 52 unit tests; CI on Windows (MSVC) and
>   Linux (GCC) with warnings as errors.
