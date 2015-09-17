/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <math.h>
#include <engine/map.h>
#include <engine/kernel.h>

#include <game/mapitems.h>
#include <game/layers.h>
#include <game/collision.h>

#include <game/generated/protocol.h> // MineTee
#include <game/block_manager.h> // MineTee

CCollision::CCollision()
{
	m_pTiles = 0;
	m_Width = 0;
	m_Height = 0;
	m_pLayers = 0;

	m_pMineTeeTiles = 0x0; // MineTee
}

void CCollision::Init(class CLayers *pLayers, class CBlockManager *pBlockManager)
{
	m_pLayers = pLayers;
	m_pBlockManager = pBlockManager;
	m_Width = m_pLayers->GameLayer()->m_Width;
	m_Height = m_pLayers->GameLayer()->m_Height;
	m_pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->GameLayer()->m_Data));

	// MineTee
	int MineTeeLayerSize = 0;
	if (m_pLayers->MineTeeLayer())
	{
        m_pMineTeeTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(m_pLayers->MineTeeLayer()->m_Data));
        MineTeeLayerSize = m_pLayers->MineTeeLayer()->m_Width*m_pLayers->MineTeeLayer()->m_Height;
	}
	else
		m_pMineTeeTiles = 0x0;
	//

	for(int i = 0; i < m_Width*m_Height; i++)
	{
        // MineTee
        if (m_pMineTeeTiles && i < MineTeeLayerSize)
        {
            int MIndex = m_pMineTeeTiles[i].m_Index;
            if (MIndex == CBlockManager::BEDROCK)
                m_pTiles[i].m_Index = TILE_NOHOOK;
        }
        //

        int Index = m_pTiles[i].m_Index;
		if(Index > 128)
			continue;

		switch(Index)
		{
		case TILE_DEATH:
			m_pTiles[i].m_Index = COLFLAG_DEATH;
			break;
		case TILE_SOLID:
			m_pTiles[i].m_Index = COLFLAG_SOLID;
			break;
		case TILE_NOHOOK:
			m_pTiles[i].m_Index = COLFLAG_SOLID|COLFLAG_NOHOOK;
			break;
		default:
			m_pTiles[i].m_Index = 0;
		}
	}
}

int CCollision::GetTile(int x, int y)
{
	int Nx = clamp(x/32, 0, m_Width-1);
	int Ny = clamp(y/32, 0, m_Height-1);

	return m_pTiles[Ny*m_Width+Nx].m_Index > 128 ? 0 : m_pTiles[Ny*m_Width+Nx].m_Index;
}

bool CCollision::IsTileSolid(int x, int y, bool nocoll)
{
    // MineTee
    if (m_pMineTeeTiles)
    {
    	int Nx = clamp(x/32, 0, m_Width-1);
    	int Ny = clamp(y/32, 0, m_Height-1);

    	int Index = Nx + Ny*m_Width;
    	int TileIndex = m_pMineTeeTiles[Index].m_Index;
		CBlockManager::CBlockInfo BlockInfo;
		m_pBlockManager->GetBlockInfo(TileIndex, &BlockInfo);

    	if (Ny > 0)
    	{
    		int TIndex = Nx + (Ny-1)*m_Width;
			int TileIndexTop =  m_pMineTeeTiles[TIndex].m_Index;
			if (BlockInfo.m_HalfTile)
			{
				Nx *= 32; Ny *= 32;

				if ((!TileIndexTop && y >= Ny+16.0f) || (TileIndexTop && y <= Ny+16.0f))
					return 1;

				return 0;
			}
    	}
        if (m_pBlockManager->IsFluid(TileIndex))
        	return 0;
        else if (nocoll && !BlockInfo.m_PlayerCollide)
            return 0;
    }

	return GetTile(x, y)&COLFLAG_SOLID;
}

// TODO: rewrite this smarter!
int CCollision::IntersectLine(vec2 Pos0, vec2 Pos1, vec2 *pOutCollision, vec2 *pOutBeforeCollision, bool nocoll)
{
	float Distance = distance(Pos0, Pos1);
	int End(Distance+1);
	vec2 Last = Pos0;

	for(int i = 0; i < End; i++)
	{
		float a = i/Distance;
		vec2 Pos = mix(Pos0, Pos1, a);
		if(CheckPoint(Pos.x, Pos.y, nocoll))
		{
			if(pOutCollision)
				*pOutCollision = Pos;
			if(pOutBeforeCollision)
				*pOutBeforeCollision = Last;
			return GetCollisionAt(Pos.x, Pos.y);
		}
		Last = Pos;
	}
	if(pOutCollision)
		*pOutCollision = Pos1;
	if(pOutBeforeCollision)
		*pOutBeforeCollision = Pos1;
	return 0;
}

// TODO: OPT: rewrite this smarter!
void CCollision::MovePoint(vec2 *pInoutPos, vec2 *pInoutVel, float Elasticity, int *pBounces)
{
	if(pBounces)
		*pBounces = 0;

	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;
	if(CheckPoint(Pos + Vel))
	{
		int Affected = 0;
		if(CheckPoint(Pos.x + Vel.x, Pos.y))
		{
			pInoutVel->x *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(CheckPoint(Pos.x, Pos.y + Vel.y))
		{
			pInoutVel->y *= -Elasticity;
			if(pBounces)
				(*pBounces)++;
			Affected++;
		}

		if(Affected == 0)
		{
			pInoutVel->x *= -Elasticity;
			pInoutVel->y *= -Elasticity;
		}
	}
	else
	{
		*pInoutPos = Pos + Vel;
	}
}

bool CCollision::TestBox(vec2 Pos, vec2 Size)
{
	Size *= 0.5f;
	if(CheckPoint(Pos.x-Size.x, Pos.y-Size.y))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y-Size.y))
		return true;
	if(CheckPoint(Pos.x-Size.x, Pos.y+Size.y))
		return true;
	if(CheckPoint(Pos.x+Size.x, Pos.y+Size.y))
		return true;
	return false;
}

void CCollision::MoveBox(vec2 *pInoutPos, vec2 *pInoutVel, vec2 Size, float Elasticity)
{
	// do the move
	vec2 Pos = *pInoutPos;
	vec2 Vel = *pInoutVel;

	float Distance = length(Vel);
	int Max = (int)Distance;

	if(Distance > 0.00001f)
	{
		//vec2 old_pos = pos;
		float Fraction = 1.0f/(float)(Max+1);
		for(int i = 0; i <= Max; i++)
		{
			//float amount = i/(float)max;
			//if(max == 0)
				//amount = 0;

			vec2 NewPos = Pos + Vel*Fraction; // TODO: this row is not nice

			if(TestBox(vec2(NewPos.x, NewPos.y), Size))
			{
				int Hits = 0;

				if(TestBox(vec2(Pos.x, NewPos.y), Size))
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					Hits++;
				}

				if(TestBox(vec2(NewPos.x, Pos.y), Size))
				{
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
					Hits++;
				}

				// neither of the tests got a collision.
				// this is a real _corner case_!
				if(Hits == 0)
				{
					NewPos.y = Pos.y;
					Vel.y *= -Elasticity;
					NewPos.x = Pos.x;
					Vel.x *= -Elasticity;
				}
			}

			Pos = NewPos;
		}
	}

	*pInoutPos = Pos;
	*pInoutVel = Vel;
}


// MineTee
int CCollision::GetMineTeeTileAt(vec2 Pos)
{
    if (!m_pLayers->MineTeeLayer())
        return -1;

    Pos = vec2(static_cast<int>(Pos.x/32.0f), static_cast<int>(Pos.y/32.0f));
    if (Pos.x >= m_Width-1 || Pos.x <= 0 || Pos.y >= m_Height-1 || Pos.y <= 0)
        return -1;

    //MineTee Layer
    int MTIndex = static_cast<int>(Pos.y*m_pLayers->MineTeeLayer()->m_Width+Pos.x);
    CTile *pMTTiles = (CTile *)m_pLayers->Map()->GetData(m_pLayers->MineTeeLayer()->m_Data);
    return pMTTiles[MTIndex].m_Index;
}


void CCollision::ModifTile(vec2 pos, int group, int layer, int index, int flags)
{
    CMapItemGroup *pGroup = m_pLayers->GetGroup(group);
    CMapItemLayer *pLayer = m_pLayers->GetLayer(pGroup->m_StartLayer+layer);
    if (pLayer->m_Type != LAYERTYPE_TILES) // protect against the dark side people
        return;

    CMapItemLayerTilemap *pTilemap = reinterpret_cast<CMapItemLayerTilemap *>(pLayer);
    int tpos = (int)pos.y*pTilemap->m_Width+(int)pos.x;
    if (tpos < 0 || tpos >= pTilemap->m_Width*pTilemap->m_Height) // protect against the dark side people
        return;

    if (pTilemap == m_pLayers->MineTeeLayer())
    {
        CTile *pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(pTilemap->m_Data));
        pTiles[tpos].m_Flags = flags;
        pTiles[tpos].m_Index = index;

    	if (index == 0)
    	{
    		m_pTiles[tpos].m_Index = 0;
    		m_pTiles[tpos].m_Flags = 0;
    	}
    	else if (index == CBlockManager::BEDROCK)
    	{
            m_pTiles[tpos].m_Index = COLFLAG_SOLID|COLFLAG_NOHOOK;
            m_pTiles[tpos].m_Flags = 0;
    	}
    	else if (!m_pBlockManager->IsFluid(index))
    	{
    		m_pTiles[tpos].m_Index = COLFLAG_SOLID;
    		m_pTiles[tpos].m_Flags = 0;
    	}
    }
    else if (pTilemap != m_pLayers->GameLayer())
    {
        CTile *pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(pTilemap->m_Data));
        pTiles[tpos].m_Flags = flags;
        pTiles[tpos].m_Index = index;
    }
    else
    {
        m_pTiles[tpos].m_Index = index;
        m_pTiles[tpos].m_Flags = flags;

        switch(index)
        {
        case TILE_DEATH:
            m_pTiles[tpos].m_Index = COLFLAG_DEATH;
            break;
        case TILE_SOLID:
            m_pTiles[tpos].m_Index = COLFLAG_SOLID;
            break;
        case TILE_NOHOOK:
            m_pTiles[tpos].m_Index = COLFLAG_SOLID|COLFLAG_NOHOOK;
            break;
        default:
            if(index <= 128)
                m_pTiles[tpos].m_Index = 0;
        }
    }
}
