# How the lead and the maintainer work together

Two Claude sessions run the plan in #2:

- the **lead** (`/lead`) implements issues, opens pull requests and gets them to a mergeable state;
- the **maintainer** (`/maintainer`) reviews and merges pull requests, and keeps the issues true to the plan and to the work already merged.

The plan in #2 sets the goals and the decisions. The human makes every major architecture decision ("Major architecture decisions" below); the maintainer settles anything else the plan leaves open.

They share one GitHub account, and they talk only through GitHub: pull request reviews, review comments, issue comments and the labels below. This file is the contract between them. Both roles also follow `AGENTS.md`.

## Directories

Each role runs in its **own clone** of this repository, with `origin` pointing at it, started with its role in the environment:

```sh
cd ~/source/repos/byte-tactics-moddable-lead        && BT_ROLE=lead claude
cd ~/source/repos/byte-tactics-moddable-maintainer  && BT_ROLE=maintainer claude
```

- The clone root stays a clean checkout of `moddable`. Nothing is edited or committed there.
- The lead works only in `.worktrees/issue-<N>` (`tools/worktree.sh <N>`), one per issue, and gives subagents its absolute path.
- The maintainer reviews only in `.worktrees/review-<PR>` (`tools/worktree.sh --pr <PR>`).
- Each clone links `toolchain/` and `orig/TotalA.exe` from an existing setup or gets them from `tools/setup_toolchain.sh`; the worktree script links them into every worktree.

`BT_ROLE` turns on a hook (`tools/role_guard.py`, registered in `.claude/settings.json`) that blocks file edits and history-changing git commands outside the role's own worktrees, for the session and its subagents alike. Sessions without `BT_ROLE` are unaffected.

## Signing

GitHub shows both roles as the same account, so every comment, review and reply starts with the role:

```
Lead: ...
Maintainer: ...
```

## Reviews

The account can't approve or request changes on its own pull requests, so the maintainer posts every review as a plain comment review whose first line is the **verdict**:

```
Maintainer review of <full head SHA>: approve
Maintainer review of <full head SHA>: changes
```

The rest of the body lists the findings, and specific findings go as inline comments on the lines they concern. A `changes` verdict lists each thing that must change; anything only suggested says "optional".

The latest verdict on a pull request, and its current head:

```sh
gh api repos/ozgb/byte-tactics-moddable/pulls/<PR>/reviews \
  --jq '[.[] | select(.body | startswith("Maintainer review of"))] | last | (.body // "no verdict") | split("\n")[0]'
gh api repos/ozgb/byte-tactics-moddable/pulls/<PR> --jq .head.sha
```

A pull request **needs review** when it is open, not a draft, and its head SHA differs from the SHA in its latest verdict (or it has none). Pushing new commits is how the author asks for another review: no label or mention is needed.

A pull request body names the gate tier it ran: `--tier` (the scenarios of `GATE_TIER`, the gate of every change the gate applies to) or the full `--check` (the gate only of a change that regenerates the stored reference data, as `reference/README.md`, "When the full suite is the gate", says). A body that names only `--tier` for such a change is sent back. The play-tests are not part of a pull request's gate: they run with the full `--check` on `moddable` when `uv run tools/fullgate.py due` passes.

## Branches behind `moddable`

Only the maintainer merges. A branch doesn't have to contain `moddable`'s head to be merged: what matters is that the tree the maintainer tested is the tree that lands. Updating a branch changes its head, which sends it back for review, so nobody updates one without a reason.

- **Merge as it stands** when GitHub reports no conflicts and the commits the branch lacks touch none of its files and none of the tools its gate runs. Check with `git diff --name-only $(git merge-base origin/moddable <head>) origin/moddable`.
- **Test the merge** when those commits do overlap the pull request but git merges them cleanly and the pull request is approved on content. The maintainer merges `origin/moddable` into the head in a review worktree, runs the pull request's data checks and tests on that tree, and merges through GitHub if they pass. The lead pushes no update and no extra CI round runs.
- **Update first** when there are conflicts, when the merged tree needs regenerated files (a data `--check` or a counts test fails on it), or when the pull request's gate is a trace gate whose inputs the commits touch. The maintainer makes this a must-change finding naming the overlapping commits. The lead merges `origin/moddable` into the branch, re-runs the gate and pushes. Merge rather than rebase: `git-safe push` refuses force-pushes.

## Labels

| Label | Set by | Meaning | Cleared by |
|---|---|---|---|
| `needs-grooming` | the lead (or anyone) | An issue needs a decision the plan doesn't make, or has parts that must land in order ("Issue size"). A comment says what's missing. | the maintainer, after fleshing out or splitting it |
| `needs-human` | the maintainer, alongside `needs-grooming` | The issue waits on a major architecture decision. A comment asks the question. | the human, after answering it |

## Issue size

Every pull request pays a fixed cost: a gate run, a review round and, when it changes generated `data/` or `reference/` files, a regeneration after each merge ahead of it. Size issues so that cost is paid as rarely as the work allows.

- **Size is never a reason to split.** Agents write and review large pull requests well, so a large issue whose decisions the plan already makes is done in one pull request, in as many commits as it needs.
- **Split only along a real boundary:** one part needs a decision or an interface that another part settles first, or the parts touch disjoint files and gain from running in parallel. Never split to fill the queue or to make issues small.
- **Split once.** A sub-issue isn't split again; if one turns out to need it, regroup its parent instead.
- **Combine siblings.** Open issues that touch the same files and run the same gate become one issue; close the others as merged into it. A pull request may also close several such issues at once (`Closes #A`, `Closes #B`).
- **Leftover work is triaged, not filed by default** (the maintainer's "Reconcile after a merge"): most of it is dropped or folded into an open issue.

## Major architecture decisions

The human makes every major architecture decision; the lead and the maintainer recommend. A decision is **major** when it:

- adds to, revises or reverses a goal or decision in #2;
- changes how a contract other work builds on works: the seams, the state block's organisation, the host interface, the module ABI, the trace format or the mod interface (adding an entry to one is routine);
- sets how the engine is structured: subsystem boundaries, scheduling, threading or the module runtime;
- would be expensive to undo once merged work depends on it.

Treat a borderline decision as major.

To consult the human, the maintainer:

1. Comments on the issue with the question, the options, their trade-offs and a recommendation, and labels it `needs-human` alongside `needs-grooming`. The issue waits, and both roles carry on with other work.
2. Names each waiting issue in its end-of-cycle report.
3. Once the human has answered and removed `needs-human`, records the answer and its reason on the issue and in #2's decisions, then finishes grooming the issue.

A pull request that makes a major decision the human hasn't made gets a `changes` verdict, and the maintainer consults the human on its issue.

## Issue states

- **Available:** open, no assignee, no open blockers (`blocked_by` API, mirrored in the "Blocked by" line), not labelled `needs-grooming`.
- **Claimed:** assigned, with a "Lead: Claimed" comment. The lead holds up to 10 claims at a time, counting each from its claim until its pull request merges or it is released. Claims that change the same files, such as the replay schema or the generated `data/` files, still merge one at a time, oldest pull request first.
- **Done:** closed by the merged pull request's `Closes #N`.

Blockers are kept in two places that must agree: the `blocked_by` dependency API, which tools query, and the "**Blocked by:**" line at the top of the body, which people read. Whoever changes one changes the other.

## Disagreements

The lead may reply to a finding with reasons instead of a change. The maintainer then either accepts and withdraws it in the next review, or keeps it. After that second round the maintainer's call is final, and the review says why. The human settles a disagreement over a major architecture decision.
