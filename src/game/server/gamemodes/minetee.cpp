#include "minetee.h"
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>
#include <game/server/gamecontext.h>
#include <game/version.h>

#include <cstring>

CGameControllerMineTee::CGameControllerMineTee(class CGameContext *pGameServer)
: IGameController(pGameServer)
{
	m_pGameType = "MineTee";
	m_TimeVegetal = Server()->Tick();
	m_TimeEnv = Server()->Tick();
	m_TimeDestruction = Server()->Tick();
	m_TimeCook = Server()->Tick();
	m_TimeWear = Server()->Tick();
}

void CGameControllerMineTee::Tick()
{
    bool Vegetal=false, Envirionment=false, Destruction=false, Cook = false, Wear = false;

    //Control Actions
    if (Server()->Tick() - m_TimeVegetal > Server()->TickSpeed()*60.0f)
    {
        Vegetal = true;
        m_TimeVegetal = Server()->Tick();
    }
    if (Server()->Tick() - m_TimeEnv > Server()->TickSpeed()*0.75f)
    {
        Envirionment = true;
        m_TimeEnv = Server()->Tick();
    }
    if (Server()->Tick() - m_TimeDestruction > Server()->TickSpeed()*0.5f)
    {
        Destruction = true;
        m_TimeDestruction = Server()->Tick();
    }
    if (Server()->Tick() - m_TimeCook > Server()->TickSpeed()*5.0f)
    {
        Cook = true;
        m_TimeCook = Server()->Tick();
    }
    if (Server()->Tick() - m_TimeWear > Server()->TickSpeed()*75.0f)
    {
        Wear = true;
        m_TimeWear = Server()->Tick();
    }


    //Actions
    if (Vegetal || Envirionment || Destruction || Cook || Wear)
    {
        CMapItemLayerTilemap *pTmap = (CMapItemLayerTilemap *)GameServer()->Layers()->MineTeeLayer();
        if (pTmap)
        {
        	CBlockManager::CBlockInfo BlockInfo;
            CTile *pTiles = (CTile *)GameServer()->Layers()->Map()->GetData(pTmap->m_Data);
            CTile *pTempTiles = static_cast<CTile*>(mem_alloc(sizeof(CTile)*pTmap->m_Width*pTmap->m_Height,1));
            mem_copy(pTempTiles, pTiles, sizeof(CTile)*pTmap->m_Width*pTmap->m_Height);

            for(int y = 0; y < pTmap->m_Height-1; y++)
            {
                for(int x = 0; x < pTmap->m_Width-1; x++)
                {
                    int c = y*pTmap->m_Width+x;
                    if (!GameServer()->m_BlockManager.GetBlockInfo(pTempTiles[c].m_Index, &BlockInfo))
                    	continue;

                    if (Envirionment)
                    {
                    	// Dinamic Fluid
                        if (((pTempTiles[c].m_Index >= CBlockManager::WATER_A && pTempTiles[c].m_Index <= CBlockManager::WATER_D) || (pTempTiles[c].m_Index >= CBlockManager::LAVA_A && pTempTiles[c].m_Index <= CBlockManager::LAVA_D)) && y+1 < pTmap->m_Height && x+1 < pTmap->m_Width)
                        {
                            int tc = (y+1)*pTmap->m_Width+x;
                            if (pTempTiles[tc].m_Index == 0 || (pTempTiles[tc].m_Index >= CBlockManager::WATER_A && pTempTiles[tc].m_Index < CBlockManager::WATER_C) || (pTempTiles[tc].m_Index >= CBlockManager::LAVA_A && pTempTiles[tc].m_Index < CBlockManager::LAVA_C))
                            {
                                int TileIndex = (pTempTiles[c].m_Index >= CBlockManager::WATER_A && pTempTiles[c].m_Index <= CBlockManager::WATER_D)?CBlockManager::WATER_A:CBlockManager::LAVA_A;

                                GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y+1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), TileIndex, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y+1), GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), TileIndex, 0);
                            }
                            else if ((pTempTiles[c].m_Index >= CBlockManager::LAVA_A && pTempTiles[c].m_Index <= CBlockManager::LAVA_D && pTempTiles[tc].m_Index >= CBlockManager::WATER_A && pTempTiles[tc].m_Index <= CBlockManager::WATER_D) ||
                                     (pTempTiles[c].m_Index >= CBlockManager::WATER_A && pTempTiles[c].m_Index <= CBlockManager::WATER_D && pTempTiles[tc].m_Index >= CBlockManager::LAVA_A && pTempTiles[tc].m_Index <= CBlockManager::LAVA_D))
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y+1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::OBSIDIAN_A, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y+1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::OBSIDIAN_A, 0);
                            }

                            tc = y*pTmap->m_Width+(x+1);
                            int tb = (y+1)*pTmap->m_Width+(x+1);
                            int te = (y-1)*pTmap->m_Width+(x+1);
                            int tl = (y+1)*pTmap->m_Width+x;
                            if (pTempTiles[tl].m_Index != 0 && (pTempTiles[tl].m_Index < CBlockManager::WATER_A || pTempTiles[tl].m_Index > CBlockManager::WATER_D) && (pTempTiles[tl].m_Index < CBlockManager::LAVA_A || pTempTiles[tl].m_Index > CBlockManager::LAVA_D) &&
                                pTempTiles[tc].m_Index == 0 &&
                                (pTempTiles[tb].m_Index < CBlockManager::WATER_A || pTempTiles[tb].m_Index > CBlockManager::WATER_D) && (pTempTiles[tb].m_Index < CBlockManager::LAVA_A || pTempTiles[tb].m_Index > CBlockManager::LAVA_D) &&
                                pTempTiles[c].m_Index-1 != CBlockManager::WATER_A-1 && pTempTiles[c].m_Index-1 != CBlockManager::LAVA_A-1 &&
                                (pTempTiles[te].m_Index < CBlockManager::WATER_A || pTempTiles[te].m_Index > CBlockManager::WATER_D) && (pTempTiles[te].m_Index < CBlockManager::LAVA_A || pTempTiles[te].m_Index > CBlockManager::LAVA_D))
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x+1, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), pTiles[c].m_Index-1, 0);
                                GameServer()->Collision()->ModifTile(vec2(x+1, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), pTiles[c].m_Index-1, 0);
                            }
                            tc = y*pTmap->m_Width+(x-1);
                            tb = (y+1)*pTmap->m_Width+(x-1);
                            te = (y-1)*pTmap->m_Width+(x-1);
                            if (pTempTiles[tl].m_Index != 0 && pTempTiles[tl].m_Index != CBlockManager::WATER_D && pTempTiles[tl].m_Index != CBlockManager::LAVA_D &&
                                pTempTiles[tc].m_Index == 0 &&
                                (pTempTiles[tb].m_Index < CBlockManager::WATER_A || pTempTiles[tb].m_Index > CBlockManager::WATER_D) && (pTempTiles[tb].m_Index < CBlockManager::LAVA_A || pTempTiles[tb].m_Index > CBlockManager::LAVA_D) &&
                                pTempTiles[c].m_Index-1 != CBlockManager::WATER_A-1 && pTempTiles[c].m_Index-1 != CBlockManager::LAVA_A-1 &&
                                (pTempTiles[te].m_Index < CBlockManager::WATER_A || pTempTiles[te].m_Index > CBlockManager::WATER_D) && (pTempTiles[te].m_Index < CBlockManager::LAVA_A || pTempTiles[te].m_Index > CBlockManager::LAVA_D))
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x-1, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), pTiles[c].m_Index-1, 0);
                                GameServer()->Collision()->ModifTile(vec2(x-1, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), pTiles[c].m_Index-1, 0);
                            }
                        }
                        // Rose & Daisy Generates a Ground Cultivate Dry
                        else if (pTempTiles[c].m_Index == CBlockManager::ROSE || pTempTiles[c].m_Index == CBlockManager::DAISY)
                        {
                            int indexT = (y+1)*pTmap->m_Width+x;
                            if (pTempTiles[indexT].m_Index == CBlockManager::DIRT)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y+1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::GROUND_CULTIVATION_DRY, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y+1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::GROUND_CULTIVATION_DRY, 0);
                            }
                        }
                        // Two Beds = One Large Bed
                        else if (pTempTiles[c].m_Index == CBlockManager::BED)
                        {
                            int indexT = y*pTmap->m_Width+(x+1);
                            if (pTempTiles[indexT].m_Index == CBlockManager::BED)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::LARGE_BED_LEFT, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::LARGE_BED_LEFT, 0);

                                GameServer()->SendTileModif(ALL_PLAYERS, vec2(x+1, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::LARGE_BED_RIGHT, 0);
                                GameServer()->Collision()->ModifTile(vec2(x+1, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::LARGE_BED_RIGHT, 0);
                            }
                        }
                        // Two Chest = One Large Chest
                        else if (pTempTiles[c].m_Index == CBlockManager::CHEST)
                        {
                            int indexT = y*pTmap->m_Width+(x+1);
                            if (pTempTiles[indexT].m_Index == CBlockManager::CHEST)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::LARGE_CHEST_LEFT, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::LARGE_CHEST_LEFT, 0);

                                GameServer()->SendTileModif(ALL_PLAYERS, vec2(x+1, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::LARGE_CHEST_RIGHT, 0);
                                GameServer()->Collision()->ModifTile(vec2(x+1, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::LARGE_CHEST_RIGHT, 0);
                            }
                        }

                        // Check what block are on top
                        if (y>0)
                        {
                        	// Dirt -> Grass
                            if (pTempTiles[c].m_Index == CBlockManager::DIRT)
                            {
                                bool found = false;
                                for (int o=y-1; o>=0; o--)
                                {
                                    int indexT = o*pTmap->m_Width+x;
                                    if (pTempTiles[indexT].m_Index != 0 && (pTempTiles[indexT].m_Index < CBlockManager::GRASS_A || pTempTiles[indexT].m_Index > CBlockManager::GRASS_F))
                                    {
                                        found = true;
                                        break;
                                    }
                                }

                                if (!found)
                                {
                                	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::GRASS, 0);
                                    GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::GRASS, 0);
                                }
                            }
                            // Grass -> Dirt or Snow
                            else if (pTempTiles[c].m_Index == CBlockManager::GRASS)
                            {
                                int indexT = (y-1)*pTmap->m_Width+x;
                                if (pTempTiles[indexT].m_Index != 0 && (pTempTiles[indexT].m_Index < CBlockManager::GRASS_A || pTempTiles[indexT].m_Index > CBlockManager::GRASS_F))
                                {
                                    int TileIndex = (pTempTiles[indexT].m_Index == CBlockManager::SNOW)?CBlockManager::DIRT_SNOW:CBlockManager::DIRT;

                                    GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), TileIndex, 0);
                                    GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), TileIndex, 0);
                                }
                            }
                            // Turn On Oven if Coal touch one side
                            // TODO: Move this!
                            else if (pTempTiles[c].m_Index == CBlockManager::OVEN_OFF)
                            {
                                int indexT = (y-1)*pTmap->m_Width+x;
                                int indexR = y*pTmap->m_Width+(x-1);
                                int indexS = y*pTmap->m_Width+(x+1);
                                if (pTempTiles[indexT].m_Index == CBlockManager::COAL || pTempTiles[indexR].m_Index == CBlockManager::COAL || pTempTiles[indexS].m_Index == CBlockManager::COAL)
                                {
                                	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::OVEN_ON, 0);
                                    GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::OVEN_ON, 0);

                                    vec2 TilePos;
                                    if (pTempTiles[indexT].m_Index == CBlockManager::COAL)
                                    	TilePos = vec2(x, y-1);
                                    else if (pTempTiles[indexR].m_Index == CBlockManager::COAL)
                                    	TilePos = vec2(x-1, y);
                                    else if (pTempTiles[indexS].m_Index == CBlockManager::COAL)
                                    	TilePos = vec2(x+1, y);

                                    GameServer()->SendTileModif(ALL_PLAYERS, TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                    GameServer()->Collision()->ModifTile(TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                }
                            }
                        }

                        //BLOCK FALL
                        if (BlockInfo.m_Gravity)
                        {
                            int indexT = (y+1)*pTmap->m_Width+x;
                            if (pTempTiles[indexT].m_Index == 0)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);

                                GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y+1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), pTempTiles[c].m_Index, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y+1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), pTempTiles[c].m_Index, 0);
                            }
                        }

                        //BLOCK DAMAGE
                        if (BlockInfo.m_Damage)
                        {
                            for (size_t e=0; e<MAX_CLIENTS; e++)
                            {
                                if (!GameServer()->m_apPlayers[e] || !GameServer()->m_apPlayers[e]->GetCharacter() || !GameServer()->m_apPlayers[e]->GetCharacter()->IsAlive())
                                    continue;

                                if (distance(GameServer()->m_apPlayers[e]->GetCharacter()->m_Pos, vec2((x<<5)+16.0f,(y<<5)+16.0f)) <= 36.0f)
                                    GameServer()->m_apPlayers[e]->GetCharacter()->TakeDamage(vec2(0.0f, 0.0f), 1.0f, GameServer()->m_apPlayers[e]->GetCID(), WEAPON_WORLD);
                            }
                        }
                    }

                    if (Destruction)
                    {
                        if (pTempTiles[c].m_Index == CBlockManager::SUGAR_CANE)
                        {
                            int indexT = (y+1)*pTmap->m_Width+x;
                            if (pTempTiles[indexT].m_Index != CBlockManager::SUGAR_CANE && pTempTiles[indexT].m_Index != CBlockManager::GROUND_CULTIVATION_WET)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);

                                CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, CBlockManager::SUGAR_CANE);
                                pPickup->m_Pos = vec2(x*32.0f + 8.0f, y*32.0f + 8.0f);
                            }
                        }
                        else if (pTempTiles[c].m_Index == CBlockManager::CACTUS)
                        {
                            int indexT = (y+1)*pTmap->m_Width+x;
                            if (pTempTiles[indexT].m_Index != CBlockManager::CACTUS && pTempTiles[indexT].m_Index != CBlockManager::SAND)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);

                                CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, CBlockManager::CACTUS);
                                pPickup->m_Pos = vec2(x*32.0f + 8.0f, y*32.0f + 8.0f);
                            }
                        }
                        else if (pTempTiles[c].m_Index == CBlockManager::TORCH)
                        {
                            bool found = false;

                            int indexT = y*pTmap->m_Width+(x-1);
                            if (pTempTiles[indexT].m_Index != 0 && pTempTiles[indexT].m_Index != CBlockManager::LAVA_D && pTempTiles[indexT].m_Index != CBlockManager::WATER_D)
                                found = true;
                            indexT = y*pTmap->m_Width+(x+1);
                            if (pTempTiles[indexT].m_Index != 0 && pTempTiles[indexT].m_Index != CBlockManager::LAVA_D && pTempTiles[indexT].m_Index != CBlockManager::WATER_D)
                                found = true;
                            indexT = (y+1)*pTmap->m_Width+x;
                            if (pTempTiles[indexT].m_Index != 0 && pTempTiles[indexT].m_Index != CBlockManager::LAVA_D && pTempTiles[indexT].m_Index != CBlockManager::WATER_D)
                                found = true;

                            if (!found)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);

                                CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, CBlockManager::TORCH);
                                pPickup->m_Pos = vec2(x*32.0f + 8.0f, y*32.0f + 8.0f);
                            }
                        }
                        else if (pTempTiles[c].m_Index >= CBlockManager::GRASS_A && pTempTiles[c].m_Index <= CBlockManager::GRASS_G)
                        {
                            int indexT = (y+1)*pTmap->m_Width+x;
                            if (pTempTiles[indexT].m_Index == 0)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);

                                CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, CBlockManager::SEED);
                                pPickup->m_Pos = vec2(x*32.0f + 8.0f, y*32.0f + 8.0f);
                            }
                        }
                        else if (pTempTiles[c].m_Index == CBlockManager::LARGE_BED_LEFT)
                        {
                            int indexT = y*pTmap->m_Width+(x+1);
                            if (pTempTiles[indexT].m_Index == 0)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);

                                CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, CBlockManager::BED);
                                pPickup->m_Pos = vec2(x*32.0f + 8.0f, y*32.0f + 8.0f);
                            }
                        }
                        else if (pTempTiles[c].m_Index == CBlockManager::LARGE_BED_RIGHT)
                        {
                            int indexT = y*pTmap->m_Width+(x-1);
                            if (pTempTiles[indexT].m_Index == 0)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);

                                CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, CBlockManager::BED);
                                pPickup->m_Pos = vec2(x*32.0f + 8.0f, y*32.0f + 8.0f);
                            }
                        }
                        else if (pTempTiles[c].m_Index == CBlockManager::LARGE_CHEST_LEFT)
                        {
                            int indexT = y*pTmap->m_Width+(x+1);
                            if (pTempTiles[indexT].m_Index == 0)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);

                                CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, CBlockManager::CHEST);
                                pPickup->m_Pos = vec2(x*32.0f + 8.0f, y*32.0f + 8.0f);
                            }
                        }
                        else if (pTempTiles[c].m_Index == CBlockManager::LARGE_CHEST_RIGHT)
                        {
                            int indexT = y*pTmap->m_Width+(x-1);
                            if (pTempTiles[indexT].m_Index == 0)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);

                                CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, CBlockManager::CHEST);
                                pPickup->m_Pos = vec2(x*32.0f + 8.0f, y*32.0f + 8.0f);
                            }
                        }
                        else if (pTempTiles[c].m_Index == CBlockManager::WATER_C)
                        {
                            int indexT = (y-1)*pTmap->m_Width+x;
                            int indexR = y*pTmap->m_Width+(x-1);
                            int indexS = y*pTmap->m_Width+(x+1);
                            if ((pTempTiles[indexT].m_Index < CBlockManager::WATER_A || pTempTiles[indexT].m_Index > CBlockManager::WATER_D) &&
                                (pTempTiles[indexR].m_Index < CBlockManager::WATER_A || pTempTiles[indexR].m_Index > CBlockManager::WATER_D) &&
                                (pTempTiles[indexS].m_Index < CBlockManager::WATER_A || pTempTiles[indexS].m_Index > CBlockManager::WATER_D))
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                            }
                        }
                        else if (pTempTiles[c].m_Index == CBlockManager::LAVA_C)
                        {
                            int indexT = (y-1)*pTmap->m_Width+x;
                            int indexR = y*pTmap->m_Width+(x-1);
                            int indexS = y*pTmap->m_Width+(x+1);
                            if ((pTempTiles[indexT].m_Index < CBlockManager::LAVA_A || pTempTiles[indexT].m_Index > CBlockManager::LAVA_D) &&
                                (pTempTiles[indexR].m_Index < CBlockManager::LAVA_A || pTempTiles[indexR].m_Index > CBlockManager::LAVA_D) &&
                                (pTempTiles[indexS].m_Index < CBlockManager::LAVA_A || pTempTiles[indexS].m_Index > CBlockManager::LAVA_D))
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                            }
                        }
                        else if (pTempTiles[c].m_Index == CBlockManager::ROSE || pTempTiles[c].m_Index == CBlockManager::DAISY)
                        {
                            int indexT = (y+1)*pTmap->m_Width+x;
                            if (pTempTiles[indexT].m_Index == 0)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);

                                CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, pTempTiles[c].m_Index);
                                pPickup->m_Pos = vec2(x*32.0f + 8.0f, y*32.0f + 8.0f);
                            }
                        }
                    }

                    if (Vegetal)
                    {
                        /** Growing **/
                        if (pTempTiles[c].m_Index == CBlockManager::SUGAR_CANE)
                        {
                            int indexT = (y+1)*pTmap->m_Width+x;
                            if (pTempTiles[indexT].m_Index == CBlockManager::SUGAR_CANE || pTempTiles[indexT].m_Index == CBlockManager::GROUND_CULTIVATION_WET)
                            {
                                indexT = (y-1)*pTmap->m_Width+x;
                                if (pTempTiles[indexT].m_Index == 0)
                                {
                                    int tam=0;
                                    bool canC = false;
                                    for(int u=1; u<=5; u++)
                                    {
                                        int indexA = (y+u)*pTmap->m_Width+x;
                                        if (pTempTiles[indexA].m_Index == CBlockManager::SUGAR_CANE)
                                            tam++;
                                        else
                                        {
                                            if (pTempTiles[indexA].m_Index == CBlockManager::GROUND_CULTIVATION_WET)
                                                canC = true;
                                            break;
                                        }
                                    }

                                    if ((rand()%10) == 7 && canC && tam < 5)
                                    {
                                    	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y-1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::SUGAR_CANE, 0);
                                        GameServer()->Collision()->ModifTile(vec2(x, y-1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::SUGAR_CANE, 0);
                                    }
                                }
                            }
                        }

                        if (pTempTiles[c].m_Index == CBlockManager::CACTUS)
                        {
                            int indexT = (y+1)*pTmap->m_Width+x;
                            if (pTempTiles[indexT].m_Index == CBlockManager::CACTUS || pTempTiles[indexT].m_Index == CBlockManager::SAND)
                            {
                                indexT = (y-1)*pTmap->m_Width+x;
                                if (pTempTiles[indexT].m_Index == 0)
                                {
                                    int tam=0;
                                    bool canC = false;
                                    for(int u=1; u<=5; u++)
                                    {
                                        int indexA = (y+u)*pTmap->m_Width+x;
                                        if (pTempTiles[indexA].m_Index == CBlockManager::CACTUS)
                                            tam++;
                                        else
                                        {
                                            if (pTempTiles[indexA].m_Index == CBlockManager::SAND)
                                                canC = true;
                                            break;
                                        }
                                    }

                                    if ((rand()%10) == 7 && canC && tam < 8)
                                    {
                                    	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y-1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::CACTUS, 0);
                                        GameServer()->Collision()->ModifTile(vec2(x, y-1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::CACTUS, 0);
                                    }
                                }
                            }
                        }
                        else if (pTempTiles[c].m_Index >= CBlockManager::GRASS_A && pTempTiles[c].m_Index < CBlockManager::GRASS_G)
                        {
                            if ((rand()%1) == 0)
                            {
                                int nindex = pTempTiles[c].m_Index+1;
                                int cindex = (y+1)*pTmap->m_Width+x;
                                if (pTempTiles[cindex].m_Index != CBlockManager::GROUND_CULTIVATION_WET && nindex > CBlockManager::GRASS_D)
                                    nindex = CBlockManager::GRASS_D;

                                GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), nindex, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), nindex, 0);
                            }
                        }

                        /** Creation **/
                        if ((rand()%100)==3 && (pTempTiles[c].m_Index == CBlockManager::SAND))
                        {
                            int indexT = (y-1)*pTmap->m_Width+x;
                            if (pTempTiles[indexT].m_Index == 0)
                            {
                                bool found = false;

                                for (int i=-4; i<5; i++)
                                {
                                    int indexT = (y-1)*pTmap->m_Width+(x+i);
                                    if (pTempTiles[indexT].m_Index == CBlockManager::CACTUS)
                                    {
                                        found = true;
                                        break;
                                    }
                                }

                                if (!found)
                                {
                                	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y-1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::CACTUS, 0);
                                    GameServer()->Collision()->ModifTile(vec2(x, y-1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::CACTUS, 0);
                                }
                            }
                        }
                        else if ((rand()%100)==3 && pTempTiles[c].m_Index == CBlockManager::GRASS_D)
                        {
                            int indexT = y*pTmap->m_Width+(x+1);
                            int indexE = (y+1)*pTmap->m_Width+(x+1);
                            if (pTempTiles[indexT].m_Index == 0 && pTempTiles[indexE].m_Index == CBlockManager::GRASS)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x+1, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::GRASS_A, 0);
                                GameServer()->Collision()->ModifTile(vec2(x+1, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::LAVA_A, 0);
                            }
                        }

                        /** Dead **/
                        if (pTempTiles[c].m_Index == CBlockManager::GROUND_CULTIVATION_WET)
                        {
                            int indexT = (y-1)*pTmap->m_Width+x;
                            if (pTempTiles[indexT].m_Index == 0)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::GROUND_CULTIVATION_DRY, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::GROUND_CULTIVATION_DRY, 0);
                            }
                        }
                        else if (pTempTiles[c].m_Index == CBlockManager::GROUND_CULTIVATION_DRY)
                        {
                            int indexT = (y-1)*pTmap->m_Width+x;
                            if (pTempTiles[indexT].m_Index == 0)
                            {
                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::DIRT, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::DIRT, 0);
                            }
                        }
                    }

                    if (Cook)
                    {
                        if (y > 0)
                        {
                            if (pTempTiles[c].m_Index == CBlockManager::OVEN_ON)
                            {
                                int indexT = (y-1)*pTmap->m_Width+x;
                                CBlockManager::CBlockInfo CookedBlock;
                                if (GameServer()->m_BlockManager.GetBlockInfo(indexT, &CookedBlock))
                                {
                                    for (std::map<int, unsigned char>::iterator it = CookedBlock.m_vOnCook.begin(); it != CookedBlock.m_vOnCook.end(); it++)
                                    {
                                    	CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, it->first);
                                    	pPickup->m_Pos = vec2(x*32.0f + 8.0f, (y-1)*32.0f + 8.0f);
                                    	pPickup->m_Amount = it->second;
                                    }
                                }

                            	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y-1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                                GameServer()->Collision()->ModifTile(vec2(x, y-1), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                            }
                        }
                    }

                    if (Wear)
                    {

                        if (pTempTiles[c].m_Index == CBlockManager::OVEN_ON)
                        {
                        	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::OVEN_OFF, 0);
                            GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::OVEN_OFF, 0);
                        }
                        else if (!(rand()%100) && pTempTiles[c].m_Index == CBlockManager::STONE_BRICK)
                        {
                        	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::STONE_BRICK_DIRTY, 0);
                            GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::STONE_BRICK_DIRTY, 0);
                        }
                        else if (!(rand()%100) && pTempTiles[c].m_Index == CBlockManager::STONE_BRICK_DIRTY)
                        {
                        	GameServer()->SendTileModif(ALL_PLAYERS, vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::STONE_BRICK_IVY, 0);
                            GameServer()->Collision()->ModifTile(vec2(x, y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::STONE_BRICK_IVY, 0);
                        }
                    }
                }
            }

            mem_free(pTempTiles);
        }
    }

	IGameController::Tick();
}

void CGameControllerMineTee::OnCharacterSpawn(class CCharacter *pChr)
{
	pChr->IncreaseHealth(10);

	// Zombies Weapons
	switch (pChr->GetPlayer()->GetTeam())
	{
        case TEAM_ENEMY_TEEPER:
            pChr->GiveBlock(CBlockManager::TNT, 1);
            pChr->SetWeapon(NUM_WEAPONS+CBlockManager::TNT);
            break;

        case TEAM_ENEMY_ZOMBITEE:
        case TEAM_ENEMY_SPIDERTEE:
            pChr->GiveWeapon(WEAPON_HAMMER, -1);
            pChr->SetWeapon(WEAPON_HAMMER);
            break;

        case TEAM_ENEMY_SKELETEE:
            pChr->GiveWeapon(WEAPON_GRENADE, -1);
            pChr->SetWeapon(WEAPON_GRENADE);
            break;

        default:
        	if (!pChr->GetPlayer()->IsBot())
        	{
				pChr->GiveWeapon(WEAPON_HAMMER, -1);
				pChr->GiveBlock(CBlockManager::TORCH, 1);
				pChr->SetWeapon(NUM_WEAPONS+CBlockManager::TORCH);
        	}
	}
}

int CGameControllerMineTee::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
    IGameController::OnCharacterDeath(pVictim, pKiller, Weapon);

    if (!(pVictim->GetPlayer()->IsBot() && pVictim->GetPlayer() == pKiller))
    {
		for (size_t i=0; i<NUM_ITEMS_INVENTORY; i++)
		{
			int Index = pVictim->m_Inventory.m_Items[i];

			if (Index != NUM_WEAPONS+CBlockManager::MAX_BLOCKS)
			{
				if (Index >= NUM_WEAPONS)
				{
					if (pVictim->m_aBlocks[Index-NUM_WEAPONS].m_Got)
					{
						CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_DROPITEM, Index-NUM_WEAPONS);
						pPickup->m_Pos = pVictim->m_Pos;
						pPickup->m_Pos.y -= 18.0f;
						pPickup->m_Vel = vec2((((rand()%2)==0)?1:-1)*(rand()%10), -5);
						pPickup->m_Amount = pVictim->m_aBlocks[Index-NUM_WEAPONS].m_Amount;
						pPickup->m_Owner = pVictim->GetPlayer()->GetCID();
					}
				}
			}
		}
    }

    if (pVictim->GetPlayer() == pKiller)
        return 1;

    if (pVictim->GetPlayer()->GetTeam() == TEAM_ANIMAL_TEECOW)
    {
		CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_FOOD, FOOD_COW);
		pPickup->m_Pos = pVictim->m_Pos;
        pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, CBlockManager::COW_LEATHER);
		pPickup->m_Pos = pVictim->m_Pos;

		return 0;
    }
    else if (pVictim->GetPlayer()->GetTeam() == TEAM_ANIMAL_TEEPIG)
    {
		CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_FOOD, FOOD_PIG);
		pPickup->m_Pos = pVictim->m_Pos;
		return 0;
    }
    else if (pVictim->GetPlayer()->GetTeam() == TEAM_ENEMY_TEEPER)
    {
		CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, CBlockManager::GUNPOWDER);
		pPickup->m_Pos = pVictim->m_Pos;
		return 0;
    }
    else if (pVictim->GetPlayer()->GetTeam() == TEAM_ENEMY_SKELETEE)
    {
		CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, CBlockManager::BONE);
		pPickup->m_Pos = pVictim->m_Pos;
		return 0;
    }

    return 1;
}

bool CGameControllerMineTee::CanJoinTeam(int Team, int NotThisID)
{
    if (!Server()->IsAuthed(NotThisID) && Team == TEAM_SPECTATORS)
    {
        GameServer()->SendChatTarget(NotThisID, "Players can't join to spectators team.");
        return false;
    }

    return IGameController::CanJoinTeam(Team, NotThisID);
}

bool CGameControllerMineTee::OnChat(int cid, int team, const char *msg)
{
    char aBuff[255];
	char *ptr;

	if (!(ptr = strtok((char*)msg, " \n\t")) || msg[0] != '/')
		return true;

	if (str_comp_nocase(ptr,"/cmdlist") == 0 || str_comp_nocase(ptr,"/help") == 0)
	{
		GameServer()->SendChatTarget(cid,"--------------------------- --------- --------");
        GameServer()->SendChatTarget(cid,"COMMANDS LIST");
        GameServer()->SendChatTarget(cid,"======================================");
		GameServer()->SendChatTarget(cid," ");
		GameServer()->SendChatTarget(cid," - /info          > View general info of this mod.");
		GameServer()->SendChatTarget(cid," - /info about    > About of this mod.");
		GameServer()->SendChatTarget(cid," - /info <cmd>    > Info about selected command.");
		GameServer()->SendChatTarget(cid," - /craft         > Craft items!");
        GameServer()->SendChatTarget(cid," ");

		return false;
	}
	else if (str_comp_nocase(ptr,"/info") == 0)
	{
		if ((ptr=strtok(NULL, " \n\t")) == NULL)
		{
			char aBuf[128];
			GameServer()->SendChatTarget(cid,"--------------------------- --------- --------");
			str_format(aBuf, sizeof(aBuf), "INFO *** [MineTee v%s] by unsigned char*", MINETEE_VERSION);
			GameServer()->SendChatTarget(cid, aBuf);
			GameServer()->SendChatTarget(cid,"======================================");
			GameServer()->SendChatTarget(cid," ");
			GameServer()->SendChatTarget(cid," Build, Destroy, Imagine, Discover....");
			GameServer()->SendChatTarget(cid," ");

			return false;
		}
		else
		{
			do
			{
				if (str_comp_nocase(ptr,"about") == 0)
				{
					char aBuf[128];
					GameServer()->SendChatTarget(cid,"--------------------------- --------- --------");
        			str_format(aBuf, sizeof(aBuf), "MineTee v%s **", MINETEE_VERSION);
        			GameServer()->SendChatTarget(cid, aBuf);
					GameServer()->SendChatTarget(cid, "Author: unsigned char*");
        			str_format(aBuf, sizeof(aBuf), "Teeworlds Version: %s", GAME_VERSION);
        			GameServer()->SendChatTarget(cid, aBuf);
                    GameServer()->SendChatTarget(cid," ");

					return false;
				}
                if (str_comp_nocase(ptr,"craft") == 0)
				{
                	GameServer()->SendChatTarget(cid,"--------------------------- --------- --------");
                    GameServer()->SendChatTarget(cid,"- CRAFTING");
					GameServer()->SendChatTarget(cid, "======================================");
					GameServer()->SendChatTarget(cid, "Syntaxis: /craft <item>");
					GameServer()->SendChatTarget(cid, "Items: Horn, Torch, Bed, Chest, GLauncher, Grenade, Gun, Grille, Gold_Block, Iron_Fence, Iron_Window, Iron_Block, Wood_Window, Wood_Fence, Dust_A");
					GameServer()->SendChatTarget(cid, " ");
					GameServer()->SendChatTarget(cid, "Usage Example: /craft torch");
                    GameServer()->SendChatTarget(cid," ");

					return false;
				}
				else
                {
					GameServer()->SendChatTarget(cid,"Oops!: Unknown command or item");
                    return false;
                }
			} while ((ptr = strtok(NULL, " \n\t")) != NULL);
		}
	}
	else if (str_comp_nocase(ptr,"/craft") == 0)
    {
		char aBuf[80]={0};
        CCharacter *pChar = GameServer()->m_apPlayers[cid]->GetCharacter();
        if (!pChar)
        	return false;

		if ((ptr=strtok(NULL, "\n\t")) == NULL)
		{
			GameServer()->SendChatTarget(cid,"** Need item! Please, write '/info craft' to see more information about crafting.");

			return false;
		}
		else
		{
			bool BlockFounded = false;
			for (int u=0; u<CBlockManager::MAX_BLOCKS; u++)
			{
				CBlockManager::CBlockInfo BlockInfo;
				if (!GameServer()->m_BlockManager.GetBlockInfo(u, &BlockInfo))
					continue;
				if (str_comp_nocase(ptr, BlockInfo.m_aName) != 0 || BlockInfo.m_CraftNum == 0 || BlockInfo.m_vCraft.size() <= 0)
					continue;

				BlockFounded = true;

				bool CanCraft = true;
				for (std::map<int, unsigned char>::iterator it = BlockInfo.m_vCraft.begin(); it != BlockInfo.m_vCraft.end(); it++)
				{
					if (!pChar->m_aBlocks[it->first].m_Got || pChar->m_aBlocks[it->first].m_Amount <= 0 )
					{
						CanCraft = false;
						break;
					}
				}

				if (!CanCraft)
				{
					GameServer()->SendChatTarget(cid, "--------------------------- --------- --------");
					str_format(aBuf, sizeof(aBuf), "- CRAFT: %s   [%d Units]", BlockInfo.m_aName, BlockInfo.m_CraftNum);
					GameServer()->SendChatTarget(cid, aBuf);
					GameServer()->SendChatTarget(cid, "===============================");
					GameServer()->SendChatTarget(cid, "You need the following items:");
					GameServer()->SendChatTarget(cid, " ");
					for (std::map<int, unsigned char>::iterator it = BlockInfo.m_vCraft.begin(); it != BlockInfo.m_vCraft.end(); it++)
					{
						CBlockManager::CBlockInfo CraftBlockInfo;
						GameServer()->m_BlockManager.GetBlockInfo(it->first, &CraftBlockInfo);
						str_format(aBuf, sizeof(aBuf), "- %d x %s", it->second, CraftBlockInfo.m_aName);
						GameServer()->SendChatTarget(cid, aBuf);
					}
					GameServer()->SendChatTarget(cid, " ");
					return false;
				}

				for (std::map<int, unsigned char>::iterator it = BlockInfo.m_vCraft.begin(); it != BlockInfo.m_vCraft.end(); it++)
					pChar->m_aBlocks[it->first].m_Amount -= it->second;

				pChar->GiveBlock(u, BlockInfo.m_CraftNum);
				pChar->SetWeapon(NUM_WEAPONS+u);

				str_format(aBuf, sizeof(aBuf), "** You have been added a %s!", BlockInfo.m_aName);
				GameServer()->SendChatTarget(cid, aBuff);
			}

			if (!BlockFounded)
			{
				if (str_comp_nocase(ptr, "GLauncher") == 0)
				{
					CCharacter *pChar = GameServer()->m_apPlayers[cid]->GetCharacter();
					if (pChar)
					{
						if (!pChar->m_aBlocks[CBlockManager::GUNPOWDER].m_Got || pChar->m_aBlocks[CBlockManager::GUNPOWDER].m_Amount < 12 || !pChar->m_aBlocks[CBlockManager::WOOD_BROWN].m_Got || pChar->m_aBlocks[CBlockManager::WOOD_BROWN].m_Amount < 7 || !pChar->m_aBlocks[CBlockManager::IRON_INGOT].m_Got || pChar->m_aBlocks[CBlockManager::IRON_INGOT].m_Amount < 4)
						{
							GameServer()->SendChatTarget(cid,"--------------------------- --------- --------");
							GameServer()->SendChatTarget(cid, "- CRAFT: GLAUNCHER   [1 Unit]");
							GameServer()->SendChatTarget(cid, "======================================");
							GameServer()->SendChatTarget(cid, "You need the following items:");
							GameServer()->SendChatTarget(cid, " ");
							GameServer()->SendChatTarget(cid, " - 12 Powder");
							GameServer()->SendChatTarget(cid, " - 7 Wood [Brown]");
							GameServer()->SendChatTarget(cid, " - 4 Iron");
							GameServer()->SendChatTarget(cid," ");
						}
						else
						{
							pChar->m_aBlocks[CBlockManager::GUNPOWDER].m_Amount-=12;
							pChar->m_aBlocks[CBlockManager::WOOD_BROWN].m_Amount-=7;
							pChar->m_aBlocks[CBlockManager::IRON_INGOT].m_Amount-=4;
							pChar->GiveWeapon(WEAPON_GRENADE, 0);
							pChar->SetWeapon(WEAPON_GRENADE);

							GameServer()->SendChatTarget(cid,"** You have been added a Grenade Launcher!");
						}
					}

					return false;
				}
				else if (str_comp_nocase(ptr, "Grenade") == 0)
				{
					CCharacter *pChar = GameServer()->m_apPlayers[cid]->GetCharacter();
					if (pChar)
					{
						if (pChar->GetCurrentAmmo(WEAPON_GRENADE) <= 0 || !pChar->m_aBlocks[CBlockManager::GUNPOWDER].m_Got || pChar->m_aBlocks[CBlockManager::GUNPOWDER].m_Amount < 7 || !pChar->m_aBlocks[CBlockManager::IRON_INGOT].m_Got || pChar->m_aBlocks[CBlockManager::IRON_INGOT].m_Amount < 2)
						{
							GameServer()->SendChatTarget(cid,"--------------------------- --------- --------");
							GameServer()->SendChatTarget(cid, "- CRAFT: GRENADE   [5 Units]");
							GameServer()->SendChatTarget(cid, "======================================");
							GameServer()->SendChatTarget(cid, "You need the following items:");
							GameServer()->SendChatTarget(cid, " ");
							GameServer()->SendChatTarget(cid, " - 1 GLauncher");
							GameServer()->SendChatTarget(cid, " - 7 Powder");
							GameServer()->SendChatTarget(cid, " - 2 Iron");
							GameServer()->SendChatTarget(cid," ");
						}
						else
						{
							pChar->m_aBlocks[CBlockManager::GUNPOWDER].m_Amount-=7;
							pChar->m_aBlocks[CBlockManager::IRON_INGOT].m_Amount-=2;

							pChar->GiveWeapon(WEAPON_GRENADE, pChar->GetCurrentAmmo(WEAPON_GRENADE)+5);

							GameServer()->SendChatTarget(cid,"** You have been added a Grenade!");
						}
					}

					return false;
				}
				else if (str_comp_nocase(ptr, "Gun") == 0)
				{
					CCharacter *pChar = GameServer()->m_apPlayers[cid]->GetCharacter();
					if (pChar)
					{
						if (!pChar->m_aBlocks[CBlockManager::IRON_INGOT].m_Got || pChar->m_aBlocks[CBlockManager::IRON_INGOT].m_Amount < 12 || !pChar->m_aBlocks[CBlockManager::WOOD_BROWN].m_Got || pChar->m_aBlocks[CBlockManager::WOOD_BROWN].m_Amount < 4)
						{
							GameServer()->SendChatTarget(cid,"--------------------------- --------- --------");
							GameServer()->SendChatTarget(cid, "- CRAFT: GUN   [1 Unit]");
							GameServer()->SendChatTarget(cid, "======================================");
							GameServer()->SendChatTarget(cid, "You need the following items:");
							GameServer()->SendChatTarget(cid, " ");
							GameServer()->SendChatTarget(cid, " - 12 Iron");
							GameServer()->SendChatTarget(cid, " - 4 Wood [Brown]");
							GameServer()->SendChatTarget(cid," ");
						}
						else
						{
							pChar->m_aBlocks[CBlockManager::WOOD_BROWN].m_Amount-=4;
							pChar->m_aBlocks[CBlockManager::IRON_INGOT].m_Amount-=12;
							pChar->GiveWeapon(WEAPON_GUN, 0);
							pChar->SetWeapon(WEAPON_GUN);

							GameServer()->SendChatTarget(cid,"** You have been added a Gun!");
						}
					}

					return false;
				}
				else
				{
					GameServer()->SendChatTarget(cid,"Oops!: Unknown item");
					return false;
				}
			}
		}
    }
	else
    {
		GameServer()->SendChatTarget(cid,"Oops!: Unknown command");
        return false;
    }

    return true;
}
