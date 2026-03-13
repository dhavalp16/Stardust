#pragma once
#include "raylib.h"
// Include the C++ Standard Library string class so we can give each planet a
// human-readable name (e.g., "Earth", "Moon"). std::string manages its own
// memory automatically — it grows and shrinks as needed, unlike C-style char
// arrays which require manual size management.
#include <string>

// Define a structure to represent a Planet in our 3D space.
struct Planet {
  // Vector3 is a struct containing three floats (x, y, z) representing
  // coordinates in 3D space. Memory-wise, it's 12 contiguous bytes (3 * 4
  // bytes).
  Vector3 position;
  // A Vector3 representing the velocity currently experienced by the planet.
  // Contains floats for speed along X, Y, and Z axes.
  Vector3 velocity;
  // A single precision floating-point number (4 bytes) representing the mass of
  // the planet.
  float mass;
  // A single precision floating-point number (4 bytes) representing the
  // physical size (radius) of the planet.
  float radius;
  // Path to the 3D model asset.
  std::string modelPath;
  // A Raylib Color struct for tinting the model.
  Color tint;
  // Variables for dynamic axial rotation.
  float rotationAngle;
  float rotationSpeed;
  // Lifecycle variable.
  bool isAlive;
  // Human-readable display name for the HUD (e.g., "Earth", "Moon").
  std::string name;
  // Per-planet slider ranges — each body gets its own safe min/max to prevent
  // accidentally setting the Sun's mass to 0.1 or the Moon's radius to 5.0,
  // which would instantly destabilize the simulation.
  float massMin, massMax;
  float radiusMin, radiusMax;

  // EXPLANATION: How the C++ Constructor makes struct initialization cleaner.
  // A constructor allows us to initialize a Planet object in a single line
  // with all required data, rather than declaring it and then setting each member 
  // line-by-line (e.g., p.mass = ..., p.radius = ...). The colon syntax here 
  // is known as a "member initializer list", which directly constructs the members 
  // in memory, making it very clean and efficient.
  Planet(Vector3 pos, Vector3 vel, float m, float r, std::string path, Color c,
         float rotSpeed, std::string n,
         float mMin, float mMax, float rMin, float rMax)
      : position(pos), velocity(vel), mass(m), radius(r), modelPath(path), tint(c),
        rotationAngle(0.0f), rotationSpeed(rotSpeed), isAlive(true),
        name(n), massMin(mMin), massMax(mMax), radiusMin(rMin), radiusMax(rMax) {}
};
