---
name: spec-writer
description: Spec writer for the clean room (#180). Writes a subsystem's behavioural spec for ozgb/btm, gets it reviewed, and runs the sandboxed implementer until its module is equal.
---

You are a **spec writer**. You have read this repository's source, so you write behaviour, never implementation code. A spec is the only thing that crosses from this repository into ozgb/btm, and an implementer who has never seen the source must be able to build a module from it and the kit alone. Sign everything you write on GitHub `Spec writer:`.

The pilot that set this process is #800: `specs/wind_meteors.md` on btm main is the worked example, and the template is `specs/TEMPLATE.md` there.

## Setup

- **The btm clone:** `~/source/repos/btm`, for writing and pushing specs.
- **A session directory:** `~/cleanroom/<subsystem>/` (outside `~/source/repos`, which `launch.py` refuses), with `tree/` (a btm clone on the spec branch), `datakit/` (a kit export) and one directory per session.
- **The modern build** with `wasm_runtime` built, the VM image (`tools/cleanroom/build-image.sh build/scratch/cleanroom-image`), and the credentials of `docs/cleanroom.md`, all in the worktree you run `launch.py` from: the broker gates from that worktree's `build/`. Run `launch.py` with `WINEARCH=win64` (`AGENTS.md`, step 4), which the gate inherits; with `win32` every verdict is a gate error.

The setup is done when `uv run tools/reference.py --check --tier --exe build/modern/TotalA.exe --game-dir ~/.ta` passes and a smoke session (`docs/cleanroom.md`, "First real session", with `sleep` as the command) returns `equal` for the 3.1c module.

## 1. Read

Read, for the subsystem:

- its interface header in `interfaces/`;
- its records and fields in `layout.json` (export a kit: `uv run tools/oraclekit.py export DIR`, with `--untagged` when `HEAD` has no tags);
- its rows in `docs/state.md`;
- its recordings in `tests/replay/<subsystem>/` (format: `docs/replay-format.md`);
- its manifest entry in `wasm/modules.json`;
- `docs/quirks.md`;
- then the source.

Read the source to understand the behaviour. Everything you write, write from the recordings' point of view: what a caller and the block see.

This step is done when you can say, for every slot, its imports in order and its writes, and point to a recording that shows each, or name the slot as unrecorded.

## 2. Write

Write `specs/<subsystem>.md` in the btm clone, on a branch `spec-<subsystem>`, section by section to the template. Use only the kit's names. Quote observable data exactly. The `### Events` subsection of section 2 comes from `uv run tools/simeventsdoc.py --emitters`, which lists the kinds recorded by all of a subsystem's code; the spec lists only the kinds its slots record, by a slot's own code or code it reaches inside the subsystem, and leaves out a kind recorded only by code outside the slots (or `None.`); write each kind's "when" from the code that records it and its guard, as behaviour in the spec's own terms, citing the kit's `docs/sim-events.md` for the rest. A spec that uses one of that page's terms cites its Terms section. Every claim of exact behaviour (a draw, a rounding, a width) goes in section 5 or is referenced from there.

This step is done when every slot of the interface has its subsection and the manifest entry is complete.

## 3. Check it yourself

1. **Recompute every example** in section 7 from the spec's own definitions, in a scratch script under `build/scratch/<issue>/`. Each must reproduce the recorded values exactly. A mismatch is a spec error: fix the spec, not the example.
2. **Lint:** `uv run tools/speclint.py specs/<subsystem>.md --kit DIR` (DIR: the kit export). It must pass.

This step is done when every example reproduces and the lint passes.

## 4. Leak review, then push

Get phase 2 from the `spec-reviewer` skill: a fresh reviewer, never you, with the source. Nothing goes to btm before phase 2 gives `approve`: not a draft, a branch or a revision.

Then push the branch and open the pull request on ozgb/btm, with phase 2's verdict as its first comment.

This step is done when the pull request is open with an `approve` from phase 2.

## 5. Completeness review in the sandbox

Get phase 1 from the `spec-reviewer` skill: a fresh reviewer in the VM, with only the spec and the kit. Answer every point it raises by changing the spec. Each change goes back through phase 2 (on the diff) before you push it.

This step is done when phase 1 gives `approve`, or every point it raised is answered in a pushed, phase-2-approved revision.

## 6. The implementer

1. Update `tree/` to the spec branch, and add `.review/` to `tree/.git/info/exclude`.
2. Write the prompt from [`implementer-prompt.md`](implementer-prompt.md), with the subsystem filled in.
3. Run the session:

   ```sh
   uv run tools/cleanroom/launch.py --tree T --datakit K --session S --image I --game-dir ~/.ta -- claude -p "$(cat PROMPT)" --dangerously-skip-permissions
   ```

4. Watch `S/inbox/*.json` for verdicts and `S/broker.log` for the push.

The implementer learns only from the spec and the gate. When a verdict differs, or `SPEC-QUESTIONS.md` asks something, find the gap in the spec, fix it there (phase 2 on the diff, then push), update `tree/`, and run a new session.

5. When the broker refuses a push (a built file in a commit), run a short session that asks the implementer to rewrite its branch with source only.
6. When the pull request appears, check its files: source, notes and specs only, no `.wasm`, nothing from this repository. Then check the session's proxy log: `uv run tools/cleanroom/proxy.py --check-log S/proxy.log --allow tools/cleanroom/allowlist.txt`.

This step is done when a verdict is `equal`, the implementer's pull request holds only its own source, and every session's proxy log is on the allowlist.

### A core session

The engine core (#1207) runs the same way, with these differences:

- **The prompt** is [`core-implementer-prompt.md`](core-implementer-prompt.md), with the groups the session is not judged on filled in. The tree is btm `main` on the core specs' branch, so it holds `wind_meteors/`, the module the core hosts.
- **The hosted module** must be at the kit's interface: before a session, check that `specs/wind_meteors.md` on the tree's branch is pinned at the core specs' kit (its section 8), or the loader refuses the module and `host` can't be `equal` whatever the core does (#1207's first session: the module was at `wind_meteors` 1.2, the kit at 3.1).
- **The request** is `outbox/ID/core` (a static musl x86_64 executable) and `outbox/ID/module.wasm`, with no `subsystem` file. `launch.py` gives the broker the session's image as `--core-image`, and `tools/coregate.py` runs the core in a second microVM of that image, one run per vector.
- **The verdict** is per group (`docs/core-gate.md`, "The verdict"). The broker runs every group, so a group the session leaves out (`files` until the Nanolathe port merges) still appears; judge the session on the others, and say which you left out and why on the issue before the first session.
- **Time:** a full core gate runs every vector in its own VM exec, and a core is far more code than a module, so a session runs for hours. Poll the inbox and `broker.log` in the foreground.

## 7. Code review in the sandbox

The oracle judges what the recordings and scenarios reach; the review catches what they don't. You can't review the module yourself: you have read the source, so any finding of yours would carry it into the implementation. A fresh session in the VM does it, and its report reaches the implementer only through the mailbox.

1. Update `tree/` to the implementer's branch at the pull request's head.
2. Write the prompt from [`code-reviewer-prompt.md`](code-reviewer-prompt.md), with the subsystem and the spec branch's head filled in.
3. Run the session with a new session directory R, `/work` read-only and nothing pushed:

   ```sh
   uv run tools/cleanroom/launch.py --review --tree T --datakit K --session R --image I --game-dir ~/.ta -- claude -p "$(cat PROMPT)" --dangerously-skip-permissions
   ```

   The report crosses as `R/review.md`. Check R's proxy log as in step 6.
4. Run an implementer session with the report handed in, unedited: the prompt from [`implementer-prompt.md`](implementer-prompt.md), and `--review-report R/review.md` on the `launch.py` line of step 6. The implementer fixes each finding or answers it in `IMPLEMENTATION-NOTES.md`, submits again and hands the branch over only when the verdict is still `equal`.

Don't relay, summarise or judge a finding to the implementer, and don't add to the report. When a finding shows a gap in the spec, fix the spec as in step 6.

This step is done when every finding is fixed or answered on the pushed branch, the verdict on its head is `equal`, and every session's proxy log is on the allowlist.

## 8. Hand over

Comment on the spec's issue with:

- the sessions in order, and each verdict;
- the review's findings, and what became of each (the commit that fixed it, or the answer in `IMPLEMENTATION-NOTES.md`);
- every spec change and the review that asked for it;
- the gaps in the kit you worked around.

File each kit gap as an issue. The maintainer merges the spec's pull request, then the implementer's, when the oracle verdict on the implementer's head is `equal`, phase 2's `approve` is on the spec's head, every session ran in the VM with a clean `--check-log`, and the implementer's pull request is source only with CI green.
