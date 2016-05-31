/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_KEYSTORE_H
#define ENGINE_KEYSTORE_H

#include "kernel.h"


class IKeyStore : public IInterface
{
	MACRO_INTERFACE("keystore", 0)
public:
	virtual void Init(const char *pFileStore, IStorage *pStorage) = 0;

	virtual const unsigned char* Get(const char *pHost) = 0;
	virtual bool Remove(const char *pHost) = 0;
};

#endif
