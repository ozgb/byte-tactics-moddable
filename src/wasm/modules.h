#ifndef WASM_MODULES_H
#define WASM_MODULES_H

#include "wasmloader.h"

struct ModuleMemory {
    unsigned char* base;
    unsigned size;
    unsigned scratch;
    unsigned scratchSize;
};

void __stdcall ModulesHideSwitch(char* cmdLine);
void __stdcall ModulesCheckStatics(unsigned tick);
void __stdcall ModulesFatal(const char* text);
const ModuleMemory* __stdcall ModulesMemory();

int __stdcall WindMeteorsInstall(wl_loader* loader);
void __stdcall WindMeteorsUseSeam();
int __stdcall WindMeteorsBind(wl_loader* loader, wl_module* module);
void __stdcall WindMeteorsDataRanges(unsigned* addresses, unsigned* count);

#endif
