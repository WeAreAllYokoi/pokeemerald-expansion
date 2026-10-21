#ifndef GUARD_BERRY_H
#define GUARD_BERRY_H

#include "admg_native.h"

void SetEnigmaBerry(u8 *src);
bool32 IsEnigmaBerryValid(void);
const struct BerryInfo *GetBerryInfo(enum BerryId berry);
struct BerryTree *GetBerryTreeInfo(u8 id);
bool32 ObjectEventInteractionWaterBerryTree(void);
bool8 IsPlayerFacingEmptyBerryTreePatch(void);
bool8 TryToWaterBerryTree(void);
void ClearBerryTrees(void);
void BerryTreeTimeUpdate(s32 minutes);
void PlantBerryTree(u8 id, enum BerryId berry, u8 stage, bool8 allowGrowth);
void RemoveBerryTree(u8 id);
u8 GetBerryTypeByBerryTreeId(u8 id);
u8 GetStageByBerryTreeId(u8 id);
void GetBerryNameByBerryType(u8 berry, u8 *string);
void Bag_ChooseBerry(void);
void Bag_ChooseMulch(void);
void ObjectEventInteractionGetBerryTreeData(void);
void ObjectEventInteractionPlantBerryTree(void);
void ObjectEventInteractionPickBerryTree(void);
void ObjectEventInteractionRemoveBerryTree(void);
void ObjectEventInteractionApplyMulch(void);
bool8 PlayerHasBerries(void);
void SetBerryTreesSeen(void);
bool32 BerryTreeGrow(struct BerryTree *tree);

extern const struct SpriteFrameImage gPicTable_CheriBerryTree[];
extern const struct SpriteFrameImage gPicTable_ChestoBerryTree[];
extern const struct SpriteFrameImage gPicTable_PechaBerryTree[];
extern const struct SpriteFrameImage gPicTable_RawstBerryTree[];
extern const struct SpriteFrameImage gPicTable_AspearBerryTree[];
extern const struct SpriteFrameImage gPicTable_LeppaBerryTree[];
extern const struct SpriteFrameImage gPicTable_OranBerryTree[];
extern const struct SpriteFrameImage gPicTable_PersimBerryTree[];
extern const struct SpriteFrameImage gPicTable_LumBerryTree[];
extern const struct SpriteFrameImage gPicTable_SitrusBerryTree[];
extern const struct SpriteFrameImage gPicTable_FigyBerryTree[];
extern const struct SpriteFrameImage gPicTable_WikiBerryTree[];
extern const struct SpriteFrameImage gPicTable_MagoBerryTree[];
extern const struct SpriteFrameImage gPicTable_AguavBerryTree[];
extern const struct SpriteFrameImage gPicTable_IapapaBerryTree[];
extern const struct SpriteFrameImage gPicTable_RazzBerryTree[];
extern const struct SpriteFrameImage gPicTable_WepearBerryTree[];
extern const struct SpriteFrameImage gPicTable_PomegBerryTree[];
extern const struct SpriteFrameImage gPicTable_KelpsyBerryTree[];
extern const struct SpriteFrameImage gPicTable_HondewBerryTree[];
extern const struct SpriteFrameImage gPicTable_GrepaBerryTree[];
extern const struct SpriteFrameImage gPicTable_TamatoBerryTree[];
extern const struct SpriteFrameImage gPicTable_CornnBerryTree[];
extern const struct SpriteFrameImage gPicTable_RabutaBerryTree[];
extern const struct SpriteFrameImage gPicTable_NomelBerryTree[];
extern const struct SpriteFrameImage gPicTable_SpelonBerryTree[];
extern const struct SpriteFrameImage gPicTable_PamtreBerryTree[];
extern const struct SpriteFrameImage gPicTable_DurinBerryTree[];
extern const struct SpriteFrameImage gPicTable_OccaBerryTree[];
extern const struct SpriteFrameImage gPicTable_YacheBerryTree[];
extern const struct SpriteFrameImage gPicTable_ChopleBerryTree[];
extern const struct SpriteFrameImage gPicTable_KebiaBerryTree[];
extern const struct SpriteFrameImage gPicTable_ShucaBerryTree[];
extern const struct SpriteFrameImage gPicTable_PayapaBerryTree[];
extern const struct SpriteFrameImage gPicTable_TangaBerryTree[];
extern const struct SpriteFrameImage gPicTable_KasibBerryTree[];
extern const struct SpriteFrameImage gPicTable_HabanBerryTree[];
extern const struct SpriteFrameImage gPicTable_ColburBerryTree[];
extern const struct SpriteFrameImage gPicTable_RoseliBerryTree[];
extern const struct SpriteFrameImage gPicTable_LiechiBerryTree[];
extern const struct SpriteFrameImage gPicTable_LansatBerryTree[];
extern const struct SpriteFrameImage gPicTable_MicleBerryTree[];
extern const struct SpriteFrameImage gPicTable_CustapBerryTree[];
extern const struct SpriteFrameImage gPicTable_JabocaBerryTree[];
extern const struct SpriteFrameImage gPicTable_RowapBerryTree[];

extern const u8 gBerryTreePaletteSlotTable_Cheri[];
extern const u8 gBerryTreePaletteSlotTable_Chesto[];
extern const u8 gBerryTreePaletteSlotTable_Pecha[];
extern const u8 gBerryTreePaletteSlotTable_Rawst[];
extern const u8 gBerryTreePaletteSlotTable_Aspear[];
extern const u8 gBerryTreePaletteSlotTable_Leppa[];
extern const u8 gBerryTreePaletteSlotTable_Oran[];
extern const u8 gBerryTreePaletteSlotTable_Persim[];
extern const u8 gBerryTreePaletteSlotTable_Lum[];
extern const u8 gBerryTreePaletteSlotTable_Sitrus[];
extern const u8 gBerryTreePaletteSlotTable_Figy[];
extern const u8 gBerryTreePaletteSlotTable_Wiki[];
extern const u8 gBerryTreePaletteSlotTable_Mago[];
extern const u8 gBerryTreePaletteSlotTable_Aguav[];
extern const u8 gBerryTreePaletteSlotTable_Iapapa[];
extern const u8 gBerryTreePaletteSlotTable_Razz[];
extern const u8 gBerryTreePaletteSlotTable_Wepear[];
extern const u8 gBerryTreePaletteSlotTable_Pomeg[];
extern const u8 gBerryTreePaletteSlotTable_Kelpsy[];
extern const u8 gBerryTreePaletteSlotTable_Hondew[];
extern const u8 gBerryTreePaletteSlotTable_Grepa[];
extern const u8 gBerryTreePaletteSlotTable_Tamato[];
extern const u8 gBerryTreePaletteSlotTable_Cornn[];
extern const u8 gBerryTreePaletteSlotTable_Rabuta[];
extern const u8 gBerryTreePaletteSlotTable_Nomel[];
extern const u8 gBerryTreePaletteSlotTable_Spelon[];
extern const u8 gBerryTreePaletteSlotTable_Pamtre[];
extern const u8 gBerryTreePaletteSlotTable_Durin[];
extern const u8 gBerryTreePaletteSlotTable_Occa[];
extern const u8 gBerryTreePaletteSlotTable_Yache[];
extern const u8 gBerryTreePaletteSlotTable_Chople[];
extern const u8 gBerryTreePaletteSlotTable_Kebia[];
extern const u8 gBerryTreePaletteSlotTable_Shuca[];
extern const u8 gBerryTreePaletteSlotTable_Payapa[];
extern const u8 gBerryTreePaletteSlotTable_Tanga[];
extern const u8 gBerryTreePaletteSlotTable_Kasib[];
extern const u8 gBerryTreePaletteSlotTable_Haban[];
extern const u8 gBerryTreePaletteSlotTable_Colbur[];
extern const u8 gBerryTreePaletteSlotTable_Roseli[];
extern const u8 gBerryTreePaletteSlotTable_Liechi[];
extern const u8 gBerryTreePaletteSlotTable_Lansat[];
extern const u8 gBerryTreePaletteSlotTable_Micle[];
extern const u8 gBerryTreePaletteSlotTable_Custap[];
extern const u8 gBerryTreePaletteSlotTable_Jaboca[];
extern const u8 gBerryTreePaletteSlotTable_Rowap[];

extern const u32 gBerryPic_Cheri[];
extern const u32 gBerryPic_Chesto[];
extern const u32 gBerryPic_Pecha[];
extern const u32 gBerryPic_Rawst[];
extern const u32 gBerryPic_Aspear[];
extern const u32 gBerryPic_Leppa[];
extern const u32 gBerryPic_Oran[];
extern const u32 gBerryPic_Persim[];
extern const u32 gBerryPic_Lum[];
extern const u32 gBerryPic_Sitrus[];
extern const u32 gBerryPic_Figy[];
extern const u32 gBerryPic_Wiki[];
extern const u32 gBerryPic_Mago[];
extern const u32 gBerryPic_Aguav[];
extern const u32 gBerryPic_Iapapa[];
extern const u32 gBerryPic_Razz[];
extern const u32 gBerryPic_Bluk[];
extern const u32 gBerryPic_Nanab[];
extern const u32 gBerryPic_Wepear[];
extern const u32 gBerryPic_Pinap[];
extern const u32 gBerryPic_Pomeg[];
extern const u32 gBerryPic_Kelpsy[];
extern const u32 gBerryPic_Qualot[];
extern const u32 gBerryPic_Hondew[];
extern const u32 gBerryPic_Grepa[];
extern const u32 gBerryPic_Tamato[];
extern const u32 gBerryPic_Cornn[];
extern const u32 gBerryPic_Magost[];
extern const u32 gBerryPic_Rabuta[];
extern const u32 gBerryPic_Nomel[];
extern const u32 gBerryPic_Spelon[];
extern const u32 gBerryPic_Pamtre[];
extern const u32 gBerryPic_Watmel[];
extern const u32 gBerryPic_Durin[];
extern const u32 gBerryPic_Belue[];
extern const u32 gBerryPic_Chilan[];
extern const u32 gBerryPic_Occa[];
extern const u32 gBerryPic_Passho[];
extern const u32 gBerryPic_Wacan[];
extern const u32 gBerryPic_Rindo[];
extern const u32 gBerryPic_Yache[];
extern const u32 gBerryPic_Chople[];
extern const u32 gBerryPic_Kebia[];
extern const u32 gBerryPic_Shuca[];
extern const u32 gBerryPic_Coba[];
extern const u32 gBerryPic_Payapa[];
extern const u32 gBerryPic_Tanga[];
extern const u32 gBerryPic_Charti[];
extern const u32 gBerryPic_Kasib[];
extern const u32 gBerryPic_Haban[];
extern const u32 gBerryPic_Colbur[];
extern const u32 gBerryPic_Babiri[];
extern const u32 gBerryPic_Roseli[];
extern const u32 gBerryPic_Liechi[];
extern const u32 gBerryPic_Ganlon[];
extern const u32 gBerryPic_Salac[];
extern const u32 gBerryPic_Petaya[];
extern const u32 gBerryPic_Apicot[];
extern const u32 gBerryPic_Lansat[];
extern const u32 gBerryPic_Starf[];
extern const u32 gBerryPic_Enigma[];
extern const u32 gBerryPic_Micle[];
extern const u32 gBerryPic_Custap[];
extern const u32 gBerryPic_Jaboca[];
extern const u32 gBerryPic_Rowap[];
extern const u32 gBerryPic_Kee[];
extern const u32 gBerryPic_Maranga[];

extern const u16 gBerryPalette_Cheri[];
extern const u16 gBerryPalette_Chesto[];
extern const u16 gBerryPalette_Pecha[];
extern const u16 gBerryPalette_Rawst[];
extern const u16 gBerryPalette_Aspear[];
extern const u16 gBerryPalette_Leppa[];
extern const u16 gBerryPalette_Oran[];
extern const u16 gBerryPalette_Persim[];
extern const u16 gBerryPalette_Lum[];
extern const u16 gBerryPalette_Sitrus[];
extern const u16 gBerryPalette_Figy[];
extern const u16 gBerryPalette_Wiki[];
extern const u16 gBerryPalette_Mago[];
extern const u16 gBerryPalette_Aguav[];
extern const u16 gBerryPalette_Iapapa[];
extern const u16 gBerryPalette_Razz[];
extern const u16 gBerryPalette_Bluk[];
extern const u16 gBerryPalette_Nanab[];
extern const u16 gBerryPalette_Wepear[];
extern const u16 gBerryPalette_Pinap[];
extern const u16 gBerryPalette_Pomeg[];
extern const u16 gBerryPalette_Kelpsy[];
extern const u16 gBerryPalette_Qualot[];
extern const u16 gBerryPalette_Hondew[];
extern const u16 gBerryPalette_Grepa[];
extern const u16 gBerryPalette_Tamato[];
extern const u16 gBerryPalette_Cornn[];
extern const u16 gBerryPalette_Magost[];
extern const u16 gBerryPalette_Rabuta[];
extern const u16 gBerryPalette_Nomel[];
extern const u16 gBerryPalette_Spelon[];
extern const u16 gBerryPalette_Pamtre[];
extern const u16 gBerryPalette_Watmel[];
extern const u16 gBerryPalette_Durin[];
extern const u16 gBerryPalette_Belue[];
extern const u16 gBerryPalette_Chilan[];
extern const u16 gBerryPalette_Occa[];
extern const u16 gBerryPalette_Passho[];
extern const u16 gBerryPalette_Wacan[];
extern const u16 gBerryPalette_Rindo[];
extern const u16 gBerryPalette_Yache[];
extern const u16 gBerryPalette_Chople[];
extern const u16 gBerryPalette_Kebia[];
extern const u16 gBerryPalette_Shuca[];
extern const u16 gBerryPalette_Coba[];
extern const u16 gBerryPalette_Payapa[];
extern const u16 gBerryPalette_Tanga[];
extern const u16 gBerryPalette_Charti[];
extern const u16 gBerryPalette_Kasib[];
extern const u16 gBerryPalette_Haban[];
extern const u16 gBerryPalette_Colbur[];
extern const u16 gBerryPalette_Babiri[];
extern const u16 gBerryPalette_Roseli[];
extern const u16 gBerryPalette_Liechi[];
extern const u16 gBerryPalette_Ganlon[];
extern const u16 gBerryPalette_Salac[];
extern const u16 gBerryPalette_Petaya[];
extern const u16 gBerryPalette_Apicot[];
extern const u16 gBerryPalette_Lansat[];
extern const u16 gBerryPalette_Starf[];
extern const u16 gBerryPalette_Enigma[];
extern const u16 gBerryPalette_Micle[];
extern const u16 gBerryPalette_Custap[];
extern const u16 gBerryPalette_Jaboca[];
extern const u16 gBerryPalette_Rowap[];
extern const u16 gBerryPalette_Kee[];
extern const u16 gBerryPalette_Maranga[];

extern const struct Berry gBerries[];

// Berry queries and actions shared by tree scripts, bag items and the platform API
enum ADMG_BerryResult
{
    ADMG_BERRY_OK = 0,
    ADMG_BERRY_INVALID_TREE = 1,
    ADMG_BERRY_PLOT_OCCUPIED = 2,
    ADMG_BERRY_NOT_A_BERRY = 3,
    ADMG_BERRY_NOT_IN_BAG = 4,
    ADMG_BERRY_NOT_WATERABLE = 5,
    ADMG_BERRY_NO_WAILMER_PAIL = 6,
    ADMG_BERRY_SOIL_NOT_DRY = 7,
    ADMG_BERRY_MULCH_DISABLED = 8,
    ADMG_BERRY_NOT_MULCH = 9,
    ADMG_BERRY_WEEDS_DISABLED = 10,
    ADMG_BERRY_NO_WEEDS = 11,
    ADMG_BERRY_NOT_RIPE = 12,
    ADMG_BERRY_BAG_FULL = 13,
    ADMG_BERRY_MUTATION_BAG_FULL = 14,
    ADMG_BERRY_HARVESTED_WITH_MUTATION = 15,
};

enum ADMG_BerryPestStart
{
    ADMG_BERRY_PEST_STARTED = 0,
    ADMG_BERRY_PEST_INVALID_TREE = 1,
    ADMG_BERRY_PEST_DISABLED = 2,
    ADMG_BERRY_PEST_BUSY = 3,
    ADMG_BERRY_PEST_NOT_FACING = 4,
    ADMG_BERRY_PEST_NO_PESTS = 5,
    ADMG_BERRY_PEST_NO_SPECIES = 6,
};

enum ADMG_BerryPestState
{
    ADMG_BERRY_PEST_IDLE = 0,
    ADMG_BERRY_PEST_RUNNING = 1,
    ADMG_BERRY_PEST_FINISHED = 2,
    ADMG_BERRY_PEST_INTERRUPTED = 3,
};

struct ScriptContext;
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetBerryGrowthHours(ADMG_Word berry);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetBerryMaxYield(ADMG_Word berry);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetBerryMinYield(ADMG_Word berry);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetBerryColor(ADMG_Word berry);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_CanWaterBerryTree(ADMG_Word id);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_WaterBerryTree(ADMG_Word id);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_PlantBerryFromBag(ADMG_Word id, ADMG_Word itemId);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_ApplyBerryMulchFromBag(ADMG_Word id, ADMG_Word itemId);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_HarvestBerryTree(ADMG_Word id);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_PullBerryTreeWeeds(ADMG_Word id);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetBerryTreeMutation(ADMG_Word id);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetBerryPestSpecies(ADMG_Word berry);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_StartBerryPestInteraction(ADMG_Word id);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_EndInterruptedBerryPestInteraction(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetBerryPestInteractionState(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetBerryPestInteractionTree(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetBerryPestInteractionSpecies(void);
ADMG_NATIVE_ENTRY ADMG_Word ADMG_GetBerryPestInteractionOutcome(void);
ADMG_NATIVE_ENTRY void ADMG_ClearBerryPestInteraction(void);
void ADMG_ScriptFinishBerryPestInteraction(struct ScriptContext *ctx);

#endif // GUARD_BERRY_H
