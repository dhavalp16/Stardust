#pragma once
#include "sim/Body.h"
#include <cstddef>
#include <vector>

namespace sim {

struct CollisionParams {
  // Two bodies merge when their centres are closer than
  // overlapFactor * (radius1 + radius2). Below 1.0 means they must overlap a bit
  // first, which forgives grazing passes (the Moon's orbit relies on this).
  float overlapFactor = 0.8f;
  // This body always survives a merge, whatever its mass (the Sun).
  size_t anchorIndex = 0;
};

// What happened in one merge, so the app can react (debris, clear selection).
// The victim's state is recorded as it was just before the merge.
struct MergeEvent {
  size_t survivor = 0;
  size_t victim = 0;
  Vec3 victimPosition;
  Vec3 victimVelocity;
  float victimRadius = 0.0f;
};

// Finds overlapping pairs of live bodies and merges each pair into one body.
// The survivor is the anchor if involved, otherwise the heavier body (the
// lower index wins a tie). The victim is flagged dead. Returns every merge.
std::vector<MergeEvent> ResolveCollisions(std::vector<Body> &bodies,
                                          const CollisionParams &params);

} // namespace sim
