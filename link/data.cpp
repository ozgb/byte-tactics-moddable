#include "globals.h"

unsigned char g_turnCosts[8] = {0, 40, 60, 80, 100, 80, 60, 40};
unsigned char g_movementClassColors[8] = {4, 14, 10};
char g_directXWarningText[200] = "Warning! The currently installed version of Microsoft%sDirectX may not function properly with Total Annihilation.%sPlease install the version of DirectX included on the%sTotal Annihilation setup CD.";
double TWO_PI = 6.28318530717958;
double ONE_EIGHTH = 0.125;
const float DAT_004fd4c0 = 0.0f;
const float DAT_004fd4cc = 5.0f;
signed char g_dirDeltaX[8] = {0, -1, -1, -1, 0, 1, 1, 1};
signed char g_dirDeltaZ[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
float g_buildPercentScale = 0.003921568859368563f;
unsigned char DAT_004fdaf0[16] = {0};
double g_shadeStep = -0.06875;
char g_hapiCopyright[40] = "Copyright 0000 Cavedog Entertainment";
unsigned char g_contourColors[32] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 111, 110, 109, 108, 107, 106,
    88, 87, 86, 85, 84, 83, 82, 81, 80, 255, 255, 255, 255, 255, 255, 255
};
char g_mappingKey[8] = "Mapping";
char g_screenshotDirFormat[16] = "%s\\screenshots";
const char g_guisDirName[8] = "guis";
char g_cameraXPosition[12] = "X Position";
char g_cameraZPosition[12] = "Z Position";
char g_cameraAccount[8] = "Camera";
int g_commandLineUnusedFlagL = 1;
int DAT_0050289c = 1;
char g_campaignKey[12] = "Campaign";
char g_dotExtSep[4] = ".";
char DAT_00502a20[4] = "&G";
char g_difficultyKey[12] = "Difficulty";
char g_okGadgetName[4] = "OK";
char g_optionsSoundName[8] = "Options";
unsigned short g_faceVertexIndices[6][4] = {
    {0, 1, 2, 3}, {2, 1, 6, 5}, {0, 3, 4, 7}, {1, 0, 7, 6},
    {3, 2, 5, 4}, {4, 5, 6, 7}
};
const char g_animsDirName[8] = "anims";
char g_descriptionKey[12] = "Description";
char g_frontendStateChangeFormat[104] = "Code segment checksum error found when switching FE states.\nState change called from [line %d, file %s]";
char g_frontendSourceFile[32] = "c:\\cavedog\\wargame\\frontend.cpp";
char g_extTdf[4] = "tdf";
char g_choice2GadgetName[8] = "CHOICE2";
char g_choice1GadgetName[8] = "CHOICE1";
char g_smallButtonSoundName[12] = "SmallButton";
char g_titleGadgetName[8] = "TITLE";
char g_noText[4] = "No";
char g_yesText[4] = "Yes";
char g_yesNoGuiName[12] = "YESORNO.GUI";
char g_bgmSoundName[4] = "BGM";
char g_serviceErrorMessage[48] = "An error occurred trying to use this service";
char g_zrbMovie4[8] = "4.zrb";
char g_zrbMovie3[8] = "3.zrb";
char g_zrbMovie5[8] = "5.zrb";
char g_zrbMovie2[8] = "2.zrb";
char g_zrbMovie1[8] = "1.zrb";
char* g_saveBankName = (char*)"Total Annihilation 3.0";
char* g_summaryAccountName = (char*)"Summary";
char g_pathSepBackslash[4] = "\\";
const char g_fontsDirName[8] = "fonts";
char g_tdfExtension[4] = "TDF";
char g_star[4] = "*";
char g_noneString[8] = "none";
char g_sayChoiceArrayName[20] = "Say Choice Array";
char g_maxUnitsKey[12] = "maxunits";
char g_radarImageBoxName[12] = "Radar Image";
char g_gameTimeKey[12] = "Game Time";
char g_gameIdKey[8] = "Game ID";
char g_betweenMissionsKey[16] = "BetweenMissions";
char g_lineOfSightTypeKey[16] = "LineOfSightType";
char g_lineOfSightKey[12] = "LineOfSight";
char g_locationKey[12] = "Location";
char g_commanderDeathKey[16] = "CommanderDeath";
char g_thumbsKey[8] = "Thumbs";
char g_gameTypeKey[12] = "Gametype";
char g_playersKey[8] = "Players";
char g_sideKey[8] = "Side";
char g_mapKey[4] = "Map";
char g_missionKey[8] = "Mission";
char g_buildTimeFormat[16] = "BUILD TIME: %s";
char g_buildDateFormat[16] = "BUILD DATE: %s";
char g_campsDirName[8] = "camps";
const char g_defaultLanguage[8] = "english";
char g_hardGadgetName[8] = "Hard";
char g_mediumGadgetName[8] = "Medium";
char g_easyGadgetName[8] = "Easy";
int g_serialBaudRateTable[6] = {115200, 57600, 38400, 19200, 14400, 9600};
unsigned int g_reporterCount = 16;
char* g_battleRoomGadgetNames[23] = {
    (char*)"PLAYERx", (char*)"READYx", (char*)"LOGOx", (char*)"SIDEx", (char*)"PINGx", (char*)"MEMx", (char*)"RESx", (char*)"ALLYx",
    (char*)"CDx", (char*)"TEAMICONSx", 0, 0, (char*)"COMMNDER", (char*)"WATCHING", (char*)"FIXEDLOC", (char*)"MAPPING",
    (char*)"TECHLEVL", (char*)"SHARING", (char*)"CHEATING", (char*)"UNITS", (char*)"METAL", (char*)"ENERGY"
};
unsigned int g_lastPlayerCount = 4294967295u;
int g_rejectPlayer = -1;
char* g_hostOnlyGadgets[8] = {(char*)"COMMANDER", (char*)"MAPPING", (char*)"LOSTYPE", (char*)"WATCHING", (char*)"CHEATING", (char*)"FIXEDLOC", (char*)"GAMEOPEN"};
char g_multiSoundName[8] = "Multi";
char g_allySoundName[8] = "Ally";
char g_gamesGadgetName[8] = "GAMES";
char g_savegameDescsName[16] = "SAVEGAME DESCS";
char g_savegameNamesName[16] = "SAVEGAME NAMES";
char g_lstExtension[4] = "LST";
char* g_leftGameTexts[8] = {(char*)"has left the scene", (char*)"has been shown the door", (char*)"has gone to a better place", (char*)"has bowed out", (char*)"has terminated", (char*)"has been eradicated", (char*)"has been liquidated", (char*)"has been obliterated"};
int g_timeoutPlayerDpid = -1;
char g_playerMessageFormat[8] = "%s %s";
char s_PACKET_DATA_00506524[12] = "PACKET DATA";
char g_talkGadgetName[8] = "TALK";
char g_integrityBreachText[56] = "has modified his executable.  Game integrity breached.";
char g_playerDisconnectedText[28] = "Player %s has disconnected";
float g_lightX = -0.800000011920929f;
float g_lightY = 1.0f;
float g_lightZ = 0.25f;
int g_optionsShellClosing = 1;
char g_notrakGadgetName[8] = "NOTRAK";
char g_fxVolGadgetName[8] = "FXVOL";
char g_trackTypeGadgetName[12] = "TRACKTYPE";
char g_cdNextGadgetName[8] = "CDNEXT";
char g_cdPlayGadgetName[8] = "CDPLAY";
char g_cdStopGadgetName[8] = "CDSTOP";
char g_cdPrevGadgetName[8] = "CDPREV";
char g_trackModeGadgetName[12] = "TRACKMODE";
char g_restoreGadgetName[8] = "RESTORE";
char g_undoGadgetName[8] = "UNDO";
char g_speechGadgetName[8] = "SPEECH";
char g_testGadgetName[8] = "TEST";
char g_volTextGadgetName[8] = "VOLTEXT";
char g_modeGadgetName[8] = "MODE";
char g_explodeSoundFile[20] = "sounds\\explode.wav";
int g_usePacketManager = 1;
char g_continueWatchingTitle[32] = "You're out!  Continue Watching?";
char g_radarPicTempName[16] = "RADARPIC TEMP";
char g_radarPictureName[16] = "RADAR PICTURE";
char g_radarMappedName[16] = "RADAR MAPPED";
char g_radarFinalName[12] = "RADAR FINAL";
int g_campaignSimplifiedLayout = 1;
unsigned char g_briefingTextColors[24] = {53, 51, 64, 208, 117, 86, 82, 212, 117, 86, 82, 212, 117, 86, 82, 212, 117, 86, 82, 212, 0, 0, 0, 0};
char* g_forcesDestroyedTexts[3] = {(char*)"forces have been obliterated", (char*)"vermin have been exterminated", (char*)"forces have gone to a better place"};
char g_alliesChatTarget[8] = "Allies";
const char g_skirmishCheatSoundName[16] = "SkirmishCheat";
const char g_cheatTenPlayers[4] = "*X";
const char g_cheatNinePlayers[4] = "*IX";
const char g_cheatThreePlayers[8] = "*III";
const char g_cheatEightPlayers[8] = "*VIII";
const char g_cheatSevenPlayers[8] = "*VII";
const char g_cheatSixPlayers[4] = "*VI";
const char g_cheatFivePlayers[4] = "*V";
const char g_cheatFourPlayers[4] = "*IV";
char g_skirmishSetupPictureName[16] = "Skirmsetup4x";
char g_skirmishGuiName[16] = "SKIRMISH.GUI";
char g_soundInitError[44] = "Error:  Sound system initialization failed.";
char g_useWindowsSoundKey[16] = "UseWindowsSound";
char g_noDirectSoundKey[16] = "NoDirectSound";
char g_killedScriptName[8] = "Killed";
char g_takenLeadFormat[36] = "%s has taken the lead with %d kills";
char s_TakeDamage_00508d68[12] = "TakeDamage";
char s_HitByWeapon_00508d74[12] = "HitByWeapon";
char s_paralyze_00508d80[12] = "paralyze";
char s_SelectMultipleUnits_00508d8c[20] = "SelectMultipleUnits";
char g_celebratedKey[12] = "Celebrated";
char g_satisfiedKey[12] = "Satisfied";
char g_killEnemyCommanderKey[36] = "VictoryCondition_KillEnemyCommander";
char g_destroyAllUnitsKey[36] = "VictoryCondition_DestroyAllUnits";
char g_numUnitsKey[12] = "NumUnits";
char g_killAllMobileUnitsKey[36] = "VictoryCondition_KillAllMobileUnits";
char g_buildUnitTypeKey[32] = "VictoryCondition_BuildUnitType";
char g_numLeftToKillKey[16] = "NumLeftToKill";
char g_killUnitTypeKey[32] = "VictoryCondition_KillUnitType";
char g_allUnitsKilledKey[32] = "DefeatCondition_AllUnitsKilled";
char* g_savegameDir = (char*)"SAVEGAME";
int g_nonCampaignGame = 1;
int g_lastCdActivityMode = -1;
const char g_offscreenSurfaceName[12] = "OFFSCREEN";
const char g_cdListsKey[8] = "CDLISTS";
const char g_unitLimitKey[12] = "UnitLimit";
const char g_hattFont11Name[12] = "hattfont11";
const char g_hattFont12Name[12] = "hattfont12";
const char g_commonGuiName[12] = "commongui";
const char g_skirmishInfoTag[16] = "SkirmishInfo";
const char g_invalidSavegameText[24] = "Invalid savegame file";
char g_enemiesChatTarget[8] = "Enemies";
char g_chatTargetSeparators[4] = ",:;";
char g_sendToGadgetName[8] = "SENDTO";
char g_sendTypeGadgetName[12] = "SENDTYPE";
char g_livePlayerPrefix[12] = "LIVEPLYR";
char g_screenshotPrefix[12] = "SHOT";
char* g_fireScriptNames[4] = {(char*)"FirePrimary", (char*)"FireSecondary", (char*)"FireTertiary"};
char* g_aimScriptNames[3] = {(char*)"AimPrimary", (char*)"AimSecondary", (char*)"AimTertiary"};
char* g_windowClassName = (char*)"Total Annihilation Class";
char* g_appName = (char*)"Total Annihilation";
int g_cdNeedsReopenAfterFocus = 1;
const char g_cdShellValueName[8] = "cdshell";
const char g_audioCdShellKey[32] = "SOFTWARE\\Classes\\AudioCD\\shell";
const char g_translationFile[24] = "gamedata\\translate.tdf";
const char g_languageValueName[12] = "language";
const char DAT_005097f4[8] = "Wargame";
const char g_errorCaption[20] = "Error";
double g_radToAngle16 = 10430.37835047;
double g_angle16ToRad = 9.587379924285e-05;
short g_sinTable[128] = {
    0, 101, 201, 302, 402, 502, 603, 703,
    803, 903, 1003, 1102, 1202, 1301, 1401, 1499,
    1598, 1697, 1795, 1893, 1990, 2088, 2185, 2282,
    2378, 2474, 2570, 2665, 2760, 2854, 2948, 3042,
    3135, 3228, 3320, 3411, 3503, 3593, 3683, 3773,
    3862, 3950, 4038, 4125, 4212, 4297, 4383, 4467,
    4551, 4634, 4717, 4799, 4880, 4960, 5040, 5119,
    5197, 5274, 5351, 5427, 5501, 5575, 5649, 5721,
    5793, 5863, 5933, 6002, 6070, 6137, 6203, 6268,
    6333, 6396, 6458, 6519, 6580, 6639, 6698, 6755,
    6811, 6867, 6921, 6974, 7027, 7078, 7128, 7177,
    7225, 7272, 7317, 7362, 7405, 7448, 7489, 7529,
    7568, 7606, 7643, 7679, 7713, 7746, 7779, 7809,
    7839, 7868, 7895, 7921, 7946, 7970, 7993, 8014,
    8035, 8054, 8071, 8088, 8103, 8117, 8130, 8142,
    8153, 8162, 8170, 8177, 8182, 8186, 8190, 8191
};
short g_cosTable[512] = {
    8192, 8191, 8190, 8186, 8182, 8177, 8170, 8162,
    8153, 8142, 8130, 8117, 8103, 8088, 8071, 8054,
    8035, 8014, 7993, 7970, 7946, 7921, 7895, 7868,
    7839, 7809, 7779, 7746, 7713, 7679, 7643, 7606,
    7568, 7529, 7489, 7448, 7405, 7362, 7317, 7272,
    7225, 7177, 7128, 7078, 7027, 6974, 6921, 6867,
    6811, 6755, 6698, 6639, 6580, 6519, 6458, 6396,
    6333, 6268, 6203, 6137, 6070, 6002, 5933, 5863,
    5793, 5721, 5649, 5575, 5501, 5427, 5351, 5274,
    5197, 5119, 5040, 4960, 4880, 4799, 4717, 4634,
    4551, 4467, 4383, 4297, 4212, 4125, 4038, 3950,
    3862, 3773, 3683, 3593, 3503, 3411, 3320, 3228,
    3135, 3042, 2948, 2854, 2760, 2665, 2570, 2474,
    2378, 2282, 2185, 2088, 1990, 1893, 1795, 1697,
    1598, 1499, 1401, 1301, 1202, 1102, 1003, 903,
    803, 703, 603, 502, 402, 302, 201, 101,
    0, -101, -201, -302, -402, -502, -603, -703,
    -803, -903, -1003, -1102, -1202, -1301, -1401, -1499,
    -1598, -1697, -1795, -1893, -1990, -2088, -2185, -2282,
    -2378, -2474, -2570, -2665, -2760, -2854, -2948, -3042,
    -3135, -3228, -3320, -3411, -3503, -3593, -3683, -3773,
    -3862, -3950, -4038, -4125, -4212, -4297, -4383, -4467,
    -4551, -4634, -4717, -4799, -4880, -4960, -5040, -5119,
    -5197, -5274, -5351, -5427, -5501, -5575, -5649, -5721,
    -5793, -5863, -5933, -6002, -6070, -6137, -6203, -6268,
    -6333, -6396, -6458, -6519, -6580, -6639, -6698, -6755,
    -6811, -6867, -6921, -6974, -7027, -7078, -7128, -7177,
    -7225, -7272, -7317, -7362, -7405, -7448, -7489, -7529,
    -7568, -7606, -7643, -7679, -7713, -7746, -7779, -7809,
    -7839, -7868, -7895, -7921, -7946, -7970, -7993, -8014,
    -8035, -8054, -8071, -8088, -8103, -8117, -8130, -8142,
    -8153, -8162, -8170, -8177, -8182, -8186, -8190, -8191,
    -8192, -8191, -8190, -8186, -8182, -8177, -8170, -8162,
    -8153, -8142, -8130, -8117, -8103, -8088, -8071, -8054,
    -8035, -8014, -7993, -7970, -7946, -7921, -7895, -7868,
    -7839, -7809, -7779, -7746, -7713, -7679, -7643, -7606,
    -7568, -7529, -7489, -7448, -7405, -7362, -7317, -7272,
    -7225, -7177, -7128, -7078, -7027, -6974, -6921, -6867,
    -6811, -6755, -6698, -6639, -6580, -6519, -6458, -6396,
    -6333, -6268, -6203, -6137, -6070, -6002, -5933, -5863,
    -5793, -5721, -5649, -5575, -5501, -5427, -5351, -5274,
    -5197, -5119, -5040, -4960, -4880, -4799, -4717, -4634,
    -4551, -4467, -4383, -4297, -4212, -4125, -4038, -3950,
    -3862, -3773, -3683, -3593, -3503, -3411, -3320, -3228,
    -3135, -3042, -2948, -2854, -2760, -2665, -2570, -2474,
    -2378, -2282, -2185, -2088, -1990, -1893, -1795, -1697,
    -1598, -1499, -1401, -1301, -1202, -1102, -1003, -903,
    -803, -703, -603, -502, -402, -302, -201, -101,
    0, 101, 201, 302, 402, 502, 603, 703,
    803, 903, 1003, 1102, 1202, 1301, 1401, 1499,
    1598, 1697, 1795, 1893, 1990, 2088, 2185, 2282,
    2378, 2474, 2570, 2665, 2760, 2854, 2948, 3042,
    3135, 3228, 3320, 3411, 3503, 3593, 3683, 3773,
    3862, 3950, 4038, 4125, 4212, 4297, 4383, 4467,
    4551, 4634, 4717, 4799, 4880, 4960, 5040, 5119,
    5197, 5274, 5351, 5427, 5501, 5575, 5649, 5721,
    5793, 5863, 5933, 6002, 6070, 6137, 6203, 6268,
    6333, 6396, 6458, 6519, 6580, 6639, 6698, 6755,
    6811, 6867, 6921, 6974, 7027, 7078, 7128, 7177,
    7225, 7272, 7317, 7362, 7405, 7448, 7489, 7529,
    7568, 7606, 7643, 7679, 7713, 7746, 7779, 7809,
    7839, 7868, 7895, 7921, 7946, 7970, 7993, 8014,
    8035, 8054, 8071, 8088, 8103, 8117, 8130, 8142,
    8153, 8162, 8170, 8177, 8182, 8186, 8190, 8191
};
int g_rotationMatrix[9] = {0};
char g_alphaTableName[12] = "ALPHA TABLE";
const char g_shadeTableName[12] = "SHADE TABLE";
char g_dotDot[4] = "..";
char g_packageDataName[16] = "Package Data";
char g_dirWildcard[4] = "\\*";
char g_hapiFileArrayName[16] = "HAPIFILE array";
EmptyStringRep g_emptyStringRep = {1, 0};
unsigned char DAT_0050a780[4] = {1, 0, 0, 0};
int g_cdNextTrackTimer = -1;
int g_cdFadeTimer = -1;
void* g_squashErrorNames[7] = {(void*)"SQUASHERR_OK", (void*)"SQUASHERR_BADHEADER", (void*)"SQUASHERR_BADCHECKSUM", (void*)"SQUASHERR_BADUNPACKSIZE", (void*)"SQUASHERR_BADUNPACKTYPE", (void*)"SQUASHERR_BADPACKTYPESQUASHERR_BADPARAMS"};
char g_cavedogTitle[8] = "Cavedog";
char* g_debugKeywords[19] = {
    (char*)"-memfussy", (char*)"-memnofussy", (char*)"-memfrontalign", (char*)"-gonzo", (char*)"-memset", (char*)"-memnoset", (char*)"-fpufussy", (char*)"-fpunofussy",
    (char*)"-dprinton", (char*)"-dprintoff", (char*)"-dprintfile", (char*)g_memoryStatusSwitch, (char*)"-performancestatus", (char*)"-disableimagehlp", (char*)"-enableimagehlp", (char*)"-disableimagehlplines",
    (char*)"-enableimagehlplines", (char*)"-debughelper", (char*)"-saveresources"
};
char g_memoryStatusSwitch[16] = "-memorystatus";
char* g_performanceWindowName = (char*)"Performance status";
char g_performanceDialogError[36] = "Performance dialog failed to open";
char* g_memoryStatusWindowName = (char*)"Memory Status";
int g_byteRatesLastReceived;
int g_compressionPercent;
unsigned int g_bytesSentPerSecond;
int g_lastPacketBytesSent;
unsigned int g_bytesReceivedPerSecond;
unsigned char DAT_00511a54[4];
int g_messageCountByType[45][2];
int g_messageBytesReceived;
int g_messageBytesSent;
int g_packetBytesSent;
int g_lastPacketBytesReceived;
char g_commandLineBuf[80];
int DAT_00511c20;
int g_lastPacketsSent;
int g_lastPacketsReceived;
int g_byteRatesLastSent;
unsigned int g_packetBytesReceivedRate;
int g_packetsSent;
unsigned int g_messageBytesReceivedRate;
unsigned int g_messageBytesSentRate;
int g_lastMessageBytesReceived;
int g_lastMessageBytesSent;
int g_packetsReceived;
unsigned int g_packetBytesSentRate;
int g_packetBytesReceived;
unsigned char DAT_00511c54[4];
int g_messageBytesByType[45][2];
unsigned int g_packetsReceivedRate;
unsigned int g_packetsSentRate;
int g_compressedBytesSent;
int g_lastCompressedBytesSent;
int g_contourSpacing;
int g_contourOffset;
unsigned int g_netStatsTick;
unsigned int g_byteRatesTick;
int g_cdBypassDriveScan;
Game* g_game;
int g_endGameGlamourSoundStarted;
int g_explosion0Duration;
int g_explosion0StartSize;
int g_explosion0EndSize;
int g_explosion1Duration;
int g_explosion1StartSize;
int g_explosion1EndSize;
int g_explosion2Duration;
int g_explosion2StartSize;
int g_explosion2EndSize;
char g_frontendErrorText[256];
unsigned char g_pictureCache[464];
int DAT_00512288;
int g_directXCheckDone;
int g_gpfCheckDone;
int g_noSoundDriverShown;
MenuSpark* g_menuSparks;
int g_cdPlayerDialogShown;
unsigned char DAT_005122a0[4];
char* g_otaEnumFileList;
int g_otaEnumCacheComplete;
int g_otaEnumFileListBytes;
int g_otaEnumFileCount;
unsigned char DAT_005122e4[4];
int DAT_00512760;
int g_unitRestrictPicLoadIndex;
int DAT_0051276c;
unsigned int g_reporterNames[16];
unsigned char g_linkInfo[432];
int g_unitRestrictPicCursor;
int g_unitRestrictRecordCursor;
char* g_modemInfo;
int g_modemCount;
ModemAccount* g_modemAccounts;
char* g_modemAccountNames;
char* g_oldMapName;
int g_battleRoomSlotsBuilt;
unsigned char DAT_00512998[12];
int g_startCountdownNextTick;
unsigned int g_heartbeatNextTick;
char* g_saveListFileNames;
char* g_saveListDisplayNames;
Record_005129b4* g_unitRestrictEntries;
int* g_unitRestrictPics;
unsigned char DAT_005129bc[4];
int DAT_005129c0;
int* g_unitRestrictOldCounts;
int g_unitRestrictNextPicTick;
unsigned char DAT_005129cc[4];
int g_broadcastPeersSeen[12];
char* g_loungeChatter;
int g_loungeRefreshTime;
unsigned int g_timeoutTimerStart;
unsigned char DAT_00512c80[336];
int g_onlineConfigLoaded;
int g_helpDialogBaseGadgetCount;
unsigned char DAT_00512ef4[4];
unsigned char g_optionsFlipFrame[24];
int g_optionsLightbarY;
int g_optionsLightbarMaxX;
unsigned char g_optionsPrefsSnapshot[85];
unsigned char g_optionsBackupGameSpeed[4];
unsigned char g_optionsBackupEdgeScroll[4];
char g_optionsBackupTrackTypes[100];
int g_optionsBackupLockedTrack;
int g_musicUiSelectedTrack;
int g_optionsShellActive;
Surface* g_optionsFlipSurface;
int g_optionsLightbarX;
int g_optionsLightbarAnim;
Surface* g_optionsBackupSurface;
int g_battleQuitIntent;
unsigned char DAT_00512ffc[4];
int g_probePanelBottom;
int g_statusPanelNextTick;
int (__stdcall* g_riInitializeEx)(int*, int);
int (__stdcall* g_riGetVersion)(char*);
void (__cdecl* g_riTerminate)(void);
unsigned char DAT_0051e55c[4];
unsigned char g_reportPlayerName[20];
void (__stdcall* g_riSetCallbacks)(void (__stdcall*)(int), int (__stdcall*)(char*, int));
int (__stdcall* g_riEnable)(int);
int g_hapinetOnlineReport;
int g_reportFlags;
char g_unitSyncStatusText[104];
char g_fxEventPoolBlocked;
unsigned char DAT_0051e609[7];
unsigned char g_particlePool[36];
unsigned char g_particlePoolDestroyed;
unsigned char DAT_0051e635[7];
char* g_briefingWrappedText;
unsigned char g_briefingPlanetFrameCursor[12];
int g_briefingPageIndex;
int g_briefingPaginateReset;
int g_briefingWindSpeed;
unsigned char DAT_0051e658[4];
char* g_campaignNameList;
char* g_missionNameList;
unsigned char DAT_0051e664[4];
int g_anyMissionMode;
int g_briefingBaseGadgetCount;
int g_briefingWindTickCountdown;
short g_briefingPanoramaScrollX;
int g_briefingPanoramaNextTick;
unsigned int g_briefingPlanetLastTick;
unsigned int g_briefingPlanetNextTick;
SpeechQueue* g_speechQueue;
int g_noDirectSound;
int g_useWindowsSound;
int g_cmdSingEnabled;
int g_playLooping;
unsigned char g_loadingBarFlashAlpha[8];
int g_shareDialogPlayerNetIds[10];
char g_savegameRadarFrame[24];
int g_cdActivitySampleRing[30];
char g_chatDraftText[136];
unsigned char g_scorePanelLossFlash[10];
unsigned char DAT_0051e81a[6];
unsigned char g_loadingBarPrevPercent[8];
unsigned char g_cdListsDiscEntries[2720];
unsigned char g_scorePanelKillFlash[10];
unsigned char DAT_0051f2d2[6];
unsigned int g_scorePanelSlidePos;
unsigned int g_cdActivityRingWriteIdx;
int g_chatDraftInitialized;
int g_scorePanelFlashDecayTick;
unsigned int g_cdActivityLastSampleTick;
int g_cdActivityStableTicks;
int g_netProbeNextTick;
int g_netHeartbeatNextTick;
unsigned char g_loadingBarFlashDecayTick[8];
unsigned char g_atexitRegistered;
unsigned char g_displayContext[2088];
int g_cmdlineDisplaySeed;
unsigned char DAT_0051fb4c[4];
char g_preferredLanguage[64];
int g_cdTrackCategory;
unsigned long g_lastSoundReapTick;
unsigned char DAT_0051fb98[4];
char* g_loopingWav;
int* g_diskWav;
int DAT_0051fba8;
int DAT_0051fbac;
int DAT_0051fbb0;
int DAT_0051fbb4;
unsigned char DAT_0051fbd4[4];
unsigned char g_timerSlots[172];
unsigned int g_lastTimerTick;
int g_defaultCommandHandler;
int g_defaultCommandMask;
unsigned char g_gafEncodeLiteralBuffer[256];
int g_gafEncodeOutputSize;
unsigned char DAT_0051fdb4[4];
TranslationTable* g_translations;
unsigned char DAT_0051fdbc[4];
char g_language[64];
int g_edgeSpan;
int g_edgeHighIndexB;
Chunk* g_edgeTable;
int g_edgeHighIndex;
int g_edgeLowIndex;
int g_reporterNotify;
int g_reporterReport;
unsigned char DAT_0051ff0c[4];
int g_cdFadeVolume;
Sound* g_cdPlayer;
void* g_cdPlayerWindow;
unsigned char DAT_0051ff1c[4];
int g_cdCategorySavedTrack[10];
int g_playBufferLooping;
unsigned char DAT_0051ff4c[12];
int g_delayedSampleVolume;
unsigned char DAT_0051ff5c[4];
char g_delayedSampleName[112];
char g_lzssPresetWindow[4113];
unsigned char DAT_00520fe1[7];
char g_lzssPresetTree[24584];
LzssTreeNode* g_squashDictTree;
int g_lzssPresetReady;
int g_lzssUsePreset;
char g_memFussyQueried;
char g_memFussyDefault;
void (__stdcall* g_outOfMemoryHandler)(void);
unsigned int g_committedBytesPeak;
unsigned int g_lastAllocOffset;
unsigned int g_currentBytesPeak;
unsigned int g_bytesRequestedLo;
void* g_blockMapFreeList;
unsigned char DAT_005289e4[8];
unsigned int g_committedBytes;
unsigned char DAT_005289f4[4];
unsigned int g_currentBytes;
unsigned int g_bytesRequestedHi;
unsigned int g_freeBlockWraps;
unsigned int g_allocSerial;
unsigned int g_liveAllocCount;
unsigned char DAT_00528a0c[4];
void* g_freeBlockFreeList;
unsigned char DAT_00528a14[8];
unsigned int g_liveAllocPeak;
unsigned char DAT_00528a20[8];
FreeBlockMap* g_freeBlockSet;
void* g_blockMap;
unsigned char g_freedBlockRing[4];
int g_blockMapLiveCount;
void* g_blockMapNil;
unsigned char DAT_00528a5c[4];
void* g_lineProcessHandle;
char g_lineProcessHandleRead;
char g_msdevDirRead;
char g_crtSourceRoot[1000];
char g_mfcSourceRoot[1000];
unsigned char DAT_005292b8[4];
unsigned char DAT_005292c8[8];
unsigned char DAT_00529504[4];
int g_pfnQueryWorkingSet;
unsigned char DAT_0052950c[4];
int g_workingSetPrivate;
unsigned char DAT_0052952c[4];
void* g_memStatusProcess;
unsigned char DAT_00529534[4];
int g_workingSetShared;
void* g_psapiModule;
int g_workingSetPageTables;
unsigned char DAT_005295c4[4];
int g_workingSetPages;
char g_psapiInited;
int g_workingSetRefreshCountdown;
int g_workingSetPagesPeak;
char g_memStatusLastText[2000];
unsigned char g_perfAutoPairing;
int g_pmcEventCount;
int g_reportIndent;
unsigned char g_perfRaisePriority;
unsigned char g_perfEnabled;
unsigned char g_perfDisplayInDebugger;
Entry_004df590* g_pmcEventCatalog;
unsigned char DAT_00529dfc[4];
char g_reportIndentText[56];
void* g_nameMapFreeList;
unsigned char DAT_00529e5c[8];
unsigned char g_perfDisplayInWindow;
unsigned char g_gdperfInitAttempted;
unsigned char g_gdperfAvailable;
NameTable* g_nameTable;
char* g_defaultSection;
unsigned char DAT_00529e84[4];
void* g_gdperfDevice;
int g_cpuFamily;
unsigned char DAT_00529ea4[4];
char g_emptyAtexitRegistered;
long g_gfxBlitLockHeld;
long g_gfxBlitLockOwner;
void* g_gfxBlitLockEvent;
int g_lzssLockOwner;
void* g_squashThreadLockEvent;
long g_squashThreadLockTicket;
