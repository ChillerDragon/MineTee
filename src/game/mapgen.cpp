#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include "mapgen.h"
#include <game/server/gamecontext.h>
#include <game/layers.h>

CMapGen::CMapGen(CGameWorld *pGameWorld)
{
	m_pGameWorld = pGameWorld;
}

void CMapGen::GenerateMap()
{
	int MineTeeLayerSize = 0;
	if (GameServer()->Layers()->MineTeeLayer())
	{
        MineTeeLayerSize = GameServer()->Layers()->MineTeeLayer()->m_Width*GameServer()->Layers()->MineTeeLayer()->m_Height;
	}

	// clear map, but keep background, envelopes etc
	for(int i = 0; i < MineTeeLayerSize; i++)
	{
		vec2 TilePos = vec2(i%GameServer()->Layers()->MineTeeLayer()->m_Width, (i-(i%GameServer()->Layers()->MineTeeLayer()->m_Width))/GameServer()->Layers()->MineTeeLayer()->m_Width);
		
		// clear the different layers
		GameServer()->Collision()->CreateTile(TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
		GameServer()->Collision()->CreateTile(TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeFGLayerIndex(), 0, 0);
		GameServer()->Collision()->CreateTile(TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeBGLayerIndex(), 0, 0);
	}

	// initial Y
	int TilePosY = DIRT_LEVEL;
	int TilePosX = 0;
	for(int i = 0; i < GameServer()->Layers()->MineTeeLayer()->m_Width; i++)
	{
		TilePosX = i;
		TilePosY -= (rand()%3)-1;

		if(TilePosY - DIRT_LEVEL < LEVEL_TOLERANCE*-1)
			TilePosY++;
		else if(TilePosY - DIRT_LEVEL > LEVEL_TOLERANCE)
			TilePosY--;

		GameServer()->Collision()->CreateTile(vec2(TilePosX, TilePosY), GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::DIRT, 0);
		
		// fill the tiles under the random tile
		int TempTileY = TilePosY+1;
		while(TempTileY < GameServer()->Layers()->MineTeeLayer()->m_Height)
		{
			GameServer()->Collision()->CreateTile(vec2(TilePosX, TempTileY), GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::DIRT, 0);
			TempTileY++;
		}
	}

	// initial Y
	TilePosY = STONE_LEVEL;
	for(int i = 0; i < GameServer()->Layers()->MineTeeLayer()->m_Width; i++)
	{
		TilePosX = i;
		TilePosY -= (rand()%3)-1;
		
		if(TilePosY - STONE_LEVEL < LEVEL_TOLERANCE*-1)
			TilePosY++;
		else if(TilePosY - STONE_LEVEL > LEVEL_TOLERANCE)
			TilePosY--;

		GameServer()->Collision()->CreateTile(vec2(TilePosX, TilePosY), GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::STONE, 0);
		
		// fill the tiles under the random tile
		int TempTileY = TilePosY+1;
		while(TempTileY < GameServer()->Layers()->MineTeeLayer()->m_Height)
		{
			GameServer()->Collision()->CreateTile(vec2(TilePosX, TempTileY), GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::STONE, 0);
			TempTileY++;
		}
	}
}
