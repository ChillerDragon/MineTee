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
	json_value_free(m_pJsonData);
	//json_value_free_ex(&m_JsonSettings, m_pJsonData);
}

bool CBlockManager::Init(char *pData, int DataSize)
{
	char aError[256];
	m_pJsonData = json_parse_ex(&m_JsonSettings, pData, DataSize, aError);
	if (!m_pJsonData)
	{
		dbg_msg("BlockManager", "Error: %s", aError);
		return false;
	}

    return true;
}

bool CBlockManager::GetBlockInfo(unsigned char BlockID, CBlockInfo *pBlockInfo)
{
	if (!pBlockInfo)
		return false;

	pBlockInfo->Reset();

	if (!m_pJsonData || BlockID < 0 || BlockID >= m_pJsonData->u.array.length)
		return false;

	json_value *pJsonObject = m_pJsonData->u.array.values[BlockID];
	if (!pJsonObject)
		return false;

	str_copy(pBlockInfo->m_aName, (*pJsonObject)["name"], sizeof(pBlockInfo->m_aName));
	pBlockInfo->m_Health = ((*pJsonObject)["health"].type == json_none)?1:(*pJsonObject)["health"].u.integer;
	pBlockInfo->m_LightSize = ((*pJsonObject)["lightSize"].type == json_none)?0:(*pJsonObject)["lightSize"].u.integer;
	pBlockInfo->m_CraftNum = ((*pJsonObject)["craftNum"].type == json_none)?1:(*pJsonObject)["craftNum"].u.integer;
	pBlockInfo->m_Gravity = ((*pJsonObject)["gravity"].type == json_none)?false:(*pJsonObject)["gravity"].u.boolean;
	pBlockInfo->m_PlayerCollide = ((*pJsonObject)["playerCollide"].type == json_none)?true:(*pJsonObject)["playerCollide"].u.boolean;
	pBlockInfo->m_HalfTile = ((*pJsonObject)["halfTile"].type == json_none)?false:(*pJsonObject)["halfTile"].u.boolean;
	pBlockInfo->m_Damage = ((*pJsonObject)["damage"].type == json_none)?false:(*pJsonObject)["damage"].u.boolean;
	pBlockInfo->m_OnPut = ((*pJsonObject)["onPut"].type == json_none)?BlockID:(*pJsonObject)["onPut"].u.integer;

	int items = (*pJsonObject)["lightColor"].u.array.length;
	if (items == 3)
	{
		pBlockInfo->m_LightColor.r = (float)((*pJsonObject)["lightColor"].u.array.values[0]->u.dbl)/255.0f;
		pBlockInfo->m_LightColor.g = (float)((*pJsonObject)["lightColor"].u.array.values[1]->u.dbl)/255.0f;
		pBlockInfo->m_LightColor.b = (float)((*pJsonObject)["lightColor"].u.array.values[2]->u.dbl)/255.0f;
	}

	items = (*pJsonObject)["craft"].u.object.length;
	for (int i=0; i<items; i++)
	{
		int CraftBlockID = -1;
		int CraftBlockAmount = (*pJsonObject)["craft"].u.object.values[i].value->u.integer;
		sscanf((const char *)(*pJsonObject)["craft"].u.object.values[i].name, "%d", &CraftBlockID);

		pBlockInfo->m_vCraft.insert(std::pair<int, unsigned char>(CraftBlockID, CraftBlockAmount));
	}

	items = (*pJsonObject)["onCook"].u.object.length;
	for (int i=0; i<items; i++)
	{
		int CookBlockID = -1;
		int CookBlockAmount = (*pJsonObject)["onCook"].u.object.values[i].value->u.integer;
		sscanf((const char *)(*pJsonObject)["onCook"].u.object.values[i].name, "%d", &CookBlockID);

		pBlockInfo->m_vOnCook.insert(std::pair<int, unsigned char>(CookBlockID, CookBlockAmount));
	}

	items = (*pJsonObject)["onBreak"].u.object.length;
	for (int i=0; i<items; i++)
	{
		int BreakBlockID = -1;
		int BreakBlockAmount = (*pJsonObject)["onBreak"].u.object.values[i].value->u.integer;
		sscanf((const char *)(*pJsonObject)["onBreak"].u.object.values[i].name, "%d", &BreakBlockID);

		pBlockInfo->m_vOnBreak.insert(std::pair<int, unsigned char>(BreakBlockID, BreakBlockAmount));
	}

	if (pBlockInfo->m_LightSize < 0)
		pBlockInfo->m_LightSize = 0;
	if (pBlockInfo->m_CraftNum < 0)
		pBlockInfo->m_CraftNum = 0;

	return true;
}

bool CBlockManager::IsFluid(int BlockID, int *pType)
{
	int ftype = 0;
	if (BlockID >= CBlockManager::WATER_A && BlockID <= CBlockManager::WATER_D)
		ftype = FLUID_WATER;
	else if (BlockID >= CBlockManager::LAVA_A && BlockID <= CBlockManager::LAVA_D)
		ftype = FLUID_LAVA;

	if (pType)
		*pType = ftype;

	return (ftype != 0);
}
