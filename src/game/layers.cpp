/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "layers.h"
#include <game/gamecore.h> // MineTee

CLayers::CLayers()
{
	m_GroupsNum = 0;
	m_GroupsStart = 0;
	m_LayersNum = 0;
	m_LayersStart = 0;
	m_pGameGroup = 0;
	m_pGameLayer = 0;
	m_pMap = 0;

	// MineTee
	m_MineTeeGroupIndex = 0;
	m_MineTeeLayerIndex = 0;
	m_pMineTeeLayer = 0;
	m_pMineTeeBGLayer = 0;
	m_pMineTeeFGLayer = 0;
	m_pMineTeeLights = 0;
	m_pMineTeeLightsTiles = 0;
}

void CLayers::Init(class IKernel *pKernel)
{
	// MineTee
	m_MineTeeGroupIndex = 0;
	m_MineTeeLayerIndex = 0;
	m_pMineTeeLayer = 0;
	m_pMineTeeBGLayer = 0;
	m_pMineTeeFGLayer = 0;
	m_pMineTeeLights = 0;
	m_pMineTeeLightsTiles = 0;
	m_MineTeeBGLayerIndex = 0;
	m_MineTeeFGLayerIndex = 0;
	//

	m_pMap = pKernel->RequestInterface<IMap>();
	m_pMap->GetType(MAPITEMTYPE_GROUP, &m_GroupsStart, &m_GroupsNum);
	m_pMap->GetType(MAPITEMTYPE_LAYER, &m_LayersStart, &m_LayersNum);

	for(int g = 0; g < NumGroups(); g++)
	{
		CMapItemGroup *pGroup = GetGroup(g);
		for(int l = 0; l < pGroup->m_NumLayers; l++)
		{
			CMapItemLayer *pLayer = GetLayer(pGroup->m_StartLayer+l);

			if(pLayer->m_Type == LAYERTYPE_TILES)
			{
				CMapItemLayerTilemap *pTilemap = reinterpret_cast<CMapItemLayerTilemap *>(pLayer);

				// MineTee
                char layerName[64]={0};
                IntsToStr(pTilemap->m_aName, sizeof(pTilemap->m_aName)/sizeof(int), layerName);

				if (!m_pMineTeeLayer && str_comp_nocase(layerName, "mt-break") == 0)
				{
					m_MineTeeGroupIndex = g;
					m_MineTeeLayerIndex = l;
				    m_pMineTeeLayer = pTilemap;
				}
                else if(!m_pMineTeeLights && str_comp_nocase(layerName, "mt-light") == 0)
                {
                    m_pMineTeeLights = pTilemap;
                    if (m_pMineTeeLights)
                    {
                        CTile *pLTiles = (CTile *)m_pMap->GetData(m_pMineTeeLights->m_Data);
                        m_pMineTeeLightsTiles = pLTiles;
                    }
                }
                else if (!m_pMineTeeBGLayer && str_comp_nocase(layerName, "mt-bg") == 0)
                {
                	m_MineTeeBGLayerIndex = l;
                    m_pMineTeeBGLayer = pTilemap;
                }
                else if (!m_pMineTeeFGLayer && str_comp_nocase(layerName, "mt-fg") == 0)
                {
                	m_MineTeeFGLayerIndex = l;
                    m_pMineTeeFGLayer = pTilemap;
                }
				//

				if(pTilemap->m_Flags&TILESLAYERFLAG_GAME)
				{
					m_pGameLayer = pTilemap;
					m_pGameGroup = pGroup;

					// make sure the game group has standard settings
					m_pGameGroup->m_OffsetX = 0;
					m_pGameGroup->m_OffsetY = 0;
					m_pGameGroup->m_ParallaxX = 100;
					m_pGameGroup->m_ParallaxY = 100;

					if(m_pGameGroup->m_Version >= 2)
					{
						m_pGameGroup->m_UseClipping = 0;
						m_pGameGroup->m_ClipX = 0;
						m_pGameGroup->m_ClipY = 0;
						m_pGameGroup->m_ClipW = 0;
						m_pGameGroup->m_ClipH = 0;
					}

					//break;
				}
			}
		}
	}
}

CMapItemGroup *CLayers::GetGroup(int Index) const
{
	return static_cast<CMapItemGroup *>(m_pMap->GetItem(m_GroupsStart+Index, 0, 0));
}

CMapItemLayer *CLayers::GetLayer(int Index) const
{
	return static_cast<CMapItemLayer *>(m_pMap->GetItem(m_LayersStart+Index, 0, 0));
}
