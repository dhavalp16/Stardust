#define _CRT_SECURE_NO_WARNINGS
#include "Physics.h"
#include <cmath>

// Newton's Law of Universal Gravitation
void ApplyGravity(Planet &attractor, float attractorMass, Planet &target,
                  float G, float dt) {
  // Direction vector from target to attractor
  Vector3 direction = Vector3{attractor.position.x - target.position.x,
                              attractor.position.y - target.position.y,
                              attractor.position.z - target.position.z};

  // Squared distance for optimization
  float distanceSquared = (direction.x * direction.x) +
                          (direction.y * direction.y) +
                          (direction.z * direction.z);

  // Singularity guard
  if (distanceSquared > 0.0001f) {
    float distance = std::sqrt(distanceSquared);

    // Normalize direction
    Vector3 normalizedDirection = Vector3{
        direction.x / distance, direction.y / distance, direction.z / distance};

    // Calculate gravitational force magnitude
    float force = G * (attractorMass * target.mass) / distanceSquared;

    // Apply acceleration to velocity (F = ma)
    float acceleration = force / target.mass;

    Vector3 velocityChange = Vector3{normalizedDirection.x * acceleration * dt,
                                     normalizedDirection.y * acceleration * dt,
                                     normalizedDirection.z * acceleration * dt};

    target.velocity.x += velocityChange.x;
    target.velocity.y += velocityChange.y;
    target.velocity.z += velocityChange.z;
  }
}

// Euler Integration
void UpdatePosition(Planet &body, float dt) {
  body.position.x += body.velocity.x * dt;
  body.position.y += body.velocity.y * dt;
  body.position.z += body.velocity.z * dt;
}
