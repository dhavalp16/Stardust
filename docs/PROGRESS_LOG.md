# Progress log

This file records the overnight overhaul as it happened, so that anyone (you, or a
new session after a restart) can see what is done and pick up where it stopped.
Newest entries are at the bottom.

## Ground rules for this work

- Starting point: `release/sd-winApp` at `4acb9f0`. Existing branches are never
  rewritten, merged into, renamed or deleted. The work goes on new branches.
- Before anything else, every existing branch head was tagged
  `archive/2026-11-02/<branch>` so it can always be recovered.
- Every commit should configure, build and pass the tests that exist at that commit.
- Numbers are only reported if they were measured, with the method stated.

## Branch plan (each branch starts from the previous one)

| # | Branch | Contents |
|---|---|---|
| 1 | `chore/cmake-build` | Audit, progress log, CMake build, asset path lookup, folder layout, tooling, CI |
| 2 | `feature/simulation-core` | Headless simulation library, tests, fixed timestep, Verlet integrator, collision fixes |
| 3 | `refactor/app-loop` | App-side fixes: selection by index, update/render split, window, HUD, HDR loader |
| 4 | `docs/learning-guide` | Learning material, interview prep, resume claims, git guide, final report |

Each branch gets a pull request into `release/sd-winApp`. They must be merged in
order 1, 2, 3, 4. See `docs/GIT_GUIDE.md`.

## Environment found on this laptop

- Git 2.x with the Git Credential Manager; push access to `origin` works.
- **No Visual Studio, no MSVC compiler, no CMake, no `gh` CLI** were installed,
  even though the plan assumed them. Nothing was installed system-wide. Instead,
  portable copies of CMake 4.4.5, Ninja 1.13.2 and llvm-mingw 20261020 (clang
  23.1.4) were unpacked into a temporary folder and used for every local build.
  MSVC builds are checked by the GitHub Actions workflow instead.
- Because `gh` is missing, pull requests were not opened automatically. Their
  titles and descriptions are written out in `docs/GIT_GUIDE.md`.
- No git identity was configured, so a repo-local identity was set to
  `dhavalp16 <146697423+dhavalp16@users.noreply.github.com>` (your GitHub
  no-reply address, already used in your history).

## Log

### Session 1 (night of 1-2 November 2026)

- Read every file on `release/sd-winApp`, compared all four branches, read the
  whole history.
- Built the untouched code with CMake + clang + raylib 5.5: builds, runs.
- Captured baseline screenshots with a temporary uncommitted patch. Saw the Moon
  merge into Earth shortly after pressing play.
- Reproduced it headlessly: the original physics keeps the Moon at steady frame
  rates but loses it after one long frame (see `docs/AUDIT.md`).
- Wrote `docs/AUDIT.md` and this log.
- `chore/cmake-build` done and pushed: CMake + presets, folder layout, asset-root
  lookup, warnings, `.clang-format`, CI. CI (MSVC) built it successfully.
- The temporary toolchain in the Temp folder was deleted mid-session by something
  on the machine (probably a cleanup or antivirus process). It was re-downloaded to
  `C:\Users\Optimus\stardust-build-tools\` (outside the repo; safe to delete when
  done). `source /c/Users/Optimus/stardust-build-tools/env.sh` in Git Bash puts
  CMake, Ninja and clang on PATH.
- `feature/simulation-core` done and pushed: raylib-free `stardust_sim` library,
  GoogleTest, fixed timestep, velocity Verlet, double precision, softening,
  conserving merges, computed initial speeds, `stardust_bench`,
  `docs/MEASUREMENTS.md`, CI runs tests on Windows and Linux.
- `refactor/app-loop`: scripted screenshots (`--screenshots <folder>`), update/draw
  split, Sun shader and light, window fits the screen, shared HUD rectangles,
  camera easing, optional music, HDR loader module (11x faster, identical
  output), warnings-as-errors in CI. Each visual change was checked by comparing
  screenshot runs.
- Next: `docs/learning-guide` (README, learning material, interview prep, git
  guide, resume claims, final report), then verify every commit builds.
- CI on `refactor/app-loop` failed once warnings became errors: MSVC flagged
  C4244 (double to float) at `src/app/HUD.cpp:24`, which clang's default warnings
  miss. CI logs need admin rights to read, so an MSVC problem matcher was added to
  turn warnings into public annotations; that showed the line; fixed in `6c3cdda`.
  All three code branches are now green on Windows/MSVC and Linux/GCC.
- `docs/learning-guide`: README, decision records 0002-0004, roadmap, check-by-eye
  list, learning chapters 0-11 + glossary, git guide, resume claims.
- Every commit was then rebuilt and tested in a separate worktree
  (`verify.sh`, results in `docs/FINAL_REPORT.md`).
- Per-commit verification finished: every commit with a build system builds, and
  every commit with tests passes all of them (table in `docs/FINAL_REPORT.md`).
- Wrote `docs/FINAL_REPORT.md`; pushed `docs/learning-guide`.
- **State at the end of the session:** four branches pushed, no pull requests opened
  (no `gh`), nothing merged. Next steps are yours: `docs/GIT_GUIDE.md`.
