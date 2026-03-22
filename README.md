# Stardust

A real-time N-body physics simulator built with C++ and Raylib. Watch planets orbit, collide, and interact under Newton's law of gravitation.

## What is this?

Stardust simulates a miniature solar system where every celestial body gravitationally pulls on every other body. You can click on planets to adjust their mass and size, then watch the chaos unfold as orbits destabilize or - if you're lucky - form beautiful resonance patterns.

The physics runs at 180 FPS with 10 sub-steps per frame to keep orbits stable. The simulation includes our solar system's 8 planets plus the Moon, each with a detailed 3D model.

## Features

- **N-body gravity simulation** - Every planet affects every other planet
- **Interactive planet editing** - Click any planet to adjust its mass and radius with sliders
- **Real-time collisions** - Planets that get too close explode in a satisfying wireframe burst
- **Free camera** - Fly around the solar system with WASD controls
- **Play/pause state** - Spacebar to freeze time and examine the current configuration
- **Reset button** - Restore the solar system to its initial stable state
- **Mobile support** - PUBG-style touch controls with virtual joystick
- **Toast notifications** - See real-world mass equivalents (e.g., "2.5 Earth masses")

## Controls

### Desktop
- **Right mouse + WASD** - Free camera movement
- **Mouse scroll** - Adjust camera speed
- **Left click** - Select a planet
- **Spacebar** - Toggle play/pause
- **R key** - Reset simulation

### Mobile
- **Left joystick** - Move camera
- **Right touch area** - Look around
- **Tap planet** - Select it

## Building

Requires Visual Studio 2022 (or later) with C++20 support.

1. Open `Stardust.slnx` in Visual Studio
2. Restore NuGet packages (Raylib 5.5.0 will auto-install)
3. Build and run

The project uses Raylib for rendering and input. All dependencies are managed through NuGet.

## Project Structure

```
Stardust/
├── main.cpp           - Main simulation loop and UI
├── Planet.h           - Planet structure definition
├── Physics.cpp/h      - Gravity calculations and position updates
├── rlights.h          - Raylib lighting system
├── raygui.h           - Immediate-mode GUI library
├── assets/            - 3D models (.glb format)
└── resources/shaders/ - GLSL lighting shaders
```

## How the physics works

The simulation uses **Euler integration** with velocity Verlet-style sub-stepping. Each frame:

1. Apply gravitational forces between all planet pairs (N² algorithm)
2. Update velocities based on acceleration
3. Update positions based on velocity
4. Check for volumetric collisions
5. Tombstone collided planets and spawn explosion effects

Momentum is conserved by applying Newton's third law symmetrically. Dead planets stay in the array (marked `isAlive = false`) to prevent desynchronization between the physics state and the parallel GPU model array.

## Why is it called Stardust?

Because when planets collide they turn into... well, stardust. Also it sounds cool.

## Performance

Runs at 180 FPS on modern hardware (tested on GTX 1060+). The O(N²) gravity calculation scales poorly beyond ~100 bodies, but for our 9-planet solar system it's negligible.

## Known quirks

- Jupiter and Saturn have reduced masses (3× and 1.5× Earth instead of their real 318× and 95×) to prevent them from eating the inner planets
- Venus and Uranus rotate backwards (retrograde) like they do IRL
- The Moon orbits Earth at 38% of the Hill sphere radius for stability
- Collision detection is purely volumetric (sphere-sphere) - no fancy mesh collision

## Future ideas

- Add comet trails
- Implement a summary screen showing orbital periods and eccentricities
- Barnes-Hut tree for O(N log N) gravity (if I ever simulate 1000+ bodies)
- Variable timestep for better energy conservation
- Roche limit calculations for tidal breakup

## License

Do whatever you want with this. It's educational code with way too many comments.
