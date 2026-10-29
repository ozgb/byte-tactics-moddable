# wasmtime C API headers

The C API headers of wasmtime 50.0.0, so that building the game needs no cargo.

- `include/` is the `include/` directory of the `wasmtime-c-api-impl` 50.0.0 crate (the C headers only, not the `.hh` C++ ones), as `tests/wasm/host/dll` pins it.
- `include/wasmtime/conf.h` is not from the crate. `tools/wasm_host.py` writes the same file (`CONF_H`): it defines `WASMTIME_FEATURE_GC` to match the DLL, since the layout of `wasmtime_val_t` depends on it.
- `LICENSE` is the wasmtime 50.0.0 licence file (Apache-2.0 WITH LLVM-exception); the crate does not ship one.

`uv run tools/wasm_host.py headers` fails when these files differ from the crate's (`--update` copies the crate's over them).
