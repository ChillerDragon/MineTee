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
	m_BotJumpTry = false;
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

    //Fix Stuck
    if (IsGrounded())
        m_BotTimeGrounded = Server()->Tick();

    // Chat
    if (Server()->Tick()-m_BotTimeLastChat > 60*Server()->TickSpeed())
    {
		char aBuff[255];
		CCharacter *aEnts[MAX_BOTS], *aEntsPlayers[MAX_BOTS];
		static const char *pPhrases[5] = {
			"[DUNE BOSS] Hi %s, you use Gamer Client?\0",
			"[DUNE BOSS] What %s?! Why you are playing this mod? This is not Teeworlds!\0",
			"[DUNE BOSS] Ei %s tell to everybody that Gamer Client is the best client!\0",
			"[DUNE BOSS] Knock %s! Fight with me if you like die... muahahahaha\0",
			"[DUNE BOSS] Please %s, i'm a boss... run and safe your life!\0",
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
			str_format(aBuff, sizeof(aBuff), pPhrases[rand()%5], Server()->ClientName(SelPlayerID));
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
