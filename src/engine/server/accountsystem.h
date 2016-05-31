/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SERVER_ACCOUNT_SYSTEM_H
#define ENGINE_SERVER_ACCOUNT_SYSTEM_H

#include "../accountsystem.h"
#include <engine/storage.h>
#include <list>


class CAccountSystem : public IAccountSystem
{
public:
	~CAccountSystem();
	void Init(const char *pFileStore, IStorage *pStorage);

	ACCOUNT_INFO* Get(const unsigned char *pKey);
	bool Remove(const unsigned char *pKey);

private:
	IStorage *m_pStorage;
	std::list<IAccountSystem::ACCOUNT_INFO> m_lAccounts;
	char m_aFileAccounts[255];

	std::list<ACCOUNT_INFO>::iterator Find(const unsigned char *pKey);
	IAccountSystem::ACCOUNT_INFO* Create(const unsigned char *pKey);
	void Save();
	void Load();
};

#endif
