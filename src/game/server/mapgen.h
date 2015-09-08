#ifndef GAME_MAPGEN_H
#define GAME_MAPGEN_H

#include <engine/shared/noise.h>
#include <game/layers.h>
#include <game/collision.h>

enum
{
	DIRT_LEVEL = 60,
	STONE_LEVEL = 90,

	WATER_LEVEL_MAX = 75,
	WATER_LEVEL_MIN = 90,

	COAL_LEVEL = 105,
	IRON_LEVEL = 120,
	GOLD_LEVEL = 145,
	DIAMOND_LEVEL = 160,

	LEVEL_TOLERANCE = 10
};

class CMapGen
{
	class CLayers *m_pLayers;
	CCollision *m_pCollision;
	class CPerlinOctave *m_pNoise;

	void GenerateBasicTerrain();
	void GenerateOre(int Type, float F, int Level, int Radius);
	void GenerateCaves();
	void GenerateWater();
	void GenerateFlowers();
	void GenerateMushrooms();
	void GenerateTrees();

	void GenerateBorder();

public:
	CMapGen();
	~CMapGen();

	void GenerateMap();
	void Init(CLayers *pLayers, CCollision *pCollision);
};

#endif
