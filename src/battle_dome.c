#include "global.h"
#include "admg_state.h"
#include "admg_trainer_actor.h"
#include <limits.h>
#include "battle_dome.h"
#include "battle.h"
#include "battle_ai_util.h"
#include "battle_main.h"
#include "battle_setup.h"
#include "battle_tower.h"
#include "battle_stat_change.h"
#include "frontier_util.h"
#include "battle_message.h"
#include "event_data.h"
#include "overworld.h"
#include "util.h"
#include "malloc.h"
#include "string_util.h"
#include "random.h"
#include "task.h"
#include "main.h"
#include "gpu_regs.h"
#include "text.h"
#include "bg.h"
#include "window.h"
#include "strings.h"
#include "palette.h"
#include "decompress.h"
#include "party_menu.h"
#include "menu.h"
#include "sound.h"
#include "pokemon_icon.h"
#include "data.h"
#include "item.h"
#include "international_string_util.h"
#include "trainer_pokemon_sprites.h"
#include "scanline_effect.h"
#include "script_pokemon_util.h"
#include "graphics.h"
#include "constants/battle_dome.h"
#include "constants/battle_move_effects.h"
#include "constants/frontier_util.h"
#include "constants/moves.h"
#include "constants/trainers.h"
#include "constants/abilities.h"
#include "constants/songs.h"
#include "constants/battle_frontier.h"
#include "constants/rgb.h"

#define TAG_BUTTONS 0

// Enough space to hold 2 match info cards worth of trainers and their parties
#define NUM_INFOCARD_SPRITES ((FRONTIER_PARTY_SIZE + 1) * 4)
#define NUM_INFOCARD_TRAINERS 2

// An 'Info Card' is a trainer or match information page that can be viewed on the Tourney Tree
struct TourneyTreeInfoCard
{
    u8 spriteIds[NUM_INFOCARD_SPRITES];
    u8 pos;
    u8 tournamentIds[NUM_INFOCARD_TRAINERS];
};

struct TourneyTreeLineSection
{
    u8 x;
    u8 y;
    u16 tile;
};

#define DOME_TRAINERS gSaveBlock2Ptr->frontier.domeTrainers
#define DOME_MONS     gSaveBlock2Ptr->frontier.domeMonIds

#define tState              data[0]

// Task data for Task_ShowTourneyTree
#define tNotInteractive     data[1]
#define tIsPrevTourneyTree  data[4]

// Task data for Task_ShowTourneyInfoCard
#define tTournamentId       data[1]
#define tMode               data[2]
#define tPrevTaskId         data[3]

enum {
    EFFECTIVENESS_MODE_GOOD,
    EFFECTIVENESS_MODE_BAD,
    EFFECTIVENESS_MODE_AI_VS_AI,
};

// Window IDs for the tourney tree
enum {
    TOURNEYWIN_NAMES_LEFT,
    TOURNEYWIN_NAMES_RIGHT,
    TOURNEYWIN_TITLE,
};

// Window IDs for the trainer (WIN_TRAINER_*) and match (WIN_MATCH_*) info cards.
// All 9 have a duplicate window at WIN + NUM_INFO_CARD_WINDOWS used by the alternate info card
enum {
    WIN_TRAINER_NAME,
    WIN_TRAINER_MON1_NAME,
    WIN_TRAINER_MON2_NAME, // Used implicitly
    WIN_TRAINER_MON3_NAME, // Used implicitly
    WIN_TRAINER_FLAVOR_TEXT = WIN_TRAINER_MON1_NAME + FRONTIER_PARTY_SIZE, // Trainer's potential, battle style, and stat texts
    WIN_MATCH_NUMBER,
    WIN_MATCH_TRAINER_NAME_LEFT,
    WIN_MATCH_TRAINER_NAME_RIGHT,
    WIN_MATCH_WIN_TEXT,
    NUM_INFO_CARD_WINDOWS
};

static u8 GetDomeTrainerMonIvs(u16);
static void SwapDomeTrainers(int, int, u16 *);
static void CalcDomeMonStats(const struct TrainerMon *fmon, int level, u8 ivs, int *stats);
static void CreateDomeOpponentMons(u16);
static int SelectOpponentMons_Good(u16, bool8);
static int SelectOpponentMons_Bad(u16, bool8);
static int GetTypeEffectivenessPoints(enum Move, enum Species, int);
static int SelectOpponentMonsFromParty(int *, bool8);
static void Task_ShowTourneyInfoCard(u8);
static void Task_HandleInfoCardInput(u8);
static u8 Task_GetInfoCardInput(u8);
static void SetFacilityTrainerAndMonPtrs(void);
static int TrainerIdToTournamentId(u16);
static bool8 ADMG_TryDomeTrainerSlot(u32 identity, u32 *slot);
static bool8 ADMG_TryDomeOpponentSlot(u32 identity, u32 *slot);
static bool8 ADMG_TryDomePlayerOpponent(u16 *identity, u32 *slot);
static u16 TrainerIdOfPlayerOpponent(void);
static void Task_ShowTourneyTree(u8);
static void Task_HandleStaticTourneyTreeInput(u8);
static void CB2_TourneyTree(void);
static void VblankCb_TourneyInfoCard(void);
static void DisplayMatchInfoOnCard(u8, u8);
static void DisplayTrainerInfoOnCard(u8, u8);
static int BufferDomeWinString(u8, u8 *);
static bool8 ADMG_TryBufferDomeMatchPair(u32, u8 *, int *);
static void CopyDomeTrainerName(u8 *, u16);
static void HblankCb_TourneyTree(void);
static void VblankCb_TourneyTree(void);
static u8 UpdateTourneyTreeCursor(u8);
static void DecideRoundWinners(u8);
static u8 GetOpposingNPCTournamentIdByRound(u8, u8);
static void DrawTourneyAdvancementLine(u8, u8) __attribute__((used, noipa));
static void SpriteCB_HorizontalScrollArrow(struct Sprite *);
static void SpriteCB_VerticalScrollArrow(struct Sprite *);
static void InitDomeChallenge(void);
static void GetDomeData(void);
static void SetDomeData(void);
static void BufferDomeRoundText(void);
static void BufferDomeOpponentName(void);
static void InitDomeOpponentParty(void);
static void ShowDomeOpponentInfo(void);
static void ShowDomeTourneyTree(void);
static void ShowPreviousDomeTourneyTree(void);
static void SetDomeOpponentId(void);
static void SetDomeOpponentGraphicsId(void);
static void ShowNonInteractiveDomeTourneyTree(void);
static void ResolveDomeRoundWinners(void);
static enum Move GetWinningMove(int, int, u8);
static void SaveDomeChallenge(void);
static void IncrementDomeStreaks(void);
static void ResetSketchedMoves(void);
static void RestoreDomePlayerPartyHeldItems(void);
static void ReduceDomePlayerPartyToSelectedMons(void);
static void GetPlayerSeededBeforeOpponent(void);
static void BufferLastDomeWinnerName(void);
static void InitRandomTourneyTreeResults(void);
static void InitDomeTrainers(void);

static EWRAM_DATA struct TourneyTreeInfoCard *sInfoCard = {0};
static EWRAM_DATA u8 *sTilemapBuffer = NULL;

// This array is searched in-order to determine what battle style a tourney trainer uses.
// If the sum of the points for the party's moves meets/exceeds all the point totals of an element, then they use that battle style
static const u8 sBattleStyleThresholds[NUM_BATTLE_STYLES - 1][NUM_MOVE_POINT_TYPES] =
{
    [DOME_BATTLE_STYLE_RISKY]           = {[MOVE_POINTS_RISKY] = 1},
    [DOME_BATTLE_STYLE_STALL]           = {[MOVE_POINTS_HEAL] = 2, [MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_DEF] = 2},
    [DOME_BATTLE_STYLE_VARIED]          = {[MOVE_POINTS_COMBO] = 1, [MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_STAT_LOWER] = 1, [MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_STATUS] = 1, [MOVE_POINTS_DEF] = 1},
    [DOME_BATTLE_STYLE_COMBO_HIGH]      = {[MOVE_POINTS_COMBO] = 3},
    [DOME_BATTLE_STYLE_RARE_MOVES]      = {[MOVE_POINTS_RARE] = 2},
    [DOME_BATTLE_STYLE_RARE_MOVE]       = {[MOVE_POINTS_RARE] = 1},
    [DOME_BATTLE_STYLE_HP]              = {[MOVE_POINTS_HEAL] = 3},
    [DOME_BATTLE_STYLE_STORE_POWER]     = {[MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_HEAL] = 1},
    [DOME_BATTLE_STYLE_ENFEEBLE_LOW]    = {[MOVE_POINTS_STAT_LOWER] = 1, [MOVE_POINTS_STATUS] = 1},
    [DOME_BATTLE_STYLE_LUCK]            = {[MOVE_POINTS_LUCK] = 2},
    [DOME_BATTLE_STYLE_REGAL]           = {[MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_DEF] = 1, [MOVE_POINTS_POPULAR] = 1, [MOVE_POINTS_STRONG] = 1},
    [DOME_BATTLE_STYLE_LOW_PP]          = {[MOVE_POINTS_LOW_PP] = 3},
    [DOME_BATTLE_STYLE_STATUS_ATK]      = {[MOVE_POINTS_STAT_RAISE] = 1, [MOVE_POINTS_STATUS] = 1},
    [DOME_BATTLE_STYLE_ENDURE]          = {[MOVE_POINTS_HEAL] = 2, [MOVE_POINTS_DEF] = 2},
    [DOME_BATTLE_STYLE_STATUS]          = {[MOVE_POINTS_STATUS] = 2},
    [DOME_BATTLE_STYLE_STRAIGHTFORWARD] = {[MOVE_POINTS_ACCURATE] = 3, [MOVE_POINTS_STRONG] = 3},
    [DOME_BATTLE_STYLE_AGGRESSIVE]      = {[MOVE_POINTS_STRONG] = 4},
    [DOME_BATTLE_STYLE_DEF]             = {[MOVE_POINTS_DEF] = 3},
    [DOME_BATTLE_STYLE_ENFEEBLE_HIGH]   = {[MOVE_POINTS_STAT_LOWER] = 2, [MOVE_POINTS_STATUS] = 2}, // BUG: This battle style is unobtainable; DOME_BATTLE_STYLE_ENFEEBLE_LOW will always succeed before it
    [DOME_BATTLE_STYLE_POPULAR_POWER]   = {[MOVE_POINTS_POWERFUL] = 3, [MOVE_POINTS_POPULAR] = 3},
    [DOME_BATTLE_STYLE_COMBO_LOW]       = {[MOVE_POINTS_COMBO] = 2},
    [DOME_BATTLE_STYLE_ACCURATE]        = {[MOVE_POINTS_HEAL] = 1, [MOVE_POINTS_ACCURATE] = 3},
    [DOME_BATTLE_STYLE_POWERFUL]        = {[MOVE_POINTS_POWERFUL] = 4},
    [DOME_BATTLE_STYLE_ATK_OVER_DEF]    = {[MOVE_POINTS_DMG] = 7},
    [DOME_BATTLE_STYLE_DEF_OVER_ATK]    = {[MOVE_POINTS_DEF] = 4}, // BUG: This battle style is unobtainable; DOME_BATTLE_STYLE_DEF will always succeed before it
    [DOME_BATTLE_STYLE_POPULAR_STRONG]  = {[MOVE_POINTS_POPULAR] = 2, [MOVE_POINTS_STRONG] = 4},
    [DOME_BATTLE_STYLE_EFFECTS]         = {[MOVE_POINTS_EFFECT] = 4},
    [DOME_BATTLE_STYLE_BALANCED]        = {0}, // If no other thresholds are met, this battle style is used
    [DOME_BATTLE_STYLE_UNUSED1]         = {0}, // Here below is unreachable
    [DOME_BATTLE_STYLE_UNUSED2]         = {0},
    [DOME_BATTLE_STYLE_UNUSED3]         = {0},
  //[DOME_BATTLE_STYLE_UNUSED4]         = {0}, // Excluded here, presumably was meant to be a style just for Dome Ace Tucker
};

// 1st array is for cursor position (sprite id): cursor can be on a trainer info button, a match info button, or the exit/cancel button
// 2nd array is for round count. For some reason this array contains an inaccessible Round 5 which is identical to Round 4
// 3rd array is movement direction (see the MOVE_DIR_* constants in UpdateTourneyTreeCursor)
// The values are sprite IDs for the cursor position to move to, with 0xFF being an invalid move
static const u8 sTourneyTreeCursorMovementMap[DOME_TOURNAMENT_TRAINERS_COUNT + DOME_TOURNAMENT_MATCHES_COUNT + 1][DOME_ROUNDS_COUNT + 1][4]=
{
    [0]  = {{   7,    1,    8,   16}, {   7,    1,    8,   16}, {   7,    1,    8,   16}, {   7,    1,    8,   16}, {   7,    1,    8,   16}},
    [1]  = {{   0,    2,    9,   16}, {   0,    2,    9,   16}, {   0,    2,    9,   16}, {   0,    2,    9,   16}, {   0,    2,    9,   16}},
    [2]  = {{   1,    3,   10,   17}, {   1,    3,   10,   17}, {   1,    3,   10,   17}, {   1,    3,   10,   17}, {   1,    3,   10,   17}},
    [3]  = {{   2,    4,   11,   17}, {   2,    4,   11,   17}, {   2,    4,   11,   17}, {   2,    4,   11,   17}, {   2,    4,   11,   17}},
    [4]  = {{   3,    5,   12,   18}, {   3,    5,   12,   18}, {   3,    5,   12,   18}, {   3,    5,   12,   18}, {   3,    5,   12,   18}},
    [5]  = {{   4,    6,   13,   18}, {   4,    6,   13,   18}, {   4,    6,   13,   18}, {   4,    6,   13,   18}, {   4,    6,   13,   18}},
    [6]  = {{   5,    7,   14,   19}, {   5,    7,   14,   19}, {   5,    7,   14,   19}, {   5,    7,   14,   19}, {   5,    7,   14,   19}},
    [7]  = {{   6,    0,   15,   19}, {   6,    0,   15,   19}, {   6,    0,   15,   19}, {   6,    0,   15,   19}, {   6,    0,   15,   19}},
    [8]  = {{  31,    9,   20,   31}, {  31,    9,   20,   31}, {  31,    9,   20,   31}, {  31,    9,   20,   31}, {  31,    9,   20,   31}},
    [9]  = {{   8,   10,   20,    1}, {   8,   10,   20,    1}, {   8,   10,   20,    1}, {   8,   10,   20,    1}, {   8,   10,   20,    1}},
    [10] = {{   9,   11,   21,    2}, {   9,   11,   21,    2}, {   9,   11,   21,    2}, {   9,   11,   21,    2}, {   9,   11,   21,    2}},
    [11] = {{  10,   12,   21,    3}, {  10,   12,   21,    3}, {  10,   12,   21,    3}, {  10,   12,   21,    3}, {  10,   12,   21,    3}},
    [12] = {{  11,   13,   22,    4}, {  11,   13,   22,    4}, {  11,   13,   22,    4}, {  11,   13,   22,    4}, {  11,   13,   22,    4}},
    [13] = {{  12,   14,   22,    5}, {  12,   14,   22,    5}, {  12,   14,   22,    5}, {  12,   14,   22,    5}, {  12,   14,   22,    5}},
    [14] = {{  13,   15,   23,    6}, {  13,   15,   23,    6}, {  13,   15,   23,    6}, {  13,   15,   23,    6}, {  13,   15,   23,    6}},
    [15] = {{  14,   31,   23,    7}, {  14,   31,   23,    7}, {  14,   31,   23,    7}, {  14,   31,   23,    7}, {  14,   31,   23,    7}},
    [16] = {{  19,   17,    0,   20}, {  19,   17,    0,   24}, {  19,   17,    0,   24}, {  19,   17,    0,   24}, {  19,   17,    0,   24}},
    [17] = {{  16,   18,    2,   21}, {  16,   18,    2,   24}, {  16,   18,    2,   24}, {  16,   18,    2,   24}, {  16,   18,    2,   24}},
    [18] = {{  17,   19,    4,   22}, {  17,   19,    4,   25}, {  17,   19,    4,   25}, {  17,   19,    4,   25}, {  17,   19,    4,   25}},
    [19] = {{  18,   16,    6,   23}, {  18,   16,    6,   25}, {  18,   16,    6,   25}, {  18,   16,    6,   25}, {  18,   16,    6,   25}},
    [20] = {{  23,   21,   16,    8}, {  23,   21,   26,    8}, {  23,   21,   26,    8}, {  23,   21,   26,    8}, {  23,   21,   26,    8}},
    [21] = {{  20,   22,   17,   10}, {  20,   22,   26,   10}, {  20,   22,   26,   10}, {  20,   22,   26,   10}, {  20,   22,   26,   10}},
    [22] = {{  21,   23,   18,   12}, {  21,   23,   27,   12}, {  21,   23,   27,   12}, {  21,   23,   27,   12}, {  21,   23,   27,   12}},
    [23] = {{  22,   20,   19,   14}, {  22,   20,   27,   14}, {  22,   20,   27,   14}, {  22,   20,   27,   14}, {  22,   20,   27,   14}},
    [24] = {{0xFF, 0xFF, 0xFF, 0xFF}, {  25,   25,   16,   26}, {  25,   25,   16,   28}, {  25,   25,   16,   28}, {  25,   25,   16,   28}},
    [25] = {{0xFF, 0xFF, 0xFF, 0xFF}, {  24,   24,   18,   27}, {  24,   24,   18,   28}, {  24,   24,   18,   28}, {  24,   24,   18,   28}},
    [26] = {{0xFF, 0xFF, 0xFF, 0xFF}, {  27,   27,   24,   20}, {  27,   27,   29,   20}, {  27,   27,   29,   20}, {  27,   27,   29,   20}},
    [27] = {{0xFF, 0xFF, 0xFF, 0xFF}, {  26,   26,   25,   22}, {  26,   26,   29,   22}, {  26,   26,   29,   22}, {  26,   26,   29,   22}},
    [28] = {{0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF,   24,   29}, {0xFF, 0xFF,   24,   30}, {0xFF, 0xFF,   24,   30}},
    [29] = {{0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF,   28,   26}, {0xFF, 0xFF,   30,   26}, {0xFF, 0xFF,   30,   26}},
    [30] = {{0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0xFF, 0xFF}, {0xFF, 0xFF,   28,   29}, {0xFF, 0xFF,   28,   29}},
    [31] = {{  15,    8,    8,    0}, {  15,    8,    8,    0}, {  15,    8,    8,    0}, {  15,    8,    8,    0}, {  15,    8,    8,    0}}, // TOURNEY_TREE_CLOSE_BUTTON
};

static const struct BgTemplate sTourneyTreeBgTemplates[4] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 28,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 29,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 30,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 2,
        .mapBaseIndex = 31,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 2,
        .baseTile = 0
    },
};

static const struct BgTemplate sInfoCardBgTemplates[4] =
{
    {
        .bg = 0,
        .charBaseIndex = 0,
        .mapBaseIndex = 20,
        .screenSize = 3,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 1,
        .charBaseIndex = 1,
        .mapBaseIndex = 24,
        .screenSize = 3,
        .paletteMode = 0,
        .priority = 0,
        .baseTile = 0
    },
    {
        .bg = 2,
        .charBaseIndex = 2,
        .mapBaseIndex = 28,
        .screenSize = 3,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
    {
        .bg = 3,
        .charBaseIndex = 2,
        .mapBaseIndex = 7,
        .screenSize = 0,
        .paletteMode = 0,
        .priority = 1,
        .baseTile = 0
    },
};

static const struct WindowTemplate sTourneyTreeWindowTemplates[] =
{
    [TOURNEYWIN_NAMES_LEFT] = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 3,
        .width = 8,
        .height = 16,
        .paletteNum = 15,
        .baseBlock = 16,
    },
    [TOURNEYWIN_NAMES_RIGHT] = {
        .bg = 0,
        .tilemapLeft = 22,
        .tilemapTop = 3,
        .width = 8,
        .height = 16,
        .paletteNum = 15,
        .baseBlock = 144,
    },
    [TOURNEYWIN_TITLE] = {
        .bg = 0,
        .tilemapLeft = 8,
        .tilemapTop = 1,
        .width = 14,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 272,
    },
    DUMMY_WIN_TEMPLATE,
};

static const struct WindowTemplate sInfoCardWindowTemplates[] =
{
    [WIN_TRAINER_NAME] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 2,
        .width = 26,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 1,
    },
    [WIN_TRAINER_MON1_NAME] = {
        .bg = 0,
        .tilemapLeft = 16,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 53,
    },
    [WIN_TRAINER_MON2_NAME] = {
        .bg = 0,
        .tilemapLeft = 19,
        .tilemapTop = 7,
        .width = 9,
        .height = 3,
        .paletteNum = 15,
        .baseBlock = 69,
    },
    [WIN_TRAINER_MON3_NAME] = {
        .bg = 0,
        .tilemapLeft = 16,
        .tilemapTop = 10,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 96,
    },
    [WIN_TRAINER_FLAVOR_TEXT] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 12,
        .width = 26,
        .height = 7,
        .paletteNum = 15,
        .baseBlock = 112,
    },
    [WIN_MATCH_NUMBER] = {
        .bg = 0,
        .tilemapLeft = 5,
        .tilemapTop = 2,
        .width = 23,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 294,
    },
    [WIN_MATCH_TRAINER_NAME_LEFT] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 340,
    },
    [WIN_MATCH_TRAINER_NAME_RIGHT] = {
        .bg = 0,
        .tilemapLeft = 20,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 356,
    },
    [WIN_MATCH_WIN_TEXT] = {
        .bg = 0,
        .tilemapLeft = 2,
        .tilemapTop = 16,
        .width = 26,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 372,
    },
    // Duplicate windows used by the alternate info card
    // Same as above but on bg 1 instead of bg 0
    [WIN_TRAINER_NAME + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 2,
        .width = 26,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 1,
    },
    [WIN_TRAINER_MON1_NAME + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 16,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 53,
    },
    [WIN_TRAINER_MON2_NAME + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 19,
        .tilemapTop = 7,
        .width = 9,
        .height = 3,
        .paletteNum = 15,
        .baseBlock = 69,
    },
    [WIN_TRAINER_MON3_NAME + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 16,
        .tilemapTop = 10,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 96,
    },
    [WIN_TRAINER_FLAVOR_TEXT + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 12,
        .width = 26,
        .height = 7,
        .paletteNum = 15,
        .baseBlock = 112,
    },
    [WIN_MATCH_NUMBER + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 5,
        .tilemapTop = 2,
        .width = 23,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 294,
    },
    [WIN_MATCH_TRAINER_NAME_LEFT + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 340,
    },
    [WIN_MATCH_TRAINER_NAME_RIGHT + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 20,
        .tilemapTop = 5,
        .width = 8,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 356,
    },
    [WIN_MATCH_WIN_TEXT + NUM_INFO_CARD_WINDOWS] = {
        .bg = 1,
        .tilemapLeft = 2,
        .tilemapTop = 16,
        .width = 26,
        .height = 2,
        .paletteNum = 15,
        .baseBlock = 372,
    },
    #ifdef UBFIX
    DUMMY_WIN_TEMPLATE,
    #endif
};

static const struct ScanlineEffectParams sTourneyTreeScanlineEffectParams =
{
    .dmaDest = &REG_BG3CNT,
    .dmaControl = SCANLINE_EFFECT_DMACNT_16BIT,
    .initState = 1,
};

static const struct CompressedSpriteSheet sTourneyTreeButtonsSpriteSheet[] =
{
    {.data = gDomeTourneyTreeButtons_Gfx, .size = 0x0600, .tag = TAG_BUTTONS},
    {},
};

// Unused
static const struct SpritePalette sTourneyTreeButtonsSpritePal[] =
{
    {.data = gDomeTourneyTreeButtons_Pal, .tag = TAG_BUTTONS},
    {},
};

static const struct OamData sOamData_TourneyTreePokeball =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x16),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

// For Exit/Cancel buttons
static const struct OamData sOamData_TourneyTreeCloseButton =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(32x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(32x16),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 1,
    .affineParam = 0,
};

static const struct OamData sOamData_VerticalScrollArrow =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(16x8),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(16x8),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 2,
    .affineParam = 0,
};

static const struct OamData sOamData_HorizontalScrollArrow =
{
    .y = 0,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_NORMAL,
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(8x16),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(8x16),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 2,
    .affineParam = 0,
};

static const union AnimCmd sSpriteAnim_TourneyTreePokeballNormal[] =
{
    ANIMCMD_FRAME(20, 1),
    ANIMCMD_END,
};
static const union AnimCmd sSpriteAnim_TourneyTreePokeballSelected[] =
{
    ANIMCMD_FRAME(24, 1),
    ANIMCMD_END,
};

static const union AnimCmd *const sSpriteAnimTable_TourneyTreePokeball[] =
{
    sSpriteAnim_TourneyTreePokeballNormal,
    sSpriteAnim_TourneyTreePokeballSelected,
};

// Sprite template for the pokeballs on the tourney tree that act as buttons to view a trainer/match info card
static const struct SpriteTemplate sTourneyTreePokeballSpriteTemplate =
{
    .tileTag = TAG_BUTTONS,
    .paletteTag = TAG_NONE,
    .oam = &sOamData_TourneyTreePokeball,
    .anims = sSpriteAnimTable_TourneyTreePokeball,
};

static const union AnimCmd sSpriteAnim_TourneyTreeCancelButtonNormal[] =
{
    ANIMCMD_FRAME(8, 1),
    ANIMCMD_END,
};

static const union AnimCmd sSpriteAnim_TourneyTreeCancelButtonSelected[] =
{
    ANIMCMD_FRAME(0, 1),
    ANIMCMD_END,
};

static const union AnimCmd *const sSpriteAnimTable_TourneyTreeCancelButton[] =
{
    sSpriteAnim_TourneyTreeCancelButtonNormal,
    sSpriteAnim_TourneyTreeCancelButtonSelected,
};

static const struct SpriteTemplate sCancelButtonSpriteTemplate =
{
    .tileTag = TAG_BUTTONS,
    .paletteTag = TAG_NONE,
    .oam = &sOamData_TourneyTreeCloseButton,
    .anims = sSpriteAnimTable_TourneyTreeCancelButton,
};

static const union AnimCmd sSpriteAnim_TourneyTreeExitButtonNormal[] =
{
    ANIMCMD_FRAME(40, 1),
    ANIMCMD_END,
};

static const union AnimCmd sSpriteAnim_TourneyTreeExitButtonSelected[] =
{
    ANIMCMD_FRAME(32, 1),
    ANIMCMD_END,
 };

static const union AnimCmd *const sSpriteAnimTable_TourneyTreeExitButton[] =
{
    sSpriteAnim_TourneyTreeExitButtonNormal,
    sSpriteAnim_TourneyTreeExitButtonSelected,
};

static const struct SpriteTemplate sExitButtonSpriteTemplate =
{
    .tileTag = TAG_BUTTONS,
    .paletteTag = TAG_NONE,
    .oam = &sOamData_TourneyTreeCloseButton,
    .anims = sSpriteAnimTable_TourneyTreeExitButton,
};

static const union AnimCmd sSpriteAnim_UpArrow[] =
{
    ANIMCMD_FRAME(18, 1),
    ANIMCMD_END,
};

static const union AnimCmd sSpriteAnim_DownArrow[] =
{
    ANIMCMD_FRAME(18, 1, .vFlip = TRUE),
    ANIMCMD_END,
 };

static const union AnimCmd sSpriteAnim_LeftArrow[] =
{
    ANIMCMD_FRAME(16, 1, .hFlip = TRUE),
    ANIMCMD_END,
};

static const union AnimCmd sSpriteAnim_RightArrow[] =
{
    ANIMCMD_FRAME(16, 1),
    ANIMCMD_END,
};

static const union AnimCmd *const sSpriteAnimTable_VerticalScrollArrow[] =
{
    sSpriteAnim_UpArrow,
    sSpriteAnim_DownArrow,
};

static const union AnimCmd *const sSpriteAnimTable_HorizontalScrollArrow[] =
{
    sSpriteAnim_LeftArrow,
    sSpriteAnim_RightArrow,
};

static const struct SpriteTemplate sHorizontalScrollArrowSpriteTemplate =
{
    .tileTag = TAG_BUTTONS,
    .paletteTag = TAG_NONE,
    .oam = &sOamData_HorizontalScrollArrow,
    .anims = sSpriteAnimTable_HorizontalScrollArrow,
    .callback = SpriteCB_HorizontalScrollArrow
};

static const struct SpriteTemplate sVerticalScrollArrowSpriteTemplate =
{
    .tileTag = TAG_BUTTONS,
    .paletteTag = TAG_NONE,
    .oam = &sOamData_VerticalScrollArrow,
    .anims = sSpriteAnimTable_VerticalScrollArrow,
    .callback = SpriteCB_VerticalScrollArrow
};

// Organized by seed starting position, i.e. seed 0 battles seed 8 first
static const u8 sTourneyTreeTrainerIds[DOME_TOURNAMENT_TRAINERS_COUNT] = {0, 8, 12, 4, 7, 15, 11, 3, 2, 10, 14, 6, 5, 13, 9, 1};

static void (*const sBattleDomeFunctions[])(void) =
{
    [BATTLE_DOME_FUNC_INIT]                     = InitDomeChallenge,
    [BATTLE_DOME_FUNC_GET_DATA]                 = GetDomeData,
    [BATTLE_DOME_FUNC_SET_DATA]                 = SetDomeData,
    [BATTLE_DOME_FUNC_GET_ROUND_TEXT]           = BufferDomeRoundText,
    [BATTLE_DOME_FUNC_GET_OPPONENT_NAME]        = BufferDomeOpponentName,
    [BATTLE_DOME_FUNC_INIT_OPPONENT_PARTY]      = InitDomeOpponentParty,
    [BATTLE_DOME_FUNC_SHOW_OPPONENT_INFO]       = ShowDomeOpponentInfo,
    [BATTLE_DOME_FUNC_SHOW_TOURNEY_TREE]        = ShowDomeTourneyTree,
    [BATTLE_DOME_FUNC_SHOW_PREV_TOURNEY_TREE]   = ShowPreviousDomeTourneyTree,
    [BATTLE_DOME_FUNC_SET_OPPONENT_ID]          = SetDomeOpponentId,
    [BATTLE_DOME_FUNC_SET_OPPONENT_GFX]         = SetDomeOpponentGraphicsId,
    [BATTLE_DOME_FUNC_SHOW_STATIC_TOURNEY_TREE] = ShowNonInteractiveDomeTourneyTree,
    [BATTLE_DOME_FUNC_RESOLVE_WINNERS]          = ResolveDomeRoundWinners,
    [BATTLE_DOME_FUNC_SAVE]                     = SaveDomeChallenge,
    [BATTLE_DOME_FUNC_INCREMENT_STREAK]         = IncrementDomeStreaks,
    [BATTLE_DOME_FUNC_SET_TRAINERS]             = SetFacilityTrainerAndMonPtrs,
    [BATTLE_DOME_FUNC_RESET_SKETCH]             = ResetSketchedMoves,
    [BATTLE_DOME_FUNC_RESTORE_HELD_ITEMS]       = RestoreDomePlayerPartyHeldItems,
    [BATTLE_DOME_FUNC_REDUCE_PARTY]             = ReduceDomePlayerPartyToSelectedMons,
    [BATTLE_DOME_FUNC_COMPARE_SEEDS]            = GetPlayerSeededBeforeOpponent,
    [BATTLE_DOME_FUNC_GET_WINNER_NAME]          = BufferLastDomeWinnerName,
    [BATTLE_DOME_FUNC_INIT_RESULTS_TREE]        = InitRandomTourneyTreeResults,
    [BATTLE_DOME_FUNC_INIT_TRAINERS]            = InitDomeTrainers,
};

static const u32 sWinStreakFlags[][2] =
{
    {STREAK_DOME_SINGLES_50, STREAK_DOME_SINGLES_OPEN},
    {STREAK_DOME_DOUBLES_50, STREAK_DOME_DOUBLES_OPEN},
};

static const u32 sWinStreakMasks[][2] =
{
    {~(STREAK_DOME_SINGLES_50), ~(STREAK_DOME_SINGLES_OPEN)},
    {~(STREAK_DOME_DOUBLES_50), ~(STREAK_DOME_DOUBLES_OPEN)},
};

// TODO: The below two arrays probably need better names. The one below for example is only true of sIdToOpponentId[i][0]
static const u8 sIdToOpponentId[DOME_TOURNAMENT_TRAINERS_COUNT][DOME_ROUNDS_COUNT] =
{
    [0]  = { 8,  0,  4,  8},
    [1]  = { 9, 12,  8,  0},
    [2]  = {10,  8, 12,  0},
    [3]  = {11,  4,  0,  8},
    [4]  = {12,  0,  4,  8},
    [5]  = {13, 12,  8,  0},
    [6]  = {14,  8, 12,  0},
    [7]  = {15,  4,  0,  8},
    [8]  = { 0,  0,  4,  8},
    [9]  = { 1, 12,  8,  0},
    [10] = { 2,  8, 12,  0},
    [11] = { 3,  4,  0,  8},
    [12] = { 4,  0,  4,  8},
    [13] = { 5, 12,  8,  0},
    [14] = { 6,  8, 12,  0},
    [15] = { 7,  4,  0,  8},
};

// sTourneyTreeTrainerIds with every other pair swapped
static const u8 sTourneyTreeTrainerOpponentIds[DOME_TOURNAMENT_TRAINERS_COUNT] = { 0, 8, 4, 12, 7, 15, 3, 11, 2, 10, 6, 14, 5, 13, 1, 9 };

// The match number - 1 that a given tournament trainer will participate in for a given round
static const u8 sIdToMatchNumber[DOME_TOURNAMENT_TRAINERS_COUNT][DOME_ROUNDS_COUNT] =
{
    { 0,  8, 12, 14},
    { 0,  8, 12, 14},
    { 1,  8, 12, 14},
    { 1,  8, 12, 14},
    { 2,  9, 12, 14},
    { 2,  9, 12, 14},
    { 3,  9, 12, 14},
    { 3,  9, 12, 14},
    { 4, 10, 13, 14},
    { 4, 10, 13, 14},
    { 5, 10, 13, 14},
    { 5, 10, 13, 14},
    { 6, 11, 13, 14},
    { 6, 11, 13, 14},
    { 7, 11, 13, 14},
    { 7, 11, 13, 14},
};

static const u8 sLastMatchCardNum[DOME_ROUNDS_COUNT] =
{
    [DOME_ROUND1]    = 23,
    [DOME_ROUND2]    = 27,
    [DOME_SEMIFINAL] = 29,
    [DOME_FINAL]     = 30
};

static const u8 sTrainerAndRoundToLastMatchCardNum[DOME_TOURNAMENT_TRAINERS_COUNT / 2][DOME_ROUNDS_COUNT] =
{
    {16, 24, 28, 30},
    {17, 24, 28, 30},
    {18, 25, 28, 30},
    {19, 25, 28, 30},
    {20, 26, 29, 30},
    {21, 26, 29, 30},
    {22, 27, 29, 30},
    {23, 27, 29, 30},
};

static const u8 sTournamentIdToPairedTrainerIds[DOME_TOURNAMENT_TRAINERS_COUNT] = {0, 15, 8, 7, 3, 12, 11, 4, 1, 14, 9, 6, 2, 13, 10, 5};

// The first line of text on a trainers info card. It describes their potential to win, based on their seed in the tournament tree.
// Dome Ace Tucker has their own separate potential text.
static const u8 *const sBattleDomePotentialTexts[DOME_TOURNAMENT_TRAINERS_COUNT + 1] =
{
    COMPOUND_STRING("The best candidate to be a champ!"), // Highest potential
    COMPOUND_STRING("A sure-finalist team."),
    COMPOUND_STRING("A likely top-three finisher."),
    COMPOUND_STRING("A candidate to finish first."),
    COMPOUND_STRING("A team with top-class potential."),
    COMPOUND_STRING("The dark horse team this tournament."),
    COMPOUND_STRING("A better-than-average team."),
    COMPOUND_STRING("This tournament's average team."),
    COMPOUND_STRING("A team with average potential."),
    COMPOUND_STRING("A weaker-than-average team."),
    COMPOUND_STRING("A team looking for its first win."),
    COMPOUND_STRING("One win will make this team proud."),
    COMPOUND_STRING("Overall, a weak team."),
    COMPOUND_STRING("A team with very low potential."),
    COMPOUND_STRING("A team unlikely to win the tournament."),
    COMPOUND_STRING("The team most unlikely to win."), // Lowest potential
    COMPOUND_STRING("The perfect, invincible superstar!"), // Dome Ace Tucker
};

// The second line of text on a trainers info card. It gives information about their battle style (dependent on their party's moves).
static const u8 *const sBattleDomeOpponentStyleTexts[NUM_BATTLE_STYLES] =
{
    [DOME_BATTLE_STYLE_RISKY]           = COMPOUND_STRING("Willing to risk total disaster at times."),
    [DOME_BATTLE_STYLE_STALL]           = COMPOUND_STRING("Skilled at enduring long battles."),
    [DOME_BATTLE_STYLE_VARIED]          = COMPOUND_STRING("Varies tactics to suit the opponent."),
    [DOME_BATTLE_STYLE_COMBO_HIGH]      = COMPOUND_STRING("Has a tough winning pattern."),
    [DOME_BATTLE_STYLE_RARE_MOVES]      = COMPOUND_STRING("Occasionally uses a very rare move."),  // Seems like the text for these two was swapped
    [DOME_BATTLE_STYLE_RARE_MOVE]       = COMPOUND_STRING("Uses startling and disruptive moves."), //
    [DOME_BATTLE_STYLE_HP]              = COMPOUND_STRING("Constantly watches HP in battle."),
    [DOME_BATTLE_STYLE_STORE_POWER]     = COMPOUND_STRING("Good at storing then loosing power."),
    [DOME_BATTLE_STYLE_ENFEEBLE_LOW]    = COMPOUND_STRING("Skilled at enfeebling foes."),
    [DOME_BATTLE_STYLE_LUCK]            = COMPOUND_STRING("Prefers tactics that rely on luck."),
    [DOME_BATTLE_STYLE_REGAL]           = COMPOUND_STRING("Attacks with a regal atmosphere."),
    [DOME_BATTLE_STYLE_LOW_PP]          = COMPOUND_STRING("Attacks with powerful, low-PP moves."),
    [DOME_BATTLE_STYLE_STATUS_ATK]      = COMPOUND_STRING("Skilled at enfeebling, then attacking."),
    [DOME_BATTLE_STYLE_ENDURE]          = COMPOUND_STRING("Battles while enduring all attacks."),
    [DOME_BATTLE_STYLE_STATUS]          = COMPOUND_STRING("Skilled at upsetting foes emotionally."),
    [DOME_BATTLE_STYLE_STRAIGHTFORWARD] = COMPOUND_STRING("Uses strong and straightforward moves."),
    [DOME_BATTLE_STYLE_AGGRESSIVE]      = COMPOUND_STRING("Aggressively uses strong moves."),
    [DOME_BATTLE_STYLE_DEF]             = COMPOUND_STRING("Battles while cleverly dodging attacks."),
    [DOME_BATTLE_STYLE_ENFEEBLE_HIGH]   = COMPOUND_STRING("Skilled at using upsetting attacks."),
    [DOME_BATTLE_STYLE_POPULAR_POWER]   = COMPOUND_STRING("Uses many popular moves."),
    [DOME_BATTLE_STYLE_COMBO_LOW]       = COMPOUND_STRING("Has moves for powerful combinations."),
    [DOME_BATTLE_STYLE_ACCURATE]        = COMPOUND_STRING("Uses high-probability attacks."),
    [DOME_BATTLE_STYLE_POWERFUL]        = COMPOUND_STRING("Aggressively uses spectacular moves."),
    [DOME_BATTLE_STYLE_ATK_OVER_DEF]    = COMPOUND_STRING("Emphasizes offense over defense."),
    [DOME_BATTLE_STYLE_DEF_OVER_ATK]    = COMPOUND_STRING("Emphasizes defense over offense."),
    [DOME_BATTLE_STYLE_POPULAR_STRONG]  = COMPOUND_STRING("Attacks quickly with strong moves."),
    [DOME_BATTLE_STYLE_EFFECTS]         = COMPOUND_STRING("Often uses moves with added effects."),
    [DOME_BATTLE_STYLE_BALANCED]        = COMPOUND_STRING("Uses a well-balanced mix of moves."),
    [DOME_BATTLE_STYLE_UNUSED1]         = COMPOUND_STRING("This is sample message 1."),
    [DOME_BATTLE_STYLE_UNUSED2]         = COMPOUND_STRING("This is sample message 2."),
    [DOME_BATTLE_STYLE_UNUSED3]         = COMPOUND_STRING("This is sample message 3."),
    [DOME_BATTLE_STYLE_UNUSED4]         = COMPOUND_STRING("This is sample message 4."),
};

// The third line of text on a trainers info card. It that gives information about their party's stat spread (based on their Pokémon's effort values and Nature).
static const u8 *const sBattleDomeOpponentStatsTexts[] =
{
    COMPOUND_STRING("Emphasizes HP and ATTACK."),              // DOME_TEXT_TWO_GOOD_STATS and DOME_TEXT_HP start here
    COMPOUND_STRING("Emphasizes HP and DEFENSE."),
    COMPOUND_STRING("Emphasizes HP and SPEED."),
    COMPOUND_STRING("Emphasizes HP and SP. ATTACK."),
    COMPOUND_STRING("Emphasizes HP and SP. DEFENSE."),
    COMPOUND_STRING("Emphasizes ATTACK and DEFENSE."),         // DOME_TEXT_ATK starts here
    COMPOUND_STRING("Emphasizes ATTACK and SPEED."),
    COMPOUND_STRING("Emphasizes ATTACK and SP. ATTACK."),
    COMPOUND_STRING("Emphasizes ATTACK and SP. DEFENSE."),
    COMPOUND_STRING("Emphasizes DEFENSE and SPEED."),          // DOME_TEXT_DEF starts here
    COMPOUND_STRING("Emphasizes DEFENSE and SP. ATTACK."),
    COMPOUND_STRING("Emphasizes DEFENSE and SP. DEFENSE."),
    COMPOUND_STRING("Emphasizes SPEED and SP. ATTACK."),       // DOME_TEXT_SPEED starts here
    COMPOUND_STRING("Emphasizes SPEED and SP. DEFENSE."),
    COMPOUND_STRING("Emphasizes SP. ATTACK and SP. DEFENSE."), // DOME_TEXT_SPATK starts here
    COMPOUND_STRING("Emphasizes HP."),                         // DOME_TEXT_ONE_GOOD_STAT starts here
    COMPOUND_STRING("Emphasizes ATTACK."),
    COMPOUND_STRING("Emphasizes DEFENSE."),
    COMPOUND_STRING("Emphasizes SPEED."),
    COMPOUND_STRING("Emphasizes SP. ATTACK."),
    COMPOUND_STRING("Emphasizes SP. DEFENSE."),
    COMPOUND_STRING("Neglects HP and ATTACK."),                // DOME_TEXT_TWO_BAD_STATS starts here
    COMPOUND_STRING("Neglects HP and DEFENSE."),
    COMPOUND_STRING("Neglects HP and SPEED."),
    COMPOUND_STRING("Neglects HP and SP. ATTACK."),
    COMPOUND_STRING("Neglects HP and SP. DEFENSE."),
    COMPOUND_STRING("Neglects ATTACK and DEFENSE."),
    COMPOUND_STRING("Neglects ATTACK and SPEED."),
    COMPOUND_STRING("Neglects ATTACK and SP. ATTACK."),
    COMPOUND_STRING("Neglects ATTACK and SP. DEFENSE."),
    COMPOUND_STRING("Neglects DEFENSE and SPEED."),
    COMPOUND_STRING("Neglects DEFENSE and SP. ATTACK."),
    COMPOUND_STRING("Neglects DEFENSE and SP. DEFENSE."),
    COMPOUND_STRING("Neglects SPEED and SP. ATTACK."),
    COMPOUND_STRING("Neglects SPEED and SP. DEFENSE."),
    COMPOUND_STRING("Neglects SP. ATTACK and SP. DEFENSE."),
    COMPOUND_STRING("Neglects HP."),                           // DOME_TEXT_ONE_BAD_STAT starts here
    COMPOUND_STRING("Neglects ATTACK."),
    COMPOUND_STRING("Neglects DEFENSE."),
    COMPOUND_STRING("Neglects SPEED."),
    COMPOUND_STRING("Neglects SP. ATTACK."),
    COMPOUND_STRING("Neglects SP. DEFENSE."),
    [DOME_TEXT_WELL_BALANCED] = COMPOUND_STRING("Raises POKéMON in a well-balanced way."),
};

static const u8 sInfoTrainerMonX[FRONTIER_PARTY_SIZE] = {104, 136, 104};
static const u8 sInfoTrainerMonY[FRONTIER_PARTY_SIZE] = { 38,  62,  78};
static const u8 sSpeciesNameTextYCoords[] = {0, 4, 0};

// Offsets within sBattleDomeOpponentStatsTexts for stat combinations
// SPDEF has no offset because by then all stat combinations have been reached, so it has no combination texts
static const u8 sStatTextOffsets[NUM_STATS - 1] =
{
    DOME_TEXT_HP,
    DOME_TEXT_ATK,
    DOME_TEXT_DEF,
    DOME_TEXT_SPEED,
    DOME_TEXT_SPATK
};

static const u8 *const sBattleDomeMatchNumberTexts[DOME_TOURNAMENT_MATCHES_COUNT] =
{
    COMPOUND_STRING("Round 1, Match 1"),
    COMPOUND_STRING("Round 1, Match 2"),
    COMPOUND_STRING("Round 1, Match 3"),
    COMPOUND_STRING("Round 1, Match 4"),
    COMPOUND_STRING("Round 1, Match 5"),
    COMPOUND_STRING("Round 1, Match 6"),
    COMPOUND_STRING("Round 1, Match 7"),
    COMPOUND_STRING("Round 1, Match 8"),
    COMPOUND_STRING("Round 2, Match 1"),
    COMPOUND_STRING("Round 2, Match 2"),
    COMPOUND_STRING("Round 2, Match 3"),
    COMPOUND_STRING("Round 2, Match 4"),
    COMPOUND_STRING("Semifinal Match 1"),
    COMPOUND_STRING("Semifinal Match 2"),
    COMPOUND_STRING("Final Match"),
};

static const u8 *const sBattleDomeWinTexts[] =
{
    [DOME_TEXT_NO_WINNER_YET]    = COMPOUND_STRING("Let the battle begin!"),
    [DOME_TEXT_WON_USING_MOVE]   = COMPOUND_STRING("{STR_VAR_1} won using {STR_VAR_2}!"),
    [DOME_TEXT_CHAMP_USING_MOVE] = COMPOUND_STRING("{STR_VAR_1} became the champ!"),
    [DOME_TEXT_WON_ON_FORFEIT]   = COMPOUND_STRING("{STR_VAR_1} won by default!"),
    [DOME_TEXT_CHAMP_ON_FORFEIT] = COMPOUND_STRING("{STR_VAR_1} won outright by default!"),
    [DOME_TEXT_WON_NO_MOVES]     = COMPOUND_STRING("{STR_VAR_1} won without using a move!"),
    [DOME_TEXT_CHAMP_NO_MOVES]   = COMPOUND_STRING("{STR_VAR_1} won outright with no moves!"),
};

static const u8 sLeftTrainerMonX[FRONTIER_PARTY_SIZE]  = { 96,  96,  96};
static const u8 sLeftTrainerMonY[FRONTIER_PARTY_SIZE]  = { 56,  80, 104};
static const u8 sRightTrainerMonX[FRONTIER_PARTY_SIZE] = {144, 144, 144};
static const u8 sRightTrainerMonY[FRONTIER_PARTY_SIZE] = { 56,  80, 104};

// Duplicate of sTourneyTreeTrainerIds
static const u8 sTourneyTreeTrainerIds2[DOME_TOURNAMENT_TRAINERS_COUNT] = {0, 8, 12, 4, 7, 15, 11, 3, 2, 10, 14, 6, 5, 13, 9, 1};

// The number of possible trainers that could be competing in a given match
#define NUM_POSSIBLE_MATCH_TRAINERS(round) (DOME_TOURNAMENT_TRAINERS_COUNT / (1 << (DOME_ROUNDS_COUNT - round - 1)))

// The range of tournament trainers to check as possible participants in a given match
// Given by the offset in sCompetitorRangeByMatch[][0], the number of trainers in sCompetitorRangeByMatch[][1], and the round
static const u8 sCompetitorRangeByMatch[DOME_TOURNAMENT_MATCHES_COUNT][3] =
{
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 0,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 1,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 2,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 3,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 4,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 5,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 6,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1) * 7,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND1),    DOME_ROUND1},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2) * 0,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2),    DOME_ROUND2},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2) * 1,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2),    DOME_ROUND2},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2) * 2,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2),    DOME_ROUND2},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2) * 3,    NUM_POSSIBLE_MATCH_TRAINERS(DOME_ROUND2),    DOME_ROUND2},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_SEMIFINAL) * 0, NUM_POSSIBLE_MATCH_TRAINERS(DOME_SEMIFINAL), DOME_SEMIFINAL},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_SEMIFINAL) * 1, NUM_POSSIBLE_MATCH_TRAINERS(DOME_SEMIFINAL), DOME_SEMIFINAL},
    { NUM_POSSIBLE_MATCH_TRAINERS(DOME_FINAL) * 0,     NUM_POSSIBLE_MATCH_TRAINERS(DOME_FINAL),     DOME_FINAL},
};

#define NAME_ROW_HEIGHT 16

// 1st value is the windowId, 2nd value is the y coord
static const u8 sTrainerNamePositions[DOME_TOURNAMENT_TRAINERS_COUNT][2] =
{
    { TOURNEYWIN_NAMES_LEFT,  0 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 7 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 0 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_LEFT,  7 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_LEFT,  3 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 4 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 3 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_LEFT,  4 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_LEFT,  1 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 6 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 1 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_LEFT,  6 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_LEFT,  2 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 5 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_RIGHT, 2 * NAME_ROW_HEIGHT},
    { TOURNEYWIN_NAMES_LEFT,  5 * NAME_ROW_HEIGHT},
};

// Coords for the pokeballs on the tourney tree that act as buttons to view trainer/match info
static const u8 sTourneyTreePokeballCoords[DOME_TOURNAMENT_TRAINERS_COUNT + DOME_TOURNAMENT_MATCHES_COUNT][2] =
{
    { 68,  33}, // Left side trainers
    { 68,  49},
    { 68,  65},
    { 68,  81},
    { 68,  97},
    { 68, 113},
    { 68, 129},
    { 68, 145},
    {172,  33}, // Right side trainers
    {172,  49},
    {172,  65},
    {172,  81},
    {172,  97},
    {172, 113},
    {172, 129},
    {172, 145},
    { 87,  41}, // Left side Round 1 matches
    { 87,  73},
    { 87, 105},
    { 87, 137},
    {153,  41}, // Right side Round 1 matches
    {153,  73},
    {153, 105},
    {153, 137},
    { 95,  57}, // Left side Round 2 matches
    { 95, 121},
    {145,  57}, // Right side Round 2 matches
    {145, 121},
    {103,  89}, // Left side semifinal match
    {137,  89}, // Right side semifinal match
    {120,  89}, // Final match
};

// Tile values from tourney_tree.png for the highlighted lines of the tourney tree.
// These tiles will be used to replace the existing, unhighlighted line tiles on the tourney tree tilemap.
#define LINE_PAL           (6 << 12)
#define LINE_H             (LINE_PAL | 0x21) // Horizontal
#define LINE_CORNER_R      (LINE_PAL | 0x23) // Horizontal into a right-side vertical
#define LINE_CORNER_L      (LINE_PAL | 0x25) // Horizontal into a left-side vertical
#define LINE_V_R           (LINE_PAL | 0x27) // Right-side vertical
#define LINE_V_L           (LINE_PAL | 0x29) // Left-side vertical
#define LINE_H_BOTTOM      (LINE_PAL | 0x2B) // Horizontal on the bottom of the tree
#define LINE_H_LOGO1       (LINE_PAL | 0x2C) // Horizontal, logo behind
#define LINE_H_LOGO2       (LINE_PAL | 0x2D) // Horizontal, logo behind
#define LINE_H_LOGO3       (LINE_PAL | 0x2E) // Horizontal, logo behind
#define LINE_H_LOGO4       (LINE_PAL | 0x2F) // Horizontal, logo behind
#define LINE_V_R_LOGO1     (LINE_PAL | 0x30) // Right-side vertical, logo behind
#define LINE_V_R_LOGO2     (LINE_PAL | 0x31) // Right-side vertical, logo behind
#define LINE_V_R_LOGO3     (LINE_PAL | 0x32) // Right-side vertical, logo behind
#define LINE_V_R_LOGO4     (LINE_PAL | 0x33) // Right-side vertical, logo behind
#define LINE_V_L_LOGO1     (LINE_PAL | 0x35) // Left-side vertical, logo behind
#define LINE_V_L_LOGO2     (LINE_PAL | 0x36) // Left-side vertical, logo behind
#define LINE_V_L_LOGO3     (LINE_PAL | 0x37) // Left-side vertical, logo behind
#define LINE_V_L_LOGO4     (LINE_PAL | 0x38) // Left-side vertical, logo behind
#define LINE_V_R_HALF_LOGO (LINE_PAL | 0x3B) // Right-side vertical, half lit from the top, logo behind
#define LINE_V_L_HALF_LOGO (LINE_PAL | 0x3C) // Left-side vertical, half lit from the top, logo behind
#define LINE_CORNER_R_HALF (LINE_PAL | 0x43) // Lit horizontal, unlit right-side vertical
#define LINE_CORNER_L_HALF (LINE_PAL | 0x45) // Lit horizontal, unlit left-side vertical
#define LINE_V_R_HALF      (LINE_PAL | 0x47) // Right-side vertical, half lit from the top
#define LINE_V_L_HALF      (LINE_PAL | 0x49) // Left-side vertical, half lit from the top

// Each of these line sections define the position of the advancement line on the tourney tree for the victor of that round
// The trainers here are numbered by tourney ID (rank/seed) and ordered according to where they start on the tourney tree
#define LINESECTION_ROUND1_TRAINER1(lastTile) \
    {.tile = LINE_H,        .y =  4, .x =  9}, \
    {.tile = LINE_CORNER_R, .y =  4, .x = 10}, \
    {.tile = LINE_V_R_HALF, .y =  5, .x = 10}, \
    {.tile = lastTile,      .y =  5, .x = 11},

#define LINESECTION_ROUND1_TRAINER9(lastTile) \
    {.tile = LINE_H,   .y =  6, .x =  9}, \
    {.tile = LINE_H,   .y =  6, .x = 10}, \
    {.tile = LINE_V_R, .y =  5, .x = 10}, \
    {.tile = lastTile, .y =  5, .x = 11},

#define LINESECTION_ROUND1_TRAINER13(lastTile) \
    {.tile = LINE_H,        .y =  8, .x =  9}, \
    {.tile = LINE_CORNER_R, .y =  8, .x = 10}, \
    {.tile = LINE_V_R_HALF, .y =  9, .x = 10}, \
    {.tile = lastTile,      .y =  9, .x = 11},

#define LINESECTION_ROUND1_TRAINER5(lastTile) \
    {.tile = LINE_H,   .y = 10, .x =  9}, \
    {.tile = LINE_H,   .y = 10, .x = 10}, \
    {.tile = LINE_V_R, .y =  9, .x = 10}, \
    {.tile = lastTile, .y =  9, .x = 11},

#define LINESECTION_ROUND1_TRAINER8(lastTile) \
    {.tile = LINE_H,        .y = 12, .x =  9}, \
    {.tile = LINE_CORNER_R, .y = 12, .x = 10}, \
    {.tile = LINE_V_R_HALF, .y = 13, .x = 10}, \
    {.tile = lastTile,      .y = 13, .x = 11},

#define LINESECTION_ROUND1_TRAINER16(lastTile) \
    {.tile = LINE_H,   .y = 14, .x =  9}, \
    {.tile = LINE_H,   .y = 14, .x = 10}, \
    {.tile = LINE_V_R, .y = 13, .x = 10}, \
    {.tile = lastTile, .y = 13, .x = 11},

#define LINESECTION_ROUND1_TRAINER12(lastTile) \
    {.tile = LINE_H,        .y = 16, .x =  9}, \
    {.tile = LINE_CORNER_R, .y = 16, .x = 10}, \
    {.tile = LINE_V_R_HALF, .y = 17, .x = 10}, \
    {.tile = lastTile,      .y = 17, .x = 11},

#define LINESECTION_ROUND1_TRAINER4(lastTile) \
    {.tile = LINE_H_BOTTOM, .y = 18, .x =  9}, \
    {.tile = LINE_H_BOTTOM, .y = 18, .x = 10}, \
    {.tile = LINE_V_R,      .y = 17, .x = 10}, \
    {.tile = lastTile,      .y = 17, .x = 11},

#define LINESECTION_ROUND1_TRAINER3(lastTile) \
    {.tile = LINE_H,        .y =  4, .x = 20}, \
    {.tile = LINE_CORNER_L, .y =  4, .x = 19}, \
    {.tile = LINE_V_L_HALF, .y =  5, .x = 19}, \
    {.tile = lastTile,      .y =  5, .x = 18},

#define LINESECTION_ROUND1_TRAINER11(lastTile) \
    {.tile = LINE_H,   .y =  6, .x = 20}, \
    {.tile = LINE_H,   .y =  6, .x = 19}, \
    {.tile = LINE_V_L, .y =  5, .x = 19}, \
    {.tile = lastTile, .y =  5, .x = 18},

#define LINESECTION_ROUND1_TRAINER15(lastTile) \
    {.tile = LINE_H,        .y =  8, .x = 20}, \
    {.tile = LINE_CORNER_L, .y =  8, .x = 19}, \
    {.tile = LINE_V_L_HALF, .y =  9, .x = 19}, \
    {.tile = lastTile,      .y =  9, .x = 18},

#define LINESECTION_ROUND1_TRAINER7(lastTile) \
    {.tile = LINE_H,   .y = 10, .x = 20}, \
    {.tile = LINE_H,   .y = 10, .x = 19}, \
    {.tile = LINE_V_L, .y =  9, .x = 19}, \
    {.tile = lastTile, .y =  9, .x = 18},

#define LINESECTION_ROUND1_TRAINER6(lastTile) \
    {.tile = LINE_H,        .y = 12, .x = 20}, \
    {.tile = LINE_CORNER_L, .y = 12, .x = 19}, \
    {.tile = LINE_V_L_HALF, .y = 13, .x = 19}, \
    {.tile = lastTile,      .y = 13, .x = 18},

#define LINESECTION_ROUND1_TRAINER14(lastTile) \
    {.tile = LINE_H,   .y = 14, .x = 20}, \
    {.tile = LINE_H,   .y = 14, .x = 19}, \
    {.tile = LINE_V_L, .y = 13, .x = 19}, \
    {.tile = lastTile, .y = 13, .x = 18},

#define LINESECTION_ROUND1_TRAINER10(lastTile) \
    {.tile = LINE_H,        .y = 16, .x = 20}, \
    {.tile = LINE_CORNER_L, .y = 16, .x = 19}, \
    {.tile = LINE_V_L_HALF, .y = 17, .x = 19}, \
    {.tile = lastTile,      .y = 17, .x = 18},

#define LINESECTION_ROUND1_TRAINER2(lastTile) \
    {.tile = LINE_H_BOTTOM, .y = 18, .x = 20}, \
    {.tile = LINE_H_BOTTOM, .y = 18, .x = 19}, \
    {.tile = LINE_V_L,      .y = 17, .x = 19}, \
    {.tile = lastTile,      .y = 17, .x = 18},

#define LINESECTION_ROUND2_MATCH1(lastTile) \
    {.tile = LINE_V_R,      .y =  6, .x = 11}, \
    {.tile = LINE_V_R_HALF, .y =  7, .x = 11}, \
    {.tile = lastTile,      .y =  7, .x = 12},

#define LINESECTION_ROUND2_MATCH2(lastTile) \
    {.tile = LINE_V_R, .y =  8, .x = 11}, \
    {.tile = LINE_V_R, .y =  7, .x = 11}, \
    {.tile = lastTile, .y =  7, .x = 12},

#define LINESECTION_ROUND2_MATCH3(lastTile) \
    {.tile = LINE_V_R,      .y = 14, .x = 11}, \
    {.tile = LINE_V_R_HALF, .y = 15, .x = 11}, \
    {.tile = lastTile,      .y = 15, .x = 12},

#define LINESECTION_ROUND2_MATCH4(lastTile) \
    {.tile = LINE_V_R, .y = 16, .x = 11}, \
    {.tile = LINE_V_R, .y = 15, .x = 11}, \
    {.tile = lastTile, .y = 15, .x = 12},

#define LINESECTION_ROUND2_MATCH5(lastTile) \
    {.tile = LINE_V_L,      .y =  6, .x = 18}, \
    {.tile = LINE_V_L_HALF, .y =  7, .x = 18}, \
    {.tile = lastTile,      .y =  7, .x = 17},

#define LINESECTION_ROUND2_MATCH6(lastTile) \
    {.tile = LINE_V_L, .y =  8, .x = 18}, \
    {.tile = LINE_V_L, .y =  7, .x = 18}, \
    {.tile = lastTile, .y =  7, .x = 17},

#define LINESECTION_ROUND2_MATCH7(lastTile) \
    {.tile = LINE_V_L,      .y = 14, .x = 18}, \
    {.tile = LINE_V_L_HALF, .y = 15, .x = 18}, \
    {.tile = lastTile,      .y = 15, .x = 17},

#define LINESECTION_ROUND2_MATCH8(lastTile) \
    {.tile = LINE_V_L, .y = 16, .x = 18}, \
    {.tile = LINE_V_L, .y = 15, .x = 18}, \
    {.tile = lastTile, .y = 15, .x = 17},

#define LINESECTION_SEMIFINAL_TOP_LEFT \
    {.tile = LINE_V_R,           .y =  8, .x = 12}, \
    {.tile = LINE_V_R,           .y =  9, .x = 12}, \
    {.tile = LINE_V_R,           .y = 10, .x = 12}, \
    {.tile = LINE_V_R_HALF_LOGO, .y = 11, .x = 12},

#define LINESECTION_SEMIFINAL_BOTTOM_LEFT \
    {.tile = LINE_V_R_LOGO4, .y = 14, .x = 12}, \
    {.tile = LINE_V_R_LOGO3, .y = 13, .x = 12}, \
    {.tile = LINE_V_R_LOGO2, .y = 12, .x = 12}, \
    {.tile = LINE_V_R_LOGO1, .y = 11, .x = 12},

#define LINESECTION_SEMIFINAL_TOP_RIGHT \
    {.tile = LINE_V_L,           .y =  8, .x = 17}, \
    {.tile = LINE_V_L,           .y =  9, .x = 17}, \
    {.tile = LINE_V_L,           .y = 10, .x = 17}, \
    {.tile = LINE_V_L_HALF_LOGO, .y = 11, .x = 17},

#define LINESECTION_SEMIFINAL_BOTTOM_RIGHT \
    {.tile = LINE_V_L_LOGO4, .y = 14, .x = 17}, \
    {.tile = LINE_V_L_LOGO3, .y = 13, .x = 17}, \
    {.tile = LINE_V_L_LOGO2, .y = 12, .x = 17}, \
    {.tile = LINE_V_L_LOGO1, .y = 11, .x = 17},

#define LINESECTION_FINAL_LEFT \
    {.tile = LINE_H_LOGO1, .y = 11, .x = 13}, \
    {.tile = LINE_H_LOGO2, .y = 11, .x = 14},

#define LINESECTION_FINAL_RIGHT \
    {.tile = LINE_H_LOGO4, .y = 11, .x = 16}, \
    {.tile = LINE_H_LOGO3, .y = 11, .x = 15},


static const struct TourneyTreeLineSection sLineSectionTrainer1Round1[] =
{
    LINESECTION_ROUND1_TRAINER1(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer1Round2[] =
{
    LINESECTION_ROUND1_TRAINER1(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH1(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer1Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER1(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH1(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer1Final[] =
{
    LINESECTION_ROUND1_TRAINER1(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH1(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer9Round1[] =
{
    LINESECTION_ROUND1_TRAINER9(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer9Round2[] =
{
    LINESECTION_ROUND1_TRAINER9(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH1(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer9Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER9(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH1(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer9Final[] =
{
    LINESECTION_ROUND1_TRAINER9(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH1(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer13Round1[] =
{
    LINESECTION_ROUND1_TRAINER13(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer13Round2[] =
{
    LINESECTION_ROUND1_TRAINER13(LINE_H)
    LINESECTION_ROUND2_MATCH2(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer13Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER13(LINE_H)
    LINESECTION_ROUND2_MATCH2(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer13Final[] =
{
    LINESECTION_ROUND1_TRAINER13(LINE_H)
    LINESECTION_ROUND2_MATCH2(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer5Round1[] =
{
    LINESECTION_ROUND1_TRAINER5(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer5Round2[] =
{
    LINESECTION_ROUND1_TRAINER5(LINE_H)
    LINESECTION_ROUND2_MATCH2(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer5Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER5(LINE_H)
    LINESECTION_ROUND2_MATCH2(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer5Final[] =
{
    LINESECTION_ROUND1_TRAINER5(LINE_H)
    LINESECTION_ROUND2_MATCH2(LINE_CORNER_R)
    LINESECTION_SEMIFINAL_TOP_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer8Round1[] =
{
    LINESECTION_ROUND1_TRAINER8(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer8Round2[] =
{
    LINESECTION_ROUND1_TRAINER8(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH3(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer8Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER8(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH3(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer8Final[] =
{
    LINESECTION_ROUND1_TRAINER8(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH3(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer16Round1[] =
{
    LINESECTION_ROUND1_TRAINER16(LINE_CORNER_R_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer16Round2[] =
{
    LINESECTION_ROUND1_TRAINER16(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH3(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer16Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER16(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH3(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer16Final[] =
{
    LINESECTION_ROUND1_TRAINER16(LINE_CORNER_R)
    LINESECTION_ROUND2_MATCH3(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer12Round1[] =
{
    LINESECTION_ROUND1_TRAINER12(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer12Round2[] =
{
    LINESECTION_ROUND1_TRAINER12(LINE_H)
    LINESECTION_ROUND2_MATCH4(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer12Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER12(LINE_H)
    LINESECTION_ROUND2_MATCH4(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer12Final[] =
{
    LINESECTION_ROUND1_TRAINER12(LINE_H)
    LINESECTION_ROUND2_MATCH4(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer4Round1[] =
{
    LINESECTION_ROUND1_TRAINER4(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer4Round2[] =
{
    LINESECTION_ROUND1_TRAINER4(LINE_H)
    LINESECTION_ROUND2_MATCH4(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer4Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER4(LINE_H)
    LINESECTION_ROUND2_MATCH4(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer4Final[] =
{
    LINESECTION_ROUND1_TRAINER4(LINE_H)
    LINESECTION_ROUND2_MATCH4(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_LEFT
    LINESECTION_FINAL_LEFT
};

static const struct TourneyTreeLineSection sLineSectionTrainer3Round1[] =
{
    LINESECTION_ROUND1_TRAINER3(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer3Round2[] =
{
    LINESECTION_ROUND1_TRAINER3(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH5(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer3Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER3(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH5(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer3Final[] =
{
    LINESECTION_ROUND1_TRAINER3(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH5(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer11Round1[] =
{
    LINESECTION_ROUND1_TRAINER11(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer11Round2[] =
{
    LINESECTION_ROUND1_TRAINER11(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH5(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer11Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER11(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH5(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer11Final[] =
{
    LINESECTION_ROUND1_TRAINER11(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH5(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer15Round1[] =
{
    LINESECTION_ROUND1_TRAINER15(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer15Round2[] =
{
    LINESECTION_ROUND1_TRAINER15(LINE_H)
    LINESECTION_ROUND2_MATCH6(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer15Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER15(LINE_H)
    LINESECTION_ROUND2_MATCH6(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer15Final[] =
{
    LINESECTION_ROUND1_TRAINER15(LINE_H)
    LINESECTION_ROUND2_MATCH6(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer7Round1[] =
{
    LINESECTION_ROUND1_TRAINER7(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer7Round2[] =
{
    LINESECTION_ROUND1_TRAINER7(LINE_H)
    LINESECTION_ROUND2_MATCH6(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer7Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER7(LINE_H)
    LINESECTION_ROUND2_MATCH6(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer7Final[] =
{
    LINESECTION_ROUND1_TRAINER7(LINE_H)
    LINESECTION_ROUND2_MATCH6(LINE_CORNER_L)
    LINESECTION_SEMIFINAL_TOP_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer6Round1[] =
{
    LINESECTION_ROUND1_TRAINER6(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer6Round2[] =
{
    LINESECTION_ROUND1_TRAINER6(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH7(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer6Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER6(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH7(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer6Final[] =
{
    LINESECTION_ROUND1_TRAINER6(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH7(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer14Round1[] =
{
    LINESECTION_ROUND1_TRAINER14(LINE_CORNER_L_HALF)
};

static const struct TourneyTreeLineSection sLineSectionTrainer14Round2[] =
{
    LINESECTION_ROUND1_TRAINER14(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH7(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer14Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER14(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH7(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer14Final[] =
{
    LINESECTION_ROUND1_TRAINER14(LINE_CORNER_L)
    LINESECTION_ROUND2_MATCH7(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer10Round1[] =
{
    LINESECTION_ROUND1_TRAINER10(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer10Round2[] =
{
    LINESECTION_ROUND1_TRAINER10(LINE_H)
    LINESECTION_ROUND2_MATCH8(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer10Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER10(LINE_H)
    LINESECTION_ROUND2_MATCH8(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer10Final[] =
{
    LINESECTION_ROUND1_TRAINER10(LINE_H)
    LINESECTION_ROUND2_MATCH8(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer2Round1[] =
{
    LINESECTION_ROUND1_TRAINER2(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer2Round2[] =
{
    LINESECTION_ROUND1_TRAINER2(LINE_H)
    LINESECTION_ROUND2_MATCH8(LINE_H)
};

static const struct TourneyTreeLineSection sLineSectionTrainer2Semifinal[] =
{
    LINESECTION_ROUND1_TRAINER2(LINE_H)
    LINESECTION_ROUND2_MATCH8(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
};

static const struct TourneyTreeLineSection sLineSectionTrainer2Final[] =
{
    LINESECTION_ROUND1_TRAINER2(LINE_H)
    LINESECTION_ROUND2_MATCH8(LINE_H)
    LINESECTION_SEMIFINAL_BOTTOM_RIGHT
    LINESECTION_FINAL_RIGHT
};

static const struct TourneyTreeLineSection *const sTourneyTreeLineSections[DOME_TOURNAMENT_TRAINERS_COUNT][DOME_ROUNDS_COUNT] =
{
    {sLineSectionTrainer1Round1,  sLineSectionTrainer1Round2,  sLineSectionTrainer1Semifinal,  sLineSectionTrainer1Final},
    {sLineSectionTrainer2Round1,  sLineSectionTrainer2Round2,  sLineSectionTrainer2Semifinal,  sLineSectionTrainer2Final},
    {sLineSectionTrainer3Round1,  sLineSectionTrainer3Round2,  sLineSectionTrainer3Semifinal,  sLineSectionTrainer3Final},
    {sLineSectionTrainer4Round1,  sLineSectionTrainer4Round2,  sLineSectionTrainer4Semifinal,  sLineSectionTrainer4Final},
    {sLineSectionTrainer5Round1,  sLineSectionTrainer5Round2,  sLineSectionTrainer5Semifinal,  sLineSectionTrainer5Final},
    {sLineSectionTrainer6Round1,  sLineSectionTrainer6Round2,  sLineSectionTrainer6Semifinal,  sLineSectionTrainer6Final},
    {sLineSectionTrainer7Round1,  sLineSectionTrainer7Round2,  sLineSectionTrainer7Semifinal,  sLineSectionTrainer7Final},
    {sLineSectionTrainer8Round1,  sLineSectionTrainer8Round2,  sLineSectionTrainer8Semifinal,  sLineSectionTrainer8Final},
    {sLineSectionTrainer9Round1,  sLineSectionTrainer9Round2,  sLineSectionTrainer9Semifinal,  sLineSectionTrainer9Final},
    {sLineSectionTrainer10Round1, sLineSectionTrainer10Round2, sLineSectionTrainer10Semifinal, sLineSectionTrainer10Final},
    {sLineSectionTrainer11Round1, sLineSectionTrainer11Round2, sLineSectionTrainer11Semifinal, sLineSectionTrainer11Final},
    {sLineSectionTrainer12Round1, sLineSectionTrainer12Round2, sLineSectionTrainer12Semifinal, sLineSectionTrainer12Final},
    {sLineSectionTrainer13Round1, sLineSectionTrainer13Round2, sLineSectionTrainer13Semifinal, sLineSectionTrainer13Final},
    {sLineSectionTrainer14Round1, sLineSectionTrainer14Round2, sLineSectionTrainer14Semifinal, sLineSectionTrainer14Final},
    {sLineSectionTrainer15Round1, sLineSectionTrainer15Round2, sLineSectionTrainer15Semifinal, sLineSectionTrainer15Final},
    {sLineSectionTrainer16Round1, sLineSectionTrainer16Round2, sLineSectionTrainer16Semifinal, sLineSectionTrainer16Final},
};

static const u8 sTourneyTreeLineSectionArrayCounts[DOME_TOURNAMENT_TRAINERS_COUNT][DOME_ROUNDS_COUNT] =
{
    {ARRAY_COUNT(sLineSectionTrainer1Round1),  ARRAY_COUNT(sLineSectionTrainer1Round2),  ARRAY_COUNT(sLineSectionTrainer1Semifinal),  ARRAY_COUNT(sLineSectionTrainer1Final)},
    {ARRAY_COUNT(sLineSectionTrainer2Round1),  ARRAY_COUNT(sLineSectionTrainer2Round2),  ARRAY_COUNT(sLineSectionTrainer2Semifinal),  ARRAY_COUNT(sLineSectionTrainer2Final)},
    {ARRAY_COUNT(sLineSectionTrainer3Round1),  ARRAY_COUNT(sLineSectionTrainer3Round2),  ARRAY_COUNT(sLineSectionTrainer3Semifinal),  ARRAY_COUNT(sLineSectionTrainer3Final)},
    {ARRAY_COUNT(sLineSectionTrainer4Round1),  ARRAY_COUNT(sLineSectionTrainer4Round2),  ARRAY_COUNT(sLineSectionTrainer4Semifinal),  ARRAY_COUNT(sLineSectionTrainer4Final)},
    {ARRAY_COUNT(sLineSectionTrainer5Round1),  ARRAY_COUNT(sLineSectionTrainer5Round2),  ARRAY_COUNT(sLineSectionTrainer5Semifinal),  ARRAY_COUNT(sLineSectionTrainer5Final)},
    {ARRAY_COUNT(sLineSectionTrainer6Round1),  ARRAY_COUNT(sLineSectionTrainer6Round2),  ARRAY_COUNT(sLineSectionTrainer6Semifinal),  ARRAY_COUNT(sLineSectionTrainer6Final)},
    {ARRAY_COUNT(sLineSectionTrainer7Round1),  ARRAY_COUNT(sLineSectionTrainer7Round2),  ARRAY_COUNT(sLineSectionTrainer7Semifinal),  ARRAY_COUNT(sLineSectionTrainer7Final)},
    {ARRAY_COUNT(sLineSectionTrainer8Round1),  ARRAY_COUNT(sLineSectionTrainer8Round2),  ARRAY_COUNT(sLineSectionTrainer8Semifinal),  ARRAY_COUNT(sLineSectionTrainer8Final)},
    {ARRAY_COUNT(sLineSectionTrainer9Round1),  ARRAY_COUNT(sLineSectionTrainer9Round2),  ARRAY_COUNT(sLineSectionTrainer9Semifinal),  ARRAY_COUNT(sLineSectionTrainer9Final)},
    {ARRAY_COUNT(sLineSectionTrainer10Round1), ARRAY_COUNT(sLineSectionTrainer10Round2), ARRAY_COUNT(sLineSectionTrainer10Semifinal), ARRAY_COUNT(sLineSectionTrainer10Final)},
    {ARRAY_COUNT(sLineSectionTrainer11Round1), ARRAY_COUNT(sLineSectionTrainer11Round2), ARRAY_COUNT(sLineSectionTrainer11Semifinal), ARRAY_COUNT(sLineSectionTrainer11Final)},
    {ARRAY_COUNT(sLineSectionTrainer12Round1), ARRAY_COUNT(sLineSectionTrainer12Round2), ARRAY_COUNT(sLineSectionTrainer12Semifinal), ARRAY_COUNT(sLineSectionTrainer12Final)},
    {ARRAY_COUNT(sLineSectionTrainer13Round1), ARRAY_COUNT(sLineSectionTrainer13Round2), ARRAY_COUNT(sLineSectionTrainer13Semifinal), ARRAY_COUNT(sLineSectionTrainer13Final)},
    {ARRAY_COUNT(sLineSectionTrainer14Round1), ARRAY_COUNT(sLineSectionTrainer14Round2), ARRAY_COUNT(sLineSectionTrainer14Semifinal), ARRAY_COUNT(sLineSectionTrainer14Final)},
    {ARRAY_COUNT(sLineSectionTrainer15Round1), ARRAY_COUNT(sLineSectionTrainer15Round2), ARRAY_COUNT(sLineSectionTrainer15Semifinal), ARRAY_COUNT(sLineSectionTrainer15Final)},
    {ARRAY_COUNT(sLineSectionTrainer16Round1), ARRAY_COUNT(sLineSectionTrainer16Round2), ARRAY_COUNT(sLineSectionTrainer16Semifinal), ARRAY_COUNT(sLineSectionTrainer16Final)},
};

void CallBattleDomeFunction(void)
{
    sBattleDomeFunctions[gSpecialVar_0x8004]();
}

static void InitDomeChallenge(void)
{
    enum FrontierLevelMode lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    gSaveBlock2Ptr->frontier.challengeStatus = 0;
    gSaveBlock2Ptr->frontier.curChallengeBattleNum = 0;
    gSaveBlock2Ptr->frontier.challengePaused = FALSE;
    gSaveBlock2Ptr->frontier.disableRecordBattle = FALSE;
    if (!(gSaveBlock2Ptr->frontier.winStreakActiveFlags & sWinStreakFlags[battleMode][lvlMode]))
        gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode] = 0;

    SetDynamicWarp(0, gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum, WARP_ID_NONE);
    TRAINER_BATTLE_PARAM.opponentA = 0;
}

static void GetDomeData(void)
{
    enum FrontierLevelMode lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    switch (gSpecialVar_0x8005)
    {
    case DOME_DATA_WIN_STREAK:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode];
        break;
    case DOME_DATA_WIN_STREAK_ACTIVE:
        gSpecialVar_Result = ((gSaveBlock2Ptr->frontier.winStreakActiveFlags & sWinStreakFlags[battleMode][lvlMode]) != 0);
        break;
    case DOME_DATA_ATTEMPTED_SINGLES_50:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeAttemptedSingles50;
        break;
    case DOME_DATA_ATTEMPTED_SINGLES_OPEN:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeAttemptedSinglesOpen;
        break;
    case DOME_DATA_HAS_WON_SINGLES_50:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeHasWonSingles50;
        break;
    case DOME_DATA_HAS_WON_SINGLES_OPEN:
        gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeHasWonSinglesOpen;
        break;
    case DOME_DATA_ATTEMPTED_CHALLENGE:
        if (VarGet(VAR_FRONTIER_BATTLE_MODE) == FRONTIER_MODE_DOUBLES)
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeAttemptedDoublesOpen;
            else
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeAttemptedDoubles50;
        }
        else
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeAttemptedSinglesOpen;
            else
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeAttemptedSingles50;
        }
        break;
    case DOME_DATA_HAS_WON_CHALLENGE:
        if (VarGet(VAR_FRONTIER_BATTLE_MODE) == FRONTIER_MODE_DOUBLES)
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeHasWonDoublesOpen;
            else
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeHasWonDoubles50;
        }
        else
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeHasWonSinglesOpen;
            else
                gSpecialVar_Result = gSaveBlock2Ptr->frontier.domeHasWonSingles50;
        }
        break;
    case DOME_DATA_SELECTED_MONS:
        ClearSelectedPartyOrder();
        gSelectedOrderFromParty[0] = gSaveBlock2Ptr->frontier.selectedPartyMons[3];
        gSelectedOrderFromParty[1] = gSaveBlock2Ptr->frontier.selectedPartyMons[3] >> 8;
        break;
    case DOME_DATA_PREV_TOURNEY_TYPE:
        gSpecialVar_Result = (gSaveBlock2Ptr->frontier.domeLvlMode * 2) - 3 + gSaveBlock2Ptr->frontier.domeBattleMode;
        break;
    }
}

static void SetDomeData(void)
{
    enum FrontierLevelMode lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    switch (gSpecialVar_0x8005)
    {
    case DOME_DATA_WIN_STREAK:
        gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode] = gSpecialVar_0x8006;
        break;
    case DOME_DATA_WIN_STREAK_ACTIVE:
        if (gSpecialVar_0x8006)
            gSaveBlock2Ptr->frontier.winStreakActiveFlags |= sWinStreakFlags[battleMode][lvlMode];
        else
            gSaveBlock2Ptr->frontier.winStreakActiveFlags &= sWinStreakMasks[battleMode][lvlMode];
        break;
    case DOME_DATA_ATTEMPTED_SINGLES_50:
        gSaveBlock2Ptr->frontier.domeAttemptedSingles50 = gSpecialVar_0x8006;
        break;
    case DOME_DATA_ATTEMPTED_SINGLES_OPEN:
        gSaveBlock2Ptr->frontier.domeAttemptedSinglesOpen = gSpecialVar_0x8006;
        break;
    case DOME_DATA_HAS_WON_SINGLES_50:
        gSaveBlock2Ptr->frontier.domeHasWonSingles50 = gSpecialVar_0x8006;
        break;
    case DOME_DATA_HAS_WON_SINGLES_OPEN:
        gSaveBlock2Ptr->frontier.domeHasWonSinglesOpen = gSpecialVar_0x8006;
        break;
    case DOME_DATA_ATTEMPTED_CHALLENGE:
        if (VarGet(VAR_FRONTIER_BATTLE_MODE) == FRONTIER_MODE_DOUBLES)
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSaveBlock2Ptr->frontier.domeAttemptedDoublesOpen = gSpecialVar_0x8006;
            else
                gSaveBlock2Ptr->frontier.domeAttemptedDoubles50 = gSpecialVar_0x8006;
        }
        else
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSaveBlock2Ptr->frontier.domeAttemptedSinglesOpen = gSpecialVar_0x8006;
            else
                gSaveBlock2Ptr->frontier.domeAttemptedSingles50 = gSpecialVar_0x8006;
        }
        break;
    case DOME_DATA_HAS_WON_CHALLENGE:
        if (VarGet(VAR_FRONTIER_BATTLE_MODE) == FRONTIER_MODE_DOUBLES)
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSaveBlock2Ptr->frontier.domeHasWonDoublesOpen = gSpecialVar_0x8006;
            else
                gSaveBlock2Ptr->frontier.domeHasWonDoubles50 = gSpecialVar_0x8006;
        }
        else
        {
            if (lvlMode != FRONTIER_LVL_50)
                gSaveBlock2Ptr->frontier.domeHasWonSinglesOpen = gSpecialVar_0x8006;
            else
                gSaveBlock2Ptr->frontier.domeHasWonSingles50 = gSpecialVar_0x8006;
        }
        break;
    case DOME_DATA_SELECTED_MONS:
        gSaveBlock2Ptr->frontier.selectedPartyMons[3] = T1_READ_16(gSelectedOrderFromParty);
        break;
    }
}

/* Private call state; no saved layout or service activation changes. */
struct ADMG_DomeRosterFrame
{
    struct SaveBlock2 *save;
    u64 generation;
    const struct BattleFrontierTrainer *trainers, *originalTrainers, *ownedTrainers;
    const struct TrainerMon *mons, *originalMons, *ownedMons;
    u16 selected[FRONTIER_PARTY_SIZE];
    u32 level;
    u16 *scores;
    int *stats;
    bool8 random, ownsTables, ownsLevel;
    u8 previousLevel, previousDomeLevel, previousBattleMode;
    __typeof__(((struct SaveBlock2 *)0)->frontier.domeTrainers) previousTrainers;
    __typeof__(((struct SaveBlock2 *)0)->frontier.domeMonIds) previousMons;
    __typeof__(((struct SaveBlock2 *)0)->frontier.domePlayerPartyData) previousPlayer;
    __typeof__(((struct SaveBlock2 *)0)->frontier.domeWinningMoves) previousMoves;
};

static bool8 ADMG_DomeRosterCurrent(struct ADMG_DomeRosterFrame *frame)
{
    if (!ADMG_EndStateAccessCheck(frame->generation)) return FALSE;
    if (frame->save == NULL || gSaveBlock2Ptr != frame->save
        || frame->save->frontier.lvlMode != frame->level
        || gFacilityTrainers != frame->trainers || gFacilityTrainerMons != frame->mons)
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    if (!frame->random)
        for (u32 i = 0; i < ARRAY_COUNT(frame->selected); i++)
            if (frame->save->frontier.selectedPartyMons[i] != frame->selected[i])
            { ADMG_StateAccessRejected(i, __func__); return FALSE; }
    return TRUE;
}

static bool8 ADMG_TryPrepareDomeRoster(struct ADMG_DomeRosterFrame *frame, bool8 random)
{
    frame->save = gSaveBlock2Ptr;
    frame->generation = ADMG_BeginStateAccessCheck();
    if (frame->save == NULL) { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    frame->random = random;
    frame->level = frame->previousLevel = frame->save->frontier.lvlMode;
    frame->originalTrainers = frame->trainers = gFacilityTrainers;
    frame->originalMons = frame->mons = gFacilityTrainerMons;
    if (!random)
        for (u32 i = 0; i < ARRAY_COUNT(frame->selected); i++)
        {
            u32 position = frame->save->frontier.selectedPartyMons[i];
            if (position == 0 || position > ARRAY_COUNT(gParties[B_TRAINER_PLAYER]))
            { ADMG_StateAccessRejected(position, __func__); return FALSE; }
            frame->selected[i] = position;
        }
    // Native mon drawing publishes its selected tables before resource access.
    // The previously published pointers are not entry prerequisites here.
    if (!ADMG_DomeRosterCurrent(frame)) return FALSE;
    frame->previousDomeLevel = frame->save->frontier.domeLvlMode;
    frame->previousBattleMode = frame->save->frontier.domeBattleMode;
    memcpy(frame->previousTrainers, frame->save->frontier.domeTrainers, sizeof(frame->previousTrainers));
    memcpy(frame->previousMons, frame->save->frontier.domeMonIds, sizeof(frame->previousMons));
    if (random)
        memcpy(frame->previousMoves, frame->save->frontier.domeWinningMoves, sizeof(frame->previousMoves));
    else
        memcpy(frame->previousPlayer, frame->save->frontier.domePlayerPartyData, sizeof(frame->previousPlayer));
    return ADMG_DomeRosterCurrent(frame);
}

static void ADMG_ReleaseDomeRoster(struct ADMG_DomeRosterFrame *frame)
{
    u16 *scores = frame->scores;
    int *stats = frame->stats;
    frame->scores = NULL; frame->stats = NULL;
    if (scores != NULL) Free(scores);
    if (stats != NULL) Free(stats);
}

static void ADMG_RestoreDomeRoster(struct ADMG_DomeRosterFrame *frame)
{
    memcpy(frame->save->frontier.domeTrainers, frame->previousTrainers, sizeof(frame->previousTrainers));
    memcpy(frame->save->frontier.domeMonIds, frame->previousMons, sizeof(frame->previousMons));
    frame->save->frontier.domeLvlMode = frame->previousDomeLevel;
    frame->save->frontier.domeBattleMode = frame->previousBattleMode;
    if (frame->random)
        memcpy(frame->save->frontier.domeWinningMoves, frame->previousMoves, sizeof(frame->previousMoves));
    else
        memcpy(frame->save->frontier.domePlayerPartyData, frame->previousPlayer, sizeof(frame->previousPlayer));
    if (frame->ownsLevel && frame->save->frontier.lvlMode == frame->level)
        frame->save->frontier.lvlMode = frame->previousLevel;
    if (frame->ownsTables)
    {
        if (gFacilityTrainers == frame->ownedTrainers) gFacilityTrainers = frame->originalTrainers;
        if (gFacilityTrainerMons == frame->ownedMons) gFacilityTrainerMons = frame->originalMons;
    }
}

static const struct TrainerMon *ADMG_DomeRosterMonRow(struct ADMG_DomeRosterFrame *frame, u32 identity)
{
    if (!ADMG_DomeRosterCurrent(frame)) return NULL;
    u32 count;
    const struct TrainerMon *row;
    if (!ADMG_TryGetFacilityMonTableCount(frame->mons, &count)
        || !ADMG_TryGetFacilityMonRowFromTable(frame->mons, count, identity, &row) || row == NULL)
    {
        if (ADMG_EndStateAccessCheck(frame->generation)) ADMG_StateAccessRejected(identity, __func__);
        return NULL;
    }
    return ADMG_DomeRosterCurrent(frame) ? row : NULL;
}

static u32 ADMG_DomeRosterMonId(struct ADMG_DomeRosterFrame *frame, u32 identity)
{
    return ADMG_DomeRosterMonRow(frame, identity) != NULL ? identity : 0;
}

static u32 ADMG_DomeRosterTrainerId(struct ADMG_DomeRosterFrame *frame, u32 identity)
{
    const struct BattleFrontierTrainer *row;
    const struct BattleFrontierTrainer *trainers;
    const struct TrainerMon *mons;
    u32 count;
    if (identity > 0x3FF
        || !ADMG_TrySelectFacilityTrainerTables(frame->level == FRONTIER_LVL_TENT, &trainers, &mons, &count)
        || !ADMG_TryGetFacilityTrainerRowFromTable(trainers, count, identity, &row) || row == NULL)
    {
        if (ADMG_EndStateAccessCheck(frame->generation)) ADMG_StateAccessRejected(identity, __func__);
        return 0;
    }
    return identity;
}

static bool8 ADMG_TryDomeRosterPublication(struct ADMG_DomeRosterFrame *frame)
{
    if (!ADMG_DomeRosterCurrent(frame)) return FALSE;
    const struct BattleFrontierTrainer *trainers;
    const struct TrainerMon *mons;
    u32 count;
    if (!ADMG_TrySelectFacilityTrainerTables(frame->level == FRONTIER_LVL_TENT, &trainers, &mons, &count))
    {
        if (ADMG_EndStateAccessCheck(frame->generation)) ADMG_StateAccessRejected(0, __func__);
        return FALSE;
    }
    if (!ADMG_DomeRosterCurrent(frame)) return FALSE;
    frame->ownedTrainers = trainers; frame->ownedMons = mons;
    frame->ownsTables = TRUE;
    return TRUE;
}

static bool8 ADMG_AcceptDomeRosterPublication(struct ADMG_DomeRosterFrame *frame)
{
    if (!ADMG_EndStateAccessCheck(frame->generation)) return FALSE;
    if (gFacilityTrainers != frame->ownedTrainers || gFacilityTrainerMons != frame->ownedMons)
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    frame->trainers = gFacilityTrainers; frame->mons = gFacilityTrainerMons;
    return ADMG_DomeRosterCurrent(frame);
}

static u32 ADMG_DomeRosterLevel(struct ADMG_DomeRosterFrame *frame)
{
    if (!ADMG_TryDomeRosterPublication(frame)) return 0;
    u32 level = SetFacilityPtrsGetLevel();
    if (!ADMG_AcceptDomeRosterPublication(frame)) return 0;
    return level;
}

static u32 ADMG_DomeRosterDrawMon(struct ADMG_DomeRosterFrame *frame, u16 trainer)
{
    if (!ADMG_TryDomeRosterPublication(frame)) return 0xFFFF;
    u32 identity = GetRandomFrontierMonFromSet(trainer);
    if (!ADMG_AcceptDomeRosterPublication(frame)) return 0xFFFF;
    return identity;
}

static u32 ADMG_DomeRosterMode(u32 value)
{
    if (value >= UCHAR_MAX)
    { ADMG_StateAccessRejected(value, __func__); return 0; }
    return value;
}

static void ADMG_DomeRosterRound(struct ADMG_DomeRosterFrame *frame, u8 round)
{
    // The original winning-move provider may publish the level-selected tables.
    if (!ADMG_TryDomeRosterPublication(frame)) return;
    DecideRoundWinners(round);
    if (ADMG_EndStateAccessCheck(frame->generation)
        && gFacilityTrainers == frame->ownedTrainers && gFacilityTrainerMons == frame->ownedMons)
    { frame->trainers = gFacilityTrainers; frame->mons = gFacilityTrainerMons; }
}

#define ADMG_DOME_ROSTER_VALUE(frame, expression) \
    ({ __typeof__(expression) admg_value = (expression); \
       if (!ADMG_DomeRosterCurrent(frame)) goto ADMG_roster_reject; admg_value; })
#define ADMG_DOME_ROSTER_ACTION(frame, expression) \
    do { expression; if (!ADMG_DomeRosterCurrent(frame)) goto ADMG_roster_reject; } while (0)

static bool8 ADMG_RunInitDomeTrainers(struct ADMG_DomeRosterFrame *frame)
{
    int i, j, k;
    int monLevel;
    enum Species species[FRONTIER_PARTY_SIZE];
    int monTypesBits, monTypesCount;
    int trainerId;
    int monId;
    u16 *rankingScores;
    int *statValues;
    u8 ivs = 0;

    species[0] = SPECIES_NONE;
    species[1] = SPECIES_NONE;
    species[2] = SPECIES_NONE;
    rankingScores = frame->scores = AllocZeroed(sizeof(u16) * DOME_TOURNAMENT_TRAINERS_COUNT);
    if (rankingScores == NULL) { ADMG_StateAccessRejected(0, __func__); goto ADMG_roster_reject; }
    if (!ADMG_DomeRosterCurrent(frame)) goto ADMG_roster_reject;
    statValues = frame->stats = AllocZeroed(sizeof(int) * NUM_STATS);
    if (statValues == NULL) { ADMG_StateAccessRejected(0, __func__); goto ADMG_roster_reject; }
    if (!ADMG_DomeRosterCurrent(frame)) goto ADMG_roster_reject;

    frame->save->frontier.domeLvlMode = frame->save->frontier.lvlMode + 1;
    frame->save->frontier.domeBattleMode = ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMode(ADMG_DOME_ROSTER_VALUE(frame, VarGet(VAR_FRONTIER_BATTLE_MODE)))) + 1;
    DOME_TRAINERS[0].trainerId = TRAINER_PLAYER;
    DOME_TRAINERS[0].isEliminated = FALSE;
    DOME_TRAINERS[0].eliminatedAt = 0;
    DOME_TRAINERS[0].forfeited = FALSE;

    // Store the data used to display party information on the player's tourney page
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        DOME_MONS[0][i] = ADMG_DOME_ROSTER_VALUE(frame, GetMonData(&gParties[B_TRAINER_PLAYER][frame->selected[i] - 1], MON_DATA_SPECIES));
        for (j = 0; j < MAX_MON_MOVES; j++)
            frame->save->frontier.domePlayerPartyData[i].moves[j] = ADMG_DOME_ROSTER_VALUE(frame, GetMonData(&gParties[B_TRAINER_PLAYER][frame->selected[i] - 1], MON_DATA_MOVE1 + j));
        for (j = 0; j < NUM_STATS; j++)
            frame->save->frontier.domePlayerPartyData[i].evs[j] = ADMG_DOME_ROSTER_VALUE(frame, GetMonData(&gParties[B_TRAINER_PLAYER][frame->selected[i] - 1], MON_DATA_HP_EV + j));

        frame->save->frontier.domePlayerPartyData[i].nature = ADMG_DOME_ROSTER_VALUE(frame, GetNature(&gParties[B_TRAINER_PLAYER][frame->selected[i] - 1]));
    }

    // Populate the tourney roster with random frontier trainers (dependent on streak)
    for (i = 1; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        // Choose trainer. First 5/16 trainers are easier than the rest
        if (i > 5)
        {
            do
            {
                trainerId = ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterTrainerId(frame, ADMG_DOME_ROSTER_VALUE(frame, GetRandomScaledFrontierTrainerId(ADMG_DOME_ROSTER_VALUE(frame, GetCurrentFacilityWinStreak()), 0))));
                for (j = 1; j < i; j++)
                {
                    if (DOME_TRAINERS[j].trainerId == trainerId)
                        break;
                }
            } while (j != i);
            DOME_TRAINERS[i].trainerId = trainerId;
        }
        else
        {
            do
            {
                trainerId = ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterTrainerId(frame, ADMG_DOME_ROSTER_VALUE(frame, GetRandomScaledFrontierTrainerId(ADMG_DOME_ROSTER_VALUE(frame, GetCurrentFacilityWinStreak()) + 1, 0))));
                for (j = 1; j < i; j++)
                {
                    if (DOME_TRAINERS[j].trainerId == trainerId)
                        break;
                }
            } while (j != i);
            DOME_TRAINERS[i].trainerId = trainerId;
        }

        // Choose party
        for (j = 0; j < FRONTIER_PARTY_SIZE; j++)
        {
            do
            {
                monId = ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonId(frame, ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterDrawMon(frame, trainerId))));
                for (k = 0; k < j; k++)
                {
                    // Make sure the mon is valid.
                    int alreadySelectedMonId = DOME_MONS[i][k];
                    if (alreadySelectedMonId == monId
                        || species[0] == (*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, monId))).species
                        || species[1] == (*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, monId))).species
                        || (*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, alreadySelectedMonId))).heldItem == (*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, monId))).heldItem)
                        break;
                }
            } while (k != j);

            DOME_MONS[i][j] = monId;
            species[j] = (*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, monId))).species;
        }

        DOME_TRAINERS[i].isEliminated = FALSE;
        DOME_TRAINERS[i].eliminatedAt = 0;
        DOME_TRAINERS[i].forfeited = FALSE;
    }

    // rankingScores is used to determine the seed (ranking) of the trainers
    // rankingScores[0] is for the player, rankingScores[1-15] are for the opponent trainers

    // Calculate player's ranking score
    monTypesBits = 0;
    rankingScores[0] = 0;
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        // trainerId var re-used here as index of selected mons
        trainerId = frame->selected[i] - 1;
        rankingScores[0] += ADMG_DOME_ROSTER_VALUE(frame, GetMonData(&gParties[B_TRAINER_PLAYER][trainerId], MON_DATA_ATK));
        rankingScores[0] += ADMG_DOME_ROSTER_VALUE(frame, GetMonData(&gParties[B_TRAINER_PLAYER][trainerId], MON_DATA_DEF));
        rankingScores[0] += ADMG_DOME_ROSTER_VALUE(frame, GetMonData(&gParties[B_TRAINER_PLAYER][trainerId], MON_DATA_SPATK));
        rankingScores[0] += ADMG_DOME_ROSTER_VALUE(frame, GetMonData(&gParties[B_TRAINER_PLAYER][trainerId], MON_DATA_SPDEF));
        rankingScores[0] += ADMG_DOME_ROSTER_VALUE(frame, GetMonData(&gParties[B_TRAINER_PLAYER][trainerId], MON_DATA_SPEED));
        rankingScores[0] += ADMG_DOME_ROSTER_VALUE(frame, GetMonData(&gParties[B_TRAINER_PLAYER][trainerId], MON_DATA_MAX_HP));
        monTypesBits |= 1u << ADMG_DOME_ROSTER_VALUE(frame, GetSpeciesType(ADMG_DOME_ROSTER_VALUE(frame, GetMonData(&gParties[B_TRAINER_PLAYER][trainerId], MON_DATA_SPECIES)), 0));
        monTypesBits |= 1u << ADMG_DOME_ROSTER_VALUE(frame, GetSpeciesType(ADMG_DOME_ROSTER_VALUE(frame, GetMonData(&gParties[B_TRAINER_PLAYER][trainerId], MON_DATA_SPECIES)), 1));
    }

    // Count the number of types in the players party, to factor into the ranking
    for (monTypesCount = 0, j = 0; j < 32; j++)
    {
        if (monTypesBits & 1)
            monTypesCount++;
        monTypesBits >>= 1;
    }

    monLevel = ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterLevel(frame));
    rankingScores[0] += (monTypesCount * monLevel) / 20;

    // Calculate rankingScores for the opponent trainers
    for (i = 1; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        monTypesBits = 0;
        rankingScores[i] = 0;
        ivs = ADMG_DOME_ROSTER_VALUE(frame, GetDomeTrainerMonIvs(DOME_TRAINERS[i].trainerId));
        for (j = 0; j < FRONTIER_PARTY_SIZE; j++)
        {
            ADMG_DOME_ROSTER_ACTION(frame, CalcDomeMonStats(&(*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, DOME_MONS[i][j]))),
                             monLevel, ivs, statValues));

            rankingScores[i] += statValues[STAT_ATK];
            rankingScores[i] += statValues[STAT_DEF];
            rankingScores[i] += statValues[STAT_SPATK];
            rankingScores[i] += statValues[STAT_SPDEF];
            rankingScores[i] += statValues[STAT_SPEED];
            rankingScores[i] += statValues[STAT_HP];
            monTypesBits |= 1u << ADMG_DOME_ROSTER_VALUE(frame, GetSpeciesType((*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, DOME_MONS[i][j]))).species, 0));
            monTypesBits |= 1u << ADMG_DOME_ROSTER_VALUE(frame, GetSpeciesType((*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, DOME_MONS[i][j]))).species, 1));
        }

        for (monTypesCount = 0, j = 0; j < 32; j++)
        {
            if (monTypesBits & 1)
                monTypesCount++;
            monTypesBits >>= 1;
        }
        rankingScores[i] += (monTypesCount * monLevel) / 20;
    }

    // Seed tourney trainers according to their ranking
    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT - 1; i++)
    {
        for (j = i + 1; j < DOME_TOURNAMENT_TRAINERS_COUNT; j++)
        {
            if (rankingScores[i] < rankingScores[j])
            {
                ADMG_DOME_ROSTER_ACTION(frame, SwapDomeTrainers(i, j, rankingScores));
            }
            else
            {
                if (rankingScores[i] == rankingScores[j])
                {
                    if (DOME_TRAINERS[j].trainerId == TRAINER_PLAYER)
                        ADMG_DOME_ROSTER_ACTION(frame, SwapDomeTrainers(i, j, rankingScores));
                    else if (DOME_TRAINERS[i].trainerId > DOME_TRAINERS[j].trainerId)
                        ADMG_DOME_ROSTER_ACTION(frame, SwapDomeTrainers(i, j, rankingScores));
                }
            }
        }
    }

    // Add Frontier Brain to the tourney if they should be fought at the end of it
    if (ADMG_DOME_ROSTER_VALUE(frame, GetFrontierBrainStatus()) != FRONTIER_BRAIN_NOT_READY)
    {
        u32 playerSlot;
        if (!ADMG_TryDomeTrainerSlot(TRAINER_PLAYER, &playerSlot)
            || !ADMG_DomeRosterCurrent(frame)) goto ADMG_roster_reject;
        i = playerSlot;

        if (sTrainerNamePositions[i][0] != TOURNEYWIN_NAMES_LEFT)
        {
            j = 0;
            DOME_TRAINERS[j].trainerId = TRAINER_FRONTIER_BRAIN;
        }
        else
        {
            j = 1;
            DOME_TRAINERS[j].trainerId = TRAINER_FRONTIER_BRAIN;
        }

        for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
            DOME_MONS[j][i] = ADMG_DOME_ROSTER_VALUE(frame, GetFrontierBrainMonSpecies(i));
    }

    ADMG_ReleaseDomeRoster(frame);
    if (!ADMG_DomeRosterCurrent(frame)) goto ADMG_roster_reject;
    return TRUE;
ADMG_roster_reject:
    return FALSE;
}

static void InitDomeTrainers(void)
{
    struct ADMG_DomeRosterFrame frame = {0};
    if (!ADMG_TryPrepareDomeRoster(&frame, FALSE)) return;
    bool8 completed = ADMG_RunInitDomeTrainers(&frame);
    ADMG_ReleaseDomeRoster(&frame);
    if (!completed || !ADMG_DomeRosterCurrent(&frame)) ADMG_RestoreDomeRoster(&frame);
}

#define CALC_STAT(base, statIndex)                                                          \
{                                                                                           \
    u8 baseStat = gSpeciesInfo[fmon->species].base;                                                 \
    stats[statIndex] = (((2 * baseStat + ivs + evs[statIndex] / 4) * level) / 100) + 5;     \
    stats[statIndex] = (u8) ModifyStatByNature(fmon->nature, stats[statIndex], statIndex);        \
}

static void CalcDomeMonStats(const struct TrainerMon *fmon, int level, u8 ivs, int *stats)
{
    int evs[NUM_STATS];

    for (enum Stat i = 0; i < NUM_STATS; i++)
    {
        if (fmon->ev != NULL)
            evs[i] = fmon->ev[i];
        else
            evs[i] = 0;
    }

    if (HasShedinjaHPHandling(fmon->species))
    {
        stats[STAT_HP] = 1;
    }
    else
    {
        int n = 2 * GetSpeciesBaseHP(fmon->species);
        stats[STAT_HP] = (((n + ivs + evs[STAT_HP] / 4) * level) / 100) + level + 10;
    }

    CALC_STAT(baseAttack, STAT_ATK);
    CALC_STAT(baseDefense, STAT_DEF);
    CALC_STAT(baseSpeed, STAT_SPEED);
    CALC_STAT(baseSpAttack, STAT_SPATK);
    CALC_STAT(baseSpDefense, STAT_SPDEF);
}

static void SwapDomeTrainers(int id1, int id2, u16 *statsArray)
{
    int i;
    u16 temp;
    if (gSaveBlock2Ptr == NULL || statsArray == NULL
        || (u32)id1 >= ARRAY_COUNT(DOME_TRAINERS) || (u32)id2 >= ARRAY_COUNT(DOME_TRAINERS))
    { ADMG_StateAccessRejected(id1, __func__); return; }

    SWAP(statsArray[id1], statsArray[id2], temp);
    SWAP(DOME_TRAINERS[id1].trainerId, DOME_TRAINERS[id2].trainerId, temp);

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
        SWAP(DOME_MONS[id1][i], DOME_MONS[id2][i], temp);
}

static void BufferDomeRoundText(void)
{
    StringCopy(gStringVar1, gRoundsStringTable[gSaveBlock2Ptr->frontier.curChallengeBattleNum]);
}

/* Dome names admit physical facility rows or the two native special roles. */
static bool8 ADMG_TryDomeNameIdentity(u32 identity, const struct BattleFrontierTrainer **trainer)
{
    if (trainer == NULL || gSaveBlock2Ptr == NULL
        || (identity >= FRONTIER_TRAINERS_COUNT && identity != TRAINER_PLAYER && identity != TRAINER_FRONTIER_BRAIN))
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    *trainer = NULL;
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    u64 generation = ADMG_BeginStateAccessCheck();
    if (identity < FRONTIER_TRAINERS_COUNT && !ADMG_TryGetFacilityTrainerRow(identity, trainer)) return FALSE;
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != save) { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    return TRUE;
}

static void BufferDomeOpponentName(void)
{
    if (gSaveBlock2Ptr == NULL || gSaveBlock2Ptr->frontier.curChallengeBattleNum >= DOME_ROUNDS_COUNT)
    { ADMG_StateAccessRejected(0, __func__); return; }
    const struct BattleFrontierTrainer *trainer;
    if (!ADMG_TryDomeNameIdentity(TRAINER_BATTLE_PARAM.opponentA, &trainer)) return;
    u64 generation = ADMG_BeginStateAccessCheck();
    u8 previousRound[sizeof(gStringVar1)], previousName[sizeof(gStringVar2)];
    memcpy(previousRound, gStringVar1, sizeof(previousRound));
    memcpy(previousName, gStringVar2, sizeof(previousName));

    StringCopy(gStringVar1, gRoundsStringTable[gSaveBlock2Ptr->frontier.curChallengeBattleNum]);
    if (ADMG_EndStateAccessCheck(generation)) CopyDomeTrainerName(gStringVar2, TRAINER_BATTLE_PARAM.opponentA);

    if (!ADMG_EndStateAccessCheck(generation))
    {
        memcpy(gStringVar1, previousRound, sizeof(previousRound));
        memcpy(gStringVar2, previousName, sizeof(previousName));
    }
}

/* Only the selected native roster/mon coordinates are borrowed here. */
static bool8 ADMG_DomePartyInputCurrent(struct SaveBlock2 *save, u64 generation,
    u16 identity, u32 slot, const u16 *mons)
{
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (save == NULL || gSaveBlock2Ptr != save || TRAINER_BATTLE_PARAM.opponentA != identity)
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    u32 selected;
    if (!ADMG_TryDomeOpponentSlot(identity, &selected) || !ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != save || TRAINER_BATTLE_PARAM.opponentA != identity || selected != slot)
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    if (mons != NULL)
        for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++)
            if (DOME_MONS[slot][i] != mons[i])
            { ADMG_StateAccessRejected(slot, __func__); return FALSE; }
    return TRUE;
}

static void ADMG_RunDomeOpponentParty(u16 identity, u32 slot, struct SaveBlock2 *save,
    u64 generation) __attribute__((noinline));
static void ADMG_RunDomeOpponentParty(u16 identity, u32 slot, struct SaveBlock2 *save,
    u64 generation)
{
    if (!ADMG_DomePartyInputCurrent(save, generation, identity, slot, NULL)) return;
    u16 mons[FRONTIER_PARTY_SIZE];
    memcpy(mons, DOME_MONS[slot], sizeof(mons));
    if (!ADMG_DomePartyInputCurrent(save, generation, identity, slot, mons)) return;
    struct Pokemon saved[2][PARTY_SIZE];
    memcpy(saved[0], gParties[B_TRAINER_OPPONENT_A], sizeof(saved[0]));
    memcpy(saved[1], gParties[B_TRAINER_OPPONENT_B], sizeof(saved[1]));
    u8 playerCount = gPartiesCount[B_TRAINER_PLAYER];
    u8 countA = gPartiesCount[B_TRAINER_OPPONENT_A], countB = gPartiesCount[B_TRAINER_OPPONENT_B];
    CalculatePlayerPartyCount();
    if (!ADMG_DomePartyInputCurrent(save, generation, identity, slot, mons)) goto ADMG_reject;
    CreateDomeOpponentMons(slot);
    if (ADMG_DomePartyInputCurrent(save, generation, identity, slot, mons)) return;
ADMG_reject:
    memcpy(gParties[B_TRAINER_OPPONENT_A], saved[0], sizeof(saved[0]));
    memcpy(gParties[B_TRAINER_OPPONENT_B], saved[1], sizeof(saved[1]));
    gPartiesCount[B_TRAINER_PLAYER] = playerCount;
    gPartiesCount[B_TRAINER_OPPONENT_A] = countA; gPartiesCount[B_TRAINER_OPPONENT_B] = countB;
}

static void InitDomeOpponentParty(void)
{
    u16 identity = TRAINER_BATTLE_PARAM.opponentA;
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    u64 generation = ADMG_BeginStateAccessCheck();
    u32 slot;
    if (!ADMG_TryDomeOpponentSlot(identity, &slot) || !ADMG_EndStateAccessCheck(generation)) return;
    if (gSaveBlock2Ptr != save || TRAINER_BATTLE_PARAM.opponentA != identity)
    { ADMG_StateAccessRejected(identity, __func__); return; }
    ADMG_RunDomeOpponentParty(identity, slot, save, generation);
}

/* A physical Dome slot is distinct from the first identity lookup match. */
static bool8 ADMG_TryDomeNpcAtSlot(u32 slot, u16 *identity)
{
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    u64 generation = ADMG_BeginStateAccessCheck();
    if (save == NULL || identity == NULL || slot >= ARRAY_COUNT(DOME_TRAINERS))
    { ADMG_StateAccessRejected(slot, __func__); return FALSE; }
    u16 actor = DOME_TRAINERS[slot].trainerId;
    u32 first;
    if (!ADMG_TryDomeOpponentSlot(actor, &first) || !ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != save || DOME_TRAINERS[slot].trainerId != actor)
    { ADMG_StateAccessRejected(actor, __func__); return FALSE; }
    *identity = actor;
    return TRUE;
}

static bool8 ADMG_TryDomeMonRowFromTable(u32 slot, u32 mon,
    const struct TrainerMon *table, const struct TrainerMon **row)
{
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    u64 generation = ADMG_BeginStateAccessCheck();
    u16 actor;
    if (row == NULL || mon >= ARRAY_COUNT(DOME_MONS[0]))
    { ADMG_StateAccessRejected(mon, __func__); return FALSE; }
    if (!ADMG_TryDomeNpcAtSlot(slot, &actor) || !ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != save)
    { ADMG_StateAccessRejected(slot, __func__); return FALSE; }
    u32 index = DOME_MONS[slot][mon], count;
    const struct TrainerMon *selected;
    if (!ADMG_TryGetFacilityMonTableCount(table, &count)
        || !ADMG_TryGetFacilityMonRowFromTable(table, count, index, &selected)
        || !ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != save || DOME_TRAINERS[slot].trainerId != actor
        || DOME_MONS[slot][mon] != index)
    { ADMG_StateAccessRejected(index, __func__); return FALSE; }
    *row = selected;
    return TRUE;
}

static bool8 ADMG_DomeMonSourceCurrent(struct SaveBlock2 *save, u64 generation,
    u32 slot, u32 mon, u16 actor, u16 index)
{
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (save == NULL || gSaveBlock2Ptr != save || slot >= ARRAY_COUNT(DOME_TRAINERS)
        || mon >= ARRAY_COUNT(DOME_MONS[0]) || DOME_TRAINERS[slot].trainerId != actor
        || DOME_MONS[slot][mon] != index)
    { ADMG_StateAccessRejected(slot, __func__); return FALSE; }
    return TRUE;
}

static void CreateDomeOpponentMon(u8 monPartyId, u16 tournamentTrainerId, u8 tournamentMonId, u32 otId)
{
    if (monPartyId >= ARRAY_COUNT(gParties[B_TRAINER_OPPONENT_A])
        || tournamentMonId >= ARRAY_COUNT(DOME_MONS[0]))
    { ADMG_StateAccessRejected(monPartyId, __func__); return; }
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    u64 generation = ADMG_BeginStateAccessCheck();
    const struct BattleFrontierTrainer *trainers;
    const struct TrainerMon *mons, *row;
    u32 count;
    u16 actor;
    if (!ADMG_TryDomeNpcAtSlot(tournamentTrainerId, &actor) || !ADMG_EndStateAccessCheck(generation)) return;
    if (gSaveBlock2Ptr != save || DOME_TRAINERS[tournamentTrainerId].trainerId != actor)
    { ADMG_StateAccessRejected(tournamentTrainerId, __func__); return; }
    u16 index = DOME_MONS[tournamentTrainerId][tournamentMonId];
    if (!ADMG_TrySelectFacilityTrainerTables(save->frontier.lvlMode == FRONTIER_LVL_TENT, &trainers, &mons, &count)
        || !ADMG_TryDomeMonRowFromTable(tournamentTrainerId, tournamentMonId, mons, &row)
        || !ADMG_DomeMonSourceCurrent(save, generation, tournamentTrainerId, tournamentMonId, actor, index)) return;
    #ifdef BUGFIX
    u8 fixedIv = GetDomeTrainerMonIvs(DOME_TRAINERS[tournamentTrainerId].trainerId);
    #else
    u8 fixedIv = GetDomeTrainerMonIvs(tournamentTrainerId); // Preserve the original native IV selection branch.
    #endif
    if (!ADMG_DomeMonSourceCurrent(save, generation, tournamentTrainerId, tournamentMonId, actor, index)) return;
    const struct BattleFrontierTrainer *previousTrainers = gFacilityTrainers;
    const struct TrainerMon *previousMons = gFacilityTrainerMons;
    u8 level = SetFacilityPtrsGetLevel();
    if (!ADMG_DomeMonSourceCurrent(save, generation, tournamentTrainerId, tournamentMonId, actor, index)) goto ADMG_reject_tables;
    if (gFacilityTrainers != trainers || gFacilityTrainerMons != mons || level > MAX_LEVEL)
    { ADMG_StateAccessRejected(level, __func__); goto ADMG_reject_tables; }
    struct Pokemon *target = &gParties[B_TRAINER_OPPONENT_A][monPartyId];
    struct Pokemon previous = *target;
    CreateFacilityMon(row, level, fixedIv, otId, 0, target);
    if (ADMG_DomeMonSourceCurrent(save, generation, tournamentTrainerId, tournamentMonId, actor, index))
    {
        if (gFacilityTrainers == trainers && gFacilityTrainerMons == mons) return;
        ADMG_StateAccessRejected(tournamentTrainerId, __func__);
    }
    *target = previous;
ADMG_reject_tables:
    if (gFacilityTrainers == trainers) gFacilityTrainers = previousTrainers;
    if (gFacilityTrainerMons == mons) gFacilityTrainerMons = previousMons;
}

static bool8 ADMG_DomeMonPartyCurrent(struct SaveBlock2 *save, u64 generation,
    u32 slot, u16 actor, const u16 *indices, u32 used)
{
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (save == NULL || gSaveBlock2Ptr != save || slot >= ARRAY_COUNT(DOME_TRAINERS)
        || DOME_TRAINERS[slot].trainerId != actor)
    { ADMG_StateAccessRejected(slot, __func__); return FALSE; }
    if (indices != NULL)
        for (u32 i = 0; i < FRONTIER_PARTY_SIZE; i++)
            if ((used & (1u << i)) && DOME_MONS[slot][i] != indices[i])
            { ADMG_StateAccessRejected(slot, __func__); return FALSE; }
    return TRUE;
}

static void ADMG_RunDomeMonParty(u16 tournamentTrainerId, u16 actor,
    struct SaveBlock2 *save, u64 generation) __attribute__((noinline));
static void ADMG_RunDomeMonParty(u16 tournamentTrainerId, u16 actor,
    struct SaveBlock2 *save, u64 generation)
{
    u8 monsCount = 0;
    u32 otId = 0;
    int i, selectedMonBits;

    if (!ADMG_DomeMonPartyCurrent(save, generation, tournamentTrainerId, actor, NULL, 0)) return;
    u16 indices[FRONTIER_PARTY_SIZE];
    memcpy(indices, DOME_MONS[tournamentTrainerId], sizeof(indices));
    u32 used = actor == TRAINER_FRONTIER_BRAIN ? 0 : (1u << FRONTIER_PARTY_SIZE) - 1;
    struct Pokemon saved[2][PARTY_SIZE];
    memcpy(saved[0], gParties[B_TRAINER_OPPONENT_A], sizeof(saved[0]));
    memcpy(saved[1], gParties[B_TRAINER_OPPONENT_B], sizeof(saved[1]));
    u8 countA = gPartiesCount[B_TRAINER_OPPONENT_A], countB = gPartiesCount[B_TRAINER_OPPONENT_B];
    const struct BattleFrontierTrainer *previousTrainers = gFacilityTrainers;
    const struct TrainerMon *previousMons = gFacilityTrainerMons;
    const struct BattleFrontierTrainer *ownedTrainers = NULL;
    const struct TrainerMon *ownedMons = NULL;
    ZeroEnemyPartyMons();
    if (!ADMG_DomeMonPartyCurrent(save, generation, tournamentTrainerId, actor, indices, used)) goto ADMG_reject;
    selectedMonBits = GetDomeTrainerSelectedMons(tournamentTrainerId);
    if (!ADMG_DomeMonPartyCurrent(save, generation, tournamentTrainerId, actor, indices, used)) goto ADMG_reject;
    used |= (u32)selectedMonBits & ((1u << FRONTIER_PARTY_SIZE) - 1);
    // Check only the native constructor inputs that this mask will consume.
    const struct BattleFrontierTrainer *trainers;
    const struct TrainerMon *mons, *row;
    u32 trainerCount;
    if (!ADMG_TrySelectFacilityTrainerTables(save->frontier.lvlMode == FRONTIER_LVL_TENT, &trainers, &mons, &trainerCount)
        || !ADMG_DomeMonPartyCurrent(save, generation, tournamentTrainerId, actor, indices, used)) goto ADMG_reject;
    for (u32 mon = 0; mon < FRONTIER_PARTY_SIZE; mon++)
        if (((u32)selectedMonBits & (1u << mon))
            && (!ADMG_TryDomeMonRowFromTable(tournamentTrainerId, mon, mons, &row)
                || !ADMG_DomeMonPartyCurrent(save, generation, tournamentTrainerId, actor, indices, used))) goto ADMG_reject;
    otId = Random32();
    if (!ADMG_DomeMonPartyCurrent(save, generation, tournamentTrainerId, actor, indices, used)) goto ADMG_reject;

    u16 order = Random();
    if (!ADMG_DomeMonPartyCurrent(save, generation, tournamentTrainerId, actor, indices, used)) goto ADMG_reject;
    if (order % 10 > 5)
    {
        // Create mon if it was selected, starting from front
        for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
        {
            if (selectedMonBits & 1)
            {
                ownedTrainers = trainers; ownedMons = mons;
                CreateDomeOpponentMon(monsCount, tournamentTrainerId, i, otId);
                if (!ADMG_DomeMonPartyCurrent(save, generation, tournamentTrainerId, actor, indices, used)) goto ADMG_reject;
                monsCount++;
            }
            selectedMonBits >>= 1;
        }
    }
    else
    {
        // Create mon if it was selected, starting from back
        for (i = FRONTIER_PARTY_SIZE - 1; i >= 0; i--)
        {
            if (selectedMonBits & (1 << (FRONTIER_PARTY_SIZE - 1)))
            {
                ownedTrainers = trainers; ownedMons = mons;
                CreateDomeOpponentMon(monsCount, tournamentTrainerId, i, otId);
                if (!ADMG_DomeMonPartyCurrent(save, generation, tournamentTrainerId, actor, indices, used)) goto ADMG_reject;
                monsCount++;
            }
            selectedMonBits <<= 1;
        }
    }
    return;
ADMG_reject:
    memcpy(gParties[B_TRAINER_OPPONENT_A], saved[0], sizeof(saved[0]));
    memcpy(gParties[B_TRAINER_OPPONENT_B], saved[1], sizeof(saved[1]));
    gPartiesCount[B_TRAINER_OPPONENT_A] = countA; gPartiesCount[B_TRAINER_OPPONENT_B] = countB;
    if (ownedTrainers != NULL && gFacilityTrainers == ownedTrainers) gFacilityTrainers = previousTrainers;
    if (ownedMons != NULL && gFacilityTrainerMons == ownedMons) gFacilityTrainerMons = previousMons;
}

static void CreateDomeOpponentMons(u16 tournamentTrainerId)
{
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    u64 generation = ADMG_BeginStateAccessCheck();
    u16 actor;
    if (!ADMG_TryDomeNpcAtSlot(tournamentTrainerId, &actor)
        || !ADMG_DomeMonPartyCurrent(save, generation, tournamentTrainerId, actor, NULL, 0)) return;
    ADMG_RunDomeMonParty(tournamentTrainerId, actor, save, generation);
}

// Returns a bitmask representing which 2 of the trainer's 3 Pokémon to select.
// The choice is calculated solely depending on the type effectiveness of their
// movesets against the player's Pokémon.
// There is a 50% chance of either a "good" or "bad" selection mode being used.
// In the good mode movesets are preferred which are more effective against the
// player, and in the bad mode the opposite is true. If all 3 Pokémon tie, the
// other mode will be tried. If they tie again, the Pokémon selection is random.
struct ADMG_DomeSelectionInput
{
    struct SaveBlock2 *save;
    u64 generation;
    u32 slot;
    u16 actor;
    const struct TrainerMon *table;
    const struct TrainerMon *rows[FRONTIER_PARTY_SIZE];
    u16 indices[FRONTIER_PARTY_SIZE];
};

static bool8 ADMG_DomeSelectionCurrent(const struct ADMG_DomeSelectionInput *input)
{
    if (!ADMG_EndStateAccessCheck(input->generation)) return FALSE;
    if (gSaveBlock2Ptr != input->save || input->save == NULL
        || input->slot >= ARRAY_COUNT(DOME_TRAINERS)
        || DOME_TRAINERS[input->slot].trainerId != input->actor)
    { ADMG_StateAccessRejected(input->slot, __func__); return FALSE; }
    if (input->actor != TRAINER_FRONTIER_BRAIN)
    {
        if (gFacilityTrainerMons != input->table)
        { ADMG_StateAccessRejected(input->slot, __func__); return FALSE; }
        for (u32 i = 0; i < ARRAY_COUNT(input->indices); i++)
            if (DOME_MONS[input->slot][i] != input->indices[i])
            { ADMG_StateAccessRejected(input->slot, __func__); return FALSE; }
    }
    return TRUE;
}

static bool8 ADMG_TryPrepareDomeSelection(u32 slot, struct ADMG_DomeSelectionInput *input)
{
    if (input == NULL) { ADMG_StateAccessRejected(slot, __func__); return FALSE; }
    struct ADMG_DomeSelectionInput selected = { .save = gSaveBlock2Ptr,
        .generation = ADMG_BeginStateAccessCheck(), .slot = slot };
    if (!ADMG_TryDomeNpcAtSlot(slot, &selected.actor)
        || !ADMG_EndStateAccessCheck(selected.generation)) return FALSE;
    if (gSaveBlock2Ptr != selected.save)
    { ADMG_StateAccessRejected(slot, __func__); return FALSE; }
    if (selected.actor != TRAINER_FRONTIER_BRAIN)
    {
        selected.table = gFacilityTrainerMons;
        for (u32 i = 0; i < ARRAY_COUNT(selected.indices); i++)
            selected.indices[i] = DOME_MONS[slot][i];
        for (u32 i = 0; i < ARRAY_COUNT(selected.rows); i++)
            if (!ADMG_TryDomeMonRowFromTable(slot, i, selected.table, &selected.rows[i])
                || !ADMG_EndStateAccessCheck(selected.generation)) return FALSE;
    }
    if (!ADMG_DomeSelectionCurrent(&selected)) return FALSE;
    *input = selected;
    return TRUE;
}

int GetDomeTrainerSelectedMons(u16 tournamentTrainerId)
{
    int selectedMonBits;
    struct ADMG_DomeSelectionInput input;
    if (!ADMG_TryPrepareDomeSelection(tournamentTrainerId, &input)) return 0;
    u16 choice = Random();
    if (!ADMG_DomeSelectionCurrent(&input)) return 0;
    if (choice & 1)
    {
        selectedMonBits = SelectOpponentMons_Good(tournamentTrainerId, FALSE);
        if (!ADMG_DomeSelectionCurrent(&input)) return 0;
        if (selectedMonBits == 0)
            selectedMonBits = SelectOpponentMons_Bad(tournamentTrainerId, TRUE);
    }
    else
    {
        selectedMonBits = SelectOpponentMons_Bad(tournamentTrainerId, FALSE);
        if (!ADMG_DomeSelectionCurrent(&input)) return 0;
        if (selectedMonBits == 0)
            selectedMonBits = SelectOpponentMons_Good(tournamentTrainerId, TRUE);
    }

    return ADMG_DomeSelectionCurrent(&input) ? selectedMonBits : 0;
}

static int SelectOpponentMons_Good(u16 tournamentTrainerId, bool8 allowRandom)
{
    int i, moveIndex, playerMonId;
    int partyMovePoints[FRONTIER_PARTY_SIZE];
    struct ADMG_DomeSelectionInput input;
    if (!ADMG_TryPrepareDomeSelection(tournamentTrainerId, &input)) return 0;

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        partyMovePoints[i] = 0;
        for (moveIndex = 0; moveIndex < MAX_MON_MOVES; moveIndex++)
        {
            for (playerMonId = 0; playerMonId < FRONTIER_PARTY_SIZE; playerMonId++)
            {
                if (!ADMG_DomeSelectionCurrent(&input)) return 0;
                if (input.actor == TRAINER_FRONTIER_BRAIN)
                {
                    partyMovePoints[i] += GetTypeEffectivenessPoints(GetFrontierBrainMonMove(i, moveIndex),
                                            GetMonData(&gParties[B_TRAINER_PLAYER][playerMonId], MON_DATA_SPECIES), EFFECTIVENESS_MODE_GOOD);
                }
                else
                {
                    partyMovePoints[i] += GetTypeEffectivenessPoints(input.rows[i]->moves[moveIndex],
                                            GetMonData(&gParties[B_TRAINER_PLAYER][playerMonId], MON_DATA_SPECIES), EFFECTIVENESS_MODE_GOOD);
                }
                if (!ADMG_DomeSelectionCurrent(&input)) return 0;
            }
        }
    }
    int result = SelectOpponentMonsFromParty(partyMovePoints, allowRandom);
    return ADMG_DomeSelectionCurrent(&input) ? result : 0;
}

// Identical to function above, but uses EFFECTIVENESS_MODE_BAD
static int SelectOpponentMons_Bad(u16 tournamentTrainerId, bool8 allowRandom)
{
    int i, moveIndex, playerMonId;
    int partyMovePoints[FRONTIER_PARTY_SIZE];
    struct ADMG_DomeSelectionInput input;
    if (!ADMG_TryPrepareDomeSelection(tournamentTrainerId, &input)) return 0;

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        partyMovePoints[i] = 0;
        for (moveIndex = 0; moveIndex < MAX_MON_MOVES; moveIndex++)
        {
            for (playerMonId = 0; playerMonId < FRONTIER_PARTY_SIZE; playerMonId++)
            {
                if (!ADMG_DomeSelectionCurrent(&input)) return 0;
                if (input.actor == TRAINER_FRONTIER_BRAIN)
                {
                    partyMovePoints[i] += GetTypeEffectivenessPoints(GetFrontierBrainMonMove(i, moveIndex),
                                            GetMonData(&gParties[B_TRAINER_PLAYER][playerMonId], MON_DATA_SPECIES), EFFECTIVENESS_MODE_BAD);
                }
                else
                {
                    partyMovePoints[i] += GetTypeEffectivenessPoints(input.rows[i]->moves[moveIndex],
                                            GetMonData(&gParties[B_TRAINER_PLAYER][playerMonId], MON_DATA_SPECIES), EFFECTIVENESS_MODE_BAD);
                }
                if (!ADMG_DomeSelectionCurrent(&input)) return 0;
            }
        }
    }
    int result = SelectOpponentMonsFromParty(partyMovePoints, allowRandom);
    return ADMG_DomeSelectionCurrent(&input) ? result : 0;
}

static int SelectOpponentMonsFromParty(int *partyMovePoints, bool8 allowRandom)
{
    int i, j;
    int selectedMonBits = 0;
    int partyPositions[FRONTIER_PARTY_SIZE];

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
        partyPositions[i] = i;

    // All party mons have equal move score totals, choose randomly
    if (partyMovePoints[0] == partyMovePoints[1]
     && partyMovePoints[0] == partyMovePoints[2])
    {
        if (allowRandom)
        {
            i = 0;
            while (i != DOME_BATTLE_PARTY_SIZE)
            {
                u32 rand = Random() & FRONTIER_PARTY_SIZE;
                if (rand != FRONTIER_PARTY_SIZE && !(selectedMonBits & (1u << rand)))
                {
                    selectedMonBits |= 1u << rand;
                    i++;
                }
            }
        }
    }
    else
    {
        for (i = 0; i < DOME_BATTLE_PARTY_SIZE; i++)
        {
            for (j = i + 1; j < FRONTIER_PARTY_SIZE; j++)
            {
                int temp;

                if (partyMovePoints[i] < partyMovePoints[j])
                {
                    SWAP(partyMovePoints[i], partyMovePoints[j],temp)
                    SWAP(partyPositions[i], partyPositions[j], temp)
                }

                if (partyMovePoints[i] == partyMovePoints[j] && (Random() & 1))
                {
                    SWAP(partyMovePoints[i], partyMovePoints[j],temp)
                    SWAP(partyPositions[i], partyPositions[j], temp)
                }
            }
        }

        for (i = 0; i < DOME_BATTLE_PARTY_SIZE; i++)
        {
            selectedMonBits |= 1u << partyPositions[i];
        }
    }

    return selectedMonBits;
}

#define TYPE_x0     0
#define TYPE_x0_25  5
#define TYPE_x0_50  10
#define TYPE_x1     20
#define TYPE_x2     40
#define TYPE_x4     80

static int GetTypeEffectivenessPoints(enum Move move, enum Species targetSpecies, int mode)
{
    enum Type defType1, defType2, moveType;
    int typePower = TYPE_x1;
    enum Ability defAbility;

    if (move == MOVE_NONE || move == MOVE_UNAVAILABLE || IsBattleMoveStatus(move))
        return 0;

    defType1 = GetSpeciesType(targetSpecies, 0);
    defType2 = GetSpeciesType(targetSpecies, 1);
    defAbility = GetSpeciesAbility(targetSpecies, 0);
    moveType = GetMoveType(move);

    if ((defAbility == ABILITY_LEVITATE || defAbility == ABILITY_EELEVATE) && moveType == TYPE_GROUND)
    {
        // They likely meant to return here, as 8 is the number of points normally used in this mode for moves with no effect.
        // Because there's no return the value instead gets interpreted by the switch, and the number of points becomes 0.
        if (mode == EFFECTIVENESS_MODE_BAD)
        {
            typePower = 8;
        #ifdef BUGFIX
            return typePower;
        #endif
        }
    }
    else
    {
        u32 typeEffectiveness1 = UQ_4_12_TO_INT(GetTypeModifier(moveType, defType1) * 2) * 5;
        u32 typeEffectiveness2 = UQ_4_12_TO_INT(GetTypeModifier(moveType, defType2) * 2) * 5;

        typePower = (typeEffectiveness1 * typePower) / 10;
        if (defType2 != defType1)
            typePower = (typeEffectiveness2 * typePower) / 10;

        if (defAbility == ABILITY_WONDER_GUARD && typeEffectiveness1 != TYPE_x1 && typeEffectiveness2 != TYPE_x1)
            typePower = 0;
    }

    switch (mode)
    {
    case EFFECTIVENESS_MODE_GOOD:
        // Weights moves that more effective.
        switch (typePower)
        {
        case TYPE_x0:
        case TYPE_x0_25:
        case TYPE_x0_50:
        default:
            typePower = 0;
            break;
        case TYPE_x1:
            typePower = 2;
            break;
        case TYPE_x2:
            typePower = 4;
            break;
        case TYPE_x4:
            typePower = 8;
            break;
        }
        break;
    case EFFECTIVENESS_MODE_BAD:
        // Weights moves that are less effective.
        // Odd that there's no limit on this being used, even the Frontier Brain could end up using this.
        switch (typePower)
        {
        case TYPE_x0:
            typePower = 8;
            break;
        case TYPE_x0_25:
            typePower = 4;
            break;
        case TYPE_x0_50:
            typePower = 2;
            break;
        default:
        case TYPE_x1:
            typePower = 0;
            break;
        case TYPE_x2:
            typePower = -2;
            break;
        case TYPE_x4:
            typePower = -4;
            break;
        }
        break;
    case EFFECTIVENESS_MODE_AI_VS_AI:
        // Used as part of calculating the winner in a battle between two AIs.
        // Weights moves that are more effective much more strongly in both directions.
        switch (typePower)
        {
        case TYPE_x0:
            typePower = -16;
            break;
        case TYPE_x0_25:
            typePower = -8;
            break;
        case TYPE_x0_50:
        default:
            typePower = 0;
            break;
        case TYPE_x1:
            typePower = 4;
            break;
        case TYPE_x2:
            typePower = 12;
            break;
        case TYPE_x4:
            typePower = 20;
            break;
        }
        break;
    }

    return typePower;
}

// Duplicate of GetFrontierTrainerFixedIvs
// NOTE: In CreateDomeOpponentMon a tournament trainer ID (0-15) is passed instead, resulting in all IVs of 3
//       To fix, see CreateDomeOpponentMon
static u8 GetDomeTrainerMonIvs(u16 trainerId)
{
    u8 fixedIv;

    if (trainerId <= FRONTIER_TRAINER_JILL)         // 0 - 99
        fixedIv = 3;
    else if (trainerId <= FRONTIER_TRAINER_CHLOE)   // 100 - 119
        fixedIv = 6;
    else if (trainerId <= FRONTIER_TRAINER_SOFIA)   // 120 - 139
        fixedIv = 9;
    else if (trainerId <= FRONTIER_TRAINER_JAZLYN)  // 140 - 159
        fixedIv = 12;
    else if (trainerId <= FRONTIER_TRAINER_ALISON)  // 160 - 179
        fixedIv = 15;
    else if (trainerId <= FRONTIER_TRAINER_LAMAR)   // 180 - 199
        fixedIv = 18;
    else if (trainerId <= FRONTIER_TRAINER_TESS)    // 200 - 219
        fixedIv = 21;
    else                                            // 220+ (- 299)
        fixedIv = MAX_PER_STAT_IVS;

    return fixedIv;
}

static int TournamentIdOfOpponent(int roundId, int trainerId)
{
    int i, j, opponentMax;

    u32 slot;
    if ((u32)roundId >= DOME_ROUNDS_COUNT)
    { ADMG_StateAccessRejected(roundId, __func__); return 0xFF; }
    if (!ADMG_TryDomeTrainerSlot(trainerId, &slot)) return 0xFF;
    i = slot;

    // Get trainer's opponent's tournament id
    if (roundId != DOME_ROUND1)
    {
        if (roundId == DOME_FINAL)
            opponentMax = sIdToOpponentId[i][roundId] + 8;
        else
            opponentMax = sIdToOpponentId[i][roundId] + 4;

        // Get first non-eliminated trainer in range of possible opponents
        for (j = sIdToOpponentId[i][roundId]; j < opponentMax; j++)
        {
            if (sTourneyTreeTrainerOpponentIds[j] != i && !DOME_TRAINERS[sTourneyTreeTrainerOpponentIds[j]].isEliminated)
                break;
        }

        if (j != opponentMax)
            return sTourneyTreeTrainerOpponentIds[j];
        else
            return 0xFF; // Already eliminated
    }
    else
    {
        if (!DOME_TRAINERS[sIdToOpponentId[i][roundId]].isEliminated)
            return sIdToOpponentId[i][roundId];
        else
            return 0xFF; // Already eliminated
    }
}

static void SetDomeOpponentId(void)
{
    u16 identity; u32 slot;
    u64 generation = ADMG_BeginStateAccessCheck();
    if (!ADMG_TryDomePlayerOpponent(&identity, &slot)
        || !ADMG_EndStateAccessCheck(generation)) return;
    TRAINER_BATTLE_PARAM.opponentA = identity;
}

// While not an issue in-game, this will overflow if called after the player's opponent for the current round has been eliminated
/* Admit the native opponent result before any caller publishes its identity. */
static bool8 ADMG_TryDomePlayerOpponent(u16 *identity, u32 *slot)
{
    if (gSaveBlock2Ptr == NULL || identity == NULL || slot == NULL)
    { ADMG_StateAccessRejected(TRAINER_PLAYER, __func__); return FALSE; }
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    u64 generation = ADMG_BeginStateAccessCheck();
    int selected = TournamentIdOfOpponent(gSaveBlock2Ptr->frontier.curChallengeBattleNum, TRAINER_PLAYER);
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != save)
    { ADMG_StateAccessRejected(TRAINER_PLAYER, __func__); return FALSE; }
    if ((u32)selected >= ARRAY_COUNT(DOME_TRAINERS))
    { ADMG_StateAccessRejected(selected, __func__); return FALSE; }
    u16 value = DOME_TRAINERS[selected].trainerId;
    u32 first;
    if (!ADMG_TryDomeOpponentSlot(value, &first)) return FALSE;
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != save)
    { ADMG_StateAccessRejected(value, __func__); return FALSE; }
    *identity = value; *slot = first;
    return TRUE;
}

static u16 TrainerIdOfPlayerOpponent(void)
{
    u16 identity; u32 slot;
    if (!ADMG_TryDomePlayerOpponent(&identity, &slot)) return TRAINER_NONE;
    return identity;
}

static void SetDomeOpponentGraphicsId(void)
{
    SetBattleFacilityTrainerGfxId(TRAINER_BATTLE_PARAM.opponentA, 0);
}

static void SaveDomeChallenge(void)
{
    ClearEnemyPartyAfterChallenge();
    gSaveBlock2Ptr->frontier.challengeStatus = gSpecialVar_0x8005;
    VarSet(VAR_TEMP_CHALLENGE_STATUS, 0);
    gSaveBlock2Ptr->frontier.challengePaused = TRUE;
    SaveGameFrontier();
}

static void IncrementDomeStreaks(void)
{
    enum FrontierLevelMode lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u8 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    if (gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode] < 999)
        gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode]++;
    if (gSaveBlock2Ptr->frontier.domeTotalChampionships[battleMode][lvlMode] < 999)
        gSaveBlock2Ptr->frontier.domeTotalChampionships[battleMode][lvlMode]++;

    if (gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode] > gSaveBlock2Ptr->frontier.domeRecordWinStreaks[battleMode][lvlMode])
        gSaveBlock2Ptr->frontier.domeRecordWinStreaks[battleMode][lvlMode] = gSaveBlock2Ptr->frontier.domeWinStreaks[battleMode][lvlMode];
}

// For showing the opponent info card of the upcoming trainer
static void ShowDomeOpponentInfo(void)
{
    u16 identity; u32 slot;
    u64 generation = ADMG_BeginStateAccessCheck();
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    if (!ADMG_TryDomePlayerOpponent(&identity, &slot) || !ADMG_EndStateAccessCheck(generation)) return;
    u32 round = save->frontier.curChallengeBattleNum;
    u32 freeSlot;
    for (freeSlot = 0; freeSlot < NUM_TASKS && gTasks[freeSlot].isActive; freeSlot++) {}
    if (freeSlot == NUM_TASKS)
    { ADMG_StateAccessRejected(freeSlot, __func__); return; }
    u8 taskId = CreateTask(Task_ShowTourneyInfoCard, 0);
    if (taskId != freeSlot || !gTasks[taskId].isActive || gTasks[taskId].func != Task_ShowTourneyInfoCard)
    { ADMG_StateAccessRejected(taskId, __func__); return; }
    u16 current; u32 currentSlot;
    if (!ADMG_TryDomePlayerOpponent(&current, &currentSlot) || !ADMG_EndStateAccessCheck(generation)
        || gSaveBlock2Ptr != save || gSaveBlock2Ptr->frontier.curChallengeBattleNum != round
        || current != identity || currentSlot != slot)
    {
        if (ADMG_EndStateAccessCheck(generation)) ADMG_StateAccessRejected(identity, __func__);
        if (gTasks[taskId].isActive && gTasks[taskId].func == Task_ShowTourneyInfoCard) DestroyTask(taskId);
        return;
    }
    gTasks[taskId].tState = 0;
    gTasks[taskId].tTournamentId = slot;
    gTasks[taskId].tMode = INFOCARD_NEXT_OPPONENT;
    gTasks[taskId].tPrevTaskId = 0;

    SetMainCallback2(CB2_TourneyTree);
}

// For showing the opponent info card or the match info card
/* The native allocator returns zero when full. A new card task must occupy
 * a previously free slot; named replacements may choose any such slot. */
static u32 ADMG_DomeFreeTaskSlots(void)
{
    _Static_assert(NUM_TASKS <= 32, "Dome task-slot mask width");
    u32 slots = 0;
    for (u32 i = 0; i < NUM_TASKS; i++) if (!gTasks[i].isActive) slots |= 1u << i;
    if (slots == 0) ADMG_StateAccessRejected(NUM_TASKS, __func__);
    return slots;
}

static bool8 ADMG_TryCreateDomeTask(TaskFunc function, u32 freeSlots, u8 *result)
{
    if (freeSlots == 0 || result == NULL)
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    u64 generation = ADMG_BeginStateAccessCheck();
    u8 task = CreateTask(function, 0);
    if (task >= NUM_TASKS || !(freeSlots & (1u << task))
        || !gTasks[task].isActive || gTasks[task].func != function)
    { ADMG_StateAccessRejected(task, __func__); return FALSE; }
    if (!ADMG_EndStateAccessCheck(generation)) { DestroyTask(task); return FALSE; }
    *result = task;
    return TRUE;
}

static bool8 ADMG_TryDomeInfoCardEntry(u8 task)
{
    if (task >= NUM_TASKS || gSaveBlock2Ptr == NULL)
    { ADMG_StateAccessRejected(task, __func__); return FALSE; }
    int id = gTasks[task].tTournamentId, mode = gTasks[task].tMode, parent = gTasks[task].tPrevTaskId;
    if ((u32)id >= (mode == INFOCARD_MATCH ? DOME_TOURNAMENT_MATCHES_COUNT : DOME_TOURNAMENT_TRAINERS_COUNT))
    { ADMG_StateAccessRejected(id, __func__); return FALSE; }
    if (mode != INFOCARD_NEXT_OPPONENT && (u32)gTasks[task].tPrevTaskId >= NUM_TASKS)
    { ADMG_StateAccessRejected(gTasks[task].tPrevTaskId, __func__); return FALSE; }
    if (mode != INFOCARD_MATCH)
    {
        const struct BattleFrontierTrainer *trainer;
        if (!ADMG_TryDomeNameIdentity(DOME_TRAINERS[id].trainerId, &trainer)) return FALSE;
    }
    if (gTasks[task].tTournamentId != id || gTasks[task].tMode != mode
        || (mode != INFOCARD_NEXT_OPPONENT && gTasks[task].tPrevTaskId != parent))
    { ADMG_StateAccessRejected(id, __func__); return FALSE; }
    return TRUE;
}

static void Task_ShowTourneyInfoCard(u8 taskId)
{
    if (taskId >= NUM_TASKS) { ADMG_StateAccessRejected(taskId, __func__); return; }
    if ((gTasks[taskId].tState == 0 || gTasks[taskId].tState == 3) && !ADMG_TryDomeInfoCardEntry(taskId)) return;

    u8 admgChild;
    u32 admgFreeSlots;
    u64 admgGeneration;
    int i;
    int tournamentId = gTasks[taskId].tTournamentId;
    int mode = gTasks[taskId].tMode;
    int id = gTasks[taskId].tPrevTaskId;

    switch (gTasks[taskId].tState)
    {
    case 0:
        SetHBlankCallback(NULL);
        SetVBlankCallback(NULL);
        EnableInterrupts(INTR_FLAG_VBLANK);
        CpuFill32(0, (void *)VRAM, VRAM_SIZE);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sInfoCardBgTemplates, ARRAY_COUNT(sInfoCardBgTemplates));
        InitWindows(sInfoCardWindowTemplates);
        DeactivateAllTextPrinters();
        gBattle_BG0_X = 0;
        gBattle_BG0_Y = 0;
        gBattle_BG1_X = 0;
        gBattle_BG1_Y = 0;
        gBattle_BG3_X = 0;
        gBattle_BG3_Y = 0;
        if (mode == INFOCARD_MATCH)
            gBattle_BG2_X = 0, gBattle_BG2_Y = 0;
        else
            gBattle_BG2_X = 0, gBattle_BG2_Y = DISPLAY_HEIGHT;

        gTasks[taskId].tState++;
        break;
    case 1:
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        SetGpuReg(REG_OFFSET_MOSAIC, 0);
        SetGpuReg(REG_OFFSET_WIN0H, 0);
        SetGpuReg(REG_OFFSET_WIN0V, 0);
        SetGpuReg(REG_OFFSET_WIN1H, 0);
        SetGpuReg(REG_OFFSET_WIN1V, 0);
        SetGpuReg(REG_OFFSET_WININ, 0);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
        ResetPaletteFade();
        ResetSpriteData();
        FreeAllSpritePalettes();
        gReservedSpritePaletteCount = 4;
        gTasks[taskId].tState++;
        break;
    case 2:
        DecompressAndLoadBgGfxUsingHeap(2, gDomeTourneyInfoCard_Gfx, 0x2000, 0, 0);
        DecompressAndLoadBgGfxUsingHeap(2, gDomeTourneyInfoCard_Tilemap, 0x2000, 0, 1);
        DecompressAndLoadBgGfxUsingHeap(3, gDomeTourneyInfoCardBg_Tilemap, 0x800, 0, 1);
        LoadCompressedSpriteSheet(sTourneyTreeButtonsSpriteSheet);
        LoadPalette(gDomeTourneyTree_Pal, BG_PLTT_OFFSET, BG_PLTT_SIZE);
        LoadPalette(gDomeTourneyTreeButtons_Pal, OBJ_PLTT_OFFSET, OBJ_PLTT_SIZE);
        LoadPalette(gBattleWindowTextPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);
        if (mode == INFOCARD_MATCH)
            LoadPalette(gDomeTourneyMatchCardBg_Pal, BG_PLTT_ID(5), PLTT_SIZE_4BPP); // Changes the moving info card bg to orange when in match card mode
        CpuFill32(0, gPlttBufferFaded, PLTT_SIZE);
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        ShowBg(3);
        gTasks[taskId].tState++;
        break;
    case 3:
        admgFreeSlots = ADMG_DomeFreeTaskSlots();
        if (admgFreeSlots == 0) return;
        admgGeneration = ADMG_BeginStateAccessCheck();
        SetVBlankCallback(VblankCb_TourneyInfoCard);
        sInfoCard = AllocZeroed(sizeof(*sInfoCard));
        for (i = 0; i < NUM_INFOCARD_SPRITES; i++)
            sInfoCard->spriteIds[i] = SPRITE_NONE;
        sInfoCard->pos = 0;
        LoadMonIconPalettes();
        if (!ADMG_TryCreateDomeTask(Task_HandleInfoCardInput, admgFreeSlots, &admgChild))
        {
            FREE_AND_SET_NULL(sInfoCard);
            DestroyTask(taskId);
            return;
        }
        i = admgChild;
        gTasks[i].data[0] = 0;
        gTasks[i].data[2] = 0;
        gTasks[i].data[3] = mode;
        gTasks[i].data[4] = id;
        if (mode == INFOCARD_MATCH)
        {
            DisplayMatchInfoOnCard(0, tournamentId);
            if (ADMG_EndStateAccessCheck(admgGeneration)) sInfoCard->pos = 1;
        }
        else
        {
            DisplayTrainerInfoOnCard(0, tournamentId);
        }
        if (!ADMG_EndStateAccessCheck(admgGeneration))
        {
            if (gTasks[admgChild].isActive && gTasks[admgChild].func == Task_HandleInfoCardInput) DestroyTask(admgChild);
            DestroyTask(taskId);
            return;
        }
        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_BG_ALL_ON | DISPCNT_OBJ_1D_MAP);
        if (mode != INFOCARD_NEXT_OPPONENT)
        {
            // Scroll up arrow
            id = CreateSprite(&sVerticalScrollArrowSpriteTemplate, 120, 4, 0);
            StartSpriteAnim(&gSprites[id], 0);
            gSprites[id].data[0] = i;

            // Scroll down arrow
            id = CreateSprite(&sVerticalScrollArrowSpriteTemplate, 120, 156, 0);
            StartSpriteAnim(&gSprites[id], 1);
            gSprites[id].data[0] = i;

            // Scroll left arrow
            id = CreateSprite(&sHorizontalScrollArrowSpriteTemplate, 6, 80, 0);
            StartSpriteAnim(&gSprites[id], 0);
            gSprites[id].data[0] = i;
            gSprites[id].data[1] = 0;
            if (mode == INFOCARD_TRAINER)
                gSprites[id].invisible = TRUE;

            // Scroll right arrow
            id = CreateSprite(&sHorizontalScrollArrowSpriteTemplate, 234, 80, 0);
            StartSpriteAnim(&gSprites[id], 1);
            gSprites[id].data[0] = i;
            gSprites[id].data[1] = 1;
        }
        DestroyTask(taskId);
        break;
    }
}

// Note: Card scrolling up means the current card goes down and another one appears from top.
// The same is true for scrolling left.
// That means that the sprite needs to move with the moving card in the opposite scrolling direction.
static void SpriteCB_TrainerIconCardScrollUp(struct Sprite *sprite)
{
    sprite->y += 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->y >= -32)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 40)
            sprite->callback = SpriteCallbackDummy;
    }
    else
    {
        if (sprite->y >= 192)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyTrainerPicSprite(sprite->data[3]);
        }
    }
}

static void SpriteCB_TrainerIconCardScrollDown(struct Sprite *sprite)
{
    sprite->y -= 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->y <= 192)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 40)
            sprite->callback = SpriteCallbackDummy;
    }
    else
    {
        if (sprite->y <= -32)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyTrainerPicSprite(sprite->data[3]);
        }
    }
}

static void SpriteCB_TrainerIconCardScrollLeft(struct Sprite *sprite)
{
    sprite->x += 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->x >= -32)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 64)
            sprite->callback = SpriteCallbackDummy;
    }
    else
    {
        if (sprite->x >= DISPLAY_WIDTH + 32)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyTrainerPicSprite(sprite->data[3]);
        }
    }
}

static void SpriteCB_TrainerIconCardScrollRight(struct Sprite *sprite)
{
    sprite->x -= 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->x <= DISPLAY_WIDTH + 32)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 64)
            sprite->callback = SpriteCallbackDummy;
    }
    else
    {
        if (sprite->x <= -32)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyTrainerPicSprite(sprite->data[3]);
        }
    }
}

#define sMonIconStill data[3]

static void SpriteCB_MonIconDomeInfo(struct Sprite *sprite)
{
    if (!sprite->sMonIconStill)
        UpdateMonIconFrame(sprite);
}

static void SpriteCB_MonIconCardScrollUp(struct Sprite *sprite)
{
    if (!sprite->sMonIconStill)
        UpdateMonIconFrame(sprite);
    sprite->y += 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->y >= -16)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 40)
            sprite->callback = SpriteCB_MonIconDomeInfo;
    }
    else
    {
        if (sprite->y >= 176)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyMonIconSprite(sprite);
        }
    }
}

static void SpriteCB_MonIconCardScrollDown(struct Sprite *sprite)
{
    if (!sprite->sMonIconStill)
        UpdateMonIconFrame(sprite);
    sprite->y -= 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->y <= 176)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 40)
            sprite->callback = SpriteCB_MonIconDomeInfo;
    }
    else
    {
        if (sprite->y <= -16)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyMonIconSprite(sprite);
        }
    }
}

static void SpriteCB_MonIconCardScrollLeft(struct Sprite *sprite)
{
    if (!sprite->sMonIconStill)
        UpdateMonIconFrame(sprite);
    sprite->x += 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->x >= -16)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 64)
            sprite->callback = SpriteCB_MonIconDomeInfo;
    }
    else
    {
        if (sprite->x >= DISPLAY_WIDTH + 16)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyMonIconSprite(sprite);
        }
    }
}

static void SpriteCB_MonIconCardScrollRight(struct Sprite *sprite)
{
    if (!sprite->sMonIconStill)
        UpdateMonIconFrame(sprite);
    sprite->x -= 4;
    if (sprite->data[0] != 0)
    {
        if (sprite->x <= DISPLAY_WIDTH + 16)
            sprite->invisible = FALSE;
        if (++sprite->data[1] == 64)
            sprite->callback = SpriteCB_MonIconDomeInfo;
    }
    else
    {
        if (sprite->x <= -16)
        {
            sInfoCard->spriteIds[sprite->data[2]] = SPRITE_NONE;
            FreeAndDestroyMonIconSprite(sprite);
        }
    }
}

/* Paired trainer positions are 0..15. Match-card positions are a different
 * domain and must never be read through this table. */
static bool8 ADMG_TryDomeCardTrainerPosition(int position, u32 *slot)
{
    if (slot == NULL || (u32)position >= ARRAY_COUNT(sTourneyTreeTrainerIds))
    { ADMG_StateAccessRejected(position, __func__); return FALSE; }
    u32 selected = sTourneyTreeTrainerIds[position];
    if (selected >= DOME_TOURNAMENT_TRAINERS_COUNT)
    { ADMG_StateAccessRejected(selected, __func__); return FALSE; }
    *slot = selected;
    return TRUE;
}

static void SpriteCB_HorizontalScrollArrow(struct Sprite *sprite)
{
    if (sprite == NULL || sInfoCard == NULL)
    { ADMG_StateAccessRejected(0, __func__); return; }
    int taskId1 = sprite->data[0];
    if ((u32)taskId1 >= NUM_TASKS)
    { ADMG_StateAccessRejected(taskId1, __func__); return; }
    u32 tournmanetTrainerId = 0;
    int roundId = 0;
    if (gTasks[taskId1].data[3] == 1 && sprite->data[1])
    {
        int parent = gTasks[taskId1].data[4];
        if ((u32)parent >= NUM_TASKS || gSaveBlock2Ptr == NULL)
        { ADMG_StateAccessRejected(parent, __func__); return; }
        if (!ADMG_TryDomeCardTrainerPosition(gTasks[parent].data[1], &tournmanetTrainerId)) return;
        roundId = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
    }

    if (gTasks[taskId1].data[3] == 1)
    {
        if (sprite->data[1])
        {
            if ((DOME_TRAINERS[tournmanetTrainerId].isEliminated
                && sInfoCard->pos - 1 < DOME_TRAINERS[tournmanetTrainerId].eliminatedAt))
            {
                sprite->invisible = FALSE;
            }
            else if (!DOME_TRAINERS[tournmanetTrainerId].isEliminated
                     && sInfoCard->pos - 1 < roundId)
            {
                sprite->invisible = FALSE;
            }
            else
            {
                if (gTasks[taskId1].data[0] == 2)
                    sprite->invisible = TRUE;
            }
        }
        else
        {
            if (sInfoCard->pos != 0)
            {
                sprite->invisible = FALSE;
            }
            else
            {
                if (gTasks[taskId1].data[0] == 2)
                    sprite->invisible = TRUE;
            }
        }
    }
    else
    {
        if (sprite->data[1])
        {
            if (sInfoCard->pos > 1)
            {
                if (gTasks[taskId1].data[0] == 2)
                    sprite->invisible = TRUE;
            }
            else
            {
                sprite->invisible = FALSE;
            }
        }
        else
        {
            if (sInfoCard->pos != 0)
            {
                sprite->invisible = FALSE;
            }
            else
            {
                if (gTasks[taskId1].data[0] == 2)
                    sprite->invisible = TRUE;
            }
        }
    }
}

static void SpriteCB_VerticalScrollArrow(struct Sprite *sprite)
{
    int taskId1 = sprite->data[0];

    if (gTasks[taskId1].data[3] == 1)
    {
        if (sInfoCard->pos != 0)
        {
            if (gTasks[taskId1].data[0] == 2)
                sprite->invisible = TRUE;
        }
        else
        {
            sprite->invisible = FALSE;
        }
    }
    else
    {
        if (sInfoCard->pos != 1)
        {
            if (gTasks[taskId1].data[0] == 2)
                sprite->invisible = TRUE;
        }
        else
        {
            sprite->invisible = FALSE;
        }
    }
}

// Task states for Task_HandleInfoCardInput
#define STATE_FADE_IN      0
#define STATE_WAIT_FADE    1
#define STATE_GET_INPUT    2
#define STATE_REACT_INPUT  3
#define STATE_MOVE_UP      4
#define STATE_MOVE_DOWN    5
#define STATE_MOVE_LEFT    6
#define STATE_MOVE_RIGHT   7
#define STATE_CLOSE_CARD   8

#define tUsingAlternateSlot data[2] // CARD_ALTERNATE_SLOT

#define ADMG_DOME_CARD_DISPLAY(expression) \
    do { expression; if (!ADMG_EndStateAccessCheck(admgCardGeneration)) goto ADMG_card_reject; } while (0)

static void Task_HandleInfoCardInput(u8 taskId)
{
    u64 admgCardGeneration = ADMG_BeginStateAccessCheck();
    __typeof__(gBattle_BG0_X) admgOldBg0X = gBattle_BG0_X;
    __typeof__(gBattle_BG0_Y) admgOldBg0Y = gBattle_BG0_Y;
    __typeof__(gBattle_BG1_X) admgOldBg1X = gBattle_BG1_X;
    __typeof__(gBattle_BG1_Y) admgOldBg1Y = gBattle_BG1_Y;
    __typeof__(gBattle_BG2_X) admgOldBg2X = gBattle_BG2_X;
    __typeof__(gBattle_BG2_Y) admgOldBg2Y = gBattle_BG2_Y;

    int i;
    int windowId = 0;
    int mode = gTasks[taskId].data[3];
    int taskId2 = gTasks[taskId].data[4];
    int trainerTourneyId = 0;
    int matchNo = 0;

    switch (gTasks[taskId].tState)
    {
    case STATE_FADE_IN:
        if (!gPaletteFade.active)
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
            gTasks[taskId].tState = STATE_WAIT_FADE;
        }
        break;
    case STATE_WAIT_FADE:
        if (!gPaletteFade.active)
            gTasks[taskId].tState = STATE_GET_INPUT;
        break;
    case STATE_GET_INPUT:
        i = Task_GetInfoCardInput(taskId);
        switch (i)
        {
        case INFOCARD_INPUT_AB:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            gTasks[taskId].tState = STATE_CLOSE_CARD;
            break;
        case TRAINERCARD_INPUT_UP ... TRAINERCARD_INPUT_RIGHT:
        case MATCHCARD_INPUT_UP ... MATCHCARD_INPUT_RIGHT:
            gTasks[taskId].data[5] = i;
            if (gTasks[taskId].tUsingAlternateSlot)
                windowId = NUM_INFO_CARD_WINDOWS;
            else
                windowId = 0;

            for (i = windowId; i < windowId + NUM_INFO_CARD_WINDOWS; i++)
            {
                CopyWindowToVram(i, COPYWIN_GFX);
                FillWindowPixelBuffer(i, PIXEL_FILL(0));
            }
            gTasks[taskId].tState = STATE_REACT_INPUT;
            break;
        case INFOCARD_INPUT_NONE:
            break;
        }
        break;
    case STATE_REACT_INPUT:
        i = gTasks[taskId].data[5];
        switch (i)
        {
        case TRAINERCARD_INPUT_UP:
        case MATCHCARD_INPUT_UP:
            if (gTasks[taskId].tUsingAlternateSlot)
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = DISPLAY_HEIGHT;
            }
            else
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = DISPLAY_HEIGHT;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = 0;
            }

            if (i == TRAINERCARD_INPUT_UP)
            {
                if (sInfoCard->pos == 0)
                {
                    gBattle_BG2_X = 0;
                    gBattle_BG2_Y = DISPLAY_HEIGHT * 2;
                    trainerTourneyId = sTourneyTreeTrainerIds[gTasks[taskId2].data[1]];
                    ADMG_DOME_CARD_DISPLAY(DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_UP, trainerTourneyId));
                }
                else
                {
                    gBattle_BG2_X = DISPLAY_WIDTH + 16;
                    gBattle_BG2_Y = 0;
                    trainerTourneyId = sTourneyTreeTrainerIds[gTasks[taskId2].data[1]];
                    ADMG_DOME_CARD_DISPLAY(DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_UP, trainerTourneyId));
                    sInfoCard->pos = 0;
                }
            }
            else // i == MATCHCARD_INPUT_UP
            {
                if (sInfoCard->pos == 0)
                {
                    matchNo = gTasks[taskId2].data[1] - 16;
                    if (!ADMG_TryBufferDomeMatchPair(matchNo, sInfoCard->tournamentIds, NULL)) goto ADMG_card_reject;
                    gBattle_BG2_X = 0;
                    gBattle_BG2_Y = DISPLAY_HEIGHT * 2;
                    trainerTourneyId = sInfoCard->tournamentIds[0];
                    ADMG_DOME_CARD_DISPLAY(DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_UP, trainerTourneyId));
                }
                else if (sInfoCard->pos == 2)
                {
                    matchNo = gTasks[taskId2].data[1] - 16;
                    if (!ADMG_TryBufferDomeMatchPair(matchNo, sInfoCard->tournamentIds, NULL)) goto ADMG_card_reject;
                    gBattle_BG2_X = 0;
                    gBattle_BG2_Y = DISPLAY_HEIGHT * 2;
                    trainerTourneyId = sInfoCard->tournamentIds[1];
                    ADMG_DOME_CARD_DISPLAY(DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_UP, trainerTourneyId));
                }
                else
                {
                    gBattle_BG2_X = DISPLAY_WIDTH + 16;
                    gBattle_BG2_Y = DISPLAY_HEIGHT;
                    matchNo = gTasks[taskId2].data[1] - 16;
                    ADMG_DOME_CARD_DISPLAY(DisplayMatchInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_UP, matchNo));
                }
            }

            for (i = 0; i < NUM_INFOCARD_SPRITES / 2; i++)
            {
                if (i < 2)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollUp;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollUp;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }
            for (i = NUM_INFOCARD_SPRITES / 2; i < NUM_INFOCARD_SPRITES; i++)
            {
                if (i < 10)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollUp;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollUp;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }

            gTasks[taskId].tState = STATE_MOVE_UP;
            gTasks[taskId].data[5] = 0;
            break;
        case TRAINERCARD_INPUT_DOWN:
        case MATCHCARD_INPUT_DOWN:
            if (gTasks[taskId].tUsingAlternateSlot)
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = -DISPLAY_HEIGHT;
            }
            else
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = -DISPLAY_HEIGHT;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = 0;
            }

            if (i == TRAINERCARD_INPUT_DOWN)
            {
                if (sInfoCard->pos == 0)
                {
                    gBattle_BG2_X = 0;
                    gBattle_BG2_Y = DISPLAY_HEIGHT;
                    trainerTourneyId = sTourneyTreeTrainerIds[gTasks[taskId2].data[1]];
                    ADMG_DOME_CARD_DISPLAY(DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_DOWN, trainerTourneyId));
                }
                else
                {
                    gBattle_BG2_X = 0;
                    gBattle_BG2_Y = 0;
                    trainerTourneyId = sTourneyTreeTrainerIds[gTasks[taskId2].data[1]];
                    ADMG_DOME_CARD_DISPLAY(DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_DOWN, trainerTourneyId));
                    sInfoCard->pos = 0;
                }
            }
            else // i == MATCHCARD_INPUT_DOWN
            {
                if (sInfoCard->pos == 0)
                {
                    matchNo = gTasks[taskId2].data[1] - 16;
                    if (!ADMG_TryBufferDomeMatchPair(matchNo, sInfoCard->tournamentIds, NULL)) goto ADMG_card_reject;
                    gBattle_BG2_X = 0;
                    gBattle_BG2_Y = DISPLAY_HEIGHT;
                    trainerTourneyId = sInfoCard->tournamentIds[0];
                    ADMG_DOME_CARD_DISPLAY(DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_DOWN, trainerTourneyId));
                }
                else if (sInfoCard->pos == 2)
                {
                    matchNo = gTasks[taskId2].data[1] - 16;
                    if (!ADMG_TryBufferDomeMatchPair(matchNo, sInfoCard->tournamentIds, NULL)) goto ADMG_card_reject;
                    gBattle_BG2_X = 0;
                    gBattle_BG2_Y = DISPLAY_HEIGHT;
                    trainerTourneyId = sInfoCard->tournamentIds[1];
                    ADMG_DOME_CARD_DISPLAY(DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_DOWN, trainerTourneyId));
                }
                else
                {
                    gBattle_BG2_X = DISPLAY_WIDTH + 16;
                    gBattle_BG2_Y = 0;
                    matchNo = gTasks[taskId2].data[1] - 16;
                    ADMG_DOME_CARD_DISPLAY(DisplayMatchInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_DOWN, matchNo));
                }
            }

            for (i = 0; i < NUM_INFOCARD_SPRITES / 2; i++)
            {
                if (i < 2)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollDown;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollDown;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }
            for (i = NUM_INFOCARD_SPRITES / 2; i < NUM_INFOCARD_SPRITES; i++)
            {
                if (i < 10)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollDown;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollDown;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }

            gTasks[taskId].tState = STATE_MOVE_DOWN;
            gTasks[taskId].data[5] = 0;
            break;
        case TRAINERCARD_INPUT_LEFT:
            if (gTasks[taskId].tUsingAlternateSlot)
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = DISPLAY_WIDTH + 16;
                gBattle_BG1_Y = 0;
            }
            else
            {
                gBattle_BG0_X = DISPLAY_WIDTH + 16;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = 0;
            }

            if (sInfoCard->pos == 0)
            {
                gBattle_BG2_X = DISPLAY_WIDTH + 16;
                gBattle_BG2_Y = DISPLAY_HEIGHT;
                trainerTourneyId = sTourneyTreeTrainerIds[gTasks[taskId2].data[1]];
                ADMG_DOME_CARD_DISPLAY(DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_LEFT, trainerTourneyId));
            }
            else
            {
                gBattle_BG2_X = DISPLAY_WIDTH + 16;
                gBattle_BG2_Y = 0;
                matchNo = sIdToMatchNumber[gTasks[taskId2].data[1]][sInfoCard->pos - 1];
                ADMG_DOME_CARD_DISPLAY(DisplayMatchInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_LEFT, matchNo));
            }

            for (i = 0; i < NUM_INFOCARD_SPRITES / 2; i++)
            {
                if (i < 2)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }
            for (i = NUM_INFOCARD_SPRITES / 2; i < NUM_INFOCARD_SPRITES; i++)
            {
                if (i < 10)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }

            gTasks[taskId].tState = STATE_MOVE_LEFT;
            gTasks[taskId].data[5] = 0;
            break;
        case MATCHCARD_INPUT_LEFT:
            if (gTasks[taskId].tUsingAlternateSlot)
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = DISPLAY_WIDTH + 16;
                gBattle_BG1_Y = 0;
            }
            else
            {
                gBattle_BG0_X = DISPLAY_WIDTH + 16;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = 0;
            }

            if (sInfoCard->pos == 0)
            {
                gBattle_BG2_X = DISPLAY_WIDTH + 16;
                gBattle_BG2_Y = DISPLAY_HEIGHT;
                trainerTourneyId = sInfoCard->tournamentIds[0];
                ADMG_DOME_CARD_DISPLAY(DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_LEFT, trainerTourneyId));
            }
            else
            {
                gBattle_BG2_X = 0;
                gBattle_BG2_Y = DISPLAY_HEIGHT;
                matchNo = gTasks[taskId2].data[1] - 16;
                ADMG_DOME_CARD_DISPLAY(DisplayMatchInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_LEFT, matchNo));
            }

            for (i = 0; i < NUM_INFOCARD_SPRITES / 2; i++)
            {
                if (i < 2)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }
            for (i = NUM_INFOCARD_SPRITES / 2; i < NUM_INFOCARD_SPRITES; i++)
            {
                if (i < 10)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollLeft;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }

            gTasks[taskId].tState = STATE_MOVE_LEFT;
            gTasks[taskId].data[5] = 0;
            break;
        case TRAINERCARD_INPUT_RIGHT:
            if (gTasks[taskId].tUsingAlternateSlot)
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = -(DISPLAY_WIDTH + 16);
                gBattle_BG1_Y = 0;
            }
            else
            {
                gBattle_BG0_X = -(DISPLAY_WIDTH + 16);
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = 0;
            }

            if (sInfoCard->pos == 1)
            {
                gBattle_BG2_X = 0;
                gBattle_BG2_Y = DISPLAY_HEIGHT;
            }
            else
            {
                gBattle_BG2_X = 0;
                gBattle_BG2_Y = 0;
            }
            matchNo = sIdToMatchNumber[gTasks[taskId2].data[1]][sInfoCard->pos - 1];
            ADMG_DOME_CARD_DISPLAY(DisplayMatchInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_RIGHT, matchNo));

            for (i = 0; i < NUM_INFOCARD_SPRITES / 2; i++)
            {
                if (i < 2)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }
            for (i = NUM_INFOCARD_SPRITES / 2; i < NUM_INFOCARD_SPRITES; i++)
            {
                if (i < 10)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }

            gTasks[taskId].tState = STATE_MOVE_RIGHT;
            gTasks[taskId].data[5] = 0;
            break;
        case MATCHCARD_INPUT_RIGHT:
            if (gTasks[taskId].tUsingAlternateSlot)
            {
                gBattle_BG0_X = 0;
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = -(DISPLAY_WIDTH + 16);
                gBattle_BG1_Y = 0;
            }
            else
            {
                gBattle_BG0_X = -(DISPLAY_WIDTH + 16);
                gBattle_BG0_Y = 0;
                gBattle_BG1_X = 0;
                gBattle_BG1_Y = 0;
            }

            if (sInfoCard->pos == 2)
            {
                gBattle_BG2_X = DISPLAY_WIDTH + 16;
                gBattle_BG2_Y = DISPLAY_HEIGHT;
                trainerTourneyId = sInfoCard->tournamentIds[1];
                ADMG_DOME_CARD_DISPLAY(DisplayTrainerInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_RIGHT, trainerTourneyId));
            }
            else
            {
                gBattle_BG2_X = 0;
                gBattle_BG2_Y = DISPLAY_HEIGHT;
                matchNo = gTasks[taskId2].data[1] - 16;
                ADMG_DOME_CARD_DISPLAY(DisplayMatchInfoOnCard(gTasks[taskId].tUsingAlternateSlot | MOVE_CARD_RIGHT, matchNo));
            }

            for (i = 0; i < NUM_INFOCARD_SPRITES / 2; i++)
            {
                if (i < 2)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot ^ 1;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }
            for (i = NUM_INFOCARD_SPRITES / 2; i < NUM_INFOCARD_SPRITES; i++)
            {
                if (i < 10)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_TrainerIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                        gSprites[sInfoCard->spriteIds[i]].data[3] = sInfoCard->spriteIds[i];
                    }
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                    {
                        gSprites[sInfoCard->spriteIds[i]].callback = SpriteCB_MonIconCardScrollRight;
                        gSprites[sInfoCard->spriteIds[i]].data[0] = gTasks[taskId].tUsingAlternateSlot;
                        gSprites[sInfoCard->spriteIds[i]].data[1] = 0;
                        gSprites[sInfoCard->spriteIds[i]].data[2] = i;
                    }
                }
            }

            gTasks[taskId].tState = STATE_MOVE_RIGHT;
            gTasks[taskId].data[5] = 0;
            break;
        }
        break;
    case STATE_MOVE_UP:
        if (++gTasks[taskId].data[5] != 41)
        {
            gBattle_BG0_Y -= 4;
            gBattle_BG1_Y -= 4;
            gBattle_BG2_Y -= 4;
        }
        else
        {
            gTasks[taskId].tState = STATE_GET_INPUT;
        }
        break;
    case STATE_MOVE_DOWN:
        if (++gTasks[taskId].data[5] != 41)
        {
            gBattle_BG0_Y += 4;
            gBattle_BG1_Y += 4;
            gBattle_BG2_Y += 4;
        }
        else
        {
            gTasks[taskId].tState = STATE_GET_INPUT;
        }
        break;
    case STATE_MOVE_LEFT:
        if (++gTasks[taskId].data[5] != 65)
        {
            gBattle_BG0_X -= 4;
            gBattle_BG1_X -= 4;
            gBattle_BG2_X -= 4;
        }
        else
        {
            gTasks[taskId].tState = STATE_GET_INPUT;
        }
        break;
    case STATE_MOVE_RIGHT:
        if (++gTasks[taskId].data[5] != 65)
        {
            gBattle_BG0_X += 4;
            gBattle_BG1_X += 4;
            gBattle_BG2_X += 4;
        }
        else
        {
            gTasks[taskId].tState = STATE_GET_INPUT;
        }
        break;
    case STATE_CLOSE_CARD:
        if (!gPaletteFade.active)
        {
            for (i = 0; i < NUM_INFOCARD_SPRITES / 2; i++)
            {
                if (i < 2)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                        FreeAndDestroyTrainerPicSprite(sInfoCard->spriteIds[i]);
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                        FreeAndDestroyMonIconSprite(&gSprites[sInfoCard->spriteIds[i]]);
                }
            }
            for (i = NUM_INFOCARD_SPRITES / 2; i < NUM_INFOCARD_SPRITES; i++)
            {
                if (i < 10)
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                        FreeAndDestroyTrainerPicSprite(sInfoCard->spriteIds[i]);
                }
                else
                {
                    if (sInfoCard->spriteIds[i] != SPRITE_NONE)
                        FreeAndDestroyMonIconSprite(&gSprites[sInfoCard->spriteIds[i]]);
                }
            }

            FreeMonIconPalettes();
            FREE_AND_SET_NULL(sInfoCard);
            FreeAllWindowBuffers();

            if (mode == INFOCARD_NEXT_OPPONENT)
            {
                SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
            }
            else
            {
                i = CreateTask(Task_ShowTourneyTree, 0);
                gTasks[i].data[0] = 0;
                gTasks[i].tNotInteractive = FALSE;
                gTasks[i].data[2] = 3;
                gTasks[i].data[3] = gTasks[taskId].data[4];
                gTasks[i].tIsPrevTourneyTree = gTasks[taskId2].data[6];
            }
            DestroyTask(taskId);
        }
        break;
    }
    return;
ADMG_card_reject:
    gBattle_BG0_X = admgOldBg0X;
    gBattle_BG0_Y = admgOldBg0Y;
    gBattle_BG1_X = admgOldBg1X;
    gBattle_BG1_Y = admgOldBg1Y;
    gBattle_BG2_X = admgOldBg2X;
    gBattle_BG2_Y = admgOldBg2Y;
}

#undef ADMG_DOME_CARD_DISPLAY

// undefine task states for Task_HandleInfoCardInput
#undef STATE_FADE_IN
#undef STATE_WAIT_FADE
#undef STATE_GET_INPUT
#undef STATE_REACT_INPUT
#undef STATE_MOVE_UP
#undef STATE_MOVE_DOWN
#undef STATE_MOVE_LEFT
#undef STATE_MOVE_RIGHT
#undef STATE_CLOSE_CARD

static u8 Task_GetInfoCardInput(u8 taskId)
{
    u8 input = INFOCARD_INPUT_NONE;
    if (taskId >= NUM_TASKS) { ADMG_StateAccessRejected(taskId, __func__); return input; }
    if (JOY_NEW(A_BUTTON | B_BUTTON)) input = INFOCARD_INPUT_AB;
    // The next-opponent card uses no parent, saved round or trainer position.
    if (gTasks[taskId].data[3] == INFOCARD_NEXT_OPPONENT) return input;
    int taskId2 = gTasks[taskId].data[4];
    if ((u32)taskId2 >= NUM_TASKS || sInfoCard == NULL)
    { ADMG_StateAccessRejected(taskId2, __func__); return INFOCARD_INPUT_NONE; }
    int position = gTasks[taskId2].data[1];
    u32 tourneyId = 0;
    u16 roundId = 0;

    if (gTasks[taskId].data[3] == INFOCARD_TRAINER)
    {
        if (!ADMG_TryDomeCardTrainerPosition(position, &tourneyId)) return INFOCARD_INPUT_NONE;
        if (gSaveBlock2Ptr == NULL || sInfoCard->pos > ARRAY_COUNT(sTrainerAndRoundToLastMatchCardNum[0]))
        { ADMG_StateAccessRejected(sInfoCard->pos, __func__); return INFOCARD_INPUT_NONE; }
        roundId = gSaveBlock2Ptr->frontier.curChallengeBattleNum;

        // For trainer info cards, pos is 0 when on a trainer info card (not viewing that trainer's match progression)
        // Scrolling up/down from a trainer info card goes to other trainer info cards
        if (JOY_NEW(DPAD_UP) && sInfoCard->pos == 0)
        {
            if (position == 0)
                position = DOME_TOURNAMENT_TRAINERS_COUNT - 1;
            else
                position--;
            input = TRAINERCARD_INPUT_UP;
        }
        else if (JOY_NEW(DPAD_DOWN) && sInfoCard->pos == 0)
        {
            if (position == DOME_TOURNAMENT_TRAINERS_COUNT - 1)
                position = 0;
            else
                position++;
            input = TRAINERCARD_INPUT_DOWN;
        }
        // Scrolling left can only be done after scrolling right
        else if (JOY_NEW(DPAD_LEFT) && sInfoCard->pos != 0)
        {
            sInfoCard->pos--;
            input = TRAINERCARD_INPUT_LEFT;
        }
        // Scrolling right from a trainer info card shows their match progression
        else if (JOY_NEW(DPAD_RIGHT))
        {
            // Can only scroll right from a trainer card until the round they were eliminated
            if (DOME_TRAINERS[tourneyId].isEliminated && sInfoCard->pos - 1 < DOME_TRAINERS[tourneyId].eliminatedAt)
            {
                if (sInfoCard->pos == ARRAY_COUNT(sTrainerAndRoundToLastMatchCardNum[0]))
                { ADMG_StateAccessRejected(sInfoCard->pos, __func__); return INFOCARD_INPUT_NONE; }
                sInfoCard->pos++;
                input = TRAINERCARD_INPUT_RIGHT;
            }
            // otherwise can scroll as far right as the current round allows
            if (!DOME_TRAINERS[tourneyId].isEliminated && sInfoCard->pos - 1 < roundId)
            {
                if (sInfoCard->pos == ARRAY_COUNT(sTrainerAndRoundToLastMatchCardNum[0]))
                { ADMG_StateAccessRejected(sInfoCard->pos, __func__); return INFOCARD_INPUT_NONE; }
                sInfoCard->pos++;
                input = TRAINERCARD_INPUT_RIGHT;
            }
        }

        if (input == INFOCARD_INPUT_AB)
        {
            if (sInfoCard->pos != 0)
                gTasks[taskId2].data[1] = sTrainerAndRoundToLastMatchCardNum[position / 2][sInfoCard->pos - 1];
            else
                gTasks[taskId2].data[1] = position;
        }
    }
    else // gTasks[taskId].data[3] == INFOCARD_MATCH
    {
        if (sInfoCard->pos > NUM_INFOCARD_TRAINERS)
        { ADMG_StateAccessRejected(sInfoCard->pos, __func__); return INFOCARD_INPUT_NONE; }
        if ((JOY_NEW(DPAD_UP) || JOY_NEW(DPAD_DOWN)) && sInfoCard->pos == 1)
        {
            if (gSaveBlock2Ptr == NULL || gSaveBlock2Ptr->frontier.curChallengeBattleNum >= ARRAY_COUNT(sLastMatchCardNum)
                || position < DOME_TOURNAMENT_TRAINERS_COUNT
                || position >= DOME_TOURNAMENT_TRAINERS_COUNT + DOME_TOURNAMENT_MATCHES_COUNT)
            { ADMG_StateAccessRejected(position, __func__); return INFOCARD_INPUT_NONE; }
            roundId = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
        }
        // For match info cards, pos is 1 when on the match card, 0 when on the left trainer, and 1 when on the right trainer
        // Scrolling up/down from a match info card goes to the next/previous match
        if (JOY_NEW(DPAD_UP) && sInfoCard->pos == 1)
        {
            if (position == DOME_TOURNAMENT_TRAINERS_COUNT)
                position = sLastMatchCardNum[roundId];
            else
                position--;
            if (position < DOME_TOURNAMENT_TRAINERS_COUNT
                || position >= DOME_TOURNAMENT_TRAINERS_COUNT + DOME_TOURNAMENT_MATCHES_COUNT)
            { ADMG_StateAccessRejected(position, __func__); return INFOCARD_INPUT_NONE; }
            input = MATCHCARD_INPUT_UP;
        }
        else if (JOY_NEW(DPAD_DOWN) && sInfoCard->pos == 1)
        {
            if (position == sLastMatchCardNum[roundId])
                position = DOME_TOURNAMENT_TRAINERS_COUNT;
            else
                position++;
            if (position < DOME_TOURNAMENT_TRAINERS_COUNT
                || position >= DOME_TOURNAMENT_TRAINERS_COUNT + DOME_TOURNAMENT_MATCHES_COUNT)
            { ADMG_StateAccessRejected(position, __func__); return INFOCARD_INPUT_NONE; }
            input = MATCHCARD_INPUT_DOWN;
        }
        // Scrolling left/right from a match info card shows the trainer info card of the competitors for that match
        else if (JOY_NEW(DPAD_LEFT) && sInfoCard->pos != 0)
        {
            input = MATCHCARD_INPUT_LEFT;
            sInfoCard->pos--;
        }
        else if (JOY_NEW(DPAD_RIGHT) && (sInfoCard->pos == 0 || sInfoCard->pos == 1))
        {
            input = MATCHCARD_INPUT_RIGHT;
            sInfoCard->pos++;
        }

        if (input == INFOCARD_INPUT_AB)
        {
            if (sInfoCard->pos == 0 && sInfoCard->tournamentIds[0] >= ARRAY_COUNT(sTournamentIdToPairedTrainerIds))
            { ADMG_StateAccessRejected(sInfoCard->tournamentIds[0], __func__); return INFOCARD_INPUT_NONE; }

            if (sInfoCard->pos == 2 && sInfoCard->tournamentIds[1] >= ARRAY_COUNT(sTournamentIdToPairedTrainerIds))
            { ADMG_StateAccessRejected(sInfoCard->tournamentIds[1], __func__); return INFOCARD_INPUT_NONE; }
            if (sInfoCard->pos == 0) // On left trainer info card
                gTasks[taskId2].data[1] = sTournamentIdToPairedTrainerIds[sInfoCard->tournamentIds[0]];
            else if (sInfoCard->pos == 2) // On right trainer info card
                gTasks[taskId2].data[1] = sTournamentIdToPairedTrainerIds[sInfoCard->tournamentIds[1]];
            else // On match info card
                gTasks[taskId2].data[1] = position;
        }
    }

    if (input != INFOCARD_INPUT_NONE && input != INFOCARD_INPUT_AB)
    {
        PlaySE(SE_SELECT);
        gTasks[taskId2].data[1] = position;
        gTasks[taskId].tUsingAlternateSlot ^= 1;
    }

    return input;
}

#undef tUsingAlternateSlot

static bool32 IsDomeHealingMove(enum Move move)
{
    if (IsHealingMove(move))
        return TRUE;
    // Check extra effects not considered plain healing by AI
    switch (GetMoveEffect(move))
    {
    case EFFECT_INGRAIN:
    case EFFECT_REFRESH:
    case EFFECT_AQUA_RING:
        return TRUE;
    default:
        return FALSE;
    }
}
static bool32 IsDomeDefensiveMove(enum Move move)
{
    switch (GetMoveEffect(move))
    {
    case EFFECT_REFLECT_DAMAGE:
    case EFFECT_LIGHT_SCREEN:
    case EFFECT_REFLECT:
    case EFFECT_AURORA_VEIL:
    case EFFECT_CONVERSION:
    case EFFECT_PROTECT:
    case EFFECT_MAT_BLOCK:
    case EFFECT_ENDURE:
    case EFFECT_SAFEGUARD:
    case EFFECT_MAGIC_COAT:
    case EFFECT_INGRAIN:
    case EFFECT_AQUA_RING:
    case EFFECT_SUBSTITUTE:
        return TRUE;
    default:
        break;
    }

    if (IsStatChangeStatusMove(move, IsAccDownEvasionUpStatChangeMove)
     || IsStatChangeStatusMove(move, IsDefSpDefStatUpMove))
        return TRUE;

    return FALSE;
}

static bool32 IsDomeRiskyMove(enum Move move)
{
    switch (GetMoveEffect(move))
    {
    // TODO: None of those functions have been updated from gen3
    case EFFECT_SPITE:
    case EFFECT_DESTINY_BOND:
    case EFFECT_PERISH_SONG:
        return TRUE;
    default:
        return IsExplosionMove(move);
    }
}

static bool32 IsDomeLuckyMove(enum Move move)
{
    if (GetMoveAccuracy(move) <= 50 && GetMoveAccuracy(move) != 0)
        return TRUE;
    switch (GetMoveEffect(move))
    {
    case EFFECT_REFLECT_DAMAGE:
        if (GetMoveReflectDamage_DamageCategories(move) != (1u << DAMAGE_CATEGORY_PHYSICAL)) // if not Counter
            return FALSE;
        // fallthrough
    case EFFECT_OHKO:
    case EFFECT_METRONOME:
    case EFFECT_MIRROR_MOVE:
    case EFFECT_SKETCH:
    case EFFECT_SLEEP_TALK:
    case EFFECT_PRESENT:
    case EFFECT_ASSIST:
    case EFFECT_MAGIC_COAT:
    case EFFECT_REVENGE:
    case EFFECT_IMPRISON:
    case EFFECT_SNATCH:
        return TRUE;
    default:
        return FALSE;
    }
}

static bool32 IsDomePopularMove(enum Move move)
{
    u8 i;
    for (i = 0; i < NUM_ALL_MACHINES; i++)
    {
        if (GetTMHMMoveId(i + 1) == move)
            return TRUE;
    }
    if (i == NUM_ALL_MACHINES)
        return FALSE;
    // Filter in TMs/HMs
    if (GetMovePower(move) >= 90)
        return TRUE;

    switch (GetMoveEffect(move))
    {
    case EFFECT_PROTECT:
    case EFFECT_MAT_BLOCK:
        return TRUE;
    default:
        break;
    }

    return IsStatChangeStatusMove(move, IsAtkSpAtkStatUpMove);
}

static bool32 IsDomeStatusMove(enum Move move)
{
    switch (GetMoveEffect(move))
    {
    case EFFECT_CONFUSE:
    case EFFECT_DISABLE:
    case EFFECT_LEECH_SEED:
    case EFFECT_TAUNT:
    case EFFECT_TORMENT:
    case EFFECT_ENCORE:
    case EFFECT_ATTRACT:
    case EFFECT_NIGHTMARE:
    case EFFECT_CURSE:
        return TRUE;
    default:
        break;
    }

    if (GetMoveNonVolatileStatus(move) != MOVE_EFFECT_NONE)
        return TRUE;
    if (MoveHasAdditionalEffect(move, MOVE_EFFECT_WRAP))
        return TRUE;

    return FALSE;
}

static bool32 IsDomeRareMove(enum Move move)
{
    u16 i, j;
    enum Species species = SPECIES_NONE;
    for (i = 0; i < NUM_SPECIES; i++)
    {
        if (!IsSpeciesEnabled(i))
            continue;
        const struct LevelUpMove *learnset = GetSpeciesLevelUpLearnset(i);
        for (j = 0; learnset[j].move != LEVEL_UP_MOVE_END; j++)
        {
            if (learnset[j].move == move)
            {
                species++;
                break;
            }
        }
        if (species >= NUM_SPECIES / 20) // At least 5% of all mons can learn this move
            return FALSE;
    }
    return TRUE;
}

static bool32 IsDomeComboMove(enum Move move)
{
    enum BattleMoveEffects effect = GetMoveEffect(move);
    switch (effect)
    {
    // Weather moves
    case EFFECT_WEATHER:
    // Terrain moves
    case EFFECT_TERRAIN:
    // Moves dependent on weather
    case EFFECT_SYNTHESIS:
    case EFFECT_MORNING_SUN:
    case EFFECT_MOONLIGHT:
    case EFFECT_SHORE_UP:
    case EFFECT_SOLAR_BEAM:
    case EFFECT_GROWTH:
    case EFFECT_AURORA_VEIL:
    case EFFECT_WEATHER_BALL:
    // Moves dependent on terrain
    case EFFECT_GRASSY_GLIDE:
    case EFFECT_TERRAIN_BOOST:
    case EFFECT_TERRAIN_PULSE:
    // Stockpile group
    case EFFECT_STOCKPILE:
    case EFFECT_SPIT_UP:
    case EFFECT_SWALLOW:
    // Entry hazards & cleaners
    case EFFECT_SPIKES:
    case EFFECT_TOXIC_SPIKES:
    case EFFECT_STEALTH_ROCK:
    case EFFECT_STICKY_WEB:
    case EFFECT_YAWN:
    case EFFECT_DREAM_EATER:
    case EFFECT_NIGHTMARE:
    case EFFECT_REST:
    case EFFECT_SLEEP_TALK:
    case EFFECT_SNORE:
    // Others
    case EFFECT_FOCUS_ENERGY:
    case EFFECT_LOCK_ON:
    case EFFECT_FLAIL:
    case EFFECT_BATON_PASS:
    case EFFECT_INGRAIN:
    case EFFECT_AQUA_RING:
    case EFFECT_LEECH_SEED:
    case EFFECT_ROAR:
        return TRUE;
    default:
        break;
    }

    // Move flags
    if (MoveAlwaysHitsInRain(move))
        return TRUE;
    else if (MoveAlwaysHitsInHailSnow(move))
        return TRUE;

    // Inflicting sleep & related effects
    switch (GetMoveNonVolatileStatus(move))
    {
    case MOVE_EFFECT_SLEEP:
        return TRUE;
    default:
        return FALSE;
    }

    return IsStatChangeStatusMove(move, IsAtkSpAtkStatUpMove);
}

// allocatedArray below needs to be large enough to hold stat totals for each mon, or totals of each type of move points
#define ALLOC_ARRAY_SIZE max(NUM_STATS * FRONTIER_PARTY_SIZE, NUM_MOVE_POINT_TYPES)

/* Card identities are physical roster slots. Native name/metadata calls may
 * publish a different declared mon table; do not freeze the initial globals. */
struct ADMG_DomeCardView
{
    struct SaveBlock2 *save;
    struct TourneyTreeInfoCard *card;
    u64 generation;
    u16 actors[NUM_INFOCARD_TRAINERS];
    u8 slots[NUM_INFOCARD_TRAINERS];
    u32 count;
};

static bool8 ADMG_DomeCardCurrent(const struct ADMG_DomeCardView *view)
{
    if (!ADMG_EndStateAccessCheck(view->generation)) return FALSE;
    if (gSaveBlock2Ptr != view->save || sInfoCard != view->card)
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    for (u32 i = 0; i < view->count; i++)
        if (view->save->frontier.domeTrainers[view->slots[i]].trainerId != view->actors[i])
        { ADMG_StateAccessRejected(view->slots[i], __func__); return FALSE; }
    return TRUE;
}

static const struct TrainerMon *ADMG_DomeCardMonRow(const struct ADMG_DomeCardView *view,
    u32 slot, u32 mon, const struct TrainerMon *table)
{
    if (!ADMG_DomeCardCurrent(view)) return NULL;
    if (slot >= ARRAY_COUNT(DOME_MONS) || mon >= ARRAY_COUNT(DOME_MONS[0]))
    { ADMG_StateAccessRejected(slot, __func__); return NULL; }
    u32 count;
    const struct TrainerMon *row = NULL;
    if (!ADMG_TryGetFacilityMonTableCount(table, &count)
        || !ADMG_TryGetFacilityMonRowFromTable(table, count, DOME_MONS[slot][mon], &row)
        || row == NULL)
    { ADMG_StateAccessRejected(slot, __func__); return NULL; }
    return ADMG_DomeCardCurrent(view) ? row : NULL;
}

static u32 ADMG_DomeCardNature(u32 nature)
{
    if (nature >= NUM_NATURES) { ADMG_StateAccessRejected(nature, __func__); return 0; }
    return nature;
}

static u32 ADMG_DomeCardSpecies(const struct ADMG_DomeCardView *view, u32 slot, u32 mon)
{
    if (!ADMG_DomeCardCurrent(view)) return SPECIES_NONE;
    if (slot >= ARRAY_COUNT(DOME_MONS) || mon >= ARRAY_COUNT(DOME_MONS[0]))
    { ADMG_StateAccessRejected(slot, __func__); return SPECIES_NONE; }
    u32 actor = DOME_TRAINERS[slot].trainerId, species;
    if (actor == TRAINER_PLAYER || actor == TRAINER_FRONTIER_BRAIN) species = DOME_MONS[slot][mon];
    else
    {
        const struct TrainerMon *row = ADMG_DomeCardMonRow(view, slot, mon, gFacilityTrainerMons);
        if (row == NULL) return SPECIES_NONE;
        species = row->species;
    }
    /* CreateMonIcon reads the palette field before its species sanitization. */
    if (species > NUM_SPECIES) { ADMG_StateAccessRejected(species, __func__); return SPECIES_NONE; }
    return species;
}

static bool8 ADMG_TryPrepareDomeCard(struct ADMG_DomeCardView *view,
    const u8 *slots, u32 count, bool8 stats)
{
    if (view == NULL || slots == NULL || gSaveBlock2Ptr == NULL || sInfoCard == NULL
        || count == 0 || count > NUM_INFOCARD_TRAINERS)
    { ADMG_StateAccessRejected(count, __func__); return FALSE; }
    *view = (struct ADMG_DomeCardView){ .save = gSaveBlock2Ptr, .card = sInfoCard,
        .generation = ADMG_BeginStateAccessCheck() };
    for (u32 i = 0; i < count; i++)
    {
        if (slots[i] >= ARRAY_COUNT(DOME_TRAINERS))
        { ADMG_StateAccessRejected(slots[i], __func__); return FALSE; }
        view->slots[i] = slots[i]; view->actors[i] = DOME_TRAINERS[slots[i]].trainerId; view->count++;
        const struct BattleFrontierTrainer *trainer;
        if (!ADMG_TryDomeNameIdentity(view->actors[i], &trainer) || !ADMG_DomeCardCurrent(view)) return FALSE;
        if (trainer != NULL && trainer->facilityClass >= FACILITY_CLASSES_COUNT)
        { ADMG_StateAccessRejected(trainer->facilityClass, __func__); return FALSE; }
        if (view->actors[i] == TRAINER_PLAYER)
        {
            u32 length;
            if (!ADMG_TryValidateTrainerText(view->save->playerName, sizeof(view->save->playerName), TRAINER_PLAYER, &length)
                || !ADMG_DomeCardCurrent(view)) return FALSE;
        }
        const struct TrainerMon *mons = NULL;
        if (trainer != NULL)
        {
            const struct BattleFrontierTrainer *trainers;
            u32 trainerCount;
            if (!ADMG_TrySelectFacilityTrainerTables(view->save->frontier.lvlMode == FRONTIER_LVL_TENT,
                    &trainers, &mons, &trainerCount) || !ADMG_DomeCardCurrent(view)) return FALSE;
        }
        for (u32 j = 0; j < FRONTIER_PARTY_SIZE; j++)
        {
            u32 species;
            if (trainer != NULL)
            {
                const struct TrainerMon *row = ADMG_DomeCardMonRow(view, slots[i], j, mons);
                if (row == NULL) return FALSE;
                species = row->species;
                if (stats) ADMG_DomeCardNature(row->nature);
            }
            else
            {
                species = DOME_MONS[slots[i]][j];
                if (stats && view->actors[i] == TRAINER_PLAYER)
                    ADMG_DomeCardNature(view->save->frontier.domePlayerPartyData[j].nature);
            }
            if (species > NUM_SPECIES) { ADMG_StateAccessRejected(species, __func__); return FALSE; }
            if (!ADMG_DomeCardCurrent(view)) return FALSE;
        }
    }
    return ADMG_DomeCardCurrent(view);
}

#define ADMG_DOME_INFO_VALUE(expression) \
    ({ __typeof__(expression) admg_value = (expression); \
       if (!ADMG_DomeCardCurrent(&admgView)) goto ADMG_info_reject; admg_value; })
#define ADMG_DOME_INFO_ACTION(expression) \
    do { expression; if (!ADMG_DomeCardCurrent(&admgView)) goto ADMG_info_reject; } while (0)

static void DisplayTrainerInfoOnCard(u8 flags, u8 trainerTourneyId)
{
    struct TextPrinterTemplate textPrinter;
    int i, j, k;
    int trainerId = 0;
    nature_t nature = 0;
    int arrId = 0;
    int windowId = WIN_TRAINER_NAME;
    int x = 0, y = 0;
    u8 palSlot = 0;
    struct ADMG_DomeCardView admgView;
    if (!ADMG_TryPrepareDomeCard(&admgView, &trainerTourneyId, 1, TRUE)) return;
    s16 *allocatedArray = AllocZeroed(sizeof(s16) * ALLOC_ARRAY_SIZE);
    trainerId = DOME_TRAINERS[trainerTourneyId].trainerId;

    if (flags & CARD_ALTERNATE_SLOT)
        arrId = 2 * (FRONTIER_PARTY_SIZE + 1), windowId = WIN_TRAINER_NAME + NUM_INFO_CARD_WINDOWS, palSlot = 2;
    if (flags & MOVE_CARD_RIGHT)
        x = DISPLAY_WIDTH + 16;
    if (flags & MOVE_CARD_DOWN)
        y = DISPLAY_HEIGHT;
    if (flags & MOVE_CARD_LEFT)
        x = -(DISPLAY_WIDTH + 16);
    if (flags & MOVE_CARD_UP)
        y = -DISPLAY_HEIGHT;

    // Create trainer pic sprite
    if (trainerId == TRAINER_PLAYER)
        sInfoCard->spriteIds[arrId] = CreateTrainerPicSprite(ADMG_DOME_INFO_VALUE(PlayerGenderToFrontTrainerPicId(gSaveBlock2Ptr->playerGender)), TRUE, x + 48, y + 64, palSlot + 12, TAG_NONE);
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
        sInfoCard->spriteIds[arrId] = CreateTrainerPicSprite(ADMG_DOME_INFO_VALUE(GetFrontierBrainTrainerPicIndex()), TRUE, x + 48, y + 64, palSlot + 12, TAG_NONE);
    else
        sInfoCard->spriteIds[arrId] = CreateTrainerPicSprite(ADMG_DOME_INFO_VALUE(GetFrontierTrainerFrontSpriteId(trainerId)), TRUE, x + 48, y + 64, palSlot + 12, TAG_NONE);

    if (flags & MOVE_CARD)
        gSprites[sInfoCard->spriteIds[arrId]].invisible = TRUE;

    // Create party mon icons
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        if (trainerId == TRAINER_PLAYER)
        {
            sInfoCard->spriteIds[2 + i + arrId] = CreateMonIcon(ADMG_DOME_INFO_VALUE(ADMG_DomeCardSpecies(&admgView, trainerTourneyId, i)),
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sInfoTrainerMonX[i],
                                                                  y + sInfoTrainerMonY[i],
                                                                  0, 0);
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].oam.priority = 0;
        }
        else if (trainerId == TRAINER_FRONTIER_BRAIN)
        {
            sInfoCard->spriteIds[2 + i + arrId] = CreateMonIcon(ADMG_DOME_INFO_VALUE(ADMG_DomeCardSpecies(&admgView, trainerTourneyId, i)),
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sInfoTrainerMonX[i],
                                                                  y + sInfoTrainerMonY[i],
                                                                  0, 0);
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].oam.priority = 0;
        }
        else
        {
            sInfoCard->spriteIds[2 + i + arrId] = CreateMonIcon(ADMG_DOME_INFO_VALUE(ADMG_DomeCardSpecies(&admgView, trainerTourneyId, i)),
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sInfoTrainerMonX[i],
                                                                  y + sInfoTrainerMonY[i],
                                                                  0, 0);
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].oam.priority = 0;
        }

        if (flags & MOVE_CARD)
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].invisible = TRUE;
    }

    // Initialize the text printer
    textPrinter.type = WINDOW_TEXT_PRINTER;
    textPrinter.fontId = FONT_SHORT;
    textPrinter.x = 0;
    textPrinter.y = 0;
    textPrinter.currentX = textPrinter.x;
    textPrinter.currentY = textPrinter.y;
    textPrinter.letterSpacing = 2;
    textPrinter.lineSpacing = 0;
    textPrinter.color.accent = TEXT_COLOR_TRANSPARENT;
    textPrinter.color.foreground = TEXT_DYNAMIC_COLOR_5;
    textPrinter.color.background = TEXT_COLOR_TRANSPARENT;
    textPrinter.color.shadow = TEXT_DYNAMIC_COLOR_4;

    // Get class and trainer name
    i = 0;
    if (trainerId == TRAINER_PLAYER)
        j = gFacilityClassToTrainerClass[FACILITY_CLASS_BRENDAN];
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
        j = ADMG_DOME_INFO_VALUE(GetFrontierBrainTrainerClass());
    else
        j = ADMG_DOME_INFO_VALUE(GetFrontierOpponentClass(trainerId));

    if ((u32)j >= ARRAY_COUNT(gTrainerClasses))
    { ADMG_StateAccessRejected(j, __func__); goto ADMG_info_reject; }
    for (;gTrainerClasses[j].name[i] != EOS; i++)
        gStringVar1[i] = gTrainerClasses[j].name[i];
    gStringVar1[i] = CHAR_SPACE;
    gStringVar1[i + 1] = EOS;

    if (trainerId == TRAINER_PLAYER)
    {
        StringAppend(gStringVar1, gSaveBlock2Ptr->playerName);
    }
    else if (trainerId == TRAINER_FRONTIER_BRAIN)
    {
        ADMG_DOME_INFO_ACTION(CopyFrontierBrainTrainerName(gStringVar2));
        StringAppend(gStringVar1, gStringVar2);
    }
    else
    {
        ADMG_DOME_INFO_ACTION(CopyDomeTrainerName(gStringVar2, trainerId));
        StringAppend(gStringVar1, gStringVar2);
    }

    // Print class and trainer name
    textPrinter.currentX = GetStringCenterAlignXOffsetWithLetterSpacing(textPrinter.fontId, gStringVar1, 0xD0, textPrinter.letterSpacing);
    textPrinter.currentChar = gStringVar1;
    textPrinter.windowId = windowId;
    PutWindowTilemap(windowId);
    CopyWindowToVram(windowId, COPYWIN_FULL);
    AddTextPrinter(&textPrinter, 0, NULL);
    textPrinter.letterSpacing = 0;

    // Print names of the party mons
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        textPrinter.currentY = sSpeciesNameTextYCoords[i];
        if (trainerId == TRAINER_PLAYER)
            textPrinter.currentChar = GetSpeciesName(ADMG_DOME_INFO_VALUE(ADMG_DomeCardSpecies(&admgView, trainerTourneyId, i)));
        else if (trainerId == TRAINER_FRONTIER_BRAIN)
            textPrinter.currentChar = GetSpeciesName(ADMG_DOME_INFO_VALUE(ADMG_DomeCardSpecies(&admgView, trainerTourneyId, i)));
        else
            textPrinter.currentChar = GetSpeciesName(ADMG_DOME_INFO_VALUE(ADMG_DomeCardSpecies(&admgView, trainerTourneyId, i)));
        textPrinter.fontId = GetFontIdToFit(textPrinter.currentChar, FONT_SHORT, 0, 60);

        textPrinter.windowId = WIN_TRAINER_MON1_NAME + i + windowId;
        if (i == 1)
            textPrinter.currentX = 7;
        else
            textPrinter.currentX = 0;

        PutWindowTilemap(WIN_TRAINER_MON1_NAME + i + windowId);
        CopyWindowToVram(WIN_TRAINER_MON1_NAME + i + windowId, COPYWIN_FULL);
        AddTextPrinter(&textPrinter, 0, NULL);
    }
    textPrinter.fontId = FONT_SHORT;

    PutWindowTilemap(windowId + WIN_TRAINER_FLAVOR_TEXT);
    CopyWindowToVram(windowId + WIN_TRAINER_FLAVOR_TEXT, COPYWIN_FULL);

    // Print text about trainers potential in the tourney
    if (trainerId == TRAINER_FRONTIER_BRAIN)
        textPrinter.currentChar = sBattleDomePotentialTexts[DOME_TOURNAMENT_TRAINERS_COUNT];
    else
        textPrinter.currentChar = sBattleDomePotentialTexts[trainerTourneyId];

    textPrinter.fontId = FONT_NORMAL;
    textPrinter.windowId = windowId + WIN_TRAINER_FLAVOR_TEXT;
    textPrinter.currentX = 0;
    textPrinter.y = 4;
    textPrinter.currentY = 4;
    AddTextPrinter(&textPrinter, 0, NULL);

    // Calculate move scores to determine the trainers battle style
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            for (k = 0; k < NUM_MOVE_POINT_TYPES; k++)
            {
                enum Move move;
                if (trainerId == TRAINER_FRONTIER_BRAIN)
                    move = ADMG_DOME_INFO_VALUE(GetFrontierBrainMonMove(i, j));
                else if (trainerId == TRAINER_PLAYER)
                    move = gSaveBlock2Ptr->frontier.domePlayerPartyData[i].moves[j];
                else
                    move = (*ADMG_DOME_INFO_VALUE(ADMG_DomeCardMonRow(&admgView, trainerTourneyId, i, gFacilityTrainerMons))).moves[j];
                u32 accuracy = GetMoveAccuracy(move);

                switch (k)
                {
                case MOVE_POINTS_COMBO:
                    allocatedArray[k] = IsDomeComboMove(move) ? 1 : 0;
                    break;
                case MOVE_POINTS_STAT_RAISE:
                    allocatedArray[k] = IsStatRaisingMove(move) ? 1 : 0;
                    break;
                case MOVE_POINTS_STAT_LOWER:
                    allocatedArray[k] = IsStatLoweringMove(move) ? 1 : 0;
                    break;
                case MOVE_POINTS_RARE:
                    allocatedArray[k] = IsDomeRareMove(move) ? 1 : 0;
                    break;
                case MOVE_POINTS_HEAL:
                    allocatedArray[k] = IsDomeHealingMove(move) ? 1 : 0;
                    break;
                case MOVE_POINTS_RISKY:
                    allocatedArray[k] = IsDomeRiskyMove(move) ? 1 : 0;
                    break;
                case MOVE_POINTS_STATUS:
                    allocatedArray[k] = IsDomeStatusMove(move);
                    break;
                case MOVE_POINTS_DMG:
                    allocatedArray[k] = (!IsBattleMoveStatus(move)) ? 1 : 0;
                    break;
                case MOVE_POINTS_DEF:
                    allocatedArray[k] = IsDomeDefensiveMove(move) ? 1 : 0;
                    break;
                case MOVE_POINTS_ACCURATE:
                    allocatedArray[k] = (accuracy == 0 || accuracy == 100) ? 1 : 0;
                    break;
                case MOVE_POINTS_POWERFUL:
                    allocatedArray[k] = (GetMovePower(move) >= 100) ? 1 : 0;
                    break;
                case MOVE_POINTS_POPULAR:
                    allocatedArray[k] = IsDomePopularMove(move) ? 1 : 0;
                    break;
                case MOVE_POINTS_LUCK:
                    allocatedArray[k] = IsDomeLuckyMove(move) ? 1 : 0;
                    break;
                case MOVE_POINTS_STRONG:
                    allocatedArray[k] = (GetMovePower(move) >= 90) ? 1 : 0;
                    break;
                case MOVE_POINTS_LOW_PP:
                    allocatedArray[k] = (GetMovePP(move) <= 5) ? 1 : 0;
                    break;
                case MOVE_POINTS_EFFECT:
                    allocatedArray[k] = MoveIsAffectedBySheerForce(move);
                    break;
                }
            }
        }
    }

    // Get the battle style the trainer uses
    // Each element of sBattleStyleThresholds is an array of point thresholds for particular move qualities
    // If all the point thresholds in the array are satisfied, the player is considered to be using that battle style
    for (i = 0; i < ARRAY_COUNT(sBattleStyleThresholds); i++)
    {
        int thresholdStatCount = 0;

        for (k = 0, j = 0; j < NUM_MOVE_POINT_TYPES; j++)
        {
            if (sBattleStyleThresholds[i][j] != 0)
            {
                thresholdStatCount++;
                if (allocatedArray[j] != 0 && allocatedArray[j] >= sBattleStyleThresholds[i][j])
                    k++; // number of point thresholds met/exceeded
            }
        }
        if (thresholdStatCount == k)
            break; // All thresholds for battle style met/exceeded, player uses this battle style
    }

    // Print the trainers battle style
    textPrinter.currentChar = sBattleDomeOpponentStyleTexts[i];
    textPrinter.y = 20;
    textPrinter.currentY = 20;
    AddTextPrinter(&textPrinter, 0, NULL);

    for (i = 0; i < ALLOC_ARRAY_SIZE; i++)
        allocatedArray[i] = 0;

    // Calculate EV/nature points for the stat portion of battle style
    if (trainerId == TRAINER_FRONTIER_BRAIN || trainerId == TRAINER_PLAYER)
    {
        for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
        {
            // Add the EVs for this mon
            for (j = 0; j < NUM_STATS; j++)
            {
                if (trainerId == TRAINER_FRONTIER_BRAIN)
                    allocatedArray[j] = ADMG_DOME_INFO_VALUE(GetFrontierBrainMonEvs(i, j));
                else
                    allocatedArray[j] = gSaveBlock2Ptr->frontier.domePlayerPartyData[i].evs[j];
            }

            // HP doesnt have a nature modifier, so just add it here
            allocatedArray[NUM_STATS] += allocatedArray[STAT_HP];

            // Add the EVs with the nature modifier for this mon and and track number of negative natures
            for (j = 0; j < NUM_NATURE_STATS; j++)
            {
                if (trainerId == TRAINER_FRONTIER_BRAIN)
                    nature = ADMG_DOME_INFO_VALUE(ADMG_DomeCardNature(ADMG_DOME_INFO_VALUE(GetFrontierBrainMonNature(i))));
                else
                    nature = ADMG_DOME_INFO_VALUE(ADMG_DomeCardNature(gSaveBlock2Ptr->frontier.domePlayerPartyData[i].nature));

                if (gNaturesInfo[nature].statUp == gNaturesInfo[nature].statDown)
                {
                    allocatedArray[j + NUM_STATS + 1] += allocatedArray[j + 1];
                }
                else if (gNaturesInfo[nature].statUp == j + 1)
                {
                    allocatedArray[j + NUM_STATS + 1] += (allocatedArray[j + 1] * 110) / 100;
                }
                else if (gNaturesInfo[nature].statDown == j + 1)
                {
                    allocatedArray[j + NUM_STATS + 1] += (allocatedArray[j + 1] * 90) / 100;
                    allocatedArray[j + NUM_STATS + NUM_NATURE_STATS + 2]++;
                }
                else
                {
                    allocatedArray[j + NUM_STATS + 1] += allocatedArray[j + 1];
                }
            }
        }
        for (j = 0, i = 0; i < NUM_STATS; i++)
            j += allocatedArray[NUM_STATS + i];
        for (i = 0; i < NUM_STATS; i++)
            allocatedArray[i] = (allocatedArray[NUM_STATS + i] * 100) / j;
    }
    // Same as above but for regular trainers instead of the frontier brain or player
    else
    {
        for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
        {
            for (j = 0; j < NUM_STATS; j++)
            {
                if ((*ADMG_DOME_INFO_VALUE(ADMG_DomeCardMonRow(&admgView, trainerTourneyId, i, gFacilityTrainerMons))).ev != NULL)
                    allocatedArray[j] = (*ADMG_DOME_INFO_VALUE(ADMG_DomeCardMonRow(&admgView, trainerTourneyId, i, gFacilityTrainerMons))).ev[j];
                else
                    allocatedArray[j] = 0;
            }

            allocatedArray[NUM_STATS] += allocatedArray[STAT_HP];
            for (j = 0; j < NUM_NATURE_STATS; j++)
            {
                nature = ADMG_DOME_INFO_VALUE(ADMG_DomeCardNature((*ADMG_DOME_INFO_VALUE(ADMG_DomeCardMonRow(&admgView, trainerTourneyId, i, gFacilityTrainerMons))).nature));

                if (gNaturesInfo[nature].statUp == gNaturesInfo[nature].statDown)
                {
                    allocatedArray[j + NUM_STATS + 1] += allocatedArray[j + 1];
                }
                else if (gNaturesInfo[nature].statUp == j + 1)
                {
                    allocatedArray[j + NUM_STATS + 1] += (allocatedArray[j + 1] * 110) / 100;
                }
                else if (gNaturesInfo[nature].statDown == j + 1)
                {
                    allocatedArray[j + NUM_STATS + 1] += (allocatedArray[j + 1] * 90) / 100;
                    allocatedArray[j + NUM_STATS + NUM_NATURE_STATS + 2]++;
                }
                else
                {
                    allocatedArray[j + NUM_STATS + 1] += allocatedArray[j + 1];
                }
            }
        }
        for (j = 0, i = 0; i < NUM_STATS; i++)
            j += allocatedArray[i + NUM_STATS];
        for (i = 0; i < NUM_STATS; i++)
            allocatedArray[i] = (allocatedArray[NUM_STATS + i] * 100) / j;
    }

    // Count the number of good/bad stats for the party
    // i is the number of good stats, j is the number of bad stats
    for (i = 0, j = 0, k = 0; k < NUM_STATS; k++)
    {
        // Any stat above 29 EVs is considered good
        if (allocatedArray[k] > 29)
        {
            // If 2 good stats have been found already, choose which to use
            if (i == 2)
            {
                if (allocatedArray[6] < allocatedArray[k])
                {
                    if (allocatedArray[7] < allocatedArray[k])
                    {
                        if (allocatedArray[6] < allocatedArray[7])
                        {
                            allocatedArray[6] = allocatedArray[7];
                            allocatedArray[7] = k;
                        }
                        else
                        {
                            allocatedArray[7] = k;
                        }
                    }
                    else
                    {
                        allocatedArray[6] = allocatedArray[7];
                        allocatedArray[7] = k;
                    }
                }
                else
                {
                    if (allocatedArray[7] < allocatedArray[k])
                        allocatedArray[7] = k;
                }
            }
            else
            {
                allocatedArray[i + 6] = k;
                i++;
            }
        }

        // Any stat with 0 EVs is considered bad
        if (allocatedArray[k] == 0)
        {
            // If 2 bad stats have been found already, choose which to use
            if (j == 2)
            {
                if (allocatedArray[k + 12] >= 2
                    || ((allocatedArray[k + 12] == 1 && allocatedArray[12 + allocatedArray[8]] == 0 && allocatedArray[12 + allocatedArray[9]] == 0)
                       )
                    )
                {
                    allocatedArray[8] = allocatedArray[9];
                    allocatedArray[9] = k;
                }
                else if (allocatedArray[k + 12] == 1 && allocatedArray[12 + allocatedArray[8]] == 0)
                {
                    allocatedArray[8] = allocatedArray[9];
                    allocatedArray[9] = k;
                }
                else if (allocatedArray[k + 12] == 1 && allocatedArray[12 + allocatedArray[9]] == 0)
                {
                    allocatedArray[9] = k;
                }
            }
            else
            {
                allocatedArray[j + 8] = k;
                j++;
            }
        }
    }

    // Get the string ID to display which stats are good/bad
    if (i == 2)
        i = sStatTextOffsets[allocatedArray[6]] + (allocatedArray[7] - (allocatedArray[6] + 1)) + DOME_TEXT_TWO_GOOD_STATS;
    else if (i == 1)
        i = allocatedArray[6] + DOME_TEXT_ONE_GOOD_STAT;
    else if (j == 2)
        i = sStatTextOffsets[allocatedArray[8]] + (allocatedArray[9] - (allocatedArray[8] + 1)) + DOME_TEXT_TWO_BAD_STATS;
    else if (j == 1)
        i = allocatedArray[8] + DOME_TEXT_ONE_BAD_STAT;
    else
        i = DOME_TEXT_WELL_BALANCED;

    // Print the stat text
    textPrinter.currentChar = sBattleDomeOpponentStatsTexts[i];
    textPrinter.y = 36;
    textPrinter.currentY = 36;
    AddTextPrinter(&textPrinter, 0, NULL);
    Free(allocatedArray);

    return;
ADMG_info_reject:
    Free(allocatedArray);
}


/* Match fields are borrowed; only output bytes and native text writes are owned. */
struct ADMG_DomeMatchView
{
    struct SaveBlock2 *save;
    u64 generation;
    struct BattleDomeTrainer trainers[DOME_TOURNAMENT_TRAINERS_COUNT];
    u16 moves[DOME_TOURNAMENT_TRAINERS_COUNT];
    u8 playerName[PLAYER_NAME_LENGTH + 1];
    u8 ids[2];
    u32 count, readMask, roundMask, nameMask, moveMask;
    u32 writtenStrings, writtenIds;
    bool8 playerNameUsed, failed;
};

static bool8 ADMG_DomeMatchCurrent(struct ADMG_DomeMatchView *view)
{
    if (view == NULL || view->save == NULL)
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    if (!ADMG_EndStateAccessCheck(view->generation)) return FALSE;
    if (gSaveBlock2Ptr != view->save)
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    for (u32 i = 0; i < ARRAY_COUNT(view->trainers); i++)
    {
        u32 bit = 1u << i;
        const struct BattleDomeTrainer *actual = &view->save->frontier.domeTrainers[i];
        if (((view->readMask & bit) && actual->isEliminated != view->trainers[i].isEliminated)
            || ((view->roundMask & bit) && actual->eliminatedAt != view->trainers[i].eliminatedAt)
            || ((view->nameMask & bit) && actual->trainerId != view->trainers[i].trainerId)
            || ((view->moveMask & bit) && (actual->forfeited != view->trainers[i].forfeited
                || view->save->frontier.domeWinningMoves[i] != view->moves[i])))
        { ADMG_StateAccessRejected(i, __func__); return FALSE; }
    }
    if (view->playerNameUsed && memcmp(view->playerName, view->save->playerName, sizeof(view->playerName)) != 0)
    { ADMG_StateAccessRejected(TRAINER_PLAYER, __func__); return FALSE; }
    return TRUE;
}

static bool8 ADMG_TryPrepareDomeMatch(u32 match, struct ADMG_DomeMatchView *view)
{
    if (view == NULL || gSaveBlock2Ptr == NULL || match >= ARRAY_COUNT(sCompetitorRangeByMatch))
    { ADMG_StateAccessRejected(match, __func__); return FALSE; }
    *view = (struct ADMG_DomeMatchView){0};
    view->save = gSaveBlock2Ptr;
    view->generation = ADMG_BeginStateAccessCheck();
    u32 first = sCompetitorRangeByMatch[match][0], length = sCompetitorRangeByMatch[match][1];
    u32 round = sCompetitorRangeByMatch[match][2];
    if (first > ARRAY_COUNT(sTourneyTreeTrainerIds2) || length > ARRAY_COUNT(sTourneyTreeTrainerIds2) - first)
    { ADMG_StateAccessRejected(match, __func__); return FALSE; }
    memcpy(view->trainers, view->save->frontier.domeTrainers, sizeof(view->trainers));
    memcpy(view->moves, view->save->frontier.domeWinningMoves, sizeof(view->moves));
    memcpy(view->playerName, view->save->playerName, sizeof(view->playerName));
    for (u32 i = first; i < first + length; i++)
    {
        u32 slot = sTourneyTreeTrainerIds2[i];
        if (slot >= ARRAY_COUNT(view->trainers))
        { ADMG_StateAccessRejected(slot, __func__); return FALSE; }
        view->readMask |= 1u << slot;
        if (!view->trainers[slot].isEliminated)
        {
            if (view->count == ARRAY_COUNT(view->ids))
            { ADMG_StateAccessRejected(match, __func__); return FALSE; }
            view->ids[view->count++] = slot;
            view->nameMask |= 1u << slot;
        }
    }
    if (view->count != ARRAY_COUNT(view->ids))
        for (u32 i = first; i < first + length; i++)
        {
            u32 slot = sTourneyTreeTrainerIds2[i];
            if (view->trainers[slot].isEliminated)
            {
                view->roundMask |= 1u << slot;
                if (view->trainers[slot].eliminatedAt >= round)
                {
                    view->ids[view->count++] = slot;
                    if (view->trainers[slot].eliminatedAt == round) view->moveMask |= 1u << slot;
                    else view->nameMask |= 1u << slot;
                }
            }
            if (view->count == ARRAY_COUNT(view->ids)) break;
        }
    for (u32 slot = 0; slot < ARRAY_COUNT(view->trainers); slot++)
        if (view->nameMask & (1u << slot))
        {
            u32 firstSlot, actor = view->trainers[slot].trainerId;
            if (!ADMG_TryDomeTrainerSlot(actor, &firstSlot)) return FALSE;
            if (actor == TRAINER_PLAYER)
            {
                view->playerNameUsed = TRUE;
                if (!ADMG_TryValidateTrainerText(view->playerName, sizeof(view->playerName), actor, NULL)) return FALSE;
            }
        }
    return ADMG_DomeMatchCurrent(view);
}

#define ADMG_DOME_MATCH_VALUE(view, expression) \
    ({ __typeof__(expression) admg_value = (expression); \
       if (!ADMG_DomeMatchCurrent(view)) goto ADMG_match_reject; admg_value; })
#define ADMG_DOME_MATCH_WRITE(view, mask, expression) \
    do { (view)->writtenStrings |= (mask); expression; \
         if (!ADMG_DomeMatchCurrent(view)) goto ADMG_match_reject; } while (0)

static int ADMG_RunDomeWinString(u8 matchNum, u8 *tournamentIds, struct ADMG_DomeMatchView *view)
{
    int i;
    u8 tournamentId;
    int winStringId = 0;
    int count = 0;

    // Get winners name
    for (i = sCompetitorRangeByMatch[matchNum][0]; i < sCompetitorRangeByMatch[matchNum][0] + sCompetitorRangeByMatch[matchNum][1]; i++)
    {
        tournamentId = sTourneyTreeTrainerIds2[i];
        if (!view->trainers[tournamentId].isEliminated)
        {
            view->writtenIds |= 1u << count;
            tournamentIds[count] = tournamentId;
            if (view->trainers[tournamentId].trainerId == TRAINER_PLAYER)
                ADMG_DOME_MATCH_WRITE(view, 1u, StringCopy(gStringVar1, view->save->playerName));
            else if (view->trainers[tournamentId].trainerId == TRAINER_FRONTIER_BRAIN)
                ADMG_DOME_MATCH_WRITE(view, 1u, CopyFrontierBrainTrainerName(gStringVar1));
            else
                ADMG_DOME_MATCH_WRITE(view, 1u, CopyDomeTrainerName(gStringVar1, view->trainers[tournamentId].trainerId));
            count++;
        }
    }

    // Neither trainer has been eliminated, battle hasn't occurred yet
    if (count == 2)
        return DOME_TEXT_NO_WINNER_YET;

    for (i = sCompetitorRangeByMatch[matchNum][0]; i < sCompetitorRangeByMatch[matchNum][0] + sCompetitorRangeByMatch[matchNum][1]; i++)
    {
        tournamentId = sTourneyTreeTrainerIds2[i];

        if (view->trainers[tournamentId].isEliminated
            && view->trainers[tournamentId].eliminatedAt >= sCompetitorRangeByMatch[matchNum][2])
        {
            view->writtenIds |= 1u << count;
            tournamentIds[count] = tournamentId;
            count++;

            if (view->trainers[tournamentId].eliminatedAt == sCompetitorRangeByMatch[matchNum][2])
            {
                // Set initial winStringId offset
                ADMG_DOME_MATCH_WRITE(view, 2u, StringCopy(gStringVar2, ADMG_DOME_MATCH_VALUE(view, GetMoveName(view->moves[tournamentId]))));
                winStringId = view->trainers[tournamentId].forfeited * 2; // (DOME_TEXT_WON_USING_MOVE - 1) or (DOME_TEXT_WON_ON_FORFEIT - 1)

                if (view->moves[tournamentId] == MOVE_NONE && view->trainers[tournamentId].forfeited == FALSE)
                    winStringId = DOME_TEXT_WON_NO_MOVES - 1;
            }
            else
            {
                if (view->trainers[tournamentId].trainerId == TRAINER_PLAYER)
                    ADMG_DOME_MATCH_WRITE(view, 1u, StringCopy(gStringVar1, view->save->playerName));
                else if (view->trainers[tournamentId].trainerId == TRAINER_FRONTIER_BRAIN)
                    ADMG_DOME_MATCH_WRITE(view, 1u, CopyFrontierBrainTrainerName(gStringVar1));
                else
                    ADMG_DOME_MATCH_WRITE(view, 1u, CopyDomeTrainerName(gStringVar1, view->trainers[tournamentId].trainerId));
            }
        }

        if (count == 2)
            break;
    }

    if (matchNum == DOME_TOURNAMENT_MATCHES_COUNT - 1)
        return winStringId + 2; // use DOME_TEXT_CHAMP_*
    else
        return winStringId + 1; // use DOME_TEXT_WON_*
ADMG_match_reject:
    view->failed = TRUE;
    return DOME_TEXT_NO_WINNER_YET;
}

static int BufferDomeWinString(u8 matchNum, u8 *tournamentIds)
{
    if (tournamentIds == NULL) { ADMG_StateAccessRejected(matchNum, __func__); return DOME_TEXT_NO_WINNER_YET; }
    struct ADMG_DomeMatchView view;
    if (!ADMG_TryPrepareDomeMatch(matchNum, &view)) return DOME_TEXT_NO_WINNER_YET;
    u8 oldIds[2], oldFirst[sizeof(gStringVar1)], oldSecond[sizeof(gStringVar2)];
    memcpy(oldIds, tournamentIds, sizeof(oldIds));
    memcpy(oldFirst, gStringVar1, sizeof(oldFirst)); memcpy(oldSecond, gStringVar2, sizeof(oldSecond));
    int result = ADMG_RunDomeWinString(matchNum, tournamentIds, &view);
    if (!view.failed && ADMG_DomeMatchCurrent(&view)) return result;
    for (u32 i = 0; i < ARRAY_COUNT(oldIds); i++) if (view.writtenIds & (1u << i)) tournamentIds[i] = oldIds[i];
    if (view.writtenStrings & 1u) memcpy(gStringVar1, oldFirst, sizeof(oldFirst));
    if (view.writtenStrings & 2u) memcpy(gStringVar2, oldSecond, sizeof(oldSecond));
    return DOME_TEXT_NO_WINNER_YET;
}

/* Preserve named BufferDomeWinString hooks; require both outputs only when used. */
static bool8 ADMG_TryBufferDomeMatchPair(u32 match, u8 *ids, int *text)
{
    if (ids == NULL || gSaveBlock2Ptr == NULL || match >= ARRAY_COUNT(sCompetitorRangeByMatch))
    { ADMG_StateAccessRejected(match, __func__); return FALSE; }
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    u64 generation = ADMG_BeginStateAccessCheck();
    u8 staged[2] = {0xFF, 0xFF};
    int result = BufferDomeWinString(match, staged);
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != save || staged[0] >= ARRAY_COUNT(DOME_TRAINERS) || staged[1] >= ARRAY_COUNT(DOME_TRAINERS)
        || (text != NULL && (u32)result >= ARRAY_COUNT(sBattleDomeWinTexts)))
    { ADMG_StateAccessRejected(match, __func__); return FALSE; }
    ids[0] = staged[0]; ids[1] = staged[1];
    if (text != NULL) *text = result;
    return TRUE;
}

static void DisplayMatchInfoOnCard(u8 flags, u8 matchNo)
{
    if (sInfoCard == NULL) { ADMG_StateAccessRejected(matchNo, __func__); return; }
    struct TextPrinterTemplate textPrinter;
    int tournamentIds[2];
    int trainerIds[2];
    bool32 lost[2];
    int i;
    int winStringId = 0;
    int arrId = 0;
    int windowId = 0;
    int x = 0, y = 0;
    u8 palSlot = 0;

    if (flags & CARD_ALTERNATE_SLOT)
        arrId = 2 * (FRONTIER_PARTY_SIZE + 1), windowId = NUM_INFO_CARD_WINDOWS, palSlot = 2;
    if (flags & MOVE_CARD_RIGHT)
        x = DISPLAY_WIDTH + 16;
    if (flags & MOVE_CARD_DOWN)
        y = DISPLAY_HEIGHT;
    if (flags & MOVE_CARD_LEFT)
        x = -(DISPLAY_WIDTH + 16);
    if (flags & MOVE_CARD_UP)
        y = -DISPLAY_HEIGHT;

    // Copy trainers information to handy arrays.
    if (!ADMG_TryBufferDomeMatchPair(matchNo, sInfoCard->tournamentIds, &winStringId)) return;
    struct ADMG_DomeCardView admgView;
    if (!ADMG_TryPrepareDomeCard(&admgView, sInfoCard->tournamentIds, NUM_INFOCARD_TRAINERS, FALSE)) return;
    for (i = 0; i < NUM_INFOCARD_TRAINERS; i++)
    {
        tournamentIds[i] = admgView.slots[i];
        trainerIds[i] = DOME_TRAINERS[tournamentIds[i]].trainerId;
        if (DOME_TRAINERS[tournamentIds[i]].eliminatedAt <= sCompetitorRangeByMatch[matchNo][2]
            && DOME_TRAINERS[tournamentIds[i]].isEliminated)
            lost[i] = TRUE;
        else
            lost[i] = FALSE;
    }

    // Draw left trainer sprite.
    if (trainerIds[0] == TRAINER_PLAYER)
        sInfoCard->spriteIds[arrId] = CreateTrainerPicSprite(ADMG_DOME_INFO_VALUE(PlayerGenderToFrontTrainerPicId(gSaveBlock2Ptr->playerGender)), TRUE, x + 48, y + 88, palSlot + 12, TAG_NONE);
    else if (trainerIds[0] == TRAINER_FRONTIER_BRAIN)
        sInfoCard->spriteIds[arrId] = CreateTrainerPicSprite(ADMG_DOME_INFO_VALUE(GetFrontierBrainTrainerPicIndex()), TRUE, x + 48, y + 88, palSlot + 12, TAG_NONE);
    else
        sInfoCard->spriteIds[arrId] = CreateTrainerPicSprite(ADMG_DOME_INFO_VALUE(GetFrontierTrainerFrontSpriteId(trainerIds[0])), TRUE, x + 48, y + 88, palSlot + 12, TAG_NONE);

    if (flags & MOVE_CARD)
        gSprites[sInfoCard->spriteIds[arrId]].invisible = TRUE;
    if (lost[0])
        gSprites[sInfoCard->spriteIds[arrId]].oam.paletteNum = 3;

    // Draw right trainer sprite.
    if (trainerIds[1] == TRAINER_PLAYER)
        sInfoCard->spriteIds[1 + arrId] = CreateTrainerPicSprite(ADMG_DOME_INFO_VALUE(PlayerGenderToFrontTrainerPicId(gSaveBlock2Ptr->playerGender)), TRUE, x + 192, y + 88, palSlot + 13, TAG_NONE);
    else if (trainerIds[1] == TRAINER_FRONTIER_BRAIN)
        sInfoCard->spriteIds[1 + arrId] = CreateTrainerPicSprite(ADMG_DOME_INFO_VALUE(GetFrontierBrainTrainerPicIndex()), TRUE, x + 192, y + 88, palSlot + 13, TAG_NONE);
    else
        sInfoCard->spriteIds[1 + arrId] = CreateTrainerPicSprite(ADMG_DOME_INFO_VALUE(GetFrontierTrainerFrontSpriteId(trainerIds[1])), TRUE, x + 192, y + 88, palSlot + 13, TAG_NONE);

    if (flags & MOVE_CARD)
        gSprites[sInfoCard->spriteIds[1 + arrId]].invisible = TRUE;
    if (lost[1])
        gSprites[sInfoCard->spriteIds[1 + arrId]].oam.paletteNum = 3;

    // Draw left trainer's Pokémon icons.
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        if (trainerIds[0] == TRAINER_PLAYER)
        {
            sInfoCard->spriteIds[2 + i + arrId] = CreateMonIcon(ADMG_DOME_INFO_VALUE(ADMG_DomeCardSpecies(&admgView, tournamentIds[0], i)),
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sLeftTrainerMonX[i],
                                                                  y + sLeftTrainerMonY[i],
                                                                  0, 0);
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].oam.priority = 0;
        }
        else if (trainerIds[0] == TRAINER_FRONTIER_BRAIN)
        {
            sInfoCard->spriteIds[2 + i + arrId] = CreateMonIcon(ADMG_DOME_INFO_VALUE(ADMG_DomeCardSpecies(&admgView, tournamentIds[0], i)),
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sLeftTrainerMonX[i],
                                                                  y + sLeftTrainerMonY[i],
                                                                  0, 0);
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].oam.priority = 0;
        }
        else
        {
            sInfoCard->spriteIds[2 + i + arrId] = CreateMonIcon(ADMG_DOME_INFO_VALUE(ADMG_DomeCardSpecies(&admgView, tournamentIds[0], i)),
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sLeftTrainerMonX[i],
                                                                  y + sLeftTrainerMonY[i],
                                                                  0, 0);
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].oam.priority = 0;
        }

        if (flags & MOVE_CARD)
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].invisible = TRUE;
        if (lost[0])
        {
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].oam.paletteNum = 3;
            gSprites[sInfoCard->spriteIds[2 + i + arrId]].sMonIconStill = TRUE;
        }
    }

    // Draw right trainer's Pokémon icons.
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        if (trainerIds[1] == TRAINER_PLAYER)
        {
            sInfoCard->spriteIds[5 + i + arrId] = CreateMonIcon(ADMG_DOME_INFO_VALUE(ADMG_DomeCardSpecies(&admgView, tournamentIds[1], i)),
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sRightTrainerMonX[i],
                                                                  y + sRightTrainerMonY[i],
                                                                  0, 0);
            gSprites[sInfoCard->spriteIds[5 + i + arrId]].oam.priority = 0;
        }
        else if (trainerIds[1] == TRAINER_FRONTIER_BRAIN)
        {
            sInfoCard->spriteIds[5 + i + arrId] = CreateMonIcon(ADMG_DOME_INFO_VALUE(ADMG_DomeCardSpecies(&admgView, tournamentIds[1], i)),
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sRightTrainerMonX[i],
                                                                  y + sRightTrainerMonY[i],
                                                                  0, 0);
            gSprites[sInfoCard->spriteIds[5 + i + arrId]].oam.priority = 0;
        }
        else
        {
            sInfoCard->spriteIds[5 + i + arrId] = CreateMonIcon(ADMG_DOME_INFO_VALUE(ADMG_DomeCardSpecies(&admgView, tournamentIds[1], i)),
                                                                  SpriteCB_MonIconDomeInfo,
                                                                  x | sRightTrainerMonX[i],
                                                                  y + sRightTrainerMonY[i],
                                                                  0, 0);
            gSprites[sInfoCard->spriteIds[5 + i + arrId]].oam.priority = 0;
        }

        if (flags & MOVE_CARD)
            gSprites[sInfoCard->spriteIds[5 + i + arrId]].invisible = TRUE;
        if (lost[1])
        {
            gSprites[sInfoCard->spriteIds[5 + i + arrId]].oam.paletteNum = 3;
            gSprites[sInfoCard->spriteIds[5 + i + arrId]].sMonIconStill = TRUE;
        }
    }

    // Print the win string (or 'Let the battle begin!').
    textPrinter.type = WINDOW_TEXT_PRINTER;
    textPrinter.x = 0;
    textPrinter.y = 2;
    textPrinter.currentX = textPrinter.x;
    textPrinter.currentY = textPrinter.y;
    textPrinter.letterSpacing = 0;
    textPrinter.lineSpacing = 0;
    textPrinter.color.accent = TEXT_COLOR_TRANSPARENT;
    textPrinter.color.foreground = TEXT_DYNAMIC_COLOR_5;
    textPrinter.color.background = TEXT_COLOR_TRANSPARENT;
    textPrinter.color.shadow = TEXT_DYNAMIC_COLOR_4;
    StringExpandPlaceholders(gStringVar4, sBattleDomeWinTexts[winStringId]);
    textPrinter.currentChar = gStringVar4;
    textPrinter.windowId = windowId + WIN_MATCH_WIN_TEXT;
    textPrinter.fontId = FONT_NORMAL;
    PutWindowTilemap(windowId + WIN_MATCH_WIN_TEXT);
    CopyWindowToVram(windowId + WIN_MATCH_WIN_TEXT, COPYWIN_FULL);
    textPrinter.currentX = 0;
    textPrinter.currentY = textPrinter.y = 0;
    AddTextPrinter(&textPrinter, 0, NULL);

    // Print left trainer's name.
    if (trainerIds[0] == TRAINER_PLAYER)
        StringCopy(gStringVar1, gSaveBlock2Ptr->playerName);
    else if (trainerIds[0] == TRAINER_FRONTIER_BRAIN)
        ADMG_DOME_INFO_ACTION(CopyFrontierBrainTrainerName(gStringVar1));
    else
        ADMG_DOME_INFO_ACTION(CopyDomeTrainerName(gStringVar1, trainerIds[0]));

    textPrinter.fontId = FONT_SHORT;
    textPrinter.letterSpacing = 2;
    textPrinter.currentChar = gStringVar1;
    textPrinter.windowId = windowId + WIN_MATCH_TRAINER_NAME_LEFT;
    textPrinter.currentX = GetStringCenterAlignXOffsetWithLetterSpacing(textPrinter.fontId, textPrinter.currentChar, 0x40, textPrinter.letterSpacing);
    textPrinter.currentY = textPrinter.y = 2;
    PutWindowTilemap(windowId + WIN_MATCH_TRAINER_NAME_LEFT);
    CopyWindowToVram(windowId + WIN_MATCH_TRAINER_NAME_LEFT, COPYWIN_FULL);
    AddTextPrinter(&textPrinter, 0, NULL);

    // Print right trainer's name.
    if (trainerIds[1] == TRAINER_PLAYER)
        StringCopy(gStringVar1, gSaveBlock2Ptr->playerName);
    else if (trainerIds[1] == TRAINER_FRONTIER_BRAIN)
        ADMG_DOME_INFO_ACTION(CopyFrontierBrainTrainerName(gStringVar1));
    else
        ADMG_DOME_INFO_ACTION(CopyDomeTrainerName(gStringVar1, trainerIds[1]));

    textPrinter.currentChar = gStringVar1;
    textPrinter.windowId = windowId + WIN_MATCH_TRAINER_NAME_RIGHT;
    textPrinter.currentX = GetStringCenterAlignXOffsetWithLetterSpacing(textPrinter.fontId, textPrinter.currentChar, 0x40, textPrinter.letterSpacing);
    textPrinter.currentY = textPrinter.y = 2;
    PutWindowTilemap(windowId + WIN_MATCH_TRAINER_NAME_RIGHT);
    CopyWindowToVram(windowId + WIN_MATCH_TRAINER_NAME_RIGHT, COPYWIN_FULL);
    AddTextPrinter(&textPrinter, 0, NULL);

    // Print match number.
    textPrinter.letterSpacing = 0;
    textPrinter.currentChar = sBattleDomeMatchNumberTexts[matchNo];
    textPrinter.windowId = windowId + WIN_MATCH_NUMBER;
    textPrinter.currentX = GetStringCenterAlignXOffsetWithLetterSpacing(textPrinter.fontId, textPrinter.currentChar, 0xA0, textPrinter.letterSpacing);
    textPrinter.currentY = textPrinter.y = 2;
    PutWindowTilemap(windowId + WIN_MATCH_NUMBER);
    CopyWindowToVram(windowId + WIN_MATCH_NUMBER, COPYWIN_FULL);
    AddTextPrinter(&textPrinter, 0, NULL);

    return;
ADMG_info_reject:
}

#undef ADMG_DOME_INFO_VALUE
#undef ADMG_DOME_INFO_ACTION


static void ShowDomeTourneyTree(void)
{
    u8 taskId = CreateTask(Task_ShowTourneyTree, 0);
    gTasks[taskId].tState = 0;
    gTasks[taskId].tNotInteractive = FALSE;
    gTasks[taskId].data[2] = 2;
    gTasks[taskId].tIsPrevTourneyTree = FALSE;
    SetMainCallback2(CB2_TourneyTree);
}

// To show the results of the last tourney on the computer in the lobby
static void ShowPreviousDomeTourneyTree(void)
{
    u8 taskId;

    SetFacilityTrainerAndMonPtrs();
    gSaveBlock2Ptr->frontier.lvlMode = gSaveBlock2Ptr->frontier.domeLvlMode - 1;
    gSaveBlock2Ptr->frontier.curChallengeBattleNum = DOME_FINAL;
    taskId = CreateTask(Task_ShowTourneyTree, 0);
    gTasks[taskId].tState = 0;
    gTasks[taskId].tNotInteractive = FALSE;
    gTasks[taskId].data[2] = 2;
    gTasks[taskId].tIsPrevTourneyTree = TRUE;
    SetMainCallback2(CB2_TourneyTree);
}

// Task states for Task_HandleTourneyTreeInput
#define STATE_FADE_IN               0
#define STATE_WAIT_FADE             1
#define STATE_GET_INPUT             2
#define STATE_SHOW_INFOCARD_TRAINER 3
#define STATE_SHOW_INFOCARD_MATCH   5
#define STATE_CLOSE_TOURNEY_TREE    7

static void Task_HandleTourneyTreeInput(u8 taskId)
{
    if (taskId >= NUM_TASKS) { ADMG_StateAccessRejected(taskId, __func__); return; }
    u32 admgFreeSlots, admgTrainerSlot;

    u8 newTaskId = 0;
    int spriteId = gTasks[taskId].data[1];

    switch (gTasks[taskId].tState)
    {
    case STATE_FADE_IN:
        if (!gPaletteFade.active)
        {
            if ((u32)spriteId >= ARRAY_COUNT(sTourneyTreeCursorMovementMap))
            { ADMG_StateAccessRejected(spriteId, __func__); return; }
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
            gTasks[taskId].tState = STATE_WAIT_FADE;
            StartSpriteAnim(&gSprites[spriteId], 1);
        }
        break;
    case STATE_WAIT_FADE:
        if (!gPaletteFade.active)
            gTasks[taskId].tState = STATE_GET_INPUT;
        break;
    case STATE_GET_INPUT:
        switch (UpdateTourneyTreeCursor(taskId))
        {
        case TOURNEY_TREE_SELECTED_CLOSE:
        default:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            gTasks[taskId].tState = STATE_CLOSE_TOURNEY_TREE;
            break;
        case TOURNEY_TREE_NO_SELECTION:
            break;
        case TOURNEY_TREE_SELECTED_TRAINER:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            gTasks[taskId].tState = STATE_SHOW_INFOCARD_TRAINER;
            break;
        case TOURNEY_TREE_SELECTED_MATCH:
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            gTasks[taskId].tState = STATE_SHOW_INFOCARD_MATCH;
            break;
        }
        break;
    case STATE_SHOW_INFOCARD_TRAINER:
        if (!gPaletteFade.active)
        {
            if (!ADMG_TryDomeCardTrainerPosition(spriteId, &admgTrainerSlot)) return;
            if (gSaveBlock2Ptr == NULL) { ADMG_StateAccessRejected(admgTrainerSlot, __func__); return; }
            const struct BattleFrontierTrainer *admgTrainer;
            if (!ADMG_TryDomeNameIdentity(DOME_TRAINERS[admgTrainerSlot].trainerId, &admgTrainer)) return;
            admgFreeSlots = ADMG_DomeFreeTaskSlots();
            if (admgFreeSlots == 0) return;
            FreeAllWindowBuffers();
            ScanlineEffect_Stop();
            FREE_AND_SET_NULL(sTilemapBuffer);
            if (!ADMG_TryCreateDomeTask(Task_ShowTourneyInfoCard, admgFreeSlots, &newTaskId)) return;
            gTasks[newTaskId].tState = 0;
            gTasks[newTaskId].tTournamentId = admgTrainerSlot;
            gTasks[newTaskId].tMode = INFOCARD_TRAINER;
            gTasks[newTaskId].tPrevTaskId = taskId;

            gTasks[taskId].tState = STATE_SHOW_INFOCARD_TRAINER + 1;
            /* Position is initialized by the card builder after allocation. */
        }
        break;
    case STATE_SHOW_INFOCARD_TRAINER + 1:
        break;
    case STATE_SHOW_INFOCARD_MATCH:
        if (!gPaletteFade.active)
        {
            if (spriteId < DOME_TOURNAMENT_TRAINERS_COUNT
                || (u32)(spriteId - DOME_TOURNAMENT_TRAINERS_COUNT) >= DOME_TOURNAMENT_MATCHES_COUNT)
            { ADMG_StateAccessRejected(spriteId, __func__); return; }
            admgFreeSlots = ADMG_DomeFreeTaskSlots();
            if (admgFreeSlots == 0) return;
            FreeAllWindowBuffers();
            ScanlineEffect_Stop();
            FREE_AND_SET_NULL(sTilemapBuffer);
            if (!ADMG_TryCreateDomeTask(Task_ShowTourneyInfoCard, admgFreeSlots, &newTaskId)) return;
            gTasks[newTaskId].tState = 0;
            gTasks[newTaskId].tTournamentId = spriteId - DOME_TOURNAMENT_TRAINERS_COUNT;
            gTasks[newTaskId].tMode = INFOCARD_MATCH;
            gTasks[newTaskId].tPrevTaskId = taskId;

            gTasks[taskId].tState = STATE_SHOW_INFOCARD_MATCH + 1;
        }
        break;
    case STATE_SHOW_INFOCARD_MATCH + 1:
        break;
    case STATE_CLOSE_TOURNEY_TREE:
        if (!gPaletteFade.active)
        {
            FreeAllWindowBuffers();
            ScanlineEffect_Stop();
            FREE_AND_SET_NULL(sTilemapBuffer);
            SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
            DestroyTask(gTasks[taskId].data[7]);
            DestroyTask(taskId);
        }
        break;
    }
}

// undefine task states for Task_HandleTourneyTreeInput
#undef STATE_FADE_IN
#undef STATE_WAIT_FADE
#undef STATE_GET_INPUT
#undef STATE_SHOW_INFOCARD_TRAINER
#undef STATE_SHOW_INFOCARD_MATCH
#undef STATE_CLOSE_TOURNEY_TREE


#define MOVE_DIR_UP    0
#define MOVE_DIR_DOWN  1
#define MOVE_DIR_LEFT  2
#define MOVE_DIR_RIGHT 3
#define MOVE_DIR_NONE  4

// Move the tourney tree cursor
// The 'cursor' is actually just which button sprite is currently doing the 'selected' animation
static u8 UpdateTourneyTreeCursor(u8 taskId)
{
    if (taskId >= NUM_TASKS)
    { ADMG_StateAccessRejected(taskId, __func__); return TOURNEY_TREE_NO_SELECTION; }

    u8 selection = TOURNEY_TREE_NO_SELECTION;
    int direction = MOVE_DIR_NONE;
    int tourneyTreeCursorSpriteId = gTasks[taskId].data[1];
    int roundId;

    if (gMain.newKeys == B_BUTTON || (JOY_NEW(A_BUTTON) && tourneyTreeCursorSpriteId == TOURNEY_TREE_CLOSE_BUTTON))
    {
        PlaySE(SE_SELECT);
        selection = TOURNEY_TREE_SELECTED_CLOSE;
    }
    else if (JOY_NEW(A_BUTTON))
    {
        if ((u32)tourneyTreeCursorSpriteId >= ARRAY_COUNT(sTourneyTreeCursorMovementMap))
        { ADMG_StateAccessRejected(tourneyTreeCursorSpriteId, __func__); return TOURNEY_TREE_NO_SELECTION; }
        if (tourneyTreeCursorSpriteId < DOME_TOURNAMENT_TRAINERS_COUNT)
        {
            PlaySE(SE_SELECT);
            selection = TOURNEY_TREE_SELECTED_TRAINER;
        }
        else
        {
            PlaySE(SE_SELECT);
            selection = TOURNEY_TREE_SELECTED_MATCH;
        }
    }
    else
    {
        if (gMain.newKeys != DPAD_UP && gMain.newKeys != DPAD_DOWN
            && gMain.newKeys != DPAD_LEFT && gMain.newKeys != DPAD_RIGHT) return selection;
        if (gSaveBlock2Ptr == NULL || (u32)tourneyTreeCursorSpriteId >= ARRAY_COUNT(sTourneyTreeCursorMovementMap))
        { ADMG_StateAccessRejected(tourneyTreeCursorSpriteId, __func__); return TOURNEY_TREE_NO_SELECTION; }
        roundId = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
        if ((u32)roundId >= ARRAY_COUNT(sTourneyTreeCursorMovementMap[0]))
        { ADMG_StateAccessRejected(roundId, __func__); return TOURNEY_TREE_NO_SELECTION; }
        if (gMain.newKeys == DPAD_UP && sTourneyTreeCursorMovementMap[tourneyTreeCursorSpriteId][roundId][0] != 0xFF)
            direction = MOVE_DIR_UP;
        else if (gMain.newKeys == DPAD_DOWN && sTourneyTreeCursorMovementMap[tourneyTreeCursorSpriteId][roundId][1] != 0xFF)
            direction = MOVE_DIR_DOWN;
        else if (gMain.newKeys == DPAD_LEFT && sTourneyTreeCursorMovementMap[tourneyTreeCursorSpriteId][roundId][2] != 0xFF)
            direction = MOVE_DIR_LEFT;
        else if (gMain.newKeys == DPAD_RIGHT && sTourneyTreeCursorMovementMap[tourneyTreeCursorSpriteId][roundId][3] != 0xFF)
            direction = MOVE_DIR_RIGHT;
    }

    if (direction != MOVE_DIR_NONE)
    {
        PlaySE(SE_SELECT);
        StartSpriteAnim(&gSprites[tourneyTreeCursorSpriteId], 0); // Do unselected sprite anim
        tourneyTreeCursorSpriteId = sTourneyTreeCursorMovementMap[tourneyTreeCursorSpriteId][roundId][direction];
        StartSpriteAnim(&gSprites[tourneyTreeCursorSpriteId], 1); // Do selected sprite anim
        gTasks[taskId].data[1] = tourneyTreeCursorSpriteId;
    }

    return selection;
}

#undef MOVE_DIR_UP
#undef MOVE_DIR_DOWN
#undef MOVE_DIR_LEFT
#undef MOVE_DIR_RIGHT
#undef MOVE_DIR_NONE

// Shows the results of the just-completed round for the current tourney
static void ShowNonInteractiveDomeTourneyTree(void)
{
    u8 taskId = CreateTask(Task_ShowTourneyTree, 0);
    gTasks[taskId].tState = 0;
    gTasks[taskId].tNotInteractive = TRUE;
    gTasks[taskId].data[2] = 2;
    gTasks[taskId].tIsPrevTourneyTree = FALSE;
    SetMainCallback2(CB2_TourneyTree);
}

/* Each round owns outcome fields, not trainer identities or raw mon words. */
struct ADMG_DomeRoundFrame
{
    struct SaveBlock2 *save;
    u64 generation;
    struct BattleDomeTrainer trainers[DOME_TOURNAMENT_TRAINERS_COUNT];
    u16 moves[DOME_TOURNAMENT_TRAINERS_COUNT];
    u8 first[sizeof(gStringVar1)], second[sizeof(gStringVar2)];
    const struct ADMG_DomeSelectionInput *left, *right;
    const struct BattleFrontierTrainer *previousTrainers, *ownedTrainers;
    const struct TrainerMon *previousMons, *ownedMons;
    u32 writtenMask, forfeitMask;
    bool8 textUsed;
};

static bool8 ADMG_DomeRoundCurrent(const struct ADMG_DomeRoundFrame *frame)
{
    if (!ADMG_EndStateAccessCheck(frame->generation)) return FALSE;
    if (frame->save == NULL || gSaveBlock2Ptr != frame->save)
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    if (frame->left != NULL && !ADMG_DomeSelectionCurrent(frame->left)) return FALSE;
    if (frame->right != NULL && !ADMG_DomeSelectionCurrent(frame->right)) return FALSE;
    return TRUE;
}

static bool8 ADMG_TryPrepareDomeRound(struct ADMG_DomeRoundFrame *frame)
{
    if (gSaveBlock2Ptr == NULL) { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    *frame = (struct ADMG_DomeRoundFrame){ .save = gSaveBlock2Ptr,
        .generation = ADMG_BeginStateAccessCheck(),
        .previousTrainers = gFacilityTrainers, .previousMons = gFacilityTrainerMons };
    memcpy(frame->trainers, DOME_TRAINERS, sizeof(frame->trainers));
    memcpy(frame->moves, frame->save->frontier.domeWinningMoves, sizeof(frame->moves));
    memcpy(frame->first, gStringVar1, sizeof(frame->first)); memcpy(frame->second, gStringVar2, sizeof(frame->second));
    return ADMG_DomeRoundCurrent(frame);
}

static void ADMG_RestoreDomeRound(const struct ADMG_DomeRoundFrame *frame)
{
    for (u32 i = 0; i < ARRAY_COUNT(frame->trainers); i++)
    {
        if (frame->writtenMask & (1u << i))
        {
            frame->save->frontier.domeTrainers[i].isEliminated = frame->trainers[i].isEliminated;
            frame->save->frontier.domeTrainers[i].eliminatedAt = frame->trainers[i].eliminatedAt;
            frame->save->frontier.domeWinningMoves[i] = frame->moves[i];
        }
        if (frame->forfeitMask & (1u << i))
            frame->save->frontier.domeTrainers[i].forfeited = frame->trainers[i].forfeited;
    }
    if (frame->textUsed)
    { memcpy(gStringVar1, frame->first, sizeof(frame->first)); memcpy(gStringVar2, frame->second, sizeof(frame->second)); }
    if (frame->ownedTrainers != NULL && gFacilityTrainers == frame->ownedTrainers) gFacilityTrainers = frame->previousTrainers;
    if (frame->ownedMons != NULL && gFacilityTrainerMons == frame->ownedMons) gFacilityTrainerMons = frame->previousMons;
}

static u16 ADMG_DomeRoundWinning(struct ADMG_DomeRoundFrame *frame, int winner, int loser, u8 round)
{
    // Winning-move publication can legitimately replace the scoring table.
    frame->left = frame->right = NULL;
    if (round > DOME_ROUND2) frame->textUsed = TRUE;
    u16 result = GetWinningMove(winner, loser, round);
    frame->ownedTrainers = gFacilityTrainers; frame->ownedMons = gFacilityTrainerMons;
    return result;
}

static void ADMG_DomeRoundChildren(struct ADMG_DomeRoundFrame *frame, u8 round)
{
    // The existing named round owns all descendant elimination/move outcomes.
    frame->writtenMask = (1u << DOME_TOURNAMENT_TRAINERS_COUNT) - 1;
    if (round > DOME_ROUND2) frame->textUsed = TRUE;
    DecideRoundWinners(round);
    frame->ownedTrainers = gFacilityTrainers; frame->ownedMons = gFacilityTrainerMons;
}

#define ADMG_DOME_ROUND_VALUE(frame, expression) \
    ({ __typeof__(expression) admg_value = (expression); \
       if (!ADMG_DomeRoundCurrent(frame)) goto ADMG_round_reject; admg_value; })
#define ADMG_DOME_ROUND_ACTION(frame, expression) \
    do { expression; if (!ADMG_DomeRoundCurrent(frame)) goto ADMG_round_reject; } while (0)

static bool8 ADMG_RunResolveDomeRound(struct ADMG_DomeRoundFrame *frame)
{
    int i;
    u32 slot;
    u32 identity = gSpecialVar_0x8005 == DOME_PLAYER_WON_MATCH ? TRAINER_BATTLE_PARAM.opponentA : TRAINER_PLAYER;
    if (!(gSpecialVar_0x8005 == DOME_PLAYER_WON_MATCH
        ? ADMG_TryDomeOpponentSlot(identity, &slot) : ADMG_TryDomeTrainerSlot(identity, &slot))) return FALSE;
    if (!ADMG_DomeRoundCurrent(frame)) return FALSE;
    u32 round = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
    if (round >= DOME_ROUNDS_COUNT)
    { ADMG_StateAccessRejected(round, __func__); return FALSE; }

    frame->writtenMask |= 1u << slot;
    if (gSpecialVar_0x8005 == DOME_PLAYER_WON_MATCH)
    {
        DOME_TRAINERS[slot].isEliminated = TRUE;
        DOME_TRAINERS[slot].eliminatedAt = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
        gSaveBlock2Ptr->frontier.domeWinningMoves[slot] = gBattleResults.lastUsedMovePlayer;

        // If the player's match was the final one, no NPC vs NPC matches to decide
        if (gSaveBlock2Ptr->frontier.curChallengeBattleNum < DOME_FINAL)
            ADMG_DOME_ROUND_ACTION(frame, ADMG_DomeRoundChildren(frame, gSaveBlock2Ptr->frontier.curChallengeBattleNum));
    }
    else // DOME_PLAYER_LOST_MATCH or DOME_PLAYER_RETIRED
    {
        DOME_TRAINERS[slot].isEliminated = TRUE;
        DOME_TRAINERS[slot].eliminatedAt = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
        gSaveBlock2Ptr->frontier.domeWinningMoves[slot] = gBattleResults.lastUsedMoveOpponent;

        if (gBattleOutcome == B_OUTCOME_FORFEITED || gSpecialVar_0x8005 == DOME_PLAYER_RETIRED)
        {
            frame->forfeitMask |= 1u << slot;
            DOME_TRAINERS[slot].forfeited = TRUE;
        }

        // Player lost, decide remaining outcome of tournament
        for (i = gSaveBlock2Ptr->frontier.curChallengeBattleNum; i < DOME_ROUNDS_COUNT; i++)
            ADMG_DOME_ROUND_ACTION(frame, ADMG_DomeRoundChildren(frame, i));
    }
    return TRUE;
ADMG_round_reject:
    return FALSE;
}

static void ResolveDomeRoundWinners(void)
{
    struct ADMG_DomeRoundFrame frame;
    if (!ADMG_TryPrepareDomeRound(&frame)) return;
    if (!ADMG_RunResolveDomeRound(&frame) || !ADMG_DomeRoundCurrent(&frame)) ADMG_RestoreDomeRound(&frame);
}

// Decides the winning move of an NPC vs NPC match
/* Winning-move roles differ: a Brain winner uses Brain moves; every loser
 * retains the original raw facility-mon semantics. No campaign selects identity. */
struct ADMG_DomeWinningInput
{
    struct ADMG_DomeSelectionInput winner;
    u32 loser;
    u16 loserActor, loserIndices[FRONTIER_PARTY_SIZE];
    const struct TrainerMon *loserRows[FRONTIER_PARTY_SIZE];
    const struct TrainerMon *table;
    bool8 failed, textUsed;
};

static bool8 ADMG_DomeWinningCurrent(const struct ADMG_DomeWinningInput *input)
{
    if (!ADMG_DomeSelectionCurrent(&input->winner)) return FALSE;
    if (gFacilityTrainerMons != input->table
        || DOME_TRAINERS[input->loser].trainerId != input->loserActor)
    { ADMG_StateAccessRejected(input->loser, __func__); return FALSE; }
    for (u32 i = 0; i < ARRAY_COUNT(input->loserIndices); i++)
        if (DOME_MONS[input->loser][i] != input->loserIndices[i])
        { ADMG_StateAccessRejected(i, __func__); return FALSE; }
    return TRUE;
}

static bool8 ADMG_TryPrepareDomeWinning(u32 winner, u32 loser, struct ADMG_DomeWinningInput *input)
{
    *input = (struct ADMG_DomeWinningInput){ .loser = loser, .table = gFacilityTrainerMons };
    if (!ADMG_TryPrepareDomeSelection(winner, &input->winner)
        || !ADMG_TryDomeNpcAtSlot(loser, &input->loserActor)) return FALSE;
    for (u32 i = 0; i < ARRAY_COUNT(input->loserRows); i++)
    {
        input->loserIndices[i] = DOME_MONS[loser][i];
        if (!ADMG_TryDomeMonRowFromTable(loser, i, input->table, &input->loserRows[i])) return FALSE;
    }
    return ADMG_DomeWinningCurrent(input);
}

static u32 ADMG_DomeWinningOpponent(struct ADMG_DomeWinningInput *input, u32 winner, u32 round)
{
    input->textUsed = TRUE;
    u32 slot = GetOpposingNPCTournamentIdByRound(winner, round);
    if (!ADMG_DomeWinningCurrent(input)) return 0;
    if (slot >= ARRAY_COUNT(DOME_TRAINERS))
    { ADMG_StateAccessRejected(slot, __func__); return 0; }
    return slot;
}

#define ADMG_DOME_WIN_VALUE(input, expression) \
    ({ __typeof__(expression) admg_value = (expression); \
       if (!ADMG_DomeWinningCurrent(input)) goto ADMG_winning_reject; admg_value; })

static enum Move ADMG_RunDomeWinningMove(int winnerTournamentId, int loserTournamentId, u8 roundId, struct ADMG_DomeWinningInput *input)
{
    int i, j, k;
    int moveScores[MAX_MON_MOVES * FRONTIER_PARTY_SIZE];
    enum Move moves[MAX_MON_MOVES * FRONTIER_PARTY_SIZE] = {MOVE_NONE};
    u16 bestScore = 0;
    u16 bestId = 0;
    int movePower = 0;


    // Calc move points of all 4 moves for all 3 Pokémon hitting all 3 target mons.
    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        for (j = 0; j < MAX_MON_MOVES; j++)
        {
            u32 moveIndex = i * MAX_MON_MOVES + j;
            enum Move move;

            moveScores[moveIndex] = 0;
            if (input->winner.actor == TRAINER_FRONTIER_BRAIN)
                move = ADMG_DOME_WIN_VALUE(input, GetFrontierBrainMonMove(i, j));
            else
                move = input->winner.rows[i]->moves[j];
            moves[moveIndex] = move;

            movePower = ADMG_DOME_WIN_VALUE(input, GetMovePower(move));
            if (ADMG_DOME_WIN_VALUE(input, IsBattleMoveStatus(move)))
                movePower = 40;
            else if (movePower == 1)
                movePower = 60;
            else if (ADMG_DOME_WIN_VALUE(input, GetConfig(B_EXPLOSION_DEFENSE)) < GEN_5 && ADMG_DOME_WIN_VALUE(input, IsExplosionMove(move)))
                movePower /= 2;

            for (k = 0; k < FRONTIER_PARTY_SIZE; k++)
            {
                u32 personality = 0;
                enum Species targetSpecies = SPECIES_NONE;
                enum Ability targetAbility = ABILITY_NONE;
                uq4_12_t typeMultiplier = 0;
                do
                {
                    personality = ADMG_DOME_WIN_VALUE(input, Random32());
                } while (input->loserRows[k]->nature != ADMG_DOME_WIN_VALUE(input, GetNatureFromPersonality(personality)));

                targetSpecies = input->loserRows[k]->species;

                if (personality & 1)
                    targetAbility = ADMG_DOME_WIN_VALUE(input, GetSpeciesAbility(targetSpecies, 1));
                else
                    targetAbility = ADMG_DOME_WIN_VALUE(input, GetSpeciesAbility(targetSpecies, 0));

                typeMultiplier = ADMG_DOME_WIN_VALUE(input, CalcPartyMonTypeEffectivenessMultiplier(moves[i * 4 + j], targetSpecies, targetAbility));
                if (typeMultiplier == UQ_4_12(0))
                    moveScores[moveIndex] += 0;
                else if (typeMultiplier >= UQ_4_12(2.0))
                    moveScores[moveIndex] += movePower * 2;
                else if (typeMultiplier <= UQ_4_12(0.5))
                    moveScores[moveIndex] += movePower / 2;
                else
                    moveScores[moveIndex] += movePower;
            }

            if (bestScore < moveScores[moveIndex])
            {
                bestId = moveIndex;
                bestScore = moveScores[moveIndex];
            }
            else if (bestScore == moveScores[moveIndex])
            {
                if (moves[bestId] < move) // Why not use (Random() & 1) instead of promoting moves with a higher id?
                    bestId = moveIndex;
            }
        }
    }

    j = bestId;
    do
    {
        for (i = 0; i < roundId - 1; i++)
        {
            if (gSaveBlock2Ptr->frontier.domeWinningMoves[ADMG_DOME_WIN_VALUE(input, ADMG_DomeWinningOpponent(input, winnerTournamentId, i))] == moves[j])
                break;
        }
        if (i != roundId - 1)
        {
            moveScores[j] = 0;
            bestScore = 0;
            j = 0;
            for (k = 0; k < MAX_MON_MOVES * FRONTIER_PARTY_SIZE; k++)
                j += moveScores[k];
            if (j == 0)
                break;
            j = 0;
            for (k = 0; k < MAX_MON_MOVES * FRONTIER_PARTY_SIZE; k++)
            {
                if (bestScore < moveScores[k])
                {
                    j = k;
                    bestScore = moveScores[k];
                }
                else if (bestScore == moveScores[k] && moves[j] < moves[k]) // Yes, these conditions are redundant
                {
                    j = k;
                    bestScore = moveScores[k];
                }
            }
        }
    } while (i != roundId - 1);

    if (moveScores[j] == 0)
        j = bestId;

    return moves[j];
ADMG_winning_reject:
    input->failed = TRUE;
    return MOVE_NONE;
}

static enum Move GetWinningMove(int winnerTournamentId, int loserTournamentId, u8 roundId)
{
    if ((u32)winnerTournamentId >= DOME_TOURNAMENT_TRAINERS_COUNT
        || (u32)loserTournamentId >= DOME_TOURNAMENT_TRAINERS_COUNT || roundId >= DOME_ROUNDS_COUNT)
    { ADMG_StateAccessRejected(roundId, __func__); return MOVE_NONE; }
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    u64 generation = ADMG_BeginStateAccessCheck();
    u16 winnerActor, loserActor;
    if (!ADMG_TryDomeNpcAtSlot(winnerTournamentId, &winnerActor)
        || !ADMG_TryDomeNpcAtSlot(loserTournamentId, &loserActor)
        || !ADMG_EndStateAccessCheck(generation)) return MOVE_NONE;
    if (gSaveBlock2Ptr != save)
    { ADMG_StateAccessRejected(roundId, __func__); return MOVE_NONE; }
    const struct BattleFrontierTrainer *previousTrainers = gFacilityTrainers;
    const struct TrainerMon *previousMons = gFacilityTrainerMons;
    u8 oldFirst[sizeof(gStringVar1)], oldSecond[sizeof(gStringVar2)];
    memcpy(oldFirst, gStringVar1, sizeof(oldFirst)); memcpy(oldSecond, gStringVar2, sizeof(oldSecond));
    struct ADMG_DomeWinningInput input = {0};
    // Keep the original native level/table publication point and ignored return value.
    SetFacilityPtrsGetLevel();
    const struct BattleFrontierTrainer *publishedTrainers = gFacilityTrainers;
    const struct TrainerMon *publishedMons = gFacilityTrainerMons;
    if (!ADMG_EndStateAccessCheck(generation)) goto ADMG_reject_winning_tables;
    if (gSaveBlock2Ptr != save || DOME_TRAINERS[winnerTournamentId].trainerId != winnerActor
        || DOME_TRAINERS[loserTournamentId].trainerId != loserActor)
    { ADMG_StateAccessRejected(roundId, __func__); goto ADMG_reject_winning_tables; }
    if (!ADMG_TryPrepareDomeWinning(winnerTournamentId, loserTournamentId, &input)) goto ADMG_reject_winning_tables;
    enum Move result = ADMG_RunDomeWinningMove(winnerTournamentId, loserTournamentId, roundId, &input);
    if (!input.failed && ADMG_DomeWinningCurrent(&input)) return result;
ADMG_reject_winning_tables:
    if (input.textUsed)
    { memcpy(gStringVar1, oldFirst, sizeof(oldFirst)); memcpy(gStringVar2, oldSecond, sizeof(oldSecond)); }
    if (gFacilityTrainers == publishedTrainers) gFacilityTrainers = previousTrainers;
    if (gFacilityTrainerMons == publishedMons) gFacilityTrainerMons = previousMons;
    return MOVE_NONE;
}

/* Only the native name actors are borrowed. Table publication remains live. */
struct ADMG_DomeTreeNames
{
    struct SaveBlock2 *save;
    u64 generation;
    u16 actors[DOME_TOURNAMENT_TRAINERS_COUNT];
};

static bool8 ADMG_DomeTreeNamesCurrent(const struct ADMG_DomeTreeNames *view)
{
    if (!ADMG_EndStateAccessCheck(view->generation)) return FALSE;
    if (gSaveBlock2Ptr != view->save)
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    for (u32 i = 0; i < ARRAY_COUNT(view->actors); i++)
        if (view->save->frontier.domeTrainers[i].trainerId != view->actors[i])
        { ADMG_StateAccessRejected(i, __func__); return FALSE; }
    return TRUE;
}

static bool8 ADMG_TryPrepareDomeTreeNames(struct ADMG_DomeTreeNames *view)
{
    if (view == NULL || gSaveBlock2Ptr == NULL)
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    *view = (struct ADMG_DomeTreeNames){ .save = gSaveBlock2Ptr,
        .generation = ADMG_BeginStateAccessCheck() };
    for (u32 i = 0; i < ARRAY_COUNT(view->actors); i++) view->actors[i] = DOME_TRAINERS[i].trainerId;
    for (u32 i = 0; i < ARRAY_COUNT(view->actors); i++)
    {
        const struct BattleFrontierTrainer *trainer;
        if (!ADMG_TryDomeNameIdentity(view->actors[i], &trainer) || !ADMG_DomeTreeNamesCurrent(view)) return FALSE;
    }
    return TRUE;
}

/* Check the caller's wide computed round before the named u8 interface. */
static bool8 ADMG_TryDomeAdvancementLine(u32 tournamentId, int roundId)
{
    if (tournamentId >= ARRAY_COUNT(sTourneyTreeLineSections)
        || (u32)roundId >= ARRAY_COUNT(sTourneyTreeLineSections[0]))
    { ADMG_StateAccessRejected(roundId, __func__); return FALSE; }
    u64 generation = ADMG_BeginStateAccessCheck();
    DrawTourneyAdvancementLine(tournamentId, roundId);
    return ADMG_EndStateAccessCheck(generation);
}

static void Task_ShowTourneyTree(u8 taskId)
{
    if (taskId >= NUM_TASKS) { ADMG_StateAccessRejected(taskId, __func__); return; }
    struct ADMG_DomeTreeNames admgNames;

    int i;
    struct TextPrinterTemplate textPrinter;
    int notInteractive = gTasks[taskId].tNotInteractive;
    int r4 = gTasks[taskId].data[2];

    switch (gTasks[taskId].tState)
    {
    case 0:
        SetHBlankCallback(NULL);
        SetVBlankCallback(NULL);
        EnableInterrupts(INTR_FLAG_HBLANK | INTR_FLAG_VBLANK);
        CpuFill32(0, (void *)VRAM, VRAM_SIZE);
        ResetBgsAndClearDma3BusyFlags(0);
        InitBgsFromTemplates(0, sTourneyTreeBgTemplates, ARRAY_COUNT(sTourneyTreeBgTemplates));
        InitWindows(sTourneyTreeWindowTemplates);
        DeactivateAllTextPrinters();
        gBattle_BG0_X = 0;
        gBattle_BG0_Y = 0;
        gBattle_BG1_X = 0;
        gBattle_BG1_Y = 0;
        ChangeBgX(2, 0, BG_COORD_SET);
        ChangeBgY(2, 0, BG_COORD_SET);
        ChangeBgX(3, 0, BG_COORD_SET);
        ChangeBgY(3, 0xB00, BG_COORD_SET);
        gTasks[taskId].tState++;
        break;
    case 1:
        SetGpuReg(REG_OFFSET_BLDCNT, 0);
        SetGpuReg(REG_OFFSET_BLDALPHA, 0);
        SetGpuReg(REG_OFFSET_BLDY, 0);
        SetGpuReg(REG_OFFSET_MOSAIC, 0);
        SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(88, 96));
        SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(0, DISPLAY_HEIGHT - 1));
        SetGpuReg(REG_OFFSET_WIN1H, WIN_RANGE(144, 152));
        SetGpuReg(REG_OFFSET_WIN1V, WIN_RANGE(0, DISPLAY_HEIGHT - 1));
        SetGpuReg(REG_OFFSET_WININ, 0);
        SetGpuReg(REG_OFFSET_WINOUT, WINOUT_WIN01_BG_ALL | WINOUT_WIN01_OBJ | WINOUT_WIN01_CLR);
        ResetPaletteFade();
        ResetSpriteData();
        FreeAllSpritePalettes();
        gTasks[taskId].tState++;
        break;
    case 2:
        sTilemapBuffer = AllocZeroed(BG_SCREEN_SIZE);
        DecompressDataWithHeaderWram(gDomeTourneyTree_Tilemap, sTilemapBuffer);
        SetBgTilemapBuffer(1, sTilemapBuffer);
        CopyBgTilemapBufferToVram(1);
        DecompressAndLoadBgGfxUsingHeap(1, gDomeTourneyTree_Gfx, 0x2000, 0, 0);
        DecompressAndLoadBgGfxUsingHeap(2, gDomeTourneyLine_Gfx, 0x2000, 0, 0);
        DecompressAndLoadBgGfxUsingHeap(2, gDomeTourneyLineDown_Tilemap, 0x2000, 0, 1);
        DecompressAndLoadBgGfxUsingHeap(3, gDomeTourneyLineUp_Tilemap, 0x2000, 0, 1);
        LoadPalette(gDomeTourneyTree_Pal, BG_PLTT_OFFSET, BG_PLTT_SIZE);
        LoadPalette(gDomeTourneyTreeButtons_Pal, OBJ_PLTT_OFFSET, OBJ_PLTT_SIZE);
        LoadPalette(gBattleWindowTextPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);
        CpuFill32(0, gPlttBufferFaded, PLTT_SIZE);
        ShowBg(0);
        ShowBg(1);
        ShowBg(2);
        ShowBg(3);
        gTasks[taskId].tState++;
        break;
    case 3:
        LoadCompressedSpriteSheet(sTourneyTreeButtonsSpriteSheet);
        if (notInteractive == FALSE)
        {
            for (i = 0; i < ARRAY_COUNT(sTourneyTreePokeballCoords); i++)
                CreateSprite(&sTourneyTreePokeballSpriteTemplate, sTourneyTreePokeballCoords[i][0], sTourneyTreePokeballCoords[i][1], 0);

            if (gTasks[taskId].tIsPrevTourneyTree)
                CreateSprite(&sExitButtonSpriteTemplate, 218, 12, 0);
            else
                CreateSprite(&sCancelButtonSpriteTemplate, 218, 12, 0);
        }

        SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_BG_ALL_ON | DISPCNT_OBJ_ON | DISPCNT_WIN0_ON | DISPCNT_WIN1_ON | DISPCNT_OBJ_1D_MAP);
        gTasks[taskId].tState++;
        break;
    case 4:
        if (!ADMG_TryPrepareDomeTreeNames(&admgNames)) return;
        textPrinter.type = WINDOW_TEXT_PRINTER;
        textPrinter.fontId = FONT_SHORT;
        textPrinter.currentChar = gText_BattleTourney;
        textPrinter.windowId = TOURNEYWIN_TITLE;
        textPrinter.x = 0;
        textPrinter.y = 0;
        textPrinter.letterSpacing = 2;
        textPrinter.lineSpacing = 0;
        textPrinter.currentX = GetStringCenterAlignXOffsetWithLetterSpacing(textPrinter.fontId, textPrinter.currentChar, 0x70, textPrinter.letterSpacing);
        textPrinter.currentY = 1;
        textPrinter.color.accent = TEXT_COLOR_TRANSPARENT;
        textPrinter.color.foreground = TEXT_DYNAMIC_COLOR_5;
        textPrinter.color.background = TEXT_COLOR_TRANSPARENT;
        textPrinter.color.shadow = TEXT_DYNAMIC_COLOR_4;
        AddTextPrinter(&textPrinter, 0, NULL);
        for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
        {
            int roundId, var2;

            CopyDomeTrainerName(gDisplayedStringBattle, admgNames.actors[i]);
            if (!ADMG_DomeTreeNamesCurrent(&admgNames)) goto ADMG_tree_reject;
            if (notInteractive == TRUE)
            {
                if (DOME_TRAINERS[i].isEliminated)
                {
                    if (DOME_TRAINERS[i].eliminatedAt != DOME_ROUND1)
                    {
                        var2 = DOME_TRAINERS[i].eliminatedAt - 1;
                        if (!ADMG_TryDomeAdvancementLine(i, var2)
                        || !ADMG_DomeTreeNamesCurrent(&admgNames)) goto ADMG_tree_reject;
                    }
                }
                else if (gSaveBlock2Ptr->frontier.curChallengeBattleNum != DOME_ROUND2)
                {
                    if (!ADMG_TryDomeAdvancementLine(i, gSaveBlock2Ptr->frontier.curChallengeBattleNum - 2)
                        || !ADMG_DomeTreeNamesCurrent(&admgNames)) goto ADMG_tree_reject;
                }
            }
            else if (notInteractive == FALSE)
            {
                if (DOME_TRAINERS[i].isEliminated)
                {
                    if (DOME_TRAINERS[i].eliminatedAt != DOME_ROUND1)
                    {
                        var2 = DOME_TRAINERS[i].eliminatedAt - 1;
                        if (!ADMG_TryDomeAdvancementLine(i, var2)
                        || !ADMG_DomeTreeNamesCurrent(&admgNames)) goto ADMG_tree_reject;
                    }
                }
                else if (gSaveBlock2Ptr->frontier.curChallengeBattleNum != DOME_ROUND1)
                {
                    if (gTasks[taskId].tIsPrevTourneyTree)
                        var2 = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
                    else
                        var2 = gSaveBlock2Ptr->frontier.curChallengeBattleNum - 1;
                    if (!ADMG_TryDomeAdvancementLine(i, var2)
                        || !ADMG_DomeTreeNamesCurrent(&admgNames)) goto ADMG_tree_reject;
                }
            }

            if (gTasks[taskId].tIsPrevTourneyTree)
                roundId = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
            else
                roundId = gSaveBlock2Ptr->frontier.curChallengeBattleNum - 1;

            if (    ((notInteractive == TRUE && DOME_TRAINERS[i].eliminatedAt < gSaveBlock2Ptr->frontier.curChallengeBattleNum - 1)
                  || (notInteractive == FALSE && DOME_TRAINERS[i].eliminatedAt <= roundId))
                && DOME_TRAINERS[i].isEliminated)
            {
                if (DOME_TRAINERS[i].trainerId == TRAINER_PLAYER)
                {
                    textPrinter.color.foreground = TEXT_COLOR_LIGHT_GRAY;
                    textPrinter.color.shadow = TEXT_COLOR_RED;
                }
                else
                {
                    textPrinter.color.foreground = TEXT_DYNAMIC_COLOR_2;
                    textPrinter.color.shadow = TEXT_DYNAMIC_COLOR_4;
                }
            }
            else
            {
                if (DOME_TRAINERS[i].trainerId == TRAINER_PLAYER)
                {
                    textPrinter.color.foreground = TEXT_COLOR_LIGHT_GRAY;
                    textPrinter.color.shadow = TEXT_COLOR_RED;
                }
                else
                {
                    textPrinter.color.foreground = TEXT_DYNAMIC_COLOR_5;
                    textPrinter.color.shadow = TEXT_DYNAMIC_COLOR_4;
                }
            }

            if (sTrainerNamePositions[i][0] == TOURNEYWIN_NAMES_LEFT)
                textPrinter.currentX = GetStringWidthDifference(textPrinter.fontId, gDisplayedStringBattle, 0x3D, textPrinter.letterSpacing);
            else
                textPrinter.currentX = 3;
            textPrinter.currentChar = gDisplayedStringBattle;
            textPrinter.windowId = sTrainerNamePositions[i][0];
            textPrinter.currentY = sTrainerNamePositions[i][1];
            AddTextPrinter(&textPrinter, 0, NULL);
        }
        gTasks[taskId].tState++;
        break;
    case 5:
        PutWindowTilemap(TOURNEYWIN_NAMES_LEFT);
        PutWindowTilemap(TOURNEYWIN_NAMES_RIGHT);
        PutWindowTilemap(TOURNEYWIN_TITLE);
        CopyWindowToVram(TOURNEYWIN_NAMES_LEFT, COPYWIN_FULL);
        CopyWindowToVram(TOURNEYWIN_NAMES_RIGHT, COPYWIN_FULL);
        CopyWindowToVram(TOURNEYWIN_TITLE, COPYWIN_FULL);
        SetHBlankCallback(HblankCb_TourneyTree);
        SetVBlankCallback(VblankCb_TourneyTree);
        if (r4 == 2)
        {
            if (notInteractive == FALSE)
            {
                i = CreateTask(Task_HandleTourneyTreeInput, 0);
                gTasks[i].data[0] = notInteractive;
                gTasks[i].data[1] = notInteractive;
                gTasks[i].data[6] = gTasks[taskId].tIsPrevTourneyTree;
            }
            else
            {
                i = CreateTask(Task_HandleStaticTourneyTreeInput, 0);
                gTasks[i].data[0] = 0;
            }
        }
        else
        {
            i = gTasks[taskId].data[3];
            gTasks[i].tState = 0;
        }
        ScanlineEffect_Clear();

        i = 0;
        while (i < 91)
        {
            gScanlineEffectRegBuffers[0][i] = BGCNT_PRIORITY(2) | BGCNT_SCREENBASE(31) | BGCNT_16COLOR | BGCNT_CHARBASE(2) | BGCNT_TXT256x256;
            gScanlineEffectRegBuffers[1][i] = BGCNT_PRIORITY(2) | BGCNT_SCREENBASE(31) | BGCNT_16COLOR | BGCNT_CHARBASE(2) | BGCNT_TXT256x256;
            i++;
        }

        while (i < 160)
        {
            gScanlineEffectRegBuffers[0][i] =  BGCNT_PRIORITY(1) | BGCNT_SCREENBASE(31) | BGCNT_16COLOR | BGCNT_CHARBASE(2) | BGCNT_TXT256x256;
            gScanlineEffectRegBuffers[1][i] =  BGCNT_PRIORITY(1) | BGCNT_SCREENBASE(31) | BGCNT_16COLOR | BGCNT_CHARBASE(2) | BGCNT_TXT256x256;
            i++;
        }

        ScanlineEffect_SetParams(sTourneyTreeScanlineEffectParams);
        DestroyTask(taskId);
        break;
    }

    return;
ADMG_tree_reject:
    return;
}


static void DrawTourneyAdvancementLine(u8 tournamentId, u8 roundId)
{
    if ((u32)tournamentId >= ARRAY_COUNT(sTourneyTreeLineSections)
        || (u32)roundId >= ARRAY_COUNT(sTourneyTreeLineSections[0]))
    { ADMG_StateAccessRejected(roundId, __func__); return; }

    int i;
    const struct TourneyTreeLineSection *lineSection = sTourneyTreeLineSections[tournamentId][roundId];

    for (i = 0; i < sTourneyTreeLineSectionArrayCounts[tournamentId][roundId]; i++)
        CopyToBgTilemapBufferRect_ChangePalette(1, &lineSection[i].tile, lineSection[i].x, lineSection[i].y, 1, 1, 17);

    CopyBgTilemapBufferToVram(1);
}

#define STATE_FADE_IN             0
#define STATE_SHOW_RESULTS        1
#define STATE_DELAY               2
#define STATE_WAIT_FOR_INPUT      3
#define STATE_CLOSE_TOURNEY_TREE  4

// The non-interactive tourney tree that's shown when a round is completed
static void Task_HandleStaticTourneyTreeInput(u8 taskId)
{
    if (taskId >= NUM_TASKS) { ADMG_StateAccessRejected(taskId, __func__); return; }
    struct ADMG_DomeTreeNames admgNames;
    s16 admgOldState = gTasks[taskId].tState, admgOldDelay = gTasks[taskId].data[3];

    int i;
    struct TextPrinterTemplate textPrinter;

    switch (gTasks[taskId].tState)
    {
    case STATE_FADE_IN:
        BeginNormalPaletteFade(PALETTES_ALL, 0, 0x10, 0, RGB_BLACK);
        gTasks[taskId].tState = STATE_SHOW_RESULTS;
        break;
    case STATE_SHOW_RESULTS:
        if (!gPaletteFade.active)
        {
            if (!ADMG_TryPrepareDomeTreeNames(&admgNames)) return;
            gTasks[taskId].tState = STATE_DELAY;
            gTasks[taskId].data[3] = 64;
            textPrinter.type = WINDOW_TEXT_PRINTER;
            textPrinter.fontId = FONT_SHORT;
            textPrinter.x = 0;
            textPrinter.y = 0;
            textPrinter.letterSpacing = 2;
            textPrinter.lineSpacing = 0;
            textPrinter.color.accent = TEXT_COLOR_TRANSPARENT;
            textPrinter.color.foreground = TEXT_DYNAMIC_COLOR_2;
            textPrinter.color.background = TEXT_COLOR_TRANSPARENT;
            textPrinter.color.shadow = TEXT_DYNAMIC_COLOR_4;

            // Update the advancement lines and gray out eliminated trainer names
            for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
            {
                CopyDomeTrainerName(gDisplayedStringBattle, admgNames.actors[i]);
                if (!ADMG_DomeTreeNamesCurrent(&admgNames)) goto ADMG_tree_reject;
                if (DOME_TRAINERS[i].eliminatedAt == gSaveBlock2Ptr->frontier.curChallengeBattleNum - 1
                    && DOME_TRAINERS[i].isEliminated)
                {
                    if (sTrainerNamePositions[i][0] == TOURNEYWIN_NAMES_LEFT)
                        textPrinter.currentX = GetStringWidthDifference(textPrinter.fontId, gDisplayedStringBattle, 0x3D, textPrinter.letterSpacing);
                    else
                        textPrinter.currentX = 3;

                    textPrinter.currentChar = gDisplayedStringBattle;
                    textPrinter.windowId = sTrainerNamePositions[i][0];
                    textPrinter.currentY = sTrainerNamePositions[i][1];
                    AddTextPrinter(&textPrinter, 0, NULL);
                }
                if (!DOME_TRAINERS[i].isEliminated)
                {
                    int roundId = gSaveBlock2Ptr->frontier.curChallengeBattleNum - 1;
                    if (!ADMG_TryDomeAdvancementLine(i, roundId)
                        || !ADMG_DomeTreeNamesCurrent(&admgNames)) goto ADMG_tree_reject;
                }
            }
        }
        break;
    case STATE_DELAY:
        if (--gTasks[taskId].data[3] == 0)
            gTasks[taskId].tState = STATE_WAIT_FOR_INPUT;
        break;
    case STATE_WAIT_FOR_INPUT:
        if (JOY_NEW(A_BUTTON | B_BUTTON))
        {
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 0x10, RGB_BLACK);
            gTasks[taskId].tState = STATE_CLOSE_TOURNEY_TREE;
        }
        break;
    case STATE_CLOSE_TOURNEY_TREE:
        if (!gPaletteFade.active)
        {
            SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
            DestroyTask(taskId);
        }
        break;
    }

    return;
ADMG_tree_reject:
    gTasks[taskId].tState = admgOldState;
    gTasks[taskId].data[3] = admgOldDelay;
}


#undef STATE_FADE_IN
#undef STATE_SHOW_RESULTS
#undef STATE_DELAY
#undef STATE_WAIT_FOR_INPUT
#undef STATE_CLOSE_TOURNEY_TREE

static void CB2_TourneyTree(void)
{
    AnimateSprites();
    BuildOamBuffer();
    RunTextPrinters();
    UpdatePaletteFade();
    RunTasks();
}

static void VblankCb_TourneyInfoCard(void)
{
    ChangeBgX(3, 0x80, BG_COORD_ADD);
    ChangeBgY(3, 0x80, BG_COORD_SUB);
    SetGpuReg(REG_OFFSET_BG0HOFS, gBattle_BG0_X);
    SetGpuReg(REG_OFFSET_BG0VOFS, gBattle_BG0_Y);
    SetGpuReg(REG_OFFSET_BG1HOFS, gBattle_BG1_X);
    SetGpuReg(REG_OFFSET_BG1VOFS, gBattle_BG1_Y);
    SetGpuReg(REG_OFFSET_BG2HOFS, gBattle_BG2_X);
    SetGpuReg(REG_OFFSET_BG2VOFS, gBattle_BG2_Y);
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

#define SET_WIN0H_WIN1H(win0H, win1H)                       \
{                                                           \
    *(vu32*)(REG_ADDR_WIN0H) = ((win0H << 16) | (win1H));   \
}

static void HblankCb_TourneyTree(void)
{
    u16 vCount = REG_VCOUNT;

    if (vCount < 42)
    {
        REG_WININ = WININ_WIN0_BG_ALL | WININ_WIN0_CLR | WININ_WIN0_OBJ
                | WININ_WIN1_BG_ALL | WININ_WIN1_CLR | WININ_WIN1_OBJ;
        SET_WIN0H_WIN1H(0, 0);
    }
    else if (vCount < 50)
    {
        REG_WININ = WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG3 | WININ_WIN0_OBJ | WININ_WIN0_CLR
                    | WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG3 | WININ_WIN1_OBJ | WININ_WIN1_CLR;
        SET_WIN0H_WIN1H(WIN_RANGE(152, 155), WIN_RANGE(85, 88));
    }
    else if (vCount < 58)
    {
        REG_WININ = WININ_WIN0_BG_ALL | WININ_WIN0_CLR | WININ_WIN0_OBJ
                | WININ_WIN1_BG_ALL | WININ_WIN1_CLR | WININ_WIN1_OBJ;
        SET_WIN0H_WIN1H(0, 0);
    }
    else if (vCount < 75)
    {
        REG_WININ = WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG3 | WININ_WIN0_OBJ | WININ_WIN0_CLR
                    | WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG3 | WININ_WIN1_OBJ | WININ_WIN1_CLR;
        SET_WIN0H_WIN1H(WIN_RANGE(144, 152), WIN_RANGE(88, 96));
    }
    else if (vCount < 82)
    {
        REG_WININ = WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG3 | WININ_WIN0_OBJ | WININ_WIN0_CLR
                    | WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG3 | WININ_WIN1_OBJ | WININ_WIN1_CLR;
        SET_WIN0H_WIN1H(WIN_RANGE(152, 155), WIN_RANGE(85, 88));
    }
    else if (vCount < 95)
    {
        REG_WININ = WININ_WIN0_BG_ALL | WININ_WIN0_CLR | WININ_WIN0_OBJ
                | WININ_WIN1_BG_ALL | WININ_WIN1_CLR | WININ_WIN1_OBJ;
        SET_WIN0H_WIN1H(0, 0);
    }
    else if (vCount < 103)
    {
        REG_WININ = WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG2 | WININ_WIN0_OBJ | WININ_WIN0_CLR
                    | WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG2 | WININ_WIN1_OBJ | WININ_WIN1_CLR;
        SET_WIN0H_WIN1H(WIN_RANGE(152, 155), WIN_RANGE(85, 88));
    }
    else if (vCount < 119)
    {
        REG_WININ = WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG2 | WININ_WIN0_OBJ | WININ_WIN0_CLR
                    | WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG2 | WININ_WIN1_OBJ | WININ_WIN1_CLR;
        SET_WIN0H_WIN1H(WIN_RANGE(144, 152), WIN_RANGE(88, 96));
    }
    else if (vCount < 127)
    {
        REG_WININ = WININ_WIN0_BG_ALL | WININ_WIN0_CLR | WININ_WIN0_OBJ
                | WININ_WIN1_BG_ALL | WININ_WIN1_CLR | WININ_WIN1_OBJ;
        SET_WIN0H_WIN1H(0, 0);
    }
    else if (vCount < 135)
    {
        REG_WININ = WININ_WIN0_BG0 | WININ_WIN0_BG1 | WININ_WIN0_BG2 | WININ_WIN0_OBJ | WININ_WIN0_CLR
                    | WININ_WIN1_BG0 | WININ_WIN1_BG1 | WININ_WIN1_BG2 | WININ_WIN1_OBJ | WININ_WIN1_CLR;
        SET_WIN0H_WIN1H(WIN_RANGE(152, 155), WIN_RANGE(85, 88));
    }
    else
    {
        REG_WININ = WININ_WIN0_BG_ALL | WININ_WIN0_CLR | WININ_WIN0_OBJ
                | WININ_WIN1_BG_ALL | WININ_WIN1_CLR | WININ_WIN1_OBJ;
        SET_WIN0H_WIN1H(0, 0);
    }
}

static void VblankCb_TourneyTree(void)
{
    SetGpuReg(REG_OFFSET_BG0HOFS, gBattle_BG0_X);
    SetGpuReg(REG_OFFSET_BG0VOFS, gBattle_BG0_Y);
    SetGpuReg(REG_OFFSET_BG1HOFS, gBattle_BG1_X);
    SetGpuReg(REG_OFFSET_BG1VOFS, gBattle_BG1_Y);
    ChangeBgY(2, 0x80, BG_COORD_SUB);
    ChangeBgY(3, 0x80, BG_COORD_ADD);
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
    ScanlineEffect_InitHBlankDmaTransfer();
}

static void SetFacilityTrainerAndMonPtrs(void)
{
    gFacilityTrainerMons = gBattleFrontierMons;
    gFacilityTrainers = gBattleFrontierTrainers;
}

/* Battle-selection order and saved-party positions are separate one-based
 * domains. Admit every used coordinate before either restoration starts. */
static bool8 ADMG_TryDomePlayerRestoreSlots(u32 *slots)
{
    if (gSaveBlock2Ptr == NULL || slots == NULL)
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    u32 staged[DOME_BATTLE_PARTY_SIZE];
    for (u32 i = 0; i < ARRAY_COUNT(staged); i++)
    {
        u32 order = gSelectedOrderFromParty[i];
        if (order == 0 || order > ARRAY_COUNT(gSaveBlock2Ptr->frontier.selectedPartyMons))
        { ADMG_StateAccessRejected(order, __func__); return FALSE; }
        u32 saved = gSaveBlock2Ptr->frontier.selectedPartyMons[order - 1];
        if (saved == 0 || saved > ARRAY_COUNT(gParties[B_TRAINER_PLAYER]))
        { ADMG_StateAccessRejected(saved, __func__); return FALSE; }
        staged[i] = saved - 1;
    }
    memcpy(slots, staged, sizeof(staged));
    return TRUE;
}

static void ResetSketchedMoves(void)
{
    u32 playerSlots[DOME_BATTLE_PARTY_SIZE];
    if (!ADMG_TryDomePlayerRestoreSlots(playerSlots)) return;
    int i, moveSlot;

    for (i = 0; i < DOME_BATTLE_PARTY_SIZE; i++)
    {
        int playerMonId = playerSlots[i];
        int count;

        for (moveSlot = 0; moveSlot < MAX_MON_MOVES; moveSlot++)
        {
            count = 0;
            while (count < MAX_MON_MOVES)
            {
                if (GetMonData(GetSavedPlayerPartyMon(playerMonId), MON_DATA_MOVE1 + count) == GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_MOVE1 + moveSlot))
                    break;
                count++;
            }
            if (count == MAX_MON_MOVES)
                SetMonMoveSlot(&gParties[B_TRAINER_PLAYER][i], MOVE_SKETCH, moveSlot);
        }

        SavePlayerPartyMon(playerMonId, &gParties[B_TRAINER_PLAYER][i]);
    }
}

static void RestoreDomePlayerPartyHeldItems(void)
{
    u32 playerSlots[DOME_BATTLE_PARTY_SIZE];
    if (!ADMG_TryDomePlayerRestoreSlots(playerSlots)) return;
    int i;

    for (i = 0; i < DOME_BATTLE_PARTY_SIZE; i++)
    {
        int playerMonId = playerSlots[i];
        enum Item item = GetMonData(GetSavedPlayerPartyMon(playerMonId), MON_DATA_HELD_ITEM);
        SetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HELD_ITEM, &item);
    }
}

static void ReduceDomePlayerPartyToSelectedMons(void)
{
    ReducePlayerPartyToSelectedMons();
}

static void GetPlayerSeededBeforeOpponent(void)
{
    u32 opponent, player;
    if (!ADMG_TryDomeOpponentSlot(TRAINER_BATTLE_PARAM.opponentA, &opponent)
        || !ADMG_TryDomeTrainerSlot(TRAINER_PLAYER, &player)) return;
    // A higher tournament ID is a worse seed
    if (opponent > player)
        gSpecialVar_Result = 1;
    else
        gSpecialVar_Result = 2;
}

static void BufferLastDomeWinnerName(void)
{
    if (gSaveBlock2Ptr == NULL) { ADMG_StateAccessRejected(0, __func__); return; }

    int i;

    SetFacilityTrainerAndMonPtrs();
    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        if (!DOME_TRAINERS[i].isEliminated)
            break;
    }
    if (i == DOME_TOURNAMENT_TRAINERS_COUNT)
    { ADMG_StateAccessRejected(i, __func__); return; }
    CopyDomeTrainerName(gStringVar1, DOME_TRAINERS[i].trainerId);
}

// For showing the previous tourney results before the player has entered a challenge
static bool8 ADMG_RunInitRandomTourneyTreeResults(struct ADMG_DomeRosterFrame *frame)
{
    int i, j, k;
    int monLevel;
    enum Species species[FRONTIER_PARTY_SIZE];
    int monTypesBits;
    int trainerId;
    int monId;
    int zero1;
    int zero2;
    enum FrontierLevelMode lvlMode;
    u16 *statSums;
    int *statValues;
    u8 ivs = 0;

    species[0] = SPECIES_NONE;
    species[1] = SPECIES_NONE;
    species[2] = SPECIES_NONE;
    if ((frame->save->frontier.domeLvlMode != -frame->save->frontier.domeBattleMode) && frame->save->frontier.challengeStatus != CHALLENGE_STATUS_SAVING)
        return TRUE;

    statSums = frame->scores = AllocZeroed(sizeof(u16) * DOME_TOURNAMENT_TRAINERS_COUNT);
    if (statSums == NULL) { ADMG_StateAccessRejected(0, __func__); goto ADMG_roster_reject; }
    if (!ADMG_DomeRosterCurrent(frame)) goto ADMG_roster_reject;
    statValues = frame->stats = AllocZeroed(sizeof(int) * NUM_STATS);
    if (statValues == NULL) { ADMG_StateAccessRejected(0, __func__); goto ADMG_roster_reject; }
    if (!ADMG_DomeRosterCurrent(frame)) goto ADMG_roster_reject;
    lvlMode = frame->save->frontier.lvlMode;
    frame->ownsLevel = TRUE;
    frame->level = FRONTIER_LVL_50;
    frame->save->frontier.lvlMode = FRONTIER_LVL_50;
    zero1 = 0;
    zero2 = 0;

    frame->save->frontier.domeLvlMode = zero1 + 1;
    frame->save->frontier.domeBattleMode = zero2 + 1;

    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        do
        {
            if (i < 5)
                trainerId = ADMG_DOME_ROSTER_VALUE(frame, Random()) % 10;
            else if (i < 15)
                trainerId = ADMG_DOME_ROSTER_VALUE(frame, Random()) % 20 + 10;
            else
                trainerId = ADMG_DOME_ROSTER_VALUE(frame, Random()) % 10 + 30;

            for (j = 0; j < i; j++)
            {
                if (DOME_TRAINERS[j].trainerId == trainerId)
                    break;
            }
        } while (j != i);

        DOME_TRAINERS[i].trainerId = trainerId;
        for (j = 0; j < FRONTIER_PARTY_SIZE; j++)
        {
            do
            {
                monId = ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonId(frame, ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterDrawMon(frame, trainerId))));
                for (k = 0; k < j; k++)
                {
                    // Make sure the mon is valid.
                    int alreadySelectedMonId = DOME_MONS[i][k];
                    if (alreadySelectedMonId == monId
                        || species[0] == (*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, monId))).species
                        || species[1] == (*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, monId))).species
                        || (*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, alreadySelectedMonId))).heldItem == (*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, monId))).heldItem)
                        break;
                }
            } while (k != j);

            DOME_MONS[i][j] = monId;
            species[j] = (*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, monId))).species;
        }
        DOME_TRAINERS[i].isEliminated = FALSE;
        DOME_TRAINERS[i].eliminatedAt = 0;
        DOME_TRAINERS[i].forfeited = FALSE;
    }

    monLevel = FRONTIER_MAX_LEVEL_50;
    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        monTypesBits = 0;
        statSums[i] = 0;
        ivs = ADMG_DOME_ROSTER_VALUE(frame, GetDomeTrainerMonIvs(DOME_TRAINERS[i].trainerId));
        for (j = 0; j < FRONTIER_PARTY_SIZE; j++)
        {
            ADMG_DOME_ROSTER_ACTION(frame, CalcDomeMonStats(&(*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, DOME_MONS[i][j]))),
                             monLevel, ivs, statValues));

            statSums[i] += statValues[STAT_ATK];
            statSums[i] += statValues[STAT_DEF];
            statSums[i] += statValues[STAT_SPATK];
            statSums[i] += statValues[STAT_SPDEF];
            statSums[i] += statValues[STAT_SPEED];
            statSums[i] += statValues[STAT_HP];
            monTypesBits |= 1u << ADMG_DOME_ROSTER_VALUE(frame, GetSpeciesType((*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, DOME_MONS[i][j]))).species, 0));
            monTypesBits |= 1u << ADMG_DOME_ROSTER_VALUE(frame, GetSpeciesType((*ADMG_DOME_ROSTER_VALUE(frame, ADMG_DomeRosterMonRow(frame, DOME_MONS[i][j]))).species, 1));
        }

        // Because GF hates temporary vars, trainerId acts like monTypesCount here.
        for (trainerId = 0, j = 0; j < 32; j++)
        {
            if (monTypesBits & 1)
                trainerId++;
            monTypesBits >>= 1;
        }
        statSums[i] += (trainerId * monLevel) / 20;
    }

    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT - 1; i++)
    {
        for (j = i + 1; j < DOME_TOURNAMENT_TRAINERS_COUNT; j++)
        {
            if (statSums[i] < statSums[j])
            {
                ADMG_DOME_ROSTER_ACTION(frame, SwapDomeTrainers(i, j, statSums));
            }
            else if (statSums[i] == statSums[j])
            {
                if (DOME_TRAINERS[i].trainerId > DOME_TRAINERS[j].trainerId)
                    ADMG_DOME_ROSTER_ACTION(frame, SwapDomeTrainers(i, j, statSums));
            }
        }
    }

    ADMG_ReleaseDomeRoster(frame);
    if (!ADMG_DomeRosterCurrent(frame)) goto ADMG_roster_reject;

    for (i = 0; i < DOME_ROUNDS_COUNT; i++)
        ADMG_DOME_ROSTER_ACTION(frame, ADMG_DomeRosterRound(frame, i));

    frame->save->frontier.lvlMode = lvlMode;
    frame->level = lvlMode;
    frame->ownsLevel = FALSE;
    return TRUE;
ADMG_roster_reject:
    return FALSE;
}

static void InitRandomTourneyTreeResults(void)
{
    if (gSaveBlock2Ptr == NULL) { ADMG_StateAccessRejected(0, __func__); return; }
    if ((gSaveBlock2Ptr->frontier.domeLvlMode != -gSaveBlock2Ptr->frontier.domeBattleMode)
        && gSaveBlock2Ptr->frontier.challengeStatus != CHALLENGE_STATUS_SAVING) return;
    struct ADMG_DomeRosterFrame frame = {0};
    if (!ADMG_TryPrepareDomeRoster(&frame, TRUE)) return;
    bool8 completed = ADMG_RunInitRandomTourneyTreeResults(&frame);
    ADMG_ReleaseDomeRoster(&frame);
    if (!completed || !ADMG_DomeRosterCurrent(&frame)) ADMG_RestoreDomeRoster(&frame);
}

/* Dome transport is an explicit native roster domain, never content ownership. */
static bool8 ADMG_TryDomeTrainerSlot(u32 identity, u32 *slot)
{
    if (gSaveBlock2Ptr == NULL || slot == NULL || identity > USHRT_MAX)
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    u64 generation = ADMG_BeginStateAccessCheck();
    if (identity != TRAINER_PLAYER && identity != TRAINER_FRONTIER_BRAIN)
    {
        const struct BattleFrontierTrainer *table, *row;
        const struct TrainerMon *mons;
        u32 count;
        if (!ADMG_TrySelectFacilityTrainerTables(FALSE, &table, &mons, &count)
            || !ADMG_EndStateAccessCheck(generation)) return FALSE;
        if (gSaveBlock2Ptr != save)
        { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
        if (!ADMG_TryGetFacilityTrainerRowFromTable(table, count, identity, &row)
            || !ADMG_EndStateAccessCheck(generation)) return FALSE;
        if (gSaveBlock2Ptr != save)
        { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    }
    for (u32 i = 0; i < ARRAY_COUNT(DOME_TRAINERS); i++)
        if (DOME_TRAINERS[i].trainerId == identity)
        { *slot = i; return TRUE; }
    ADMG_StateAccessRejected(identity, __func__);
    return FALSE;
}

static bool8 ADMG_TryDomeOpponentSlot(u32 identity, u32 *slot)
{
    struct ADMG_TrainerActorRef actor;
    if (!ADMG_TryClassifyTrainerActor(identity, ADMG_ACTOR_FACILITY, ADMG_ACTOR_OPPONENT, &actor)) return FALSE;
    return ADMG_TryDomeTrainerSlot(identity, slot);
}

static int TrainerIdToTournamentId(u16 trainerId)
{
    u32 slot;
    if (!ADMG_TryDomeTrainerSlot(trainerId, &slot)) return DOME_TOURNAMENT_TRAINERS_COUNT;
    return slot;
}

// The same as the above one, but has global scope.
int TrainerIdToDomeTournamentId(u16 trainerId)
{
    return TrainerIdToTournamentId(trainerId);
}

static u8 GetOpposingNPCTournamentIdByRound(u8 tournamentId, u8 round)
{
    if (gSaveBlock2Ptr == NULL || tournamentId >= ARRAY_COUNT(sTournamentIdToPairedTrainerIds)
        || round >= ARRAY_COUNT(sTrainerAndRoundToLastMatchCardNum[0]))
    { ADMG_StateAccessRejected(tournamentId, __func__); return 0xFF; }
    u32 paired = sTournamentIdToPairedTrainerIds[tournamentId] / 2;
    if (paired >= ARRAY_COUNT(sTrainerAndRoundToLastMatchCardNum))
    { ADMG_StateAccessRejected(paired, __func__); return 0xFF; }
    u32 card = sTrainerAndRoundToLastMatchCardNum[paired][round];
    if (card < DOME_TOURNAMENT_TRAINERS_COUNT)
    { ADMG_StateAccessRejected(card, __func__); return 0xFF; }
    u8 tournamentIds[2];
    if (!ADMG_TryBufferDomeMatchPair(card - DOME_TOURNAMENT_TRAINERS_COUNT, tournamentIds, NULL)) return 0xFF;
    if (tournamentId == tournamentIds[0]) return tournamentIds[1];
    return tournamentIds[0];
}

// Determines which trainers won in the NPC vs NPC battles
static bool8 ADMG_RunDomeRound(u8 roundId, struct ADMG_DomeRoundFrame *frame)
{
    int i;
    int moveSlot, monId1, monId2;
    int tournamentId1, tournamentId2;
    enum Species species;
    int points1 = 0, points2 = 0;

    for (i = 0; i < DOME_TOURNAMENT_TRAINERS_COUNT; i++)
    {
        if (DOME_TRAINERS[i].isEliminated || DOME_TRAINERS[i].trainerId == TRAINER_PLAYER)
            continue;

        frame->left = frame->right = NULL;
        u16 actor;
        if (!ADMG_TryDomeNpcAtSlot(i, &actor) || !ADMG_DomeRoundCurrent(frame)) goto ADMG_round_reject;
        tournamentId1 = i;
        tournamentId2 = ADMG_DOME_ROUND_VALUE(frame, TournamentIdOfOpponent(roundId, DOME_TRAINERS[tournamentId1].trainerId));
        if (tournamentId2 != 0xFF && (u32)tournamentId2 >= DOME_TOURNAMENT_TRAINERS_COUNT)
        { ADMG_StateAccessRejected(tournamentId2, __func__); goto ADMG_round_reject; }
        // Frontier Brain always wins, check tournamentId1.
        if (DOME_TRAINERS[tournamentId1].trainerId == TRAINER_FRONTIER_BRAIN && tournamentId2 != 0xFF)
        {
            frame->writtenMask |= 1u << tournamentId2;
            DOME_TRAINERS[tournamentId2].isEliminated = TRUE;
            DOME_TRAINERS[tournamentId2].eliminatedAt = roundId;
            gSaveBlock2Ptr->frontier.domeWinningMoves[tournamentId2] = ADMG_DOME_ROUND_VALUE(frame, ADMG_DomeRoundWinning(frame, tournamentId1, tournamentId2, roundId));
        }
        // Frontier Brain always wins, check tournamentId2.
        else if (tournamentId2 != 0xFF && DOME_TRAINERS[tournamentId2].trainerId == TRAINER_FRONTIER_BRAIN && tournamentId1 != 0xFF)
        {
            frame->writtenMask |= 1u << tournamentId1;
            DOME_TRAINERS[tournamentId1].isEliminated = TRUE;
            DOME_TRAINERS[tournamentId1].eliminatedAt = roundId;
            gSaveBlock2Ptr->frontier.domeWinningMoves[tournamentId1] = ADMG_DOME_ROUND_VALUE(frame, ADMG_DomeRoundWinning(frame, tournamentId2, tournamentId1, roundId));
        }
        // Decide which one of two trainers wins!
        else if (tournamentId2 != 0xFF)
        {
            // BUG: points1 and points2 are not cleared at the beginning of the loop resulting in not fair results.
            #ifdef BUGFIX
            points1 = 0;
            points2 = 0;
            #endif

            struct ADMG_DomeSelectionInput left, right;
            if (!ADMG_TryPrepareDomeSelection(tournamentId1, &left)
                || !ADMG_TryPrepareDomeSelection(tournamentId2, &right)) goto ADMG_round_reject;
            frame->left = &left; frame->right = &right;
            if (!ADMG_DomeRoundCurrent(frame)) goto ADMG_round_reject;
            // Calculate points for both trainers.
            for (monId1 = 0; monId1 < FRONTIER_PARTY_SIZE; monId1++)
            {
                for (moveSlot = 0; moveSlot < MAX_MON_MOVES; moveSlot++)
                {
                    for (monId2 = 0; monId2 < FRONTIER_PARTY_SIZE; monId2++)
                    {
                        points1 += ADMG_DOME_ROUND_VALUE(frame, GetTypeEffectivenessPoints(left.rows[monId1]->moves[moveSlot],
                                                right.rows[monId2]->species, EFFECTIVENESS_MODE_AI_VS_AI));
                    }
                }
                species = left.rows[monId1]->species;
                points1 += ADMG_DOME_ROUND_VALUE(frame, GetTotalBaseStat(species)) / 10;
            }
            // Random part of the formula.
            points1 += (ADMG_DOME_ROUND_VALUE(frame, Random()) & 0x1F);
            // Favor trainers with higher id;
            points1 += tournamentId1;

            for (monId1 = 0; monId1 < FRONTIER_PARTY_SIZE; monId1++)
            {
                for (moveSlot = 0; moveSlot < MAX_MON_MOVES; moveSlot++)
                {
                    for (monId2 = 0; monId2 < FRONTIER_PARTY_SIZE; monId2++)
                    {
                        points2 += ADMG_DOME_ROUND_VALUE(frame, GetTypeEffectivenessPoints(right.rows[monId1]->moves[moveSlot],
                                                left.rows[monId2]->species, EFFECTIVENESS_MODE_AI_VS_AI));
                    }
                }
                species = right.rows[monId1]->species;
                points2 += ADMG_DOME_ROUND_VALUE(frame, GetTotalBaseStat(species)) / 10;
            }
            // Random part of the formula.
            points2 += (ADMG_DOME_ROUND_VALUE(frame, Random()) & 0x1F);
            // Favor trainers with higher id;
            points2 += tournamentId2;

            if (points1 > points2)
            {
                frame->writtenMask |= 1u << tournamentId2;
            DOME_TRAINERS[tournamentId2].isEliminated = TRUE;
                DOME_TRAINERS[tournamentId2].eliminatedAt = roundId;
                gSaveBlock2Ptr->frontier.domeWinningMoves[tournamentId2] = ADMG_DOME_ROUND_VALUE(frame, ADMG_DomeRoundWinning(frame, tournamentId1, tournamentId2, roundId));
            }
            else if (points1 < points2)
            {
                frame->writtenMask |= 1u << tournamentId1;
            DOME_TRAINERS[tournamentId1].isEliminated = TRUE;
                DOME_TRAINERS[tournamentId1].eliminatedAt = roundId;
                gSaveBlock2Ptr->frontier.domeWinningMoves[tournamentId1] = ADMG_DOME_ROUND_VALUE(frame, ADMG_DomeRoundWinning(frame, tournamentId2, tournamentId1, roundId));
            }
            // Points are the same, so we favor the one with the higher id.
            else if (tournamentId1 > tournamentId2)
            {
                frame->writtenMask |= 1u << tournamentId2;
            DOME_TRAINERS[tournamentId2].isEliminated = TRUE;
                DOME_TRAINERS[tournamentId2].eliminatedAt = roundId;
                gSaveBlock2Ptr->frontier.domeWinningMoves[tournamentId2] = ADMG_DOME_ROUND_VALUE(frame, ADMG_DomeRoundWinning(frame, tournamentId1, tournamentId2, roundId));
            }
            else
            {
                frame->writtenMask |= 1u << tournamentId1;
            DOME_TRAINERS[tournamentId1].isEliminated = TRUE;
                DOME_TRAINERS[tournamentId1].eliminatedAt = roundId;
                gSaveBlock2Ptr->frontier.domeWinningMoves[tournamentId1] = ADMG_DOME_ROUND_VALUE(frame, ADMG_DomeRoundWinning(frame, tournamentId2, tournamentId1, roundId));
            }
        }
    }
    return TRUE;
ADMG_round_reject:
    return FALSE;
}

static void DecideRoundWinners(u8 roundId)
{
    if (roundId >= DOME_ROUNDS_COUNT) { ADMG_StateAccessRejected(roundId, __func__); return; }
    struct ADMG_DomeRoundFrame frame;
    if (!ADMG_TryPrepareDomeRound(&frame)) return;
    if (!ADMG_RunDomeRound(roundId, &frame) || !ADMG_DomeRoundCurrent(&frame)) ADMG_RestoreDomeRound(&frame);
}

static void CopyDomeTrainerName(u8 *str, u16 trainerId)
{
    const struct BattleFrontierTrainer *trainer = NULL;
    if (str == NULL) { ADMG_StateAccessRejected(trainerId, __func__); return; }
    if (!ADMG_TryDomeNameIdentity(trainerId, &trainer)) return;
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    u64 generation = ADMG_BeginStateAccessCheck();

    int i = 0;
    SetFacilityPtrsGetLevel();
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (gSaveBlock2Ptr != save) { ADMG_StateAccessRejected(trainerId, __func__); return; }

    if (trainerId == TRAINER_FRONTIER_BRAIN)
    {
        CopyFrontierBrainTrainerName(str);
    }
    else
    {
        if (trainerId == TRAINER_PLAYER)
        {
            for (i = 0; i < PLAYER_NAME_LENGTH; i++)
                str[i] = gSaveBlock2Ptr->playerName[i];
        }
        else if (trainerId < FRONTIER_TRAINERS_COUNT)
        {
            for (i = 0; i < PLAYER_NAME_LENGTH; i++)
                str[i] = trainer->trainerName[i];
        }
        str[i] = EOS;
    }
}
