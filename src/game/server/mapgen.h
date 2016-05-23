#ifndef GAME_MAPGEN_H
#define GAME_MAPGEN_H

#include <engine/shared/noise.h>
#include <game/layers.h>
#include <game/collision.h>

enum
{
	SNOW_LEVEL = 30,
	DIRT_LEVEL = 40,
	STONE_LEVEL = 90,
	LAVA_LEVEL = 110,
	TUNNEL_LEVEL = 40,

	WATER_LEVEL_MAX = 75,
	WATER_LEVEL_MIN = 90,

	COAL_LEVEL = 115,
	IRON_LEVEL = 125,
	GOLD_LEVEL = 140,
	DIAMOND_LEVEL = 160,

	LEVEL_TOLERANCE = 10
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
	class CPerlinOctave *m_pNoise;

	void GenerateBasicTerrain();
	void GenerateOre(int Type, float F, int Level, int Radius, int ClusterSize);
	void GenerateCaves(int FillBlock);
	void GenerateWater();
	void GenerateFlowers();
	void GenerateMushrooms();
	void GenerateTrees();
	void GenerateTunnels();

	void GenerateBorder();

	void GenerateSkip();

public:
	CMapGen();
	~CMapGen();

	void GenerateMap();
	void Init(CLayers *pLayers, CCollision *pCollision);
};

#endif
