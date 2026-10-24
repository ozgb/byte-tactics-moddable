#ifndef FILE_LIST_H
#define FILE_LIST_H

struct FileRequester;
struct Gui;

void __stdcall RefreshFileList(FileRequester* obj);
void __stdcall FileRequesterHandler(Gui* gadget);
void __stdcall EnableBlinkWords(int);
void __stdcall DisableBlinkWords(int);
void __stdcall ClearBlinkWord(void* param1, int param2);

#endif
