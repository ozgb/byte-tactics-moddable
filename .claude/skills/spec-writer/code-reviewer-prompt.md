# Code reviewer prompt

The prompt of the code review session (#1218), which runs with `launch.py --review` on the implementer's branch, only on a head the module gate judged `equal`. Replace `<subsystem>` throughout, and `<base>` with the spec branch's head commit (which the implementer's branch is on) in the first round, and with the head the last round reviewed in each round after it.

```text
You are reviewing one subsystem of a game engine, a WebAssembly module in Rust, against its behavioural specification. Work only from what is in this machine: the module's crate at /work/<subsystem>/, its spec at /work/specs/<subsystem>.md (and /work/specs/TEMPLATE.md), the implementer's notes at /work/IMPLEMENTATION-NOTES.md, and the data kit at /data (read /data/README.md first; /data/docs/module-abi.md is the module ABI; /data/interfaces/ has the interface headers; /data/layout.json the state block layout; /data/tests/replay/<subsystem>/ recordings of the reference behaviour, format in /data/docs/replay-format.md). Do not try to find the original game's code or any other implementation of it.

The module already passes the gate on every recording and scenario. Your job is what the gate doesn't reach: a panic or trap on a path no recording takes, an out-of-bounds read or write, an overflow, a division by zero, a slot or branch the spec defines that the code gets wrong or leaves out, dead code, an import, export or data word the spec's manifest entry lists that nothing exercises or that the code uses differently, and any departure from the ABI (fuel metering, the size budget, the memory and window layout).

How:
- Run the code-review skill at high effort, `/code-review high`, on the whole crate /work/<subsystem>/ and on the branch's changes since <base> (`git -C /work diff <base>..HEAD`). Do not pass --comment or --fix. If the skill is not available, say so on the first line of the report and do the same review yourself.
- Judge only by what the spec and the kit say. Never write "the original does", "the reference does" or anything that guesses at code you have not seen: cite the spec section, the ABI section or the kit file each finding rests on.
- A finding IMPLEMENTATION-NOTES.md already answers is not a finding again, unless the answer is wrong by the spec or the kit: then raise it again and say why the answer doesn't hold.
- Change nothing and post nothing. /work is read-only; build or test only in a copy under /tmp. Do not submit to the gate: this session has none.

Write the report, in Markdown, to /srv/outbox/.review.tmp and then rename it to /srv/outbox/review.md (`mv`), so that it is never read half written: a numbered list of findings, each with its file and line, its severity (`bug`, `risk` or `nit`), what goes wrong and on which input, and the spec or kit text it rests on. The report's first line counts them by severity, exactly `Findings: B bug, R risk, N nit.` (`Findings: 0 bug, 0 risk, 0 nit.` when there are none). Keep it under 1000 words.
```

## A core

For the engine core (#1207), the prompt is this one. Replace `<groups left out>` as in [`core-implementer-prompt.md`](core-implementer-prompt.md), and `<base>` as above (the core specs' branch head in the first round).

```text
You are reviewing the engine core of a game engine, one native Rust program that hosts WebAssembly modules, against its behavioural specification. Work only from what is in this machine: the core's crate at /work/core/, its contract at /work/specs/engine-core.md and its scaffolding at /work/specs/engine-core-transition.md (and /work/specs/TEMPLATE.md), the implementer's notes at /work/core/IMPLEMENTATION-NOTES.md (and the other notes in /work/core/), and the data kit at /data (read /data/README.md first; /data/docs/core-gate.md is how a core is run and judged; /data/reference/core/ holds the gate's inputs; /data/docs/module-abi.md is the module ABI the core hosts modules by; /data/docs/trace-format.md the trace and its hash; /data/docs/sim-events.md the sim event queue; /data/docs/commands.md the commands and console lines; /data/runtime/vectors/README.md the runtime entries; /data/interfaces/ the interface headers). Do not try to find the original game's code or any other implementation of it.

The core gate already judges the core `equal` on every group but <groups left out>. /work/wind_meteors/ is the module the core hosts and is not under review. Your job is what the gate doesn't reach: a panic, trap or abort on a path no vector takes, an out-of-bounds read or write (in the core, or into a hosted module's memory), an overflow, a division by zero, unbounded memory or time on an input the specs allow, a group, call, rule or branch the specs define that the code gets wrong or leaves out, dead code, any departure from the module ABI the core must enforce (fuel metering, the size budget, the memory and window layout, the imports and exports a module may have, what the loader must refuse), and anything that would keep the program from being one static executable.

How:
- Run the code-review skill at high effort, `/code-review high`, on the whole crate /work/core/ (its source and its tools, not target/), and on the branch's changes since <base> (`git -C /work diff <base>..HEAD -- core/`). Do not pass --comment or --fix. If the skill is not available, say so on the first line of the report and do the same review yourself.
- Judge only by what the specs and the kit say. Never write "the original does", "the reference does" or anything that guesses at code you have not seen: cite the spec section, the ABI section or the kit file each finding rests on.
- A finding the notes already answer is not a finding again, unless the answer is wrong by the specs or the kit: then raise it again and say why the answer doesn't hold.
- Change nothing and post nothing. /work is read-only; build or test only in a copy under /tmp. Do not submit to the gate: this session has none.

Write the report, in Markdown, to /srv/outbox/.review.tmp and then rename it to /srv/outbox/review.md (`mv`), so that it is never read half written: a numbered list of findings, each with its file and line, its severity (`bug`, `risk` or `nit`), what goes wrong and on which input, and the spec or kit text it rests on. The report's first line counts them by severity, exactly `Findings: B bug, R risk, N nit.` (`Findings: 0 bug, 0 risk, 0 nit.` when there are none). Keep it under 1500 words.
```
