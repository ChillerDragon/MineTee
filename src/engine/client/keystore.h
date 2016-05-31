/* (c) Alexandre Díaz. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_KEYSTORE_H
#define GAME_CLIENT_KEYSTORE_H
#include <engine/storage.h>
#include <engine/keystore.h>
#include <engine/shared/config.h>
#include <list>
#include <string>

class CKeyStore : public IKeyStore
{
public:
	struct CKey
	{
		char m_aHost[64];
		unsigned char m_aKey[MINETEE_USER_KEY_SIZE];
	};

	virtual void Init(const char *pFileStore, IStorage *pStorage);

	const unsigned char* Get(const char *pHost);
	bool Remove(const char *pHost);

private:
	IStorage *m_pStorage;
	std::list<CKey> m_lKeys;
	std::list<CKey>::iterator Find(const char *pHost);
	const unsigned char* Create(const char *pHost);
	void Save();
	void Load();
	char m_aFileStore[255];
};

#endif
