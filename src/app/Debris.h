#pragma once
#include "raylib.h"
#include "sim/Collision.h"
#include <vector>

// Short-lived cubes that burst out of a merged planet. Purely visual: they do
// not feel or cause gravity.
struct Fragment {
  Vector3 position;
  Vector3 velocity;
  float size;
  float life; // 1.0 when spawned, fades to 0.0
  Color color;
  bool isAlive;
};

// Adds 30-50 fragments flying out of the victim of a merge.
void SpawnDebris(const sim::MergeEvent &event, Color color, std::vector<Fragment> &fragments);
