#ifndef GAME_MAPGEN_H
#define GAME_MAPGEN_H

#include <base/vmath.h>
#include <game/server/gameworld.h>

#include "noise.h"

enum
{
	DIRT_LEVEL = 60,
	STONE_LEVEL = 90,

	LEVEL_TOLERANCE = 10
};

class CMapGen
{
	class CGameWorld *m_pGameWorld;
	class CGameContext *GameServer() { return m_pGameWorld->GameServer(); }
	class PerlinOctave *m_pNoise;

	void GenerateBasicTerrain();
	void GenerateCaves();
	void GenerateTrees();

public:
	CMapGen(CGameWorld *pGameWorld);

	void GenerateMap();
};

#endif
