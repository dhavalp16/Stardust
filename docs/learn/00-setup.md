# 0. Setting up and running everything

Goal: build the project, run the app, run the tests and the benchmark. You do not
need to understand any of it yet; the later chapters explain each piece.

## What you need to install

**Important:** when this overhaul was done, this laptop did **not** have Visual
Studio, a C++ compiler or CMake installed, even though the plan assumed it did.
(The work was done with temporary portable tools, see "How the overnight work was
built" below.) So install these first:

1. **Visual Studio Community 2022** (or newer), free from visualstudio.microsoft.com.
   In the installer tick the workload **"Desktop development with C++"**. That
   brings:
   - the **compiler** (MSVC): the program that turns C++ text into machine code;
   - **CMake**: the tool that describes how to build the project (chapter 7);
   - **Ninja**: a fast build runner that CMake uses.
2. **Git for Windows** (already installed on this laptop). Git keeps the history
   of the project (chapter 8).

## Build and run in Visual Studio

1. *File > Open > Folder...* and pick the repository folder (the one containing
   `CMakeLists.txt`).
2. Visual Studio reads `CMakePresets.json` and starts "CMake generation". The first
   time, it downloads raylib 5.5 and GoogleTest 1.17.0 (about a minute). Wait for
   "CMake generation finished" in the Output window.
3. In the toolbar, choose the **Release** configuration.
4. In the "Select Startup Item" dropdown, choose **Stardust.exe**. Press **F5**.

**Debug** builds are slower but let you pause the program and look inside it. Use
Release to judge speed.

## Build and run from a terminal

Open **Developer PowerShell for VS** (from the Start menu; an ordinary terminal
cannot find the compiler), go to the repository folder, and run:

```
cmake --preset release
cmake --build --preset release
build\release\Stardust.exe
```

- `cmake --preset release` **configures**: reads `CMakeLists.txt` and writes build
  instructions into `build/release/`.
- `cmake --build --preset release` **builds**: compiles every `.cpp` file and links
  them into programs.
- The programs appear in `build/release/`:
  - `Stardust.exe`: the app;
  - `stardust_tests.exe`: the unit tests;
  - `stardust_bench.exe`: the benchmark.

## Run the tests

```
ctest --preset release
```

You should see `100% tests passed, 0 tests failed out of 52`. A **test** is a
small program that checks one piece of the code gives the right answer (chapter 7).

## Run the benchmark

```
build\release\stardust_bench.exe
```

It prints four tables: integrator accuracy, what a stalled frame does, how long a
physics step takes for 10 to 3000 bodies, and how long the sky takes to load.
Compare with `docs/MEASUREMENTS.md`: the first two tables should match exactly,
the timings will be close.

## Run the screenshot script

```
mkdir C:\temp\shots
build\release\Stardust.exe --screenshots C:\temp\shots
```

The app runs a 6-second demo by itself and saves `1_paused.png`, `2_playing.png`
and `3_tracking_earth.png`. Run it before and after you change rendering code
and compare the pictures.

## How the overnight work was built

For the record (and so you can do the same on a machine without Visual Studio):
the overhaul was built with portable copies of CMake 4.4.5, Ninja 1.13.2 and
llvm-mingw 20261020 (the clang 23.1.4 compiler), unzipped into
`C:\Users\Optimus\stardust-build-tools\`. Nothing was installed system-wide. You can
delete that folder once Visual Studio is installed. GitHub Actions builds every
push with MSVC (chapter 7), which is how MSVC compatibility was checked.

## Exercises

1. Build Debug and Release. Run the app in each. Which has higher FPS when you
   fly close to the Sun? (Write down the numbers.)
2. Run the tests. Then open `tests/Vec3Tests.cpp`, change `5` on the line
   `EXPECT_DOUBLE_EQ(sum.x, 5);` to `6`, rebuild, run the tests again and read the
   failure message. Change it back.
3. Run the benchmark twice. Which numbers changed between runs and which did not?
   Why? (Answer: the accuracy and stall tables are pure maths and always identical;
   the timings depend on what else the laptop is doing.)
