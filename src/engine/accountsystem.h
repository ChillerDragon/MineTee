/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_ACCOUNT_SYSTEM_H
#define ENGINE_ACCOUNT_SYSTEM_H

#include "kernel.h"
#include "storage.h"
#include "shared/config.h"
#include <base/vmath.h>
#include <game/block_manager.h>
#include <engine/shared/protocol.h>
#include <game/generated/protocol.h>


class IAccountSystem : public IInterface
{
	MACRO_INTERFACE("accountsystem", 0)
public:
	struct ACCOUNT_INFO
	{
		vec2 m_Pos;
		unsigned m_Level;
		unsigned char m_aKey[MINETEE_USER_KEY_SIZE];

	    struct {
	        int m_Items[9];
	        int m_Selected;
	    } m_Inventory;

		struct BlockStat
		{
		    int m_Amount;
		    bool m_Got;
		} m_aBlocks[CBlockManager::MAX_BLOCKS];

		struct WeaponStat
		{
			int m_AmmoRegenStart;
			int m_Ammo;
			int m_Ammocost;
			bool m_Got;

		} m_aWeapons[NUM_WEAPONS];

		struct
		{
			vec2 m_Pos;
			int m_Type;
			int m_Skin;
			char m_aName[MAX_NAME_LENGTH];
			int m_Weapon;
			int m_Ammount;
			bool m_Used;
		} m_PetInfo;
	};

	virtual void Init(const char *pFileStore, IStorage *pStorage) = 0;

	virtual ACCOUNT_INFO* Get(const unsigned char *pKey) = 0;
	virtual bool Remove(const unsigned char *pKey) = 0;
};

#endif
