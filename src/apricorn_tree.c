#include "global.h"
#include "admg_feature_state.h"
#include "admg_registry.h"
#include "admg_retention.h"
#include "apricorn_tree.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "item.h"
#include "random.h"
#include "string_util.h"
#include "data/apricorns.h"

// Read tree definitions through ADMG_GetApricornTree and keep picked bits in ADMG_FEATURE_STATE_APRICORN_TREES
// This keeps apricorn state out of SaveBlock3
// Tree interactions use IDs 1-255 and check that the tree exists and its item and yield range are valid
// Picking reuses the displayed yield, and random rolls include the maximum
// A tree counts as picked only after the bag accepts the items

// Look up a built-in tree; APRICORN_TREE_NONE never names a tree
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetApricornTree(ADMG_Word id, struct ADMG_ApricornTree *out)
{
#if APRICORN_TREE_COUNT > 0
    if (out != NULL && id != APRICORN_TREE_NONE && id < APRICORN_TREE_COUNT)
    {
        out->item = gApricornTrees[id].apricornType;
        out->minimum = gApricornTrees[id].minimum;
        out->maximum = gApricornTrees[id].maximum;
        return TRUE;
    }
#endif
    return FALSE;
}
ADMG_RETAIN_FUNCTION(ADMG_GetApricornTree);

static bool32 ReadTree(u32 id, struct ADMG_ApricornTree *tree)
{
    if (id == APRICORN_TREE_NONE || id >= ADMG_APRICORN_TREE_ID_LIMIT || !ADMG_GetApricornTree(id, tree))
        return FALSE;
    return tree->minimum >= 1 && tree->maximum >= tree->minimum && IsItemIdValid(tree->item) && tree->item != ITEM_NONE;
}

static struct ADMG_ApricornTreesPicked *PickedState(void)
{
    if (ADMG_GetFeatureStateBridgeVersion() != 1)
        return NULL;
    return (struct ADMG_ApricornTreesPicked *)ADMG_GetFeatureState(ADMG_FEATURE_STATE_APRICORN_TREES);
}

// Keep the displayed yield for the matching tree's next pick
static EWRAM_DATA struct { u16 tree; u8 count; bool8 valid; } sApricornYield = {0};

void DailyResetApricornTrees(void)
{
    struct ADMG_ApricornTreesPicked *picked = PickedState();
    if (picked != NULL)
        memset(picked, 0, sizeof(*picked));
    sApricornYield.valid = FALSE;
}

void ObjectEventInteractionGetApricornTreeData(void)
{
    u32 id = GetObjectEventApricornTreeId(gSelectedObjectEvent);
    sApricornYield.valid = FALSE;
    gSpecialVar_0x8004 = GetApricornTypeByApricornTreeId(id);
    gSpecialVar_0x8005 = GetApricornCountByApricornTreeId(id);
    if (gSpecialVar_0x8005 != 0)
    {
        sApricornYield.tree = id;
        sApricornYield.count = gSpecialVar_0x8005;
        sApricornYield.valid = TRUE;
    }

    CopyItemNameHandlePlural(gSpecialVar_0x8004, gStringVar1, gSpecialVar_0x8005);
}

ADMG_NATIVE_ENTRY ADMG_Word ADMG_PickApricornTree(ADMG_Word id, ADMG_Word *countOut)
{
    struct ADMG_ApricornTree tree;
    u32 count;
    if (countOut != NULL)
        *countOut = 0;
    if (!ReadTree(id, &tree))
        return ADMG_APRICORN_INVALID_TREE;
    if (PickedState() == NULL)
        return ADMG_APRICORN_STATE_UNAVAILABLE;
    if (IsApricornTreePicked(id))
        return ADMG_APRICORN_ALREADY_PICKED;
    count = sApricornYield.valid && sApricornYield.tree == id ? sApricornYield.count : GetApricornCountByApricornTreeId(id);
    sApricornYield.valid = FALSE;
    if (countOut != NULL)
        *countOut = count;
    if (!CheckBagHasSpace(tree.item, count) || !AddBagItem(tree.item, count))
        return ADMG_APRICORN_BAG_FULL;
    SetApricornTreePicked(id);
    return ADMG_APRICORN_PICKED;
}
ADMG_RETAIN_FUNCTION(ADMG_PickApricornTree);

void ObjectEventInteractionPickApricornTree(void)
{
    u32 id = GetObjectEventApricornTreeId(gSelectedObjectEvent);
    enum ApricornType apricorn = GetApricornTypeByApricornTreeId(id);
    gSpecialVar_0x8006 = ADMG_PickApricornTree(id, NULL) == ADMG_APRICORN_PICKED;
    gSpecialVar_Result = GetItemPocket((enum Item)apricorn);
}

enum ApricornType GetApricornTypeByApricornTreeId(u32 id)
{
    struct ADMG_ApricornTree tree;
    return ReadTree(id, &tree) ? tree.item : 0;
}

u8 GetApricornCountByApricornTreeId(u32 id)
{
    struct ADMG_ApricornTree tree;
    if (!ReadTree(id, &tree) || IsApricornTreePicked(id))
        return 0;
    if (tree.maximum > tree.minimum)
        return tree.minimum + Random() % (tree.maximum - tree.minimum + 1);
    return tree.minimum;
}

bool8 IsApricornTreePicked(u32 id)
{
    struct ADMG_ApricornTree tree;
    struct ADMG_ApricornTreesPicked *picked = PickedState();
    if (!ReadTree(id, &tree) || picked == NULL)
        return TRUE;
    return (picked->bits[id / 8] >> (id % 8)) & 1;
}

void SetApricornTreePicked(u32 id)
{
    struct ADMG_ApricornTree tree;
    struct ADMG_ApricornTreesPicked *picked = PickedState();
    if (!ReadTree(id, &tree) || picked == NULL)
        return;
    picked->bits[id / 8] |= 1 << (id % 8);
}

// Set or clear a tree's picked bit
// Return FALSE if the tree is invalid or its state is unavailable
ADMG_NATIVE_ENTRY ADMG_Word ADMG_SetApricornTreePicked(ADMG_Word id, ADMG_Word picked)
{
    struct ADMG_ApricornTree tree;
    struct ADMG_ApricornTreesPicked *state = PickedState();
    if (!ReadTree(id, &tree) || state == NULL)
        return FALSE;
    if (picked)
        state->bits[id / 8] |= 1 << (id % 8);
    else
        state->bits[id / 8] &= ~(1 << (id % 8));
    return TRUE;
}
ADMG_RETAIN_FUNCTION(ADMG_SetApricornTreePicked);
ADMG_RETAIN_FUNCTION(DailyResetApricornTrees);
ADMG_RETAIN_FUNCTION(IsApricornTreePicked);
ADMG_RETAIN_FUNCTION(GetApricornCountByApricornTreeId);
ADMG_RETAIN_FUNCTION(GetApricornTypeByApricornTreeId);
