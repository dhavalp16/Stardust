#pragma once
// Include our custom Planet structure definition so we know what a 'Planet' is.
#include "Planet.h"

// Define our physics functions here so they can be called from main.cpp.

// This function calculates and applies the gravitational pull from an
// 'attractor' planet onto a 'target' planet. We use references (the '&'
// ampersand symbol) for the Planet parameters. This is crucial in C++. By
// passing by reference (e.g., Planet& target), we are passing the actual
// original object in memory, not a copy. Any changes made inside this function
// (like updating the target's velocity) will modify the actual planet back in
// main.cpp.
void ApplyGravity(Planet &attractor, float attractorMass, Planet &target,
                  float G, float dt);

// This function updates the position of a planet based on its current velocity
// and how much time has passed. Again, we use a reference (Planet& body) so we
// update the actual planet's position. 'dt' is Delta Time: the amount of time
// in seconds that has passed since the last frame.
void UpdatePosition(Planet &body, float dt);
