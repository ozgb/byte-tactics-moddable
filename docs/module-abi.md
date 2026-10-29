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
15. [Appendix: values in the manifests](#15-appendix-values-in-the-manifests)

## 1. Conventions

- All values are little-endian. "i32" and "f64" are the WebAssembly value types; an address or an
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
  `adapters`) and `tools/wasm_fuel.py`. They are the executable form of the rules below.

## 2. The shared memory and table

Every module runs in **one linear memory and one function table** that the loader creates before
any module is loaded. Modules do not create or grow them.

### 2.1 The memory

- The module imports it as `env.memory`. Its size is `memory_pages` of the manifest (528 pages of
  65536 bytes in the game manifest, 34603008 bytes). Its minimum and maximum are both
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
  | the state block, 32 MiB | 0x100000 (1048576) | `block_base`, `block_size` |

  The block ends exactly at the end of the memory: `block_base + block_size` equals
  `memory_pages * 65536`.
- Windows sit below the scratch area. The loader refuses a window that reaches the scratch base or
  the block base (section 5).
- **The state block** holds the whole simulation state. The engine allocates and owns it; modules
  read and write it through the addresses the engine gives them. A module keeps nothing between
  ticks outside the block (section 12).
- **The scratch area** is where a native host puts a copy of a slot argument that does not live in
  the block, for the length of one call (section 7). A module never allocates from it and never
  keeps an address in it.

### 2.2 A module's window

A **window** is the `window` bytes (65536 in the game manifest) starting at the module's
`data_base`. A module's data, bss and stack must lie entirely inside its window:

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
  (80 entries in the game manifest, 64 in the fixture manifest), minimum equal to maximum, every entry
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

- `--no-stack-first` is required: without it lld puts every module's stack at address 0.
- Do not pass `--allow-undefined`. Every function the module does not define must be one of its
  declared imports (section 9), so a missing definition is a link error, not a runtime trap.
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
- **NaN bit patterns.** The engine canonicalises NaNs. A browser does not. A module must not let a
  result depend on a NaN's bit pattern.
- **Freestanding.** The reference compiles C and C++ with `-nostdinc`, against its own small
  headers (`wasm/include`) and a small C runtime (`wasm/crt`: `memcpy`, `memmove`, `memset`,
  `memcmp`, `strcpy`, `strlen`, `strcmp`, `strncpy`, `_strcmpi`). There is no host libc and no
  exceptions or RTTI. C++ is compiled with `-fno-rtti -fno-exceptions`.
- **WebAssembly features.** The reference modules use the WebAssembly 1.0 instructions plus
  sign extension and bulk memory (which wasm-ld uses to zero bss). The engine's configuration
  gives deterministic relaxed SIMD and NaN canonicalisation and has no threads or fuel; do not rely
  on more than the reference modules use.
- **Value types at the boundary** are i32 and f64 only (section 7). No slot or import takes or
  returns an i64, an f32 or more than one result.
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
| `stack_size` | bytes of every module stack | host and checks (the loader accepts it) |
| `block_base`, `block_size` | the state block: both or neither | loader |
| `scratch` | `[base, size]` of the scratch area | loader |
| `majors`, `signatures`, `adapters` | section 10 | loader |
| `modules` | a non-empty list of module entries | loader, required |
| `tools` | the pinned tool versions: `clang`, `wasm-tools` (game manifest); `clang`, `rustc`, `wasmtime` (fixture manifest) | checks |
| `build_flags` | compile flags for every game module | build |
| `fuel` | `{"rule": text, "version": 1}`, the metering rule of section 11 | checks |

A number must be a non-negative integer.

### 4.2 Module entries

| Field | Meaning | Read by |
| --- | --- | --- |
| `name` | a non-empty string, unique in the manifest | loader, required |
| `data_base`, `table_base` | the module's window and table range (section 2) | loader, required |
| `exports` | the slot names, in the interface's slot order | loader; checks |
| `data` | the loader data words (section 8.2) | loader; checks |
| `interface` | the interface whose slots the module implements | loader (majors) |
| `majors` | the major of each interface it uses, section 10 | loader |
| `slots` | `{slot: [[argument kinds], result kind]}` | loader (adapters); checks |
| `imports` | `{group: {import: [[argument kinds], result kind]}}` | loader (adapters); checks |
| `fuel_budget` | fuel per tick, section 11 | host; checks |
| `size_budget` | the most bytes the `.wasm` may be | checks |
| `cxx`, `c` | source files of the module in the reference repository | the reference build only |

A kind is one of `int`, `f64`, `void` (a result only), `linear`, `block`, `scratch`, `handle`
(section 7). A signature has at most 16 arguments and at most one result. `exports` and `data`
must be lists of names.

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
them at start-up (section 13).

- A manifest that is not valid JSON or not an object; a required field missing; a number that is
  not a non-negative integer; `memory_pages` or `window` zero, or `memory_pages` over 65536; a
  `scratch` that is not `[base, size]`.
- A block that does not fit the memory; a scratch area that does not fit the memory or that passes
  the block base.
- No modules, a module without a name, a duplicate name, a missing `data_base` or `table_base`,
  `exports` or `data` that is not a list of names.
- A module whose window does not fit the memory, or whose table range does not fit the table.
- A window that reaches the block base or the scratch base.
- Two windows that overlap, or two table ranges that overlap.
- A module not in the manifest, loaded twice, or loaded before the memory exists.
- A module that does not compile, or does not instantiate: an import the host did not define, an
  import whose type differs from the host's (the host's types are `i` for i32 and `d` for f64),
  an import of the memory or table with a minimum above the real size or a maximum below it.
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

A conforming build runs those checks.

## 6. Slots

A slot is a function export of the module's wasm instance, named exactly like the interface's slot
(`UpdateWind`, `SetMeteorParams`). The module's `exports` lists them, in the interface's slot order.

- **Finding.** The host finds a slot by export name in the module's instance. It never looks one up
  in the table. (The loader keeps the slots in the order of `exports`, and puts free table entries in
  that order, but a host calls by name.)
- **Signature.** A slot takes at most 16 arguments and returns at most one result, each i32 or f64,
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
  only the outermost call as the boundary (sections 8 and 11).

## 7. Argument kinds and the pointer rule

A kind says what an argument or a result is. The wasm type of `int`, `linear`, `block`, `scratch`
and `handle` is i32; of `f64`, f64; `void` is no result.

| Kind | At the boundary |
| --- | --- |
| `int` | a 32-bit integer: a count, an enumerator (a random stream is 0 for the main stream, 1 for the load stream), a flag, a signed value |
| `f64` | an IEEE 754 double |
| `void` | no result |
| `linear` | an address in the shared memory; 0 is null. The object it points to is in the module's window, on its stack, or in the block (or in the scratch area, for a slot argument). A callee reads and writes the object in place |
| `block` | a linear address that lies inside the state block, or 0. As a result, the host checks that it lies inside the block |
| `scratch` | a linear address in the scratch area, where the host put a copy of a native object for this call |
| `handle` | an opaque u32 (section 8) |

### 7.1 The pointer rule for Milestone 2

- **A pointer stored in the block is a block offset.** It is a 32-bit offset from the block's base
  (0 is null), not an address. A module and the engine store and read the same bytes. To follow it,
  add the block base: the value of the data word `g_blockBase` (section 8), which equals
  `block_base`. To store one, subtract the base. A record that stores a pointer has a field of this
  kind (a `Ref`); the record layout says which fields.
- **A pointer argument is a linear address.** Every pointer given to a slot or to an import, and
  every pointer returned, is an address in the shared memory, never an offset and never a native
  address.
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
  takes it (`CommandArgs_GetIntArg`); its raw bytes are for the engine.
- An import's `linear` argument is checked to lie inside the memory (not past its size); an
  argument outside it is a trap that names the import and the argument index. The check does not
  look at how many bytes the callee will use, so the module is responsible for passing a buffer big
  enough.
- A `block` result that is not inside the block is a trap.

## 8. Handles and the loader data words

### 8.1 Handles

A **handle** is how the engine gives a module a thing the module can only hand back, never look at:
a TDF record, a bank of saved-game items (`HapiBank`).

- It is an opaque `u32`: index plus 1 into a table the host keeps; 0 is the null handle.
- It is valid **only until the outermost slot call returns**. The host clears its table then, so a
  handle kept in the block, in a static or in a `TdfFile` that outlives the call is garbage later,
  and a later call may give the same number to something else.
- Within one outermost call, the same engine object always has the same handle. The table holds 32
  handles; more than 32 distinct objects in one call is a fatal error.
- A module must **never read through a handle**: it is not an address. It only passes it back to an
  import whose parameter kind is `handle`. An unknown handle (above the number issued) and a null
  handle where an object is required are traps naming the import.
- A `TdfFile` object (three 32-bit words: root record handle, current record handle, flags) lives
  in the module's own memory, usually on its stack, and the TDF imports write the two handles into
  it. A module therefore constructs, uses and destroys a `TdfFile` inside one slot call.

### 8.2 The loader data words

A module exports four **data words**. Each is an exported i32 global whose value is the address, in
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
  arguments, `f64` exactly where the wasm type is f64, and `void` exactly where there is no result.

The reference's `imports` check enforces all of this on every built module. It is the check that
catches non-determinism at the boundary whatever language the module is written in.

### 9.2 Interface imports

A group named after an interface imports the interface's slots by their names, with the interface's
signature. The game manifest's module imports `weapons.SpawnProjectile` and
`weapons.FindWeaponByName`. The host defines each by calling the engine's slot of the same name.

| Import | Signature (kinds) | Behaviour |
| --- | --- | --- |
| `weapons.SpawnProjectile` | `[block, linear, linear, int] -> int` | adds a projectile of the weapon at a position with a velocity (two 3-vectors in the memory); returns 0 when the projectile table is full |
| `weapons.FindWeaponByName` | `[linear] -> block` | finds a weapon definition by its TDF name (a NUL-terminated string); returns its address in the block, or 0 |

### 9.3 Engine services

The `services` group is the engine's plain functions and the member functions of its classes. A
C++ member is imported as `<Class>_<Member>`, with `this` as the first argument, a `linear` address
when the object lives in the module's memory and a `handle` when it is an engine object. Their
signatures are `signatures.services` of the manifest.

| Import | Signature (kinds) | Behaviour |
| --- | --- | --- |
| `GameRand` | `[int] -> int` | advances the named random stream and returns the next value, 0 to 0x7fff; the stream's state is in the block |
| `RandomInt` | `[int] -> int` | a random number in `[0, range)` from the main stream |
| `FatalError` | `[linear] -> void` | shows the NUL-terminated message and ends the game; does not return |
| `BuildDataPath` | `[linear, linear, linear, linear] -> linear` | builds `dir/name.ext` below the data directory into the buffer (first argument) and returns the buffer's address; the host traps if the result is not the buffer |
| `TdfFile_Construct` | `[linear] -> void` | builds an empty `TdfFile` in place |
| `TdfFile_Destruct` | `[linear] -> void` | unloads the file and frees the engine's records |
| `TdfFile_LoadFile` | `[linear, linear] -> int` | loads and parses the file at a path; the result is the engine's status |
| `TdfFile_SelectRecord` | `[linear, linear] -> int` | selects the record of the given name as the current one |
| `TdfRecord_GetFieldInt` | `[handle, linear, int] -> int` | reads an integer field, or the default (third argument) when it is missing |
| `TdfRecord_GetFieldDouble` | `[handle, linear, f64] -> f64` | reads a double field, or the default |
| `TdfRecord_GetFieldString` | `[handle, linear, linear, int, linear] -> int` | copies a field's text into a buffer (arguments: record, destination, key, size, default) |
| `CommandArgs_GetIntArg` | `[scratch, int, int] -> int` | reads argument *index* of a console command as an integer, or the default |
| `HapiBank_OpenAccount` | `[handle, linear] -> int` | opens the named account of a saved-game bank |
| `HapiBank_SetIntegerItem` | `[handle, linear, int] -> int` | writes an integer item of the open account |
| `HapiBank_GetIntegerItem` | `[handle, linear, int] -> int` | reads an integer item, or the default |

A service never returns a native address. A service that finds something outside what it may reach
(a pointer outside the memory, an unknown handle) traps with a message that names it (section 13).

## 10. Interface majors, signatures and adapters

An interface has a major version that changes when a slot is removed or its signature changes.
(A minor version, for an added slot, needs nothing here.)

### 10.1 What the manifest records

- `majors`: the current major of each interface and of each import group's interface, read from the
  interface headers. The game manifest has, for example, `services` 1 and `wind_meteors` 1.
- `modules[].interface`: the interface whose slots the module implements.
- `modules[].majors`: for its own interface and for every import group, the major the module was
  built against.
- `signatures`: for each interface, `{name: [[kinds], result]}`, the **current** signature of every
  slot and import the manifest's modules use.
- `adapters`: a list of `{"interface", "from_major", "name", "args", "result"}`.

### 10.2 What the loader does

For each interface in the module's `majors`:

- A module built against a **newer** major than the manifest's, or whose `majors` or the manifest's
  entry is not a number, is refused.
- The **same** major: nothing to do.
- An **older** major: for each slot (own interface) or import (a group) of the module, compare its
  signature with the manifest's current one. Equal: nothing to do. Different: the manifest must
  have an adapter for `(interface, from_major, name)`. With none, the load is refused with an error
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
whose result is not `int`, more than 16 values. For an adapted import the host must have defined the
import with the manifest's current signature, or the module is refused.

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
- `tools/wasm_fuel.py` is the reference pass. It reads and writes a module through `wasm-tools`.
  The reference's `build` runs it as the last step of building a game module, and `check` and
  `imports` verify the surface (the two exports present, no counter exported, no imported global).

Fuel counts the function entries and loop iterations of a module's own code, so it depends on the
final code: a different compiler, an inlining decision or a different optimisation level changes it.
A module's budget belongs to that build.

### 11.2 What the host does

- Before the first outermost slot call of each tick, the host calls `__fuel_set(fuel_budget)`, with
  `fuel_budget` from the module's manifest entry. It does not reset the counter again that tick, so
  the budget covers all the slot calls of the tick together. The tick is the game's tick counter.
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
  `pointer argument <n> is outside the module memory`, `unknown handle <n>`, `null handle`,
  `returned a buffer other than the one it was given`, `returned a weapon outside the sim block`.
- **Glue errors** name the slot: `slot <name>: g_game is not in the sim block`,
  `slot <name>: g_simGlobals is not in the sim block`, `slot <name>: the argument does not fit the
  scratch area`, and `<module>: the handle table is full`.
- **A service that ends the game on purpose.** `FatalError(message)` shows the message and ends the
  game; it is not a trap.

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

## 15. Appendix: values in the manifests

The values below are read from the files named; they are copied here for orientation only.

| Name | Game manifest | Fixture manifest |
| --- | --- | --- |
| `memory_pages` | 528 | 4 |
| `table_size` | 80 | 64 |
| `window` | 65536 | 65536 |
| `table_slots` | 32 | 16 |
| `stack_size` | 4096 | 4096 |
| `block_base`, `block_size` | 1048576 (0x100000), 33554432 | none |
| `scratch` | `[983040, 65536]` (0xF0000) | none |
| `build_flags` | `-mno-nontrapping-fptoint` | none |
| `fuel` | version 1 | none |
| module `wind_meteors` | `data_base` 196608 (0x30000), `table_base` 48, `fuel_budget` 250, `size_budget` 13100 | |
| module `rust` | | `data_base` 65536, `table_base` 16 |
| module `c` | | `data_base` 131072, `table_base` 32 |
