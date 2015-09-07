#ifndef GAME_MAPGEN_H
#define GAME_MAPGEN_H

#include <base/vmath.h>
#include <game/server/gameworld.h>

enum
{
	DIRT_LEVEL = 75,
	STONE_LEVEL = 90,

	LEVEL_TOLERANCE = 10
};

class CMapGen
{
	class CGameWorld *m_pGameWorld;
	class CGameContext *GameServer() { return m_pGameWorld->GameServer(); }

	void GenerateBasicTerrain();
	void GenerateTrees();

public:
	CMapGen(CGameWorld *pGameWorld);

	void GenerateMap();
};

#endif
