# 2. Vectors and the maths you need

Almost everything in a 3D engine is a **vector**: a position, a velocity, a
direction, a colour. This chapter covers the vector maths Stardust uses, and the
little bit of trigonometry for placing planets on circles.

## 2.1 Coordinates

Stardust's world has three axes: **x** (left/right), **y** (up/down) and **z**
(towards/away from you). A point is three numbers `(x, y, z)`. The planets all
start in the flat **x-z plane** (y = 0), like a table top, and the camera starts
above it at `(0, 160, 200)` looking at the origin `(0, 0, 0)`
(`src/app/main.cpp`, "camera.position").

Units are made up ("simulation units"): the Sun has radius 3, Earth orbits at 20.
Only ratios matter; see chapter 3.

## 2.2 What a vector is

A vector is an arrow: it has a **direction** and a **length** (also called
**magnitude**). Written as three numbers, `(3, 4, 0)` means "3 along x, 4 along y".
The same type holds positions ("where"), velocities ("how fast, which way"),
accelerations and forces. In this code it is `sim::Vec3` (`src/sim/Vec3.h`) in
the physics and raylib's `Vector3` in the drawing code.

## 2.3 Adding and subtracting

Add component by component: `(1,2,3) + (4,5,6) = (5,7,9)`. Geometrically: walk
the first arrow, then the second.

Subtracting gives the arrow **from one point to another**:
`d = b.position - a.position` points from `a` to `b`. That is the first line of
the gravity loop (`src/sim/Gravity.cpp:18`).

## 2.4 Scaling

Multiply every component by a number: `(1,2,3) * 2 = (2,4,6)`. Same direction,
twice the length. A negative number flips the direction. This is how velocity
becomes a position change: `position += velocity * dt` moves "velocity for `dt`
seconds" (`src/sim/Integrator.cpp:13`).

## 2.5 Length

By Pythagoras in 3D: `|v| = sqrt(x² + y² + z²)`. For `(3, 4, 0)` that is
`sqrt(9 + 16) = 5` (tested in `tests/Vec3Tests.cpp`).

The square root is relatively slow and often unnecessary. To ask "is the distance
less than R?", compare **squared** lengths: `|d|² < R²`. The collision code does
exactly that (`src/sim/Collision.cpp:16`). `LengthSquared` exists for this.

## 2.6 Direction (normalising)

Dividing a vector by its own length gives a **unit vector**: length 1, same
direction. `d / |d|` is "which way", without "how far". raylib calls this
`Vector3Normalize`. Careful: normalising a zero-length vector divides by zero.

## 2.7 The dot product

`a · b = ax·bx + ay·by + az·bz`. A single number. Its meaning:

`a · b = |a| |b| cos(θ)`, where θ is the angle between them. So:

- positive: they point roughly the same way (angle < 90°);
- zero: they are at right angles (**perpendicular**);
- negative: roughly opposite.

Uses in Stardust:

- **Is the Sun in front of the camera?** `dot(cameraForward, directionToSun) > 0`
  (`DrawSunBloom` in `src/app/main.cpp`). If not, we skip the glow.
- **Is the starting velocity at right angles to the radius** (a circular orbit)?
  The test `SolarSystem.OrbitVelocityIsAtRightAnglesToTheParent` checks the dot
  product is zero.
- **Lighting**: how much a surface faces the light is `dot(normal, toLight)`
  (chapter 5).
- **Stop the camera flipping over the top**: `|dot(newForward, up)| < 0.95` limits
  how close to straight up you can look (`src/app/Input.cpp:69`).

## 2.8 The cross product

`a × b` gives a vector perpendicular to both. The camera uses it to find "right"
from "forward" and "up": `right = normalize(cross(forward, up))`
(`src/app/Input.cpp`, in the free-look block). Its length is `|a||b| sin θ`, which
is zero when `a` and `b` are parallel, which is why the camera must never look
exactly straight up (the 0.95 limit above).

## 2.9 Circles and trigonometry

A point at angle φ (**phi**, in **radians**) on a circle of radius r centred on the
origin is `(r cos φ, 0, r sin φ)` in the x-z plane. Radians measure angles by arc
length: a full turn is 2π ≈ 6.283 radians; 1 radian ≈ 57.3°. raylib's `RAD2DEG`
converts.

The direction of travel around that circle (the **tangent**) is
`(-sin φ, 0, cos φ)`. Check with the dot product: `cos φ · (-sin φ) + sin φ · cos φ = 0`,
so it is perpendicular to the radius. That is how every planet's starting position
and velocity are set (`src/sim/SolarSystem.cpp:42-45`).

## 2.10 Rotating a vector

To turn the camera, `Vector3RotateByAxisAngle(forward, up, angle)` rotates the
"forward" arrow around the "up" axis (`src/app/Input.cpp:66`). You don't need the
formula yet (it is Rodrigues' rotation formula); know that rotations are defined by
an **axis** and an **angle**.

## 2.11 Interpolation

**Linear interpolation** ("lerp") blends two values: `lerp(a, b, t) = a + (b - a)·t`.
`t = 0` gives `a`, `t = 1` gives `b`, `t = 0.5` the midpoint.

The tracking camera moves a fraction of the way to its goal every frame. Using
`t = dt·k` (the old code) means a long frame can make `t > 1` and overshoot. Using
`t = 1 - e^(-k·dt)` (now, `src/app/main.cpp:94`) always stays below 1 and gives
the same motion whatever the frame rate, because two frames of `dt/2` multiply out
to exactly one frame of `dt`: `e^(-k·dt/2) · e^(-k·dt/2) = e^(-k·dt)`.

## 2.12 Powers, roots, exponents

- `x²` is `x * x`; `x³` is `x * x * x`; `x^1.5` is `x * sqrt(x)` (used in the
  softened gravity, `src/sim/Gravity.cpp:22`).
- The **cube root** undoes cubing: `cbrt(8) = 2`. Used when merged planets keep the
  total volume (`src/sim/Collision.cpp:50`, chapter 3).
- `2^n` with negative `n` is a fraction: `2^-3 = 1/8`. The HDR format stores numbers
  as `mantissa · 2^exponent` (chapter 5).

## Exercises

Do these on paper first, then check with a tiny C++ program or a test.

1. `a = (1, 2, 2)`. What is `|a|`? What is the unit vector in its direction?
2. `a = (1, 0, 0)`, `b = (0, 0, 1)`. Compute `a · b` and `a × b`. Which way does the
   cross product point?
3. A planet at radius 20, angle φ = 5 radians. Compute its starting position and
   its direction of travel. Compare with Earth's row in `src/sim/SolarSystem.cpp`.
4. Show that `1 - e^(-k·dt)` is always between 0 and 1 for positive `k` and `dt`.
5. Two planets of radius 1 and 2. What radius does a ball with the same total
   volume have? (Answer: cube root of 9 ≈ 2.08.)
