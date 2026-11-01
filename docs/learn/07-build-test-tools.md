# 7. Build, tests and tools

Professional code is not just the program; it is everything that lets a team change
it safely. This chapter covers how Stardust is built, tested, checked and measured.

## 7.1 CMake

**CMake** is a **build system generator**: you describe your project once in
`CMakeLists.txt`, and CMake writes the actual build files for whichever tool runs
the compiler (Ninja, a Visual Studio solution, Makefiles). It is the most common
build description for C++ projects, which is why it replaced the Visual Studio
`.vcxproj` (`docs/decisions/0001-cmake-and-folder-layout.md`).

Reading `CMakeLists.txt` top to bottom:

- `cmake_minimum_required`, `project`: CMake version and project name.
- `set(CMAKE_CXX_STANDARD 20)`: compile as C++20.
- `option(STARDUST_BUILD_APP ...)`: on/off switches. CI turns the app off on Linux.
- `function(stardust_set_warnings target)`: our own CMake function that turns on
  strict warnings (7.3).
- `add_library(stardust_sim STATIC ...)`: a **static library** is compiled code
  packed into one file that other programs link in.
- `target_include_directories(stardust_sim PUBLIC src)`: anyone using the library
  can `#include "sim/Body.h"`. `PUBLIC` passes it on; `PRIVATE` keeps it to this
  target.
- `add_executable(Stardust ...)`: the app, from its `.cpp` files.
- `target_link_libraries(Stardust PRIVATE stardust_sim stardust_io raylib)`: what it
  is built on.
- `target_compile_definitions(... STARDUST_SOURCE_DIR=...)`: bakes the source path
  into the program so it can find `assets/` (`src/app/AssetPaths.cpp`).

Modern CMake thinks in **targets** (libraries and executables) and their
properties, not global settings. If you remember one thing, remember that.

## 7.2 Dependencies: FetchContent and hashes

raylib and GoogleTest are downloaded at configure time:

```cmake
FetchContent_Declare(raylib
  URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
  URL_HASH SHA256=aea98ecf...)
```
The **SHA-256 hash** is a fingerprint of the exact file. If the download is
different in any byte, CMake refuses it. That makes builds **reproducible** (same
inputs on every machine) and guards against tampering. `SYSTEM` marks their headers
as third-party so their warnings don't appear in ours.

## 7.3 Presets

`CMakePresets.json` names standard configurations (`debug`, `release`) so everyone
(and Visual Studio) builds the same way: `cmake --preset release`. Each preset builds
into its own folder, `build/<preset>/`, which git ignores.

## 7.4 Warnings

Compilers can warn about code that is legal but suspicious: an unused variable, a
`double` silently squeezed into a `float`, a missing `#include`. `/W4` (MSVC) and
`-Wall -Wextra -Wpedantic` (GCC/clang) turn on most of them for our code only
(`stardust_set_warnings`). CI adds `/WX` / `-Werror` (warnings become errors) via
`STARDUST_WARNINGS_AS_ERRORS`, so no new warning can slip in.

A real example from this overhaul: `float rot = GetTime() * 80.0f;` compiled cleanly
with clang's default warnings, but MSVC flagged it (C4244: `double` to `float`). CI
caught it; the fix is an explicit `static_cast<float>` (`src/app/HUD.cpp:24`). Two
compilers catch more than one.

## 7.5 Unit tests with GoogleTest

A **unit test** checks one small piece of code in isolation. **GoogleTest** is a widely
used C++ testing library:

```cpp
TEST(Gravity, AccelerationFollowsTheInverseSquareLaw) {
  std::vector<Body> bodies = {MakeBody({0, 0, 0}, 100.0f), MakeBody({10, 0, 0}, 1.0f)};
  std::vector<Vec3> acc;
  sim::ComputeAccelerations(bodies, sim::GravityParams{}, acc);
  EXPECT_NEAR(acc[1].x, -1.0f, 1e-5f);
}
```
(`tests/GravityTests.cpp`). Arrange (set up), act (call), assert (check).
`EXPECT_NEAR(actual, expected, tolerance)` allows for floating-point rounding;
`ASSERT_...` stops the test on failure, `EXPECT_...` continues.

What the 52 tests cover (one file per area in `tests/`):

| File | Checks |
|---|---|
| `Vec3Tests` | Vector arithmetic |
| `GravityTests` | Inverse-square law, equal and opposite forces, G, dead bodies, softening |
| `IntegratorTests` | Kick/Drift; Verlet vs Euler energy; second-order convergence; accuracy at the app's step |
| `CollisionTests` | When merges happen, who survives, mass/volume/momentum/centre-of-mass conservation |
| `FixedTimestepTests` | Step counting, leftovers, clamping, reset |
| `SimulationTests` | Frame-rate independence, the stalled-frame regression, 5-minute stability of the real system |
| `SolarSystemTests` | Starting positions, speeds, zero total momentum |
| `HdrLoaderTests` | All 65536 tone-table entries, run-length decoding, bad files rejected |

Good tests to study first: `Simulation.ALongStallDoesNotLoseTheMoon` (a **regression
test**: it reproduces a real bug so it can never come back) and
`HdrLoader.EveryMantissaAndExponentDecodesThroughTheTable` (an **exhaustive test**:
the input space is small enough to check every case).

**CTest** runs the tests: `ctest --preset release`. `gtest_discover_tests` in
`CMakeLists.txt` registers each `TEST` with CTest automatically.

## 7.6 Continuous integration (CI)

**CI** means every push is built and tested automatically on clean machines.
`.github/workflows/ci.yml` defines two **jobs** on GitHub Actions:

- **Windows / MSVC**: configure with warnings as errors, build Release, run the tests.
  A **problem matcher** (`.github/problem-matchers/msvc.json`) turns compiler warning
  lines into annotations shown next to the code on pull requests.
- **Linux / GCC**: builds only the libraries, tests and benchmark (no raylib). Proves
  the physics is portable and independent of the renderer.

You can see results on GitHub under the **Actions** tab, and as a tick or cross next
to each commit and pull request.

## 7.7 The benchmark and honest measurement

`tools/bench/Bench.cpp` (`stardust_bench`) produces every number in
`docs/MEASUREMENTS.md`. Rules it follows, which you should follow whenever you claim a
number:

- **Measure Release builds.** Debug is several times slower and means nothing.
- **Repeat and take the median** (or best) of several runs; one run is noisy.
- **Say how you measured**: machine, compiler, build type, command.
- **Separate deterministic results** (accuracy, stability: identical every run) from
  **timings** (vary by machine and load).
- **Never quote a number you did not run.**

`std::chrono::steady_clock` is the right clock for timing (it never jumps backwards).

## 7.8 The screenshot script

`Stardust.exe --screenshots <folder>` (`src/app/ScreenshotScript.cpp`) plays a short
scripted demo and saves three pictures. It is a cheap **visual regression test**: run
it before and after a rendering change and compare. It was used for every visual
change in the overhaul.

## 7.9 Formatting

`.clang-format` describes the code style (2-space indent, 100 columns). Running
`clang-format -i file.cpp` formats a file; Visual Studio uses the same file. A shared
formatter ends style arguments in code review. `.editorconfig` sets basic editor
behaviour (spaces, final newline).

## Exercises

1. Add a new test to `tests/GravityTests.cpp` checking that three bodies in a line
   (masses 1, 1, 1 at x = -1, 0, 1) give the middle one zero acceleration. Run it.
2. Break `CircularOrbitSpeed` on purpose (`sqrt(G * parentMass * r)`). Which tests
   fail? Is that enough? Restore it.
3. Add a fifth section to `stardust_bench` that times `ResolveCollisions` for 1000
   bodies. Report the median of 7 runs in the same style as the others.
4. Configure with `-DSTARDUST_WARNINGS_AS_ERRORS=ON`, add an unused variable
   somewhere in `src/sim`, build, and read the error. Remove it.
5. Open the repository's Actions tab on GitHub and find the run for the latest
   commit on `release/sd-winApp` after you merge. Which jobs ran, and how long did
   each take?
