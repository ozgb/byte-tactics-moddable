#ifndef CD_CHECK_H
#define CD_CHECK_H

void __stdcall RegisterDataArchives();
int __stdcall UsesDataArchives(void);
void* __stdcall AllocZeroedWithTickOffset(unsigned int size);

#endif
