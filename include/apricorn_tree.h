#ifndef GUARD_APRICORN_TREE_H
#define GUARD_APRICORN_TREE_H

#include "constants/apricorn_tree.h"

// A tree's item and yield range; the 32-bit item ID can name any valid nonzero item
struct ADMG_ApricornTree
{
    u32 item;
    u8 minimum;
    u8 maximum;
};

// Object events keep a tree's ID in a u8 (trainerRange_berryTreeId), so IDs are 1-255
#define ADMG_APRICORN_TREE_ID_LIMIT 256

enum ADMG_ApricornPickResult
{
    ADMG_APRICORN_PICKED,
    ADMG_APRICORN_INVALID_TREE,
    ADMG_APRICORN_ALREADY_PICKED,
    ADMG_APRICORN_BAG_FULL,
    ADMG_APRICORN_STATE_UNAVAILABLE,
};

bool8 IsApricornTreePicked(u32 id);
void SetApricornTreePicked(u32 id);

void DailyResetApricornTrees(void);
void ObjectEventInteractionGetApricornTreeData(void);
void ObjectEventInteractionPickApricornTree(void);
enum ApricornType GetApricornTypeByApricornTreeId(u32 id);
u8 GetApricornCountByApricornTreeId(u32 id);
u32 ADMG_GetApricornTree(u32 id, struct ADMG_ApricornTree *out);
u32 ADMG_PickApricornTree(u32 id, u32 *countOut);
u32 ADMG_SetApricornTreePicked(u32 id, u32 picked);

#endif //GUARD_APRICORN_TREE_H
