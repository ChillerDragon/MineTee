/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "boss.h"

#include <new>
#include <engine/shared/config.h>
#include <game/server/gamecontext.h>
#include <game/mapitems.h>
#include "../pickup.h"
#include "../character.h"
#include "../laser.h"
#include "../projectile.h"

MACRO_ALLOC_POOL_ID_IMPL(CBoss, MAX_CLIENTS)

CBoss::CBoss(CGameWorld *pWorld)
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

bool CBoss::TakeDamage(vec2 Force, int Dmg, int From, int Weapon)
{
	return CCharacter::TakeDamage(Force, Dmg, From, Weapon);
}
