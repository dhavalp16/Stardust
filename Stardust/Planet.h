#pragma once
#include "raylib.h"
#include <string>

// Celestial body data structure
struct Planet {
  Vector3 position;
  Vector3 velocity;
  float mass;
  float radius;
  std::string modelPath;
  Color tint;
  float rotationAngle;
  float rotationSpeed;
  bool isAlive;
  std::string name;
  float massMin, massMax;
  float radiusMin, radiusMax;

  Planet(Vector3 pos, Vector3 vel, float m, float r, std::string path, Color c,
         float rotSpeed, std::string n,
         float mMin, float mMax, float rMin, float rMax)
      : position(pos), velocity(vel), mass(m), radius(r), modelPath(path), tint(c),
        rotationAngle(0.0f), rotationSpeed(rotSpeed), isAlive(true),
        name(n), massMin(mMin), massMax(mMax), radiusMin(rMin), radiusMax(rMax) {}
};
