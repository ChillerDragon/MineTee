/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "bossdune.h"

#include <new>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>
#include "../pickup.h"
#include "../character.h"
#include "../laser.h"
#include "../projectile.h"

MACRO_ALLOC_POOL_ID_IMPL(CBossDune, MAX_CLIENTS)

CBossDune::CBossDune(CGameWorld *pWorld)
: CCharacter(pWorld)
{
	m_BotDir = 0;
	m_BotLastPos = m_Pos;
	m_BotLastStuckTime = 0.0f;
	m_BotStuckCount = 0;
	m_BotTimePlayerFound = Server()->Tick();
	m_BotTimeGrounded = Server()->Tick();
	m_BotTimeLastOption = Server()->Tick();
	m_BotTimeLastDamage = 0.0f;
	m_BotClientIDFix = -1;
	m_BotTimeLastSound = Server()->Tick();
	m_BotTimeLastChat = Server()->Tick();
	m_BotTimeFirstDamage = Server()->Tick();
	m_BotTimeExplosion = Server()->Tick();
	m_BotTimeLastWeaponChange = m_BotTimeNextWeaponChange = Server()->Tick();
	m_BotJumpTry = false;
	m_Damaged = false;
}

void CBossDune::Tick()
{
	TickBotAI();
	CCharacter::Tick();
}

bool CBossDune::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	if (CCharacter::TakeDamage(Force, Dmg, From, Weapon))
	{
		m_BotTimeLastDamage = Server()->Tick();
		if (!m_Damaged)
		{
			char aBuf[255];
			str_format(aBuf, sizeof(aBuf), "[DUNE BOSS] Ohh... '%s' found me! Diiieeee!!!!", Server()->ClientName(From));
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
			m_BotTimeFirstDamage = Server()->Tick();
		}
		m_Damaged = true;
		return true;
	}

	return false;
}

void CBossDune::TickBotAI()
{
	bool PlayerClose = false;

    //Clean m_Input
	m_Input.m_Direction = 0;
	m_Input.m_Hook = 0;
	m_Input.m_Fire = 0;
	m_Input.m_Jump = 0;

	// Attack!
	if (m_Damaged)
	{
		// Hello Explosions
		if (Server()->Tick()-m_BotTimeFirstDamage < 3.0f*Server()->TickSpeed() && Server()->Tick()-m_BotTimeExplosion >= 20)
		{
			vec2 ExpPos = { (float)((!(rand()%1)?1:-1)*rand()%60), (float)((!(rand()%1)?1:-1)*rand()%60) };
			GameServer()->CreateExplosion(m_Pos+ExpPos, m_pPlayer->GetCID(), WEAPON_GAME, false, m_pPlayer->GetCID());
			GameServer()->CreateSound(m_Pos+ExpPos, SOUND_GRENADE_EXPLODE);
			m_BotTimeExplosion = Server()->Tick();
		}

		//Interact with users
	    bool PlayerFound = false;
	    float LessDist = 500.0f;

	    m_BotClientIDFix = -1;
		for (int i=0; i<g_Config.m_SvMaxClients; i++)
		{
		    CPlayer *pPlayer = GameServer()->m_apPlayers[i];
		    if (!pPlayer || !pPlayer->GetCharacter() || pPlayer->IsBot())
	            continue;

	        int Dist = distance(pPlayer->GetCharacter()->m_Pos, m_Pos);
	        if (Dist < LessDist)
	            LessDist = Dist;
	        else
	            continue;

	        if (m_pPlayer->GetBotSubType() == BOT_MONSTER_EYE && Dist < 600.0f)
	        {
	        	const vec2 dir = normalize(pPlayer->GetCharacter()->m_Pos-m_Pos);
	        	m_Core.m_Vel = dir*6.0f;
	        }

	        if (Dist < 280.0f)
	        {
	            if (Dist > 120.0f)
	            {
	                vec2 DirPlayer = normalize(pPlayer->GetCharacter()->m_Pos - m_Pos);
	                if (m_FastInventory[m_ActiveInventoryItem].m_ItemId == WEAPON_GRENADE)
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

	                if (m_FastInventory[m_ActiveInventoryItem].m_ItemId == WEAPON_HAMMER_IRON)
	                {
	                	if (Dist < 42.0f)
	                	{
							m_BotDir = 0;
							m_BotClientIDFix = pPlayer->GetCID();
	                	}
	                }
	                else
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

		// Auto Change Weapon
		if (Server()->Tick()-m_BotTimeLastWeaponChange > m_BotTimeNextWeaponChange)
		{
			CCharacter::SetInventoryItem(rand()%4);
			m_BotTimeLastWeaponChange = Server()->Tick();
			m_BotTimeNextWeaponChange = ((rand()%4)+1)*Server()->TickSpeed();
		}

	    //Fix target
	    if (!PlayerFound)
	    {
	        m_Input.m_TargetX = m_BotDir;
	        m_Input.m_TargetY = 0;
	    }
	    else if (m_BotClientIDFix != -1)
	    {
	    	CPlayer *pPlayer = GameServer()->m_apPlayers[m_BotClientIDFix];
	    	if (pPlayer && pPlayer->GetCharacter() && m_Pos.y > pPlayer->GetCharacter()->m_Pos.y) // Jump to player
	    		m_Input.m_Jump = 1;

	    	m_LatestInput.m_Fire = m_Input.m_Fire = 1;
	    }

	    //Random Actions
		if (!PlayerFound)
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
	}

    //Fix Stuck
    if (IsGrounded())
        m_BotTimeGrounded = Server()->Tick();

    // Chat
    if (Server()->Tick()-m_BotTimeLastChat > 60*Server()->TickSpeed())
    {
		char aBuff[255];
		CCharacter *aEnts[MAX_BOTS], *aEntsPlayers[MAX_BOTS];
		static const char *pPhrases[6] = {
			"[DUNE BOSS] Hi %s, you use Gamer Client? no??? DIEEE!!\0",
			"[DUNE BOSS] What %s?! Why you are playing this mod? This is not Teeworlds!\0",
			"[DUNE BOSS] I can see you... ¬¬\0",
			"[DUNE BOSS] Knock %s! Fight with me if you like die... muahahahaha\0",
			"[DUNE BOSS] Please %s, i'm a boss... run and safe your life!\0",
			"[DUNE BOSS] ... ... ...\0",
		};
		int Num = GameServer()->m_World.FindEntities(m_Pos, 800.0f, (CEntity**)aEnts, MAX_BOTS, CGameWorld::ENTTYPE_CHARACTER);
		int NumPlayers = 0;
		for (int i=0; i<Num; i++)
		{
			if (aEnts[i]->GetPlayer()->IsBot())
				continue;
			aEntsPlayers[NumPlayers++] = aEnts[i];
		}
		if (NumPlayers > 0)
		{
			int SelPlayerID = aEntsPlayers[rand()%NumPlayers]->GetPlayer()->GetCID();
			str_format(aBuff, sizeof(aBuff), pPhrases[rand()%6], Server()->ClientName(SelPlayerID));
			GameServer()->SendChatTarget(SelPlayerID, aBuff);
		}
		m_BotTimeLastChat = Server()->Tick();
    }

    //Falls
    if (m_Core.m_Vel.y > GameServer()->Tuning()->m_Gravity)
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
    if (GameServer()->m_BlockManager.IsFluid(GameServer()->Collision()->GetMineTeeTileIndexAt(m_Pos)))
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
	if (m_LatestPrevInput.m_Fire && m_Input.m_Fire) m_Input.m_Fire = 0;
	if (m_LatestInput.m_Jump && m_Input.m_Jump) m_Input.m_Jump = 0;
	//Ceck Double Jump
	if (m_Input.m_Jump && (m_Jumped&1) && !(m_Jumped&2) && m_Core.m_Vel.y < GameServer()->Tuning()->m_Gravity)
		m_Input.m_Jump = 0;

	m_LatestPrevInput = m_LatestInput;
	m_LatestInput = m_Input;
	m_BotLastPos = m_Pos;
	CCharacter::FireWeapon();
}
