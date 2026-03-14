#define _CRT_SECURE_NO_WARNINGS // Tells Microsoft to ignore standard C warnings
// Include the Raylib library header file to access all its functions, such as
// window creation and 3D drawing.
#include "raylib.h"
// Include Raylib's math library for 3D vector math functions.
#include "raymath.h"
// Include the standard C++ math library for math operations like square root
// (std::sqrt).
#include <cmath>
// Include the C++ Standard Library algorithms for std::remove_if, used to
// clean up dead explosions from the pool without leaving gaps.
#include <algorithm>

// Define the implementation macro for the single-file rlights header
#define RLIGHTS_IMPLEMENTATION
#include "rlights.h"

// =============================================================================
// NEW INCLUDE: <vector> — The C++ Standard Library Dynamic Array
// =============================================================================
//
// WHAT IS A std::vector?
// ----------------------
// A std::vector is a dynamic array provided by the C++ Standard Library. Unlike
// a plain C array (e.g., 'Planet planets[8]') which has a FIXED size decided at
// compile time, a vector can grow and shrink at runtime. Think of it like a
// magical bookshelf that automatically adds more shelves when you put more
// books on it.
//
// WHY USE A VECTOR INSTEAD OF HARDCODED VARIABLES?
// -------------------------------------------------
// Before this refactor, we had individual variables: 'Planet earth', 'Planet
// moon'. This approach has severe scaling problems:
//   - Adding Mars means adding a new variable, new gravity calls, new draw
//     calls, new collision checks — EVERYWHERE.
//   - With 8 planets, you'd need 8 variables, 56 gravity pairs (8*7), 8 draw
//     calls, 8 collision checks — all hardcoded and error-prone.
//
// With a vector, adding a new planet is ONE line: planets.push_back({...}).
// Every loop in the engine (gravity, drawing, picking) automatically handles
// the new planet because they iterate over the ENTIRE vector.
//
// KEY VECTOR OPERATIONS WE USE:
//   - planets.reserve(N)    → Pre-allocate memory for N items (CRITICAL!)
//   - planets.push_back(p)  → Add planet 'p' to the end of the vector
//   - planets[i]            → Access the planet at index 'i' (0-based)
//   - planets.size()        → Returns how many planets are currently stored
//   - vectorA = vectorB     → Deep-copies ALL elements from B into A
//
// MEMORY LAYOUT:
//   A vector stores its elements in a CONTIGUOUS block of heap memory, just
//   like a C array. The elements sit side-by-side in RAM:
//     [ Planet_0 | Planet_1 | Planet_2 | ... | Planet_N ]
//   This means '&planets[i]' gives you a stable pointer to planet i — AS LONG
//   AS the vector doesn't reallocate (see reserve() explanation below).
// =============================================================================
#include <vector>

#include "Planet.h"
// Include our custom physics header so we can use ApplyGravity and
// UpdatePosition functions.
#include "Physics.h"

// Define RAYGUI_IMPLEMENTATION to generate the implementation part of the GUI
// library.
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// =============================================================================
// FEATURE 1: STATE MACHINE — The EngineState Enum
// =============================================================================
//
// WHAT IS AN ENUM?
// ----------------
// An 'enum' (short for "enumeration") is a special C++ data type that lets you
// define a set of named integer constants. Instead of using raw numbers like
// 0, 1, 2 to represent different states (which is confusing and error-prone),
// an enum lets you give those numbers human-readable names.
//
// Think of it like a label maker: instead of remembering that "0 means paused"
// and "1 means playing", you just write PAUSED and PLAYING. Under the hood,
// the compiler assigns each name an integer value automatically (PAUSED = 0,
// PLAYING = 1, SUMMARY = 2), but you never need to care about the numbers.
//
// WHY USE AN ENUM?
// ----------------
// 1. Readability: 'if (state == PLAYING)' is infinitely clearer than
//    'if (state == 1)'.
// 2. Safety: The compiler can warn you if you use a value that isn't in your
//    enum, catching bugs early.
// 3. Maintainability: If you add a new state later (e.g., GAME_OVER), you just
//    add it to the list — no need to remember which number it maps to.
//
// SYNTAX BREAKDOWN:
//   enum EngineState { PAUSED, PLAYING, SUMMARY };
//   ^     ^             ^       ^        ^
//   |     |             |       |        +-- Third constant (value 2)
//   |     |             |       +----------- Second constant (value 1)
//   |     |             +------------------- First constant (value 0)
//   |     +--------------------------------- The name of this enum type
//   +--------------------------------------- The 'enum' keyword
//
// After this definition, 'EngineState' becomes a new type you can use just
// like 'int' or 'float'. You can declare variables of type EngineState and
// assign them one of the three values: PAUSED, PLAYING, or SUMMARY.
// =============================================================================
enum EngineState { PAUSED, PLAYING, SUMMARY };

// =============================================================================
// EXPLOSION POOL — Transient Visual Effects for Collisions
// =============================================================================
//
// WHAT IS AN EXPLOSION STRUCT?
// ----------------------------
// When two planets collide, we “tombstone” them (set isAlive = false) and spawn
// a short-lived visual effect at their midpoint. This struct holds everything
// needed to animate one expanding, fading wireframe sphere.
//
// WHY A GLOBAL VECTOR?
// --------------------
// Unlike activePlanets (which is a parallel array paired with planetModels),
// explosions are standalone transient effects with NO parallel dependencies.
// This means it IS safe to use .erase() on them — removing a dead explosion
// doesn’t desynchronize any other array.
//
// WHY NOT ERASE PLANETS?
// ----------------------
// activePlanets[i] and planetModels[i] MUST stay aligned. If we erased
// activePlanets[2], then planetModels[3] would suddenly render at position [2],
// showing Jupiter’s model on Saturn’s physics body. This is a classic parallel
// array desynchronization bug that causes visual glitches and Segfaults on
// Reset (when we deep-copy initialPlanets back). Tombstoning avoids this
// entirely: dead planets stay in the array, just invisible.
// =============================================================================
struct Explosion {
  Vector3 position; // Center of the explosion in world space
  float radius;     // Current expanding radius
  float maxRadius;  // Maximum radius before the explosion dies
  float alpha;      // Opacity (1.0 = fully visible, 0.0 = invisible/dead)
  bool isAlive;     // Whether this explosion is still animating
};
std::vector<Explosion> activeExplosions;

// The main function where our C++ program begins execution.
int main() {
  // Initialize the window with a width of 1280 pixels.
  const int screenWidth = 1920;
  // Initialize the window with a height of 720 pixels.
  const int screenHeight = 1080;

  // Call Raylib's InitWindow function to open a window with the specified
  // dimensions and a title "Stardust".
  InitWindow(screenWidth, screenHeight, "Stardust");

  // ===========================================================================
  // FEATURE 10: 3D LIGHTING (rlights.h)
  // ===========================================================================
  // Load the standard Raylib lighting shaders from the resources folder.
#if defined(PLATFORM_ANDROID)
  Shader lightShader = LoadShader("resources/shaders/glsl100/lighting.vs",
                                  "resources/shaders/glsl100/lighting.fs");
#else
  Shader lightShader = LoadShader("resources/shaders/glsl330/lighting.vs",
                                  "resources/shaders/glsl330/lighting.fs");
#endif

  // Get the memory location of the "viewPos" variable inside the shader
  // so we can update it every frame for accurate specular highlights.
  int viewPosLoc = GetShaderLocation(lightShader, "viewPos");

  // Create a point light at the Sun's position (the origin). Unlike a
  // directional light (which illuminates everything from one fixed angle like
  // a distant star), a POINT light radiates outward in all directions from a
  // specific location. This means each planet is correctly lit from the Sun's
  // side — the face toward the Sun is bright, the far side is dark.
  Light sun = CreateLight(LIGHT_POINT,
                          Vector3{0.0f, 0.0f, 0.0f}, // Position: Sun at origin
                          Vector3Zero(),       // Target (unused for point)
                          WHITE, lightShader); // Color and shader

  // Declare a Camera3D struct instance named 'camera' to handle our 3D
  // viewpoint.
  Camera3D camera = {};
  // Set the camera's position in 3D space. Pulled far back to see the entire
  // 280-unit-wide solar system (Neptune orbits at r=140).
  camera.position = Vector3{0.0f, 160.0f, 200.0f};
  // Set the camera's target to look at. We point it at the center of the world,
  // meaning x=0.0f, y=0.0f, z=0.0f. This is another Vector3 in memory.
  camera.target = Vector3{0.0f, 0.0f, 0.0f};
  // Set the camera's "up" vector to tell it which way is up in our 3D world. In
  // our case, the positive Y-axis is up (x=0, y=1, z=0). It's also a Vector3.
  camera.up = Vector3{0.0f, 1.0f, 0.0f};
  // Set the field of view (fovy) for the camera in degrees. 45.0f provides a
  // natural perspective, similar to human vision.
  camera.fovy = 45.0f;
  // Set the projection type for the camera. CAMERA_PERSPECTIVE makes objects
  // appear smaller as they get further away, creating depth.
  camera.projection = CAMERA_PERSPECTIVE;

  // ===========================================================================
  // FEATURE 5: SOLAR SYSTEM VECTORS — Scalable Planet Storage
  // ===========================================================================
  //
  // THE TWO-VECTOR PATTERN (initialPlanets + activePlanets):
  // --------------------------------------------------------
  // We maintain TWO separate vectors of planets:
  //
  //   1. 'initialPlanets' — The "blueprint" or "save file". This vector holds
  //      the ORIGINAL configuration of every planet (position, velocity, mass,
  //      etc.) as they were at the start. This vector is NEVER modified during
  //      simulation. It's read-only after setup.
  //
  //   2. 'activePlanets' — The "live" working copy. This is the vector that
  //      the physics engine, renderer, and mouse picker all operate on. As the
  //      simulation runs, planets in this vector get their positions,
  //      velocities, and properties modified every frame.
  //
  // WHY TWO VECTORS?
  //   When the user clicks RESET, we need to restore everything to the initial
  //   state. If we only had one vector, we'd have no way to recover the
  //   original data — it would be overwritten by the simulation. By keeping
  //   the originals in 'initialPlanets', we can reset with one line:
  //     activePlanets = initialPlanets;  // Deep-copy all original data back!
  //
  // WHAT IS reserve()?
  // ------------------
  //   vector.reserve(N) pre-allocates enough heap memory to hold N elements
  //   WITHOUT actually creating any elements. The vector's size() stays 0,
  //   but its capacity() becomes N.
  //
  //   Think of it like renting an 8-car garage when you currently only own 2
  //   cars. The garage is mostly empty, but when you buy car #3, #4, etc.,
  //   they can park immediately without needing to move to a bigger garage.
  //
  // WHY IS reserve() CRITICAL FOR POINTER SAFETY?
  // -----------------------------------------------
  //   When a vector runs out of capacity and you push_back() another element,
  //   the vector must:
  //     1. Allocate a NEW, LARGER block of memory on the heap
  //     2. COPY all existing elements to the new block
  //     3. DEALLOCATE (free) the old block
  //
  //   This is called REALLOCATION. The problem? Any pointers (like our
  //   'selectedPlanet') that pointed to elements in the OLD block are now
  //   DANGLING POINTERS — they point to freed memory! Accessing them causes
  //   undefined behavior (crashes, data corruption, or seemingly random bugs).
  //
  //   By calling reserve(8), we guarantee the vector will NEVER reallocate as
  //   long as we don't exceed 8 planets. This means '&activePlanets[i]' will
  //   remain a valid, stable pointer for the lifetime of the vector.
  //
  //   ANALOGY:
  //     Without reserve(): You rent a 2-car garage. When you buy car #3, the
  //     landlord moves ALL your cars to a new 4-car garage and demolishes the
  //     old one. Anyone who had your old garage address now drives to a pile
  //     of rubble (dangling pointer!).
  //
  //     With reserve(8): You rent an 8-car garage upfront. Adding cars 3–8
  //     just parks them in an empty slot. No one ever has to change addresses.
  //
  // std::vector<Planet> SYNTAX:
  //   The angle brackets '<Planet>' tell the compiler what TYPE of objects
  //   this vector will hold. This is called a "template parameter". You can
  //   read 'std::vector<Planet>' as "a vector of Planet structs".
  // ===========================================================================
  std::vector<Planet> initialPlanets;
  std::vector<Planet> activePlanets;

  // ===========================================================================
  // FEATURE 9: DATA-ORIENTED DESIGN — Parallel Arrays for 3D Models
  // ===========================================================================
  //
  // WHY NOT PUT 'Model' INSIDE THE 'Planet' STRUCT?
  // -----------------------------------------------
  // It seems intuitive to add 'Model 3dModel;' to Planet.h. But doing so
  // creates a massive architectural trap when combined with our Reset Button
  // vector copy: 'activePlanets = initialPlanets;'.
  //
  // A Raylib 'Model' struct doesn't hold the actual 3D geometry in RAM. It
  // holds POINTERS and IDs (like unsigned int vaoId, vboId) pointing to
  // resources living inside the Graphics Card (GPU / VRAM).
  //
  // THE SHALLOW COPY DISASTER:
  // When C++ copies a struct (like during our reset), it does a "Shallow
  // Copy". It just copies the raw numbers. So the new Planet gets a copy of
  // the GPU IDs (e.g., vaoId = 42).
  // Now you have TWO structs claiming ownership of GPU resource #42.
  // When the vector is destroyed or reallocated, the destructor might call
  // UnloadModel() to free the memory. Resource #42 gets deleted.
  // But the OTHER struct still has vaoId = 42! When IT tries to render or free
  // the model, the program crashes instantly (a "Double-Free" or
  // "Use-After-Free" error).
  //
  // THE SOLUTION: PARALLEL ARRAYS
  // -----------------------------
  // Keep the 'Planet' struct pure math data (position, mass, velocity). This
  // is safe to copy infinitely.
  // Store the GPU models in a separate, parallel vector where index [i] in
  // the physics vector corresponds to index [i] in the models vector:
  //   activePlanets[0] (Earth Physics)  <--->  planetModels[0] (Earth Visuals)
  //
  // By separating the Math from the Art, we prevent memory corruption, improve
  // CPU cache friendliness (Data-Oriented Design), and make saving/loading
  // save states trivial.
  // ===========================================================================
  std::vector<Model> planetModels;

  // CRITICAL: Pre-allocate memory for 16 planets in ALL vectors. This prevents
  // reallocation when we push_back() planets, keeping our pointers safe.
  initialPlanets.reserve(16);
  activePlanets.reserve(16);
  planetModels.reserve(16);

  // Pre-allocate the explosion pool. 32 slots is generous — in practice we’ll
  // rarely have more than a handful active at once, but reserve() ensures no
  // reallocation surprises during gameplay.
  activeExplosions.reserve(32);

  // ===========================================================================
  // PLANET INITIALIZATION — Setting Up The 9-Body Solar System
  // ===========================================================================
  //
  // The C++ Constructor makes struct initialization cleaner by allowing us to
  // initialize a complete struct inline inside the array, avoiding repetitive
  // 'p.mass = X; p.radius = Y;' expressions entirely.
  // ===========================================================================

  // ─── STABLE WIDE-ORBIT SCATTER
  // ─────────────────────────────────────────────── G = 1.0f, M_sun = 2000.0f
  // All orbital speeds derived from v = sqrt(G * M_sun / r) for circular
  // orbits. Gas giant masses drastically reduced (Jupiter 50→3, Saturn 30→1.5)
  // to prevent inner-planet slingshots. Earth inflated to 5.0 so its Hill
  // sphere (1.88 units) safely contains the Moon at distance 0.8 (42% of Hill
  // radius).
  //
  // Scatter pattern: position  = { r*cos(θ), 0, r*sin(θ) }
  //                  velocity  = { -v*sin(θ), 0, v*cos(θ) }  (tangential, CCW)
  // ─────────────────────────────────────────────────────────────────────────────
  initialPlanets = {
      // SUN — stationary anchor at origin (mass = 2000)
      Planet({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, 2000.0f, 3.00f,
             "assets/sun.glb", YELLOW, 0.10f, "Sun", 500.0f, 5000.0f, 1.0f,
             6.0f),

      // MERCURY — r=8, v=sqrt(2000/8)=15.811, θ=1.0 rad (57°)
      Planet({8.0f * cosf(1.0f), 0.0f, 8.0f * sinf(1.0f)},
             {-15.811f * sinf(1.0f), 0.0f, 15.811f * cosf(1.0f)}, 0.055f, 0.25f,
             "assets/mercury.glb", GRAY, 0.02f, "Mercury", 0.01f, 1.0f, 0.1f,
             1.0f),

      // VENUS — r=14, v=sqrt(2000/14)=11.952, θ=3.0 rad (172°) | retrograde
      // spin
      Planet({14.0f * cosf(3.0f), 0.0f, 14.0f * sinf(3.0f)},
             {-11.952f * sinf(3.0f), 0.0f, 11.952f * cosf(3.0f)}, 0.815f, 0.50f,
             "assets/venus.glb", ORANGE, -0.01f, "Venus", 0.1f, 5.0f, 0.1f,
             2.0f),

      // EARTH — r=20, v=sqrt(2000/20)=10.000, θ=5.0 rad (286°)
      // Mass inflated to 10.0 so its Hill sphere (2.37 units) can comfortably
      // hold the Moon at distance 0.9 (38% of Hill radius — very stable).
      Planet({20.0f * cosf(5.0f), 0.0f, 20.0f * sinf(5.0f)},
             {-10.000f * sinf(5.0f), 0.0f, 10.000f * cosf(5.0f)}, 10.00f, 0.55f,
             "assets/earth.glb", BLUE, 0.50f, "Earth", 1.0f, 20.0f, 0.2f, 3.0f),

      // MARS — r=30, v=sqrt(2000/30)=8.165, θ=0.5 rad (29°)
      Planet({30.0f * cosf(0.5f), 0.0f, 30.0f * sinf(0.5f)},
             {-8.165f * sinf(0.5f), 0.0f, 8.165f * cosf(0.5f)}, 0.107f, 0.35f,
             "assets/mars.glb", RED, 0.48f, "Mars", 0.01f, 2.0f, 0.1f, 1.5f),

      // JUPITER — r=55, v=sqrt(2000/55)=6.030, θ=2.5 rad (143°)
      // Mass reduced from 50→3 to stop gravitational slingshots on inner
      // planets.
      Planet({55.0f * cosf(2.5f), 0.0f, 55.0f * sinf(2.5f)},
             {-6.030f * sinf(2.5f), 0.0f, 6.030f * cosf(2.5f)}, 3.00f, 1.80f,
             "assets/jupiter.glb", BEIGE, 1.50f, "Jupiter", 0.5f, 15.0f, 0.5f,
             4.0f),

      // SATURN — r=80, v=sqrt(2000/80)=5.000, θ=4.5 rad (258°)
      // Mass reduced from 30→1.5 to prevent perturbation cascades.
      Planet({80.0f * cosf(4.5f), 0.0f, 80.0f * sinf(4.5f)},
             {-5.000f * sinf(4.5f), 0.0f, 5.000f * cosf(4.5f)}, 1.50f, 1.50f,
             "assets/saturn.glb", GOLD, 1.30f, "Saturn", 0.2f, 10.0f, 0.5f,
             3.5f),

      // URANUS — r=110, v=sqrt(2000/110)=4.264, θ=1.5 rad (86°) | retrograde
      // spin
      Planet({110.0f * cosf(1.5f), 0.0f, 110.0f * sinf(1.5f)},
             {-4.264f * sinf(1.5f), 0.0f, 4.264f * cosf(1.5f)}, 0.50f, 1.00f,
             "assets/uranus.glb", SKYBLUE, -0.80f, "Uranus", 0.1f, 5.0f, 0.3f,
             2.5f),

      // NEPTUNE — r=140, v=sqrt(2000/140)=3.780, θ=3.5 rad (201°)
      Planet({140.0f * cosf(3.5f), 0.0f, 140.0f * sinf(3.5f)},
             {-3.780f * sinf(3.5f), 0.0f, 3.780f * cosf(3.5f)}, 0.60f, 0.95f,
             "assets/neptune.glb", DARKBLUE, 0.90f, "Neptune", 0.1f, 5.0f, 0.3f,
             2.5f),

      // MOON — orbits Earth at distance 0.9 (38% of Earth's Hill sphere = 2.37)
      // Position = Earth position + {0.9, 0, 0}
      // Velocity = Earth velocity + {0, 0, 3.333}  where 3.333 = sqrt(G *
      // M_earth / r)
      //          = sqrt(10.0 / 0.9) = sqrt(11.11) = 3.333
      // At 38% of the Hill sphere, this is deep in the stable zone for Euler
      // integration. Clearance above combined radii (0.70) is 0.20 units.
      Planet({20.0f * cosf(5.0f) + 0.9f, 0.0f, 20.0f * sinf(5.0f)},
             {-10.000f * sinf(5.0f), 0.0f, 10.000f * cosf(5.0f) + 3.333f},
             0.012f, 0.15f, "assets/moon.glb", LIGHTGRAY, 0.05f, "Moon", 0.001f,
             0.5f, 0.05f, 0.5f),
  };

  // ===========================================================================
  // FEATURE 6: CONSERVATION OF MOMENTUM — Fixing System Drift
  // ===========================================================================
  // We sum the momentum of all celestial bodies and then apply an exact
  // counter-velocity to the heaviest body (the Sun) to ensure the total system
  // momentum is zero. This prevents the entire solar system from drifting.
  // ===========================================================================
  Vector3 totalMomentum = {0.0f, 0.0f, 0.0f};
  for (size_t i = 1; i < initialPlanets.size(); i++) {
    totalMomentum.x += initialPlanets[i].mass * initialPlanets[i].velocity.x;
    totalMomentum.y += initialPlanets[i].mass * initialPlanets[i].velocity.y;
    totalMomentum.z += initialPlanets[i].mass * initialPlanets[i].velocity.z;
  }

  // Apply counter-velocity to the Sun (index 0)
  initialPlanets[0].velocity.x = -totalMomentum.x / initialPlanets[0].mass;
  initialPlanets[0].velocity.y = -totalMomentum.y / initialPlanets[0].mass;
  initialPlanets[0].velocity.z = -totalMomentum.z / initialPlanets[0].mass;

  // Locate the existing LoadModel() calls in main.cpp and replace them entirely
  // with a loop over initialPlanets, loading p.modelPath in order.
  for (size_t i = 0; i < initialPlanets.size(); i++) {
    planetModels.push_back(LoadModel(initialPlanets[i].modelPath.c_str()));
  }

  // Cache the default, unlit shader that Raylib automatically assigns to the
  // Sun model when it's loaded. We need this so the Sun can emit its own light
  // and not be incorrectly shaded by the point light at its center.
  Shader defaultSunShader = planetModels[0].materials[0].shader;

  // Apply the lighting shader to ALL materials on ALL loaded models so they
  // can receive the light from our point source.
  for (size_t i = 0; i < planetModels.size(); i++) {
    for (int m = 0; m < planetModels[i].materialCount; m++) {
      planetModels[i].materials[m].shader = lightShader;
    }
  }

  // Deep-copy the initial configuration into the active (working) vector.
  // The '=' operator on vectors copies ALL elements from the source to the
  // destination. Each Planet struct is copied member-by-member (name, position,
  // radius, mass, velocity, color). This gives us a fresh working copy that
  // the simulation can modify freely without touching the originals.
  activePlanets = initialPlanets;

  // Set the target frame rate to 180 frames per second so the window updates at
  // a consistent, smooth speed.
  SetTargetFPS(180);

  // ===========================================================================
  // FEATURE 1 (continued): Create a variable to track the current engine state.
  // ===========================================================================
  //
  // Here we declare a variable of our new 'EngineState' type and set it to
  // PAUSED. This means the simulation will NOT run physics when the program
  // first starts — the planets will be frozen in place until the user presses
  // SPACE to begin the simulation.
  //
  // Think of it like a DVD player: when you first put in a disc, it's paused.
  // You press PLAY (SPACE bar) to start watching (simulating).
  // ===========================================================================
  EngineState currentState = PAUSED;

  // ===========================================================================
  // FEATURE 2: MOUSE PICKING — The selectedPlanet Pointer
  // ===========================================================================
  //
  // WHAT IS A POINTER?
  // ------------------
  // A pointer is a variable that stores a MEMORY ADDRESS instead of a direct
  // value. Think of it like a bookmark in a library: the bookmark doesn't
  // contain the book's text, but it tells you exactly where to find the book
  // on the shelf. Similarly, a pointer doesn't contain a Planet's data — it
  // stores the address in your computer's RAM where that Planet lives.
  //
  // SYNTAX BREAKDOWN:
  //   Planet* selectedPlanet = nullptr;
  //   ^     ^  ^                ^
  //   |     |  |                +-- 'nullptr' means "pointing at nothing"
  //   |     |  +------------------- The name of this pointer variable
  //   |     +---------------------- The '*' (asterisk) makes this a POINTER
  //   +---------------------------- The type this pointer CAN point to
  //
  // IMPORTANT WITH VECTORS:
  // -----------------------
  // Our pointer now points INTO the activePlanets vector's internal memory:
  //   selectedPlanet = &activePlanets[i];
  // This is safe because we called reserve(8), guaranteeing the vector's
  // memory block won't move. If we hadn't reserved, a push_back() could
  // reallocate, and selectedPlanet would become a DANGLING POINTER — pointing
  // at freed memory (instant crash or data corruption).
  // ===========================================================================
  Planet *selectedPlanet = nullptr;

  // ===========================================================================
  // FEATURE 3: 3D VIEWPORT — Free Camera State Tracking
  // ===========================================================================
  //
  // WHAT IS A 'bool'?
  // -----------------
  // A 'bool' (short for "boolean") is a C++ data type that can hold exactly
  // TWO values: 'true' or 'false'. It's named after George Boole, the
  // mathematician who invented Boolean algebra. Under the hood, a bool is
  // just 1 byte of memory: 0 = false, anything else = true.
  //
  // WHY DO WE NEED THIS VARIABLE?
  // ------------------------------
  // Raylib's CAMERA_FREE mode is designed for first-person-shooter-style
  // camera controls: WASD to move, mouse to look around. BUT it comes with
  // a catch — it calls DisableCursor() internally, which HIDES and LOCKS the
  // mouse cursor to the center of the screen.
  //
  // This is great for flying around, but TERRIBLE for our UI! We need the
  // cursor visible to:
  //   1. Click on planets (mouse picking)
  //   2. Drag the GUI sliders (raygui)
  //
  // THE SOLUTION — A Toggle:
  //   We only activate free-camera mode while the user HOLDS DOWN the Right
  //   Mouse Button (RMB). This is the same pattern used in 3D editors like
  //   Unity, Unreal, and Blender:
  //     - Hold RMB → cursor disappears, WASD + mouse controls the camera
  //     - Release RMB → cursor reappears, you can click planets and sliders
  //
  // We use this bool to track whether the camera was active LAST frame, so
  // we know when the user TRANSITIONS from "holding" to "not holding" the
  // button. This prevents calling EnableCursor() every single frame (which
  // would be wasteful).
  // ===========================================================================
  bool isCameraActive = false;

  // ===========================================================================
  // CAMERA SPEED — Adjustable with Scroll Wheel
  // ===========================================================================
  // Raylib's built-in CAMERA_FREE has a hardcoded move speed that's far too
  // slow for our 280-unit-wide solar system. We define a custom speed variable
  // that the user can adjust with the mouse scroll wheel. This gives a much
  // more responsive fly-through experience.
  // ===========================================================================
  float cameraSpeed = 2.0f;
  // =========================================================================
  // MOBILE INPUT STATE — Must persist across frames
  // =========================================================================
  // Left Joystick (Movement)
  bool joystickActive = false;
  Vector2 joystickCenter = {0.0f, 0.0f};
  Vector2 joystickThumb = {0.0f, 0.0f};
  int joystickTouchPointId = -1;

  // Right Screen (Camera Look)
  bool lookActive = false;
  Vector2 lastLookPos = {0.0f, 0.0f};
  int lookTouchPointId = -1;

  // Tracks WHICH planet was selected last frame so the toast only fires
  // when the user drags a slider, not when they first tap a planet.
  Planet *prevSelectedPlanet = nullptr;

  // =========================================================================
  // TOAST NOTIFICATION STATE — Real-World Value Display
  // =========================================================================
  // When the user drags the mass or radius slider, a notification appears at
  // the top-center of the screen showing the real-world equivalent value.
  //
  // prevSliderMass / prevSliderRadius: store the last known slider values so
  // we can detect when the user actually moved a slider this frame.
  //
  // toastTimer: counts DOWN from 3.0 seconds to 0. While > 0, the toast is
  // visible. The last 0.5 seconds fade out by scaling the alpha channel.
  //
  // toastText: the formatted string built whenever a slider changes, e.g.:
  //   "Earth | Mass: 1.00 Earth masses  (5.97e+24 kg)"
  // =========================================================================
  float toastTimer = 0.0f;
  char toastText[192] = {0};
  float prevSliderMass = -1.0f; // -1 = "no planet was selected last frame"
  float prevSliderRadius = -1.0f;

  // ===========================================================================
  // GRAVITATIONAL CONSTANT — Shared by all gravity calculations
  // ===========================================================================
  // We define G as a named constant so it's easy to find and tweak. This value
  // is NOT the real-world gravitational constant (6.674e-11) — it's scaled up
  // to produce visible orbital behavior at the scale of our simulation.
  // ===========================================================================
  const float G = 1.0f;

  // Enter a continuous while-loop that will run until the user presses the ESC
  // key or closes the window via the cross button.
  while (!WindowShouldClose()) {
    // Calculate the time it took to render the last frame, in seconds. We use
    // this 'delta time' to decouple our physics loop from the monitor's frame
    // rate, ensuring consistent speed across different computers.
    float dt = GetFrameTime();

    // =========================================================================
    // FEATURE 3 (continued): CAMERA TOGGLE — Right Mouse Button Control
    // =========================================================================
    //
    // IsMouseButtonDown(int button):
    //   A Raylib function that returns 'true' for EVERY frame the specified
    //   mouse button is held down. This is different from
    //   IsMouseButtonPressed() which only fires once on the initial click.
    //   - IsMouseButtonPressed() = fires ONCE when you click
    //   - IsMouseButtonDown()    = fires CONTINUOUSLY while you hold
    //
    // We use IsMouseButtonDown() because we want camera control to persist
    // for the entire duration the user holds RMB, not just one frame.
    //
    // MOUSE_BUTTON_RIGHT is a Raylib constant for the right mouse button.
    //
    // THE LOGIC:
    // ----------
    // When RMB IS held down:
    //   1. DisableCursor() → hides the mouse cursor and locks it to the
    //      center of the window. This is required for CAMERA_FREE to work
    //      because the camera reads mouse MOVEMENT (delta), not position.
    //      If the cursor wasn't locked, it would fly off the edge of the
    //      screen and stop providing movement data.
    //   2. UpdateCamera(&camera, CAMERA_FREE) → this is Raylib's built-in
    //      free camera controller. It handles ALL the math for:
    //        - W key → move camera forward
    //        - A key → strafe camera left
    //        - S key → move camera backward
    //        - D key → strafe camera right
    //        - Mouse movement → rotate/look around (pitch and yaw)
    //        - Mouse wheel → zoom in/out
    //      The '&camera' passes the MEMORY ADDRESS of our camera variable.
    //      This lets UpdateCamera modify our camera directly.
    //   3. Set isCameraActive = true → remember that camera mode is ON.
    //
    // When RMB is NOT held down:
    //   1. Check if isCameraActive was true (meaning we WERE in camera mode
    //      last frame). If so, call EnableCursor() to bring the mouse back.
    //   2. Set isCameraActive = false → update our tracking variable.
    //
    // WHY CHECK 'isCameraActive' BEFORE EnableCursor()?
    //   Without this check, EnableCursor() would be called EVERY FRAME when
    //   RMB isn't held. While that technically works, it's wasteful — we only
    //   need to re-enable the cursor ONCE, at the exact moment the user
    //   releases the button. The bool acts as a "transition detector".
    // =========================================================================
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
      // User is holding Right Mouse Button → activate free camera mode.

      // IMPORTANT: Only call DisableCursor() on the TRANSITION frame (when
      // camera was NOT active last frame). If we call it EVERY frame, Raylib
      // resets the internal mouse delta tracking each time, which kills the
      // mouse-look rotation. We only need to hide/lock the cursor ONCE.
      if (!isCameraActive) {
        DisableCursor();
        isCameraActive = true;
      }

      // =====================================================================
      // MANUAL CAMERA MOVEMENT — Configurable WASD Speed
      // =====================================================================
      // Raylib's CAMERA_FREE has a hardcoded speed that's too slow for our
      // 280-unit solar system. We use UpdateCamera for mouse-look only, then
      // manually apply WASD movement at our adjustable cameraSpeed.
      //
      // GetCameraForward/GetCameraRight return normalized direction vectors
      // from the camera's current orientation. Multiplying by cameraSpeed * dt
      // gives frame-rate-independent movement.
      //
      // Scroll wheel adjusts cameraSpeed for faster/slower navigation.
      // =====================================================================
      UpdateCamera(&camera, CAMERA_FREE);

      // Calculate normalized direction vectors from camera orientation
      Vector3 forward =
          Vector3Normalize(Vector3Subtract(camera.target, camera.position));
      Vector3 right = Vector3Normalize(Vector3CrossProduct(forward, camera.up));

      float moveAmount = cameraSpeed * dt;

      // Manual WASD movement at our custom speed (layered on top of
      // UpdateCamera)
      if (IsKeyDown(KEY_W)) {
        camera.position =
            Vector3Add(camera.position, Vector3Scale(forward, moveAmount));
        camera.target =
            Vector3Add(camera.target, Vector3Scale(forward, moveAmount));
      }
      if (IsKeyDown(KEY_S)) {
        camera.position =
            Vector3Subtract(camera.position, Vector3Scale(forward, moveAmount));
        camera.target =
            Vector3Subtract(camera.target, Vector3Scale(forward, moveAmount));
      }
      if (IsKeyDown(KEY_D)) {
        camera.position =
            Vector3Add(camera.position, Vector3Scale(right, moveAmount));
        camera.target =
            Vector3Add(camera.target, Vector3Scale(right, moveAmount));
      }
      if (IsKeyDown(KEY_A)) {
        camera.position =
            Vector3Subtract(camera.position, Vector3Scale(right, moveAmount));
        camera.target =
            Vector3Subtract(camera.target, Vector3Scale(right, moveAmount));
      }

      // Scroll wheel adjusts camera speed. Clamp between 0.5 and 50.0.
      float wheel = GetMouseWheelMove();
      if (wheel != 0.0f) {
        cameraSpeed += wheel * 1.0f;
        if (cameraSpeed < 0.5f)
          cameraSpeed = 0.5f;
        if (cameraSpeed > 50.0f)
          cameraSpeed = 50.0f;
      }
    } else {
      // User is NOT holding Right Mouse Button.

      // Only call EnableCursor() on the TRANSITION from active → inactive.
      // This means: "If camera WAS active last frame but RMB is now released,
      // bring the cursor back." We don't want to call this every single frame.
      if (isCameraActive) {
        // Re-show the mouse cursor so the user can click planets and sliders.
        EnableCursor();

        // Mark camera as no longer active.
        isCameraActive = false;
      }
    }

    // =========================================================================
    // FEATURE 1 (continued): INPUT — Toggle Between PAUSED and PLAYING
    // =========================================================================
    //
    // IsKeyPressed() is a Raylib function that returns 'true' (a boolean value)
    // for exactly ONE frame when the specified key transitions from "not
    // pressed" to "pressed". This prevents the state from flickering back and
    // forth if you hold the key down — it only triggers on the initial press.
    //
    // KEY_SPACE is a Raylib constant representing the spacebar key.
    //
    // THE IF / ELSE IF STRUCTURE:
    //   'if' checks a condition. If the condition is true, the code inside { }
    //   runs. 'else if' is checked ONLY if the previous 'if' was false. This
    //   creates a chain of mutually exclusive checks — only ONE block runs.
    //
    // LOGIC:
    //   - If the engine is currently PAUSED and SPACE is pressed → switch to
    //     PLAYING (start the simulation).
    //   - Else if the engine is currently PLAYING and SPACE is pressed → switch
    //     to PAUSED (freeze the simulation).
    //   - If the engine is in SUMMARY state, SPACE does nothing (you could
    //     extend this later).
    // =========================================================================
    if (IsKeyPressed(KEY_SPACE)) {
      if (currentState == PAUSED) {
        // Switch from PAUSED to PLAYING. The physics pipeline below will now
        // execute because we check 'if (currentState == PLAYING)' before
        // running it.
        currentState = PLAYING;
      } else if (currentState == PLAYING) {
        // Switch from PLAYING to PAUSED. The physics pipeline will be skipped
        // on the next frame because the state is no longer PLAYING.
        currentState = PAUSED;
      }
      // Note: We don't handle SUMMARY here yet. You can add it later when you
      // build out the SUMMARY screen feature.
    }

    // =========================================================================
    // FEATURE 2 (continued): MOUSE PICKING — Detect which planet was clicked
    // =========================================================================
    //
    // REFACTORED FOR VECTORS:
    // Instead of checking two hardcoded planets, we now loop through EVERY
    // planet in activePlanets. This means adding Mars, Jupiter, etc. requires
    // ZERO changes to the picking code — the loop handles them automatically.
    //
    // HOW IT WORKS:
    //   1. Fire a ray from the camera through the mouse click position
    //   2. Loop through ALL planets, testing the ray against each sphere
    //   3. Track the CLOSEST hit (smallest distance)
    //   4. Set selectedPlanet = &activePlanets[closestIndex]
    //
    // THE 'size_t' TYPE:
    //   'size_t' is an unsigned integer type used for sizes and indices. It's
    //   guaranteed to be large enough to represent the maximum size of any
    //   object in memory. Using 'size_t' for loop indices that compare with
    //   .size() avoids signed/unsigned comparison warnings.
    //
    // POINTER SAFETY WITH VECTORS:
    //   We set 'selectedPlanet = &activePlanets[i]'. This gives us a pointer
    //   directly into the vector's internal memory. This is safe because:
    //   1. We called reserve(8), so no reallocation will occur
    //   2. We never push_back() during the game loop
    //   3. On reset, we set selectedPlanet = nullptr BEFORE modifying the
    //      vector (see the reset button section)
    // =========================================================================
    if (!isCameraActive && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
        GetMouseY() > 120 && GetMouseY() < (screenHeight - 280)) {
      // GUARD: 'GetMouseY() > 120' prevents clicks on the GUI area (top 120
      // pixels) from triggering mouse picking. Without this, clicking a slider
      // or the reset button would fire a ray into 3D space, miss all planets,
      // and set selectedPlanet to nullptr — causing the sliders to vanish.

      // STEP 1: Get a 3D ray from the camera through the mouse's 2D position.
      Ray mouseRay = GetMouseRay(GetMousePosition(), camera);

      // Track the closest planet hit so far. We initialize closestDistance to
      // a huge number so ANY real hit will be closer.
      float closestDistance = 999999.0f;
      int closestIndex = -1; // -1 means "no planet hit yet"

      // STEP 2: Loop through every planet in the active simulation.
      for (size_t i = 0; i < activePlanets.size(); i++) {
        // Test if the ray intersects this planet's sphere.
        RayCollision collision = GetRayCollisionSphere(
            mouseRay, activePlanets[i].position, activePlanets[i].radius);

        // If the ray hit this planet AND it's closer than any previous hit,
        // record it as the new closest candidate.
        if (collision.hit && collision.distance < closestDistance) {
          closestDistance = collision.distance;
          closestIndex = (int)i;
        }
      }

      // STEP 3: Set the selected planet based on results.
      if (closestIndex >= 0) {
        // A planet was hit! Point selectedPlanet to its memory address inside
        // the activePlanets vector. The '&' gets the address of the element
        // at index 'closestIndex' within the vector's contiguous memory block.
        selectedPlanet = &activePlanets[closestIndex];
      } else {
        // No planet was hit. Clear the selection.
        selectedPlanet = nullptr;
      }
    }

    // =========================================================================
    // FEATURE 1 (continued): CONDITIONAL PHYSICS — Only run when PLAYING
    // =========================================================================
    //
    // This 'if' statement is the core of our state machine for the game loop.
    // The physics calculations (gravity and position updates) are wrapped
    // inside this check. They will ONLY execute when 'currentState' equals
    // 'PLAYING'. When the state is 'PAUSED' (or 'SUMMARY'), the entire physics
    // pipeline is skipped, and the planets remain frozen in their current
    // positions.
    //
    // This is the power of a state machine: by changing ONE variable
    // (currentState), we control the behavior of the entire simulation loop.
    // =========================================================================
    if (currentState == PLAYING) {
      // =====================================================================
      // FEATURE 7: N-BODY GRAVITY LOOP — Every Planet Pulls Every Other Planet
      // =====================================================================
      //
      // WHAT IS N-BODY GRAVITY?
      // -----------------------
      // In real space, EVERY object with mass pulls on EVERY other object.
      // The Sun pulls Earth, Earth pulls the Sun, the Moon pulls Earth, Earth
      // pulls the Moon, the Moon pulls the Sun, etc. — ALL simultaneously.
      //
      // Previously we had hardcoded pairs:
      //   ApplyGravity(earth, earth.mass, moon, G, dt);
      //   ApplyGravity(moon, moon.mass, earth, G, dt);
      // This only works for 2 bodies. With 8 planets, we'd need 56 lines!
      //
      // THE NESTED FOR-LOOP PATTERN:
      // ----------------------------
      // We use a classic "every pair exactly once" pattern:
      //   for i = 0 to N-1:
      //     for j = i+1 to N-1:
      //
      // Starting j at i+1 has two benefits:
      //   1. SKIPS SELF: A planet never pairs with itself (i != j always)
      //   2. AVOIDS DUPLICATES: The pair (Earth, Moon) is visited once, not
      //      twice. We apply gravity in BOTH directions inside the loop.
      //
      // For 2 planets (Earth=0, Moon=1), this gives us:
      //   i=0, j=1 → Earth↔Moon (one pair, both directions applied)
      //
      // For 4 planets (0,1,2,3), this gives us:
      //   i=0: j=1, j=2, j=3  →  3 pairs
      //   i=1: j=2, j=3       →  2 pairs
      //   i=2: j=3             →  1 pair
      //   Total: 6 pairs = N*(N-1)/2 = 4*3/2 = 6 ✓
      //
      // Each pair applies gravity in BOTH directions (Newton's Third Law),
      // so every planet feels the pull of every other planet.
      // =====================================================================
      // =====================================================================
      // PHYSICS SUB-STEPPING (Crucial for Orbital Stability)
      // =====================================================================
      // The Moon has a very tight, fast orbit (period ~1.6 seconds). At 60 FPS,
      // a single frame's dt (0.016s) represents 1% of the entire orbit. In
      // numerical integration (Euler), taking such "large" steps introduces
      // error, causing tight orbits to drift and crash.
      //
      // The classic engine solution is to slice the frame's dt into smaller
      // micro-steps. Running the physics 10 times per frame with 1/10th the dt
      // completely stabilizes the math without noticeable performance loss.
      // =====================================================================
      const int SUB_STEPS = 10;
      float subDt = dt / SUB_STEPS;

      for (int step = 0; step < SUB_STEPS; step++) {
        // ---------------------------------------------------------------------
        // N-BODY GRAVITY LOOP
        // ---------------------------------------------------------------------
        for (size_t i = 0; i < activePlanets.size(); i++) {
          for (size_t j = i + 1; j < activePlanets.size(); j++) {
            // =================================================================
            // TOMBSTONE GUARD — Skip Dead Planets
            // =================================================================
            // If either planet in this pair has already been destroyed (isAlive
            // == false), there is no point calculating gravity between them.
            // Without this guard, dead planets would continue to exert phantom
            // gravitational forces, and distance calculations on overlapping
            // corpses could produce NaN/Inf values that corrupt the simulation.
            // =================================================================
            if (!activePlanets[i].isAlive || !activePlanets[j].isAlive)
              continue;

            // Planet i pulls planet j toward it.
            ApplyGravity(activePlanets[i], activePlanets[i].mass,
                         activePlanets[j], G, subDt);
            // Planet j pulls planet i toward it (Newton's Third Law).
            ApplyGravity(activePlanets[j], activePlanets[j].mass,
                         activePlanets[i], G, subDt);

            // =================================================================
            // VOLUMETRIC COLLISION DETECTION
            // =================================================================
            // We check if the distance between two planets is less than the sum
            // of their radii.
            //
            // THE 0.8x FORGIVENESS FACTOR:
            // For incredibly tight orbits like the Earth-Moon system (distance
            // 0.9, combined radii 0.70), even sub-stepped numerical integration
            // produces tiny elliptical wobbles. To prevent these micro-wobbles
            // from triggering instant collisions, we require the planets to
            // penetrate by 20% (threshold * 0.8f) before exploding.
            //
            // We use SQUARED distance and SQUARED threshold to avoid sqrt().
            // =================================================================

            float distSqr = Vector3DistanceSqr(activePlanets[i].position,
                                               activePlanets[j].position);
            float combinedRadii =
                activePlanets[i].radius + activePlanets[j].radius;
            float collisionThreshold = combinedRadii * 0.8f;

            if (distSqr < (collisionThreshold * collisionThreshold)) {
              // ===============================================================
              // TOMBSTONE PROTOCOL — Flag Both Planets as Dead
              // ===============================================================
              // Instead of calling .erase() on activePlanets (which would
              // desynchronize the parallel planetModels array and cause
              // index-mismatched rendering + Segfaults on Reset), we simply
              // set isAlive = false. The rendering loop already checks this
              // flag and skips dead planets. The physics loop (above) also
              // skips them. The planets remain in memory at their original
              // indices, keeping planetModels[i] perfectly aligned.
              //
              // This pattern is called “Tombstoning” — the entity is logically
              // dead but physically still occupies its slot in the array,
              // like a tombstone marking where someone used to be.
              // ===============================================================
              activePlanets[i].isAlive = false;
              activePlanets[j].isAlive = false;

              // ===============================================================
              // POINTER SAFETY — Clear Selection if a Dying Planet is Selected
              // ===============================================================
              // If the user has one of the colliding planets selected, the
              // slider code would try to read/write mass and radius of a dead
              // planet. While not an immediate crash (the memory is still
              // valid), it’s semantically wrong — you can’t adjust the mass of
              // something that no longer exists. Clear the pointer.
              // ===============================================================
              if (selectedPlanet == &activePlanets[i] ||
                  selectedPlanet == &activePlanets[j]) {
                selectedPlanet = nullptr;
              }

              // ===============================================================
              // SPAWN EXPLOSION — Visual Feedback at the Collision Midpoint
              // ===============================================================
              // Calculate the midpoint between the two colliding planets.
              // This is where the explosion effect will appear.
              // maxRadius = combinedRadii * 4 gives a satisfying “boom” that
              // extends well beyond the original planets’ surfaces.
              // ===============================================================
              Vector3 midpoint = {
                  (activePlanets[i].position.x + activePlanets[j].position.x) /
                      2.0f,
                  (activePlanets[i].position.y + activePlanets[j].position.y) /
                      2.0f,
                  (activePlanets[i].position.z + activePlanets[j].position.z) /
                      2.0f};

              Explosion exp;
              exp.position = midpoint;
              exp.radius =
                  combinedRadii * 0.5f; // Start at half the collision zone
              exp.maxRadius =
                  combinedRadii * 3.0f; // Expand to 3x for dramatic effect
              exp.alpha = 1.0f;         // Fully opaque at spawn
              exp.isAlive = true;
              activeExplosions.push_back(exp);
            }
          }
        }
      }

      // --- POSITION INTEGRATION ---
      // After ALL gravitational forces have been applied (updating velocities),
      // we update every planet's position based on its new velocity.
      // This MUST happen in a SEPARATE loop AFTER all gravity calculations
      // are complete. If we updated positions inside the gravity loop, later
      // gravity calculations would use partially-updated positions, leading
      // to subtle inaccuracies.
      // Also skip dead planets from position integration — there’s no need
      // to move tombstoned corpses, and it prevents NaN velocities from
      // propagating into positions.
      for (size_t i = 0; i < activePlanets.size(); i++) {
        if (!activePlanets[i].isAlive)
          continue;
        UpdatePosition(activePlanets[i], dt);
      }
    }
    // If currentState is PAUSED or SUMMARY, we skip the physics entirely.
    // The planets stay frozen but are still drawn below — you can observe
    // their positions and click on them while paused!

    // Update the light shader with the current camera position. This is
    // required for the shader to calculate accurate specular highlights
    // (shiny spots) based on where the user is looking from.
    float camPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(lightShader, viewPosLoc, camPos, SHADER_UNIFORM_VEC3);

    // Begin the drawing phase for this frame. All rendering elements must be
    // placed between BeginDrawing() and EndDrawing().
    BeginDrawing();

    // Clear the screen background to a dark color to represent the emptiness of
    // space and reset the color buffer before drawing new frames.
    ClearBackground(BLACK);

    // Initialize the 3D drawing mode, passing our configured 3D camera to
    // Raylib so it calculates the correct perspective matrices.
    BeginMode3D(camera);

    // =========================================================================
    // RENDERING LOOP — Draw All Planets From the Parallel Arrays
    // =========================================================================
    //
    // Instead of hardcoded DrawSphere(), we now use DrawModelEx() to render
    // actual 3D meshes.
    //
    // Because we used Parallel Arrays, the physics data for Earth is at
    // activePlanets[0], and the 3D model for Earth is at planetModels[0].
    //
    // DrawModelEx parameters:
    //   - model: The 3D geometry and textures (from our separate vector)
    //   - position: Where to draw it (from our physics vector)
    //   - rotationAxis: The axis to rotate around (we use Y, {0,1,0})
    //   - rotationAngle: The angle in degrees (0.0f for now, no local spin)
    //   - scale: A Vector3 representing how much to stretch the model.
    //
    // THE DYNAMIC SCALE TRICK:
    // By passing {radius, radius, radius} as the scale, the 3D model will
    // smoothly grow and shrink in real-time when the user drags the GUI radius
    // slider! The physics bounding sphere perfectly overlays the visual mesh.
    // =========================================================================
    for (size_t i = 0; i < activePlanets.size(); i++) {
      if (!activePlanets[i].isAlive)
        continue; // Skip rendering destroyed planets

      const float visualScaleFactor = 1.0f;
      // THE HOLY GRAIL OF PHYSICS ENGINES (1:1 Scale Normalization):
      // ------------------------------------------------------------
      // Importing a mathematically pure 1-unit radius mesh from Blender is the
      // "Holy Grail" because it perfectly aligns art with physics.
      // visualScaleFactor = 1.0f means that multiplying the physics radius
      // by this factor gives the EXACT same value. The 3D geometry scales 1:1
      // with the invisible physics bounding sphere, guaranteeing that
      // collisions happen exactly where the surface appears to be!
      Vector3 modelScale = {activePlanets[i].radius * visualScaleFactor,
                            activePlanets[i].radius * visualScaleFactor,
                            activePlanets[i].radius * visualScaleFactor};

      // EXPLANATION: Why RAD2DEG is required for Raylib's rotation functions.
      // Raylib's mathematical functions (like sin, cos in shaders) often work
      // internally with radians, but the high-level DrawModelEx function
      // expects rotation angles in DEGREES. Since rotationSpeed is typically
      // defined in radians per second (a standard physics unit), we multiply by
      // RAD2DEG to convert it to degrees before feeding it into the drawing
      // function.
      if (currentState == PLAYING) {
        activePlanets[i].rotationAngle +=
            activePlanets[i].rotationSpeed * dt * RAD2DEG;
      }

      // WHITE prevents Raylib from multiplying the custom Blender Albedo
      // textures by a solid color, allowing the true authored textures to
      // render.
      //
      // EMISSIVE SUN TRICK:
      // The Sun sits at the point light's position, so the lighting shader
      // produces zero illumination on it (a light can't illuminate itself).
      // To make the Sun always fully bright (it's a star — it EMITS light),
      // we temporarily swap its materials to the default Raylib shader we
      // cached at setup. This shader has no lighting math, so the Sun renders
      // at full albedo brightness. All other planets keep the lighting shader.
      if (i == 0) {
        for (int m = 0; m < planetModels[i].materialCount; m++) {
          planetModels[i].materials[m].shader = defaultSunShader;
        }
      }

      DrawModelEx(
          planetModels[i],                // The 3D model from VRAM
          activePlanets[i].position,      // The current physics world position
          Vector3{0.0f, 1.0f, 0.0f},      // Rotate around Y-axis (up)
          activePlanets[i].rotationAngle, // Factor in dynamic rotation angle
          modelScale, // Dynamic scale matching the GUI slider
          WHITE       // Use true authored textures (no color tinting)
      );

      // Restore the lighting shader on the Sun after drawing so it doesn't
      // affect future frames incorrectly if the model setup changes.
      if (i == 0) {
        for (int m = 0; m < planetModels[i].materialCount; m++) {
          planetModels[i].materials[m].shader = lightShader;
        }
      }
    }

    // =========================================================================
    // FEATURE 2 (continued): VISUAL SELECTION INDICATOR
    // =========================================================================
    //
    // If selectedPlanet is NOT nullptr (meaning the user has clicked on a
    // planet), we draw a visual indicator around the selected planet so the
    // user can clearly see which one they picked.
    //
    // DrawSphereWires draws a WIREFRAME sphere (just the outlines, not filled).
    // We use 'selectedPlanet->radius * 1.1f' to make the wireframe 10% bigger
    // than the actual planet, creating a visible "halo" effect around it.
    // =========================================================================
    if (selectedPlanet != nullptr) {
      DrawSphereWires(
          selectedPlanet->position, // Center: the selected planet's position
          selectedPlanet->radius * 1.1f, // Radius: 10% bigger for visibility
          16,                            // Rings: 16 horizontal lines
          16,                            // Slices: 16 vertical lines
          YELLOW                         // Color: bright yellow stands out
      );
    }

    // =========================================================================
    // EXPLOSION RENDERING — Additive Blended Wireframe Bursts
    // =========================================================================
    //
    // WHAT IS ADDITIVE BLENDING (BLEND_ADDITIVE)?
    // -------------------------------------------
    // Normal rendering (alpha blending) mixes the new color with whatever is
    // behind it: result = src*alpha + dst*(1-alpha). This makes things
    // semi-transparent.
    //
    // ADDITIVE blending instead ADDS the new color on top of what’s already
    // there: result = src + dst. This creates a glowing, light-emitting
    // effect — perfect for explosions, fire, and energy effects. Multiple
    // overlapping additive draws become brighter and brighter, simulating
    // the intensity of a real explosion.
    //
    // We switch to additive mode, draw all active explosions, then switch
    // back to the default alpha blend so the rest of the UI renders normally.
    // =========================================================================
    BeginBlendMode(BLEND_ADDITIVE);

    for (size_t e = 0; e < activeExplosions.size(); e++) {
      if (!activeExplosions[e].isAlive)
        continue;

      // Expand the explosion radius over time. The expansion speed of 3.0f
      // gives a dramatic burst that completes in about 2 seconds.
      activeExplosions[e].radius += 3.0f * dt;

      // Fade out the explosion’s opacity as it expands. Speed of 0.5f gives
      // a slow, cinematic fade — visible long enough to appreciate.
      activeExplosions[e].alpha -= 0.5f * dt;

      // Kill the explosion if it’s fully faded or has reached max size.
      if (activeExplosions[e].alpha <= 0.0f ||
          activeExplosions[e].radius >= activeExplosions[e].maxRadius) {
        activeExplosions[e].isAlive = false;
        continue;
      }

      // Convert the float alpha (0.0–1.0) to an unsigned byte (0–255) for
      // Raylib’s Color struct. We cast to unsigned char to match the type.
      unsigned char byteAlpha =
          (unsigned char)(activeExplosions[e].alpha * 255.0f);

      // Draw two concentric wireframe spheres with fiery colors.
      // The outer sphere is orange, the inner is yellow — this creates a
      // layered “fireball” look with depth.
      Color outerColor = {255, 161, 0, byteAlpha}; // Orange with dynamic alpha
      Color innerColor = {255, 255, 0, byteAlpha}; // Yellow with dynamic alpha

      DrawSphereWires(activeExplosions[e].position, activeExplosions[e].radius,
                      8, 8, outerColor);
      DrawSphereWires(activeExplosions[e].position,
                      activeExplosions[e].radius * 0.6f, 8, 8, innerColor);
    }

    EndBlendMode();

    // =========================================================================
    // EXPLOSION CLEANUP — Remove Dead Explosions via Erase-Remove Idiom
    // =========================================================================
    //
    // WHY IS IT SAFE TO ERASE EXPLOSIONS BUT NOT PLANETS?
    // ---------------------------------------------------
    // Explosions are standalone — they have NO parallel array partner. Removing
    // activeExplosions[3] doesn’t affect any other array. Planets, however, are
    // paired: activePlanets[i] and planetModels[i] MUST stay aligned. Erasing
    // activePlanets[3] would make planetModels[4] render at index [3], showing
    // the wrong model on the wrong body. This is why we use tombstoning for
    // planets and erase-remove for explosions.
    //
    // THE ERASE-REMOVE IDIOM:
    //   std::remove_if moves all “dead” elements to the END of the vector and
    //   returns an iterator to the first dead element. Then .erase() chops off
    //   that tail. This is O(n) and the standard C++ way to filter a vector.
    // =========================================================================
    activeExplosions.erase(
        std::remove_if(activeExplosions.begin(), activeExplosions.end(),
                       [](const Explosion &e) { return !e.isAlive; }),
        activeExplosions.end());

    // End the 3D drawing mode to return to standard 2D rendering if necessary,
    // finishing the 3D perspective projection.
    EndMode3D();

    // --- 2D UI RENDERING ---
    // All 2D elements drawn after EndMode3D() so they overlay the 3D world
    // in flat screen-space, unaffected by the 3D camera matrix.

    // =========================================================================
    // LEFT INSTRUMENT PANEL — Planet Controls
    // =========================================================================
    // A single rounded dark panel groups the planet name, mass/radius sliders,
    // and reset button into one coherent instrument cluster. This prevents
    // controls from looking scattered and gives touch users a clear hit zone.
    //
    // Layout (all Y positions relative to panel top-left):
    //   Row 1 (y+12):  Planet name  /  "Tap a planet" hint
    //   Separator
    //   Row 2 (y+46):  Mass slider
    //   Row 3 (y+84):  Radius slider
    //   Row 4 (y+122): RESET SIMULATION button
    // =========================================================================
    {
      const float PNL_X = 10.0f;
      const float PNL_Y = 10.0f;
      const float PNL_W = 449.0f;        // Scaled +20%
      const float PNL_H = 221.0f;        // Scaled +20%
      const float PAD = 16.0f;           
      const float SLD_X = PNL_X + 110.0f; 
      const float SLD_W = PNL_W - 110.0f - PAD - 68.0f; 
      const float ROW_H = 40.0f;         

      DrawRectangleRounded(Rectangle{PNL_X, PNL_Y, PNL_W, PNL_H}, 0.06f, 8, Color{5, 5, 15, 175});
      DrawRectangleRoundedLinesEx(Rectangle{PNL_X, PNL_Y, PNL_W, PNL_H}, 0.06f, 8, 1.5f, Color{55, 55, 85, 210});

      float r1Y = PNL_Y + PAD;
      if (selectedPlanet != nullptr) {
        DrawText(selectedPlanet->name.c_str(), (int)(PNL_X + PAD), (int)r1Y, 26, YELLOW); 
        DrawText("SELECTED", (int)(PNL_X + PNL_W - 108.0f), (int)(r1Y + 5), 14, Color{200, 200, 70, 150});
      } else {
        DrawText("Tap a planet to select", (int)(PNL_X + PAD), (int)r1Y, 22, Color{100, 100, 100, 200});
      }

      DrawLineEx(Vector2{PNL_X + PAD, r1Y + 35.0f}, Vector2{PNL_X + PNL_W - PAD, r1Y + 35.0f}, 1.0f, Color{55, 55, 80, 180});

      float r2Y = r1Y + 46.0f; 
      if (selectedPlanet != nullptr) {
        GuiSlider(Rectangle{SLD_X, r2Y, SLD_W, ROW_H}, "Mass", TextFormat("%.2f", selectedPlanet->mass), &selectedPlanet->mass, selectedPlanet->massMin, selectedPlanet->massMax);
      } else {
        GuiSetState(STATE_DISABLED);
        float dummy = 0.5f;
        GuiSlider(Rectangle{SLD_X, r2Y, SLD_W, ROW_H}, "Mass", "--", &dummy, 0.0f, 1.0f);
        GuiSetState(STATE_NORMAL);
      }

      float r3Y = r2Y + ROW_H + 11.0f; 
      if (selectedPlanet != nullptr) {
        GuiSlider(Rectangle{SLD_X, r3Y, SLD_W, ROW_H}, "Radius", TextFormat("%.2f", selectedPlanet->radius), &selectedPlanet->radius, selectedPlanet->radiusMin, selectedPlanet->radiusMax);
      } else {
        GuiSetState(STATE_DISABLED);
        float dummy = 0.5f;
        GuiSlider(Rectangle{SLD_X, r3Y, SLD_W, ROW_H}, "Radius", "--", &dummy, 0.0f, 1.0f);
        GuiSetState(STATE_NORMAL);
      }

      float r4Y = r3Y + ROW_H + 11.0f;
      if (GuiButton(Rectangle{PNL_X + PAD, r4Y, PNL_W - PAD * 2.0f, ROW_H}, "RESET SIMULATION")) {
        selectedPlanet = nullptr;
        prevSelectedPlanet = nullptr;
        prevSliderMass = -1.0f;
        prevSliderRadius = -1.0f;
        toastTimer = 0.0f;
        currentState = PAUSED;
        activePlanets = initialPlanets;
        activeExplosions.clear();
      }
    }

    // =========================================================================
    // RIGHT STATUS PANEL — Simulation State + Camera Speed
    // =========================================================================
    // Mirrors the left panel's visual language: same dark background, same
    // border style. Contains the PLAYING/PAUSED indicator and the camera speed
    // slider so the user can adjust navigation speed without a keyboard.
    // =========================================================================
    {
      const float RP_W = 422.0f; // Scaled +20%
      const float RP_X = (float)screenWidth - RP_W - 10.0f;
      const float RP_Y = 10.0f;
      const float RP_H = 122.0f; // Scaled +20%

      DrawRectangleRounded(Rectangle{RP_X, RP_Y, RP_W, RP_H}, 0.06f, 8, Color{5, 5, 15, 175});
      DrawRectangleRoundedLinesEx(Rectangle{RP_X, RP_Y, RP_W, RP_H}, 0.06f, 8, 1.5f, Color{55, 55, 85, 210});

      bool isPlaying = (currentState == PLAYING);
      DrawCircleV(Vector2{RP_X + 24.0f, RP_Y + 29.0f}, 10.0f, isPlaying ? Color{60, 220, 80, 255} : Color{220, 60, 60, 255});
      DrawText(isPlaying ? "PLAYING" : "PAUSED", (int)(RP_X + 43.0f), (int)(RP_Y + 17.0f), 26, isPlaying ? GREEN : RED);

      DrawLineEx(Vector2{RP_X + 14.0f, RP_Y + 58.0f}, Vector2{RP_X + RP_W - 14.0f, RP_Y + 58.0f}, 1.0f, Color{55, 55, 80, 180});

      GuiSlider(Rectangle{RP_X + 116.0f, RP_Y + 68.0f, RP_W - 134.0f, 37.0f}, "Cam Spd", TextFormat("%.1f", cameraSpeed), &cameraSpeed, 0.5f, 50.0f);
    }

    // =========================================================================
    // TOAST TRIGGER — Real-world value (fires on DRAG only, not on selection)
    // =========================================================================
    //
    // ROOT CAUSE OF THE OLD BUG:
    //   prevSliderMass was reset to -1.0f on deselection. The moment ANY planet
    //   was tapped, the condition (-1 != planet.mass) fired immediately —
    //   showing a toast before the user had moved anything. This felt like a
    //   system error notification, not a helpful feedback.
    //
    // FIX — SELECTION CHANGE GUARD:
    //   We now track prevSelectedPlanet (a Planet pointer). When the pointer
    //   changes (new planet selected), we silently SYNC prevSliderMass and
    //   prevSliderRadius to the NEW planet's current values. The next
    //   comparison will be (newValue == newValue) → false → no toast. Toast
    //   only fires when a value changes WHILE the same planet remains selected
    //   = user dragged.
    // =========================================================================
    if (selectedPlanet != nullptr) {

      // Silent sync on new selection — no toast triggered here
      if (selectedPlanet != prevSelectedPlanet) {
        prevSelectedPlanet = selectedPlanet;
        prevSliderMass = selectedPlanet->mass;
        prevSliderRadius = selectedPlanet->radius;
        // Do NOT set toastTimer here — silence on selection is intentional.
      }

      // -----------------------------------------------------------------------
      // Conversion anchors derived from the planet initialisation data:
      //   Sim Earth mass   = 10.0  →  real 5.972e24 kg
      //   Sim Earth radius = 0.55  →  real 6,371 km
      // All other bodies scale linearly from these two anchors.
      // -----------------------------------------------------------------------

      if (prevSliderMass != selectedPlanet->mass) {
        prevSliderMass = selectedPlanet->mass;
        toastTimer = 3.0f;

        // Convert sim mass → real-world kg using Earth as the anchor.
        // Sim Earth mass = 10.0 → real Earth mass = 5.972e24 kg
        // earthRatio = how many times heavier/lighter than Earth this body is.
        const double SIM_EARTH_MASS = 10.0;
        const double REAL_EARTH_MASS = 5.972e24; // kg

        double earthRatio = (double)selectedPlanet->mass / SIM_EARTH_MASS;
        double realKg = earthRatio * REAL_EARTH_MASS;

        // Display: "PlanetName  |  X.XXe+YY kg  |  Z.Zx Earth mass"
        // earthRatio IS the multiplier — no further division needed.
        // If < 1 we still show the decimal (e.g. 0.15x Earth) because that's
        // accurate and the kg value already gives the user the intuition.
        snprintf(toastText, sizeof(toastText),
                 "%s  |  %.3e kg  |  %.3gx Earth mass",
                 selectedPlanet->name.c_str(), realKg, earthRatio);
      }

      if (prevSliderRadius != selectedPlanet->radius) {
        prevSliderRadius = selectedPlanet->radius;
        toastTimer = 3.0f;

        // Convert sim radius → real-world km using Earth as the anchor.
        // Sim Earth radius = 0.55 → real Earth radius = 6,371 km
        // earthRadiusRatio = how many times larger/smaller than Earth this body
        // is.
        const double SIM_EARTH_RADIUS = 0.55;
        const double REAL_EARTH_RADIUS = 6371.0; // km

        double earthRadiusRatio =
            (double)selectedPlanet->radius / SIM_EARTH_RADIUS;
        double realKm = earthRadiusRatio * REAL_EARTH_RADIUS;

        // Display: "PlanetName  |  XXXXX km  |  Z.Zx Earth radius"
        snprintf(toastText, sizeof(toastText),
                 "%s  |  %.0f km  |  %.3gx Earth radius",
                 selectedPlanet->name.c_str(), realKm, earthRadiusRatio);
      }

    } else {
      // Planet deselected — reset all tracking for a clean slate next selection
      prevSelectedPlanet = nullptr;
      prevSliderMass = -1.0f;
      prevSliderRadius = -1.0f;
    }

    // =========================================================================
    // TOAST DRAW — Centered, fades out in the last 0.5 seconds
    // =========================================================================
    // Positioned at y=175 so it sits below both panels and never overlaps them.
    // A rounded pill background improves legibility over the 3D scene.
    // =========================================================================
    if (toastTimer > 0.0f) {
      toastTimer -= dt;
      if (toastTimer < 0.0f)
        toastTimer = 0.0f;

      unsigned char alpha = 255;
      if (toastTimer < 0.5f)
        alpha = (unsigned char)((toastTimer / 0.5f) * 255.0f);

      const int FS = 22; // +10% from 20
      int tw = MeasureText(toastText, FS);
      int tx = screenWidth / 2 - tw / 2;
      int ty = 175;

      DrawRectangleRounded(Rectangle{(float)(tx - 22), (float)(ty - 10),
                                     (float)(tw + 44), (float)(FS + 22)},
                           0.45f, 8,
                           Color{0, 0, 0, (unsigned char)(alpha * 0.65f)});

      DrawText(toastText, tx, ty, FS, Color{255, 228, 110, alpha});
    }

    // =========================================================================
    // BOTTOM CONTROL HINT
    // =========================================================================
    DrawText("SPACE: Play/Pause  |  Hold RMB + WASD: Free Camera  |  LMB: "
             "Select Planet",
             10, screenHeight - 28, 15, Color{90, 90, 90, 200}); // was 26, 14

    // =========================================================================
    // PLAY / PAUSE BUTTON — Centered at bottom of screen
    // =========================================================================
    {
      const float PP_W = 220.0f;
      const float PP_H = 88.0f;
      const float PP_X = (float)screenWidth / 2.0f - PP_W / 2.0f;
      const float PP_Y = (float)screenHeight - PP_H -
                         40.0f; // Shifted lower to the bottom edge

      DrawRectangleRounded(
          Rectangle{PP_X - 5.0f, PP_Y - 5.0f, PP_W + 10.0f, PP_H + 10.0f},
          0.40f, 8,
          (currentState == PLAYING) ? Color{50, 180, 70, 80}
                                    : Color{200, 60, 60, 80});

      GuiSetStyle(DEFAULT, TEXT_SIZE, 28);
      GuiSetAlpha(0.92f);
      if (GuiButton(Rectangle{PP_X, PP_Y, PP_W, PP_H},
                    (currentState == PLAYING) ? "|| PAUSE" : ">  PLAY")) {
        currentState = (currentState == PLAYING) ? PAUSED : PLAYING;
      }
      GuiSetAlpha(1.0f);
      GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
    }

    // =========================================================================
    // MOBILE UI NAVIGATION — PUBG Style (Left Move, Right Look, Right Elevate)
    // =========================================================================
    if (!isCameraActive) {
      const float JS_BASE_RADIUS  = 156.0f; // Scaled +20%
      const float JS_THUMB_RADIUS = 54.0f;  // Scaled +20%
      const float JS_DEAD_ZONE    = 0.10f;
      const float MARGIN          = 96.0f;

      // Left joystick base position (fixed on left side)
      Vector2 jsBase = { MARGIN + JS_BASE_RADIUS, (float)screenHeight - MARGIN - JS_BASE_RADIUS };

      // Elevation buttons (Right side)
      Rectangle btnUp   = { (float)screenWidth - 160.0f, (float)screenHeight - 360.0f, 120.0f, 100.0f };
      Rectangle btnDown = { (float)screenWidth - 160.0f, (float)screenHeight - 220.0f, 120.0f, 100.0f };

      // Camera vectors
      Vector3 camForward = Vector3Normalize(Vector3Subtract(camera.target, camera.position));
      Vector3 camRight   = Vector3Normalize(Vector3CrossProduct(camForward, camera.up));
      Vector3 worldUp    = {0.0f, 1.0f, 0.0f};
      float   moveAmount = cameraSpeed * dt;

      int tc = GetTouchPointCount();

      // 1. Process Elevation Buttons First
      auto isRectPressed = [&](Rectangle rect) -> bool {
        if (tc > 0) {
          for (int t = 0; t < tc; t++) {
            if (CheckCollisionPointRec(GetTouchPosition(t), rect)) return true;
          }
          return false;
        }
        return IsMouseButtonDown(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), rect);
      };

      if (isRectPressed(btnUp)) {
        camera.position = Vector3Add(camera.position, Vector3Scale(worldUp, moveAmount));
        camera.target   = Vector3Add(camera.target,   Vector3Scale(worldUp, moveAmount));
      }
      if (isRectPressed(btnDown)) {
        camera.position = Vector3Subtract(camera.position, Vector3Scale(worldUp, moveAmount));
        camera.target   = Vector3Subtract(camera.target,   Vector3Scale(worldUp, moveAmount));
      }

      // 2. Touch Routing (Identify Joystick vs Look Area)
      bool currentJoystickHeld = false;
      bool currentLookHeld     = false;

      // Define UI Exclusion Zones so sliders and panels don't trigger camera movement (SCALED UP)
      Rectangle leftPanelRec  = { 10.0f, 10.0f, 449.0f, 221.0f };
      Rectangle rightPanelRec = { (float)screenWidth - 432.0f, 10.0f, 422.0f, 122.0f };
      Rectangle playPauseRec  = { (float)screenWidth / 2.0f - 120.0f, (float)screenHeight - 140.0f, 240.0f, 110.0f };

      if (tc > 0) {
        for (int t = 0; t < tc; t++) {
          Vector2 tp = GetTouchPosition(t);
          int tid = GetTouchPointId(t);

          // Ignore touches on ALL UI elements (Panels, Play Button, Fly
          // Buttons)
          if (CheckCollisionPointRec(tp, btnUp) ||
              CheckCollisionPointRec(tp, btnDown) ||
              CheckCollisionPointRec(tp, leftPanelRec) ||
              CheckCollisionPointRec(tp, rightPanelRec) ||
              CheckCollisionPointRec(tp, playPauseRec)) {
            continue;
          }

          // Track Joystick (Left Side)
          if (joystickActive && tid == joystickTouchPointId) {
            currentJoystickHeld = true;
            float dx = tp.x - joystickCenter.x;
            float dy = tp.y - joystickCenter.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > JS_BASE_RADIUS) {
              dx = (dx / dist) * JS_BASE_RADIUS;
              dy = (dy / dist) * JS_BASE_RADIUS;
            }
            joystickThumb = {joystickCenter.x + dx, joystickCenter.y + dy};
          }
          // Track Look (Right Side)
          else if (lookActive && tid == lookTouchPointId) {
            currentLookHeld = true;
            Vector2 delta = {tp.x - lastLookPos.x, tp.y - lastLookPos.y};
            lastLookPos = tp;

            // Apply Look Delta (Yaw and Pitch)
            float sensitivity = 0.004f;
            float yaw = -delta.x * sensitivity;
            float pitch = -delta.y * sensitivity;

            // Yaw
            camForward = Vector3RotateByAxisAngle(camForward, worldUp, yaw);
            camRight =
                Vector3Normalize(Vector3CrossProduct(camForward, worldUp));

            // Pitch (Prevent gimbal lock / flipping upside down)
            Vector3 newForward =
                Vector3RotateByAxisAngle(camForward, camRight, pitch);
            float dot = Vector3DotProduct(newForward, worldUp);
            if (dot < 0.95f && dot > -0.95f) {
              camForward = newForward;
            }

            camera.target = Vector3Add(camera.position, camForward);
          }
          // Unassigned touches (New finger placed on screen)
          else if (!joystickActive && tp.x < screenWidth / 2.0f) {
            joystickActive = true;
            joystickTouchPointId = tid;
            joystickCenter = jsBase;
            joystickThumb = tp;
            currentJoystickHeld = true;
          } else if (!lookActive && tp.x >= screenWidth / 2.0f) {
            lookActive = true;
            lookTouchPointId = tid;
            lastLookPos = tp;
            currentLookHeld = true;
          }
        }
      } else if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        // Desktop fallback for testing
        Vector2 mp = GetMousePosition();
        // Ignore desktop UI clicks too
        if (!CheckCollisionPointRec(mp, leftPanelRec) &&
            !CheckCollisionPointRec(mp, rightPanelRec) &&
            !CheckCollisionPointRec(mp, playPauseRec) &&
            !CheckCollisionPointRec(mp, btnUp) &&
            !CheckCollisionPointRec(mp, btnDown)) {
          if (mp.x < screenWidth / 2.0f) {
            joystickActive = true;
            currentJoystickHeld = true;
            joystickCenter = jsBase;
            float dx = mp.x - jsBase.x;
            float dy = mp.y - jsBase.y;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > JS_BASE_RADIUS) {
              dx = (dx / dist) * JS_BASE_RADIUS;
              dy = (dy / dist) * JS_BASE_RADIUS;
            }
            joystickThumb = {joystickCenter.x + dx, joystickCenter.y + dy};
          }
        }
      }

      // Cleanup lifted fingers
      if (!currentJoystickHeld) {
        joystickActive = false;
        joystickTouchPointId = -1;
        joystickThumb = jsBase;
      }
      if (!currentLookHeld) {
        lookActive = false;
        lookTouchPointId = -1;
      }

      // 3. Apply Joystick Movement to Camera
      if (joystickActive) {
        float normX = (joystickThumb.x - joystickCenter.x) / JS_BASE_RADIUS;
        float normY = (joystickThumb.y - joystickCenter.y) / JS_BASE_RADIUS;

        if (fabsf(normX) > JS_DEAD_ZONE) {
          camera.position = Vector3Add(
              camera.position, Vector3Scale(camRight, normX * moveAmount));
          camera.target = Vector3Add(
              camera.target, Vector3Scale(camRight, normX * moveAmount));
        }
        if (fabsf(normY) > JS_DEAD_ZONE) {
          // Negative Y is up on screen, which means move forward.
          camera.position = Vector3Subtract(
              camera.position, Vector3Scale(camForward, normY * moveAmount));
          camera.target = Vector3Subtract(
              camera.target, Vector3Scale(camForward, normY * moveAmount));
        }
      }

      // --- DRAW UI ELEMENTS ---

      // Joystick
      DrawCircleV(jsBase, JS_BASE_RADIUS, Color{50, 50, 50, 120});
      DrawCircleLinesV(jsBase, JS_BASE_RADIUS, Color{255, 255, 255, 160});
      Vector2 thumbDrawPos = joystickActive ? joystickThumb : jsBase;
      DrawCircleV(thumbDrawPos, JS_THUMB_RADIUS,
                  joystickActive ? Color{255, 255, 255, 220}
                                 : Color{160, 160, 160, 170});
      DrawText("MOVE", (int)(jsBase.x - 36), (int)(jsBase.y - JS_BASE_RADIUS - 42), 29, LIGHTGRAY);

      // Elevation Buttons
      GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
      GuiSetAlpha(0.75f);
      GuiButton(btnUp, "^ FLY UP");
      GuiButton(btnDown, "v FLY DN");
      GuiSetAlpha(1.0f);
      GuiSetStyle(DEFAULT, TEXT_SIZE, 10);

      // Look Hint
      if (!lookActive && tc == 0) {
        DrawText("[ Drag Right Half to Look Around ]", screenWidth - 420,
                 screenHeight / 2, 22, Color{255, 255, 255, 100});
      }

    } // end if (!isCameraActive)

    // End the drawing phase and swap the buffers to display the rendered image
    // on the screen.
    EndDrawing();
  }

  // ===========================================================================
  // CLEANUP: Ensure the cursor is visible before exiting.
  // ===========================================================================
  // If the user closes the window while holding RMB (camera mode active),
  // the cursor would remain hidden system-wide. This safety call guarantees
  // the cursor is always restored before the program exits.
  // ===========================================================================
  EnableCursor();

  // ===========================================================================
  // CLEANUP: Free GPU Resources
  // ===========================================================================
  // Before closing the program, we must tell the Graphics Card to delete the
  // 3D models we loaded, preventing memory leaks in VRAM.
  for (size_t i = 0; i < planetModels.size(); i++) {
    UnloadModel(planetModels[i]);
  }

  // Unload the lighting shader from VRAM
  UnloadShader(lightShader);

  // After the loop breaks, properly close the Raylib window to free allocated
  // resources safely and tell the OS we are done here.
  CloseWindow();

  // End the main function, returning 0 to signal that the program executed
  // successfully without any errors.
  return 0;
}
