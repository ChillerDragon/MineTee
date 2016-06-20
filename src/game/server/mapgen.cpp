#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>
#include <engine/shared/config.h>

#include "mapgen.h"
#include "mapstructures.h"
#include <game/server/gamecontext.h>
#include <game/layers.h>
#include <game/mapitems.h>

CMapGen::CMapGen()
{
	m_pLayers = 0x0;
	m_pCollision = 0x0;
	m_pBlockManager = 0x0;
	m_pNoise = 0x0;
}
CMapGen::~CMapGen()
{
	if (m_pNoise)
		delete m_pNoise;
}

void CMapGen::Init(CLayers *pLayers, CCollision *pCollision, CBlockManager *pBlockManager)
{
	m_pLayers = pLayers;
	m_pCollision = pCollision;
	m_pBlockManager = pBlockManager;
}

void CMapGen::FillMap(int Seed)
{
	if (!m_pLayers->MineTeeLayer())
		return;

	dbg_msg("mapget", "started map generation with seed=%d", Seed);

	if (m_pNoise)
		delete m_pNoise;
	m_pNoise = new CPerlinOctave(7, Seed);

	int64 ProcessTime = 0;
	int64 TotalTime = time_get();

	int MineTeeLayerSize = m_pLayers->MineTeeLayer()->m_Width*m_pLayers->MineTeeLayer()->m_Height;

	// clear map, but keep background, envelopes etc
	ProcessTime = time_get();
	for(int i = 0; i < MineTeeLayerSize; i++)
	{
		int x = i%m_pLayers->MineTeeLayer()->m_Width;
		ivec2 TilePos(x, (i-x)/m_pLayers->MineTeeLayer()->m_Width);
		
		// clear the different layers
		m_pCollision->ModifTile(TilePos, m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), 0, 0);
		m_pCollision->ModifTile(TilePos, m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeFGLayerIndex(), 0, 0);
		m_pCollision->ModifTile(TilePos, m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeBGLayerIndex(), 0, 0);
	}
	dbg_msg("mapgen", "map sanitized in %.5fs", (float)(time_get()-ProcessTime)/time_freq());

	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	/* ~~~ Generate the world ~~~ */
	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	// terrain
	ProcessTime = time_get();
	GenerateBasicTerrain();
	GenerateBiomes(CBlockManager::SAND);
	dbg_msg("mapgen", "terrain generated in %.5fs", (float)(time_get()-ProcessTime)/time_freq());

	// tunnels
	ProcessTime = time_get();
	GenerateTunnels(m_pNoise->Perlin()->GetURandom(10,20));
	dbg_msg("mapgen", "tunnels generated in %.5fs", (float)(time_get()-ProcessTime)/time_freq());

	// ores
	ProcessTime = time_get();
	GenerateOre(CBlockManager::COAL_ORE, 200.0f, COAL_LEVEL, 50, 4);
	GenerateOre(CBlockManager::IRON_ORE, 320.0f, IRON_LEVEL, 30, 2);
	GenerateOre(CBlockManager::GOLD_ORE, 350.0f, GOLD_LEVEL, 15, 2);
	GenerateOre(CBlockManager::DIAMOND_ORE, 680.0f, DIAMOND_LEVEL, 15, 1);
	dbg_msg("mapgen", "ores generated in %.5fs", (float)(time_get()-ProcessTime)/time_freq());

	// caves
	ProcessTime = time_get();
	GenerateCaves(CBlockManager::AIR);
	GenerateCaves(CBlockManager::WATER_D);
	GenerateCaves(CBlockManager::LAVA_D);
	dbg_msg("mapgen", "caves generated in %.5fs", (float)(time_get()-ProcessTime)/time_freq());

	// Improve Environment
	ProcessTime = time_get();
	DoFallSteps();
	dbg_msg("mapgen", "environment processed in %.5fs", (float)(time_get()-ProcessTime)/time_freq());

	// Fluids
	ProcessTime = time_get();
	GenerateWater();
	DoWaterSteps();
	dbg_msg("mapgen", "fluids generated in %.5fs", (float)(time_get()-ProcessTime)/time_freq());

	// vegetation
	ProcessTime = time_get();
	GenerateFlowers();
	GenerateMushrooms();
	GenerateTrees();
	dbg_msg("mapgen", "vegetation generated in %.5fs", (float)(time_get()-ProcessTime)/time_freq());

	// Spawn Bosses
	GenerateBossZones();

	// misc
	dbg_msg("mapgen", "creating boarders...");
	GenerateBorder(); // as long as there are no infinite (chunked) maps

	// Performance
	dbg_msg("mapgen", "finalizing map...");
	GenerateSkip();

	dbg_msg("mapgen", "map successfully generated in %.5fs", (float)(time_get()-TotalTime)/time_freq());
}

void CMapGen::GenerateBasicTerrain()
{
	const int DirtLevel = PercOf(DIRT_LEVEL, m_pLayers->MineTeeLayer()->m_Height);
	const int StoneLevel = PercOf(STONE_LEVEL, m_pLayers->MineTeeLayer()->m_Height);
	const int ToleranceLevel = PercOf(LEVEL_TOLERANCE, m_pLayers->MineTeeLayer()->m_Height);

	// generate the surface, 1d noise
	int TilePosY;
	for(int TilePosX = 0; TilePosX < m_pLayers->MineTeeLayer()->m_Width; TilePosX++)
	{
		float frequency = 10.0f / (float)m_pLayers->MineTeeLayer()->m_Width;
		TilePosY = m_pNoise->Noise((float) TilePosX * frequency) * (m_pLayers->MineTeeLayer()->m_Height) + DirtLevel;
		TilePosY = max(TilePosY, 1);

		m_pCollision->ModifTile(ivec2(TilePosX, TilePosY), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), (TilePosY >= DirtLevel)?CBlockManager::GRASS:CBlockManager::DIRT_SNOW, 0);
		
		// fill the tiles under the defined tile
		const int startTilePos = TilePosY+1;
		int TempTileY = startTilePos;
		while(TempTileY < m_pLayers->MineTeeLayer()->m_Height)
		{
			m_pCollision->ModifTile(ivec2(TilePosX, TempTileY), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::DIRT, 0);
			if (TempTileY-startTilePos > 4) // Background
				m_pCollision->ModifTile(ivec2(TilePosX, TempTileY), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeBGLayerIndex(), CBlockManager::DIRT, 0);
			TempTileY++;
		}
	}

	// fill the underground with stones
	for(int TilePosX = 0; TilePosX < m_pLayers->MineTeeLayer()->m_Width; TilePosX++)
	{
		TilePosY -= (m_pNoise->Perlin()->GetURandom(0,3))-1;
		
		if(TilePosY - StoneLevel < ToleranceLevel*-1)
			TilePosY++;
		else if(TilePosY - StoneLevel > ToleranceLevel)
			TilePosY--;

		m_pCollision->ModifTile(ivec2(TilePosX, TilePosY), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::STONE, 0);
		
		// fill the tiles under the random tile
		const int startTilePos = TilePosY+1;
		int TempTileY = startTilePos;
		while(TempTileY < m_pLayers->MineTeeLayer()->m_Height)
		{
			m_pCollision->ModifTile(ivec2(TilePosX, TempTileY), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::STONE, 0);
			if (TempTileY-startTilePos > 4) // Background
				m_pCollision->ModifTile(ivec2(TilePosX, TempTileY), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeBGLayerIndex(), CBlockManager::STONE, 0);
			TempTileY++;
		}
	}
}

void CMapGen::GenerateOre(int Type, float F, int Level, int Radius, int ClusterSize)
{
	for(int x = 0; x < m_pLayers->MineTeeLayer()->m_Width; x++)
	{
		for(int y = Level; y < Level + Radius; y++)
		{
			float frequency = F / (float)m_pLayers->MineTeeLayer()->m_Width;
			float noise = m_pNoise->Noise((float)x * frequency, (float)y * frequency);

			if(noise > 0.85f)
			{
				// place the "entry point"-tile
				m_pCollision->ModifTile(ivec2(x, y), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), Type, 0);

				// generate a cluster
				int CS = ClusterSize + m_pNoise->Perlin()->GetURandom(0,4);

				for(int cx = x-CS/2; cx < x+CS/2; cx++)
				{
					for(int cy = y-CS/2; cy < y+CS/2; cy++)
					{
						if(!m_pNoise->Perlin()->GetURandom(0,3))
							m_pCollision->ModifTile(ivec2(cx, cy), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), Type, 0);
					}
				}
			}
		}
	}
}

void CMapGen::GenerateCaves(int FillBlock)
{
	const int StoneLevel = PercOf(STONE_LEVEL, m_pLayers->MineTeeLayer()->m_Height);

	// cut in the caves with a 2d perlin noise
	for(int x = 0; x < m_pLayers->MineTeeLayer()->m_Width; x++)
	{
		for(int y = StoneLevel; y < m_pLayers->MineTeeLayer()->m_Height; y++)
		{
			float frequency = 32.0f / (float)m_pLayers->MineTeeLayer()->m_Width;
			float noise = m_pNoise->Noise((float)x * frequency, (float)y * frequency);
	
			if(noise > 0.4f)
				m_pCollision->ModifTile(ivec2(x, y), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), FillBlock, 0);
		}
	}
}

void CMapGen::GenerateBiomes(int FillBlock)
{
	const int StoneLevel = PercOf(DIRT_LEVEL, m_pLayers->MineTeeLayer()->m_Height);

	// cut in the caves with a 2d perlin noise
	for(int x = 0; x < m_pLayers->MineTeeLayer()->m_Width; x++)
	{
		for(int y = StoneLevel; y < m_pLayers->MineTeeLayer()->m_Height; y++)
		{
			float frequency = 32.0f / (float)m_pLayers->MineTeeLayer()->m_Width;
			float noise = m_pNoise->Noise((float)x * frequency, (float)y * frequency);

			if(noise > 0.4f)
				m_pCollision->ModifTile(ivec2(x, y), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), FillBlock, 0);
		}
	}
}

void CMapGen::GenerateTunnels(int Num)
{
	const int TunnelLevel = PercOf(TUNNEL_LEVEL, m_pLayers->MineTeeLayer()->m_Height);

	int TilePosY, Level, Freq, TunnelSize, StartX, EndX;
	for (int r=0; r<Num; ++r)
	{
		Level = TunnelLevel + m_pNoise->Perlin()->GetURandom(0,m_pLayers->MineTeeLayer()->m_Height);
		Freq = m_pNoise->Perlin()->GetURandom(2,5);
		TunnelSize = m_pNoise->Perlin()->GetURandom(2,2);
		StartX = m_pNoise->Perlin()->GetURandom(0,m_pLayers->MineTeeLayer()->m_Width);
		EndX = min(m_pLayers->MineTeeLayer()->m_Width, m_pNoise->Perlin()->GetURandom(0,m_pLayers->MineTeeLayer()->m_Width)+StartX);
		for(int TilePosX = StartX; TilePosX < EndX; TilePosX++)
		{
			float frequency = Freq / (float)m_pLayers->MineTeeLayer()->m_Width;
			TilePosY = m_pNoise->Noise((float)TilePosX * frequency) * (m_pLayers->MineTeeLayer()->m_Height) + Level;
			if (TilePosY < m_pLayers->MineTeeLayer()->m_Height-1)
				for (int i=-TunnelSize; i<=TunnelSize; i++)
					m_pCollision->ModifTile(ivec2(TilePosX, TilePosY+i), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::AIR, 0);
		}
	}
}

void CMapGen::GenerateWater()
{
	const int WaterLevel = PercOf(WATER_LEVEL, m_pLayers->MineTeeLayer()->m_Height);

	for(int x = 0; x < m_pLayers->MineTeeLayer()->m_Width; x++)
		for(int y = WaterLevel; m_pCollision->GetMineTeeTileAt(vec2(x*32, y*32)) == CBlockManager::AIR; y++)
			m_pCollision->ModifTile(ivec2(x, y), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), y==WaterLevel?CBlockManager::WATER_A:CBlockManager::WATER_D, 0);
}

void CMapGen::GenerateFlowers()
{
	const int aFlowers[] = {CBlockManager::ROSE, CBlockManager::DAISY, CBlockManager::BROWN_TREE_SAPLING, CBlockManager::GRASS_A};
	for(int x = 1; x < m_pLayers->MineTeeLayer()->m_Width-1; x++)
	{
		int FieldSize = m_pNoise->Perlin()->GetURandom(2,3);
		int rnd = m_pNoise->Perlin()->GetURandom(0,(sizeof(aFlowers)/sizeof(int))-1);
		if(!m_pNoise->Perlin()->GetURandom(0,32))
		{
			for(int f = x; f - x <= FieldSize; f++)
			{
				int y = 0;
				while(m_pCollision->GetMineTeeTileAt(vec2(f*32, (y+1)*32)) != CBlockManager::GRASS && y < m_pLayers->MineTeeLayer()->m_Height-1)
					y++;

				if(m_pCollision->GetMineTeeTileAt(vec2(f*32, y*32)) != CBlockManager::AIR)
					continue;

				m_pCollision->ModifTile(ivec2(f, y), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), aFlowers[rnd], 0);
			}

			x += FieldSize;
		}
	}
}

void CMapGen::GenerateMushrooms()
{
	for(int x = 1; x < m_pLayers->MineTeeLayer()->m_Width-1; x++)
	{
		int FieldSize = m_pNoise->Perlin()->GetURandom(5,8);
		if(!m_pNoise->Perlin()->GetURandom(0,256))
		{
			for(int f = x; f - x <= FieldSize; f++)
			{
				int y = 0;
				while(m_pCollision->GetMineTeeTileAt(vec2(f*32, (y+1)*32)) != CBlockManager::GRASS
						&& m_pCollision->GetMineTeeTileAt(vec2(f*32, y*32)) != CBlockManager::AIR
						&& y < m_pLayers->MineTeeLayer()->m_Height)
				{
					y++;
				}

				if(y >= m_pLayers->MineTeeLayer()->m_Height)
					break;

				if(m_pCollision->GetMineTeeTileAt(vec2(x*32, f*32)) != CBlockManager::AIR)
					continue;

				m_pCollision->ModifTile(ivec2(f, y), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::MUSHROOM_RED + m_pNoise->Perlin()->GetURandom(0,2), 0);
			}

			x += FieldSize;
		}
	}
}

void CMapGen::GenerateTrees()
{
	int LastTreeX = 0;

	for(int x = 10; x < m_pLayers->MineTeeLayer()->m_Width-10; x++)
	{
		// trees like to spawn in groups
		if((abs(LastTreeX - x) >= 8) || (abs(LastTreeX - x) <= 8 && abs(LastTreeX - x) >= 3 && !m_pNoise->Perlin()->GetURandom(0,8)))
		{
			int TempTileY = 0;
			while((m_pCollision->GetMineTeeTileAt(vec2(x*32, (TempTileY+1)*32)) != CBlockManager::GRASS
					|| m_pCollision->GetMineTeeTileAt(vec2(x*32, (TempTileY+1)*32)) != CBlockManager::SAND)
					&& m_pCollision->GetMineTeeTileAt(vec2(x*32, TempTileY*32)) != CBlockManager::AIR
					&& TempTileY < m_pLayers->MineTeeLayer()->m_Height)
			{
				TempTileY++;
			}

			if(TempTileY >= m_pLayers->MineTeeLayer()->m_Height)
				continue;

			const int TileBase = m_pCollision->GetMineTeeTileAt(vec2(x*32, TempTileY*32));
			if (TileBase == CBlockManager::GRASS)
				GenerateTree(ivec2(x, TempTileY));
			else if (TileBase == CBlockManager::SAND)
			{
				for (int i=0; i>-4; i--)
					m_pCollision->ModifTile(ivec2(x, TempTileY+i), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::CACTUS, 0);
			}
			LastTreeX = x;
		}
	}
}

void CMapGen::GenerateTree(ivec2 Pos)
{
	// plant the tree \o/
	int TreeHeight = m_pNoise->Perlin()->GetURandom(4,5);
	for(int h = 0; h <= TreeHeight; h++)
		m_pCollision->ModifTile(ivec2(Pos.x, Pos.y-h), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::WOOD_BROWN, 0);

	// add the leafs
	for(int l = 0; l <= TreeHeight/2.5f; l++)
		m_pCollision->ModifTile(ivec2(Pos.x, Pos.y - TreeHeight - l), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::LEAFS, 0);

	int TreeWidth = TreeHeight/1.2f;
	if(!(TreeWidth%2)) // odd numbers please
		TreeWidth++;
	for(int h = 0; h <= TreeHeight/2; h++)
	{
		for(int w = 0; w < TreeWidth; w++)
		{
			if(m_pCollision->GetMineTeeTileAt(vec2((Pos.x-(w-(TreeWidth/2)))*32, (Pos.y-(TreeHeight-(TreeHeight/2.5f)+h))*32)) != CBlockManager::AIR)
				continue;

			m_pCollision->ModifTile(ivec2(Pos.x-(w-(TreeWidth/2)), Pos.y-(TreeHeight-(TreeHeight/2.5f)+h)), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::LEAFS, 0);
		}
	}
}

void CMapGen::GenerateBorder()
{
	// draw a boarder all around the map
	for(int i = 0; i < m_pLayers->MineTeeLayer()->m_Width; i++)
	{
		// bottom border
		m_pCollision->ModifTile(ivec2(i, m_pLayers->MineTeeLayer()->m_Height-1), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::BEDROCK, 0);
	}

	for(int i = 0; i < m_pLayers->MineTeeLayer()->m_Height; i++)
	{
		// left border
		m_pCollision->ModifTile(ivec2(0, i), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::BEDROCK, 0);
		
		// right border
		m_pCollision->ModifTile(ivec2(m_pLayers->MineTeeLayer()->m_Width-1, i), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::BEDROCK, 0);
	}
}

void CMapGen::GenerateSkip()
{

	for(int g = 0; g < m_pLayers->NumGroups(); g++)
	{
		CMapItemGroup *pGroup = m_pLayers->GetGroup(g);

		for(int l = 0; l < pGroup->m_NumLayers; l++)
		{
			CMapItemLayer *pLayer = m_pLayers->GetLayer(pGroup->m_StartLayer+l);

			if(pLayer->m_Type == LAYERTYPE_TILES)
			{
				CMapItemLayerTilemap *pTmap = (CMapItemLayerTilemap *)pLayer;
				CTile *pTiles = (CTile *)m_pLayers->Map()->GetData(pTmap->m_Data);
				for(int y = 0; y < pTmap->m_Height; y++)
				{
					for(int x = 1; x < pTmap->m_Width;)
					{
						int sx;
						for(sx = 1; x+sx < pTmap->m_Width && sx < 255; sx++)
						{
							if(pTiles[y*pTmap->m_Width+x+sx].m_Index)
								break;
						}

						pTiles[y*pTmap->m_Width+x].m_Skip = sx-1;
						x += sx;
					}
				}
			}
		}
	}
}

void CMapGen::GenerateBossZones()
{
	const int BossLevel = PercOf(BOSS_LEVEL, m_pLayers->MineTeeLayer()->m_Height);

	ivec2 SpawnPos;
	for (int i=0; i<NUM_BOSSES; i++)
	{
		SpawnPos.x = m_pNoise->Perlin()->GetURandom(1,m_pLayers->MineTeeLayer()->m_Width-1);
		SpawnPos.y = m_pNoise->Perlin()->GetURandom(BossLevel, m_pLayers->MineTeeLayer()->m_Height-1);

		if (i == BOT_BOSS_DUNE)
			CreateStructure(STRUCTURE_SPAWN_BOSS_DUNE, SpawnPos);

		m_pCollision->ModifTile(SpawnPos, m_pLayers->GetGameGroupIndex(), m_pLayers->GetGameLayerIndex(), ENTITY_OFFSET+ENTITY_SPAWN_BOSS_DUNE+i, 0);
	}
}

void CMapGen::CreateStructure(int StructureID, ivec2 Pos)
{
	const int M = NUM_STRUCTURE_TILES/2;
	ivec2 TilePos;
	int h=0;
	for (int i=0; i<NUM_STRUCTURE_TILES*NUM_STRUCTURE_TILES; i++)
	{
		if (i%NUM_STRUCTURE_TILES == 0)
			++h;

		TilePos.x = (Pos.x - M) + i%NUM_STRUCTURE_TILES;
		TilePos.y = (Pos.y - M) + h;

		m_pCollision->ModifTile(TilePos, m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeBGLayerIndex(), gs_MapStructures[StructureID][0][i], 0);
		m_pCollision->ModifTile(TilePos, m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), gs_MapStructures[StructureID][1][i], 0);
		m_pCollision->ModifTile(TilePos, m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeFGLayerIndex(), gs_MapStructures[StructureID][2][i], 0);
	}
}

void CMapGen::DoFallSteps()
{
	for(int TilePosX = 0; TilePosX < m_pLayers->MineTeeLayer()->m_Width; TilePosX++)
	{
		for(int TilePosY = m_pLayers->MineTeeLayer()->m_Height-1; TilePosY >= 0; TilePosY--)
		{
			const int TileIndexCenter = m_pCollision->GetMineTeeTileAt(vec2(TilePosX*32, TilePosY*32));
			const int TileIndexBottom = m_pCollision->GetMineTeeTileAt(vec2(TilePosX*32, (TilePosY+1)*32));
			const CBlockManager::CBlockInfo *pBlockInfo = m_pBlockManager->GetBlockInfo(TileIndexCenter);

			if (pBlockInfo->m_Gravity && (TileIndexBottom == 0 || m_pBlockManager->IsFluid(TileIndexBottom)))
			{
				int e=TilePosY+1;
				for (; e<m_pLayers->MineTeeLayer()->m_Height; e++)
				{
					const int TileIndex = m_pCollision->GetMineTeeTileAt(vec2(TilePosX*32, e*32));
					if (TileIndex != 0 && !m_pBlockManager->IsFluid(TileIndex))
					{
						--e;
						break;
					}
				}

				if (e != TilePosY)
				{
					m_pCollision->ModifTile(ivec2(TilePosX, e), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), TileIndexCenter, 0);
					m_pCollision->ModifTile(ivec2(TilePosX, TilePosY), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), 0, 0);
				}
			}
		}
	}
}
void CMapGen::DoWaterSteps()
{
	for (int q=0; q<20; q++)
	{
		for(int TilePosX = 0; TilePosX < m_pLayers->MineTeeLayer()->m_Width; TilePosX++)
		{
			for(int TilePosY = 0; TilePosY < m_pLayers->MineTeeLayer()->m_Height; TilePosY++)
			{
				const int TileIndexCenter = m_pCollision->GetMineTeeTileAt(vec2(TilePosX*32, TilePosY*32));
				int FluidType = 0;
				if (!m_pBlockManager->IsFluid(TileIndexCenter, &FluidType))
					continue;

				const int TileIndexBottom = m_pCollision->GetMineTeeTileAt(vec2(TilePosX*32, (TilePosY+1)*32));
				const int TileIndexRight = m_pCollision->GetMineTeeTileAt(vec2((TilePosX+1)*32, TilePosY*32));
				const int TileIndexRightTop = m_pCollision->GetMineTeeTileAt(vec2((TilePosX+1)*32, (TilePosY-1)*32));
				const int TileIndexRightBottom = m_pCollision->GetMineTeeTileAt(vec2((TilePosX+1)*32, (TilePosY+1)*32));
				const int TileIndexTop = m_pCollision->GetMineTeeTileAt(vec2(TilePosX*32, (TilePosY-1)*32));
				const int TileIndexLeft = m_pCollision->GetMineTeeTileAt(vec2((TilePosX-1)*32, TilePosY*32));
				const int TileIndexLeftTop = m_pCollision->GetMineTeeTileAt(vec2((TilePosX-1)*32, (TilePosY-1)*32));
				const int TileIndexLeftBottom = m_pCollision->GetMineTeeTileAt(vec2((TilePosX-1)*32, (TilePosY+1)*32));
				const CBlockManager::CBlockInfo *pBlockBottomInfo = m_pBlockManager->GetBlockInfo(TileIndexBottom);

				// Fall
				if (TileIndexBottom == 0 || (pBlockBottomInfo && !pBlockBottomInfo->m_PlayerCollide) ||
					(TileIndexBottom >= CBlockManager::WATER_A && TileIndexBottom < CBlockManager::WATER_C) ||
					(TileIndexBottom >= CBlockManager::LAVA_A && TileIndexBottom < CBlockManager::LAVA_C))
				{
					int TileIndexTemp = (FluidType==CBlockManager::FLUID_WATER)?CBlockManager::WATER_C:CBlockManager::LAVA_C;
					m_pCollision->ModifTile(ivec2(TilePosX, TilePosY+1), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), TileIndexTemp, 0);
				}
				// Create Obsidian?
				else if ((FluidType == CBlockManager::FLUID_LAVA && TileIndexBottom >= CBlockManager::WATER_A && TileIndexBottom <= CBlockManager::WATER_D) ||
						 (FluidType == CBlockManager::FLUID_WATER && TileIndexBottom >= CBlockManager::LAVA_A && TileIndexBottom <= CBlockManager::LAVA_D))
				{
					m_pCollision->ModifTile(ivec2(TilePosX, TilePosY+1), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::OBSIDIAN_A, 0);
				}

				// Spread To Right
				if (TileIndexRight == 0 && TileIndexBottom != 0 &&
					!m_pBlockManager->IsFluid(TileIndexBottom) &&
					!m_pBlockManager->IsFluid(TileIndexRightBottom) &&
					!m_pBlockManager->IsFluid(TileIndexRightTop) &&
					TileIndexCenter-1 != CBlockManager::WATER_A-1 && TileIndexCenter-1 != CBlockManager::LAVA_A-1)
				{
					m_pCollision->ModifTile(ivec2(TilePosX+1, TilePosY), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), TileIndexCenter-1, 0);
				}
				// Spread To Left
				if (TileIndexLeft == 0 && TileIndexBottom != 0 &&
					!m_pBlockManager->IsFluid(TileIndexBottom) &&
					!m_pBlockManager->IsFluid(TileIndexLeftBottom) &&
					!m_pBlockManager->IsFluid(TileIndexLeftTop) &&
					TileIndexCenter-1 != CBlockManager::WATER_A-1 && TileIndexCenter-1 != CBlockManager::LAVA_A-1)
				{
					m_pCollision->ModifTile(ivec2(TilePosX-1, TilePosY), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), TileIndexCenter-1, 0);
				}

				// Check for correct tiles FIXME
				int FluidTypeTop = 0;
				bool IsFluidTop = m_pBlockManager->IsFluid(TileIndexTop, &FluidTypeTop);
				if (IsFluidTop && TileIndexCenter < CBlockManager::WATER_C)
				{
					m_pCollision->ModifTile(ivec2(TilePosX, TilePosY), m_pLayers->GetMineTeeGroupIndex(), m_pLayers->GetMineTeeLayerIndex(), FluidTypeTop==CBlockManager::FLUID_WATER?CBlockManager::WATER_C:CBlockManager::LAVA_C, 0);
				}
			}
		}
	}
}
