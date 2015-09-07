/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
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
    m_BotDir = 1;
    m_BotLastPos = m_Pos;
    m_BotLastStuckTime = 0.0f;
    m_BotStuckCount = 0;
    m_BotTimePlayerFound = Server()->Tick();
    m_BotTimeGrounded = Server()->Tick();
    m_BotTimeLastOption = Server()->Tick();
    m_BotTimeLastDamage = 0.0f;
    m_BotClientIDFix = -1;
    m_BotTimeLastSound = Server()->Tick();
    m_BotJumpTry = false;

    m_NeedSendInventory = true;
	TimerFluidDamage = Server()->Tick();
	inWater = false;
    //
}

void CCharacter::Reset()
{
	Destroy();
}

bool CCharacter::Spawn(CPlayer *pPlayer, vec2 Pos)
{
	// MineTee
	for (int i=0; i<CBlockManager::MAX_BLOCKS; i++)
	{
	    m_aBlocks[i].m_Got = false;
	    m_aBlocks[i].m_Amount = 0;
	}

	for (int i=0; i<NUM_ITEMS_INVENTORY; m_Inventory.m_Items[i++]=NUM_WEAPONS+CBlockManager::MAX_BLOCKS);
    m_Inventory.m_Selected=0;
    //

	m_EmoteStop = -1;
	m_LastAction = -1;
	m_LastNoAmmoSound = -1;
	m_ActiveWeapon = WEAPON_GUN;
	m_LastWeapon = WEAPON_HAMMER;
	m_QueuedWeapon = -1;

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

	return true;
}

void CCharacter::Destroy()
{
	GameServer()->m_World.m_Core.m_apCharacters[m_pPlayer->GetCID()] = 0;
	m_Alive = false;
}

void CCharacter::SetWeapon(int W)
{
	if(W == m_ActiveWeapon)
		return;

	m_LastWeapon = m_ActiveWeapon;
	m_QueuedWeapon = -1;
	m_ActiveWeapon = W;
	GameServer()->CreateSound(m_Pos, SOUND_WEAPON_SWITCH);

	if(m_ActiveWeapon < 0 || m_ActiveWeapon >= NUM_WEAPONS+CBlockManager::MAX_BLOCKS) // MineTee
		m_ActiveWeapon = 0;

    // MineTee
    for (int i=0; i<NUM_ITEMS_INVENTORY; i++)
    {
        if (m_Inventory.m_Items[i] == W)
        {
            m_Inventory.m_Selected = i;
            break;
        }
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


void CCharacter::HandleNinja()
{
	if(m_ActiveWeapon != WEAPON_NINJA)
		return;

	if ((Server()->Tick() - m_Ninja.m_ActivationTick) > (g_pData->m_Weapons.m_Ninja.m_Duration * Server()->TickSpeed() / 1000))
	{
		// time's up, return
		m_aWeapons[WEAPON_NINJA].m_Got = false;
		m_ActiveWeapon = m_LastWeapon;

		SetWeapon(m_ActiveWeapon);
		return;
	}

	// force ninja Weapon
	SetWeapon(WEAPON_NINJA);

	m_Ninja.m_CurrentMoveTime--;

	if (m_Ninja.m_CurrentMoveTime == 0)
	{
		// reset velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir*m_Ninja.m_OldVelAmount;
	}

	if (m_Ninja.m_CurrentMoveTime > 0)
	{
		// Set velocity
		m_Core.m_Vel = m_Ninja.m_ActivationDir * g_pData->m_Weapons.m_Ninja.m_Velocity;
		vec2 OldPos = m_Pos;
		GameServer()->Collision()->MoveBox(&m_Core.m_Pos, &m_Core.m_Vel, vec2(m_ProximityRadius, m_ProximityRadius), 0.f);

		// reset velocity so the client doesn't predict stuff
		m_Core.m_Vel = vec2(0.f, 0.f);

		// check if we Hit anything along the way
		{
			CCharacter *aEnts[MAX_CLIENTS];
			vec2 Dir = m_Pos - OldPos;
			float Radius = m_ProximityRadius * 2.0f;
			vec2 Center = OldPos + Dir * 0.5f;
			int Num = GameServer()->m_World.FindEntities(Center, Radius, (CEntity**)aEnts, MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for (int i = 0; i < Num; ++i)
			{
				if (aEnts[i] == this)
					continue;

				// make sure we haven't Hit this object before
				bool bAlreadyHit = false;
				for (int j = 0; j < m_NumObjectsHit; j++)
				{
					if (m_apHitObjects[j] == aEnts[i])
						bAlreadyHit = true;
				}
				if (bAlreadyHit)
					continue;

				// check so we are sufficiently close
				if (distance(aEnts[i]->m_Pos, m_Pos) > (m_ProximityRadius * 2.0f))
					continue;

				// Hit a player, give him damage and stuffs...
				GameServer()->CreateSound(aEnts[i]->m_Pos, SOUND_NINJA_HIT);
				// set his velocity to fast upward (for now)
				if(m_NumObjectsHit < 10)
					m_apHitObjects[m_NumObjectsHit++] = aEnts[i];

				aEnts[i]->TakeDamage(vec2(0, -10.0f), g_pData->m_Weapons.m_Ninja.m_pBase->m_Damage, m_pPlayer->GetCID(), WEAPON_NINJA);
			}
		}

		return;
	}

	return;
}


void CCharacter::DoWeaponSwitch()
{
	// make sure we can switch
	if(m_ReloadTimer != 0 || m_QueuedWeapon == -1 || m_aWeapons[WEAPON_NINJA].m_Got)
		return;

	// switch Weapon
	SetWeapon(m_QueuedWeapon);
}

void CCharacter::HandleWeaponSwitch()
{
	int WantedWeapon = m_ActiveWeapon;
	if(m_QueuedWeapon != -1)
		WantedWeapon = m_QueuedWeapon;

	// select Weapon
	int Next = CountInput(m_LatestPrevInput.m_NextWeapon, m_LatestInput.m_NextWeapon).m_Presses;
	int Prev = CountInput(m_LatestPrevInput.m_PrevWeapon, m_LatestInput.m_PrevWeapon).m_Presses;

	if(Next < 128) // make sure we only try sane stuff
	{
		while(Next) // Next Weapon selection
		{
			// MineTee
		    if (str_find_nocase(GameServer()->GameType(), "MineTee"))
		    {
		        WantedWeapon = (m_Inventory.m_Selected+1>8)?0:m_Inventory.m_Selected+1;

		        if (m_Inventory.m_Items[WantedWeapon] != NUM_WEAPONS+CBlockManager::MAX_BLOCKS)
		        {
		            m_Inventory.m_Selected = WantedWeapon;
		            WantedWeapon = m_Inventory.m_Items[WantedWeapon];
		            Next--;
		        }
		        else
		            m_Inventory.m_Selected = WantedWeapon;
		    }
		    else
		    {
                WantedWeapon = (WantedWeapon+1)%NUM_WEAPONS;
                if(m_aWeapons[WantedWeapon].m_Got)
                    Next--;
		    }
		    //
		}
	}

	if(Prev < 128) // make sure we only try sane stuff
	{
		while(Prev) // Prev Weapon selection
		{
			// MineTee
		    if (str_find_nocase(GameServer()->GameType(), "MineTee"))
		    {
		        WantedWeapon = (m_Inventory.m_Selected-1<0)?8:m_Inventory.m_Selected-1;

		        if (m_Inventory.m_Items[WantedWeapon] != NUM_WEAPONS+CBlockManager::MAX_BLOCKS)
		        {
		            m_Inventory.m_Selected = WantedWeapon;
		            WantedWeapon = m_Inventory.m_Items[WantedWeapon];
		            Prev--;
		        }
		        else
                   m_Inventory.m_Selected = WantedWeapon;
		    }
		    else
		    {
                WantedWeapon = (WantedWeapon-1)<0?NUM_WEAPONS-1:WantedWeapon-1;

                if(m_aWeapons[WantedWeapon].m_Got)
                    Prev--;
		    }
		    //
		}
	}

	// Direct Weapon selection
	if(m_LatestInput.m_WantedWeapon)
		WantedWeapon = m_Input.m_WantedWeapon-1;

	// check for insane values
	if(WantedWeapon >= 0 && WantedWeapon < NUM_WEAPONS+CBlockManager::MAX_BLOCKS && WantedWeapon != m_ActiveWeapon && ((WantedWeapon < NUM_WEAPONS && m_aWeapons[WantedWeapon].m_Got) || (WantedWeapon >= NUM_WEAPONS && m_aBlocks[WantedWeapon-NUM_WEAPONS].m_Got))) // MineTee
		m_QueuedWeapon = WantedWeapon;

	DoWeaponSwitch();
	UpdateInventory(); // MineTee
}

// MineTee
void CCharacter::Construct()
{
	if(m_ReloadTimer != 0)
		return;

    bool Builded = false;
    int ActiveBlock = (m_ActiveWeapon - NUM_WEAPONS) % CBlockManager::MAX_BLOCKS;

    DoWeaponSwitch();
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(!WillFire)
		return;

    vec2 ProjStartPos= m_Pos + Direction * 38.0f;
    vec2 colTilePos = ProjStartPos+Direction * 120.0f;

    if (m_pPlayer->m_PlayerFlags&PLAYERFLAG_BGPAINT)
    {
        ivec2 TilePos = ivec2((m_Pos.x+m_LatestInput.m_TargetX)/32.0f, (m_Pos.y+m_LatestInput.m_TargetY)/32.0f);
        if (TilePos.x > 0 && TilePos.x < GameServer()->Layers()->MineTeeLayer()->m_Width && TilePos.y > 0 && TilePos.y < GameServer()->Layers()->MineTeeLayer()->m_Height)
        {
            int Index = TilePos.y*GameServer()->Layers()->MineTeeLayer()->m_Width+TilePos.x;
            CTile *pMTTiles = (CTile *)GameServer()->Layers()->Map()->GetData(GameServer()->Layers()->MineTeeLayer()->m_Data);
            CTile *pMTBGTiles = (CTile *)GameServer()->Layers()->Map()->GetData(GameServer()->Layers()->MineTeeBGLayer()->m_Data);
            if (pMTTiles[Index].m_Index != 0 || pMTBGTiles[Index].m_Index == ActiveBlock)
                return;

        	GameServer()->SendTileModif(ALL_PLAYERS, vec2(TilePos.x, TilePos.y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeBGLayerIndex(), (m_ActiveWeapon == WEAPON_HAMMER)?0:ActiveBlock, 0);
            GameServer()->Collision()->ModifTile(vec2(TilePos.x, TilePos.y), GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeBGLayerIndex(), (m_ActiveWeapon == WEAPON_HAMMER)?0:ActiveBlock, 0);
            GameServer()->CreateSound(m_Pos, SOUND_DESTROY_BLOCK);

            Builded = true;
        }
    }
    else if (m_pPlayer->m_PlayerFlags&PLAYERFLAG_FGPAINT)
    {
        ivec2 TilePos = ivec2((m_Pos.x+m_LatestInput.m_TargetX)/32.0f, (m_Pos.y+m_LatestInput.m_TargetY)/32.0f);
        if (TilePos.x > 0 && TilePos.x < GameServer()->Layers()->MineTeeLayer()->m_Width && TilePos.y > 0 && TilePos.y < GameServer()->Layers()->MineTeeLayer()->m_Height)
        {
            int Index = TilePos.y*GameServer()->Layers()->MineTeeLayer()->m_Width+TilePos.x;
            CTile *pMTTiles = (CTile *)GameServer()->Layers()->Map()->GetData(GameServer()->Layers()->MineTeeLayer()->m_Data);
            CTile *pMTFGTiles = (CTile *)GameServer()->Layers()->Map()->GetData(GameServer()->Layers()->MineTeeFGLayer()->m_Data);
            if (pMTTiles[Index].m_Index != 0 || pMTFGTiles[Index].m_Index == ActiveBlock)
                return;

        	GameServer()->SendTileModif(ALL_PLAYERS, vec2(TilePos.x, TilePos.y), GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeFGLayerIndex(), (m_ActiveWeapon == WEAPON_HAMMER)?0:ActiveBlock, 0);
            GameServer()->Collision()->ModifTile(vec2(TilePos.x, TilePos.y), GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeFGLayerIndex(), (m_ActiveWeapon == WEAPON_HAMMER)?0:ActiveBlock, 0);
            GameServer()->CreateSound(m_Pos, SOUND_DESTROY_BLOCK);

            Builded = true;
        }
    }
    else if (GameServer()->Collision()->IntersectLine(ProjStartPos, colTilePos, &colTilePos, 0x0))
    {

        vec2 finishPosPost = colTilePos-Direction * 8.0f;
        if (GameServer()->Collision()->GetCollisionAt(finishPosPost.x, finishPosPost.y) != CCollision::COLFLAG_SOLID)
        {
            vec2 TilePos = vec2(static_cast<int>(finishPosPost.x/32.0f), static_cast<int>(finishPosPost.y/32.0f));
            CBlockManager::CBlockInfo BlockInfo;
            GameServer()->m_BlockManager.GetBlockInfo(ActiveBlock, &BlockInfo);
            int TileIndex = BlockInfo.m_OnPut;

            //Check player stuck
            for (int i=0; i<MAX_CLIENTS; i++)
            {
                if (!GameServer()->m_apPlayers[i])
                    continue;

                CCharacter *pChar = GameServer()->m_apPlayers[i]->GetCharacter();
                if (!pChar || !pChar->IsAlive())
                    continue;

                if (distance(vec2(static_cast<int>(pChar->m_Pos.x/32), static_cast<int>(pChar->m_Pos.y/32)), TilePos) < 2)
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
            	GameServer()->SendTileModif(ALL_PLAYERS, TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), TileIndex, 0);
                GameServer()->Collision()->ModifTile(TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(), GameServer()->Layers()->GetMineTeeLayerIndex(), TileIndex, 0);
                GameServer()->CreateSound(m_Pos, SOUND_DESTROY_BLOCK);

                Builded = true;
            }
        }
    }

    if (Builded)
    {
        m_aBlocks[ActiveBlock].m_Amount--;
        if(m_aBlocks[ActiveBlock].m_Amount == 0)
        {
            m_aBlocks[ActiveBlock].m_Got = false;
            /*for (int i=m_Inventory.m_Selected-1; i>=0; i--)
            {
                if (m_Inventory.m_Items[i] != NUM_WEAPONS+NUM_BLOCKS)
                {
                    ActiveBlock = m_Inventory.m_Items[i];
                    m_Inventory.m_Selected = i;
                    break;
                }
            }*/

            ActiveBlock = m_Inventory.m_Items[0];
            m_Inventory.m_Selected = 0;
            SetWeapon(ActiveBlock);
        }
        UpdateInventory();
    }
}
//

void CCharacter::FireWeapon()
{
	// MineTee
    if (str_comp_nocase(GameServer()->GameType(), "MineTee") == 0 && (m_ActiveWeapon >= NUM_WEAPONS || (m_ActiveWeapon == WEAPON_HAMMER && (m_pPlayer->m_PlayerFlags&PLAYERFLAG_BGPAINT || m_pPlayer->m_PlayerFlags&PLAYERFLAG_FGPAINT))))
    {
        Construct();
        return;
    }
    //

	if(m_ReloadTimer != 0)
		return;

	DoWeaponSwitch();
	vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

	bool FullAuto = false;
	if(m_ActiveWeapon == WEAPON_GRENADE || m_ActiveWeapon == WEAPON_SHOTGUN || m_ActiveWeapon == WEAPON_RIFLE)
		FullAuto = true;


	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(m_LatestPrevInput.m_Fire, m_LatestInput.m_Fire).m_Presses)
		WillFire = true;

	if(FullAuto && (m_LatestInput.m_Fire&1) && m_aWeapons[m_ActiveWeapon].m_Ammo)
		WillFire = true;

	if(!WillFire)
		return;

	// check for ammo
	if(!m_aWeapons[m_ActiveWeapon].m_Ammo)
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

	switch(m_ActiveWeapon)
	{
		case WEAPON_HAMMER:
		{
			// reset objects Hit
			m_NumObjectsHit = 0;
			GameServer()->CreateSound(m_Pos, SOUND_HAMMER_FIRE);

            if (str_comp_nocase(GameServer()->GameType(), "MineTee") == 0)
            {
                vec2 colTilePos = ProjStartPos+Direction * 80.0f;
                if (GameServer()->Collision()->IntersectLine(ProjStartPos, colTilePos, &colTilePos, 0x0, false))
                {
                    vec2 finishPosPost = colTilePos+Direction * 8.0f;
                    if (GameServer()->Collision()->GetCollisionAt(finishPosPost.x, finishPosPost.y) == CCollision::COLFLAG_SOLID)
                    {
						int TIndex = -1;
						if ((TIndex = GameServer()->Collision()->GetMineTeeTileAt(finishPosPost)) > 0)
						{
							vec2 TilePos = vec2(static_cast<int>(finishPosPost.x/32.0f), static_cast<int>(finishPosPost.y/32.0f));
                        	GameServer()->SendTileModif(ALL_PLAYERS, TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
                            GameServer()->Collision()->ModifTile(TilePos, GameServer()->Layers()->GetMineTeeGroupIndex(),  GameServer()->Layers()->GetMineTeeLayerIndex(), 0, 0);
							GameServer()->CreateSound(m_Pos, SOUND_DESTROY_BLOCK);

							if (TIndex == CBlockManager::TNT)
							{
								GameServer()->CreateExplosion(finishPosPost, GetPlayer()->GetCID(), WEAPON_WORLD, false);
								GameServer()->CreateSound(finishPosPost, SOUND_GRENADE_EXPLODE);
							}
							else
							{
								CBlockManager::CBlockInfo BlockInfo;
								if (GameServer()->m_BlockManager.GetBlockInfo(TIndex, &BlockInfo))
								{
									if (BlockInfo.m_vOnBreak.size() > 0)
									{
										for (std::map<int, unsigned char>::iterator it = BlockInfo.m_vOnBreak.begin(); it != BlockInfo.m_vOnBreak.end(); it++)
										{
											if (it->first == 0)
											{
												++it;
												continue;
											}

											CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, it->first);
											pPickup->m_Pos = vec2(TilePos.x*32.0f + 8.0f, (TilePos.y)*32.0f + 8.0f);
											pPickup->m_Amount = it->second;
										}
									}
									else
									{
										CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_BLOCK, TIndex);
										pPickup->m_Pos = vec2(TilePos.x*32.0f + 8.0f, TilePos.y*32.0f + 8.0f);
									}
								}
							}
						}

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
					m_pPlayer->GetCID(), m_ActiveWeapon);
				Hits++;
			}

			// if we Hit anything, we have to wait for the reload
			if(Hits)
				m_ReloadTimer = Server()->TickSpeed()/3;

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
		} break;

		case WEAPON_RIFLE:
		{
			new CLaser(GameWorld(), m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, m_pPlayer->GetCID());
			GameServer()->CreateSound(m_Pos, SOUND_RIFLE_FIRE);
		} break;

		case WEAPON_NINJA:
		{
			// reset Hit objects
			m_NumObjectsHit = 0;

			m_Ninja.m_ActivationDir = Direction;
			m_Ninja.m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
			m_Ninja.m_OldVelAmount = length(m_Core.m_Vel);

			GameServer()->CreateSound(m_Pos, SOUND_NINJA_FIRE);
		} break;

	}

	m_AttackTick = Server()->Tick();

	if(m_aWeapons[m_ActiveWeapon].m_Ammo > 0) // -1 == unlimited
		m_aWeapons[m_ActiveWeapon].m_Ammo--;

	if(!m_ReloadTimer)
	{
		// MineTee
		// FIXME: Very Ugly...
		if (m_pPlayer->IsBot() && m_ActiveWeapon == WEAPON_GRENADE)
			m_ReloadTimer = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Firedelay*4 * Server()->TickSpeed() / 1000;
		else
			m_ReloadTimer = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Firedelay * Server()->TickSpeed() / 1000;
	}
}

void CCharacter::HandleWeapons()
{
	//ninja
	HandleNinja();

	// check reload timer
	if(m_ReloadTimer)
	{
		m_ReloadTimer--;
		return;
	}

	// fire Weapon, if wanted
	FireWeapon();

	// ammo regen
	int AmmoRegenTime = g_pData->m_Weapons.m_aId[m_ActiveWeapon].m_Ammoregentime;
	if(AmmoRegenTime)
	{
		// If equipped and not active, regen ammo?
		if (m_ReloadTimer <= 0)
		{
			if (m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart < 0)
				m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = Server()->Tick();

			if ((Server()->Tick() - m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart) >= AmmoRegenTime * Server()->TickSpeed() / 1000)
			{
				// Add some ammo
				m_aWeapons[m_ActiveWeapon].m_Ammo = min(m_aWeapons[m_ActiveWeapon].m_Ammo + 1, 10);
				m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = -1;
			}
		}
		else
		{
			m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart = -1;
		}
	}

	return;
}

bool CCharacter::GiveWeapon(int Weapon, int Ammo)
{
	// MineTee
    if (str_find_nocase(GameServer()->GameType(), "minetee") && IsInventoryFull())
        return false;

	if(m_aWeapons[Weapon].m_Ammo < g_pData->m_Weapons.m_aId[Weapon].m_Maxammo || !m_aWeapons[Weapon].m_Got)
	{
	    m_aWeapons[Weapon].m_Ammo = min(g_pData->m_Weapons.m_aId[Weapon].m_Maxammo, Ammo);

        if (!m_aWeapons[Weapon].m_Got)
            UpdateInventory(Weapon);
        else
            UpdateInventory();

		m_aWeapons[Weapon].m_Got = true;

		return true;
	}
	return false;
}

void CCharacter::GiveNinja()
{
	m_Ninja.m_ActivationTick = Server()->Tick();
	m_aWeapons[WEAPON_NINJA].m_Got = true;
	m_aWeapons[WEAPON_NINJA].m_Ammo = -1;
	if (m_ActiveWeapon != WEAPON_NINJA)
		m_LastWeapon = m_ActiveWeapon;
	m_ActiveWeapon = WEAPON_NINJA;

	GameServer()->CreateSound(m_Pos, SOUND_PICKUP_NINJA);
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
		HandleWeaponSwitch();
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
	// MineTee
	if (m_pPlayer->IsBot())
        BotIA();

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
	int BlockID = GameServer()->Collision()->GetMineTeeTileAt(vec2(m_Pos.x+m_ProximityRadius/3.f, m_Pos.y-m_ProximityRadius/3.f));
	if (Server()->Tick() - TimerFluidDamage >= Server()->TickSpeed()/2 && (BlockID >= CBlockManager::LAVA_A && BlockID <= CBlockManager::LAVA_D))
	{
	    TakeDamage(vec2(0.0f,-1.0f), 1, m_pPlayer->GetCID(), WEAPON_WORLD);
	    TimerFluidDamage = Server()->Tick();
	}

	if (!inWater)
	{
        if ((BlockID >= CBlockManager::WATER_A && BlockID <= CBlockManager::WATER_D) && (Server()->Tick() - TimerFluidDamage >= Server()->TickSpeed()*8))
        {
            inWater = true;
            TimerFluidDamage = Server()->Tick();
        }
    }
	else
	{
	    if (BlockID < CBlockManager::WATER_A || BlockID > CBlockManager::WATER_D)
	    {
            inWater = false;
            TimerFluidDamage = Server()->Tick();
	    }
        else if (Server()->Tick() - TimerFluidDamage >= Server()->TickSpeed()*2)
        {
            TakeDamage(vec2(0.0f,-1.0f), 1, m_pPlayer->GetCID(), WEAPON_WORLD);
            TimerFluidDamage = Server()->Tick();
        }
	}
	//

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
	HandleWeapons();

	// Previnput
	m_PrevInput = m_Input;
	return;
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
	if (plHooked >= MAX_CLIENTS-MAX_BOTS && plHooked < MAX_CLIENTS && GameServer()->m_apPlayers[plHooked] &&
     (GameServer()->m_apPlayers[plHooked]->GetTeam() == TEAM_ANIMAL_TEECOW || GameServer()->m_apPlayers[plHooked]->GetTeam() == TEAM_ANIMAL_TEEPIG))
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
	if(m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart > -1)
		++m_aWeapons[m_ActiveWeapon].m_AmmoRegenStart;
	if(m_EmoteStop > -1)
		++m_EmoteStop;
}

bool CCharacter::IncreaseHealth(int Amount)
{
	if(m_Health >= 10)
		return false;
	m_Health = clamp(m_Health+Amount, 0, 10);
	return true;
}

bool CCharacter::IncreaseArmor(int Amount)
{
	if(m_Armor >= 10)
		return false;
	m_Armor = clamp(m_Armor+Amount, 0, 10);
	return true;
}

void CCharacter::Die(int Killer, int Weapon)
{
	// we got to wait 0.5 secs before respawning
	if (m_pPlayer->IsBot()) // MineTee
        m_pPlayer->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()*20;
	else
        m_pPlayer->m_RespawnTick = Server()->Tick()+Server()->TickSpeed()/2;
	int ModeSpecial = GameServer()->m_pController->OnCharacterDeath(this, GameServer()->m_apPlayers[Killer], Weapon);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "kill killer='%d:%d:%s' victim='%d:%d:%s' weapon=%d special=%d",
		Killer, GameServer()->m_apPlayers[Killer]->GetTeam(), Server()->ClientName(Killer),
		m_pPlayer->GetCID(), m_pPlayer->GetTeam(), Server()->ClientName(m_pPlayer->GetCID()), Weapon, ModeSpecial); // MineTee
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);

	// send the kill message
	if (m_pPlayer->IsBot() && Killer >= 0 && Killer < MAX_CLIENTS-MAX_BOTS) // MineTee
	{
		CNetMsg_Sv_KillMsg Msg;
		Msg.m_Killer = Killer;
		Msg.m_Victim = m_pPlayer->GetCID();
		Msg.m_Weapon = Weapon;
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
	if (str_find_nocase(GameServer()->GameType(), "minetee")  && m_pPlayer->GetTeam() <= TEAM_BLUE)
	    GameServer()->CreateTombstone(m_Pos);
}

bool CCharacter::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	m_Core.m_Vel += Force;

	if(GameServer()->m_pController->IsFriendlyFire(m_pPlayer->GetCID(), From) && !g_Config.m_SvTeamdamage)
		return false;

	// MineTee
    if (m_pPlayer->GetCID() >= MAX_CLIENTS-MAX_BOTS)
        m_BotTimeLastDamage = Server()->Tick();
    //

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
		Die(From, Weapon);

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

	// MineTee
    if ((g_Config.m_SvMonsters == 0 && m_pPlayer->GetTeam() >= TEAM_ENEMY_TEEPER && m_pPlayer->GetTeam() <= TEAM_ENEMY_SPIDERTEE) ||
        (g_Config.m_SvAnimals == 0 && m_pPlayer->GetTeam() >= TEAM_ANIMAL_TEECOW && m_pPlayer->GetTeam() <= TEAM_ANIMAL_TEEPIG))
        return;
    //

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

	pCharacter->m_Weapon = m_ActiveWeapon;
	pCharacter->m_AttackTick = m_AttackTick;

	pCharacter->m_Direction = m_Input.m_Direction;

	if(m_pPlayer->GetCID() == SnappingClient || SnappingClient == -1 ||
		(!g_Config.m_SvStrictSpectateMode && m_pPlayer->GetCID() == GameServer()->m_apPlayers[SnappingClient]->m_SpectatorID))
	{
		pCharacter->m_Health = m_Health;
		pCharacter->m_Armor = m_Armor;

		// MineTee
		if (m_ActiveWeapon >= NUM_WEAPONS)
		{
		    int ActiveBlock = m_ActiveWeapon - NUM_WEAPONS;
            if(m_aBlocks[ActiveBlock].m_Amount > 0)
                pCharacter->m_AmmoCount = m_aBlocks[ActiveBlock].m_Amount;
		}
		else if(m_aWeapons[m_ActiveWeapon].m_Ammo > 0)
			pCharacter->m_AmmoCount = m_aWeapons[m_ActiveWeapon].m_Ammo;
		//
	}

	if(pCharacter->m_Emote == EMOTE_NORMAL)
	{
		if(250 - ((Server()->Tick() - m_LastAction)%(250)) < 5)
			pCharacter->m_Emote = EMOTE_BLINK;
	}

	pCharacter->m_PlayerFlags = GetPlayer()->m_PlayerFlags;

	// MineTee
	if (str_find_nocase(GameServer()->GameType(), "minetee"))
	{
		int TileIndexD = GameServer()->Collision()->GetMineTeeTileAt(vec2(m_Pos.x, m_Pos.y+16.0f));
		int TileIndex = GameServer()->Collision()->GetMineTeeTileAt(vec2(m_Pos.x, m_Pos.y));
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

		if(m_pPlayer->GetCID() == SnappingClient && m_NeedSendInventory)
		{
			CNetObj_Inventory *pClientInventory = static_cast<CNetObj_Inventory *>(Server()->SnapNewItem(NETOBJTYPE_INVENTORY, m_pPlayer->GetCID(), sizeof(CNetObj_Inventory)));
			if(!pClientInventory)
				return;

			//TODO: Ugly
			mem_copy(&pClientInventory->m_Item1, m_Inventory.m_Items, sizeof(int)*NUM_ITEMS_INVENTORY);
			pClientInventory->m_Ammo1 = GetCurrentAmmo(pClientInventory->m_Item1);
			pClientInventory->m_Ammo2 = GetCurrentAmmo(pClientInventory->m_Item2);
			pClientInventory->m_Ammo3 = GetCurrentAmmo(pClientInventory->m_Item3);
			pClientInventory->m_Ammo4 = GetCurrentAmmo(pClientInventory->m_Item4);
			pClientInventory->m_Ammo5 = GetCurrentAmmo(pClientInventory->m_Item5);
			pClientInventory->m_Ammo6 = GetCurrentAmmo(pClientInventory->m_Item6);
			pClientInventory->m_Ammo7 = GetCurrentAmmo(pClientInventory->m_Item7);
			pClientInventory->m_Ammo8 = GetCurrentAmmo(pClientInventory->m_Item8);
			pClientInventory->m_Ammo9 = GetCurrentAmmo(pClientInventory->m_Item9);

			pClientInventory->m_Selected = m_Inventory.m_Selected;
			if (m_Inventory.m_Items[m_Inventory.m_Selected] >= NUM_WEAPONS)
			{
				CBlockManager::CBlockInfo BlockInfo;
				GameServer()->m_BlockManager.GetBlockInfo(m_Inventory.m_Items[m_Inventory.m_Selected]-NUM_WEAPONS, &BlockInfo);
				StrToInts(&pClientInventory->m_SelectedName0, 6, BlockInfo.m_aName);
			}
			else
				StrToInts(&pClientInventory->m_SelectedName0, 6, "");
			m_NeedSendInventory = false;
		}
	}
}

// MineTee
bool CCharacter::GiveBlock(int Block, int Amount)
{
    if (str_find_nocase(GameServer()->GameType(), "minetee") && IsInventoryFull() && !m_aBlocks[Block].m_Got)
        return false;

    if (Block >= CBlockManager::MAX_BLOCKS || Block < 0)
        return false;

    if (!m_aBlocks[Block].m_Got)
    {
        m_aBlocks[Block].m_Amount = Amount;
        m_aBlocks[Block].m_Got = true;

        UpdateInventory(NUM_WEAPONS+Block);
    }
    else
    {
        if (m_aBlocks[Block].m_Amount > 255)
            return false;

        m_aBlocks[Block].m_Amount+=Amount;

        UpdateInventory();
    }

    return true;
}

void CCharacter::UpdateInventory(int item)
{
	if (item != NUM_WEAPONS+CBlockManager::MAX_BLOCKS && IsInventoryFull())
	{
		vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));
        CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_DROPITEM, item-NUM_WEAPONS);
        pPickup->m_Pos = m_Pos;
        pPickup->m_Pos.y -= 18.0f;
        pPickup->m_Vel = vec2(Direction.x*5.0f, -5);
        pPickup->m_Amount = m_aBlocks[item-NUM_WEAPONS].m_Amount;
        pPickup->m_Owner = m_pPlayer->GetCID();
	}

    for (int i=0; i<NUM_ITEMS_INVENTORY; i++)
    {
        if (item != NUM_WEAPONS+CBlockManager::MAX_BLOCKS)
        {
            if (m_Inventory.m_Items[i] == NUM_WEAPONS+CBlockManager::MAX_BLOCKS)
            {
                m_Inventory.m_Items[i] = item;
                break;
            }
        }
        else
        {
            int Index = m_Inventory.m_Items[i];
            if (Index != NUM_WEAPONS+CBlockManager::MAX_BLOCKS)
            {
                if (Index >= NUM_WEAPONS)
                {
                    if (!m_aBlocks[Index-NUM_WEAPONS].m_Got)
                        m_Inventory.m_Items[i] = NUM_WEAPONS+CBlockManager::MAX_BLOCKS;
                }
                else if (!m_aWeapons[Index].m_Got)
                    m_Inventory.m_Items[i] = NUM_WEAPONS+CBlockManager::MAX_BLOCKS;
            }
        }
    }

    m_NeedSendInventory = true;
}

bool CCharacter::IsInventoryFull()
{
    for (int i=0; i<NUM_ITEMS_INVENTORY; i++)
    {
        if (m_Inventory.m_Items[i] == NUM_WEAPONS+CBlockManager::MAX_BLOCKS)
            return false;
    }

    return true;
}

void CCharacter::BotIA()
{
    //Sounds
    if (Server()->Tick() - m_BotTimeLastSound > Server()->TickSpeed()*5.0f)
    {
        if (m_pPlayer->GetTeam() == TEAM_ANIMAL_TEECOW)
            GameServer()->CreateSound(m_Pos, SOUND_ANIMAL_TEECOW);
        else if (m_pPlayer->GetTeam() == TEAM_ENEMY_ZOMBITEE)
            GameServer()->CreateSound(m_Pos, SOUND_ENEMY_ZOMBITEE);
        m_BotTimeLastSound = Server()->Tick();
    }

    //Run actions
    if (m_pPlayer->GetTeam() == TEAM_ENEMY_TEEPER && (Server()->Tick() - m_BotTimePlayerFound > Server()->TickSpeed()*0.35f || Server()->Tick()-m_BotTimeGrounded > Server()->TickSpeed()*4))
    {

        Die(m_pPlayer->GetCID(), WEAPON_WORLD);
        GameServer()->CreateExplosion(m_Pos, m_pPlayer->GetCID(), WEAPON_WORLD, false);
        GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
        return;
    }
    else if (m_pPlayer->GetTeam() == TEAM_ENEMY_ZOMBITEE)
    {
        if (Server()->Tick()-m_BotTimeGrounded > Server()->TickSpeed()*4)
        {
            Die(m_pPlayer->GetCID(), WEAPON_WORLD);
            return;
        }
        if (m_BotClientIDFix != -1 && GameServer()->m_apPlayers[m_BotClientIDFix])
        {
            CCharacter *pChar = GameServer()->m_apPlayers[m_BotClientIDFix]->GetCharacter();
            if (!pChar)
            {
                m_BotClientIDFix = -1;
                return;
            }

            vec2 DirHit = vec2(0.f, -1.f);
            if (length(pChar->m_Pos - m_Pos) > 0.0f)
                DirHit = normalize(pChar->m_Pos - m_Pos);
            pChar->TakeDamage(vec2(0.f, -1.f) + normalize(DirHit + vec2(0.f, -1.1f)) * 10.0f, 3, m_pPlayer->GetCID(), WEAPON_WORLD);
            GameServer()->CreateHammerHit(m_Pos);
            GameServer()->CreateSound(m_Pos, SOUND_HIT);
            m_BotDir = 0;

            m_BotClientIDFix = -1;
            return;
        }
    }
    else if (m_pPlayer->GetTeam() == TEAM_ENEMY_SKELETEE)
    {
        if (Server()->Tick()-m_BotTimeGrounded > Server()->TickSpeed()*4)
        {
            Die(m_pPlayer->GetCID(), WEAPON_WORLD);
            return;
        }
        if (m_BotClientIDFix != -1 && GameServer()->m_apPlayers[m_BotClientIDFix])
        {
            CCharacter *pChar = GameServer()->m_apPlayers[m_BotClientIDFix]->GetCharacter();
            if (pChar && !GameServer()->Collision()->IntersectLine(m_Pos, pChar->m_Pos, 0x0, 0x0) && GameServer()->IntersectCharacter(m_Pos, pChar->m_Pos, 0x0, m_pPlayer->GetCID()) == m_BotClientIDFix)
            {
				m_LatestInput.m_Fire = 1;
				m_LatestPrevInput.m_Fire = 1;
				m_Input.m_Fire = 1;
				m_BotDir = 0;
            }

            m_BotClientIDFix = -1;
            return;
        }
    }

    //Clean m_Input
	m_Input.m_Hook = 0;
	m_Input.m_Fire = 0;
	m_Input.m_Jump = 0;

    //Interact with users
    bool PlayerClose = false;
    bool PlayerFound = false;
    float LessDist = 500.0f;

    m_BotClientIDFix = -1;
	for (int i=0; i<MAX_CLIENTS-MAX_BOTS; i++)
	{
	    CPlayer *pPlayer = GameServer()->m_apPlayers[i];
	    if (!pPlayer || !pPlayer->GetCharacter() || pPlayer->IsBot())
            continue;

        int Dist = distance(pPlayer->GetCharacter()->m_Pos, m_Pos);
        if (Dist < LessDist)
            LessDist = Dist;
        else
            continue;

	    if (Dist < 280.0f)
        {
            if (Dist > 120.0f)
            {
                vec2 DirPlayer = normalize(pPlayer->GetCharacter()->m_Pos - m_Pos);

                bool isHooked = false;
                for (int e=0; e<MAX_CLIENTS-MAX_BOTS; e++)
                {
                    if (!GameServer()->m_apPlayers[e] || !GameServer()->m_apPlayers[e]->GetCharacter())
                        continue;

                    int HookedPL = GameServer()->m_apPlayers[e]->GetCharacter()->GetCore()->m_HookedPlayer;
                    if (HookedPL < 0 || HookedPL != m_pPlayer->GetCID() || !GameServer()->m_apPlayers[HookedPL])
                        continue;

                    if (GameServer()->m_apPlayers[HookedPL]->GetTeam() == TEAM_ANIMAL_TEECOW || GameServer()->m_apPlayers[HookedPL]->GetTeam() == TEAM_ANIMAL_TEEPIG)
                    {
                        isHooked = true;
                        break;
                    }
                }

                if (m_pPlayer->GetTeam() == TEAM_ANIMAL_TEECOW || m_pPlayer->GetTeam() == TEAM_ANIMAL_TEEPIG)
                    m_BotDir = 0;
                else if (m_pPlayer->GetTeam() == TEAM_ENEMY_SKELETEE)
                {
                    m_BotDir = 0;
                    m_BotClientIDFix = pPlayer->GetCID();
                }
                else
                {
                    if (DirPlayer.x < 0)
                        m_BotDir = -1;
                    else
                        m_BotDir = 1;
                }
            }
            else
            {
                PlayerClose = true;

                if (m_pPlayer->GetTeam() == TEAM_ENEMY_TEEPER)
                    m_BotDir = 0;
                else if (m_pPlayer->GetTeam() == TEAM_ENEMY_ZOMBITEE && Dist < 32.0f)
                {
                    m_BotDir = 0;
                    m_BotClientIDFix = pPlayer->GetCID();
                }
            }


            m_Input.m_TargetX = static_cast<int>(pPlayer->GetCharacter()->m_Pos.x - m_Pos.x);
            m_Input.m_TargetY = static_cast<int>(pPlayer->GetCharacter()->m_Pos.y - m_Pos.y);

            PlayerFound = true;
        }
	}

    //Fix target
    if (!PlayerFound)
    {
        m_Input.m_TargetX = m_BotDir;
        m_Input.m_TargetY = 0;
    }
    else if (m_pPlayer->GetTeam() < TEAM_ANIMAL_TEECOW && m_BotClientIDFix != -1)
    {
    	CPlayer *pPlayer = GameServer()->m_apPlayers[m_BotClientIDFix];
    	if (pPlayer && pPlayer->GetCharacter() && m_Pos.y > pPlayer->GetCharacter()->m_Pos.y) // Jump to player
    		m_Input.m_Jump = 1;
    }

    //Random Actions to animals
	if (m_pPlayer->GetTeam() == TEAM_ANIMAL_TEECOW || m_pPlayer->GetTeam() == TEAM_ANIMAL_TEEPIG || !PlayerFound)
	{
        if (Server()->Tick()-m_BotTimeLastOption > Server()->TickSpeed()*10.0f)
        {
            int Action = rand()%3;
            if (Action == 0)
                m_BotDir = -1;
            else if (Action == 1)
                m_BotDir = 1;
            else if (Action == 2)
                m_BotDir = 0;

            m_BotTimeLastOption = Server()->Tick();
        }
	}

    //Interact with the envirionment
	float radiusZ = ms_PhysSize/2.0f;
    if (distance(m_Pos, m_BotLastPos) < radiusZ || abs(m_Pos.x-m_BotLastPos.x) < radiusZ)
    {
        if (Server()->Tick() - m_BotLastStuckTime > Server()->TickSpeed()*0.5f)
        {
            m_BotStuckCount++;
            if (m_BotStuckCount == 15)
            {
            	if (!m_BotJumpTry)
                {
            		m_Input.m_Jump = 1;
            		m_BotJumpTry = true;
                }
            	else
            	{
            		m_BotDir = (!(rand()%2))?1:-1;
            		m_BotJumpTry = false;
            	}

                m_BotStuckCount = 0;
                m_BotLastStuckTime = Server()->Tick();
            }
        }
    }

    //Fix Stuck
    if (IsGrounded())
        m_BotTimeGrounded = Server()->Tick();

    //Falls
    if (m_pPlayer->GetTeam() != TEAM_ENEMY_ZOMBITEE && m_pPlayer->GetTeam() != TEAM_ANIMAL_TEECOW  && m_pPlayer->GetTeam() != TEAM_ANIMAL_TEEPIG && m_Core.m_Vel.y > GameServer()->Tuning()->m_Gravity)
    {
    	if (m_BotClientIDFix != -1)
    	{
    		CPlayer *pPlayer = GameServer()->m_apPlayers[m_BotClientIDFix];
    		if (pPlayer && pPlayer->GetCharacter() && m_Pos.y > pPlayer->GetCharacter()->m_Pos.y)
    			m_Input.m_Jump = 1;
    		else
    			m_Input.m_Jump = 0;
    	}
    	else
    		m_Input.m_Jump = 1;
    }

    // Fluids
    if (GameServer()->m_BlockManager.IsFluid(GameServer()->Collision()->GetMineTeeTileAt(m_Pos)))
    	m_Input.m_Jump = 1;

    //Limits
    int tx = m_Pos.x+m_BotDir*45.0f;
    if (tx < 0)
        m_BotDir = 1;
    else if (tx >= GameServer()->Collision()->GetWidth()*32.0f)
    	m_BotDir = -1;

    //Delay of actions
    if (!PlayerClose)
        m_BotTimePlayerFound = Server()->Tick();

    //Set data
    m_Input.m_Direction = m_BotDir;
	m_Input.m_PlayerFlags = PLAYERFLAG_PLAYING;
	//Check for legacy input
	if (m_LatestInput.m_Fire && m_Input.m_Fire) m_Input.m_Fire = 0;
	if (m_LatestInput.m_Jump && m_Input.m_Jump) m_Input.m_Jump = 0;
	//Ceck Double Jump
	if (m_Input.m_Jump && (m_Jumped&1) && !(m_Jumped&2) && m_Core.m_Vel.y < GameServer()->Tuning()->m_Gravity)
		m_Input.m_Jump = 0;

	m_LatestPrevInput = m_LatestInput = m_Input;
	m_BotLastPos = m_Pos;
}

int CCharacter::GetCurrentAmmo(int wid)
{
    if (wid < 0 || wid >= NUM_WEAPONS+CBlockManager::MAX_BLOCKS)
        return 0;

    if (wid >= NUM_WEAPONS)
        return m_aBlocks[wid-NUM_WEAPONS].m_Amount;
    else
        return m_aWeapons[wid].m_Ammo;

    return 0;
}

void CCharacter::DropItem(int ItemID)
{
    if (ItemID == -1)
        ItemID = m_Inventory.m_Selected;

    if (ItemID == 0 || ItemID >= NUM_ITEMS_INVENTORY)
        return;

    bool dropped = false;
    int Index = m_Inventory.m_Items[ItemID];
    if (Index != NUM_WEAPONS+CBlockManager::MAX_BLOCKS)
    {
        vec2 Direction = normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY));

        if (Index >= NUM_WEAPONS)
        {
            if (m_aBlocks[Index-NUM_WEAPONS].m_Got)
            {
                m_Inventory.m_Items[ItemID] = NUM_WEAPONS+CBlockManager::MAX_BLOCKS;
                m_aBlocks[Index-NUM_WEAPONS].m_Got = false;
                CPickup *pPickup = new CPickup(&GameServer()->m_World, POWERUP_DROPITEM, Index-NUM_WEAPONS);
                pPickup->m_Pos = m_Pos;
                pPickup->m_Pos.y -= 18.0f;
                pPickup->m_Vel = vec2(Direction.x*5.0f, -5);
                pPickup->m_Amount = m_aBlocks[Index-NUM_WEAPONS].m_Amount;
                pPickup->m_Owner = m_pPlayer->GetCID();
                dropped = true;
            }
        }

        if (dropped)
        {
            int ActiveItem = 0;
            for (int i=m_Inventory.m_Selected-1; i>=0; i--)
            {
                if (m_Inventory.m_Items[i] != NUM_WEAPONS+CBlockManager::MAX_BLOCKS)
                {
                    ActiveItem = m_Inventory.m_Items[i];
                    m_Inventory.m_Selected = i;
                    break;
                }
            }

            SetWeapon(ActiveItem);
        }
    }

    UpdateInventory();
}

