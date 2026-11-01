# Learning Stardust from zero

This folder teaches you everything in this project, assuming you know nothing yet:
not programming, not C++, not physics, not graphics, not git. By the end you should
be able to explain every part of Stardust in an interview and rebuild the
important parts yourself.

## How to use this

- **Go in order.** Each chapter uses words from the earlier ones. New terms are in
  **bold** and explained the first time they appear. `GLOSSARY.md` lists all of them.
- **Read with the code open.** References look like `src/sim/Gravity.cpp:18`:
  file `src/sim/Gravity.cpp`, line 18. In Visual Studio press Ctrl+G to jump to a
  line. Line numbers are for the final code on `docs/learning-guide`; if you edit
  the code, they will drift, so search for the quoted text instead.
- **Do the exercises.** Reading is not the same as being able to do it. Every
  chapter ends with exercises; `10-exercises.md` has the bigger "rebuild it
  yourself" exercises, each with a way to check your version works.
- **Explain it out loud.** After each chapter, explain its main idea in two minutes
  without notes, as if to an interviewer. If you can't, re-read.
- **Expect weeks, not days.** A realistic pace is one chapter per week alongside a
  C++ course, slower for chapters 1 and 4.

## Study order

| # | Chapter | What you will be able to do afterwards |
|---|---|---|
| 0 | [00-setup.md](00-setup.md) | Install the tools, build the project, run the app, the tests and the benchmark |
| 1 | [01-cpp.md](01-cpp.md) | Read every line of C++ in this project and explain what it does |
| 2 | [02-vectors-and-maths.md](02-vectors-and-maths.md) | Work with 3D vectors: add, scale, dot product, length, direction |
| 3 | [03-physics.md](03-physics.md) | Explain gravity, momentum, energy, circular orbits and how merges work |
| 4 | [04-numerical-integration.md](04-numerical-integration.md) | Explain Euler vs Verlet, fixed timesteps, and float vs double, with this project's measurements |
| 5 | [05-graphics.md](05-graphics.md) | Explain how a frame gets drawn: GPU, shaders, lighting, textures, HDR, tone mapping, the sky, picking |
| 6 | [06-architecture.md](06-architecture.md) | Draw the project's structure from memory and justify it |
| 7 | [07-build-test-tools.md](07-build-test-tools.md) | Explain CMake, dependencies, warnings, unit tests, CI, benchmarks |
| 8 | [08-git.md](08-git.md) | Use git confidently and explain this repository's history and branches |
| 9 | [09-what-changed-and-why.md](09-what-changed-and-why.md) | Walk through every change of the overhaul and defend it |
| 10 | [10-exercises.md](10-exercises.md) | Rebuild the core pieces yourself and prove they work |
| 11 | [11-interview-questions.md](11-interview-questions.md) | Answer likely interview questions, including the follow-ups |

Also useful: [GLOSSARY.md](GLOSSARY.md), [../RESUME_CLAIMS.md](../RESUME_CLAIMS.md)
(what you may claim and what you must know first) and
[../GIT_GUIDE.md](../GIT_GUIDE.md) (the morning steps for merging this work).

## What to learn alongside

This material explains *this project*. It is not a complete C++ course. Pair it with:

- a beginner C++ course or book (for example "learncpp.com", free, chapters 0-12
  first);
- for maths, Khan Academy's vectors and basic trigonometry;
- for graphics, "learnopengl.com" (Getting Started and Lighting sections).

When this material and a course disagree on a detail, the course is probably more
general and this material is probably simplifying; ask why.
