# Glossary

Every term the learning path introduces, in alphabetical order, with the chapter
that explains it.

| Term | Meaning | Ch. |
|---|---|---|
| Acceleration | Rate of change of velocity | 3 |
| Accumulator | Leftover real time waiting to be turned into fixed physics steps | 4 |
| Additive blending | New colour is added to the old; overlaps get brighter | 5 |
| Alpha blending | New colour mixed with the old by its alpha (transparency) | 5 |
| Anonymous namespace | `namespace { }`: makes helpers private to one file | 1 |
| Back-face culling | Skipping triangles that face away from the camera | 5 |
| Barnes-Hut | O(N log N) gravity approximation using an octree | Roadmap |
| Batch (render batch) | Many draw commands collected and sent to the GPU together | 5 |
| Benchmark | A program that measures speed or accuracy repeatably | 7 |
| Bloom | Glow around bright things; here imitated with additive circles | 5 |
| Branch (git) | A movable name pointing at a commit | 8 |
| Build system generator | Tool (CMake) that writes build files for other tools | 7 |
| Centre of mass | Mass-weighted average position | 3 |
| Centripetal acceleration | Acceleration towards the centre needed to move in a circle, v²/r | 3 |
| CI (continuous integration) | Automatic build and test of every push | 7 |
| Class | A type bundling private data with the methods allowed to change it | 1 |
| Commit (git) | A saved snapshot with message, author and parent | 8 |
| Compiler | Program that translates source code into machine code | 1 |
| const | Promise that something will not change | 1 |
| Constructor / destructor | Functions that run when an object is created / destroyed | 1 |
| Cross product | Vector perpendicular to two others | 2 |
| CTest | CMake's test runner | 7 |
| Dangling pointer | Pointer to memory that is no longer valid | 1 |
| Depth buffer | Per-pixel distance of the nearest thing drawn so far | 5 |
| Diffuse lighting | Light that depends on how directly a surface faces the light (Lambert) | 5 |
| Dot product | `ax·bx + ay·by + az·bz`; measures how aligned two vectors are | 2 |
| Draw call | One request to the GPU to draw something | 5 |
| Encapsulation | Hiding data so it can only change in allowed ways | 1 |
| enum class | A scoped list of named choices | 1 |
| Equirectangular | Image mapping longitude/latitude to x/y, like a world map | 5 |
| Escape velocity | Speed needed to leave orbit for good, √2 × circular speed | 3 |
| Event | A record of something that happened, passed to whoever reacts | 6 |
| FetchContent | CMake feature that downloads dependencies | 7 |
| Fixed timestep | Physics always advances in equal steps | 4 |
| Floating point | Number format with mantissa and exponent; `float` ~7 digits, `double` ~16 | 1, 4 |
| Fragment (pixel) shader | GPU program run per pixel to choose its colour | 5 |
| Frame loop | Repeat: input, update, draw | 4, 6 |
| Gamma / sRGB | Non-linear brightness encoding of images and screens | 5 |
| glTF / .glb | Standard 3D model file format (.glb = binary) | 5 |
| GLSL | OpenGL Shading Language | 5 |
| GoogleTest | C++ unit testing library | 7 |
| GPU | Graphics processor | 5 |
| Gravitational constant (G) | Strength of gravity; 1 in this simulation's units | 3 |
| Hash (git, SHA-256) | Fingerprint of content; identifies commits and verifies downloads | 7, 8 |
| HDR | High dynamic range: brightness beyond 0-255 | 5 |
| Header / source file | `.h` declares, `.cpp` defines | 1 |
| Hill sphere | Region where a planet's gravity dominates its star's | 3 |
| Immediate-mode GUI | GUI drawn and handled by function calls every frame (raygui) | 5 |
| Index-paired arrays | Several arrays where index i always means the same thing | 6 |
| Inelastic collision | Bodies stick together; momentum kept, kinetic energy lost | 3 |
| Integrator | Method for stepping motion forward in time | 4 |
| Interpolation (lerp) | Blending between two values | 2, 4 |
| Inverse-square law | Strength falls with the square of distance | 3 |
| Kick / drift | Velocity update / position update | 4 |
| Kinetic energy | ½mv², energy of motion | 3 |
| Lambda | Small unnamed function written inline | 1 |
| Linker | Joins compiled files and libraries into a program | 1 |
| Mantissa / exponent | The digits and the power-of-two scale of a floating-point or RGBE number | 4, 5 |
| Matrix (4x4) | Grid of numbers that transforms points (move, rotate, scale, project) | 5 |
| Merge (git) | Combine another branch's commits into this one | 8 |
| Mesh | 3D shape made of triangles | 5 |
| Mipmaps | Pre-shrunk copies of a texture | 5 |
| Momentum | Mass × velocity; total is conserved without outside forces | 3 |
| MVP | Model-view-projection matrix | 5 |
| Namespace | Named group for code, avoiding name clashes | 1 |
| Normal | Direction a surface faces | 5 |
| Normalise | Scale a vector to length 1 | 2 |
| Numerical integration | Approximating motion in discrete time steps | 4 |
| O(N²) | Cost grows with the square of the number of items | 3, 7 |
| Operator overloading | Defining `+`, `*` etc. for your own types | 1 |
| Order (of an integrator) | How fast error shrinks with step size | 4 |
| Perpendicular | At right angles; dot product zero | 2 |
| Phong specular | Shiny highlight model | 5 |
| Picking | Finding what the mouse is over via a ray | 5 |
| Plummer softening | Using r² + ε² in gravity to avoid infinities | 3 |
| Point light | Light radiating from a point in all directions | 5 |
| Pointer | Variable holding a memory address | 1 |
| Potential energy | Energy of position; −Gm₁m₂/r for gravity | 3 |
| Preprocessor | Step before compiling that handles `#include`, `#define` | 1 |
| Preset (CMake) | Named configuration in CMakePresets.json | 7 |
| Problem matcher | CI rule turning log lines into annotations | 7 |
| Projection matrix | Applies perspective | 5 |
| Pull request | Request on GitHub to merge one branch into another | 8 |
| RAII | Tying a resource's release to an object's destructor | 1 |
| Radians | Angle unit; full turn = 2π | 2 |
| Range-based for | `for (T &x : container)` | 1 |
| Rasterisation | Working out which pixels a triangle covers | 5 |
| Reference | Another name for an existing variable (`T&`) | 1 |
| Regression test | Test that reproduces a fixed bug so it can't return | 7 |
| Reinhard tone mapping | `x / (x + 1)` | 5 |
| Remote (git) | Another copy of the repository, e.g. `origin` on GitHub | 8 |
| Reproducible build | Same inputs give the same build everywhere | 7 |
| RGBE | HDR pixel format: 3 mantissas + shared exponent | 5 |
| RLE (run-length encoding) | Storing "repeat this n times" instead of repeats | 5 |
| Semi-implicit Euler | Kick then drift with the new velocity; first order, symplectic | 4 |
| Shader | Program running on the GPU | 5 |
| Smart pointer (`unique_ptr`) | Pointer that frees what it owns automatically | 1 |
| Spiral of death | Slow frames causing more physics steps causing slower frames | 4 |
| Static library | Compiled code bundled for linking into programs | 7 |
| Struct | Plain bundle of data | 1 |
| Symplectic | Integrator property: energy error stays bounded | 4 |
| Tag (git) | Fixed name for one commit | 8 |
| Target (CMake) | A library or executable and its settings | 7 |
| Texture / UV | Image on the GPU / coordinates into it | 5 |
| Tone mapping | Squeezing HDR values into displayable range | 5 |
| Translation unit | One `.cpp` file plus its includes, compiled on its own | 1 |
| Undefined behaviour | Unchecked mistakes with unpredictable results | 1 |
| Uniform | Shader input that is the same for a whole draw call | 5 |
| Unit test | Small automated check of one piece of code | 7 |
| Unit vector | Vector of length 1 | 2 |
| Velocity | Rate of change of position | 3 |
| Velocity Verlet / leapfrog | Kick(½) drift kick(½); second order, symplectic | 4 |
| Vertex shader | GPU program run per vertex | 5 |
| View matrix | Moves the world so the camera is at the origin | 5 |
| Vsync | Waiting for the monitor refresh before showing a frame | 5 |
| Working tree / staging area | Your files / changes marked for the next commit | 8 |
