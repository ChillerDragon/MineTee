/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <base/cephes_math.h>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>
#include "animal.h"
#include "../pickup.h"
#include "../character.h"
#include "../laser.h"
#include "../projectile.h"

MACRO_ALLOC_POOL_ID_IMPL(CPet, MAX_CLIENTS)

CPet::CPet(CGameWorld *pWorld)
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

void CPet::Tick()
{
	TickBotAI();
	CCharacter::Tick();
}

bool CPet::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	if (CCharacter::TakeDamage(Force, Dmg, From, Weapon))
	{
		m_BotTimeLastDamage = Server()->Tick();
		return true;
	}

	return false;
}

void CPet::TickBotAI()
{
	// Remove Gravity
	m_Core.m_Vel.y = -GameServer()->Tuning()->m_Gravity;
	m_Core.m_Vel.x = 0.0f;

    //Clean m_Input
	m_Input.m_Hook = 0;
	m_Input.m_Fire = 0;
	m_Input.m_Jump = 0;

	// Follow Owner
	m_BotDir = 0;
	if (m_pOwner && m_pOwner->GetCharacter())
	{
		const vec2 charPos = m_pOwner->GetCharacter()->m_Pos-vec2(ms_PhysSize*1.25f, ms_PhysSize*1.25f);
		const float dist = length(charPos - m_Pos);
		if (dist > 1500.0f)
		{
			GameServer()->CreatePlayerSpawn(charPos);
			m_Core.m_Pos = m_Pos = charPos;
		}
		else if (dist > 150.0f)
		{
			const vec2 dir = normalize(charPos-m_Pos);
			if (m_Pos.x > charPos.x)
				m_BotDir = -1;
			else
				m_BotDir = 1;
			m_Pos += dir*8.0f;
			m_Core.m_Pos = m_Pos;
			m_Input.m_TargetX = static_cast<int>(charPos.x - m_Pos.x);
			m_Input.m_TargetY = static_cast<int>(charPos.y - m_Pos.y);
		}
		else
		{
			float timelaps = (int)time_get()/time_freq();
			m_Pos.x += cephes_cosf(timelaps);
			m_Pos.y += cephes_sinf(timelaps);
			m_Core.m_Pos = m_Pos;

			for (int i=MAX_CLIENTS-MAX_BOTS; i<MAX_CLIENTS; i++)
			{
				CPlayer *pPlayer = GameServer()->m_apPlayers[i];
				if (!pPlayer || pPlayer == m_pPlayer || !pPlayer->GetCharacter())
					continue;

				const vec2 botPos = pPlayer->GetCharacter()->m_Pos;
				if (length(botPos-m_Pos) < 300.0f && !GameServer()->Collision()->IntersectLine(m_Pos, botPos, 0x0, 0x0))
				{
					m_Input.m_TargetX = static_cast<int>(pPlayer->GetCharacter()->m_Pos.x - m_Pos.x);
					m_Input.m_TargetY = static_cast<int>(pPlayer->GetCharacter()->m_Pos.y - m_Pos.y);
					m_Input.m_Fire = 1;
					break;
				}
			}
		}
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
