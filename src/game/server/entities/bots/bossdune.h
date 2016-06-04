/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_BOT_DUNE_BOSS_H
#define GAME_SERVER_ENTITIES_BOT_DUNE_BOSS_H

#include <game/server/entity.h>
#include <game/generated/server_data.h>
#include <game/generated/protocol.h>
#include <game/block_manager.h> // MineTee

#include <game/gamecore.h>
#include "boss.h"

class CBossDune : public CBoss
{
public:
	CBossDune(CGameWorld *pWorld);

	virtual void Tick();
	virtual bool TakeDamage(vec2 Force, int Dmg, int From, int Weapon);

private:
	void TickBotAI();
};

#endif
