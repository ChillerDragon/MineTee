/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* File modified by Alexandre Díaz */
#ifndef ENGINE_SERVER_H
#define ENGINE_SERVER_H
#include "kernel.h"
#include "message.h"
#include "accountsystem.h" // MineTee
#include <base/vmath.h> // MineTee
#include <game/server/entities/bots/pet.h> // MineTee
#include <game/server/entities/bots/boss.h> // MineTee

enum
{
	BOT_ANIMAL=0,
	BOT_MONSTER,
	BOT_PET,
	BOT_BOSS,

	BOT_ANIMAL_COW=0,
	BOT_ANIMAL_PIG,
	BOT_ANIMAL_SHEEP,
	NUM_ANIMALS,

	BOT_MONSTER_TEEPER=0,
	BOT_MONSTER_ZOMBITEE,
	BOT_MONSTER_SKELETEE,
	//BOT_MONSTER_SPIDERTEE,
	BOT_MONSTER_EYE,
	//BOT_MONSTER_CLOUD,
	NUM_MONSTERS,

	BOT_PET_DEFAULT=0,
	BOT_PET_FIRE,
	BOT_PET_GHOST,
	BOT_PET_GRIFFIN,
	BOT_PET_ICE,
	BOT_PET_NOSEY,
	BOT_PET_ONION,
	BOT_PET_PIG,
	NUM_PETS,

	BOT_BOSS_DUNE=0,
	//BOT_BOSS_GREYFOX,
	//BOT_BOSS_PEDOBEAR,
	//BOT_BOSS_ZOMBIE,
	NUM_BOSSES,
};

class IServer : public IInterface
{
	MACRO_INTERFACE("server", 0)
protected:
	int m_CurrentGameTick;
	int m_TickSpeed;

public:
	/*
		Structure: CClientInfo
	*/
	struct CClientInfo
	{
		const char *m_pName;
		int m_Latency;
	};

    // MineTee
    struct CClientMapInfo
    {
        unsigned m_CurrentMapCrc;
        unsigned char *m_pCurrentMapData;
        unsigned int m_CurrentMapSize;
    };
    CClientMapInfo m_aClientsMapInfo[16];
    bool m_MapGenerated;
    //

	int Tick() const { return m_CurrentGameTick; }
	int TickSpeed() const { return m_TickSpeed; }

	virtual int MaxClients() const = 0;
	virtual const char *ClientName(int ClientID) = 0;
	virtual const char *ClientClan(int ClientID) = 0;
	virtual int ClientCountry(int ClientID) = 0;
	virtual bool ClientIngame(int ClientID) = 0;
	virtual int GetClientInfo(int ClientID, CClientInfo *pInfo) = 0;
	virtual void GetClientAddr(int ClientID, char *pAddrStr, int Size) = 0;

	virtual int SendMsg(CMsgPacker *pMsg, int Flags, int ClientID) = 0;

	template<class T>
	int SendPackMsg(T *pMsg, int Flags, int ClientID)
	{
		CMsgPacker Packer(pMsg->MsgID());
		if(pMsg->Pack(&Packer))
			return -1;
		return SendMsg(&Packer, Flags, ClientID);
	}

	virtual void SetClientName(int ClientID, char const *pName, bool isBot = false) = 0;
	virtual void SetClientClan(int ClientID, char const *pClan) = 0;
	virtual void SetClientCountry(int ClientID, int Country) = 0;
	virtual void SetClientScore(int ClientID, int Score) = 0;

	virtual int SnapNewID() = 0;
	virtual void SnapFreeID(int ID) = 0;
	virtual void *SnapNewItem(int Type, int ID, int Size) = 0;

	virtual void SnapSetStaticsize(int ItemType, int Size) = 0;

	enum
	{
		RCON_CID_SERV=-1,
		RCON_CID_VOTE=-2,
	};
	virtual void SetRconCID(int ClientID) = 0;
	virtual bool IsAuthed(int ClientID) = 0;
	virtual void Kick(int ClientID, const char *pReason) = 0;

	virtual void DemoRecorder_HandleAutoStart() = 0;
	virtual bool DemoRecorder_IsRecording() = 0;

	// MineTee
	virtual int SendMsgEx(CMsgPacker *pMsg, int Flags, int ClientID, bool System) = 0;
	virtual IAccountSystem *AccountSystem() = 0;
	virtual void ResetBotInfo(int ClientID, int BotType, int BotSubType) = 0;
	virtual char *GetMapName() = 0;
	virtual char *GetBlocksData() = 0;
	virtual int GetBlocksDataSize() = 0;

	virtual const unsigned char *ClientKey(int ClientID) = 0;
	virtual void InitClientBot(int ClientID) = 0;
	virtual const char* GetWorldName() = 0;
	//
};

class IGameServer : public IInterface
{
	MACRO_INTERFACE("gameserver", 0)
protected:
public:
	virtual void OnInit() = 0;
	virtual void OnConsoleInit() = 0;
	virtual void OnShutdown() = 0;

	virtual void OnTick() = 0;
	virtual void OnPreSnap() = 0;
	virtual void OnSnap(int ClientID) = 0;
	virtual void OnPostSnap() = 0;

	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker, int ClientID) = 0;

	virtual void OnClientConnected(int ClientID) = 0;
	virtual void OnClientEnter(int ClientID) = 0;
	virtual void OnClientDrop(int ClientID, const char *pReason) = 0;
	virtual void OnClientDirectInput(int ClientID, void *pInput) = 0;
	virtual void OnClientPredictedInput(int ClientID, void *pInput) = 0;

	virtual bool IsClientReady(int ClientID) = 0;
	virtual bool IsClientPlayer(int ClientID) = 0;

	virtual const char *GameType() = 0;
	virtual const char *Version() = 0;
	virtual const char *NetVersion() = 0;

	// MineTee
	virtual bool OnSendMap(int ClientID) = 0;
	virtual void SaveMap(const char *path) = 0;
	virtual void GiveItem(int ClientID, int ItemID, int ammo) = 0;
	virtual void Teleport(int ClientID, int ToID) = 0;
	virtual int IntersectCharacter(vec2 HookPos, vec2 NewPos, vec2 *pNewPos2, int ownID) = 0;
	virtual void AdvanceTime(int amount) = 0;
	virtual void BlockInfo(int BlockID) = 0;

	virtual IAccountSystem::ACCOUNT_INFO* GetAccount(int ClientID) = 0;
	virtual void SaveAccount(int ClientID) = 0;
	virtual CPet* SpawnPet(CPlayer *pOwner, vec2 Pos) = 0;
	virtual IBoss* SpawnBoss(vec2 Pos, int Type) = 0;
	virtual void OnClientMoveCell(int ClientID, int From, int To, unsigned char Qty) = 0;
	virtual void OnClientRelease(int ClientID) = 0;
	//
};

extern IGameServer *CreateGameServer();
#endif
