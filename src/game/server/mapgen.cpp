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
		ModifTile(TilePos, m_pLayers->GetMineTeeLayerIndex(), 0);
		ModifTile(TilePos, m_pLayers->GetMineTeeFGLayerIndex(), 0);
		ModifTile(TilePos, m_pLayers->GetMineTeeBGLayerIndex(), 0);
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
	ProcessTime = time_get();
	GenerateBossZones();
	dbg_msg("mapgen", "Bosses spawned in %.5fs", (float)(time_get()-ProcessTime)/time_freq());

	// Chests
	ProcessTime = time_get();
	GenerateChests();
	dbg_msg("mapgen", "Chests generated in %.5fs", (float)(time_get()-ProcessTime)/time_freq());


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

		ModifTile(ivec2(TilePosX, TilePosY), m_pLayers->GetMineTeeLayerIndex(), (TilePosY >= DirtLevel)?CBlockManager::GRASS:CBlockManager::DIRT_SNOW);
		
		// fill the tiles under the defined tile
		const int startTilePos = TilePosY+1;
		int TempTileY = startTilePos;
		while(TempTileY < m_pLayers->MineTeeLayer()->m_Height)
		{
			ModifTile(ivec2(TilePosX, TempTileY), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::DIRT);
			if (TempTileY-startTilePos > 4) // Background
				ModifTile(ivec2(TilePosX, TempTileY), m_pLayers->GetMineTeeBGLayerIndex(), CBlockManager::DIRT);
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

		ModifTile(ivec2(TilePosX, TilePosY), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::STONE);
		
		// fill the tiles under the random tile
		const int startTilePos = TilePosY+1;
		int TempTileY = startTilePos;
		while(TempTileY < m_pLayers->MineTeeLayer()->m_Height)
		{
			ModifTile(ivec2(TilePosX, TempTileY), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::STONE);
			if (TempTileY-startTilePos > 4) // Background
				ModifTile(ivec2(TilePosX, TempTileY), m_pLayers->GetMineTeeBGLayerIndex(), CBlockManager::STONE);
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
				ModifTile(ivec2(x, y), m_pLayers->GetMineTeeLayerIndex(), Type);

				// generate a cluster
				int CS = ClusterSize + m_pNoise->Perlin()->GetURandom(0,4);

				for(int cx = x-CS/2; cx < x+CS/2; cx++)
				{
					for(int cy = y-CS/2; cy < y+CS/2; cy++)
					{
						if(!m_pNoise->Perlin()->GetURandom(0,3))
							ModifTile(ivec2(cx, cy), m_pLayers->GetMineTeeLayerIndex(), Type);
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
				ModifTile(ivec2(x, y), m_pLayers->GetMineTeeLayerIndex(), FillBlock);
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
				ModifTile(ivec2(x, y), m_pLayers->GetMineTeeLayerIndex(), FillBlock);
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
					ModifTile(ivec2(TilePosX, TilePosY+i), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::AIR);
		}
	}
}

void CMapGen::GenerateWater()
{
	const int WaterLevel = PercOf(WATER_LEVEL, m_pLayers->MineTeeLayer()->m_Height);

	for(int x = 0; x < m_pLayers->MineTeeLayer()->m_Width; x++)
		for(int y = WaterLevel; m_pCollision->GetMineTeeTileIndexAt(vec2(x*32, y*32)) == CBlockManager::AIR; y++)
			ModifTile(ivec2(x, y), m_pLayers->GetMineTeeLayerIndex(), y==WaterLevel?CBlockManager::WATER_A:CBlockManager::WATER_D);
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
				while(m_pCollision->GetMineTeeTileIndexAt(vec2(f*32, (y+1)*32)) != CBlockManager::GRASS && y < m_pLayers->MineTeeLayer()->m_Height-1)
					++y;

				if(m_pCollision->GetMineTeeTileIndexAt(vec2(f*32, y*32)) != CBlockManager::AIR)
					continue;

				ModifTile(ivec2(f, y), m_pLayers->GetMineTeeLayerIndex(), aFlowers[rnd]);
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
				while(m_pCollision->GetMineTeeTileIndexAt(vec2(f*32, (y+1)*32)) != CBlockManager::GRASS
						&& m_pCollision->GetMineTeeTileIndexAt(vec2(f*32, y*32)) != CBlockManager::AIR
						&& y < m_pLayers->MineTeeLayer()->m_Height)
				{
					++y;
				}

				if(y >= m_pLayers->MineTeeLayer()->m_Height)
					break;

				if(m_pCollision->GetMineTeeTileIndexAt(vec2(x*32, f*32)) != CBlockManager::AIR)
					continue;

				ModifTile(ivec2(f, y), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::MUSHROOM_RED + m_pNoise->Perlin()->GetURandom(0,2));
			}

			x += FieldSize;
		}
	}
}

void CMapGen::GenerateTrees()
{
	int LastTreeX = 0;
	const int TreesLevelMin = PercOf(VEGETATION_LEVEL_MIN, m_pLayers->MineTeeLayer()->m_Height);
	const int TreesLevelMax = PercOf(VEGETATION_LEVEL_MAX, m_pLayers->MineTeeLayer()->m_Height);

	for(int x = 10; x < m_pLayers->MineTeeLayer()->m_Width-10; x++)
	{
		// trees like to spawn in groups
		if((abs(LastTreeX - x) >= 8) || (abs(LastTreeX - x) <= 8 && abs(LastTreeX - x) >= 3 && !m_pNoise->Perlin()->GetURandom(0,8)))
		{
			int TempTileY = TreesLevelMin;
			while(m_pCollision->GetMineTeeTileIndexAt(vec2(x*32, (TempTileY+1)*32)) != CBlockManager::GRASS
					&& m_pCollision->GetMineTeeTileIndexAt(vec2(x*32, (TempTileY+1)*32)) != CBlockManager::SAND
					&& TempTileY < TreesLevelMax)
			{
				++TempTileY;
			}

			if(TempTileY >= TreesLevelMax || m_pCollision->GetMineTeeTileIndexAt(vec2(x*32, TempTileY*32)) != CBlockManager::AIR)
				continue;

			const int TileBase = m_pCollision->GetMineTeeTileIndexAt(vec2(x*32, (TempTileY+1)*32));
			if (TileBase == CBlockManager::GRASS)
				GenerateTree(ivec2(x, TempTileY));
			else if (TempTileY > 4 && TileBase == CBlockManager::SAND)
			{
				for (int i=0; i>-4; i--)
					ModifTile(ivec2(x, TempTileY+i), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::CACTUS);
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
		ModifTile(ivec2(Pos.x, Pos.y-h), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::WOOD_BROWN);

	// add the leafs
	for(int l = 0; l <= TreeHeight/2.5f; l++)
		ModifTile(ivec2(Pos.x, Pos.y - TreeHeight - l), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::LEAFS);

	int TreeWidth = TreeHeight/1.2f;
	if(!(TreeWidth%2)) // odd numbers please
		TreeWidth++;
	for(int h = 0; h <= TreeHeight/2; h++)
	{
		for(int w = 0; w < TreeWidth; w++)
		{
			if(m_pCollision->GetMineTeeTileIndexAt(vec2((Pos.x-(w-(TreeWidth/2)))*32, (Pos.y-(TreeHeight-(TreeHeight/2.5f)+h))*32)) != CBlockManager::AIR)
				continue;

			ModifTile(ivec2(Pos.x-(w-(TreeWidth/2)), Pos.y-(TreeHeight-(TreeHeight/2.5f)+h)), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::LEAFS);
		}
	}
}

void CMapGen::GenerateBorder()
{
	// draw a boarder all around the map
	for(int i = 0; i < m_pLayers->MineTeeLayer()->m_Width; i++)
	{
		// bottom border
		ModifTile(ivec2(i, m_pLayers->MineTeeLayer()->m_Height-1), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::BEDROCK);
	}

	for(int i = 0; i < m_pLayers->MineTeeLayer()->m_Height; i++)
	{
		// left border
		ModifTile(ivec2(0, i), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::BEDROCK);
		
		// right border
		ModifTile(ivec2(m_pLayers->MineTeeLayer()->m_Width-1, i), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::BEDROCK);
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
					for(int x = 0; x < pTmap->m_Width;)
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

		ModifTile(SpawnPos, m_pLayers->GetGameLayerIndex(), ENTITY_OFFSET+ENTITY_SPAWN_BOSS_DUNE+i);
	}
}

void CMapGen::GenerateChests()
{
	const int ChestLevel = PercOf(CHESTS_LEVEL, m_pLayers->MineTeeLayer()->m_Height);

	for(int x = 0; x < m_pLayers->MineTeeLayer()->m_Width; x++)
	{
		for(int y = ChestLevel; y < m_pLayers->MineTeeLayer()->m_Height; y++)
		{
			if (!m_pNoise->Perlin()->GetURandom(0, 100)
					&& m_pCollision->CheckPoint(x*32, (y+1)*32)
					&& m_pCollision->GetMineTeeTileIndexAt(vec2(x*32, y*32)) == CBlockManager::AIR
					&& m_pCollision->GetMineTeeTileIndexAt(vec2((x-1)*32, y*32)) != CBlockManager::CHEST
					&& m_pCollision->GetMineTeeTileIndexAt(vec2((x+1)*32, y*32)) != CBlockManager::CHEST
					&& m_pCollision->GetMineTeeTileIndexAt(vec2(x*32, (y-1)*32)) != CBlockManager::CHEST
					&& m_pCollision->GetMineTeeTileIndexAt(vec2(x*32, (y+1)*32)) != CBlockManager::CHEST)
			{
				ModifTile(ivec2(x, y), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::CHEST);
			}
		}
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

		ModifTile(TilePos, m_pLayers->GetMineTeeBGLayerIndex(), gs_MapStructures[StructureID][0][i]);
		ModifTile(TilePos, m_pLayers->GetMineTeeLayerIndex(), gs_MapStructures[StructureID][1][i]);
		ModifTile(TilePos, m_pLayers->GetMineTeeFGLayerIndex(), gs_MapStructures[StructureID][2][i]);
	}
}

void CMapGen::DoFallSteps()
{
	for(int TilePosX = 0; TilePosX < m_pLayers->MineTeeLayer()->m_Width; TilePosX++)
	{
		for(int TilePosY = m_pLayers->MineTeeLayer()->m_Height-1; TilePosY >= 0; TilePosY--)
		{
			const int TileIndexCenter = m_pCollision->GetMineTeeTileIndexAt(vec2(TilePosX*32, TilePosY*32));
			const int TileIndexBottom = m_pCollision->GetMineTeeTileIndexAt(vec2(TilePosX*32, (TilePosY+1)*32));
			const CBlockManager::CBlockInfo *pBlockInfo = m_pBlockManager->GetBlockInfo(TileIndexCenter);

			if (pBlockInfo->m_Gravity && (TileIndexBottom == 0 || m_pBlockManager->IsFluid(TileIndexBottom)))
			{
				int e=TilePosY+1;
				for (; e<m_pLayers->MineTeeLayer()->m_Height; e++)
				{
					const int TileIndex = m_pCollision->GetMineTeeTileIndexAt(vec2(TilePosX*32, e*32));
					if (TileIndex != 0 && !m_pBlockManager->IsFluid(TileIndex))
					{
						--e;
						break;
					}
				}

				if (e != TilePosY)
				{
					ModifTile(ivec2(TilePosX, e), m_pLayers->GetMineTeeLayerIndex(), TileIndexCenter);
					ModifTile(ivec2(TilePosX, TilePosY), m_pLayers->GetMineTeeLayerIndex(), 0);
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
				const int TileIndexCenter = m_pCollision->GetMineTeeTileIndexAt(vec2(TilePosX*32, TilePosY*32));
				int FluidType = 0;
				if (!m_pBlockManager->IsFluid(TileIndexCenter, &FluidType))
					continue;

				const int TileIndexBottom = m_pCollision->GetMineTeeTileIndexAt(vec2(TilePosX*32, (TilePosY+1)*32));
				const int TileIndexRight = m_pCollision->GetMineTeeTileIndexAt(vec2((TilePosX+1)*32, TilePosY*32));
				const int TileIndexRightTop = m_pCollision->GetMineTeeTileIndexAt(vec2((TilePosX+1)*32, (TilePosY-1)*32));
				const int TileIndexRightBottom = m_pCollision->GetMineTeeTileIndexAt(vec2((TilePosX+1)*32, (TilePosY+1)*32));
				const int TileIndexTop = m_pCollision->GetMineTeeTileIndexAt(vec2(TilePosX*32, (TilePosY-1)*32));
				const int TileIndexLeft = m_pCollision->GetMineTeeTileIndexAt(vec2((TilePosX-1)*32, TilePosY*32));
				const int TileIndexLeftTop = m_pCollision->GetMineTeeTileIndexAt(vec2((TilePosX-1)*32, (TilePosY-1)*32));
				const int TileIndexLeftBottom = m_pCollision->GetMineTeeTileIndexAt(vec2((TilePosX-1)*32, (TilePosY+1)*32));
				const CBlockManager::CBlockInfo *pBlockBottomInfo = m_pBlockManager->GetBlockInfo(TileIndexBottom);

				// Fall
				if (TileIndexBottom == 0 || (pBlockBottomInfo && !pBlockBottomInfo->m_PlayerCollide) ||
					(TileIndexBottom >= CBlockManager::WATER_A && TileIndexBottom < CBlockManager::WATER_C) ||
					(TileIndexBottom >= CBlockManager::LAVA_A && TileIndexBottom < CBlockManager::LAVA_C))
				{
					int TileIndexTemp = (FluidType==CBlockManager::FLUID_WATER)?CBlockManager::WATER_C:CBlockManager::LAVA_C;
					ModifTile(ivec2(TilePosX, TilePosY+1), m_pLayers->GetMineTeeLayerIndex(), TileIndexTemp);
				}
				// Create Obsidian?
				else if ((FluidType == CBlockManager::FLUID_LAVA && TileIndexBottom >= CBlockManager::WATER_A && TileIndexBottom <= CBlockManager::WATER_D) ||
						 (FluidType == CBlockManager::FLUID_WATER && TileIndexBottom >= CBlockManager::LAVA_A && TileIndexBottom <= CBlockManager::LAVA_D))
				{
					ModifTile(ivec2(TilePosX, TilePosY+1), m_pLayers->GetMineTeeLayerIndex(), CBlockManager::OBSIDIAN_A);
				}

				// Spread To Right
				if (TileIndexRight == 0 && TileIndexBottom != 0 &&
					!m_pBlockManager->IsFluid(TileIndexBottom) &&
					!m_pBlockManager->IsFluid(TileIndexRightBottom) &&
					!m_pBlockManager->IsFluid(TileIndexRightTop) &&
					TileIndexCenter-1 != CBlockManager::WATER_A-1 && TileIndexCenter-1 != CBlockManager::LAVA_A-1)
				{
					ModifTile(ivec2(TilePosX+1, TilePosY), m_pLayers->GetMineTeeLayerIndex(), TileIndexCenter-1);
				}
				// Spread To Left
				if (TileIndexLeft == 0 && TileIndexBottom != 0 &&
					!m_pBlockManager->IsFluid(TileIndexBottom) &&
					!m_pBlockManager->IsFluid(TileIndexLeftBottom) &&
					!m_pBlockManager->IsFluid(TileIndexLeftTop) &&
					TileIndexCenter-1 != CBlockManager::WATER_A-1 && TileIndexCenter-1 != CBlockManager::LAVA_A-1)
				{
					ModifTile(ivec2(TilePosX-1, TilePosY), m_pLayers->GetMineTeeLayerIndex(), TileIndexCenter-1);
				}

				// Check for correct tiles FIXME
				int FluidTypeTop = 0;
				bool IsFluidTop = m_pBlockManager->IsFluid(TileIndexTop, &FluidTypeTop);
				if (IsFluidTop && TileIndexCenter < CBlockManager::WATER_C)
				{
					ModifTile(ivec2(TilePosX, TilePosY), m_pLayers->GetMineTeeLayerIndex(), FluidTypeTop==CBlockManager::FLUID_WATER?CBlockManager::WATER_C:CBlockManager::LAVA_C);
				}
			}
		}
	}
}

inline void CMapGen::ModifTile(ivec2 Pos, int Layer, int BlockId)
{
	CBlockManager::CBlockInfo *pBlockInfo = m_pCollision->GetBlockManager()->GetBlockInfo(BlockId);
	if (pBlockInfo)
		m_pCollision->ModifTile(Pos, m_pLayers->GetMineTeeGroupIndex(), Layer, BlockId, 0, pBlockInfo->m_Health);
}
