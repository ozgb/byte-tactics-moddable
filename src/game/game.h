#ifndef GAME_H
#define GAME_H

#include "../graphics/rect.h"
#include "../graphics/movies.h"
#include "../ingame/frame_timers.h"
#include "../map/grid.h"
#include "../network/player.h"
#include "../network/settings.h"
#include "settings.h"
#include "../util/vec3.h"
#include "../../interfaces/ref.h"
#include "../frontend/net_game_flag_low_bits.h"
#include "../frontend/connection_info.h"
#include "../frontend/track_option_flags.h"
#include "../frontend/options_slider_menu.h"
#include "../frontend/player_info_entry.h"
#include "../../include/v4i.h"
#include "../graphics/object3do.h"
#include "../graphics/piece_frame.h"
#include "../gui/gui.h"
#include "../util/mouse_event.h"
#include "../ingame/camera_flags.h"
#include "../ingame/game_cursor_state.h"
#include "../ingame/net_game_flags.h"
#include "../ingame/cursor_input_flags.h"
#include "../ingame/orders_panel_flags.h"
#include "../ingame/graphics_option_flags.h"
#include "../ingame/broadcast_toggle_flags.h"
#include "../ingame/info_mode_flags.h"
#include "../ingame/map_grid.h"
#include "../ingame/orders.h"
#include "../ingame/unit_info_snapshot.h"
#include "../ingame/light_bar_counters.h"
#include "../ingame/light_bar_sprite.h"
#include "../map/feature_anim_pool.h"
#include "../map/grid2.h"
#include "../map/map_info.h"
#include "../map/player_grid.h"
#include "../map/pool.h"
#include "../map/radar_timer.h"
#include "../network/chat_hud_entry.h"
#include "../network/chat_hud_entry_view.h"
#include "../network/player_id_entry.h"
#include "../sound/sound_setting_flags.h"
#include "../units/f14373.h"
#include "../util/point.h"
#include "../weapons/debris.h"
#include "../weapons/weapon_def.h"
#include "ui_option_flags.h"
#include "pre_battle_flags.h"
#include "loading_flags.h"
#include "menu.h"
#include "main2_menu.h"
#include "all_missions_flags.h"
#include "loading_player_slots.h"
#include "mission_option_flags.h"
#include "side_def.h"
#include "end_game_player_slot.h"
#include "view_flags.h"
#include "chat_recipients.h"

struct AnimFileEntry;
struct BuildMenuPage;
struct Cell;
struct ChatPacket;
class MemoryCacheView;
struct Object3do;
class UnitSyncView;
struct ParticleListsView;
class SpatialBucket;
class SoundView;
struct ConnectionBlob;
struct Display;
struct GameDescEntry;
struct SoundCategoryEntry;
struct EyeView;
struct Feature;
struct FeatureSpot;
struct Font;
class HapiBank;
struct FrameTable;
struct MapTileSet;
class Mission;
class MissionConditions;
struct DplConnectionView;
struct Projectile;
class Pathfinder;
struct PlayerInfo;
struct SideData;
struct GafSequenceSource;
struct LayerHeadView;
struct Unit;
struct UnitDef;

#pragma pack(push, 1)
struct Game {
    union {
        struct {
            char unknown_0;
            signed char version;
            signed char versionMinor;
            unsigned char field_3;
        };
        int field_0;
    };
    char* build_date;
    char* build_time;
    Display* displayContext;
    SoundView* sound;
    union {
        struct {
            char session[4];
            char pad_18[1069];
            V4i* guids;
            char pad_449[40];
            Settings settings;
            char pad_4c1[8];
            int from_id;
            int lobby1;
            char pad_4d1[12];
        };
        struct {
            char session_14_1[1];
            char pad_15[1156];
            int field_499;
            char pad_49d[64];
        };
        char session_14[1117];
        char net_14[1225];
    };
    char pad_4dd[8];
    DplConnectionView* dplConnection;
    char pad_4e9[4];
    int compressionDisabled;
    int netMode;
    char pad_4f5[4];
    int count;
    int pad_4fd;
    char pad_501[16];
    Player* slowest;
    int lag;
    union {
        struct {
            char menu[1];
            char pad_51a[3];
            void* gaf;
            int field_521;
            int field_525;
            char pad_529[4];
            int field_52d;
            LayerHeadView* layer;
            char pad_535[72];
            int team_index;
            int selected;
            char pad_585[4];
            int field_589;
            char unknown_58d[2110];
            unsigned char textColor;
            unsigned char pad_dcc;
            char pad_dcd[1];
            unsigned char pad_dce;
            unsigned char pad_dcf;
            char pad_dd0[3];
            unsigned char color2;
            unsigned char pad_dd4;
            unsigned char pad_dd5;
            char pad_dd6[1];
            unsigned char pad_dd7;
            char pad_dd8[1];
            unsigned char pad_dd9;
            unsigned char pad_dda;
            char pad_ddb[1040];
            void* surface;
            char pad_11ef[256];
            ChatHudEntryView entries[30];
            int lobbySyncTick;
            Player players[10];
            char pad_2851[331];
            union {
                int duplicateIds;
                int duplicatePlayerTypeFlag;
            };
        };
        struct {
            int pad_519;
            char unknown_51d[2222];
            unsigned char colors[16];
            char pad_ddb_a[1300];
            char chatHudRing[4];
            char unknown_12f3[1868];
            PlayerInfo* infos[11];
            char unknown_1a6b[287];
            Ref<SideData> p1b8a;
            char unknown_1b8e[327];
            Ref<SideData> p1cd5;
            char unknown_1cd9[281];
            short field_1df2;
            char pad_1df4[2988];
        };
        struct {
            char menu_519[16];
            char unknown_529_a[2210];
            unsigned char colors_dcb[42];
            char pad_df5[1274];
            char chatHudRing_12ef[30][72];
            char pad_1b5f[4];
            Player players_1b63_14b[1];
            char unknown_1cae[3314];
        };
        struct {
            char message[24];
            char unknown_531[3559];
            ChatHudEntry entries_1318[30];
            char pad_1b88[2];
            PlayerInfoEntry players_1b8a[10];
            char pad_2878[296];
        };
        struct {
            Main2Menu menu_519_18;
            char unknown_531_a[5686];
            PlayerIdEntry players_1b67[10];
            char pad_2855[331];
        };
        struct {
            OptionsSliderMenu menu_519_1c;
            char unknown_535_a[5802];
            PlayerGrid players_1bdf[10];
            char pad_28cd[211];
        };
        struct {
            Menu gui_519;
            char unknown_559[5642];
            Player players_1b63[11];
            char pad_299c[4];
        };
        char menu_519_8b2[2226];
        Gui gui;
        char menu_519_164a[5706];
        char message_519[5745];
        char messages[9351];
    };
    SkirmishOptionsTable* options;
    union {
        struct {
            int loaded[10];
            char pad_29cc[4];
            int startPosAssignAck[11];
            int startPosShuffle[10];
            char pad_2a24[4];
            int startPosShuffleReady;
            char pad_2a2c[4];
        };
        int shareVisionReady[11];
        LoadingPlayerSlots slots_29a4;
    };
    UnitSyncView* sync;
    int recvPacketSize;
    union {
        unsigned char* recvPacketPtr;
        ChatPacket* chat;
    };
    union {
        unsigned short numPlayers;
        short numPlayersSigned;
    };
    unsigned short tail;
    unsigned short head;
    unsigned char localPlayer;
    unsigned char playerIndex;
    union {
        unsigned short flags_2a44;
        unsigned char netGameFlags;
        struct {
            unsigned char leaving : 1;
            unsigned char rest_2a44 : 7;
        } bits_2a44_2a44;
        NetGameFlags netGameFlags_2a44;
        struct {
            unsigned short bit0 : 1;
            unsigned short bit1 : 1;
            unsigned short bit2 : 1;
            unsigned short bit3 : 1;
            unsigned short bits4_2a44 : 12;
        } bits_2a44;
        struct {
            unsigned short pad_2a44 : 2;
            unsigned short bit2 : 1;
            unsigned short rest_2a44 : 13;
        } bits_2a44_1;
    };
    unsigned char field_2a46;
    union {
        struct {
            char pad_2a47[4];
            void* data[15];
        };
        void* data16[16];
    };
    char pad_2a87[20];
    void* list;
    V4i* sessions;
    ConnectionBlob* conns;
    GameDescEntry* desc;
    char* shared;
    union {
        struct {
            unsigned short bit0 : 1;
            unsigned short bit1 : 1;
            unsigned short bits2 : 14;
        };
        NetGameFlagLowBits dplayAddressDialogFlags;
        unsigned short dplayAddressDialogFlags_2aaf;
    };
    union {
        struct {
            char pad_2ab1[155];
            unsigned short low_2b4c : 4;
            unsigned short flag_2b4c : 1;
            unsigned short rest_2b4c : 11;
            char pad_2b4e[28];
        };
        struct {
            char pad_2ab1_a[155];
            PreBattleFlags flags_2b4c;
            char pad_2b4e_a[28];
        };
        char buffer_2ab1[185];
    };
    union {
        char game_info[84];
        struct {
            char unknown_2b6a[56];
            V4i joinGuid;
            char pad_2bb2[12];
        };
    };
    unsigned char frontendState;
    unsigned char frontendSubState;
    unsigned char frontendNextSubState;
    union {
        struct {
            char gameName[16];
            char pad_2bd1;
        };
        char gameName_2bc1[17];
    };
    char nickname[17];
    union {
        struct {
            char password[11];
            unsigned short lobbyUiDirtyFlags;
            unsigned char mode_2bf0;
            unsigned char chatRecipients[11];
            char pad_2bfc[44];
            int table_2c28[10];
            char pad_2c50[36];
            unsigned char lockFlags;
            char pad_2c75[1];
            MouseEvent view;
            short x;
            short y;
            int boxStartX;
            int boxStartHeight;
            int boxStartZ;
            int boxEndX;
            int boxEndHeight;
            int boxEndZ;
            Vec3 pos;
            int boxStartTick;
            unsigned short hoverUnitId;
            unsigned short cellFeature;
            char cursorMode;
            char pad_2cbf[4];
            unsigned char orderMode;
            unsigned short buildTypeIndex;
            unsigned char pad_2cc6 : 5;
            unsigned char bit5_2cc6 : 1;
            unsigned char bit6_2cc6 : 1;
            unsigned char rest_2cc6 : 1;
            GameCursorState cursor;
            WeaponDef weapons[256];
            int projectileCount;
            Ref<Projectile> projectiles;
            void* field_141fb;
            void* field_141ff;
            void* field_14203;
            Ref<Pathfinder> pathfinder;
            Ref<FeatureSpot> spots;
            Ref<Unit> unit;
            char pad_14213[8];
            int list_1421b;
            void** fogGrid;
            int baseX;
            int baseY;
            int mapWidth;
            int mapHeight;
            int width;
            int height;
            int screenTilesX;
            int screenTilesY;
            char pad_14243[16];
            int featureCount;
            int scanIndex;
            int windSpeedMin;
            int windSpeedMax;
            int gravity;
            float tidal;
            void* radarFrame;
            Ref<Feature> features;
            Ref<unsigned short> visibilityMask;
            int count_14277;
            EyeView* eyes;
            unsigned char seaLevel;
            unsigned char debugMode;
            unsigned short mapFlags;
            MapTileSet* iconSet;
            Ref<Cell> cells;
            unsigned short* mapValues;
            Grid grid1;
            Ref<SpatialBucket> cells_1429f;
            unsigned int width_142a3;
            unsigned int height_142a7;
            char pad_142ab[4];
            int field_142af;
            int field_142b3;
            Ref<SpatialBucket> overflowBucket;
            Rect viewLimit;
            char radarViewportRect[16];
            void* finalSurface;
            void* mappedSurface;
            void* pictureSurface;
            short posX;
            short posY;
            short width_142eb;
            short screen_h;
            short blinkTimer;
            unsigned short flags_142f1;
            int value_142f3;
            Projectile* tracked;
            int xs[4];
            int ys[4];
            unsigned char valid[4];
            int scrollX;
            int scrollY;
            int x2;
            int y2;
            int value_1432f;
            int value_14333;
            int sum_x;
            int sum_y;
            Vec3 trackedPos;
            short value_1434b;
        };
        struct {
            char pad_2be3[11];
            struct {
                unsigned short dirty : 1;
                unsigned short bits1 : 15;
            } bits_2bee_1;
            char pad_2bf0[1];
            unsigned char chatRecipients_2bf1[10];
            char pad_2bfb[45];
            int playerIds[11];
            char pad_2c54[32];
            struct {
                unsigned short locked : 1;
                unsigned short locked_rest : 15;
            } bits_2c74;
            int cursorScreenX;
            int cursorScreenY;
            int cursorKeyFlags;
            char pad_2c82[12];
            Point16 point;
            char pad_2c92[26];
            short xWhole;
            char pad_2cae[2];
            short yWhole;
            char pad_2cb2[2];
            short zWhole;
            char pad_2cb6[16];
            unsigned char inputFlags;
            char pad_2cc7[24];
            int middleScrollActive;
            char unknown_2ce3[70952];
            Pool pool;
            char pad_1421f[98];
            MapViewFlags mapFlags_14281;
            char pad_14283[28];
            Grid spatialGrid;
            char pad_142af[12];
            int viewLeft;
            int viewTop;
            int viewRight;
            int viewBottom;
            char pad_142cb[28];
            struct {
                short v[4];
            } dim;
            char pad_142ef[2];
            unsigned char viewDirtyFlags_142f1;
            char pad_142f2;
            char pad_142f3[90];
        };
        struct {
            char pad_2be3_a[11];
            struct {
                unsigned short dirty : 1;
            } bits_2bee_3;
            char pad_2bf0_a[1];
            ChatRecipients chatRecipients_2bf1_b;
            char pad_2bfc_a[122];
            Point view_2c76;
            char pad_2c7e[44];
            void* pos_2caa;
            char pad_2cae_a[24];
            CursorInputFlags inputFlags_2cc6;
            char pad_2cc7_a[70964];
            FeatureAnimPool anim;
            char pad_14273[14];
            struct {
                unsigned short bit0 : 1;
                unsigned short bit1 : 1;
                unsigned short flag2 : 1;
                unsigned short flag3 : 1;
                unsigned short rest : 12;
            } bits_14281_1;
            char pad_14283_a[28];
            Grid2 spatialGrid_1429f;
            char pad_142b7[20];
            char radarViewport[38];
            CameraFlags viewDirtyFlags;
            char pad_142f3_a[90];
        };
        struct {
            char pad_2be3_b[11];
            NetGameFlagLowBits lobbyUiDirtyFlags_2bee;
            char pad_2bf0_b[134];
            char viewSpanB_2c76[28];
            char pad_2c92_a[71017];
            MapInfo info_141fb;
            char pad_14277[10];
            struct {
                unsigned short bit0_14281 : 1;
                unsigned short bit1_14281 : 1;
                unsigned short bit2_14281 : 1;
                unsigned short rest_14281 : 13;
            } bits_14281;
            char unknown_14283_b[110];
            struct {
                unsigned short blinkOn : 1;
                unsigned short rest_142f1 : 15;
            } bits_142f1;
            char pad_142f3_b[90];
        };
        struct {
            char pad_2be3_c[11];
            struct {
                unsigned short bits0 : 4;
                unsigned short flag4 : 1;
                unsigned short bits5 : 11;
            } bits_2bee;
            char pad_2bf0_c[134];
            char viewSpanC_2c76[68];
            char pad_2cba[70977];
            MapGrid sortUnits;
            char unknown_1428b[102];
            struct {
                unsigned short flags_142f1_bit0 : 1;
                unsigned short flags_142f1_bit1 : 1;
                unsigned short flags_142f1_mapChanged : 1;
                unsigned short flags_142f1_rest : 13;
            } bits_142f1_1;
            char pad_142f3_c[90];
        };
        struct {
            char pad_2be3_d[11];
            struct {
                unsigned short pad_2bee : 5;
                unsigned short flags_2bee : 3;
                unsigned short rest_2bee : 8;
            } bits_2bee_2;
            char unknown_2bf0_d[71425];
            struct {
                unsigned short f0 : 1;
                unsigned short f1 : 1;
                unsigned short flagA : 1;
                unsigned short f3 : 5;
                unsigned short fhi : 8;
            } bits_142f1_2;
            char pad_142f3_d[90];
        };
        struct {
            char password_2be3[17];
            char unknown_2bf4[71419];
            RadarTimer timer;
            char pad_142f3_e[90];
        };
        char name_2be3[64];
        char password_2be3_1176a[71530];
    };
    unsigned char scrollSpeed;
    unsigned char flags_1434e;
    unsigned short unitsPerPlayer;
    unsigned short poolCount;
    int f14353;
    Ref<Unit> units;
    Ref<Unit> unitsEnd;
    unsigned short* list_1435f;
    void* hotRadar;
    int count_14367;
    int hotRadarCount;
    unsigned short focusUnitId;
    unsigned short autoFollowTicks;
    union {
        struct {
            unsigned int bit0_14373 : 1;
            unsigned int autoFollow : 1;
            unsigned int rest : 30;
        };
        F14373 autoFollowFlags;
        unsigned int autoFollowFlags_14373;
    };
    Ref<Object3do*> types;
    MemoryCacheView* unitTable;
    char pad_1437f[4];
    void* xform;
    void* projected;
    void* assemPoints;
    int unitDefCount;
    int unitDefCountBits;
    int unitDefEnumDirty;
    Ref<UnitDef> defs;
    char pad_1439f[8];
    union {
        struct {
            char pad_143a7[4];
            char pad_143ab[1020];
        };
        char paletteRgba[1024];
    };
    int baseHeight;
    int count_147ab;
    AnimFileEntry* entries_147af;
    char pad_147b3[8];
    void* gaf_147bb;
    void* gaf_147bf;
    void* gaf_147c3;
    void* gaf_147c7;
    void* gaf_147cb;
    void* smoke1;
    void* smoke2;
    int fire1;
    int alfboom1;
    void* radlogo;
    void* radlogohigh;
    void* nuclogo;
    void* src1;
    void* src2;
    void* flamestreamGaf;
    union {
        struct {
            void* src3;
            int explode2;
            int explode3;
            int explode4;
            int explode5;
            int nuke1;
        };
        void* sources[6];
    };
    void* gaf_1480f;
    unsigned short* image_14813;
    unsigned short* image_14817;
    int igpaused;
    union {
        struct {
            unsigned short* sidePanelTopSeq[5];
            unsigned short* sidePanelBotSeq[5];
        };
        unsigned short* sidePanelRows[10];
    };
    unsigned short* sidePanelSideSeq[5];
    FrameTable* losTable;
    union {
        void* black[4];
        struct {
            int black1Seq;
            int black2;
            int black3;
            int black4;
        };
    };
    union {
        void* gray[4];
        struct {
            int gray1Seq;
            int gray2;
            int gray3;
            int gray4;
        };
    };
    union {
        struct {
            GafSequenceSource* table[1];
            int cursorAttack;
            int cursorAirstrike;
            int cursorTooFar;
            int cursorCapture;
            int cursorDefend;
            int cursorRepair;
            int cursorPatrol;
            int cursorPickup;
            int cursorTeleport;
            int cursorRevive;
            int cursorReclamate;
            int cursorLoad;
            int cursorUnload;
            int cursorMove;
            int cursorSelect;
            int cursorFindSite;
            int cursorRed;
            int cursorGrn;
            unsigned short* cursorNormal;
            int cursorHourglass;
        };
        void* cursorModeSeq[21];
    };
    int pathIcon;
    void* logos;
    void* logos32;
    int count_148df;
    void** blocks;
    int count_148e7;
    int* data_148eb;
    int gafAnimClearedSlot;
    int fxGaf;
    int igTitles;
    int vismasks;
    int fog;
    int cursorsGaf;
    union {
        struct {
            int array_14907[1];
            char pad_1490b[16];
        };
        int panelTop[5];
    };
    int count_1491b;
    Debris debris[300];
    void* lists[3];
    void* explosionLensFrame;
    Object3do slots[300];
    union {
        char vertices[300][96];
        Vec3 vertices_1e88f[300][8];
    };
    PieceFrame frames[300][6];
    int soundCount;
    void* sounds[256];
    union {
        struct {
            char soundNames[1][32];
            char pad_33e33[8160];
            char soundFiles[256][32];
        };
        char soundNames_33e13[256][32];
        char soundNames_33e13_4000[512][32];
    };
    SoundCategoryEntry* entries_37e13;
    int entry_count;
    void* screen;
    int width_37e1f;
    int screenHeight;
    union {
        struct {
            Rect rect;
            int viewWidth;
            int viewHeight;
            LightBarCounters a;
            LightBarSprite b;
            int field_37e98;
            unsigned short unitIndex;
            unsigned short unitDefIndex;
            char name[30];
            unsigned short flags_37ebe;
            unsigned short moveOrder : 3;
            unsigned short cloak : 2;
            unsigned short activation : 2;
            unsigned short flags_7 : 9;
            char pad_37ec2[2];
            unsigned int windCounter;
            int windSpeedDenominator;
        };
        struct {
            int viewCullMinX;
            int viewCullMinY;
            int viewCullMaxX;
            int viewCullMaxY;
            char pad_37e37[89];
            int panel;
            void* sprite;
            char pad_37e98[38];
            unsigned char ordersPanelFlags;
            char pad_37ebf[13];
        };
        struct {
            int viewport[4];
            char pad_37e37_a[41];
            UnitInfoSnapshot selectionInfoCache;
            char pad_37e9c[34];
            OrdersPanelFlags ordersPanelFlags_37ebe;
            char pad_37ec0[12];
        };
        struct {
            char pad_37e27[151];
            struct {
                unsigned short bit0_37ebe : 1;
                unsigned short pad_37ebe : 10;
                unsigned short bit11_37ebe : 1;
                unsigned short rest_37ebe : 4;
            } bits_37ebe;
            char pad_37ec0_a[12];
        };
        struct {
            char pad_37e27_a[151];
            struct {
                unsigned short bit0_37ebe : 1;
                unsigned short bit1_37ebe : 1;
                unsigned short bit2_37ebe : 1;
                unsigned short bit3_37ebe : 1;
                unsigned short bit4_37ebe : 1;
                unsigned short bit5_37ebe : 1;
                unsigned short bit6_37ebe : 1;
                unsigned short bit7_37ebe : 1;
                unsigned short bits8_37ebe : 3;
                unsigned short bit11_37ebe : 1;
                unsigned short rest_37ebe : 4;
            } bits_37ebe_1;
            char pad_37ec0_b[12];
        };
        struct {
            char pad_37e27_b[151];
            struct {
                unsigned short flags_0 : 4;
                unsigned short loaded : 1;
                unsigned short flags_5 : 11;
            } bits_37ebe_2;
            char pad_37ec0_c[12];
        };
        struct {
            char pad_37e27_c[151];
            struct {
                unsigned short pad_37ebe : 6;
                unsigned short bit6_37ebe : 1;
                unsigned short rest_37ebe : 9;
            } bits_37ebe_3;
            char pad_37ec0_d[12];
        };
        struct {
            char pad_37e27_d[151];
            struct {
                unsigned short low_37ebe : 11;
                unsigned short bit11_37ebe : 1;
                unsigned short high_37ebe : 4;
            } bits_37ebe_4;
            char pad_37ec0_e[12];
        };
        struct {
            char pad_37e27_e[151];
            Orders ordersPanelFlags_37ebe_6;
            char pad_37ec4[8];
        };
        char region_37e27[165];
    };
    union {
        struct {
            int windX;
            int windY;
            int windZ;
        };
        Vec3 wind;
    };
    unsigned short windDirection;
    int windSpeed;
    float windStrength;
    int windEnabled;
    union {
        struct {
            unsigned short maxUnitsSetting;
            unsigned short lobbyInitScratch;
            unsigned short maxUnits;
            unsigned short maxUnits_37eec;
            int difficulty;
            int flag_37ef2;
            int commanderDeath;
            int interfaceType;
            int mode;
            int screenchat;
            GraphicsOptionFlags flags_37f06;
            int brightness;
            int volume1;
            int volume2;
            unsigned short flags_37f14;
            unsigned char cdMode;
            unsigned char unitChat;
            unsigned char unitChatText;
            SoundSettingFlags soundFlags;
            int width_37f1b;
            int height_37f1f;
            int textScroll;
            int textLines;
            int mousespeed;
            union {
                unsigned short uiOptionFlags;
                struct {
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
                    unsigned short flag10 : 1;
                    unsigned short rest_37f2f : 5;
                } uiOptionBits;
            };
            unsigned int lobbyTimeoutSecs;
            int netSendDropPct;
        };
        struct {
            char pad_37ee6[12];
            unsigned char side;
            char pad_37ef3[19];
            unsigned char visualFlagsByte;
            char pad_37f07[5];
            short volume1Word;
            char pad_37f0e[2];
            short volume2Word;
            char pad_37f12[2];
            unsigned char musicMode;
            unsigned char pad_37f15;
            char pad_37f16[3];
            unsigned char soundFlags_37f19;
            char pad_37f1a[1];
            unsigned short width_37f1b_2;
            char pad_37f1d[2];
            unsigned short height_37f1f_2;
            char pad_37f21[14];
            unsigned char uiOptionFlags_37f2f_1;
            unsigned char matchFlags;
            char pad_37f31[8];
        };
        struct {
            char pad_37ee6_a[32];
            union {
                unsigned short visualFlags;
                struct {
                    unsigned short damagebars : 1;
                    unsigned short antiAlias : 1;
                    unsigned short shadows : 1;
                    unsigned short vehicleShadows : 1;
                    unsigned short featureShadows : 1;
                    unsigned short shading : 1;
                    unsigned short ditheredFog : 1;
                    unsigned short unused7 : 1;
                    unsigned short switchAlt : 1;
                    unsigned short rest : 7;
                } visualBits;
            };
            char pad_37f08[12];
            TrackOptionFlags flags_37f14_37f14;
            char pad_37f16_a[3];
            unsigned short soundFlags_37f19_2;
            char pad_37f1b[20];
            UiOptionFlags uiOptionFlags_37f2f;
            char pad_37f31_a[8];
        };
        struct {
            char pad_37ee6_b[32];
            struct {
                unsigned short bit0 : 1;
                unsigned short bit1 : 1;
                unsigned short bit2 : 1;
                unsigned short bit3 : 1;
                unsigned short bit4 : 1;
                unsigned short bit5 : 1;
                unsigned short bit6 : 1;
                unsigned short rest : 9;
            } bits_37f06;
            char pad_37f08_a[39];
            struct {
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
                unsigned short flag10 : 1;
                unsigned short rest_37f2f : 5;
            } bits_37f2f;
            char pad_37f31_b[8];
        };
        struct {
            char pad_37ee6_c[32];
            struct {
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
            } bits_37f06_1;
            char pad_37f08_b[39];
            struct {
                unsigned short pad_37f2f : 7;
                unsigned short bit7_37f2f : 1;
                unsigned short bit8_37f2f : 1;
                unsigned short bit9_37f2f : 1;
                unsigned short rest_37f2f : 6;
            } bits_37f2f_1;
            char pad_37f31_c[8];
        };
        struct {
            char pad_37ee6_d[73];
            struct {
                unsigned short screenBitA : 1;
                unsigned short screenBitB : 1;
            } bits_37f2f_2;
            char pad_37f31_d[8];
        };
        char block[83];
    };
    int sides;
    union {
        struct {
            char sideDefsSpanFirst_37f3d[562];
            char pad_3816f[2248];
            unsigned int lastTick;
            int frames_38a3b;
            int scrollScale;
            int carry;
            int ticks;
            unsigned short gameSpeed;
            short effectiveGameSpeed;
            short streak;
            unsigned short otherFlags;
            char installPath[1];
            char pad_38a54[255];
            char text_38b53[256];
            int movieCaptureIndex;
            int movieOutputRate;
            unsigned int nextMovieFrameTick;
            int valueSet;
            int valueSet_38c63;
            int unusedAfterMovieOutputRateDirty;
            char saveName[1];
            char pad_38c6c[255];
            HapiBank* pendingSaveStore;
            char progress;
            unsigned char loadProgress;
            unsigned char loadPctUnits;
            unsigned char loadPercent;
            unsigned char progress_38d73;
            unsigned char shade;
            unsigned short net_flags;
            ParticleListsView* lists_38d77;
            MoviePlayer* moviePlayer;
            unsigned short flags_38d7f;
            int playerCount;
            FrameTimers timers;
            int profileBarsEnabled;
            EndGamePlayerSlot slots_38dd9[10];
            char pad_3901d[58];
            int endGameState;
            unsigned int deadline;
            unsigned int nextTime;
            int done;
            int steps;
            int statBarStep;
            int savedPaletteBrightness;
            int skip;
            void* lastFrame;
            void* image_3907b;
            void* palette_3907f;
            void* currentPalette;
            void* desiredPalette;
            void* fadeTable;
            char glamour[256];
        };
        struct {
            char sideDefsSpanHead_37f3d[1154];
            char unknown_383bf[1682];
            unsigned char pauseFlags;
            char pad_38a52[1];
            char movieCaptureDir[256];
            char pad_38b53[280];
            char saveName_38c6b[256];
            char pad_38d6b[4];
            unsigned char progress_38d6f[6];
            unsigned char flags_38d75;
            char pad_38d76[9];
            AllMissionsFlags allMissionsFlags;
            char pad_38d81[8];
            int total;
            int values[8];
            char pad_38dad[994];
        };
        struct {
            char unknown_37f3d_a[34];
            char startPos[2792];
            char unknown_38a47[10];
            struct {
                unsigned short bit_38a51 : 1;
            } bits_38a51;
            char imageOutputDirectory[516];
            char pad_38c57[286];
            LoadingFlags loadingFlags;
            char pad_38d77[1048];
        };
        struct {
            SideDef sideDefs[5];
            char unknown_38a37[26];
            struct {
                unsigned short flag_38a51 : 1;
                unsigned short bits_38a51 : 15;
            } bits_38a51_2;
            char path[524];
            char pad_38c5f[278];
            struct {
                unsigned short : 2;
                unsigned short net_bit2 : 1;
            } net_bits;
            char pad_38d77_a[1046];
        };
        struct {
            char unknown_37f3d_b[2836];
            BroadcastToggleFlags flags_38a51;
            char pad_38a53[802];
            struct {
                unsigned short : 3;
                unsigned short synced : 1;
            } netBits;
            char pad_38d77_b[1046];
        };
        struct {
            char unknown_37f3d_c[2836];
            struct {
                unsigned short bits_38a51 : 15;
            } bits_38a51_1;
            char pad_38a53_a[1852];
        };
        struct {
            char unknown_37f3d_d[2836];
            struct {
                unsigned short paused : 1;
                unsigned short lagging : 1;
                unsigned short faster : 1;
                unsigned short rest : 13;
            } bits_38a51_3;
            char pad_38a53_b[1852];
        };
        struct {
            char unknown_37f3d_e[4690];
        };
    };
    union {
        int maxStats[7];
        struct {
            int maxKills;
            int maxLosses;
            int max_26;
            int max_2a;
            int max_2e;
            int max_32;
            int maxScore;
        };
    };
    int mission;
    int won;
    int unitInfoHoverTrack;
    unsigned short unitInfoHoverUnitId;
    int unitBuilderHoverTrack;
    unsigned short unitBuilderHoverUnitId;
    int showRanges;
    int showBps;
    int buildListCount;
    BuildMenuPage* buildLists;
    union {
        struct {
            char pad_391cf[24];
            char pad_391e7;
            char pad_391e8;
        };
        char campaignMissionStatus_391cf[25];
        char campaignMissionStatus[26];
    };
    Ref<Mission> net;
    Ref<MissionConditions> list_391ed;
    int mode_391f1;
    void (__stdcall *handler)(void);
    Font* fontComix;
    Font* fontSmlFont;
    union {
        struct {
            char pad_39201[1];
            char pad_39202[15];
            int connection;
            int pad_39215;
        };
        char providerGuid[16];
        ConnectionInfo info;
    };
    union {
        struct {
            int singleCommanderDeath;
            int singleMapping;
            int singleLineOfSight;
            int singleLOSType;
        };
        MissionOptionFlags singleSettings;
    };
    int multiCommanderDeath;
    int multiMapping;
    int multiLineOfSight;
    int multiLOSType;
    short endGameCountdown;
    union {
        unsigned short endGameFlags;
        unsigned char flagsByte_3923b;
        InfoModeFlags endGameFlags_3923b;
        struct {
            unsigned short bits0_3923b : 2;
            unsigned short flag2_3923b : 1;
            unsigned short bit3_3923b : 1;
            unsigned short flag4_3923b : 1;
            unsigned short rest_3923b : 11;
        } bits_3923b;
        struct {
            unsigned short bits_3923b : 2;
            unsigned short flag2 : 1;
            unsigned short bit3 : 1;
            unsigned short flag4 : 1;
            unsigned short flag5 : 1;
            unsigned short flag6 : 1;
            unsigned short rest_3923b : 9;
        } bits;
        struct {
            unsigned short bit0_3923b : 1;
            unsigned short rest_3923b : 15;
        } bits_3923b_1;
        struct {
            unsigned short : 2;
            unsigned short bit2_3923b : 1;
            unsigned short : 1;
            unsigned short bit4_3923b : 1;
        } bits_3923b_2;
    };
    int playMovie;
    int introMovieLoop;
    int cmdlineSkipIntroMovie;
    int restartMissionRequest;
    Game(void);
    Player* Current(void);
    Player* Current2(void);
};
#pragma pack(pop)

inline Game::Game(void)
    : build_date("Jul 30 1998"), build_time("11:16:36"), players_1b63()
{
    grid1.width = 0;
    grid1.height = 0;
    grid1.count = 0;
    grid1.cells = 0;
    spatialGrid.width = 0;
    spatialGrid.height = 0;
    spatialGrid.count = 0;
    spatialGrid.cells = 0;
    field_142af = 0;
    field_142b3 = 0;
}

inline Player* Game::Current(void) { return &players[playerIndex]; }

inline Player* Game::Current2(void)
{
    int i = playerIndex;
    return &players[i];
}

#if defined(__clang__) || !defined(_MSC_VER) || _MSC_VER >= 1600
#include <stddef.h>
static_assert(sizeof(Game) == 0x3924d, "Game is 0x3924d bytes");
static_assert(offsetof(Game, unknown_0) == 0x0, "unknown_0");
static_assert(offsetof(Game, field_0) == 0x0, "field_0");
static_assert(offsetof(Game, session) == 0x14, "session");
static_assert(offsetof(Game, session_14_1) == 0x14, "session_14_1");
static_assert(offsetof(Game, session_14) == 0x14, "session_14");
static_assert(offsetof(Game, net_14) == 0x14, "net_14");
static_assert(offsetof(Game, menu) == 0x519, "menu");
static_assert(offsetof(Game, pad_519) == 0x519, "pad_519");
static_assert(offsetof(Game, menu_519) == 0x519, "menu_519");
static_assert(offsetof(Game, message) == 0x519, "message");
static_assert(offsetof(Game, menu_519_18) == 0x519, "menu_519_18");
static_assert(offsetof(Game, menu_519_1c) == 0x519, "menu_519_1c");
static_assert(offsetof(Game, gui_519) == 0x519, "gui_519");
static_assert(offsetof(Game, menu_519_8b2) == 0x519, "menu_519_8b2");
static_assert(offsetof(Game, gui) == 0x519, "gui");
static_assert(offsetof(Game, menu_519_164a) == 0x519, "menu_519_164a");
static_assert(offsetof(Game, message_519) == 0x519, "message_519");
static_assert(offsetof(Game, messages) == 0x519, "messages");
static_assert(offsetof(Game, loaded) == 0x29a4, "loaded");
static_assert(offsetof(Game, shareVisionReady) == 0x29a4, "shareVisionReady");
static_assert(offsetof(Game, slots_29a4) == 0x29a4, "slots_29a4");
static_assert(offsetof(Game, flags_2a44) == 0x2a44, "flags_2a44");
static_assert(offsetof(Game, netGameFlags) == 0x2a44, "netGameFlags");
static_assert(offsetof(Game, bits_2a44_2a44) == 0x2a44, "bits_2a44_2a44");
static_assert(offsetof(Game, netGameFlags_2a44) == 0x2a44, "netGameFlags_2a44");
static_assert(offsetof(Game, bits_2a44) == 0x2a44, "bits_2a44");
static_assert(offsetof(Game, bits_2a44_1) == 0x2a44, "bits_2a44_1");
static_assert(offsetof(Game, data) == 0x2a4b, "data");
static_assert(offsetof(Game, data16) == 0x2a47, "data16");
static_assert(offsetof(Game, dplayAddressDialogFlags) == 0x2aaf, "dplayAddressDialogFlags");
static_assert(offsetof(Game, dplayAddressDialogFlags_2aaf) == 0x2aaf, "dplayAddressDialogFlags_2aaf");
static_assert(offsetof(Game, flags_2b4c) == 0x2b4c, "flags_2b4c");
static_assert(offsetof(Game, buffer_2ab1) == 0x2ab1, "buffer_2ab1");
static_assert(offsetof(Game, game_info) == 0x2b6a, "game_info");
static_assert(offsetof(Game, joinGuid) == 0x2ba2, "joinGuid");
static_assert(offsetof(Game, gameName) == 0x2bc1, "gameName");
static_assert(offsetof(Game, gameName_2bc1) == 0x2bc1, "gameName_2bc1");
static_assert(offsetof(Game, password) == 0x2be3, "password");
static_assert(offsetof(Game, bits_2bee_1) == 0x2bee, "bits_2bee_1");
static_assert(offsetof(Game, bits_2bee_3) == 0x2bee, "bits_2bee_3");
static_assert(offsetof(Game, lobbyUiDirtyFlags_2bee) == 0x2bee, "lobbyUiDirtyFlags_2bee");
static_assert(offsetof(Game, bits_2bee) == 0x2bee, "bits_2bee");
static_assert(offsetof(Game, bits_2bee_2) == 0x2bee, "bits_2bee_2");
static_assert(offsetof(Game, password_2be3) == 0x2be3, "password_2be3");
static_assert(offsetof(Game, name_2be3) == 0x2be3, "name_2be3");
static_assert(offsetof(Game, password_2be3_1176a) == 0x2be3, "password_2be3_1176a");
static_assert(offsetof(Game, autoFollowFlags) == 0x14373, "autoFollowFlags");
static_assert(offsetof(Game, autoFollowFlags_14373) == 0x14373, "autoFollowFlags_14373");
static_assert(offsetof(Game, pad_143a7) == 0x143a7, "pad_143a7");
static_assert(offsetof(Game, paletteRgba) == 0x143a7, "paletteRgba");
static_assert(offsetof(Game, src3) == 0x147f7, "src3");
static_assert(offsetof(Game, sources) == 0x147f7, "sources");
static_assert(offsetof(Game, sidePanelTopSeq) == 0x1481f, "sidePanelTopSeq");
static_assert(offsetof(Game, sidePanelRows) == 0x1481f, "sidePanelRows");
static_assert(offsetof(Game, black) == 0x1485f, "black");
static_assert(offsetof(Game, black1Seq) == 0x1485f, "black1Seq");
static_assert(offsetof(Game, gray) == 0x1486f, "gray");
static_assert(offsetof(Game, gray1Seq) == 0x1486f, "gray1Seq");
static_assert(offsetof(Game, table) == 0x1487f, "table");
static_assert(offsetof(Game, cursorModeSeq) == 0x1487f, "cursorModeSeq");
static_assert(offsetof(Game, array_14907) == 0x14907, "array_14907");
static_assert(offsetof(Game, panelTop) == 0x14907, "panelTop");
static_assert(offsetof(Game, vertices) == 0x1e88f, "vertices");
static_assert(offsetof(Game, vertices_1e88f) == 0x1e88f, "vertices_1e88f");
static_assert(offsetof(Game, sounds) == 0x33a13, "sounds");
static_assert(offsetof(Game, soundNames) == 0x33e13, "soundNames");
static_assert(offsetof(Game, soundNames_33e13) == 0x33e13, "soundNames_33e13");
static_assert(offsetof(Game, soundNames_33e13_4000) == 0x33e13, "soundNames_33e13_4000");
static_assert(offsetof(Game, rect) == 0x37e27, "rect");
static_assert(offsetof(Game, viewCullMinX) == 0x37e27, "viewCullMinX");
static_assert(offsetof(Game, viewport) == 0x37e27, "viewport");
static_assert(offsetof(Game, bits_37ebe) == 0x37ebe, "bits_37ebe");
static_assert(offsetof(Game, bits_37ebe_1) == 0x37ebe, "bits_37ebe_1");
static_assert(offsetof(Game, bits_37ebe_2) == 0x37ebe, "bits_37ebe_2");
static_assert(offsetof(Game, bits_37ebe_3) == 0x37ebe, "bits_37ebe_3");
static_assert(offsetof(Game, bits_37ebe_4) == 0x37ebe, "bits_37ebe_4");
static_assert(offsetof(Game, ordersPanelFlags_37ebe_6) == 0x37ebe, "ordersPanelFlags_37ebe_6");
static_assert(offsetof(Game, region_37e27) == 0x37e27, "region_37e27");
static_assert(offsetof(Game, windX) == 0x37ecc, "windX");
static_assert(offsetof(Game, wind) == 0x37ecc, "wind");
static_assert(offsetof(Game, maxUnitsSetting) == 0x37ee6, "maxUnitsSetting");
static_assert(offsetof(Game, side) == 0x37ef2, "side");
static_assert(offsetof(Game, visualFlags) == 0x37f06, "visualFlags");
static_assert(offsetof(Game, bits_37f06) == 0x37f06, "bits_37f06");
static_assert(offsetof(Game, bits_37f06_1) == 0x37f06, "bits_37f06_1");
static_assert(offsetof(Game, bits_37f2f_2) == 0x37f2f, "bits_37f2f_2");
static_assert(offsetof(Game, block) == 0x37ee6, "block");
static_assert(offsetof(Game, startPos) == 0x37f5f, "startPos");
static_assert(offsetof(Game, sideDefs) == 0x37f3d, "sideDefs");
static_assert(offsetof(Game, flags_38a51) == 0x38a51, "flags_38a51");
static_assert(offsetof(Game, bits_38a51_1) == 0x38a51, "bits_38a51_1");
static_assert(offsetof(Game, bits_38a51_3) == 0x38a51, "bits_38a51_3");
static_assert(offsetof(Game, maxStats) == 0x3918f, "maxStats");
static_assert(offsetof(Game, maxKills) == 0x3918f, "maxKills");
static_assert(offsetof(Game, pad_391cf) == 0x391cf, "pad_391cf");
static_assert(offsetof(Game, campaignMissionStatus_391cf) == 0x391cf, "campaignMissionStatus_391cf");
static_assert(offsetof(Game, campaignMissionStatus) == 0x391cf, "campaignMissionStatus");
static_assert(offsetof(Game, pad_39201) == 0x39201, "pad_39201");
static_assert(offsetof(Game, providerGuid) == 0x39201, "providerGuid");
static_assert(offsetof(Game, info) == 0x39201, "info");
static_assert(offsetof(Game, singleCommanderDeath) == 0x39219, "singleCommanderDeath");
static_assert(offsetof(Game, singleSettings) == 0x39219, "singleSettings");
static_assert(offsetof(Game, endGameFlags) == 0x3923b, "endGameFlags");
static_assert(offsetof(Game, flagsByte_3923b) == 0x3923b, "flagsByte_3923b");
static_assert(offsetof(Game, endGameFlags_3923b) == 0x3923b, "endGameFlags_3923b");
static_assert(offsetof(Game, bits_3923b) == 0x3923b, "bits_3923b");
static_assert(offsetof(Game, bits) == 0x3923b, "bits");
static_assert(offsetof(Game, bits_3923b_1) == 0x3923b, "bits_3923b_1");
static_assert(offsetof(Game, bits_3923b_2) == 0x3923b, "bits_3923b_2");
#endif

#endif
