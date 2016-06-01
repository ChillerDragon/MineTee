/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
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
#include <game/server/entities/character.h>


class IAccountSystem : public IInterface
{
	MACRO_INTERFACE("accountsystem", 0)
public:
	struct ACCOUNT_INFO
	{
		// General
		unsigned char m_aKey[MINETEE_USER_KEY_SIZE];

		// Player Info
		unsigned m_Level;
		char m_aSkinName[64];

		// Character Info
		vec2 m_Pos;
		bool m_Alive;
		int m_Health;
		int m_ActiveWeapon;
		CCharacter::Inventory m_Inventory;
		CCharacter::BlockStat m_aBlocks[CBlockManager::MAX_BLOCKS];
		CCharacter::WeaponStat m_aWeapons[NUM_WEAPONS];

		// Per Info
		struct PepInfo
		{
			vec2 m_Pos;
			int m_Type;
			char m_aSkinName[64];
			char m_aName[MAX_NAME_LENGTH];
			int m_ActiveWeapon;
			bool m_Alive;
		} m_PetInfo;
	};

	virtual void Init(const char *pFileStore, IStorage *pStorage) = 0;

	virtual ACCOUNT_INFO* Get(const unsigned char *pKey) = 0;
	virtual bool Remove(const unsigned char *pKey) = 0;
	virtual int GetNum() const = 0;
};

#endif
