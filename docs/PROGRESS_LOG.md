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
