# Status

Progress updates, newest first. Each entry is headed by the date and the
`moddable` commit it describes, and covers what changed since the entry
before it.

## 2026-10-24 12:40 UTC, [dfc62a0d](https://github.com/ozgb/byte-tactics-moddable/commit/dfc62a0d)

The plan changed. This repository now prepares for a clean-room
re-implementation of the game in Rust, and the work that re-implementation
would replace is deferred to it.

### The new plan

- **Milestone 1**
  ([#2](https://github.com/ozgb/byte-tactics-moddable/issues/2), this
  repository): the decompiled 3.1c source, made deterministic, traced and cut
  into subsystems behind seams. It no longer aims to become the moddable
  engine itself. Its job is to be the oracle and the contract that Milestone 2
  is judged by.
- **Milestone 2**
  ([#180](https://github.com/ozgb/byte-tactics-moddable/issues/180),
  [ozgb/btm](https://github.com/ozgb/btm)): a clean-room re-implementation in
  Rust, under the MIT licence, with subsystems and mods as WASM modules over
  one shared wasm32 memory. Implementers never see the decompiled code: they
  work from behaviour-only specs, in a sandbox that can't reach this
  repository, and a module is accepted only when its traces equal the
  reference's. Milestone 2 issues are filed in ozgb/btm once the sandbox and
  the oracle kit are done.
- **Milestone 3** ([ozgb/btm#1](https://github.com/ozgb/btm/issues/1)): the
  features built on that engine: multiplayer, the native and browser hosts,
  portable graphics and sound, and mod support.

Why: everything built here is compiled from a decompilation of Cavedog's
code, so it can only be released as source. A build made entirely of
clean-room code can be released as binaries under the project's own licence.
And the netcode, hosts, renderer and mod features would be written again in
Rust anyway, so building them on the decompiled code now would be thrown
away.

### Done

- **clang-tidy on the lines a pull request adds**
  ([#364](https://github.com/ozgb/byte-tactics-moddable/issues/364)), the first
  part of readable source
  ([#363](https://github.com/ozgb/byte-tactics-moddable/issues/363)).

### Up next

- **WASM runtime and the wind and meteors pilot**
  ([#9](https://github.com/ozgb/byte-tactics-moddable/issues/9)), first. Its
  loader and module ABI are now also the boundary every Rust module is gated
  through, so it must load modules rustc emits, and the ABI gets a written
  spec.
- **The oracle**, so a trace match proves something without the source:
  - the trace format as a spec, with a second, independent hasher
    ([#384](https://github.com/ozgb/byte-tactics-moddable/issues/384));
  - scenario coverage per subsystem, and scenarios to raise it
    ([#385](https://github.com/ozgb/byte-tactics-moddable/issues/385));
  - replay recordings for every subsystem
    ([#10](https://github.com/ozgb/byte-tactics-moddable/issues/10),
    [#389](https://github.com/ozgb/byte-tactics-moddable/issues/389));
  - differential fuzzing against the reference
    ([#47](https://github.com/ozgb/byte-tactics-moddable/issues/47));
  - frame dumps and a sound log for presentation
    ([#339](https://github.com/ozgb/byte-tactics-moddable/issues/339),
    [#335](https://github.com/ozgb/byte-tactics-moddable/issues/335)).
- **The contracts:** a link check that each subsystem reaches the rest only
  through its seams
  ([#386](https://github.com/ozgb/byte-tactics-moddable/issues/386)), standing
  in for moving every subsystem to WASM here, and the rest of `Ref<T>`
  ([#187](https://github.com/ozgb/byte-tactics-moddable/issues/187)).
- **The clean room:**
  - a microVM sandbox for implementer agents, with no route to this
    repository, the original exe or any credential that reaches them
    ([#387](https://github.com/ozgb/byte-tactics-moddable/issues/387));
  - the oracle kit that crosses into ozgb/btm, and a gate runner that stays
    here and grades the modules sent to it
    ([#388](https://github.com/ozgb/byte-tactics-moddable/issues/388)).
- **Still going:** readable source
  ([#363](https://github.com/ozgb/byte-tactics-moddable/issues/363)), which
  helps the spec writers, and cross-host determinism
  ([#19](https://github.com/ozgb/byte-tactics-moddable/issues/19)).

### Deferred to Milestone 3

48 issues, closed as not planned with the
[`milestone-3`](https://github.com/ozgb/byte-tactics-moddable/issues?q=label%3Amilestone-3)
label and indexed in
[#392](https://github.com/ozgb/byte-tactics-moddable/issues/392). Their text
stays here as design input; they describe the decompiled source, so each is
rewritten as a clean issue in ozgb/btm when Milestone 3 starts.

- each subsystem as a WASM module
  ([#11](https://github.com/ozgb/byte-tactics-moddable/issues/11) to
  [#18](https://github.com/ozgb/byte-tactics-moddable/issues/18));
- lockstep netcode, server-client mode and the network transport
  ([#21](https://github.com/ozgb/byte-tactics-moddable/issues/21),
  [#23](https://github.com/ozgb/byte-tactics-moddable/issues/23),
  [#28](https://github.com/ozgb/byte-tactics-moddable/issues/28)) and the
  player view
  ([#22](https://github.com/ozgb/byte-tactics-moddable/issues/22));
- game data loading, audio and video, and the OpenGL renderer
  ([#25](https://github.com/ozgb/byte-tactics-moddable/issues/25) to
  [#27](https://github.com/ozgb/byte-tactics-moddable/issues/27));
- the whole game as wasm32, the native and browser hosts, threads, the debug
  build and releases
  ([#29](https://github.com/ozgb/byte-tactics-moddable/issues/29) to
  [#34](https://github.com/ozgb/byte-tactics-moddable/issues/34));
- mods: manifests, loading, settings, hooks, Lua, COB extensions, Open
  Annihilation's hacks and UI modules
  ([#39](https://github.com/ozgb/byte-tactics-moddable/issues/39),
  [#20](https://github.com/ozgb/byte-tactics-moddable/issues/20),
  [#41](https://github.com/ozgb/byte-tactics-moddable/issues/41) to
  [#46](https://github.com/ozgb/byte-tactics-moddable/issues/46));
- the tree-wide format and lint passes
  ([#375](https://github.com/ozgb/byte-tactics-moddable/issues/375),
  [#376](https://github.com/ozgb/byte-tactics-moddable/issues/376)).

## 2026-10-24 07:17 UTC, [5732ae96](https://github.com/ozgb/byte-tactics-moddable/commit/5732ae96)

First entry, covering everything since the fork started.

The fork started on 2026-10-14. In its first 10 days, 143 pull requests
merged, 149 issues closed and about 1,200 commits landed.

The goal is to turn the matching decompilation of TA 3.1c into a moddable
engine where every game-logic subsystem is a swappable WASM module. The rule
that never bends is **conformity**: with no mods loaded, the game plays
exactly like 3.1c, bit for bit, tick for tick.

The plan, its decisions and its order are in
[issue #2](https://github.com/ozgb/byte-tactics-moddable/issues/2), with one
sub-issue per step. The mod-support design it builds on is
[issue #1](https://github.com/ozgb/byte-tactics-moddable/issues/1).

### Done

- **Conformity harness**
  ([#3](https://github.com/ozgb/byte-tactics-moddable/issues/3)): a test mode
  with fixed seeds and one step per frame, AI-vs-AI skirmishes, and a hash of
  the game state after every phase of every tick. When two builds diverge, it
  names the first tick, phase and field that differ. Getting there meant
  finding and removing every source of non-determinism in 3.1c's game logic:
  seeds from the clock, `rand()` shared with graphics and sound, per-thread
  random state, uninitialised memory, and reads of the wall clock and of
  render-only state. A game now replays exactly from the same initial state
  and seeds, which bit-for-bit conformity depends on.
  - [`reference/README.md`](reference/README.md)
  - [`reference/scenarios/`](reference/scenarios)
  - [`src/harness/`](src/harness)
  - [`tools/reference.py`](tools/reference.py)
  - [`tools/tracediff.py`](tools/tracediff.py)
- **Bit-exact maths**
  ([#4](https://github.com/ozgb/byte-tactics-moddable/issues/4)): `rand`,
  `qsort`, the assembly trig and the x87 float spots give identical results
  on a modern compiler.
  - [`lib/bitexact/`](lib/bitexact) and its [README](lib/bitexact/README.md)
- **Modern build**
  ([#5](https://github.com/ozgb/byte-tactics-moddable/issues/5)): clang-cl,
  matching the original MSVC 5 reference on 27 of 27 scenarios. The
  `reference` branch is frozen, and the modern build is now the gate.
  - [`cmake/clang-cl-i686.cmake`](cmake/clang-cl-i686.cmake)
  - [`CMakeLists.txt`](CMakeLists.txt)
  - [`docs/reference-traces.md`](docs/reference-traces.md)
- **State inventory and coupling map**
  ([#6](https://github.com/ozgb/byte-tactics-moddable/issues/6)): every piece
  of game state, and which subsystem reads and writes it.
  - [`docs/state.md`](docs/state.md)
  - [`docs/subsystems.md`](docs/subsystems.md)
  - [`data/state_classes.csv`](data/state_classes.csv)
  - [`data/coupling.csv`](data/coupling.csv)
- **Subsystem seams**
  ([#8](https://github.com/ozgb/byte-tactics-moddable/issues/8)): every
  subsystem is called through a function table, and the interfaces are
  versioned (`interfaces-v1`), so a retype is a major ABI bump.
  - [`interfaces/`](interfaces) (start with [`seams.h`](interfaces/seams.h))
  - [`src/seams/native.cpp`](src/seams/native.cpp)
  - [`tools/abidiff.py`](tools/abidiff.py)
- **Quirk flags**
  ([#40](https://github.com/ozgb/byte-tactics-moddable/issues/40)): 3.1c
  quirks can be fixed behind flags that are off by default, so the default
  game stays 3.1c. Six flags so far, listed in
  [docs/quirks.md](docs/quirks.md).
  - [`src/quirks/quirks.def`](src/quirks/quirks.def)
- **Host interface**
  ([#24](https://github.com/ozgb/byte-tactics-moddable/issues/24)): every
  platform call (files, window, input, sound, network, display, movies) goes
  through one host layer, and there is a headless host.
  - [`interfaces/host.h`](interfaces/host.h)
  - [`src/host/win32/`](src/host/win32)
  - [`tools/hostlint.py`](tools/hostlint.py)
- **Game state in one block**
  ([#183](https://github.com/ozgb/byte-tactics-moddable/issues/183),
  [#186](https://github.com/ozgb/byte-tactics-moddable/issues/186)): one
  definition per state type, with `SimGlobals`, `Game` and the sim containers
  in a shared, zeroed state block.
  - [`src/state/`](src/state) (start with [`block.h`](src/state/block.h) and [`sim_globals.h`](src/state/sim_globals.h))

### In progress

- **`Ref<T>`**
  ([#187](https://github.com/ozgb/byte-tactics-moddable/issues/187)):
  pointers stored in the state block become 4-byte offsets from the block's
  base instead of process addresses. That lets native code and WASM modules
  share one block while subsystems move to WASM one at a time, and lets the
  block be saved and restored at a different address. Units, orders, weapons,
  game, paths, map and LOS are done.
  - [`interfaces/ref.h`](interfaces/ref.h)
  - [`tools/refcheck.py`](tools/refcheck.py)
- **Game data loading**
  ([#25](https://github.com/ozgb/byte-tactics-moddable/issues/25)):
  player-supplied files, case-insensitive lookup and a user directory.
  - [`interfaces/files.h`](interfaces/files.h)
  - [`src/files/`](src/files)

### Up next

- **Readable source**
  ([#363](https://github.com/ozgb/byte-tactics-moddable/issues/363)): real
  names for the ~250 `FUN_xxx` functions and the `field_xx` and `unk*`
  members, files named after what they hold, and clang-tidy and clang-format
  in CI. Possible now that the byte match is gone.
- **WASM runtime**
  ([#9](https://github.com/ozgb/byte-tactics-moddable/issues/9)), with wind
  and meteors as the pilot module, then one subsystem at a time
  ([#11](https://github.com/ozgb/byte-tactics-moddable/issues/11) to
  [#18](https://github.com/ozgb/byte-tactics-moddable/issues/18)).
  - [`docs/module-abi.md`](docs/module-abi.md)

Further out: lockstep netcode
([#21](https://github.com/ozgb/byte-tactics-moddable/issues/21)), native
(wasmtime, SDL3 and OpenGL) and browser hosts
([#30](https://github.com/ozgb/byte-tactics-moddable/issues/30),
[#31](https://github.com/ozgb/byte-tactics-moddable/issues/31)), Lua hooks
([#42](https://github.com/ozgb/byte-tactics-moddable/issues/42),
[#43](https://github.com/ozgb/byte-tactics-moddable/issues/43)) and mod
manifests ([#39](https://github.com/ozgb/byte-tactics-moddable/issues/39)).
