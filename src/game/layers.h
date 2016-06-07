/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* File modified by Alexandre Díaz */
#ifndef GAME_LAYERS_H
#define GAME_LAYERS_H

#include <engine/map.h>
#include <game/mapitems.h>

class CLayers
{
	int m_GroupsNum;
	int m_GroupsStart;
	int m_LayersNum;
	int m_LayersStart;
	CMapItemGroup *m_pGameGroup;
	CMapItemLayerTilemap *m_pGameLayer;
	class IMap *m_pMap;

	// MineTee
	int m_GameGroupIndex;
	int m_GameLayerIndex;
	int m_MineTeeGroupIndex;
	int m_MineTeeLayerIndex;
	int m_MineTeeBGLayerIndex;
	int m_MineTeeFGLayerIndex;
	CMapItemLayerTilemap *m_pMineTeeLayer;
	CMapItemLayerTilemap *m_pMineTeeBGLayer;
	CMapItemLayerTilemap *m_pMineTeeFGLayer;
	CMapItemLayerTilemap *m_pMineTeeLights;
	CTile *m_pMineTeeLightsTiles;
	//

public:
	CLayers();
	void Init(class IKernel *pKernel);
	int NumGroups() const { return m_GroupsNum; };
	class IMap *Map() const { return m_pMap; };
	CMapItemGroup *GameGroup() const { return m_pGameGroup; };
	CMapItemLayerTilemap *GameLayer() const { return m_pGameLayer; };
	CMapItemGroup *GetGroup(int Index) const;
	CMapItemLayer *GetLayer(int Index) const;

	// MineTee
	int GetGameGroupIndex() const { return m_GameGroupIndex; }
	int GetGameLayerIndex() const { return m_GameLayerIndex; }
	int GetMineTeeGroupIndex() const { return m_MineTeeGroupIndex; }
	int GetMineTeeLayerIndex() const { return m_MineTeeLayerIndex; }
	int GetMineTeeBGLayerIndex() const { return m_MineTeeBGLayerIndex; }
	int GetMineTeeFGLayerIndex() const { return m_MineTeeFGLayerIndex; }

    CMapItemLayerTilemap *Lights() const { return m_pMineTeeLights; };
    CTile *TileLights() const { return m_pMineTeeLightsTiles; };
    CMapItemLayerTilemap *MineTeeLayer() const { return m_pMineTeeLayer; };
    CMapItemLayerTilemap *MineTeeFGLayer() const { return m_pMineTeeFGLayer; };
    CMapItemLayerTilemap *MineTeeBGLayer() const { return m_pMineTeeBGLayer; };
    //
};

#endif
