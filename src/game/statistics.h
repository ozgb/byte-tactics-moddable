#ifndef STATISTICS_H
#define STATISTICS_H

int __stdcall AllocScoreTables();
bool __stdcall IsReporterDllLoaded();
void __stdcall ShutdownScoreTables();

int __stdcall EnableReporter(int enable);

#endif
