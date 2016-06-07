/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* File modified by Alexandre Díaz */
#ifndef GAME_COLLISION_H
#define GAME_COLLISION_H

#include <base/vmath.h>
#include <base/math.h>

class CCollision
{
	class CTile *m_pTiles;
	class CTile *m_pMineTeeTiles; // MineTee
	int m_Width;
	int m_Height;
	class CLayers *m_pLayers;
	class CBlockManager *m_pBlockManager; // MineTee

	bool IsTileSolid(int x, int y, bool nocoll = true); // MineTee
	int GetTile(int x, int y);

public:
	enum
	{
		COLFLAG_SOLID=1,
		COLFLAG_DEATH=2,
		COLFLAG_NOHOOK=4,

		// MineTee
		FLUID_WATER=1,
		FLUID_LAVA
	};

	CCollision();
	void Init(class CLayers *pLayers, class CBlockManager *pBlockManager); // MineTee
	bool CheckPoint(float x, float y, bool nocoll = true) { return IsTileSolid(int_round(x), int_round(y), nocoll); } // MineTee
	bool CheckPoint(vec2 Pos, bool nocoll = true) { return CheckPoint(Pos.x, Pos.y, nocoll); } // MineTee
	int GetCollisionAt(float x, float y) { return GetTile(int_round(x), int_round(y)); }
	int GetWidth() { return m_Width; };
	int GetHeight() { return m_Height; };
	int IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision, bool nocoll = true); // MineTee
	void MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces);
	void MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity);
	bool TestBox(vec2 Pos, vec2 Size);

	// MineTee
	bool ModifTile(ivec2 pos, int group, int layer, int index, int flags);
	void RegenerateSkip(CTile *pTiles, int Width, int Height, ivec2 Pos, bool Delete);
	int GetMineTeeTileAt(vec2 Pos);
	CBlockManager *GetBlockManager() const { return m_pBlockManager; }
	void UpdateLayerLights(float ScreenX0, float ScreenY0, float ScreenX1, float ScreenY1, int DarknessLevel); // TODO: Perhaps not the best place...
	//
};

#endif
