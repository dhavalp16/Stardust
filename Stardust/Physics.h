#pragma once
#include "Planet.h"

// Gravitational force application
void ApplyGravity(Planet &attractor, float attractorMass, Planet &target,
                  float G, float dt);

// Kinematic position update
void UpdatePosition(Planet &body, float dt);
