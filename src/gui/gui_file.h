#ifndef GUI_FILE_H
#define GUI_FILE_H
class TdfFile;
struct FileHandle;
struct FreeCommonGuiGafDialog;
struct FreeGafSlotDialog;
struct GuiFileCommonFields;
struct ReadButtonFieldsRecord;
struct ReadButtonFieldsSource;
struct ReadFontFilenameFieldRecord;
struct ReadFontFilenameFieldSource;
struct ReadHotOrNotFieldRecord;
struct ReadHotOrNotFieldSource;
struct ReadListBoxFieldsRecord;
struct ReadListBoxFieldsSource;
struct ReadNuttinFieldRecord;
struct ReadNuttinFieldSource;
struct ReadPanelFieldsRecord;
struct ReadSliderFieldsSource;
struct ReadStatusFieldRecord;
struct ReadStatusFieldSource;
struct ReadSurfFilenameFieldRecord;
struct ReadSurfFilenameFieldSource;
struct ReadTextInputFieldsRecord;
struct ReadTextInputFieldsSource;
struct ReadTextLinkFieldsRecord;
struct ReadTextLinkFieldsSource;
struct SliderFieldsRecord;
struct WriteButtonFieldsRecord;
struct WriteFontFilenameFieldRecord;
struct WriteHotOrNotFieldRecord;
struct WriteListBoxFieldsRecord;
struct WriteNuttinFieldRecord;
struct WritePanelFieldsRecord;
struct WriteStatusFieldRecord;
struct WriteSurfFilenameFieldRecord;
struct WriteTextInputFieldsRecord;
struct WriteTextLinkFieldsRecord;

struct FileHandle;

void __stdcall WriteSectionHeader(FileHandle* file, char* name, int depth);
void __stdcall WriteSectionEnd(FileHandle* file, int depth);
void __stdcall WriteKeyValue(FileHandle* file, char* key, char* value, int depth);
void __stdcall WriteGuiFile(char* obj, char* name);

void __stdcall WriteCommonFields(GuiFileCommonFields* obj, FileHandle* out, int indent);
void __stdcall ReadCommonSection(GuiFileCommonFields* obj, TdfFile* tree);
void __stdcall WritePanelFields(WritePanelFieldsRecord* obj, FileHandle* out, int indent);
void __stdcall ReadPanelFields(ReadPanelFieldsRecord* obj, TdfFile* src);
void __stdcall WriteStatusField(WriteStatusFieldRecord* obj, FileHandle* out, int indent);
void __stdcall ReadStatusField(ReadStatusFieldRecord* a, ReadStatusFieldSource* b);
void __stdcall WriteButtonFields(WriteButtonFieldsRecord* obj, FileHandle* out, int indent);
void __stdcall ReadButtonFields(ReadButtonFieldsRecord* obj, ReadButtonFieldsSource* src);
void __stdcall WriteListBoxFields(WriteListBoxFieldsRecord* obj, FileHandle* out, int indent);
void __stdcall ReadListBoxFields(ReadListBoxFieldsRecord* param1, ReadListBoxFieldsSource* param2);
void __stdcall WriteTextInputFields(WriteTextInputFieldsRecord* obj, FileHandle* out, int indent);
void __stdcall ReadTextInputFields(ReadTextInputFieldsRecord* obj, ReadTextInputFieldsSource* src);
void __stdcall WriteSliderFields(SliderFieldsRecord* obj, FileHandle* out, int indent);
void __stdcall ReadSliderFields(SliderFieldsRecord* obj, ReadSliderFieldsSource* src);
void __stdcall WriteTextLinkFields(WriteTextLinkFieldsRecord* obj, FileHandle* out, int indent);
void __stdcall ReadTextLinkFields(ReadTextLinkFieldsRecord* obj, ReadTextLinkFieldsSource* src);
void __stdcall WriteHotOrNotField(WriteHotOrNotFieldRecord* obj, FileHandle* out, int indent);
void __stdcall ReadHotOrNotField(ReadHotOrNotFieldRecord* obj, ReadHotOrNotFieldSource* src);
void __stdcall WriteFontFilenameField(WriteFontFilenameFieldRecord* obj, FileHandle* out, int indent);
void __stdcall ReadFontFilenameField(ReadFontFilenameFieldRecord* obj, ReadFontFilenameFieldSource* src);
void __stdcall WriteSurfFilenameField(WriteSurfFilenameFieldRecord* obj, FileHandle* out, int indent);
void __stdcall ReadSurfFilenameField(ReadSurfFilenameFieldRecord* obj, ReadSurfFilenameFieldSource* src);
void __stdcall WriteNuttinField(WriteNuttinFieldRecord* obj, FileHandle* out, int indent);
void __stdcall ReadNuttinField(ReadNuttinFieldRecord* a, ReadNuttinFieldSource* b);
void __stdcall FreeGafSlot(FreeGafSlotDialog* obj, int i);
void __stdcall FreeCommonGuiGaf(FreeCommonGuiGafDialog* obj);

#endif
