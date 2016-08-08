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
	m_lpChests.clear();
	m_lpSigns.clear();

	LoadData();
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
        CMapItemLayerTilemap *pTmap = GameServer()->Layers()->MineTeeLayer();
        if (pTmap)
        {
            CTile *pTiles = (CTile *)GameServer()->Layers()->Map()->GetData(pTmap->m_Data);
            CTile *pTempTiles = static_cast<CTile*>(mem_alloc(sizeof(CTile)*pTmap->m_Width*pTmap->m_Height,1));
            mem_copy(pTempTiles, pTiles, sizeof(CTile)*pTmap->m_Width*pTmap->m_Height);

            int StartX, EndX, StartY, EndY;
            for (int q=0; q<g_Config.m_SvMaxClients; q++)
            {
				if (!GetPlayerArea(q, &StartX, &EndX, &StartY, &EndY))
					continue;

				CCharacter *pChar = GameServer()->GetPlayerChar(q);
				// STUCK
				if (pChar && Server()->Tick()-pChar->m_TimeStuck > 5*Server()->TickSpeed() && GameServer()->Collision()->TestBox(pChar->GetCore()->m_Pos, vec2(28.0f, 28.0f)))
				{
					pChar->TakeDamage(vec2(0.0f, 0.0f), 1, q, WEAPON_GAME);
					pChar->m_TimeStuck = Server()->Tick();
				}

				for(int y = StartY; y < EndY; y++)
					for(int x = StartX; x < EndX; x++)
					{
						const int Index[NUM_TILE_POS] = { y*pTmap->m_Width+x, (y-1)*pTmap->m_Width+x, (y-1)*pTmap->m_Width+(x-1), y*pTmap->m_Width+(x-1), (y+1)*pTmap->m_Width+(x-1), (y+1)*pTmap->m_Width+x, (y+1)*pTmap->m_Width+(x+1), y*pTmap->m_Width+(x+1), (y-1)*pTmap->m_Width+(x+1) };
						const int TileIndex[NUM_TILE_POS] = { pTempTiles[Index[0]].m_Index, pTempTiles[Index[1]].m_Index, pTempTiles[Index[2]].m_Index, pTempTiles[Index[3]].m_Index, pTempTiles[Index[4]].m_Index, pTempTiles[Index[5]].m_Index, pTempTiles[Index[6]].m_Index, pTempTiles[Index[7]].m_Index, pTempTiles[Index[8]].m_Index };

						const CBlockManager::CBlockInfo *pBlockInfo = GameServer()->m_BlockManager.GetBlockInfo(TileIndex[TILE_CENTER]);
						if (TileIndex[TILE_CENTER] != pTiles[Index[TILE_CENTER]].m_Index || !pBlockInfo)
						{
							x += pTiles[Index[TILE_CENTER]].m_Skip;
							continue;
						}

						if (Envirionment)
							EnvirionmentTick(pTempTiles, TileIndex, x, y, pBlockInfo);

						if (Destruction)
							DestructionTick(pTempTiles, TileIndex, x, y, pBlockInfo);

						if (Vegetal)
							VegetationTick(pTempTiles, TileIndex, x, y, pBlockInfo);

						// OnCook
						if (Cook && y > 0)
							CookTick(pTempTiles, TileIndex, x, y, pBlockInfo);

						// OnWear
						if (Wear && pBlockInfo->m_OnWear != -1)
							WearTick(pTempTiles, TileIndex, x, y, pBlockInfo);

						x += pTiles[Index[TILE_CENTER]].m_Skip;
					}
			}

            mem_free(pTempTiles);
        }
    }

	CGameController::Tick();
}

void CGameControllerMineTee::EnvirionmentTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo)
{
	CMapItemLayerTilemap *pTmap = GameServer()->Layers()->MineTeeLayer();
	CTile *pTiles = (CTile *)GameServer()->Layers()->Map()->GetData(pTmap->m_Data);
	const ivec2 Positions[NUM_TILE_POS] = { ivec2(x,y), ivec2(x,y-1), ivec2(x-1,y-1), ivec2(x-1,y), ivec2(x-1,y+1), ivec2(x,y+1), ivec2(x+1,y+1), ivec2(x+1,y), ivec2(x+1,y-1) };
	const int Index[NUM_TILE_POS] = { y*pTmap->m_Width+x, (y-1)*pTmap->m_Width+x, (y-1)*pTmap->m_Width+(x-1), y*pTmap->m_Width+(x-1), (y+1)*pTmap->m_Width+(x-1), (y+1)*pTmap->m_Width+x, (y+1)*pTmap->m_Width+(x+1), y*pTmap->m_Width+(x+1), (y-1)*pTmap->m_Width+(x+1) };
	int FluidType = 0;
	bool IsFluid = GameServer()->m_BlockManager.IsFluid(pTileIndex[TILE_CENTER], &FluidType);

	// Fluids
	if (IsFluid)
	{
		const CBlockManager::CBlockInfo *pBlockBottomInfo = GameServer()->m_BlockManager.GetBlockInfo(pTileIndex[TILE_BOTTOM]);

		// Fall
		if (pTileIndex[TILE_BOTTOM] == 0 || (pBlockBottomInfo && !pBlockBottomInfo->m_PlayerCollide) ||
			(pTileIndex[TILE_BOTTOM] >= CBlockManager::WATER_A && pTileIndex[TILE_BOTTOM] < CBlockManager::WATER_C) ||
			(pTileIndex[TILE_BOTTOM] >= CBlockManager::LAVA_A && pTileIndex[TILE_BOTTOM] < CBlockManager::LAVA_C))
		{
			int TileIndexTemp = (FluidType==CBlockManager::FLUID_WATER)?CBlockManager::WATER_C:CBlockManager::LAVA_C;
			ModifTile(ivec2(x, y+1), TileIndexTemp);
		}
		// Create Obsidian?
		else if ((FluidType == CBlockManager::FLUID_LAVA && pTileIndex[TILE_BOTTOM] >= CBlockManager::WATER_A && pTileIndex[TILE_BOTTOM] <= CBlockManager::WATER_D) ||
				 (FluidType == CBlockManager::FLUID_WATER && pTileIndex[TILE_BOTTOM] >= CBlockManager::LAVA_A && pTileIndex[TILE_BOTTOM] <= CBlockManager::LAVA_D))
		{
			ModifTile(ivec2(x, y+1), CBlockManager::OBSIDIAN_A);
		}

		// Spread To Right
		if (pTileIndex[TILE_RIGHT] == 0 && pTileIndex[TILE_BOTTOM] != 0 &&
			!GameServer()->m_BlockManager.IsFluid(pTileIndex[TILE_BOTTOM]) &&
			!GameServer()->m_BlockManager.IsFluid(pTileIndex[TILE_RIGHT_BOTTOM]) &&
			!GameServer()->m_BlockManager.IsFluid(pTileIndex[TILE_RIGHT_TOP]) &&
			pTileIndex[TILE_CENTER]-1 != CBlockManager::WATER_A-1 && pTileIndex[TILE_CENTER]-1 != CBlockManager::LAVA_A-1)
		{
			ModifTile(ivec2(x+1, y), pTiles[Index[TILE_CENTER]].m_Index-1);
		}
		// Spread To Left
		if (pTileIndex[TILE_LEFT] == 0 && pTileIndex[TILE_BOTTOM] != 0 &&
			!GameServer()->m_BlockManager.IsFluid(pTileIndex[TILE_BOTTOM]) &&
			!GameServer()->m_BlockManager.IsFluid(pTileIndex[TILE_LEFT_BOTTOM]) &&
			!GameServer()->m_BlockManager.IsFluid(pTileIndex[TILE_LEFT_TOP]) &&
			pTileIndex[TILE_CENTER]-1 != CBlockManager::WATER_A-1 && pTileIndex[TILE_CENTER]-1 != CBlockManager::LAVA_A-1)
		{
			ModifTile(ivec2(x-1, y), pTiles[Index[TILE_CENTER]].m_Index-1);
		}

		// Check for correct tiles FIXME
		int FluidTypeTop = 0;
		bool IsFluidTop = GameServer()->m_BlockManager.IsFluid(pTileIndex[TILE_TOP], &FluidTypeTop);
		if (IsFluidTop && pTileIndex[TILE_CENTER] < CBlockManager::WATER_C)
		{
			ModifTile(ivec2(x, y), FluidTypeTop==CBlockManager::FLUID_WATER?CBlockManager::WATER_C:CBlockManager::LAVA_C);
		}
	}

	// Mutations
	for (int i=0; i<pBlockInfo->m_vMutations.size(); i+=2)
	{
		bool MutationCheck = true;
		const array<int> *pArray = &pBlockInfo->m_vMutations[i];
		const array<int> *pArrayF = &pBlockInfo->m_vMutations[i+1];

		if (pArray->size() != NUM_TILE_POS-1 || pArrayF->size() != NUM_TILE_POS)
			continue;

		for (int e=0; e<pArray->size(); e++)
		{
			if ((*pArray)[e] == -2)
				continue;
			else if (((*pArray)[e] == -1 && !pTileIndex[TILE_TOP+e]) ||
					((*pArray)[e] != -1 && pTileIndex[TILE_TOP+e] != (*pArray)[e]))
			{
				MutationCheck = false;
				break;
			}
		}


		if (MutationCheck)
		{
			const bool IsChest = (str_comp_nocase(pBlockInfo->m_Functionality.m_aType, "chest") == 0);

			if (IsChest)
			{
				std::map<int, CChest*>::iterator it = m_lpChests.find(y*pTmap->m_Width+x);
				if (it != m_lpChests.end())
					it->second->Resize(pBlockInfo->m_Functionality.m_MutatedItems);
			}

			for (int e=0; e<NUM_TILE_POS; e++)
			{
				if ((*pArrayF)[e] == -1)
					continue;

				// Special checks
				if (IsChest && (Positions[e].x != x || Positions[e].y != y))
				{
					const int CurChestID = Positions[e].y*pTmap->m_Width+Positions[e].x;
					std::map<int, CChest*>::iterator StoredIt = m_lpChests.find(y*pTmap->m_Width+x);
					std::map<int, CChest*>::iterator RemovedIt = m_lpChests.find(CurChestID);
					if (RemovedIt != m_lpChests.end() && StoredIt != m_lpChests.end() && RemovedIt->second != StoredIt->second)
					{
						CChest *pChest = RemovedIt->second;
						mem_copy(StoredIt->second->m_apItems+NUM_CELLS_LINE*2, RemovedIt->second->m_apItems, NUM_CELLS_LINE*2*sizeof(CCellData));
						mem_free(pChest->m_apItems);
						mem_free(pChest);
						RemovedIt->second = StoredIt->second;
					}
				}

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
					ModifTile(ivec2(x, y), pBlockInfo->m_OnSun);
				else if (!(rand()%pBlockInfo->m_RandomActions))
					ModifTile(ivec2(x, y), pBlockInfo->m_OnSun);
			}
		}
		// Turn On Oven if Coal touch one side
		// TODO: Move this!
		else if (pTileIndex[TILE_CENTER] == CBlockManager::OVEN_OFF &&
				(pTileIndex[TILE_TOP] == CBlockManager::COAL || pTileIndex[TILE_LEFT] == CBlockManager::COAL || pTileIndex[TILE_RIGHT] == CBlockManager::COAL))
		{
			ivec2 TilePos;
			if (pTileIndex[TILE_TOP] == CBlockManager::COAL)
				TilePos = ivec2(x, y-1);
			else if (pTileIndex[TILE_LEFT] == CBlockManager::COAL)
				TilePos = ivec2(x-1, y);
			else if (pTileIndex[TILE_RIGHT] == CBlockManager::COAL)
				TilePos = ivec2(x+1, y);

			ModifTile(ivec2(x, y), CBlockManager::OVEN_ON);
			ModifTile(TilePos, 0);
			OnPlayerDestroyBlock(g_Config.m_SvMaxClients, TilePos); // FIXME: NUM_CLIENTS needs by something like 'WORLD_CLIENT'
		}
	}

	// Gravity
	if (pBlockInfo->m_Gravity && (pTileIndex[TILE_BOTTOM] == 0 || GameServer()->m_BlockManager.IsFluid(pTileIndex[TILE_BOTTOM])))
	{
		ModifTile(ivec2(x, y+1), pTileIndex[TILE_CENTER], pTiles[Index[TILE_CENTER]].m_Reserved);
		ModifTile(ivec2(x, y), 0);
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
void CGameControllerMineTee::DestructionTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo)
{
	// Place Check
	if (pBlockInfo->m_vPlace.size() > 0)
	{
		bool PlaceCheck = false;
		for (int i=0; i<pBlockInfo->m_vPlace.size(); i++)
		{
			const array<int> *pArray = &pBlockInfo->m_vPlace[i];
			for (int e=0; e<pArray->size(); e++)
			{
				if ((*pArray)[e] == -2)
					continue;
				else if (((*pArray)[e] == -1 && pTileIndex[TILE_TOP+i] != 0) ||
					((*pArray)[e] != -1 && pTileIndex[TILE_TOP+i] == (*pArray)[e]))
				{
					PlaceCheck = true;
					break;
				}
			}

			if (PlaceCheck)
				break;
		}

		if (!PlaceCheck)
			OnPlayerDestroyBlock(g_Config.m_SvMaxClients, ivec2(x, y)); // FIXME: NUM_CLIENTS needs by something like 'WORLD_CLIENT'
	}

	// Cut Fluids
	if ((pTileIndex[TILE_CENTER] == CBlockManager::WATER_C || pTileIndex[TILE_CENTER] == CBlockManager::LAVA_C) &&
			!GameServer()->m_BlockManager.IsFluid(pTileIndex[TILE_TOP]) &&
			!GameServer()->m_BlockManager.IsFluid(pTileIndex[TILE_LEFT]) &&
			!GameServer()->m_BlockManager.IsFluid(pTileIndex[TILE_RIGHT]))
	{
		ModifTile(ivec2(x, y), 0);
	}

	// Remove 'Dead' Blocks
	CMapItemLayerTilemap *pTmap = (CMapItemLayerTilemap *)GameServer()->Layers()->MineTeeLayer();
	const int TileIndexTemp = y*pTmap->m_Width+x;
	if (pTileIndex[TILE_CENTER] && pTempTiles[TileIndexTemp].m_Reserved == 0)
		OnPlayerDestroyBlock(-1, ivec2(x, y)); // TODO: Implement last user recognition
}

void CGameControllerMineTee::WearTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo)
{
	if (!pBlockInfo->m_RandomActions)
		ModifTile(ivec2(x, y), pBlockInfo->m_OnWear);
	else if (!(rand()%pBlockInfo->m_RandomActions))
		ModifTile(ivec2(x, y), pBlockInfo->m_OnWear);
}

void CGameControllerMineTee::CookTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo)
{
	if (pTileIndex[TILE_CENTER] == CBlockManager::OVEN_ON)
	{
		CBlockManager::CBlockInfo *pCookedBlock = GameServer()->m_BlockManager.GetBlockInfo(pTileIndex[TILE_TOP]);
		if (pCookedBlock)
		{
			for (std::map<int, unsigned char>::iterator it = pCookedBlock->m_vOnCook.begin(); it != pCookedBlock->m_vOnCook.end(); it++)
			{
				CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, it->first);
				pPickup->m_Pos = vec2((x<<5) + 8.0f, ((y-1)<<5) + 8.0f);
				pPickup->m_Amount = it->second;
			}
		}

		ModifTile(ivec2(x, y-1), 0);
	}
}

void CGameControllerMineTee::VegetationTick(CTile *pTempTiles, const int *pTileIndex, int x, int y, const CBlockManager::CBlockInfo *pBlockInfo)
{
	CMapItemLayerTilemap *pTmap = (CMapItemLayerTilemap *)GameServer()->Layers()->MineTeeLayer();
	int64 Time = -1;
	bool IsDay = false;
	GameServer()->GetServerTime(&IsDay, &Time);

	if (IsDay)
	{
		const bool HasWaterNear = (GameServer()->Collision()->IsBlockNear(CBlockManager::WATER_A, ivec2(x, y), 3)
									|| GameServer()->Collision()->IsBlockNear(CBlockManager::WATER_B, ivec2(x, y), 3)
									|| GameServer()->Collision()->IsBlockNear(CBlockManager::WATER_C, ivec2(x, y), 3)
									|| GameServer()->Collision()->IsBlockNear(CBlockManager::WATER_D, ivec2(x, y), 3));

		/** Growing **/
		// Need Water
		if (HasWaterNear)
		{
			if (pTileIndex[TILE_CENTER] == CBlockManager::SUGAR_CANE &&
				(pTileIndex[TILE_BOTTOM] == CBlockManager::SAND || pTileIndex[TILE_BOTTOM] == CBlockManager::GROUND_CULTIVATION_WET) &&
				pTileIndex[TILE_TOP] == 0)
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
					ModifTile(ivec2(x, y-1), CBlockManager::SUGAR_CANE);
				}
			}
		}

		// Not need water near
		if (pTileIndex[TILE_CENTER] == CBlockManager::CACTUS &&
			(pTileIndex[TILE_BOTTOM] == CBlockManager::CACTUS || pTileIndex[TILE_BOTTOM] == CBlockManager::SAND) &&
			pTileIndex[TILE_TOP] == 0)
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
				ModifTile(ivec2(x, y-1), CBlockManager::CACTUS);
			}
		}
		else if (pTileIndex[TILE_CENTER] >= CBlockManager::GRASS_A && pTileIndex[TILE_CENTER] < CBlockManager::GRASS_G && !(rand()%1))
		{
			int nindex = pTileIndex[TILE_CENTER]+1;
			if (pTileIndex[TILE_BOTTOM] != CBlockManager::GROUND_CULTIVATION_WET && nindex > CBlockManager::GRASS_D)
				nindex = CBlockManager::GRASS_D;

			ModifTile(ivec2(x, y), nindex);
		}

		/** Creation **/
		if (!(rand()%100) && (pTileIndex[TILE_CENTER] == CBlockManager::SAND) && pTileIndex[TILE_TOP] == 0)
		{
			if (HasWaterNear)
			{
				ModifTile(ivec2(x, y-1), CBlockManager::SUGAR_CANE);
			}
			else
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
					ModifTile(ivec2(x, y-1), CBlockManager::CACTUS);
			}
		}
		else if (!(rand()%100) && pTileIndex[TILE_CENTER] == CBlockManager::GRASS_D &&
				pTileIndex[TILE_RIGHT] == 0 && pTileIndex[TILE_BOTTOM] == CBlockManager::GRASS)
		{
			ModifTile(ivec2(x+1, y), CBlockManager::GRASS_A);
		}
		else if (!(rand()%100) && pTileIndex[TILE_CENTER] == CBlockManager::BROWN_TREE_SAPLING && pTileIndex[TILE_BOTTOM] == CBlockManager::GRASS)
		{
			GenerateTree(ivec2(x, y));
		}
	}

	/** Dead **/
	if (pTileIndex[TILE_CENTER] == CBlockManager::GROUND_CULTIVATION_WET && pTileIndex[TILE_TOP] == 0)
	{
		ModifTile(ivec2(x, y), CBlockManager::GROUND_CULTIVATION_DRY);
	}
	else if (pTileIndex[TILE_CENTER] == CBlockManager::GROUND_CULTIVATION_DRY && pTileIndex[TILE_TOP] == 0)
	{
		ModifTile(ivec2(x, y), CBlockManager::DIRT);
	}
}

void CGameControllerMineTee::OnCharacterSpawn(class CCharacter *pChr)
{
	// Zombies Weapons
	if (pChr->GetPlayer()->IsBot())
	{
		if (pChr->GetPlayer()->GetBotType() == BOT_MONSTER)
		{
			switch (pChr->GetPlayer()->GetBotSubType())
			{
				case BOT_MONSTER_TEEPER:
					pChr->IncreaseHealth(10);
					pChr->GiveItem(NUM_WEAPONS+CBlockManager::TNT, 1);
					break;

				case BOT_MONSTER_ZOMBITEE:
				//case CPlayer::BOT_MONSTER_SPIDERTEE:
				case BOT_MONSTER_EYE:
					pChr->IncreaseHealth(15);
					pChr->GiveItem(WEAPON_HAMMER, 255);
					break;

				case BOT_MONSTER_SKELETEE:
					pChr->IncreaseHealth(20);
					pChr->GiveItem(WEAPON_GRENADE, 255);
					break;
			}
		}
		else
			pChr->IncreaseHealth(5);
	}
	else if (!pChr->GetPlayer()->IsBot())
	{
		pChr->IncreaseHealth(10);
		pChr->GiveItem(WEAPON_HAMMER, 255);
		pChr->GiveItem(NUM_WEAPONS+CBlockManager::TORCH, 1);
	}
	pChr->SetInventoryItem(0);
}

int CGameControllerMineTee::OnCharacterDeath(class CCharacter *pVictim, class CPlayer *pKiller, int Weapon)
{
    CGameController::OnCharacterDeath(pVictim, pKiller, Weapon);

    if (!(pVictim->GetPlayer()->IsBot() && pVictim->GetPlayer() == pKiller) && Weapon > WEAPON_WORLD)
    {
		for (size_t i=0; i<NUM_CELLS_LINE; i++)
		{
			int Index = pVictim->m_FastInventory[i].m_ItemId;

			if (Index != 0)
			{
				if (Index >= NUM_WEAPONS)
				{
					CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_DROPITEM, Index);
					pPickup->m_Pos = pVictim->m_Pos;
					pPickup->m_Pos.y -= 18.0f;
					pPickup->m_Vel = vec2((((rand()%2)==0)?1:-1)*(rand()%10), -5);
					pPickup->m_Amount = pVictim->m_FastInventory[i].m_Amount;
					pPickup->m_Owner = pVictim->GetPlayer()->GetCID();
				}
			}
		}
    }

    if (pVictim->GetPlayer() == pKiller)
        return 1;

    if (pVictim->GetPlayer()->GetBotType() == BOT_ANIMAL)
    {
		if (pVictim->GetPlayer()->GetBotSubType() == BOT_ANIMAL_COW)
		{
			CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_FOOD, FOOD_COW);
			pPickup->m_Pos = pVictim->m_Pos;
			pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, CBlockManager::COW_LEATHER);
			pPickup->m_Pos = pVictim->m_Pos;

			return 0;
		}
		else if (pVictim->GetPlayer()->GetBotSubType() == BOT_ANIMAL_PIG)
		{
			CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_FOOD, FOOD_PIG);
			pPickup->m_Pos = pVictim->m_Pos;
			return 0;
		}
    }
    else if(pVictim->GetPlayer()->GetBotType() == BOT_MONSTER)
    {
		if (pVictim->GetPlayer()->GetBotSubType() == BOT_MONSTER_TEEPER)
		{
			CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, CBlockManager::GUNPOWDER);
			pPickup->m_Pos = pVictim->m_Pos;
			return 0;
		}
		else if (pVictim->GetPlayer()->GetBotSubType() == BOT_MONSTER_SKELETEE)
		{
			CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, CBlockManager::BONE);
			pPickup->m_Pos = pVictim->m_Pos;
			return 0;
		}
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

bool CGameControllerMineTee::CanSpawn(int Team, vec2 *pOutPos, int BotType, int BotSubType)
{
	CSpawnEval Eval;

	// spectators can't spawn
	if(Team == TEAM_SPECTATORS)
		return false;

	if ((g_Config.m_SvMonsters == 0 && BotType == BOT_MONSTER) ||
		(g_Config.m_SvAnimals == 0 && BotType == BOT_ANIMAL))
		return false;

	GenerateRandomSpawn(&Eval, BotType);

	// TODO: Improve this
	if (BotType == BOT_MONSTER && BotSubType == BOT_MONSTER_EYE && Eval.m_Pos.y < 150.0f)
		Eval.m_Got = false;

	*pOutPos = Eval.m_Pos;
	return Eval.m_Got;
}

void CGameControllerMineTee::SendInventory(int ClientID, bool IsCraftTable)
{
	CCharacter *pChar = GameServer()->GetPlayerChar(ClientID);
	if (!pChar)
		return;

	CCellData *pCellData = (CCellData*)mem_alloc(sizeof(CCellData)*(NUM_CELLS_LINE*5+1), 1);
	mem_copy(pCellData, pChar->m_FastInventory, sizeof(CCellData)*NUM_CELLS_LINE);
	mem_copy(pCellData+NUM_CELLS_LINE, pChar->GetPlayer()->m_aInventory, sizeof(CCellData)*NUM_CELLS_LINE*3);
	mem_copy(pCellData+NUM_CELLS_LINE*4, pChar->GetPlayer()->m_aCraft, sizeof(CCellData)*(NUM_CELLS_LINE+1));
	GameServer()->SendCellData(ClientID, pCellData, NUM_CELLS_LINE*5+1, IsCraftTable?CELLS_CRAFT_TABLE:CELLS_INVENTORY);
	mem_free(pCellData);
	if (!IsCraftTable)
		pChar->m_ActiveBlockId = -2; // Inventory
}

void CGameControllerMineTee::CreateChestSingle(int ClientID, ivec2 TilePos, int NumTiles, CCellData *pCellData)
{
	const int ChestID = TilePos.y*GameServer()->Collision()->GetWidth()+TilePos.x;
	CChest *pChest = (CChest*)mem_alloc(sizeof(CChest), 1);
	if (ClientID != -1)
		mem_copy(pChest->m_aOwnerKey, Server()->ClientKey(ClientID), sizeof(pChest->m_aOwnerKey));
	pChest->m_NumItems = NumTiles;
	pChest->m_apItems = (CCellData*)mem_alloc(sizeof(CCellData)*pChest->m_NumItems, 1);
	mem_zero(pChest->m_apItems, sizeof(CCellData)*pChest->m_NumItems);
	if (pCellData)
		mem_copy(pChest->m_apItems, pCellData, sizeof(CCellData)*pChest->m_NumItems);
	m_lpChests.insert(std::make_pair(ChestID, pChest));
}

bool CGameControllerMineTee::OnFunctionalBlock(int BlockID, ivec2 TilePos)
{
	if (CGameController::OnFunctionalBlock(BlockID, TilePos))
		return false;

	if (BlockID == CBlockManager::CHEST)
	{
		const int ChestID = TilePos.y*GameServer()->Collision()->GetWidth()+TilePos.x;
		if (m_lpChests.find(ChestID) == m_lpChests.end())
		{
			CBlockManager::CBlockInfo *pBlockInfo = GameServer()->m_BlockManager.GetBlockInfo(BlockID);
			if (pBlockInfo)
			{
				const int NumItems = pBlockInfo->m_Functionality.m_NormalItems;
				CCellData *pCellData = (CCellData*)mem_alloc(sizeof(CCellData)*NumItems, 1);
				CBlockManager::CBlockInfo *pNBlockInfo = 0x0;
				for (int i=0; i<NumItems; i++)
				{
					pCellData[i].m_Amount = 0;
					pCellData[i].m_ItemId = 0;

					if (rand()%10)
						continue;

					pCellData[i].m_Amount = rand()%125;
					if (pCellData[i].m_Amount)
					{
						int ItemID = 0;
						do
						{
							ItemID = rand()%(CBlockManager::MAX_BLOCKS+NUM_WEAPONS);
							pCellData[i].m_ItemId = ItemID;
							pNBlockInfo = (ItemID >= NUM_WEAPONS)?GameServer()->m_BlockManager.GetBlockInfo(ItemID-NUM_WEAPONS):0x0;
						} while (ItemID >= NUM_WEAPONS && pNBlockInfo && str_find(pNBlockInfo->m_aName, "NOT USED"));
					}
				}
				CreateChestSingle(-1, TilePos, NumItems, pCellData);
				mem_free(pCellData);
				return true;
			}
		}
	}

	return false;
}

void CGameControllerMineTee::OnClientActiveBlock(int ClientID)
{
	CCharacter *pChar = GameServer()->GetPlayerChar(ClientID);
	if (!pChar)
		return;

    bool Actived = false;
    bool Collide = false;
    const vec2 NextPos = pChar->m_Pos + pChar->GetMouseDirection() * 32.0f;
    vec2 FinishPos;

    if (GameServer()->Collision()->GetMineTeeTileIndexAt(pChar->m_Pos) != 0)
    {
    	FinishPos = pChar->m_Pos;
    	Collide = true;
    }
    else if (GameServer()->Collision()->GetMineTeeTileIndexAt(NextPos) != 0)
    {
    	FinishPos = NextPos;
    	Collide = true;
    }

	if (Collide)
	{
		const int MTTIndex = GameServer()->Collision()->GetMineTeeTileIndexAt(FinishPos);
		CBlockManager::CBlockInfo *pBlockInfo = GameServer()->m_BlockManager.GetBlockInfo(MTTIndex);

		if (!pBlockInfo || pBlockInfo->m_Functionality.m_aType[0] == 0)
		{
			pChar->m_ActiveBlockId = -1;
			return;
		}

		const int BlockID = ((int)FinishPos.y/32)*GameServer()->Collision()->GetWidth()+((int)FinishPos.x/32);

		// Check if block are used by other player
		for (int i=0; i<g_Config.m_SvMaxClients; i++)
		{
			CCharacter *pCompChar = GameServer()->GetPlayerChar(i);
			if (!pCompChar)
				continue;

			if (pCompChar->m_ActiveBlockId == BlockID)
			{
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "'%s' is using this...", Server()->ClientName(pChar->GetPlayer()->GetCID()));
				GameServer()->SendChatTarget(pChar->GetPlayer()->GetCID(), aBuf);
				pChar->m_ActiveBlockId = -1;
				return;
			}
		}

		if (str_comp(pBlockInfo->m_Functionality.m_aType, "chest") == 0)
		{
			std::map<int, CChest*>::iterator it = m_lpChests.find(BlockID);
			if (it != m_lpChests.end())
			{
				CCellData *pCellsData = (CCellData*)mem_alloc(sizeof(CCellData)*(it->second->m_NumItems+NUM_CELLS_LINE), 1);
				mem_copy(pCellsData, pChar->m_FastInventory, sizeof(CCellData)*NUM_CELLS_LINE);
				mem_copy(pCellsData+NUM_CELLS_LINE, it->second->m_apItems, sizeof(CCellData)*it->second->m_NumItems);
				GameServer()->SendCellData(ClientID, pCellsData, it->second->m_NumItems+NUM_CELLS_LINE, CELLS_CHEST);
				mem_free(pCellsData);
				pChar->m_ActiveBlockId = BlockID;
				Actived = true;
			}
		}
		else if (str_comp(pBlockInfo->m_Functionality.m_aType, "sign") == 0)
		{
			std::map<int, CSign>::iterator it = m_lpSigns.find(BlockID);
			if (it != m_lpSigns.end())
				GameServer()->SendChatTarget(ClientID, (it->second.m_aText[0] != 0)?it->second.m_aText:"EMPTY!");
		}
		else if (str_comp(pBlockInfo->m_Functionality.m_aType, "craft-table") == 0)
		{
			pChar->m_ActiveBlockId = BlockID;
			SendInventory(ClientID, true);
			Actived = true;
		}
	}

	if (!Actived)
		pChar->m_ActiveBlockId = -1;
}

void CGameControllerMineTee::OnPlayerPutBlock(int ClientID, ivec2 TilePos, int BlockID, int BlockFlags, int Reserved)
{
	CBlockManager::CBlockInfo *pBlockInfo = GameServer()->m_BlockManager.GetBlockInfo(BlockID);
	GameServer()->SendTileModif(ALL_PLAYERS, TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), BlockID, BlockFlags, Reserved);
	GameServer()->CreateSound(vec2(TilePos.x*32.0f, TilePos.y*32.0f), SOUND_DESTROY_BLOCK);

	// Create empty chest, but can be replaced in "mutation" process!
	if (pBlockInfo && str_comp(pBlockInfo->m_Functionality.m_aType, "chest") == 0)
	{
		CreateChestSingle(ClientID, TilePos, pBlockInfo->m_Functionality.m_NormalItems);
	}
	if (pBlockInfo && str_comp(pBlockInfo->m_Functionality.m_aType, "sign") == 0)
	{
		int SignID = TilePos.y*GameServer()->Collision()->GetWidth()+TilePos.x;
		m_lpSigns.insert(std::make_pair(SignID, CSign(Server()->ClientKey(ClientID))));
	}
}

void CGameControllerMineTee::OnPlayerDestroyBlock(int ClientID, ivec2 TilePos)
{
	const vec2 WorldTilePos = vec2(TilePos.x*32.0f, TilePos.y*32.0f);
	CTile *pTile = GameServer()->Collision()->GetMineTeeTileAt(WorldTilePos);
	if (!pTile)
		return;

	const int TileIndex = pTile->m_Index;
	const int ItemID = TileIndex+NUM_WEAPONS;
	CBlockManager::CBlockInfo *pBlockInfo = GameServer()->m_BlockManager.GetBlockInfo(TileIndex);

	GameServer()->CreateBlockRubble(WorldTilePos, TileIndex);
	GameServer()->SendTileModif(ALL_PLAYERS, TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0, 0);
	GameServer()->Collision()->ModifTile(TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0, 0);

	if (pBlockInfo->m_Explode)
	{
		GameServer()->CreateExplosion(WorldTilePos, ClientID, ItemID, false);
		GameServer()->CreateSound(WorldTilePos, SOUND_GRENADE_EXPLODE);
	}
	else
	{
		if (pBlockInfo->m_vOnBreak.size() > 0)
		{
			for (std::map<int, unsigned char>::iterator it = pBlockInfo->m_vOnBreak.begin(); it != pBlockInfo->m_vOnBreak.end(); it++)
			{
				if (it->first == 0)
				{
					++it;
					continue;
				}

				std::map<int, unsigned int>::iterator itProb = pBlockInfo->m_vProbabilityOnBreak.find(it->first);
				if (itProb != pBlockInfo->m_vProbabilityOnBreak.end() && !(rand()%itProb->second))
				{
					CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, it->first);
					pPickup->m_Pos = WorldTilePos + vec2(8.0f, 8.0f);
					pPickup->m_Amount = it->second;
				}
			}
		}
		else
		{
			CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, TileIndex);
			pPickup->m_Pos = WorldTilePos + vec2(8.0f, 8.0f);
		}
	}

	GameServer()->CreateSound(WorldTilePos, SOUND_DESTROY_BLOCK);

	if (pBlockInfo && str_comp(pBlockInfo->m_Functionality.m_aType, "chest") == 0)
	{
		int ChestID = TilePos.y*GameServer()->Collision()->GetWidth()+TilePos.x;
		std::map<int, CChest*>::iterator it = m_lpChests.find(ChestID);
		if (it != m_lpChests.end())
		{
			CChest *pChest = it->second;
			for (unsigned i=0; i<pChest->m_NumItems; i++)
			{
				if (pChest->m_apItems[i].m_ItemId == 0)
					continue;

				CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_DROPITEM, pChest->m_apItems[i].m_ItemId);
				pPickup->m_Pos = WorldTilePos;
				pPickup->m_Pos.y -= 18.0f;
				pPickup->m_Vel = vec2((((rand()%2)==0)?1:-1)*(rand()%10), -5);
				pPickup->m_Amount = pChest->m_apItems[i].m_Amount;
				pPickup->m_Owner = ClientID;
			}
			mem_free(pChest->m_apItems);
			mem_free(pChest);
			m_lpChests.erase(it);

			it = m_lpChests.begin();
			while (it != m_lpChests.end())
			{
				if (it->second == pChest)
				{
					it = m_lpChests.erase(it);
				}
				else
					++it;
			}
		}
	}
	else if (pBlockInfo && str_comp(pBlockInfo->m_Functionality.m_aType, "sign") == 0)
	{
		int SignID = TilePos.y*GameServer()->Collision()->GetWidth()+TilePos.x;
		std::map<int, CSign>::iterator it = m_lpSigns.find(SignID);
		if (it != m_lpSigns.end())
			m_lpSigns.erase(it);
	}
}

bool CGameControllerMineTee::OnChat(int cid, int team, const char *msg)
{
	char *ptr;

	if (!(ptr = strtok((char*)msg, " \n\t")) || msg[0] != '/')
		return true;

	/*if (str_comp_nocase(ptr,"/pet") == 0)
	{
		if ((ptr=strtok(NULL, " \n\t")) == NULL)
		{
			GameServer()->SendChatTarget(cid,"--------------------------- --------- --------");
			GameServer()->SendChatTarget(cid, "AVAILABLE PET COMMANDS");
			GameServer()->SendChatTarget(cid,"======================================");
			GameServer()->SendChatTarget(cid," ");
			GameServer()->SendChatTarget(cid," - /pet create : Create a new pet (Only for Dev. Version)");
			GameServer()->SendChatTarget(cid," - /pet name <name> : Change the pet name");
			GameServer()->SendChatTarget(cid," - /pet die : Kill your pet");
			GameServer()->SendChatTarget(cid," - /pet info : Know your pet");

			return false;
		}
		else
		{
			do
			{
				if (str_comp_nocase(ptr,"create") == 0)
				{
					CCharacter *pChar = GameServer()->GetPlayerChar(cid);
					if (!pChar)
						return false;
					const vec2 spawnPos = pChar->m_Pos-vec2(CPet::ms_PhysSize*1.25f, CPet::ms_PhysSize*1.25f);
					if (GameServer()->SpawnPet(pChar->GetPlayer(), spawnPos))
						GameServer()->SendChatTarget(cid,"Pet Created!");
					return false;
				}
				else if (str_comp_nocase(ptr,"name") == 0)
				{
					if ((ptr = strtok(NULL, "\n\t")) != 0x0)
					{
						CPlayer *pPlayer = GameServer()->m_apPlayers[cid];
						if (!pPlayer || !pPlayer->GetPet())
							return false;
						Server()->SetClientName(pPlayer->GetPet()->GetPlayer()->GetCID(), ptr, true);
					}
					return false;
				}
				else if (str_comp_nocase(ptr,"die") == 0)
				{
					CPlayer *pPlayer = GameServer()->m_apPlayers[cid];
					if (!pPlayer || !pPlayer->GetPet())
						return false;
					pPlayer->GetPet()->Die(cid, WEAPON_GAME);
					return false;
				}
				else if (str_comp_nocase(ptr,"info") == 0)
				{
					CPlayer *pPlayer = GameServer()->m_apPlayers[cid];
					if (!pPlayer || pPlayer->GetPet())
						return false;
					GameServer()->SendChatTarget(cid,"--------------------------- --------- --------");
					GameServer()->SendChatTarget(cid, "PET INFORMATION");
					GameServer()->SendChatTarget(cid,"======================================");
					return false;
				}
			} while ((ptr = strtok(NULL, " \n\t")) != NULL);
		}
	}*/
	if (str_comp_nocase(ptr,"/text") == 0 )
	{
		CCharacter *pChar = GameServer()->GetPlayerChar(cid);
		if (!pChar)
			return false;

	    bool Collide = false;
	    const vec2 NextPos = pChar->m_Pos + pChar->GetMouseDirection() * 32.0f;
	    vec2 FinishPos;

	    if (GameServer()->Collision()->GetMineTeeTileIndexAt(pChar->m_Pos) != 0)
	    {
	    	FinishPos = pChar->m_Pos;
	    	Collide = true;
	    }
	    else if (GameServer()->Collision()->GetMineTeeTileIndexAt(NextPos) != 0)
	    {
	    	FinishPos = NextPos;
	    	Collide = true;
	    }

		if (Collide)
		{
			const int MTTIndex = GameServer()->Collision()->GetMineTeeTileIndexAt(FinishPos);
			CBlockManager::CBlockInfo *pBlockInfo = GameServer()->m_BlockManager.GetBlockInfo(MTTIndex);

			if (!pBlockInfo || pBlockInfo->m_Functionality.m_aType[0] == 0)
			{
				pChar->m_ActiveBlockId = -1;
				return false;
			}

			if (str_comp(pBlockInfo->m_Functionality.m_aType, "sign") == 0)
			{
				int SignID = ((int)FinishPos.y/32)*GameServer()->Collision()->GetWidth()+((int)FinishPos.x/32);
				std::map<int, CSign>::iterator it = m_lpSigns.find(SignID);
				if (it != m_lpSigns.end())
				{
					if (mem_comp(it->second.m_aOwnerKey, Server()->ClientKey(cid), sizeof(it->second.m_aOwnerKey)) == 0)
					{
						str_copy(it->second.m_aText, msg+6, sizeof(it->second.m_aText));
						GameServer()->SendChatTarget(cid,"Sign text changed successfully!");
					}
					else
						GameServer()->SendChatTarget(cid,"You aren't the owner of this sign!");
					return false;
				}
			}
		}
		else
		{
			GameServer()->SendChatTarget(cid,"Any block near to change text on it :/");
			return false;
		}
	}
	else if (str_comp_nocase(ptr,"/cmdlist") == 0 || str_comp_nocase(ptr,"/help") == 0)
	{
		GameServer()->SendChatTarget(cid,"--------------------------- --------- --------");
        GameServer()->SendChatTarget(cid,"COMMANDS LIST");
        GameServer()->SendChatTarget(cid,"======================================");
		GameServer()->SendChatTarget(cid," ");
		GameServer()->SendChatTarget(cid," - /text <text>   > Change the text of the nearest sign.");
		GameServer()->SendChatTarget(cid," - /info          > View general info of this mod.");
		GameServer()->SendChatTarget(cid," - /info about    > About of this mod.");
		GameServer()->SendChatTarget(cid," - /info <cmd>    > Info about selected command.");
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
					GameServer()->SendChatTarget(cid, "--------------------------- --------- --------");
					GameServer()->SendChatTarget(cid, "Author: unsigned char* & contributors");
					GameServer()->SendChatTarget(cid, "Github Repo: https://github.com/CytraL/MineTee");
					GameServer()->SendChatTarget(cid, "Special thanks to: xush, android272, H-M-H");
        			str_format(aBuf, sizeof(aBuf), "MineTee v%s **", MINETEE_VERSION);
        			GameServer()->SendChatTarget(cid, aBuf);
        			str_format(aBuf, sizeof(aBuf), "Teeworlds Version: %s", GAME_VERSION);
        			GameServer()->SendChatTarget(cid, aBuf);
                    GameServer()->SendChatTarget(cid," ");

					return false;
				}
				if (str_comp_nocase(ptr,"text") == 0)
				{
					GameServer()->SendChatTarget(cid, "--------------------------- --------- --------");
					GameServer()->SendChatTarget(cid, "/text <text>");
					GameServer()->SendChatTarget(cid, "--------------------------- --------- --------");
					GameServer()->SendChatTarget(cid, "Change the text of the near sign.");
					GameServer()->SendChatTarget(cid, "For use this command you need be near of a sign.");

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
	else
    {
		GameServer()->SendChatTarget(cid,"Oops!: Unknown command");
        return false;
    }

    return true;
}

void CGameControllerMineTee::GenerateRandomSpawn(CSpawnEval *pEval, int BotType)
{
	vec2 P;
	bool IsBot = (BotType != -1);

	if (IsBot)
	{
		for (int q=0; q<g_Config.m_SvMaxClients;q++)
		{
			int StartX, StartY, EndX, EndY;
			if (!GetPlayerArea(q, &StartX, &EndX, &StartY, &EndY))
				continue;


			if (BotType != BOT_ANIMAL) // Enemies can spawn underground or outside
			{
				P = vec2(int_rand(StartX, EndX), int_rand(StartY, EndY));
				P = vec2(P.x*32.0f + 16.0f, P.y*32.0f + 16.0f);

				if (GameServer()->Collision()->GetCollisionAt(P.x-32, P.y) != 0 ||
					GameServer()->Collision()->GetCollisionAt(P.x+32, P.y) != 0 ||
					GameServer()->Collision()->GetCollisionAt(P.x, P.y-32) != 0)
					return;
			}
			else // Animals only spawn in the outside
			{
				bool found = false;
				do
				{
					found = false;
					P = vec2(int_rand(StartX, EndX), 0);
					P = vec2(P.x*32.0f + 16.0f, 16.0f);
					const int TotalH = GameServer()->Collision()->GetHeight()*32;
					for (int i=P.y; i<TotalH-1; i+=32)
					{
						if (GameServer()->Collision()->CheckPoint(vec2(P.x, i), true))
						{
							P.y = i-32;
							found = true;
							break;
						}
					}
				} while (!found);
			}

			int TileIndex = GameServer()->Collision()->GetMineTeeTileIndexAt(P);
			if (GameServer()->Collision()->GetCollisionAt(P.x, P.y+32) == CCollision::COLFLAG_SOLID
					&& !GameServer()->m_BlockManager.IsFluid(TileIndex))
			{
				float S = EvaluateSpawnPos(pEval, P);
				if(!pEval->m_Got || pEval->m_Score > S)
				{
					pEval->m_Got = true;
					pEval->m_Score = S;
					P.y -= 8; // Be secure that not spawn stuck
					pEval->m_Pos = P;
				}
			}

			if (pEval->m_Got)
			{
				// Other bots/players near?
				CCharacter *aEnts[MAX_BOTS];
				int Num = GameServer()->m_World.FindEntities(P, 1200, (CEntity**)aEnts, MAX_BOTS, CGameWorld::ENTTYPE_CHARACTER);
				if (BotType != BOT_ANIMAL)
				{
					for (int i=0; i<Num; i++)
					{
						if (aEnts[i]->GetPlayer() && aEnts[i]->GetPlayer()->IsBot())
						{
							pEval->m_Got = false;
							return;
						}
					}
				}
				else if (Num)
				{
					pEval->m_Got = false;
					return;
				}

				// Good Light?
				UpdateLayerLights(P); // TODO: Can decrease performance... because slow light implementation :/
				CTile *pMTLTiles = GameServer()->Layers()->TileLights();
				int TileLightIndex = static_cast<int>(P.y/32)*GameServer()->Layers()->Lights()->m_Width+static_cast<int>(P.x/32);
				if (BotType == BOT_ANIMAL && pMTLTiles[TileLightIndex].m_Index != 0)
					pEval->m_Got = false;
				else if (BotType != BOT_ANIMAL && pMTLTiles[TileLightIndex].m_Index != 202)
					pEval->m_Got = false;
			}
		}
	}
	else
	{
		bool found = false;
		do
		{
			found = false;
			P = vec2(rand()%GameServer()->Collision()->GetWidth(), 0);
			P = vec2(P.x*32.0f + 16.0f, 16.0f);
			const int TotalH = GameServer()->Collision()->GetHeight()*32;
			for (int i=P.y; i<TotalH-1; i+=32)
			{
				if (GameServer()->Collision()->CheckPoint(vec2(P.x, i), true))
				{
					P.y = i-32;
					found = true;
					break;
				}
			}
		} while (!found);

		int TileIndex = GameServer()->Collision()->GetMineTeeTileIndexAt(P);
		if (GameServer()->Collision()->GetCollisionAt(P.x, P.y+32) == CCollision::COLFLAG_SOLID
				&& !GameServer()->m_BlockManager.IsFluid(TileIndex))
		{
			float S = EvaluateSpawnPos(pEval, P);
			if(!pEval->m_Got || pEval->m_Score > S)
			{
				pEval->m_Got = true;
				pEval->m_Score = S;
				P.y -= 8; // Be secure that not spawn stuck
				pEval->m_Pos = P;
			}
		}
	}
}

// FIXME: Duplicated from MapGen
void CGameControllerMineTee::GenerateTree(ivec2 Pos)
{
	// plant the tree \o/
	int TreeHeight = GameServer()->MapGen()->GetNoise()->Perlin()->GetURandom(4,5);
	for(int h = 0; h <= TreeHeight; h++)
		ModifTile(ivec2(Pos.x, Pos.y-h), GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::WOOD_BROWN);

	// add the leafs
	for(int l = 0; l <= TreeHeight/2.5f; l++)
		ModifTile(ivec2(Pos.x, Pos.y - TreeHeight - l), GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::LEAFS);

	int TreeWidth = TreeHeight/1.2f;
	if(!(TreeWidth%2)) // odd numbers please
		TreeWidth++;
	for(int h = 0; h <= TreeHeight/2; h++)
	{
		for(int w = 0; w < TreeWidth; w++)
		{
			if(GameServer()->Collision()->GetMineTeeTileIndexAt(vec2((Pos.x-(w-(TreeWidth/2)))*32, (Pos.y-(TreeHeight-(TreeHeight/2.5f)+h))*32)) != CBlockManager::AIR)
				continue;

			ModifTile(ivec2(Pos.x-(w-(TreeWidth/2)), Pos.y-(TreeHeight-(TreeHeight/2.5f)+h)), GameServer()->Layers()->GetMineTeeLayerIndex(), CBlockManager::LEAFS);
		}
	}
}
//

void CGameControllerMineTee::ModifTile(ivec2 MapPos, int TileIndex, int Reserved)
{
	if (Reserved == -1)
	{
		CBlockManager::CBlockInfo *pBlockInfo = GameServer()->m_BlockManager.GetBlockInfo(TileIndex);
		Reserved = pBlockInfo->m_Health;
	}

	GameServer()->SendTileModif(ALL_PLAYERS, MapPos, GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), TileIndex, 0, Reserved);
	GameServer()->Collision()->ModifTile(MapPos, GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), TileIndex, 0, Reserved);
}

bool CGameControllerMineTee::GetPlayerArea(int ClientID, int *pStartX, int *pEndX, int *pStartY, int *pEndY)
{
	const int PlayerRadius = 24; // In Tiles
	*pStartX = *pEndX = *pStartY = *pEndY = 0;
	if (ClientID < 0 || ClientID >= MAX_CLIENTS || !GameServer()->m_apPlayers[ClientID] || !GameServer()->m_apPlayers[ClientID]->GetCharacter())
		return false;

	const vec2 ClientWorldPos = GameServer()->m_apPlayers[ClientID]->GetCharacter()->m_Pos;
	const ivec2 ClientMapPos(ClientWorldPos.x/32, ClientWorldPos.y/32);

	*pStartX = max(ClientMapPos.x-PlayerRadius, 1);
	*pEndX = min(ClientMapPos.x+PlayerRadius, GameServer()->Collision()->GetWidth()-2);
	*pStartY = max(ClientMapPos.y-PlayerRadius, 1);
	*pEndY = min(ClientMapPos.y+PlayerRadius, GameServer()->Collision()->GetHeight()-2);
	return true;
}

void CGameControllerMineTee::GetPlayerArea(vec2 Pos, int *pStartX, int *pEndX, int *pStartY, int *pEndY)
{
	const int MapChunkSize = 24;
	*pStartX = *pEndX = *pStartY = *pEndY = 0;
	const ivec2 ClientMapPos(Pos.x/32, Pos.y/32);

	*pStartX = max(ClientMapPos.x-MapChunkSize, 1);
	*pEndX = min(ClientMapPos.x+MapChunkSize, GameServer()->Collision()->GetWidth()-2);
	*pStartY = max(ClientMapPos.y-MapChunkSize, 1);
	*pEndY = min(ClientMapPos.y+MapChunkSize, GameServer()->Collision()->GetHeight()-2);
}

void CGameControllerMineTee::UpdateLayerLights(vec2 Pos)
{
	//CTile *pMTLTiles = 0x0;
	static int s_LightLevel = 0;
	int64 Time = -1;
	bool IsDay = false;

	GameServer()->GetServerTime(&IsDay, &Time);

	float tt = Time;
	int itt = (int)tt%(int)GameServer()->m_World.m_Core.m_Tuning.m_DayNightDuration;
	tt = itt/GameServer()->m_World.m_Core.m_Tuning.m_DayNightDuration;

	if (tt < 0.01f)
		s_LightLevel=(IsDay)?4:0;
	else if (tt < 0.02f)
		s_LightLevel=(IsDay)?3:1;
	else if (tt < 0.03f)
		s_LightLevel=(IsDay)?2:2;
	else if (tt < 0.04f)
		s_LightLevel=(IsDay)?1:3;
	else
		s_LightLevel=(IsDay)?0:4;


	if (s_LightLevel >= 0)
	{
		int ScreenX0, ScreenY0, ScreenX1, ScreenY1;
		GetPlayerArea(Pos, &ScreenX0, &ScreenX1, &ScreenY0, &ScreenY1);
		GameServer()->Collision()->UpdateLayerLights((float)ScreenX0*32.0f, (float)ScreenY0*32.0f, (float)ScreenX1*32.0f, (float)ScreenY1*32.0f, s_LightLevel);
	}
}

bool CGameControllerMineTee::TakeBlockDamage(vec2 WorldPos, int WeaponItemID, int Dmg, int Owner)
{
	CTile *pTile = GameServer()->Collision()->GetMineTeeTileAt(WorldPos);
	if (pTile && pTile->m_Index > 0)
	{
		pTile->m_Reserved = max(0, pTile->m_Reserved-Dmg);

		const ivec2 TilePos = ivec2(WorldPos.x/32, WorldPos.y/32);
		if (pTile->m_Reserved == 0)
		{
			if (WeaponItemID < NUM_WEAPONS)
				OnPlayerDestroyBlock(Owner, TilePos);
			return true;
		}
		else if (pTile->m_Reserved > 0)
		{
			ModifTile(TilePos, pTile->m_Index, pTile->m_Reserved);
			GameServer()->CreateSound(WorldPos, SOUND_DESTROY_BLOCK);
		}
	}

	return false;
}

void CGameControllerMineTee::OnClientMoveCell(int ClientID, int From, int To, unsigned char Qty)
{
	CCharacter *pChar = GameServer()->GetPlayerChar(ClientID);
	if (!pChar || pChar->m_ActiveBlockId == -1 || Qty == 0 || From == To)
		return;

	const int x = pChar->m_ActiveBlockId%GameServer()->Collision()->GetWidth();
	const int y = pChar->m_ActiveBlockId/GameServer()->Collision()->GetWidth();
	const vec2 BlockPos = vec2(x*32.0f+16.0f, y*32.0f+16.0f);

	int MTTIndex = GameServer()->Collision()->GetMineTeeTileIndexAt(BlockPos);
	CBlockManager::CBlockInfo *pBlockInfo = GameServer()->m_BlockManager.GetBlockInfo(MTTIndex);
	if (!pBlockInfo)
	{
		SendInventory(ClientID, false);
		return;
	}

	const bool IsCraftTable = (str_comp(pBlockInfo->m_Functionality.m_aType, "craft-table") == 0);
	CPlayer *pPlayer = pChar->GetPlayer();
	CCellData *pCellFrom = 0x0;
	CCellData *pCellTo = 0x0;
	bool IsFromCraftRes = false;
	bool IsToCraftZone = false;

	if (From < NUM_CELLS_LINE)
		pCellFrom = &pChar->m_FastInventory[From];
	else
	{
		if (pChar->m_ActiveBlockId == -2 || IsCraftTable)
		{
			if (From >= NUM_CELLS_LINE*4)
			{
				const int FromID = From-NUM_CELLS_LINE*4;
				if (FromID == NUM_CELLS_LINE)
					IsFromCraftRes = true;
				pCellFrom = &pPlayer->m_aCraft[FromID];
			}
			else
				pCellFrom = &pPlayer->m_aInventory[From-NUM_CELLS_LINE];
		}
		else
		{
			std::map<int, CChest*>::iterator it = m_lpChests.find(pChar->m_ActiveBlockId);
			if (it != m_lpChests.end())
				pCellFrom = &it->second->m_apItems[From-NUM_CELLS_LINE];
		}
	}

	if (To != -1)
	{
		if (To < NUM_CELLS_LINE)
			pCellTo = &pChar->m_FastInventory[To];
		else
		{
			if (pChar->m_ActiveBlockId == -2 || IsCraftTable)
			{
				if (To >= NUM_CELLS_LINE*4)
				{
					IsToCraftZone = true;
					const int ToID = To-NUM_CELLS_LINE*4;
					if (ToID == NUM_CELLS_LINE)
					{
						SendInventory(ClientID, IsCraftTable);
						return;
					}
					pCellTo = &pPlayer->m_aCraft[To-NUM_CELLS_LINE*4];
				}
				else
					pCellTo = &pPlayer->m_aInventory[To-NUM_CELLS_LINE];
			}
			else
			{
				std::map<int, CChest*>::iterator it = m_lpChests.find(pChar->m_ActiveBlockId);
				if (it != m_lpChests.end())
					pCellTo = &it->second->m_apItems[To-NUM_CELLS_LINE];
			}
		}
	}

	if (IsFromCraftRes && IsToCraftZone)
	{
		SendInventory(ClientID, IsCraftTable);
		return;
	}

	if (pCellFrom && pCellTo)
	{
		if (pCellFrom->m_ItemId < NUM_WEAPONS)
		{
			CCellData TempCell = *pCellTo;
			*pCellTo = *pCellFrom;
			*pCellFrom = TempCell;
		}
		else
		{
			if (pCellTo->m_ItemId == pCellFrom->m_ItemId || pCellTo->m_ItemId == 0)
			{
				unsigned NewAmount = pCellTo->m_Amount+Qty;
				if (NewAmount > 255)
				{
					CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_DROPITEM, pCellFrom->m_ItemId);
					pPickup->m_Pos = pChar->m_Pos;
					pPickup->m_Pos.y -= 18.0f;
					pPickup->m_Vel = vec2((((rand()%2)==0)?1:-1)*(rand()%10), -5);
					pPickup->m_Amount = NewAmount-255;
					pPickup->m_Owner = ClientID;
				}
				pCellTo->m_ItemId = pCellFrom->m_ItemId;
				pCellTo->m_Amount = min(255, pCellTo->m_Amount+Qty);
				pCellFrom->m_Amount -= Qty;
				if (pCellFrom->m_Amount == 0)
				{
					pCellFrom->m_ItemId = 0x0;
					pCellFrom->m_Amount = 0;
				}
			}
			else if (pCellFrom->m_Amount == Qty)
			{
				CCellData TempCell = *pCellTo;
				*pCellTo = *pCellFrom;
				*pCellFrom = TempCell;
			}
			else
			{
				// TODO: SEND MOVE FAIL
				SendInventory(ClientID, IsCraftTable);
				return;
			}
		}

		if (IsFromCraftRes)
		{
			for (int i=0; i<NUM_CELLS_LINE; i++)
			{
				if (pPlayer->m_aCraftRecipe[i].m_ItemId == 0)
					continue;

				if (pPlayer->m_aCraftRecipe[i].m_ItemId == pPlayer->m_aCraft[i].m_ItemId)
				{
					pPlayer->m_aCraft[i].m_Amount = max(0, pPlayer->m_aCraft[i].m_Amount-pPlayer->m_aCraftRecipe[i].m_Amount);
					if (pPlayer->m_aCraft[i].m_Amount == 0)
						pPlayer->m_aCraft[i].m_ItemId = 0;
				}
			}
		}
	}
	else if (pCellFrom && !pCellTo && !IsFromCraftRes && pCellFrom->m_ItemId != 0)
	{
		CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_DROPITEM, pCellFrom->m_ItemId);
		pPickup->m_Pos = pChar->m_Pos;
		pPickup->m_Pos.y -= 18.0f;
		pPickup->m_Vel = vec2((((rand()%2)==0)?1:-1)*(rand()%10), -5);
		pPickup->m_Amount = pCellFrom->m_Amount;
		pPickup->m_Owner = ClientID;

		pCellFrom->m_ItemId = 0x0;
		pCellFrom->m_Amount = 0;
	}

	if (pChar->m_ActiveBlockId == -2 || IsCraftTable)
	{
		CheckCraft(ClientID);
		SendInventory(ClientID, IsCraftTable);
	}

	pChar->m_NeedSendFastInventory = true;
}

void CGameControllerMineTee::CheckCraft(int ClientID)
{
	CCharacter *pChar = GameServer()->GetPlayerChar(ClientID);
	if (!pChar)
		return;

	CPlayer *pPlayer = pChar->GetPlayer();

	bool DoCraft = false;
	for (int q=0; q<CBlockManager::MAX_BLOCKS; q++)
	{
		mem_zero(pPlayer->m_aCraftRecipe, sizeof(CCellData)*NUM_CELLS_LINE);

		CBlockManager::CBlockInfo *pBlockInfo = GameServer()->m_BlockManager.GetBlockInfo(q);
		if (!pBlockInfo)
			continue;

		bool NoCrafts = false;
		for (int i=0; i<NUM_CELLS_LINE; i++)
		{
			if (pBlockInfo->m_vCraft[i].size() == 0)
			{
				NoCrafts = true;
				break;
			}

			std::map<int, unsigned char>::iterator it = pBlockInfo->m_vCraft[i].begin();
			while (it != pBlockInfo->m_vCraft[i].end())
			{
				if (it->first > 0 && pChar->GetPlayer()->m_aCraft[i].m_ItemId == 0)
				{
					pPlayer->m_aCraftRecipe[i].m_ItemId = it->first+NUM_WEAPONS;
					pPlayer->m_aCraftRecipe[i].m_Amount = it->second;
				}
				else if (it->first+NUM_WEAPONS == pChar->GetPlayer()->m_aCraft[i].m_ItemId
					&& it->second <= pChar->GetPlayer()->m_aCraft[i].m_Amount)
				{
					pPlayer->m_aCraftRecipe[i].m_ItemId = it->first+NUM_WEAPONS;
					pPlayer->m_aCraftRecipe[i].m_Amount = it->second;
					break;
				}

				++it;
			}
		}

		if (NoCrafts)
			continue;

		bool Crafted = true;
		for (int i=0; i<NUM_CELLS_LINE; i++)
		{
			if (pPlayer->m_aCraftRecipe[i].m_ItemId != pChar->GetPlayer()->m_aCraft[i].m_ItemId)
			{
				Crafted = false;
				break;
			}
		}

		// q=0, ommit Air
		if (Crafted && q != 0)
		{
			pChar->GetPlayer()->m_aCraft[NUM_CELLS_LINE].m_ItemId = q+NUM_WEAPONS;
			pChar->GetPlayer()->m_aCraft[NUM_CELLS_LINE].m_Amount = pBlockInfo->m_CraftNum;
			DoCraft = true;
			break;
		}
		else
		{
			mem_zero(pPlayer->m_aCraftRecipe, sizeof(CCellData)*NUM_CELLS_LINE);
			pChar->GetPlayer()->m_aCraft[NUM_CELLS_LINE].m_ItemId = 0;
			pChar->GetPlayer()->m_aCraft[NUM_CELLS_LINE].m_Amount = 0;
			if (q == 0)
				break;
		}
	}

	// TODO: Only for pre-alpha... need be changed to not hard-coded solution
	if (!DoCraft)
	{ // It's a weapon recipe?
		static const CCellData s_WeaponsRecipes[NUM_WEAPONS][NUM_CELLS_LINE] =
		{
			{ {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0} }, // None
			{ {4,1}, {4,1}, {4,1}, {0,0}, {40,1}, {0,0}, {0,0}, {40,1}, {0,0} }, // Hammer Wood
			{ {212,4}, {212,4}, {212,4}, {0,0}, {4,2}, {0,0}, {4,2}, {0,0}, {0,0} }, // Pistol
			{ {212,8}, {212,8}, {212,8}, {0,0}, {4,2}, {212,8}, {4,2}, {0,0}, {0,0} }, // Shotgun
			{ {212,4}, {212,4}, {212,4}, {212,4}, {212,4}, {212,4}, {31,2}, {4,2}, {31,2} }, // Grenade Launcher
			{ {213,4}, {213,4}, {213,4}, {0,0}, {212,1}, {0,0}, {212,1}, {0,0}, {0,0} }, // Rifle
			{ {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0}, {0,0} }, // Ninja
			{ {1,2}, {1,2}, {1,2}, {0,0}, {40,1}, {0,0}, {0,0}, {40,1}, {0,0} }, // Hammer Stone
			{ {212,1}, {212,1}, {212,1}, {0,0}, {40,1}, {0,0}, {0,0}, {40,1}, {0,0} }, // Hammer Iron
		};
		static const int s_Amounts[NUM_WEAPONS] = { 0, 255, 200, 150, 25, 15, 0, 255, 255 };

		for (int q=0; q<NUM_WEAPONS; q++)
		{
			mem_zero(pPlayer->m_aCraftRecipe, sizeof(CCellData)*NUM_CELLS_LINE);
			for (int i=0; i<NUM_CELLS_LINE; i++)
			{
				if (s_WeaponsRecipes[q][i].m_ItemId > 0 && pChar->GetPlayer()->m_aCraft[i].m_ItemId == 0)
				{
					pPlayer->m_aCraftRecipe[i].m_ItemId = s_WeaponsRecipes[q][i].m_ItemId+NUM_WEAPONS;
					pPlayer->m_aCraftRecipe[i].m_Amount = s_WeaponsRecipes[q][i].m_Amount;
				}
				else if (s_WeaponsRecipes[q][i].m_ItemId+NUM_WEAPONS == pChar->GetPlayer()->m_aCraft[i].m_ItemId
					&& s_WeaponsRecipes[q][i].m_Amount <= pChar->GetPlayer()->m_aCraft[i].m_Amount)
				{
					pPlayer->m_aCraftRecipe[i].m_ItemId = s_WeaponsRecipes[q][i].m_ItemId+NUM_WEAPONS;
					pPlayer->m_aCraftRecipe[i].m_Amount = s_WeaponsRecipes[q][i].m_Amount;
				}
			}

			bool Crafted = true;
			for (int i=0; i<NUM_CELLS_LINE; i++)
			{
				if (pPlayer->m_aCraftRecipe[i].m_ItemId != pChar->GetPlayer()->m_aCraft[i].m_ItemId)
				{
					Crafted = false;
					break;
				}
			}

			if (Crafted)
			{
				pChar->GetPlayer()->m_aCraft[NUM_CELLS_LINE].m_ItemId = q;
				pChar->GetPlayer()->m_aCraft[NUM_CELLS_LINE].m_Amount = s_Amounts[q];
				break;
			}
			else
			{
				pChar->GetPlayer()->m_aCraft[NUM_CELLS_LINE].m_ItemId = 0;
				pChar->GetPlayer()->m_aCraft[NUM_CELLS_LINE].m_Amount = 0;
			}
		}
	}
}

void CGameControllerMineTee::LoadData()
{
	char aBuf[512];
	unsigned NumRegs=0;

	str_format(aBuf, sizeof(aBuf), "worlds/%s/minetee.dat", Server()->GetWorldName());
	IOHANDLE File = GameServer()->Storage()->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_SAVE);
	if (!File)
		return;

	// Get file version
	short Version = 0;
	io_read(File, &Version, sizeof(short));
	if (Version != 1)
		return;

	unsigned ItemCount = 0;
	io_read(File, &ItemCount, sizeof(unsigned));

	unsigned ItemType = 0;
	for (unsigned q=0; q<ItemCount; q++)
	{
		io_read(File, &ItemType, sizeof(unsigned));
		io_read(File, &NumRegs, sizeof(unsigned));

		if (NumRegs == 0)
			continue;

		if (ItemType == TYPE_CHESTS)
		{
			int ChestID;
			for (unsigned i=0; i<NumRegs; i++)
			{
				CChest *pStoredChest = (CChest*)mem_alloc(sizeof(CChest), 1);
				io_read(File, &ChestID, sizeof(int)); // Get ID
				io_read(File, pStoredChest, sizeof(CChest)); // Get base chest
				pStoredChest->m_apItems = (CCellData*)mem_alloc(sizeof(CCellData)*pStoredChest->m_NumItems, 1);
				mem_zero(pStoredChest->m_apItems, sizeof(pStoredChest->m_apItems));
				io_read(File, pStoredChest->m_apItems, sizeof(CCellData)*pStoredChest->m_NumItems); // Get Items
				m_lpChests.insert(std::make_pair(ChestID, pStoredChest));
			}
		}
		else if (ItemType == TYPE_SIGNS)
		{
			int SignID;
			CSign StoredSign(0);
			for (unsigned i=0; i<NumRegs; i++)
			{
				io_read(File, &SignID, sizeof(int)); // Get ID
				io_read(File, &StoredSign, sizeof(CSign)); // Get base sign
				m_lpSigns.insert(std::make_pair(SignID, StoredSign));
			}
		}
	}

	io_close(File);
}

void CGameControllerMineTee::SaveData()
{
	char aBuf[512];
	unsigned NumRegs = 0;
	unsigned ItemType = 0;

	str_format(aBuf, sizeof(aBuf), "worlds/%s/minetee.dat", Server()->GetWorldName());
	IOHANDLE File = GameServer()->Storage()->OpenFile(aBuf, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if (!File)
		return;

	// Save file version
	short Version = 1;
	io_write(File, &Version, sizeof(short));

	// Save item count
	unsigned ItemCount = 2;
	io_write(File, &ItemCount, sizeof(unsigned));

	// Save Chests
	ItemType = TYPE_CHESTS;
	io_write(File, &ItemType, sizeof(unsigned));
	NumRegs = m_lpChests.size();
	io_write(File, &NumRegs, sizeof(unsigned));
	std::map<int, CChest*>::iterator itChest = m_lpChests.begin();
	while (itChest != m_lpChests.end())
	{
		io_write(File, &itChest->first, sizeof(int)); // The ID
		io_write(File, itChest->second, sizeof(CChest)); // The base info
		io_write(File, itChest->second->m_apItems, sizeof(CCellData)*itChest->second->m_NumItems); // Chest Items
		++itChest;
	}

	// Save Signs
	ItemType = TYPE_SIGNS;
	io_write(File, &ItemType, sizeof(unsigned));
	NumRegs = m_lpSigns.size();
	io_write(File, &NumRegs, sizeof(unsigned));
	std::map<int, CSign>::iterator itSign = m_lpSigns.begin();
	while (itSign != m_lpSigns.end())
	{
		io_write(File, &itSign->first, sizeof(int)); // The ID
		io_write(File, &itSign->second, sizeof(CSign)); // The base sign
		++itSign;
	}


	io_close(File);
}
