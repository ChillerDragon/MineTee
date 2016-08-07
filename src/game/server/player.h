/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* File modified by Alexandre Díaz */
#ifndef GAME_SERVER_PLAYER_H
#define GAME_SERVER_PLAYER_H

// this include should perhaps be removed
#include "entities/character.h"
#include "entities/bots/pet.h"
#include "gamecontext.h"

// player object
class CPlayer
{
	MACRO_ALLOC_POOL_ID()

public:
	enum
	{
		NUM_INVENTORY_CELLS = NUM_CELLS_LINE*3,
		NUM_CRAFT_CELLS = NUM_CELLS_LINE+1, // +1 because the last item its the result of the craft
		NUM_RECIPE_CELLS = NUM_CELLS_LINE, // The recipe of the current craft
	};

	CPlayer(CGameContext *pGameServer, int ClientID, int Team);
	~CPlayer();

	void Init(int CID);

	void TryRespawn();
	void Respawn();
	void SetTeam(int Team, bool DoChatMsg=true);
	int GetTeam() const { return m_Team; };
	int GetCID() const { return m_ClientID; };

	void Tick();
	void PostTick();
	void Snap(int SnappingClient);

	void OnDirectInput(CNetObj_PlayerInput *NewInput);
	void OnPredictedInput(CNetObj_PlayerInput *NewInput);
	void OnDisconnect(const char *pReason);

	void KillCharacter(int Weapon = WEAPON_GAME);
	CCharacter *GetCharacter();

	//---------------------------------------------------------
	// this is used for snapping so we know how we can clip the view for the player
	vec2 m_ViewPos;

	// states if the client is chatting, accessing a menu etc.
	int m_PlayerFlags;

	// used for snapping to just update latency if the scoreboard is active
	int m_aActLatency[MAX_CLIENTS];

	// used for spectator mode
	int m_SpectatorID;

	bool m_IsReady;

	//
	int m_Vote;
	int m_VotePos;
	//
	int m_LastVoteCall;
	int m_LastVoteTry;
	int m_LastChat;
	int m_LastSetTeam;
	int m_LastSetSpectatorMode;
	int m_LastChangeInfo;
	int m_LastEmote;
	int m_LastKill;

	// TODO: clean this up
	struct
	{
		char m_SkinName[64];
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;
	} m_TeeInfos;

	int m_RespawnTick;
	int m_DieTick;
	int m_Score;
	int m_ScoreStartTick;
	bool m_ForceBalanced;
	int m_LastActionTick;
	int m_TeamChangeTick;
	struct
	{
		int m_TargetX;
		int m_TargetY;
	} m_LatestActivity;

	// network latency calculations
	struct
	{
		int m_Accum;
		int m_AccumMin;
		int m_AccumMax;
		int m_Avg;
		int m_Min;
		int m_Max;
	} m_Latency;

	// MineTee
	CCellData m_aInventory[NUM_INVENTORY_CELLS];
	CCellData m_aCraft[NUM_CRAFT_CELLS];
	CCellData m_aCraftRecipe[NUM_RECIPE_CELLS];
	bool m_IsFirstJoin;
	int m_Level;
	bool IsBot() const { return m_Bot; }
	int GetBotType() const { return m_BotType; }
	int GetBotSubType() const { return m_BotSubType; }
	CPet* GetPet() { return m_pPet; }
	void SetPet(CPet *pPet)
	{
		m_pPet = pPet;
		if (m_pPet)
			m_pPet->SetOwner(this);
	}
	void SetHardTeam(int team) { m_Team = team; }
	void SetCharacter(CCharacter *pChar) { if (!m_pCharacter) m_pCharacter = pChar; }
	void FillAccountData(void *pAccountData);
	void UseAccountData(void *pAccountData);
	void SetBotType(int BotType) { m_BotType = BotType; }
	void SetBotSubType(int BotSubType) { m_BotSubType = BotSubType; }
	int GetFirstEmptyInventoryIndex();

private:
	CCharacter *m_pCharacter;
	CGameContext *m_pGameServer;
	CPet *m_pPet; // MineTee

	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const;

	//
	bool m_Spawning;
	int m_ClientID;
	int m_Team;

	// MineTee
	bool m_Bot;
	int m_BotType;
	int m_BotSubType;
	//
};

#endif
