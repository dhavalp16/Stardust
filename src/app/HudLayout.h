#pragma once
#include "raylib.h"

// Where the HUD panels sit on screen. HUD.cpp draws them here and Input.cpp
// ignores clicks inside them, so both must use the same numbers. Keeping them
// in one place means they cannot drift apart (they had: the planet panel was
// drawn down to y = 230 but clicks were only blocked above y = 220).

// Top-left panel: tall when a planet is selected (name, stats, sliders,
// button), short when not ("No planet selected").
inline Rectangle PlanetPanelRect(bool hasSelection) {
  return hasSelection ? Rectangle{10, 10, 310, 220} : Rectangle{10, 10, 310, 55};
}

// Top-right panel: FPS, planet count, play state.
inline Rectangle StatsPanelRect(int screenWidth) {
  return Rectangle{static_cast<float>(screenWidth) - 260.0f, 10, 250, 110};
}

// True if a click at `point` lands on a HUD panel rather than on space.
inline bool IsOverHud(Vector2 point, bool hasSelection, int screenWidth) {
  return CheckCollisionPointRec(point, PlanetPanelRect(hasSelection)) ||
         CheckCollisionPointRec(point, StatsPanelRect(screenWidth));
}
