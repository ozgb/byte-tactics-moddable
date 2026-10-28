#ifndef TA_TYPES_H
#define TA_TYPES_H

#include "win_types.h"
#include <stdio.h>
#include <vector>
#include <list>
#include <map>
#include "../src/game/unit_resources.h"
#include "../src/weapons/unit_weapon_slot.h"
#include "../src/weapons/weapon_def.h"
#include "../src/weapons/damage_table.h"
#include "../src/graphics/nano_particle_vector.h"
#include "../src/units/squad.h"
#include "../src/util/vec3.h"
#include "../src/network/packet_ring.h"
#include "../src/network/packet_receiver.h"
#include "../src/network/packet_manager.h"
#include "../interfaces/ref.h"
#include "../src/graphics/rect.h"
#include "../src/network/bit_reader.h"
#include "../src/network/unit_sync.h"
#include "../src/network/unit_sync_state.h"
#include "../src/units/unit_type_set.h"
#include "../src/debug/command_line.h"

struct PlayerInfo;

#pragma pack(push, 1)

struct AI;
struct Account_004b4d70;
struct Aim_0049dd60;
struct Alloc_0046d040;
struct Alloc_0046e5c0;
struct Alloc_004b2290;
struct Alloc_004dc130;
class Alloc_004dd8c0;
class Alloc_004e2b60;
struct Amount;
struct Angles_0049be60;
struct AnimEntry_00421f20;
struct AnimEntry_00429700;
struct FeatureAnimPool;
struct AnimPair_00423550;
struct AnimSeq_00445110;
struct AnimSrc_004233a0;
struct AnimSrc_00423550;
struct Anim_00424890;
struct Anim_004394e0;
struct Anim_00439740;
struct App_0049e830;
struct App_004bec70;
struct App_004bed70;
struct App_004c0b10;
struct App_004c25e0;
struct Arena_004da9f0;
struct Arg_00419670;
struct Arg_004c6210;
struct Args_00451640;
struct Arr_00421550;
struct ArrayA;
struct ArrayB;
class Attached_004388d0;
class Attached_004388d0_2;
struct Attached_00438a00;
class Attached_0043a1f0;
struct Bank_004b4d70;
class Base;
class Base_00490a10;
class Base_00490a10_2;
class Base_00490a10_3;
struct Base_004d8ae0;
struct Big_004b1ec0;
struct BitFlags16_00499200;
struct BitFlags_004197d0;
struct BitmapInfo_004b5510;
struct Bitmap_00437b50;
struct Bitmap_004caec0;
struct Bitmap_004cb170;
struct Bits10F_00487080;
struct Bits_0045c570;
struct Bits_0045d280;
struct BlinkWord_004afc60;
struct Blk;
struct Blob32_4cb330;
struct BlobRec_004b4270;
struct Block_00421620;
struct Block_004b1ec0;
struct Block_004c5ff0;
struct BmpFileHeader;
struct BmpInfo;
struct BmpInfoHeader;
struct Bounds_src_004b8310;
struct Box;
struct Box_004b4d70;
struct Buffer_00434ff0;
struct Buffer_00462d30;
struct Buffer_004635b0;
struct Buffer_004636b0;
struct Buffer_004b39c0;
struct Buffer_461020;
struct Buffers_00462d30;
struct BuildEntry_0041ace0;
struct BuildList_0041ace0;
struct BuildList_0045a950;
struct Button_0041e420;
struct Button_004a7f70;
struct ByteMap_00408090;
struct CalcedExplosion;
struct Callback;
struct Callback_0043a1f0;
struct CdLists_490f80;
struct Cell;
struct Cell_0040d8b0;
struct Cell_0040d900;
struct Cell_0040da70;
struct Cell_0040e050;
struct Cell_0040e160;
struct Cell_0040e630;
struct Cell_00421eb0;
struct Cell_0047e5c0;
struct FrameTable;
struct Cell_004848e0;
struct Cell_004b0230;
struct Channel;
struct Channel_00460f60;
struct Channel_004b08c0;
struct Channel_004b0940;
struct Channel_004b0a10;
struct Channel_004b0b00;
struct Channel_004b0c40;
struct Channel_4619e0;
struct Chunk;
struct Chunk_4d1820;
struct Chunk_4d1970;
class Class_004010b0;
class Class_00401110;
class Class_00401180;
class Class_004011c0;
class DamagedAllyCollector;
class SquadManager;
class Class_004071f0;
class Class_00407560;
class AssaultTimer;
class EscortTimer;
class SquadScoutTimer;
class SpatialTimer;
class ScoutTimer;
class Class_00408620;
class Class_00408810;
class Class_00408830;
class Class_004089a0;
class Class_00408bf0;
class Class_00409730;
struct PlayerAI;
struct Class_0040a7b0;
class Class_0040aa40;
class Class_0040ad20;
class Class_0040d7b0;
class Class_0040d880;
class Class_0040d8b0;
class Class_0040da40;
class Class_0040da70;
class Class_0040df00;
class Class_0040e050;
class Class_0040e160;
class Class_0040e630;
class Class_0040e9a0;
class Class_0040ef20;
class OpenHeap;
class Class_0040f110;
class Class_0040f1e0;
class LandingPadList;
class Class_00415b60;
struct Class_00415b90;
class BitWriter;
struct Class_00415da0;
class BitReader;
class Class_00417e00;
class AIProfileParser;
class Class_00428d10;
struct Class_00428f40;
class Class_00428f60;
class Class_00432c00;
class Class_00432c20;
class Class_004330b0;
class Class_00433130;
class Class_00433270;
class Class_00433380;
class Class_00433500;
class Class_00433520;
class Class_00433540;
class Class_004335c0;
class Class_004335f0;
class Class_004336f0;
class Class_004339c0;
class LosLine;
class Class_00434f70;
class Class_004352b0;
class Class_00435320;
class Class_004353b0;
class Class_004356f0;
class Class_00435760;
class Class_00435900;
class Class_00435910;
class Class_00435920;
class Class_00435980;
class Class_00435c20;
class Class_00435c40;
class Class_00435c50;
class Class_00435d30;
class Class_00436c30;
class Class_00437280;
class Class_00437300;
class Class_004373a0;
class MapCache;
class Class_00437800;
class MapCacheEntry;
class Class_004379a0;
class Class_004379a0_2;
class Class_004379b0;
class Class_00437a20;
class CMemoryCache;
class Class_00437c80;
class MeteorParams;
class MissionType;
class Class_00438760_2;
struct Class_00438870;
class Class_00438880;
class Class_004388b0;
class Class_004388d0;
class Class_00438930;
class Class_00438a00;
class Class_00438ad0;
class Class_00438b90;
class Class_00439e80;
class OrderBase;
class Class_0043a420;
class MissionOrderTable;
class Class_0043cbb0;
class Class_0043cc20;
class Class_0043cd20;
class Class_0043d210;
class Class_0043d290;
class Class_0043d6d0;
class Class_0043da70;
class Class_0043db50;
struct UnitMotion;
class Class_0043dd70;
struct MovementClassTable;
struct Class_004402e0;
struct MovementClass;
class Class_00440470;
struct Class_004404c0;
class Class_00440500;
class Class_00440830;
class Class_00440af0;
class Class_0044ce20;
class OrderFx;
class Class_0044ce70;
class Class_0044ce90;
class Class_0044cf00;
class Class_0044cf60;
class Class_0044cff0;
class Class_0044d010;
class ApproachRadius;
class Class_0044d290;
class Class_0044d2c0;
class Class_0044d310;
class Class_0044d350;
class Class_0044d3b0;
struct Class_0044d450;
class Class_0044d470;
class Class_0044d560;
class Class_0044d720;
class Class_0044d7c0;
class Class_0044d800;
class RingApproach;
class Class_0044d8a0;
class Class_0044d910;
class Class_0044d930;
class Class_0044da00;
struct PointMarker;
class Class_0044dcb0;
class Class_0044dd00;
class PathOrder;
class Class_0044de80;
class Class_0044df80;
class Class_0044dfb0;
class Class_0044e080;
class Class_0044e190;
class Class_0044e250;
class Class_0044e2d0;
class Class_0044e330;
class Class_0044e3a0;
class Class_0044e3c0;
class Class_0044e530;
class Class_0044e5b0;
class Class_0044e6c0;
class Class_0044e720;
class Class_0044e730;
class Class_0044e740;
class Class_0044e7b0;
class Class_0044e9c0;
class AirManeuverOrder;
class Class_0044eb40;
struct Class_0044ec20;
class PathGoal;
class Class_0044ef80;
class AiSearchGoal;
class Class_0044f150;
class Class_0044f260;
class Class_0044f290;
struct Class_0044f480;
struct Class_0044f480_link;
struct Class_0044f480_target;
class Class_0044f4a0;
class Class_0044f570;
class Class_0044f5b0;
class PatrolGoal;
class Class_0044f650;
class Class_0044f8a0;
class Class_0044f940;
class Class_0044f9c0_2;
class NetCondenser;
class NetPacket;
class Class_00451fd0;
struct Class_00452370;
class Class_00456030;
class Class_00458180;
struct Class_004581e0;
class Class_00458310;
class Class_00458430;
class Class_004584d0;
class Class_00458d30;
class Class_00458fa0;
class Class_00459170;
class Class_00459200;
struct Class_0045a470;
class Class_0045a510;
class Class_0045a610;
struct Class_0045ae80;
class Class_00460f40;
class Class_00460f60_3;
struct Class_004615f0;
class Class_00461620;
class Class_00461630;
class Class_00461750;
class PacketManager;
class Class_00461860;
class Class_004618a0;
class Class_00461900;
class Class_00461a70;
class Class_00461b10;
struct Class_00461d80;
struct Class_00461da0;
class Class_00461f90;
class Class_00461fd0;
class Class_00462370;
class PacketRing;
class Class_004623e0;
class Class_00462470;
class Class_004624a0;
class Class_00462710;
class Class_00462860;
class Class_004628a0;
class Class_004629b0;
struct Class_00462a40;
class Class_00462bd0;
struct Class_00462bf0;
class Class_00462d30;
class Class_00462d30_3;
class Class_00462d30_4;
class PacketReceiver;
class Class_00462f30_2;
class Class_00463680;
class Class_004636b0;
struct Class_00463730;
class Class_00463be0;
class Class_00463c40;
struct Class_00464ac0;
class DetectionVisitor;
class RadarJamVisitor;
class SonarJamVisitor;
struct FrameTimers;
struct Class_0046c620;
class Class_0046cc10;
class Class_0046cef0;
class Class_0046d040;
class Class_0046d4c0;
class Class_0046d630;
class Class_0046d6c0;
class Class_0046d860;
class Class_0046e160;
class Class_0046e330;
class Class_0046e3c0;
struct Class_0046e450;
struct Class_0046e4d0;
struct SyncTaggedVector;
class SyncTempTaggedVector;
class SyncChecksumVector;
class Class_0046e880;
class Class_0046e9b0;
struct SyncPlayerRecord;
class Class_0046eba0;
class Class_0046f720;
class InsertResult;
class Class_0046fe60;
class Class_00470250;
struct Class_00470560;
class ObjectPool;
class Class_00470c10;
class Class_00470e50;
class Class_00470eb0;
class Class_00470ed0;
class ParticleLists;
class Class_00471120;
class Class_00471340;
class TeleportParticles;
class Class_00471470;
class NanoParticles;
class Class_004715a0;
class ThrustParticles;
class Class_004716e0;
class WakeParticles;
class Class_00471820;
class Class_00471a50;
class ParticleSystem;
struct ParticleSystem;
class Class_00471d70;
struct Class_00472200;
class Class_00472e00;
struct Class_00472e30;
struct Class_00472e70;
class Class_00472f60;
struct Class_00472fd0;
class Class_004730c0;
class Class_004730f0;
struct Class_00473130;
class Class_00473170;
class Class_00473220;
class Class_00473250;
struct Class_00473290;
class Class_00473590;
class Class_004736c0;
class Class_004736e0;
class Class_00473a00;
class Class_00473b30;
class Class_00473b50;
struct ThrustParticle;
class Class_00474170;
class Class_004742a0;
class Class_004742c0;
class Class_004743a0;
class WakeParticle;
class Class_004745e0;
class Class_00474720;
class Class_00474760;
struct SmokeParticle;
class Class_00474cb0;
class SmokeParticles;
class Class_00474d50;
struct Class_00474f80;
class TimedSubParticle;
class Class_00475040;
class Class_00475090;
class TimedSubParticles;
class Class_004750f0;
class Class_00475150;
class Class_00475440;
class Class_00475bd0;
class Class_00476210;
class Vec_00476490;
class MoviePlayer;
class Class_0047bf20;
class Class_0047bf70;
class Class_0047c150;
class Class_0047c230;
class Class_0047c2a0;
class Class_0047c2f0;
class Class_0047c330;
class Class_0047c3a0;
class Class_0047c6c0;
struct Class_0047cb00;
struct Class_0047cb40;
class ClaimFootprintVisitor;
class Class_0047f960;
class Class_0047f990;
class Class_0047fa30;
class SpeechQueue;
class Class_0047ffa0;
class Class_00480020;
class Class_004800c0;
class IntDynArray;
class Squad;
class Class_00480c30;
class Class_00480c50;
class Class_00480cb0;
class Class_00480d40;
class Class_00480d50;
class Class_00480e30;
class Class_00480e50;
class Class_00480e70;
class Class_00481340;
class Class_004813b0;
class Class_00481430;
class Class_00481470;
struct Eye;
class UnitScript;
class Class_00488c50;
class UnitTypeSet;
class Class_00489240;
class UnitCategory;
class Class_00489540;
class PathOrderAttach;
class Class_00489650;
class Class_004896f0;
class Class_00489740;
class Class_004897e0;
class Class_00489800;
class Class_004898b0;
class Class_00489960;
class Class_00489a70;
struct Class_0048a440;
class Class_0048dfb0;
class VictoryKillEnemyCommander;
class Class_0048ea40;
class Class_0048eac0;
class Class_0048eb00;
class VictoryDestroyAllUnits;
class Class_0048eb80;
class Class_0048ebc0;
class Class_0048ec00;
class VictoryKillAllMobileUnits;
class Class_0048ecb0;
class Class_0048ed00;
class Class_0048ed50;
class VictoryBuildUnitType;
class Class_0048ee30;
class Class_0048ee70;
class VictoryCaptureUnitType;
class Class_0048ef00;
class Class_0048ef40;
class Class_0048ef80;
class VictoryKillAllOfType;
class Class_0048f070;
class Class_0048f0b0;
class VictoryKillUnitType;
class Class_0048f160;
class Class_0048f1b0;
class VictoryMoveUnitToRadius;
class Class_0048f2f0;
class Class_0048f330;
class Class_0048f370;
class VictoryUnitTypePassesX;
class Class_0048f440;
class Class_0048f480;
class Class_0048f4c0;
class VictoryUnitTypePassesZ;
class Class_0048f590;
class Class_0048f5d0;
class VictoryTimerRunsOut;
class Class_0048f630;
class Class_0048f670;
class DefeatCommanderKilled;
class Class_0048f710;
class Class_0048f750;
class Class_0048f7e0;
class DefeatAllUnitsKilled;
class Class_0048f880;
class DefeatUnitTypeKilled;
class Class_0048f900;
class Class_0048f950;
struct Class_0048f9a0;
class DefeatAllUnitsKilledOfType;
class Class_0048fab0;
class Class_0048faf0;
struct Class_0048fb30;
class DefeatAnyUnitPassesX;
class Class_0048fbc0;
class Class_0048fc00;
struct Class_0048fc40;
class DefeatAnyUnitPassesZ;
class Class_0048fcd0;
class Class_0048fd10;
class DefeatDeathTimerRunsOut;
class Class_0048fd70;
class Class_0048fdb0;
class MissionConditions;
struct Class_00490360;
class PackedGoal;
class Class_00490650;
class LiteGoal;
class PackedPosGoal;
struct Class_00490880_link;
struct Class_00490880_target;
struct Class_004908b0;
class Class_004908c0;
class Class_00490a10;
struct Class_0049f8c0;
struct Class_004a0e00;
struct Class_004a1530;
struct Class_004a16f0;
struct Class_004a1ab0;
struct Class_004a2480;
struct Class_004a30c0;
struct Class_004a3eb0;
struct Class_004a4620;
struct Class_004a4980;
struct Class_004a4c90;
struct Class_004a53c0;
struct Class_004a56b0;
struct Class_004a6a40;
struct Class_004a8150;
struct Class_004ab040;
struct Class_004ab2b0;
struct Class_004ac610;
struct Class_004acc70;
struct Class_004afc60;
struct Class_004aff00;
class CobScript;
class Class_004b07a0;
class Class_004b0830;
class Class_004b08c0;
class Class_004b0a10;
class Class_004b0a70;
class Class_004b0b00;
class ScriptCallback;
class Class_004b0c40;
class Class_004b0d20;
class Class_004b0da0;
class Class_004b1c00;
class Class_004b2290;
class Class_004b2fb0;
class Class_004b3430;
class Class_004b3490;
class Class_004b3590;
class Class_004b3630;
class Class_004b3750;
class Class_004b3770;
class HapiBank;
class Class_004b4630;
class Class_004b46c0;
class Class_004b4750;
void RegisterEmptyAtexitA(void);
class Class_004b4850;
class Class_004b48f0;
class Class_004b49d0;
class Class_004b4a80;
class Class_004b4b50;
class Class_004b4bf0;
class Class_004b4c10;
class Class_004b4c50;
class Class_004b4cf0;
class Class_004b4d70;
struct Class_004b52e0;
class Class_004b73b0;
class Class_004b73c0;
class Class_004b73e0;
class Class_004b7410;
class Class_004b7440;
class Class_004b7540;
class CommandEntry;
class Class_004b7e30;
struct Class_004bb650;
class Class_004bb670;
struct Class_004bbbe0;
struct Class_004bbc40;
struct Class_004bbc40_Entry;
struct Class_004bbc40_File;
struct Class_004bbc40_Other;
struct Class_004bbd00_File;
struct Class_004bbd00_Other;
struct Class_004bce10;
struct Class_004be010;
#include "../src/util/tdf.h"
class Class_004c2f60;
class Class_004c3240;
class Class_004c33a0;
class Class_004c3490;
class Class_004c3e10;
class Class_004c3e10_2;
class Class_004c3e20;
class Class_004c3e30;
class Class_004c3e40;
class Class_004c42a0;
class Class_004c4340;
class Class_004c43f0;
class Class_004c4440;
class Class_004c4450;
class Class_004c4470;
class Class_004c44c0;
class Class_004c45c0;
class Class_004c45e0;
class Class_004c4630;
class Class_004c4760;
class Class_004c4800;
class Class_004c5470;
class Class_004c54a0;
class Class_004c54d0;
class TranslationTable;
class TdfFieldVector;
struct Class_004c5c60;
struct Class_004c61f0;
struct Class_004c6890;
struct Class_004c6b10;
class Class_004c90b0;
class Class_004c9180;
class Class_004c91b0;
class Class_004c9230;
class Class_004c9290;
class Class_004c9310;
class Class_004c9390;
class Class_004c93f0;
class Class_004c9490;
struct Class_004cb7c0;
struct Class_004cb7d0;
class Class_004cb7f0;
struct Class_004cb940;
class Class_004cd9c0;
class Class_004cd9d0;
class Class_004cda00;
class Class_004cdb40;
class Class_004ce020;
class Class_004ce1d0;
class Class_004ce260;
class Class_004ce3e0;
class Class_004ce410;
class Class_004ce460;
class Class_004ce5a0;
class Class_004ce7a0;
class Class_004ce800;
class Class_004ce8c0;
class Class_004ce910;
class Class_004ceb60;
class Class_004ced40;
class Class_004cedc0;
class Class_004cee50;
class Class_004ceee0;
class Class_004cf0b0;
struct Class_004cf150;
class Class_004cf180;
class Class_004cf210;
class Sound;
class Class_004cf230;
class Class_004cf370;
class Class_004cf540;
class Class_004cf800;
class Class_004cf8a0;
class Class_004cfb40;
class Class_004cfe40;
class Class_004cfe80;
class Class_004cfe90;
class Class_004cfea0;
class Class_004cfeb0;
class Class_004cff20;
class Class_004cff30;
class Class_004cfff0;
class Class_004d0040;
class Class_004d0070;
class Class_004d00d0;
struct Class_004d01b0;
class Class_004d02a0;
class Class_004d0660;
class Class_004d06c0;
class Class_004d0720;
class Class_004d07f0;
class Class_004d0910;
class BlockInfo;
class Class_004d87f0_2;
class Class_004d8820;
class Class_004d8850;
class TraceRecord;
class Class_004d88d0;
class CallSite;
struct Class_004d8b30;
class Class_004d8b60;
class Class_004d8bd0;
class BlockHistory;
class Class_004d8d40;
class StackTrace;
class Class_004d9ca0;
class CommandLineSwitch;
class CommandLineNumber;
class FreeBlockMap;
class Class_004db450;
class Class_004dbd00;
class Class_004dbd20;
class Class_004dbeb0;
class Class_004dbec0;
class Class_004dc130;
class Class_004dc620;
class Class_004dc680;
class Class_004dc910;
class Class_004dce00;
class Class_004dce60;
class Class_004dd150;
class Class_004dd1f0;
class Class_004dd250;
class FreeBlockIter;
class Class_004dd3d0;
class Class_004dd430;
class Class_004dd710;
class Class_004dd770;
class Class_004dd7d0;
class MapInsertResult;
class Class_004ddc00;
class Class_004ddc90;
class Class_004ddce0;
class FreeBlockAllocator;
class BlockMapAllocator;
class BlockMapIter;
class LoadedImage;
class Class_004ddfa0;
class Class_004ddfe0;
class Class_004de020;
class Class_004df1e0;
class Class_004df250;
class Class_004df280;
class Class_004df380;
class Class_004df4e0;
class Class_004df590;
class Class_004dfea0;
class Class_004e03f0;
class NameMapIter;
class Class_004e0520;
class Class_004e0570;
class Class_004e05c0;
class Class_004e05f0;
class Class_004e0b90;
class MappedFile;
class Class_004e1590;
class Class_004e1650_2;
class NameTable;
class Class_004e18c0;
class Class_004e1990;
class NameKey;
class Class_004e1d20;
class Class_004e1d60;
class Class_004e1e30;
class Class_004e1e50;
class Class_004e20a0;
struct Class_004e2150;
class Class_004e2160;
struct Class_004e2180;
struct Class_004e21a0;
struct Class_004e21c0;
class Class_004e21f0;
class Class_004e2240;
class Class_004e2250;
class Class_004e2580;
class Class_004e2620;
class Class_004e2950;
class Class_004e29b0;
class NameMapInsertResult;
class Class_004e2a30;
class NameMapAllocator;
class CavedogRegistryKey;
class Class_004e2cc0;
class Class_004e2ce0;
class Class_004e2d00;
class Class_004e2d70;
class Class_004e2d90;
class Class_004e2e00;
class Class_004e2e20;
class Class_004e2e60;
class Class_004e2ea0;
class Class_004e2ee0;
class Class_004e2f30;
class Class_004e2f90;
class Class_004e2fe0;
class Class_004e3030;
class Class_004e8230;
class LosTable;
class Class_44ef90;
struct DisplayContext;
struct UnitKilledPacket;
struct Cmp_0046d040;
struct CobFile_0042d1f0;
struct Color_004acc70;
struct Colors_00467b60;
struct ColourEntry_00478790;
struct Colour_00478790;
struct Common_004ad350_2;
struct Comp_004b2290;
class MissionCondition;
struct ConnInfo_443ff0;
struct Conn_00443cb0;
struct Conn_443ff0;
struct Conn_4ca100;
struct Connection_004ca450;
struct Connection_4c9a70;
union Coord_00407ae0;
union Coord_004239c0;
union Coord_0044f2a0;
struct Count_00458d20;
class CritSec_004e1ac0;
struct Ctx_004720d0;
struct Ctx_00499890;
struct CursorState_0041cc60;
struct CursorState_0041cd20;
class DDraw_0047bf70;
struct DPNAME;
struct DPName_004ca6a0;
struct DPSessionDesc_4c9a70;
struct DXVersion;
struct Data1;
struct Data2;
struct Data_0041f0a0;
struct Data_00444ea0;
struct Data_0044d470;
struct Data_0046d970;
struct Data_0046dad0;
struct Data_0047aaf0;
struct Data_00480c30;
struct Data_004b1c00;
struct Data_004b1ec0;
struct DblRec_004b4270;
struct Debris;
struct DebugDir_004ddfa0;
struct DefFlags_0049d270;
struct Def_004899b0;
struct Desc;
struct Desc_004437c0;
struct Desc_004b8ec0;
struct Desc_004ca450;
struct Descriptor_00421620;
struct Dialog;
struct Dialog_00464f80;
struct Diff_00480570;
struct DirectDrawState;
class DirectPlayLobby_4ca490;
class DirectPlayLobby_4ca880;
class DirectPlay_004c9920;
class DirectPlay_004c9e50;
class DirectPlay_004ca6a0;
class DirectPlay_004ca990;
class DirectPlay_4c97b0;
class DirectPlay_4c9800;
class DirectPlay_4c9840;
class DirectPlay_4c9890;
class DirectPlay_4c9b70;
class DirectPlay_4c9d30;
class DirectPlay_4c9d80;
class DirectPlay_4c9dd0;
class DirectPlay_4c9f90;
class DirectPlay_4c9fd0;
class DirectPlay_4ca250;
class DirectPlay_4ca780;
class DirectPlay_4ca7c0;
class DirectPlay_4ca800;
class DirectPlay_4ca840;
struct DisplaySurface;
struct Display_0041ce90;
struct Display_0041da60;
struct Display_0041ec50;
struct Display_0041f760;
struct Display_00425d80;
struct Display_0047bdf0;
struct Display_0047c3a0;
struct Display_004b8310;
struct Display_004b8500;
struct Display_004b86e0;
struct Display_004b8ec0;
struct Display_004b9740;
struct Display_004ba200;
struct Display_004ba4c0;
struct Display_004be0b0;
struct Display_004be400;
struct Display_004c5df0;
struct Display_004c5e70;
struct Display_004c5fa0;
struct Display_004c5ff0;
struct Display_004c60d0;
struct Display_004c6210;
struct Display_004c62c0;
struct Display_004c63a0;
struct Display_004c6b70;
struct Display_004c6c50;
struct Display_004c6d20;
struct Display_004c6dc0;
struct Display_004c6e70;
struct Display_004c8020;
struct UnitDamagePacketOut;
struct Driver_004c6890;
struct Dst_00440ca0;
class Dummy_00440be0;
union Dword_0051f410;
struct Effect;
struct Effects;
struct ElemA_0046c190;
struct ElemArray_4a4930;
struct ElemB_0046c190;
struct Elem_00401000;
struct Elem_0040cc40;
struct Elem_0040cc40_2;
struct Elem_0040cfb0;
struct Elem_0040d4f0;
struct Elem_0040d550;
struct Elem_0040f110;
struct Elem_004222e0;
struct Elem_0042a140;
struct Elem_00432be0;
struct Elem_00432be0_2;
struct Elem_004336c0;
struct Elem_00434020;
struct Elem_00434360;
struct Elem_00434360_2;
struct Elem_00434a60;
struct OrderTableEntry;
struct Elem_0043ca70;
struct Elem_00441c30;
struct Elem_0044ce90;
struct Elem_0046e5e0;
struct Elem_004702a0;
struct Elem_004702a0_2;
struct Elem_00470a40;
struct Elem_00472e70;
struct Elem_00473130;
struct Elem_00473290;
class Elem_00475880;
struct Elem_00476210;
struct Elem_00476490;
struct Elem_00488a00;
struct Elem_00488a00_2;
struct Elem_00488a00_3;
struct Elem_004b75d0;
struct Elem_004b75d0_2;
struct Elem_004b75d0_3;
struct Elem_004b75d0_4;
struct ArchiveEntry;
struct Elem_004c5740;
struct TdfField;
struct Elem_004cb4c0;
struct Elem_004cb5f0;
struct Elem_004dd8c0;
struct Elem_432cb0;
struct Elem_432cb0_2;
struct Elem_4b0610;
struct Element_00475ef0;
struct Emitter_0047fd70;
struct Engine;
struct Entries_004c51b0;
struct Entry;
struct EntryAnim_004a9fd0;
struct EntryTail_004a9fd0;
struct Entry_0040f1e0;
struct Entry_0041aa00;
struct Entry_00420960;
struct Entry_004263b0;
struct Entry_0042f740;
struct Entry_00431a20;
struct Entry_004426e0;
struct Entry_0044c370;
struct Entry_00452570;
struct Entry_0045a950;
struct Entry_0045aaa0;
struct Entry_004618a0;
struct Entry_00461db0;
struct Entry_00462d30;
struct Entry_00462d30_2;
struct Entry_00462d30_3;
struct Entry_00462d30_4;
struct Entry_00463ca0;
struct Entry_00463ef0;
struct Entry_0046d6c0;
struct Entry_0046e000;
struct Entry_00479620;
struct Entry_0047f8c0;
struct Entry_0047f990;
struct Entry_0047fa30;
struct Entry_0047fca0;
struct Entry_0047ffa0;
struct Entry_00480020;
struct Entry_00480c30;
struct Entry_00480c50;
struct Entry_00480d50;
struct Entry_00480eb0;
struct Entry_00487af0;
struct Entry_004898b0;
struct Entry_004898b0_2;
struct Entry_0048a0f0;
struct Entry_0048a160;
struct Entry_0048a190;
struct Entry_0048a1e0;
struct Entry_0048a440;
struct Entry_0049d120;
struct Entry_0049d270;
struct Entry_004a0bf0;
struct Entry_004a7ee0;
struct Entry_004aafe0;
struct Entry_004abba0;
struct Entry_004abd20;
struct Entry_004b0d20;
struct Entry_004b7ee0;
struct Entry_004b7f70;
struct Entry_004b8b30;
struct Entry_004d0a10;
struct Entry_2;
struct Entry_3;
struct Entry_4;
struct Entry_439d20;
struct Entry_482ac0;
struct Entry_5;
struct EventEntry;
struct Event_0046d860;
struct Event_0049d1e0;
struct Event_4b5cc0;
struct ExceptionName;
struct Expired;
struct Explosion;
struct ExplosionEntry;
struct Eye_00482130;
struct F0_00462d30;
struct F0_00462d30_2;
union F110_0048ad30;
struct FP_0043d6d0;
struct Fade_004c6890;
struct Feature;
struct FeatureDef_00421eb0;
struct FeatureName_00424890;
struct FeatureName_00424c00;
struct FeatureSpot;
struct FeatureSpot_2;
struct FeatureUnit_00421f20;
union Field136_004a6ae0;
struct Field_0046d6c0;
class File_0043a970;
struct File_004b4800;
struct File_004b4850;
struct File_004b4910;
struct File_004bdd70;
struct File_004be070;
struct File_004be400;
struct FindData_0041d4c0;
struct FindData_0042a440;
struct FindData_00495930;
struct FindData_004bc930;
struct FindData_004bca30;
struct FindData_004bcd10;
struct FindData_004cb170;
struct FindHandle_004bc930;
struct Find_004bc8d0;
union Fix_004736e0;
union Fix_00473d50;
union Fix_0047d2e0;
union Fixed;
struct FixedParts_00407ae0;
struct FixedParts_004853b0;
union Fixed_00407ae0;
struct Fixed_004239c0;
struct Fixed_00424050;
struct Fixed_00485140;
struct Fixed_00485330;
union Fixed_004853b0;
union Fixed_0048bae0;
union Fixed_0049abb0;
union Fixed_2;
union Fixed_3;
union Fixed_4;
union Fixed_437840;
union Fixed_497180;
union Fixed_5;
class Fixed_6;
struct FlagBits_00457d30;
struct FlagBits_00487080;
struct FlagBits_004876c0;
union FlagWord;
union Flags110_0043f0e0;
union Flags111_0043f0e0;
union Flags114_485a40;
union Flags142f0_00466dc0;
struct Flags16_00493bf0;
union Flags16_00499200;
struct Flags241_00466dc0;
struct FlagsBits_0049cc20;
union FlagsU_00419670;
struct Flags_004165c0;
struct Flags_00416660;
struct Flags_00416e00;
struct Flags_00416e30;
struct Flags_00416e60;
struct Flags_00417060;
struct Flags_00417090;
struct Flags_00417300;
struct Flags_004177a0;
struct Flags_00419670;
union Flags_004197d0;
struct Flags_004268b0;
struct Flags_004269d0;
struct Flags_0043e490;
union Flags_0045c570;
union Flags_0045d280;
struct Flags_0046a530;
union Flags_004898b0;
union Flags_004898b0_2;
struct Flags_00495e90_37ebe;
struct Flags_00495e90_37f06;
struct Flags_00495e90_38a51;
union Flags_00495e90_3923b;
struct Flags_00496ce0;
struct Flags_00499cd0;
struct Flags_0049e010;
struct Flags_0049e1a0;
struct Flags_004ab400;
struct Flags_004ab440;
struct Flags_142f1_004816a0;
struct Flags_416630;
struct Flags_418ca0;
struct Flags_418fd0;
union Flags_4b5980;
struct FloatVec;
struct Follow_0041ca10;
struct Font_004a50b0;
struct Form_00464e70;
struct Fp_004bbff0;
struct Fpo_004de020;
struct FrameCounter_4b6570;
struct Frame_00424050;
struct Frame_00462f30;
struct Frame_00478b40;
struct Frame_0047ae60;
struct Frame_00481930;
struct Frame_00481d50;
struct Frame_0049e070;
struct Frame_004a4980;
struct Frame_004a7f70;
struct Frame_004c7580;
struct Frame_004db450;
struct Frame_004de700;
struct Frame_482270;
struct FreeObj_0045e100;
struct Gadget_00426190;
struct Gadget_0045d7c0;
struct Gadget_00494290;
struct Gadget_004ab0b0;
struct Gadget_004ac340;
struct Gadget_004ac4c0;
struct Gadget_004ac8c0;
struct GafEntry_004a5f40;
struct GafEntry_004b8d40;
struct Gaf_004224b0;
struct Gaf_004b8d40;
struct Game;
struct GameRec_4c9c50;
struct GameState_4a4930;
struct Game_004bfe10;
struct Game_004c14f0;
struct Game_004c1830;
struct Game_004caec0;
struct Game_004cb170;
struct GlobalObj;
struct GlobalObj_2;
struct GlobalObj_3;
struct Glyph_004a1b40;
struct Grid;
struct Grid2_482c20;
struct Grid_0040d8b0;
struct Grid_0040d900;
struct Grid_0040da70;
struct Grid_0040e050;
struct Grid_0040e630;
struct Grid_0047e5c0;
struct Group;
struct Group_00409160;
struct Group_004bc640;
struct Groups_004bc640;
struct Gui_00443100;
struct Gui_00453a50;
struct Gun_0049c9c0;
struct Handle;
struct HandlerSlot_004b7900;
class Handler_004388d0;
struct HapiBuf;
struct Hapi_004bd160;
struct Head_0043a420;
struct Head_004455b0;
struct Head_004635b0;
struct Head_0049cc20;
struct Header_0041e420;
struct Header_00421620;
struct Header_0044d010;
struct Header_0044d010_2;
struct Header_0044d470;
struct Header_0044d470_2;
struct Header_0044d930;
struct Header_0044d930_2;
struct Header_0044e7d0;
struct Header_0044e880;
struct Header_00479c50;
struct Header_004be400;
struct Header_004cac40;
struct Heading_0049cde0;
struct HeapNode_0040f000;
struct Heap_0040e630;
struct Holder_00401000;
struct Holder_0041e420;
struct Holder_00479bf0;
struct Holder_004908c0;
struct Holder_00499eb0;
struct Holder_004a75d0;
struct Hs_0048a490;
struct IDirectSound3DBuffer;
struct IconSet_00466780;
class Id_004df1e0;
struct Ids_0046d970;
class Iface_00417f30;
class Iface_0043dd20;
class Iface_4ca590;
struct ImageFrame;
struct Image_004b3770;
struct Image_004caf30;
struct Image_004cb940;
struct Image_004cb9e0;
struct Info_00408670;
struct Info_00477510;
struct Info_0048eeb0;
struct Info_0048f0f0;
struct Info_0048f8c0;
struct Info_0048f9d0;
struct Info_004aafe0;
struct Info_48efb0;
struct Info_4c7310;
struct Inner_00421550;
struct Inner_00444910;
struct Inner_0044d2c0;
struct Inner_0044d720;
struct Inner_00494290;
struct Inner_0049fb30;
struct Inner_004afd20;
struct Inner_004c51b0;
struct Inner_44b3c0;
struct Input_004c2cc0;
struct InsertResult_004db450;
struct IntLess_004b3430;
struct IntLess_004b3490;
struct IntRec_004b4270;
class Intf_004c6210;
struct Ints_459200;
struct Item_00470ae0;
struct Item_00478790;
struct Item_00478b40;
struct Item_004794d0;
struct Item_00481430;
struct Item_00485940;
struct Item_004b3630_10;
struct Item_004b4d70;
struct Item_004bb2e0;
struct Item_4afec0;
class Iter_0046e330;
struct Iterator_004df590;
struct Kfn_004dbbc0;
struct Kfn_004e2250;
struct Kind_00499ab0;
struct Len;
struct Less_0046d2e0;
struct Less_0046d970;
struct Less_0046e160;
struct Less_0046e280;
struct Less_0046e330;
struct Less_0046e330_2;
struct Less_0046e3c0;
struct Less_0046e450;
struct Less_0046e4d0;
struct Less_0046fe60;
struct Less_004db000;
struct Less_004dbbc0;
struct Less_004dbd20;
struct Less_004dbec0;
struct Less_004dc130;
struct Less_004dc620;
struct Less_004dc680;
struct Less_004dce00;
struct Less_004dce60;
struct Less_004dd250;
struct Less_004dd3d0;
struct Less_004dd430;
struct Less_004dd7d0;
struct Less_004ddc90;
struct Less_004e17c0;
struct Less_004e1990;
struct Less_004e2250;
struct Less_004e2580;
struct Less_004e2620;
struct Limits_0043e490;
struct Line_004de550;
struct Link_0044f4a0;
struct Link_00462ed0;
struct Link_00481430;
class Link_004908c0;
struct ListOwner_0043a1f0;
class ListWrap_0046d1a0;
struct List_00420960;
struct List_004be3b0;
class Listener_00470fb0;
class Listener_00471120;
class Listener_004896f0;
class Listener_00489740;
class Listener_0048f250;
class Listener_0048ff40;
class Listener_0048ff40_2;
class Listener_0048ff40_3;
struct LiveNode;
union LoadFlags_00497f40;
class Lobby_4c9a70;
struct LockEntry_004c5d90;
struct LogoEntry_00445110;
struct Logos_00445110;
struct MapGrid;
struct MapInfo;
struct MapInfo_00483610;
struct MapInfo_44dc60;
struct MapRow_0048a490;
struct MapSize_00408090;
struct MapVertex_0048a490;
struct Map_0040d7b0;
struct Map_00425b80;
class Map_0042e440;
struct Map_00458fa0;
struct Map_004658e0;
class Map_0046d6c0;
class Map_0046e280;
class Map_004b2540;
class Map_004c3e40;
class Map_004df380;
struct Map_004df590;
struct Menu;
struct Menu_0041b2e0;
struct Menu_0041b8d0;
struct Menu_00476d80;
struct Menu_00477410;
struct Menu_00494220;
struct Menu_44b3c0;
struct Message_00447150;
struct Message_004565a0;
struct Message_0047f780;
struct Message_0047fad0;
struct Mid_00462d30;
struct MissionFeature_00436c30;
struct MissionFlags_0042f9a0;
struct MissionHolder;
struct MissionName_00467e50;
struct MissionRule_00436c30;
struct MissionUnit_00436c30;
struct Mission_00467e50;
struct MobHdr_43dd70;
struct ModeList_004b5330;
struct Mode_00446310;
struct Mode_4b5980;
struct Model_00424890;
struct Model_00424c00;
struct Model_0042a140;
struct Mouse_0041ce90;
struct Movement;
struct Mover;
struct Msg13_00456310;
struct Msg20_00456310;
struct Msg26_00456310;
struct Msg_00441220;
struct Msg_00450530;
struct Msg_00461900;
struct Muzzle_0048a1e0;
struct NameLess_00499cd0;
struct NameLess_004b7620;
struct NameLess_004b7760;
struct NameLess_004b7900;
struct NameLess_004c4630;
struct NameLess_004c46c0;
struct NameLess_004c4760;
struct NameNe_004b7620;
struct NameNe_004b7760;
struct NameTable_004b0830;
struct Name_0046c620;
struct Name_004866d0;
struct Name_0048d630;
struct Named_004c4450;
class Mission;
struct Net2_4517b0;
struct Net_00443100;
struct Net_004c9920;
struct Net_004c9e50;
struct Net_004ca450;
struct Net_004ca6a0;
struct Net_4517b0;
struct Net_4c97b0;
struct Net_4c9800;
struct Net_4c9840;
struct Net_4c9890;
struct Net_4c9a70;
struct Net_4c9b70;
struct Net_4c9c20;
struct Net_4c9c50;
struct Net_4c9d30;
struct Net_4c9d80;
struct Net_4c9dd0;
struct Net_4c9f90;
struct Net_4c9fd0;
struct Net_4ca100;
struct Net_4ca250;
struct Net_4ca330;
struct Net_4ca400;
struct Net_4ca780;
struct Net_4ca7c0;
struct Net_4ca800;
struct Net_4ca840;
struct NodeData_0040da70;
struct Node_0040df00;
struct Node_0040e630;
struct Node_0040eb70;
struct Node_0040ef20;
struct Node_004390a0;
struct Node_00439e30;
struct Node_0043ad50;
struct Node_0046fad0;
struct Node_0047e570;
struct Node_00489a90;
struct Node_004bb2e0;
struct Node_004d0b10;
struct Node_004d0f60;
struct Node_004dbd00;
struct Node_439d20;
struct Node_439d80;
struct Normal_00424890;
struct Obj;
struct Obj18_004aff00;
struct Obj1_49fd20;
struct Obj2_49fd20;
struct Obj_00401320;
struct Obj_0041b200;
struct Obj_00423bf0;
struct Obj_00426e80;
struct Obj_00431950;
struct Obj_00439e30;
struct Obj_0043e0b0;
struct Obj_00444910;
struct Obj_0044d310;
struct Obj_00462d30;
struct Obj_00479bf0;
struct Obj_0047e5c0;
struct Obj_00499ab0;
struct Obj_004a7f70;
struct Obj_004ade20;
struct Obj_004adf10;
struct Obj_004ae170_2;
struct Obj_004ae230;
struct Obj_004ae300;
struct Obj_004ae440;
struct Obj_004ae4b0;
struct Obj_004ae4e0;
struct Obj_004ae550;
struct Obj_004ae580;
struct Obj_004aeda0;
struct Obj_004aef80;
struct Obj_004b7f00;
struct Obj_004b7f70;
struct Obj_004ba590;
struct Obj_004ba750;
struct Obj_004baad0;
struct Obj_004bab30;
struct Obj_004bab60;
struct Obj_004baba0;
struct Obj_004babd0;
struct Obj_004badf0;
struct Obj_004c13d0;
struct Obj_004c23e0;
struct Obj_004c2470;
struct Obj_004c24b0;
struct Obj_004c2870;
struct Obj_004c2bd0;
struct Obj_004c61b0;
class Obj_2;
struct Obj_3;
struct Obj_4;
struct Obj_403010;
struct Obj_403040;
struct Obj_444ba0;
struct Obj_4ca590;
struct Obj_5;
struct Obj_6;
struct Obj_7;
struct Obj_8;
struct Object3do;
struct ObjectState_0045a8d0;
struct Object_00419940;
struct Object_0044e530;
struct Object_0044e5b0;
struct Object_0044e740;
struct Object_0044e880;
struct Object_0044ea60;
struct Object_0044eb40;
struct Object_00477510;
struct Object_0048a440;
struct Object_00499c10;
struct Object_0049f930;
struct Object_004aafe0;
struct Object_004abd20;
struct Order;
struct Order_00401e00;
struct Order_00402160;
struct Order_004021f0;
struct Order_00402d10;
struct Order_00402fc0;
struct Order_004030d0;
struct Order_00403100;
class Order_004031d0;
class Order_00415250;
struct Order_0048ab70;
struct Order_11;
struct Order_12;
struct Order_13;
struct Order_14;
struct Order_15;
struct Order_2;
struct Order_3;
struct Order_4;
struct Order_5;
struct Order_6;
struct Order_7;
struct Order_8;
struct Order_9;
struct Orders_0041a120;
struct Orders_0048d630;
struct Other_0048fb30;
struct Other_0048fc40;
struct Out_004c5ff0;
struct Out_482830;
struct Outer_0049fb30;
struct Outs_t;
class Owner;
struct Owner_00407560;
struct Owner_0041a920;
struct Owner_0043ad50;
struct Owner_0044d2c0;
struct Owner_0044d720;
struct Owner_0044e080;
struct Owner_0044e9c0;
struct Owner_0044f5c0;
struct Owner_00481430;
struct Owner_004866d0;
struct Owner_00489540;
struct Owner_004896f0;
struct Owner_00489740;
struct Owner_004936f0;
struct Owner_00494290;
struct Owner_2;
struct Owner_439d20;
struct Owner_439d80;
struct Owner_459c70;
struct PCX_004caa40;
struct PackHeader_004d1c00;
struct Packet;
struct Packet_004233a0;
struct Packet_004237d0;
struct Packet_004244b0;
struct Packet_00450a10;
struct Packet_00450f90;
struct Packet_00452960;
struct Packet_00452b70;
struct UnitCreatePacket;
struct BuilderLinkPacket;
struct UnitScriptCallPacket;
struct Packet_00456190;
struct Packet_00456200;
struct Packet_00456290;
struct Packet_00456de0;
struct Packet_00456ee0;
struct Packet_00457050;
struct Packet_004571c0;
struct Packet_004573d0;
struct Packet_00457540;
struct Packet_00457d30;
class Packet_00461db0;
struct Packet_0046cef0;
struct Packet_0046cef0_2;
struct Packet_0046d530;
struct Packet_0046d5b0;
struct Packet_0046d630;
struct Packet_0046d6c0;
struct Packet_0046d860;
struct Packet_0046dad0;
struct Packet_0047f0c0;
struct Packet_0047f300;
struct OwnershipTransferPacket;
struct ActivateFlagsPacket;
struct WeaponFirePacket;
struct Packet_00499ba0;
struct ProjectileDetonatePacket;
struct Packet_0049d580;
struct Packet_0049d9c0;
struct Packet_0049db70;
struct Packet_0049dd60;
struct Packet_0049df10;
struct Packet_2;
struct Packet_4517b0;
struct Page_0045f8c0;
struct Pair_0040d880;
struct Pair_0040e630;
struct Pair_00419560;
struct Pair_0046faf0;
struct Pair_00472ab0;
struct PalEntry_004ac710;
struct PaletteSource;
struct Palette_004ac7d0;
struct Palette_004b95a0;
struct Palette_004b96e0;
struct Param;
struct Param2_004addf0;
struct PathCell;
struct PathGrid;
class PathMap;
struct Path_004bc640;
struct Pic_004295b0;
struct Pic_4665d0;
struct Piece;
struct PieceBits_00487080;
union PieceFlags_459c70;
struct PieceFrame;
struct Piece_00458310;
struct Placement;
struct Player;
struct PlayerData_004437c0;
struct PlayerData_004956c0;
struct PlayerData_004ca6a0;
struct PlayerData_467c00;
struct PlayerEntry_0041b0f0;
struct PlayerFlags_497180;
struct PlayerGrid;
struct PlayerInfo_00466dc0;
struct PlayerInfo_0046dad0;
class UnitResources;
struct PlayerRes_00401360;
struct PlayerSlots_00497f40;
struct Player_00407ae0;
struct Player_00409730;
struct Player_0041a920;
struct Player_00437cd0;
struct Player_00439740;
struct Player_004437c0;
struct Player_00446080;
struct Player_004467c0;
struct Player_0044feb0;
struct Player_00466dc0;
struct Player_00467cb0;
struct Player_004689c0;
struct Player_00473a00;
struct Player_0047a0e0;
struct Player_0047acd0;
struct Player_0047ae60;
struct Player_00481340;
struct Player_004813b0;
struct Player_00481930;
struct Player_0048ec20;
struct Player_0048eeb0;
struct Player_0048f0f0;
struct Player_0048f9d0;
struct Player_467c00;
struct Player_48efb0;
struct Point16;
struct Point16_00437de0;
struct PointInit_0043a1f0;
struct Point_00417f60;
struct Point_0041cd50;
struct Point_0046ba80;
struct Point_0048c6a0;
struct Point_004948e0;
struct Point_00498cd0;
struct Point_004a4b50;
struct Point_004bf060;
struct Point_004bf260;
struct Point_004c0330;
struct Point_004c07b0;
struct Point_004c0c70;
struct Poly_459c70;
class PoolAlloc_004db610;
struct Pool_00422170;
struct Pool_004232a0;
struct Pos16_44dc60;
struct Pos2_0048a490;
struct Pos_00409160;
struct Pos_004658e0;
struct Pos_00465ac0;
struct Pos_00473590;
struct Pos_00473590_2;
struct Pos_004745e0;
struct Pos_00474b80;
struct Pos_0047d2e0;
struct Pos_00485330;
struct Pos_0048d630;
struct Pos_004cf570;
struct Pos_4589c0;
class Prim_0046bae0;
struct Quad;
class Queue_00461f90;
struct Queue_004c1ab0;
struct Queue_004c1ab0_2;
struct Queue_004c1ab0_3;
struct Queue_004c1ab0_4;
struct Queue_004c1ab0_5;
struct Queue_4c2d60;
struct Queue_4c2de0;
struct Queue_4c2e30;
struct RGBA;
struct RGBX_004ba4c0;
struct Range;
struct Range_00464da0;
struct Range_2;
struct Rate_004e0b90;
struct Rating;
struct Rec1_00479c50;
struct Rec2_00479c50;
struct Rec2_004b1ec0;
struct Rec2_004b2040;
struct Rec_00421620;
struct Rec_0043e180;
struct Rec_0044de80;
struct Rec_0044dfb0;
struct ConsoleCommand;
struct Rec_004c7580;
struct Record_0043de30;
struct Record_0044c0d0;
struct Record_00474cd0;
struct Record_004ab2b0;
struct Record_004ab3a0;
struct Record_005129b4;
struct Rect;
struct Rect_004c2380;
struct Rect_004c2870;
struct Res_0045bbf0;
struct Resources;
struct RgbQuad;
struct Rot16;
struct Rule_0045f1d0;
struct S;
struct SaveDesc_0043a420;
struct SaveRec_00487080;
struct Save_00492df0;
struct SavedPiece_004876c0;
struct Saved_00493bf0;
struct Screen_004a75d0;
struct Screen_004b8310;
struct Screen_004b8500;
struct Screen_004c5df0;
struct Screen_004c5fa0;
struct Screen_004c60d0;
struct Screen_004c6210;
struct Screen_004c62c0;
struct Screen_004c63a0;
struct Screen_004c6b70;
struct Screen_004c6c50;
struct Screen_004c6d20;
struct Screen_004c6dc0;
struct Screen_004c6e70;
struct ScriptTable;
struct Section_004b4800;
struct Section_004b4850;
struct Seg_00473b50;
struct Serial_00441c30;
struct SessionDesc_004c9920;
struct SessionDesc_4c9dd0;
struct Settings;
struct Settings_00441460;
struct Settings_00460160;
struct Settings_00496e10;
struct Settings_00496e10_2;
struct Shape;
struct Shape_00472ab0;
struct Shot_0049cc20;
struct Shot_0049d000;
struct Side_00431a60;
struct Size_0046b9d0;
struct SkirmishPlayer;
struct Slider_004acc70;
struct Slot;
struct Slot_0041e420;
struct Slot_0042f740;
struct Slot_004390a0;
struct Slot_00471120;
union Slot_00483610;
struct Slot_0048cd80;
struct Slot_4b62d0;
struct Smack_0047c330;
struct SmackerSurface;
struct SmackerSurfaces;
struct SmkStats_0047c530;
struct Smk_0047bf70;
struct Smk_0047bf70_b;
struct Smoke_00425b80;
struct Snapshot_0046a860;
struct SoundInfo_005086fc;
struct Sound_0047f210;
struct Sound_45ffb0;
struct Source_0042f450;
struct Source_00440340;
struct Source_0046d630;
struct Source_00499c10;
struct Source_004adf10;
struct Source_004ae170;
struct Source_004ae300;
struct Source_004ae410;
struct Source_004ae4b0;
struct Source_004ae550;
struct Span_004c0a90;
struct Spawn_00420e50;
struct Spawn_004861d0;
union SpotField_004246b0;
struct SpotState;
struct Sprite_0049be60;
struct Sprite_004b94c0;
struct SrcPiece_00487080;
struct Src_00440ca0;
struct Src_004ab400;
struct Src_004ab5d0;
struct Src_004b8310;
struct Src_004b8a80;
struct Stack_004d89b0;
struct State_004bb2e0;
struct StructA_004ada10;
struct StructA_004ae610;
struct StructB_004ada10;
struct StructB_004ae610;
struct Struct_0040bab0;
struct Struct_00487af0;
struct Struct_004907e0;
struct Struct_0049fb50;
struct Struct_0049fba0;
struct Struct_0049fbf0;
struct Struct_004a81b0;
struct Struct_004ad980;
struct Struct_004add60;
struct Struct_004addf0;
struct Struct_004bcea0;
struct Struct_004c2ac0;
struct Struct_004c2ac0_2;
struct Struct_004c6ac0;
struct Sub;
struct Sub_00409520;
struct Sub_004679a0_a;
struct Sub_004679a0_b;
struct Sub_0046e000;
struct Sub_00477360;
struct Sub_00496b10;
struct Sub_004e0570;
struct Sub_2;
struct Sub_403010;
struct Sub_403040;
struct Surface;
struct SurfaceWrapper;
struct Surface_004c0a90;
struct Surface_004c5d90;
struct Surface_004c5df0;
struct Surface_004c6f80;
class Surface_4b6570;
struct Surfaces_0047bdf0;
struct Sym_004dea00;
struct Table_0040d880;
struct Table_0041ace0;
struct Table_004611e0;
struct Table_0047acd0;
struct Table_0047ae60;
struct Table_00499cd0;
struct Table_004aedd0;
struct Table_004b08c0;
struct Table_004b3630;
struct Table_004b7ee0;
struct Table_004cb2f0;
struct Tail_004635b0;
struct Target_004030a0;
class Target_0040da40;
struct Target_0040da70;
class Target_0040e160;
class Target_0040e630;
class Target_0040eb70;
struct Target_0044ddc0;
struct Target_0044f2a0;
struct Target_0044f4a0;
struct Target_0044f5c0;
struct Target_0046d530;
struct Target_0046d5b0;
struct Target_0046d630;
struct Target_004908c0;
struct Target_0049e1a0;
struct Target_439d20;
union UnitTypeFlags_459200;
struct ShareLosAckPacket;
struct Team_0048d9a0;
struct Texture_00421700;
struct Thing_0043f0e0;
struct TlsBlock;
struct TntHeader_00483610;
struct Trail_004394e0;
class Tree_004da8d0;
struct Triple_004e0570;
union UType_0042b370_flags;
struct Unit;
struct Unit32_0048d9a0;
struct Unit8_0048d9a0;
class UnitCallback_0048edb0;
class UnitCallback_0048f530;
class UnitCallback_0048f7e0;
class UnitCallback_0048f7e0_2;
class UnitDef;
struct UnitFlag_0044e7d0;
struct UnitFlags_0048d630;
class UnitList;
struct UnitList_00409160;
struct UnitList_00409730;
struct UnitList_0048ec20;
struct UnitRange_0048d920;
struct UnitRecord_00486fd0;
struct UnitRecord_004876c0;
struct UnitRef;
struct UnitType_00489280;
struct OrderTypeEntry;
struct UnitType_499e50;
class UnitVisitor_0048ec20;
class UnitVisitor_0048ed50;
class UnitVisitor_0048f200;
class UnitVisitor_0048f250;
class UnitVisitor_0048f370;
class UnitVisitor_0048f3e0;
class UnitVisitor_0048f4c0;
class UnitVisitor_0048f9d0;
class UnitVisitor_0048fb60;
class UnitVisitor_0048fc70;
class UnitVisitor_48efb0;
struct Unit_00401cc0;
struct Unit_00402fc0;
struct Unit_004030d0;
struct Unit_00403190;
struct Unit_00408670;
struct Unit_00419ac0;
struct Unit_00420e50;
struct Unit_00438880;
struct Unit_0044dfb0;
struct Unit_0044e7d0;
struct Unit_00452570;
struct Unit_00467960;
struct Unit_00467980;
struct Unit_0046e3c0;
struct Unit_0047fad0;
struct Unit_00485d40;
struct Unit_004894f0;
struct Unit_0048ec20;
struct Unit_0048f8c0;
struct Unit_0049c880;
struct Unit_0049df10;
struct Unit_2;
struct Unit_3;
class Unknown_4c9b70;
struct V4i;
struct Value_004df380;
struct Value_004df590;
struct Value_004e17c0;
struct Value_004e2250;
struct Vec;
struct Vec3;
struct Vec3Init_0043a1f0;
struct Vec3_004b1ec0;
struct Vec3_2;
union Vec3_459200;
struct Vec3f;
struct Vec3i_00474130;
class VecElems_0046ca60;
class VecInt_0046ca60;
class Vec_0046d1a0;
class Vec_0046d6c0;
class Vec_0046d860;
struct Vector3;
struct Vector3s;
struct Video_0045bb00;
struct MapViewFlags;
struct View_00438c00;
struct View_0044d8a0;
struct View_4b5980;
class Visitor_0047e750;
struct WaveFormat;
struct WeaponDef;
struct WeaponType;
struct Weapon_0049abb0;
struct Weapon_0049b000;
struct Weapon_2;
struct Weapon_3;
struct Widget_00464f80;
struct Word_004afd80;
struct Word_004aff00;
union Word_0051f522;
struct Wrap_00433270;
struct Zero11_004917d0;

class Obj_2 {
public:
    char unknown_0[1];
};

class Class_004010b0 {
public:
    char acc0[24];
    char acc1[24];
    void SaveUnitAccounts(Unit*, HapiBank*);
};

class Class_004b4c10 {
public:
    Table_004b3630* table;
    void SeekBox(int);
};

class Class_004b4cf0 {
public:
    Table_004b3630* table;
    int WriteBox(void*, int);
};

class Class_00401110 {
public:
    char acc0[24];
    char acc1[24];
    void LoadUnitAccounts(Unit*, HapiBank*);
};

class Class_00401180 {
public:
    char unknown_0[4];
    float x0;
    float x1;
    float x2;
    int RequestEnergy(Class_00401180*, float);
};

class Class_004011c0 {
public:
    char unknown_0[4];
    float x0;
    float x1;
    float x2;
    char unknown_10[12];
    float y0;
    float y1;
    float y2;
    int RequestEnergyAndMetal(float, float);
};

struct PlayerRes_00401360 {
    float stored;
    float produced;
    float used;
};

#include "../src/orders/mission_type.h"

class Class_00438880 {
public:
    char unknown_0[14];
    Unit_00438880* unit;
    char unknown_12[48];
    unsigned int flags;
    void AnnounceStatusIfFlagged(char*);
};

struct Unit_00401cc0 {
    char unknown_0[272];
    unsigned int flags;
};

class Class_00439e80 {
public:
    char unknown_0[1];
    void SetDeadlineTicks(int);
};

struct Order_2 {
    char unknown_0[5];
    unsigned char state;
    char unknown_6[48];
    int wait;
    int radius;
};

struct Order_00401e00 {
    char unknown_0[5];
    unsigned char state;
    char unknown_6[48];
    int id;
};

struct Obj_3 {
    char unknown_0[5];
    unsigned char state;
    int field_6;
    char unknown_a[12];
    int flag_16;
};

struct Order_3 {
    char unknown_0[6];
    unsigned int flags;
    char unknown_a[44];
    int done;
    unsigned int count;
};

struct Order_00402160 {
    char unknown_0[5];
    unsigned char state;
    int field_6;
    char unknown_a[12];
    int field_16;
};

#include "../src/orders/path_order_attach.h"

struct Order_004021f0 {
    char unknown_0[5];
    unsigned char state;
    unsigned int flags;
    char unknown_a[8];
    PathOrderAttach target;
    Vec3 pos;
    char unknown_2e[8];
    int wait;
    int waitLimit;
};

#include "../src/orders/box.h"

struct Order_4 {
    char unknown_0[5];
    unsigned char state;
    unsigned int flags;
    char unknown_a[8];
    PathOrderAttach target;
    Vec3 pos;
    char unknown_2e[8];
    int unitType;
    int count;
};

#include "../src/units/unit.h"

#include "../src/units/unit_def.h"

#include "../src/orders/order.h"

struct Order_5 {
    char unknown_0[5];
    unsigned char state;
    char unknown_6[48];
    int weapon;
    int count;
    int progress;
};

struct WeaponType {
    char unknown_0[192];
    float metalCost;
    float energyCost;
    char unknown_c8[28];
    unsigned short buildTime;
};

struct Weapon_2 {
    WeaponType* type;
    char unknown_4[10];
    unsigned char stockpile;
    char unknown_f[13];
};

struct Unit_2 {
    char unknown_0[16];
    Weapon_2 weapons[3];
    char unknown_64[88];
    Class_004011c0 resources;
};

class Class_004388d0 {
public:
    char unknown_0[14];
    Unit* unit;
    char unknown_12[60];
    unsigned int flags;
    Attached_004388d0* attached;
    void SetAttachedFx(Attached_004388d0*);
};

struct Order_00402d10 {
    char unknown_0[54];
    int ticks;
};

struct Order_00402fc0 {
    char unknown_0[5];
    unsigned char state;
};

struct Unit_00402fc0 {
    char unknown_0[134];
    int field_86;
};

struct Obj_403010 {
    char unknown_0[146];
    Sub_403010* sub;
};

struct Obj_403040 {
    char unknown_0[146];
    Sub_403040* sub;
};

struct Sub_403040 {
    char unknown_0[581];
    unsigned int unused_bits : 2;
    unsigned int flag : 1;
    unsigned int unused_bits2 : 29;
};

struct Target_004030a0 {
    char unknown_0[581];
    unsigned int lowbits : 13;
    unsigned int flag : 1;
    unsigned int : 18;
};

struct Order_004030d0 {
    char unknown_0[54];
    int mode;
};

struct Unit_004030d0 {
    char unknown_0[272];
    unsigned int unknown_bits : 18;
    unsigned int mode2 : 2;
    unsigned int unknown_bits2 : 12;
};

struct Order_00403100 {
    char unknown_0[54];
    int mode;
};

struct Slot_004390a0 {
    UnitMotion* weapon;
    int field_4;
    short field_8;
    char unknown_a[4];
    unsigned char field_e;
    unsigned char flags;
    char unknown_10[12];
};

class Class_00438b90 {
public:
    char unknown_0[4];
    unsigned char kind;
    char unknown_5[17];
    Unit_00403190* target;
    char unknown_1a[28];
    int state;
    char unknown_3a[8];
    unsigned int flags;
    void MergeFlagsFromTable(int);
};

class Class_00438930 {
public:
    char unknown_0[14];
    Unit* unit;
    char unknown_12[60];
    unsigned int flags;
    Attached_004388d0_2* attached;
    void AttachApproachRadiusGoal(int*, int);
};

class Order_004031d0 {
public:
    char unknown_0[5];
    unsigned char state;
    int field_6;
    char unknown_a[24];
    int pos[5];
    int field_36;
};

struct Order_6 {
    char unknown_0[5];
    unsigned char state;
    unsigned int flags;
    char unknown_a[12];
    Unit* target;
    char unknown_1a[8];
    Vec3 pos;
};

class Class_00438a00 {
public:
    char unknown_0[14];
    Unit* unit;
    char unknown_12[60];
    unsigned int flags;
    Attached_00438a00* attached;
    void AttachRingApproachGoal(Vec3*, int, int);
};

class Class_004897e0 {
public:
    char unknown_0[31];
    unsigned char flags_1f;
    char unknown_20[27];
    unsigned char flags_3b;
    char unknown_3c[27];
    unsigned char flags_57;
    unsigned char ChooseWeapon(void);
};

struct Order_7 {
    char unknown_0[5];
    unsigned char state;
    unsigned int flags;
    char unknown_a[12];
    Unit* target;
    char unknown_1a[8];
    Vec3 pos;
    short x;
    short z;
    char unknown_32[4];
    int weapon;
    int step;
    int radius;
    int Advance(int);
};

struct Order_8 {
    char unknown_0[5];
    unsigned char state;
    unsigned int flags;
    char unknown_a[44];
    int weapon;
    int radius;
};

class Class_00438ad0 {
public:
    char unknown_0[14];
    Unit* unit;
    char unknown_12[60];
    unsigned int flags;
    Class_0044d8a0* attached;
    void AttachBuildFootprintMarker(Point16, Point16);
};

union Fixed {
    int value;
};

struct UnitRef {
    int vtable;
    Unit* ptr;
    Unit* Get(void);
};

class Class_00489960 {
public:
    char unknown_0[1];
    int CanReclaim(void*);
};


struct Packet_2 {
    unsigned char type;
    unsigned char sub;
    short x;
    short z;
};

#include "../src/util/rot16.h"


struct Order_9 {
    char unknown_0[5];
    unsigned char state;
    unsigned int flags;
    char unknown_a[4];
    Unit* source;
    UnitRef target;
};

class DamagedAllyCollector {
public:
    Player* owner;
    std::vector<Unit*>* units;
    Unit* self;
    DamagedAllyCollector(DamagedAllyCollector&);
    DamagedAllyCollector(Player*, std::vector<Unit*>*, Unit*);
    virtual void CollectDamagedAlly(Unit*);
    virtual void CollectGroundAlly(Unit*);
    virtual void CollectRepairableUnit(Unit*);
};

struct Order_11 {
    char unknown_0[5];
    unsigned char state;
    unsigned int flags;
};

struct Order_12 {
    char unknown_0[5];
    unsigned char state;
    unsigned int flags;
    char unknown_a[64];
    void* next;
};

struct Order_13 {
    char unknown_0[5];
    unsigned char state;
    unsigned int flags;
    char unknown_a[12];
    Unit* target;
    char unknown_1a[28];
    int attempts;
};

struct Order_14 {
    char unknown_0[5];
    unsigned char state;
    unsigned int flags;
    char unknown_a[8];
    int ref;
    Unit* target;
    char unknown_1a[8];
    Vec3 pos;
    char unknown_2e[8];
    int attempts;
};

struct Order_15 {
    char unknown_0[34];
    Vec3 pos;
};

class Class_004b73c0 {
public:
    char* args[52];
    int count;
    char* GetArg(int, char*);
    int FUN_004b73c0(int, int);
    void SubstituteArgs(Class_004b73c0*);
};

class Class_004b7410 {
public:
    char unknown_0[1];
    float GetFloatArg(int, float);
};

class Class_004b73e0 {
public:
    char* args[52];
    int count;
    int GetIntArg(int, int);
};

#include "../src/ai/squad_manager.h"

#include "../src/weapons/projectile.h"

class Class_004071f0 {
public:
    Unit* owner;
    Unit* FindNearestEnemyUnit(Vec3);
    Unit* FindNearestEnemyUnit(int, int, int);
};

union Coord_00407ae0 {
    union { int value; struct { unsigned short frac; short whole; } s; };
};

#include "../src/ai/byte_map.h"

#include "../src/network/player.h"

#include "../src/orders/unit_motion.h"

struct Slot_4b62d0 {
    int counter;
    int unknown_8;
    int unknown_c;
    virtual void Fire(void);
};

#include "../src/ai/player_ai.h"

struct Point16_00437de0 {
    short x;
    short y;
};

class Class_004b73b0 {
public:
    char unknown_0[208];
    int field_d0;
    Class_004b73b0* InitArgs(void);
};

class Class_004800c0 {
public:
    std::vector<Unit*> units;
    Unit** EraseSwapBack(Unit**);
};

struct Player_00446080 {
    char name[72];
    unsigned char control;
    char unknown_49[258];
};

class Class_0040d7b0 {
public:
    char unknown_0[100];
    Map_0040d7b0* map;
    char unknown_68[16];
    unsigned char player;
    int GetCellState(int, int);
};

struct Map_0040d7b0 {
    char unknown_0[4];
    short footprintX;
    short footprintZ;
    char unknown_8[8];
    unsigned int width;
    unsigned int height;
    unsigned int* cells;
    int InBounds(unsigned int, unsigned int);
    int Get(int, int);
};

struct Cell_0040d8b0 {
    unsigned char kind;
    char unknown_1[3];
};

struct Grid_0040d8b0 {
    Cell_0040d8b0* cells;
    unsigned int width;
    unsigned int height;
    char unknown_c[4];
    unsigned int* dirty;
    int InBounds(unsigned int, unsigned int);
    void Set(unsigned int, unsigned int, unsigned char);
};

class Class_0040d8b0 {
public:
    char unknown_0[28];
    Grid_0040d8b0 grid;
    void MarkGoalCell(unsigned int, unsigned int);
};

struct Cell_0040d900 {
    unsigned char kind;
    char unknown_1[3];
};

struct Grid_0040d900 {
    Cell_0040d900* cells;
    unsigned int width;
    unsigned int height;
    int count;
    unsigned int* dirty;
    void ClearBlock(int, int);
    void ClearLast(int);
};

class PathMap {
public:
    void* field_0;
    void* field_4;
    int freeHead;
    int used;
    int field_10;
    int field_14;
    int topPopped;
    void* field_1c;
    unsigned int width;
    int field_24;
    int field_28;
    void* field_2c;
    char unknown_30[4];
    Point16 found;
    char unknown_38[12];
    int range;
    int value;
    int steps;
    int field_50;
    int fixed;
    int field_58;
    int field_5c;
    int field_60;
    int field_64;
    char unknown_68[16];
    unsigned char player;
    int field_79[10];
    int field_a1[10];
    PathMap(void);
    ~PathMap(void);
    int Size(void);
    void ClearDirtyCells(void);
    void AbortIfGoalMatch(int);
    void RunSearches(void);
    void Release(void);
};

class Class_0040da40 {
public:
    char unknown_0[80];
    int field_50;
    char unknown_54[12];
    Target_0040da40* field_60;
    __int64 FUN_0040da40(int, int);
};

class Target_0040da40 {
public:
    virtual int unused0(int, int);
    virtual int unused1(int, int);
    virtual int unused2(int, int);
    virtual int unused3(int, int);
    virtual int unused4(int, int);
    virtual int unused5(int, int);
    virtual int unused6(int, int);
    virtual int Func(int, int);
};

struct Cell_0040da70 {
    unsigned char flags;
    char unknown_1;
    short node;
};

struct Grid_0040da70 {
    Cell_0040da70* cells;
    unsigned int width;
    unsigned int height;
    char unknown_c[4];
    unsigned int* dirty;
    int InBounds(unsigned int, unsigned int);
};

struct HeapNode_0040f000 {
    int index;
    char unknown_4[8];
    int key;
};

struct NodeData_0040da70 {
    Point16 pos;
    int g;
    int f;
    short unknown_c;
    short depth;
    NodeData_0040da70(short, short, int, int, short);
};

struct Target_0040da70 {
    virtual int unused0(int, int);
    virtual int unused1(int, int);
    virtual int unused2(int, int);
    virtual int unused3(int, int);
    virtual int unused4(int, int);
    virtual int unused5(int, int);
    virtual int unused6(int, int);
    virtual int Func(int, int);
};

class Class_0040df00 {
public:
    Node_0040df00* nodes;
    Node_0040df00** heap;
    int field_8;
    char unknown_c[8];
    int count;
    int field_18;
    Cell_0040da70* cells;
    int width;
    int height;
    char unknown_28[12];
    int field_34;
    char unknown_38[12];
    int field_44;
    void Pop(void);
    int ExpandBestNode(void);
};

struct Node_0040df00 {
    int heapPos;
    NodeData_0040da70 data;
};

struct Cell_0040e050 {
    unsigned char flags;
    char dir;
    short node;
};

struct Grid_0040e050 {
    Cell_0040e050* cells;
    int width;
    int height;
    int count;
    unsigned int* dirty;
    Cell_0040e050* At(int, int);
};

class Class_0040e050 {
public:
    char unknown_0[28];
    Grid_0040e050 grid;
    Point16 start;
    Point16 found;
    char unknown_38[36];
    AiSearchGoal* path;
    char unknown_60[4];
    Map_0040d7b0* map;
    void TracePath(void);
};

struct Cell_0040e160 {
    unsigned char flags;
    char dir;
    short heapIndex;
};

class Class_0040e160 {
public:
    char unknown_0[28];
    Cell_0040e160* cells;
    unsigned int width;
    unsigned int height;
    char unknown_28[4];
    unsigned int* dirty;
    short startX;
    short startY;
    char unknown_34[4];
    int goalX;
    int goalY;
    char unknown_40[12];
    int steps;
    int costScale;
    char unknown_54[12];
    Target_0040e160* target;
    int Cost(int, int);
    int ProbeStraightPath(void);
    int CostW(int, int);
    unsigned int Passable(int, int);
    unsigned char Visit(unsigned int, unsigned int, char);
    int OnLine(int, int, int, int);
    int Dir(int, int);
};

class Target_0040e160 {
public:
    virtual int unused0(int, int);
    virtual int unused1(int, int);
    virtual int unused2(int, int);
    virtual int unused3(int, int);
    virtual int unused4(int, int);
    virtual int unused5(int, int);
    virtual int unused6(int, int);
    virtual int Cost(int, int);
};

class Base_00490a10 {
public:
    int satisfied;
    int celebrated;
    virtual ~Base_00490a10(void);
    virtual void Unknown_0(void);
    virtual void SerializeSave(void);
    virtual void SetPathOrder(void*);
    virtual void FUN_0044ce40(void);
    virtual void TickTowardGoal(void);
    virtual int FillWaypointWorldPos(int, int, int);
    virtual void IsFxStyle(void);
    virtual int FUN_0044cf00(Unit*);
    virtual int ExportGoalPose(Unit*);
    virtual int HasReadyWaypoints(void);
    virtual void ContainsCell(void);
    virtual int TryClaimRepath(void);
    virtual void FillGoalCells(void);
    virtual int HasNetUnitState(void);
    virtual void ApproxDist(void);
    virtual void SerializeNetUnitState(Vec3*);
    virtual void FUN_004e6110(void);
    virtual int DeserializeNetUnitState(short*);
    virtual void TryGetDesiredHeading(void);
    virtual void FUN_0044cf50(void);
    virtual void DrawOnSurface(int);
    virtual int KeepAfterComplete(void);
    virtual int FUN_0044ef50_11(void);
    void AddFlags(int);
};

struct Cell_0040e630 {
    unsigned char flags;
    char dir;
    short node;
};

class Class_0040f110 {
public:
    Elem_0040f110* elems;
    Elem_0040f110** ptrs;
    int field_8;
    int count;
    int capacity;
    int ptr_count;
    void GrowNodes(int);
};

class Class_00440af0 {
public:
    void* field_0;
    char unknown_4[24];
    unsigned int lastTick;
    void RefreshMovedUnits(Unit*);
};

class Dummy_00440be0 {
public:
    int* field_0;
    char unknown_4[24];
    unsigned int field_1c;
    void RefreshUnitIfStale(Unit*);
};

struct Grid_0040e630 {
    Cell_0040e630* cells;
    unsigned int width;
    unsigned int height;
    char unknown_c[4];
    unsigned int* dirty;
    int InBounds(unsigned int, unsigned int);
    void Set(unsigned int, unsigned int, unsigned char);
    unsigned int Index(unsigned int, unsigned int);
};

struct Heap_0040e630 {
    Node_0040e630* pool;
    Node_0040e630** items;
    int freeHead;
    int used;
    int capacity;
    int count;
    int topPopped;
    void Clear(void);
    int Push(NodeData_0040da70&);
};

struct Node_0040e630 {
    int index;
    NodeData_0040da70 data;
};

struct Pair_0040e630 {
    unsigned char a;
    unsigned char b;
};

class Target_0040e630 {
public:
    virtual void unused0(void);
    virtual void unused1(void);
    virtual void unused2(void);
    virtual void unused3(void);
    virtual void unused4(void);
    virtual int IsGoal(int, int);
    virtual void GetGoals(std::vector<Point16>&);
    virtual int Cost(int, int);
};

class Class_0040e9a0 {
public:
    char unknown_0[88];
    void* field_0x58;
    char unknown_5c[8];
    void* field_0x64;
    void ReleaseHeldPathLock(void);
};

class Class_0040ef20 {
public:
    Node_0040ef20* pool;
    Node_0040ef20** items;
    int freeHead;
    int used;
    int capacity;
    int count;
    void RemoveNode(int);
    void Free(int);
    void SiftDown(int);
};

class Class_0040f1e0 {
public:
    Entry_0040f1e0* entries;
    int field_4;
    int free_head;
    void FreeNode(int);
};

class Handler_004388d0 {
public:
    Attached_0043a1f0* current;
    char unknown_8[24];
    int field_20;
    virtual void FUN_Virtual0(int);
    virtual void Slot0(void);
    virtual void unused0(void);
    virtual void v0(void);
    virtual void vf0(void);
    virtual void Attach(void*);
    virtual void Slot1(int);
    virtual void unused1(void);
    virtual void v1(void);
    virtual void vf1(void);
    virtual void unused2(void);
    virtual void v2(void);
    virtual void vf2(void);
    virtual void unused3(void);
    virtual void v3(void);
    virtual void vf3(void);
    virtual void unused4(void);
    virtual void v4(void);
    virtual void vf4(void);
    virtual void unused5(void);
    virtual void v5(void);
    virtual void vf5(void);
    virtual AiSearchGoal* GetPath(void);
    virtual void v6(void);
    virtual void vf6(void);
    virtual int vf7(void);
    virtual void v7(void);
    virtual void WriteTo(BitWriter*);
    virtual void v8(void);
    virtual void ReadFrom(BitReader*);
};

struct Node_0040eb70 {
    int index;
    NodeData_0040da70 data;
};

struct Node_0040ef20 {
    int index;
    NodeData_0040da70 data;
};

struct Elem_0040f110 {
    int field_0;
    int field_4;
    int field_8;
    int field_c;
    int field_10;
};

struct Entry_0040f1e0 {
    int next;
    char unknown_4[16];
};

class Class_0043d210 {
public:
    void* obj;
    int field_4;
    Vec3 velocity;
    Vec3 v2;
    int field_20;
    short field_24;
    int field_26;
    char unknown_2a[4];
    unsigned char state : 2;
    unsigned char flag : 1;
    unsigned char rest : 5;
    void ApplyBankAndPitch(Unit*, Vec3*);
    void SetFlightMode(Unit*, int);
    void LoadMotion(Unit*, HapiBank*);
};

class Class_0044e6c0 {
public:
    char unknown_0[8];
    unsigned short field_8;
    short field_a;
    short field_c;
    char unknown_e[24];
    Vec3 pos;
    void SetAltitude(int);
};

class Class_0044e730 {
public:
    char unknown_0[8];
    unsigned short unknown_bits : 4;
    unsigned short flag : 1;
    unsigned short unknown_rest : 11;
    short value;
    void SetApproachRadius(short);
};

union Fixed_2 {
    union { int value; struct { unsigned short frac; short whole; } parts; };
};

class LandingPadList {
public:
    unsigned char field_0;
    char unknown_1[3];
    int field_4;
    int field_8;
    int field_c;
    LandingPadList(LandingPadList&);
    LandingPadList(void);
};

union Fixed_3 {
    union { int value; struct { unsigned short frac; short whole; } parts; };
};

union Fixed_4 {
    union { int value; struct { unsigned short frac; short whole; } parts; };
};

class UnitList {
public:
    char unknown_0[16];
};

struct Mover {
    char unknown_0[220];
    int speed;
    char unknown_e0[49];
    union { unsigned int f111; struct { unsigned int b0_7 : 8; unsigned int b8 : 1; unsigned int b9_31 : 23; } f111b; };
};

union Fixed_5 {
    union { int v; struct { unsigned short frac; short whole; } p; };
};

class Class_0044e720 {
public:
    char unknown_0[8];
    unsigned short unknown_bits : 6;
    unsigned short flag : 1;
    unsigned short unknown_rest : 9;
    char unknown_a[4];
    short value;
    void SetHeading(short);
};

class Order_00415250 {
public:
    char unknown_0[5];
    unsigned char state;
    unsigned int flag_0 : 1;
    unsigned int flag_1 : 1;
    unsigned int flag_2 : 1;
    unsigned int started : 1;
    unsigned int flag_rest : 28;
    char unknown_a[12];
    int target;
};

class Class_00415b60 {
public:
    int bit;
    int index;
    int capacity;
    unsigned int* data;
    unsigned int buffer[256];
    Class_00415b60(void);
    void GrowBuffer(void);
};

struct Class_00415b90 {
    char unknown_0[12];
    void* ptr;
    char unknown_10[4];
    void FreeBuffer(void);
};

class BitWriter {
public:
    int bit;
    int index;
    int capacity;
    unsigned int* data;
    unsigned int buffer[256];
    void WriteBits(int, int);
};

struct Class_00415da0 {
    char unknown_0[12];
    void* field_c;
    void SetByteAt(int, unsigned char);
};

struct Pair_00419560 {
    unsigned int offset;
    int length;
    Pair_00419560(unsigned int, unsigned int);
    Pair_00419560(void);
};

class Class_004618a0 {
public:
    char unknown_0[1];
    int AppendToSendBuffer(unsigned char*, unsigned int);
    int SendAllQueued(int);
    void NopRet_D(void);
    void QueueOnChannel(int, Class_00462710*, int, int);
    void SetDefaultSendPacing(int);
    void* QueuePacket(int, int, void*, unsigned int);
};

class Class_00437c80 {
public:
    int length;
    Record_00474cd0* records;
    char unknown_8[4];
    void* handle;
    void* ptr;
    Class_00437c80* Construct(void);
    void FlushCache(void);
    void ReleaseHandle(int);
    void Destroy(void);
    void DrawObjectState(SpotState*, Vec3*);
};

#include "../src/game/victory_conditions.h"

struct Flags_004165c0 {
    unsigned short low : 8;
    unsigned short flag : 1;
    unsigned short rest : 7;
};

struct Flags_416630 {
    unsigned short low : 3;
    unsigned short flag : 1;
    unsigned short rest : 12;
};

struct Flags_00416660 {
    unsigned short low : 4;
    unsigned short flag : 1;
    unsigned short rest : 11;
};

class Class_004ceb60 {
public:
    char unknown_0[32];
    int field_20;
    char unknown_24[472];
    int field_1fc;
    int field_200;
    int field_204;
    int field_208;
    int field_20c;
    int field_210;
    char arr_214[100];
    int field_278;
    int field_27c;
    int field_280;
    int field_284;
    char unknown_288[4];
    int field_28c;
    int PlayCdTrack(int, int);
};

class Class_004ced40 {
public:
    char unknown_0[512];
    int unknown_200;
    char unknown_204[4];
    int unknown_208;
    int unknown_20c;
    char unknown_210[116];
    int unknown_284;
    int StopCdAudio(void);
};

class Class_004cfe80 {
public:
    char unknown_0[4];
    int field_4;
    void Enable3D(void);
};

class Class_004cfe90 {
public:
    char unknown_0[4];
    int field_4;
    void Disable3D(void);
};

class Class_004cfea0 {
public:
    char unknown_0[1];
    int Is3DEnabled(void);
};

struct Flags_00416e00 {
    unsigned short low : 2;
    unsigned short flag : 1;
    unsigned short rest : 13;
};

struct Flags_00416e30 {
    unsigned short b0 : 1;
    unsigned short b1 : 1;
    unsigned short b2 : 1;
    unsigned short b3 : 1;
    unsigned short rest : 12;
};

struct Flags_00416e60 {
    unsigned short b0 : 1;
    unsigned short b1 : 1;
    unsigned short b2 : 1;
    unsigned short b3 : 1;
    unsigned short b4 : 1;
    unsigned short rest : 11;
};

struct Flags_00417060 {
    unsigned short low : 8;
    unsigned short flag : 1;
    unsigned short rest : 7;
};

struct Flags_00417090 {
    unsigned short b0 : 1;
    unsigned short b1 : 1;
    unsigned short b2 : 1;
    unsigned short b3 : 1;
    unsigned short b4 : 1;
    unsigned short b5 : 1;
    unsigned short b6 : 1;
    unsigned short b7 : 1;
    unsigned short b8 : 1;
    unsigned short b9 : 1;
    unsigned short rest : 6;
};

struct Flags_00417300 {
    unsigned short low : 6;
    unsigned short flag : 1;
    unsigned short rest : 9;
};

class Class_004cdb40 {
public:
    int field_0;
    int field_4;
    float field_8;
    float field_c;
    int wave_devices;
    unsigned int aux_device;
    int wave_volume;
    int field_1c;
    int field_20;
    void* field_24;
    int field_28;
    int field_2c;
    int count;
    int field_34;
    void* buffers[32];
    int priority[32];
    int flags[32];
    int field_1b8;
    int field_1bc;
    int field_1c0;
    char unknown_1c4[32];
    int field_1e4;
    Class_004bbbe0* file;
    char unknown_1ec[16];
    int field_1fc;
    int size;
    int value;
    int field_208;
    int field_20c;
    int field_210;
    unsigned char field_214;
    char unknown_215[99];
    int field_278;
    int field_27c;
    int field_280;
    int field_284;
    int handle;
    int field_28c;
    int field_290;
    int GetTrackCount(void);
    int GetTrackCategory(void);
    int GetCurrentTrack(void);
    int InitDirectSound(int, int, int, void*);
    int PlaySampleSet(void**, long, Pos_004cf570*);
    int IsStreamActive(void);
    unsigned char GetCategoryOfTrack(int);
    void PlayNextTrack(void);
    void CloseCdPlayerWindow(void);
    void SetLockedTrack(int);
    void SetTrackCategory(int);
    void SetCategoryOfTrack(int, unsigned char);
    void ReleaseSampleSet(void**);
    void StopStream(void);
    void RestoreMixerVolumes(void);
    void LoadSample(char*);
    void PlaySample(char*, int, int);
};

struct Flags_004177a0 {
    unsigned short b0 : 1;
    unsigned short rest : 15;
};

class Class_004b7440 {
public:
    char* field_0[20];
    char field_50[126];
    char unknown_ce;
    char unknown_cf[1];
    int field_d0;
    void Tokenize(char*, char*);
};

class Class_00417e00 {
public:
    char unknown_0[8];
    char* field_8;
    Point16 points[20];
    int count;
    char unknown_60[4];
    unsigned char field_64;
    char unknown_65[3];
    void FUN_00417e00(void*);
};

struct Movement {
    char unknown_0[16];
    int width;
    char unknown_14[4];
    unsigned int* states;
};

struct Flags_418ca0 {
    unsigned short low : 10;
    unsigned short flag : 1;
    unsigned short rest : 5;
};

struct Flags_418fd0 {
    char unknown_0[2];
};

#include "../src/ingame/arg_00419670.h"

struct Flags_00419670 {
    unsigned int bits_0 : 6;
    unsigned int flag_6 : 1;
    unsigned int bits_7 : 4;
    unsigned int flag_11 : 1;
    unsigned int bits_12 : 20;
};

union FlagsU_00419670 {
    union { unsigned int raw; Flags_00419670 bits; };
};

#include "../src/ingame/bit_flags_004197d0.h"

#include "../src/ingame/flags_004197d0.h"

struct Entry {
    unsigned char type;
    unsigned char kind;
    unsigned short h;
    int value;
    int size;
    int current;
    char unknown_10[2];
    char unknown_12;
    short x;
    short y;
    short w;
    short h_19;
    int flags;
    int colours;
    int image;
    unsigned char f27;
    char group;
    unsigned char field_29;
    unsigned char flags_2a;
    char unknown_2b[4];
    GafEntry_004a5f40* gaf;
    char unknown_33[37];
    int field_58;
    char unknown_5c[4];
    int index;
    char unknown_64[82];
    short count;
    short unknown_b8;
    short field_ba;
    char unknown_bc[4];
    short field_c0;
    char* text;
    unsigned short field_c6;
    unsigned int c8_0 : 1;
    unsigned int c8_rest : 31;
    char unknown_cc[2];
    void* field_ce;
    int showText;
    int id;
    short field_da;
    short field_da_dc;
    char unknown_de[14];
    char focusName[16];
    char unknown_fc[58];
    short steps;
    short field_138;
    unsigned char field_13a;
    unsigned char field_13b;
    int max;
    short pos;
    short unknown_142;
    void (__stdcall *handler)(void*, int);
    char unknown_148[2];
    int field_14a;
    unsigned short* glyphs;
    unsigned char sliderStyle;
    char unknown_153[4];
    int field_157;
    char unknown_15b[3817];
    ~Entry(void);
};

struct Object_00419940 {
    char unknown_0[24];
    BuildList_0045a950* data;
};

struct Unit_00419ac0 {
    char unknown_0[270];
    unsigned short on : 1;
    unsigned short : 15;
};

struct Orders_0041a120 {
    char unknown_0[2];
    unsigned short moveOrder : 3;
    unsigned short cloak : 2;
    unsigned short onOff : 2;
    unsigned short canMove : 1;
    unsigned short canStop : 1;
    unsigned short canAttack : 1;
    unsigned short canDefend : 1;
    unsigned short canPatrol : 1;
    unsigned short canLoad : 1;
    unsigned short canReclaim : 1;
    unsigned short canCapture : 1;
    unsigned short canRepair : 1;
    unsigned short canBlast : 1;
    unsigned short unknown_1 : 15;
};

struct Player_0041a920 {
    char name[562];
};

struct BuildEntry_0041ace0 {
    unsigned short typeId;
    unsigned char page;
    unsigned char slot;
    char name[33];
};

struct Table_0041ace0 {
    char unknown_0[182];
    short count;
};

struct PlayerEntry_0041b0f0 {
    Unit* unit;
    char unknown_4[327];
};

struct Obj_0041b200 {
    char unknown_0[272];
    unsigned int bits_0 : 22;
    unsigned int flag_22 : 1;
    unsigned int value_23 : 3;
    unsigned int : 6;
};

struct Menu_0041b8d0 {
    char unknown_0[24];
    void* entry;
    char unknown_1c[3246];
    int field_cca;
};

struct CursorState_0041cd20 {
    int x;
    int y;
    char unknown_8[16];
    int flag;
};

#include "../src/ingame/point_0041cd50.h"

#include "../src/ingame/cursor_state_0041cc60.h"

struct Display_0041ce90 {
    char unknown_0[64];
    void* hwnd;
    char unknown_44[172];
    unsigned char flags_f0;
};

struct Mouse_0041ce90 {
    int x;
    int y;
    int unknown_8[4];
};

class Class_004b4630 {
public:
    Table_004b3630* table;
    int SetIntegerItem(char*, int);
};

class Class_004c2f60 {
public:
    Class_004c42a0* root;
    int field_4;
    int field_8;
    int LoadFile(char*);
    void LoadBuffer(char*, int, int, char*);
};


#include "../src/map/grid.h"

class Class_00463be0 {
public:
    int field_0;
    char unknown_4[35];
    Ref<PlayerInfo> data;
    char unknown_2b[72];
    char field_73;
    char unknown_74[8];
    Grid grid;
    char unknown_8c[186];
    char field_146;
    char unknown_147[4];
    Class_00463be0(void);
};

struct Display_0041da60 {
    char unknown_0[1556];
    float field_614;
};

struct Header_0041e420 {
    unsigned char type;
    char unknown_1;
    char name[17];
    short x;
    short y;
    short width;
    short height;
    int attr;
    int color;
    int color2;
    char unknown_27[2];
    unsigned char flag;
    char unknown_2a[140];
};

struct Button_0041e420 {
    Header_0041e420 h;
    union { int field_b6; short count; };
    char unknown_ba[4];
    int field_be;
    int field_c2;
    short field_c6;
    int flags;
};

#include "../src/game/slot_0041e420.h"

struct Display_0041ec50 {
    char unknown_0[1556];
    int field_614;
};

struct Entry_00479620 {
    int active;
    int unknown_4;
    int type;
    int field_c;
    int field_10;
    int owner;
    char unknown_18[240];
    int skirmishCommanderDeath;
    int skirmishMapping;
    int skirmishLineOfSight;
    int skirmishLOSType;
    int fixedloc;
    char text_11c[1];
    char unknown_11d[259];
    int field_220;
    int field_224;
    int skirmishDifficulty;
};

struct Data_0046d970 {
    char unknown_0[128];
    char name[1];
    char unknown_81[10];
    unsigned short field_8b;
    unsigned short field_8d;
    char unknown_8f[1];
    int id;
    unsigned char field_94;
    unsigned char side;
    unsigned char field_96;
    unsigned char f97_0 : 1;
    unsigned char f97_rest : 7;
    char unknown_98[1];
    unsigned short field_99;
    unsigned short field_9b;
    unsigned short flags_9d;
    char unknown_9f[2];
    unsigned short energy;
    unsigned short metal;
    unsigned short maxUnits;
    unsigned char versionMajor;
    unsigned char versionMinor;
    unsigned int field_a9;
    char unknown_ad[12];
};

struct Display_0041f760 {
    char unknown_0[212];
    int x;
    int y;
};

struct Engine {
    char unknown_0[196];
    unsigned char* fade_neg;
    unsigned char* fade_pos;
    char unknown_cc[8];
    int width;
    int height;
    char unknown_dc[20];
    unsigned short low : 1;
    unsigned short network : 1;
    unsigned short high : 14;
};

class Class_004379a0 {
public:
    char unknown_0[1];
    int AllocHandle(int*, int);
    void ClearPointers(void);
    void InitCache(int);
    void FUN_00437a20(void);
};

struct ImageFrame {
    void* image;
    short duration;
    short unknown;
};

struct Explosion {
    short count;
    char flags;
    char unknown_3[37];
    ImageFrame frames[1];
};

#include "../src/graphics/object3do.h"

struct Shape {
    int a;
    int b;
};

struct Entry_00420960 {
    void* ptr;
    int field_4;
};

struct Handle {
    unsigned short index;
    unsigned short value;
    unsigned char kind;
    char unknown_5[3];
    void* src;
};


#include "../src/weapons/debris.h"

struct Effect {
    void* model;
    Handle image;
    Handle shadow;
    Vec3 position;
    char unknown_28[36];
    short rotation[4];
};

struct Effects {
    int count;
    Effect entries[300];
};

struct ExplosionEntry {
    void* frame;
    short kind;
    short pad;
};

struct CalcedExplosion {
    short count;
    char flag;
    char unknown_3[37];
    ExplosionEntry entries[1];
};

struct Spawn_00420e50 {
    void* unit;
    int index;
    int x08;
    int x0c;
    int x10;
    int x14;
    int x18;
    int x1c;
    int x20;
    int x24;
    unsigned int b0 : 1;
    unsigned int b1 : 1;
    unsigned int b2 : 1;
    unsigned int b3 : 1;
    unsigned int b4 : 1;
    unsigned int b5 : 1;
    unsigned int b6 : 26;
    int x2c;
};

struct Unit_00420e50 {
    char unknown_0[158];
    int* records;
};

struct Arr_00421550 {
    char unknown_0[4];
    int count;
    int faceCount;
    int firstFace;
    char unknown_10[20];
    Vec3* items;
    PieceFrame* faces;
};

#include "../src/graphics/piece_frame.h"

struct Inner_00421550 {
    Arr_00421550* f0;
    char unknown_4[12];
    short f10;
    short f12;
    short f14;
    int f16;
    int f1a;
    int f1e;
    Vec3* f22;
};

struct Descriptor_00421620 {
    char unknown_0[4];
    int field_4;
};

struct Header_00421620 {
    int size;
    int index;
    int nInts;
    int nDoubles;
    int nStrings;
    int version;
    int compressed;
    int unknown_1c;
    int featureOffset;
    int scale;
    unsigned int bits;
    void* field_2c;
};

struct Rec_00421620 {
    Descriptor_00421620* desc;
    char unknown_4[18];
    int field_16;
    int field_1a;
    int field_1e;
    void* field_22;
    char unknown_26[2];
    unsigned short field_28;
    char unknown_2a[12];
};

struct Block_00421620 {
    Header_00421620 header;
    Rec_00421620 rec;
};

struct Piece_00458310 {
    Object3do* info;
    int x;
    int y;
    int z;
    short short_10;
    short short_12;
    short short_14;
    int offset_x;
    int offset_y;
    int offset_z;
    Vec3* vertices;
    short modified;
    unsigned char flags;
    char unknown_29[1];
    Piece_00458310* sibling;
    Piece_00458310* child;
    Piece_00458310* parent;
};

struct Texture_00421700 {
    int tex;
    int unknown_4;
    unsigned short* texlist;
};

struct Cell_00421eb0 {
    short x;
    short y;
};

struct FeatureDef_00421eb0 {
    char unknown_0[148];
    Cell_00421eb0 footprint;
};

#include "../src/map/feature_anim_pool.h"

struct Pool_00422170 {
    Entry* entries;
    void* field_4;
    int head8;
    int headC;
};

#include "../src/map/class_004c9390.h"

struct Elem_004222e0 {
    Class_004c9390 name;
    ~Elem_004222e0(void);
};

class Class_004c3e10 {
public:
    char unknown_0[4];
    int field_0x4;
    void ResetCurrentRecord(void);
};

class Class_00435980 {
public:
    int unknown_0;
    char name[2564];
    Class_004c3e10 list;
    int MissionExists(int);
};

class Class_00435760 {
public:
    int unknown_0;
    char name[2564];
    Class_004c3e10 list;
    int BuildMissionList(int*);
};

struct List_004be3b0 {
    int count;
    Entry* entries;
    Class_004c3e10** last;
    char* end;
    void FUN_004758c0(char*, int, Class_00473590&);
    void FUN_00475ef0(char*, int, Class_004745e0&);
};

class Class_004c4760 {
public:
    char unknown_0[25];
    Pair_00419560* first;
    Pair_00419560* last;
    double GetFieldDouble(char*, double);
};

struct Placement {
    char name[128];
    int x;
    int y;
};

struct Pool_004232a0 {
    FeatureSpot* entries;
    char unknown_4[4];
    int usedHead;
    int unknown_c;
    int freeHead;
};

struct Packet_004233a0 {
    unsigned char type;
    unsigned char sub;
    short x;
    short z;
};

struct AnimPair_00423550 {
    AnimSrc_00423550* anim;
    AnimSrc_00423550* shadow;
};

struct Packet_004237d0 {
    unsigned char type;
    unsigned char sub;
    short x;
    short z;
};

struct Fixed_004239c0 {
    unsigned short frac;
    short whole;
};

union Coord_004239c0 {
    union { int value; Fixed_004239c0 f; };
};

struct Obj_00423bf0 {
    char unknown_0[40];
    short x;
    short y;
    unsigned short entry;
};

struct Fixed_00424050 {
    unsigned short frac;
    short whole;
};

struct Frame_00424050 {
    unsigned short width;
    unsigned short height;
    unsigned short originX;
    unsigned short originY;
};

struct Packet_004244b0 {
    unsigned char type;
    unsigned char sub;
    short x;
    short z;
};

union SpotField_004246b0 {
    char unknown_0[2];
};

struct Anim_00424890 {
    unsigned short x;
    unsigned short y;
    unsigned short feature;
    unsigned short damage;
    unsigned char frame;
    unsigned char anim : 4;
    unsigned char animHi : 4;
};

class Class_004b4bf0 {
public:
    Table_004b3630* table;
    int GetBoxSize(void);
};

struct FeatureName_00424890 {
    char name[128];
};

struct Model_00424890 {
    short x;
    short y;
    unsigned short feature;
    unsigned short damage;
    Vec3 pos;
    Rot16 rot;
};

struct Normal_00424890 {
    unsigned short x;
    unsigned short y;
    unsigned short feature;
    unsigned short spot;
};

struct FeatureName_00424c00 {
    char name[128];
};

struct Model_00424c00 {
    unsigned short x;
    unsigned short y;
    unsigned short feature;
    unsigned short damage;
    Vec3 pos;
    Rot16 rot;
};

struct Gadget_00426190 {
    char unknown_0[24];
    Dialog* owner;
    char unknown_1c[68];
    int field_60;
};

class Class_004ce1d0 {
public:
    char unknown_0[1];
    bool HasCdPlayerWindow(void);
};

class Class_004cff20 {
public:
    char unknown_0[1];
    int HasNoDriver(void);
};

struct Entry_004263b0 {
    void* value;
    char name[36];
};

class Class_0047bf20 {
public:
    void* smack;
    char unknown_4[1040];
    int hasSurfaces;
    char unknown_418[300];
    Surfaces_0047bdf0* surfaces;
    void FUN_0047bf20(void);
};

class Class_0047c6c0 {
public:
    void* smack;
    int unknown_4;
    int stopped;
    void* hwnd;
    void WriteSmackStats(void);
    void Play(void);
};

struct Flags_004268b0 {
    unsigned short b0 : 1;
    unsigned short b1 : 1;
    unsigned short b2 : 1;
    unsigned short b3 : 1;
    unsigned short rest : 12;
};

struct Flags_004269d0 {
    unsigned short b0 : 1;
    unsigned short b1 : 1;
    unsigned short b2 : 1;
    unsigned short b3 : 1;
    unsigned short rest : 12;
};

struct Obj_00426e80 {
    char unknown_0[240];
    unsigned short bit0 : 1;
    unsigned short flag : 1;
    unsigned short : 14;
};

#include "v4i.h"

class AIProfileParser {
public:
    char flag;
    char unknown_1[127];
    char* field_80;
    char* field_84;
    char* field_88;
    int field_8c;
    int field_90;
    int field_94;
    AIProfileParser(void);
    void SetBuffer(char*, int);
    int NextToken(void);
    int ReadInt(void);
};

class Class_00428d10 {
public:
    char token[128];
    char* field_80;
    char* field_84;
    char* field_88;
    int field_8c;
    int field_90;
    int field_94;
    int ScanToken(void);
};

struct Class_00428f40 {
    char unknown_0[1];
    bool TokenEquals(char*);
};

class Class_00428f60 {
public:
    char unknown_0[140];
    int errorReported;
    void ReportParseError(char*);
};

struct Bitmap_00437b50 {
    unsigned short width;
    unsigned short height;
    short dx;
    short dy;
    unsigned char colour;
    unsigned char flag9;
    unsigned char count;
    unsigned char kind;
    int unknown_c;
    unsigned char* data;
    unsigned char* depth;
    unsigned char pixels[1];
    unsigned short Pitch(void);
};

struct AnimEntry_00429700 {
    char name[64];
    void* gaf;
};

class Class_004c3240 {
public:
    Class_004c42a0* root;
    int field_4;
    int field_8;
    void Unload(void);
};

struct Class_004402e0 {
    int* field_0;
    short field_4;
    short field_6;
    short field_8;
    short field_a;
    unsigned char field_c;
    unsigned char field_d;
    unsigned char field_e;
    unsigned char field_f;
    int field_10;
    int field_14;
    void* field_18;
    int field_1c;
    Class_004402e0(void);
    ~Class_004402e0(void);
};

#include "../src/units/movement_class.h"

class Class_004c4630 {
public:
    char unknown_0[25];
    Pair_00419560* first;
    Pair_00419560* last;
    int FindFieldValue(char*);
};

class Class_004c4800 {
public:
    char unknown_0[25];
    Entry* first;
    Entry* last;
    Fixed_6 GetFieldFixed(char*, Fixed_6);
    int* GetFieldFixed(int*, char*, int);
};

class Fixed_6 {
public:
    int value;
    Fixed_6(int);
};

struct MissionHolder {
    char unknown_0[3];
    MissionType mission;
    MissionHolder(char*);
};

struct CobFile_0042d1f0 {
    int unknown_0;
    int count_1;
    int count_2;
    int unknown_c;
    int checksum;
    int count_3;
    int offset_18;
    int offset_1c;
    int offset_20;
    int offset_24;
    int offset_28;
    char unknown_2c[356];
    int field_18e;
};

class Class_00458180 {
public:
    char unknown_0[16];
    void* buffer;
    int Initialize(unsigned int);
};

union UType_0042b370_flags {
    unsigned int bits_0 : 11;
    unsigned int flag_11 : 1;
    unsigned int bits_12 : 16;
    unsigned int flag_28 : 1;
    unsigned int bits_29 : 3;
};

class Class_004c3490 {
public:
    Class_004c42a0* root;
    int field_4;
    int SelectRecordAt(int);
};

class Class_004c4440 {
public:
    char unknown_0[1];
    int GetRecordName(void);
};

class Class_004c4470 {
public:
    int unknown_0;
    std::vector<TdfRecord*> entries;
    TdfRecord* FindSubRecord(char*);
};

class Class_004c45e0 {
public:
    char unknown_0[21];
    std::vector<Pair_00419560> pairs;
    char* GetFieldName(int);
};

class Class_004c91b0 : public Class_004c9390 {
public:
    Class_004c91b0(char*);
    ~Class_004c91b0(void);
};

class Map_0042e440 {
public:
    char unknown_0;
    std::vector<Elem_0040cc40> vec;
    Elem_0040cc40* LowerBound(char*);
};

class Class_004c4450 {
public:
    int unknown_0;
    std::vector<Named_004c4450*> entries;
    int GetSubRecordCount(void);
};

struct Slot_0042f740 {
    int count;
    int* a;
    int* b;
};

class Class_00435d30 {
public:
    char unknown_0[3364];
    char* list;
    int count;
    int multi;
    void RefreshMapList(int);
};

class Class_004cf210 {
public:
    char unknown_0[44];
    int field_2c;
    void SetMaxBuffers(int);
};

class Class_004d0070 {
public:
    char unknown_0[16];
    int wave_devices;
    int SetWaveVolume(int);
};

class Class_004d00d0 {
public:
    char unknown_0[20];
    unsigned int aux_device;
    char unknown_18[8];
    int aux_volume_set;
    char unknown_24[608];
    int field_284;
    int SetAuxVolume(int, int);
};

#include "../src/ingame/flags_00495e90_37f06.h"

#include "../src/game/mission_flags_0042f9a0.h"

class Sound {
public:
    char unknown_0[1];
    int GetMaxBuffers(void);
};

class Class_004cfff0 {
public:
    char unknown_0[16];
    int wave_devices;
    int QueryWaveVolume(void);
};

class Class_004d0040 {
public:
    char unknown_0[20];
    int aux_device;
    int QueryAuxVolume(void);
};

class Class_004c3e20 {
public:
    char unknown_0[1];
    int GetCurrentRecord(void);
};

class Class_004c3e30 {
public:
    char unknown_0[4];
    int field_4;
    void SetCurrentRecord(int);
};

struct Obj_00431950 {
    char unknown_0[4];
    TdfRecord* table;
};

struct Entry_00431a20 {
    int* buffer;
    char unknown_4[558];
};

#include "../src/game/side_00431a60.h"

class Class_004b3630 {
public:
    int field_0;
    Class_004b3630* InitBank(void);
    char* GetStringItem(char*, char*);
    void CloseBank(void);
};

class Class_004b3770 : public Class_004b3630 {
public:
    char unknown_4[4];
    int field_8;
    int OpenBank(char*, char*, void*);
    void LoadAccount(Class_004bbbe0*, int*, char*);
};

class Class_004b48f0 {
public:
    File_004b4910* file;
    int HasItem(char*);
    int FindItem(char*, int);
};

class Class_004352b0 {
public:
    char unknown_0[4];
    char name[1];
    char* GetCampaignName(void);
};

class Class_004b3750 {
public:
    void* field_0;
    char unknown_4[4];
    int field_8;
    int SaveBank(char*, char*, int, int);
    void NewBank(void);
    void SaveAccount(int, _iobuf*, Buffer_004b39c0*, int);
};

class Class_004b4750 {
public:
    Table_004b3630* table;
    int SetStringItem(char*, char*);
};

struct Elem_00432be0_2 {
    char* data;
};

class Class_00432c00 {
public:
    char unknown_0[1];
};

class Class_00432c20 {
public:
    char unknown_0[1];
};

struct Elem_432cb0 {
    Class_004c9390 handle;
    int field4;
};

struct Elem_432cb0_2 {
    Class_004c9390 handle;
    int field4;
};

class Class_004330b0 {
public:
    std::allocator<int> allocator;
    char unknown_1[3];
    int* first;
    int* last;
    int* end;
    Class_004330b0(std::allocator<int>&);
    ~Class_004330b0(void);
    void FreeTables(void);
};

struct Elem_00434360_2 {
    std::vector<Elem_00434020> v;
};

class Class_00433130 {
public:
    std::vector<std::vector<Elem_00434360> > tables;
    void LoadLosTables(void);
};

class Class_00433270 {
public:
    char unknown_0[16];
    void ResizeTables(short);
};

struct Wrap_00433270 {
    std::vector<Elem_00434020> v;
};

class Class_004335f0 {
public:
    char unknown_0[16];
    Elem_00434360* GetLine(short);
    void ResizeLines(short);
    void SetNumLines(short);
};

class Class_00433380 {
public:
    std::vector<Class_004335f0> tables;
    Class_004335f0* GetTable(short);
    void LoadLosTable(Unit*, short);
};

class Class_004336f0 {
public:
    char unknown_0[16];
    void LoadLosLine(Unit*, short, short);
};

class Class_00433500 {
public:
    std::vector<std::vector<Elem_00434020> > items;
    std::vector<Elem_00434020>* GetLosTable(int);
};

class Class_00433520 {
public:
    char unknown_0[4];
    int field_4;
    int field_8;
    int GetLosTableCount(void);
};

class Class_00433540 {
public:
    char unknown_0[1];
    void FreeLines(void);
};

class Class_004335c0 {
public:
    char unknown_0[4];
    int field_4;
    int field_8;
    int GetLosLineCount(void);
};

class LosTable {
public:
    char unknown_0[4];
    int field_4;
    int GetLosLine(short);
};

class Class_004339c0 {
public:
    char unknown_0[4];
    int field_4;
    int field_8;
    int GetLosLineStepCount(void);
};

class LosLine {
public:
    char unknown_0[1];
    void GetLosLineStep(short, unsigned short*, unsigned short*);
};

struct Elem_00434a60 {
    Class_004c9390 name;
    int value;
    ~Elem_00434a60(void);
};


class Class_00434f70 {
public:
    int owner;
    char unknown_4[2560];
    int field_a04;
    TdfFile field_a08;
    char text_a14[256];
    char text_b14[256];
    int field_c14;
    char unknown_c18[268];
    int field_d24;
    int field_d28;
    int field_d2c;
    char unknown_d30[124];
    int field_dac;
    int field_db0;
    int field_db4;
    int field_db8;
    int field_dbc;
    int field_dc0;
    Class_00434f70(int);
};

struct Buffer_00434ff0 {
    int* data;
    int size;
};

#include "../src/map/mission.h"

class Class_0048dfb0 {
public:
    void* bufsA[16];
    int countA;
    void* bufsB[16];
    int countB;
    void FreeConditions(void);
};

class Class_00435320 {
public:
    char unknown_0[772];
    char text_304[2320];
    char* field_c14;
    void FUN_00435320(void);
};

class Class_004353b0 {
public:
    char unknown_0[260];
    char names[9][256];
    int exists;
    void SetNameSlot(int, char*);
};

class Class_004356f0 {
public:
    int unknown_0;
    char name[2564];
    Class_004c3e10 list;
    int CountMissions(void);
};

class Class_00435900 {
public:
    char unknown_0[1];
    int GetDescription(void);
};

class Class_00435910 {
public:
    char unknown_0[3380];
    int field_d34;
    int field_d38;
    int GetPlanet(void);
};

class Class_00435920 {
public:
    char unknown_0[2564];
    int field_a04;
    int GetTerrainSizeTier(void);
};

class Class_004c3e10_2 {
public:
    char unknown_0[1];
    void ResetCurrentRecord(void);
};

class Class_00435c20 {
public:
    char unknown_0[1];
    int GetTranslatedName(void);
};

class Class_00435c40 {
public:
    char unknown_0[2580];
    unsigned char field_a14;
    bool HasMissionName(void);
};

class Class_00435c50 {
public:
    char unknown_0[1];
    int GetMissionIndex(void);
};

class Class_00436c30 {
public:
    char unknown_0[3500];
    MissionUnit_00436c30* units;
    int unitCount;
    MissionRule_00436c30* rules;
    int ruleCount;
    MissionFeature_00436c30* features;
    int featureCount;
    void LoadMissionData(char*, TdfFile*);
};

class MeteorParams {
public:
    char name[32];
    int radius;
    float density;
    float duration;
    float interval;
    void LoadMeteorDefaults(void);
};

class Class_004c44c0 {
public:
    int unknown_0;
    std::vector<TdfRecord*> entries;
    TdfRecord* GetSubRecord(int);
};

struct MissionFeature_00436c30 {
    char name[128];
    int x;
    int z;
};

struct MissionRule_00436c30 {
    int type;
    int id;
    short x;
    short z;
};

struct MissionUnit_00436c30 {
    char* name;
    char* ident;
    char* initialMission;
    int x;
    int y;
    int z;
    short angle;
    short health;
    int creationCountdown;
    short buildPriority;
    unsigned char player;
    unsigned char initialGroup : 4;
    unsigned char missionCritical : 1;
    unsigned char aiIgnore : 1;
    unsigned char aiPriorityTarget : 1;
    unsigned char immunity : 1;
};

class Class_00437280 {
public:
    char unknown_0[3092];
    int* field_c14;
    char unknown_c18[404];
    Buffer_00434ff0 buffer0;
    Buffer_00434ff0 buffer1;
    Buffer_00434ff0 buffer2;
    void FreeMissionData(void);
};

class Class_00437300 {
public:
    char unknown_0[3508];
    Entry* entries;
    int entry_count;
    int CountStartPositions(void);
};

class Class_004373a0 {
public:
    int type;
    char campaign[256];
    char names[9][256];
    char unknown_a04[532];
    int missionIndex;
    int field_c1c;
    int field_c20;
    int ComputeMapChecksum(void);
    char* GetName(int);
};

class MapCache {
public:
    char unknown_0[4];
    void InsertMapCacheEntry(MapCacheEntry*, int, MapCacheEntry&);
};

class MapCacheEntry {
public:
    Class_004c9390 handle;
    int field_4;
    MapCacheEntry(MapCacheEntry&);
    MapCacheEntry& operator=(MapCacheEntry&);
    MapCacheEntry& SetChecksum(Class_004373a0*);
};

class Class_00437800 {
public:
    char unknown_0[4];
    int field_4;
    Class_00437800(int*);
};

union Fixed_437840 {
    union { int value; struct { unsigned short fraction; short whole; } parts; };
};

class Class_004379a0_2 {
public:
    char unknown_0[4];
    int field_4;
    char unknown_8[4];
    int field_c;
    Class_004379a0_2* ClearPointers(void);
};

struct Chunk {
    int field_0;
    int field_4;
};

class Class_004379b0 {
public:
    unsigned int size;
    int* block;
    Chunk* free;
    int InitCache(unsigned int);
};

class Class_00437a20 {
public:
    char unknown_0[4];
    int* field_4;
    void FUN_00437a20(void);
};

#include "../src/graphics/memory_cache.h"

struct Record_00474cd0 {
    void* data;
    Vec3 pos;
    int size;
    int field_14;
    int field_18;
    int field_1c;
    int field_20;
    int field_24;
    int field_28;
    int field_2c;
    int field_30;
    int field_34;
    int field_38;
    void Draw(void*, short, short);
    void DrawIfVisible(void*, short, short);
    void FUN_00474b80(void*, short, short);
};

struct Player_00437cd0 {
    char unknown_0[273];
    unsigned char flags;
};

struct Callback_0043a1f0 {
    int value;
    void (__stdcall *notify)(Unit*, Order*, int);
    char unknown_8[4];
    unsigned int flags;
    unsigned char field_10;
    unsigned int flags_11;
    char* name;
};

struct Class_00438870 {
    char unknown_0[76];
    unsigned short unknown_0x4c;
    char unknown_4e[2];
    int field_0x4e;
    void FUN_00438870(unsigned int);
};

class Class_004388b0 {
public:
    char unknown_0[14];
    void* obj_ptr;
    char unknown_12[64];
    int value;
    void ReattachFxToUnit(void);
};

class Attached_004388d0 {
public:
    virtual ~Attached_004388d0(void);
};

class Attached_004388d0_2 {
public:
    virtual ~Attached_004388d0_2(void);
};

class Class_0044cf60 {
public:
    char unknown_0[8];
    Point16 pos;
    int radius;
    int radiusSq;
    Class_0044cf60(Class_0044cf60&);
    Class_0044cf60(Order*, int, int, int);
};

struct Attached_00438a00 {
    virtual ~Attached_00438a00(void);
};

class Class_0044d3b0 {
public:
    char unknown_0[4];
    void* field_4;
    int field_8;
    int radius2;
    int radius1;
    int radius2Sq;
    int radius1Sq;
    Class_0044d3b0(Class_0044d3b0&);
    Class_0044d3b0(Order*, int, int, int, int);
};

class Class_0044d8a0 {
public:
    void* field_4;
    int a;
    int b;
    int c;
    int d;
    virtual ~Class_0044d8a0(void);
    Class_0044d8a0(Order*, Point16, Point16);
    Class_0044d8a0(Class_0044d8a0&);
};

struct View_00438c00 {
    char unknown_0[44];
    int cx;
    int cy;
};

struct Anim_004394e0 {
    unsigned short count;
    char unknown_2[42];
    unsigned short field_2c;
};

struct Trail_004394e0 {
    Vec3 start;
    Vec3 delta;
    int dist;
    int pos;
    int idx;
    void* surface;
    View_00438c00* view;
    Anim_004394e0* anim;
    void Run(void);
};

struct Anim_00439740 {
    unsigned short count;
    char unknown_2[42];
    unsigned short field_2c;
};

struct Entry_439d20 {
    Target_439d20* target;
    char unknown_4[24];
};

struct Node_439d20 {
    char unknown_0[54];
    int index;
    char unknown_3a[4];
    int amount;
    unsigned int flags;
    char unknown_46[4];
    Node_439d20* next;
};

struct Owner_439d20 {
    char unknown_0[16];
    Entry_439d20 entries[2];
    char unknown_48[24];
    Node_439d20* nodes;
};

struct Target_439d20 {
    char unknown_0[228];
    unsigned short field_e4;
};

struct Node_439d80 {
    char unknown_0[54];
    int index;
    int amount;
    char unknown_3e[4];
    unsigned int flags;
    char unknown_46[4];
    Node_439d80* next;
};

struct Owner_439d80 {
    char unknown_0[92];
    Node_439d80* nodesA;
    Node_439d80* nodesB;
};

struct Obj_4 {
    char unknown_0[92];
    Sub_2* sub;
};

struct Sub_2 {
    char unknown_0[4];
    unsigned char index;
};

struct Node_00439e30 {
    char unknown_0[4];
    unsigned char kind;
    char unknown_5[69];
    Node_00439e30* next;
};

struct Obj_00439e30 {
    char unknown_0[92];
    Node_00439e30* list;
    Node_00439e30* list2;
};

#include "../src/orders/order_base.h"

class Attached_0043a1f0 {
public:
    virtual ~Attached_0043a1f0(void);
    virtual void Slot0(void);
    virtual int Slot1(Order*, File_0043a970*, char*);
    virtual int Slot2(void);
};

class Class_00489650 {
public:
    char unknown_0[1];
    void Unlink(void);
};

struct Head_0043a420 {
    unsigned char kind;
    unsigned char flag5;
    unsigned int flags6;
    int last_id;
    Unit* unit;
    Head_0043a420(void);
};

struct SaveDesc_0043a420 {
    unsigned short unitType;
    unsigned short ownerType;
    int field_4;
    unsigned char kind;
    unsigned char flag5;
    unsigned int flags6;
    int last_id;
    Vec3 pos;
    int field_1e;
    int field_22;
    int field_26;
    int field_2a;
    int field_2e;
    unsigned int flags;
    int field_36;
};

class File_0043a970 {
public:
    char unknown_0[1];
};

struct Node_0043ad50 {
    char unknown_0[14];
    Owner_0043ad50* owner;
    char unknown_12[48];
    unsigned int flags;
    char unknown_46[4];
    Node_0043ad50* next;
};

struct Owner_0043ad50 {
    char unknown_0[92];
    Node_0043ad50* list_a;
    Node_0043ad50* list_b;
};

#include "../src/orders/order_type_entry.h"

class MissionOrderTable {
public:
    char unknown_0[4];
    int field_4;
    int field_8;
    int GetCount(void);
};

struct Elem_0043ca70 {
    char unknown_0[21];
    char* name;
};

class Class_0043cbb0 {
public:
    char unknown_0[36];
    short turn;
    void ApplyClampedTurnDelta(Unit*, short);
};

class Class_0043cc20 {
public:
    char unknown_0[8];
    Vec3 pos;
    char unknown_14[12];
    int field_20;
    void UpdateVelocityFromHeading(Unit*, int);
};

class Class_0043cd20 {
public:
    Iface_0043dd20* obj;
    char unknown_4[28];
    int field_20;
    short turn;
    char unknown_26[2];
    void SteerGroundUnit(Unit*);
};

class Class_0043d290 : public Class_0043d210 {
public:
    void SteerAircraft(Unit*);
};

class Class_0043d6d0 {
public:
    char unknown_0[8];
    Vec3 p1;
    char unknown_14[12];
    int field_20;
    char unknown_24[6];
    int field_2a;
    unsigned char mode : 2;
    unsigned char flag : 1;
    unsigned char unknown_2f : 5;
    void UpdatePosition(Unit*);
};

struct FP_0043d6d0 {
    unsigned int frac : 16;
    unsigned int whole : 16;
};

class Class_0043da70 {
public:
    char unknown_0[32];
    int field_20;
    short field_24;
    char unknown_26[8];
    unsigned char field_2e;
    void UpdateMoveRate(Unit*);
};

class Class_0043db50 {
public:
    char unknown_0[1];
    void UpdateSfxOccupy(Unit*);
};

struct Vec3i_00474130 {
    int x;
    int y;
    int z;
    Vec3i_00474130& operator+=(Vec3i_00474130&);
};

class Class_0043dd70 {
public:
    char unknown_0[8];
    Vec3i_00474130 a;
    Vec3i_00474130 b;
    int g;
    unsigned short h;
    int i;
    int unknown_2a;
    unsigned char f1 : 2;
    unsigned char f2 : 1;
    unsigned char : 5;
    char unknown_2f[1];
    void SaveMotion(Unit*, HapiBank*);
};

struct MobHdr_43dd70 {
    Vec3i_00474130 a;
    Vec3i_00474130 b;
    int g;
    unsigned short h;
    int i;
    unsigned char f1 : 2;
    unsigned char f2 : 1;
    unsigned char : 5;
    char unknown_23[1];
};

struct Record_0043de30 {
    Vec3 velocity;
    Vec3 v2;
    int d6;
    short w7;
    int d8;
    unsigned char state : 2;
    unsigned char flag : 1;
    unsigned char : 5;
};

struct Obj_0043e0b0 {
    char unknown_0[106];
    Vec3 pos;
    char unknown_76[36];
    Class_004b0a70* table;
    char* table_9e;
};

class Class_00489a70 {
public:
    int f0;
    char unknown_4[106];
    int f6e;
    char unknown_72[24];
    int field_8a;
    char unknown_8e[4];
    UnitDef* def;
    char unknown_96[110];
    float f104;
    char unknown_108[8];
    int f110;
    int CountCargo(void);
    int CanLoad(Class_00489a70*);
};

union F110_0048ad30 {
    unsigned int bits_0 : 4;
    unsigned int flag_4 : 1;
    unsigned int flag_5 : 1;
    unsigned int flag_6 : 1;
    unsigned int flag_7 : 1;
    unsigned int flag_8 : 1;
    unsigned int flag_9 : 1;
    unsigned int flag_10 : 1;
    unsigned int flag_11 : 1;
    unsigned int flag_12 : 1;
    unsigned int bits_13 : 1;
    unsigned int flag_14 : 1;
    unsigned int bits_15 : 17;
};

union Flags110_0043f0e0 {
    unsigned int bits_0 : 31;
    unsigned int flag_31 : 1;
};

union Flags111_0043f0e0 {
    unsigned int bits_0 : 8;
    unsigned int flag_8 : 1;
    unsigned int bits_9 : 8;
    unsigned int flag_17 : 1;
    unsigned int bits_18 : 14;
};

class Class_00440470 {
public:
    char unknown_0[16];
    unsigned int width;
    unsigned int height;
    int* cells;
    void ResizePassMap(unsigned int, unsigned int);
};

struct Class_004404c0 {
    char unknown_0[16];
    int stride;
    char unknown_14[4];
    unsigned int* base;
    void SetPassMapCell(int, int, int);
};

class Class_00440500 {
public:
    void* field_0;
    short footprintX;
    short footprintZ;
    short maxWaterDepth;
    short minWaterDepth;
    unsigned char maxSlope;
    unsigned char badSlope;
    unsigned char field_e;
    unsigned char field_f;
    unsigned int field_10;
    unsigned int field_14;
    unsigned int* field_18;
    unsigned int lastTick;
    void BuildPassMap(void);
};

class Class_00440830 {
public:
    int* field_0;
    short field_4;
    short field_6;
    short maxWaterDepth;
    short minWaterDepth;
    unsigned char maxSlope;
    unsigned char badSlope;
    unsigned char maxWaterSlope;
    unsigned char badWaterSlope;
    unsigned int width;
    unsigned int height;
    unsigned int* cells;
    unsigned int lastTick;
    void RefreshPassMap(Point16, Point16);
};

struct Dst_00440ca0 {
    char unknown_0[153];
    Rect rect;
};

struct Src_00440ca0 {
    int unknown_0;
    Rect rect;
};

struct Elem_00441c30 {
    V4i guid;
    unsigned long size;
    void* data;
};

struct Serial_00441c30 {
    char unknown_0[8];
    unsigned long unknown_8;
    unsigned long unknown_c;
    unsigned long unknown_10;
};

struct Gui_00443100 {
    char unknown_0[24];
};

struct Net_00443100 {
    void* dp;
    void* dp3;
    char unknown_8[1221];
    DirectPlayLobby_4ca880* lobby;
};

struct Class_004bbd00_Other {
    int unknown_0;
    int size;
    unsigned char compressed;
    char unknown_9[3];
    unsigned int d;
};

struct Msg_00441220 {
    char unknown_0[153];
    Class_004bbd00_Other group;
    char unknown_a9[19];
};

struct PlayerData_004437c0 {
    char unknown_0[128];
    char buf[27];
    unsigned short flags;
    char unknown_9d[64];
};

struct Player_004437c0 {
    char unknown_0[39];
    Ref<PlayerInfo> data;
    char unknown_2b[288];
};

struct Conn_00443cb0 {
    char unknown_0[34];
    unsigned char status;
    char unknown_23[296];
};

#include "../src/frontend/connection_blob.h"

#include "../src/frontend/connection_info.h"

struct Inner_00444910 {
    char unknown_0[4];
    int field_4;
};

struct Obj_00444910 {
    char unknown_0[24];
    Inner_00444910* field_18;
    char unknown_1c[68];
    int field_60;
};

struct Obj_444ba0 {
    char unknown_0[96];
    int field_60;
};

struct AnimSeq_00445110 {
    char unknown_0[40];
    void* field_28;
    char unknown_2c[4];
};

struct LogoEntry_00445110 {
    void* ptr;
    int unknown_4;
};

struct Head_004455b0 {
    unsigned char state;
    char unknown_1;
    char name[19];
    short field_15;
    char unknown_17[2];
    short field_19;
    int flags;
    char unknown_1f[10];
    unsigned char field_29;
    char unknown_2a[140];
    char text[136];
};

struct Mode_00446310 {
    int width;
    int height;
    int refreshRate;
    char unknown_c[308];
    short field_140;
};

struct ModeList_004b5330 {
    int count;
    Mode_00446310* modes;
    char unknown_8[12];
    char* buffer;
};

struct Player_004467c0 {
    char unknown_0[319];
    unsigned char alliance;
};

struct Message_00447150 {
    char unknown_0[5706];
};

struct Record_005129b4 {
    char name[82];
    int unitIndex;
    int previousMax;
    int max;
    int peerEnabled;
};

struct Inner_44b3c0 {
    Inner_44b3c0* unknown_0;
    void* gadgets;
};

struct Menu_44b3c0 {
    char unknown_0[24];
    Inner_44b3c0* inner;
    char unknown_1c[68];
    int current;
};

struct Record_0044c0d0 {
    unsigned short a;
    unsigned short b;
    unsigned short e;
    unsigned short f;
    unsigned char flag8;
    unsigned char flag9;
    unsigned char flaga;
    unsigned char flagb;
    int unknown_c;
    int d;
    int unknown_14;
};

struct Entry_0044c370 {
    char unknown_0[82];
    int unitType;
    char unknown_56[12];
};

class Class_0044ce20 {
public:
    void* vtable;
    Order* field_4;
    Class_0044ce20(int);
};

#include "../src/orders/path_fx.h"

class Class_0044e740 : public Class_0044ce20 {
public:
    unsigned short flag;
    Vec3 target;
    Vec3 other;
    short field_22;
    unsigned short heading;
    unsigned short value_26;
    Object_0044e880* self;
    Class_0044e740(int, HapiBank*, char*);
    int FUN_0044e880(int, HapiBank*, char*);
    int FUN_0044eb60(Object_0044e880*);
    void FUN_0044e930(BitWriter*);
    void FUN_0044ec10(int);
};

class Class_0044d010 : public Class_0044ce20 {
public:
    Vec3 v;
    Class_0044d010(int, HapiBank*, char*);
    int FUN_0044d090(int, HapiBank*, char*);
};

class Class_0044d930 : public Class_0044ce20 {
public:
    Rect r;
    Class_0044d930(int, HapiBank*, char*);
    int FUN_0044d9a0(int, HapiBank*, char*);
};

#include "../src/orders/order_fx.h"

class Class_0044ce70 {
public:
    void* vtable;
    Class_0044ce70(int, int, int);
};

class Class_0044ce90 {
public:
    char unknown_0[1];
    void FillGoalCells(std::vector<Elem_0044ce90*>*);
};

struct Elem_0044ce90 {
    int unknown_0;
};

class Class_0044cf00 {
public:
    virtual void FUN_00000000(void);
    virtual void FUN_00000001(void);
    virtual void FUN_00000002(void);
    virtual void FUN_00000003(void);
    virtual void ContainsUnitSlot(void);
    virtual void FUN_00000005(int, int);
    void FUN_0044cf00(int);
};

class Class_0044cff0 {
public:
    void* vtable;
    void* FUN_0044cff0(unsigned char);
};

struct Header_0044d010 {
    int magic;
    Vec3 v;
};

struct Header_0044d010_2 {
    int magic;
    Vec3 v;
};

#include "../src/orders/approach_areas.h"

class Class_0044d290 {
public:
    char unknown_0[8];
    short x;
    short y;
    char unknown_c[4];
    int radiusSq;
    int FUN_0044d290(int, int);
};

class Class_0044d2c0 {
public:
    char unknown_0[4];
    Owner_0044d2c0* owner;
    Point16 pos;
    int FUN_0044d2c0(int*);
};

struct Inner_0044d2c0 {
    char unknown_0[126];
    Point16 pos;
};

struct Owner_0044d2c0 {
    char unknown_0[14];
    Inner_0044d2c0* inner;
};

class Class_0044d310 {
public:
    char unknown_0[8];
    short x;
    short y;
    char unknown_c[4];
    int radiusSq;
    int FUN_0044d310(Obj_0044d310*);
};

struct Obj_0044d310 {
    char unknown_0[118];
    short x;
    short y;
};

class Class_0044d350 {
public:
    char unknown_0[8];
    short x;
    short y;
    int radius;
    int FUN_0044d350(int, int);
};

struct Class_0044d450 {
    void** vtable;
    char unknown_4[1184];
    Class_0044d450* FUN_0044d450(unsigned char);
};

struct Data_0044d470 {
    int a;
    int b;
    int c;
    int d;
    int e;
};

class Class_0044d470 : public Class_0044ce20 {
public:
    Data_0044d470 data;
    Class_0044d470(int, HapiBank*, char*);
    int FUN_0044d500(int, HapiBank*, char*);
};

struct Header_0044d470 {
    int magic;
    Data_0044d470 data;
};

struct Header_0044d470_2 {
    int magic;
    Data_0044d470 data;
};

class Class_0044d560 {
public:
    char unknown_0[8];
    Point16 pos;
    int field_c;
    int field_10;
    void FUN_0044d560(std::vector<Point16>*);
};

class Class_0044d720 {
public:
    char unknown_0[4];
    Owner_0044d720* owner;
    Point16 pos;
    int radius1;
    int radius2;
    int FUN_0044d720(Vec3*);
};

struct Inner_0044d720 {
    char unknown_0[106];
    Vec3 pos;
    char unknown_76[8];
    Point16 cell;
};

struct Owner_0044d720 {
    char unknown_0[14];
    Inner_0044d720* inner;
};

class Class_0044d7c0 {
public:
    char unknown_0[8];
    short field_8;
    short field_a;
    char unknown_c[8];
    int field_14;
    int field_18;
    bool FUN_0044d7c0(int, int);
};

class Class_0044d800 {
public:
    char unknown_0[8];
    short field_8;
    short field_a;
    char unknown_c[8];
    int field_14;
    int field_18;
    int FUN_0044d800(Unit*);
};

class Class_0044d910 {
public:
    virtual ~Class_0044d910(void);
    Class_0044d910* FUN_0044d910(unsigned char);
};

struct Header_0044d930 {
    int magic;
    Rect r;
};

struct Header_0044d930_2 {
    int magic;
    Rect r;
};

class Class_0044da00 {
public:
    char unknown_0[8];
    int x1;
    int x2;
    int y1;
    int y2;
    void FUN_0044da00(std::vector<Point16>*);
};

struct Pos16_44dc60 {
    short x;
    short z;
};

struct MapInfo_44dc60 {
    char unknown_0[126];
    Pos16_44dc60 pos;
};

class Class_0044dcb0 {
public:
    char unknown_0[8];
    int x1;
    int x2;
    int y1;
    int y2;
    int FUN_0044dcb0(int, int);
};

class Class_0044dd00 {
public:
    char unknown_0[8];
    int x1;
    int x2;
    int y1;
    int y2;
    int FUN_0044dd00(int, int);
};

class PathOrder {
public:
    char unknown_0[8];
    unsigned short flags;
    short field_a;
    short field_c;
    short field_e;
    short field_10;
    char unknown_12[8];
    Target_0044ddc0* ptr;
    char unknown_1e[8];
    int field_26;
    int field_2a;
    int field_2e;
    void FUN_0044ddc0(BitWriter*);
};

struct Target_0044ddc0 {
    char unknown_0[168];
    unsigned short field_a8;
};

struct Rec_0044de80 {
    int unknown_0;
    int unknown_4;
    short id1;
    void* ref_vt;
    void* ref_owner;
    void* ref_next;
    int ref_value;
    short id2;
    short f1;
    short f2;
    short f3;
    short f4;
    short f5;
    Vec3 pos;
    int i4;
};

class Class_0044df80 {
public:
    void** vtable;
    char unknown_4[20];
    void* FUN_0044df80(unsigned char);
};

class Class_0044dfb0 {
public:
    void* vtable;
    void* field_4;
    short f8;
    short fa;
    short fc;
    short fe;
    short f10;
    Unit_0044dfb0* unit1;
    void* ref_vt;
    Unit_0044dfb0* owner;
    void* ref_next;
    int ref_value;
    Vec3 pos;
    int i4;
    int FUN_0044dfb0(int, HapiBank*, char*);
};

struct Rec_0044dfb0 {
    int unknown_0;
    int unknown_4;
    short id1;
    void* ref_vt;
    void* ref_owner;
    void* ref_next;
    int ref_value;
    short id2;
    short f1;
    short f2;
    short f3;
    short f4;
    short f5;
    Vec3 pos;
    int i4;
};

struct Unit_0044dfb0 {
    char unknown_0[168];
    short id;
};

class Class_0044e3a0 {
public:
    char unknown_0[8];
    unsigned char field_8;
    char unknown_9[17];
    int field_1a;
    int FUN_0044e3a0(void);
};

class Class_0044e3c0 {
public:
    char unknown_0[8];
    unsigned short flags;
    short field_a;
    short field_c;
    short field_e;
    short field_10;
    Unit* unit;
    char unknown_16[4];
    Unit* target;
    char unknown_1e[8];
    Vec3 pos;
    int field_32;
    int FUN_0044e3c0(Vec3*);
};

class Class_0044e530 {
public:
    char unknown_0[8];
    unsigned short flags;
    char unknown_a[4];
    unsigned short heading;
    char unknown_10[2];
    Object_0044e530* self;
    char unknown_16[4];
    Object_0044e530* target;
    int FUN_0044e530(unsigned short*);
};

struct Object_0044e530 {
    char unknown_0[102];
    unsigned short heading;
    char unknown_68[2];
    Vec3 pos;
};

class Class_0044e5b0 {
public:
    char unknown_4[4];
    unsigned short flags;
    short field_a;
    char unknown_c[14];
    Object_0044e5b0* target;
    virtual void v0(void);
    virtual void v1(void);
    virtual void v2(void);
    virtual void v3(void);
    virtual void v4(void);
    virtual void v5(void);
    virtual void v6(void);
    virtual void v7(void);
    virtual void v8(Vec3*);
    int FUN_0044e5b0(Object_0044e5b0*);
};

struct Object_0044e5b0 {
    char unknown_0[102];
    unsigned short heading;
    char unknown_68[2];
    Vec3 pos;
};

class Class_0044e7b0 {
public:
    void* vtable;
    void* FUN_0044e7b0(int);
};

struct UnitFlag_0044e7d0 {
    short unit;
    short flag;
};

struct Header_0044e7d0 {
    char unknown_0[8];
    union { int id; UnitFlag_0044e7d0 id_flag; };
    Vec3 target;
    Vec3 other;
    short value_24;
    unsigned short heading;
    unsigned short value_28;
};

struct Header_0044e880 {
    char unknown_0[8];
    unsigned short unit_id;
    unsigned short flag;
    Vec3 target;
    Vec3 other;
    short value_24;
    unsigned short heading;
    unsigned short value_28;
};

struct Object_0044e880 {
    char unknown_0[106];
    Vec3 pos;
    char unknown_76[50];
    unsigned short id;
};

class Class_0044e9c0 {
public:
    void* vtable;
    int field_4;
    unsigned short flags;
    int field_a;
    int field_e;
    int field_12;
    int field_16;
    int field_1a;
    int field_1e;
    short field_22;
    unsigned short field_24;
    short pad_26;
    Owner_0044e9c0* owner;
    Class_0044e9c0(Class_0044e9c0&);
    Class_0044e9c0(Owner_0044e9c0*, BitReader*);
};

#include "../src/orders/air_maneuver_order.h"

#include "../src/orders/order_target_types.h"

class Class_0044eb40 {
public:
    char unknown_0[10];
    Vec3 target;
    char unknown_16[18];
    Object_0044eb40* self;
    int FUN_0044eb40(unsigned short*);
};

struct Object_0044eb40 {
    char unknown_0[106];
    Vec3 pos;
};

struct Class_0044ec20 {
    char unknown_0[8];
    unsigned short flag : 1;
    unsigned short unknown_rest : 15;
    char unknown_a[26];
    short value;
    void FUN_0044ec20(short);
};

#include "../src/ai/ai_search_goal.h"

class Class_0044f570 : public PathGoal {
public:
    char unknown_c[12];
    int field_18;
    Class_0044f570(int);
    virtual void FillWaypointWorldPos(int, int, int);
    virtual int HasReadyWaypoints(void);
    virtual void DeserializeNetUnitState(int);
};

class Class_0044ef80 {
public:
    char unknown_0[4];
    int field_4;
    int HasReadyWaypoints(void);
};

class Class_44ef90 {
public:
    char unknown_0[4];
    void* ptr_at_0x4;
    void SetPathOrder(void*);
};

class Class_0044f150 {
public:
    char unknown_0[12];
    Point16 points[20];
    int count;
    void FUN_0044f150(Vec3*, int, int);
};

class Class_0044f260 {
public:
    char unknown_0[96];
    unsigned int field_60;
    unsigned char field_64;
    char unknown_65[3];
    Class_0044f260* FUN_0044f260(void);
};

class Class_0044f290 {
public:
    char unknown_0[1];
    unsigned char FUN_0044f290(void);
};

class Base_00490a10_2 {
public:
    virtual ~Base_00490a10_2(void);
    virtual void SerializeSave(void);
    virtual void FUN_0044ce40(void);
    virtual void IsFxStyle(void);
    virtual int FUN_0044cf00(Struct_004907e0*);
    virtual int ContainsCell(int, int);
    virtual void FillGoalCells(void);
    virtual void ApproxDist(void);
    virtual int FUN_004e6110(Vec3*);
    virtual int TryGetDesiredHeading(void);
    virtual void FUN_0044cf50(void);
    virtual int KeepAfterComplete(void);
};

struct Class_0044f480 {
    char unknown_0[8];
    Class_0044f480_link* field_8;
    char unknown_c[88];
    unsigned char field_64;
    char unknown_65[3];
    int FUN_0044f480(void);
};

struct Class_0044f480_link {
    Class_0044f480_target* ptr;
};

struct Class_0044f480_target {
    char unknown_0[46];
    unsigned char field_2e;
};

class Class_0044f4a0 {
public:
    void* vtable;
    int field_4;
    Link_0044f4a0* owner;
    Point16 points[20];
    int count;
    int field_60;
    unsigned char active : 1;
    unsigned char flag_1 : 1;
    unsigned char flag_2 : 1;
    unsigned char flag_3 : 1;
    unsigned char : 4;
    char unknown_65[3];
    void FUN_0044f4a0(BitWriter*);
};

struct Link_0044f4a0 {
    Target_0044f4a0* target;
};

struct Target_0044f4a0 {
    char unknown_0[46];
    unsigned char field_2e;
};

class Class_0044f5b0 {
public:
    char unknown_0[24];
    int field_18;
    int FUN_0044f5b0(void);
};

#include "../src/orders/order_goals.h"

class Class_0044f650 {
public:
    char unknown_0[12];
    Point16 points[3];
    int count;
    void FUN_0044f650(Vec3*, int, int);
};

class Class_0044f8a0 {
public:
    char* buffer;
    int unknown_4;
    int unknown_8;
    int unknown_c;
    char* buffer_10;
    char* buffer_14;
    char* buffer_18;
    unsigned char unknown_1c;
    int unknown_1d;
    int unknown_21;
    Class_0044f8a0(void);
    ~Class_0044f8a0(void);
};

class NetCondenser {
public:
    char unknown_0[16];
    char* sendBuffer;
    char* unknown_14;
    char* packet;
    unsigned char flag_1c;
    int size_1d;
    int to_21;
    int SendPacket(void*, int);
};

class NetPacket {
public:
    char unknown_0[1];
    void DispatchPacket(void);
};

struct Player_0044feb0 {
    char unknown_0[326];
    unsigned char field_146;
};

struct Msg_00450530 {
    unsigned char type;
    unsigned char flag;
    int id;
    int arg;
};

struct Packet_00450a10 {
    unsigned char type;
    Data_0046d970 data;
};

struct Packet_00450f90 {
    unsigned char type;
    Data_0046d970 data;
};

struct Args_00451640 {
    char* name;
    int arg_4;
    int arg_8;
    int arg_c;
    int arg_10;
    int arg_14;
    int result;
};

struct Net2_4517b0 {
    char unknown_0[8];
    char* field_8;
    char* field_c;
};

struct Net_4517b0 {
    char unknown_0[4];
    unsigned int field_4;
    char* field_8;
    Net2_4517b0* field_c;
};

struct Packet_4517b0 {
    unsigned char type;
    Data_0046d970 data;
};

class Class_00451fd0 {
public:
    char unknown_0[2160];
    unsigned int field_870;
    char unknown_874[3793];
    int field_1745;
    int* field_1749;
};

struct Class_00452370 {
    char unknown_0[5961];
    int* buffer;
};

struct Entry_00452570 {
    int id;
    char unknown_4[31];
    Unit_00452570* unit;
    char name[64];
    char unknown_67[8];
    unsigned char valid;
    char unknown_70[219];
};

struct Unit_00452570 {
    char unknown_0[150];
    unsigned char field_96;
};

struct Packet_00452960 {
    unsigned char type;
    int from;
    int to;
    unsigned char value;
    int extra;
};

struct Packet_00452b70 {
    unsigned char type;
    int from;
    int to;
    char value;
    int extra;
};

#include "../src/network/entry_00463ca0.h"

struct Gui_00453a50 {
    char unknown_0[4];
    void* entries;
    void (__stdcall *callback)(void*);
    char unknown_c[16];
    void (__stdcall *field_1c)(void*);
};

class Class_00456030 {
public:
    int field_0;
    char unknown_4[111];
    char field_73;
    int IsPlayableSlot(void);
};

class Class_00461620 {
public:
    char unknown_0[1];
    void HandleIntegrityNop(int, int, int);
};

class Class_00463c40 {
public:
    char unknown_0[39];
    void* field_27;
    char unknown_2b[81];
    void* field_7c;
    void FreeSideDataAndFogSightCounts(void);
};

struct Packet {
    int field_0;
    int offset;
    int size;
    Packet* owner;
    int queued;
    int sentTime;
    Packet* prev;
    Packet* next;
    int AppendPacket(Packet*, int, void*, unsigned int, int);
};

struct Settings {
    int field_471;
    int flags_475;
    char unknown_8[72];
};

struct UnitCreatePacket {
    unsigned char type;
    short field_1;
    short field_3;
    Vec3 field_5;
    Rot16 field_11;
};

struct BuilderLinkPacket {
    unsigned char type;
    short field_1;
    short field_3;
};

struct UnitScriptCallPacket {
    unsigned char type;
    short id;
    short index;
    char field_5;
    int field_6;
    int field_a;
    int field_e;
    int field_12;
};

struct Packet_00456190 {
    unsigned char type;
    short id;
    short index;
    char field_5;
    int field_6;
    int field_a;
    int field_e;
    int field_12;
};

struct Packet_00456200 {
    unsigned char type;
    short id;
    short index;
    char field_5;
    int field_6;
    int field_a;
    int field_e;
    int field_12;
};

struct Packet_00456290 {
    unsigned char type;
    short id;
    short index;
    char field_5;
    int field_6;
    int field_a;
    int field_e;
    int field_12;
};

struct Msg13_00456310 {
    unsigned char type;
    int start_tick;
    int sent_tick;
    int id;
};

struct Msg20_00456310 {
    unsigned char type;
    Data_0046d970 info;
};

struct Message_004565a0 {
    char unknown_0[1];
    int start_tick;
    int sent_tick;
    int id;
};

struct Packet_00456ee0 {
    unsigned char type;
    int subtype;
    int from;
    int to;
    int extra;
};

struct Packet_00457050 {
    unsigned char type;
    int subtype;
    int from;
    int to;
    int value;
};

struct Packet_004571c0 {
    unsigned char type;
    int subtype;
    int from;
    int to;
    int zero;
};

struct Packet_004573d0 {
    unsigned char type;
    unsigned char flag;
    int field_2;
    int field_6;
    int field_a;
    int field_e;
    int field_12;
    int field_16;
    int field_1a;
    int field_1e;
    float field_22;
    float field_26;
    float field_2a;
    float field_2e;
    float field_32;
    float field_36;
};

struct Packet_00457540 {
    unsigned char type;
    unsigned char flag;
    short field_2;
    char unknown_4[2];
    short field_6;
    char unknown_8[2];
    short field_a;
    char unknown_c[2];
    short field_e;
    char unknown_10[2];
    int field_12;
    int field_16;
    int field_1a;
    int field_1e;
    float field_22;
    float field_26;
    float field_2a;
    float field_2e;
    float field_32;
    float field_36;
};

struct ShareLosAckPacket {
    unsigned char type;
    unsigned char flag;
    unsigned char flag2;
};

struct Net_4c9f90 {
    char unknown_0[1221];
    DirectPlay_4c9f90* dp;
};

struct FlagBits_00457d30 {
    unsigned short b0 : 1;
    unsigned short b1 : 1;
    unsigned short b2 : 1;
    unsigned short b3 : 1;
    unsigned short b4 : 1;
    unsigned short b5 : 1;
    unsigned short b6 : 1;
    unsigned short b7 : 1;
    unsigned short b8 : 1;
    unsigned short b9 : 1;
    unsigned short b10 : 1;
    unsigned short b11 : 1;
    unsigned short b12 : 1;
    unsigned short b13 : 1;
    unsigned short b14 : 1;
    unsigned short b15 : 1;
};

struct Packet_00457d30 {
    unsigned char type;
    int subtype;
    int from;
    int to;
    int zero;
};

struct Class_004581e0 {
    char unknown_0[16];
    Bitmap_00437b50* shadow;
    int BuildObjectPicture(SpotState*, int, int);
    void MeasureModel(int*, int*, int*, int*, SpotState*, Vec3*);
    void DrawPieces(Bitmap_00437b50*, SpotState*, int, int);
    void DrawLitPieces(Bitmap_00437b50*, SpotState*, int, int);
};

class Class_00458310 {
public:
    char unknown_0[1];
    void AddModelBounds(int*, int*, int*, int*, SpotState*, Pos_4589c0);
    void AddModelBounds(int*, int*, int*, int*, SpotState*, int, int, int);
};

class Class_00458430 {
public:
    char unknown_0[1];
    void DrawObjectPieces(int, SpotState*, Vec3, int);
};

class Class_004584d0 {
public:
    char unknown_0[1];
    void DrawPiece(SpotState*, void*, Vec3*, Object3do*, Vec3*, unsigned int, int);
};

class Class_00459200 {
public:
    char unknown_0[16];
    int bitmap;
    void MergeIntoComposite(Surface_004c0a90*, SpotState*);
    void DrawObjectPicture(int, SpotState*, Vec3_459200, int);
};

class Class_00458d30 {
public:
    char unknown_0[1];
    int ShadeByIntensity(Surface_004c0a90*, SpotState*);
    void RecolorByShade(Surface_004c0a90*, unsigned char, int, int, int);
};

struct Surface {
    int width;
    int height;
    int pitch;
    unsigned char* pixels;
    int field_10;
    int field_14;
    unsigned short x;
    unsigned short y;
    Rect clip;
    unsigned int flag0 : 1;
    unsigned int flag1 : 1;
    unsigned int : 30;
    Rect* GetClipRect(Rect*);
    void Unlock(void);
};

struct Surface_004c0a90 {
    unsigned short width;
    unsigned short height;
    short field_4;
    short field_6;
    unsigned char transparent;
    unsigned char flag9;
    unsigned char count;
    unsigned char kind;
    char unknown_c[4];
    unsigned char* pixels;
    unsigned char* shade;
};

class Class_00458fa0 {
public:
    char unknown_0[1];
    void DrawPieceEdges(Surface_004c0a90*, SpotState*, int);
};

struct Map_00458fa0 {
    char unknown_0[577];
    unsigned int unknown_241 : 30;
    unsigned int brightFaces : 1;
    unsigned int unknown_242 : 1;
};

class Class_00459170 {
public:
    char unknown_0[16];
    Surface_004c0a90* image;
    void CopyPicture(Surface_004c0a90*);
};

struct Class_0045a470 {
    char unknown_0[16];
    Surface_004c0a90* image;
    Surface_004c0a90* MakeSilhouette(Surface_004c0a90*);
};

struct Fixed_00485140 {
    unsigned short frac;
    short whole;
};

union UnitTypeFlags_459200 {
    union { unsigned int word; struct { unsigned int hi : 30; unsigned int b30 : 1; unsigned int lo : 1; } bits; };
};

#include "../src/graphics/model_types.h"
#include "../src/graphics/spot_state.h"

class Class_0045a510 {
public:
    char unknown_0[1];
    void MeasureShadow(int*, int*, int*, int*, SpotState*);
};

class Class_0045a610 {
public:
    char unknown_0[1];
    void DrawShadowShape(Bitmap_00437b50*, SpotState*);
};

struct Class_0045ae80 {
    int unknown_0;
    int unknown_4;
    int unknown_8;
    int unknown_c;
    int x;
    int y;
    int z;
    int unknown_1c;
    int unknown_20;
    void* points;
    int unknown_28;
    Class_0045ae80* unknown_2c;
    Class_0045ae80* unknown_30;
};

struct Entry_0045a950 {
    Class_0045ae80* object;
    char unknown_4[30];
    void* points;
    short unknown_26;
    unsigned short flags;
    Entry_0045a950* sibling;
    Entry_0045a950* child;
    Entry_0045a950* parent;
};

struct ObjectState_0045a8d0 {
    int count;
    char unknown_4[4];
    int field_8;
    void* field_c;
    int field_10;
    int field_14;
    char unknown_18[6];
    void* field_1e;
    Entry_0045a950 entries[1];
};

struct Entry_0045aaa0 {
    void* ptr;
    char unknown_4[50];
};

struct Vector3 {
    int x;
    int y;
    int z;
};

struct Video_0045bb00 {
    char unknown_0[310];
    short field_136;
    char unknown_138[4];
    int field_13c;
    short field_140;
    char unknown_142[8];
    ModeList_004b5330* field_14a;
};

#include "../src/sound/bits_0045c570.h"

#include "../src/sound/flags_0045c570.h"

class Class_004ce3e0 {
public:
    char unknown_0[512];
    unsigned int size;
    char unknown_204[17];
    char buf[1];
    void CopyTrackTypeTable(void*);
};

class Class_004ce7a0 {
public:
    char unknown_0[508];
    int field_1fc;
    int SetPlaybackOrder(int);
};

class Class_004ce5a0 {
public:
    char unknown_0[1];
    int GetLockedTrack(void);
};

struct Settings_00460160 {
    char block[83];
    unsigned short bit0 : 1;
    unsigned short bit1 : 1;
    unsigned short rest : 14;
};

struct Bits_0045d280 {
    unsigned short b0 : 1;
    unsigned short b1 : 1;
    unsigned short bits_2_15 : 14;
};

class Class_004ce8c0 {
public:
    char unknown_0[512];
    int count;
    char unknown_204[4];
    int current;
    int mode;
    int SelectTrack(int);
};

class Class_004cedc0 {
public:
    char unknown_0[512];
    int unknown_200;
    char unknown_204[4];
    int unknown_208;
    int unknown_20c;
    char unknown_210[108];
    int enabled;
    char unknown_280[4];
    int unknown_284;
    void EnableCdAudio(int);
};

union Flags_0045d280 {
    union { unsigned short word; Bits_0045d280 bits; unsigned char byte; };
};

#include "../src/ingame/flags_00495e90_37ebe.h"

struct Rule_0045f1d0 {
    char unknown_0[12];
    int startMetal;
    int startEnergy;
    char unknown_14[4];
};

struct Point_004948e0 {
    int x;
    int y;
};

struct Quad {
    Point_004948e0 p[4];
};

struct Sound_45ffb0 {
    char unknown_0[4];
    short start;
    short end;
};

struct Flags16_00493bf0 {
    char unknown_0[2];
};

struct Buffers_00462d30 {
    char* a;
    int field_4;
    int field_8;
    int field_c;
    char* b;
    Obj_00462d30* c;
    ~Buffers_00462d30(void);
};

class Class_00460f40 {
public:
    int field_0;
    int field_4;
    int field_8;
    Class_00460f40(void);
    void Init(void);
};

struct Channel_00460f60 {
    int field_0;
    unsigned int sendPacingTicks;
    void** items;
    unsigned int count;
    int field_10;
    int field_14;
    unsigned int timeoutTicks;
    int field_1c;
    int field_20;
    int field_24;
    void* field_28;
    int field_2c;
    int field_30;
    int field_34;
    Class_00460f40 field_38;
    char unknown_44[4096];
    Channel_00460f60(void);
    ~Channel_00460f60(void);
    void Init(void);
};

class Class_00462860 {
public:
    char unknown_0[24];
    unsigned int ticks;
    void SetMinRetainMs(unsigned int);
};

class Class_004628a0 {
public:
    char unknown_0[4];
    unsigned int field_4;
    void SetSendPacingMs(int);
};

struct Entry_00462d30_2 {
    int field_0;
    int field_4;
    int field_8;
    int field_c;
    int field_10;
    Buffers_00462d30 buffers;
    int field_2c;
    int field_30;
};

struct Buffer_461020 {
    char unknown_0[12];
    int field_c;
};

class Class_0044f940 {
public:
    char unknown_0[1];
    void Accumulate(void*, int);
    void SendPacketTo(void*, int, int, void*, int);
};

struct Buffer_00462d30 {
    int count;
    int used;
    int tail;
    char data[6144];
    Buffer_00462d30(void);
    Frame_00462f30* Pop(void);
    int Push(int, void*, int);
};

struct Class_00463730 {
    int field_0;
    int field_4;
    int field_8;
    int field_c;
    Buffer_00462d30* buffer;
    int field_14;
    int field_18;
    Class_00463730(void);
    int Count(void);
    int ResetFrames(void);
    int QueueFrames(char*, unsigned int, int, int, int, int);
    void Init(void);
    Frame_00462f30* Peek(void);
    void* Take(Frame_00462f30*, int, int&);
    void* GetFrame(int, int&);
};

struct Entry_00462d30 {
    int field_0;
    int field_4;
    int field_8;
    int field_c;
    int field_10;
    int field_14;
    Class_00463730 tail;
    Entry_00462d30(void);
    void Init(long);
    void Reset(void);
};

class Class_00462d30 {
public:
    int field_4;
    void* owner;
    int field_c;
    int field_10;
    int field_14;
    void* field_18;
    void* field_1c;
    Entry_00462d30 entries[10];
    int field_228;
    int field_22c;
    int field_230;
    int field_234;
    int field_238;
    virtual ~Class_00462d30(void);
    Class_00462d30(void);
    Class_00462d30(void*);
    Entry_00462d30* FindPlayerFrameInfo(long);
};

struct F0_00462d30 {
    int field_0;
    F0_00462d30(void);
};

struct Mid_00462d30 {
    int field_18;
    int field_1c;
    int field_20;
    int field_24;
    void Init(void);
};

struct Table_004611e0 {
    Channel_00460f60 channels[11];
    char* buffer;
    int used;
    int capacity;
    Table_004611e0(void);
};

struct Entry_00462d30_3 {
    int field_0;
    int field_4;
    int field_8;
    int field_c;
    int field_10;
    Buffers_00462d30 buffers;
    char unknown_2c[8];
};

class Class_00462d30_3 {
public:
    int field_4;
    int field_8;
    int field_c;
    int field_10;
    int field_14;
    void* field_18;
    void* field_1c;
    Entry_00462d30_3 entries[10];
    char unknown_228[36];
    virtual ~Class_00462d30_3(void);
};

class Class_00460f60_3 {
public:
    int field_4;
    int field_8;
    int field_c;
    Entry entries[11];
    int field_b2fc;
    Class_00462d30_3 member;
    virtual ~Class_00460f60_3(void);
};

struct Entry_00462d30_4 {
    int field_0;
    int field_4;
    int field_8;
    int field_c;
    int field_10;
    Buffers_00462d30 buffers;
    int field_2c;
    int field_30;
};

class Class_00462d30_4 {
public:
    int field_4;
    int field_8;
    int field_c;
    int field_10;
    int field_14;
    void* field_18;
    void* field_1c;
    Entry_00462d30_4 entries[10];
    virtual ~Class_00462d30_4(void);
    Class_00462d30_4(Class_00462d30_4&);
    Class_00462d30_4(void);
};

struct Class_004615f0 {
    char unknown_0[45812];
    int field_b2f4;
    int field_b2f8;
    void ClearSendBuffer(void);
};

struct Entry_004618a0 {
    char unknown_0[20];
    int field_14;
    char unknown_18[4140];
};

class Class_00461900 {
public:
    char unknown_0[45812];
    int* buffer;
    int size;
    int SendFrameState(int, Msg_00461900*);
};

struct Channel_4619e0 {
    unsigned long pacing;
    char unknown_4[4160];
};

class Class_00461a70 {
public:
    int field_0;
    int field_4;
    void** blocks;
    unsigned int count;
    int field_10;
    int field_14;
    int field_18;
    int field_1c;
    int field_20;
    int field_24;
    void* field_28;
    int field_2c;
    int field_30;
    int field_34;
    int field_38;
    int field_3c;
    int field_40;
    Class_00461a70(void);
    ~Class_00461a70(void);
};

class Class_00461b10 {
public:
    int head;
    char unknown_4[4];
    Class_004629b0** array;
    unsigned int count;
    char unknown_10[8];
    int minRetain;
    Class_004629b0* AllocBuffer(void);
};

struct Class_00461d80 {
    char unknown_0[24];
    int field_18;
    int GetMinRetainMs(void);
};

struct Class_00461da0 {
    char unknown_0[40];
    int field_28;
    int GetPacketEntry(int);
};

struct Entry_00461db0 {
    int field_0;
    int field_4;
    int field_8;
    int field_c;
    int field_10;
    int field_14;
    int field_18;
    int field_1c;
};

class Packet_00461db0 {
public:
    char unknown_0[1088];
};

class Queue_00461f90 {
public:
    int count;
    int readIdx;
    int writeIdx;
    Packet* buf[1024];
    Packet* Peek(void);
    int Push(int);
    int Pop(void);
};

class Class_004624a0 {
public:
    int field_00;
    unsigned int ticks;
    void** field_08;
    char unknown_c[4];
    int frame;
    int dpid;
    char unknown_18[8];
    unsigned int queuedBytes;
    unsigned int nextSend;
    char unknown_28[16];
    Queue_00461f90 queue;
    int SendQueued(int);
};

class Class_00462710 {
public:
    int field_00;
    char unknown_4[4];
    void** field_08;
    char unknown_c[12];
    unsigned int minRetain;
    int index;
    unsigned int field_20;
    char unknown_24[4];
    Packet* packets;
    unsigned int poolSize;
    Packet* field_30;
    Packet* field_34;
    Queue_00461f90 queue;
    Packet* AllocPacket(int);
    int AddPacket(int, void*, unsigned int);
};

class Class_00461fd0 {
public:
    int field_0;
    int field_4;
    Packet** packets;
    unsigned int field_c;
    char unknown_10[4];
    int field_14;
    char unknown_18[4];
    int field_1c;
    int field_20;
    char unknown_24[4];
    Packet* entries;
    unsigned int count;
    Packet* field_30;
    Packet* field_34;
    Queue_00461f90 queue;
    int GrowPools(int, int);
    void DequeuePacket(Packet*);
    void EnqueuePacket(Packet*);
};

class Class_00461f90 {
public:
    char unknown_0[32];
    int total;
    char unknown_24[20];
    Queue_00461f90 queue;
    void EnqueuePacket(Packet*);
};

class Class_00462370 {
public:
    int count;
    char unknown_4[4];
    int writeIdx;
    int buf[1024];
    int PushPacket(int);
};

class Class_00462470 {
public:
    int field_0;
    unsigned long m_pacingMs;
    Packet_00461db0** packets;
    unsigned int field_c;
    char unknown_10[4];
    int field_14;
    char unknown_18[4];
    int field_1c;
    int total;
    int field_24;
    Entry_00461db0* packets_28;
    unsigned int size;
    int field_30;
    int field_34;
    PacketRing queue;
    int InitPools(int, unsigned int, int, int);
    void ResetChannel(void);
};

class Class_00461630 {
public:
    char unknown_0[4];
    int field_4;
    Class_00462470 entries[11];
    Class_00462470* FindChannel(int, int);
};

class Class_00461750 {
public:
    char unknown_0[4];
    int field_4;
    Class_00462470 entries[11];
    char unknown_b2f4[32];
    int field_b314;
    int field_b318;
    int field_b31c;
    char unknown_b320[520];
    int field_b528;
    int field_b52c;
    int InitChannels(int, int);
};

class Class_00461860 {
public:
    char unknown_0[8];
    Class_00462470 entries[11];
    void ReleaseAllChannels(void);
};

class Class_004623e0 {
public:
    char unknown_0[32];
    int total;
    char unknown_24[20];
    Queue_00461f90 queue;
    void DequeuePacket(Packet*);
};

struct Class_00462a40 {
    char unknown_0[8];
    Entry_2* field_8;
    char unknown_c[4];
    Entry_2* field_10;
    void FindOwnerChainTail(void);
};

struct Entry_2 {
    char unknown_0[12];
    void* owner;
    char unknown_10[12];
    Entry_2* next;
};

class Class_00462bd0 {
public:
    char unknown_0[12];
    void* field_c;
    void RemoveFromBuffer(void);
};

struct Class_00462bf0 {
    char unknown_0[4];
    int field_0x4;
    char unknown_8[4];
    int field_0xc;
    int GetData(void);
};

struct F0_00462d30_2 {
    int field_0;
    F0_00462d30_2(void);
};

struct Link_00462ed0 {
    char unknown_0[12];
    int field_c;
};

class Class_0044f9c0_2 {
public:
    char unknown_0[1];
    int ReceivePacket(void*, char*, int*);
};

class Class_00462f30_2 {
public:
    void* vtable;
    int field_4;
    void* owner;
    int field_c;
    int field_10;
    Entry_00462d30* field_14;
    char* buffer;
    char* spare;
    Entry_00462d30 entries[10];
    int capacity;
    int length;
    int field_230;
    int field_234;
    int field_238;
    int ReceiveFrame(void*, unsigned char*, int*);
};

struct Frame_00462f30 {
    int tick;
    void* data;
    int size;
};

struct Buffer_004635b0 {
    int count;
    int used;
    int current;
    char data[6144];
    Buffer_004635b0(void);
};

struct Head_004635b0 {
    int field_0;
    int field_4;
    int field_8;
    int field_c;
    int field_10;
    int field_14;
    Head_004635b0(void);
    void Init(long);
};

struct Tail_004635b0 {
    int field_0;
    int field_4;
    int field_8;
    int field_c;
    Buffer_004635b0* buffer;
    int field_14;
    int field_18;
    Tail_004635b0(void);
    void Init(void);
};

class Class_00463680 {
public:
    char unknown_0[20];
    void* field_14;
    char unknown_18[12];
    void* field_24;
    void* field_28;
    ~Class_00463680(void);
};

struct Buffer_004636b0 {
    int count;
    int used;
    int current;
    char data[6144];
    Buffer_004636b0(void);
};

class Class_004636b0 {
public:
    int field_0;
    int field_4;
    int field_8;
    void* field_c;
    Buffer_004636b0* buffer;
    int field_14;
    int field_18;
    Class_004636b0(void);
    ~Class_004636b0(void);
};

struct Entry_00463ef0 {
    unsigned int time;
    char unknown_4[68];
};

struct Entry_3 {
    char unknown_0[30];
    unsigned char flags;
    char unknown_1f[41];
};

struct Ctx_00499890 {
    char unknown_0[64];
    void* hwnd;
    char unknown_44[136];
    unsigned char* colorMap;
    char unknown_d0[33];
    unsigned char flags;
    char unknown_f2[1326];
    int field_620;
};

struct Class_00464ac0 {
    char unknown_0[148];
    float field_0x94;
};

struct Range_00464da0 {
    char unknown_0[103];
    Unit* first;
    Unit* last;
};

struct Class_00490360 {
    MissionCondition* first[16];
    int firstCount;
    MissionCondition* second[16];
    int secondCount;
    int field_88;
    int CheckDefeat(void);
};

struct Dialog_00464f80 {
    char unknown_0[4];
    Widget_00464f80* field_4;
    void* field_8;
};

struct Widget_00464f80 {
    char unknown_0[204];
    char field_cc[16];
    char field_dc[32];
};

struct Map_004658e0 {
    char unknown_0[124];
    ByteMap_00408090 explored;
};

struct Pos_004658e0 {
    short xFrac;
    short x;
    short yFrac;
    short y;
    short zFrac;
    short z;
};

struct Pos_00465ac0 {
    int x;
    int y;
    int z;
};

struct Pic_4665d0 {
    unsigned short w;
    unsigned short h;
    unsigned short x;
    unsigned short y;
};

struct IconSet_00466780 {
    int count;
    unsigned char* data;
};

union FlagWord {
    union { unsigned short all; struct { unsigned short bit0 : 1; unsigned short bit1 : 1; unsigned short bit9 : 1; } bits; };
};

union Flags142f0_00466dc0 {
    union { unsigned char bytes[2]; struct { unsigned char lo; unsigned char hi; } b; struct { unsigned short bit0 : 1; unsigned short bit1 : 1; } bits; };
};

struct PlayerInfo_00466dc0 {
    char unknown_0[39];
    Ref<PlayerInfo> data;
    char unknown_2b[81];
    ByteMap_00408090 explored;
    char unknown_88[195];
};

struct Player_00466dc0 {
    char unknown_0[150];
    unsigned char field_96;
};

class DetectionVisitor {
public:
    int field_4;
    int field_8;
    Vec3 pos;
    virtual void MarkUnitsInRadarOrSonarRadius(Unit*);
};

struct Unit_00467980 {
    char unknown_0[272];
    unsigned int flags;
};

#include "../src/ingame/sub_004679a0_a.h"

#include "../src/ingame/sub_004679a0_b.h"

struct PlayerData_467c00 {
    char unknown_0[150];
    unsigned char field_96;
};

struct Player_467c00 {
    char unknown_0[39];
    PlayerData_467c00* data;
};

struct Player_00467cb0 {
    char unknown_0[184];
    unsigned short kills;
};

struct MissionName_00467e50 {
    char unknown_0[21];
    char* name;
};

struct Mission_00467e50 {
    char unknown_0[5];
    unsigned char state;
    char unknown_6[16];
    void* target;
    char unknown_1a[48];
    void* next;
};

struct Player_004689c0 {
    char unknown_0[281];
    unsigned short field_119;
    char unknown_11b[48];
};

struct FrameTimers {
    int last;
    int total;
    int values[9];
    int acc[9];
    void AccumulateProfileTime(int);
};

#include "../src/game/game.h"

struct Class_004c6b10 {
    int data[12];
    void SetClipRect(Rect);
};

#include "../src/ingame/map_grid.h"

#include "../src/ingame/snapshot_0046a860.h"

struct Point_0046ba80 {
    int x;
    int y;
};

class Prim_0046bae0 {
public:
    int field_0;
    int count;
    int field_8;
    unsigned short* indices;
    int field_10;
    char unknown_14[8];
    unsigned int flag0 : 1;
    unsigned int texIndexed : 1;
    unsigned int rest : 30;
};

struct ElemA_0046c190 {
    char unknown_0[20];
    void* field_14;
};

struct ElemB_0046c190 {
    char unknown_0[8];
    void* ppScores;
};

struct Name_0046c620 {
    char text[16];
    char flag;
};

struct Class_0046c620 {
    Name_0046c620 name;
    char unknown_11[1211];
    int field_4c9;
    char unknown_4d0[4];
};

class VecInt_0046ca60 {
public:
    std::allocator<int> alloc;
    char unknown_1[3];
    int* _First;
    int* _Last;
    int* _End;
    ~VecInt_0046ca60(void);
};

struct Class_00470560 {
    int field_0;
    int field_4;
    int field_8;
    std::vector<Elem_0046faf0> list_c;
    std::vector<Elem_0046faf0> list_d;
    Class_00470560(Class_00470560&);
    Class_00470560(void);
    Class_00470560& operator=(Class_00470560&);
};

class Class_0046cc10 {
public:
    int field_0;
    char unknown_4[8];
    std::vector<Elem_0046faf0> vec;
    void SendSequenced(Elem_0046faf0*, Elem_0046faf0*);
};

class Class_0046eba0 {
public:
    char unknown_0[4];
    Packet_0046cef0* first;
    Packet_0046cef0* last;
    Packet_0046cef0* end;
    void InsertPacket(Packet_0046cef0*, int, Packet_0046cef0*);
};

class Class_0046cef0 {
public:
    char unknown_0[4];
    unsigned int cur;
    unsigned int max;
    Class_0046eba0 sent;
    Class_0046eba0 queue;
    void ReceiveSequenced(Packet_0046cef0*, int, void*, unsigned int);
};

struct Packet_0046cef0 {
    unsigned char type;
    unsigned char arg;
    unsigned int id;
    int field_6;
    int field_a;
};

class Class_0046f720 {
public:
    char unknown_0[4];
    void* head;
    char unknown_8[4];
    int size;
    Class_0046f720(Cmp_0046d040, Alloc_0046d040);
    LiveNode* Buynode(LiveNode*, int);
    void FUN_0046f720(void);
    Rect& operator[](unsigned int);
};

class ListWrap_0046d1a0 {
public:
    char unknown_0[12];
};

class Vec_0046d1a0 {
public:
    std::allocator<int> alloc;
    char unknown_1[3];
    int* _First;
    int* _Last;
    int* _End;
    ~Vec_0046d1a0(void);
    void _Destroy(int*, int*);
};

class Class_0046e880 {
public:
    char unknown_0[4];
    int* field_4;
    int* FUN_0046e880(int*);
};

class InsertResult {
public:
    Node_0046fad0* first;
    unsigned char second;
    char unknown_5[3];
    InsertResult(Node_0046fad0*&, bool&);
};

struct Less_0046d2e0 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

struct LiveNode {
    LiveNode* left;
    LiveNode* parent;
    LiveNode* right;
    unsigned int key;
    int value;
    int color;
    char unknown_18[4];
    int ptr_1c;
    int color_20;
    int* table;
    Elem_004cb4c0* elems;
    LiveNode* next;
    LiveNode* child;
    char unknown_34[8];
    int color_3c;
    char unknown_40[452];
    int color_204;
};

class Class_0046d4c0 {
public:
    char unknown_0[88];
    int direct;
    char unknown_5c[8];
    int disabled;
    void SendSyncPacket(unsigned int*, Packet_0046d860*, int);
    void SendSyncMessageTo(Target_0046d5b0*, unsigned char, int, int, int);
    void Send(Target_0046d530*, void*);
    void SendSyncMessage(unsigned char, int, int, int);
};

struct Packet_0046d860 {
    unsigned char type;
    unsigned char arg;
    int field_2;
    int field_6;
    unsigned char field_a;
    unsigned char field_b;
    short field_c;
};

struct Packet_0046d530 {
    unsigned char type;
    unsigned char arg;
    int field_2;
    int field_6;
    int field_a;
};

struct Target_0046d530 {
    unsigned int id;
};

struct Packet_0046d5b0 {
    unsigned char type;
    unsigned char arg;
    int field_2;
    int field_6;
    int field_a;
};

struct Target_0046d5b0 {
    unsigned int id;
    char unknown_4[36];
    int sent;
};

class Class_0046d630 {
public:
    char unknown_0[88];
    int direct;
    char unknown_5c[8];
    int disabled;
    void SendEntryTo(Target_0046d630*, unsigned char, Source_0046d630*, int);
};

struct Packet_0046d630 {
    unsigned char type;
    unsigned char arg;
    int field_2;
    int field_6;
    unsigned char field_a;
    unsigned char field_b;
    short field_c;
};

struct Source_0046d630 {
    int field_0;
    char unknown_4[4];
    unsigned char field_8;
    char unknown_9;
    unsigned char field_a;
    char unknown_b;
    short field_c;
    char unknown_e[2];
};

struct Target_0046d630 {
    unsigned int id;
    char unknown_4[36];
    int sent;
};

struct Field_0046d6c0 {
    union { int all; struct { unsigned char lo; unsigned char hi; short top; } part; };
};

class Map_0046d6c0 {
public:
    char unknown_0[16];
    Rect& operator[](unsigned int&);
};

struct Packet_0046d6c0 {
    unsigned char type;
    unsigned char arg;
    int field_2;
    int field_6;
    Field_0046d6c0 field_a;
};

class Vec_0046d6c0 {
public:
    int pad;
    int* first;
    int* last;
    int* cap;
    int* begin(void);
    int* end(void);
    void insert(int*, int, int&);
};

struct Entry_0046d6c0 {
    int id;
    Vec_0046d6c0 ids;
    Vec_0046d6c0 pairs;
    int field_24;
    char unknown_28[4];
    unsigned int field_2c;
    char unknown_30[44];
};

class Class_0046d6c0 {
public:
    Map_0046d6c0 map;
    std::vector<Entry_0046d6c0> players;
    char unknown_20[56];
    int direct;
    int field_5c;
    char unknown_60[4];
    int disabled;
    void HandleSyncPacket(Packet_0046d6c0*, unsigned char);
};

class Iter_0046e330 {
public:
    LiveNode* ptr;
    Iter_0046e330(LiveNode*);
    Iter_0046e330(void);
    Iter_0046e330 operator++(int);
    Iter_0046e330& operator++(void);
    unsigned char operator==(Iter_0046e330&);
    Class_004d8820& operator*(void);
    LiveNode* _Mynode(void);
    Class_004d8820* operator->(void);
    unsigned char operator!=(Iter_0046e330&);
    void _Inc(void);
};

struct Ids_0046d970 {
    int* begin;
    int* end;
    int* capacity;
};

struct Less_0046d970 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class Class_0046e9b0 {
public:
    Less_0046d970 compare;
    char unknown_1[3];
    LiveNode* head;
    Iter_0046e330 End(void);
    Iter_0046e330 FindExact(unsigned int&);
};

struct Alloc_0046e5c0 {
    char unknown_0[1];
};

struct SyncTaggedVector {
    char field_0x0;
    char unknown_1[3];
    int field_0x4;
    int field_0x8;
    int field_0xc;
    SyncTaggedVector* InitTaggedVector(char*);
};

struct Data_0046dad0 {
    char unknown_0[148];
    unsigned char field_94;
};

struct PlayerInfo_0046dad0 {
    int field_0;
    int field_4;
    char unknown_8[31];
    Data_0046dad0* data;
    char unknown_2b[72];
    unsigned char type;
};

class Vec_0046d860 {
public:
    char unknown_0[16];
    void InsertPlayerRecord(SyncPlayerRecord*, unsigned int, SyncPlayerRecord&);
    void Push(SyncPlayerRecord&);
};

struct Entry_0046e000 {
    int id;
    char unknown_4[88];
};

struct Sub_0046e000 {
    Ids_0046d970 ids;
    char unknown_c[16];
    int count;
    int field_20;
    int field_24;
    char unknown_28[52];
};

struct Less_0046e160 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class Class_0046e160 {
public:
    char allocator;
    Less_0046e160 compare;
    char unknown_2[2];
    LiveNode* head;
    char unknown_8[92];
    int field_64;
    Iter_0046e330 End(void);
    void ApplyToUnitTypes(void);
    Iter_0046e330 Find(unsigned int*);
};

struct Less_0046e280 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class Map_0046e280 {
public:
    Less_0046e280 compare;
    char unknown_1[3];
    LiveNode* head;
    Iter_0046e330 End(void);
    Iter_0046e330 Find(unsigned int*);
};

struct Less_0046e330 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

struct Less_0046e3c0 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class Class_0046e3c0 {
public:
    Less_0046e3c0 compare;
    char unknown_1[3];
    LiveNode* head;
    Iter_0046e330 End(void);
    Iter_0046e330 Find(unsigned int*);
    int ToggleUnitAllowed(Unit_0046e3c0*);
};

struct Unit_0046e3c0 {
    char unknown_0[318];
    unsigned int key;
};

struct Less_0046e450 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

struct Class_0046e450 {
    Less_0046e450 compare;
    char unknown_1[3];
    LiveNode* head;
    Iter_0046e330 End(void);
    int DisallowUnit(UnitDef*);
    Iter_0046e330 Find(unsigned int*);
};

struct Less_0046e4d0 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

struct Class_0046e4d0 {
    Less_0046e4d0 compare;
    char unknown_1[3];
    LiveNode* head;
    Iter_0046e330 End(void);
    int AllowUnit(UnitDef*);
    Iter_0046e330 Find(unsigned int*);
};

struct Less_0046e330_2 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class Class_0046e330 {
public:
    Less_0046e330_2 compare;
    char unknown_1[3];
    LiveNode* head;
    Iter_0046e330 End(void);
    void SetUnitLimit(UnitDef*, int);
    Iter_0046e330 Find(unsigned int*);
};

struct SyncPlayerRecord {
    int field_0;
    SyncTempTaggedVector list_a;
    SyncTempTaggedVector list_b;
    int field_24;
    int field_28;
    int field_2c;
    Class_00470560 sub;
    SyncPlayerRecord& operator=(SyncPlayerRecord&);
};

class Class_0046d860 {
public:
    std::map<unsigned int, Rect> map;
    std::vector<SyncPlayerRecord> players;
    std::list<unsigned int> queue;
    char unknown_2c[44];
    int direct;
    int field_5c;
    int field_60;
    int disabled;
    void NotifyEntryChanged(unsigned int);
    void ProcessSync(void);
    void CheckUnitAvailable(unsigned int, int);
};

class Class_0046d040 {
public:
    std::map<unsigned int, Rect> rects;
    std::vector<UnitSyncPlayer> players;
    std::list<int> ids;
    int field_2c;
    int field_30;
    int field_34;
    std::vector<Elem_0046faf0> field_38;
    std::vector<int> field_48;
    int field_58;
    int field_5c;
    int field_60;
    int field_64;
    Class_0046d040(int);
    ~Class_0046d040(void);
    Iter_0046e330 End(void);
    char* GetSyncStatusText(void);
    int IsPlayerSynced(int);
    int PopChangedEntry(Event_0046d860*);
    int GetUnitEntry(UnitDef*, Rect*);
    void ResetEntries(void);
    void ReceiveSyncPacket(void*, int);
    Iter_0046e330 Find(unsigned int*);
};

class VecElems_0046ca60 {
public:
    std::allocator<UnitSyncPlayer> alloc;
    char unknown_1[3];
    UnitSyncPlayer* _First;
    UnitSyncPlayer* _Last;
    UnitSyncPlayer* _End;
    ~VecElems_0046ca60(void);
    void _Destroy(UnitSyncPlayer*, UnitSyncPlayer*);
};

struct Elem_004702a0_2 {
    int unknown_0;
};

struct Packet_0046cef0_2 {
    unsigned char type;
    unsigned char arg;
    unsigned int id;
    int field_6;
    int field_a;
};

struct Less_0046fe60 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class Class_0046fe60 {
public:
    char allocator;
    Less_0046fe60 key_compare;
    char unknown_2[2];
    LiveNode* head;
    LiveNode* FUN_0046fe60(unsigned int*);
};

class Class_00470250 {
public:
    char unknown_0[4];
    int first;
    char unknown_8[4];
    int last;
    int Capacity(void);
    unsigned int count(void);
};

struct Pair_0046faf0 {
    std::vector<Elem_0046faf0> list_c;
    std::vector<Elem_0046faf0> list_d;
    Pair_0046faf0(Pair_0046faf0&);
};

class ObjectPool {
public:
    std::vector<Item_00470ae0*> items;
    void* slots;
    int slotSize;
    int capacity;
    int used;
    virtual ~ObjectPool(void);
    ObjectPool(ObjectPool&);
    ObjectPool(int, int);
};

struct Item_00470ae0 {
    int unknown_0;
};

struct Elem_00470f00 {
    int unknown_0;
};

class Class_00470c10 {
public:
    int unknown_0;
    std::vector<Elem_00470f00> items;
    Elem_00470f00** field_14;
    int field_18;
    int field_1c;
    int field_20;
    int FUN_00470c10(int, int);
};

class Class_00470e50 {
public:
    int unknown_0;
    std::vector<int*> items;
    int* extra;
    void FreeBlocks(void);
};

class Class_00470eb0 {
public:
    char unknown_0[20];
    void* field_14;
    char unknown_18[4];
    int field_1c;
    int field_20;
    int AllocSlot(int);
};

class ParticleLists {
public:
    std::vector<Listener_00470fb0*> lists[10];
    ParticleLists(ParticleLists&);
    ~ParticleLists(void);
    ParticleLists(void);
    void UpdateAll(void);
    void DrawAll(void*);
};

class Listener_00470fb0 {
public:
    virtual ~Listener_00470fb0(void);
    virtual void Update(void);
    virtual void Notify(void*);
    virtual int IsDone(void);
};

class Listener_00471120 {
public:
    virtual void unk0(void);
    virtual void unk1(void);
    virtual void Notify(void*);
};

struct Slot_00471120 {
    char unknown_0[4];
    Listener_00471120** first;
    Listener_00471120** last;
    char unknown_c[4];
};

class SmokeParticles {
public:
    char unknown_4[4];
    int time;
    std::vector<Record_00474cd0> records;
    int emitPeriod;
    int holdPeriod;
    int maxFrame;
    void* altAnimation;
    Vec3 pos;
    virtual ~SmokeParticles(void);
    SmokeParticles(SmokeParticles&);
    SmokeParticles(void);
    virtual void Notify(void*);
    virtual void FUN_00472d50(void);
    virtual void Render(int);
    virtual int IsFinished(void);
    virtual void FUN_00474df0(void);
    virtual int FUN_00475440(void);
    virtual void FUN_00474d50(Vec3*, int, int, int, int, int);
};

struct Elem_00473500 {
    int unknown_0;
};

class Class_00471120 {
public:
    std::vector<Elem_00473500> lists[1];
    void DrawList(void*, short);
    void AddToList(Elem_00473500, short);
};

class Class_00470ed0 {
public:
    char unknown_0[4];
    void FreeSlot(void*);
};

class Class_00471340 {
public:
    std::vector<ParticleSystem*> lists[1];
    void Add(short, ParticleSystem*);
    void AddTeleportParticles(int, int, int, short);
};

class Class_00473590 {
public:
    void* data;
    Vec3 pos1;
    Vec3 pos2;
    Vec3 vel;
    int frameCount;
    int frame;
    int endTime;
    void FUN_00473560(void);
    void FUN_00473590(void*, short, short);
};

class TeleportParticles {
public:
    int deadline;
    int time;
    std::vector<Class_00473590> items;
    int sparkLifetime;
    Vec3 pos1;
    Vec3 pos2;
    Vec3 dir;
    TeleportParticles(TeleportParticles&);
    TeleportParticles(void);
    virtual void FUN_00471430(void);
    virtual void FUN_00472d50(void);
    virtual void Render(int);
    virtual int IsFinished(void);
    virtual void FUN_004737c0(void);
    virtual int FUN_00472e00(void);
    virtual void FUN_004736e0(int, int, int);
};

class Class_00471470 {
public:
    std::vector<ParticleSystem*> lists[1];
    void Add(short, ParticleSystem*);
    void AddNanoParticles(int, int, int, short);
};

class Elem_00475880 {
public:
    int dwords[12];
    void FUN_004739b0(void);
    void FUN_00473a00(int, short, short);
};

class NanoParticles {
public:
    char unknown_0[4];
    int deadline;
    int time;
    std::vector<Elem_00475880> items;
    Vec3 center;
    Vec3 radius;
    Vec3 target;
    Vec3 spread;
    NanoParticles(NanoParticles&);
    NanoParticles(void);
    void FUN_00472d50(void);
    void Render(int);
    int IsFinished(void);
    void FUN_00473d50(void);
    int FUN_00472f60(void);
    void FUN_00473b50(int, int, int);
};

class Class_004715a0 {
public:
    std::vector<ParticleSystem*> lists[1];
    void Add(short, ParticleSystem*);
    void AddThrustParticles(int, int, int, int, short);
};

class Class_004716e0 {
public:
    std::vector<ParticleSystem*> lists[1];
    void Add(short, ParticleSystem*);
    void AddWakeParticles(int, int, int, int, short, int);
};

class Class_004745e0 {
public:
    void* data;
    Vec3 pos;
    Vec3 pos2;
    Vec3 vel;
    int min;
    int max;
    int value;
    int step;
    int tick;
    int period;
    int endTime;
    void FUN_004745e0(void*, short, short);
};

class WakeParticles {
public:
    char unknown_0[8];
    int time;
    std::vector<Class_004745e0> items;
    int period;
    Vec3 pos_a;
    Vec3 pos_b;
    Vec3 dir;
    int ascending;
    WakeParticles(WakeParticles&);
    WakeParticles(void);
    void FUN_00472d50(void);
    void Render(int);
    int IsFinished(void);
    void FUN_00474880(void);
    int FUN_00473220(void);
    void FUN_00474760(int, int, int, int, int);
};

class Class_00471820 {
public:
    std::vector<Elem_00473500> lists[1];
    void Add(short, SmokeParticles*);
    void AddSmoke(Vec3*, int, int, int, int, short, int);
};

class Class_00471a50 {
public:
    std::vector<Elem_00473500> lists[1];
    void Add(short, TimedSubParticles*);
    void AddTimedSubParticles(Vec3*, int, int, int, short);
};

class TimedSubParticles {
public:
    char unknown_0[8];
    int time;
    std::vector<Record_00474cd0> records;
    int emitPeriod;
    int holdPeriod;
    int maxFrame;
    Vec3 pos;
    TimedSubParticles(TimedSubParticles&);
    TimedSubParticles(void);
    void FUN_00472d50(void);
    void Render(int);
    int IsFinished(void);
    void FUN_004751c0(void);
    int FUN_004750f0(void);
    void FUN_00475150(Vec3*, int, int, int);
};

class Class_00471d70 {
public:
    char unknown_0[4];
    int deadline;
    void SetLifetime(int);
};

struct Class_00472200 {
    std::vector<ParticleSystem*> lists[10];
    ~Class_00472200(void);
    void Add(short, ParticleSystem*);
};

struct Ctx_004720d0 {
    Vec3 a;
    Vec3 b;
};

struct Pair_00472ab0 {
    short lo;
    short hi;
};

struct Shape_00472ab0 {
    Pair_00472ab0 v[3];
};

class Class_004736c0 {
public:
    char unknown_0[48];
    int endTime;
    int FUN_004736c0(int);
};

class Class_00472e00 {
public:
    char unknown_0[4];
    int deadline;
    int time;
    int FUN_00472e00(void);
};

struct Class_00472e30 {
    char unknown_0[12];
    std::vector<Class_00473590> items;
    void Render(void*);
};

struct Elem_00472e70 {
    char unknown_0[52];
};

struct Class_00472e70 {
    char unknown_0[12];
    std::vector<Elem_00472e70> items;
    int IsFinished(void);
};

class Class_00473b30 {
public:
    char unknown_0[44];
    int endTime;
    int FUN_00473b30(int);
};

class Class_00472f60 {
public:
    char unknown_0[4];
    int deadline;
    int time;
    int FUN_00472f60(void);
};

class Class_00473a00 {
public:
    char unknown_0[2];
    short x;
    char unknown_4[2];
    short height;
    char unknown_8[2];
    short y;
    char unknown_c[28];
    int color;
    char unknown_2c[4];
    void FUN_00473a00(int, short, short);
};

struct Class_00472fd0 {
    char unknown_0[12];
    std::vector<Class_00473a00> items;
    int FUN_00472fd0(void);
    void FUN_00472f90(int);
};

class Class_004742a0 {
public:
    char unknown_0[56];
    int endTime;
    int FUN_004742a0(int);
};

class Class_004730c0 {
public:
    char unknown_0[4];
    int deadline;
    int time;
    int FUN_004730c0(void);
};

struct Elem_00473130 {
    char unknown_0[60];
};

struct Class_00473130 {
    char unknown_0[12];
    std::vector<Elem_00473130> items;
    int FUN_00473130(void);
};

class WakeParticle {
public:
    int data;
    Vec3 pos;
    char unknown_10[12];
    Vec3 vel;
    int min;
    int max;
    int value;
    int step;
    int tick;
    int period;
    char unknown_40[4];
    void FUN_00474580(void);
};

class Class_00473170 {
public:
    char unknown_4[8];
    std::vector<WakeParticle> items;
    virtual void FUN_004717e0(void);
    virtual void FUN_00473170(void);
    virtual void FUN_00473250(int);
    virtual int FUN_00473290(void);
    virtual void FUN_00474880(void);
    virtual int FUN_00473220(void);
    virtual void FUN_00474760(void);
};

class Class_00474720 {
public:
    char unknown_0[4];
    char sub_4[60];
    int endTime;
    int FUN_00474720(int);
};

class Class_00473220 {
public:
    char unknown_0[4];
    int deadline;
    int time;
    int FUN_00473220(void);
};

class Class_00473250 {
public:
    char unknown_0[12];
    std::vector<Class_004745e0> items;
    void FUN_00473250(int);
};

struct Elem_00473290 {
    char unknown_0[68];
};

struct Class_00473290 {
    char unknown_0[12];
    std::vector<Elem_00473290> items;
    int FUN_00473290(void);
};

struct Pos_00473590 {
    short x;
    char unknown_2[2];
    short h;
    char unknown_6[2];
    short y;
    char unknown_a[28];
};

class Class_004736e0 {
public:
    char unknown_4[24];
    int sparkLifetime;
    Vec3 pos1;
    Vec3 pos2;
    Vec3 dir;
    virtual void v0(void);
    virtual void v1(void);
    virtual void v2(void);
    virtual void v3(void);
    virtual void v4(void);
    void FUN_004736e0(Vec3*, Vec3*, int);
};

union Fix_004736e0 {
    union { int whole; short half[2]; };
};

struct Player_00473a00 {
    char unknown_0[124];
    ByteMap_00408090 explored;
    char unknown_88[195];
};

struct Seg_00473b50 {
    Vec3 start;
    Vec3 end;
};

class Class_00473b50 {
public:
    char unknown_4[24];
    Seg_00473b50 seg_1c;
    Seg_00473b50 seg_34;
    virtual void v0(void);
    virtual void v1(void);
    virtual void v2(void);
    virtual void v3(void);
    virtual void v4(void);
    void FUN_00473b50(Seg_00473b50*, Seg_00473b50*, int);
};

union Fix_00473d50 {
    union { int whole; short half[2]; };
};

struct Pos_00473590_2 {
    short x;
    char unknown_2[2];
    short h;
    char unknown_6[2];
    short y;
    char unknown_a[28];
};

class Class_00474170 {
public:
    void* data;
    char unknown_4[2];
    Pos_00473590_2 pos;
    int frame;
    char unknown_30[12];
    void FUN_00474170(void*, short, short);
};

class ThrustParticles {
public:
    char unknown_0[8];
    int time;
    std::vector<Class_00474170> items;
    char unknown_1c[40];
    ThrustParticles(ThrustParticles&);
    ThrustParticles(void);
    void FUN_00472d50(void);
    void Render(int);
    int IsFinished(void);
    void Emit(void);
    int FUN_004730c0(void);
    void FUN_004742c0(int, int, int, int);
    void FUN_00473010(void);
};

class Class_004730f0 {
public:
    char unknown_0[12];
    std::vector<Class_00474170> items;
    void FUN_004730f0(int);
};

class Class_004742c0 {
public:
    char unknown_4[24];
    int period;
    Vec3 pos0;
    Vec3 pos1;
    Vec3 pos2;
    virtual void v0(void);
    virtual void v1(void);
    virtual void v2(void);
    virtual void v3(void);
    virtual void v4(void);
    void FUN_004742c0(Vec3*, Vec3*, int, int);
};

class Class_004743a0 {
public:
    int deadline;
    int time;
    std::vector<Record_00474cd0> records;
    int period;
    Vec3 pos0;
    Vec3 pos1;
    Vec3 pos2;
    virtual void Emit(void);
};

class Class_00475bd0 {
public:
    char unknown_0[1];
    void* FUN_00475bd0(Record_00474cd0*, unsigned int, Record_00474cd0&);
};

class Class_00474760 {
public:
    int deadline;
    char unknown_8[20];
    int period;
    Vec3 pos_a;
    Vec3 pos_b;
    Vec3 dir;
    int ascending;
    virtual void unused0(void);
    virtual void unused1(void);
    virtual void unused2(int);
    virtual int unused3(void);
    virtual void FUN_00474880(void);
    virtual void FUN_00474760(Vec3*, Vec3*, int, int, int);
};

struct SmokeParticle {
    int data;
    Pair_00419560* cursor1;
    int* cursor2;
    Pair_00419560* cursor3;
    int limit;
    int rounds;
    int period;
    int countdown;
    void FUN_00474b00(void);
};

struct Pos_00474b80 {
    short x;
    char unknown_2[2];
    short h;
    char unknown_6[2];
    short y;
};

class Class_00474cb0 {
public:
    char unknown_0[16];
    int a;
    int b;
    int FUN_00474cb0(int);
};

class Class_00474d50 {
public:
    char unknown_4[24];
    int emitPeriod;
    int holdPeriod;
    int maxFrame;
    int altAnimation;
    Vec3 pos;
    virtual void v0(void);
    virtual void v1(void);
    virtual void v2(void);
    virtual void v3(void);
    virtual void v4(void);
    void FUN_00474d50(Vec3*, int, int, int, int, int);
};

class Class_00476210 {
public:
    char unknown_0[1];
    void FUN_00476210(Record_00474cd0*, unsigned int, Record_00474cd0&);
};

struct Class_00474f80 {
    char unknown_0[4];
    int deadline;
    char unknown_8[8];
    int field_10;
    int field_14;
    int FUN_00474f80(void);
};

class TimedSubParticle {
public:
    int data;
    int x;
    int y;
    int z;
    int limit;
    int count;
    int period;
    int timer;
    void FUN_00474fc0(void);
};

class Class_00475040 {
public:
    void* data;
    char unknown_4[2];
    short x;
    char unknown_8[2];
    short height;
    char unknown_c[2];
    short y;
    char unknown_10[4];
    int count;
    void FUN_00475040(void*, short, short);
};

class Class_00475090 {
public:
    char unknown_0[16];
    int limit;
    int count;
    int FUN_00475090(int);
};

class Class_004750f0 {
public:
    char unknown_0[1];
    bool FUN_004750f0(void);
};

class Class_00475150 {
public:
    char unknown_4[24];
    int emitPeriod;
    int holdPeriod;
    int maxFrame;
    Vec3 pos;
    virtual void v0(void);
    virtual void v1(void);
    virtual void v2(void);
    virtual void v3(void);
    virtual void v4(void);
    void FUN_00475150(Vec3*, int, int, int);
};

class Vec_00476490 {
public:
    char unknown_0[1];
    void FUN_00476490(Record_00474cd0*, int, Record_00474cd0*);
};

class Class_00475440 {
public:
    char unknown_0[4];
    int deadline;
    int time;
    int FUN_00475440(void);
};

struct Element_00475ef0 {
    int data[17];
};

struct Elem_00476210 {
    int dwords[8];
};

struct Elem_00476490 {
    int dwords[8];
};

struct Menu_00476d80 {
    char unknown_0[1];
};

struct Sub_00477360 {
    char unknown_0[149];
    unsigned char side;
    char flag_96;
};

struct Menu_00477410 {
    char unknown_0[24];
};

struct ColourEntry_00478790 {
    unsigned char colour;
    char unknown_1[3];
};

struct Colour_00478790 {
    char unknown_0[2226];
    unsigned char colour;
};

struct Item_00478790 {
    char unknown_0[19];
    short x;
    short y;
    short w;
    short h;
    char unknown_1b[8];
    Colour_00478790* colours;
    char unknown_27[151];
    void* gaf;
    char unknown_c2[4];
    short frame;
};

struct Frame_00478b40 {
    unsigned short width;
    unsigned short height;
    unsigned short xoffset;
    unsigned short yoffset;
};

struct Item_00478b40 {
    char unknown_0[19];
    short x;
    short y;
    short w;
    short h;
    char unknown_1b[8];
    Colour_00478790* colours;
    char unknown_27[151];
    void* gaf;
    char unknown_c2[4];
    short frame;
};

struct Item_004794d0 {
    int type;
    char unknown_4[20];
};

struct Obj_00479bf0 {
    char unknown_0[23];
    short x;
    short y;
    char unknown_1b[20];
    GafEntry_004b8d40* entry;
    char unknown_33[264];
    unsigned char frame;
};

struct Header_00479c50 {
    unsigned char type;
    unsigned char group;
    char name[17];
    short x;
    short y;
    short w;
    short h;
    int attr;
    int color;
    int color2;
    char unknown_27[2];
    unsigned char flag;
};

struct Rec1_00479c50 {
    Header_00479c50 h;
    char unknown_2a[5];
    void* entry;
    char text[259];
    unsigned char f136;
    char pad_137;
    short f138;
    char pad_13a;
    unsigned char frame;
    char unknown_13c[2];
};

struct Rec2_00479c50 {
    Header_00479c50 h;
    char unknown_2a[9];
    char text[131];
    short count;
    char unknown_b8[16];
    int flags;
};

struct Player_0047a0e0 {
    int active;
    int shade;
    int type;
    int metal;
    int energy;
    short color;
    char unknown_16[2];
};

struct Player_0047acd0 {
    int active;
    int shade;
    int type;
    char unknown_c[8];
    int color;
};

struct Frame_0047ae60 {
    char sA[12];
    char sB[12];
    int ev[6];
    char bf[64];
};

struct Player_0047ae60 {
    int active;
    int shade;
    int type;
    int metal;
    int energy;
    short color;
    char unknown_16[2];
};

struct Table_0047ae60 {
    Player_0047ae60 players[11];
    int commanderDeath;
    int mapping;
    int lineOfSight;
    int losType;
    int fixedLocations;
    char mapName[260];
    int baseGadgetCount;
    int current;
    int difficulty;
};

struct Surfaces_0047bdf0 {
    void* surface_0;
    void* surface_4;
    void* back;
    void* field_c;
    void* surface_10;
};

class MoviePlayer {
public:
    void* smack;
    int frame;
    int field_8;
    void* hwnd;
    char unknown_10[1028];
    int hasSurfaces;
    char unknown_418[0x5b8 - 0x418];
    MoviePlayer(char*, int, int, int, int, int);
};

class Class_0047c150 {
public:
    char unknown_0[1348];
    SmackerSurfaces* surfaces;
    int GetBlitMode(SmackerSurface*);
};

class DDraw_0047bf70 {
public:
    void* lpDD;
    void* lpSurface;
    int width;
    int height;
    void* lpPalette;
};

struct Smk_0047bf70 {
    unsigned long unknown_0;
    unsigned long width;
    unsigned long height;
    unsigned int frames;
    char unknown_10[88];
    unsigned long newPalette;
    unsigned char rgb[256][3];
    unsigned long unknown_36c;
    unsigned short field_370;
    char unknown_372[2];
    int frameNum;
    char unknown_378[8];
    int lastLeft;
    int lastTop;
    int lastWidth;
    int lastHeight;
};

struct Smk_0047bf70_b {
    char unknown_0[44];
    unsigned long paletteColors;
    char unknown_30[12];
    unsigned long palette;
    char unknown_40[1020];
    unsigned long paletteType;
};

class Class_0047bf70 {
public:
    Smk_0047bf70* video;
    int counter;
    int stopped;
    void* hwnd;
    tagPALETTEENTRY entries[256];
    int paletteResult;
    char unknown_414[304];
    DDraw_0047bf70* ddraw;
    char surfaceDesc[108];
    int SetupDirectDraw(void);
};

struct SmackerSurface {
    void* methods;
};

struct SmackerSurfaces {
    char unknown_0[4];
    SmackerSurface* surface;
};

class Class_0047c230 {
public:
    char unknown_0[12];
    void* hwnd;
    tagPALETTEENTRY entries[256];
};

class Class_0047c2a0 {
public:
    PaletteSource* source;
    char unknown_4[12];
    tagPALETTEENTRY entries[256];
    char unknown_410[308];
    DisplaySurface* display;
    void UpdatePalette(void);
};

struct DisplaySurface {
    char unknown_0[16];
    void* palette;
};

struct PaletteSource {
    char unknown_0[108];
    unsigned char rgb[256][3];
};

class Class_0047c2f0 {
public:
    char unknown_0[1348];
    SurfaceWrapper* wrapper;
    void ClearScreen(void);
};

struct SurfaceWrapper {
    char unknown_0[4];
    void* surface;
};

class Class_0047c330 {
public:
    Smack_0047c330* smack;
    unsigned int frame;
};

struct Smack_0047c330 {
    unsigned int Version;
    unsigned int Width;
    unsigned int Height;
};

class Class_0047c3a0 {
public:
    void* smack;
    int unknown_4;
    int stopped;
    void* hwnd;
    tagPALETTEENTRY entries[256];
    char unknown_410[308];
    Display_0047c3a0* display;
    void PlayFrame(void*);
};

struct Class_0047cb00 {
    unsigned char a;
    unsigned char b;
    int flags;
    Unit* head;
    Class_0047cb00(void);
    void UnlinkUnit(Unit*);
};

struct Class_0047cb40 {
    char unknown_0[6];
    int head;
    void PrependUnit(int);
};

union Fix_0047d2e0 {
    union { int v; struct { short lo; short hi; } p; };
};

struct Pos_0047d2e0 {
    Fix_0047d2e0 x;
    Fix_0047d2e0 y;
    Fix_0047d2e0 z;
};

struct Cell_0047e5c0 {
    char unknown_0[6];
    Obj_0047e5c0* head;
};

struct Grid_0047e5c0 {
    Cell_0047e5c0* cells;
    unsigned int width;
    unsigned int height;
};

struct Obj_0047e5c0 {
    char unknown_0[118];
    Point16 pos;
    char unknown_7a[4];
    Point16 size;
    char unknown_82[8];
    Obj_0047e5c0* child;
    Obj_0047e5c0* next;
};

struct Entry_4 {
    Vec3 pos;
    float val;
};

class Class_004ce260 {
public:
    int open;
    char unknown_4[504];
    int field_1fc;
    int field_200;
    int field_204;
    int field_208;
    int field_20c;
    int field_210;
    unsigned char arr_214[100];
    int field_278;
    int field_27c;
    int field_280;
    int field_284;
    char unknown_288[4];
    int field_28c;
    int OpenCdAudio(void);
};

struct Entry_0047f8c0 {
    int field_0;
    int field_4;
    int field_8;
    int* data;
    char field_10;
};

class Class_0047f960 {
public:
    Entry_0047f8c0 entries[9];
    int count;
    int field_9d;
    int field_a1;
    int field_a5;
    Class_0047f960(int, int);
    Class_0047f960(void);
    void EnqueueSpeech(Unit_0047fad0*, int, char*);
    void PlaySpeech(int, int, int);
};

struct Table_004b3630 {
    int count;
    Entry* slots;
    int index;
    int field_c;
    int unknown_10;
    int unknown_14;
};

class Class_004ceee0 {
public:
    char unknown_0[36];
    void* field_24;
    void* field_28;
    char unknown_2c[408];
    void** sets[8];
    void* stream;
    Class_004bbbe0* file;
    char unknown_1ec[156];
    int handle;
    void ReleaseDirectSound(void);
};

class Class_004d06c0 {
public:
    char unknown_0[648];
    int timer;
    int StreamSampleDelayed(char*, int, int);
};

class Class_004cf540 {
public:
    char unknown_0[1];
    void PlayLooping(int, int);
};

struct Sound_0047f210 {
    char name[32];
};

class Class_004cfeb0 {
public:
    char unknown_0[8];
    int field_8;
    int field_c;
    void Set3DDistances(int, int);
};

struct Packet_0047f300 {
    unsigned char type;
    unsigned char flag;
    int index;
    char unknown_6[12];
};

struct Class_004cf150 {
    char unknown_0[48];
    int count;
    char unknown_34[4];
    void* items[32];
    void StopAllBuffers(void);
};

struct Message_0047f780 {
    char* text;
    char unknown_4[20];
};

struct Entry_0047f990 {
    int field_0;
    int field_4;
    int field_8;
    int* data;
    char field_10;
};

class Class_0047f990 {
public:
    Entry_0047f990 entries[9];
    int count;
    int field_9d;
    void Remove(int);
    void ClearSpeechList(void);
};

struct Entry_0047fa30 {
    int field_0;
    int field_4;
    int field_8;
    int* data;
    char field_10;
};

class Class_0047fa30 {
public:
    Entry_0047fa30 entries[9];
    int count;
    int field_9d;
    void Remove(int);
    void ClearSpeechEntries(void);
};

struct Message_0047fad0 {
    int priority;
    char unknown_4[8];
    char* text;
    unsigned int minFrame;
    char unknown_14[4];
};

struct Entry_0047fca0 {
    int field_0;
    int field_4;
    int field_8;
    int* data;
    char field_10;
};

class SpeechQueue {
public:
    Entry_0047fca0 entries[9];
    int count;
    unsigned int field_9d;
    unsigned int field_a1;
    void PlayNextSpeechEntry(void);
};

struct Emitter_0047fd70 {
    char unknown_0[146];
    char* name;
    char unknown_96[18];
    unsigned short field_a8;
    char unknown_aa[102];
    int field_110;
};

struct Entry_0047ffa0 {
    int field_0;
    int field_4;
    int field_8;
    int* data;
    char field_10;
};

class Class_0047ffa0 {
public:
    Entry_0047ffa0 entries[9];
    int count;
    void RemoveSpeechAt(int);
};

struct Entry_00480020 {
    int field_0;
    int field_4;
    int field_8;
    int* data;
    char field_10;
};

class Class_00480020 {
public:
    Entry_00480020 entries[9];
    int count;
    void RemoveSpeechOfId(int);
};

class IntDynArray {
public:
    std::vector<int> items;
    int EraseByValue(int);
};

class Class_00480c30 {
public:
    char unknown_0[1344];
    Data_00480c30* data;
    int GetPieceTranslation(int, int);
};

struct Entry_00480c30 {
    int unknown_0;
    int values[3];
    char unknown_10[38];
};

struct Data_00480c30 {
    char unknown_0[34];
    Entry_00480c30 entries[1];
};

class Class_00480c50 {
public:
    char unknown_0[1344];
    BuildList_0045a950* data;
    void SetPieceTranslation(int, int, int);
};

struct Entry_00480c50 {
    char unknown_0[16];
    int values[3];
    unsigned short shorts[11];
    short value;
    unsigned short flag0 : 1;
    unsigned short flag1 : 1;
    unsigned short : 14;
};

class Class_00480cb0 {
public:
    char unknown_0[1344];
    void* table;
    int GetPieceRotation(int, int);
};

struct Elem_4b0610 {
    int value;
    char unknown_4[28];
    int field_20;
    char unknown_24[128];
};

#include "../src/units/cob_script.h"

#include "../src/units/unit_script.h"

class Class_00480d40 {
public:
    char unknown_0[1344];
    void* field_540;
    void SetObjectState(int);
};

class Class_00480d50 {
public:
    char unknown_0[1344];
    BuildList_0045a950* data;
    void SetPieceVisible(int, int);
};

struct Entry_00480d50 {
    short value;
    unsigned short flag0 : 1;
    unsigned short flag1 : 1;
    unsigned short flag2 : 1;
    unsigned short : 13;
    char unknown_4[50];
};

class Class_00480e30 {
public:
    char unknown_0[1344];
    void* buffer;
    int FUN_00480e30(int);
};

class Class_00480e50 {
public:
    char unknown_0[1];
    unsigned char FUN_00480e50(int);
};

class Class_00480e70 {
public:
    char unknown_0[1344];
    int* field_540;
    unsigned int FUN_00480e70(int);
};

struct Entry_00480eb0 {
    Vec3 pos;
    int* offset;
    char unknown_10[38];
};

class Class_00481340 {
public:
    char unknown_0[1344];
    Player_00481340* player;
    void FUN_00481340(unsigned short, int, int);
};

struct Player_00481340 {
    char unknown_0[12];
    int id;
};

class Class_004813b0 {
public:
    char unknown_0[1344];
    Player_004813b0* player;
    void FUN_004813b0(unsigned short);
};

struct Player_004813b0 {
    char unknown_0[12];
    int id;
};

class Class_00481430 {
public:
    char unknown_0[1344];
    Link_00481430* link;
    int FUN_00481430(int);
};

struct Item_00481430 {
    char unknown_0[142];
    Item_00481430* next;
    char unknown_92[22];
    unsigned short id;
};

struct Link_00481430 {
    char unknown_0[12];
    Owner_00481430* owner;
};

struct Owner_00481430 {
    char unknown_0[138];
    Item_00481430* items;
};

class Class_00481470 {
public:
    char unknown_0[1344];
    void* ptr_540;
    unsigned short FUN_00481470(void);
};

struct FrameTable {
    unsigned short count;
    char unknown_2[38];
    Entry* entries;
};

struct Eye_00482130 {
    void* field_0;
    short* field_4;
    short field_8;
    unsigned char field_a;
    char unknown_b;
    unsigned char* field_c;
    Vec3 pos;
    int unknown_1c;
    int unknown_20;
    Eye_00482130& operator=(Eye_00482130&);
};

struct Flags_142f1_004816a0 {
    unsigned short bit0 : 1;
    unsigned short bit1 : 1;
    unsigned short mapChanged : 1;
    unsigned short rest : 13;
};

struct Frame_00481930 {
    unsigned short width;
    unsigned short height;
    char unknown_4[4];
    unsigned char mask;
    char unknown_9[7];
    unsigned char* data;
};

struct Player_00481930 {
    char unknown_0[124];
    Grid grid;
    char unknown_8c[186];
    unsigned char field_146;
};

struct Frame_00481d50 {
    unsigned short width;
    unsigned short height;
    char unknown_4[4];
    unsigned char mask;
    char unknown_9[7];
    unsigned char* data;
};

#include "../src/map/eye.h"

struct Expired {
    char unknown_0[1];
    int operator()(Eye_00482130&);
};

struct Out_482830 {
    short x;
    short y;
};

struct Frame_482270 {
    unsigned short width;
    unsigned short height;
    char unknown_4[4];
    unsigned char mask;
    char unknown_9[7];
    unsigned char* data;
};

struct Entry_482ac0 {
    int field_0;
    short xOffset;
    short yOffset;
    short field_8;
    char unknown_a[2];
};

struct Grid2_482c20 {
    unsigned char* cells;
    int width;
    int height;
    int field_c;
    int field_10;
    int field_14;
};

struct MapInfo_00483610 {
    int version;
    int width;
    int height;
    int flag;
    int sea_a;
    int sea_b;
    int sea_d;
    int tile_count;
    int map_size;
    int* tile_set_src;
    unsigned int tile_set_count;
    int* tile_map_src;
    unsigned char* attr_b;
    unsigned char* attr_a;
    int attr_limit;
    unsigned int feature_flags;
    unsigned short* feature_data;
};

union Slot_00483610 {
    int n;
};

struct TntHeader_00483610 {
    unsigned short width;
    unsigned short height;
    unsigned short pad0;
    unsigned short pad1;
    unsigned char flag[4];
    int zero0;
    unsigned short* data;
    int zero1;
};

struct Cell_004848e0 {
    unsigned char lo;
    unsigned char level1;
};

#include "../src/map/map_info.h"

#include "../src/map/player_grid.h"

struct Fixed_00485330 {
    unsigned short frac;
    short whole;
};

struct Pos_00485330 {
    Fixed_00485330 x;
    Fixed_00485330 y;
    Fixed_00485330 z;
};

struct FixedParts_004853b0 {
    unsigned int frac : 16;
    unsigned int whole : 16;
};

union Fixed_004853b0 {
    union { int value; FixedParts_004853b0 parts; };
};

struct Item_00485940 {
    char unknown_0[4];
    unsigned int key;
    char unknown_8[318];
    unsigned char field_146;
    char unknown_147[5];
};

union Flags114_485a40 {
    union { unsigned int all; struct { unsigned int b0 : 1; } bits; };
};

struct Unit_00485d40 {
    char unknown_0[398];
    BuildList_0045a950* data;
};

struct Spawn_004861d0 {
    unsigned char player;
    unsigned short type;
    unsigned short id;
    Vec3 pos;
    char unknown_11[6];
};

struct Name_0048d630 {
    char name[562];
};

struct UnitKilledPacket {
    unsigned char type;
    unsigned short unitId;
    int killerId;
    unsigned short parentId;
    char field_9;
    unsigned char count : 4;
    unsigned char kind : 4;
};

struct Name_004866d0 {
    char name[562];
};

class Class_004b4b50 {
public:
    Table_004b3630* table;
    int OpenNumberedBox(int);
};

struct UnitRecord_00486fd0 {
    char unknown_0[33];
    short id;
    char unknown_23[149];
};

struct Bits10F_00487080 {
    unsigned char b0 : 1;
    unsigned char b1 : 1;
    unsigned char b2 : 1;
    unsigned char b3 : 1;
    unsigned char b4 : 4;
};

class Class_0043a420 {
public:
    char unknown_0[66];
    unsigned int flags;
    char unknown_46[4];
    Class_0043a420* next;
    char unknown_4e[8];
    Class_0043a420(Unit*, HapiBank*, char*);
};

struct FlagBits_00487080 {
    unsigned int a0 : 1;
    unsigned int a1 : 1;
    unsigned int a2 : 1;
    unsigned int a3 : 1;
    unsigned int b : 12;
    unsigned int c : 1;
    unsigned int d : 3;
    unsigned int e : 12;
};

struct PieceBits_00487080 {
    unsigned char b0 : 1;
    unsigned char b1 : 1;
    unsigned char c : 2;
    unsigned char d : 1;
    unsigned char b5 : 3;
};

struct Slot {
    int f0;
    int unused;
    int f8;
    void* obj;
    int fc;
    short f14;
    short f16;
    short f18;
    unsigned char f1a;
    PieceBits_00487080 flags;
};

struct SrcPiece_00487080 {
    int f0;
    int f4;
    unsigned char f8;
    char unknown_9[3];
    int fc;
    short f10;
    short f12;
    short f14;
    unsigned char f16;
    PieceBits_00487080 fl;
};

struct SaveRec_00487080 {
    char name[32];
    unsigned char player;
    unsigned short id;
    int f23;
    int f27;
    int f2b;
    int f2f;
    int f33;
    int f37;
    short f3b;
    short f3d;
    short f3f;
    SrcPiece_00487080 pieces[3];
    short childA;
    short childB;
    char b8d;
    unsigned char b8e;
    int f8f;
    int f93;
    int f97;
    int f9b;
    int f9f;
    int fa3;
    int fa7;
    unsigned char bab;
    unsigned char bac;
    unsigned char bad;
    short bae;
    unsigned char bb0;
    unsigned char bb1;
    unsigned char bb2;
    unsigned char b3;
    FlagBits_00487080 flags;
};

struct FlagBits_004876c0 {
    unsigned int a0 : 1;
    unsigned int a1 : 1;
    unsigned int a2 : 1;
    unsigned int a3 : 1;
    unsigned int b : 12;
    unsigned int c : 1;
    unsigned int d : 3;
    unsigned int e : 12;
};

struct SavedPiece_004876c0 {
    int f0;
    int f4;
    int f8;
    int fc;
    short f10;
    short f12;
    short f14;
    unsigned char f16;
    PieceBits_00487080 flags;
};

struct UnitRecord_004876c0 {
    char name[32];
    unsigned char f20;
    unsigned short id;
    int f23;
    int f27;
    Vec3 pos;
    char unknown_37[6];
    short f3d;
    short f3f;
    SavedPiece_004876c0 pieces[3];
    short f89;
    short f8b;
    unsigned char f8d;
    unsigned char f8e;
    int f8f;
    int f93;
    int f97;
    int f9b;
    int f9f;
    int fa3;
    int fa7;
    unsigned char fab;
    unsigned char fac;
    unsigned char fad;
    unsigned short fae;
    unsigned char fb0;
    unsigned char fb1;
    unsigned short fb2;
    FlagBits_004876c0 flags;
};

struct Entry_00487af0 {
    char* name;
    char* unknown_4;
    char* extra;
    Vec3 pos;
    short f18;
    short f1a;
    char unknown_1c[6];
    unsigned char player;
    unsigned char flags;
};

struct Struct_00487af0 {
    char unknown_0[4];
    int* ids;
};

struct Outs_t {
    char unknown_0[20];
};

struct OwnershipTransferPacket {
    unsigned char type;
    short team;
    int who;
    int x;
    int y;
    char unknown_f[6];
    unsigned char b15;
    unsigned char b16;
    unsigned char b17;
};

struct Elem_00488a00 {
    Class_004c9390 name;
    int value;
    ~Elem_00488a00(void);
};

struct Elem_00488a00_2 {
    Class_004c9390 name;
    int value;
    ~Elem_00488a00_2(void);
};

struct Elem_00488a00_3 {
    Class_004c9390 name;
    void* value;
    ~Elem_00488a00_3(void);
};

class Class_00488c50 {
public:
    int unknown_0[16];
    Class_00488c50(void);
};

class UnitCategory {
public:
    Class_004c9390 name;
    int value;
    UnitCategory(UnitCategory&);
    UnitCategory(Class_004c9390&, void*);
    ~UnitCategory(void);
    UnitCategory& operator=(UnitCategory&);
};

class Class_00489240 {
public:
    Class_004c9390 name;
    int value;
    Class_00489240* AssignCategory(Class_00489240*);
};

struct UnitType_00489280 {
    char unknown_0[390];
    float field_186;
    float field_18a;
    char unknown_18e[4];
    int field_192;
    char unknown_196[8];
    int field_19e;
    char unknown_1a2[24];
    unsigned short field_1ba;
    char unknown_1bc[46];
    int field_1ea;
    char unknown_1ee[65];
    unsigned char field_22f;
    char unknown_230[16];
};

struct Unit_004894f0 {
    char unknown_0[146];
    UnitDef* def;
};

class Class_00489540 {
public:
    char unknown_0[4];
    Owner_00489540* owner;
    Class_00489540* next;
    void LinkToUnit(Owner_00489540*);
    void UnlinkFromUnit(void);
};

struct Owner_00489540 {
    char unknown_0[162];
    Class_00489540* head;
    short flag;
};

class Class_004896f0 {
public:
    char unknown_0[4];
    Owner_004896f0* owner;
    Class_004896f0* next;
    Listener_004896f0* listener;
    void ClearRef(void);
};

struct Owner_004896f0 {
    char unknown_0[162];
    Class_004896f0* head;
};

class Class_00489740 {
public:
    char unknown_0[4];
    Owner_00489740* owner;
    Class_00489740* next;
    Listener_00489740* listener;
};

class Listener_00489740 {
public:
    virtual void Notify(int);
};

struct Owner_00489740 {
    char unknown_0[162];
    Class_00489740* head;
};

union Flags_004898b0 {
    union { struct { unsigned char bit0 : 1; unsigned char bit1 : 1; unsigned char bit2 : 1; unsigned char bit3 : 1; unsigned char bit4 : 1; unsigned char rest : 3; } bits; unsigned char all; };
};

struct Entry_004898b0 {
    Point16 point;
    char unknown_4[23];
    Flags_004898b0 flags;
};

class Class_00489800 {
public:
    int unknown_0;
    Entry_004898b0 entries[5];
    char unknown_90[10];
    Class_004b0a70* script;
    void ReleaseWeapons(unsigned char);
};

union Flags_004898b0_2 {
    union { struct { unsigned char bit0 : 1; unsigned char bit1 : 1; unsigned char bit2 : 1; unsigned char bit3 : 1; unsigned char bit4 : 1; unsigned char rest : 3; } bits; unsigned char all; };
};

struct Entry_004898b0_2 {
    Point16 point;
    char unknown_4[23];
    Flags_004898b0_2 flags;
};

class Class_004898b0 {
public:
    int unknown_0;
    Entry_004898b0_2 entries[5];
    char unknown_90[10];
    Class_004b0a70* script;
    void ClaimWeapons(unsigned char);
};

struct Def_004899b0 {
    char unknown_0[368];
    short f170;
    char unknown_172[76];
    short f1be;
    char unknown_1c0[58];
    int f1fa;
    char unknown_1fe[67];
    int f241;
    int f245;
};

struct Node_00489a90 {
    char unknown_0[134];
    Class_00489a70* owner;
    char unknown_8a[4];
    Node_00489a90* next;
};

struct UnitDamagePacketOut {
    unsigned char type;
    unsigned short attacker;
    unsigned short target;
    unsigned short amount;
    unsigned char param_7;
    unsigned char type_8;
};

struct Entry_0048a0f0 {
    Point16 point;
    char unknown_4[24];
};

struct Entry_0048a160 {
    Point16 point;
    char unknown_4[24];
};

struct Entry_0048a190 {
    Point16 point;
    char unknown_4[24];
};

struct Entry_0048a1e0 {
    Point16 point;
    char* name;
    int f_8;
    Target_0049e1a0* target;
    char unknown_10[4];
    unsigned short f_14;
    short f_16;
    short f_18;
    unsigned char f_1a;
    unsigned char flags;
};

struct Muzzle_0048a1e0 {
    char unknown_0[8];
    Vec3 offset;
};

struct Entry_0048a440 {
    short id;
    short kind;
    char unknown_4[24];
};

struct Class_0048a440 {
    char unknown_0[4];
    Entry_0048a440 entries[3];
};

struct Object_0048a440 {
    char unknown_0[168];
    unsigned short id;
};

struct Order_0048ab70 {
    unsigned char type;
    unsigned short id1;
    unsigned short id2;
    char param;
    unsigned char param2;
};

struct ActivateFlagsPacket {
    unsigned char type;
    short field_1;
    unsigned char field_3;
};

union Fixed_0048bae0 {
    union { int value; struct { unsigned short fraction; short whole; } parts; };
};

struct Point_0048c6a0 {
    int x;
    int y;
};

struct Orders_0048d630 {
    unsigned short bits0 : 4;
    unsigned short selected : 1;
    unsigned short bits5 : 11;
};

struct Pos_0048d630 {
    short x;
    short y;
    short z;
};

struct UnitFlags_0048d630 {
    unsigned int bits0 : 4;
    unsigned int selected : 1;
    unsigned int done : 1;
    unsigned int bit6 : 1;
    unsigned int bit7 : 1;
    unsigned int bits8 : 24;
};

struct UnitRange_0048d920 {
    Unit* begin;
    Unit* end;
};

struct Unit32_0048d9a0 {
    char unknown_0[134];
    Team_0048d9a0* owner;
    char unknown_8a[28];
    unsigned short type;
    char unknown_a8[4];
    int field_ac;
    char unknown_b0[75];
    int field_fb;
    char unknown_ff[5];
    float field_104;
    char unknown_108[8];
    unsigned int flags;
    char unknown_114[4];
};

class Class_0048ea40 {
public:
    int unknown_0;
    int done;
    int announced;
    void FUN_0048ea40(Unit*);
};

class Class_0048eac0 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048eac0(HapiBank*);
};

class Class_0048eb00 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048eb00(HapiBank*);
};

class Class_0048eb80 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048eb80(HapiBank*);
};

class Class_0048ebc0 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048ebc0(HapiBank*);
};

class Class_0048ec00 {
public:
    char unknown_0[4];
    int field_4;
    bool FUN_0048ec00(int*);
};

struct Player_0048ec20 {
    int field_0;
    char unknown_4[251];
    unsigned char kind;
};

struct UnitList_0048ec20 {
    Unit_0048ec20* first;
    Unit_0048ec20* last;
    void ForEach(UnitVisitor_48efb0*);
};

class UnitVisitor_0048ec20 {
public:
    virtual int Visit(Unit_0048ec20*) = 0;
};

struct Unit_0048ec20 {
    char unknown_0[166];
    short field_a6;
    char unknown_a8[112];
};

class Class_0048ecb0 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    char unknown_c[4];
    int numUnits;
    void FUN_0048ecb0(HapiBank*);
};

class Class_0048ed00 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    char unknown_c[4];
    int numUnits;
    void FUN_0048ed00(HapiBank*);
};

class Class_0048ed50 {
public:
    char unknown_0[16];
    char name[32];
    short id;
    int FUN_0048ed50(Unit*);
};

class UnitVisitor_0048ed50 {
public:
    virtual int FUN_0048ed50(Unit*) = 0;
};

class UnitCallback_0048edb0 {
public:
    virtual int Visit(Unit_0048ec20*) = 0;
};

class Class_0048ee30 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048ee30(HapiBank*);
};

class Class_0048ee70 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048ee70(HapiBank*);
};

struct Info_0048eeb0 {
    char unknown_0[32];
    char name[32];
};

struct Player_0048eeb0 {
    char unknown_0[146];
    Info_0048eeb0* info;
    char unknown_96[105];
    unsigned char kind;
};

class Class_0048ef00 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048ef00(HapiBank*);
};

class Class_0048ef40 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048ef40(HapiBank*);
};

class Class_0048ef80 {
public:
    char unknown_0[36];
    short field_24;
    int field_26;
    bool FUN_0048ef80(int);
};

struct Info_48efb0 {
    char unknown_0[32];
    char name[32];
};

struct Player_48efb0 {
    char unknown_0[146];
    Info_48efb0* info;
    char unknown_96[105];
    unsigned char kind;
};

class UnitVisitor_48efb0 {
public:
    virtual int Visit(Unit_0048ec20*) = 0;
};

class Class_0048f070 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048f070(HapiBank*);
};

class Class_0048f0b0 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048f0b0(HapiBank*);
};

struct Info_0048f0f0 {
    char unknown_0[32];
    char name[32];
};

struct Player_0048f0f0 {
    char unknown_0[146];
    Info_0048f0f0* info;
    char unknown_96[105];
    unsigned char kind;
};

class Class_0048f160 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    char unknown_c[32];
    int numLeftToKill;
    void FUN_0048f160(HapiBank*);
};

class Class_0048f1b0 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    char unknown_c[32];
    int numLeftToKill;
    void FUN_0048f1b0(HapiBank*);
};

class UnitVisitor_0048f200 {
public:
    virtual void Visit(void*) = 0;
};

class UnitVisitor_0048f250 {
public:
    virtual void FUN_0048f250(Unit*) = 0;
};

class Class_0048f2f0 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048f2f0(HapiBank*);
};

class Class_0048f330 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048f330(HapiBank*);
};

class Class_0048f370 {
public:
    char unknown_0[16];
    char name[32];
    int field_30;
    int FUN_0048f370(Unit*);
};

class UnitVisitor_0048f370 {
public:
    virtual int FUN_0048f370(Unit*) = 0;
};

class UnitVisitor_0048f3e0 {
public:
    virtual int Visit(Unit_0048ec20*) = 0;
};

class Class_0048f440 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048f440(HapiBank*);
};

class Class_0048f480 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048f480(HapiBank*);
};

class Class_0048f4c0 {
public:
    char unknown_0[16];
    char name[32];
    int field_30;
    int FUN_0048f4c0(Unit*);
};

class UnitVisitor_0048f4c0 {
public:
    virtual int FUN_0048f4c0(Unit*) = 0;
};

class UnitCallback_0048f530 {
public:
    virtual int Visit(Unit_0048ec20*) = 0;
};

class Class_0048f590 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048f590(HapiBank*);
};

class Class_0048f5d0 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048f5d0(HapiBank*);
};

class Class_0048f630 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048f630(HapiBank*);
};

class Class_0048f670 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048f670(HapiBank*);
};

class Class_0048f710 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048f710(HapiBank*);
};

class Class_0048f750 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048f750(HapiBank*);
};

class Class_0048f7e0 {
public:
    char unknown_0[16];
    int VisitUnit(Unit*);
    int FUN_0048f7e0(void);
};

class UnitCallback_0048f7e0 {
public:
    virtual int VisitUnit(Unit*) = 0;
};

class UnitCallback_0048f7e0_2 {
public:
    virtual int Visit(Unit_0048ec20*) = 0;
};

class Class_0048f880 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void LoadState(HapiBank*);
};

struct Info_0048f8c0 {
    char unknown_0[32];
    char name[32];
};

struct Unit_0048f8c0 {
    char unknown_0[146];
    Info_0048f8c0* info;
};

class Class_0048f900 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    char unknown_c[32];
    int numLeftToKill;
    void FUN_0048f900(HapiBank*);
};

class Class_0048f950 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    char unknown_c[32];
    int numLeftToKill;
    void FUN_0048f950(HapiBank*);
};

struct Class_0048f9a0 {
    char unknown_0[42];
    bool FUN_0048f9a0(Param*);
};

struct Param {
    char unknown_0[166];
    unsigned short field_a6;
};

struct Info_0048f9d0 {
    char unknown_0[32];
    char name[32];
};

struct Player_0048f9d0 {
    char unknown_0[146];
    Info_0048f9d0* info;
    char unknown_96[105];
    unsigned char kind;
};

class UnitVisitor_0048f9d0 {
public:
    virtual int Visit(Unit_0048ec20*) = 0;
};

class Class_0048fab0 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048fab0(HapiBank*);
};

class Class_0048faf0 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048faf0(HapiBank*);
};

struct Class_0048fb30 {
    int unknown_0;
    int unknown_4;
    bool FUN_0048fb30(Other_0048fb30*);
};

struct Other_0048fb30 {
    char unknown_0[118];
    short value;
};

class UnitVisitor_0048fb60 {
public:
    virtual int Visit(Unit_0048ec20*) = 0;
};

class Class_0048fbc0 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048fbc0(HapiBank*);
};

class Class_0048fc00 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048fc00(HapiBank*);
};

struct Class_0048fc40 {
    int unknown_0;
    int unknown_4;
    bool FUN_0048fc40(Other_0048fc40*);
};

struct Other_0048fc40 {
    char unknown_0[120];
    short value;
};

class UnitVisitor_0048fc70 {
public:
    virtual int Visit(Unit_0048ec20*) = 0;
};

class Class_0048fcd0 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048fcd0(HapiBank*);
};

class Class_0048fd10 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048fd10(HapiBank*);
};

class Class_0048fd70 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048fd70(HapiBank*);
};

class Class_0048fdb0 {
public:
    char unknown_0[4];
    int satisfied;
    int celebrated;
    void FUN_0048fdb0(HapiBank*);
};

class Listener_0048ff40_2 {
public:
    virtual void VisitUnit(void*) = 0;
};

class Listener_0048ff40_3 {
public:
    virtual void VisitUnit(void*) = 0;
};

#include "../src/game/packed_goal.h"

class Class_00490650 {
public:
    char unknown_0[12];
    Vec3 a;
    Vec3 b;
    short c;
    char unknown_26[2];
    void FUN_00490650(Vec3*, Vec3*, short*);
};

struct Class_00490880_link {
    Class_00490880_target* ptr;
};

struct Class_00490880_target {
    char unknown_0[46];
    unsigned char field_2e;
};

struct Class_004908b0 {
    char unknown_0[39];
    unsigned char field_0x27;
    unsigned char FUN_004908b0(void);
};

class Class_004908c0 {
public:
    char unknown_0[4];
    Link_004908c0* link;
    Holder_004908c0* holder;
    char unknown_c[27];
    unsigned char state : 3;
    unsigned char : 5;
    void FUN_004908c0(BitWriter*);
};

struct Holder_004908c0 {
    Target_004908c0* ptr;
};

class Link_004908c0 {
public:
    virtual void vf0(void);
    virtual void vf1(void);
    virtual int GetType(void);
    virtual void vf3(void);
    virtual void vf4(void);
    virtual void vf5(void);
    virtual void vf6(void);
    virtual void vf7(void);
    virtual void vf8(void);
    virtual void vf9(void);
    virtual void Write(BitWriter*);
};

struct Target_004908c0 {
    char unknown_0[46];
    unsigned char mode : 2;
    unsigned char : 6;
};

class Base_00490a10_3 {
public:
    virtual ~Base_00490a10_3(void);
};

class Class_00490a10 {
public:
    int unknown_0;
    Base_00490a10_3* current;
    Unit* owner;
    void FUN_00490a10(BitReader*);
};

class Class_004cd9c0 {
public:
    char unknown_0[1];
    int GetDiscSerial(void);
};

class Class_004ce460 {
public:
    char unknown_0[1];
    int IsFirstTrackData(void);
};

class Class_004cd9d0 {
public:
    char unknown_0[652];
    void (__stdcall *callback)(void);
    int SetCdCallback(void (__stdcall *)(void));
};

#include "../src/game/zero11_004917d0.h"

struct Saved_00493bf0 {
    int a;
    int b;
    short c;
};

struct Menu_00494220 {
    char unknown_0[1];
};

struct Gadget_00494290 {
    char unknown_0[24];
    Inner_00494290* inner;
};

struct Inner_00494290 {
    int unknown_0;
    Owner_00494290* entries;
};

struct Owner_00494290 {
    char unknown_0[188];
    void* surface;
};

struct Class_004cb7c0 {
    char unknown_0[12];
    int field_c;
    Class_004cb7c0* FUN_004cb7c0(void);
};

struct Class_004cb7d0 {
    char unknown_0[12];
    void* file;
    void FUN_004cb7d0(void);
};

class Class_004cb7f0 : public Class_004cb7c0 {
public:
    bool Open(char*, int, int);
};

struct Class_004cb940 {
    int unknown_0;
    int height;
    int dataOffset;
    _iobuf* file;
    bool WriteRows(Image_004cb940*, int, int, int, int, int, int);
};

#include "../src/ingame/flags_00495e90_38a51.h"

#include "../src/ingame/flags_00495e90_3923b.h"

struct Sub_00496b10 {
    char unknown_0[166];
};

#include "../src/game/flags_00496ce0.h"

#include "../src/game/settings_00496e10.h"

union Fixed_497180 {
    union { int i; struct { short frac; short whole; } h; };
};

struct PlayerFlags_497180 {
    unsigned short low : 8;
    unsigned short b8 : 1;
    unsigned short b9 : 1;
    unsigned short b10 : 1;
    unsigned short b11_12 : 2;
    unsigned short b13 : 1;
    unsigned short rest : 2;
};

struct Settings_00496e10_2 {
    int value;
    int flag_4;
    int flag_8;
    int flag_c;
};

#include "../src/game/view_flags.h"

class Class_004ce800 {
public:
    char unknown_0[1];
    int IsCdPlaying(void);
};

#include "../src/game/load_flags_00497f40.h"

#include "../src/game/player_slots_00497f40.h"

#include "../src/ingame/bit_flags16_00499200.h"

#include "../src/ingame/flags16_00499200.h"

struct Obj_00499ab0 {
    char unknown_0[150];
    Kind_00499ab0* kind;
    char unknown_9a[14];
    unsigned short team;
};

struct WeaponFirePacket {
    unsigned char type;
    Vec3 a;
    Vec3 b;
    unsigned char f19;
    unsigned char flag : 1;
    unsigned char : 7;
    unsigned short f1b;
    unsigned short f1d;
    unsigned short f1f;
    unsigned short f21;
    unsigned char f23;
};

struct Packet_00499ba0 {
    unsigned char type;
    Vec3 a;
    Vec3 b;
    char field_19;
    char unknown_1a[10];
};

struct Object_00499c10 {
    char unknown_0[102];
    short heading;
    char unknown_68[50];
    Class_004b0a70* anims;
};

struct NameLess_00499cd0 {
    char unknown_0[1];
    unsigned char operator()(char*, char*);
};

struct UnitType_499e50 {
    char unknown_0[254];
    short value;
};

union Fixed_0049abb0 {
    union { int value; struct { unsigned short fraction; short whole; } parts; };
};

struct Weapon_0049abb0 {
    WeaponDef* def;
    char unknown_4[24];
};

struct ProjectileDetonatePacket {
    char kind;
    Vec3 pos;
    unsigned char typeId;
};

struct Angles_0049be60 {
    short x;
    short y;
    short z;
};

struct FlagsBits_0049cc20 {
    unsigned int low : 9;
    unsigned int bit9 : 1;
    unsigned int high : 22;
};

struct Head_0049cc20 {
    char unknown_0[76];
    short heading;
    char unknown_4e[50];
};

struct Shot_0049cc20 {
    char unknown_0[8];
    int piece;
    UnitMotion* def;
    char unknown_10[6];
    short heading;
    short pitch;
    char unknown_1a;
    unsigned char weapon;
};

struct Heading_0049cde0 {
    char unknown_0[76];
    short heading;
};

struct Shot_0049d000 {
    char unknown_0[12];
    void* weapon;
};

struct Entry_0049d120 {
    char unknown_0[12];
    UnitMotion* def;
    char unknown_10[10];
    unsigned char active;
    char unknown_1b;
};

struct Event_0049d1e0 {
    char unknown_0[13];
    Vec3 pos;
    unsigned char team;
    unsigned char flag : 1;
    unsigned char unknown_1a : 7;
    unsigned short f_1b;
    unsigned short f_1d;
    unsigned short ownerId;
    unsigned short unitId;
    unsigned char entryIndex;
};

struct DefFlags_0049d270 {
    unsigned int b0 : 1;
    unsigned int b1 : 1;
    unsigned int b2 : 2;
    unsigned int b4 : 1;
    unsigned int b5 : 1;
    unsigned int b6 : 2;
    unsigned int b8 : 1;
    unsigned int b9 : 11;
    unsigned int b20 : 1;
    unsigned int b21 : 11;
};

struct Entry_0049d270 {
    char unknown_0[12];
    UnitMotion* type;
    char unknown_10[6];
    short f_16;
    short f_18;
    char unknown_1a[2];
};

struct Packet_0049d580 {
    unsigned char type;
    Vec3 a;
    Vec3 b;
    unsigned char team;
    unsigned char unknown_1a;
    short heading;
    short pitch;
    short target_a8;
    short unit_a8;
    unsigned char weapon;
};

struct Packet_0049d9c0 {
    unsigned char type;
    Vec3 a;
    Vec3 b;
    unsigned char team;
    unsigned char unknown_1a;
    short heading;
    short pitch;
    short target_a8;
    short unit_a8;
    unsigned char weapon;
};

struct Packet_0049db70 {
    unsigned char type;
    Vec3 pos;
    Vec3 aim;
    unsigned char field_19;
    unsigned char flag : 1;
    unsigned char : 7;
    short heading;
    short pitch;
    short target_team;
    short source_team;
    unsigned char weapon;
};

struct Aim_0049dd60 {
    char unknown_0[12];
    UnitMotion* type;
    char unknown_10[6];
    short field_16;
    short field_18;
    char unknown_1a;
    unsigned char weapon;
};

struct Packet_0049dd60 {
    unsigned char type;
    Vec3 a;
    Vec3 b;
    unsigned char team;
    unsigned char unknown_1a;
    short heading;
    short pitch;
    short target_a8;
    short unit_a8;
    unsigned char weapon;
};

struct Packet_0049df10 {
    unsigned char type;
    Vec3 a;
    Vec3 b;
    unsigned char field_19;
    char unknown_1a[10];
};

struct Unit_0049df10 {
    char unknown_0[266];
    unsigned char team;
};

struct Flags_0049e010 {
    unsigned int bit0 : 1;
    unsigned int unknown_1 : 3;
    unsigned int bit4 : 1;
    unsigned int unknown_5 : 3;
    unsigned int bit8 : 1;
    unsigned int unknown_9 : 10;
    unsigned int bit19 : 1;
    unsigned int bit20 : 1;
    unsigned int : 11;
};

struct Frame_0049e070 {
    unsigned char i;
    char unknown_1[3];
    int maxTime;
};

struct Flags_0049e1a0 {
    unsigned int b0 : 1;
    unsigned int b1 : 1;
    unsigned int b2_3 : 2;
    unsigned int b4 : 1;
    unsigned int b5_18 : 14;
    unsigned int b19 : 1;
    unsigned int b20_25 : 6;
    unsigned int b26 : 1;
    unsigned int b27 : 1;
    unsigned int b28 : 1;
    unsigned int b29_31 : 3;
};

class Class_004ce410 {
public:
    int open;
    void CloseCdAudio(void);
};

class Class_004cee50 {
public:
    int field_0;
    int field_4;
    float field_8;
    float field_c;
    int wave_devices;
    unsigned int aux_device;
    int wave_volume;
    int aux_volume;
    int aux_volume_set;
    int field_24;
    int field_28;
    int field_2c;
    int field_30;
    int field_34;
    int array_38[32];
    int array_b8[32];
    int array_138[32];
    char unknown_1b8[12];
    int array_1c4[8];
    int field_1e4;
    char unknown_1e8[156];
    int field_284;
    int field_288;
    char unknown_28c[4];
    int field_290;
    Class_004cee50(void);
};

class Class_004cf0b0 {
public:
    char unknown_0[48];
    int count;
    char unknown_34[4];
    void* buffers[32];
    char unknown_b8[268];
    void** sets[8];
    int field_1e4;
    void ReapFinishedBuffers(void);
};

union Dword_0051f410 {
    union { int value; struct { unsigned int b0 : 1; unsigned int b1 : 1; unsigned int b2 : 1; unsigned int b3 : 1; unsigned int b4 : 1; unsigned int b5 : 1; unsigned int b6 : 1; unsigned int b7 : 1; unsigned int b8 : 1; unsigned int b9 : 1; unsigned int b10 : 1; unsigned int b11 : 1; unsigned int spare : 20; } bits; };
};

union Word_0051f522 {
    union { unsigned short value; struct { unsigned short b0 : 1; unsigned short b1 : 1; unsigned short b2 : 1; unsigned short b3 : 1; unsigned short b4 : 1; unsigned short b5 : 1; unsigned short b6 : 1; unsigned short b7 : 1; unsigned short b8 : 1; unsigned short b9 : 1; unsigned short spare : 6; } bits; };
};

struct App_0049e830 {
    int hInstance;
    int nCmdShow;
    int className;
    int title;
    int unknown_10;
    int menuId;
    char unknown_18[200];
    int field_e0;
    char unknown_e4[12];
    Dword_0051f410 flags;
    char unknown_f4[262];
    int startWidth;
    int startHeight;
    Word_0051f522 video;
};

struct Class_0049f8c0 {
    char unknown_0[24];
    Dialog* holder;
};

struct Object_0049f930 {
    char unknown_0[24];
    BuildList_0045a950* data;
};

struct Inner_0049fb30 {
    char unknown_0[20];
    int value;
    int field_18;
};

struct Outer_0049fb30 {
    char unknown_0[24];
    Inner_0049fb30* inner;
};

struct Struct_0049fb50 {
    char unknown_0[2998];
    char path[256];
};

struct Struct_0049fba0 {
    char unknown_0[2486];
    char path[256];
};

struct Struct_0049fbf0 {
    char unknown_0[2742];
    char path[256];
};

struct Entry_004a0bf0 {
    char name[16];
    char unknown_10[331];
};

struct Class_004a0e00 {
    char unknown_0[24];
    Dialog* holder;
    char unknown_1c[72];
    int current;
    char unknown_68[12];
    int length;
    char unknown_78[3154];
    int changed;
};

struct Class_004a1530 {
    char unknown_0[24];
    Dialog* holder;
    char unknown_1c[3246];
    int f_cca;
};

struct Class_004a16f0 {
    char unknown_0[24];
    Dialog* holder;
    char unknown_1c[3246];
    int f_cca;
};

struct Class_004a1ab0 {
    char unknown_0[24];
    Dialog* holder;
};

struct Glyph_004a1b40 {
    unsigned short width;
    unsigned short height;
    short xoff;
    short yoff;
};

struct Class_004a2480 {
    char unknown_0[24];
    Dialog* holder;
};

struct Class_004a30c0 {
    char unknown_0[24];
    Dialog* holder;
};

struct Class_004a3eb0 {
    char unknown_0[24];
    Table_004b3630* table;
};

struct Class_004a4620 {
    char unknown_0[24];
    Table_004b3630* table;
};

struct Class_004a4980 {
    char unknown_0[24];
    Dialog* holder;
    char unknown_1c[2205];
    unsigned char field_8b9;
};

struct Frame_004a4980 {
    unsigned short w;
    unsigned short h;
    short field_4;
    short field_6;
    char unknown_8;
    unsigned char field_9;
};

struct Class_004a4c90 {
    char unknown_0[24];
    Dialog* holder;
};

struct Class_004a53c0 {
    char unknown_0[24];
    Dialog* holder;
};

struct Class_004a56b0 {
    char unknown_0[8];
    void* field_08;
    void* field_0c;
    char unknown_10[4];
    void* field_14;
    Dialog* holder;
    char unknown_1c[2198];
    unsigned char colours[256];
};

struct GafEntry_004a5f40 {
    unsigned short count;
    unsigned short unknown_2;
};

union Field136_004a6ae0 {
    union { short value; struct { unsigned char lo; unsigned char hi; } bytes; };
};

struct Holder_004a75d0 {
    char unknown_0[192];
    void* gaf;
};

struct Screen_004a75d0 {
    char unknown_0[4];
    Holder_004a75d0* holder;
};

struct Entry_004a7ee0 {
    char unknown_0[347];
};

struct Button_004a7f70 {
    char unknown_0[4];
    Gaf_004b8d40* gaf;
    char unknown_8[16];
    Screen_004a75d0* screen;
};

struct Frame_004a7f70 {
    unsigned short x;
    unsigned short y;
    unsigned short w;
    unsigned short h;
};

struct Obj_004a7f70 {
    char unknown_0[2];
    char name[21];
    short x;
    short y;
    int flags;
    char unknown_1f[16];
    GafEntry_004b8d40* entry;
    char unknown_33[259];
    unsigned char stage;
    char unknown_137[4];
    unsigned char frame;
};

struct Class_004a8150 {
    char unknown_0[24];
    Dialog* holder;
};

struct Struct_004a81b0 {
    char unknown_0[2742];
    char name[256];
};

struct EntryAnim_004a9fd0 {
    short count;
    char unknown_2[2];
    int field_ba;
    int field_be;
    int field_c2;
    int field_c6;
    float field_ca;
    int field_ce;
    int field_d2;
};

struct EntryTail_004a9fd0 {
    unsigned char field_136;
    unsigned char field_137;
    short field_138;
    char unknown_4[2];
    unsigned char field_13c;
    char unknown_7[9];
};

struct Entry_004aafe0 {
    char unknown_0[4];
    Info_004aafe0* info;
};

struct Info_004aafe0 {
    char unknown_0[19];
    short x;
    short y;
    short width;
    short height;
};

struct Object_004aafe0 {
    char unknown_0[24];
    Entry_004aafe0* entry;
    char unknown_1c[32];
    int x;
    int y;
};

struct Class_004ab040 {
    char unknown_0[24];
    int field_18;
};

struct Class_004ab2b0 {
    char unknown_0[24];
    Dialog* holder;
};

struct Flags_004ab400 {
    unsigned int active : 1;
    unsigned int rest : 31;
};

struct Obj_00462d30 {
    int a;
    int b;
    int c;
    short sC;
    short sE;
    short s10;
    short s12;
    int f14;
    int f18;
    int field_1c;
    int field_20;
    int field_24;
    unsigned int b0 : 1;
    unsigned int b1 : 1;
    unsigned int b2 : 1;
    unsigned int b3 : 1;
    unsigned int b4 : 1;
    unsigned int rest : 27;
    Inner_00421550* inner;
    Handle ref;
    char unknown_3c[24];
    int field_54;
    char unknown_58[4];
    Flags_004ab400 flags_5c;
    char unknown_60[2646];
    char name[256];
    Obj_00462d30(void);
};

struct Flags_004ab440 {
    unsigned int active : 1;
    unsigned int rest : 31;
};

struct Class_004a6a40 {
    int unknown_0;
    Entry* entries;
    void (__stdcall *fn)(void*, int);
    void* surface;
    int unknown_10;
    void* oldSurface;
    Dialog* holder;
    void (__cdecl *callback1c)(void);
    int y;
    int z;
    int unknown_28;
    Src_004ab400* src_2c;
    Handle ref;
    Arg_00419670 point;
    int field_54;
    char unknown_58[4];
    Flags_004ab440 flags_5c;
    int field_60;
    int focus;
    int field_68;
    int used;
    char unknown_70[4];
    int length;
    char unknown_78[30];
    int field_96;
    int field_9a;
    char unknown_9e[4];
    int field_a2;
    char unknown_a6[2];
    short field_a8;
    char unknown_aa[2056];
    unsigned char field_8b2;
    char unknown_8b3[11];
    unsigned char colour_8be;
    char unknown_8bf[2];
    unsigned char field_8c1;
    char unknown_8c2[1];
    unsigned char light;
    char unknown_8c4[2];
    unsigned char fill;
    char unknown_8c7[495];
    char dir[256];
    char unknown_bb6[272];
    int field_cc6;
    int changed;
    int field_cce;
    void* fallback;
};

struct Entry_004abd20 {
    char unknown_0[4];
    char* name;
};

struct Object_004abd20 {
    char unknown_0[24];
    Entry_004abd20* entry;
};

struct Class_004ac610 {
    char unknown_0[24];
    Dialog* holder;
};

struct PalEntry_004ac710 {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char flags;
};

struct Palette_004ac7d0 {
    char unknown_0[178];
    PalEntry_004ac710 dest[256];
    char unknown_4b2[1024];
    unsigned char table[256];
};

struct Class_004acc70 {
    char unknown_0[2482];
    int index;
    char unknown_9b6[768];
    Slider_004acc70* red;
    Slider_004acc70* green;
    Slider_004acc70* blue;
};

struct Color_004acc70 {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char flags;
};

struct Slider_004acc70 {
    char unknown_0[320];
    unsigned char value;
};

struct Class_004bbbe0 {
    _iobuf* file;
    int error;
    Class_004bbd00_Other* info;
    unsigned int pos;
    int* buffer;
    int* buffer2;
    char name[256];
};

struct Common_004ad350_2 {
    unsigned char id;
    unsigned char assoc;
    char name[17];
    short xpos;
    short ypos;
    short width;
    short height;
    int attribs;
    int colorf;
    int colorb;
    unsigned char texturenumber;
    unsigned char fontnumber;
    unsigned char active;
    unsigned char commonattribs;
    char unknown_2b[8];
    char help[129];
    unsigned short gaffile;
};

struct Obj_004ae170_2 {
    char unknown_0[182];
    char text[128];
    short range;
    char unknown_138[4];
    int thick;
    short knobpos;
    short knobsize;
    int field_144;
};

struct BlinkWord_004afc60 {
    char text[160];
    int value;
};

struct Class_004afc60 {
    char unknown_0[166];
    BlinkWord_004afc60* words;
    int count;
    int active;
};

struct Inner_004afd20 {
    char unknown_0[160];
    int field_a0;
};

struct Word_004afd80 {
    char text[128];
    int field_80;
    int field_84;
    int field_88;
    float field_8c;
    int field_90;
    float field_94;
    int field_98;
    float field_9c;
    int value;
};

struct Item_4afec0 {
    char used;
    char unknown_1[163];
};

struct Class_004aff00 {
    char unknown_0[24];
    Obj18_004aff00* field_18;
    char unknown_1c[138];
    Word_004aff00* words;
    int count;
    int active;
};

struct Obj18_004aff00 {
    char unknown_0[4];
    char* field_4;
};

struct Word_004aff00 {
    char text[128];
    int field_80;
    int field_84;
    int field_88;
    float field_8c;
    int field_90;
    float field_94;
    int field_98;
    float field_9c;
    int value;
};

class Class_004b07a0 {
public:
    char unknown_0[1];
    int GetCob(void);
};

class Class_004b0830 {
public:
    char unknown_0[8];
    NameTable_004b0830* table;
    int StartThreadByName(char*);
};

struct NameTable_004b0830 {
    int unknown_0;
    int count;
    char unknown_8[20];
    char** names;
};

struct Channel_004b08c0 {
    int used;
    int entry;
    int unknown_8;
    char unknown_c[16];
    int unknown_1c;
    int value;
    char unknown_24[128];
};

class Class_004b08c0 {
public:
    char unknown_0[8];
    Table_004b08c0* table;
    char unknown_c[16];
    Channel_004b08c0 channels[8];
    int activeCount;
    int StartThread(int);
};

struct Table_004b08c0 {
    char unknown_0[4];
    int count;
    char unknown_8[16];
    int* entries;
};

struct Channel_004b0940 {
    int used;
    int id;
    char unknown_8[24];
    int value;
    char unknown_24[128];
};

class Class_004b0a70 {
public:
    int unknown_4;
    NameTable_004b0830* table;
    char unknown_c[16];
    Channel_004b0940 channels[8];
    int activeCount;
    virtual ~Class_004b0a70(void);
    virtual void v00(void);
    virtual void v04(void);
    virtual void v08(void);
    virtual void v0c(void);
    virtual void v10(void);
    virtual void v14(void);
    virtual void v18(void);
    virtual void v1c(void);
    virtual void v20(void);
    virtual void v24(void);
    virtual void v28(void);
    virtual void v2c(void);
    virtual void v30(void);
    virtual void v34(void);
    virtual void v38(void);
    virtual void v3c(void);
    virtual void v40(void);
    virtual void v44(void);
    virtual void v48(void);
    virtual void v4c(void);
    int FindScript(char*);
    int StartScript(char*, int, int);
    int StartScriptWithArgs(char*, void*, int, int, int, int, int, int);
    int QueryScript(char*, int*, int*, int*, int*);
    void RunScripts(int);
};

class Class_004b1c00 {
public:
    int field_4;
    Table_004b3630* field_8;
    char unknown_c[8];
    Data_004b1c00* ptr14;
    int field_18;
    virtual void SetPieceTranslation(int, int, int) = 0;
    virtual void SetPieceRotation(int, int, int) = 0;
    virtual int SetPieceVisible(int, int) = 0;
    virtual int SetPieceCached(int, int) = 0;
    virtual int SetPieceShaded(int, int) = 0;
    virtual int GetPieceTranslation(int, int) = 0;
    virtual int GetPieceRotation(int, int) = 0;
    void AnimatePieces(int);
};

struct Channel_004b0a10 {
    int used;
    int id;
    int unknown_8;
    char unknown_c[16];
    int unknown_1c;
    int value;
    char unknown_24[128];
};

class Class_004b0a10 {
public:
    char unknown_0[28];
    Channel_004b0a10 channels[8];
    int activeCount;
    int StartScriptByIndex(int, int, int);
};

struct Channel_004b0b00 {
    char unknown_0[36];
    int sp;
    char unknown_28[20];
    int value;
    int stack[25];
};

class ScriptCallback {
public:
    virtual void StartScriptWithArgsByIndex(int);
};

struct Channel_004b0c40 {
    char unknown_0[36];
    int count;
    char unknown_28[20];
    int unknown_3c;
    int values[5];
    char unknown_54[80];
};

class Class_004b0c40 {
public:
    Channel_004b0c40 channels[8];
    int activeCount;
    int QueryScriptByIndex(int, int*, int*, int*, int*);
};

struct Entry_004b0d20 {
    unsigned int flags;
    char unknown_4[28];
    int handle;
    char unknown_24[128];
};

class Class_004b0d20 {
public:
    char unknown_0[28];
    Entry_004b0d20 entries[8];
    int guard_53c;
    void RemoveCallback(int);
};

#include "../src/units/cob_types.h"

class Class_004b0da0 {
public:
    int scale;
    ScriptTable* table;
    int unknown_c;
    int* statics;
    Piece* pieces;
    int changed;
    Channel channels[8];
    int activeCount;
    virtual void SetPieceTranslation(int, int, int) = 0;
    virtual void SetPieceRotation(int, int, int) = 0;
    virtual void SetPieceVisible(int, int) = 0;
    virtual void SetPieceCached(int, int) = 0;
    virtual void SetPieceShaded(int, int) = 0;
    virtual int GetPieceTranslation(int, int) = 0;
    virtual int GetPieceRotation(int, int) = 0;
    virtual int IsPieceVisible(int);
    virtual int IsPieceCached(int);
    virtual int IsPieceShaded(int);
    virtual void ExplodeLegacy(int, int, int);
    virtual void PlaySoundNoop(int);
    virtual void EmitSfx(int, int);
    virtual void ExplodePiece(int, int);
    virtual void AttachUnit(int, int, int);
    virtual void DropUnit(int);
    virtual void SetUnitValue(int, int);
    virtual int GetUnitValue(int, int, int, int, int);
    virtual int IsCarryingUnit(int);
    virtual int GetTransporterId(void);
    void RunThread(unsigned int, int);
    void Wake(unsigned int);
};

struct Data_004b1c00 {
    int flag;
    int e[6][3];
};

struct Big_004b1ec0 {
    int magic;
    Elem_4b0610 recs[8];
    int tail;
};

struct Data_004b1ec0 {
    int flag;
    int e[6][3];
};

struct Vec3_004b1ec0 {
    int v[3];
};

struct Block_004b1ec0 {
    Vec3_004b1ec0 e[6];
};

struct Rec2_004b1ec0 {
    Block_004b1ec0 block;
    int a[3];
    int b[3];
    int h0;
    int h1;
    int h2;
};

struct Rec2_004b2040 {
    int e[6][3];
    int a[3];
    int b[3];
    int h[3];
};

struct Alloc_004b2290 {
    char unknown_0[1];
};

struct Comp_004b2290 {
    char unknown_0[1];
};

class Class_004b2290 {
public:
    Comp_004b2290 comp;
    Alloc_004b2290 alloc;
    char unknown_2[2];
    LiveNode* head;
    char flag;
    char unknown_9[3];
    int size;
    Class_004b2290(Comp_004b2290&, Alloc_004b2290&);
    ~Class_004b2290(void);
};

class Class_004b2fb0 {
public:
    char alloc;
    char comp;
    char unknown_2[2];
    LiveNode* head;
    char multi;
    char unknown_9[3];
    int size;
    ~Class_004b2fb0(void);
    void FUN_004b2fb0(LiveNode*);
    void _Freenode(LiveNode*);
    int erase(int&);
};

class Class_004b3590 {
public:
    LiveNode* ptr;
    void FUN_004b3590(void);
};

class Map_004b2540 {
public:
    Class_004b2fb0 _Tr;
    int erase(int&);
};

struct IntLess_004b3430 {
    char unknown_0[1];
    unsigned char operator()(int&, int&);
};

class Class_004b3430 {
public:
    char allocator;
    IntLess_004b3430 key_compare;
    char unknown_2[2];
    LiveNode* head;
    LiveNode* FUN_004b3430(int&);
};

struct IntLess_004b3490 {
    char unknown_0[1];
    unsigned char operator()(int&, int&);
};

class Class_004b3490 {
public:
    char allocator;
    IntLess_004b3490 key_compare;
    char unknown_2[2];
    LiveNode* head;
    LiveNode* FUN_004b3490(int&);
};

struct Item_004b3630_10 {
    void* field_0;
    int type;
    int value;
    int unknown_c;
};

struct Image_004b3770 {
    void* buf;
    int size;
};

struct Buffer_004b39c0 {
    char* data;
    int len;
    int csize;
};

class Class_004b4d70 {
public:
    void* field_0;
    void WriteAuditFile(char*);
};

struct BlobRec_004b4270 {
    char* name;
    int id;
    int offset;
    int len;
};

class Class_004b46c0 {
public:
    Table_004b3630* table;
    int SetDoubleItem(char*, double);
};

class Class_004b49d0 {
public:
    Table_004b3630* table;
    int FindNumberedBox(int, int);
};

class Class_004b4a80 {
public:
    Table_004b3630* table;
    int FindNamedBox(char*, int);
};

struct DblRec_004b4270 {
    char* name;
    double value;
};

struct IntRec_004b4270 {
    char* name;
    int value;
};

struct File_004b4800 {
    char unknown_0[4];
    Section_004b4800* sections;
    int current;
};

struct Section_004b4800 {
    char unknown_0[16];
    Item_004b3630_10* values;
    char unknown_14[4];
};

class Class_004b4850 {
public:
    File_004b4850* file;
    double GetDoubleItem(char*, double);
};

struct File_004b4850 {
    char unknown_0[4];
    Section_004b4850* sections;
    int current;
};

struct Section_004b4850 {
    char unknown_0[16];
    Item_004b3630_10* values;
    char unknown_14[4];
};

struct File_004b4910 {
    char unknown_0[4];
    Entry* sections;
    int current;
};

struct ArrayB {
    char unknown_0[8];
    int value;
    char unknown_c[8];
};

class Class_004b4c50 {
public:
    BuildList_0045a950* data;
    int SeekBoxEnd(void);
};

struct Account_004b4d70 {
    char* name;
    int item_count;
    int box_count;
    char unknown_c[4];
    Item_004b4d70* items;
    Box_004b4d70* boxes;
};

struct Bank_004b4d70 {
    int account_count;
    Account_004b4d70* accounts;
};

struct Box_004b4d70 {
    int named;
    int value;
    int size;
    int unknown_c;
    int unknown_10;
};

struct Item_004b4d70 {
    char* name;
    int type;
    int value;
    int value_high;
};

struct DXVersion {
    unsigned int majhi;
    unsigned int majlo;
    unsigned int minhi;
    unsigned int minlo;
    DXVersion(void);
};

struct Class_004b52e0 {
    char unknown_0[490];
    int clipLeft;
    int clipTop;
    int clipRight;
    int clipBottom;
    int width;
    int height;
    unsigned short flags : 10;
    unsigned short rest : 6;
    char unknown_204[1040];
    float scale;
};

struct BitmapInfo_004b5510 {
    tagBITMAPINFOHEADER bmiHeader;
    tagRGBQUAD bmiColors[256];
};

struct DirectDrawState {
    void* ddraw;
    void* primary;
    void* back;
    void* clipper;
    void* palette;
    void* field_98;
    int field_9c;
};

union Flags_4b5980 {
    union { unsigned short value; struct { unsigned short opt : 1; unsigned short bit1 : 1; unsigned short gdi : 1; unsigned short bit3 : 1; unsigned short bit4 : 1; unsigned short has_c0 : 1; unsigned short has_c4 : 1; unsigned short has_c8 : 1; unsigned short has_cc : 1; unsigned short has_d0 : 1; unsigned short sound_opt : 1; unsigned short no_video : 1; unsigned short bit12 : 4; } bits; };
};

struct Mode_4b5980 {
    int unknown_00;
    int unknown_04;
    int unknown_08;
    int unknown_0c;
};

struct View_4b5980 {
    int x;
    int y;
    int z;
    int unknown_0c;
    int unknown_10;
    int unknown_14;
};

struct Event_4b5cc0 {
    int data[6];
};

struct GlobalObj {
    char unknown_0[232];
    int field_e8;
};

struct FrameCounter_4b6570 {
    int accum;
    unsigned int lastTick;
    int frames;
    int rate;
    void Tick(void);
};

struct Obj {
    void* hInstance;
    int nCmdShow;
    char* className;
    char* title;
    long (__stdcall *wndProc)(void*, unsigned int, unsigned int, long);
    unsigned short menuId;
    char unknown_16[2];
    tagWNDCLASSA wc;
    void* hwnd;
    void* old_palette;
    void* dc;
    void* hpalette;
    char unknown_50[48];
    void (__cdecl *callback)(int, int, int);
    void* ddraw;
    void* primary;
    Surface_4b6570* surface;
    void* field_90;
    void* palette;
    char unknown_98[8];
    Mode_4b5980 mode;
    void* font;
    char unknown_b4[12];
    void* ptr_at_0xc0;
    void* field_c4;
    void* light_table;
    void* gray_table;
    void* ptr_at_0xd0;
    int width;
    int height;
    int unknown_dc;
    int active;
    char unknown_e4[4];
    unsigned int rate;
    int wa_left;
    Flags_4b5980 flags;
    char wa_rest[10];
    char unknown_fc[182];
    int field_1b2;
    char unknown_1b6[36];
    FrameCounter_4b6570 counter;
    Mode_4b5980 startMode;
    int startWidth;
    int startHeight;
    unsigned short videoFlags;
    char unknown_204[16];
    tagPALETTEENTRY entries[256];
    char unknown_614[4];
    void** items;
    int item_count;
    int availPhys;
    int unknown_624;
    char unknown_628[256];
    unsigned char unknown_728;
};

class Surface_4b6570 {
public:
    virtual int Slot_00(void);
    virtual int Slot_04(void);
    virtual int Slot_08(void);
    virtual int Slot_0c(void);
    virtual int Slot_10(void);
    virtual int Slot_14(void);
    virtual int Slot_18(void);
    virtual int Slot_1c(void);
    virtual int Slot_20(void);
    virtual int Slot_24(void);
    virtual int Slot_28(void);
    virtual int Slot_2c(void);
    virtual int Slot_30(void);
    virtual int Slot_34(void);
    virtual int Slot_38(void);
    virtual int Slot_3c(void);
    virtual int Slot_40(void);
    virtual int Slot_44(void);
    virtual int Slot_48(void);
    virtual int Slot_4c(void);
    virtual int Slot_50(void);
    virtual int Slot_54(void);
    virtual int Slot_58(void);
    virtual int Slot_5c(void);
    virtual int Slot_60(void);
    virtual int Slot_64(void);
    virtual int __stdcall EndDraw(void*);
};

struct GlobalObj_2 {
    char unknown_0[212];
    int field_d4;
};

struct GlobalObj_3 {
    char unknown_0[216];
    int field_d8;
};

class Class_004b7540 {
public:
    char* args[52];
    int count;
    void ShiftArgs(int);
};

struct Elem_004b75d0 {
    Class_004c9390 name;
    int value1;
    int value2;
    ~Elem_004b75d0(void);
};

struct Elem_004b75d0_2 {
    Class_004c9390 name;
    int value1;
    int value2;
    ~Elem_004b75d0_2(void);
};

class Class_004b7e30 {
public:
    Class_004c9390 handle;
    int value1;
    int value2;
    Class_004b7e30(Class_004b7e30&);
    Class_004b7e30(Class_004c9390&, Pair_00419560);
    ~Class_004b7e30(void);
    Class_004b7e30& operator=(Class_004b7e30&);
};

struct NameLess_004b7620 {
    char unknown_0[1];
    unsigned char operator()(char*, char*);
};

struct NameNe_004b7620 {
    char unknown_0[1];
    unsigned char operator()(Class_004c9390&, Class_004c9390&);
};

struct NameLess_004b7760 {
    char unknown_0[1];
    unsigned char operator()(char*, char*);
};

struct NameNe_004b7760 {
    char unknown_0[1];
    unsigned char operator()(Class_004c9390&, Class_004c9390&);
};

struct ConsoleCommand {
    char* name;
    void (__stdcall *fn)(void*);
    int mask;
};

class Class_004c9290 {
public:
    char* data;
    Class_004c9290* MakeLower(void);
    char* GetUnique(void);
};

struct Elem_004b75d0_3 {
    Class_004c9390 name;
    int value1;
    int value2;
    ~Elem_004b75d0_3(void);
};

struct HandlerSlot_004b7900 {
    void (__stdcall *fn)(void*);
    int mask;
};

struct NameLess_004b7900 {
    char unknown_0[1];
    unsigned char operator()(char*, char*);
};

struct Elem_004b75d0_4 {
    Class_004c9390 name;
    int value1;
    int value2;
    ~Elem_004b75d0_4(void);
};

class CommandEntry {
public:
    char unknown_0[4];
    int field1;
    int field2;
    void* AssignEntry(int*);
};

struct Display_004b8310 {
    char unknown_0[200];
    unsigned char* lightTable;
    char unknown_cc[36];
    unsigned char flags;
    char unknown_f1[3];
};

struct Src_004b8310 {
    int width;
    int height;
    int pitch;
    Bitmap_00437b50** pixels;
};

struct Display_004b8500 {
    char unknown_0[192];
    unsigned char* field_c0;
    char unknown_c4[44];
    unsigned char flags;
    char unknown_f1[3];
};

struct Screen_004b8500 {
    char unknown_0[32];
    Surface surf;
};

struct Display_004b86e0 {
    char unknown_0[204];
    unsigned char* field_cc;
    char unknown_d0[32];
    unsigned short flags;
    char unknown_f2[2];
};

struct Src_004b8a80 {
    unsigned short width;
    unsigned short height;
    unsigned short x;
    unsigned short y;
    char unknown_8[8];
    int bits;
};

struct Entry_004b8b30 {
    unsigned short value;
    char unknown_2[6];
};

struct GafEntry_004b8d40 {
    unsigned short count;
    unsigned char kind;
    char unknown_3[5];
    char name[1];
    char unknown_9[31];
    Bitmap_00437b50* child[1];
    Entry_004b8b30 entries[1];
};

struct Entry_5 {
    int off;
    int pad;
};

struct Blk {
    unsigned short count;
    char unknown_2[8];
    unsigned char subcount;
    char unknown_b[29];
    Entry_5 e[2];
};

struct Desc_004b8ec0 {
    int field_0;
    int field_4;
    int field_8;
    void* field_c;
    char unknown_10[32];
};

struct Display_004b8ec0 {
    char unknown_0[200];
    int field_c8;
    char unknown_cc[36];
    unsigned short unknown_bits : 5;
    unsigned short has_obj_c0 : 1;
    unsigned short has_obj_c4 : 1;
    unsigned short has_obj_c8 : 1;
    unsigned short : 8;
    char unknown_f2[2];
};

struct Sprite_004b94c0 {
    unsigned short width;
    unsigned short height;
    short x;
    short y;
    char unknown_8[8];
    unsigned char* bits;
};

struct Palette_004b95a0 {
    char unknown_0[192];
    unsigned char* table;
};

struct Palette_004b96e0 {
    char unknown_0[208];
    unsigned char* remap;
};

struct Display_004b9740 {
    char unknown_0[192];
    unsigned char* blend;
};

struct Display_004ba200 {
    char unknown_0[68];
    int field_44;
    void* dc;
    void* palette;
    char unknown_50[68];
    void* ddPalette;
    char unknown_98[88];
    unsigned short pad_f0 : 2;
    unsigned short bit2 : 1;
    unsigned short rest_f0 : 13;
    char unknown_f2[290];
    unsigned int entries[256];
};

struct Display_004ba4c0 {
    char unknown_0[68];
    int field_44;
    char unknown_48[4];
    void* hpalette;
    char unknown_50[68];
    void* palette;
    char unknown_98[88];
    unsigned short bit0_1 : 2;
    unsigned short flag2 : 1;
    unsigned short bit3_15 : 13;
};

struct RGBX_004ba4c0 {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char x;
};

struct Obj_004ba590 {
    char unknown_0[532];
    unsigned char palette[1024];
    int field_614;
};

struct Obj_004ba750 {
    char unknown_0[192];
    unsigned int* buffer;
    char unknown_c4[44];
    unsigned short bit0_4 : 5;
    unsigned short flag5 : 1;
    unsigned short bit6_15 : 10;
};

struct Obj_004baad0 {
    char unknown_0[192];
    unsigned int* buffer;
    char unknown_c4[44];
    unsigned short bit0_4 : 5;
    unsigned short flag5 : 1;
    unsigned short bit6_15 : 10;
};

struct Obj_5 {
    char unknown_0[196];
    unsigned int* buffer;
    char unknown_c8[39];
    char pad_odd;
    unsigned short bit0_5 : 6;
    unsigned short flag6 : 1;
    unsigned short bit7_15 : 9;
};

struct Obj_004bab30 {
    char unknown_0[200];
    unsigned int* buffer;
    char unknown_cc[36];
    unsigned short bit0_6 : 7;
    unsigned short flag7 : 1;
    unsigned short bit8_15 : 8;
};

struct Obj_004bab60 {
    char unknown_0[204];
    unsigned int* buffer;
    char unknown_d0[32];
    unsigned short bit0_7 : 8;
    unsigned short flag8 : 1;
    unsigned short bit9_15 : 7;
};

struct Obj_004baba0 {
    char unknown_0[208];
    unsigned int* buffer;
    char unknown_d4[28];
    unsigned short bit0_8 : 9;
    unsigned short flag9 : 1;
    unsigned short bit10_15 : 6;
};

struct Obj_004babd0 {
    char unknown_0[200];
    unsigned char* buffer;
    char unknown_cc[36];
    unsigned short bit0_6 : 7;
    unsigned short flag7 : 1;
    unsigned short bit8_15 : 8;
};

struct Obj_004badf0 {
    char unknown_0[196];
    unsigned char* buffer;
    char unknown_c8[40];
    unsigned short bit0_5 : 6;
    unsigned short flag6 : 1;
    unsigned short bit7_15 : 9;
};

struct Item_004bb2e0 {
    _iobuf* fp;
    int pos;
    Node_004bb2e0* node;
    int refCount;
    int field_10;
    char name[256];
};

struct Node_004bb2e0 {
    char unknown_0[12];
    unsigned char obfuscate;
    char unknown_d[3];
    List_004be3b0* list;
};

struct State_004bb2e0 {
    char unknown_0[1560];
    Record_00474cd0** records;
    int numRecords;
};

struct Class_004bb650 {
    char unknown_0[4];
    int field_4;
};

class Class_004bb670 {
public:
    char unknown_0[24];
    char name[256];
    void SetFileName(char*);
};

struct Class_004bbc40 {
    Class_004bbc40_File* file;
    Class_004bbc40_Entry* entry;
    Class_004bbc40_Other* other;
    int unknown_c;
    int unknown_10;
    int unknown_14;
};

struct Class_004bbc40_Entry {
    _iobuf* fp;
    int unknown_4;
    int unknown_8;
    int refcount;
    int unknown_10;
};

struct Class_004bbc40_File {
    char unknown_0[16];
    int fd;
};

struct Class_004bbc40_Other {
    char unknown_0[4];
    long value;
};

struct Class_004bbd00_File {
    char unknown_0[16];
    int fd;
};

struct Fp_004bbff0 {
    char unknown_0[16];
    int fd;
};

struct Find_004bc8d0 {
    char dir[256];
    char pattern[256];
    int state;
    char recursive;
    long handle;
    int index;
};

struct Group_004bc640 {
    char unknown_0[8];
    Path_004bc640* path;
};

struct Groups_004bc640 {
    char unknown_0[1560];
    Group_004bc640** group;
    int count;
};

struct Path_004bc640 {
    char unknown_0[16];
    List_004be3b0* list;
};

struct FindData_004bc930 {
    unsigned char attributes;
    char unknown_1[19];
    char name[260];
};

struct FindHandle_004bc930 {
    char unknown_0[512];
    int field_200;
    unsigned char field_204;
    int field_205;
};

struct FindData_004bca30 {
    int attr;
    char unknown_4[16];
    char name[260];
};

struct FindData_004bcd10 {
    int flags;
    char unknown_4[16];
    char name[260];
};

struct Class_004bce10 {
    char unknown_0[1576];
    char cwd[256];
};

struct Struct_004bcea0 {
    char unknown_0[1832];
    char name[256];
};

struct File_004bdd70 {
    _iobuf* fp;
    int field_4;
    Header_00421620* header;
    int field_c;
    int mode;
    char path[260];
};

struct Class_004be010 {
    int count;
    int base;
};

struct ArchiveEntry {
    int a;
    int b;
    unsigned char flags;
    char unknown_9[3];
};

struct File_004be070 {
    _iobuf* file;
    int unknown_4;
    int* buffer;
};

struct Display_004be0b0 {
    char unknown_0[1560];
    int* files;
    int count;
};

struct Display_004be400 {
    char unknown_0[1560];
    File_004be400** files;
    int count;
};

struct File_004be400 {
    char unknown_0[8];
    Header_004be400* header;
};

struct Header_004be400 {
    char unknown_0[16];
    List_004be3b0* table;
};

struct Point_004bf060 {
    int x;
    int y;
};

struct Point_004bf260 {
    int x;
    int y;
};

struct Game_004bfe10 {
    char unknown_0[204];
    int field_cc;
    char unknown_d0[33];
    unsigned char flags;
    char unknown_f2[2];
};

struct Point_004c0330 {
    int x;
    int y;
};

struct Span_004c0a90 {
    int x1;
    int x2;
    char unknown_8[16];
    int z1;
    int z2;
    int s1;
    int s2;
};

struct Point_004c07b0 {
    int x;
    int y;
    int z;
};

struct Obj_6 {
    char unknown_0[528];
    int field_210;
};

struct Obj_7 {
    char unknown_0[520];
    int field_208;
};

struct Obj_8 {
    char unknown_0[524];
    int field_20c;
};

struct Game_004c14f0 {
    char unknown_0[516];
    Font_004a50b0* font;
    int colour1;
    int colour2;
    int colour3;
};

struct Game_004c1830 {
    char unknown_0[516];
    Font_004a50b0* font;
};

struct Queue_004c1ab0_2 {
    char unknown_0[242];
    int size;
    int entries[30];
    int head;
    int tail;
};

struct Queue_004c1ab0_3 {
    char unknown_0[242];
    int size;
    int entries[30];
    int head;
    int tail;
};

struct Queue_004c1ab0_4 {
    char unknown_0[242];
    int size;
    int entries[30];
    int head;
    int tail;
};

struct Queue_004c1ab0_5 {
    char unknown_0[242];
    int size;
    int entries[30];
    int head;
    int tail;
};

struct DisplayContext {
    char unknown_0[466];
    int field_at_0x1d2;
};

struct Rect_004c2380 {
    int x;
    int y;
    int data[4];
};

struct Obj_004c23e0 {
    char unknown_0[438];
    int x;
    int y;
    Surface* bitmap;
    char unknown_1c2[16];
    int visible;
};

struct Obj_004c2470 {
    char unknown_0[430];
    int count;
    char unknown_1b2[4];
    int x;
    int y;
    void* image;
    char unknown_1c2[12];
    int mode;
};

struct Obj_004c24b0 {
    char unknown_0[406];
    int rect_x;
    int rect_y;
    char unknown_19e[20];
    Bitmap_00437b50* bmp;
    int x;
    int y;
    int* field_1be;
    char unknown_1c2[16];
    int visible;
};

struct App_004c25e0 {
    char unknown_0[406];
    int cursorX;
    int cursorY;
    char unknown_19e[20];
    Bitmap_00437b50* sprite;
    int savedX;
    int savedY;
    Surface* saved;
    Surface* under;
    Surface* work;
    char unknown_1ca[8];
    int enabled;
};

struct Rect_004c2870 {
    int x;
    int y;
    int data[4];
};

struct Obj_004c2870 {
    char unknown_0[406];
    Rect_004c2870 rect;
    int count;
    Bitmap_00437b50* bmp;
    int x;
    int y;
    int* field_1be;
    char unknown_1c2[12];
    int mode;
};

struct Struct_004c2ac0 {
    char unknown_0[458];
    int active;
    int running;
    char unknown_1d2[4];
    int pending;
};

struct Struct_004c2ac0_2 {
    char unknown_0[458];
    int active;
    char unknown_1ce[8];
    int pending;
};

struct Obj_004c2bd0 {
    char unknown_0[390];
    int capacity;
    int entries;
    int head;
    int tail;
    char unknown_196[24];
    int active;
    int unknown_1b2;
    char unknown_1b6[8];
    int obj_1be;
    int obj_1c2;
    int obj_1c6;
    int running;
    int unknown_1ce;
    int unknown_1d2;
    int pending;
};

struct Input_004c2cc0 {
    char unknown_0[240];
    unsigned short bit0_9 : 10;
    unsigned short active : 1;
    unsigned short bit11_15 : 5;
    char unknown_f2[152];
    void* entries;
    char unknown_18e[48];
    void* obj_1be;
    void* obj_1c2;
    void* obj_1c6;
    int running;
    char unknown_1ce[8];
    int stopping;
};

struct Queue_4c2d60 {
    char unknown_0[390];
    int capacity;
    Arg_00419670* entries;
    int head;
    int tail;
    Arg_00419670 empty;
};

struct Queue_4c2de0 {
    char unknown_0[390];
    int capacity;
    Arg_00419670* entries;
    int head;
    int tail;
    Arg_00419670 empty;
};

struct Queue_4c2e30 {
    char unknown_0[390];
    int capacity;
    Event_4b5cc0* entries;
    int head;
    int tail;
    Event_4b5cc0 empty;
};

class Class_004c33a0 {
public:
    char unknown_0[1];
    void StripComments(char*);
};

class Class_004c42a0 {
public:
    int* name;
    std::vector<Class_004c42a0*> children;
    char unknown_14;
    std::vector<TdfField> entries;
    ~Class_004c42a0(void);
    Class_004c42a0* FUN_004c3e40(char*, char*, int, char*);
    Class_004c42a0* GetChild(int);
};

struct Inner_004c51b0 {
    std::vector<TdfField> v;
};

struct Entries_004c51b0 {
    Inner_004c51b0 v;
};

class Class_004e8230 {
public:
    char* _m_what;
    int _m_doFree;
    virtual ~Class_004e8230(void);
    Class_004e8230(Class_004e8230&);
    Class_004e8230(void);
    virtual char* what(void);
    virtual void _Doraise(void);
};

class Class_004c4340 {
public:
    char unknown_0[1];
    Class_004c9390 MakeTrimmedString(char*, char*);
    Class_004c9390* MakeTrimmedString(Class_004c9390*, char*, char*);
};

class Class_004c54a0 {
public:
    Class_004c9390 first;
    Class_004c9390 second;
    Class_004c54a0(Class_004c54a0&);
    Class_004c54a0& operator=(Class_004c54a0&);
};

class Class_004c54d0 {
public:
    Class_004c9390 field_0;
    Class_004c9390 field_4;
    Class_004c54d0(Class_004c54d0&);
    Class_004c54d0(Class_004c9390&, Class_004c9390&);
};

class Class_004c9180 : public Class_004c9390 {
public:
    Class_004c9180(Class_004c9180&);
    Class_004c9180(void);
    ~Class_004c9180(void);
};

class Map_004c3e40 {
public:
    char less;
    std::vector<Class_004c54a0> v;
    Class_004c54a0* LowerBound(char*);
    Class_004c9390* InsertNew(Class_004c54a0*, Class_004c9390&);
};

class Class_004c3e40 {
public:
    int field_0;
    std::vector<Class_004c3e40*> children;
    Map_004c3e40 entries;
    char* text;
    Class_004c3e40(Class_004c3e40&);
    Class_004c3e40(char*, char*, char**, char*);
};

class Class_004c43f0 {
public:
    char unknown_0[37];
    int field_25;
    void ComputeRecordChecksum(unsigned int, unsigned int);
};

struct Named_004c4450 {
    char* name;
};

class Class_004c45c0 {
public:
    char unknown_0[25];
    int field_19;
    int field_1d;
    int GetFieldCount(void);
};

struct NameLess_004c4630 {
    char unknown_0[1];
    unsigned char operator()(char*, char*);
};

struct NameLess_004c46c0 {
    char unknown_0[1];
    unsigned char operator()(char*, char*);
};

struct NameLess_004c4760 {
    char unknown_0[1];
    unsigned char operator()(char*, char*);
};

class Class_004c5470 {
public:
    char unknown_0[8];
    void* AssignPair(int*);
};

class TranslationTable {
public:
    char unknown_0;
    std::vector<Pair_00419560> pairs;
    TranslationTable(char);
};

class TdfFieldVector {
public:
    char count;
    char unknown_1[3];
    int field_4;
    int field_8;
    TdfField* end;
    TdfField* Insert(TdfField*, TdfField&);
    int GetPairCount(void);
};

struct Class_004c5c60 {
    char unknown_0[5];
    TdfField* first;
    TdfField* last;
    TdfField* FindLowerBound(char*);
};

class Class_004c93f0 {
public:
    char* ptr;
    Class_004c93f0* AssignText(char*);
};

struct Elem_004c5740 {
    char* key;
    char* value;
};

struct LockEntry_004c5d90 {
    void* surface;
    char flag;
};

struct Screen_004c5df0 {
    char unknown_0[12];
    void* surface;
    void UnlockSurface(void);
};

struct Display_004c5df0 {
    char unknown_0[68];
    int field_44;
    char unknown_48[56];
    Screen_004c5df0 screen;
    char unknown_90[76];
    int field_dc;
};

struct Surface_004c5df0 {
    int data[12];
};

struct Display_004c5e70 {
    char unknown_0[68];
    int field_44;
    char unknown_48[8];
    Surface cached;
    char unknown_80[32];
    Rect vec;
    char unknown_b0[12];
    Surface* field_bc;
    char unknown_c0[20];
    int field_d4;
    int field_d8;
    int field_dc;
    int LockMe(Surface*);
};

struct Screen_004c5fa0 {
    char unknown_0[12];
    void* surface;
    void UnlockSurface(void);
};

struct Display_004c5fa0 {
    char unknown_0[68];
    int field_44;
    char unknown_48[56];
    Screen_004c5fa0 screen;
    char unknown_90[76];
    int field_dc;
};

struct Block_004c5ff0 {
    int a;
    int b;
    int c;
    int d;
};

struct Out_004c5ff0 {
    int field_0;
    int field_4;
    int field_8;
    int field_c;
    int pad[8];
};

struct Display_004c5ff0 {
    char unknown_0[68];
    int field_44;
    char unknown_48[8];
    Out_004c5ff0 cached;
    char unknown_80[32];
    Block_004c5ff0 block;
    char unknown_b0[36];
    int field_d4;
    int field_d8;
    int LockMe(Out_004c5ff0*);
};

struct Screen_004c60d0 {
    void* surface;
    void Unlock(void);
};

struct Display_004c60d0 {
    char unknown_0[64];
    void* hwnd;
    void* offscreenDC;
    void* sourceDC;
    void* hpalette;
    char unknown_50[56];
    Screen_004c60d0 screen;
};

struct Obj_004c61b0 {
    char unknown_0[156];
    int field_9c;
    char unknown_a0[80];
    unsigned short flag0 : 1;
    unsigned short bit1_15 : 15;
};

struct Class_004c61f0 {
    char unknown_0[152];
    int field_98;
};

struct Arg_004c6210 {
    int unknown_0;
    Intf_004c6210* field_4;
    Intf_004c6210* field_8;
};

class Intf_004c6210 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall Slot18(void);
    virtual int __stdcall Slot19(void);
    virtual int __stdcall Slot20(void);
    virtual int __stdcall Slot21(void);
    virtual int __stdcall Slot22(void);
    virtual int __stdcall Slot23(void);
    virtual int __stdcall Slot24(void);
    virtual int __stdcall Slot25(void);
    virtual int __stdcall Slot26(void);
    virtual int __stdcall Slot27(void);
};

struct Screen_004c6210 {
    char unknown_0[12];
    void* surface;
    void UnlockRect(tagRECT*);
};

struct Display_004c6210 {
    char unknown_0[68];
    int field_44;
    char unknown_48[56];
    Screen_004c6210 screen;
    char unknown_90[8];
    Surface* field_98;
    char unknown_9c[64];
    int field_dc;
};

struct Screen_004c62c0 {
    char unknown_0[8];
    void* field_88;
    void* surface;
    void UnlockSurface(void);
};

struct Display_004c62c0 {
    char unknown_0[68];
    int field_44;
    char unknown_48[56];
    Screen_004c62c0 screen;
    char unknown_90[8];
    void* field_98;
    char unknown_9c[24];
    void* field_b4;
    char unknown_b8[36];
    int field_dc;
    char unknown_e0[16];
    unsigned short bit0 : 1;
    unsigned short flag1 : 1;
    unsigned short bit2_15 : 14;
};

struct Desc {
    unsigned long dwSize;
    unsigned long dwFlags;
    unsigned long height;
    unsigned long width;
    long lPitch;
    unsigned long backbuffers;
    unsigned long mipmaps;
    unsigned long alpha;
    unsigned long reserved;
    void* lpSurface;
    char fields[68];
};

struct Screen_004c63a0 {
    char unknown_0[8];
    void* primary;
    void* surface;
    char unknown_10[8];
    void UnlockSurface(void);
};

struct Display_004c63a0 {
    char unknown_0[64];
    void* hwnd;
    int field_44;
    void* srcDC;
    void* palette;
    Out_004c5ff0 cached;
    Screen_004c63a0 screen;
    Surface* field_98;
    int field_9c;
    char unknown_a0[28];
    Surface* field_bc;
    char unknown_c0[20];
    int field_d4;
    int field_d8;
    int field_dc;
    char unknown_e0[16];
    unsigned short flags;
    char unknown_f2[164];
    int rect_x;
    int rect_y;
    char unknown_19e[20];
    Surface* field_1b2;
    int x;
    int y;
    int* field_1be;
    char unknown_1c2[12];
    int field_1ce;
    int field_1d2;
};

struct Class_004c6890 {
    char unknown_0[84];
    int field_54;
    int field_58;
    char* pixels;
    char unknown_60[44];
    Driver_004c6890* driver;
    char unknown_90[44];
    Surface* surface;
    char unknown_c0[28];
    int field_dc;
    char unknown_e0[16];
    unsigned char flags;
    char unknown_f1[3];
};

struct Driver_004c6890 {
    Table_004b3630* table;
};

struct Fade_004c6890 {
    int amount;
    char unknown_4[76];
    int colour;
    char unknown_54[16];
};

struct Struct_004c6ac0 {
    char unknown_0[44];
    unsigned char flags;
};

struct Screen_004c6b70 {
    char unknown_0[12];
    void* surface;
    void UnlockSurface(void);
};

struct Display_004c6b70 {
    char unknown_0[68];
    int field_44;
    char unknown_48[56];
    Screen_004c6b70 screen;
    char unknown_90[76];
    int field_dc;
};

struct Screen_004c6c50 {
    char unknown_0[12];
    void* surface;
    void UnlockSurface(void);
};

struct Display_004c6c50 {
    char unknown_0[68];
    int field_44;
    char unknown_48[56];
    Screen_004c6c50 screen;
    char unknown_90[76];
    int field_dc;
};

struct Screen_004c6d20 {
    char unknown_0[12];
    void* surface;
    void UnlockSurface(void);
};

struct Display_004c6d20 {
    char unknown_0[68];
    int field_44;
    char unknown_48[56];
    Screen_004c6d20 screen;
    char unknown_90[76];
    int field_dc;
};

struct Screen_004c6dc0 {
    char unknown_0[12];
    void* surface;
    void UnlockSurface(void);
};

struct Display_004c6dc0 {
    char unknown_0[68];
    int field_44;
    char unknown_48[56];
    Screen_004c6dc0 screen;
    char unknown_90[76];
    int field_dc;
};

struct Screen_004c6e70 {
    char unknown_0[12];
    void* surface;
    void UnlockSurface(void);
};

struct Display_004c6e70 {
    char unknown_0[68];
    int field_44;
    char unknown_48[56];
    Screen_004c6e70 screen;
    char unknown_90[76];
    int field_dc;
};

struct Surface_004c6f80 {
    int width;
    int height;
    int pitch;
    char* data;
    int field_10;
    int field_14;
    short field_18;
    short field_1a;
    Rect clip;
    unsigned int flag0 : 1;
    unsigned int flag1 : 1;
    unsigned int : 30;
    unsigned char pixels[1];
    char unknown_31[3];
};

struct Range_2 {
    int low;
    int high;
};

struct Info_4c7310 {
    unsigned short bits;
    char unknown_2[14];
    unsigned char* data;
};

struct Frame_004c7580 {
    unsigned short w;
    unsigned short h;
    char unknown_4[12];
    void* data;
};

struct Display_004c8020 {
    char unknown_0[196];
    unsigned char* palette;
};

class Class_004c9230 {
public:
    char* ptr;
    Class_004c9230(char*, int);
};

class Class_004c9310 {
public:
    char* data;
    char* GetUnique(void);
    Class_004c9310* MakeUpper(void);
};

class Class_004c9490 {
public:
    char* ptr;
    Class_004c9490(char*, int);
    Class_004c9490(void);
    Class_004c9490 SubString(int, int);
};

class DirectPlay_4c97b0 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall Slot18(void);
    virtual int __stdcall Slot19(void);
    virtual int __stdcall Slot20(void);
    virtual int __stdcall Slot21(void);
    virtual int __stdcall Slot22(void);
    virtual int __stdcall Slot23(void);
    virtual int __stdcall Slot24(void);
    virtual int __stdcall Slot25(void);
    virtual int __stdcall Send(unsigned long, unsigned long, unsigned long, void*, unsigned long);
};

struct Net_4c97b0 {
    char unknown_0[1221];
    DirectPlay_4c97b0* dp;
};

class DirectPlay_4c9800 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall Slot18(void);
    virtual int __stdcall Slot19(void);
    virtual int __stdcall Slot20(void);
    virtual int __stdcall Slot21(void);
    virtual int __stdcall Slot22(void);
    virtual int __stdcall Slot23(void);
    virtual int __stdcall Slot24(void);
    virtual int __stdcall Slot25(void);
    virtual int __stdcall Send(unsigned long, unsigned long, unsigned long, void*, unsigned long);
};

struct Net_4c9800 {
    char unknown_0[1221];
    DirectPlay_4c9800* dp;
};

class DirectPlay_4c9840 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall Slot18(void);
    virtual int __stdcall Slot19(void);
    virtual int __stdcall Slot20(void);
    virtual int __stdcall Slot21(void);
    virtual int __stdcall Slot22(void);
    virtual int __stdcall Slot23(void);
    virtual int __stdcall Slot24(void);
    virtual int __stdcall Receive(unsigned long*, unsigned long*, unsigned long, void*, unsigned long*);
};

struct Net_4c9840 {
    char unknown_0[1205];
    unsigned long from;
    unsigned long to;
    char unknown_4bd[8];
    DirectPlay_4c9840* dp;
};

class DirectPlay_4c9890 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall Slot18(void);
    virtual int __stdcall Slot19(void);
    virtual int __stdcall Slot20(void);
    virtual int __stdcall Slot21(void);
    virtual int __stdcall Slot22(void);
    virtual int __stdcall Slot23(void);
    virtual int __stdcall Slot24(void);
    virtual int __stdcall Slot25(void);
    virtual int __stdcall Slot26(void);
    virtual int __stdcall Slot27(void);
    virtual int __stdcall Slot28(void);
    virtual int __stdcall Slot29(void);
    virtual int __stdcall Slot30(void);
    virtual int __stdcall SetSessionDesc(void*, unsigned long);
};

struct SessionDesc_004c9920 {
    unsigned long size;
    unsigned long dwFlags;
    V4i guidInstance;
    V4i guidApplication;
    unsigned long dwMaxPlayers;
    unsigned long dwCurrentPlayers;
    char* lpszSessionName;
    char* lpszPassword;
    unsigned long dwReserved1;
    unsigned long dwReserved2;
    unsigned long user1;
    unsigned long user2;
    unsigned long user3;
    unsigned long user4;
};

struct Net_4c9890 {
    char name[16];
    char unknown_10[1101];
    SessionDesc_004c9920 desc;
    char unknown_4ad[24];
    DirectPlay_4c9890* dp;
};

class DirectPlay_004c9920 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall Slot18(void);
    virtual int __stdcall Slot19(void);
    virtual int __stdcall Slot20(void);
    virtual int __stdcall Slot21(void);
    virtual int __stdcall GetSessionDesc(void*, unsigned long*);
    virtual int __stdcall Slot23(void);
    virtual int __stdcall Slot24(void*, unsigned long);
};

struct Net_004c9920 {
    char name[16];
    char unknown_10[1069];
    V4i application;
    char unknown_44d[16];
    SessionDesc_004c9920 session;
    char unknown_4ad[24];
    DirectPlay_004c9920* dp;
    char unknown_4c9[20];
    int maxPlayers;
};

struct Connection_4c9a70 {
    char unknown_0[8];
    DPSessionDesc_4c9a70* desc;
};

struct DPSessionDesc_4c9a70 {
    unsigned long size;
    unsigned long flags;
    char guidInstance[16];
    char guidApplication[16];
    unsigned long maxPlayers;
    unsigned long currentPlayers;
    char* sessionName;
    char* password;
    unsigned long reserved1;
    unsigned long reserved2;
    unsigned long user1;
    unsigned long user2;
    unsigned long user3;
    unsigned long user4;
};

class Lobby_4c9a70 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Connect(unsigned long, unsigned long*, int);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall SetConnectionSettings(unsigned long, unsigned long, Connection_4c9a70*);
};

struct Net_4c9a70 {
    char unknown_0[1117];
    DPSessionDesc_4c9a70 desc;
    char unknown_4ad[24];
    void* dp;
    void* field_4c9;
    Lobby_4c9a70* lobby;
    Connection_4c9a70* connection;
    int connection_size;
};

class DirectPlay_4c9b70 {
public:
    virtual int __stdcall QueryInterface(void*, void**);
    virtual unsigned long __stdcall AddRef(void);
    virtual unsigned long __stdcall Release(void);
    virtual int __stdcall AddPlayerToGroup(unsigned long, unsigned long);
    virtual int __stdcall Close(void);
};

struct Net_4c9b70 {
    char unknown_0[1217];
    DirectPlay_4c9b70* dp1;
    DirectPlay_4c9b70* dp;
    Unknown_4c9b70* lobby2;
    Unknown_4c9b70* lobby1;
    int* connection;
    int connection_size;
};

class Unknown_4c9b70 {
public:
    virtual int __stdcall QueryInterface(void*, void**);
    virtual unsigned long __stdcall AddRef(void);
    virtual unsigned long __stdcall Release(void);
};

struct Net_4c9c20 {
    char unknown_0[1245];
    int field_4dd;
    int field_4e1;
};

struct GameRec_4c9c50 {
    int unknown_0;
    int user1;
    int user2;
    int user3;
    int user4;
    int maxPlayers;
    char name[32];
    V4i guidInstance;
    char unknown_48[12];
};

struct Net_4c9c50 {
    char unknown_0[1213];
    GameRec_4c9c50* games;
    char unknown_4c1[40];
    int count;
};

class DirectPlay_4c9d30 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall Slot18(void);
    virtual int __stdcall Slot19(void);
    virtual int __stdcall Slot20(void);
    virtual int __stdcall Slot21(void);
    virtual int __stdcall Slot22(void);
    virtual int __stdcall Slot23(void);
    virtual int __stdcall Slot24(void);
    virtual int __stdcall Slot25(void);
    virtual int __stdcall Slot26(void);
    virtual int __stdcall Slot27(void);
    virtual int __stdcall Slot28(void);
    virtual int __stdcall SetPlayerData(unsigned long, void*, unsigned long, unsigned long);
};

struct Net_4c9d30 {
    char unknown_0[1221];
    DirectPlay_4c9d30* dp;
};

class DirectPlay_4c9d80 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall Slot18(void);
    virtual int __stdcall Slot19(void);
    virtual int __stdcall GetPlayerData(unsigned long, void*, unsigned long*, unsigned long);
};

struct Net_4c9d80 {
    char unknown_0[1221];
    DirectPlay_4c9d80* dp;
};

class DirectPlay_4c9dd0 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall Slot18(void);
    virtual int __stdcall Slot19(void);
    virtual int __stdcall Slot20(void);
    virtual int __stdcall Slot21(void);
    virtual int __stdcall GetSessionDesc(void*, unsigned long*);
};

struct Net_4c9dd0 {
    char unknown_0[1221];
    DirectPlay_4c9dd0* dp;
};

struct SessionDesc_4c9dd0 {
    char unknown_0[44];
    int currentPlayers;
};

class DirectPlay_004c9e50 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall EnumSessions(void*, unsigned long, void*, void*, unsigned long);
};

struct Net_004c9e50 {
    char unknown_0[1085];
    V4i application;
    char unknown_44d[24];
    V4i instance;
    char unknown_475[72];
    void* sessions;
    char unknown_4c1[4];
    DirectPlay_004c9e50* dp;
    char unknown_4c9[24];
    unsigned long timeout;
    char unknown_4e5[4];
    int count;
};

class DirectPlay_4c9f90 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Close(void);
};

class DirectPlay_4c9fd0 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall Slot18(void);
    virtual int __stdcall Slot19(void);
    virtual int __stdcall Slot20(void);
    virtual int __stdcall Slot21(void);
    virtual int __stdcall GetSessionDesc(void*, unsigned long*);
    virtual int __stdcall Slot23(void);
    virtual int __stdcall Slot24(void*, unsigned long);
};

struct Net_4c9fd0 {
    char name[1085];
    V4i appGuid;
    char unknown_44d[16];
    SessionDesc_004c9920 desc;
    char unknown_4ad[24];
    DirectPlay_4c9fd0* dp;
};

struct Conn_4ca100 {
    void* data;
    unsigned long size;
};

struct DPName_004ca6a0 {
    unsigned long dwSize;
    unsigned long dwFlags;
    char* lpszShortNameA;
    char* lpszLongNameA;
};

struct Net_4ca100 {
    char unknown_0[1073];
    V4i* guids;
    Conn_4ca100* conns;
    char* names;
    char unknown_43d[168];
    int field_4e5;
};

class DirectPlay_4ca250 {
public:
    virtual int __stdcall QueryInterface(void*, void**);
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall Slot18(void);
    virtual int __stdcall Slot19(void);
    virtual int __stdcall Slot20(void);
    virtual int __stdcall Slot21(void);
    virtual int __stdcall Slot22(void);
    virtual int __stdcall Slot23(void);
    virtual int __stdcall Slot24(void);
    virtual int __stdcall Slot25(void);
    virtual int __stdcall Slot26(void);
    virtual int __stdcall Slot27(void);
    virtual int __stdcall Slot28(void);
    virtual int __stdcall Slot29(void);
    virtual int __stdcall Slot30(void);
    virtual int __stdcall Slot31(void);
    virtual int __stdcall Slot32(void);
    virtual int __stdcall Slot33(void);
    virtual int __stdcall Slot34(void);
    virtual int __stdcall EnumConnections(V4i*, void*, void*, unsigned long);
};

struct Net_4ca250 {
    char unknown_0[1073];
    int field_431;
    int field_435;
    int field_439;
    V4i application;
    V4i sp;
    char unknown_45d[80];
    int field_4ad;
    char unknown_4b1[16];
    DirectPlay_4ca250* dp;
    DirectPlay_4ca250* dp2;
    int field_4c9;
    int field_4cd;
    int field_4d1;
    int field_4d5;
    char unknown_4d9[12];
    int field_4e5;
};

struct Net_4ca330 {
    char unknown_0[1073];
    V4i* guids;
    char* conns;
    char* names;
    char unknown_43d[168];
    int field_4e5;
};

struct Net_4ca400 {
    char unknown_0[1073];
    int field_431;
    int field_435;
    int field_439;
    char unknown_43d[168];
    int field_4e5;
};

struct Connection_004ca450 {
    char unknown_0[8];
    Desc_004ca450* desc;
};

struct Desc_004ca450 {
    int unknown_0;
    unsigned int flags;
};

struct Net_004ca450 {
    char unknown_0[1121];
    unsigned int flags;
    char unknown_465[108];
    Connection_004ca450* connection;
};

class DirectPlayLobby_4ca490 {
public:
    virtual int __stdcall QueryInterface(_GUID*, void**);
    virtual unsigned long __stdcall AddRef(void);
    virtual unsigned long __stdcall Release(void);
    virtual int __stdcall Connect(unsigned long, void**, void*);
    virtual int __stdcall CreateAddress(_GUID*, _GUID*, void*, unsigned long, void*, unsigned long*);
    virtual int __stdcall EnumAddress(void*, void*, unsigned long, void*);
    virtual int __stdcall EnumAddressTypes(void*, _GUID*, void*, unsigned long);
    virtual int __stdcall EnumLocalApplications(void*, void*, unsigned long);
    virtual int __stdcall GetConnectionSettings(unsigned long, void*, unsigned long*);
};

class Iface_4ca590 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall Slot18(void);
    virtual int __stdcall Slot19(void);
    virtual int __stdcall Slot20(void);
    virtual int __stdcall Slot21(void);
    virtual int __stdcall Slot22(void);
    virtual int __stdcall Slot23(void);
    virtual int __stdcall Slot24(void);
    virtual int __stdcall Slot25(void);
    virtual int __stdcall Slot26(void);
    virtual int __stdcall Slot27(void);
    virtual int __stdcall Slot28(void);
    virtual int __stdcall Slot29(void);
    virtual int __stdcall Slot30(void);
    virtual int __stdcall Slot31(void);
    virtual int __stdcall Slot32(void);
    virtual int __stdcall Slot33(void);
    virtual int __stdcall Slot34(void);
    virtual int __stdcall Slot35(void);
    virtual int __stdcall Slot36(void);
    virtual int __stdcall Slot37(void);
    virtual int __stdcall Slot38(int, int);
};

struct Obj_4ca590 {
    char unknown_0[1221];
    Iface_4ca590* iface;
};

class DirectPlay_004ca6a0 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall CreatePlayer(unsigned long*, DPName_004ca6a0*, void*, void*, unsigned long, unsigned long);
};

struct Net_004ca6a0 {
    char unknown_0[17];
    char longName[16];
    char shortName[16];
    char unknown_31[1172];
    DirectPlay_004ca6a0* dp;
};

struct PlayerData_004ca6a0 {
    char name[17];
    short field_11;
    short field_13;
};

class DirectPlay_4ca780 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall DestroyPlayer(unsigned long);
};

struct Net_4ca780 {
    char unknown_0[1221];
    DirectPlay_4ca780* dp;
};

class DirectPlay_4ca7c0 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall Slot18(void);
    virtual int __stdcall Slot19(void);
    virtual int __stdcall Slot20(void);
    virtual int __stdcall GetPlayerName(unsigned long, void*, unsigned long*);
};

struct Net_4ca7c0 {
    char unknown_0[1221];
    DirectPlay_4ca7c0* dp;
};

class DirectPlay_4ca800 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall Slot18(void);
    virtual int __stdcall Slot19(void);
    virtual int __stdcall Slot20(void);
    virtual int __stdcall Slot21(void);
    virtual int __stdcall Slot22(void);
    virtual int __stdcall Slot23(void);
    virtual int __stdcall Slot24(void);
    virtual int __stdcall Slot25(void);
    virtual int __stdcall Slot26(void);
    virtual int __stdcall Slot27(void);
    virtual int __stdcall Slot28(void);
    virtual int __stdcall Slot29(void);
    virtual int __stdcall SetPlayerName(unsigned long, void*, unsigned long);
};

struct Net_4ca800 {
    char unknown_0[1221];
    DirectPlay_4ca800* dp;
};

class DirectPlay_4ca840 {
public:
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual int __stdcall Slot02(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall EnumPlayers(void*, void*, void*, unsigned long);
};

struct Net_4ca840 {
    char unknown_0[1221];
    DirectPlay_4ca840* dp;
};

class DirectPlayLobby_4ca880 {
public:
    virtual int __stdcall QueryInterface(void*, void**);
    virtual int __stdcall Slot00(void);
    virtual int __stdcall Slot01(void);
    virtual unsigned long __stdcall AddRef(void);
    virtual int __stdcall Slot02(void);
    virtual unsigned long __stdcall Release(void);
    virtual int __stdcall Connect(unsigned long, void**, void*);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall CreateAddress(void*, void*, void*, unsigned long, void*, unsigned long*);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall EnumAddress(void*, void*, unsigned long, void*);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall CreateCompoundAddress(void*, unsigned long, void*, unsigned long*);
};

class DirectPlay_004ca990 {
public:
    virtual int __stdcall Slot00(void);
    virtual long __stdcall QueryInterface(void*, void**);
    virtual int __stdcall Slot01(void);
    virtual unsigned long __stdcall AddRef(void);
    virtual int __stdcall Slot02(void);
    virtual unsigned long __stdcall Release(void);
    virtual int __stdcall Slot03(void);
    virtual int __stdcall Slot04(void);
    virtual int __stdcall Slot05(void);
    virtual int __stdcall Slot06(void);
    virtual int __stdcall Slot07(void);
    virtual int __stdcall Slot08(void);
    virtual int __stdcall Slot09(void);
    virtual int __stdcall Slot10(void);
    virtual int __stdcall Slot11(void);
    virtual int __stdcall Slot12(void);
    virtual int __stdcall Slot13(void);
    virtual int __stdcall Slot14(void);
    virtual int __stdcall Slot15(void);
    virtual int __stdcall Slot16(void);
    virtual int __stdcall Slot17(void);
    virtual int __stdcall GetPlayerAddress(unsigned long, void*, unsigned long*);
};

struct Header_004cac40 {
    unsigned char a;
    unsigned char b;
    unsigned char c;
    unsigned char d;
    unsigned short e;
    unsigned short f;
    short g;
    short h;
    short i;
    short j;
    unsigned char palette[48];
    unsigned char k;
    unsigned char l;
    unsigned short m;
    unsigned short n;
    char unknown_46[58];
};

struct Bitmap_004caec0 {
    int width;
    int height;
    char unknown_8[4];
    unsigned char* data;
};

struct Game_004caec0 {
    char unknown_0[532];
    tagPALETTEENTRY palette[256];
};

struct Image_004caf30 {
    int width;
    int height;
    int field_8;
    unsigned char* pixels;
};

struct Bitmap_004cb170 {
    int width;
    int height;
    char unknown_8[4];
    unsigned char* data;
};

struct Game_004cb170 {
    char unknown_0[188];
    Bitmap_004cb170* bitmap;
    char unknown_c0[28];
    int field_dc;
    char unknown_e0[308];
    tagPALETTEENTRY palette[256];
};

struct Table_004cb2f0 {
    char unknown_0[36];
    Entry* entries;
};

struct Blob32_4cb330 {
    int words[8];
};

struct Elem_004cb4c0 {
    int unknown_0;
    int count;
    int ptr_8;
    unsigned short* indices;
    int ptr_10;
    int unknown_14[3];
};

struct Elem_004cb5f0 {
    int unknown_0;
    int value;
    int unknown_8;
};

struct Image_004cb940 {
    int width;
    int unknown_4;
    int pitch;
    unsigned char* pixels;
};

struct Image_004cb9e0 {
    int width;
    int height;
    int pitch;
    unsigned char* pixels;
};

class Class_004cda00 {
public:
    char unknown_0[512];
    int field_200;
    int field_204;
    int field_208;
    int field_20c;
    int field_210;
    char unknown_214[108];
    int field_280;
    int QueryDisc(void);
};

class Class_004ce020 {
public:
    char unknown_0[1];
    int GetPlayState(void);
};

class Class_004d02a0 {
public:
    char unknown_0[512];
    int field_200;
    char unknown_204[4];
    int field_208;
    int field_20c;
    char unknown_210[104];
    int field_278;
    char unknown_27c[8];
    int step;
    int field288;
    void (__stdcall *callback)(void);
    int OpenSample(char*, int, int, int);
};

class Class_004cff30 {
public:
    char unknown_0[16];
    int wave_devices;
    int aux_device;
    int wave_volume;
    int aux_volume;
    int GetWaveVolume(void);
    void InitMixerVolumes(void);
    int GetAuxVolume(void);
};

class Class_004cfb40 {
public:
    char unknown_0[36];
    void* field_24;
    char unknown_28[444];
    void* stream;
    Class_004bbbe0* file;
    int bits;
    int size;
    int pos;
    int off;
    char unknown_1fc[140];
    int handle;
    void StartStream(Class_004bbbe0*, int, int, int, long);
    void UpdateStream(void);
    void FillStreamHalf(void);
};

class Class_004cf180 {
public:
    char unknown_0[48];
    int count;
    char unknown_34[4];
    void* buffers[32];
    int priority[32];
    int flags[32];
    void StopOldestBuffer(void);
};

class Class_004cf230 {
public:
    char unknown_0[36];
    void* field_24;
    void** CreateSampleFromMemory(void*, unsigned long, int, int, int);
};

class Class_004cf370 {
public:
    char unknown_0[36];
    void* field_24;
    void** CreateSampleFromFile(Class_004bbbe0*, unsigned long, int, int, int);
};

struct IDirectSound3DBuffer {
    char unknown_0[4];
    long __stdcall GetAllParameters(void*);
    long __stdcall GetConeAngles(unsigned long*, unsigned long*);
    long __stdcall GetConeOrientation(void*);
    long __stdcall GetConeOutsideVolume(long*);
    long __stdcall GetMaxDistance(float*);
    long __stdcall GetMinDistance(float*);
    long __stdcall GetMode(unsigned long*);
    long __stdcall GetPosition(void*);
    long __stdcall GetVelocity(void*);
    long __stdcall SetAllParameters(void*, unsigned long);
    long __stdcall SetConeAngles(unsigned long, unsigned long, unsigned long);
    long __stdcall SetConeOrientation(float, float, float, unsigned long);
    long __stdcall SetConeOutsideVolume(long, unsigned long);
    long __stdcall SetMaxDistance(float, unsigned long);
    long __stdcall SetMinDistance(float, unsigned long);
    long __stdcall SetMode(unsigned long, unsigned long);
    long __stdcall SetPosition(float, float, float, unsigned long);
};

class Class_004cf800 {
public:
    char unknown_0[452];
    void** sets[8];
    int PlayMemorySample(int, int, int, int, int, int, int);
};

class Class_004cf8a0 {
public:
    char unknown_0[452];
    void** sets[8];
    int PlayFileSample(int, int, int, int, int, int, int);
};

class Class_004cfe40 {
public:
    char unknown_0[492];
    int format;
    void FillSilence(void*, unsigned int);
};

struct Class_004d01b0 {
    char unknown_0[1];
    int DetectSampleFormat(void*);
};

class Class_004d0660 {
public:
    char unknown_0[1];
    void StreamSample(char*, int);
};

class Class_004d0720 {
public:
    char unknown_0[1];
    int FindChunkSize(void*, char*);
};

class Class_004d07f0 {
public:
    char unknown_0[1];
    int ReadWaveFormat(void*, int*, int*, int*);
};

class Class_004d0910 {
public:
    char unknown_0[1];
    int FindDataChunkSize(void*);
};

struct Node_004d0b10 {
    unsigned short parent;
    unsigned short smaller;
    unsigned short larger;
    char unknown_6[24570];
    short field_6000;
    short field_6002;
    short field_6004;
};

struct Node_004d0f60 {
    unsigned short parent;
    unsigned short smaller;
    unsigned short larger;
};

struct Chunk_4d1970 {
    int marker;
    unsigned char version;
    unsigned char method;
    unsigned char encrypt;
    int compressedSize;
    int size;
    int checksum;
};

struct PackHeader_004d1c00 {
    char unknown_0[5];
    unsigned char packType;
    unsigned char encryptType;
    long packedSize;
    long unpackedSize;
};

class Class_004d8820 {
public:
    unsigned int key;
    unsigned int size;
    int field_8;
    char unknown_c[32];
    unsigned int tag;
    Class_004d8820(int, int, int, int, char*);
    Class_004d8820(void);
};

class BlockInfo {
public:
    int field_0;
    int field_4;
    int field_8;
    BlockInfo(void);
};

class Class_004d8850 {
public:
    char unknown_0[12];
    char field_c[32];
    void FUN_004d8850(char*);
};

struct Stack_004d89b0 {
    unsigned long addrs[14];
    int count;
};

class CallSite {
public:
    char name[64];
    int line;
    Stack_004d89b0 stack;
    void FormatCallSite(char*, int);
};

struct Base_004d8ae0 {
    char unknown_0[140];
};

struct Class_004d8b30 : public Base_004d8ae0 {
    int count;
    unsigned int min;
    unsigned int max;
    char name[32];
    unsigned __int64 total;
    Class_004d8b30(Base_004d8ae0&, char*);
    void FUN_004d8b30(char*);
};

class Class_004d8b60 {
public:
    char unknown_0[140];
    int count;
    unsigned int min;
    unsigned int max;
    char unknown_98[32];
    unsigned __int64 total;
    void AddSample(unsigned int);
};

class TraceRecord {
public:
    char data[140];
    TraceRecord(void);
};

class Class_004d8bd0 : public BlockInfo {
public:
    char unknown_c[36];
    TraceRecord member_30;
    TraceRecord member_bc;
    char unknown_148[4];
    Class_004d8bd0(void);
};

class Class_004d88d0 {
public:
    char unknown_0[140];
    Class_004d88d0(char*, int, int);
};

class BlockHistory {
public:
    char unknown_0[48];
    Class_004d88d0 field_30;
    TraceRecord field_bc;
    char field_148;
    BlockHistory(Class_004d87f0_2&, char*, int, int);
};

class Class_004d8d40 {
public:
    Class_004d8820 block;
    char unknown_30[280];
    void FormatBlockHistory(char*, int, char);
    void FUN_004d8d40(char);
};

struct S {
    char unknown_0[4];
    int val;
};

struct TlsBlock {
    char unknown_0[8];
    void* limit;
};

class StackTrace {
public:
    char unknown_0[120];
    int field_78;
    int field_7c;
    char unknown_80[8188];
    int field_207c;
    int* field_2080;
    char dump_text[42060];
    void CaptureStack(int*, int*, int, int);
};

class Class_004d9ca0 {
public:
    unsigned long ret[30];
    int count;
    int stack[2048];
    int copied;
    unsigned long* pc;
    char buf[42060];
    void FormatStackReport(void);
};

class CritSec_004e1ac0 {
public:
    _RTL_CRITICAL_SECTION cs;
    CritSec_004e1ac0(void);
    ~CritSec_004e1ac0(void);
};

class BlockMapAllocator {
public:
    char unknown_0[1];
    void* FUN_004dddf0(unsigned int);
};

class Tree_004da8d0 {
public:
    char allocator;
    char key_compare;
    char unknown_2[2];
    LiveNode* _Head;
    unsigned char _Multi;
    char unknown_9[3];
    unsigned int _Size;
    Tree_004da8d0(char&, char&);
    ~Tree_004da8d0(void);
    LiveNode*& _Root(void);
    static int& _Color(LiveNode*);
    static void* operator new(unsigned int);
    static NameKey& _Key(LiveNode*);
    LiveNode*& _Lmost(void);
    static LiveNode*& _Left(LiveNode*);
    LiveNode*& _Rmost(void);
    void Init(void);
    static LiveNode*& _Parent(LiveNode*);
    unsigned int size(void);
    NameMapIter begin(void);
    static LiveNode*& _Right(LiveNode*);
    NameMapIter end(void);
    static std::pair<NameKey, Value_004e2250>& _Value(LiveNode*);
    NameMapIter erase(NameMapIter, NameMapIter);
    static LiveNode* _Max(LiveNode*);
};

class FreeBlockIter {
public:
    LiveNode* ptr;
    FreeBlockIter(LiveNode*);
    FreeBlockIter(void);
    void FUN_004dd340(void);
    FreeBlockIter operator++(int);
    FreeBlockIter& operator++(void);
    unsigned char operator==(FreeBlockIter&);
    void Dec(void);
    void Inc(void);
    FreeBlockIter Next(int);
    LiveNode* Mynode(void);
    LiveNode* _Mynode(void);
    unsigned char operator!=(FreeBlockIter&);
    void FUN_004dd2a0(void);
    void FUN_004dd820(void);
    Pair_00419560& operator*(void);
    Pair_00419560* operator->(void);
    FreeBlockIter Previous(int);
    FreeBlockIter operator--(int);
    FreeBlockIter& operator--(void);
};

class Class_004db450 {
public:
    char unknown_0[4];
    LiveNode* head;
    char unknown_8[8];
    int total;
    bool GrowReservation(unsigned int);
    unsigned char Neq(FreeBlockIter, FreeBlockIter);
    void Tail(unsigned int, unsigned int, FreeBlockIter&);
};

class Class_004dbd00 {
public:
    char unknown_0[1];
    FreeBlockIter FUN_004dbd00(FreeBlockIter);
};

class Class_004dbeb0 {
public:
    char unknown_0[4];
    int* field_4;
    FreeBlockIter FUN_004dbeb0(void);
    int* FUN_004dbeb0(int*);
};

class MapInsertResult {
public:
    FreeBlockIter first;
    unsigned char second;
    char unknown_5[3];
    MapInsertResult(FreeBlockIter, unsigned char);
    MapInsertResult(MapInsertResult&);
    MapInsertResult(void);
    MapInsertResult* FUN_004ddbe0(int*, unsigned char*);
};

struct Less_004db000 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class FreeBlockMap {
public:
    Less_004db000 key_compare;
    char unknown_1[3];
    LiveNode* head;
    unsigned char rebuild;
    char unknown_9[3];
    unsigned int count;
    unsigned int total;
    FreeBlockMap(FreeBlockMap&);
    FreeBlockMap(void);
    FreeBlockIter Begin(void);
    FreeBlockIter begin(void);
    FreeBlockIter End(void);
    FreeBlockIter end(void);
    static void* operator new(unsigned int);
    FreeBlockIter upper_bound(unsigned int&);
    unsigned char Neq(FreeBlockIter, FreeBlockIter);
    unsigned int size(void);
    FreeBlockIter erase(FreeBlockIter);
    unsigned int TakeFreeBlock(unsigned int);
    MapInsertResult insert(Pair_00419560&);
    void AddFreeBlock(Pair_00419560);
    unsigned char Grow(unsigned int);
};

struct InsertResult_004db450 {
    FreeBlockIter first;
    int second;
};

struct Frame_004db450 {
    InsertResult_004db450 r;
    Pair_00419560 p;
};

class FreeBlockAllocator {
public:
    char unknown_0[1];
    void* FUN_004ddd70(unsigned int);
};

class PoolAlloc_004db610 {
public:
    char unknown_0[1];
    char* _Charalloc(unsigned int);
};

class Alloc_004dd8c0 {
public:
    char unknown_0[1];
    Elem_004dd8c0* allocate(unsigned int, void*);
    void deallocate(Elem_004dd8c0*, unsigned int);
    void construct(Elem_004dd8c0*, Elem_004dd8c0&);
    void destroy(Elem_004dd8c0*);
    unsigned int max_size(void);
};

struct Arena_004da9f0 {
    char unknown_0[16];
    unsigned int count;
};

class Class_004dc910 {
public:
    int unknown_0;
    LiveNode* _Head;
    unsigned char _Multi;
    char unknown_9[3];
    int _Size;
    Iter_0046e330 erase(Iter_0046e330);
    LiveNode*& _Root(void);
    LiveNode*& _Lmost(void);
    LiveNode*& _Rmost(void);
    static LiveNode* _Min(LiveNode*);
    static LiveNode* _Max(LiveNode*);
    void _Lrotate(LiveNode*);
    void _Rrotate(LiveNode*);
};

struct Less_004dd3d0 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class Class_004dd3d0 {
public:
    Less_004dd3d0 compare;
    char unknown_1[3];
    LiveNode* head;
    Iter_0046e330 End(void);
    LiveNode* Lbound(unsigned int&);
    Iter_0046e330 FUN_004dd3d0(unsigned int&);
    Iter_0046e330 find(unsigned int&);
};

struct Kfn_004dbbc0 {
    char unknown_0[1];
    unsigned int& operator()(Pair_00419560&);
};

struct Less_004dbbc0 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class Class_004dce60 {
public:
    unsigned char allocator;
    Less_004dbbc0 key_compare;
    char unknown_2[2];
    LiveNode* head;
    char unknown_8[4];
    int size;
    FreeBlockIter FUN_004dce60(LiveNode*, LiveNode*, Pair_00419560*);
    MapInsertResult FUN_004dbbc0(Pair_00419560&);
    static LiveNode*& Left(LiveNode*);
    void Lrotate(LiveNode*);
    static LiveNode*& Right(LiveNode*);
    void Rrotate(LiveNode*);
    static unsigned int& Key(LiveNode*);
    LiveNode*& Root(void);
    LiveNode*& Lmost(void);
    FreeBlockIter begin(void);
    MapInsertResult TreeInsert(Pair_00419560&);
};

struct Less_004dbd20 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class Class_004dbd20 {
public:
    Less_004dbd20 key_compare;
    char unknown_1[3];
    LiveNode* head;
    FreeBlockIter UpperBound(unsigned int&);
};

class Class_004dd150 {
public:
    int unknown_0;
    LiveNode* head;
    void FUN_004dd150(LiveNode*);
};

class Class_004dd1f0 {
public:
    int unknown_0;
    LiveNode* head;
    void FUN_004dd1f0(LiveNode*);
};

class Class_004ddc00 {
public:
    char unknown_0[1];
    LiveNode* FUN_004ddc00(int, int);
};

struct Less_004dbec0 {
    char unknown_0[1];
    unsigned char operator()(unsigned int, unsigned int);
};

class Class_004dbec0 {
public:
    Less_004dbec0 key_compare;
    char unknown_1[3];
    LiveNode* head;
    unsigned char rebuild;
    char unknown_9[3];
    int size;
    FreeBlockIter Begin(void);
    MapInsertResult FUN_004dbec0(Pair_00419560*);
    void FUN_004dbec0(InsertResult_004db450*, Pair_00419560*);
};

struct Alloc_004dc130 {
    char unknown_0[1];
};

struct Less_004dc130 {
    char unknown_0[1];
};

class Class_004dc130 {
public:
    Alloc_004dc130 allocator;
    Less_004dc130 key_compare;
    char unknown_2[2];
    LiveNode* head;
    unsigned char multi;
    char unknown_9[3];
    unsigned int size;
    FreeBlockIter FUN_004dc130(FreeBlockIter);
    static LiveNode*& Left(LiveNode*);
    void FUN_004dc130(FreeBlockIter*, FreeBlockIter);
    static LiveNode*& Parent(LiveNode*);
    static LiveNode*& Right(LiveNode*);
    static int& Color(LiveNode*);
    static unsigned int* _Value(LiveNode*);
    LiveNode*& Root(void);
    LiveNode*& Lmost(void);
    LiveNode*& Rmost(void);
    static LiveNode* Min(LiveNode*);
    static LiveNode* Max(LiveNode*);
    void Lrotate(LiveNode*);
    void Rrotate(LiveNode*);
    static void Destval(unsigned int*);
    static void Freenode(LiveNode*);
};

struct Less_004dc620 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class Class_004dc620 {
public:
    Less_004dc620 key_compare;
    char unknown_1[3];
    LiveNode* head;
    FreeBlockIter FUN_004dc620(unsigned int&);
    void FUN_004dc620(FreeBlockIter*, unsigned int&);
};

class Class_004dd710 {
public:
    int unknown_0;
    LiveNode* head;
    void FUN_004dd710(LiveNode*);
};

class Class_004dd770 {
public:
    int unknown_0;
    LiveNode* head;
    void FUN_004dd770(LiveNode*);
};

class Class_004ddce0 {
public:
    char unknown_0[1];
    LiveNode* FUN_004ddce0(int, int);
};

struct Less_004dc680 {
    char unknown_0[1];
    unsigned char operator()(unsigned int, unsigned int);
};

class Class_004dc680 {
public:
    Less_004dc680 key_compare;
    char unknown_1[3];
    LiveNode* head;
    unsigned char rebuild;
    char unknown_9[3];
    int size;
    FreeBlockIter Begin(void);
    MapInsertResult FUN_004dc680(Class_004d8820*);
};

class BlockMapIter {
public:
    LiveNode* ptr;
    void FUN_004dde70(void);
};

struct Less_004dce00 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class Class_004dce00 {
public:
    Less_004dce00 compare;
    char unknown_1[3];
    LiveNode* head;
    Iter_0046e330 End(void);
    Iter_0046e330 end(void);
    Iter_0046e330 FUN_004dce00(unsigned int&);
    Iter_0046e330 erase(Iter_0046e330);
};

struct Less_004dce60 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

struct Less_004dd250 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class Class_004dd250 {
public:
    Less_004dd250 key_compare;
    char unknown_1[3];
    LiveNode* head;
    LiveNode* FUN_004dd250(unsigned int&);
};

struct Less_004dd430 {
    char unknown_0[1];
    unsigned char operator()(unsigned int, unsigned int);
};

class Class_004dd430 {
public:
    Less_004dd430 key_compare;
    char unknown_1[3];
    LiveNode* head;
    unsigned char unknown_8;
    char unknown_9[3];
    int size;
    FreeBlockIter* FUN_004dd430(FreeBlockIter*, LiveNode*, LiveNode*, Class_004d8820*);
    void Lrotate(LiveNode*);
    void Rrotate(LiveNode*);
    FreeBlockIter FUN_004dd430(LiveNode*, LiveNode*, Class_004d8820&);
};

struct Less_004dd7d0 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class Class_004dd7d0 {
public:
    Less_004dd7d0 key_compare;
    char unknown_1[3];
    LiveNode* head;
    FreeBlockIter End(void);
    LiveNode* FUN_004dd7d0(unsigned int&);
    FreeBlockIter Begin(void);
};

struct Less_004ddc90 {
    char unknown_0[1];
    unsigned char operator()(unsigned int&, unsigned int&);
};

class Class_004ddc90 {
public:
    Less_004ddc90 compare;
    char unknown_1[3];
    LiveNode* head;
    LiveNode* FUN_004ddc90(unsigned int&);
};

class MappedFile {
public:
    void* hFile;
    void* hMapping;
    void* view;
    int size;
    int state;
    MappedFile(char*);
};

class LoadedImage : public MappedFile {
public:
    void* module;
    unsigned int imageBase;
    _IMAGE_DOS_HEADER* dosHeader;
    _IMAGE_NT_HEADERS* ntHeaders;
    _IMAGE_DEBUG_DIRECTORY* debugDirs;
    unsigned int numDebugDirs;
    LoadedImage(void*);
    ~LoadedImage(void);
};

class Class_004e1590 {
public:
    void* hFile;
    void* hMapping;
    void* view;
    unsigned long size;
    int state;
    void OpenMappedFile(char*);
};

class Class_004ddfa0 {
public:
    char unknown_0[8];
    char* base;
    char unknown_c[24];
    DebugDir_004ddfa0* debugDirs;
    int numDebugDirs;
    Fpo_004de020* GetFpoRecords(void);
};

struct DebugDir_004ddfa0 {
    unsigned int characteristics;
    unsigned int timeDateStamp;
    unsigned short majorVersion;
    unsigned short minorVersion;
    unsigned int type;
    unsigned int sizeOfData;
    unsigned int addressOfRawData;
    unsigned int pointerToRawData;
};

class Class_004ddfe0 {
public:
    char unknown_0[8];
    char* base;
    char unknown_c[24];
    DebugDir_004ddfa0* debugDirs;
    int numDebugDirs;
    unsigned int GetFpoRecordCount(void);
};

class Class_004de020 {
public:
    char unknown_0[24];
    unsigned int imageBase;
    Fpo_004de020* FindFpoRecord(unsigned int);
};

struct Line_004de550 {
    unsigned long SizeOfStruct;
    unsigned long Key;
    unsigned long LineNumber;
    unsigned long FileName;
    unsigned long Address;
};

struct Frame_004de700 {
    int addrFrame;
    int unknown_04;
    int flags0;
    char unknown_c[12];
    int addrStack;
    char unknown_1c[4];
    int flags1;
    int handler;
    char unknown_28[4];
    int flags2;
    char unknown_30[64];
};

struct Sym_004dea00 {
    unsigned long SizeOfStruct;
    unsigned long Address;
    unsigned long Size;
    unsigned long Flags;
    unsigned long MaxNameLength;
    char Name[516];
};

class Class_004df250 {
public:
    char unknown_0[1];
    void CreatePerformanceDialog(void);
};

class Id_004df1e0 {
public:
    char* id;
    char text[500];
    Id_004df1e0(char*);
};

class Class_004df280 {
public:
    void* hwnd;
    char unknown_4[28];
    unsigned char flag_20;
    char unknown_21[3];
    void SetPerformanceWindowVisible(char);
};

class Class_004df380 {
public:
    void* hwnd;
    char unknown_4[32];
    char* name;
    unsigned char Same(char*);
    void SyncSelectedText(void);
};

class Map_004df380 {
public:
    char compare;
    char allocator;
    char unknown_2[2];
    LiveNode* head;
    char multi;
    char unknown_9[3];
    int size;
    char changed;
    char unknown_11[3];
    Iter_0046e330 begin(void);
    Iter_0046e330 end(void);
};

struct Value_004df380 {
    char text[500];
};

class Class_004df4e0 {
public:
    void* hwnd;
    void EnableControls(void);
};

class NameMapIter {
public:
    LiveNode* ptr;
    NameMapIter(LiveNode*);
    NameMapIter(void);
    void FUN_004e0450(void);
    void FUN_004e2ab0(void);
    NameMapIter operator++(int);
    NameMapIter& operator++(void);
    NameMapIter& operator--(void);
    unsigned char operator==(NameMapIter&);
    LiveNode* _Mynode(void);
    unsigned char operator!=(NameMapIter&);
};

class Class_004e18c0 {
public:
    char unknown_0[4];
    LiveNode* head;
    unsigned char multi;
    char unknown_9[3];
    int size;
    unsigned char changed;
    char unknown_11[3];
    NameMapIter begin(void);
    void FUN_004e18c0(void);
    NameMapIter end(void);
    NameMapIter erase(NameMapIter, NameMapIter);
};

struct Iterator_004df590 {
    LiveNode* ptr;
    Iterator_004df590(LiveNode*);
    unsigned char operator==(Iterator_004df590&);
    unsigned char operator!=(Iterator_004df590&);
};

struct Map_004df590 {
    char compare;
    char allocator;
    char unknown_2[2];
    LiveNode* head;
    char multi;
    char unknown_9[3];
    int size;
    char changed;
    char unknown_11[3];
};

class NameTable {
public:
    Map_004df590 names;
    NameTable(NameTable&);
    NameTable(void);
};

class Class_004df1e0 {
public:
    int unknown_0;
    int unknown_4;
    int unknown_8;
    int unknown_c;
    double time;
    int count;
    void* table;
    char flag_20;
    char unknown_21[3];
    Id_004df1e0 ident;
    NameTable map;
    Class_004df1e0(void);
    ~Class_004df1e0(void);
};

struct Value_004df590 {
    char* name;
    char text[500];
    Value_004df590& operator=(Value_004df590&);
};

class Class_004df590 {
public:
    void* hwnd;
    int left;
    int top;
    char unknown_c[12];
    int count;
    Entry* entries;
    char flag_20;
    char unknown_21[3];
    Value_004df590 selected;
    Map_004df590 set;
    int HandlePerformanceMessage(unsigned int, unsigned int, long);
};

class Class_004dfea0 {
public:
    char _Alnod[4];
    LiveNode* _Head;
    char _Multi;
    char unknown_9[3];
    unsigned int _Size;
    NameMapIter FUN_004dfea0(NameMapIter);
    static int& _Color(LiveNode*);
    static LiveNode*& _Left(LiveNode*);
    static LiveNode*& _Parent(LiveNode*);
    static LiveNode*& _Right(LiveNode*);
    LiveNode*& _Root(void);
    LiveNode*& _Lmost(void);
    LiveNode*& _Rmost(void);
    static LiveNode* _Min(LiveNode*);
    static LiveNode* _Max(LiveNode*);
    void _Lrotate(LiveNode*);
    void _Rrotate(LiveNode*);
    static void _Freenode(LiveNode*);
};

class Class_004e03f0 {
public:
    char unknown_0[1];
    void EraseSubtree(LiveNode*);
};

class Class_004e0520 {
public:
    char unknown_0[121];
    unsigned char workingSet;
    void LoadWorkingSetPref(int);
};

class CavedogRegistryKey {
public:
    int key;
    unsigned char readOnly;
    char unknown_5[3];
    tagWINDOWPLACEMENT placement;
    CavedogRegistryKey(char, char*, char*);
    ~CavedogRegistryKey(void);
};

class Class_004e2fe0 {
public:
    void* key;
    char reading;
    void ApplyBool(char*, bool*, bool);
};

class Class_004e05c0 {
public:
    char unknown_0[1];
    void CreateMemoryStatusDialog(void);
};

struct Triple_004e0570 {
    int a;
    int b;
    int c;
    Triple_004e0570(void);
};

struct Sub_004e0570 {
    int table[20];
    Triple_004e0570 triple;
    Sub_004e0570(void);
};

class Class_004e0570 {
public:
    int field_0;
    int field_4;
    int field_8;
    int field_c;
    double time;
    Sub_004e0570 sub;
    int unknown_74;
    unsigned char flag_78;
    char unknown_79[7];
    Class_004e0570(void);
    ~Class_004e0570(void);
};

class Class_004e05f0 {
public:
    void* hwnd;
    char unknown_4[116];
    char flag_78;
    char unknown_79[3];
    void SetMemoryStatusWindowVisible(char);
};

struct Rate_004e0b90 {
    double table[10];
    double total;
    int index;
    int unknown_5c;
};

class Class_004e0b90 {
public:
    void* hwnd;
    int field_4;
    int left;
    int top;
    double time;
    Rate_004e0b90 rates;
    char flag_78;
    unsigned char workingSet;
    char unknown_7a[6];
    int HandleMemoryStatusMessage(unsigned int, int, int);
};

class Class_004e1650_2 {
public:
    void* hFile;
    void* hMapping;
    void* view;
    void CloseMappedFile(void);
};

class NameMapAllocator {
public:
    char unknown_0[1];
    std::pair<char*, Value_004e17c0>* address(std::pair<char*, Value_004e17c0>&);
    void* FUN_004e2b60(unsigned int);
    std::pair<char*, Value_004e17c0>* allocate(unsigned int, void*);
    void deallocate(void*, unsigned int);
    unsigned int max_size(void);
};

struct Less_004e17c0 {
    char unknown_0[1];
    unsigned char operator()(char*, char*);
};

struct Value_004e17c0 {
    char text[500];
};

class Class_004e2240 {
public:
    char unknown_0[4];
    int* field_4;
    NameMapIter FUN_004e2240(void);
    int* FUN_004e2240(int*);
};

class NameKey {
public:
    char* name;
    int LessThan(NameKey&);
};

class Class_004e2250 {
public:
    char unknown_0[16];
    NameMapIter begin(void);
    NameMapInsertResult FUN_004e2250(std::pair<NameKey, Value_004e2250>&);
};

class NameMapInsertResult {
public:
    NameMapIter first;
    unsigned char second;
    char unknown_5[3];
    NameMapInsertResult(NameMapIter&, unsigned char&);
    NameMapInsertResult(void);
    NameMapInsertResult* FUN_004e2a10(Data1*, Data2*);
};

struct Less_004e1990 {
    char unknown_0[1];
    unsigned char operator()(NameKey&, NameKey&);
};

class Class_004e1990 {
public:
    Less_004e1990 compare;
    char unknown_1[3];
    LiveNode* head;
    unsigned char multi;
    char unknown_9[3];
    int size;
    unsigned char changed;
    char unknown_11[3];
    NameMapIter End(void);
    void Upsert(NameKey&);
    NameMapIter Find(NameKey&);
};

class Class_004e2e20 {
public:
    void* key;
    char reading;
    char unknown_5[3];
    void ApplyDword(char*, unsigned long*, unsigned long, unsigned long, unsigned long);
};

struct EventEntry {
    int id;
    int unk4;
    int unk8;
    int unkc;
};

class Class_004e1d20 {
public:
    char unknown_0[68];
    char* name;
    char field_48;
    char boosted;
    char unknown_4a[2];
    unsigned long oldPriorityClass;
    int oldThreadPriority;
    Class_004e1d20(int);
    Class_004e1d20(int, int);
    ~Class_004e1d20(void);
};

class Class_004e1d60 {
public:
    char unknown_0[64];
    int field_40;
    int field_44;
    char field_48;
    char boosted;
    char unknown_4a[2];
    unsigned long oldPriorityClass;
    int oldThreadPriority;
    void StartTimer(int, int);
};

class Class_004e20a0 {
public:
    char unknown_0[1];
    double RestartTimer(void);
};

class Class_004e1e50 {
public:
    char unknown_0[1];
    void ReportElapsedTime(char*);
};

class Class_004e1e30 {
public:
    double time;
    char unknown_8[64];
    char stopped;
    char unknown_49[7];
    double GetElapsedSeconds(void);
};

struct Class_004e2150 {
    double field_0;
    char unknown_8[64];
    unsigned char field_48;
    char unknown_49[7];
    void StopTimer(void);
};

class Class_004e2160 {
public:
    double value;
    char unknown_8[64];
    char flag_48;
    char unknown_49[7];
    void ResumeTimer(void);
};

struct Class_004e2180 {
    int field_0;
    int field_4;
    char unknown_8[64];
    unsigned char field_48;
    char unknown_49[3];
    void ResetTimer(void);
};

struct Class_004e21a0 {
    double time;
    char unknown_8[64];
    char stopped;
    char unknown_49[7];
    void AddElapsed(double);
};

struct Class_004e21c0 {
    double time;
    char unknown_8[64];
    char stopped;
    char unknown_49[7];
    void SetElapsed(double);
};

class Class_004e21f0 {
public:
    double time;
    char unknown_8[16];
    double history[5];
    char unknown_40[8];
    char stopped;
    char unknown_49[7];
    double ComputeSampleRate(void);
};

class Alloc_004e2b60 {
public:
    char unknown_0[1];
    char* FUN_004e2b60(unsigned int);
};

class Class_004e2950 {
public:
    int unknown_0;
    LiveNode* head;
    char unknown_8[8];
    void FUN_004e2950(LiveNode*);
};

class Class_004e29b0 : public Tree_004da8d0 {
public:
    void FUN_004e29b0(LiveNode*);
};

class Class_004e2a30 : public Class_004e2950 {
public:
    LiveNode* FUN_004e2a30(int, int);
    void _Consval(std::pair<NameKey, Value_004e2250>*, std::pair<NameKey, Value_004e2250>&);
};

struct Kfn_004e2250 {
    char unknown_0[1];
    NameKey& operator()(std::pair<NameKey, Value_004e2250>&);
};

struct Less_004e2250 {
    char unknown_0[1];
    unsigned char operator()(NameKey&, NameKey&);
};

struct Value_004e2250 {
    char text[500];
};

struct Less_004e2580 {
    char unknown_0[1];
    unsigned char operator()(char*&, char*&);
};

class Class_004e2580 {
public:
    Less_004e2580 compare;
    char unknown_1[3];
    LiveNode* head;
    NameMapIter FUN_004e2580(char*&);
    LiveNode* Lbound(char*&);
};

struct Less_004e2620 {
    char unknown_0[1];
    unsigned char operator()(NameKey&, NameKey&);
};

class Class_004e2620 {
public:
    Less_004e2620 compare;
    char unknown_1[3];
    LiveNode* head;
    int nilref;
    unsigned int size;
    NameMapIter FUN_004e2620(LiveNode*, LiveNode*, Value_004df590&);
    LiveNode*& _Root(void);
    LiveNode*& _Lmost(void);
    LiveNode*& _Rmost(void);
    static LiveNode*& _Left(LiveNode*);
    static LiveNode*& _Right(LiveNode*);
    static LiveNode*& _Parent(LiveNode*);
    static int& _Color(LiveNode*);
    void _Lrotate(LiveNode*);
    void _Rrotate(LiveNode*);
};

struct Data1 {
    int field_0;
};

struct Data2 {
    char field_0;
};

class Class_004e2cc0 {
public:
    char unknown_0[1];
    bool ReadBool(char*, unsigned int);
};

class Class_004e2d00 {
public:
    void* key;
    int FUN_004e2d00(char*, int, int, int);
};

class Class_004e2ce0 {
public:
    char unknown_0[1];
    void WriteBool(void*, unsigned int);
};

class Class_004e2d70 {
public:
    void* field_0;
    void WriteDword(char*, unsigned long);
};

class Class_004e2d90 {
public:
    void* key;
    unsigned long ReadDword(char*, unsigned long, unsigned long, unsigned long);
};

class Class_004e2e00 {
public:
    void* key;
    void WriteInt(char*, int);
};

class Class_004e2e60 {
public:
    void* key;
    char reading;
    char unknown_5[3];
    void ApplyInt(char*, int*, int, int, int);
};

class Class_004e2ea0 {
public:
    void* key;
    char reading;
    char unknown_5[3];
    void ApplyInt2(char*, int*, int, int, int);
};

class Class_004e2ee0 {
public:
    void* key;
    char reading;
    char unknown_5[3];
    void ApplyShort(char*, short*, short, short, short);
};

class Class_004e2f30 {
public:
    void* key;
    char reading;
    char unknown_5[3];
    void ApplyWord(char*, unsigned short*, unsigned short, unsigned short, unsigned short);
};

class Class_004e2f90 {
public:
    void* key;
    char reading;
    char unknown_5[3];
    void ApplyChar(char*, char*, char, char, char);
};

class Class_004e3030 {
public:
    void* key;
    char reading;
    char unknown_5[3];
    void ApplyByte(char*, unsigned char*, unsigned char, unsigned char, unsigned char);
};

#include "../src/ai/squad_timer.h"

#include "../src/ai/group.h"

struct FixedParts_00407ae0 {
    unsigned int frac : 16;
    unsigned int whole : 16;
};

union Fixed_00407ae0 {
    union { int value; FixedParts_00407ae0 parts; };
};

class Class_00408620 {
public:
    char unknown_0[16];
    int field_10;
    void TryIssueIdleFactoryBuildOrder(Unit*);
};

struct Info_00408670 {
    char unknown_0[140];
    float field_8c;
    float field_90;
    float field_94;
    float field_98;
};

struct Unit_00408670 {
    char unknown_0[150];
    Info_00408670* info;
};

class Class_00408830 {
public:
    Player* player;
    void AssignSquads(void);
};

class Class_004089a0 {
public:
    Player* player;
    char unknown_4[53];
    Unit* cursor;
    void RetargetWeapons(int);
};

class Class_00408bf0 {
public:
    void* target;
    char unknown_4;
    int countdown;
    char unknown_9[8];
    Slot_4b62d0* timers[10];
    void TickTimers(void);
};

class Owner {
public:
    char* player;
    unsigned char index;
    std::vector<Unit*> units;
    UnitList_00409160 list_15;
    Group_00409160 group_25;
    Vec3 pos_35;
    Vec3 pos_41;
    std::vector<Elem_0040cc40> vec_4d;
    Pos_00409160 center;
    std::vector<Elem_0040cfb0> vec_65;
    int field_75;
    int field_79;
    std::vector<short> vec_7d;
    std::vector<unsigned char> vec_8d;
    std::vector<unsigned char> vec_9d;
    std::vector<Elem_0040d4f0> vec_ad;
    char unknown_bd[4];
    int* d;
    char unknown_c5[8];
    std::vector<Elem_0040d550> values;
    char unknown_dd[4];
    int* locked;
    char unknown_e5[8];
    unsigned int lastTick;
    Point16 spacing0;
    Point16 offset0;
    int margin0;
    Point16 spacing1;
    Point16 offset1;
    int margin1;
    int field_109;
    Owner(unsigned char);
    bool FindCellNearFeatures(UnitDef*, Vec3*, std::vector<Elem_0040cc40>*, int, Point16*);
    bool FindRandomPlacementCell(UnitDef*, Vec3*, int, Point16*);
    void InitUnitTables(void);
};

struct Player_00409730 {
    char unknown_0[324];
    unsigned short field_144;
};

struct Elem_0040cc40_2 {
    Point16_00437de0 pos;
    float key;
    Elem_0040cc40_2(Elem_0040cc40_2&);
    Elem_0040cc40_2(void);
    unsigned char operator<(Elem_0040cc40_2&);
};

struct Unit_3 {
    int unknown_0;
};

struct Sub {
    short v0;
    short v1;
    short v2;
    short v3;
    int v4;
};

struct Class_0040a7b0 {
    char unknown_0[77];
    std::vector<Elem_0040cc40> cells;
    void BuildFeatureCells(void);
};

class Class_0040aa40 {
public:
    Player* owner;
    char pad4;
    char unknown_5[112];
    int builders;
    int hasSpecial;
    std::vector<short> counts;
    char unknown_8d[16];
    std::vector<char> weights;
    void RefreshUnitLists(void);
};

struct FloatVec {
    float x;
    float y;
    float z;
    FloatVec(void);
};

class Class_0040ad20 {
public:
    char unknown_0[237];
    unsigned int lastTick;
    void UpdateEveryThirtyTicks(void);
};

struct Weapon_3 {
    char unknown_0[273];
    unsigned char flags;
};

struct Rating {
    char normal;
    char metal;
    char energy;
};

static_assert(sizeof(Class_004cdb40) == 0x294, "Class_004cdb40 size");
static_assert(sizeof(Class_0047bf20) == 0x548, "Class_0047bf20 size");
static_assert(sizeof(Surfaces_0047bdf0) == 0x14, "Surfaces_0047bdf0 size");
static_assert(sizeof(MoviePlayer) == 0x5b8, "MoviePlayer size");
static_assert(sizeof(Class_0047bf70) == 0x5b4, "Class_0047bf70 size");
static_assert(sizeof(Class_0047c230) == 0x410, "Class_0047c230 size");
static_assert(sizeof(Class_0047c2a0) == 0x548, "Class_0047c2a0 size");
static_assert(sizeof(Class_0047c3a0) == 0x548, "Class_0047c3a0 size");
static_assert(sizeof(Class_004ceee0) == 0x28c, "Class_004ceee0 size");
static_assert(sizeof(Class_004cf0b0) == 0x1e8, "Class_004cf0b0 size");
static_assert(sizeof(BitmapInfo_004b5510) == 0x428, "BitmapInfo_004b5510 size");
static_assert(sizeof(Obj) == 0x729, "Obj size");
static_assert(sizeof(Surface_4b6570) == 0x4, "Surface_4b6570 size");
static_assert(sizeof(Screen_004c6210) == 0x10, "Screen_004c6210 size");
static_assert(sizeof(Game_004caec0) == 0x614, "Game_004caec0 size");
static_assert(sizeof(Game_004cb170) == 0x614, "Game_004cb170 size");
static_assert(sizeof(Class_004cfb40) == 0x28c, "Class_004cfb40 size");
static_assert(sizeof(Class_004cf180) == 0x1b8, "Class_004cf180 size");
static_assert(sizeof(Class_004cf230) == 0x28, "Class_004cf230 size");
static_assert(sizeof(Class_004cf370) == 0x28, "Class_004cf370 size");
static_assert(sizeof(IDirectSound3DBuffer) == 0x4, "IDirectSound3DBuffer size");
static_assert(sizeof(Class_004cf800) == 0x1e4, "Class_004cf800 size");
static_assert(sizeof(Class_004cf8a0) == 0x1e4, "Class_004cf8a0 size");
static_assert(sizeof(CavedogRegistryKey) == 0x34, "CavedogRegistryKey size");

#pragma pack(pop)

#endif
