/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "accountsystem.h"
#include <game/server/entities/character.h>

CAccountSystem::~CAccountSystem()
{
	Save();
}
void CAccountSystem::Init(const char *pFileStore, IStorage *pStorage)
{
	m_pStorage = pStorage;
	str_copy(m_aFileAccounts, pFileStore, sizeof(m_aFileAccounts));
	Load();
}

IAccountSystem::ACCOUNT_INFO* CAccountSystem::Get(const unsigned char *pKey)
{
	std::list<ACCOUNT_INFO>::iterator it = Find(pKey);
	if (it != m_lAccounts.end())
		return &(*it);
	else
		return Create(pKey);
}

bool CAccountSystem::Remove(const unsigned char *pKey)
{
	std::list<ACCOUNT_INFO>::iterator it = Find(pKey);
	if (it == m_lAccounts.end())
		return false;
	m_lAccounts.erase(it);
	Save();
	return true;
}

IAccountSystem::ACCOUNT_INFO* CAccountSystem::Create(const unsigned char *pKey)
{
	ACCOUNT_INFO NewAccount;
	mem_copy(NewAccount.m_aKey, pKey, sizeof(NewAccount.m_aKey));
	NewAccount.m_Alive = false;
	NewAccount.m_Pos = vec2(0.0f, 0.0f);
	NewAccount.m_Level = 0;
	mem_zero(NewAccount.m_aBlocks, sizeof(CCharacter::BlockStat)*255);
	mem_zero(NewAccount.m_aWeapons, sizeof(CCharacter::WeaponStat)*NUM_WEAPONS);
	m_lAccounts.push_back(NewAccount);
	Save();
	return &(*m_lAccounts.rbegin());
}

std::list<IAccountSystem::ACCOUNT_INFO>::iterator CAccountSystem::Find(const unsigned char *pKey)
{
	std::list<ACCOUNT_INFO>::iterator it = m_lAccounts.begin();
	while (it != m_lAccounts.end())
	{
		ACCOUNT_INFO *pInfo = &(*it);
		if (mem_comp(pInfo->m_aKey, pKey, sizeof(pInfo->m_aKey)) == 0)
		{
			return it;
		}
		++it;
	}

	return m_lAccounts.end();
}

void CAccountSystem::Save()
{
	IOHANDLE File = m_pStorage->OpenFile(m_aFileAccounts, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if (!File)
		return;

	int32_t DataSize = m_lAccounts.size();
	io_write(File, &DataSize, sizeof(int32_t));

	std::list<ACCOUNT_INFO>::iterator it = m_lAccounts.begin();
	while (it != m_lAccounts.end())
	{
		ACCOUNT_INFO *pAccount = &(*it);
		io_write(File, pAccount, sizeof(ACCOUNT_INFO));
		++it;
	}
	io_close(File);
}

void CAccountSystem::Load()
{
	IOHANDLE File = m_pStorage->OpenFile(m_aFileAccounts, IOFLAG_READ, IStorage::TYPE_SAVE);
	if (!File)
		return;

	int32_t Count = 0;
	bool hasErrors = false;
	io_read(File, &Count, sizeof(int32_t));
	for (int i=0; i<Count; i++)
	{
		ACCOUNT_INFO NewAccount;
		hasErrors = (io_read(File, &NewAccount, sizeof(ACCOUNT_INFO)) == 0);
		if (hasErrors) break;
		m_lAccounts.push_back(NewAccount);
	}

	io_close(File);
}
