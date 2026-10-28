#ifndef DIALOGS_H
#define DIALOGS_H
struct PaletteEditorDialog;
struct RemapPaletteEntry;

struct Gui;

struct FileHandle;

int __stdcall IsPointInEntryRect(Gui* obj);
int __stdcall HasLayer(Gui* obj);
void __stdcall SetCursorHover(Gui* p, int alt);
void __stdcall ApplySharedCursorFrame(Gui* p);
void __stdcall UpdateCursorAndMouse(Gui* p);
int __stdcall GetClickMode(Gui* obj);
void __stdcall CommitTextEdit(Gui* control, int index, char* text, int maxLength, int clear);
int __stdcall HandleTextEditKey(Gui* control, int index, int key);
int __stdcall OpenConfirmDialog(Gui* sub, char* title);
void __stdcall YesNoDialogHandler(Gui* gadget);
int __stdcall OpenYesNoDialog(Gui* sub, char* title, char* chc2, char* chc1);
void __stdcall MessageBoxHandler(Gui* menu);
int __stdcall IsMessageBoxScreen(Gui* obj);
void __stdcall NotExistDialogHandler(Gui* obj);
int __stdcall OpenNotExistDialog(Gui* sub, char* name);
void __stdcall Choice3DialogHandler(Gui* gui);
int __stdcall OpenChoice3Dialog(Gui* menu, const char* title, const char* choice1, const char* choice2, const char* choice3);
void __stdcall InputDialogHandler(Gui* gui);
void __stdcall CopyPaletteEntries(char* obj, void* dest);
void __stdcall DrawColorGrid(Gui* obj);
void __stdcall ShowPaletteEntryRgb(Gui* obj, unsigned char* colors, int index);
void __stdcall LerpPaletteRange(unsigned char* data, int a, int b);
int __stdcall GetColorCellAt(Gui* obj, int x, int y);
void __stdcall WriteTabs(FileHandle* file, int depth);

int __stdcall IsScreenNamed(Gui* obj, const char* name);
void __stdcall ClearSelectedGadget(void* gui);
int __stdcall IsMouseButtonMessage(Gui* obj, unsigned char buttons);
int __stdcall IsDoubleClickMessage(Gui* obj, unsigned char buttons);
int __stdcall HasMouseKeyFlags(Gui* obj, unsigned int mask);
void __stdcall SetClickMode(void* gui, int mode);
int __stdcall OpenMessageBox(Gui* gui, char* text, int wrapWidth, int centre, int autoHeight);
int __stdcall OpenInputDialog(Gui* sub, char* title, char* input, char* chc2, char* chc1, int unused);
char* __stdcall WordWrapText(Gui* menu, char* text, int width, int index);

void __stdcall DrawColorCellOutline(Gui* obj, int cell, int color);

void __stdcall BuildColorRemapTable(RemapPaletteEntry* src, RemapPaletteEntry* dest, unsigned char* table);
void __stdcall ApplySlidersToPaletteEntry(PaletteEditorDialog* obj, RemapPaletteEntry* palette);

#endif
