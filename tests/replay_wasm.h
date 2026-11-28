#ifndef TESTS_REPLAY_WASM_H
#define TESTS_REPLAY_WASM_H

void ReplayWasmSelect(const char* wasmPath, const char* subsystem = nullptr);
const char* ReplayWasmRefusal(const char* text);
void ReplayWasmSetTrapHook(void (*hook)(const char* errorClass, const char* text));
bool ReplayWasmActive();
const char* ReplayWasmSubsystem();
void ReplayWasmReload();
void ReplayWasmUseSeam();
void ReplayWasmNoBudgets();
unsigned char* ReplayWasmLinear(unsigned address);
unsigned ReplayWasmLoadDataBase();

#endif
