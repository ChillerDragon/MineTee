/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "dm.h"


CGameControllerDM::CGameControllerDM(class CGameContext *pGameServer)
: CGameController(pGameServer)
{
	m_pGameType = "DM";
}

void CGameControllerDM::Tick()
{
	CGameController::Tick();
}
