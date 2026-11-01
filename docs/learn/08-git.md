# 8. Git, and this repository's history

**Git** records the history of a project: every saved version, who made it, and why.
**GitHub** hosts a copy online so others can see it and collaborate.
`docs/GIT_GUIDE.md` has the exact morning steps for merging the overhaul; this
chapter teaches what those steps mean.

## 8.1 The core ideas

- **Repository** (repo): the project folder plus its full history (kept in the
  hidden `.git` folder).
- **Commit**: a saved snapshot of all files, with a message, an author, a date, and a
  pointer to the previous commit (its **parent**). Each commit has an ID, a long
  **hash** like `4acb9f0...`; the first 7 characters are usually enough.
- **Working tree**: the files you see and edit. **Staging area** (index): the changes
  you have marked for the next commit with `git add`.
- **Branch**: a movable name pointing at a commit. Committing on a branch moves the
  name forward. Branches let several lines of work exist at once.
- **HEAD**: which branch (and so which commit) you are on now.
- **Remote**: another copy of the repo, here `origin` = GitHub.
  `origin/main` is your copy's memory of where `main` is on GitHub.
- **Merge**: combine another branch's commits into the current one. If both changed
  different things, git does it automatically; if both changed the same lines, you
  get a **conflict** to resolve by hand.
- **Merge commit**: a commit with two parents, recording that two lines joined.
- **Tag**: a fixed name for one commit (unlike a branch, it never moves). Used for
  releases (`v1.0`) and, here, for safety snapshots.
- **Pull request (PR)**: on GitHub, a request to merge one branch into another, with
  the diff, a description, review comments and CI results. **Draft** PRs are marked
  "not ready to merge yet".

## 8.2 Everyday commands

```
git status                     what changed, what is staged, which branch
git diff                       exact line changes not yet staged
git add <file>                 stage a file
git commit -m "message"        commit the staged changes
git log --oneline --graph --all  the history as a picture
git switch <branch>            go to a branch
git switch -c <new-branch>     create a branch here and go to it
git fetch                      download new commits from GitHub (changes nothing local)
git pull                       fetch + merge into the current branch
git push                       upload your commits
git merge <branch>             merge <branch> into the current branch
git tag -l                     list tags
git show <commit>              one commit's message and changes
git log --follow <file>        a file's history, across renames
```

## 8.3 Good commits

Look at `git log` on `refactor/app-loop` for examples. A good commit:

- does **one thing** (one bug fix, one refactor), so it is easy to review and to undo;
- **builds and passes the tests** on its own (every commit in this overhaul was
  checked; see `docs/FINAL_REPORT.md`);
- has a message with a short summary line (prefix like `fix:`, `refactor:`, `docs:`,
  `ci:`, `build:`, `test:`, `perf:`: a common convention called "Conventional
  Commits"), then a body explaining **why**, not just what.

## 8.4 This repository's history, read with fresh eyes

Run `git log --oneline --graph --all` and follow along.

1. **March 2026: the beginning.** "Day 1: Initialized Stardust 3D Engine", then
   velocity, orbits, a mass slider, delta time, splitting into Planet/Physics files,
   play/pause and picking, 3D models and lighting, the nine-body system with the Moon,
   mobile controls. All on `main`.
2. **March 2026: a README PR** from an automated assistant (`claude/create-readme-file`),
   merged as PR #1.
3. **"Sync main branch with StellAR"** (`8ba8b83`): `main` became the **Android/mobile**
   line, with a JNI bridge to a separate host app.
4. **June 2026: the desktop line.** Branch `desktop` started from `8ba8b83` with
   "Clean desktop branch without Android and raygui", then the HDR sky, bloom,
   tracking, vsync, HUD polish.
5. **`release/sd-winApp`** was created from `desktop`. A "dummy commit" was merged into
   it through PR #2.
6. **July 2026:** README edits on both lines; PR #4 merged `main` (the Android line)
   *into* `release/sd-winApp`, keeping the desktop code (so the merge only brought
   README history across). PR #5 merged `main` into `release/sd-androidApp`.

Findings, as an engineer reviewing it would see them (`docs/AUDIT.md` item 21):

- The default branch `main` is the **mobile** line, but the portfolio piece is the
  desktop app. Visitors to the repository see the wrong one first.
- `desktop` and `release/sd-winApp` differ only in the README; `main` and
  `release/sd-androidApp` have identical files. Four branches, two real lines.
- Merging the Android `main` into the Windows release mixed two unrelated lines'
  histories.
- No tags, so there is no named "this is version 1".
- The 93 MB sky and ~46 MB of models are in normal history, so every clone is about
  100 MB.

None of this was rewritten: your resume links point at `release/sd-winApp`, and
rewriting published history (force-pushing) breaks other people's copies and links.

## 8.5 What was done during the overhaul

- **Safety first:** before any change, every branch's current commit was tagged
  `archive/2026-11-02/<branch>` and pushed. Whatever happens, `git switch --detach
  archive/2026-11-02/release/sd-winApp` gets the old code back.
- **Short-lived topic branches**, each started from the previous, each with one
  theme:

  ```
  release/sd-winApp (4acb9f0, untouched)
    └─ chore/cmake-build        build system, layout, tooling, CI
        └─ feature/simulation-core   physics library, tests, physics fixes
            └─ refactor/app-loop      app fixes, HDR loader, screenshots
                └─ docs/learning-guide    README and all documentation
  ```
- Branch names follow a common `type/short-description` convention.
- Each branch gets a pull request into `release/sd-winApp`. Because they are stacked,
  they must be merged **in order** (1, 2, 3, 4) and as **merge commits** (not
  "squash" or "rebase"), so later PRs still recognise the earlier commits.
  `docs/GIT_GUIDE.md` walks through it.

## 8.6 The recommended structure going forward

This is what a small professional team would do, and what `docs/GIT_GUIDE.md`
suggests you do yourself when you are ready:

- **One default branch that is the product.** Make the desktop line the default
  branch on GitHub (Settings > Branches), so visitors see it. Keep the name
  `release/sd-winApp` if your links need it, or rename later and update the links.
- **Topic branches for every change**, merged by pull request, deleted after merging.
- **Tags for releases**: `git tag -a v1.0.0 -m "First public release"` on the merged
  result, so your resume can link to a version that never changes.
- **The Android line** stays on `release/sd-androidApp` (identical to `main` today).
- **Archive what is dead**: `desktop` is superseded; after merging, it can be deleted
  (the `archive/2026-11-02/desktop` tag keeps it recoverable).
- Consider **Git LFS** for new large assets (`docs/ROADMAP.md` item 9).

## Exercises

1. Run `git log --oneline --graph --all` and draw the history on paper. Mark where
   each branch points.
2. Find the commit that introduced the fixed timestep (hint:
   `git log --oneline --all -- src/sim/FixedTimestep.h`). Read it with `git show`.
3. Make a practice branch, change a comment, commit it, look at `git log`, then switch
   back and delete the branch with `git branch -D practice`.
4. Explain to a friend the difference between a branch and a tag.
5. Why must the stacked PRs be merged with "Create a merge commit" and not "Squash and
   merge"? (Answer in `docs/GIT_GUIDE.md`.)
