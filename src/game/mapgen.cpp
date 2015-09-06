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

		GameServer()->Collision()->CreateTile(vec2(TilePosX, TilePosY), GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::GRASS, 0);
		
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

	GenerateTrees();
}

void CMapGen::GenerateTrees()
{
	int LastTreeX = 0;

	for(int i = 10; i < GameServer()->Layers()->MineTeeLayer()->m_Width-10; i++)
	{
		if((rand()%64 == 0 && abs(LastTreeX - i) >= 8) || (abs(LastTreeX - i) <= 8 && abs(LastTreeX - i) >= 3 && rand()%8 == 0))
		{
			int TempTileY = 0;
			while(GameServer()->Collision()->GetMineTeeTileAt(vec2(i*32, (TempTileY+1)*32)) != CBlockManager::GRASS && TempTileY < GameServer()->Layers()->MineTeeLayer()->m_Height)
			{
				TempTileY++;
			}

			if(TempTileY >= GameServer()->Layers()->MineTeeLayer()->m_Height)
				return;

			LastTreeX = i;

			// plant the tree \o/
			int TreeHeight = 4 + (rand()%5);
			for(int h = 0; h <= TreeHeight; h++)
				GameServer()->Collision()->CreateTile(vec2(i, TempTileY-h), GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::WOOD_BROWN, 0);
		
			// add the leafs
			for(int l = 0; l <= TreeHeight/2.5f; l++)
				GameServer()->Collision()->CreateTile(vec2(i, TempTileY - TreeHeight - l), GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::LEAFS, 0);
		
			int TreeWidth = TreeHeight/1.2f;
			if(!(TreeWidth%2)) // odd numbers please
				TreeWidth++;
			for(int h = 0; h <= TreeHeight/2; h++)
			{
				for(int w = 0; w < TreeWidth; w++)
				{
					if(GameServer()->Collision()->GetMineTeeTileAt(vec2((i-(w-(TreeWidth/2)))*32, (TempTileY-(TreeHeight-(TreeHeight/2.5f)+h))*32)) == CBlockManager::WOOD_BROWN)
						continue;

					GameServer()->Collision()->CreateTile(vec2(i-(w-(TreeWidth/2)), TempTileY-(TreeHeight-(TreeHeight/2.5f)+h)), GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::LEAFS, 0);
				}
			}

		}
	}
}
