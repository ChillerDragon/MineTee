#include <base/system.h>
#include <game/block_manager.h>
#include <cstdio>

CBlockManager::CBlockManager()
{
	m_pJsonData = 0x0;
	mem_zero(&m_JsonSettings, sizeof(m_JsonSettings));
}
CBlockManager::~CBlockManager()
{
	json_value_free_ex(&m_JsonSettings, m_pJsonData);
}

bool CBlockManager::Init()
{
    IOHANDLE fileAutoUpdate = io_open("data/blocks.json", IOFLAG_READ);
    if (fileAutoUpdate)
    {
		io_seek(fileAutoUpdate, 0, IOSEEK_END);
		long size = io_tell(fileAutoUpdate);
		io_seek(fileAutoUpdate, 0, IOSEEK_START);

		char aFileBuff[size];
		if (io_read(fileAutoUpdate, aFileBuff, size))
		{
			char aError[256];
			m_pJsonData = json_parse_ex(&m_JsonSettings, aFileBuff, size, aError);
			if (!m_pJsonData)
			{
				dbg_msg("BlockManager", "Error: %s", aError);
				return false;
			}
		}

		io_close(fileAutoUpdate);
		return true;
    }

    return false;
}

bool CBlockManager::GetBlockInfo(unsigned char BlockID, CBlockInfo *pBlockInfo)
{
	if (!pBlockInfo)
		return false;

	pBlockInfo->Reset();

	if (BlockID < 0 || BlockID >= m_pJsonData->u.array.length)
		return false;

	json_value *pJsonObject = m_pJsonData->u.array.values[BlockID];
	if (!pJsonObject)
		return false;

	str_copy(pBlockInfo->m_aName, (*pJsonObject)["name"], sizeof(pBlockInfo->m_aName));
	pBlockInfo->m_Health = ((*pJsonObject)["health"].type == json_none)?1:(*pJsonObject)["health"].u.integer;
	pBlockInfo->m_Gravity = ((*pJsonObject)["gravity"].type == json_none)?false:(*pJsonObject)["gravity"].u.boolean;
	pBlockInfo->m_PlayerCollide = ((*pJsonObject)["playerCollide"].type == json_none)?true:(*pJsonObject)["playerCollide"].u.boolean;
	pBlockInfo->m_HalfTile = ((*pJsonObject)["halfTile"].type == json_none)?false:(*pJsonObject)["halfTile"].u.boolean;
	pBlockInfo->m_Damage = ((*pJsonObject)["damage"].type == json_none)?false:(*pJsonObject)["damage"].u.boolean;
	pBlockInfo->m_OnPut = ((*pJsonObject)["onPut"].type == json_none)?BlockID:(*pJsonObject)["onPut"].u.integer;


	int items = (*pJsonObject)["craft"].u.object.length;
	for (int i=0; i<items; i++)
	{
		int CraftBlockID = -1;
		int CraftBlockAmount = (*pJsonObject)["craft"].u.object.values[i].value->u.integer;
		sscanf((const char *)(*pJsonObject)["craft"].u.object.values[i].name, "%d", &CraftBlockID);

		pBlockInfo->m_vCraft.insert(std::pair<int, char>(CraftBlockID, CraftBlockAmount));
	}

	items = (*pJsonObject)["onCook"].u.object.length;
	for (int i=0; i<items; i++)
	{
		int CookBlockID = -1;
		int CookBlockAmount = (*pJsonObject)["onCook"].u.object.values[i].value->u.integer;
		sscanf((const char *)(*pJsonObject)["onCook"].u.object.values[i].name, "%d", &CookBlockID);

		pBlockInfo->m_vOnCook.insert(std::pair<int, char>(CookBlockID, CookBlockAmount));
	}

	items = (*pJsonObject)["onBreak"].u.object.length;
	for (int i=0; i<items; i++)
	{
		int BreakBlockID = -1;
		int BreakBlockAmount = (*pJsonObject)["onBreak"].u.object.values[i].value->u.integer;
		sscanf((const char *)(*pJsonObject)["onBreak"].u.object.values[i].name, "%d", &BreakBlockID);

		pBlockInfo->m_vOnBreak.insert(std::pair<int, char>(BreakBlockID, BreakBlockAmount));
	}

	return true;
}
