#pragma once
#include <cmath>

namespace sim {

// A 3D vector: a position, a velocity or an acceleration.
//
// The simulation uses its own small vector type instead of raylib's Vector3 so
// that it does not depend on raylib at all. That is what lets the tests run
// without opening a window. The app converts to raylib's type when drawing
// (see ToRaylib() in src/app/Convert.h).
//
// Components are double precision. A float has about 7 significant digits; a
// planet 10 units out moving 0.017 units per step loses its last digits to
// rounding on every step, and over thousands of steps that rounding swamped the
// integrator's own error (measured in docs/MEASUREMENTS.md). Double has about
// 16 digits. Rendering still uses float, which is plenty for drawing.
struct Vec3 {
  double x = 0.0;
  double y = 0.0;
  double z = 0.0;
};

// Operator overloading lets the physics read like the maths: a + b * dt.
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline Vec3 operator-(Vec3 a) { return {-a.x, -a.y, -a.z}; }
inline Vec3 operator*(Vec3 a, double s) { return {a.x * s, a.y * s, a.z * s}; }
inline Vec3 operator*(double s, Vec3 a) { return a * s; }
inline Vec3 operator/(Vec3 a, double s) { return {a.x / s, a.y / s, a.z / s}; }

inline Vec3 &operator+=(Vec3 &a, Vec3 b) { return a = a + b; }
inline Vec3 &operator-=(Vec3 &a, Vec3 b) { return a = a - b; }
inline Vec3 &operator*=(Vec3 &a, double s) { return a = a * s; }

inline double Dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline double LengthSquared(Vec3 a) { return Dot(a, a); }
inline double Length(Vec3 a) { return std::sqrt(LengthSquared(a)); }

} // namespace sim
