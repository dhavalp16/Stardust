#pragma once
#include "raylib.h"
#include "sim/SolarSystem.h"
#include <string>
#include <vector>

// Everything the app knows about a planet that the physics does not need.
// planetInfo[i] describes the same planet as bodies[i] and planetModels[i].
struct PlanetInfo {
  std::string name;
  std::string modelPath;
  Color tint;
  float spinRate;          // radians per second
  float spinAngle = 0.0f;  // degrees, for DrawModelEx
  float massMin, massMax;  // slider ranges; the max grows if a merge exceeds it
  float radiusMin, radiusMax;
};

inline std::vector<PlanetInfo> MakePlanetInfo(const std::vector<sim::PlanetSpec> &specs) {
  std::vector<PlanetInfo> info;
  for (const sim::PlanetSpec &s : specs) {
    info.push_back({s.name, s.modelPath, Color{s.tint[0], s.tint[1], s.tint[2], s.tint[3]},
                    s.spinRate, 0.0f, s.massMin, s.massMax, s.radiusMin, s.radiusMax});
  }
  return info;
}
