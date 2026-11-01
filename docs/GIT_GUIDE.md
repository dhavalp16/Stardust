# Git guide: what exists now and what to do in the morning

Read `docs/learn/08-git.md` first if words like *branch*, *merge* or *pull request*
are new. This guide is the exact sequence of steps.

## What exists right now

Nothing that existed before was changed, renamed, merged into or deleted.

**Your existing branches (untouched):**

| Branch | Commit | What it is |
|---|---|---|
| `release/sd-winApp` | `4acb9f0` | Windows desktop app; your resume links here |
| `desktop` | `59fd68d` | Older copy of the desktop line (README differs) |
| `main` | `70054a1` | Android/mobile line (GitHub's default branch) |
| `release/sd-androidApp` | `5cbf1e7` | Same files as `main` |

**New safety tags**, one per branch, pushed to GitHub:
`archive/2026-11-02/release/sd-winApp`, `archive/2026-11-02/desktop`,
`archive/2026-11-02/main`, `archive/2026-11-02/release/sd-androidApp`. A tag never
moves, so these always point at the code as it was. To look at the old desktop code
at any time: `git switch --detach archive/2026-11-02/release/sd-winApp`.

**New work branches**, pushed to GitHub, each starting from the previous one:

```
release/sd-winApp (4acb9f0)
  └─ 1. chore/cmake-build          build system, folder layout, tooling, CI
      └─ 2. feature/simulation-core    physics library, tests, physics fixes
          └─ 3. refactor/app-loop        app fixes, HDR loader, screenshots
              └─ 4. docs/learning-guide    README and all documentation
```

Branch 4 contains everything. Each branch's commits build and pass their tests.

**Why this structure:** small, themed branches are how teams make big changes
reviewable. Each pull request is about one thing, CI checks each one, and if one
turns out wrong it can be reverted without losing the others.

**Pull requests were not opened**, because the `gh` command-line tool is not
installed on this laptop (the instructions said to open them only if it was). Step 2
below opens them in the browser, with the text ready to paste.

## Step 1: check it works (before merging anything)

1. Install Visual Studio as described in `docs/learn/00-setup.md`.
2. Get the branches:
   ```
   cd C:\dev\Stardust-v2
   git fetch origin
   git switch docs/learning-guide
   ```
3. Open the folder in Visual Studio, build Release, run it.
4. Go through `docs/CHECK_BY_EYE.md`. If something is wrong, stop here: nothing has
   been merged, so nothing on your resume link has changed. Note what failed.

## Step 2: open the four pull requests

For each row, open the link while signed in to GitHub, tick nothing else, paste the
title and description from below, and click **Create pull request**. (If you prefer,
use the arrow on the button to create a **draft** first.)

| # | Open this link | Title |
|---|---|---|
| 1 | https://github.com/dhavalp16/Stardust/compare/release/sd-winApp...chore/cmake-build?expand=1 | Build: CMake, standard layout, warnings, CI |
| 2 | https://github.com/dhavalp16/Stardust/compare/release/sd-winApp...feature/simulation-core?expand=1 | Physics: tested simulation library, fixed timestep, Verlet, conservation fixes |
| 3 | https://github.com/dhavalp16/Stardust/compare/release/sd-winApp...refactor/app-loop?expand=1 | App: update/draw split, window and HUD fixes, 11x faster HDR loader |
| 4 | https://github.com/dhavalp16/Stardust/compare/release/sd-winApp...docs/learning-guide?expand=1 | Docs: README, decisions, roadmap, learning path |

All four point into `release/sd-winApp`. Until PR 1 is merged, PR 2 also shows PR 1's
commits (it was built on top of it); that is expected, and it shrinks after each
merge.

Wait for the green tick (CI) on each before merging.

### Description for PR 1

```
Replaces the Visual Studio project with a CMake build and tidies the repository.

- CMake + CMakePresets.json; raylib 5.5 fetched at a pinned version and SHA-256 hash.
  Visual Studio opens the folder directly (File > Open > Folder).
- Standard layout: src/, third_party/, assets/, resources/ (moved with git mv).
- The app finds its asset folder itself, so it runs from any working directory.
- Strict warnings on our code only; .clang-format and .editorconfig.
- GitHub Actions builds with MSVC on every push.
- docs/AUDIT.md (ranked problems with evidence) and docs/PROGRESS_LOG.md.

Merge first. Use "Create a merge commit".

🤖 Generated with [Claude Code](https://claude.com/claude-code)
```

### Description for PR 2

```
Moves the physics into a raylib-free library with unit tests and fixes the physics bugs.

- stardust_sim library (no raylib); Planet split into sim::Body + PlanetInfo;
  selection by index instead of a pointer into a vector.
- Fixed 1/600 s timestep with an accumulator and a 0.1 s clamp (one long frame used
  to be able to knock the Moon into Earth; now a regression test).
- Velocity Verlet integration; double-precision state (float rounding was measured
  to dominate the error).
- Plummer softening instead of a hard cutoff; merges conserve momentum and centre of
  mass, including into the Sun; starting speeds computed from sqrt(G*M/r).
- GoogleTest suite, stardust_bench, docs/MEASUREMENTS.md; CI runs tests on Windows
  (MSVC) and Linux (GCC, no raylib).

Depends on PR 1: merge after it. Use "Create a merge commit".

🤖 Generated with [Claude Code](https://claude.com/claude-code)
```

### Description for PR 3

```
Fixes in the application layer, each checked with before/after screenshots.

- Stardust.exe --screenshots <folder>: scripted run that saves three pictures.
- Update before draw: spin and debris no longer change inside the draw loop.
- The Sun keeps its unlit shader; the light follows the Sun.
- Window fits a 1920x1080 screen (resizable, maximised when needed); HUD panel
  rectangles shared with click handling (fixes a click-through strip).
- Frame-rate independent camera easing; optional music; deduplicated camera speed.
- HDR loader rewritten as a tested module: 3.4 s -> 0.3 s on the real sky,
  byte-identical output.
- main() split into named sections and helpers; CI treats warnings as errors and
  shows MSVC warnings as annotations.

Depends on PR 2: merge after it. Use "Create a merge commit".

🤖 Generated with [Claude Code](https://claude.com/claude-code)
```

### Description for PR 4

```
Documentation only (no code changes).

- New README describing the current code.
- docs/decisions/: why CMake, why a raylib-free simulation, why fixed timestep and
  Verlet, why double precision.
- docs/ROADMAP.md, docs/CHECK_BY_EYE.md, docs/RESUME_CLAIMS.md, docs/FINAL_REPORT.md,
  docs/GIT_GUIDE.md.
- docs/learn/: a learning path from zero, with exercises and interview questions.

Depends on PR 3: merge last. Use "Create a merge commit".

🤖 Generated with [Claude Code](https://claude.com/claude-code)
```

## Step 3: merge them, in order

For PR 1, then 2, then 3, then 4:

1. Open the PR. Check the CI tick is green.
2. Use the arrow next to the merge button and choose **Create a merge commit** (not
   "Squash and merge", not "Rebase and merge"). Confirm.

**Why merge commits and this order:** PR 2 was built on top of PR 1's exact commits.
A merge commit keeps those commits as they are, so GitHub sees that PR 2's first
commits are already in `release/sd-winApp` and only PR 2's own commits remain.
"Squash" would make a new, different commit for PR 1; PR 2 would then still contain
the originals, and you would see duplicates or conflicts.

Do not delete the branches when GitHub offers (you can, they are merged, but leaving
them costs nothing while you study the history).

**Simpler alternative:** merge only PR 4 (it contains all four branches' commits) and
close PRs 1-3 with a comment "merged via PR 4". You lose the per-theme review history
on GitHub, but the commits are identical.

## Step 4: update your laptop

```
git switch release/sd-winApp
git pull
git log --oneline --graph -15
```
You should see the four merge commits on top.

Build and run once more on `release/sd-winApp` to be sure.

## Step 5 (recommended): tag a release

A tag gives your resume a link that never changes, even as you keep working:

```
git tag -a v1.0.0 -m "Stardust 1.0: tested physics library, fixed timestep, docs"
git push origin v1.0.0
```
Link: `https://github.com/dhavalp16/Stardust/tree/v1.0.0`.

## Step 6 (when you are ready): tidy the repository

These change what visitors see, so they are your decision. None loses anything,
because the archive tags exist.

1. **Make the desktop line the default branch.** On GitHub: *Settings > General >
   Default branch*, choose `release/sd-winApp`. Visitors then see the desktop README
   first instead of the Android line.
2. **Retire `desktop`.** It is superseded by `release/sd-winApp` and kept forever by its
   archive tag. Delete it on GitHub (*Branches* page) or with
   `git push origin --delete desktop`.
3. **Delete the merged topic branches** the same way, once you no longer want them.
4. **Later**, if you rename branches (for example `release/sd-winApp` to `main` and the
   Android line to `android`), update your resume and LinkedIn links on the same day.

## If something goes wrong

- **A merged PR broke something:** open it on GitHub and click **Revert**. That
  creates a new PR that undoes it. Or locally:
  `git revert -m 1 <merge-commit-hash>` then `git push`.
- **You want the old code:** `git switch --detach archive/2026-11-02/release/sd-winApp`.
- **Your local folder is in a strange state:** `git status` tells you what is going on;
  `git switch release/sd-winApp` gets you back to the branch. Avoid `git reset --hard`
  and `git push --force` until you are sure what they do.

## A note on the commit identity

This laptop had no git name or email set. The overnight commits were made with a
setting for this repository only:
`dhavalp16 <146697423+dhavalp16@users.noreply.github.com>` (your GitHub no-reply
address, which your earlier commits already use). Each overnight commit also says
`Co-Authored-By: Claude`. To set your own identity for all repositories:

```
git config --global user.name "Your Name"
git config --global user.email "you@example.com"
```
