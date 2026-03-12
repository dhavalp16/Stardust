#pragma once
#include "raylib.h"
// Include the C++ Standard Library string class so we can give each planet a
// human-readable name (e.g., "Earth", "Moon"). std::string manages its own
// memory automatically — it grows and shrinks as needed, unlike C-style char
// arrays which require manual size management.
#include <string>

// Define a structure to represent a Planet in our 3D space.
struct Planet {
  // A human-readable name for this planet (e.g., "Earth", "Moon", "Mars").
  // Used by the HUD to display which planet is selected. By storing the name
  // INSIDE the struct, we avoid hardcoded pointer comparisons like
  // 'selectedPlanet == &earth'. Instead, we just read selectedPlanet->name.
  std::string name;
  // Vector3 is a struct containing three floats (x, y, z) representing
  // coordinates in 3D space. Memory-wise, it's 12 contiguous bytes (3 * 4
  // bytes).
  Vector3 position;
  // A single precision floating-point number (4 bytes) representing the
  // physical size (radius) of the planet.
  float radius;
  // A single precision floating-point number (4 bytes) representing the mass of
  // the planet (unused for now but ready for physics).
  float mass;
  // A Vector3 representing the velocity currently experienced by the planet.
  // Contains floats for speed along X, Y, and Z axes.
  Vector3 velocity;
  // A Raylib Color struct containing four unsigned bytes (r, g, b, a)
  // representing red, green, blue, and alpha channels.
  Color color;
};
