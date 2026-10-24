# Status

Progress updates, newest first. Each entry is headed by the date and the
`moddable` commit it describes, and covers what changed since the entry
before it.

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
  names the first tick, phase and field that differ.
  Explore: [`reference/README.md`](reference/README.md),
  [`reference/scenarios/`](reference/scenarios),
  [`src/harness/`](src/harness), [`tools/reference.py`](tools/reference.py),
  [`tools/tracediff.py`](tools/tracediff.py).
- **Bit-exact maths**
  ([#4](https://github.com/ozgb/byte-tactics-moddable/issues/4)): `rand`,
  `qsort`, the assembly trig and the x87 float spots give identical results
  on a modern compiler.
  Explore: [`lib/bitexact/`](lib/bitexact) and its
  [README](lib/bitexact/README.md).
- **Modern build**
  ([#5](https://github.com/ozgb/byte-tactics-moddable/issues/5)): clang-cl,
  matching the original MSVC 5 reference on 27 of 27 scenarios. The
  `reference` branch is frozen, and the modern build is now the gate.
  Explore: [`cmake/clang-cl-i686.cmake`](cmake/clang-cl-i686.cmake),
  [`CMakeLists.txt`](CMakeLists.txt),
  [`docs/reference-traces.md`](docs/reference-traces.md).
- **State inventory and coupling map**
  ([#6](https://github.com/ozgb/byte-tactics-moddable/issues/6)): every piece
  of game state, and which subsystem reads and writes it.
  Explore: [`docs/state.md`](docs/state.md),
  [`docs/subsystems.md`](docs/subsystems.md),
  [`data/state_classes.csv`](data/state_classes.csv),
  [`data/coupling.csv`](data/coupling.csv).
- **Subsystem seams**
  ([#8](https://github.com/ozgb/byte-tactics-moddable/issues/8)): every
  subsystem is called through a function table, and the interfaces are
  versioned (`interfaces-v1`), so a retype is a major ABI bump.
  Explore: [`interfaces/`](interfaces) (start with
  [`seams.h`](interfaces/seams.h)), [`src/seams/native.cpp`](src/seams/native.cpp),
  [`tools/abidiff.py`](tools/abidiff.py).
- **Quirk flags**
  ([#40](https://github.com/ozgb/byte-tactics-moddable/issues/40)): 3.1c
  quirks can be fixed behind flags that are off by default, so the default
  game stays 3.1c. Six flags so far, listed in
  [docs/quirks.md](docs/quirks.md).
  Explore: [`src/quirks/quirks.def`](src/quirks/quirks.def).
- **Host interface**
  ([#24](https://github.com/ozgb/byte-tactics-moddable/issues/24)): every
  platform call (files, window, input, sound, network, display, movies) goes
  through one host layer, and there is a headless host.
  Explore: [`interfaces/host.h`](interfaces/host.h),
  [`src/host/win32/`](src/host/win32),
  [`tools/hostlint.py`](tools/hostlint.py).
- **Game state in one block**
  ([#183](https://github.com/ozgb/byte-tactics-moddable/issues/183),
  [#186](https://github.com/ozgb/byte-tactics-moddable/issues/186)): one
  definition per state type, with `SimGlobals`, `Game` and the sim containers
  in a shared, zeroed state block.
  Explore: [`src/state/`](src/state) (start with
  [`block.h`](src/state/block.h) and
  [`sim_globals.h`](src/state/sim_globals.h)).

### In progress

- **`Ref<T>`**
  ([#187](https://github.com/ozgb/byte-tactics-moddable/issues/187)):
  pointers stored in the state block become offsets, valid in every WASM
  module. Units, orders, weapons, game, paths, map and LOS are done.
  Explore: [`interfaces/ref.h`](interfaces/ref.h),
  [`tools/refcheck.py`](tools/refcheck.py).
- **Game data loading**
  ([#25](https://github.com/ozgb/byte-tactics-moddable/issues/25)):
  player-supplied files, case-insensitive lookup and a user directory.
  Explore: [`interfaces/files.h`](interfaces/files.h),
  [`src/files/`](src/files).

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

Further out: lockstep netcode
([#21](https://github.com/ozgb/byte-tactics-moddable/issues/21)), native
(wasmtime, SDL3 and OpenGL) and browser hosts
([#30](https://github.com/ozgb/byte-tactics-moddable/issues/30),
[#31](https://github.com/ozgb/byte-tactics-moddable/issues/31)), Lua hooks
([#42](https://github.com/ozgb/byte-tactics-moddable/issues/42),
[#43](https://github.com/ozgb/byte-tactics-moddable/issues/43)) and mod
manifests ([#39](https://github.com/ozgb/byte-tactics-moddable/issues/39)).
