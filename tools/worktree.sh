#!/usr/bin/env bash
# Make a separate working copy, so the lead, the maintainer and their
# subagents never touch the clone root or each other's files:
#
#   tools/worktree.sh 12        # .worktrees/issue-12 on branch issue-12, from origin/moddable
#   tools/worktree.sh --pr 48   # .worktrees/review-48 at pull request 48's head (detached)
#   tools/worktree.sh --tip     # .worktrees/review-tip at origin/moddable (detached), for the nightly full gate
#
# Re-running --pr or --tip moves an existing review worktree to the current
# head. The compiler, the original exe and the Ghidra export are not
# in git, so they are linked in from the clone root.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [ "${1:-}" = "--tip" ]; then
    DIR="$ROOT/.worktrees/review-tip"
    git -C "$ROOT" fetch -q origin moddable
    SHA="$(git -C "$ROOT" rev-parse origin/moddable)"
    if [ -d "$DIR" ]; then
        git -C "$DIR" checkout -q --detach "$SHA"
    else
        git -C "$ROOT" worktree add -q --detach "$DIR" "$SHA"
    fi
elif [ "${1:-}" = "--pr" ]; then
    PR="${2:?usage: tools/worktree.sh --pr <pull request number>}"
    PR="${PR#\#}"
    DIR="$ROOT/.worktrees/review-$PR"
    git -C "$ROOT" fetch -q origin "pull/$PR/head"
    SHA="$(git -C "$ROOT" rev-parse FETCH_HEAD)"
    if [ -d "$DIR" ]; then
        git -C "$DIR" checkout -q --detach "$SHA"
    else
        git -C "$ROOT" worktree add -q --detach "$DIR" "$SHA"
    fi
else
    N="${1:?usage: tools/worktree.sh <issue number> | --pr <pull request number> | --tip}"
    N="${N#\#}"
    DIR="$ROOT/.worktrees/issue-$N"
    if [ ! -d "$DIR" ]; then
        git -C "$ROOT" fetch -q origin moddable
        git -C "$ROOT" worktree add -q -b "issue-$N" "$DIR" origin/moddable
    fi
fi

ln -sfn "$ROOT/toolchain" "$DIR/toolchain"
mkdir -p "$DIR/orig" "$DIR/build"
ln -sf "$ROOT/orig/TotalA.exe" "$DIR/orig/TotalA.exe"
[ -d "$ROOT/build/ghidra" ] && ln -sfn "$ROOT/build/ghidra" "$DIR/build/ghidra"
echo "$DIR"
