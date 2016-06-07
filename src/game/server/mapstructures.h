#ifndef H_GAME_SERVER_MAP_STRUCTURES
#define H_GAME_SERVER_MAP_STRUCTURES
#include <game/block_manager.h>

#define NUM_STRUCTURE_TILES 14

enum
{
	STRUCTURE_SPAWN_BOSS_DUNE=0,
	MAX_STRUCTURES
};

static int gs_MapStructures[MAX_STRUCTURES][NUM_STRUCTURE_TILES*NUM_STRUCTURE_TILES] =
{
	{
		CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,
		CBlockManager::GREEN_ORE,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::GREEN_ORE,
		CBlockManager::GREEN_ORE,CBlockManager::BRICK,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::BRICK,CBlockManager::GREEN_ORE,
		CBlockManager::GREEN_ORE,CBlockManager::BRICK,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::IRON_FENCE,CBlockManager::BRICK,CBlockManager::GREEN_ORE,
		CBlockManager::GREEN_ORE,CBlockManager::BRICK,CBlockManager::IRON_FENCE,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::IRON_FENCE,CBlockManager::BRICK,CBlockManager::GREEN_ORE,
		CBlockManager::GREEN_ORE,CBlockManager::BRICK,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::BRICK,CBlockManager::GREEN_ORE,
		CBlockManager::GREEN_ORE,CBlockManager::BRICK,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::BRICK,CBlockManager::GREEN_ORE,
		CBlockManager::GREEN_ORE,CBlockManager::BRICK,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::BRICK,CBlockManager::GREEN_ORE,
		CBlockManager::GREEN_ORE,CBlockManager::BRICK,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::BRICK,CBlockManager::GREEN_ORE,
		CBlockManager::GREEN_ORE,CBlockManager::BRICK,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::BRICK,CBlockManager::GREEN_ORE,
		CBlockManager::GREEN_ORE,CBlockManager::BRICK,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::BRICK,CBlockManager::GREEN_ORE,
		CBlockManager::GREEN_ORE,CBlockManager::BRICK,CBlockManager::LARGE_BED_LEFT,CBlockManager::LARGE_BED_RIGHT,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::AIR,CBlockManager::ENCHANTING_TABLE,CBlockManager::TORCH,CBlockManager::BRICK,CBlockManager::GREEN_ORE,
		CBlockManager::GREEN_ORE,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::BRICK,CBlockManager::GREEN_ORE,
		CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,CBlockManager::GREEN_ORE,
	}
};

#endif