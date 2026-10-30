#ifndef FILE_LIST_H
#define FILE_LIST_H
struct Gui;

struct FileRequester;
struct Gui;

void __stdcall RefreshFileList(FileRequester* obj);
void __stdcall FileRequesterHandler(Gui* gadget);
void __stdcall EnableBlinkWords(Gui*);
void __stdcall DisableBlinkWords(Gui*);
void __stdcall ClearBlinkWord(Gui* param1, int param2);

void __stdcall SortFileList(char* list1, char* list2, int* keys, int count);
void __stdcall AllocBlinkWords(Gui* obj, int count);
void __stdcall SetBlinkGadget(Gui* obj, int value);
void __stdcall ClearBlinkWords(Gui* obj);
void __stdcall DrawBlinkWords(Gui* obj);

void __stdcall FreeBlinkWords(Gui*);

#endif
