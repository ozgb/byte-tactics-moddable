#ifndef NET_GAME_H
#define NET_GAME_H

struct Player;
struct Unit;

int __stdcall DefaultPacketHandler(int arg1);
int __stdcall RejectPacketType2(int arg1);
int __stdcall RejectPacketType5(int);
int __stdcall RejectPacketType6(int arg1);
int __stdcall RejectPacketType7(int arg1);
int __stdcall RejectPacketType8(int arg1);
unsigned char __stdcall FindSlotByDpid(int id);
unsigned char __stdcall GetPlayerSlot(Player* player);
Player* __stdcall FindPlayerByDpid(int id);
int __stdcall GetSlotDpid(unsigned char index);
int __stdcall GetPlayerDpid(Player* obj);
int __stdcall SetPlayerName(unsigned char index, char* shortName, char* longName);
int __stdcall GetPlayerName(int dpid, char* shortName, char* longName);
unsigned char __stdcall PickNewHost();
void __stdcall AnnouncePlayerLeft(int id);
int __stdcall FindFreeSlot();
char __stdcall FindFreePlayerId(void);
void __stdcall CheckDuplicatePlayerIds(void);
void __stdcall CloseNetSession();
void __stdcall LeaveNetGame();
void __stdcall BroadcastPlayerInfo();
void __stdcall BuildGameInfo(char* name, int* d, int* c, int* b, int* a);
void __stdcall UpdateNetGameInfo(void);
int __stdcall CreateLocalPlayer(unsigned char playerIndex, int flag);
void __stdcall CreateNetGame(void);
void __stdcall EnumPlayersCallback(int id, int unused1, int unused2, int unused3, int unused4);
int __stdcall JoinLobbyGame(Player* p);
void __stdcall RemoveLocalPlayers();
int __stdcall IsColorFree(int id, int slot);
int __stdcall BroadcastPlayerLeft(int id);
int __stdcall SendAlliance(int from, int to, char value, int extra);
void __stdcall RemovePlayer(int id);
int __stdcall ReceiveNetPacket(void);
void __stdcall OpenTimeoutDialog(int id);
void __stdcall CheckPlayerTimeouts();
void __stdcall SendNewUnit(Unit* obj);
void __stdcall BroadcastBuilderLink(Unit* obj, Unit* target);
int __stdcall SendScriptCallNoArgsByName(Unit* obj, char* name);
int __stdcall SendScriptCallNoArgs(Unit* obj, short index);
void __stdcall SendNetHeartbeat();
int __stdcall AreAllPlayersReady();
void __stdcall ReportPacketGap(int, int, int);
void __stdcall SendShareMetal(unsigned char from, unsigned char to, int amount);
void __stdcall SendShareEnergy(unsigned char from, unsigned char to, int value);
void __stdcall SendShareMapInfo(unsigned char from, unsigned char to);
int __stdcall BroadcastPendingViewState();
int __stdcall InitLobbiedConnection();
void __stdcall SetPacketRate(int rate);
int __stdcall IsHostLocal();
int __stdcall CountHumanPlayers();
int __stdcall CountComputerPlayers();
int __stdcall CountLocalComputerPlayers();
int __stdcall CountActiveAIPlayers();
int __stdcall CountActiveHumanOrAiPlayers();
int __stdcall CountRemotePlayers();
int __stdcall CountCombatPlayers();
void __stdcall UpdateResourceSharing(Player* player);

#endif
