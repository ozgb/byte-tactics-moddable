---
name: progress
description: Refresh the clean-room progress page on GitHub Pages (one conduit per module from byte-tactics-moddable to ozgb/btm). Use when asked to update the progress page, or under /loop.
---

This branch, `gh-pages`, is the site: GitHub Pages serves its root at https://ozgb.github.io/byte-tactics-moddable/ and redeploys on every push. `index.html` draws `data.json`; this skill rewrites `data.json`. Work in the branch's own worktree, next to the moddable clone.

1. `uv run .claude/skills/progress/collect.py`. It writes `data.json` at the branch root, reading GitHub (#2, #180, the spec issues, ozgb/btm's specs, crates and pull requests) and the moddable clone this worktree belongs to (`tests/replay/`, the `interfaces-v*` tags).
2. `git add data.json && git commit -m "Update: Refresh the progress data" -m "Assisted-by: <agent>:<model>"`, then `git push origin gh-pages`. Pushes are fast-forward only; never force-push.
3. Report one line: the collector's output, plus any conduit whose state changed since the last run in this session.

Under `/loop` with no interval, wake again in about an hour (ScheduleWakeup, 3600 s), and mark the tick `noop` when no conduit changed.

A conduit is lit when one of its btm pull requests was updated in the last 48 hours or its spec issue is claimed; the other states are worked out in `collect.py`. When a new spec issue or btm crate appears, add it to `FOUNDATIONS` or `SUBSYSTEMS` there. A change to `index.html` is committed and pushed the same way.
