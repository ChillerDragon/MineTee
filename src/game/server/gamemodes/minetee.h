/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_MINETEE_H
#define GAME_SERVER_GAMEMODES_MINETEE_H
#include <game/server/gamecontroller.h>

#define MAP_CHUNK	32*24

class CGameControllerMineTee : public IGameController
{
	enum
	{
		TILE_CENTER = 0,
		TILE_TOP,
		TILE_LEFT_TOP,
		TILE_LEFT,
		TILE_LEFT_BOTTOM,
		TILE_BOTTOM,
		TILE_RIGHT_BOTTOM,
		TILE_RIGHT,
		TILE_RIGHT_TOP,
		NUM_TILE_POS
	};

	void ModifTile(vec2 MapPos, int TileIndex);
	void ModifTile(int x, int y, int TileIndex) { ModifTile(vec2(x, y), TileIndex); }
	void GetPlayerArea(int ClientID, int *pStartX, int *pEndX, int *pStartY, int *pEndY);

public:
	CGameControllerMineTee(class CGameContext *pGameServer);
	virtual void Tick();

	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	virtual bool OnChat(int cid, int team, const char *msg);
	bool CanJoinTeam(int Team, int NotThisID);

private:
	float m_TimeVegetal;
	float m_TimeEnv;
	float m_TimeDestruction;
    float m_TimeWear;
    float m_TimeCook;
};

#endif

