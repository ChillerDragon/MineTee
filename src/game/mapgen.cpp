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
		GameServer()->Collision()->ModifTile(TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
		GameServer()->Collision()->ModifTile(TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeFGLayerIndex(), 0, 0);
		GameServer()->Collision()->ModifTile(TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeBGLayerIndex(), 0, 0);
	}

	// generate a very basic map (for now, EXTEND ME!)
	for(int i = MineTeeLayerSize/1.3f; i < MineTeeLayerSize; i++)
	{
		vec2 TilePos = vec2(i%GameServer()->Layers()->MineTeeLayer()->m_Width, (i-(i%GameServer()->Layers()->MineTeeLayer()->m_Width))/GameServer()->Layers()->MineTeeLayer()->m_Width);
		GameServer()->Collision()->ModifTile(TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::DIRT, 0);
	}
}
