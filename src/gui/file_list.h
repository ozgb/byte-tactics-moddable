#ifndef FILE_LIST_H
#define FILE_LIST_H

struct FileRequester;
struct Gui;

void __stdcall RefreshFileList(FileRequester* obj);
void __stdcall FileRequesterHandler(Gui* gadget);
void __stdcall EnableBlinkWords(int);
void __stdcall DisableBlinkWords(int);
void __stdcall ClearBlinkWord(void* param1, int param2);

void __stdcall SortFileList(char* list1, char* list2, int* keys, int count);
void __stdcall AllocBlinkWords(Gui* obj, int count);
void __stdcall SetBlinkGadget(Gui* obj, int value);
void __stdcall ClearBlinkWords(Gui* obj);
void __stdcall DrawBlinkWords(Gui* obj);

#endif
