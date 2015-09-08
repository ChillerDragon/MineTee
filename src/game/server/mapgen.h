#ifndef GAME_MAPGEN_H
#define GAME_MAPGEN_H

#include <engine/shared/noise.h>
#include <game/layers.h>
#include <game/collision.h>

enum
{
	DIRT_LEVEL = 60,
	STONE_LEVEL = 90,

	LEVEL_TOLERANCE = 10
};

class CMapGen
{
	class CLayers *m_pLayers;
	CCollision *m_pCollision;
	class CPerlinOctave *m_pNoise;

	void GenerateBasicTerrain();
	void GenerateCaves();
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
