#define _CRT_SECURE_NO_WARNINGS // Tells Microsoft to ignore standard C warnings
// Include the Raylib library header file to access all its functions, such as window creation and 3D drawing.
#include "raylib.h"
// Include Raylib's math library for 3D vector math functions.
#include "raymath.h"
// Include the standard C++ math library for math operations like square root (std::sqrt).
#include <cmath>

// Define RAYGUI_IMPLEMENTATION to generate the implementation part of the GUI library.
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

// Define a structure to represent a Planet in our 3D space.
struct Planet {
    // Vector3 is a struct containing three floats (x, y, z) representing coordinates in 3D space. Memory-wise, it's 12 contiguous bytes (3 * 4 bytes).
    Vector3 position;
    // A single precision floating-point number (4 bytes) representing the physical size (radius) of the planet.
    float radius;
    // A single precision floating-point number (4 bytes) representing the mass of the planet (unused for now but ready for physics).
    float mass;
    // A Vector3 representing the velocity currently experienced by the planet. Contains floats for speed along X, Y, and Z axes.
    Vector3 velocity;
    // A Raylib Color struct containing four unsigned bytes (r, g, b, a) representing red, green, blue, and alpha channels.
    Color color;
};

// The main function where our C++ program begins execution.
int main() {
    // Initialize the window with a width of 1280 pixels.
    const int screenWidth = 1280;
    // Initialize the window with a height of 720 pixels.
    const int screenHeight = 720;

    // Call Raylib's InitWindow function to open a window with the specified dimensions and a title "Stardust".
    InitWindow(screenWidth, screenHeight, "Stardust");

    // Declare a Camera3D struct instance named 'camera' to handle our 3D viewpoint.
    Camera3D camera = {};
    // Set the camera's position in 3D space. We place it at x=0.0f, y=10.0f (up), and z=10.0f (backwards). This creates a Vector3 in memory.
    camera.position = Vector3{ 0.0f, 10.0f, 10.0f };
    // Set the camera's target to look at. We point it at the center of the world, meaning x=0.0f, y=0.0f, z=0.0f. This is another Vector3 in memory.
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    // Set the camera's "up" vector to tell it which way is up in our 3D world. In our case, the positive Y-axis is up (x=0, y=1, z=0). It's also a Vector3.
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    // Set the field of view (fovy) for the camera in degrees. 45.0f provides a natural perspective, similar to human vision.
    camera.fovy = 45.0f;
    // Set the projection type for the camera. CAMERA_PERSPECTIVE makes objects appear smaller as they get further away, creating depth.
    camera.projection = CAMERA_PERSPECTIVE;

    // Declare a Planet struct instance for the Earth and initialize its memory block.
    Planet earth = {};
    // Assign a Vector3 to Earth's position at the center of the world coordinates (x=0, y=0, z=0).
    earth.position = Vector3{ 0.0f, 0.0f, 0.0f };
    // Assign a float value of 2.0f to Earth's radius, making it a fairly large sphere.
    earth.radius = 2.0f;
    // Assign a float value of 5.97f to Earth's mass. Currently static, but will be used in future gravitational calculations.
    earth.mass = 5.97f;
    // Initialize Earth's velocity to zero so it remains stationary at the center of the world.
    earth.velocity = Vector3{ 0.0f, 0.0f, 0.0f };
    // Assign a bright BLUE color to Earth using Raylib's pre-defined color constants.
    earth.color = BLUE;

    // Declare another Planet struct instance for the Moon and initialize its memory block.
    Planet moon = {};
    // Assign a Vector3 to the Moon's position. We place it slightly offset from Earth along the X-axis at x=5.0f, leaving y=0, z=0.
    moon.position = Vector3{ 5.0f, 0.0f, 0.0f };
    // Assign a float value of 0.5f to the Moon's radius so it appears smaller than Earth.
    moon.radius = 0.5f;
    // Assign a float value of 0.073f to the Moon's mass.
    moon.mass = 0.073f;
    // Assign an initial velocity to the Moon. In a 3D vector space, this vector determines how much X, Y, and Z will change each frame.
    // Setting Z to 14.64f gives the moon an initial push forward so it will orbit instead of crashing straight into Earth.
    // (Scaled up from 0.244f by multiplying by 60 because we decoupled physics from framerate using delta time)
    moon.velocity = Vector3{ 0.0f, 0.0f, 14.64f };
    // Assign a LIGHTGRAY color to the Moon using Raylib's predefined color constants.
    moon.color = LIGHTGRAY;

    // Set the target frame rate to 60 frames per second so the window updates at a consistent, smooth speed.
    SetTargetFPS(165);

    // Initialize the float variable to represent the Earth's mass. This will be updated by our UI slider.
    float earthMassSlider = 5.97f;

    // Enter a continuous while-loop that will run until the user presses the ESC key or closes the window via the cross button.
    while (!WindowShouldClose()) {
        // Calculate the time it took to render the last frame, in seconds. We use this 'delta time'
        // to decouple our physics loop from the monitor's frame rate.
        float dt = GetFrameTime();

        // --- GRAVITY CALCULATION ---
        // 1. Calculate the distance vector (direction) from the Moon to the Earth.
        // We subtract the Moon's coordinates from the Earth's to get a vector pointing TO the Earth.
        Vector3 direction = Vector3{
            earth.position.x - moon.position.x,
            earth.position.y - moon.position.y,
            earth.position.z - moon.position.z
        };

        // 2. Calculate the squared distance (the length squared of the direction vector).
        // We use the Pythagorean theorem extended to 3D: a^2 + b^2 + c^2 = d^2
        float distanceSquared = (direction.x * direction.x) + (direction.y * direction.y) + (direction.z * direction.z);

        // We avoid dividing by zero just in case they overlap perfectly.
        if (distanceSquared > 0.0001f) {
            // Calculate the actual distance by taking the square root. (Needs <cmath> for std::sqrt)
            float distance = std::sqrt(distanceSquared);

            // 3. Normalize the direction vector so its length is exactly 1.
            // By dividing the X/Y/Z vector components by the total length (distance), we get a pure 'direction' without magnitude.
            Vector3 normalizedDirection = Vector3{
                direction.x / distance,
                direction.y / distance,
                direction.z / distance
            };

            // 4. Calculate the magnitude of the gravitational pull using Newton's Law of Universal Gravitation: F = G * (m1 * m2) / r^2
            // We use a predefined gravitational constant (G) tuned for our game scale.
            // (Scaled up from 0.05f to 180.0f by multiplying by 60 * 60 to counteract the double application of dt)
            float G = 180.0f;
            // The force increases with mass and decreases with the square of the distance.
            // We use earthMassSlider here instead of the static earth.mass so the gravity alters in real-time based on the UI.
            float force = G * (earthMassSlider * moon.mass) / distanceSquared;

            // 5. Apply the force as Acceleration to the Moon's Velocity.
            // From Newton's Second Law (F = ma), Acceleration = Force / Mass. We divide by the Moon's mass.
            float acceleration = force / moon.mass;

            // 6. Multiply the pure direction vector by the acceleration magnitude, and track how much time passed (dt).
            // This spreads the pull across the X, Y, and Z axes appropriately.
            Vector3 velocityChange = Vector3{
                normalizedDirection.x * acceleration * dt,
                normalizedDirection.y * acceleration * dt,
                normalizedDirection.z * acceleration * dt
            };

            // 7. Add the calculated velocity change to the Moon's current Velocity vector.
            moon.velocity.x = moon.velocity.x + velocityChange.x;
            moon.velocity.y = moon.velocity.y + velocityChange.y;
            moon.velocity.z = moon.velocity.z + velocityChange.z;
        }

        // --- PHYSICS UPDATE ---
        // Every frame before drawing, update the position of all physics bodies by integrating their velocity over one frame.
        // In linear algebra, Velocity is the rate of change of Position. By adding the components of the Velocity vector
        // to the components of the Position vector (scaled by dt), the object moves through the 3D space accurately over time.

        // Update Earth's Position (currently zero velocity, so it will not move)
        // X-axis update: Take the current X position and add the X velocity scaled by time to it.
        earth.position.x = earth.position.x + (earth.velocity.x * dt);
        // Y-axis update: Take the current Y position and add the Y velocity scaled by time to it.
        earth.position.y = earth.position.y + (earth.velocity.y * dt);
        // Z-axis update: Take the current Z position and add the Z velocity scaled by time to it.
        earth.position.z = earth.position.z + (earth.velocity.z * dt);

        // Update Moon's Position
        // X-axis update: Moves the moon left/right across the 3D grid, scaled by time.
        moon.position.x = moon.position.x + (moon.velocity.x * dt);
        // Y-axis update: Moves the moon up/down across the 3D grid, scaled by time.
        moon.position.y = moon.position.y + (moon.velocity.y * dt);
        // Z-axis update: Moves the moon forward/backward across the 3D grid, scaled by time.
        moon.position.z = moon.position.z + (moon.velocity.z * dt);

        // Begin the drawing phase for this frame. All rendering elements must be placed between BeginDrawing() and EndDrawing().
        BeginDrawing();

        // Clear the screen background to a dark color to represent the emptiness of space and reset the color buffer before drawing new frames.
        ClearBackground(BLACK);

        // Initialize the 3D drawing mode, passing our configured 3D camera to Raylib so it calculates the correct perspective matrices.
        BeginMode3D(camera);

        // Draw the Earth sphere using its Vector3 position in memory, its radius size, and its color.
        DrawSphere(earth.position, earth.radius, earth.color);

        // Draw the Moon sphere using its distinct Vector3 position, smaller radius, and gray color.
        DrawSphere(moon.position, moon.radius, moon.color);

        // End the 3D drawing mode to return to standard 2D rendering if necessary, finishing the 3D perspective projection.
        EndMode3D();

        // --- 2D UI RENDERING ---
        // UI text and controls MUST be drawn outside of the 3D camera mode (i.e., after EndMode3D()).
        // This is because the UI needs to be drawn in 2D screen space (pixels mapping directly to the window)
        // rather than 3D world space. If we drew the UI inside Mode3D, it would be affected by the camera's
        // perspective, position, and rotation, causing it to render as an object scattered in the 3D environment.
        // Drawing it here overlays the 2D elements seamlessly on top of our 3D world.
        
        // Draw the slider. We pass a pointer to earthMassSlider (&earthMassSlider) so raygui modifies it directly.
        GuiSlider(Rectangle{ 20, 10, 240, 40 }, "1.0", "20.0", &earthMassSlider, 1.0f, 20.0f);

        // End the drawing phase and swap the buffers to display the rendered image on the screen.
        EndDrawing();
    }

    // After the loop breaks, properly close the Raylib window to free allocated resources safely and tell the OS we are done here.
    CloseWindow();

    // End the main function, returning 0 to signal that the program executed successfully without any errors.
    return 0;
}
