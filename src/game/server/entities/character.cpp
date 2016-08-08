/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* File modified by Alexandre Díaz */
#include <new>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>

#include "pickup.h" // MineTee
#include "character.h"
#include "laser.h"
#include "projectile.h"

//input count
struct CInputCount
{
	int m_Presses;
	int m_Releases;
};

CInputCount CountInput(int Prev, int Cur)
{
	CInputCount c = {0, 0};
	Prev &= INPUT_STATE_MASK;
	Cur &= INPUT_STATE_MASK;
	int i = Prev;

	while(i != Cur)
	{
		i = (i+1)&INPUT_STATE_MASK;
		if(i&1)
			c.m_Presses++;
		else
			c.m_Releases++;
	}

	return c;
}


MACRO_ALLOC_POOL_ID_IMPL(CCharacter, MAX_CLIENTS)

// Character, "physical" player's part
CCharacter::CCharacter(CGameWorld *pWorld)
: CEntity(pWorld, CGameWorld::ENTTYPE_CHARACTER)
{
	m_ProximityRadius = ms_PhysSize;
	m_Health = 0;
	m_Armor = 0;

	// MineTee
    m_NeedSendFastInventory = true;
	m_TimerFluidDamage = Server()->Tick();
	m_InWater = false;
	m_TimeStuck = Server()->Tick();
	m_ActiveBlockId = -1;
    //
}

void CCharacter::Reset()
{
	Destroy();
}

bool CCharacter::Spawn(CPlayer *pPlayer, vec2 Pos)
{
	// MineTee
	mem_zero(m_FastInventory, sizeof(CCellData)*NUM_CELLS_LINE);
    //

	m_EmoteStop = -1;
	m_LastAction = -1;
	m_LastNoAmmoSound = -1;
	m_ActiveInventoryItem = 0;
	m_LastInventoryItem = 0;
	m_QueuedInventoryItem = -1;

	m_pPlayer = pPlayer;
	m_Pos = Pos;

	m_Core.Reset();
	m_Core.Init(&GameServer()->m_World.m_Core, GameServer()->Collision());
	m_Core.m_Pos = m_Pos;
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = &m_Core;

	m_ReckoningTick = 0;
	mem_zero(&m_SendCore, sizeof(m_SendCore));
	mem_zero(&m_ReckoningCore, sizeof(m_ReckoningCore));

	GameServer()->m_World.InsertEntity(this);
	m_Alive = true;

	GameServer()->m_pController->OnCharacterSpawn(this);

	if (m_pPlayer->m_IsFirstJoin)
	{
		IAccountSystem::ACCOUNT_INFO *pAccountInfo = GameServer()->GetAccount(m_pPlayer->GetCID());
        if (pAccountInfo && pAccountInfo->m_Alive)
        {
        	UseAccountData(pAccountInfo);
        	if (pAccountInfo->m_PetInfo.m_Alive)
        	{
        		CPet *pPet = GameServer()->SpawnPet(GetPlayer(), pAccountInfo->m_PetInfo.m_Pos);
        		if (pPet)
        			pPet->UseAccountPetData(&pAccountInfo->m_PetInfo);
        		else
        			GameServer()->SendChatTarget(GetPlayer()->GetCID(), "Oops! I need fix this! Sry but your pet is lost :/");
        	}
        }

        m_NeedSendFastInventory = true;
		m_pPlayer->m_IsFirstJoin = false;
	}

	return true;
}

void CCharacter::Destroy()
{
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	m_Alive = false;
}

void CCharacter::SetInventoryItem(int Index)
{
	if(Index == m_ActiveInventoryItem)
		return;

	if (m_FastInventory[Index].m_ItemId != 0)
	{
		m_LastInventoryItem = m_ActiveInventoryItem;
		m_QueuedInventoryItem = -1;
		m_ActiveInventoryItem = Index;
		GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH);
		m_NeedSendFastInventory = true;
		return;
	}
}

bool CCharacter::IsGrounded()
{
	if(GameServer()->Collision()->CheckPoint(m_Pos.x+m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	if(GameServer()->Collision()->CheckPoint(m_Pos.x-m_ProximityRadius/2, m_Pos.y+m_ProximityRadius/2+5))
		return true;
	return false;
}


void CCharacter::DoInventoryItemSwitch()
{
	// make sure we can switch
	if(m_ReloadTimer != 0 || m_QueuedInventoryItem == -1)
		return;

	// switch Weapon
	SetInventoryItem(m_QueuedInventoryItem);
}

void CCharacter::HandleInventoryItemSwitch()
{
	int WantedInventoryItem = m_ActiveInventoryItem;
	if(m_QueuedInventoryItem != -1)
		WantedInventoryItem = m_QueuedInventoryItem;

	// select Item
	int Next = CountInput(m_LatestPrevInput.m_NextInventoryItem, m_LatestInput.m_NextInventoryItem).m_Presses;
	int Prev = CountInput(m_LatestPrevInput.m_PrevInventoryItem, m_LatestInput.m_PrevInventoryItem).m_Presses;

	if(Next < 128) // make sure we only try sane stuff
	{
		while(Next) // Next Weapon selection
		{
			for (int i=0; i<NUM_CELLS_LINE; i++)
			{
				++WantedInventoryItem;
				if (WantedInventoryItem > NUM_CELLS_LINE-1)
					WantedInventoryItem = 0;

				if (m_FastInventory[WantedInventoryItem].m_ItemId)
					break;
			}

			Next--;
		}
	}

	if(Prev < 128) // make sure we only try sane stuff
	{
		while(Prev) // Prev Weapon selection
		{
			for (int i=0; i<NUM_CELLS_LINE; i++)
			{
				--WantedInventoryItem;
				if (WantedInventoryItem < 0)
					WantedInventoryItem = NUM_CELLS_LINE-1;

				if (m_FastInventory[WantedInventoryItem].m_ItemId)
					break;
			}

			Prev--;
		}
	}

	// Direct Weapon selection
	if(m_LatestInput.m_WantedInventoryItem)
		WantedInventoryItem = m_Input.m_WantedInventoryItem-1;

	// check for insane values
	if(WantedInventoryItem != m_ActiveInventoryItem && m_FastInventory[WantedInventoryItem].m_ItemId != 0)
	{
		m_QueuedInventoryItem = WantedInventoryItem;
		m_NeedSendFastInventory = true;
	}

	DoInventoryItemSwitch();
}

// MineTee
void CCharacter::Construct()
{
	if(m_ReloadTimer != 0)
		return;

    bool Builded = false;
    const int ActiveItem = m_FastInventory[m_ActiveInventoryItem].m_ItemId;
    const int ActiveBlock = ActiveItem-NUM_WEAPONS;

    DoInventoryItemSwitch();
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(!WillFire)
		return;

    vec2 ProjStartPos= m_Pos + Direction * 38.0f;
    vec2 colTilePos = ProjStartPos+Direction * MAX_CONSTRUCT_DISTANCE;
    const float Zoom = 1.35f;

    if (m_pPlayer->m_PlayerFlags&PLAYERFLAG_BGPAINT)
    {
        ivec2 TilePos = ivec2((m_Pos.x+m_LatestInput.m_TargetX*Zoom)/32.0f, (m_Pos.y+m_LatestInput.m_TargetY*Zoom)/32.0f);
        if (TilePos.x > 0 && TilePos.x < GameServer()->Layers()->MineTeeLayer()->m_Width && TilePos.y > 0 && TilePos.y < GameServer()->Layers()->MineTeeLayer()->m_Height)
        {
            int Index = TilePos.y*GameServer()->Layers()->MineTeeLayer()->m_Width+TilePos.x;
            CTile *pMTTiles = (CTile *)GameServer()->Layers()->Map()->GetData(GameServer()->Layers()->MineTeeLayer()->m_Data);
            CTile *pMTBGTiles = (CTile *)GameServer()->Layers()->Map()->GetData(GameServer()->Layers()->MineTeeBGLayer()->m_Data);
            if (pMTTiles[Index].m_Index != 0 || pMTBGTiles[Index].m_Index == ActiveBlock)
                return;

            if (GameServer()->Collision()->ModifTile(TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeBGLayerIndex(), (ActiveItem == WEAPON_HAMMER)?0:ActiveBlock, 0, 0))
            {
            	GameServer()->SendTileModif(ALL_PLAYERS, TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeBGLayerIndex(), (ActiveItem == WEAPON_HAMMER)?0:ActiveBlock, 0, 0);
            	GameServer()->CreateSound(m_Pos, SOUND_DESTROY_BLOCK);
            	Builded = true;
            }
        }
    }
    else if (m_pPlayer->m_PlayerFlags&PLAYERFLAG_FGPAINT)
    {
        ivec2 TilePos = ivec2((m_Pos.x+m_LatestInput.m_TargetX*Zoom)/32.0f, (m_Pos.y+m_LatestInput.m_TargetY*Zoom)/32.0f);
        if (TilePos.x > 0 && TilePos.x < GameServer()->Layers()->MineTeeLayer()->m_Width && TilePos.y > 0 && TilePos.y < GameServer()->Layers()->MineTeeLayer()->m_Height)
        {
            int Index = TilePos.y*GameServer()->Layers()->MineTeeLayer()->m_Width+TilePos.x;
            CTile *pMTTiles = (CTile *)GameServer()->Layers()->Map()->GetData(GameServer()->Layers()->MineTeeLayer()->m_Data);
            CTile *pMTFGTiles = (CTile *)GameServer()->Layers()->Map()->GetData(GameServer()->Layers()->MineTeeFGLayer()->m_Data);
            if (pMTTiles[Index].m_Index != 0 || pMTFGTiles[Index].m_Index == ActiveBlock)
                return;

            if (GameServer()->Collision()->ModifTile(TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeFGLayerIndex(), (ActiveItem == WEAPON_HAMMER)?0:ActiveBlock, 0, 0))
            {
            	GameServer()->CreateSound(m_Pos, SOUND_DESTROY_BLOCK);
            	GameServer()->SendTileModif(ALL_PLAYERS, TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeFGLayerIndex(), (ActiveItem == WEAPON_HAMMER)?0:ActiveBlock, 0, 0);
            	Builded = true;
            }
        }
    }
    else if (GameServer()->Collision()->IntersectLine(ProjStartPos, colTilePos, &colTilePos, 0x0))
    {
        vec2 finishPosPost = colTilePos-Direction * 8.0f;
        if (!(GameServer()->Collision()->GetCollisionAt(finishPosPost.x, finishPosPost.y)&CCollision::COLFLAG_SOLID))
        {
            ivec2 TilePos(finishPosPost.x/32, finishPosPost.y/32);
            CBlockManager::CBlockInfo *pBlockInfo = GameServer()->m_BlockManager.GetBlockInfo(ActiveBlock);
            int TileIndex = pBlockInfo->m_OnPut;
            if (TileIndex == -1)
            	return;

            //Check player stuck
            for (int i=0; i<MAX_CLIENTS; i++)
            {
                if (!GameServer()->m_apPlayers[i])
                    continue;

                CCharacter *pChar = GameServer()->m_apPlayers[i]->GetCharacter();
                if (!pChar || !pChar->IsAlive())
                    continue;

                if (distance(ivec2(pChar->m_Pos.x/32, pChar->m_Pos.y/32), TilePos) < 2)
                    return;
            }

            //check blocks
            unsigned char DenyBlocks[] = { CBlockManager::TORCH };
            CTile *pMTTiles = (CTile *)GameServer()->Layers()->Map()->GetData(GameServer()->Layers()->MineTeeLayer()->m_Data);
            for (size_t i=0; i<sizeof(DenyBlocks); i++)
            {
                if (ActiveBlock != DenyBlocks[i])
                    continue;

                int Index = (TilePos.y-1)*GameServer()->Layers()->MineTeeLayer()->m_Width+TilePos.x;
                if (pMTTiles[Index].m_Index == DenyBlocks[i])
                    return;
                Index = (TilePos.y+1)*GameServer()->Layers()->MineTeeLayer()->m_Width+TilePos.x;
                if (pMTTiles[Index].m_Index == DenyBlocks[i])
                    return;
                Index = TilePos.y*GameServer()->Layers()->MineTeeLayer()->m_Width+(TilePos.x-1);
                if (pMTTiles[Index].m_Index == DenyBlocks[i])
                    return;
                Index = TilePos.y*GameServer()->Layers()->MineTeeLayer()->m_Width+(TilePos.x+1);
                if (pMTTiles[Index].m_Index == DenyBlocks[i])
                    return;
                Index = (TilePos.y-1)*GameServer()->Layers()->MineTeeLayer()->m_Width+(TilePos.x-1);
                if (pMTTiles[Index].m_Index == DenyBlocks[i])
                    return;
                Index = (TilePos.y+1)*GameServer()->Layers()->MineTeeLayer()->m_Width+(TilePos.x+1);
                if (pMTTiles[Index].m_Index == DenyBlocks[i])
                    return;
                Index = (TilePos.y+1)*GameServer()->Layers()->MineTeeLayer()->m_Width+(TilePos.x-1);
                if (pMTTiles[Index].m_Index == DenyBlocks[i])
                    return;
                Index = (TilePos.y-1)*GameServer()->Layers()->MineTeeLayer()->m_Width+(TilePos.x+1);
                if (pMTTiles[Index].m_Index == DenyBlocks[i])
                    return;
            }

            if (distance(m_Pos, finishPosPost) >= 42.0f)
            {
            	if (GameServer()->Collision()->ModifTile(TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), TileIndex, 0, pBlockInfo->m_Health))
            	{
            		GameServer()->m_pController->OnPlayerPutBlock(m_pPlayer->GetCID(), TilePos, TileIndex, 0, pBlockInfo->m_Health);
            		Builded = true;
            	}
            }
        }
    }

    if (Builded)
    	UseInventoryItem(m_ActiveInventoryItem);
}

void CCharacter::UseInventoryItem(int Index)
{
	if (m_FastInventory[Index].m_ItemId != 0)
	{
		CCellData *pCellData = &m_FastInventory[Index];
		--pCellData->m_Amount;
		if(pCellData->m_Amount <= 0)
		{
			pCellData->m_ItemId = 0;
			pCellData->m_Amount = 0;
		}
		m_NeedSendFastInventory = true;
	}
}
//

void CCharacter::FireWeapon()
{
	if(m_ReloadTimer != 0)
		return;

	const int ActiveItem = m_FastInventory[m_ActiveInventoryItem].m_ItemId;
	const int ActiveItemAmount = m_FastInventory[m_ActiveInventoryItem].m_Amount;

	DoInventoryItemSwitch();
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	bool FullAuto = false;
	if(ActiveItem == WEAPON_GRENADE || ActiveItem == WEAPON_SHOTGUN || ActiveItem == WEAPON_RIFLE)
		FullAuto = true;


	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(FullAuto && (m_LatestInput.m_Fire&1) && (m_FastInventory[m_ActiveInventoryItem].m_Amount || m_pPlayer->IsBot()))
		WillFire = true;

	if(!WillFire)
		return;

	// check for ammo
	if(!m_pPlayer->IsBot() && !ActiveItemAmount)
	{
		// 125ms is a magical limit of how fast a human can click
		m_ReloadTimer = 125 * Server()->TickSpeed() / 1000;
		if(m_LastNoAmmoSound+Server()->TickSpeed() <= Server()->Tick())
		{
			GameServer()->CreateSound(m_Pos, SOUND_WEAPON_NOAMMO);
			m_LastNoAmmoSound = Server()->Tick();
		}
		return;
	}

	vec2 ProjStartPos = m_Pos+Direction*m_ProximityRadius*0.75f;
	bool Fired = false;

	switch(ActiveItem)
	{
		case WEAPON_HAMMER:
		case WEAPON_HAMMER_STONE:
		case WEAPON_HAMMER_IRON:
		{
			// reset objects Hit
			m_NumObjectsHit = 0;
			GameServer()->CreateSound(m_Pos, SOUND_HAMMER_FIRE);

            if (GameServer()->IsMineTeeSrv())
            {
                vec2 colTilePos = ProjStartPos+Direction * 80.0f;
                if (GameServer()->Collision()->IntersectLine(ProjStartPos, colTilePos, &colTilePos, 0x0, false))
                {
                    vec2 finishPosPost = colTilePos+Direction * 8.0f;
                    if (GameServer()->Collision()->GetCollisionAt(finishPosPost.x, finishPosPost.y) == CCollision::COLFLAG_SOLID)
                    {
                    	CTile *pMTTile = GameServer()->Collision()->GetMineTeeTileAt(finishPosPost);
                    	CBlockManager::CBlockInfo *pBlockInfo = GameServer()->m_BlockManager.GetBlockInfo(pMTTile->m_Index);
						if (pMTTile && pBlockInfo && pBlockInfo->m_Health > 0)
							GameServer()->m_pController->TakeBlockDamage(finishPosPost, ActiveItem, g_pData->m_Weapons.m_aId[ActiveItem].m_Blockdamage, m_pPlayer->GetCID());

						Fired = true;
						m_ReloadTimer=Server()->TickSpeed()/8;
                    }
                }
            }

			CCharacter *apEnts[MAX_CLIENTS];
			int Hits = 0;
			int Num = GameServer()->m_World.FindEntities(ProjStartPos, m_ProximityRadius*0.5f, (CEntity**)apEnts,
														MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				CCharacter *pTarget = apEnts[i];

				if ((pTarget == this) || GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
					continue;

				// set his velocity to fast upward (for now)
				if(length(pTarget->m_Pos-ProjStartPos) > 0.0f)
					GameServer()->CreateHammerHit(pTarget->m_Pos-normalize(pTarget->m_Pos-ProjStartPos)*m_ProximityRadius*0.5f);
				else
					GameServer()->CreateHammerHit(ProjStartPos);

				vec2 Dir;
				if (length(pTarget->m_Pos - m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - m_Pos);
				else
					Dir = vec2(0.f, -1.f);

				pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, g_pData->m_Weapons.m_Hammer.m_pBase->m_Damage,
					m_pPlayer->GetCID(), ActiveItem);
				Hits++;
			}

			// if we Hit anything, we have to wait for the reload
			if(Hits)
			{
				Fired = true;
				m_ReloadTimer = Server()->TickSpeed()/3;
			}

		} break;

		case WEAPON_GUN:
		{
			CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GUN,
				m_pPlayer->GetCID(),
				ProjStartPos,
				Direction,
				(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GunLifetime),
				1, 0, 0, -1, WEAPON_GUN);

			// pack the Projectile and send it to the client Directly
			CNetObj_Projectile p;
			pProj->FillInfo(&p);

			CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
			Msg.AddInt(1);
			for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
				Msg.AddInt(((int *)&p)[i]);

			Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());

			GameServer()->CreateSound(m_Pos, SOUND_GUN_FIRE);
			Fired = true;
		} break;

		case WEAPON_SHOTGUN:
		{
			int ShotSpread = 2;

			CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
			Msg.AddInt(ShotSpread*2+1);

			for(int i = -ShotSpread; i <= ShotSpread; ++i)
			{
				float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
				float a = GetAngle(Direction);
				a += Spreading[i+2];
				float v = 1-(absolute(i)/(float)ShotSpread);
				float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
				CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_SHOTGUN,
					m_pPlayer->GetCID(),
					ProjStartPos,
					vec2(cosf(a), sinf(a))*Speed,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime),
					1, 0, 0, -1, WEAPON_SHOTGUN);

				// pack the Projectile and send it to the client Directly
				CNetObj_Projectile p;
				pProj->FillInfo(&p);

				for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
					Msg.AddInt(((int *)&p)[i]);
			}

			Server()->SendMsg(&Msg, 0,m_pPlayer->GetCID());

			GameServer()->CreateSound(m_Pos, SOUND_SHOTGUN_FIRE);
			Fired = true;
		} break;

		case WEAPON_GRENADE:
		{
			CProjectile *pProj = new CProjectile(GameWorld(), WEAPON_GRENADE,
				m_pPlayer->GetCID(),
				ProjStartPos,
				Direction,
				(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_GrenadeLifetime),
				1, true, 0, SOUND_GRENADE_EXPLODE, WEAPON_GRENADE);

			// pack the Projectile and send it to the client Directly
			CNetObj_Projectile p;
			pProj->FillInfo(&p);

			CMsgPacker Msg(NETMSGTYPE_SV_EXTRAPROJECTILE);
			Msg.AddInt(1);
			for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
				Msg.AddInt(((int *)&p)[i]);
			Server()->SendMsg(&Msg, 0, m_pPlayer->GetCID());

			GameServer()->CreateSound(m_Pos, SOUND_GRENADE_FIRE);
			Fired = true;
		} break;

		case WEAPON_RIFLE:
		{
			new CLaser(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID());
			GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
			Fired = true;
		} break;

		case WEAPON_NINJA:
		{
			// reset Hit objects
			m_NumObjectsHit = 0;

			m_Ninja.m_ActivationDir = Direction;
			m_Ninja.m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
			m_Ninja.m_OldVelAmount = length(m_Core.m_Vel);

			GameServer()->CreateSound(m_Pos, SOUND_NINJA_FIRE);
			Fired = true;
		} break;

	}

	m_AttackTick = Server()->Tick();

	if(Fired && ActiveItemAmount > 0) // -1 == unlimited
		m_FastInventory[m_ActiveInventoryItem].m_Amount--;

	if (m_FastInventory[m_ActiveInventoryItem].m_Amount == 0)
		m_FastInventory[m_ActiveInventoryItem].m_ItemId = 0;

	m_NeedSendFastInventory = true;

	if(!m_ReloadTimer)
	{
		// MineTee
		// FIXME: Very Ugly...
		if (m_pPlayer->IsBot() && ActiveItem == WEAPON_GRENADE)
			m_ReloadTimer = g_pData->m_Weapons.m_aId[ActiveItem].m_Firedelay*4 * Server()->TickSpeed() / 1000;
		else
			m_ReloadTimer = g_pData->m_Weapons.m_aId[ActiveItem].m_Firedelay * Server()->TickSpeed() / 1000;
	}
}

void CCharacter::HandleInventoryItems()
{
	// check reload timer
	if(m_ReloadTimer)
	{
		m_ReloadTimer--;
		return;
	}

	return;
}

CCellData* CCharacter::GiveItem(int ItemID, int Amount)
{
	const bool IsWeapon = (ItemID < NUM_WEAPONS);
    if (IsWeapon)
    {
    	CCellData *pInvItem = GetFirstEmptyInventoryIndex();
        if (!pInvItem)
            return 0x0;

		if(pInvItem->m_Amount < g_pData->m_Weapons.m_aId[ItemID].m_Maxammo)
		{
			pInvItem->m_Amount = clamp(pInvItem->m_Amount+Amount, 0, g_pData->m_Weapons.m_aId[ItemID].m_Maxammo);
			pInvItem->m_ItemId = ItemID;

			m_NeedSendFastInventory = true;
			return pInvItem;
		}
    }
    else
    {
    	bool HasItem = true;
    	CCellData *pInvItem = InInventory(ItemID);
    	if (!pInvItem)
    	{
    		pInvItem = GetFirstEmptyInventoryIndex();
    		HasItem = false;
    	}

        if (!pInvItem)
            return 0x0;

        pInvItem->m_Amount = min(255, pInvItem->m_Amount+Amount);
    	if (!HasItem)
    		pInvItem->m_ItemId = ItemID;

    	m_NeedSendFastInventory = true;
    	return pInvItem;
    }

    return 0x0;
}

void CCharacter::SetEmote(int Emote, int Tick)
{
	m_EmoteType = Emote;
	m_EmoteStop = Tick;
}

void CCharacter::OnPredictedInput(CNetObj_PlayerInput *pNewInput)
{
	// check for changes
	if(mem_comp(&m_Input, pNewInput, sizeof(CNetObj_PlayerInput)) != 0)
		m_LastAction = Server()->Tick();

	// copy new input
	mem_copy(&m_Input, pNewInput, sizeof(m_Input));
	m_NumInputs++;

	// it is not allowed to aim in the center
	if(m_Input.m_TargetX == 0 && m_Input.m_TargetY == 0)
		m_Input.m_TargetY = -1;
}

void CCharacter::OnDirectInput(CNetObj_PlayerInput *pNewInput)
{
	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
	mem_copy(&m_LatestInput, pNewInput, sizeof(m_LatestInput));

	// it is not allowed to aim in the center
	if(m_LatestInput.m_TargetX == 0 && m_LatestInput.m_TargetY == 0)
		m_LatestInput.m_TargetY = -1;

	if(m_NumInputs > 2 && m_pPlayer->GetTeam() != TEAM_SPECTATORS)
	{
		HandleInventoryItemSwitch();

		// MineTee
		if (m_ActiveInventoryItem == -1)
			return;

		const int ActiveItem = m_FastInventory[m_ActiveInventoryItem].m_ItemId;
	    if (ActiveItem >= NUM_WEAPONS
	    		|| (ActiveItem == WEAPON_HAMMER && ((m_pPlayer->m_PlayerFlags&PLAYERFLAG_BGPAINT) || (m_pPlayer->m_PlayerFlags&PLAYERFLAG_FGPAINT))))
	    {
	        Construct();
	    }
	    else
	    	FireWeapon();
	}

	mem_copy(&m_LatestPrevInput, &m_LatestInput, sizeof(m_LatestInput));
}

void CCharacter::ResetInput()
{
	m_Input.m_Direction = 0;
	m_Input.m_Hook = 0;
	// simulate releasing the fire button
	if((m_Input.m_Fire&1) != 0)
		m_Input.m_Fire++;
	m_Input.m_Fire &= INPUT_STATE_MASK;
	m_Input.m_Jump = 0;
	m_LatestPrevInput = m_LatestInput = m_Input;
}

void CCharacter::Tick()
{
	if(m_pPlayer->m_ForceBalanced)
	{
		char Buf[128];
		str_format(Buf, sizeof(Buf), "You were moved to %s due to team balancing", GameServer()->m_pController->GetTeamName(m_pPlayer->GetTeam()));
		GameServer()->SendBroadcast(Buf, m_pPlayer->GetCID());

		m_pPlayer->m_ForceBalanced = false;
	}

	m_Core.m_Input = m_Input;
	m_Core.Tick(true);

	// Fluid Damages
	int BlockID = GameServer()->Collision()->GetMineTeeTileIndexAt(vec2(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f));
	if (Server()->Tick() - m_TimerFluidDamage >= Server()->TickSpeed()/2 && (BlockID >= CBlockManager::LAVA_A && BlockID <= CBlockManager::LAVA_D))
	{
	    TakeDamage(vec2(0.0f,-1.0f), 1, m_pPlayer->GetCID(), WEAPON_WORLD);
	    m_TimerFluidDamage = Server()->Tick();
	}

	if (!m_InWater)
	{
        if ((BlockID >= CBlockManager::WATER_A && BlockID <= CBlockManager::WATER_D) && (Server()->Tick() - m_TimerFluidDamage >= Server()->TickSpeed()*8))
        {
            m_InWater = true;
            m_TimerFluidDamage = Server()->Tick();
        }
    }
	else
	{
	    if (BlockID < CBlockManager::WATER_A || BlockID > CBlockManager::WATER_D)
	    {
            m_InWater = false;
            m_TimerFluidDamage = Server()->Tick();
	    }
        else if (Server()->Tick() - m_TimerFluidDamage >= Server()->TickSpeed()*2)
        {
            TakeDamage(vec2(0.0f,-1.0f), 1, m_pPlayer->GetCID(), WEAPON_WORLD);
            m_TimerFluidDamage = Server()->Tick();
        }
	}
	//

	// AutoDisable if far from actived block
	if (m_ActiveBlockId >= 0)
	{
		const int x = m_ActiveBlockId%GameServer()->Collision()->GetWidth();
		const int y = m_ActiveBlockId/GameServer()->Collision()->GetWidth();
		const vec2 BlockPos = vec2(x*32.0f+16.0f, y*32.0f+16.0f);
		if (length(m_Pos-BlockPos) > 40.0f)
		{
			m_ActiveBlockId = -1;
			GameServer()->SendRelease(m_pPlayer->GetCID());
		}

	}

	// handle death-tiles and leaving gamelayer
	if(GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameServer()->Collision()->GetCollisionAt(m_Pos.x-m_ProximityRadius/3.f, m_Pos.y+m_ProximityRadius/3.f)&CCollision::COLFLAG_DEATH ||
		GameLayerClipped(m_Pos))
	{
		Die(m_pPlayer->GetCID(), WEAPON_WORLD);
	}

	// handle Weapons
	HandleInventoryItems();

	// Previnput
	m_PrevInput = m_Input;
}

void CCharacter::TickDefered()
{
	// advance the dummy
	{
		CWorldCore TempWorld;
		m_ReckoningCore.Init(&TempWorld, GameServer()->Collision());
		m_ReckoningCore.Tick(false);
		m_ReckoningCore.Move();
		m_ReckoningCore.Quantize();
	}

	//lastsentcore
	vec2 StartPos = m_Core.m_Pos;
	vec2 StartVel = m_Core.m_Vel;
	bool StuckBefore = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));

	m_Core.Move();
	bool StuckAfterMove = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Core.Quantize();
	bool StuckAfterQuant = GameServer()->Collision()->TestBox(m_Core.m_Pos, vec2(28.0f, 28.0f));
	m_Pos = m_Core.m_Pos;

	if(!StuckBefore && (StuckAfterMove || StuckAfterQuant))
	{
		// Hackish solution to get rid of strict-aliasing warning
		union
		{
			float f;
			unsigned u;
		}StartPosX, StartPosY, StartVelX, StartVelY;

		StartPosX.f = StartPos.x;
		StartPosY.f = StartPos.y;
		StartVelX.f = StartVel.x;
		StartVelY.f = StartVel.y;

		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "STUCK!!! %d %d %d %f %f %f %f %x %x %x %x",
			StuckBefore,
			StuckAfterMove,
			StuckAfterQuant,
			StartPos.x, StartPos.y,
			StartVel.x, StartVel.y,
			StartPosX.u, StartPosY.u,
			StartVelX.u, StartVelY.u);
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	int Events = m_Core.m_TriggeredEvents;
	int Mask = CmaskAllExceptOne(m_pPlayer->GetCID());

	if(Events&COREEVENT_GROUND_JUMP) GameServer()->CreateSound(m_Pos, SOUND_PLAYER_JUMP, Mask);

	if(Events&COREEVENT_HOOK_ATTACH_PLAYER) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_PLAYER, CmaskAll());
	if(Events&COREEVENT_HOOK_ATTACH_GROUND) GameServer()->CreateSound(m_Pos, SOUND_HOOK_ATTACH_GROUND, Mask);
	if(Events&COREEVENT_HOOK_HIT_NOHOOK) GameServer()->CreateSound(m_Pos, SOUND_HOOK_NOATTACH, Mask);


	if(m_pPlayer->GetTeam() == TEAM_SPECTATORS)
	{
		m_Pos.x = m_Input.m_TargetX;
		m_Pos.y = m_Input.m_TargetY;
	}

	// update the m_SendCore if needed
	{
		CNetObj_Character Predicted;
		CNetObj_Character Current;
		mem_zero(&Predicted, sizeof(Predicted));
		mem_zero(&Current, sizeof(Current));
		m_ReckoningCore.Write(&Predicted);
		m_Core.Write(&Current);

		// only allow dead reackoning for a top of 3 seconds
		if(m_ReckoningTick+Server()->TickSpeed()*3 < Server()->Tick() || mem_comp(&Predicted, &Current, sizeof(CNetObj_Character)) != 0)
		{
			m_ReckoningTick = Server()->Tick();
			m_SendCore = m_Core;
			m_ReckoningCore = m_Core;
		}
	}

	// MineTee: Check Hook State
    int plHooked = m_Core.m_HookedPlayer;
	if (plHooked >= g_Config.m_SvMaxClients && plHooked < MAX_CLIENTS && GameServer()->m_apPlayers[plHooked] && GameServer()->m_apPlayers[plHooked]->GetBotType() == BOT_ANIMAL)
	    m_Core.m_HookTick = 0;
}

void CCharacter::TickPaused()
{
	++m_AttackTick;
	++m_DamageTakenTick;
	++m_Ninja.m_ActivationTick;
	++m_ReckoningTick;
	if(m_LastAction != -1)
		++m_LastAction;
	if(m_EmoteStop > -1)
		++m_EmoteStop;
}

bool CCharacter::IncreaseHealth(int Amount)
{
	if(m_Health >= 999)
		return false;
	m_Health = clamp(m_Health+Amount, 0, 999);
	return true;
}

bool CCharacter::IncreaseArmor(int Amount)
{
	if(m_Armor >= 999)
		return false;
	m_Armor = clamp(m_Armor+Amount, 0, 999);
	return true;
}

void CCharacter::Die(int Killer, int ItemID)
{
	if (Killer == -1) // MineTee
		Killer = m_pPlayer->GetCID();

	// we got to wait 0.5 secs or 20 if bot before respawning
	float RespawnTick = (m_pPlayer->IsBot())?Server()->TickSpeed()*20:Server()->TickSpeed()/2; // MineTee
    m_pPlayer->m_RespawnTick = Server()->Tick()+RespawnTick;
	int ModeSpecial = GameServer()->m_pController->OnCharacterDeath(this, GameServer()->m_apPlayers[Killer], ItemID);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "kill killer='%d:%d:%s' victim='%d:%d:%s' weapon=%d special=%d",
		Killer, GameServer()->m_apPlayers[Killer]->GetTeam(), Server()->ClientName(Killer),
		m_pPlayer->GetCID(), m_pPlayer->GetTeam(), Server()->ClientName(m_pPlayer->GetCID()), ItemID, ModeSpecial); // MineTee
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	// send the kill message
	if (m_pPlayer->IsBot() && Killer >= 0 && Killer < g_Config.m_SvMaxClients) // MineTee
	{
		CNetMsg_Sv_KillMsg Msg;
		Msg.m_Killer = Killer;
		Msg.m_Victim = m_pPlayer->GetCID();
		Msg.m_Weapon = ItemID;
		Msg.m_ModeSpecial = ModeSpecial;
		Server()->SendPackMsg(&Msg, MSGFLAG_VITAL, -1);
	}

	// a nice sound
	GameServer()->CreateSound(m_Pos, SOUND_PLAYER_DIE);

	// this is for auto respawn after 3 secs
	m_pPlayer->m_DieTick = Server()->Tick();

	m_Alive = false;
	GameServer()->m_World.RemoveEntity(this);
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	GameServer()->CreateDeath(m_Pos, m_pPlayer->GetCID());

	// MineTee
	if (GameServer()->IsMineTeeSrv()  && m_pPlayer->GetTeam() <= TEAM_BLUE)
	{
	    GameServer()->CreateTombstone(m_Pos);
	}
}

bool CCharacter::TakeDamage(vec2 Force, int Dmg, int From, int ItemID)
{
	m_Core.m_Vel += Force;

	if(GameServer()->m_pController->IsFriendlyFire(m_pPlayer->GetCID(), From) && !g_Config.m_SvTeamdamage)
		return false;

	// m_pPlayer only inflicts half damage on self
	if(From == m_pPlayer->GetCID())
		Dmg = max(1, Dmg/2);

	m_DamageTaken++;

	// create healthmod indicator
	if(Server()->Tick() < m_DamageTakenTick+25)
	{
		// make sure that the damage indicators doesn't group together
		GameServer()->CreateDamageInd(m_Pos, m_DamageTaken*0.25f, Dmg);
	}
	else
	{
		m_DamageTaken = 0;
		GameServer()->CreateDamageInd(m_Pos, 0, Dmg);
	}

	if(Dmg)
	{
		if(m_Armor)
		{
			if(Dmg > 1)
			{
				m_Health--;
				Dmg--;
			}

			if(Dmg > m_Armor)
			{
				Dmg -= m_Armor;
				m_Armor = 0;
			}
			else
			{
				m_Armor -= Dmg;
				Dmg = 0;
			}
		}

		m_Health -= Dmg;
	}

	m_DamageTakenTick = Server()->Tick();

	// do damage Hit sound
	if(From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
	{
		int Mask = CmaskOne(From);
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetTeam() == TEAM_SPECTATORS && GameServer()->m_apPlayers[i]->m_SpectatorID == From)
				Mask |= CmaskOne(i);
		}
		GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, Mask);
	}

	// check for death
	if(m_Health <= 0)
	{
		Die(From, ItemID);

		// set attacker's face to happy (taunt!)
		if (From >= 0 && From != m_pPlayer->GetCID() && GameServer()->m_apPlayers[From])
		{
			CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
			if (pChr)
			{
				pChr->m_EmoteType = EMOTE_HAPPY;
				pChr->m_EmoteStop = Server()->Tick() + Server()->TickSpeed();
			}
		}

		return false;
	}

	if (Dmg > 2)
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_LONG);
	else
		GameServer()->CreateSound(m_Pos, SOUND_PLAYER_PAIN_SHORT);

	m_EmoteType = EMOTE_PAIN;
	m_EmoteStop = Server()->Tick() + 500 * Server()->TickSpeed() / 1000;

	return true;
}

void CCharacter::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient))
		return;

	CNetObj_Character *pCharacter = static_cast<CNetObj_Character *>(Server()->SnapNewItem(NETOBJTYPE_CHARACTER, m_pPlayer->GetCID(), sizeof(CNetObj_Character)));
	if(!pCharacter)
		return;

	// write down the m_Core
	if(!m_ReckoningTick || GameServer()->m_World.m_Paused)
	{
		// no dead reckoning when paused because the client doesn't know
		// how far to perform the reckoning
		pCharacter->m_Tick = 0;
		m_Core.Write(pCharacter);
	}
	else
	{
		pCharacter->m_Tick = m_ReckoningTick;
		m_SendCore.Write(pCharacter);
	}

	// set emote
	if (m_EmoteStop < Server()->Tick())
	{
		m_EmoteType = EMOTE_NORMAL;
		m_EmoteStop = -1;
	}

	pCharacter->m_Emote = m_EmoteType;

	pCharacter->m_AmmoCount = 0;
	pCharacter->m_Health = 0;
	pCharacter->m_Armor = 0;

	pCharacter->m_Weapon = m_FastInventory[m_ActiveInventoryItem].m_ItemId;
	pCharacter->m_AttackTick = m_AttackTick;

	pCharacter->m_Direction = m_Input.m_Direction;

	if(m_pPlayer->GetCID() == SnappingClient || SnappingClient == -1 ||
		(!g_Config.m_SvStrictSpectateMode && m_pPlayer->GetCID() == GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID))
	{
		pCharacter->m_Health = m_Health;
		pCharacter->m_Armor = m_Armor;

		if(m_FastInventory[m_ActiveInventoryItem].m_Amount > 0)
			pCharacter->m_AmmoCount = m_FastInventory[m_ActiveInventoryItem].m_Amount;
	}

	if(pCharacter->m_Emote == EMOTE_NORMAL)
	{
		if(250 - ((Server()->Tick() - m_LastAction)%(250)) < 5)
			pCharacter->m_Emote = EMOTE_BLINK;
	}

	pCharacter->m_PlayerFlags = GetPlayer()->m_PlayerFlags;

	// MineTee
	if (GameServer()->IsMineTeeSrv())
	{
		int TileIndexD = GameServer()->Collision()->GetMineTeeTileIndexAt(vec2(m_Pos.x, m_Pos.y+16.0f));
		int TileIndex = GameServer()->Collision()->GetMineTeeTileIndexAt(vec2(m_Pos.x, m_Pos.y));
		int FluidType = 0;
		if (!m_Jumped && ((TileIndexD >= CBlockManager::LARGE_BED_LEFT && TileIndexD <= CBlockManager::LARGE_BED_RIGHT) || TileIndexD == CBlockManager::BED))
			pCharacter->m_Emote = EMOTE_BLINK;
		else if (GameServer()->m_BlockManager.IsFluid(TileIndex, &FluidType))
		{
			if (FluidType == CCollision::FLUID_WATER)
				pCharacter->m_Emote = EMOTE_BLINK;
			else if (FluidType == CCollision::FLUID_LAVA)
				pCharacter->m_Emote = EMOTE_PAIN;
		}

		if(m_pPlayer->GetCID() == SnappingClient && m_NeedSendFastInventory)
		{
		    if (m_ActiveBlockId != -1)
				GameServer()->m_pController->SendInventory(m_pPlayer->GetCID(), (m_ActiveBlockId != -2));

			CNetObj_Inventory *pClientInventory = static_cast<CNetObj_Inventory *>(Server()->SnapNewItem(NETOBJTYPE_INVENTORY, m_pPlayer->GetCID(), sizeof(CNetObj_Inventory)));
			if(!pClientInventory)
				return;

			//TODO: Ugly
			int Items[NUM_CELLS_LINE], Amounts[NUM_CELLS_LINE];
			for (int u=0; u<NUM_CELLS_LINE; Items[u]=m_FastInventory[u].m_ItemId,++u);
			for (int u=0; u<NUM_CELLS_LINE; Amounts[u]=m_FastInventory[u].m_Amount,++u);
			mem_copy(&pClientInventory->m_Item1, Items, sizeof(int)*NUM_CELLS_LINE);
			mem_copy(&pClientInventory->m_Ammo1, Amounts, sizeof(int)*NUM_CELLS_LINE);
			pClientInventory->m_Selected = m_ActiveInventoryItem;

			m_NeedSendFastInventory = false;
		}
	}
}

// MineTee
bool CCharacter::IsInventoryFull()
{
    for (int i=0; i<NUM_CELLS_LINE; i++)
    {
        if (m_FastInventory[i].m_ItemId == 0)
            return false;
    }

    return true;
}

void CCharacter::DropItem(int Index)
{
    if (Index < 0 || Index >= NUM_CELLS_LINE)
    	Index = m_ActiveInventoryItem;

    int ItemID = m_FastInventory[Index].m_ItemId;
    if (ItemID > 0)
    {
        vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

		CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_DROPITEM, m_FastInventory[Index].m_ItemId);
		pPickup->m_Pos = m_Pos;
		pPickup->m_Pos.y -= 18.0f;
		pPickup->m_Vel = vec2(Direction.x*5.0f, -5);
		pPickup->m_Amount = m_FastInventory[Index].m_Amount;
		pPickup->m_Owner = m_pPlayer->GetCID();
		mem_zero(&m_FastInventory[Index], sizeof(m_FastInventory[Index]));

		for (int i=m_ActiveInventoryItem-1; i>=0; i--)
		{
			if (m_FastInventory[i].m_ItemId != 0)
			{
				m_ActiveInventoryItem = i;
				break;
			}
		}

		m_NeedSendFastInventory = true;
    }
}

void CCharacter::FillAccountData(void *pAccountInfo)
{
	IAccountSystem::ACCOUNT_INFO *pInfo = (IAccountSystem::ACCOUNT_INFO*)pAccountInfo;
	pInfo->m_Alive = m_Alive;
	pInfo->m_Pos = m_Pos;
	pInfo->m_Health = m_Health;
	pInfo->m_ActiveInventoryItem = m_ActiveInventoryItem;
	mem_copy(&pInfo->m_FastInventory, &m_FastInventory, sizeof(CCellData)*NUM_CELLS_LINE);
}
void CCharacter::UseAccountData(void *pAccountInfo)
{
	IAccountSystem::ACCOUNT_INFO *pInfo = (IAccountSystem::ACCOUNT_INFO*)pAccountInfo;
	m_Core.m_Pos = m_Pos = pInfo->m_Pos;
	m_Health = pInfo->m_Health;
	m_ActiveInventoryItem = pInfo->m_ActiveInventoryItem;
	mem_copy(&m_FastInventory, &pInfo->m_FastInventory, sizeof(CCellData)*NUM_CELLS_LINE);

	// Check that can spawn in that place
	if (GameServer()->Collision()->CheckPoint(m_Pos))
	{
		const ivec2 TileMapPos = ivec2(m_Pos.x/32, m_Pos.y/32);
		GameServer()->SendTileModif(ALL_PLAYERS, TileMapPos, GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0, 0);
		GameServer()->Collision()->ModifTile(TileMapPos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0, 0);
	}
}

CCellData* CCharacter::InInventory(int ItemID)
{
	for (int i=0; i<NUM_CELLS_LINE; i++)
	{
		if (m_FastInventory[i].m_ItemId == ItemID)
			return &m_FastInventory[i];
	}

	CPlayer *pPlayer = GetPlayer();
	for (int i=0; i<CPlayer::NUM_INVENTORY_CELLS; i++)
	{
		if (pPlayer->m_aInventory[i].m_ItemId == ItemID)
			return &pPlayer->m_aInventory[i];
	}

	return 0x0;
}

CCellData* CCharacter::GetFirstEmptyInventoryIndex()
{
	for (int i=0; i<NUM_CELLS_LINE; i++)
	{
		if (m_FastInventory[i].m_ItemId <= 0)
			return &m_FastInventory[i];
	}

	CPlayer *pPlayer = GetPlayer();
	for (int i=0; i<CPlayer::NUM_INVENTORY_CELLS; i++)
	{
		if (pPlayer->m_aInventory[i].m_ItemId <= 0)
			return &pPlayer->m_aInventory[i];
	}

	return 0x0;
}
