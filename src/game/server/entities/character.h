/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* File modified by Alexandre Díaz */
#ifndef GAME_SERVER_ENTITIES_CHARACTER_H
#define GAME_SERVER_ENTITIES_CHARACTER_H

#include <game/server/entity.h>
#include <game/generated/server_data.h>
#include <game/generated/protocol.h>
#include <game/block_manager.h> // MineTee
#include <engine/shared/network.h> // MineTee

#include <game/gamecore.h>

enum
{
	WEAPON_GAME = -3, // team switching etc
	WEAPON_SELF = -2, // console kill command
	WEAPON_WORLD = -1, // death tiles etc
};

class CCharacter : public CEntity
{
	MACRO_ALLOC_POOL_ID()
	friend class CMonster;
	friend class CAnimal;
	friend class CPet;
	friend class CBossDune;

public:

	//character's size
	static const int ms_PhysSize = 28;

	CCharacter(CGameWorld *pWorld);

	virtual void Reset();
	virtual void Destroy();
	virtual void Tick();
	virtual void TickDefered();
	virtual void TickPaused();
	virtual void Snap(int SnappingClient);

	bool IsGrounded();

	void SetInventoryItem(int Index);
	void HandleInventoryItemSwitch();
	void DoInventoryItemSwitch();

	void HandleInventoryItems();
	void HandleNinja();

	void OnPredictedInput(CNetObj_PlayerInput *pNewInput);
	void OnDirectInput(CNetObj_PlayerInput *pNewInput);
	void ResetInput();
	void FireWeapon();

	virtual void Die(int Killer, int ItemID); // MineTee
	virtual bool TakeDamage(vec2 Force, int Dmg, int From, int ItemID); // MineTee

	virtual bool Spawn(class CPlayer *pPlayer, vec2 Pos); // MineTee
	bool Remove();

	bool IncreaseHealth(int Amount);
	bool IncreaseArmor(int Amount);

	int GiveItem(int ItemID, int Amount);
	void GiveNinja();

	void SetEmote(int Emote, int Tick);

	bool IsAlive() const { return m_Alive; }
	class CPlayer *GetPlayer() { return m_pPlayer; }

    // MineTee
	bool m_NeedSendFastInventory;
	float m_TimeStuck;
	CCharacterCore* GetCore() { return &m_Core; }
    void DropItem(int ItemID = -1);

	int m_ActiveBlockId;
	vec2 GetMousePosition() const { return vec2(m_Pos.x+m_LatestInput.m_TargetX, m_Pos.y+m_LatestInput.m_TargetY); }
	vec2 GetMouseDirection() const { return normalize(vec2(m_LatestInput.m_TargetX, m_LatestInput.m_TargetY)); }

	CCellData m_FastInventory[NUM_CELLS_LINE];

	bool IsInventoryFull();
	int InInventory(int ItemID);
	int GetFirstEmptyInventoryIndex();

    int GetCurrentAmmo(int wid);
    void FillAccountData(void *pAccountInfo);
    void UseAccountData(void *pAccountInfo);

private:
	// player controlling this character
	class CPlayer *m_pPlayer;

	bool m_Alive;

	// weapon info
	CEntity *m_apHitObjects[10];
	int m_NumObjectsHit;

	int m_ActiveInventoryItem;
	int m_LastInventoryItem;
	int m_QueuedInventoryItem;

	int m_ReloadTimer;
	int m_AttackTick;

	int m_DamageTaken;

	int m_EmoteType;
	int m_EmoteStop;

	// last tick that the player took any action ie some input
	int m_LastAction;
	int m_LastNoAmmoSound;

	// these are non-heldback inputs
	CNetObj_PlayerInput m_LatestPrevInput;
	CNetObj_PlayerInput m_LatestInput;

	// input
	CNetObj_PlayerInput m_PrevInput;
	CNetObj_PlayerInput m_Input;
	int m_NumInputs;
	int m_Jumped;

	int m_DamageTakenTick;

	int m_Health;
	int m_Armor;

	// ninja
	struct
	{
		vec2 m_ActivationDir;
		int m_ActivationTick;
		int m_CurrentMoveTime;
		int m_OldVelAmount;
	} m_Ninja;

	// the player core for the physics
	CCharacterCore m_Core;

	// info for dead reckoning
	int m_ReckoningTick; // tick that we are performing dead reckoning From
	CCharacterCore m_SendCore; // core that we should send
	CCharacterCore m_ReckoningCore; // the dead reckoning core

	// H-Client
	float m_TimerFluidDamage;
	bool m_InWater;
	void Construct();
	void UseInventoryItem(int Index);
	//
};

#endif
