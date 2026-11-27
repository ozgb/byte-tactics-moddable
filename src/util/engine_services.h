#ifndef UTIL_ENGINE_SERVICES_H
#define UTIL_ENGINE_SERVICES_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "../../interfaces/services.h"
#include "../../interfaces/ref.h"
#include "../../interfaces/sim_events.h"

int HarnessTestMode();
void EnterHarnessTestMode();

struct Vec3;
struct Vec3f;
struct HeapEntry {
    HeapEntry() {}
    HeapEntry(const HeapEntry& o) : value(o.value), key(o.key) {}
    int value;
    float key;
};

struct PathPoint;
struct PathGoalArea;
int __stdcall PathGoalContains(const PathGoalArea* goal, int x, int y);
int __stdcall PathGoalDistance(const PathGoalArea* goal, int x, int y);
int __stdcall PathGoalCells(const PathGoalArea* goal, PathPoint* out, int max);
struct PathfindingEventHeader;
void __stdcall HarnessPathEvent(const PathfindingEventHeader* event, unsigned int size, const PathPoint* points, int capacity);
struct EconomyEventHeader;
void __stdcall HarnessEconomyEvent(const EconomyEventHeader* event, unsigned int size, int outcome);
struct FeaturesEventHeader;
void __stdcall HarnessFeaturesEvent(const FeaturesEventHeader* event, unsigned int size, int outcome);
struct TargetingEventHeader;
void __stdcall HarnessTargetingEvent(const TargetingEventHeader* event, unsigned int size, int outcome);
struct LosRadarEventHeader;
void __stdcall HarnessLosRadarEvent(const LosRadarEventHeader* event, unsigned int size, int outcome);
struct MapEventHeader;
void __stdcall HarnessMapEvent(const MapEventHeader* event, unsigned int size, int outcome);
struct PlayersEventHeader;
void __stdcall HarnessPlayersEvent(const PlayersEventHeader* event, unsigned int size, int outcome);
struct AiEventHeader;
void __stdcall HarnessAiEvent(const AiEventHeader* event, unsigned int size, int outcome);
struct WeaponsEventHeader;
void __stdcall HarnessWeaponsEvent(const WeaponsEventHeader* event, unsigned int size, int outcome);
struct OrdersEventHeader;
void __stdcall HarnessOrdersEvent(const OrdersEventHeader* event, unsigned int size, int outcome);
struct MovementEventHeader;
void __stdcall HarnessMovementEvent(const MovementEventHeader* event, unsigned int size, int outcome);
struct UnitsEventHeader;
void __stdcall HarnessUnitsEvent(const UnitsEventHeader* event, unsigned int size, int outcome);
struct ScriptsEventHeader;
void __stdcall HarnessScriptsEvent(const ScriptsEventHeader* event, unsigned int size, int outcome);
enum SchedModule {
    SCHED_ENGINE = 0,
    SCHED_ECONOMY = 1,
    SCHED_UNITS = 2,
    SCHED_ORDERS = 3,
    SCHED_PLAYERS = 4,
    SCHED_FEATURES = 5,
    SCHED_WEAPONS = 6,
    SCHED_MAP = 7,
    SCHED_WIND_METEORS = 8,
    SCHED_TARGETING = 9,
    SCHED_AI = 10,
    SCHED_LOS_RADAR = 11,
    SCHED_PATHFINDING = 12,
    SCHED_SCRIPTS = 13,
    SCHED_MOVEMENT = 14,
    SCHED_MODULE_COUNT = 15
};
void __stdcall SchedPublish(int outbox, const void* event, unsigned int size);
void __stdcall EmitFrom(int outbox, const void* effect, unsigned int size);
struct EconomySpend;
int __stdcall ScheduleEconomySpend(const struct EconomySpend* spend);
int __stdcall ScheduleEconomyRequestGranted(unsigned int unit, unsigned int resources);
void __stdcall ScheduleMetalExtraction(unsigned int unit);
void __stdcall ScheduleUnitsDeliver();
void __stdcall HarnessWindMeteorsEvent(const void* record, unsigned int size);
void __stdcall HarnessCommandEvent(const void* event, unsigned int size);

struct FeaturesPlace;
int __stdcall ScheduleFeaturesPlace(const FeaturesPlace* place);

struct Unit;
Unit* __stdcall ScheduleTargetingTargetUnit(Unit* unit, int weapon);
int __stdcall ScheduleTargetingTargetPos(Unit* unit, Vec3* pos, int weapon);
Unit* __stdcall ScheduleTargetingFind(Unit* unit, unsigned char weapon, int useRange);
Unit* __stdcall ScheduleTargetingFireAtWill(Unit* unit);

class UnitDef;
struct Player;
struct Point16;
int __stdcall ScheduleMapCanBuildAt(UnitDef* unit, Point16 cell, short type, Player* player);
int __stdcall ScheduleMapCanPlaceUnitFootprint(UnitDef* unit, unsigned short otherTypeId, Point16 cell, int flags);
int __stdcall ScheduleMapPickReclaimable(Vec3* pos, int range, Vec3** energy, float* energyAmount, Vec3** metal, float* metalAmount);

int __stdcall GetHeadingBetween(Vec3* from, Vec3* to);
Vec3f __stdcall VectorFromTo(Vec3f from, Vec3f to);
Vec3f __stdcall CrossProduct(Vec3f a, Vec3f b);
Vec3f __stdcall NormalizeVector(Vec3f v);
void __stdcall RotateByAngles(Vec3* in, Vec3* out, short* angles);
int __stdcall ComputeChecksum(unsigned char* data, int len);

void* __cdecl GameAllocIgnoreTag(const char* name, unsigned int size);
void __cdecl GameFreeThunk(void* p);
char* __cdecl GameStrdup(const char* s);
void __cdecl ProtectBlockReadOnly(void* p);
void __cdecl ProtectBlockReadWrite(void* p);
void __stdcall CopyDwordIfNonNull(int* dest, int* src);
void __stdcall MakeHeap(HeapEntry* first, HeapEntry* last, int* distanceTag, HeapEntry* valueTag);
void __stdcall PopHeapFirst(HeapEntry* first, HeapEntry* last, HeapEntry* dest, HeapEntry val, int* distanceTag);

unsigned char FindHostSlot();

char* __stdcall Translate(const char* key);
int __stdcall FindTranslation(const char* name);
char* __stdcall SkipTextLines(char* text, int n);
int FilePrintf(void* file, const char* fmt, ...);
char* __stdcall LoadFileWithProgress(const char* path);
void* __stdcall LoadObject3d(const char* name);
int __stdcall UsesDataArchives(void);
int __stdcall GetPreferredLanguage(void);
void __stdcall SaveSettings();
void* __stdcall LoadAnimGaf(char* name);

class CommandArgs {
public:
    char* args[0x14];
    char buffer[0x7e];
    char bufferTail[2];
    int count;

    char* GetArg(int index, char* default_val);
    int GetIntArg(int index, int default_val);
    int Count();
    float GetFloatArg(int index, float default_val);
    CommandArgs* InitArgs();
    void Tokenize(char* text, char* end);
    void ShiftArgs(int n);
    void SubstituteArgs(CommandArgs* other);

    char* GetArgPtr(int index, char* def)
    {
        if (index < 0 || index >= count) return def;
        return args[index];
    }
};

int __stdcall ExecuteCommandText(char* text, int len, CommandArgs* vars, int mask);

class BitWriter {
public:
    int index;
    int bit;
    int capacity;
    unsigned int* data;
    unsigned int buffer[0x100];

    BitWriter();
    void FreeBuffer();
    void GrowBuffer();
    void WriteBits(int value, int bits);
    void SetByteAt(int offset, unsigned char value);
};

class BitReader {
public:
    unsigned int* data;
    int index;
    int bit;

    int ReadBits(int bits);
    int ReadSignedBits(int bits);

    int ReadBit()
    {
        int r = (data[index] & (1 << bit)) != 0;
        if (++bit == 32) {
            bit = 0;
            index++;
        }
        return r;
    }
};

#pragma pack(push, 8)
class StringRef {
public:
    union {
        char* p;
        char* data;
        char* ptr;
    };

    StringRef();
    StringRef(const StringRef& other);
    StringRef(const char* text);
    StringRef(const char* text, int len);
    ~StringRef() { ReleaseRef(); }
    StringRef& operator=(const StringRef& other)
    {
        Assign(&other);
        return *this;
    }
    void ReleaseRef();
    StringRef* Assign(const StringRef* other);
    StringRef* Append(const StringRef& other);
    StringRef* MakeLower();
    StringRef* MakeUpper();
    StringRef* AssignText(const char* text);
    char* GetUnique()
    {
        int len = (int)strlen(data);
        if (*(int*)(data - 4) != 1) {
            int* block = (int*)malloc(len + 5);
            *block = 1;
            char* copy = (char*)(block + 1);
            strcpy(copy, data);
            (*(int*)(data - 4))--;
            if (*(int*)(data - 4) == 0) {
                free(data - 4);
            }
            data = copy;
            return copy;
        }
        return data;
    }
    int IsEmpty() const;
    StringRef SubString(int start, int end) const;
};

class TdfRecord;

struct TdfField {
    StringRef key;
    StringRef value;

    TdfField(const StringRef& a, const StringRef& b);
    // FUNCTION: 0x4c54a0 ??0TdfField@@QAE@ABU0@@Z
    TdfField(const TdfField& other)
        : key(other.key), value(other.value)
    {
    }
    ~TdfField();
    void* AssignPair(int* other);
};
#pragma pack(pop)

#pragma pack(push, 1)
struct TdfRecordList {
    char allocatorPad[4];
    TdfRecord** first;
    TdfRecord** last;
    TdfRecord** end;
};

struct TdfFieldList {
    char allocatorPad[4];
    TdfField* first;
    TdfField* last;
    TdfField* end;
};

struct TdfEntries {
    char less;
    TdfFieldList v;
};

class TdfRecord {
public:
    char* name;
    TdfRecordList children;
    TdfEntries entries;
    int text;

    TdfRecord(char* name, char* text, char** nextblock, const char* filename);
    ~TdfRecord();

    StringRef MakeTrimmedString(char* start, char* end);
    TdfRecord* FindChild(const char* key);
    TdfRecord* GetChild(int index);
    void ComputeRecordChecksum(char* start, char* end);
    TdfRecord* FindSubRecord(const char* name);
    int GetRecordName();
    char* GetFieldName(int index);
    int FindFieldValue(char* name);
    int* GetFieldFixed(int* dst, char* key, int def);
    int GetFieldInt(const char* name, int def);
    double GetFieldDouble(const char* name, double def);
    int GetFieldString(char* dst, char* key, int size, char* def);
    void CopyRecordName(char* dest, int count);
    int GetSubRecordCount();
    TdfRecord* GetSubRecord(int index);
    int GetFieldCount();
};
#pragma pack(pop)

class TdfFile {
public:
    TdfRecord* root;
    TdfRecord* current;
    int flags;

    TdfFile();
    ~TdfFile();
    int GetString(char* dst, char* key, int size, char* def)
    {
        return current->GetFieldString(dst, key, size, def);
    }
    int GetInt(char* key, int def) { return current->GetFieldInt(key, def); }
    double GetDouble(char* key, double def) { return current->GetFieldDouble(key, def); }
    char* GetValue(char* key) { return (char*)current->FindFieldValue(key); }
    int LoadFile(const char* path);
    void StripComments(char* p);
    void LoadBuffer(char* data, int size, int flag, char* path);
    void Unload();
    int SelectRecord(const char* name);
    int SelectRecordAt(int index);
    void ResetCurrentRecord();
    int GetCurrentRecord();
    void SetCurrentRecord(int val);
};
static_assert(sizeof(TdfFile) == sizeof(TdfFileLayout), "TdfFile");
static_assert(offsetof(TdfFile, root) == offsetof(TdfFileLayout, root), "TdfFile::root");
static_assert(offsetof(TdfFile, current) == offsetof(TdfFileLayout, current), "TdfFile::current");
static_assert(offsetof(TdfFile, flags) == offsetof(TdfFileLayout, flags), "TdfFile::flags");

int __stdcall GetLocalizedString(TdfFile* file, char* dst, char* key, int size, char* def);
struct Handle;
struct GafEntry;
struct GafFrame;
struct FrameTable;
struct GafArchive;
struct Object3do;
struct CobScriptHeader;

GafFrame* __stdcall GetGafFrame(FrameTable* table, int index);
GafFrame* __stdcall GetGafSequenceFrame(Handle* player);
void __stdcall InitGafSequence(Handle* player, GafEntry* entry, int index);
int __stdcall StepGafSequence(Handle* player);
GafEntry* __stdcall FindGafEntry(GafArchive* gaf, const char* name);
GafArchive* __stdcall LoadGaf(const char* path);
void* __stdcall Load3do(char* path);
void __stdcall MirrorObject(Object3do* model);
void __stdcall BindModelTextures(Object3do* model, const char* name);
int __stdcall GetObjectHeight(Object3do* model);
CobScriptHeader* __stdcall LoadCobScript(char* path);
void __stdcall FreeCobScript(CobScriptHeader* script);
int __stdcall GetCobChecksum(CobScriptHeader* script);
GafFrame* __stdcall AllocFrame(const char* name, int width, int height);
void* __stdcall BuildLensFrame(int w, int h, int lens);
char* __stdcall StripExtension(char* name);
int __stdcall LoadSoundByName(char* name, const char* file);
struct ModelState;
void __stdcall ReleaseUnitPictures(ModelState* state);
void __stdcall BuildTedRadarFrame(unsigned short* data);
void __stdcall InitUnitPictureCache(void);
void __stdcall FreeUnitPictureCache(void);
int __stdcall LoadMapList(void** out, int multiplayer, int unused);
void FreeOtaEnumCache();

#endif
