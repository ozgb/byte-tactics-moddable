# Module ABI

This page defines what a WASM module must be and do for the loader to accept it and the engine to
call it, so that a module author who never sees this repository can build a conforming module in any
language. It describes behaviour and interfaces only. The numbers it gives are the ones in
`wasm/modules.json` (the game manifest) and `tests/wasm/modules/modules.json` (the fixture manifest)
today; the text names the manifest field each one is read from, and the manifest wins if a number
here goes stale.

Contents:

1. [Conventions](#1-conventions)
2. [The shared memory and table](#2-the-shared-memory-and-table)
3. [Building a module](#3-building-a-module)
4. [The manifest](#4-the-manifest)
5. [What the loader does and refuses](#5-what-the-loader-does-and-refuses)
6. [Slots](#6-slots)
7. [Argument kinds and the pointer rule](#7-argument-kinds-and-the-pointer-rule)
8. [Handles and the loader data words](#8-handles-and-the-loader-data-words)
9. [Imports](#9-imports)
10. [Interface majors, signatures and adapters](#10-interface-majors-signatures-and-adapters)
11. [Fuel](#11-fuel)
12. [No hidden state](#12-no-hidden-state)
13. [Errors and traps](#13-errors-and-traps)
14. [Worked example: the Rust fixture](#14-worked-example-the-rust-fixture)
15. [Judging a module](#15-judging-a-module)
16. [Appendix: values in the manifests](#16-appendix-values-in-the-manifests)

## 1. Conventions

- All values are little-endian. "i32", "f32" and "f64" are the WebAssembly value types; an address or an
  offset is an unsigned 32-bit value held in an i32.
- A **slot** is a function a module exports for the engine to call, one per entry of an interface
  (a function table the engine calls a subsystem through). A **module** implements the slots of one
  interface.
- An **import group** is a WebAssembly import module name other than `env`. A group is either an
  interface the module calls (`weapons`) or the engine services (`services`).
- The **loader** reads the manifest, creates the shared memory and table, and instantiates modules.
  The **host** is the engine that owns the loader: it defines the imports, calls the slots, and
  writes the loader data words. The two are separate only in this text; the in-repo host links the
  loader as a library.
- The reference toolchain is clang and wasm-ld at the version `tools.clang` of `wasm/modules.json`
  pins, `wasm-tools` at `tools.wasm-tools`, and (for the fixtures) rustc at `tools.rustc` of the
  fixture manifest. The engine is wasmtime; the fixtures are checked under the version in
  `tools.wasmtime` of the fixture manifest, and the game embeds the 50.0.0 C API on its Pulley
  interpreter. Behaviour that the document states does not depend on the engine's back end.
- "The reference's checks" are `tools/wasm_modules.py` (`build`, `check`, `imports`, `statics`,
  `adapters`) and `tools/wasm_fuel.py`. They are the executable form of the rules below. The data kit carries
  `wasm/modules.json` (the game manifest, section 4) and `tools/wasm_fuel.py` (the fuel pass, section 11);
  it does not carry `tools/wasm_modules.py`.

## 2. The shared memory and table

Every module runs in **one linear memory and one function table** that the loader creates before
any module is loaded. Modules do not create or grow them.

### 2.1 The memory

- The module imports it as `env.memory`. Its size is `memory_pages` of the manifest (4112 pages of
  65536 bytes in the game manifest, 269484032 bytes: the block's ceiling and the 1 MiB below it). Its minimum and maximum are both
  `memory_pages`, so it never grows and never moves, and an address stays valid for the whole run.
- A module declares its own minimum in its import. It must be at most `memory_pages`. It must not
  declare a maximum smaller than `memory_pages`. Linking with `--import-memory` and no
  `--max-memory`, as section 3 says, does both.
- The layout of the game manifest, from low addresses to high:

  | Region | Where | Read from |
  | --- | --- | --- |
  | unused, so that address 0 is never an object | below the lowest window (0x10000 in the manifests) | convention |
  | module windows, one per module, `window` bytes each | from each module's `data_base` | `window`, `modules[].data_base` |
  | scratch area, 65536 bytes | 0xF0000 (983040) | `scratch` = `[base, size]` |
  | the state block, 32 MiB to 256 MiB | 0x100000 (1048576) | `block_base`, `block_size`, `block_max` |

  The memory holds the block at its ceiling: `block_base + block_max` equals `memory_pages * 65536`.
  The block itself is as large as the match needs (`block_size` to `block_max`, a multiple of 65536):
  it starts at `block_base`, the engine tells modules
  only addresses inside it, and the bytes after its end are never read or written. A module must not
  assume the block ends at the end of the memory. A manifest without `block_max` has a block of
  `block_size` bytes only.
- Windows sit below the scratch area. The loader refuses a window that reaches the scratch base or
  the block base (section 5).
- **The state block** holds the whole simulation state. The engine allocates and owns it; modules
  read and write it through the addresses the engine gives them. A module keeps nothing between
  ticks outside the block (section 12).
- **The scratch area** is where a native host puts a copy of a slot argument that does not live in
  the block, for the length of one call (section 7). A module never allocates from it and never
  keeps an address in it.

### 2.2 A module's window

A **window** is the `window` bytes (32768 in the game manifest) starting at the module's
`data_base`. The game manifest has an entry for each of the 15 modules, wind_meteors first and then the
14 subsystems in the manifest's order: entry *i* has its window at `0x30000 + i * 0x8000`
and its table range at `48 + i * 32`, so the windows fill `0x30000..0xA8000`, below the scratch area, and the
ranges fill the table. A module's data, bss and stack must lie entirely inside its window:

- `data_base` is the address wasm-ld is told to start the module's data at (`--global-base`, section 3).
  Data comes first, then bss.
- The stack comes after the data (`--no-stack-first`) and grows down from its top. Its size is
  `stack_size` of the manifest (4096 bytes). There is no guard below the stack: an overflow writes
  into the module's own statics and nothing traps. Size the work of a slot to fit.
- The module has no heap. The ABI provides no allocator; the freestanding headers of the reference
  build declare `malloc` and `free` but the module links without them.
- The loader zeroes the whole window before it instantiates the module, then the instance writes its
  active data segments. A module may rely on its bss and unwritten data being zero at instantiation.
- The windows of two modules must not overlap, and a window's end must not pass the scratch base.

### 2.3 The table

- The module imports it as `env.__indirect_function_table`. Its size is `table_size` of the manifest
  (528 entries in the game manifest, 64 in the fixture manifest), minimum equal to maximum, every entry
  empty (null) when the first module loads. A module declares a minimum of at most `table_size` and no
  maximum.
- Each module owns the range `[table_base, table_base + table_slots)` (`table_slots` is 32 in the
  game manifest, 16 in the fixtures). Ranges must not overlap.
- Linking with `--table-base=table_base` makes the module's function pointers (indirect call
  targets, virtual tables, a seam record of function pointers) start at `table_base`. The linker
  writes them as an active element segment, applied at instantiation, so they are in the table before
  any host call. **A module's element segments must stay inside its range.** The reference's build
  check refuses a module whose segments leave it; the loader does not look at segments.
- The loader also puts the module's slots in its range: slot *i* (the *i*-th name of the module's
  `exports`) goes to entry `table_base + i`, **only if that entry is still empty after
  instantiation** and the slot is not adapted (section 10). A module whose own function pointers
  already fill the range, as the game manifest's module does, gets no loader entries; the host calls
  such a slot by its export name, which is the only way the host ever calls a slot (section 6).
- A call through the table is a plain `call_indirect` in the calling module. The only table entries a
  module may call are in its own range. A module that wants a function of another module calls an
  import (section 9).
- An **adapted slot is not in the table**: the adapter is a host-side conversion and only the host's
  call reaches it. A module must not call another module's adapted slot through the table. If
  multi-module work needs that call, it places a trampoline in the table, a function that calls the
  adapted slot through the host.

## 3. Building a module

### 3.1 The fixed-base convention

Every module is linked at a fixed place. Its data base and table base are chosen when the module is
built, are recorded in the manifest, and the loader needs no relocation at load. A module is **not**
a dynamic-linking side module: it has no `dylink.0` section. It must be rebuilt, not just
re-registered, to move.

The link step takes these `wasm-ld` options, with `D` the module's `data_base`, `T` its `table_base`
and `N` the manifest's `stack_size`:

```
--import-memory --import-table --global-base=D --table-base=T --no-stack-first --no-entry -zstack-size=N
```

- The stack must come after the data, inside the window (section 2.2); the loader never looks at it, and the
  reference's `build` and `check-module` refuse a module whose stack leaves the window. `wasm-ld` at the version
  `tools.clang` pins (22.1.8) spells this `--no-stack-first`; without it lld puts every module's stack at
  address 0. That linker's `wasm-ld --help` lists both `--stack-first` (the default) and `--no-stack-first`. A
  linker whose help does not list the option cannot be told what to do here: check its help, and check the result,
  which is the first `i32.const` global (`__stack_pointer`) lying inside the window.
- Do not pass `--allow-undefined`. Every function the module does not define must be one of its
  declared imports (section 9), so a missing definition is a link error, not a runtime trap. This is a
  convention: what the loader and the checks depend on is that the module has no import outside its entry's
  list, and a function left undefined by `--allow-undefined` becomes an import from `env`, which the
  `imports` check refuses and the loader cannot instantiate (the host defines nothing in `env` but the memory
  and the table). A build that passes it still fails, later and with a less direct message.
- Do not pass `--max-memory` or `--shared-memory`. The module has no threads and no atomics.
- `--gc-sections` is recommended, so that only the slots and what they reach remain.
- Pass `--export=<name>` for each data word of section 8 if the source does not export it already.
  Slots are exported by the source (section 6).

Through `rustc`, the same options go through the linker one at a time:

```
cargo rustc --release --locked --target wasm32-unknown-unknown -- \
  -C link-arg=--import-memory -C link-arg=--import-table \
  -C link-arg=--global-base=D -C link-arg=--table-base=T \
  -C link-arg=--no-stack-first -C link-arg=--no-entry -C link-arg=-zstack-size=N
```

A rustc build meets the rules of this section as follows (checked with rustc 1.99.1, the version
`tools.rustc` of the fixture manifest pins; `cargo rustc ... -- --print link-args` prints the link line of any
other version, which is the thing to read):

| What rustc does | Who depends on the rule | What a build does |
| --- | --- | --- |
| passes `--stack-first` before the `-C link-arg` options | the checks (the stack inside the window); not the loader | nothing else: the last of the two options wins, and the `-C link-arg` options come after rustc's own, so `--no-stack-first` takes effect (and `-zstack-size=N` wins over rustc's `-z stack-size=1048576` the same way) |
| passes `--allow-undefined` (a rustc that does; 1.99.1 does not) | nobody: a convention (see above) | may leave it; an undefined function shows up as an `env` import that `imports` refuses. The rule stays, for the clearer message |
| exports `__heap_base` and `__data_end` (a rustc that does; 1.99.1 does not) | the checks: the exports must equal the entry's `exports` plus `data` plus the two fuel functions, exactly (section 9.1). The loader ignores extra exports | must not export them. lld has no option that takes back an `--export`, so a build whose rustc adds them filters those two arguments out of the link line with a linker wrapper (`-C linker=`) and calls the real linker with the rest |

The unmodified commands above link the fixture with rustc 1.99.1 with no wrapper, and the fixture's `check` passes.

The crate is `no_std`, of crate type `cdylib`, with `panic = "abort"` in the release profile and a
`#[panic_handler]`. The reference uses stable rustc; stable rustc cannot emit a dynamic-linking side
module for code with a panic path, which is why the convention is fixed-base. A Rust module declares
an import and an export like this:

```rust
#![no_std]

#[link(wasm_import_module = "services")]
extern "C" {
    #[link_name = "GameRand"]
    fn game_rand(stream: i32) -> i32;
}

#[no_mangle]
pub static mut g_blockBase: u32 = 0;

#[no_mangle]
pub extern "C" fn UpdateWind() { /* ... */ }

#[panic_handler]
fn panic(_: &core::panic::PanicInfo) -> ! { loop {} }
```

A `#[no_mangle] pub static` is exported as an immutable i32 global holding its address, which is
what section 8 asks for.

For clang, a module is compiled with `--target=wasm32` and declares imports and exports with
attributes:

```c
#define IMPORT(module, name) __attribute__((import_module(module), import_name(name)))
#define EXPORT(name) __attribute__((export_name(name)))
```

### 3.2 Compile flags and constraints

- **`build_flags`** of the game manifest (`-mno-nontrapping-fptoint`) is passed to every compile of
  a game module. It matters because of float-to-int conversion: the conformity target converts a
  double or a float that overflows an i32, or is NaN, to 0x80000000. With the nontrapping
  conversion instructions a wasm toolchain gives 0x7fffffff on overflow and 0 on NaN, which differs.
  With the flag, clang lowers the conversion to a guarded sequence with the required result. A
  language whose float-to-int cast has its own definition (Rust's `as` saturates) must do the
  guarded conversion itself wherever the value can overflow or be NaN. The bit-exact result is a
  conformity requirement, not a taste.
- **No imported globals.** A module reads its block base from the data word `g_blockBase`
  (section 8), never from an imported global. (Stable Rust cannot declare one.) The reference's
  `check` and `imports` refuse any imported global.
- **Statics are hashed.** A module's statics must not change between ticks (section 12).
- **No clock, no random, no files, no network, no WASI.** The only way to the outside is an import in
  the module's manifest entry (section 9).
- **NaN bit patterns.** The engine canonicalises the NaN an arithmetic instruction computes. A browser does not. A
  module must not let a result depend on a NaN's bit pattern. `neg`, `abs` and `copysign` are not arithmetic: as
  WebAssembly defines them, they change only the sign bit, and a NaN's payload survives them, in the engine as in a
  browser.
- **Freestanding.** The reference compiles C and C++ with `-nostdinc`, against its own small
  headers (`wasm/include`) and a small C runtime (`wasm/crt`: `memcpy`, `memmove`, `memset`,
  `memcmp`, `strcpy`, `strlen`, `strcmp`, `strncpy`, `_strcmpi`). There is no host libc and no
  exceptions or RTTI. C++ is compiled with `-fno-rtti -fno-exceptions`.
- **WebAssembly features.** The reference modules use the WebAssembly 1.0 instructions plus
  sign extension and bulk memory (which wasm-ld uses to zero bss). The engine's configuration
  gives deterministic relaxed SIMD and NaN canonicalisation and has no threads or fuel; do not rely
  on more than the reference modules use.
- **Value types at the boundary** are i32, f32 and f64 only (section 7). No slot or import takes or
  returns an i64 or more than one result. An f32 crosses the boundary bit for bit: neither the host
  nor the module widens, rounds, canonicalises or otherwise converts it, and a NaN payload and a
  denormal arrive as they left.
- **Start function.** A module may have one. It runs while the module is instantiated, before the
  host writes the data words and before the fuel counter is set, so it runs unmetered
  (section 11). It must not depend on the data words.

## 4. The manifest

A manifest is one JSON object. The game manifest is `wasm/modules.json`; the fixture manifest,
`tests/wasm/modules/modules.json`, has the same layout for the fixtures. The loader loads one
manifest per run.

### 4.1 Top-level fields

| Field | Meaning | Read by |
| --- | --- | --- |
| `memory_pages` | size of the shared memory in 65536-byte pages, 1 to 65536 | loader, required |
| `table_size` | entries in the shared table | loader, required |
| `window` | bytes of every module window, more than 0 | loader, required |
| `table_slots` | table entries of every module range | loader, required |
| `stack_size` | bytes of every module stack | host and checks (the loader checks only that it is a non-negative integer) |
| `block_base`, `block_size`, `block_max` | the state block: `block_base` needs `block_size` (a `block_size` without `block_base` is ignored, and there is no block); `block_max`, optional, at least `block_size` (the default), is the most the block can grow to | loader |
| `scratch` | `[base, size]` of the scratch area | loader |
| `majors`, `signatures`, `adapters` | section 10 | loader |
| `modules` | a non-empty list of module entries | loader, required |
| `tools` | the pinned tool versions: `clang`, `wasm-tools` (game manifest); `clang`, `rustc`, `wasmtime` (fixture manifest) | checks |
| `build_flags` | compile flags for every game module | build |
| `fuel` | `{"rule": text, "version": 1}`, the metering rule of section 11 | checks |

A number must be a non-negative integer. A number with a fraction or an exponent (`4096.5`, `4096.0`, `1e3`) is not
valid JSON to the loader, so the whole manifest is refused as not valid JSON. A number above 2147483647 is refused too.

### 4.2 Module entries

| Field | Meaning | Read by |
| --- | --- | --- |
| `name` | a non-empty string, unique in the manifest | loader, required |
| `data_base`, `table_base` | the module's window and table range (section 2) | loader, required |
| `exports` | the slot names, in the interface's slot order, without the engine slots | loader; checks |
| `engine` | `{slot: reason}`, the slots of the interface that stay in the engine (section 6): the host calls the native default for them, and the module does not export them | host; checks |
| `blocked` | the subsystem's default does not build to WASM yet: the reason, or `true` in the data kit, whose manifest leaves the reason out (it names source files). Such an entry has no `.wasm` and no budgets yet, the host runs the native default for every slot, and the gate judges no fuel or size for a module of it until the entry's budgets are set | build and checks; host; gate |
| `data` | the loader data words (section 8.2), in order: the four every module has, then `g_region` for a module whose slots take its region | loader; checks |
| `interface` | the interface whose slots the module implements | loader (majors) |
| `majors` | the major of each interface it uses, section 10 | loader |
| `slots` | `{slot: [[argument kinds], result kind]}` | loader (adapters); checks |
| `imports` | `{group: {import: [[argument kinds], result kind]}}`; a group is `services`, an interface (section 9.2), or a world view of an interface header (`econworld`, the `EconomyWorldView` of `economy.h`; `pathworld`, the `PathWorldView` of `pathfinding.h`), whose major is its header's | loader (adapters); checks |
| `publishes` | `{kind: payload size}`, the kinds the module may publish through `Emit` (the kind as a decimal string, the size in bytes; wind_meteors': `{"80": 36}`); each is a kind the route table (`data/routes.csv`, `docs/schedule.md`, "Routes") takes from the entry's outbox, its `interface` (its own effect kinds and the routed inbox kinds of other modules), with that kind's payload size | host (the `Emit` import); checks |
| `fuel_budget` | fuel per tick, section 11; required unless `blocked` | host; loader (its range, 11.3); checks |
| `size_budget` | the most bytes the `.wasm` may be; required unless `blocked` | checks; loader (a non-negative integer) |
| `cxx`, `c` | source files of the module in the reference repository | the reference build only; the kit's manifest leaves them out |

A kind is one of `int`, `f32`, `f64`, `void` (a result only), `linear`, `block`, `scratch`, `handle`
(section 7). A signature has at most 16 arguments and at most one result. `exports` and `data`
must be lists of names.

`tools/wasm_glue.py` writes every entry but wind_meteors' budgets and sources from the subsystem's interface header (the
slots in its order and their kinds), the replay schema (`data/replay_schema.json`: the argument modes, the services, the
header's imports and the engine slots, `record: false` with a load, save, network, presentation or query reason) and the
subsystem's recordings (the services they import; an import of another subsystem's load- or save-class engine slot is
left out, since only engine slots make it), and `uv run tools/wasm_glue.py --check` fails when the manifest is
not what it writes. `uv run tools/wasm_modules.py check-module FILE.wasm --name <subsystem>` checks a file against any
entry.

The fixture manifest's entries also have `slot_arg`, `slot_result` and `data_word`, the values its
check uses (section 14).

## 5. What the loader does and refuses

### 5.1 Order

1. Read the manifest and check it (5.2).
2. Create the memory and the table and define `env.memory` and `env.__indirect_function_table`.
3. The host defines every import of every group (sections 9 and 10). An import that is not defined
   by the time a module that needs it is loaded is an instantiation error (the loader can stub missing
   imports as traps; the game does not ask for that).
4. For each module: check the interface majors (section 10); zero its window; compile it; link its
   imports; instantiate it (this runs the start function); look up each name of `exports` as a
   function export and fill free table entries (2.3); read each name of `data` as an i32 global
   export, whose value is the address of the data word (section 8).
5. Before each slot call the host writes the data words (section 8). At the first slot call of a
   tick the host sets the fuel counter (section 11).

### 5.2 Refusals

Each refusal is an error that names the manifest or the module (and, for overlaps, both modules). The game ends on any of
them at start-up (section 13). When a manifest has more than one fault, the loader reports the first in the order
`docs/core-gate.md`, "loader and fuel", states.

- A manifest that is not valid JSON (a number with a fraction or an exponent included, section 4.1) or not an
  object; a required field missing; a value that is not a non-negative integer (`stack_size`, and a module's
  `fuel_budget` and `size_budget`, included); `memory_pages` or `window` zero, or `memory_pages` over 65536; a
  `fuel_budget` of 0 (section 11.3); a `scratch` that is not `[base, size]`.
- A block that does not fit the memory; a scratch area that does not fit the memory or that passes
  the block base.
- No modules, a module without a name, a duplicate name, a missing `data_base` or `table_base`,
  `exports` or `data` that is not a list of names.
- A module whose window does not fit the memory, or whose table range does not fit the table.
- A window that reaches the block base or the scratch base.
- Two windows that overlap, or two table ranges that overlap.
- A module not in the manifest, loaded twice, or loaded before the memory exists.
- A module that does not compile, or does not instantiate: an import the host did not define, an
  import whose type differs from the host's (the host's types are `i` for i32, `f` for f32 and `d` for f64),
  an import of the memory or table with a minimum above the real size or a maximum below it, or a trap in its
  start function.
- A name in `exports` that is not a function export, or a name in `data` that is not an i32 global
  export.
- An interface-major problem (section 10).

What the loader does **not** check, and the build must:

- that the module's data, stack and element segments lie inside its window and table range
  (the reference's `build` fails when the linked data or stack leave the window, when the table
  entries leave the range, or when the window meets another module's or the scratch area);
- that the base a module was linked at is the manifest's `data_base`: the reference's build checks
  only that the linked data and stack lie inside the window. A module linked at another base
  instantiates without a loader error, and its data goes where it was linked, not where the
  loader zeroed and the statics check looks. Link at the manifest's base;
- that its imports are exactly the allowlist, its exports exactly the entry's, and its manifest
  kinds agree with its real signatures (the reference's `imports` check);
- that it has no imported global, that its metering surface is intact, that its size is within its
  budget, and that the interface majors and signatures agree with the headers (the reference's
  `check`).

A conforming build runs those checks. The judge (section 15) runs the reference's own checks on a submitted module before it
plays: `uv run tools/wasm_modules.py check-module FILE.wasm --name SUBSYSTEM` runs the window and table check, the
imports, exports and kinds check, the metering surface and the size budget on that one file, against the manifest's entry. It
exits 0 when the file passes, 1 when it prints problems, and 2 when the check could not run (an unknown subsystem, no
`wasm-tools`, an internal error): that is not a verdict on the module.

## 6. Slots

A slot is a function export of the module's wasm instance, named exactly like the interface's slot
(`UpdateWind`, `Apply`). The module's `exports` lists them, in the interface's slot order.
`Apply` takes a `linear` address of the module's `<Module>EventHeader` record, the event of its inbox, and the size
in bytes of the event's payload (6.1). The in-repo host copies at most the largest payload of the module's inbox kinds
(the header's size when no kind has a payload struct) and passes the smaller of that and the size it was given.

**Engine slots.** A slot named in the entry's `engine` stays in the engine: loading and saving a game, the network, the
presentation, and the queries only the dump, a saved game or the campaign makes. The module does not export it, and
the host calls the engine's native slot instead, with the module's state in the block as the native slot reads it. A
re-implementation leaves these out of its module; they are the engine's until a later issue moves them.

### 6.1 Event payloads

The events and effects of a module with an inbox share one convention. Each module has an `<Module>EventHeader` that
starts with `int kind` and holds the fields every kind of the module has (pathfinding's is `PathfindingEventHeader`).
Each kind has a payload struct `<Module><Kind>` (`TargetingSetUnit`, `EconomySpend`) that starts with the header and adds
the fields of that kind, or has no payload struct, in which case the event is the header alone and its size is the
header's. A payload is plain data: 4-byte fields and arrays of them, or of bytes, no pointers; a unit or a piece state is a block
offset. Two kinds share one payload struct only when their fields mean the same.

A payload that names a set of units carries it as a bit set over one player's range of the unit array, not as a counted
list: the header's `unit` is the first unit of the range (the player's `unitsBegin`), and bit `i` (bit `i % 32` of word
`i / 32`) stands for the unit `i` records after it. The set has a fixed size, so the payload has one size and fits an
inbox entry, and the units come in the order of the unit array, the order 3.1c gives them their orders in.
`OrdersGroupCommand` holds 512 bits, above the 500 units a player may have.

The publisher passes the payload's size with it: `Emit(&payload, sizeof payload)`. An event of fewer
than 4 bytes has no kind to route by, and is a fatal error. The scheduler copies `size` bytes into a 128-byte inbox entry,
keeps the size with it, and hands the subscriber the event as a pointer to the header together with the size the publisher
gave, whether it delivers the event at once or from the inbox. A slot that consumes an event takes the header and that
size, `Apply(header, size)`: it first refuses a size smaller than the header with outcome 0, before it reads `kind`; then
it switches on `kind`, checks that `size` is at least the size of that kind's payload, and refuses a smaller one with
outcome 0 and no change (a `void` `Apply`, as wind_meteors', just ignores it); only then does it read the payload's fields.
The module's interface header says which struct belongs to which kind. An effect is routed by its kind, so its subscriber
reads the payload of the kind it subscribed to, and does nothing with an effect smaller than that payload.

**Where routes are stated.** Which subsystem receives each effect, and whether it is delivered before the publish returns
(synchronous) or from the subscriber's inbox (queued), is stated in two places and nowhere else: the interface headers,
whose effect enums name the kinds a module publishes and whose event enums name the kinds its inbox takes, with what each
does; and `docs/schedule.md`, "Routes", which is the route table itself, a row per publisher, kind and subscriber in the
order the effect is delivered, with each row's delivery and the rules for when an inbox is delivered. The table is
generated from the engine and checked in CI, so the two do not drift; it is also a file of the kit, `data/routes.csv`.

- **Finding.** The host finds a slot by export name in the module's instance. It never looks one up
  in the table. (The loader keeps the slots in the order of `exports`, and puts free table entries in
  that order, but a host calls by name.)
- **Signature.** A slot takes at most 16 arguments and returns at most one result, each i32, f32 or f64,
  as the manifest's `slots` entry says in kinds (section 7). Calling-convention keywords of the
  engine's native language do not exist at the boundary: a slot called with an implicit `this` in
  the native build takes it as its first argument.
- **Calling.** The host calls with its arguments in order. It does not check the real wasm signature
  against the manifest on each call (the call is unchecked, for speed); a mismatch is a build
  error that the reference's `imports` check finds, not something the host detects.
- **Exactly the slots.** A module exports its slots, the data words, and the two fuel functions
  (section 11), and nothing else.
- **Nested calls.** The host supports a slot call made while another slot call of the same module
  is running (an import that calls back into the engine). The handle table and the fuel counter treat
  only the outermost call as the boundary (sections 8 and 11). A slot call of any module made while an
  import runs is refused (section 13), so one module's import cannot reach another WASM module's slot yet.

- **Hook points.** Every slot's doc comment ends with a `Hooks:` line: the synced mod hooks of
  [#1](https://github.com/ozgb/byte-tactics-moddable/issues/1) that attach to that call, or
  `Hooks: none`. The `Apply` slots, the tick drivers and the world-view slots have one too. A hook
  that changes how a module answers an event (`UnitDestroyed`, `AllowCommand`) is named on that
  event kind's doc comment instead, and an event kind with no hook has no `Hooks:` line. A hook that
  runs once a tick (`GameFrame`) is named on the tick driver that starts the tick,
  `units.UpdateAllUnits`. A named slot or event kind is where the hook runs, so a re-implementation
  keeps it as a call or an event of its own, with the same arguments and at the same point in the
  tick: it does not fold it into another slot or inline it into its caller. Milestone 3 decides how
  a module calls the hook; until then the line only fixes where.

## 7. Argument kinds and the pointer rule

A kind says what an argument or a result is. The wasm type of `int`, `linear`, `block`, `scratch`
and `handle` is i32; of `f32`, f32; of `f64`, f64; `void` is no result.

| Kind | At the boundary |
| --- | --- |
| `int` | a 32-bit integer: a count, an enumerator (a random stream is 0 for the main stream, 1 for the load stream), a flag, a signed value |
| `f32` | an IEEE 754 single, the 32 bits carried unchanged: the host moves it as a 32-bit pattern and does not pass it through a wider or a canonicalising type |
| `f64` | an IEEE 754 double |
| `void` | no result |
| `linear` | an address in the shared memory; 0 is null. The object it points to is in the module's window, on its stack, or in the block (or in the scratch area, for a slot argument), and a callee reads and writes it in place; or it is load data outside the block (a frame table or one of its frames, section 7.1), which a module reads and never writes |
| `block` | a linear address that lies inside the state block, or 0. As a result, the host checks that it lies inside the block |
| `scratch` | a linear address in the scratch area, where the host put a copy of a native object for this call |
| `handle` | an opaque u32 (section 8) |

A params or event record such as `MeteorShowerParams` (the `LoadMeteorDefaults` argument) or `WindMeteorsParams` (a payload the `Apply` slot reads) is a `linear` argument: the caller owns the object and the callee reads and writes it in place.

The published headers spell every integer at the boundary as `int` or `unsigned`, which are 32 bits
on every target; `long` and the C library's size type, whose width depends on the platform, are not used. A slot or a
service may still take or return a `char`, a `short` or a `bool` (`unsigned char player`, an
`unsigned short` unit id). Each is the `int` kind, an i32, carried as clang's wasm32 C ABI carries
it: the side that produces the value extends it to 32 bits, the caller for an argument and the
callee for a result, and the side that receives it does not extend it again. A `signed char` or a
`short` is sign-extended, an `unsigned char` or an `unsigned short` is zero-extended, and a `bool` is
0 or 1. A module in another language must keep the same rule: it passes and returns these values
already extended, and it may rely on what it receives being extended. A `bool` field of a record is
one byte of the record's layout; it is not an argument kind.

### 7.1 The pointer rule for Milestone 2

- **A pointer stored in the block is a block offset.** It is a 32-bit offset from the block's base
  (0 is null), not an address. A module and the engine store and read the same bytes. To follow it,
  add the block base: the value of the data word `g_blockBase` (section 8), which equals
  `block_base`. To store one, subtract the base. A record that stores a pointer has a field of this
  kind (a `Ref`); the record layout says which fields.
- **A pointer argument is a linear address.** Every pointer given to a slot or to an import, and
  every pointer returned, is an address in the shared memory, never an offset and never a native
  address.
- **A pointer to load data is a linear address.** Some block fields point at load data outside the
  block: a frame table (a GAF animation's frames) or one of the frames it lists. Such a field holds
  the object's address in the shared memory, not a block offset. The engine loads or makes every
  frame table before the match starts and neither changes nor frees it while the match runs, so the
  address stays valid across ticks. The recordings (`tests/replay/`) show these fields:

  | Subsystem | Field | Points at |
  | --- | --- | --- |
  | los_radar | `Game.losTable` | the sight masks, a frame table loaded from a GAF |
  | features | `Feature.seqburn`, `Feature.seqdie`, `Feature.seqreclamate` | sequences of the feature type's GAF |
  | features | `Spot.anim.src` | the animation of a sequence cursor, a frame table |
  | weapons | `weapondef.explosionSeq`, `weapondef.splash` | the explosion sequences of the weapon type's GAFs (the splash one on a lava world) |
  | weapons | `Debris.ref1.src`, `Debris.ref2.src` | the animation of a sequence cursor, a frame table |
  | weapons | `DebrisFace.texTeamSrc` | a face's team texture, a frame table whose address weapons copies from the loaded 3DO model |
  | weapons | `DebrisFace.texPic` | a frame of that table, which weapons writes from `GetGafFrame`'s result |
  | weapons | `GafSrc1.ptr` to `GafSrc3.ptr`, `GafSources.ptr` | sequences of the fx GAF (`Game.src1` to `Game.src3`, `Game.sources`) |
  | weapons | `GafLists.ptr` | the three explosions of `Game.lists`, frame tables the engine makes at load rather than reads from a GAF |

  A module gets these addresses from such fields, from the GAF services (section 9.3) and from the
  loaded 3DO model data (a face's texture). It reads a frame table and its frames
  (`FrameTable.count`, a frame's size, offsets, transparency and pixels) and never writes them.
  Nothing traps a write: a wasm32 memory has no read-only pages, so this is the module's obligation,
  not something the host enforces. The host maps frame tables into the shared memory outside the
  state block (#1208). Of the modules, only los_radar reads frame data as simulation input (the
  sight masks of `Game.losTable`). features uses a sequence only as a timer, and weapons' cursors and
  debris pictures are presentation state kept in the block. units reads one more such field,
  `ModelState.bitmap`, a unit's cached picture: that frame is not load data but the presentation's,
  which draws it and may replace it while the match runs, and units only tests whether the field is
  null.
- In Milestone 2 every module is WASM and every object the simulation uses is already in the shared
  memory, so no translation happens: the caller passes the address of the object where it lives. The
  two rules together mean that a pointer into the block is an offset when it is stored and an address
  when it is passed.

### 7.2 What the in-repo host does (a native engine)

The in-repo host is a native program that still owns some objects in its own memory, so it translates
at the boundary. A Milestone 2 host does none of this, but the kinds were named for it, and the
behaviour is the contract the existing module was built against.

- A `linear` slot argument that is an object in the block is passed as its address in the memory.
  An object outside the block is copied into the scratch area and the scratch address is passed;
  after the call it is copied back if the slot can write it. An object that is bigger than the
  scratch area is a fatal error.
- A `scratch` slot argument is always such a copy. All copies start at the scratch base, so a slot
  has at most one `scratch` argument. The module reads such an object only through the import that
  takes it; its raw bytes and the offsets inside them are for the engine, which is free to change them.
- An import's `linear` argument is checked to lie inside the memory (not past its size); an
  argument outside it is a trap that names the import and the argument index. An import that takes a
  buffer together with its size checks the whole buffer, from its address to its size, and refuses a
  size below 1. An import that takes a buffer without a size does not look at how many bytes the callee
  will use, so the module is responsible for passing a buffer big enough.
- A `block` result that is not inside the block is a trap.
- **The glue is generated.** One generic glue holds the loader, the services and the checks above, and a generated
  table gives each module's slot table, slot calls and interface imports, from its manifest entry, the replay schema's
  argument modes and the interface header. A
  slot argument of mode `handle` becomes a handle, `text` a copy in scratch, an object in the block (`object`, `weapon`,
  `load`, `pending`, or a pointer with no mode) its address, `region` its address and the `g_region` word (8.2),
  `world` nothing (the host keeps the world view and a module reaches it through the view's imports), an `event` a
  scratch copy of at most the inbox's largest payload, a buffer with its length a scratch copy of that many elements,
  and a pointer to a record a scratch copy of the record, copied back unless it is only read. An interface import's
  pointer argument is checked to lie in the memory, a `handle` one to be live, and an argument the callee writes a pointer
  through has that pointer turned back into an address.
- **Publishing.** The host hands an `Emit` to the scheduler as the subsystem's native default publishes the same kind:
  wind_meteors and targeting as an effect (their recordings' `services.Emit` lines), every other module as a publish of
  its outbox (their recordings' `services.SchedPublish` lines, with the outbox and the size), so a replay compares a
  module's `Emit` with the line its recording has.

### 7.3 A buffer the caller supplies

A slot that returns a list of objects takes a buffer from the caller, so that no memory is handed
across the seam and no native container layout is part of the contract. `los_radar.GetVisibleEnemiesInRadius` (kinds `[int, linear, int, int, linear, int] -> int`: the player, a
position, a radius, flags, the output buffer and its capacity in units) is the slot of this form, and
`units.GetFactoriesInRadius` (the player, a position, a radius, the buffer and its capacity) has it too.

- It returns the number of units found, which may be above `capacity`.
- It writes the first `min(count, capacity)` units to `out`, each as a block offset (section 7.1), in
  the order the engine finds them, and writes nothing past `capacity`.
- A caller sizes the buffer for the most the list can hold: the engine's list of units has each unit
  once, so a buffer of one entry per slot of the unit pool never gets a count above its capacity.

## 8. Handles and the loader data words

### 8.1 Handles

A **handle** is how the engine gives a module a thing the module can only hand back, never look at:
a TDF record, a bank of saved-game items (`HapiBank`).

- It is an opaque `u32`: index plus 1 into a table the host keeps; 0 is the null handle.
- It is valid **only until the outermost slot call returns**. The host clears its table then, so a
  handle kept in the block, in a static or in a `TdfFile` that outlives the call is garbage later,
  and a later call may give the same number to something else.
- Within one outermost call, the same engine object always has the same handle. The table holds 32
  handles; more than 32 live at once in one call is a fatal error. `TdfFile_Destruct` releases the
  handles of the file's records, and the host gives a released handle's number to the next new object,
  so a slot may load and destroy any number of files one after another. A walk over more than 32
  records of one file in one call still runs out, since every record it selects keeps its handle until the
  file is destroyed. Mission's `CountMissions` and `MissionExists`, which make such a walk on a campaign of
  more than 31 missions, are engine slots (campaign queries), so no module makes it yet; a module that takes them needs an
  ABI change first (a larger table, or a record handle that lasts only while it is its file's current record). A released
  handle is an unknown handle to every import.
- A module must **never read through a handle**: it is not an address. It only passes it back to an
  import whose parameter kind is `handle`. An unknown handle (above the number issued) and a null
  handle where an object is required are traps naming the import.
- A `TdfFile` object (three 32-bit words: root record handle, current record handle, flags) lives
  in the module's own memory, usually on its stack, and the TDF imports write the two handles into
  it. A module therefore constructs, uses and destroys a `TdfFile` inside one slot call.

### 8.2 The loader data words

A module exports four **data words**, five for economy, pathfinding, players and los_radar. Each is an exported i32 global whose value is the address, in
the module's window, of a 4-byte location (`g_extTdf`: a 4-byte area). The name list is `data` of the
module's manifest entry. The loader reads each global once, when the module is loaded. **The host
writes the words before every slot call, so a module may assume they are set whenever a slot runs**,
and not in the start function.

| Export | The host writes | Used for |
| --- | --- | --- |
| `g_game` | the linear address of the game record (the top-level simulation record, in the block) | the module's pointer to the game record |
| `g_simGlobals` | the linear address of the simulation globals record, in the block | the module's pointer to the globals record |
| `g_extTdf` | the 4 bytes `tdf` plus a terminating 0, the extension of TDF data files | building a data path |
| `g_blockBase` | the linear address of the block's first byte (`block_base`, 1048576 today) | adding to a block offset, section 7.1 |
| `g_region` | the linear address of the module's state region in the block, the record the last slot that takes a `region` argument was given (0 before one has); the host refuses a region outside the block | the module's pointer to its own state |

The `MeteorShowerParams` record and the event payloads (6.1) are not data words: they reach a module as the `linear` argument of a slot (section 6).

The host writes the words directly, not through an import. They are excluded from the hash of
section 12.

## 9. Imports

A module's imports are the allowlist below and nothing else.

### 9.1 The allowlist

- `env.memory` and `env.__indirect_function_table`, as in section 2. The reference's `imports` check
  requires both; wasm-ld leaves the table import out of a module that never uses it, so such a
  module takes one function pointer to keep it.
- The functions its manifest entry lists under `imports`, group by group: `imports` is
  `{group: {name: [[kinds], result]}}`. A group is an interface (`weapons`) or `services`. The module
  imports each function by group and name exactly as listed, and imports nothing the entry does not
  list, and every listed function must be imported (the lists are exact).
- No imported globals, tables or memories other than the two above. No function from `env`, from
  a group whose name starts with `wasi`, or from any group the entry does not list. No clock, no
  random source, no file or network function.
- The module's exports must equal the entry's `exports` plus its `data` plus `__fuel_set` and
  `__fuel_get`, exactly.
- For every import and slot, the manifest's kinds must match the wasm signature: the same number of
  arguments, `f32` exactly where the wasm type is f32, `f64` exactly where the wasm type is f64, and `void` exactly where there is no result.

The reference's `imports` check enforces all of this on every built module. It is the check that
catches non-determinism at the boundary whatever language the module is written in.

### 9.2 Interface imports

A group named after an interface imports the interface's slots by their names, with the interface's
signature. The game manifest's module imports `weapons.FindWeaponByName`. The host defines each by calling the engine's slot of the same name.

| Import | Signature (kinds) | Behaviour |
| --- | --- | --- |
| `weapons.FindWeaponByName` | `[linear] -> block` | finds a weapon definition by its TDF name (a NUL-terminated string); returns its address in the block, or 0 |

### 9.3 Engine services

The `services` group is the engine functions a module may import, and `interfaces/services.h` declares exactly those,
each as a free function under its import name. `signatures.services` of the manifest gives each one's kinds, whether or
not a module of the manifest imports it yet, and a module's `imports.services` is a subset of it. An engine class's
member is imported as `<Class>_<Member>`, with the object as the first argument: a `linear` address when it lives in the
module's memory (a `TdfFileLayout`) and a `handle` when it is an engine object (a `TdfRecord`, a `HapiBank`). The rest
of the engine's services (the scheduler's publishing and synchronous calls, the recorders of the dump's events, the
whole TDF and console command classes) are in an engine header outside `interfaces/`, which only the native engine
includes; a module reaches the scheduler's calls through the imports of "Routes in the recordings" below. The imports'
signatures are below.

| Import | Signature (kinds) | Behaviour |
| --- | --- | --- |
| `GameRand` | `[int] -> int` | advances the named random stream and returns the next value, 0 to 0x7fff; the stream's state is in the block |
| `RandomInt` | `[int] -> int` | a random number in `[0, range)` from a generator of its own, whose state is `rng.seed` in the recordings and is not a game logic stream (`rng.main`, `rng.load`); a range below 2 gives 0 and leaves the generator alone |
| `FatalError` | `[linear] -> void` | ends the game with the NUL-terminated message after the call unwinds (section 13); does not return to the module |
| `BuildDataPath` | `[linear, linear, linear, linear] -> linear` | builds `dir/name.ext` below the data directory into the buffer (first argument) and returns the buffer's address; the host traps if the result is not the buffer |
| `TdfFile_Construct` | `[linear] -> void` | builds an empty file in place, a `TdfFileLayout` (root, current record, flags) |
| `TdfFile_Destruct` | `[linear] -> void` | unloads the file and frees the engine's records |
| `TdfFile_LoadFile` | `[linear, linear] -> int` | loads and parses the file at a path; the result is the engine's status |
| `TdfFile_SelectRecord` | `[linear, linear] -> int` | selects the child of the given name of the current record (of the root when none is current) as the current one |
| `TdfRecord_GetFieldInt` | `[handle, linear, int] -> int` | reads an integer field, or the default (third argument) when it is missing |
| `TdfRecord_GetFieldDouble` | `[handle, linear, f64] -> f64` | reads a double field, or the default |
| `TdfRecord_GetFieldString` | `[handle, linear, linear, int, linear] -> int` | copies a field's text into a buffer (arguments: record, destination, key, size, default) |
| `HapiBank_OpenAccount` | `[handle, linear] -> int` | opens the named account of a saved-game bank |
| `HapiBank_SetIntegerItem` | `[handle, linear, int] -> int` | writes an integer item of the open account |
| `HapiBank_GetIntegerItem` | `[handle, linear, int] -> int` | reads an integer item, or the default |
| `SimAlloc` | `[int] -> linear` | allocates the size in bytes in the state block, zeroed and 8-byte aligned, and returns its address; the allocation stays until `SimFree` returns it, across ticks, and its bytes are game state like any other of the block; a size of 0 is allocated like any small size, as the block's smallest chunk, so it returns a new address, never null, which `SimFree` takes back; a size the block cannot hold ends the game (section 13) |
| `SimAllocTagged` | `[linear, int] -> linear` | allocates as `SimAlloc` does (arguments: tag, size); the tag is a NUL-terminated name of the allocation, which the recordings carry and which changes nothing |
| `SimFree` | `[linear] -> void` | returns an allocation of `SimAlloc` or `SimAllocTagged`, by the address they returned, to the block; null does nothing |
| `RecordSimEvent` | `[int, int, linear, int] -> void` | appends a sim event to the sim event queue of the block (arguments: kind, flags, the arguments as an array of 32-bit words, their count): one record per six arguments, at least one, each with the kind, the tick and the next six arguments (zero past the count), every record after the first with the flags and the continuation flag; the kinds, their arguments and the flags are in `docs/sim-events.md`; a queue that has no room ends the game in test mode, and otherwise drops the event and counts it in the queue's dropped count |
| `SimCaptureAlloc` | `[int, linear] -> linear` | reserves the size in bytes, rounded up to 8, in the radar arena of the block (`docs/sim-events.md`, "Radar events"), zeroes it, writes its offset in the arena to the 32-bit word at the second argument and returns its address; the offset is what a radar event records, and it holds until the drain that hands over that event empties the arena; an arena that has no room ends the game in test mode, and otherwise returns 0, writes nothing and counts it in the arena's dropped count |
| `GetGafFrame` | `[linear, int] -> linear` | returns the frame of the given index of a frame table (a GAF animation's frames: its count, a 16-bit word at the start, and 8-byte entries from offset 40, each the frame's address and its duration in ticks), or 0 when the index is negative or not below the count; the table and the frame are load data (section 7.1), so the result is an address outside the block that the module reads and never writes; it is a pure query (its result depends on its arguments alone and it changes nothing), so a module may call it as often as it likes and in any order: a replay answers each call by its arguments, and does not compare how many calls there are or where they fall among the other imports (section 15) |
| `InitGafSequence` | `[linear, linear, int] -> void` | starts a sequence cursor (a 12-byte record of the block: the frame index and the ticks left on it, two 16-bit words, the loop flag, a byte, and at offset 8 the animation's address) on an animation (a frame table, load data of section 7.1, its loop flag the byte at offset 2) at a frame index, the first frame when the index is not below the count, with that frame's duration as its ticks left |
| `StepGafSequence` | `[linear] -> int` | advances a sequence cursor by a tick: a cursor with no animation stays and returns 0; one with two or more ticks left loses one and returns 0; otherwise it moves to the next frame and its duration and returns 1, and past the last frame it goes back to the first when the animation loops, and when it does not, it drops the animation (address 0) and returns 1 |
| `GetSlotDpid` | `[int] -> int` | returns the network id of the player in a slot of the game record, or -1 when the slot is 10 (none) or its player's type is 0 (empty) |
| `FindSlotByDpid` | `[int] -> int` | returns the lowest slot, 0 to 9, whose network id (as `GetSlotDpid` gives it) is the argument, or 10 when none is or the argument is -1 |
| `Emit` | `[linear, int] -> void` | publishes an effect (a payload that starts with its interface's event header; the second argument is its size in bytes) to the calling module's outbox, the one of its entry's `interface`, never one an argument names; `Emit` takes any kind a route carries from that outbox, the same set the native `SchedPublish` delivers; the host refuses, as an import error and in this order, a pointer outside the module memory, a size outside 4 to 128 bytes (an effect holds at least its kind, and at most an inbox entry), an effect that runs past the module memory, a kind not in the entry's `publishes` table, a kind of that table that no route takes from the module's outbox (the checks refuse such a manifest, so only one edited by hand gets this far), and a size other than that kind's; a per-kind hook then runs (wind_meteors' SPAWN, a `WindMeteorsSpawn`: the weapon slot must be in the table of weapons); the engine delivers the effect exactly as `SchedPublish` does, as `docs/schedule.md`, "Routes", states (at once on a synchronous route or outside the tick, otherwise from the subscriber's inbox), and the recordings carry the effect's fields as the import's argument |

**A malformed argument changes nothing and does not trap** (ozgb/btm#25). The default modules never pass one, so no
recording shows what it does, and this is the contract: an address the import does not trap on (section 7.2) that is not
what the argument must be, or a number outside its range, makes the call return 0 (null, for an address; `-1` for
`GetSlotDpid` and `10` for `FindSlotByDpid`, their answers for none) and write nothing. For each service above:
`SimFree` of an address that is not a live allocation (not one `SimAlloc` or `SimAllocTagged` returned, or one already
returned); `RecordSimEvent` with a kind that is not
a kind of `interfaces/sim_events.h`, or with a null array and a count above 0; `SimCaptureAlloc` with a null offset
address; `GetGafFrame` with a null table; `InitGafSequence` with a null cursor or animation, or a negative index;
`StepGafSequence` with a null cursor; `GetSlotDpid` with a slot above 10. `SimAllocTagged` takes any tag, null
included, since the tag changes nothing.

**Routes in the recordings.** A recording carries some calls a module makes to the engine under the engine's own
service names (`docs/replay-format.md`): the scheduler's publishing call and its synchronous calls into another
subsystem. No module imports these names. A module makes the call through the import of the table below, and a spec
names that import, never the recorded name. The import takes the arguments of the slot it names, in the slot's order. The
recorded line shows the ones the recording keeps: an argument it leaves out (the event of `SchedPublish`, the position
buffer of `GetWeaponTargetPos`, the record of `Spend`) is not in the line, and a cell the line gives by value (`(x,y)`)
is passed as the address of a `Point16` holding it. The engine delivers the
callee's pending inbox before the call when the slot's header says so, and records a synchronous call into a module in
the dump as section "Who records the dump's events" states.

| Recorded call | Import | What the module does |
| --- | --- | --- |
| `services.SchedPublish` | `services.Emit` | publishes through `Emit` any kind its outbox's routes carry (its own effect kinds and other modules' inbox kinds, `data/routes.csv`) to its own outbox; the recorded outbox (the first argument) is always the module's own, the interface of its entry, and the recorded size is `Emit`'s second argument |
| `services.ScheduleTargetingTargetUnit` | `targeting.GetWeaponTargetUnit` | asks for the unit a weapon aims at |
| `services.ScheduleTargetingTargetPos` | `targeting.GetWeaponTargetPos` | asks for the position a weapon aims at, into a `Vec3` of its own; the recording's `x`, `y` and `z` outputs are the three words the callee wrote |
| `services.ScheduleTargetingFind` | `targeting.FindWeaponTarget` | picks the best enemy for a weapon |
| `services.ScheduleTargetingFireAtWill` | `targeting.FindBestTargetIfFireAtWill` | picks the best enemy for a unit that fires at will |
| `services.ScheduleMapCanBuildAt` | `map.CanBuildAt` | asks whether a unit type can be built at a cell; the recording's `SimGlobals.g_lastPlaceMetalSum` output is the placement record the callee filled |
| `services.ScheduleMapCanPlaceUnitFootprint` | `map.CanPlaceUnitFootprint` | asks whether a unit type's footprint fits at a cell, with the same output |
| `services.ScheduleEconomySpend` | `economy.Spend` | spends a player's energy and metal (an `EconomySpend`) |
| `services.ScheduleEconomyRequestGranted` | `economy.RequestGranted` | asks whether a unit's request for resources would be granted |
| `services.ScheduleMetalExtraction` | `economy.UpdateMetalExtraction` | sets a metal extractor's rate |

The kinds each module's outbox carries are the rows of `data/routes.csv` with that publisher; a spec names a publish by its kind and cites that file.

`economy.RequestGranted` reaches the engine's `ScheduleEconomyRequestGranted`, so a module's call gets the same check a
native caller's does: a call made while economy's inbox has events not yet delivered is a fatal error. The engine's
call that delivers units' inbox at once has no import: only units' handling of the unit states the network receives
makes it, and that stays in the engine, so no module makes it and no slot matches it.

**A call into another module during an import is refused.** A slot call of any WASM module made while an import runs,
such as an import of the table above whose callee is another WASM module, does nothing: the glue refuses it with
`slot <name>: called inside an import` (section 13), and the game ends once the outer call returns. A core refuses it
the same way. A synchronous call between two modules waits on a nesting rule for the fuel and handle boundaries of
both, which comes with the first two modules that call each other and both build.

**What stays in the engine.** A slot of an entry's `engine` (section 6) is not a module's, and nothing a module imports
reaches its state. `WriteState`, the dump's writer of a subsystem's state, is one: it is called by the test harness
only, outside the tick, and writes through the engine's `StateWriter`, which is not in the shared memory; the engine
writes a module's state from the block as it does a native one's, so a module writes nothing for the dump.
`mission.GetCampaignMissionName` is one too: it serves the menus and the map list, and the localised key it builds
needs the language prefix of the translation table, which no import gives. Map's plot-map slots (`LoadMetalPlotmap`,
`SaveMetalPlotmap`, `LoadPlayerFeaturesPlotmap`, `SavePlayerFeaturesPlotmap`) are save-class slots: their buffers of
`width x height` bytes (65536 on a 256 x 256 map) are the engine's, in its own memory, and never cross the boundary, so
no window or scratch area has to hold one. Map's entry imports no `files.Bank*` service for the same reason: the
saved-game bank is used only by those slots.

`tools/speclint.py` reads the first two columns of this table from the kit's copy of this page: a spec may name a
recorded call of the first column (to say which recorded line an import makes) as well as its import.

**Who records the dump's events.** In test mode a dump lists the events and effects of each module on the scheduler
(`docs/trace-format.md`, sections 8.22 and 8.24 to 8.30). The engine records each event it delivers to a module's
inbox, with the outcome `Apply` returns, and each synchronous call into the module (economy's `Spend`, features'
`Place`) with its answer. An effect is recorded by the service that publishes it: `Emit` puts the effect in
the calling module's outbox and records it in one call, so a module built from a spec has nothing else to call, and the dump's events
come out whole. The host takes the outbox from the calling module's manifest entry, never from an argument, so a module
cannot publish as another; an event kind cannot name the outbox, since kinds repeat across interfaces. The engine's native
modules publish and record their effects through an engine-only call that names the outbox, which no module can import. A module that takes over
another subsystem imports `Emit` and its manifest entry gets a `publishes` table, with no host change. No trace row hashes
these records. A replay carries a publishing service's call as an `i services.<Name>` line (for `Emit`, with the effect's
value and its size), and the gate judges an effect through what its subscribers do with it. The `services` list of a subsystem in the replay schema (`annotations.subsystems.<name>.services` of
`data/replay_schema.json`) is not the module's import list, which is `imports.services` of its manifest entry:
targeting's names `RandomInt` and `Emit`, and its recordings carry `i services.Emit` lines.

The services interface is at major 4 and minor 2 in the header (`SERVICES_VERSION_MAJOR`, `SERVICES_VERSION_MINOR` of
`interfaces/services.h`). The 1.x line added the scheduler's services one minor at a time; major 2 gave every recorder
of the dump's events and wind_meteors' publishing import the payload's size, and 2.1 and 2.2 added the recorders of orders, movement,
units and scripts. Major 3 splits the header: `interfaces/services.h` keeps the imports above, the TDF and bank
members become free functions under their import names, the strings they only read become const, the size
of `TdfRecord_GetFieldString` becomes an `int`, and the rest moves to the engine header. Since the import names and kinds stay, a module built against major 2 imports the same functions. Major 4 replaces that wind_meteors-only import with the general `Emit` (same kinds, `[linear, int] -> void`; the host takes the outbox from the calling module's entry). A module built against major 3 imports the old service, which no longer exists, so it must be rebuilt against major 4; no adapter covers a renamed import. Minor 1 adds the allocation services (`SimAlloc`, `SimAllocTagged`, `SimFree`), the sim event services (`RecordSimEvent`, `SimCaptureAlloc`) and the player slot services (`GetSlotDpid`, `FindSlotByDpid`), services the recordings of the subsystems other than wind_meteors import, so that each module's manifest entry can list them. Minor 2 adds the GAF services (`GetGafFrame`, `InitGafSequence`, `StepGafSequence`), which the recordings of features, los_radar and weapons import, with `linear` kinds: the frame tables they take are load data in the shared memory (section 7.1).

`tools/abidiff.py` judges `services.h` entry by entry: every free function and non-virtual member function, keyed
`Class::Method` (the name a module imports is `Class_Method`) or by its plain name, with its return type, parameter
types, const-ness, calling convention and `static`. An entry removed or retyped is breaking and needs a major bump; one
added needs the minor bump. An overload added to a name that had one signature is breaking too, since both signatures
would map to the same `Class_Method` import. It also ties the header to the manifest: the entries, by import name, must
be exactly the names of `signatures.services`, each with its kinds there, and every name of a module's `imports.services`
must be one of them, or the `services (imports)` line fails. A service is added to the header and to
`signatures.services` in one change, before or with the first module that imports it.
The baseline is the newest `interfaces-v*` tag.

A service never returns a native address. A service that finds something outside what it may reach
(a pointer outside the memory, an unknown handle) traps with a message that names it (section 13).

## 10. Interface majors, signatures and adapters

An interface has a major version that changes when a slot is removed or its signature changes.
(A minor version, for an added slot, needs nothing here.)

### 10.1 What the manifest records

- `majors`: the current major of each interface and of each import group's interface, read from the
  interface headers. The game manifest's are in `wasm/modules.json` (section 16).
- `modules[].interface`: the interface whose slots the module implements.
- `modules[].majors`: for its own interface and for every import group, the major the module was
  built against.
- `signatures`: for each interface, `{name: [[kinds], result]}`, the **current** signature of every
  slot and import the manifest's modules use. A name no module of the manifest uses is dropped, even
  one the interface still declares. A module built against the current major gives its name's
  signature; a name only older modules use keeps its entry, and the build fails when the interface's
  header no longer declares it (that module needs an adapter or a rebuild before the slot can go).
  `services` is the exception: it has every service `interfaces/services.h` declares, imported or
  not (section 9.3).
- `adapters`: a list of `{"interface", "from_major", "name", "args", "result"}`.

### 10.2 What the loader does

For each interface in the module's `majors`:

- A module built against a **newer** major than the manifest's, or whose `majors` or the manifest's
  entry is not a number, is refused.
- The **same** major: nothing to do.
- An **older** major: for each slot (own interface) or import (a group) of the module, compare its
  signature with the manifest's current one. Equal: nothing to do. Different: the manifest must
  have an adapter for the interface, the older major and the name. With none, the load is refused with an error
  naming the module, the interface, the slot or import and both majors. An adapter the loader cannot
  build (below) is refused with the reason.

The adapter makes the call between the two signatures; the module is not rebuilt. The **caller**
makes the call and the **callee** receives the converted call: for a slot, the caller is the host
(with the current signature) and the callee the module (the old one); for an import, the caller is
the module (the old signature) and the callee the host (the current one). `args` has **one entry per
parameter of the callee**: the index of a caller parameter of the same kind, or `{"const": n}`, an
integer constant for an `int` parameter. `result` is `"pass"` (the callee's result is the caller's;
both must have one, of the same kind), `"void"` (the caller has none) or `{"const": n}` (the
caller's `int` result is the constant).

Only reorders, drops, added constants and result changes between these forms fit. The loader
**refuses** any conversion that needs code: a different kind (an `int` into a `linear`, a `linear`
into a `handle`, an `int` into an `f64`), a constant for a parameter that is not `int`, an index out
of range, an `args` of the wrong length, a `result` of another form, a constant result for a caller
whose result is not `int`, a signature of more than 16 parameters. For an adapted import the host must have defined the
import with the manifest's current signature, or the module is refused. The loader judges the adapters from the
entry's `slots` and `imports` and the manifest's `signatures` alone, before it compiles the module, so a refusal does
not depend on what the module exports or imports.

Adapters apply to the module's slots as the host calls them, and to its imports as the module calls
them. They do not apply to a call through the table (2.3).

## 11. Fuel

Fuel bounds the work a module does in a tick, counted by code that is part of the module, so that a
host without engine fuel (a browser) stops a module at the same point. The engine's own fuel is off.

### 11.1 Rule v1

The rule is `fuel` of the game manifest, version 1:

> The counter drops by 1 at each function entry and at the top of each loop body (once per call and
> once per iteration). When it goes below zero the module traps (`unreachable`).

An implementation applies it to the finished module, after linking:

- The counter is a **mutable i64 global defined by the module**. It is neither imported nor exported.
  It starts at the largest i64 (9223372036854775807), so that a start function runs unmetered.
- At the **start of every function the module defines** (before any other instruction) and at the
  **start of every `loop` body** (so a branch back to the loop's top counts again), the module does:
  counter = counter - 1; if counter < 0, `unreachable`.
- Functions the metering adds (below) and imports cost nothing.
- The module exports `__fuel_set(i64)`, which stores its argument in the counter, and
  `__fuel_get() -> i64`, which returns the counter. These are the only way in or out.
- Nothing else about the module changes: no function, global, table, data or memory index moves, and
  the layout of memory and the table is the same. The reference pass adds the counter global, the
  two functions and the two exports after everything the module already has. It refuses a module that
  is already metered, since a second counter would burn twice per step.
- `tools/wasm_fuel.py` is the reference pass, and the data kit carries it as a standalone script (standard
  library and `wasm-tools` only, no game source): `python3 wasm_fuel.py module.wasm -o metered.wasm`, with
  `wasm-tools` at `tools.wasm-tools` of the manifest, found by `--wasm-tools`, `$WASM_TOOLS` or `PATH`. An
  implementation may run it or write its own pass from the rule above: the gate accepts a module by the surface
  (the exports, the counter not exported, no imported global) and by the fuel the module burns against its budget, never
  by how the pass was made. The reference's `build` runs the pass as the last step of building a game module, and
  `check` and `imports` verify the surface (the two exports present, `__fuel_set` of type `(param i64)` and
  `__fuel_get` of type `(result i64)`, no counter exported, no imported global). The loader does not (section 5.2).

Fuel counts the function entries and loop iterations of a module's own code, so it depends on the
final code: a different compiler, an inlining decision or a different optimisation level changes it.
A module's budget belongs to that build.

### 11.2 What the host does

- Before the first outermost slot call of each tick, the host calls `__fuel_set(fuel_budget)`, with
  `fuel_budget` from the module's manifest entry. It does not reset the counter again that tick, so
  the budget covers all the slot calls of the tick together. The tick is the game's tick counter. A trap in
  `__fuel_set` is a trap of that slot call (section 13), and the slot does not run.
- In test mode the host reads `__fuel_get` after the tick; the fuel used is the budget minus the
  value read.
- Running out: a trap with the counter below zero is out of fuel. The game ends with a message that
  names the module, the tick and the budget (section 13).

### 11.3 Budgets

- `fuel_budget` is an integer from 1 to 2147483647. The game manifest's value is ten times the
  most fuel any tick of the conformity gate's scenarios burns (the gate's runs record the maximum);
  the reference's `statics` run checks its stubbed run against the same budget.
- `size_budget` is the most bytes the module's `.wasm` may be; the reference's `check` fails above it.

## 12. No hidden state

A module keeps nothing between ticks outside the block. Game state in a static would be missed by
the synchronisation hash, by saved games, by rejoining a game and by snapshots.

- Statics are the module's memory from its `data_base` up to the bottom of its stack
  (`stack_top - stack_size`), including bss. The stack and the four data words are excluded.
- In a test-mode run, the host hashes the statics after each tick. The first hash is the
  baseline. Any later hash that differs is fatal: the module name and the tick are in the message
  (section 13).
- The reference's `statics` check does the same on a stubbed run of 2000 ticks, with the imports
  replaced by stubs, and fails on the first tick whose hash differs.
- A module may keep constants in its data, and values that are rebuilt every tick from the block
  (for instance, a pointer it computes each call). It must not keep anything the next tick reads.

## 13. Errors and traps

Every error or trap ends the game, in this version. There is no recovery and no skipping a module.

- **Where it is reported.** The message goes to the log. In test mode the process then exits with
  code 2; otherwise the game shows a fatal-error dialog with the message.
- **Start-up errors** (a missing `.wasm`, a manifest or loader refusal from section 5, a module
  that fails to instantiate) end the game before the match. The message starts with the module name
  and then the loader's text, which names the module (and both modules of an overlap).
- **A trap in a slot or in an import's error.** The message is
  `<module> trapped at tick <tick>: <engine text>`. Before the match it reads
  `<module> trapped before the match: ...`. The tick is the game's tick counter. Causes:
  `unreachable`, an out-of-bounds memory access, a division by zero, a call through an empty table
  entry or one of the wrong type, a stack-overflow trap of the engine, an integer overflow trap, and an
  import returning an error (below).
- **Out of fuel.** `<module> ran out of fuel at tick <tick>: it burned more than its fuel_budget
  of <budget> in one tick`.
- **Hidden state.** `<module> kept state between ticks: its statics changed at tick <tick>`.
- **An import's refusal** is a trap whose text begins `import <name>: ` and names the cause:
  `pointer argument <n> is outside the module memory`, `the buffer size is below 1`,
  `the buffer runs past the module memory`, `unknown handle <n>`, `null handle`,
  `returned a buffer other than the one it was given`, `returned a weapon outside the sim block` (or an `object`, a
  `region` or a `pointer`, by the result's mode), `returned a pointer outside the module memory`, `the world view is not
  set`; `SimFree` adds `the pointer is outside the sim block`; `Emit` adds `the size is outside an effect`, `the effect runs past the module memory`, `the kind is not in the module's publishes table`, `the kind is not routed from the module's outbox` and `the size is not the payload's size of its kind`, and a per-kind hook adds its own (wind_meteors' SPAWN: `the weapon slot is outside the table of weapons`).
- **Glue errors** name the slot: `slot <name>: g_game is not in the sim block`, `slot <name>: the region is not in the
  sim block`, `slot <name>: an object argument is not in the sim block`,
  `slot <name>: g_simGlobals is not in the sim block`, `slot <name>: the argument does not fit the
  scratch area`, `slot <name>: called inside an import` (a slot call made while an import runs; the
  call does nothing and the game ends once the outer call returns), and `<module>: the handle table
  is full`.
- **A service that ends the game on purpose.** `FatalError(message)` does not end the process inside
  the call. The import returns an error carrying the message, so the call unwinds as a trap does, and
  the game ends with the message itself (not a trap text) once the call has returned: logged, then
  exit code 2 in test mode, otherwise the fatal-error dialog titled `Byte Tactics`. A fatal of the
  game's own code reached inside an import (a parse error of a TDF file) ends the game the same way.

A conformance run uses these as pass/fail: a module that traps or runs out of fuel in a scenario
fails it.

## 14. Worked example: the Rust fixture

`tests/wasm/modules/rust` is the feasibility fixture for sections 2 and 3: a `no_std` Rust module
built by the commands of 3.1. It exercises the **memory and table imports, the fixed base, the
window and the table range, and an element segment in the module's range**. It does **not**
exercise the rest of this page: it exports `module_data` and `module_slot` (a function returning the
address of one data word, and one returning the address of a function-pointer cell), not slots by
name; it has no data words, no services, no fuel exports and no `imports`. It is checked by
`uv run tools/wasm_modules.py check`, and by `imports` against the two exports it has; the game's
`statics` check does not run it.

The fixture manifest gives it: `memory_pages` 4, `table_size` 64, `window` 65536, `table_slots` 16,
`stack_size` 4096, and for module `rust`, `data_base` 65536 and `table_base` 16. Build it, from
`tests/wasm/modules/rust`, with the command of 3.1 and `D` = 65536, `T` = 16, `N` = 4096 (and
`--offline` to stay on the committed lockfile). Then:

1. Create a memory of 4 pages with the same minimum and maximum, and a funcref table of 64 entries
   with the same minimum and maximum.
2. Define them as `env.memory` and `env.__indirect_function_table` and instantiate the module. It
   has no other imports.
3. The module's element segment has put its function at table entry 16, `table_base`. Calling the
   function at that entry with 7 returns 4342 (`slot_result` of the fixture manifest), 7 times 3 plus
   the data word 4321 (`data_word`).
4. `module_data()` returns an address inside the window, 65536 up to 131072, and the word at that
   address is 4321.

The same fixture's C counterpart is linked at `data_base` 131072 and `table_base` 32, so both fit
in one memory and one table side by side; the fixture manifest's windows and ranges do not overlap.

## 15. Judging a module

`tools/oraclegate.py` judges a module by behaviour alone. It plays the suite and the subsystem's stored recordings with the
submitted module in place of the default one, and writes a result that holds verdicts and the module's own values.

```sh
uv run tools/oraclegate.py MODULE.wasm --subsystem wind_meteors [--full | --recordings-only] [--manifest M] [--out DIR] [--build-dir build/modern] [--game-dir DIR]
```

Every run first removes `DIR/result.json`, `DIR/runtime/` and `DIR/logs/`, so a run that stops with an error leaves no
earlier result behind. It has two routes, and the result's `scope` names the one it took:

- **`scenarios`**, the steps below: the suite and the recordings. It is the route of a subsystem whose manifest entry builds
  (no `blocked`), so that the game loads the module in every scenario.
- **`recordings`**, for a subsystem whose entry is `blocked` (its default does not build to WASM, so no scenario can load a
  module of it), or any subsystem with `--recordings-only`: step 1, then every recording of `tests/replay/<subsystem>/`
  replayed against the module, one `replay_native_test.exe --wasm` run per recording (`docs/replay-format.md`, "Against the
  WASM module"). The scenarios are not played: the result has `scenarios: not_judged`, and the module has met its contract
  on those recordings only. `--manifest` judges against another game manifest than `wasm/modules.json`, on this route only.
  Before the first recording it checks the host: the build must have `replay_native_test.exe` and `wasmtime_host.dll`, and
  `wine replay_native_test.exe` with no arguments must print the replayer's usage line, which proves that wine starts a
  program in its prefix. When a check fails the run plays nothing and its result is an error of the host (below).

**A blocked entry is judged without budgets.** It has no `size_budget` or `fuel_budget` (section 4.2), so check-module does
not judge the module's size, and the replayer runs it with the largest fuel budget of section 11.3, 2147483647 a tick
(`replay_native_test.exe --wasm MODULE.wasm --no-budgets`). Everything else of step 1 is checked as for any module. The
result has `budgets: not_judged`, and the summary line ends `budgets not judged`. A module that is equal there has met its
contract on its recordings, and its fuel and size are judged once the entry has budgets. Every other result has
`budgets: judged`.

The recordings route names the replay rules it judged under in `rules`:

- `queries`: the imports answered as pure queries, by their arguments, whenever the module calls them. They are not counted
  and their order among the other imports is not compared, so a module that asks more often, less often or at another point
  of the call is equal. `services.GetGafFrame` is the one (section 9.3); `InitGafSequence` and `StepGafSequence` change the
  cursor they take, so they stay in the recorded order. A query whose arguments no recorded call has is reported at the
  query (`query services.GetGafFrame`), with the arguments the module passed.
- `unread_arguments`: arguments the callee never reads, which are left out of the comparison of the import's arguments.
  `los_radar.GetMapCellAtPosition pos.y` is the one: the cell lookup reads only `x` and `z`, and the reclaim scan (the
  pick of a random reclaimable feature in a radius) passes a position whose `y` it never sets, so the recorded `y` is whatever was on
  the stack and no module can be asked to reproduce it. The `x` and `z` are compared.

1. It runs `uv run tools/wasm_modules.py check-module MODULE.wasm --name <subsystem>` on the file: the data, bss and stack
   inside the entry's window and the table entries inside its table range, no import off the manifest's list (no global,
   WASI, clock, random, file or network import), exactly the manifest's exports, the manifest's kinds against the wasm
   signatures, the metering surface, and the size budget (section 5.2, the checks the loader leaves to the build). A problem
   ends the run before the game starts: the problems are the result's `message`, the verdict is `refused`. The interface
   majors are not part of it. When the check could not run (exit 2), the run stops with an error and writes no result.
2. It copies the runtime files of the modern build (the exe, `wasmtime_host.dll`, `modules.json` and every module as built) into
   `DIR/runtime/`, removing what was there, and puts `MODULE.wasm` in the place of `<subsystem>.wasm`. The manifest is kept.
3. It starts the game on a ten-tick scenario. A refusal of section 5.2 (and any other error before the match, section 13) ends
   that start, and the loader's message is the result: `refused`. A module linked at another base inside its window is not
   refused by either step (section 5.2): it is judged like any other, and differs where its data is not where the loader zeroed
   it.
4. It plays `tools/reference.py --check --end-of-tick --tier` on that runtime (`--full`: the full `--check --end-of-tick`), and then
   `tools/replay.py --check --subsystem <subsystem>` on the recordings of `tests/replay/<subsystem>/`. The comparisons are theirs.
   `--end-of-tick` judges each scenario on its `state` column (the `tick` row, `docs/trace-format.md`, section 6), the state at
   the end of each tick, and reports the `digest` column (every row of the tick) and the `view` column (the `view` row, the player view of the tick, `docs/view.md`, "The recorded view") beside it without failing on either.
   The shared gate lock is kept, a cached pass is not used, and the key of the cache covers the module, so a cached pass could
   not stand for another one.

`DIR/result.json` (default `build/oracle/result.json`) is built from fields parsed out of the two reports, never from their text:

| Field | Meaning |
| --- | --- |
| `format` | 4 |
| `scope` | `scenarios` or `recordings`: the route the run took |
| `budgets` | `judged`, or `not_judged` for a blocked entry's module, judged without its size and fuel budgets |
| `rules` | with `recordings`: `queries` and `unread_arguments`, the replay rules above |
| `verdict` | `equal`, `differs`, `failed` (a run did not finish, or a check other than the traces failed) or `refused`; a scenario counts by its `state` verdict |
| `digest` | `equal`, `differs` or `not_judged`: whether every judged scenario is also equal on every row of each tick (absent with `refused`) |
| `view` | `equal`, `differs` or `not_judged`: whether every judged scenario is also equal on the `view` row of each tick (absent with `refused`) |
| `subsystem`, `suite` | the subsystem, and `tier` or `full` (with `scenarios`) |
| `module` | `sha256` and `bytes` of the submitted file |
| `kit` | `interfaces` and `reference_traces`: the newest `interfaces-v*` and `reference-traces-v*` tags |
| `seconds` | wall time of the run |
| `message` | with `refused`: the problems `check-module` found or the loader's message, and no `scenarios` or `recordings` |
| `scenarios` | `not_judged` with `recordings`; otherwise by scenario name: `status` (the end-of-tick state) and, for `differs`, `tick`, `phase`, `field` and `value`; `digest` and `view`: `status` `equal`, `differs` with the first `tick` whose digest (or view) differs, or `not_judged`; an `other_checks` entry (`failed`) when every scenario is equal and another check of the run failed, with `reason` `block-pointer` when it was the block pointer check: a fault of the harness's scan of the shared state block, not of the module |
| `recordings` | by `<subsystem>/<recording>`: `status` and, for `differs`, `line`, `tick`, `phase` (the slot), `kind`, `field` and `value`; with `recordings`, `call` (the replayer's call index) in place of `line` and `kind`, and `field` is the replayer's key (a field, `import <n> <name>` or `query <name>`) |
| `reason`, `class` | in a scenario or recording whose `status` is `failed`: why it did not run to its end, below |

A `status` is `equal`, `differs`, `failed` (the scenario did not run to its end) or `not_judged` (the run stopped at the first
divergence before this scenario was reported).

**Why a run failed.** A `failed` scenario or recording has a `reason`, one word of a closed set that says whose fault it is.
It is parsed from the run's output, and no text of the output (no message of wine, the replayer, the engine or the module, and
no source name) is in the result.

| `reason` | Whose fault | When |
| --- | --- | --- |
| `host-runtime` | the host's | wine did not start the program (its prefix does not load, say), or the replayer could not load `wasmtime_host.dll` |
| `host-replayer` | the host's | wine found no `replay_native_test.exe` |
| `host-timeout` | the host's | the run did not end in its time: the replayer's hour, or a scenario's timeout |
| `module-load` | the module's | the loader refused the module (section 5.2); `class` is `compile`, `instantiate`, `export` or `major` (an interface-major refusal, section 10), the classes of `docs/core-gate.md`, "loader and fuel" |
| `module-trap` | the module's | the module trapped or was stopped (section 13); `class` is `unreachable`, `memory_access`, `divide`, `table`, `stack` or `trap` (any other engine trap) for an engine trap, `out_of_fuel`, `import` (an import's refusal), `glue` (a glue error or a full handle table) or `fatal` (the module called `FatalError`) |
| `unknown` | either | anything else: a crash of the process, a stop of the replayer that is none of the above, or a run that ended without a verdict |

Only `module-load` and `module-trap` have a `class`. A module that traps on a recording gets `failed` with `reason:
module-trap` and the trap's `class`; on the scenario route the same words come from the game's stop message (section 13)
where `tools/reference.py` reports it, and are `unknown` where it does not.

**An error of the host.** When the host check of the recordings route fails, the run writes `DIR/result.json` with only
`format`, `subsystem`, `reason` (`host-runtime` or `host-replayer`) and `error`, a fixed sentence for the reason that says the
fault is the host's, and exits 2. The clean room's broker answers such a result with an `error` and its `reason`, never
a verdict, and passes on no other text: an `error` result whose `reason` is not one of those two is answered with
`reason: unknown`. The broker's own timeout is an `error` with `reason: host-timeout`. Only the first divergence of a run carries `phase`, `field` and `value` for a
scenario: the others have their `tick`. A `value` is what the module produced; the reference's value, its memory and anything of
the build other than the names of the traced fields are never in the file. The raw reports are kept in `DIR/logs/` for the host
and are not part of the result.

A module that is equal on `state` and differs on `digest` kept the end-of-tick contract with another order of work inside the
tick: the state at the end of each tick is 3.1c's, and some phase row in between is not. Format 1 had no `digest` and judged
the scenarios on every row; format 2 had no `view`; format 3 had no `scope` and no recordings route.

Exit status: 0 when everything is equal, 1 when something differs or failed, 2 when the loader refused the module or the host
check failed.

#519 wires the tool to the sandbox's mailbox: a submitted module goes in, and `result.json` comes back.

## 16. Appendix: values in the manifests

The values below are read from the files named; they are copied here for orientation only.

| Name | Game manifest | Fixture manifest |
| --- | --- | --- |
| `memory_pages` | 4112 | 4 |
| `table_size` | 528 | 64 |
| `window` | 32768 | 65536 |
| `table_slots` | 32 | 16 |
| `stack_size` | 4096 | 4096 |
| `block_base`, `block_size`, `block_max` | 1048576 (0x100000), 33554432, 268435456 | none |
| `scratch` | `[983040, 65536]` (0xF0000) | none |
| `build_flags` | `-mno-nontrapping-fptoint` | none |
| `fuel` | version 1 | none |
| `majors` | one per interface header, as `wasm/modules.json` records them; they move with the headers | none |
| module `wind_meteors` | `data_base` 196608 (0x30000), `table_base` 48, `fuel_budget` 250, `size_budget` 13100 | |
| the 14 subsystems | entry *i* (1 to 14, section 2.1): `data_base` 0x30000 + *i* * 0x8000, `table_base` 48 + *i* * 32 | |
| module `rust` | | `data_base` 65536, `table_base` 16 |
| module `c` | | `data_base` 131072, `table_base` 32 |
