/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* File modified by Alexandre Díaz */
#ifndef GAME_SERVER_GAMECONTROLLER_H
#define GAME_SERVER_GAMECONTROLLER_H

#include <base/math.h>
#include <base/vmath.h>
#include <base/system.h>
#include <engine/server.h> // MineTee

/*
	Class: Game Controller
		Controls the main game logic. Keeping track of team and player score,
		winning conditions and specific game logic.
*/
class CGameController
{
	vec2 m_aaSpawnPoints[3][64];
	int m_aNumSpawnPoints[3];

	vec2 m_aBossSpawnPoints[NUM_BOSSES];

	class CGameContext *m_pGameServer;
	class IServer *m_pServer;

protected:
	CGameContext *GameServer() const { return m_pGameServer; }
	IServer *Server() const { return m_pServer; }

	struct CSpawnEval
	{
		CSpawnEval()
		{
			m_Got = false;
			m_FriendlyTeam = -1;
			m_Pos = vec2(100,100);
			m_Score = 0;
		}

		vec2 m_Pos;
		bool m_Got;
		int m_FriendlyTeam;
		float m_Score;
	};

	float EvaluateSpawnPos(CSpawnEval *pEval, vec2 Pos);
	void EvaluateSpawnType(CSpawnEval *pEval, int Type);
	bool EvaluateSpawn(class CPlayer *pP, vec2 *pPos);

	void CycleMap();
	void ResetGame();

	char m_aMapWish[128];


	int m_RoundStartTick;
	int m_GameOverTick;
	int m_SuddenDeath;

	int m_aTeamscore[2];

	int m_Warmup;
	int m_UnpauseTimer;
	int m_RoundCount;

	int m_GameFlags;
	int m_UnbalancedTick;
	bool m_ForceBalanced;

	vec2 m_MapSpawn; // MineTee

public:
	const char *m_pGameType;

	bool IsTeamplay() const;
	bool IsGameOver() const { return m_GameOverTick != -1; }

	CGameController(class CGameContext *pGameServer);
	virtual ~CGameController();

	virtual void DoWincheck();

	void DoWarmup(int Seconds);
	void TogglePause();

	void StartRound();
	void EndRound();
	void ChangeMap(const char *pToMap);

	bool IsFriendlyFire(int ClientID1, int ClientID2);

	bool IsForceBalanced();

	/*

	*/
	virtual bool CanBeMovedOnBalance(int ClientID);

	virtual void Tick();

	virtual void Snap(int SnappingClient);

	/*
		Function: on_entity
			Called when the map is loaded to process an entity
			in the map.

		Arguments:
			index - Entity index.
			pos - Where the entity is located in the world.

		Returns:
			bool?
	*/
	virtual bool OnEntity(int Index, vec2 Pos);

	/*
		Function: on_CCharacter_spawn
			Called when a CCharacter spawns into the game world.

		Arguments:
			chr - The CCharacter that was spawned.
	*/
	virtual void OnCharacterSpawn(class CCharacter *pChr);

	/*
		Function: on_CCharacter_death
			Called when a CCharacter in the world dies.

		Arguments:
			victim - The CCharacter that died.
			killer - The player that killed it.
			weapon - What weapon that killed it. Can be -1 for undefined
				weapon when switching team or player suicides.
	*/
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int ItemID);


	virtual void OnPlayerInfoChange(class CPlayer *pP);

	//
	virtual bool CanSpawn(int Team, vec2 *pPos, int BotType, int BotSubType);

	/*

	*/
	virtual const char *GetTeamName(int Team);
	virtual int GetAutoTeam(int NotThisID);
	virtual bool CanJoinTeam(int Team, int NotThisID);
	bool CheckTeamBalance();
	bool CanChangeTeam(CPlayer *pPplayer, int JoinTeam);
	int ClampTeam(int Team);

	virtual void PostReset();

	// MineTee
	int GetRoundStartTick() const { return m_RoundStartTick; }
	void AdvanceRoundStartTick(int amount) { m_RoundStartTick = max(m_RoundStartTick+amount, 0); }
	virtual bool OnFunctionalBlock(int BlockID, ivec2 TilePos) { return false; }
	virtual bool OnChat(int cid, int team, const char *msg) { return true; };
	virtual void OnClientActiveBlock(int ClientID) {}
	virtual void SendCellsData(int ClientID, int CellsType) {}
	virtual void OnPlayerPutBlock(int ClientID, ivec2 TilePos, int BlockID, int BlockFlags, int Reserved) {}
	virtual void OnPlayerDestroyBlock(int ClientID, ivec2 TilePos) {}
	virtual bool TakeBlockDamage(vec2 WorldPos, int WeaponItemID, int Dmg, int Owner) { return false; }
	virtual void OnClientMoveCell(int ClientID, int From, int To, unsigned char Qty) {}
	virtual void LoadData() {}
	virtual void SaveData() {}
	virtual void GenerateMapSpawn() {}
	virtual vec2 GetMapSpawn() const { return m_MapSpawn; }
	virtual bool CheckBlockPosition(ivec2 MapPos) { return true; }
};

#endif
