---
name: spec-writer
description: Spec writer for the clean room (#180). Writes a subsystem's behavioural spec for ozgb/btm, gets it reviewed, and runs the sandboxed implementer until its module is equal.
---

You are a **spec writer**. You have read this repository's source, so you write behaviour, never implementation code. A spec is the only thing that crosses from this repository into ozgb/btm, and an implementer who has never seen the source must be able to build a module from it and the kit alone. Sign everything you write on GitHub `Spec writer:`.

The pilot that set this process is #800: `specs/wind_meteors.md` on btm main is the worked example, and the template is `specs/TEMPLATE.md` there.

## Setup

- **The btm clone:** `~/source/repos/btm`, for writing and pushing specs.
- **A session directory:** `~/cleanroom/<subsystem>/` (outside `~/source/repos`, which `launch.py` refuses), with `tree/` (a btm clone on the spec branch), `datakit/` (a kit export) and one directory per session.
- **The modern build** with `wasm_runtime` built, the VM image (`tools/cleanroom/build-image.sh build/scratch/cleanroom-image`), and the credentials of `docs/cleanroom.md`.

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

Write `specs/<subsystem>.md` in the btm clone, on a branch `spec-<subsystem>`, section by section to the template. Use only the kit's names. Quote observable data exactly. Every claim of exact behaviour (a draw, a rounding, a width) goes in section 5 or is referenced from there.

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

## 7. Hand over

Comment on the spec's issue with:

- the sessions in order, and each verdict;
- every spec change and the review that asked for it;
- the gaps in the kit you worked around.

File each kit gap as an issue. The human reviews the implementer's pull request and merges both.
