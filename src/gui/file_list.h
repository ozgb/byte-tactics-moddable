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
void __stdcall AllocBlinkWords(Dialog* obj, int count);
void __stdcall SetBlinkGadget(Dialog* obj, int value);
void __stdcall ClearBlinkWords(Dialog* obj);
void __stdcall DrawBlinkWords(Dialog* obj);

#endif
