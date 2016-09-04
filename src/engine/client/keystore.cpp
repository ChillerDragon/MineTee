/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include "keystore.h"
#include <cstdint>

void CKeyStore::Init(const char *pFileStore, IStorage *pStorage)
{
	m_pStorage = pStorage;
	str_copy(m_aFileStore, pFileStore, sizeof(m_aFileStore));
	Load();
}

const unsigned char* CKeyStore::Get(const char *pHost)
{
	std::list<CKey>::iterator it = Find(pHost);
	if (it != m_lKeys.end())
		return (*it).m_aKey;
	else
		return Create(pHost);
}

const unsigned char* CKeyStore::Create(const char *pHost)
{
	unsigned char aGeneratedKey[MINETEE_USER_KEY_SIZE];
	secure_random_fill(aGeneratedKey, sizeof(aGeneratedKey));
	CKey NewKey;
	str_copy(NewKey.m_aHost, pHost, sizeof(NewKey.m_aHost));
	mem_copy(NewKey.m_aKey, aGeneratedKey, sizeof(NewKey.m_aKey));
	m_lKeys.push_back(NewKey);
	Save();
	return (*m_lKeys.rbegin()).m_aKey;
}

bool CKeyStore::Remove(const char *pHost)
{
	std::list<CKey>::iterator it = Find(pHost);
	if (it == m_lKeys.end())
		return false;
	m_lKeys.erase(it);
	Save();
	return true;
}

std::list<CKeyStore::CKey>::iterator CKeyStore::Find(const char *pHost)
{
	std::list<CKey>::iterator it = m_lKeys.begin();
	while (it != m_lKeys.end())
	{
		CKey *pKey = &(*it);
		if (str_comp_nocase(pHost, pKey->m_aHost) == 0)
			return it;
		++it;
	}

	return m_lKeys.end();
}

void CKeyStore::Save()
{
	IOHANDLE File = m_pStorage->OpenFile(m_aFileStore, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if (!File)
		return;

	int32_t DataSize = m_lKeys.size();
	io_write(File, &DataSize, sizeof(int32_t));

	std::list<CKey>::iterator it = m_lKeys.begin();
	while (it != m_lKeys.end())
	{
		CKey *pKey = &(*it);
		io_write(File, pKey, sizeof(CKey));
		++it;
	}
	io_close(File);
}

void CKeyStore::Load()
{
	IOHANDLE File = m_pStorage->OpenFile(m_aFileStore, IOFLAG_READ, IStorage::TYPE_SAVE);
	if (!File)
		return;

	int32_t Count = 0;
	bool hasErrors = false;
	io_read(File, &Count, sizeof(int32_t));
	for (int i=0; i<Count; i++)
	{
		CKey NewKey;
		hasErrors = (io_read(File, &NewKey, sizeof(CKey)) == 0);
		if (hasErrors) break;
		m_lKeys.push_back(NewKey);
	}

	io_close(File);
}
