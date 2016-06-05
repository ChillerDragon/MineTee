/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>
#include "monster.h"
#include "../pickup.h"
#include "../character.h"
#include "../laser.h"
#include "../projectile.h"

MACRO_ALLOC_POOL_ID_IMPL(CMonster, MAX_CLIENTS)

CMonster::CMonster(CGameWorld *pWorld)
: CCharacter(pWorld)
{
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
}

void CMonster::Tick()
{
	if (g_Config.m_SvMonsters == 0)
	{
		CCharacter::Die(m_pPlayer->GetCID(), WEAPON_GAME);
		return;
	}

	TickBotAI();
	CCharacter::Tick();
}

bool CMonster::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	if (CCharacter::TakeDamage(Force, Dmg, From, Weapon))
	{
		m_BotTimeLastDamage = Server()->Tick();
		return true;
	}

	return false;
}

void CMonster::PlaySound()
{
	switch(m_pPlayer->GetBotSubType())
	{
		case CPlayer::BOT_MONSTER_ZOMBITEE:
			GameServer()->CreateSound(m_Pos, SOUND_ENEMY_ZOMBITEE);
			break;
	}
}

void CMonster::RunAction()
{
	switch(m_pPlayer->GetBotSubType())
	{
		case CPlayer::BOT_MONSTER_TEEPER:
			if ((Server()->Tick() - m_BotTimePlayerFound > Server()->TickSpeed()*0.35f || Server()->Tick()-m_BotTimeGrounded > Server()->TickSpeed()*4))
			{ // Kill-Self

				Die(m_pPlayer->GetCID(), WEAPON_WORLD);
				GameServer()->CreateExplosion(m_Pos, m_pPlayer->GetCID(), WEAPON_WORLD, false);
				GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
			}
			break;

		case CPlayer::BOT_MONSTER_EYE:
		case CPlayer::BOT_MONSTER_ZOMBITEE:
			if (m_pPlayer->GetBotSubType() == CPlayer::BOT_MONSTER_EYE)
			{
				if (m_BotClientIDFix == -1)
		            m_Core.m_Vel = vec2(rand()%2,rand()%2);
				else
					m_Core.m_Vel.y = -GameServer()->Tuning()->m_Gravity;
			}
	        else if (Server()->Tick()-m_BotTimeGrounded > Server()->TickSpeed()*4)
	        {
	            Die(m_pPlayer->GetCID(), WEAPON_WORLD);
	            break;
	        }

	        if (m_BotClientIDFix != -1 && GameServer()->m_apPlayers[m_BotClientIDFix])
	        {
	        	CCharacter *pChar = GameServer()->m_apPlayers[m_BotClientIDFix]->GetCharacter();
	            if (!pChar)
	            {
	                m_BotClientIDFix = -1;
	                return;
	            }

	            if (Server()->Tick() - m_BotTimePlayerFound > Server()->TickSpeed()*0.45f)
	            {
					vec2 DirHit = vec2(0.f, -1.f);
					if (length(pChar->m_Pos - m_Pos) > 0.0f)
						DirHit = normalize(pChar->m_Pos - m_Pos);
					pChar->TakeDamage(vec2(0.f, -1.f) + normalize(DirHit + vec2(0.f, -1.1f)) * 10.0f, 3, m_pPlayer->GetCID(), WEAPON_WORLD);
					GameServer()->CreateHammerHit(m_Pos);
					GameServer()->CreateSound(m_Pos, SOUND_HIT);
					m_BotDir = 0;
					m_BotTimePlayerFound = Server()->Tick();
	            }

	            m_BotClientIDFix = -1;
	        }
			break;

		case CPlayer::BOT_MONSTER_SKELETEE:
	        if (Server()->Tick()-m_BotTimeGrounded > Server()->TickSpeed()*4)
	        {
	            Die(m_pPlayer->GetCID(), WEAPON_WORLD);
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
	        }
	        break;
	}
}

void CMonster::TickBotAI()
{
    //Sounds
    if (Server()->Tick() - m_BotTimeLastSound > Server()->TickSpeed()*5.0f)
    {
        PlaySound();
        m_BotTimeLastSound = Server()->Tick();
    }

    //Run actions
    RunAction();

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

        if (m_pPlayer->GetBotSubType() == CPlayer::BOT_MONSTER_EYE && Dist < 600.0f)
        {
        	const vec2 dir = normalize(pPlayer->GetCharacter()->m_Pos-m_Pos);
        	m_Core.m_Vel = dir*6.0f;
        }

        if (Dist < 280.0f)
        {
            if (Dist > 120.0f)
            {
                vec2 DirPlayer = normalize(pPlayer->GetCharacter()->m_Pos - m_Pos);
                if (m_pPlayer->GetBotSubType() == CPlayer::BOT_MONSTER_SKELETEE)
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

                if (m_pPlayer->GetBotSubType() == CPlayer::BOT_MONSTER_TEEPER)
                    m_BotDir = 0;
                else if ((m_pPlayer->GetBotSubType() == CPlayer::BOT_MONSTER_ZOMBITEE || m_pPlayer->GetBotSubType() == CPlayer::BOT_MONSTER_EYE) && Dist < 42.0f)
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
    else if (m_BotClientIDFix != -1)
    {
    	CPlayer *pPlayer = GameServer()->m_apPlayers[m_BotClientIDFix];
    	if (pPlayer && pPlayer->GetCharacter() && m_Pos.y > pPlayer->GetCharacter()->m_Pos.y) // Jump to player
    		m_Input.m_Jump = 1;
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

    //Fix Stuck
    if (IsGrounded())
        m_BotTimeGrounded = Server()->Tick();

    //Falls
    if (m_pPlayer->GetBotSubType() != CPlayer::BOT_MONSTER_ZOMBITEE && m_Core.m_Vel.y > GameServer()->Tuning()->m_Gravity)
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

    // Disables
    if (m_pPlayer->GetBotSubType() == CPlayer::BOT_MONSTER_EYE)
    {
    	m_Input.m_Jump = 0;
    }

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
