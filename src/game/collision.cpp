/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* File modified by Alexandre Díaz */
#include <base/system.h>

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

	m_pBlockManager = 0x0; // MineTee
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

	const int MapSize = m_Width*m_Height;
	for(int i = 0; i < MapSize; i++)
	{
        // MineTee
        if (m_pMineTeeTiles && i < MineTeeLayerSize)
        {
        	const int MTIndex = m_pMineTeeTiles[i].m_Index;
            if (MTIndex == CBlockManager::BEDROCK)
                m_pTiles[i].m_Index = TILE_NOHOOK;
            CBlockManager::CBlockInfo *pBlockInfo = m_pBlockManager->GetBlockInfo(MTIndex);
            if (pBlockInfo)
            	m_pMineTeeTiles[i].m_Reserved = pBlockInfo->m_Health;
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
		CBlockManager::CBlockInfo *pBlockInfo = m_pBlockManager->GetBlockInfo(TileIndex);

    	if (Ny > 0)
    	{
    		int TIndex = Nx + (Ny-1)*m_Width;
			int TileIndexTop =  m_pMineTeeTiles[TIndex].m_Index;
			if (pBlockInfo->m_HalfTile)
			{
				Nx *= 32; Ny *= 32;

				if ((!TileIndexTop && y >= Ny+16.0f) || (TileIndexTop && y <= Ny+16.0f))
					return 1;

				return 0;
			}
    	}
        if (m_pBlockManager->IsFluid(TileIndex))
        	return 0;
        else if (nocoll && !pBlockInfo->m_PlayerCollide)
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
		float a = i/float(End);
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
int CCollision::GetMineTeeTileIndexAt(vec2 Pos)
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

CTile* CCollision::GetMineTeeTileAt(vec2 Pos)
{
    if (!m_pLayers->MineTeeLayer())
        return nullptr;

    Pos = vec2(static_cast<int>(Pos.x/32.0f), static_cast<int>(Pos.y/32.0f));
    if (Pos.x >= m_Width-1 || Pos.x <= 0 || Pos.y >= m_Height-1 || Pos.y <= 0)
        return nullptr;

    //MineTee Layer
    int MTIndex = static_cast<int>(Pos.y*m_pLayers->MineTeeLayer()->m_Width+Pos.x);
    CTile *pMTTiles = (CTile *)m_pLayers->Map()->GetData(m_pLayers->MineTeeLayer()->m_Data);
    return &pMTTiles[MTIndex];
}


bool CCollision::ModifTile(ivec2 pos, int group, int layer, int index, int flags, int reserved)
{
    CMapItemGroup *pGroup = m_pLayers->GetGroup(group);
    CMapItemLayer *pLayer = m_pLayers->GetLayer(pGroup->m_StartLayer+layer);
    if (pLayer->m_Type != LAYERTYPE_TILES || pos.y <= 1)
        return false;

    CMapItemLayerTilemap *pTilemap = reinterpret_cast<CMapItemLayerTilemap *>(pLayer);
    int TotalTiles = pTilemap->m_Width*pTilemap->m_Height;
    int tpos = (int)pos.y*pTilemap->m_Width+(int)pos.x;
    if (tpos < 0 || tpos >= TotalTiles)
        return false;

    if (pTilemap == m_pLayers->MineTeeLayer())
    {
        CTile *pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(pTilemap->m_Data));
        pTiles[tpos].m_Flags = flags;
        pTiles[tpos].m_Index = index;
        pTiles[tpos].m_Reserved = reserved;

        RegenerateSkip(pTiles, pTilemap->m_Width, pTilemap->m_Height, pos, !index);

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
        pTiles[tpos].m_Reserved = 1;

        RegenerateSkip(pTiles, pTilemap->m_Width, pTilemap->m_Height, pos, !index);
    }
    else
    {
        m_pTiles[tpos].m_Index = index;
        m_pTiles[tpos].m_Flags = flags;
        m_pTiles[tpos].m_Reserved = 1;

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

    return true;
}

void CCollision::RegenerateSkip(CTile *pTiles, int Width, int Height, ivec2 Pos, bool Delete)
{
	if (!pTiles || Pos.x < 0 || Pos.x >= Width || Pos.y < 0 || Pos.y >= Height)
		return;


	int sx, i;

	if (Delete)
	{
		// Back Tile
		sx = 1;
		for (i=Pos.x+1; i<Width; i++)
		{
			if (!pTiles[Pos.y*Width+i].m_Index)
				sx++;
			else
				break;
		}
		for (i=Pos.x-1; i>=0; i--)
		{
			if (!pTiles[Pos.y*Width+i].m_Index)
				sx++;
			else
				break;
		}
		i = max(0, i);
		pTiles[Pos.y*Width+i].m_Skip = sx;
		// Current Tile
		pTiles[Pos.y*Width+Pos.x].m_Skip = 0;
	}
	else
	{
		// Back Tile
		for (i=Pos.x-1, sx=0; i>=0; i--)
		{
			if (!pTiles[Pos.y*Width+i].m_Index)
				sx++;
			else
				break;
		}
		i = max(0, i);
		pTiles[Pos.y*Width+i].m_Skip = sx;
		// Current Tile
		for (i=Pos.x+1, sx=0; i<Width; i++)
		{
			if (!pTiles[Pos.y*Width+i].m_Index)
				sx++;
			else
				break;
		}
		pTiles[Pos.y*Width+Pos.x].m_Skip = sx;
	}
}

bool CCollision::IsBlockNear(int BlockID, ivec2 MapPos, int Radius)
{
	CMapItemLayerTilemap *pTilemap = m_pLayers->MineTeeLayer();
	CTile *pTiles = static_cast<CTile *>(m_pLayers->Map()->GetData(pTilemap->m_Data));

	ivec2 InitPos = MapPos-ivec2(Radius, Radius);
	const int Diam = Radius*2;
	for (int x=InitPos.x; x<InitPos.x+Diam; x++)
	{
		for (int y=InitPos.y; y<InitPos.y+Diam; y++)
		{
			const int Index = y*m_pLayers->MineTeeLayer()->m_Width+x;
			if (pTiles[Index].m_Index == BlockID)
				return true;
		}
	}
	return false;
}

// FIXME: 6-phase seems very ugly and slow... :(
void CCollision::UpdateLayerLights(float ScreenX0, float ScreenY0, float ScreenX1, float ScreenY1, int DarknessLevel)
{
	CTile *pTiles = 0x0;
    CMapItemLayerTilemap *pMTMap = m_pLayers->MineTeeLayer();
    if (pMTMap)
        pTiles = (CTile *)m_pLayers->Map()->GetData(pMTMap->m_Data);

    CTile *pLights = m_pLayers->TileLights();

	if (!pTiles || !pLights)
		return;

	const int w = m_pLayers->Lights()->m_Width;
	const int h = m_pLayers->Lights()->m_Height;

	const int LayerSize = w*h*sizeof(CTile);
    CTile *pLightsTemp = static_cast<CTile*>(mem_alloc(LayerSize, 1));
    mem_zero(pLightsTemp, LayerSize);

    const int aTileIndexDarkness[] = {0, 154, 170, 186, 202};
    const float Scale = 32.0f;

	int StartY = (int)(ScreenY0/Scale)-1;
	int StartX = (int)(ScreenX0/Scale)-1;
	int EndY = (int)(ScreenY1/Scale)+1;
	int EndX = (int)(ScreenX1/Scale)+1;

    StartX = clamp(StartX-25, 0, w);
    StartY = clamp(StartY-25, 0, h);
    EndX = clamp(EndX+25, 0, w);
    EndY = clamp(EndY+25, 0, h);

    // Fill base shadow
	for(int y = StartY; y < EndY; y++)
		for(int x = StartX; x < EndX; x++)
		{
            int c = x + y * w;

            pLightsTemp[c].m_Index = aTileIndexDarkness[DarknessLevel];
            pLightsTemp[c].m_Reserved = 0;
            pLightsTemp[c].m_Skip = 0;
        }

	// Environment Light (The Sun)
	if (DarknessLevel < 4)
	{
		// Hard Shadows
		for(int y = StartY; y < EndY; y++)
			for(int x = StartX; x < EndX; x++)
			{
				int c = x + y*w;
				if (CheckPoint(x*Scale, y*Scale, true) || GetBlockManager()->IsFluid(pTiles[c].m_Index))
				{
					int tmy = y+1;
					int tc = x + tmy*w;
					while (!CheckPoint(x*Scale, tmy*Scale, true) && tmy < h)
					{
						pLightsTemp[tc].m_Index = aTileIndexDarkness[4];
						++tmy;
						tc = x + tmy*w;
					}

					if (tmy < h)
						pLightsTemp[tc].m_Index = aTileIndexDarkness[4];
				}
			}


		// Ground Shadows
		for(int y = StartY; y < EndY; y++)
			for(int x = StartX; x < EndX; x++)
			{
				int c = x + y*w;

				if (pTiles[c].m_Index != 0)
				{
					bool light = false;
					ivec2 Positions[] = { ivec2(-1,-1), ivec2(1,-1), ivec2(1,1), ivec2(-1,1), ivec2(-1,0), ivec2(1,0), ivec2(0,-1), ivec2(0,1) };
					bool PositionsTest[] = { false, false, false, false, false, false, false, false };

					// Search where is the light in relation to the current tile
					for (size_t o=0; o<8; o++)
					{
						int tc = (x+Positions[o].x) + (y+Positions[o].y)*w;
						if (tc >= 0 && tc < w*h && pLightsTemp[tc].m_Index == 0 && pTiles[tc].m_Index == 0)
							PositionsTest[o] = light = true;
					}

					if (light) // Light founded?
					{
						pLightsTemp[c].m_Index = 0;

						// Do Diffuse..
						for (int i=1;i<=4;i++)
						{
							// Don't put shadows in the tiles affected directly by the light.. move in..
							// By default assumed up-down
							int tc = x + (y+i)*w;
							if (PositionsTest[4]) // left-right
								tc = (x+i) + y*w;
							else if (PositionsTest[5]) // right-left
								tc = (x-i) + y*w;

							// Check for correct shadow
							if (tc < 0 || tc >= w*h || pTiles[tc].m_Index == 0 || aTileIndexDarkness[i] > pLightsTemp[tc].m_Index)
								continue;

							pLightsTemp[tc].m_Index = aTileIndexDarkness[i];
						}
					}
				}
			}

		if (DarknessLevel == 0)
		{
			// Gloom (Right-Left)
			for(int y = StartY; y < EndY; y++)
				for(int x = StartX; x < EndX; x++)
				{
					int c = x + y*w;

					if (pLightsTemp[c].m_Index != 0)
					{
						int tcs[2] = { (x-1)+y*w, x+(y-1)*w };
						for (int e=0; e<2; e++)
						{
							if (tcs[e] < 0 || tcs[e] >= w*h)
								continue;

							if (pLightsTemp[tcs[e]].m_Index == aTileIndexDarkness[0] && pTiles[c].m_Index == 0 && pTiles[tcs[e]].m_Index == 0)
							{
								pLightsTemp[c].m_Reserved = 1;
								pLightsTemp[c].m_Index = aTileIndexDarkness[1];
								break;
							}
							else if (pLightsTemp[tcs[e]].m_Index == aTileIndexDarkness[1] && pTiles[tcs[e]].m_Index == 0)
							{
								pLightsTemp[c].m_Reserved = 1;
								pLightsTemp[c].m_Index = aTileIndexDarkness[2];
								break;
							}
							else if (pLightsTemp[tcs[e]].m_Index == aTileIndexDarkness[2] && pTiles[tcs[e]].m_Index == 0)
							{
								pLightsTemp[c].m_Reserved = 1;
								pLightsTemp[c].m_Index = aTileIndexDarkness[3];
								break;
							}
							else if (pLightsTemp[tcs[e]].m_Index == aTileIndexDarkness[3] && pTiles[tcs[e]].m_Index == 0)
							{
								pLightsTemp[c].m_Reserved = 1;
								pLightsTemp[c].m_Index = aTileIndexDarkness[4];
								break;
							}
						}
					}
				}

			// Gloom (Left-Right)
			for(int y = StartY; y < EndY; y++)
				for(int x = EndX-1; x >= StartX; x--)
				{
					int c = x + y*w;

					if (pLightsTemp[c].m_Index != 0)
					{
						int tc = (x+1)+y*w;
						if (tc >= 0 && tc < w*h && pLightsTemp[tc].m_Index == aTileIndexDarkness[0] && pTiles[c].m_Index == 0 && pTiles[tc].m_Index == 0 && (pLightsTemp[c].m_Reserved != 1 || (aTileIndexDarkness[1] < pLightsTemp[c].m_Index && pLightsTemp[c].m_Reserved == 1)))
						{
							pLightsTemp[c].m_Reserved = 1;
							pLightsTemp[c].m_Index = aTileIndexDarkness[1];
						}
						else if (tc >= 0 && tc < w*h && pLightsTemp[tc].m_Index == aTileIndexDarkness[1] && pTiles[tc].m_Index == 0 && (pLightsTemp[c].m_Reserved != 1 || (aTileIndexDarkness[2] < pLightsTemp[c].m_Index && pLightsTemp[c].m_Reserved == 1)))
						{
							pLightsTemp[c].m_Reserved = 1;
							pLightsTemp[c].m_Index = aTileIndexDarkness[2];
						}
						else if (tc >= 0 && tc < w*h && pLightsTemp[tc].m_Index == aTileIndexDarkness[2] && pTiles[tc].m_Index == 0 && (pLightsTemp[c].m_Reserved != 1 || (aTileIndexDarkness[3] < pLightsTemp[c].m_Index && pLightsTemp[c].m_Reserved == 1)))
						{
							pLightsTemp[c].m_Reserved = 1;
							pLightsTemp[c].m_Index = aTileIndexDarkness[3];
						}
						else if (tc >= 0 && tc < w*h && pLightsTemp[tc].m_Index == aTileIndexDarkness[3] && pTiles[tc].m_Index == 0 && (pLightsTemp[c].m_Reserved != 1 || (aTileIndexDarkness[4] < pLightsTemp[c].m_Index && pLightsTemp[c].m_Reserved == 1)))
						{
							pLightsTemp[c].m_Reserved = 1;
							pLightsTemp[c].m_Index = aTileIndexDarkness[4];
						}
					}
				}

		}
	}

    // Spot Lights & Bright Tiles
    static int64 LightTime = time_get();
    static bool BigSize = false;
    if (time_get() - LightTime > 8.5f*time_freq()) // Blink
    {
    	BigSize = !BigSize;
        LightTime = time_get();
    }

    for(int y = StartY; y < EndY; y++)
		for(int x = StartX; x < EndX; x++)
		{
            int c = x + y*w;
            int TileIndex = pTiles[c].m_Index;
            CBlockManager::CBlockInfo *pBlockInfo = GetBlockManager()->GetBlockInfo(TileIndex);
            if (!pBlockInfo)
            	continue;

            if (pBlockInfo->m_LightSize && pBlockInfo->m_LightSize <= 5)
            	pLightsTemp[c].m_Index = aTileIndexDarkness[5-pBlockInfo->m_LightSize];
            else if (pBlockInfo->m_LightSize)
            {
            	int LightSize = (!BigSize)?(pBlockInfo->m_LightSize - pBlockInfo->m_LightSize/3):pBlockInfo->m_LightSize;
                for (int e=0; e<=LightSize; e++)
                {
                    int index = 4-(e*4)/LightSize;
                    const int ff = (LightSize-e)/2;
                    for (int i=ff; i>=-ff; i--)
                        for (int o=-ff; o<=ff; o++)
                        {
                            int tc = clamp(x+o, 0, w-1) + clamp(y-i, 0, h-1)*w;
                            if (aTileIndexDarkness[index] < pLightsTemp[tc].m_Index)
                                pLightsTemp[tc].m_Index = aTileIndexDarkness[index];
                        }
                }

                pLightsTemp[c].m_Index = 0;
            }
		}

    // Update Skip Info
    /*for(int y = StartY; y < EndY; y++)
		for(int x = StartX; x < EndX; x++)
	{
		int sx;
		for(sx = 1; x+sx < w && sx < 255; sx++)
		{
			if(pLightsTemp[y*w+x+sx].m_Index)
				break;
		}

		pLightsTemp[y*w+x].m_Skip = sx-1;
		x += sx;
	}*/

    mem_copy(pLights, pLightsTemp, sizeof(CTile)*w*h);
    mem_free(pLightsTemp);
}
