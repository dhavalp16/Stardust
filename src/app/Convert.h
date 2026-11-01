#pragma once
#include "raylib.h"
#include "sim/Vec3.h"

// The simulation has its own vector type (sim::Vec3); raylib has Vector3.
// They hold the same three floats, so converting is just copying.
inline Vector3 ToRaylib(sim::Vec3 v) { return Vector3{v.x, v.y, v.z}; }
inline sim::Vec3 FromRaylib(Vector3 v) { return sim::Vec3{v.x, v.y, v.z}; }
