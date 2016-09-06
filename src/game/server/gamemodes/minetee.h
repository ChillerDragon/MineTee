/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_GAMEMODES_MINETEE_H
#define GAME_SERVER_GAMEMODES_MINETEE_H
#include <game/server/gamecontroller.h>
#include <game/mapitems.h>
#include <game/block_manager.h>
#include <engine/shared/network.h>
#include <engine/shared/noise.h>

enum
{
	TYPE_CHESTS=1,
	TYPE_SIGNS,
	TYPE_MAP_SPAWN,
};

class CChest
{
public:
	CChest()
	{
		m_apItems = 0x0;
		m_NumItems = 0;
		mem_zero(m_aOwnerKey, sizeof(m_aOwnerKey));
	}

	unsigned m_NumItems;
	unsigned char m_aOwnerKey[MINETEE_USER_KEY_SIZE];
	CCellData *m_apItems;

	void Resize(int NumItems)
	{
		if (!m_apItems)
			return;

		const unsigned TotalSize = NumItems*sizeof(CCellData);
		CCellData *pNewItems = (CCellData*)mem_alloc(TotalSize, 1);
		mem_zero(pNewItems, TotalSize);
		mem_copy(pNewItems, m_apItems, m_NumItems*sizeof(CCellData));
		mem_free(m_apItems);
		m_apItems = pNewItems;
		m_NumItems = NumItems;
	}
};

class CSign
{
public:
	CSign(const unsigned char *pOwnerKey)
	{
		if (pOwnerKey)
			mem_copy(m_aOwnerKey, pOwnerKey, sizeof(m_aOwnerKey));
		else
			mem_zero(m_aOwnerKey, sizeof(m_aOwnerKey));
		mem_zero(m_aText, sizeof(m_aText));
	}

	char m_aText[MAX_INPUT_SIZE];
	unsigned char m_aOwnerKey[MINETEE_USER_KEY_SIZE];
};

class CGameControllerMineTee : public CGameController
{
	enum
	{
		TILE_CENTER = 0,
		TILE_LEFT_TOP,
		TILE_TOP,
		TILE_RIGHT_TOP,
		TILE_LEFT,
		TILE_RIGHT,
		TILE_LEFT_BOTTOM,
		TILE_BOTTOM,
		TILE_RIGHT_BOTTOM,
		NUM_TILE_POS
	};

	void ModifTile(ivec2 MapPos, int TileIndex, int Reserved = -1);
	bool GetPlayerArea(int ClientID, int *pStartX, int *pEndX, int *pStartY, int *pEndY);
	void GetPlayerArea(vec2 Pos, int *pStartX, int *pEndX, int *pStartY, int *pEndY);
	void GenerateTree(ivec2 Pos);

public:
	CGameControllerMineTee(class CGameContext *pGameServer);
	virtual void Tick();
	void UpdateLayerLights(vec2 Pos);

	virtual void OnCharacterSpawn(class CCharacter *pChr);
	virtual int OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon);
	virtual bool OnChat(int cid, int team, const char *msg);
	virtual bool CanSpawn(int Team, vec2 *pPos, int BotType, int BotSubType);
	virtual bool OnFunctionalBlock(int BlockID, ivec2 TilePos);
	virtual bool CheckBlockPosition(ivec2 MapPos);
	bool CanJoinTeam(int Team, int NotThisID);
	void OnClientActiveBlock(int ClientID);
	void OnPlayerPutBlock(int ClientID, ivec2 TilePos, int BlockID, int BlockFlags, int Reserved);
	void OnPlayerDestroyBlock(int ClientID, ivec2 TilePos);
	void OnClientMoveCell(int ClientID, int From, int To, unsigned char Qty);
	bool TakeBlockDamage(vec2 WorldPos, int WeaponItemID, int Dmg, int Owner);
	void SendCellsData(int ClientID, int CellsType);
	void GenerateMapSpawn();

	void CreateChestSingle(int ClientID, ivec2 TilePos, int NumTiles = 0, CCellData *pCellData = 0x0);

	void LoadData();
	void SaveData();

	std::map<int, CChest*> m_lpChests;
	std::map<int, CSign> m_lpSigns;

private:
	float m_TimeVegetal;
	float m_TimeEnv;
	float m_TimeDestruction;
    float m_TimeWear;
    float m_TimeCook;
    CPerlinOctave *m_pNoise;
    int64 m_AutoSaveTimer;

    void VegetationTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo);
    void EnvirionmentTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo);
    void DestructionTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo);
    void WearTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo);
    void CookTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo);

    void CheckCraft(int ClientID);

    void GenerateRandomSpawn(CSpawnEval *pEval, int Team);
};

#endif

