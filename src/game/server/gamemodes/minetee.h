/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_MINETEE_H
#define GAME_SERVER_GAMEMODES_MINETEE_H
#include <game/server/gamecontroller.h>
#include <game/mapitems.h>
#include <game/block_manager.h>
#include <engine/shared/network.h>

class CChest
{
public:
	CCellData m_aItems[NUM_CELLS_CHEST];

	bool Move(int From, int To);
	bool Remove(int Index, int amount = 0);
};


class CGameControllerMineTee : public CGameController
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

	void ModifTile(ivec2 MapPos, int TileIndex, int Reserved = -1);
	bool GetPlayerArea(int ClientID, int *pStartX, int *pEndX, int *pStartY, int *pEndY);
	void GetPlayerArea(vec2 Pos, int *pStartX, int *pEndX, int *pStartY, int *pEndY);

public:
	CGameControllerMineTee(class CGameContext *pGameServer);
	virtual void Tick();
	void UpdateLayerLights(vec2 Pos);

	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	virtual bool OnChat(int cid, int team, const char *msg);
	virtual bool CanSpawn(int Team, vec2 *pPos, int BotType);
	bool CanJoinTeam(int Team, int NotThisID);
	void OnClientActiveBlock(int ClientID);
	void OnPlayerPutBlock(int ClientID, ivec2 TilePos, int BlockID, int BlockFlags, int Reserved);
	void OnPlayerDestroyBlock(int ClientID, ivec2 TilePos, int BlockID);

	std::map<int, CChest*> m_lpChests;

private:
	float m_TimeVegetal;
	float m_TimeEnv;
	float m_TimeDestruction;
    float m_TimeWear;
    float m_TimeCook;

    void VegetationTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo);
    void EnvirionmentTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo);
    void DestructionTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo);
    void WearTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo);
    void CookTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo);

    void GenerateRandomSpawn(CSpawnEval *pEval, int Team);
};

#endif

