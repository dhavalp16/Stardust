// Include the Raylib library header file to access all its functions, such as window creation and 3D drawing.
#include "raylib.h"

// Define a structure to represent a Planet in our 3D space.
struct Planet {
    // Vector3 is a struct containing three floats (x, y, z) representing coordinates in 3D space. Memory-wise, it's 12 contiguous bytes (3 * 4 bytes).
    Vector3 position;
    // A single precision floating-point number (4 bytes) representing the physical size (radius) of the planet.
    float radius;
    // A single precision floating-point number (4 bytes) representing the mass of the planet (unused for now but ready for physics).
    float mass;
    // A Raylib Color struct containing four unsigned bytes (r, g, b, a) representing red, green, blue, and alpha channels.
    Color color;
};

// The main function where our C++ program begins execution.
int main() {
    // Initialize the window with a width of 800 pixels.
    const int screenWidth = 800;
    // Initialize the window with a height of 600 pixels.
    const int screenHeight = 600;

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
    // Assign a LIGHTGRAY color to the Moon using Raylib's predefined color constants.
    moon.color = LIGHTGRAY;

    // Set the target frame rate to 60 frames per second so the window updates at a consistent, smooth speed.
    SetTargetFPS(60);

    // Enter a continuous while-loop that will run until the user presses the ESC key or closes the window via the cross button.
    while (!WindowShouldClose()) {
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

        // End the drawing phase and swap the buffers to display the rendered image on the screen.
        EndDrawing();
    }

    // After the loop breaks, properly close the Raylib window to free allocated resources safely and tell the OS we are done here.
    CloseWindow();

    // End the main function, returning 0 to signal that the program executed successfully without any errors.
    return 0;
}
