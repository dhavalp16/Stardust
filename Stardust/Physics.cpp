#define _CRT_SECURE_NO_WARNINGS
// Include our physics header file which contains the declarations of the
// functions we are implementing here.
#include "Physics.h"
// Include the standard C++ math library. We need this specifically for the
// std::sqrt (square root) function used in the distance calculation.
#include <cmath>

// Implement the ApplyGravity function declared in Physics.h.
// This applies Newton's Law of Universal Gravitation.
void ApplyGravity(Planet &attractor, float attractorMass, Planet &target,
                  float G, float dt) {
  // 1. Calculate the distance vector (direction) from the target to the
  // attractor. In 3D space, subtracting point A's coordinates from point B's
  // coordinates gives you a vector pointing from A to B. Here, we subtract the
  // target's position from the attractor's position to get a vector pointing TO
  // the attractor.
  Vector3 direction = Vector3{attractor.position.x - target.position.x,
                              attractor.position.y - target.position.y,
                              attractor.position.z - target.position.z};

  // 2. Calculate the squared distance (the length squared of the direction
  // vector). We use the Pythagorean theorem extended to 3D: a^2 + b^2 + c^2 =
  // d^2. We calculate distance squared first because square roots are
  // computationally expensive. If objects are too close, we can skip the square
  // root entirely to save performance.
  float distanceSquared = (direction.x * direction.x) +
                          (direction.y * direction.y) +
                          (direction.z * direction.z);

  // We avoid dividing by zero or calculating gravity when objects perfectly
  // overlap (distance is practically 0). If distanceSquared is too small,
  // gravity approaches infinity, which breaks the simulation.
  if (distanceSquared > 0.0001f) {
    // Calculate the actual scalar distance by taking the square root.
    float distance = std::sqrt(distanceSquared);

    // 3. Normalize the direction vector so its length is exactly 1.
    // A normalized vector represents pure direction without magnitude (size).
    // By dividing each X/Y/Z component by the total length (distance), we scale
    // the vector down to a length of 1.
    Vector3 normalizedDirection = Vector3{
        direction.x / distance, direction.y / distance, direction.z / distance};

    // 4. Calculate the magnitude of the gravitational pull using Newton's Law
    // of Universal Gravitation: F = G * (m1 * m2) / r^2 G is the gravitational
    // constant (passed in, scaled for our game). The force increases with mass
    // and decreases exponentially with the square of the distance. We use the
    // attractorMass parameter instead of attractor.mass so we can easily swap
    // in dynamic values (like from a UI slider).
    float force = G * (attractorMass * target.mass) / distanceSquared;

    // 5. Apply the force as Acceleration to the target's Velocity.
    // Newton's Second Law: Force = mass * acceleration (F = ma)
    // Rearranged to solve for acceleration: Acceleration = Force / mass.
    // We divide by the target's mass to see how much this specific object
    // accelerates.
    float acceleration = force / target.mass;

    // 6. Calculate the actual change in velocity for this frame.
    // We multiply our pure 3D direction vector by the scalar acceleration
    // magnitude. Crucially, we also multiply by 'dt' (Delta Time). Acceleration
    // is "change in velocity per second". Delta time is "fraction of a second
    // that passed". Multiplying them gives the exact change in velocity for
    // this specific drawing frame.
    Vector3 velocityChange = Vector3{normalizedDirection.x * acceleration * dt,
                                     normalizedDirection.y * acceleration * dt,
                                     normalizedDirection.z * acceleration * dt};

    // 7. Add the calculated velocity change to the target's current Velocity
    // vector. Because 'target' was passed by reference (Planet&), modifying
    // 'target.velocity' here directly updates the original planet object living
    // back in main.cpp.
    target.velocity.x = target.velocity.x + velocityChange.x;
    target.velocity.y = target.velocity.y + velocityChange.y;
    target.velocity.z = target.velocity.z + velocityChange.z;
  }
}

// Implement the UpdatePosition function declared in Physics.h.
// This handles basic kinematics: applying velocity to change position over
// time. We use a simple method called 'Euler Integration'.
void UpdatePosition(Planet &body, float dt) {
  // Velocity is the rate of change of Position (e.g., meters per second).
  // To find out how far an object moved this frame, we multiply its velocity
  // (per second) by the fraction of a second that actually passed (dt).
  //
  // For example: if velocity.x is 10 units/sec, and the frame took 0.1 seconds
  // (dt), the object moved 10 * 0.1 = 1 unit along the X axis this frame.
  //
  // We do this for all three axes (X, Y, Z).
  // Because 'body' is a reference (Planet&), updating positions here changes
  // the original object.

  // X-axis update: Add velocity scaled by time to current X position.
  body.position.x = body.position.x + (body.velocity.x * dt);

  // Y-axis update: Add velocity scaled by time to current Y position.
  body.position.y = body.position.y + (body.velocity.y * dt);

  // Z-axis update: Add velocity scaled by time to current Z position.
  body.position.z = body.position.z + (body.velocity.z * dt);
}
