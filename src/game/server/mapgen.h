#ifndef GAME_MAPGEN_H
#define GAME_MAPGEN_H

#include <engine/shared/noise.h>
#include <game/layers.h>
#include <game/collision.h>

enum
{
	SNOW_LEVEL = 0,
	DIRT_LEVEL = 20,
	STONE_LEVEL = 40,
	LAVA_LEVEL = 70,
	TUNNEL_LEVEL = 30,

	BIOME_LEVEL = 10,

	WATER_LEVEL = 25,

	COAL_LEVEL = 60,
	IRON_LEVEL = 70,
	GOLD_LEVEL = 80,
	DIAMOND_LEVEL = 90,

	LEVEL_TOLERANCE = 10,
	BOSS_LEVEL = 75
};

class CMapGenBiome
{
public:
	CMapGenBiome(int type, float freq, float amp, int baseTile, int groundTile)
	: m_Freq(freq),
	  m_Amp(amp),
	  m_BaseTile(baseTile),
	  m_GroundTile(groundTile),
	  m_TreeType(type)
	{ }

	const float m_Freq;
	const float m_Amp;
	const int m_BaseTile;
	const int m_GroundTile;
	const int m_TreeType;
};

class CMapGen
{
	class CLayers *m_pLayers;
	CCollision *m_pCollision;
	CBlockManager *m_pBlockManager;
	class CPerlinOctave *m_pNoise;

	void GenerateBasicTerrain();
	void GenerateOre(int Type, float F, int Level, int Radius, int ClusterSize);
	void GenerateCaves(int FillBlock);
	void GenerateWater();
	void GenerateFlowers();
	void GenerateMushrooms();
	void GenerateTrees();
	void GenerateTunnels(int Num);
	void GenerateBorder();
	void GenerateSkip();
	void GenerateBossZones();
	void GenerateBiomes(int FillBlock);

	void DoFallSteps();
	void DoWaterSteps();

	void CreateStructure(int StructureID, ivec2 Pos);

public:
	CMapGen();
	~CMapGen();

	void GenerateTree(ivec2 Pos);

	void FillMap(int Seed);
	void Init(CLayers *pLayers, CCollision *pCollision, CBlockManager *pBlockManager);
};

inline int PercOf(int Prc, int Total) { return Prc*Total*0.01f; }

#endif
