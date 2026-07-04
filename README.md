# Stardust

A real-time 3D N-body gravity simulator written in C++ on Raylib. Ten bodies (the Sun, the eight planets, and the Moon), each pulling on every other one every frame. Nothing is on rails: the orbits you see fall out of Newton's law and the starting conditions, and you can break them by clicking a planet and dragging its mass slider.

I built this as a from-scratch physics and rendering project. Raylib handles the window, input, and model loading; the simulation, collision rules, camera, and the HDR skybox loader are mine.

## Which branch do you want?

This repo carries two versions of the engine on separate branches, because the desktop and Android builds ended up with incompatible dependencies.

| Branch | What it is |
|---|---|
| `release/sd-winApp` | The Windows desktop app. Most polished version, start here. |
| `desktop` | Working branch behind `release/sd-winApp`. Same content right now. |
| `main` / `release/sd-androidApp` | The mobile build: touch controls, a JNI bridge, OpenGL ES shaders. Currently identical to each other. |

The desktop branches have things the mobile line doesn't (HDR nebula skybox, sun bloom, tracking camera, a key-press overlay for recordings). The mobile line has things desktop doesn't (virtual joystick controls, a kill-feed, and a JNI bridge that feeds collision events out to a host Android app and receives AI-narration captions back). That host app is part of StellAR, a separate project; this repo only contains the C++ half, so you won't get an APK out of it on its own.

Everything below describes the desktop build unless it says otherwise.

## What it does

- Full pairwise N-body gravity. Every body attracts every other body; the Moon orbits Earth because Earth is heavy and close, not because I told it to.
- Click a planet to select it, then edit its mass and radius with sliders while the simulation runs. The sliders write straight into the physics state, so consequences are immediate.
- Planets that overlap deep enough merge. Momentum is conserved, mass adds, radius grows by volume (cube root of summed cubes), and the loser bursts into fragments that fade out.
- Free camera in the Unreal style: hold right mouse to look, WASD/QE to fly, scroll to change speed. Movement speed scales with distance to the nearest planet so you can cross the system fast and still creep along a surface.
- A tracking mode that smoothly chases the selected planet, with scroll-to-zoom.
- HDR nebula skybox. The source file is a 93 MB Radiance panorama that Raylib's built-in loader chokes on, so there's a custom streaming loader (`hdr_loader.h`) that decodes the RLE format scanline by scanline and tone-maps inline.
- Screen-space sun bloom, a selection reticle, an FPS/state panel, and a little overlay that shows which keys are being pressed (I added that for recording clips).

## Building (desktop)

1. Check out `release/sd-winApp`.
2. Open `Stardust.slnx` in a recent Visual Studio.
3. Restore NuGet packages. Raylib 5.5.0 comes down automatically.
4. Build and run (x64). The project is set to C++20.

Assets load by relative path, so the working directory needs to be the `Stardust/` project folder. Visual Studio's debugger does this by default. If the sky comes up as a magenta checkerboard, your working directory is wrong; that pattern is the deliberate "asset didn't load" signal.

Fair warning: the clone is heavy because the 93 MB skybox lives in git history. Moving it to LFS is on the list.

## Controls (desktop)

| Input | Action |
|---|---|
| Space | Play / pause (it starts paused) |
| RMB hold | Free look |
| RMB + WASD / QE | Fly |
| RMB + scroll | Camera speed |
| Scroll | Zoom (or orbit distance while tracking) |
| LMB | Select planet / click empty space to deselect |
| R | Reset to the initial system |

## How the physics actually works

Each frame, while playing:

1. For every pair of live bodies, apply Newton's gravity to both velocities: `F = G·m1·m2/r²`, with a small squared-distance guard so nothing divides by zero. This runs in 10 sub-steps of `dt/10`, with collision checks inside the loop.
2. After the loop, positions integrate once with the full `dt`, using the already-updated velocities.

Because velocities update before positions, this is semi-implicit Euler rather than plain explicit Euler. That ordering is the reason the orbits stay closed instead of spiraling outward; it's a known property of the symplectic variant and it costs nothing.

One thing I'll own here rather than let you find it: with the position update outside the sub-step loop, positions are frozen during the ten passes, so the sub-stepping mostly re-applies the same velocity kick in slices. The thing that actually keeps the Moon from getting eaten during close passes is the collision threshold, which requires 20% overlap of the combined radii (a 0.8 factor) before a merge fires. Moving position integration inside the sub-step loop is the top item on the roadmap.

Collision rules: heavier body survives, except the Sun, which always survives and never inherits impact momentum, because one dramatic Jupiter impact shouldn't punt the anchor of the whole system. Everything else merges inelastically with momentum conserved.

## Where the starting numbers come from

None of the initial conditions are eyeballed:

- Every planet starts at exactly circular-orbit speed, `v = sqrt(G·M/r)` with `G = 1` and a Sun mass of 2000. Earth at radius 20 moves at exactly 10.
- The Sun starts with a small velocity that cancels the planets' total momentum, so the barycenter stays put and the system never drifts off screen.
- The Moon sits 0.9 units from Earth, about 38% of Earth's Hill radius, moving at circular speed relative to Earth on top of Earth's own velocity.

The masses themselves are not realistic, on purpose. Earth is 10 and Jupiter is 3; real Jupiter is ~318 Earths and would wreck a system this compressed, and a realistically light Earth couldn't hold the Moon this close to the Sun. The structure is real physics, the values are tuned for a system that stays watchable.

## Known quirks and honest gaps

- `dt` isn't clamped, so anything that stalls the frame (dragging the window, a breakpoint) delivers one big impulse and can visibly kick orbits.
- The point light sits at the world origin while the Sun's body wobbles slightly around the barycenter. Off by hundredths of a unit, invisible, technically wrong.
- The Sun looks self-lit because its material gets swapped to an unlit shader for its draw call. There's no shadowing anywhere, so planets don't eclipse each other.
- Collision is sphere-vs-sphere only.
- `main.cpp` references `assets/ambient_space.mp3`, which I never committed. The app runs silent on a fresh clone and Raylib just logs a warning. Drop any mp3 with that name in `assets/` if you want ambience.
- `pluto.glb` ships in assets but never gets spawned. Poor Pluto, twice now.
- The desktop window is fixed at 1920x1080; the HUD layout assumes it.
- Venus and Uranus spin backwards. That one's not a bug, they really do.
- Merged planets aren't removed from the array, they're flagged dead and skipped. That's deliberate: the planet list is index-paired with the loaded models, and the selection is a pointer into the vector, so erasing would corrupt both.

## Roadmap, roughly in order

- Clamp `dt`, then a proper fixed-timestep loop.
- Position integration inside the sub-step loop (see above).
- Selection by index instead of a raw pointer.
- Git LFS for the skybox.
- Orbit trails, and the summary screen the unused `SUMMARY` state was reserved for.
- Barnes-Hut if I ever push the body count into the thousands. At 10 bodies the O(N²) loop is 900 force evaluations a frame and costs nothing.

## Why "Stardust"?

Because that's what planets turn into when they collide here. Also it sounded good.

## License

Educational code, take what you want from it. If it teaches you something or you spot a bug, I'd like to hear about it.
