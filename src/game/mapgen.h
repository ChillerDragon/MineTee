#ifndef GAME_MAPGEN_H
#define GAME_MAPGEN_H

#include <base/vmath.h>
#include <game/server/gameworld.h>

class CMapGen
{
	class CGameWorld *m_pGameWorld;
	class CGameContext *GameServer() { return m_pGameWorld->GameServer(); }
public:
	CMapGen(CGameWorld *pGameWorld);

	void GenerateMap();
};

#endif
