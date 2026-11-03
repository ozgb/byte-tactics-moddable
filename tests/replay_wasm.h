#ifndef TESTS_REPLAY_WASM_H
#define TESTS_REPLAY_WASM_H

void ReplayWasmSelect(const char* wasmPath);
bool ReplayWasmActive();
const char* ReplayWasmSubsystem();
void ReplayWasmReload();
void ReplayWasmUseSeam();

#endif
