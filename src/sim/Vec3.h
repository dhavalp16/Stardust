#pragma once
#include <cmath>

namespace sim {

// A 3D vector: a position, a velocity or an acceleration.
//
// The simulation uses its own small vector type instead of raylib's Vector3 so
// that it does not depend on raylib at all. That is what lets the tests run
// without opening a window. The app converts to raylib's type when drawing
// (see ToRaylib() in src/app/Convert.h).
struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
};

// Operator overloading lets the physics read like the maths: a + b * dt.
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(Vec3 a) { return {-a.x, -a.y, -a.z}; }
inline Vec3 operator*(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3 operator*(float s, Vec3 a) { return a * s; }
inline Vec3 operator/(Vec3 a, float s) { return {a.x / s, a.y / s, a.z / s}; }

inline Vec3 &operator+=(Vec3 &a, Vec3 b) { return a = a + b; }
inline Vec3 &operator-=(Vec3 &a, Vec3 b) { return a = a - b; }
inline Vec3 &operator*=(Vec3 &a, float s) { return a = a * s; }

inline float Dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline float LengthSquared(Vec3 a) { return Dot(a, a); }
inline float Length(Vec3 a) { return std::sqrt(LengthSquared(a)); }

} // namespace sim
