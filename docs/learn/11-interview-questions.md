# 11. Interview questions

Questions an interviewer for a graphics or engine programming role is likely to ask
about this project, with follow-ups and answers. The answers are short on purpose:
say them in your own words, and only claim what you can back up. Practise out loud.

Rule for every answer: **lead with the decision, then the reason, then the evidence.**
"I used a fixed timestep because the old loop's result depended on frame rate; I
measured a single 0.1 s frame knocking the Moon into Earth, and there's a regression
test for it now."

---

## Overview

**Q1. Walk me through the project in two minutes.**
> It's a real-time N-body gravity simulation in C++20 with raylib: the Sun, eight
> planets and the Moon, all pulling on each other. The physics is a separate library
> with no graphics dependency, tested with GoogleTest. It integrates with velocity
> Verlet at a fixed 600 steps per second, decoupled from the frame rate. The renderer
> loads glTF planets, lights them with a Phong shader from the Sun, and draws an HDR
> nebula sky through a custom loader. You can click planets and change their mass and
> radius live; collisions merge bodies while conserving momentum.

- *Follow-up: what are you proudest of?* Finding and proving the timestep bug:
  reproducing it headlessly, fixing it, and locking it down with a regression test.
- *Follow-up: what would you do next?* Barnes-Hut for thousands of bodies, render
  interpolation, linear-space lighting. (`docs/ROADMAP.md`)

**Q2. What did raylib do for you and what did you write?**
> raylib opened the window, read input, loaded models and textures, wrapped OpenGL and
> drew meshes and 2D shapes. I wrote the simulation (gravity, integrator, collisions,
> timestep), the HDR decoder, the camera controls, picking logic, HUD layout, the
> tests, the benchmark and the build.

- *Follow-up: so you never wrote raw OpenGL?* Correct, not in this project. The
  lighting shaders are adapted from raylib's lighting example, and the sky shaders are
  minimal; I can explain every line and the pipeline raylib drives (vertex and fragment
  shaders, uniforms, depth buffer, blending). *(Only say this once chapter 5 is solid
  for you.)*

## Physics and numerics

**Q3. How do you compute gravity? What's the complexity?**
> For every pair of bodies, `a = G·m·d / (|d|² + ε²)^1.5`. Each pair is visited once
> and both bodies get equal and opposite accelerations, Newton's third law. That's
> N(N−1)/2 pairs, so O(N²).

- *How does it scale in practice?* Measured: 10 bodies 0.36 µs per step, 100 bodies
  37 µs, 1000 bodies about 4 ms: ten times the bodies costs about a hundred times
  more.
- *How would you go to 100,000 bodies?* Barnes-Hut (octree, O(N log N)) or the fast
  multipole method; on the GPU, a compute shader with tiled shared-memory loads.
- *Why the ε?* Plummer softening: avoids infinite force when centres coincide and
  removes the discontinuity the old cutoff had.

**Q4. Which integrator and why?**
> Velocity Verlet, kick-drift-kick. It's second order and symplectic, so energy
> doesn't drift over long runs and orbits stay closed. I kept semi-implicit Euler as
> an option and compared: on a test orbit at 600 steps/s, Verlet ended 5.8e-4 from the
> exact answer versus 1.2e-3 for Euler, and at large steps its energy error was over
> ten times smaller.

- *What does symplectic mean?* It preserves the phase-space structure of Hamiltonian
  systems; practically, energy errors stay bounded and oscillate instead of growing.
- *Why not RK4?* Higher order per step but not symplectic: energy drifts over long
  runs; and four force evaluations per step.
- *How do you know it's second order?* A test halves the step and checks the error
  falls by 3.5 to 4.5 times.

**Q5. Tell me about the timestep bug.**
> Physics used the raw frame time, and the "ten sub-steps" kicked velocities ten
> times with positions frozen, so it was really one Euler step per frame. The orbit
> depended on refresh rate, and one long frame was one giant step. I replayed the
> original physics files headlessly: steady frame rates kept the Moon, a single 0.1 s
> frame lost it. The fix is the standard fixed-timestep accumulator with a 0.1 s clamp.

- *What's the spiral of death?* Slow frame → more steps → slower frame → even more
  steps. The clamp breaks it, at the cost of running slower than real time after a
  stall.
- *How do you test frame-rate independence?* Same simulated time delivered at 64 and
  128 frames per second gives bit-identical positions (power-of-two step sizes keep
  the accumulator exact).
- *Rendering between steps?* Not interpolated; at 600 Hz the lag is at most 1/600 s.
  With fewer steps I'd interpolate the last two states by the accumulator fraction.

**Q6. Why double precision? GPUs use float.**
> I measured it. With float positions, Verlet on a test orbit ended 1.4e-2 off and
> smaller steps didn't help, because adding a 0.017 move to a position of 10 loses
> digits every step. With double, 5.8e-4 and improving with smaller steps. Simulation
> state is double; I convert to float only to draw.

- *Cost?* 8 more bytes per vector, a conversion per drawn object. Irrelevant at 10
  bodies; at scale you'd also consider camera-relative rendering.

**Q7. How do collisions work? Are they physically correct?**
> Sphere overlap past 80% of the summed radii triggers a perfectly inelastic merge:
> mass adds, volume adds (cube root of summed cubes), the merged body keeps the pair's
> momentum and sits at their centre of mass. The Sun always survives.

- *What was wrong before?* Sun impacts dropped the planet's momentum, so total
  momentum wasn't conserved, and survivors kept their own position.
- *Is energy conserved?* No: inelastic collisions lose kinetic energy (it would become
  heat), so energy checks only compare between merges.
- *Can things tunnel through each other?* In principle, yes: it's checked once per step.
  At 600 steps/s the fastest planet moves 0.027 per step, much less than any radius.
  Continuous collision detection would solve it in general.

**Q8. How did you set up the initial conditions?**
> Circular-orbit speed `v = sqrt(G·M/r)` from balancing gravity with centripetal
> acceleration, perpendicular to the radius, plus the parent's velocity for the Moon.
> Then the Sun gets the velocity that zeroes total momentum, so the barycentre stays put.

- *Why is the Moon stable?* It's at about 38% of Earth's Hill radius, and prograde.
  Measured: over 600 s its distance stays between 0.667 and 1.132; merge is at 0.56.
- *Are the masses realistic?* No, deliberately: Earth 10, Jupiter 3, so a compact
  system stays watchable. The laws are real; the numbers are tuned.

**Q9. How do you know the simulation is correct?**
> Conservation laws. Total momentum and energy must stay constant without merges. A
> test runs the whole system for 300 simulated seconds: energy within 1e-6 relative,
> momentum within 1e-9, no merges. Plus unit tests of each law (inverse square,
> equal and opposite forces) and a comparison against an exact circular orbit.

## Graphics

**Q10. Describe the rendering of one frame.**
> Clear; 3D mode with the camera's view and projection; the sky sphere centred on the
> camera with culling and depth writes off; each planet with a model matrix from
> position, spin and radius, using a Phong shader with one point light at the Sun;
> debris cubes with alpha; then 2D: an additive glow for the Sun, the HUD with raygui;
> then present with vsync.

- *Why turn off depth writes for the sky?* So it never occludes anything drawn later;
  it's infinitely far away conceptually.
- *Why turn off back-face culling?* We see the sphere from inside.

**Q11. Explain the lighting shader.**
> It is adapted from raylib's lighting example: Lambert diffuse, `max(dot(N, L), 0)`,
> plus a Phong specular term with shininess 16, then a gamma power of 1/2.2. One point light at the Sun's position, updated each
> frame; the Sun itself uses an unlit shader.

- *Is the gamma handling right?* Not quite: it encodes at the end but never decodes the
  sRGB texture first, so colours wash out, and it applies the power to alpha. The fix
  is to decode textures to linear (or use sRGB textures) and encode once. It's on my
  roadmap; I left it because it changes the look.
- *Why does the night side look pure black?* No ambient term, no indirect light.

**Q12. Tell me about the HDR loader.**
> Radiance RGBE, mostly run-length encoded, 10000x5000. raylib's loader expands to
> 32-bit floats (about 600 MB), so I decode row by row and tone-map straight to 8-bit:
> exposure, Reinhard `x/(x+1)`, a gamma-and-contrast power. I made it 11× faster: a
> 64 KB lookup table for all 65536 (mantissa, exponent) pairs instead of 150 million
> `pow` calls, and parsing from memory instead of `fgetc` per byte. 3.4 s to 0.3 s,
> byte-identical output, checked by a test over every table entry.

- *Why was fgetc slow?* Per-call overhead including locking in the Windows C runtime;
  one big `fread` and pointer walking avoids it.
- *What's tone mapping?* Mapping unbounded scene brightness into the displayable range.

**Q13. How does clicking a planet work?**
> Unproject the mouse into a ray from the camera, test it against each planet's
> bounding sphere, keep the nearest hit. Clicks on HUD panels are filtered first using
> the same rectangles the HUD draws with; that fixed a strip where clicks went through.

## Architecture and engineering

**Q14. How is the code structured, and why?**
> Three layers: a physics library and an HDR decoder that only use the standard
> library, and the raylib app on top. The libraries are tested and benchmarked without
> a window; CI builds them on Linux with no raylib to enforce the boundary. Physics
> reports merges as events; the app reacts.

- *Downsides?* Two vector types and a conversion; index-paired arrays mean you must
  never erase one without the others, so dead bodies are flagged, not erased.

**Q15. What's your testing strategy?**
> Unit tests of each physical law, conservation tests on the real scenario, a
> regression test for the stall bug, exhaustive tests where the input space is small
> (the tone table), screenshot comparisons for rendering, and by-eye checks for
> interaction.

- *What isn't tested automatically?* Input handling and the HUD; I have a written
  manual checklist for those.

**Q16. Tell me about the build and CI.**
> CMake with presets; raylib and GoogleTest fetched at pinned versions with SHA-256
> hashes; strict warnings on our code only; CI on Windows with MSVC and on Linux with
> GCC, warnings as errors, plus a problem matcher so MSVC warnings show up on pull
> requests.

- *Did CI ever catch something?* Yes: MSVC flagged an implicit double-to-float
  conversion that clang's default warnings missed.

**Q17. How do you measure performance?**
> Release builds, several runs, median or best, state the machine and method, never
> quote a number I didn't run. All numbers come from one benchmark program and are
> written up with their method.

**Q18. What was the hardest bug?**
> Pick one you can tell with a beginning, middle and end. Good candidates: the timestep
> bug (Q5), float precision masking the integrator (Q6), or the screenshot tool missing
> the HUD because 2D drawing was batched.

## C++

**Q19. Why did you replace the selected-planet pointer with an index?**
> A pointer into a `std::vector` dangles when the vector reallocates. It only worked
> because nothing pushed to it. An index stays valid as long as elements aren't erased,
> which is also why dead bodies are flagged instead.

**Q20. Where do you use RAII?**
> The HDR loader closes its file through a `std::unique_ptr` with a custom deleter, so
> every early return is safe. raylib resources are still freed by hand; wrapping them
> is an exercise I'd do next.

**Q21. What does `const` buy you in this code?**
> Intent and safety: `const std::vector<Body>&` lets gravity read bodies without copying
> and without being able to change them; const methods like `StepSeconds()` promise not
> to change the object.

**Q22. Why `static_cast` instead of C-style casts?**
> It only allows well-defined conversions and is easy to search for; C-style casts can
> silently do reinterpretations.

## Questions to expect about you

**Q23. Did you write all of this yourself?** Answer honestly. The original version was
yours; the overhaul was done with an AI assistant overnight, and you then studied
every change until you could explain and rebuild it. Say that plainly and then prove it
by explaining any part in depth. Interviewers respect honesty and test understanding;
they do not forgive being caught overclaiming. See `docs/RESUME_CLAIMS.md`.

**Q24. What would you do differently if you started again?** Separate simulation and
rendering from day one; fixed timestep from day one; tests before features; one
branch for the product line.
