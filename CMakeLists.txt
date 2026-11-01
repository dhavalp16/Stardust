# Stardust - top-level build script.
#
# CMake is a "build system generator": it reads this file and writes the real
# build files for whatever tool you use (a Visual Studio solution, Ninja, ...).
# Visual Studio can open this folder directly (File > Open > Folder).
cmake_minimum_required(VERSION 3.25)

project(Stardust LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(STARDUST_BUILD_APP "Build the raylib application" ON)
option(STARDUST_BUILD_TESTS "Build the unit tests" ON)
option(STARDUST_BUILD_TOOLS "Build the benchmark tool" ON)

# Strict warnings for code we write. Applied per target (never to raylib or
# third_party/) through stardust_set_warnings().
function(stardust_set_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive-)
    # fopen/sscanf are fine here; MSVC's "secure" variants are not portable.
    target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

include(FetchContent)

# ---------------------------------------------------------------------------
# The simulation library: pure C++, no raylib, no window. Both the app and the
# tests link against it, which is what makes the physics testable.
# ---------------------------------------------------------------------------
add_library(stardust_sim STATIC
  src/sim/Collision.cpp
  src/sim/Diagnostics.cpp
  src/sim/Gravity.cpp
  src/sim/Integrator.cpp
  src/sim/Simulation.cpp
  src/sim/SolarSystem.cpp
)
target_include_directories(stardust_sim PUBLIC src)
stardust_set_warnings(stardust_sim)

# ---------------------------------------------------------------------------
# The application
# ---------------------------------------------------------------------------
if(STARDUST_BUILD_APP)
  # Downloaded at configure time and pinned by hash, so every machine builds
  # against exactly the same source.
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SYSTEM  # treat raylib headers as third-party: no warnings from them
  )
  FetchContent_MakeAvailable(raylib)

  add_executable(Stardust
    src/app/main.cpp
    src/app/AssetPaths.cpp
    src/app/Debris.cpp
    src/app/HUD.cpp
    src/app/Input.cpp
    src/app/ScreenshotScript.cpp
    src/app/ThirdPartyImpl.cpp
  )
  target_include_directories(Stardust PRIVATE src/app)
  # SYSTEM tells the compiler these are not our headers, so it does not warn about them.
  target_include_directories(Stardust SYSTEM PRIVATE third_party)
  target_link_libraries(Stardust PRIVATE stardust_sim raylib)
  stardust_set_warnings(Stardust)
  # ThirdPartyImpl.cpp compiles other people's code; silence its warnings.
  if(MSVC)
    set_source_files_properties(src/app/ThirdPartyImpl.cpp PROPERTIES COMPILE_OPTIONS "/W0")
  else()
    set_source_files_properties(src/app/ThirdPartyImpl.cpp PROPERTIES COMPILE_OPTIONS "-w")
  endif()
  # Lets the app find assets/ and resources/ when run straight from the build folder.
  target_compile_definitions(Stardust PRIVATE STARDUST_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
endif()

# ---------------------------------------------------------------------------
# Benchmark: reproduces the numbers in docs/MEASUREMENTS.md. Use a Release build.
# ---------------------------------------------------------------------------
if(STARDUST_BUILD_TOOLS)
  add_executable(stardust_bench tools/bench/Bench.cpp)
  target_link_libraries(stardust_bench PRIVATE stardust_sim)
  stardust_set_warnings(stardust_bench)
endif()

# ---------------------------------------------------------------------------
# Unit tests (GoogleTest). Run them with:  ctest --preset debug
# ---------------------------------------------------------------------------
if(STARDUST_BUILD_TESTS)
  enable_testing()
  # MSVC: make GoogleTest use the same C runtime as our code.
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
    URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SYSTEM
  )
  FetchContent_MakeAvailable(googletest)

  add_executable(stardust_tests
    tests/CollisionTests.cpp
    tests/FixedTimestepTests.cpp
    tests/GravityTests.cpp
    tests/IntegratorTests.cpp
    tests/SimulationTests.cpp
    tests/SolarSystemTests.cpp
    tests/Vec3Tests.cpp
  )
  target_link_libraries(stardust_tests PRIVATE stardust_sim GTest::gtest_main)
  stardust_set_warnings(stardust_tests)

  include(GoogleTest)
  gtest_discover_tests(stardust_tests)
endif()
