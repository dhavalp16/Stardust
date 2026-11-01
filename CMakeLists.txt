# Stardust - top-level build script.
#
# CMake is a "build system generator": it reads this file and writes the real
# build files for whatever tool you use (a Visual Studio solution, Ninja, ...).
# Visual Studio can open this folder directly (File > Open > Folder).
cmake_minimum_required(VERSION 3.24)

project(Stardust LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Dependencies, downloaded at configure time and pinned by hash so every
# machine builds against exactly the same source.
# ---------------------------------------------------------------------------
include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  raylib
  URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
  URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(raylib)

# ---------------------------------------------------------------------------
# The application
# ---------------------------------------------------------------------------
add_executable(Stardust
  src/main.cpp
  src/AssetPaths.cpp
  src/Physics.cpp
  src/Collision.cpp
  src/HUD.cpp
  src/Input.cpp
)
target_include_directories(Stardust PRIVATE src third_party)
target_link_libraries(Stardust PRIVATE raylib)
# Lets the app find assets/ and resources/ when run straight from the build folder.
target_compile_definitions(Stardust PRIVATE STARDUST_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
