# 1. C++, through this project

This chapter teaches the C++ you need to read every line of Stardust. It does not
replace a course (see the README for one); it shows each idea where it appears in
this code, so you learn the language and the project at the same time.

## 1.1 What a program is

A **CPU** (the processor) can only run **machine code**: numbers that mean "add these
two", "jump there". Nobody writes that by hand. You write **source code** in a
language like C++, and a **compiler** translates it into machine code.

C++ is **compiled ahead of time**: you build an `.exe` once and then run it. It is
used for games and engines because it is fast and gives you control over memory,
which also means it lets you make mistakes other languages would stop.

Building has two stages:

1. **Compiling**: each `.cpp` file (a **translation unit**) becomes an **object
   file** on its own.
2. **Linking**: the **linker** joins all object files and libraries into one `.exe`,
   connecting each use of a function to its definition.

Errors from stage 1 are "compile errors" (you wrote invalid C++). Errors from
stage 2 are "link errors" ("unresolved external symbol": something was declared but
never defined, or defined twice).

## 1.2 Headers and source files

Look at `src/sim/Gravity.h` and `src/sim/Gravity.cpp`.

- A **header** (`.h`) says *what exists*: the **declaration** of a function (its
  name, inputs and output) or the layout of a type. Other files include it.
- A **source file** (`.cpp`) says *how it works*: the **definition** (the body).

`#include "sim/Gravity.h"` is a **preprocessor directive**: before compiling, the
**preprocessor** pastes that file's text in place. `#pragma once` at the top of
every header stops it being pasted twice into one file.

Why split? Many files need to *call* `ComputeAccelerations`; only one needs its
body. Changing the body then only recompiles `Gravity.cpp`.

`#include <vector>` (angle brackets) means a standard library or system header;
`#include "sim/Body.h"` (quotes) means one of ours.

## 1.3 Types and variables

A **variable** is a named piece of memory holding a value of some **type**:

| Type | Holds | Example in the code |
|---|---|---|
| `int` | whole numbers | `int steps = 0;` in `src/sim/FixedTimestep.h:27` |
| `float` | decimal numbers, ~7 significant digits, 4 bytes | `float mass` in `src/sim/Body.h` |
| `double` | decimal numbers, ~16 significant digits, 8 bytes | `double x` in `src/sim/Vec3.h` |
| `bool` | `true` or `false` | `bool isAlive` in `src/sim/Body.h` |
| `size_t` | non-negative sizes and indexes | loop counters over vectors |
| `unsigned char` | 0-255, one byte | pixel channels in `src/io/HdrLoader.cpp` |

`1.0f` is a float literal, `1.0` a double. Converting double to float can lose
digits, so compilers warn about doing it silently; we write
`static_cast<float>(x)` to say "yes, I mean it" (`src/app/Convert.h:9`).
Chapter 4 shows why the physics needs `double`.

`const` means "this will not change": `const double eps2 = ...` in
`src/sim/Gravity.cpp:8`. Use it whenever you can; it prevents accidents and tells
the reader what to expect.

## 1.4 Functions

```cpp
double CircularOrbitSpeed(double G, double parentMass, double r) {
  return std::sqrt(G * parentMass / r);
}
```
(`src/sim/SolarSystem.cpp:26`). It takes three **parameters**, returns a `double`.
`std::sqrt` is the square-root function from `<cmath>`; `std::` means it lives in
the standard library's **namespace** (see 1.10).

## 1.5 Structs: grouping data

```cpp
struct Body {
  Vec3 position;
  Vec3 velocity;
  float mass = 1.0f;
  float radius = 1.0f;
  bool isAlive = true;
};
```
(`src/sim/Body.h`). A **struct** bundles related variables (**members**, also called
**fields**) into one type. `Body b; b.mass = 5.0f;` uses the dot to reach a member.
The `= 1.0f` parts are **default member initialisers**: a new `Body` starts with
those values.

## 1.6 References and pointers

By default, C++ **copies** arguments into functions. For big things, or when the
function must change the caller's variable, pass a **reference** with `&`:

```cpp
void Drift(std::vector<Body> &bodies, double dt)
```
(`src/sim/Integrator.h:14`). `bodies` *is* the caller's vector, not a copy, so
moving bodies inside `Drift` moves the real ones. `const std::vector<Body> &bodies`
(in `Gravity.h`) means "use the real one, but promise not to change it": fast and
safe.

A **pointer** (`Type *p`) holds a memory address. `&x` gives the address of `x`;
`*p` reaches what `p` points at; `nullptr` means "points at nothing". Raw pointers
are easy to get wrong:

> **Bug that was fixed:** the old code selected a planet with
> `Planet *selectedPlanet = &activePlanets[i];`, a pointer into a `std::vector`.
> When a vector grows it may move all its elements to new memory, and the pointer
> would then point at freed memory (a **dangling pointer**). It happened to work
> because the vector never grew. The new code stores the **index** instead:
> `int selected = -1;` (`src/app/main.cpp:203`), which stays valid.

The HUD sliders do use a pointer on purpose: `GuiSlider(..., &body.mass, ...)`
(`src/app/HUD.cpp:60`) gives raygui the address of the mass so it can write the new
value straight into the simulation.

## 1.7 std::vector: a growable array

`std::vector<Body> bodies;` is a list of `Body` values stored one after another in
memory. `bodies.size()`, `bodies[i]`, `bodies.push_back(b)`, `bodies.reserve(n)`.
Being contiguous makes it fast to loop over, which matters in engines.

Loops over it, two styles:

```cpp
for (size_t i = 0; i < bodies.size(); i++) { ... bodies[i] ... }   // need the index
for (Body &body : bodies) { ... body ... }                         // don't need it
```
Both appear in `src/sim/Integrator.cpp`. The second is a **range-based for loop**.

`std::array<unsigned char, 65536>` (`src/io/HdrLoader.cpp:144`) is a fixed-size
array whose size is known when compiling.

## 1.8 Classes: data plus the rules for changing it

`sim::FixedTimestep` (`src/sim/FixedTimestep.h:13`) is a **class**. Like a struct,
but its data is `private:` (only its own functions may touch it) and it offers
`public:` **member functions** (also called **methods**) to use it:

```cpp
class FixedTimestep {
public:
  explicit FixedTimestep(double stepSeconds, double maxFrameSeconds = 0.1)
      : step_(stepSeconds), maxFrame_(maxFrameSeconds) {}
  int Advance(double frameSeconds) { ... }
  void Reset() { accumulator_ = 0.0; }
  double StepSeconds() const { return step_; }
private:
  double step_;
  double maxFrame_;
  double accumulator_ = 0.0;
};
```

- The function with the class's own name is the **constructor**: it runs when an
  object is created, e.g. `sim::FixedTimestep physicsClock(1.0 / 600.0);`
  (`src/app/main.cpp:211`). The `: step_(...)` part is the **member initialiser
  list**.
- `= 0.1` is a **default argument**: callers may leave it out.
- `explicit` stops C++ from silently turning a plain number into a FixedTimestep.
- `const` after `StepSeconds()` promises the method does not change the object.
- The trailing underscore in `step_` is a naming habit for private members.

Why a class here? The accumulator must only change in the ways `Advance` and `Reset`
allow. Making it private makes misuse impossible. That idea is called
**encapsulation**. `sim::Simulation` (`src/sim/Simulation.h`) is another class: it
owns the bodies and its `Step` method is the only way physics advances.

`struct` and `class` are almost identical in C++ (only the default visibility
differs). Convention here: `struct` for plain data, `class` when there are rules.

## 1.9 enum and enum class

```cpp
enum class IntegratorType { SemiImplicitEuler, VelocityVerlet };
```
(`src/sim/Simulation.h`). A fixed set of named choices. With `enum class` you must
write `IntegratorType::VelocityVerlet`, which avoids name clashes. The older
`enum EngineState { PAUSED, PLAYING };` (`src/app/HUD.h:7`) puts `PAUSED` directly
in scope.

## 1.10 Namespaces

All simulation code is inside `namespace sim { ... }`, so its types are
`sim::Body`, `sim::Vec3`. This prevents clashes (raylib also has types with common
names) and tells the reader which part of the project something belongs to.
`namespace { ... }` with no name (an **anonymous namespace**, e.g. at the top of
`src/app/main.cpp`) makes helpers private to that one file.

## 1.11 Operator overloading

You can define what `+`, `-`, `*` mean for your own types:

```cpp
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline Vec3 operator*(Vec3 a, double s) { return {a.x * s, a.y * s, a.z * s}; }
```
(`src/sim/Vec3.h`). Now physics code reads like the maths:
`body.position += body.velocity * dt;` (`src/sim/Integrator.cpp:13`).
`inline` allows a function's body to live in a header that many files include.
`return {a.x + b.x, ...};` builds a `Vec3` from three values (**brace
initialisation**).

## 1.12 static

`static` means different things in different places:

- `static const std::vector<PlanetSpec> specs = {...};` inside a function
  (`src/sim/SolarSystem.cpp:9`): created once, the first time the function runs, then
  kept. Every call returns the same table.
- `static std::vector<KeyEntry> entries;` in `DrawKeyPressOverlay`
  (`src/app/HUD.cpp:100`): the same idea, used to remember keys between frames.
  Convenient, but it is hidden state; a class would make it visible.

## 1.13 Lambdas

A **lambda** is a small unnamed function written where it is used:

```cpp
[](const Fragment &f) { return !f.isAlive; }
```
(`src/app/Debris.cpp`), passed to `std::remove_if` to say "remove dead fragments".
`[]` lists what the lambda may use from outside (`[&]` means "everything, by
reference"). `src/io/HdrLoader.cpp:144` uses a lambda that is called immediately
(`[] { ... }()`) to build the lookup table once.

## 1.14 The standard algorithms

`<algorithm>` has ready-made, tested loops: `std::min`, `std::max`,
`std::remove_if`, `std::sort`. The "erase-remove idiom" in `UpdateDebris`
(`src/app/Debris.cpp`) removes all dead fragments in one pass: `remove_if` shuffles
the living ones to the front and returns where the dead ones begin; `erase` cuts
them off.

## 1.15 Resource management and RAII

Some things must be given back: memory, open files, GPU textures. Forgetting is a
**leak**; giving back twice is a crash.

C++'s answer is **RAII** ("Resource Acquisition Is Initialisation"): tie the resource
to an object whose **destructor** (the function that runs automatically when an
object goes out of scope) releases it. In `src/io/HdrLoader.cpp:32-36`:

```cpp
struct FileCloser { void operator()(std::FILE *f) const { std::fclose(f); } };
using FilePtr = std::unique_ptr<std::FILE, FileCloser>;
```
`std::unique_ptr` is a **smart pointer**: it owns what it points to and, when it is
destroyed, calls `FileCloser` to close the file. Every early `return` in
`LoadHdr` therefore closes the file without anyone writing `fclose` again. The
original loader had `free(scanline); free(pixels); fclose(f);` repeated nine times.

raylib is a C library, so its resources (`Model`, `Shader`, `Texture2D`) are plain
structs that you must free yourself with `UnloadModel` and friends. That is the
"Cleanup" section at the end of `src/app/main.cpp`. Wrapping them in RAII classes is
a good exercise (see `10-exercises.md`).

## 1.16 Casting

`static_cast<float>(x)`: a deliberate, checked conversion.
The old code used C-style casts like `(float)x`, which can do several kinds of
conversion silently; `static_cast` only does the safe, obvious kind. Both appear in
this code; prefer `static_cast` in new code.

## 1.17 Undefined behaviour

C++ lets some mistakes go unchecked: reading past the end of an array, using a
dangling pointer, dividing an integer by zero. The result is **undefined
behaviour**: it might work, crash, or corrupt something far away. The tests,
compiler warnings and careful bounds checks (for example in `ReadScanline`,
`src/io/HdrLoader.cpp:59`, which refuses runs that would overflow the row) are how
this project defends itself.

## Exercises

1. In `src/sim/Vec3.h`, find every operator. For each, write one line of maths it
   implements. Then add `operator/=` and write a test for it in
   `tests/Vec3Tests.cpp` (copy the style of the existing tests). Build and run
   `ctest --preset debug -R Vec3`.
2. Explain in your own words why `Simulation::Bodies()` exists twice
   (`src/sim/Simulation.h`), once with `const` and once without.
3. Change `int selected` back to a `sim::Body *` on a scratch branch. List every
   place that would need to change, and write down the exact situation in which
   the pointer would dangle. Don't commit it.
4. Rewrite the `for (size_t i ...)` loop in `Kick` (`src/sim/Integrator.cpp:6`) as a
   range-based loop. Why can't you, easily? (Hint: you need `i` for two vectors.)
5. Read `src/io/HdrLoader.cpp` top to bottom and list every early `return`. For
   each, say what would happen to the open file if `FilePtr` did not exist.
