# 3. The physics

This chapter explains the physics Stardust simulates: what gravity does, why
planets orbit, what is conserved, and how collisions work. Chapter 4 then explains
how a computer actually steps it forward in time.

## 3.1 Position, velocity, acceleration

- **Position** `x`: where something is.
- **Velocity** `v`: how fast the position changes, and in which direction. Units
  of distance per second. If `v` stays constant, after time `t` you have moved `v·t`.
- **Acceleration** `a`: how fast the velocity changes. A planet in a circular
  orbit has constant *speed* but its *direction* keeps turning, so it is
  accelerating all the time, towards the centre.

## 3.2 Newton's laws, the parts we use

1. Without a force, velocity does not change.
2. **F = m·a**: a force F on a body of **mass** m produces acceleration `a = F / m`.
   Heavy things are harder to push.
3. Forces come in equal and opposite pairs: if A pulls B, B pulls A just as hard
   the other way.

## 3.3 Gravity

Newton's **law of universal gravitation**: two masses attract each other with force

```
F = G · m1 · m2 / r²
```

along the line between them. `G` is the **gravitational constant**, `r` the distance
between their centres. Doubling the distance quarters the force: an
**inverse-square law** (tested in `Gravity.DoublingTheDistanceQuartersTheAcceleration`).

The acceleration of body 1 caused by body 2 is `F / m1 = G · m2 / r²`. Body 1's
own mass cancels out: a feather and a hammer fall the same way. So the code computes
accelerations directly and never forms the force.

To make it a vector, multiply by the unit direction `d / r` (from body 1 to body 2):

```
a1 = G · m2 · d / r³
```

(one `1/r` for the unit direction, `1/r²` for the inverse square). That is line 22 of
`src/sim/Gravity.cpp`:

```cpp
Vec3 pull = d * (params.G / (r2 * std::sqrt(r2)));   // d / r^3
out[i] += pull * bodies[j].mass;                      // i towards j
out[j] -= pull * bodies[i].mass;                      // j towards i, Newton's third law
```

Each pair is visited once (`j` starts at `i + 1`), and the third law gives the
other body's pull for free. With N bodies there are `N·(N-1)/2` pairs: 45 for the 10
bodies here. That number grows like N², which is why the benchmark shows 1000 bodies
costing about a hundred times more than 100 (chapter 7 and `docs/MEASUREMENTS.md`).

## 3.4 Softening

As two centres get close, `1/r²` grows without limit, and at `r = 0` it divides by
zero. Real planets never get that close (they collide first), but a simulation must
not produce infinity. Two ways to handle it:

- **Cutoff** (the old code): if `r < 0.01`, ignore gravity. That makes the force
  jump from huge to zero, a **discontinuity**.
- **Plummer softening** (now): use `r² + ε²` instead of `r²` (ε, epsilon, = 0.01
  here, `src/sim/Gravity.h`). Far away it makes no difference (at r = 1 the force
  is 0.015% weaker); close in, the force smoothly falls to zero at the centre.
  This is the standard trick in N-body codes.

Tested in `Gravity.SofteningGivesZeroPullAtZeroDistance`,
`SofteningIsNegligibleAtPlanetaryDistances` and `SofteningKeepsThePullContinuous`.

## 3.5 Units and why G = 1

Real values (G = 6.674e-11, the Sun's mass 2e30 kg) would give huge and tiny
numbers that are awkward on screen and in floating point. Stardust uses its own
units: G = 1, Sun mass 2000, Earth at distance 20. What makes an orbit look right is
the *relationships* between mass, distance and speed, and those are the same laws.
The masses are not realistic on purpose (Earth 10, Jupiter 3) so that Earth can hold
the Moon close by in such a compact system.

## 3.6 Circular orbits

For a small body circling a big mass M at radius r, gravity must provide exactly the
pull needed to keep it turning. Moving in a circle at speed v needs an acceleration
`v²/r` towards the centre (**centripetal acceleration**). Set it equal to gravity:

```
v² / r = G · M / r²     =>     v = sqrt(G · M / r)
```

That is `CircularOrbitSpeed` (`src/sim/SolarSystem.cpp:26`). Earth: `sqrt(2000/20) = 10`.
Slower than this and the orbit becomes an ellipse dipping inwards; faster and it
swings outwards; above `sqrt(2)` times this (the **escape velocity**) it leaves for
good.

The **period** (time for one orbit) is `2πr / v`. Earth: `2π·20/10 ≈ 12.6 s`. The
Moon around Earth: `v = sqrt(10/0.9) ≈ 3.33`, period ≈ 1.7 s.

## 3.7 The Moon and the Hill sphere

The Moon orbits Earth while both orbit the Sun, and the Sun pulls on the Moon too.
Close to Earth, Earth's pull wins; far away, the Sun's wins. The boundary is roughly
the **Hill sphere** radius:

```
r_H ≈ a · (m / 3M)^(1/3)
```

with `a` Earth's distance from the Sun, `m` Earth's mass, `M` the Sun's mass. Here:
`20 · (10 / 6000)^(1/3) ≈ 2.37`. The Moon sits at 0.9, about 38% of that. Orbits
well inside about half the Hill radius are generally stable; that is why the Moon
survives. It also orbits in the same direction as Earth goes round the Sun
(**prograde**), which is more stable than the opposite.

Measured: over 600 simulated seconds the Moon's distance from Earth stays between
0.667 and 1.132 (`docs/MEASUREMENTS.md` section 3); the merge distance is 0.56.

## 3.8 Momentum

**Momentum** `p = m·v` (a vector). Because forces come in equal and opposite pairs,
gravity between bodies never changes the **total** momentum of the system. If the
total is zero, the system's **centre of mass** (the mass-weighted average position,
`sim::CenterOfMass`) stays still.

`BuildBodies` gives the Sun a tiny velocity that cancels all the planets' momentum
(`src/sim/SolarSystem.cpp:52`), so the whole system does not drift off screen.
`SolarSystem.TotalMomentumStartsAtZero` checks it; the five-minute stability test
checks it stays below 1e-9.

## 3.9 Energy

- **Kinetic energy** (energy of motion): `½·m·v²` per body.
- **Gravitational potential energy** (energy of position): `-G·m1·m2 / r` per pair.
  It is negative and rises towards zero as bodies separate.

With only gravity acting, their sum, the **total energy**, is constant. That makes
it a powerful check on the simulation: if the computed total drifts, the numerical
method is making errors. `sim::TotalEnergy` (`src/sim/Diagnostics.cpp:25`) computes
it, using the same softening as the forces so the two agree. Measured: over 300
simulated seconds it drifts by about 1.6e-8 of its starting value.

## 3.10 Collisions: perfectly inelastic merges

When two bodies overlap enough (their centres closer than 0.8 × the sum of their
radii; 0.8 forgives grazing contact), they merge into one
(`src/sim/Collision.cpp`):

- **Who survives:** the Sun, if involved (it is the "anchor"); otherwise the heavier
  body. The other is marked dead (`isAlive = false`) rather than removed, so indexes
  keep meaning the same planet.
- **Mass:** adds up.
- **Momentum is conserved:** the merged body's velocity is
  `(m1·v1 + m2·v2) / (m1 + m2)` (`Collision.cpp:43`). This is a **perfectly inelastic
  collision**: the bodies stick together. Kinetic energy is *not* conserved (it
  would become heat), which is why energy checks only compare between merges.
- **Centre of mass is conserved:** the merged body sits at
  `(m1·x1 + m2·x2) / (m1 + m2)` (`Collision.cpp:44`).
- **Volume adds:** a sphere's volume is `4/3·π·r³`; the `4/3·π` cancels, so
  `r_new = cbrt(r1³ + r2³)` (`Collision.cpp:50`).

> **Bug that was fixed:** the old code let the Sun keep its own velocity when it
> swallowed something, so the planet's momentum vanished and the "momentum is
> conserved" claim was false for Sun impacts. It also left survivors at their old
> position. Both are now correct and tested
> (`Collision.MergeIntoTheAnchorAlsoConservesMomentum`,
> `MergedBodySitsAtTheCentreOfMass`).

## Exercises

1. Compute the circular speed and period for Mars (r = 30) and Neptune (r = 140).
   Check the speeds against `ComputedSpeedsMatchTheOldHandTypedValues` in
   `tests/SolarSystemTests.cpp`.
2. Earth's mass is 10. Using the Hill radius formula, how far out could a moon of
   Earth orbit, roughly? What happens to that distance if you set Earth's mass to 1
   with the slider? Try it in the app.
3. Two bodies: mass 3 at velocity (2, 0, 0) and mass 1 at velocity (-1, 1, 0) merge.
   Compute the merged velocity. Check with `Collision.MergeConservesMomentumBetweenPlanets`.
4. Compute the kinetic energy before and after the merge in exercise 3. Where did
   the difference go?
5. Explain, without equations, why a feather and a hammer fall at the same rate,
   using the line `out[i] += pull * bodies[j].mass;`.
