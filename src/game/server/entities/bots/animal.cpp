/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>
#include "animal.h"
#include "../pickup.h"
#include "../character.h"
#include "../laser.h"
#include "../projectile.h"

MACRO_ALLOC_POOL_ID_IMPL(CAnimal, MAX_CLIENTS)

CAnimal::CAnimal(CGameWorld *pWorld)
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

void CAnimal::Tick()
{
	if (g_Config.m_SvAnimals == 0)
	{
		CCharacter::Die(m_pPlayer->GetCID(), WEAPON_GAME);
		return;
	}

	TickBotAI();
	CCharacter::Tick();
}

bool CAnimal::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	if (CCharacter::TakeDamage(Force, Dmg, From, Weapon))
	{
		m_BotTimeLastDamage = Server()->Tick();
		return true;
	}

	return false;
}

void CAnimal::TickBotAI()
{
    //Sounds
    if (Server()->Tick() - m_BotTimeLastSound > Server()->TickSpeed()*5.0f && !(rand()%50))
    {
        if (m_pPlayer->GetBotSubType() == BOT_ANIMAL_COW)
            GameServer()->CreateSound(m_Pos, SOUND_ANIMAL_TEECOW);
        if (m_pPlayer->GetBotSubType() == BOT_ANIMAL_SHEEP)
        	GameServer()->CreateSound(m_Pos, SOUND_ANIMAL_TEESHEEP);
        if (m_pPlayer->GetBotSubType() == BOT_ANIMAL_PIG)
            GameServer()->CreateSound(m_Pos, SOUND_ANIMAL_TEEPIG);
        m_BotTimeLastSound = Server()->Tick();
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

	    if (Dist < 280.0f)
        {
            if (Dist > 120.0f)
            {
                /*vec2 DirPlayer = normalize(pPlayer->GetCharacter()->m_Pos - m_Pos);
                bool isHooked = false;
                for (int e=0; e<g_Config.m_SvMaxClients; e++)
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
                }*/

                m_BotDir = 0;
            }
            else
                PlayerClose = true;


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

    //Random Actions to animals
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
	if (m_LatestInput.m_Fire && m_Input.m_Fire) m_Input.m_Fire = 0;
	if (m_LatestInput.m_Jump && m_Input.m_Jump) m_Input.m_Jump = 0;
	//Ceck Double Jump
	if (m_Input.m_Jump && (m_Jumped&1) && !(m_Jumped&2) && m_Core.m_Vel.y < GameServer()->Tuning()->m_Gravity)
		m_Input.m_Jump = 0;

	m_LatestPrevInput = m_LatestInput = m_Input;
	m_BotLastPos = m_Pos;
}
