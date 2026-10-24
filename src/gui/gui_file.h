#ifndef GUI_FILE_H
#define GUI_FILE_H

struct FileHandle;

void __stdcall WriteSectionHeader(FileHandle* file, char* name, int depth);
void __stdcall WriteSectionEnd(FileHandle* file, int depth);
void __stdcall WriteKeyValue(FileHandle* file, char* key, char* value, int depth);
void __stdcall WriteGuiFile(char* obj, char* name);

#endif
