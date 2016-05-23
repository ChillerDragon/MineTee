/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "minetee.h"
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/server/entities/character.h>
#include <game/server/entities/pickup.h>
#include <game/server/gamecontext.h>
#include <game/version.h>

#include <cstring>

CGameControllerMineTee::CGameControllerMineTee(class CGameContext *pGameServer)
: CGameController(pGameServer)
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
            CTile *pTiles = (CTile *)GameServer()->Layers()->Map()->GetData(pTmap->m_Data);
            CTile *pTempTiles = static_cast<CTile*>(mem_alloc(sizeof(CTile)*pTmap->m_Width*pTmap->m_Height,1));
            mem_copy(pTempTiles, pTiles, sizeof(CTile)*pTmap->m_Width*pTmap->m_Height);

            int StartX, EndX, StartY, EndY;
            for (int q=0; q<MAX_CLIENTS-MAX_BOTS; q++)
            {
				GetPlayerArea(q, &StartX, &EndX, &StartY, &EndY);

				for(int y = StartY; y < EndY; y++)
					for(int x = StartX; x < EndX; x++)
					{
						const ivec2 Positions[NUM_TILE_POS] = { ivec2(x,y), ivec2(x,y-1), ivec2(x-1,y-1), ivec2(x-1,y), ivec2(x-1,y+1), ivec2(x,y+1), ivec2(x+1,y+1), ivec2(x+1,y), ivec2(x+1,y-1) };
						const int Index[NUM_TILE_POS] = { y*pTmap->m_Width+x, (y-1)*pTmap->m_Width+x, (y-1)*pTmap->m_Width+(x-1), y*pTmap->m_Width+(x-1), (y+1)*pTmap->m_Width+(x-1), (y+1)*pTmap->m_Width+x, (y+1)*pTmap->m_Width+(x+1), y*pTmap->m_Width+(x+1), (y-1)*pTmap->m_Width+(x+1) };
						const int TileIndex[NUM_TILE_POS] = { pTempTiles[Index[0]].m_Index, pTempTiles[Index[1]].m_Index, pTempTiles[Index[2]].m_Index, pTempTiles[Index[3]].m_Index, pTempTiles[Index[4]].m_Index, pTempTiles[Index[5]].m_Index, pTempTiles[Index[6]].m_Index, pTempTiles[Index[7]].m_Index, pTempTiles[Index[8]].m_Index };

						CBlockManager::CBlockInfo *pBlockInfo = GameServer()->m_BlockManager.GetBlockInfo(TileIndex[TILE_CENTER]);
						if (TileIndex[TILE_CENTER] != pTiles[Index[TILE_CENTER]].m_Index || !pBlockInfo)
						{
							x += pTiles[Index[TILE_CENTER]].m_Skip;
							continue;
						}


						if (Envirionment)
						{
							int FluidType = 0;
							bool IsFluid = GameServer()->m_BlockManager.IsFluid(TileIndex[TILE_CENTER], &FluidType);

							// Fluids
							if (IsFluid)
							{
								// Fall
								if (TileIndex[TILE_BOTTOM] == 0 ||
									(TileIndex[TILE_BOTTOM] >= CBlockManager::WATER_A && TileIndex[TILE_BOTTOM] < CBlockManager::WATER_C) ||
									(TileIndex[TILE_BOTTOM] >= CBlockManager::LAVA_A && TileIndex[TILE_BOTTOM] < CBlockManager::LAVA_C))
								{
									int TileIndexTemp = (FluidType==CBlockManager::FLUID_WATER)?CBlockManager::WATER_C:CBlockManager::LAVA_C;
									ModifTile(x, y+1, TileIndexTemp);
								}
								// Create Obsidian?
								else if ((FluidType == CBlockManager::FLUID_LAVA && TileIndex[TILE_BOTTOM] >= CBlockManager::WATER_A && TileIndex[TILE_BOTTOM] <= CBlockManager::WATER_D) ||
										 (FluidType == CBlockManager::FLUID_WATER && TileIndex[TILE_BOTTOM] >= CBlockManager::LAVA_A && TileIndex[TILE_BOTTOM] <= CBlockManager::LAVA_D))
								{
									ModifTile(x, y+1, CBlockManager::OBSIDIAN_A);
								}

								// Spread To Right
								if (TileIndex[TILE_RIGHT] == 0 && TileIndex[TILE_BOTTOM] != 0 &&
									!GameServer()->m_BlockManager.IsFluid(TileIndex[TILE_BOTTOM]) &&
									!GameServer()->m_BlockManager.IsFluid(TileIndex[TILE_RIGHT_BOTTOM]) &&
									!GameServer()->m_BlockManager.IsFluid(TileIndex[TILE_RIGHT_TOP]) &&
									TileIndex[TILE_CENTER]-1 != CBlockManager::WATER_A-1 && TileIndex[TILE_CENTER]-1 != CBlockManager::LAVA_A-1)
								{
									ModifTile(x+1, y, pTiles[Index[TILE_CENTER]].m_Index-1);
								}
								// Spread To Left
								if (TileIndex[TILE_LEFT] == 0 && TileIndex[TILE_BOTTOM] != 0 &&
									!GameServer()->m_BlockManager.IsFluid(TileIndex[TILE_BOTTOM]) &&
									!GameServer()->m_BlockManager.IsFluid(TileIndex[TILE_LEFT_BOTTOM]) &&
									!GameServer()->m_BlockManager.IsFluid(TileIndex[TILE_LEFT_TOP]) &&
									TileIndex[TILE_CENTER]-1 != CBlockManager::WATER_A-1 && TileIndex[TILE_CENTER]-1 != CBlockManager::LAVA_A-1)
								{
									ModifTile(x-1, y, pTiles[Index[TILE_CENTER]].m_Index-1);
								}

								// Check for correct tiles FIXME
								int FluidTypeTop = 0;
								bool IsFluidTop = GameServer()->m_BlockManager.IsFluid(TileIndex[TILE_TOP], &FluidTypeTop);
								if (IsFluidTop && TileIndex[TILE_CENTER] < CBlockManager::WATER_C)
								{
									ModifTile(x, y, FluidTypeTop==CBlockManager::FLUID_WATER?CBlockManager::WATER_C:CBlockManager::LAVA_C);
								}

							}

							// Mutations
							for (int i=0; i<pBlockInfo->m_vMutations.size(); i+=2)
							{
								bool MutationCheck = true;
								array<int> *pArray = &pBlockInfo->m_vMutations[i];
								array<int> *pArrayF = &pBlockInfo->m_vMutations[i+1];

								if (pArray->size() != NUM_TILE_POS-1 || pArrayF->size() != NUM_TILE_POS)
									continue;

								for (int e=0; e<pArray->size(); e++)
								{
									if ((*pArray)[e] == -2)
										continue;
									else if (((*pArray)[e] == -1 && !TileIndex[TILE_TOP+e]) ||
											((*pArray)[e] != -1 && TileIndex[TILE_TOP+e] != (*pArray)[e]))
									{
										MutationCheck = false;
										break;
									}
								}


								if (MutationCheck)
								{
									for (int e=0; e<NUM_TILE_POS; e++)
									{
										if ((*pArrayF)[e] == -1)
											continue;

										ModifTile(Positions[e], (*pArrayF)[e]);
									}

									break;
								}
							}

							// Check what block are on top
							if (y>0)
							{
								// OnSun
								if (pBlockInfo->m_OnSun != -1 && !(rand()%pBlockInfo->m_RandomActions))
								{
									bool found = false;
									for (int o=y-1; o>=0; o--)
									{
										int TileIndexTemp = o*pTmap->m_Width+x;
										if (pTempTiles[TileIndexTemp].m_Index != 0)
										{
											found = true;
											break;
										}
									}

									if (!found)
									{
										if (!pBlockInfo->m_RandomActions)
											ModifTile(x, y, pBlockInfo->m_OnSun);
										else if (!(rand()%pBlockInfo->m_RandomActions))
											ModifTile(x, y, pBlockInfo->m_OnSun);
									}
								}
								// Turn On Oven if Coal touch one side
								// TODO: Move this!
								else if (TileIndex[TILE_CENTER] == CBlockManager::OVEN_OFF &&
										(TileIndex[TILE_TOP] == CBlockManager::COAL || TileIndex[TILE_LEFT] == CBlockManager::COAL || TileIndex[TILE_RIGHT] == CBlockManager::COAL))
								{
									ivec2 TilePos;
									if (TileIndex[TILE_TOP] == CBlockManager::COAL)
										TilePos = ivec2(x, y-1);
									else if (TileIndex[TILE_LEFT] == CBlockManager::COAL)
										TilePos = ivec2(x-1, y);
									else if (TileIndex[TILE_RIGHT] == CBlockManager::COAL)
										TilePos = ivec2(x+1, y);

									ModifTile(x, y, CBlockManager::OVEN_ON);
									ModifTile(TilePos, 0);
								}
							}

							// Gravity
							if (pBlockInfo->m_Gravity && TileIndex[TILE_BOTTOM] == 0)
							{
								ModifTile(x, y, 0);
								ModifTile(x, y+1, TileIndex[TILE_CENTER]);
							}

							// Damage
							if (pBlockInfo->m_Damage)
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
							// Place Check
							if (pBlockInfo->m_vPlace.size() > 0)
							{
								bool PlaceCheck = false;
								for (int i=0; i<pBlockInfo->m_vPlace.size(); i++)
								{
									array<int> *pArray = &pBlockInfo->m_vPlace[i];
									for (int e=0; e<pArray->size(); e++)
									{
										if ((*pArray)[e] == -2)
											continue;
										else if (((*pArray)[e] == -1 && TileIndex[TILE_TOP+i] != 0) ||
											((*pArray)[e] != -1 && TileIndex[TILE_TOP+i] == (*pArray)[e]))
										{
											PlaceCheck = true;
											break;
										}
									}

									if (PlaceCheck)
										break;
								}

								if (!PlaceCheck)
								{
									ModifTile(x, y, 0);

									if (pBlockInfo->m_vOnBreak.size() > 0)
									{
										for (std::map<int, unsigned char>::iterator it = pBlockInfo->m_vOnBreak.begin(); it != pBlockInfo->m_vOnBreak.end(); it++)
										{
											if (it->first == 0)
											{
												++it;
												continue;
											}

											CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, it->first);
											pPickup->m_Pos = vec2((x<<5) + 8.0f, (y<<5) + 8.0f);
											pPickup->m_Amount = it->second;
										}
									}
									else
									{
										CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, TileIndex[TILE_CENTER]);
										pPickup->m_Pos = vec2((x<<5) + 8.0f, (y<<5) + 8.0f);
									}
								}
							}

							// Cut Fluids
							if ((TileIndex[TILE_CENTER] == CBlockManager::WATER_C || TileIndex[TILE_CENTER] == CBlockManager::LAVA_C) &&
									!GameServer()->m_BlockManager.IsFluid(TileIndex[TILE_TOP]) &&
									!GameServer()->m_BlockManager.IsFluid(TileIndex[TILE_LEFT]) &&
									!GameServer()->m_BlockManager.IsFluid(TileIndex[TILE_RIGHT]))
							{
								ModifTile(x, y, 0);
							}
						}

						if (Vegetal)
						{
							/** Growing **/
							if (TileIndex[TILE_CENTER] == CBlockManager::SUGAR_CANE &&
								(TileIndex[TILE_BOTTOM] == CBlockManager::SUGAR_CANE || TileIndex[TILE_BOTTOM] == CBlockManager::GROUND_CULTIVATION_WET) &&
								TileIndex[TILE_TOP] == 0)
							{
								int tam=0;
								bool canC = false;
								for(int u=1; u<=5; u++)
								{
									int TileIndexTemp = (y+u)*pTmap->m_Width+x;
									if (pTempTiles[TileIndexTemp].m_Index == CBlockManager::SUGAR_CANE)
										tam++;
									else
									{
										if (pTempTiles[TileIndexTemp].m_Index == CBlockManager::GROUND_CULTIVATION_WET)
											canC = true;
										break;
									}
								}

								if (!(rand()%10) && canC && tam < 5)
								{
									ModifTile(x, y-1, CBlockManager::SUGAR_CANE);
								}
							}

							if (TileIndex[TILE_CENTER] == CBlockManager::CACTUS &&
								(TileIndex[TILE_BOTTOM] == CBlockManager::CACTUS || TileIndex[TILE_BOTTOM] == CBlockManager::SAND) &&
								TileIndex[TILE_TOP] == 0)
							{
								int tam=0;
								bool canC = false;
								for(int u=1; u<=5; u++)
								{
									int TileIndexTemp = (y+u)*pTmap->m_Width+x;
									if (pTempTiles[TileIndexTemp].m_Index == CBlockManager::CACTUS)
										tam++;
									else
									{
										if (pTempTiles[TileIndexTemp].m_Index == CBlockManager::SAND)
											canC = true;
										break;
									}
								}

								if (!(rand()%10) && canC && tam < 8)
								{
									ModifTile(x, y-1, CBlockManager::CACTUS);
								}
							}
							else if (TileIndex[TILE_CENTER] >= CBlockManager::GRASS_A && TileIndex[TILE_CENTER] < CBlockManager::GRASS_G && !(rand()%1))
							{
								int nindex = TileIndex[TILE_CENTER]+1;
								if (TileIndex[TILE_BOTTOM] != CBlockManager::GROUND_CULTIVATION_WET && nindex > CBlockManager::GRASS_D)
									nindex = CBlockManager::GRASS_D;

								ModifTile(x, y, nindex);
							}

							/** Creation **/
							if (!(rand()%100) && (TileIndex[TILE_CENTER] == CBlockManager::SAND) && TileIndex[TILE_TOP] == 0)
							{
								bool found = false;

								for (int i=-4; i<5; i++)
								{
									int TileIndexTemp = (y-1)*pTmap->m_Width+(x+i);
									if (pTempTiles[TileIndexTemp].m_Index == CBlockManager::CACTUS)
									{
										found = true;
										break;
									}
								}

								if (!found)
									ModifTile(x, y-1, CBlockManager::CACTUS);
							}
							else if ((rand()%100)==3 && TileIndex[TILE_CENTER] == CBlockManager::GRASS_D &&
									TileIndex[TILE_RIGHT] == 0 && TileIndex[TILE_RIGHT_BOTTOM] == CBlockManager::GRASS)
							{
								ModifTile(x+1, y, CBlockManager::GRASS_A);
							}

							/** Dead **/
							if (TileIndex[TILE_CENTER] == CBlockManager::GROUND_CULTIVATION_WET && TileIndex[TILE_TOP] == 0)
							{
								ModifTile(x, y, CBlockManager::GROUND_CULTIVATION_DRY);
							}
							else if (TileIndex[TILE_CENTER] == CBlockManager::GROUND_CULTIVATION_DRY && TileIndex[TILE_TOP] == 0)
							{
								ModifTile(x, y, CBlockManager::DIRT);
							}
						}

						// OnCook
						if (Cook && y > 0)
						{
							if (TileIndex[TILE_CENTER] == CBlockManager::OVEN_ON)
							{
								CBlockManager::CBlockInfo *pCookedBlock = GameServer()->m_BlockManager.GetBlockInfo(TileIndex[TILE_TOP]);
								if (pCookedBlock)
								{
									for (std::map<int, unsigned char>::iterator it = pCookedBlock->m_vOnCook.begin(); it != pCookedBlock->m_vOnCook.end(); it++)
									{
										CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, it->first);
										pPickup->m_Pos = vec2((x<<5) + 8.0f, ((y-1)<<5) + 8.0f);
										pPickup->m_Amount = it->second;
									}
								}

								ModifTile(x, y-1, 0);
							}
						}

						// OnWear
						if (Wear && pBlockInfo->m_OnWear != -1)
						{
							if (!pBlockInfo->m_RandomActions)
								ModifTile(x, y, pBlockInfo->m_OnWear);
							else if (!(rand()%pBlockInfo->m_RandomActions))
								ModifTile(x, y, pBlockInfo->m_OnWear);
						}

						x += pTiles[Index[TILE_CENTER]].m_Skip;
					}
			}

            mem_free(pTempTiles);
        }
    }

	CGameController::Tick();
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
    CGameController::OnCharacterDeath(pVictim, pKiller, Weapon);

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

    return CGameController::CanJoinTeam(Team, NotThisID);
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
				CBlockManager::CBlockInfo *pBlockInfo = GameServer()->m_BlockManager.GetBlockInfo(u);
				if (!pBlockInfo)
					continue;
				if (str_comp_nocase(ptr, pBlockInfo->m_aName) != 0 || pBlockInfo->m_CraftNum == 0 || pBlockInfo->m_vCraft.size() <= 0)
					continue;

				BlockFounded = true;

				bool CanCraft = true;
				for (std::map<int, unsigned char>::iterator it = pBlockInfo->m_vCraft.begin(); it != pBlockInfo->m_vCraft.end(); it++)
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
					str_format(aBuf, sizeof(aBuf), "- CRAFT: %s   [%d Units]", pBlockInfo->m_aName, pBlockInfo->m_CraftNum);
					GameServer()->SendChatTarget(cid, aBuf);
					GameServer()->SendChatTarget(cid, "===============================");
					GameServer()->SendChatTarget(cid, "You need the following items:");
					GameServer()->SendChatTarget(cid, " ");
					for (std::map<int, unsigned char>::iterator it = pBlockInfo->m_vCraft.begin(); it != pBlockInfo->m_vCraft.end(); it++)
					{
						CBlockManager::CBlockInfo *pCraftBlockInfo = GameServer()->m_BlockManager.GetBlockInfo(it->first);
						if (pCraftBlockInfo)
						{
							str_format(aBuf, sizeof(aBuf), "- %d x %s", it->second, pCraftBlockInfo->m_aName);
							GameServer()->SendChatTarget(cid, aBuf);
						}
					}
					GameServer()->SendChatTarget(cid, " ");
					return false;
				}

				for (std::map<int, unsigned char>::iterator it = pBlockInfo->m_vCraft.begin(); it != pBlockInfo->m_vCraft.end(); it++)
					pChar->m_aBlocks[it->first].m_Amount -= it->second;

				pChar->GiveBlock(u, pBlockInfo->m_CraftNum);
				pChar->SetWeapon(NUM_WEAPONS+u);

				str_format(aBuf, sizeof(aBuf), "** You have been added a %s!", pBlockInfo->m_aName);
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

void CGameControllerMineTee::ModifTile(ivec2 MapPos, int TileIndex)
{
	//dbg_msg("MineTee", "MODIF TILE: %d [%dx%d]", TileIndex, MapPos.x, MapPos.y);
	GameServer()->SendTileModif(ALL_PLAYERS, MapPos, GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), TileIndex, 0);
    GameServer()->Collision()->ModifTile(MapPos, GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), TileIndex, 0);
}

void CGameControllerMineTee::GetPlayerArea(int ClientID, int *pStartX, int *pEndX, int *pStartY, int *pEndY)
{
	const int MapChunkSize = 24;
	*pStartX = *pEndX = *pStartY = *pEndY = 0;
	if (ClientID < 0 || ClientID > MAX_CLIENTS-MAX_BOTS || !GameServer()->m_apPlayers[ClientID] || !GameServer()->m_apPlayers[ClientID]->GetCharacter())
		return;

	vec2 ClientWorldPos = GameServer()->m_apPlayers[ClientID]->GetCharacter()->m_Pos;
	ivec2 ClientMapPos(ClientWorldPos.x/32, ClientWorldPos.y/32);

	*pStartX = max(ClientMapPos.x-MapChunkSize, 1);
	*pEndX = min(ClientMapPos.x+MapChunkSize, GameServer()->Collision()->GetWidth()-2);
	*pStartY = max(ClientMapPos.y-MapChunkSize, 1);
	*pEndY = min(ClientMapPos.y+MapChunkSize, GameServer()->Collision()->GetHeight()-2);
}
