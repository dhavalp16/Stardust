# 0001: CMake as the only build system, and a standard folder layout

*Status: accepted, 2 November 2026*

## Context

The project was built by a Visual Studio project file (`Stardust.vcxproj`) that:

- needed NuGet to download raylib before it would build;
- named a specific toolset (`v145`), so other Visual Studio versions had to
  retarget it first;
- only worked on Windows, only in Visual Studio;
- had to list every source file by hand in two XML files.

The app also only worked if the current folder was `Stardust/`.

## Decision

- **CMake** (`CMakeLists.txt` + `CMakePresets.json`) is the single build
  description. raylib 5.5 is fetched from its release tag and verified by SHA-256.
- The `.vcxproj`, `.vcxproj.filters`, `packages.config` and `.slnx` are removed
  rather than kept alongside. Two build systems drift apart, and the old one
  could not be tested on the machine this work was done on.
- Visual Studio still works: *File > Open > Folder* on the repo root. Visual
  Studio reads `CMakePresets.json` and offers "Debug" and "Release".
- Folder layout:

  ```
  src/          our code
  third_party/  single-header libraries we did not write (raygui, rlights)
  assets/       models and the HDR sky
  resources/    shaders
  tests/        unit tests (added later)
  docs/         documentation
  ```

- The app finds the folder containing `assets/` itself (`src/AssetPaths.cpp`).

## Consequences

- Good: one place to add a file; any IDE or CI machine can build; raylib version is
  exact; this is the setup most C++ teams use.
- Good: separating our code from third-party code lets us turn on strict warnings
  for our code only.
- Cost: the first configure downloads raylib (about 20 seconds). You need CMake 3.25+
  (Visual Studio 2022 and later include it).
- Cost: muscle memory. Opening `Stardust.slnx` no longer works on new branches. The
  old project files are still in history and on the `archive/2026-11-02/*` tags.
