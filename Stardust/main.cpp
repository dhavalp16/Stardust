#define _CRT_SECURE_NO_WARNINGS // Tells Microsoft to ignore standard C warnings
// Include the Raylib library header file to access all its functions, such as
// window creation and 3D drawing.
#include "raylib.h"
// Include Raylib's math library for 3D vector math functions.
#include "raymath.h"
// Include the standard C++ math library for math operations like square root
// (std::sqrt).
#include <cmath>

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

// The main function where our C++ program begins execution.
int main() {
  // Initialize the window with a width of 1280 pixels.
  const int screenWidth = 1280;
  // Initialize the window with a height of 720 pixels.
  const int screenHeight = 720;

  // Call Raylib's InitWindow function to open a window with the specified
  // dimensions and a title "Stardust".
  InitWindow(screenWidth, screenHeight, "Stardust");

  // Declare a Camera3D struct instance named 'camera' to handle our 3D
  // viewpoint.
  Camera3D camera = {};
  // Set the camera's position in 3D space. We place it at x=0.0f, y=10.0f (up),
  // and z=10.0f (backwards). This creates a Vector3 in memory.
  camera.position = Vector3{0.0f, 10.0f, 10.0f};
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

  // Declare a Planet struct instance for the Earth and initialize its memory
  // block.
  Planet earth = {};
  // Assign a Vector3 to Earth's position at the center of the world coordinates
  // (x=0, y=0, z=0).
  earth.position = Vector3{0.0f, 0.0f, 0.0f};
  // Assign a float value of 2.0f to Earth's radius, making it a fairly large
  // sphere.
  earth.radius = 2.0f;
  // Assign a float value of 5.97f to Earth's mass. Currently static, but will
  // be used in future gravitational calculations.
  earth.mass = 5.97f;
  // Initialize Earth's velocity to zero so it remains stationary at the center
  // of the world.
  earth.velocity = Vector3{0.0f, 0.0f, 0.0f};
  // Assign a bright BLUE color to Earth using Raylib's pre-defined color
  // constants.
  earth.color = BLUE;

  // Declare another Planet struct instance for the Moon and initialize its
  // memory block.
  Planet moon = {};
  // Assign a Vector3 to the Moon's position. We place it slightly offset from
  // Earth along the X-axis at x=5.0f, leaving y=0, z=0.
  moon.position = Vector3{5.0f, 0.0f, 0.0f};
  // Assign a float value of 0.5f to the Moon's radius so it appears smaller
  // than Earth.
  moon.radius = 0.5f;
  // Assign a float value of 0.073f to the Moon's mass.
  moon.mass = 0.073f;
  // Assign an initial velocity to the Moon. In a 3D vector space, this vector
  // determines how much X, Y, and Z will change each frame. Setting Z to 14.64f
  // gives the moon an initial push forward so it will orbit instead of crashing
  // straight into Earth. (Scaled up from 0.244f by multiplying by 60 because we
  // decoupled physics from framerate using delta time)
  moon.velocity = Vector3{0.0f, 0.0f, 14.64f};
  // Assign a LIGHTGRAY color to the Moon using Raylib's predefined color
  // constants.
  moon.color = LIGHTGRAY;

  // Set the target frame rate to 60 frames per second so the window updates at
  // a consistent, smooth speed.
  SetTargetFPS(180);

  // Initialize the float variable to represent the Earth's mass. This will be
  // updated by our UI slider.
  float earthMassSlider = 5.97f;

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
  // THE '*' (ASTERISK / STAR):
  //   When you see 'Planet*', read it as "a pointer to a Planet". The asterisk
  //   is how C++ knows this variable holds an address, not a Planet value.
  //   - 'Planet earth;'   <-- 'earth' IS a Planet (holds actual data)
  //   - 'Planet* ptr;'    <-- 'ptr' is a pointer TO a Planet (holds an address)
  //
  // WHAT IS nullptr?
  // ----------------
  // 'nullptr' is a special C++ keyword meaning "null pointer" — it means the
  // pointer is intentionally pointing at NOTHING. We initialize to nullptr
  // because when the program starts, no planet has been clicked yet.
  //
  // WHY INITIALIZE TO nullptr?
  //   If you leave a pointer uninitialized, it contains garbage data — a random
  //   memory address left over from whatever used that RAM previously. If you
  //   accidentally try to read from that garbage address, your program CRASHES
  //   (a "segmentation fault"). By setting it to nullptr, we have a safe,
  //   predictable "empty" state we can check:
  //     if (selectedPlanet != nullptr) { /* a planet IS selected, safe to use
  //     */ }
  //
  // THE '&' (AMPERSAND / ADDRESS-OF OPERATOR):
  //   Later in this code, you'll see us write 'selectedPlanet = &earth;'.
  //   The '&' placed before a variable name means "give me the memory address
  //   of this variable". So '&earth' evaluates to the address in RAM where
  //   the 'earth' struct lives. We then store that address in 'selectedPlanet'.
  //
  // ANALOGY:
  //   Imagine your planets are houses on a street.
  //   - 'earth'           = the actual house at 123 Galaxy Lane
  //   - '&earth'          = the street address "123 Galaxy Lane"
  //   - 'selectedPlanet'  = a GPS device that can store one address
  //   - 'nullptr'         = the GPS displaying "No destination set"
  //   - '*selectedPlanet' = "go to the address on the GPS and look at the
  //   house"
  // ===========================================================================
  Planet *selectedPlanet = nullptr;

  // Enter a continuous while-loop that will run until the user presses the ESC
  // key or closes the window via the cross button.
  while (!WindowShouldClose()) {
    // Calculate the time it took to render the last frame, in seconds. We use
    // this 'delta time' to decouple our physics loop from the monitor's frame
    // rate, ensuring consistent speed across different computers.
    float dt = GetFrameTime();

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
    // IsMouseButtonPressed() is a Raylib function that returns 'true' for
    // exactly ONE frame when the specified mouse button transitions from
    // "not pressed" to "pressed". MOUSE_BUTTON_LEFT is the left mouse button.
    //
    // HOW RAY PICKING WORKS (conceptual overview):
    // -----------------------------------------------
    // Your 3D world is displayed on a flat 2D screen. When you click on the
    // screen, you're clicking a 2D pixel coordinate (e.g., x=640, y=360).
    // But your planets exist in 3D space with (x, y, z) coordinates. To figure
    // out WHICH 3D object lives "behind" that 2D pixel, we shoot an invisible
    // laser beam (a "ray") from the camera, through the clicked pixel, and out
    // into the 3D world. If that ray hits a planet's sphere, we know the user
    // clicked on it!
    //
    // STEP-BY-STEP:
    //   1. Get the 2D mouse position on screen → GetMousePosition()
    //   2. Convert that 2D point into a 3D ray    → GetMouseRay()
    //   3. Test if the ray hits Earth's sphere    → GetRayCollisionSphere()
    //   4. Test if the ray hits Moon's sphere     → GetRayCollisionSphere()
    //   5. If a hit occurred, set selectedPlanet  → selectedPlanet = &earth;
    //
    // GetMouseRay(Vector2 mousePosition, Camera3D camera):
    //   This Raylib function takes the 2D screen position where the mouse
    //   clicked and the camera (which defines perspective, position, and
    //   orientation) and returns a 'Ray' struct. The Ray struct contains:
    //     - ray.position  : the 3D origin point of the ray (the camera
    //     position)
    //     - ray.direction : a 3D vector pointing from the camera through the
    //                       clicked pixel and into the world
    //
    // GetRayCollisionSphere(Ray ray, Vector3 center, float radius):
    //   This Raylib function tests if the given ray intersects (hits) a sphere
    //   defined by a center point and radius. It returns a 'RayCollision'
    //   struct containing:
    //     - collision.hit      : a boolean (true/false) — did the ray hit?
    //     - collision.distance : how far along the ray the hit occurred
    //     - collision.point    : the 3D point where the ray hit the sphere
    //     - collision.normal   : the surface normal at the hit point
    //   We only need '.hit' to know if the user clicked on a planet.
    // =========================================================================
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      // STEP 1 & 2: Get a 3D ray from the camera through the mouse's 2D
      // screen position. GetMousePosition() returns a Vector2 (x, y) with the
      // current mouse coordinates on the window. GetMouseRay() uses the camera
      // to transform that 2D point into a 3D ray shooting into the world.
      Ray mouseRay = GetMouseRay(GetMousePosition(), camera);

      // STEP 3: Test if the ray hits the Earth's sphere.
      // We pass the ray, the Earth's center position (Vector3), and its radius.
      // The function returns a RayCollision struct with the results.
      RayCollision earthCollision =
          GetRayCollisionSphere(mouseRay, earth.position, earth.radius);

      // STEP 4: Test if the ray hits the Moon's sphere.
      // Same process, but using the Moon's position and radius.
      RayCollision moonCollision =
          GetRayCollisionSphere(mouseRay, moon.position, moon.radius);

      // STEP 5: Determine which planet (if any) was clicked.
      //
      // WHY CHECK BOTH AND COMPARE DISTANCE?
      // If the Earth and Moon overlap on screen (one is behind the other),
      // the ray could hit BOTH spheres. In that case, we want to select the
      // one that is CLOSER to the camera (smaller .distance value), because
      // that's the one the user visually intended to click.
      //
      // LOGIC:
      //   - If ONLY Earth was hit → select Earth
      //   - If ONLY Moon was hit → select Moon
      //   - If BOTH were hit → select whichever is closer (smaller distance)
      //   - If NEITHER was hit → deselect (set to nullptr, meaning "nothing")
      if (earthCollision.hit && moonCollision.hit) {
        // Both were hit! Compare distances to pick the closer one.
        // The '&' here is the "address-of" operator. '&earth' gives us the
        // memory address of the 'earth' variable, which we store in our
        // pointer. Now 'selectedPlanet' points to 'earth'.
        if (earthCollision.distance <= moonCollision.distance) {
          selectedPlanet = &earth; // Earth is closer, select it.
        } else {
          selectedPlanet = &moon; // Moon is closer, select it.
        }
      } else if (earthCollision.hit) {
        // Only Earth was hit by the ray. Select Earth.
        selectedPlanet = &earth;
      } else if (moonCollision.hit) {
        // Only Moon was hit by the ray. Select Moon.
        selectedPlanet = &moon;
      } else {
        // The ray didn't hit any planet. Deselect by setting the pointer back
        // to nullptr ("pointing at nothing"). This clears the selection.
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
      // --- PHYSICS PIPELINE ---
      // Instead of doing all the math inside the loop, we call our abstracted
      // functions. This makes the main loop much cleaner and easier to read. We
      // pass our planet objects into these functions. Because the functions
      // expect references (Planet&), they don't make copies. They modify these
      // exact 'earth' and 'moon' variables directly in memory.

      // 1. Calculate and apply gravitational forces.
      // Calculate the pull of the Earth on the Moon.
      // We pass earthMassSlider instead of earth.mass so the gravity updates
      // dynamically when the user moves the UI slider. We use a constant G of
      // 180.0f scaled for our game space.
      ApplyGravity(earth, earthMassSlider, moon, 180.0f, dt);

      // 2. Integrate velocities to update positions.
      // Apply Earth's velocity to its position (currently zero, so it won't
      // move, but good practice).
      UpdatePosition(earth, dt);
      // Apply Moon's velocity (which was just changed by gravity) to its
      // position, causing orbital movement.
      UpdatePosition(moon, dt);
    }
    // If currentState is PAUSED or SUMMARY, we skip the physics entirely.
    // The planets stay frozen but are still drawn below — you can observe
    // their positions and click on them while paused!

    // Begin the drawing phase for this frame. All rendering elements must be
    // placed between BeginDrawing() and EndDrawing().
    BeginDrawing();

    // Clear the screen background to a dark color to represent the emptiness of
    // space and reset the color buffer before drawing new frames.
    ClearBackground(BLACK);

    // Initialize the 3D drawing mode, passing our configured 3D camera to
    // Raylib so it calculates the correct perspective matrices.
    BeginMode3D(camera);

    // Draw the Earth sphere using its Vector3 position in memory, its radius
    // size, and its color.
    DrawSphere(earth.position, earth.radius, earth.color);

    // Draw the Moon sphere using its distinct Vector3 position, smaller radius,
    // and gray color.
    DrawSphere(moon.position, moon.radius, moon.color);

    // =========================================================================
    // FEATURE 2 (continued): VISUAL SELECTION INDICATOR
    // =========================================================================
    //
    // If targetPlanet is NOT nullptr (meaning the user has clicked on a
    // planet), we draw a visual indicator around the selected planet so the
    // user can clearly see which one they picked.
    //
    // 'selectedPlanet != nullptr' checks if the pointer is currently pointing
    // at a valid Planet. Remember:
    //   - nullptr means "no planet selected" → skip drawing indicator
    //   - &earth or &moon means "a planet IS selected" → draw indicator
    //
    // THE '->' (ARROW OPERATOR):
    //   When you have a POINTER to a struct (like 'selectedPlanet'), you use
    //   the '->' operator to access members of the struct the pointer points
    //   to. It's a shorthand for dereferencing and then accessing:
    //     selectedPlanet->position   is equivalent to
    //     (*selectedPlanet).position
    //   The '*' before the pointer name is called "dereferencing" — it means
    //   "go to the address this pointer holds and give me the actual data".
    //   The '->' combines both steps into one clean syntax.
    //
    // DrawSphereWires(Vector3 center, float radius, int rings, int slices,
    //                 Color color):
    //   This Raylib function draws a WIREFRAME sphere (just the outlines, not
    //   filled). Parameters:
    //   - center : the 3D position to draw the wireframe around
    //   - radius : how big the wireframe sphere should be (we make it slightly
    //              larger than the planet's radius so it appears around it,
    //              not overlapping)
    //   - rings  : number of horizontal ring lines (more = smoother)
    //   - slices : number of vertical slice lines (more = smoother)
    //   - color  : the color of the wireframe lines
    //
    // We use 'selectedPlanet->radius * 1.2f' to make the wireframe 20% bigger
    // than the actual planet, creating a visible "halo" effect around it.
    // =========================================================================
    if (selectedPlanet != nullptr) {
      DrawSphereWires(
          selectedPlanet->position, // Center: the selected planet's position
          selectedPlanet->radius * 1.1f, // Radius: 20% bigger for visibility
          16,                            // Rings: 16 horizontal lines
          16,                            // Slices: 16 vertical lines
          YELLOW                         // Color: bright yellow stands out
      );
    }

    // End the 3D drawing mode to return to standard 2D rendering if necessary,
    // finishing the 3D perspective projection.
    EndMode3D();

    // --- 2D UI RENDERING ---
    // UI text and controls MUST be drawn outside of the 3D camera mode (i.e.,
    // after EndMode3D()). This is because the UI needs to be drawn in 2D screen
    // space (pixels mapping directly to the window) rather than 3D world space.
    // If we drew the UI inside Mode3D, it would be affected by the camera's
    // perspective, position, and rotation, causing it to render as an object
    // scattered in the 3D environment. Drawing it here overlays the 2D elements
    // seamlessly on top of our 3D world.

    // Draw the slider. We pass a pointer to earthMassSlider (&earthMassSlider)
    // so raygui modifies it directly.
    GuiSlider(Rectangle{20, 10, 240, 40}, "1.0", "20.0", &earthMassSlider, 1.0f,
              20.0f);

    // =========================================================================
    // FEATURE 1 (continued): HUD — Display the current engine state on screen
    // =========================================================================
    //
    // DrawText(const char* text, int posX, int posY, int fontSize, Color
    // color):
    //   A Raylib function that draws a string of text on the screen at the
    //   given (x, y) pixel position with the specified font size and color.
    //   This is 2D rendering, so it appears as a flat overlay on top of the
    //   3D scene.
    //
    // THE TERNARY OPERATOR (condition ? valueIfTrue : valueIfFalse):
    //   This is a compact one-line 'if/else'. It evaluates the condition:
    //   - If TRUE, it returns the value after '?'
    //   - If FALSE, it returns the value after ':'
    //
    //   Example:  (currentState == PLAYING) ? "PLAYING" : "PAUSED"
    //   Reads as: "Is currentState equal to PLAYING? If yes, use the text
    //   'PLAYING'. If no, use the text 'PAUSED'."
    //
    // We display this in the top-right area so it doesn't overlap with the
    // mass slider in the top-left.
    // =========================================================================
    DrawText((currentState == PLAYING) ? "[ PLAYING ]" : "[ PAUSED ]",
             screenWidth - 200, // X position: 200 pixels from the right edge
             20,                // Y position: 20 pixels from the top
             20,                // Font size in pixels
             (currentState == PLAYING)
                 ? GREEN
                 : RED // Green when playing, red when paused
    );

    // Display a helpful hint for the user at the bottom of the screen so they
    // know how to toggle the simulation.
    DrawText("Press SPACE to play/pause", 10, screenHeight - 30, 18, DARKGRAY);

    // =========================================================================
    // FEATURE 2 (continued): HUD — Display selected planet info
    // =========================================================================
    //
    // If a planet is selected, show its name near the bottom-right of the
    // screen. We compare the pointer's value (the memory address it holds) to
    // the address of 'earth' to determine which planet is selected.
    //
    // 'selectedPlanet == &earth' reads as: "Does the address stored in
    // selectedPlanet equal the address of earth?" If yes, the user clicked
    // Earth. Otherwise, it must be the Moon (since those are our only two
    // planets).
    // =========================================================================
    if (selectedPlanet != nullptr) {
      DrawText((selectedPlanet == &earth) ? "Selected: Earth"
                                          : "Selected: Moon",
               screenWidth - 200, // X position: 200px from right edge
               50,                // Y position: below the state indicator
               20,                // Font size
               YELLOW             // Matches the wireframe indicator color
      );
    }

    // End the drawing phase and swap the buffers to display the rendered image
    // on the screen.
    EndDrawing();
  }

  // After the loop breaks, properly close the Raylib window to free allocated
  // resources safely and tell the OS we are done here.
  CloseWindow();

  // End the main function, returning 0 to signal that the program executed
  // successfully without any errors.
  return 0;
}
