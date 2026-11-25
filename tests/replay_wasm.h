#ifndef TESTS_REPLAY_WASM_H
#define TESTS_REPLAY_WASM_H

void ReplayWasmSelect(const char* wasmPath, const char* subsystem = nullptr);
void ReplayWasmSetTrapHook(void (*hook)(const char* errorClass));
bool ReplayWasmActive();
const char* ReplayWasmSubsystem();
void ReplayWasmReload();
void ReplayWasmUseSeam();

#endif
