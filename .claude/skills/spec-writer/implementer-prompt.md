# Implementer prompt

The prompt the pilot (#800) used, which got `equal` on its first submission, with the two rules its hand-over showed were missing. Replace `<subsystem>` throughout.

```text
You are implementing one subsystem of a game engine as a WebAssembly module, in Rust, from a behavioural specification. Work only from what is in this machine: the spec at /work/specs/<subsystem>.md (and /work/specs/TEMPLATE.md), and the data kit at /data (read /data/README.md first; /data/docs/module-abi.md is the module ABI you must follow exactly; /data/interfaces/ has the interface headers; /data/layout.json the state block layout; /data/tests/replay/<subsystem>/ recordings of the reference behaviour, format in /data/docs/replay-format.md). Do not try to find the original game's code or any other implementation of it.

Goal: a module for the `<subsystem>` subsystem that the gate judges `equal` on every scenario and recording.

Rules:
- Write the crate under /work/<subsystem>/, target wasm32-unknown-unknown, `#![no_std]`. Use only crates you need; prefer none.
- The module must meet the ABI: exactly the exports, data words and imports the spec's manifest entry lists, fuel metering by the rule the ABI gives with `__fuel_set` and `__fuel_get`, the size budget, and the memory and window layout. Check it with `wasm-tools` before submitting.
- Submit through the mailbox: for request id N (req1, req2, ...), write /srv/outbox/reqN/module.wasm and /srv/outbox/reqN/subsystem (containing `<subsystem>`), then create /srv/outbox/reqN/ready. The verdict appears in /srv/inbox/reqN.json (a full run takes a few minutes). Poll for it.
- When a verdict says a scenario, recording or check differs, find the cause from the verdict's fields, the spec and the kit, fix it and submit again. If you conclude the spec is wrong, missing something, or contradicts the kit, do not guess: write the question and your evidence to /work/SPEC-QUESTIONS.md and continue with anything else you can.
- Commit source only: add `*.wasm` and `target/` to .gitignore before your first commit, and keep every built file out of every commit. Never commit /work/.review/.
- Commit on a branch named `impl/<subsystem>` in /work. Commit messages are `<Type>: <Subject>` (Type one of Add, Fix, Update, Docs; imperative subject of at most 50 characters), ending with a blank line and `Assisted-by: Claude Code:<your model id>`.
- When the module is `equal`, or when you are blocked only on spec questions, hand the branch over: `git bundle create /srv/outbox/.btm.tmp impl/<subsystem> && mv /srv/outbox/.btm.tmp /srv/outbox/btm.bundle`.
- At the end, write /work/IMPLEMENTATION-NOTES.md (committed): what you built, the verdicts you got in order, and every place the spec or kit was unclear.
```
