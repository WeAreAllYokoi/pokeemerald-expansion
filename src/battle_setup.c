#include "global.h"
#include "admg_feature_bindings.h"
#include "admg_script.h"
#include "admg_trainer_actor.h"
#include "admg_rematch.h"
#include "admg_rematch_battle.h"
#include "admg_trainer.h"
#include "data.h"
#include "main.h"
#include "battle.h"
#include "battle_frontier.h"
#include "battle_pike.h"
#include "battle_pyramid.h"
#include "battle_setup.h"
#include "battle_special.h"
#include "battle_partner.h"
#include "battle_tower.h"
#include "battle_transition.h"
#include "event_data.h"
#include "admg_state.h"
#include "event_object_movement.h"
#include "event_scripts.h"
#include "fieldmap.h"
#include "field_name_box.h"
#include "field_control_avatar.h"
#include "field_message_box.h"
#include "field_player_avatar.h"
#include "field_screen_effect.h"
#include "field_weather.h"
#include "fishing.h"
#include "fldeff.h"
#include "fldeff_misc.h"
#include "follower_npc.h"
#include "gym_leader_rematch.h"
#include "item.h"
#include "load_save.h"
#include "malloc.h"
#include "metatile_behavior.h"
#include "mirage_tower.h"
#include "palette.h"
#include "random.h"
#include "safari_zone.h"
#include "script.h"
#include "script_pokemon_util.h"
#include "secret_base.h"
#include "sound.h"
#include "starter_choose.h"
#include "strings.h"
#include "string_util.h"
#include "task.h"
#include "trainer_hill.h"
#include "trainer_pools.h"
#include "trainer_see.h"
#include "trainer_util.h"
#include "tv.h"
#include "overworld.h"
#include "vs_seeker.h"
#include "wild_encounter_ow.h"
#include "admg_trainer_script.h"
#include "admg_registry.h"
#include "admg_battle_request.h"
#include "admg_retention.h"
#include "window.h"
#include "constants/battle_frontier.h"
#include "constants/battle_special.h"
#include "constants/event_objects.h"
#include "constants/game_stat.h"
#include "constants/items.h"
#include "constants/songs.h"
#include "constants/trainers.h"
#include "constants/trainer_hill.h"
#include "constants/weather.h"

enum TransitionType
{
    TRANSITION_TYPE_NORMAL,
    TRANSITION_TYPE_CAVE,
    TRANSITION_TYPE_FLASH,
    TRANSITION_TYPE_WATER,
};

// this file's functions
static void DoBattlePikeWildBattle(void);
static void DoSafariBattle(void);
static void DoGhostBattle(void);
static void DoStandardWildBattle(bool32 isDouble);
static void CB2_EndWildBattle(void);
static void CB2_EndScriptedWildBattle(void);
static void CB2_EndMarowakBattle(void);
static void TryUpdateGymLeaderRematchFromWild(void);
static void TryUpdateGymLeaderRematchFromTrainer(void);
static void CB2_GiveStarter(void);
static void CB2_StartFirstBattle(void);
static void CB2_EndFirstBattle(void);
static void SaveChangesToPlayerParty(void);
static void HandleBattleVariantEndParty(void);
static void CB2_EndTrainerBattle(void);
static bool32 IsPlayerDefeated(u32 battleOutcome);
u16 GetRematchTrainerId(enum TrainerId trainerId);
static void RegisterTrainerInMatchCall(void);
static void HandleRematchVarsOnBattleEnd(void);
static const u8 *GetIntroSpeechOfApproachingTrainer(void);
static const u8 *GetTrainerCantBattleSpeech(void);
static void CreateNPCTrainerParty(struct Pokemon *party, enum TrainerId trainerNum);
static bool8 DoTrainerBattle(void);
static bool8 IsNativeTrainerContent(u32 trainerId);
static bool8 IsNativeTrainerBattleContent(u32 trainerId);
static bool8 NativeBattleDefeatPairValid(void);

EWRAM_DATA TrainerBattleParameter gTrainerBattleParameter = {0};
EWRAM_DATA enum TrainerId gPartnerTrainerId = 0;
EWRAM_DATA static u8 *sTrainerBattleEndScript = NULL;
EWRAM_DATA static bool8 sShouldCheckTrainerBScript = FALSE;
EWRAM_DATA static u8 sNoOfPossibleTrainerRetScripts = 0;

// The first transition is used if the enemy PokÃ©mon are lower level than our PokÃ©mon.
// Otherwise, the second transition is used.
static const u8 sBattleTransitionTable_Wild[][2] =
{
    [TRANSITION_TYPE_NORMAL] = {B_TRANSITION_SLICE,          B_TRANSITION_WHITE_BARS_FADE},
    [TRANSITION_TYPE_CAVE]   = {B_TRANSITION_CLOCKWISE_WIPE, B_TRANSITION_GRID_SQUARES},
    [TRANSITION_TYPE_FLASH]  = {B_TRANSITION_BLUR,           B_TRANSITION_GRID_SQUARES},
    [TRANSITION_TYPE_WATER]  = {B_TRANSITION_WAVE,           B_TRANSITION_RIPPLE},
};

static const u8 sBattleTransitionTable_Trainer[][2] =
{
    [TRANSITION_TYPE_NORMAL] = {B_TRANSITION_POKEBALLS_TRAIL, B_TRANSITION_ANGLED_WIPES},
    [TRANSITION_TYPE_CAVE]   = {B_TRANSITION_SHUFFLE,         B_TRANSITION_BIG_POKEBALL},
    [TRANSITION_TYPE_FLASH]  = {B_TRANSITION_BLUR,            B_TRANSITION_GRID_SQUARES},
    [TRANSITION_TYPE_WATER]  = {B_TRANSITION_SWIRL,           B_TRANSITION_RIPPLE},
};

// Battle Frontier (excluding Pyramid and Dome, which have their own tables below)
static const u8 sBattleTransitionTable_BattleFrontier[] =
{
    B_TRANSITION_FRONTIER_LOGO_WIGGLE,
    B_TRANSITION_FRONTIER_LOGO_WAVE,
    B_TRANSITION_FRONTIER_SQUARES,
    B_TRANSITION_FRONTIER_SQUARES_SCROLL,
    B_TRANSITION_FRONTIER_CIRCLES_MEET,
    B_TRANSITION_FRONTIER_CIRCLES_CROSS,
    B_TRANSITION_FRONTIER_CIRCLES_ASYMMETRIC_SPIRAL,
    B_TRANSITION_FRONTIER_CIRCLES_SYMMETRIC_SPIRAL,
    B_TRANSITION_FRONTIER_CIRCLES_MEET_IN_SEQ,
    B_TRANSITION_FRONTIER_CIRCLES_CROSS_IN_SEQ,
    B_TRANSITION_FRONTIER_CIRCLES_ASYMMETRIC_SPIRAL_IN_SEQ,
    B_TRANSITION_FRONTIER_CIRCLES_SYMMETRIC_SPIRAL_IN_SEQ
};

static const u8 sBattleTransitionTable_BattlePyramid[] =
{
    B_TRANSITION_FRONTIER_SQUARES,
    B_TRANSITION_FRONTIER_SQUARES_SCROLL,
    B_TRANSITION_FRONTIER_SQUARES_SPIRAL
};

static const u8 sBattleTransitionTable_BattleDome[] =
{
    B_TRANSITION_FRONTIER_LOGO_WIGGLE,
    B_TRANSITION_FRONTIER_SQUARES,
    B_TRANSITION_FRONTIER_SQUARES_SCROLL,
    B_TRANSITION_FRONTIER_SQUARES_SPIRAL
};

#define REMATCH(trainer1, trainer2, trainer3, trainer4, trainer5, map)  \
{                                                                       \
    .trainerIds = {trainer1, trainer2, trainer3, trainer4, trainer5},   \
    .mapId = map,                                                       \
}

const struct RematchTrainer gRematchTable[REMATCH_TABLE_ENTRIES] =
{
    [REMATCH_ROSE] = REMATCH(TRAINER_ROSE_1, TRAINER_ROSE_2, TRAINER_ROSE_3, TRAINER_ROSE_4, TRAINER_ROSE_5, MAP_ROUTE118),
    [REMATCH_ANDRES] = REMATCH(TRAINER_ANDRES_1, TRAINER_ANDRES_2, TRAINER_ANDRES_3, TRAINER_ANDRES_4, TRAINER_ANDRES_5, MAP_ROUTE105),
    [REMATCH_DUSTY] = REMATCH(TRAINER_DUSTY_1, TRAINER_DUSTY_2, TRAINER_DUSTY_3, TRAINER_DUSTY_4, TRAINER_DUSTY_5, MAP_ROUTE111),
    [REMATCH_LOLA] = REMATCH(TRAINER_LOLA_1, TRAINER_LOLA_2, TRAINER_LOLA_3, TRAINER_LOLA_4, TRAINER_LOLA_5, MAP_ROUTE109),
    [REMATCH_RICKY] = REMATCH(TRAINER_RICKY_1, TRAINER_RICKY_2, TRAINER_RICKY_3, TRAINER_RICKY_4, TRAINER_RICKY_5, MAP_ROUTE109),
    [REMATCH_LILA_AND_ROY] = REMATCH(TRAINER_LILA_AND_ROY_1, TRAINER_LILA_AND_ROY_2, TRAINER_LILA_AND_ROY_3, TRAINER_LILA_AND_ROY_4, TRAINER_LILA_AND_ROY_5, MAP_ROUTE124),
    [REMATCH_CRISTIN] = REMATCH(TRAINER_CRISTIN_1, TRAINER_CRISTIN_2, TRAINER_CRISTIN_3, TRAINER_CRISTIN_4, TRAINER_CRISTIN_5, MAP_ROUTE121),
    [REMATCH_BROOKE] = REMATCH(TRAINER_BROOKE_1, TRAINER_BROOKE_2, TRAINER_BROOKE_3, TRAINER_BROOKE_4, TRAINER_BROOKE_5, MAP_ROUTE111),
    [REMATCH_WILTON] = REMATCH(TRAINER_WILTON_1, TRAINER_WILTON_2, TRAINER_WILTON_3, TRAINER_WILTON_4, TRAINER_WILTON_5, MAP_ROUTE111),
    [REMATCH_VALERIE] = REMATCH(TRAINER_VALERIE_1, TRAINER_VALERIE_2, TRAINER_VALERIE_3, TRAINER_VALERIE_4, TRAINER_VALERIE_5, MAP_MT_PYRE_6F),
    [REMATCH_CINDY] = REMATCH(TRAINER_CINDY_1, TRAINER_CINDY_3, TRAINER_CINDY_4, TRAINER_CINDY_5, TRAINER_CINDY_6, MAP_ROUTE104),
    [REMATCH_THALIA] = REMATCH(TRAINER_THALIA_1, TRAINER_THALIA_2, TRAINER_THALIA_3, TRAINER_THALIA_4, TRAINER_THALIA_5, MAP_ABANDONED_SHIP_ROOMS_1F),
    [REMATCH_JESSICA] = REMATCH(TRAINER_JESSICA_1, TRAINER_JESSICA_2, TRAINER_JESSICA_3, TRAINER_JESSICA_4, TRAINER_JESSICA_5, MAP_ROUTE121),
    [REMATCH_WINSTON] = REMATCH(TRAINER_WINSTON_1, TRAINER_WINSTON_2, TRAINER_WINSTON_3, TRAINER_WINSTON_4, TRAINER_WINSTON_5, MAP_ROUTE104),
    [REMATCH_STEVE] = REMATCH(TRAINER_STEVE_1, TRAINER_STEVE_2, TRAINER_STEVE_3, TRAINER_STEVE_4, TRAINER_STEVE_5, MAP_ROUTE114),
    [REMATCH_TONY] = REMATCH(TRAINER_TONY_1, TRAINER_TONY_2, TRAINER_TONY_3, TRAINER_TONY_4, TRAINER_TONY_5, MAP_ROUTE107),
    [REMATCH_NOB] = REMATCH(TRAINER_NOB_1, TRAINER_NOB_2, TRAINER_NOB_3, TRAINER_NOB_4, TRAINER_NOB_5, MAP_ROUTE115),
    [REMATCH_KOJI] = REMATCH(TRAINER_KOJI_1, TRAINER_KOJI_2, TRAINER_KOJI_3, TRAINER_KOJI_4, TRAINER_KOJI_5, MAP_ROUTE127),
    [REMATCH_FERNANDO] = REMATCH(TRAINER_FERNANDO_1, TRAINER_FERNANDO_2, TRAINER_FERNANDO_3, TRAINER_FERNANDO_4, TRAINER_FERNANDO_5, MAP_ROUTE123),
    [REMATCH_DALTON] = REMATCH(TRAINER_DALTON_1, TRAINER_DALTON_2, TRAINER_DALTON_3, TRAINER_DALTON_4, TRAINER_DALTON_5, MAP_ROUTE118),
    [REMATCH_BERNIE] = REMATCH(TRAINER_BERNIE_1, TRAINER_BERNIE_2, TRAINER_BERNIE_3, TRAINER_BERNIE_4, TRAINER_BERNIE_5, MAP_ROUTE114),
    [REMATCH_ETHAN] = REMATCH(TRAINER_ETHAN_1, TRAINER_ETHAN_2, TRAINER_ETHAN_3, TRAINER_ETHAN_4, TRAINER_ETHAN_5, MAP_JAGGED_PASS),
    [REMATCH_JOHN_AND_JAY] = REMATCH(TRAINER_JOHN_AND_JAY_1, TRAINER_JOHN_AND_JAY_2, TRAINER_JOHN_AND_JAY_3, TRAINER_JOHN_AND_JAY_4, TRAINER_JOHN_AND_JAY_5, MAP_METEOR_FALLS_1F_2R),
    [REMATCH_JEFFREY] = REMATCH(TRAINER_JEFFREY_1, TRAINER_JEFFREY_2, TRAINER_JEFFREY_3, TRAINER_JEFFREY_4, TRAINER_JEFFREY_5, MAP_ROUTE120),
    [REMATCH_CAMERON] = REMATCH(TRAINER_CAMERON_1, TRAINER_CAMERON_2, TRAINER_CAMERON_3, TRAINER_CAMERON_4, TRAINER_CAMERON_5, MAP_ROUTE123),
    [REMATCH_JACKI] = REMATCH(TRAINER_JACKI_1, TRAINER_JACKI_2, TRAINER_JACKI_3, TRAINER_JACKI_4, TRAINER_JACKI_5, MAP_ROUTE123),
    [REMATCH_WALTER] = REMATCH(TRAINER_WALTER_1, TRAINER_WALTER_2, TRAINER_WALTER_3, TRAINER_WALTER_4, TRAINER_WALTER_5, MAP_ROUTE121),
    [REMATCH_KAREN] = REMATCH(TRAINER_KAREN_1, TRAINER_KAREN_2, TRAINER_KAREN_3, TRAINER_KAREN_4, TRAINER_KAREN_5, MAP_ROUTE116),
    [REMATCH_JERRY] = REMATCH(TRAINER_JERRY_1, TRAINER_JERRY_2, TRAINER_JERRY_3, TRAINER_JERRY_4, TRAINER_JERRY_5, MAP_ROUTE116),
    [REMATCH_ANNA_AND_MEG] = REMATCH(TRAINER_ANNA_AND_MEG_1, TRAINER_ANNA_AND_MEG_2, TRAINER_ANNA_AND_MEG_3, TRAINER_ANNA_AND_MEG_4, TRAINER_ANNA_AND_MEG_5, MAP_ROUTE117),
    [REMATCH_ISABEL] = REMATCH(TRAINER_ISABEL_1, TRAINER_ISABEL_2, TRAINER_ISABEL_3, TRAINER_ISABEL_4, TRAINER_ISABEL_5, MAP_ROUTE110),
    [REMATCH_MIGUEL] = REMATCH(TRAINER_MIGUEL_1, TRAINER_MIGUEL_2, TRAINER_MIGUEL_3, TRAINER_MIGUEL_4, TRAINER_MIGUEL_5, MAP_ROUTE103),
    [REMATCH_TIMOTHY] = REMATCH(TRAINER_TIMOTHY_1, TRAINER_TIMOTHY_2, TRAINER_TIMOTHY_3, TRAINER_TIMOTHY_4, TRAINER_TIMOTHY_5, MAP_ROUTE115),
    [REMATCH_SHELBY] = REMATCH(TRAINER_SHELBY_1, TRAINER_SHELBY_2, TRAINER_SHELBY_3, TRAINER_SHELBY_4, TRAINER_SHELBY_5, MAP_MT_CHIMNEY),
    [REMATCH_CALVIN] = REMATCH(TRAINER_CALVIN_1, TRAINER_CALVIN_2, TRAINER_CALVIN_3, TRAINER_CALVIN_4, TRAINER_CALVIN_5, MAP_ROUTE102),
    [REMATCH_ELLIOT] = REMATCH(TRAINER_ELLIOT_1, TRAINER_ELLIOT_2, TRAINER_ELLIOT_3, TRAINER_ELLIOT_4, TRAINER_ELLIOT_5, MAP_ROUTE106),
    [REMATCH_ISAIAH] = REMATCH(TRAINER_ISAIAH_1, TRAINER_ISAIAH_2, TRAINER_ISAIAH_3, TRAINER_ISAIAH_4, TRAINER_ISAIAH_5, MAP_ROUTE128),
    [REMATCH_MARIA] = REMATCH(TRAINER_MARIA_1, TRAINER_MARIA_2, TRAINER_MARIA_3, TRAINER_MARIA_4, TRAINER_MARIA_5, MAP_ROUTE117),
    [REMATCH_ABIGAIL] = REMATCH(TRAINER_ABIGAIL_1, TRAINER_ABIGAIL_2, TRAINER_ABIGAIL_3, TRAINER_ABIGAIL_4, TRAINER_ABIGAIL_5, MAP_ROUTE110),
    [REMATCH_DYLAN] = REMATCH(TRAINER_DYLAN_1, TRAINER_DYLAN_2, TRAINER_DYLAN_3, TRAINER_DYLAN_4, TRAINER_DYLAN_5, MAP_ROUTE117),
    [REMATCH_KATELYN] = REMATCH(TRAINER_KATELYN_1, TRAINER_KATELYN_2, TRAINER_KATELYN_3, TRAINER_KATELYN_4, TRAINER_KATELYN_5, MAP_ROUTE128),
    [REMATCH_BENJAMIN] = REMATCH(TRAINER_BENJAMIN_1, TRAINER_BENJAMIN_2, TRAINER_BENJAMIN_3, TRAINER_BENJAMIN_4, TRAINER_BENJAMIN_5, MAP_ROUTE110),
    [REMATCH_PABLO] = REMATCH(TRAINER_PABLO_1, TRAINER_PABLO_2, TRAINER_PABLO_3, TRAINER_PABLO_4, TRAINER_PABLO_5, MAP_ROUTE126),
    [REMATCH_NICOLAS] = REMATCH(TRAINER_NICOLAS_1, TRAINER_NICOLAS_2, TRAINER_NICOLAS_3, TRAINER_NICOLAS_4, TRAINER_NICOLAS_5, MAP_METEOR_FALLS_1F_2R),
    [REMATCH_ROBERT] = REMATCH(TRAINER_ROBERT_1, TRAINER_ROBERT_2, TRAINER_ROBERT_3, TRAINER_ROBERT_4, TRAINER_ROBERT_5, MAP_ROUTE120),
    [REMATCH_LAO] = REMATCH(TRAINER_LAO_1, TRAINER_LAO_2, TRAINER_LAO_3, TRAINER_LAO_4, TRAINER_LAO_5, MAP_ROUTE113),
    [REMATCH_CYNDY] = REMATCH(TRAINER_CYNDY_1, TRAINER_CYNDY_2, TRAINER_CYNDY_3, TRAINER_CYNDY_4, TRAINER_CYNDY_5, MAP_ROUTE115),
    [REMATCH_MADELINE] = REMATCH(TRAINER_MADELINE_1, TRAINER_MADELINE_2, TRAINER_MADELINE_3, TRAINER_MADELINE_4, TRAINER_MADELINE_5, MAP_ROUTE113),
    [REMATCH_JENNY] = REMATCH(TRAINER_JENNY_1, TRAINER_JENNY_2, TRAINER_JENNY_3, TRAINER_JENNY_4, TRAINER_JENNY_5, MAP_ROUTE124),
    [REMATCH_DIANA] = REMATCH(TRAINER_DIANA_1, TRAINER_DIANA_2, TRAINER_DIANA_3, TRAINER_DIANA_4, TRAINER_DIANA_5, MAP_JAGGED_PASS),
    [REMATCH_AMY_AND_LIV] = REMATCH(TRAINER_AMY_AND_LIV_1, TRAINER_AMY_AND_LIV_2, TRAINER_AMY_AND_LIV_4, TRAINER_AMY_AND_LIV_5, TRAINER_AMY_AND_LIV_6, MAP_ROUTE103),
    [REMATCH_ERNEST] = REMATCH(TRAINER_ERNEST_1, TRAINER_ERNEST_2, TRAINER_ERNEST_3, TRAINER_ERNEST_4, TRAINER_ERNEST_5, MAP_ROUTE125),
    [REMATCH_CORY] = REMATCH(TRAINER_CORY_1, TRAINER_CORY_2, TRAINER_CORY_3, TRAINER_CORY_4, TRAINER_CORY_5, MAP_ROUTE108),
    [REMATCH_EDWIN] = REMATCH(TRAINER_EDWIN_1, TRAINER_EDWIN_2, TRAINER_EDWIN_3, TRAINER_EDWIN_4, TRAINER_EDWIN_5, MAP_ROUTE110),
    [REMATCH_LYDIA] = REMATCH(TRAINER_LYDIA_1, TRAINER_LYDIA_2, TRAINER_LYDIA_3, TRAINER_LYDIA_4, TRAINER_LYDIA_5, MAP_ROUTE117),
    [REMATCH_ISAAC] = REMATCH(TRAINER_ISAAC_1, TRAINER_ISAAC_2, TRAINER_ISAAC_3, TRAINER_ISAAC_4, TRAINER_ISAAC_5, MAP_ROUTE117),
    [REMATCH_GABRIELLE] = REMATCH(TRAINER_GABRIELLE_1, TRAINER_GABRIELLE_2, TRAINER_GABRIELLE_3, TRAINER_GABRIELLE_4, TRAINER_GABRIELLE_5, MAP_MT_PYRE_3F),
    [REMATCH_CATHERINE] = REMATCH(TRAINER_CATHERINE_1, TRAINER_CATHERINE_2, TRAINER_CATHERINE_3, TRAINER_CATHERINE_4, TRAINER_CATHERINE_5, MAP_ROUTE119),
    [REMATCH_JACKSON] = REMATCH(TRAINER_JACKSON_1, TRAINER_JACKSON_2, TRAINER_JACKSON_3, TRAINER_JACKSON_4, TRAINER_JACKSON_5, MAP_ROUTE119),
    [REMATCH_HALEY] = REMATCH(TRAINER_HALEY_1, TRAINER_HALEY_2, TRAINER_HALEY_3, TRAINER_HALEY_4, TRAINER_HALEY_5, MAP_ROUTE104),
    [REMATCH_JAMES] = REMATCH(TRAINER_JAMES_1, TRAINER_JAMES_2, TRAINER_JAMES_3, TRAINER_JAMES_4, TRAINER_JAMES_5, MAP_PETALBURG_WOODS),
    [REMATCH_TRENT] = REMATCH(TRAINER_TRENT_1, TRAINER_TRENT_2, TRAINER_TRENT_3, TRAINER_TRENT_4, TRAINER_TRENT_5, MAP_ROUTE112),
    [REMATCH_SAWYER] = REMATCH(TRAINER_SAWYER_1, TRAINER_SAWYER_2, TRAINER_SAWYER_3, TRAINER_SAWYER_4, TRAINER_SAWYER_5, MAP_MT_CHIMNEY),
    [REMATCH_KIRA_AND_DAN] = REMATCH(TRAINER_KIRA_AND_DAN_1, TRAINER_KIRA_AND_DAN_2, TRAINER_KIRA_AND_DAN_3, TRAINER_KIRA_AND_DAN_4, TRAINER_KIRA_AND_DAN_5, MAP_ABANDONED_SHIP_ROOMS2_1F),
    [REMATCH_WALLY_VR] = REMATCH(TRAINER_WALLY_VR_2, TRAINER_WALLY_VR_3, TRAINER_WALLY_VR_4, TRAINER_WALLY_VR_5, TRAINER_WALLY_VR_5, MAP_VICTORY_ROAD_1F),
    [REMATCH_ROXANNE] = REMATCH(TRAINER_ROXANNE_1, TRAINER_ROXANNE_2, TRAINER_ROXANNE_3, TRAINER_ROXANNE_4, TRAINER_ROXANNE_5, MAP_RUSTBORO_CITY),
    [REMATCH_BRAWLY] = REMATCH(TRAINER_BRAWLY_1, TRAINER_BRAWLY_2, TRAINER_BRAWLY_3, TRAINER_BRAWLY_4, TRAINER_BRAWLY_5, MAP_DEWFORD_TOWN),
    [REMATCH_WATTSON] = REMATCH(TRAINER_WATTSON_1, TRAINER_WATTSON_2, TRAINER_WATTSON_3, TRAINER_WATTSON_4, TRAINER_WATTSON_5, MAP_MAUVILLE_CITY),
    [REMATCH_FLANNERY] = REMATCH(TRAINER_FLANNERY_1, TRAINER_FLANNERY_2, TRAINER_FLANNERY_3, TRAINER_FLANNERY_4, TRAINER_FLANNERY_5, MAP_LAVARIDGE_TOWN),
    [REMATCH_NORMAN] = REMATCH(TRAINER_NORMAN_1, TRAINER_NORMAN_2, TRAINER_NORMAN_3, TRAINER_NORMAN_4, TRAINER_NORMAN_5, MAP_PETALBURG_CITY),
    [REMATCH_WINONA] = REMATCH(TRAINER_WINONA_1, TRAINER_WINONA_2, TRAINER_WINONA_3, TRAINER_WINONA_4, TRAINER_WINONA_5, MAP_FORTREE_CITY),
    [REMATCH_TATE_AND_LIZA] = REMATCH(TRAINER_TATE_AND_LIZA_1, TRAINER_TATE_AND_LIZA_2, TRAINER_TATE_AND_LIZA_3, TRAINER_TATE_AND_LIZA_4, TRAINER_TATE_AND_LIZA_5, MAP_MOSSDEEP_CITY),
    [REMATCH_JUAN] = REMATCH(TRAINER_JUAN_1, TRAINER_JUAN_2, TRAINER_JUAN_3, TRAINER_JUAN_4, TRAINER_JUAN_5, MAP_SOOTOPOLIS_CITY),
    [REMATCH_SIDNEY] = REMATCH(TRAINER_SIDNEY, TRAINER_SIDNEY, TRAINER_SIDNEY, TRAINER_SIDNEY, TRAINER_SIDNEY, MAP_EVER_GRANDE_CITY),
    [REMATCH_PHOEBE] = REMATCH(TRAINER_PHOEBE, TRAINER_PHOEBE, TRAINER_PHOEBE, TRAINER_PHOEBE, TRAINER_PHOEBE, MAP_EVER_GRANDE_CITY),
    [REMATCH_GLACIA] = REMATCH(TRAINER_GLACIA, TRAINER_GLACIA, TRAINER_GLACIA, TRAINER_GLACIA, TRAINER_GLACIA, MAP_EVER_GRANDE_CITY),
    [REMATCH_DRAKE] = REMATCH(TRAINER_DRAKE, TRAINER_DRAKE, TRAINER_DRAKE, TRAINER_DRAKE, TRAINER_DRAKE, MAP_EVER_GRANDE_CITY),
    [REMATCH_WALLACE] = REMATCH(TRAINER_WALLACE, TRAINER_WALLACE, TRAINER_WALLACE, TRAINER_WALLACE, TRAINER_WALLACE, MAP_EVER_GRANDE_CITY),
};

#define tState data[0]
#define tTransition data[1]

/* Original task state/transition stay in data[0..1]; the rest is transient. */
enum ADMG_FieldBattleTaskData
{
    ADMG_FB_FLAGS = 2, ADMG_FB_USES = 4, ADMG_FB_PARTNER = 6, // the partner's trainer ID, as a word
    ADMG_FB_SAVED_CALLBACK = 8, ADMG_FB_TAG = 10,
};

// The opponents' trainer IDs are in the task's content slots.
enum { ADMG_FB_CONTENT_A, ADMG_FB_CONTENT_B };

static void Task_BattleStart(u8 taskId);
static void Task_BattleStart_Debug(u8 taskId);

static bool8 ADMG_TrySelectFieldBattlePartner(bool8 prepareFollower, enum TrainerId *identity, bool8 *deferred)
{
    *deferred = prepareFollower && (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
        && FollowerNPCIsBattlePartner();
    *identity = gPartnerTrainerId;
    if (*deferred)
    {
        u32 partner = GetFollowerNPCData(FNPC_DATA_BATTLE_PARTNER);
        if (partner <= PARTNER_NONE || partner >= PARTNER_COUNT)
        { ADMG_StateAccessRejected(partner, __func__); return FALSE; }
        *identity = TRAINER_PARTNER(partner);
    }
    return TRUE;
}

static bool8 ADMG_FieldBattleActorsCurrent(const struct ADMG_BattleTrainerActors *snapshot)
{
    enum TrainerId partner; bool8 deferred;
    if (!ADMG_TrySelectFieldBattlePartner(TRUE, &partner, &deferred)) return FALSE;
    return ADMG_BattleTrainerActorsCurrent(snapshot, gBattleTypeFlags,
        TRAINER_BATTLE_PARAM.opponentA, TRAINER_BATTLE_PARAM.opponentB, partner);
}

static bool8 ADMG_FieldBattleTaskOwned(u8 taskId)
{
    return taskId < NUM_TASKS && gTasks[taskId].isActive
        && (gTasks[taskId].func == Task_BattleStart || gTasks[taskId].func == Task_BattleStart_Debug)
        && (u16)gTasks[taskId].data[ADMG_FB_TAG] == 0x4B35;
}

static bool8 ADMG_FieldBattleContextCurrent(u8 taskId, u64 generation)
{
    if (!ADMG_FieldBattleTaskOwned(taskId))
    { ADMG_StateAccessRejected(taskId, __func__); return FALSE; }
    const s16 *data = gTasks[taskId].data;
    if (data[0] < 0 || data[0] > 1)
    { ADMG_StateAccessRejected((u16)data[0], __func__); return FALSE; }
    struct ADMG_BattleTrainerActors snapshot;
    snapshot.flags = GetWordTaskArg(taskId, ADMG_FB_FLAGS);
    snapshot.opponentA = gTasks[taskId].contentIds[ADMG_FB_CONTENT_A]; snapshot.opponentB = gTasks[taskId].contentIds[ADMG_FB_CONTENT_B];
    snapshot.partner = GetWordTaskArg(taskId, ADMG_FB_PARTNER); snapshot.uses = data[ADMG_FB_USES] & 7;
    enum TrainerId partner; bool8 deferred;
    if (!ADMG_TrySelectFieldBattlePartner(gTasks[taskId].func == Task_BattleStart, &partner, &deferred)) return FALSE;
    if (deferred != ((data[ADMG_FB_USES] & 8) != 0)
     || (deferred && partner != snapshot.partner)
     || (u32)gMain.savedCallback != GetWordTaskArg(taskId, ADMG_FB_SAVED_CALLBACK))
    { ADMG_StateAccessRejected(partner, __func__); return FALSE; }
    if (!deferred || (data[ADMG_FB_USES] & 16)) partner = gPartnerTrainerId;
    return ADMG_EndStateAccessCheck(generation)
        && ADMG_BattleTrainerActorsCurrent(&snapshot, gBattleTypeFlags,
            TRAINER_BATTLE_PARAM.opponentA, TRAINER_BATTLE_PARAM.opponentB, partner);
}

static void ADMG_RejectFieldBattleTask(u8 taskId)
{
    if (!ADMG_FieldBattleTaskOwned(taskId)) return;
    DestroyTask(taskId);
    ADMG_DiscardNativeBattleAssociation();
    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

static bool8 ADMG_QueueFieldBattleTask(TaskFunc func, enum BattleTransition transition, u16 song)
{
    u64 generation = ADMG_BeginStateAccessCheck();
    struct ADMG_BattleTrainerActors snapshot;
    enum TrainerId partner; bool8 deferred;
    if (!ADMG_TrySelectFieldBattlePartner(func == Task_BattleStart, &partner, &deferred)
     || !ADMG_TryCaptureBattleTrainerActors(gBattleTypeFlags, TRAINER_BATTLE_PARAM.opponentA,
        TRAINER_BATTLE_PARAM.opponentB, partner, &snapshot)
     || !ADMG_EndStateAccessCheck(generation)) return FALSE;
    u32 freeSlot;
    for (freeSlot = 0; freeSlot < NUM_TASKS && gTasks[freeSlot].isActive; ++freeSlot) {}
    if (freeSlot == NUM_TASKS)
    { ADMG_StateAccessRejected(freeSlot, __func__); return FALSE; }
    u8 taskId = CreateTask(func, 1);
    if (taskId != freeSlot || !gTasks[taskId].isActive || gTasks[taskId].func != func)
    { ADMG_StateAccessRejected(taskId, __func__); return FALSE; }
    gTasks[taskId].tTransition = transition;
    SetWordTaskArg(taskId, ADMG_FB_FLAGS, snapshot.flags);
    gTasks[taskId].contentIds[ADMG_FB_CONTENT_A] = snapshot.opponentA;
    gTasks[taskId].contentIds[ADMG_FB_CONTENT_B] = snapshot.opponentB;
    SetWordTaskArg(taskId, ADMG_FB_PARTNER, snapshot.partner);
    gTasks[taskId].data[ADMG_FB_USES] = snapshot.uses | (deferred << 3);
    SetWordTaskArg(taskId, ADMG_FB_SAVED_CALLBACK, (u32)gMain.savedCallback);
    gTasks[taskId].data[ADMG_FB_TAG] = 0x4B35;
    PlayMapChosenOrBattleBGM(song);
    if (!ADMG_FieldBattleContextCurrent(taskId, generation))
    { ADMG_RejectFieldBattleTask(taskId); return FALSE; }
    return TRUE;
}

static void Task_BattleStart(u8 taskId)
{
    if (taskId >= NUM_TASKS || !gTasks[taskId].isActive
     || gTasks[taskId].func != Task_BattleStart)
    { ADMG_StateAccessRejected(taskId, __func__); return; }
    u64 generation = ADMG_BeginStateAccessCheck();
    if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        bool8 poison = FldEffPoison_IsActive();
        if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
        if (!poison) // is poison not active?
        {
            BattleTransition_StartOnField(tTransition);
            if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
            ClearMirageTowerPulseBlendEffect();
            if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
            tState++; // go to case 1.
        }
        break;
    case 1:
        bool8 done = IsBattleTransitionDone();
        if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
        if (done == TRUE)
        {
            PrepareForFollowerNPCBattle();
            data[ADMG_FB_USES] |= 16;
            if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
            CleanupOverworldWindowsAndTilemaps();
            if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
            SetMainCallback2(CB2_InitBattle);
            if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
            RestartWildEncounterImmunitySteps();
            if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
            ClearPoisonStepCounter();
            if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
            DespawnOWEOnBattleStart();
            if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
            DestroyTask(taskId);
        }
        break;
    }
    return;
rejected:
    ADMG_RejectFieldBattleTask(taskId);
}

static void CreateBattleStartTask(enum BattleTransition transition, u16 song)
{
    (void)ADMG_QueueFieldBattleTask(Task_BattleStart, transition, song);
}

static void Task_BattleStart_Debug(u8 taskId)
{
    if (taskId >= NUM_TASKS || !gTasks[taskId].isActive
     || gTasks[taskId].func != Task_BattleStart_Debug)
    { ADMG_StateAccessRejected(taskId, __func__); return; }
    u64 generation = ADMG_BeginStateAccessCheck();
    if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
    s16 *data = gTasks[taskId].data;

    switch (tState)
    {
    case 0:
        bool8 poison = FldEffPoison_IsActive();
        if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
        if (!poison) // is poison not active?
        {
            BattleTransition_StartOnField(tTransition);
            if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
            ClearMirageTowerPulseBlendEffect();
            if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
            tState++; // go to case 1.
        }
        break;
    case 1:
        bool8 done = IsBattleTransitionDone();
        if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
        if (done == TRUE)
        {
            CleanupOverworldWindowsAndTilemaps();
            if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
            SetMainCallback2(CB2_InitBattle);
            if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
            RestartWildEncounterImmunitySteps();
            if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
            ClearPoisonStepCounter();
            if (!ADMG_FieldBattleContextCurrent(taskId, generation)) goto rejected;
            DestroyTask(taskId);
        }
        break;
    }
    return;
rejected:
    ADMG_RejectFieldBattleTask(taskId);
}

static void CreateBattleStartTask_Debug(u8 transition, u16 song)
{
    (void)ADMG_QueueFieldBattleTask(Task_BattleStart_Debug, transition, song);
}

#undef tState
#undef tTransition

static bool8 CheckSilphScopeInPokemonTower(MapId mapId)
{
    if ((mapId == MAP_POKEMON_TOWER_1F
      || mapId == MAP_POKEMON_TOWER_2F
      || mapId == MAP_POKEMON_TOWER_3F
      || mapId == MAP_POKEMON_TOWER_4F
      || mapId == MAP_POKEMON_TOWER_5F
      || mapId == MAP_POKEMON_TOWER_6F
      || mapId == MAP_POKEMON_TOWER_7F)
     && !(CheckBagHasItem(ITEM_SILPH_SCOPE, 1)))
        return TRUE;
    else
        return FALSE;
}

void BattleSetup_StartWildBattle(void)
{
    if (GetSafariZoneFlag())
        DoSafariBattle();
    else if (CheckSilphScopeInPokemonTower(gSaveBlock1Ptr->location.mapId))
        DoGhostBattle();
    else
        DoStandardWildBattle(FALSE);
}

void BattleSetup_StartDoubleWildBattle(void)
{
    DoStandardWildBattle(TRUE);
}

static bool8 ADMG_StartMultiBattle(void);

// Resume the script so a refused start doesn't leave the multi-battle macros stuck at waitstate
// BattleSetup_StartMultiBattle sets VAR_RESULT to ADMG_BATTLE_REQUEST_REFUSED before queuing this task
static void Task_ADMG_ResumeRefusedBattleScript(u8 taskId)
{
    DestroyTask(taskId);
    ScriptContext_Enable();
}

void BattleSetup_StartMultiBattle(void)
{
    void (*savedCallback)(void) = gMain.savedCallback;
    if (ADMG_StartMultiBattle())
        return;
    gMain.savedCallback = savedCallback;
    gSpecialVar_Result = ADMG_BATTLE_REQUEST_REFUSED;
    CreateTask(Task_ADMG_ResumeRefusedBattleScript, 80);
}

static bool8 ADMG_HasUsableMultiBattleMon(void)
{
    // Look for a living, non-Egg Pokémon in the first half of the player's party
    for (u32 i = 0; i < PARTY_SIZE / 2; i++)
    {
        if (GetMonData(&gPlayerParty[i], MON_DATA_SPECIES) != SPECIES_NONE && !GetMonData(&gPlayerParty[i], MON_DATA_IS_EGG)
         && GetMonData(&gPlayerParty[i], MON_DATA_HP) != 0)
            return TRUE;
    }
    return FALSE;
}

ADMG_NATIVE_ENTRY void ADMG_ScriptStartMultiBattle(struct ScriptContext *ctx)
{
    if (ctx == NULL || ctx->scriptPtr == NULL) { ADMG_StateAccessRejected(0, __func__); ADMG_StopInvalidStateCommand(ctx); return; }
    u32 format = ScriptReadHalfword(ctx);
    u32 opponentA = ScriptReadWord(ctx);
    const u8 *defeatTextA = (const u8 *)ScriptReadWord(ctx);
    u32 opponentB = ScriptReadWord(ctx);
    const u8 *defeatTextB = (const u8 *)ScriptReadWord(ctx);
    u32 partner = ScriptReadHalfword(ctx);
    u32 flags, type;

    switch (format)
    {
    case ADMG_MULTI_BATTLE_TWO_VS_TWO:
        flags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER;
        type = MULTI_BATTLE_2_VS_2;
        break;
    case ADMG_MULTI_BATTLE_TWO_VS_ONE:
        flags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER;
        type = MULTI_BATTLE_2_VS_1;
        break;
    default:
        gSpecialVar_Result = ADMG_BATTLE_REQUEST_INVALID_FORMAT;
        return;
    }
    if (partner == PARTNER_NONE || partner >= PARTNER_COUNT)
    {
        gSpecialVar_Result = ADMG_BATTLE_REQUEST_INVALID_PARTNER;
        return;
    }
    // In 2v1, both opposing Pokémon belong to opponentA
    // Require TRAINER_NONE for opponentB, then use the battle setup's 0xFFFF sentinel
    if (type == MULTI_BATTLE_2_VS_1 && opponentB != TRAINER_NONE)
    {
        gSpecialVar_Result = ADMG_BATTLE_REQUEST_INVALID_OPPONENT;
        return;
    }
    if (type == MULTI_BATTLE_2_VS_1)
        opponentB = 0xFFFF;
    if (opponentA == TRAINER_NONE || (type == MULTI_BATTLE_2_VS_2 && (opponentB == TRAINER_NONE || opponentB == opponentA))
     || !ADMG_TryValidateBattleTrainerActors(flags, opponentA, opponentB, TRAINER_PARTNER(partner)))
    {
        gSpecialVar_Result = ADMG_BATTLE_REQUEST_INVALID_OPPONENT;
        return;
    }
    if (!ADMG_HasUsableMultiBattleMon())
    {
        gSpecialVar_Result = ADMG_BATTLE_REQUEST_NOT_ENOUGH_MONS;
        return;
    }

    TrainerBattleParameter previous = gTrainerBattleParameter;
    u32 previousPartner = gPartnerTrainerId, previousType = gSpecialVar_0x8005;
    void (*savedCallback)(void) = gMain.savedCallback;
    InitTrainerBattleParameter();
    TRAINER_BATTLE_PARAM.opponentA = opponentA;
    TRAINER_BATTLE_PARAM.defeatTextA = (u8 *)defeatTextA;
    TRAINER_BATTLE_PARAM.opponentB = type == MULTI_BATTLE_2_VS_1 ? TRAINER_NONE : opponentB;
    TRAINER_BATTLE_PARAM.defeatTextB = (u8 *)defeatTextB;
    gPartnerTrainerId = TRAINER_PARTNER(partner);
    gSpecialVar_0x8005 = type;
    if (ADMG_StartMultiBattle())
    {
        gSpecialVar_Result = ADMG_BATTLE_REQUEST_STARTED;
        return;
    }
    gTrainerBattleParameter = previous;
    gPartnerTrainerId = previousPartner;
    gSpecialVar_0x8005 = previousType;
    gMain.savedCallback = savedCallback;
    gBattleTypeFlags = 0;
    gSpecialVar_Result = ADMG_BATTLE_REQUEST_REFUSED;
}

static bool8 ADMG_StartMultiBattle(void)
{
    gBattleScripting.specialTrainerBattleType = SPECIAL_BATTLE_MULTI;
    gMain.savedCallback = CB2_EndSpecialTrainerBattle;

    if (gSpecialVar_0x8005 & MULTI_BATTLE_2_VS_WILD) // Player + AI against wild mon
    {
        gBattleTypeFlags = BATTLE_TYPE_DOUBLE | BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER;
    }
    else if (gSpecialVar_0x8005 & MULTI_BATTLE_2_VS_1) // Player + AI against one trainer
    {
        TRAINER_BATTLE_PARAM.opponentB = 0xFFFF;
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER;
    }
    else // MULTI_BATTLE_2_VS_2
    {
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER;
    }

    FillPartnerParty(gPartnerTrainerId);
    if (gSpecialVar_0x8005 & MULTI_BATTLE_CHOOSE_MONS) // Skip mons restoring(done in the script)
        gBattleScripting.specialTrainerBattleType = 0xFF;

    if (gSpecialVar_0x8005 & MULTI_BATTLE_2_VS_WILD)
    {
        { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(GetWildBattleTransition(), 0); if (!ADMG_EndStateAccessCheck(queued)) return FALSE; }
        IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
        IncrementGameStat(GAME_STAT_WILD_BATTLES);
        IncrementDailyWildBattles();
        TryUpdateGymLeaderRematchFromWild();
        return TRUE;
    }
    return DoTrainerBattle();
}


void BattleSetup_StartBattlePikeWildBattle(void)
{
    DoBattlePikeWildBattle();
}

static void DoStandardWildBattle(bool32 isDouble)
{
    LockPlayerFieldControls();
    FreezeObjectEvents();
    StopPlayerAvatar();
    gMain.savedCallback = CB2_EndWildBattle;
    gBattleTypeFlags = 0;
    if (IsNPCFollowerWildBattle())
    {
        gBattleTypeFlags |= BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_DOUBLE;
    }
    else if (isDouble)
        gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
    {
        VarSet(VAR_TEMP_E, 0);
        gBattleTypeFlags |= BATTLE_TYPE_PYRAMID;
    }
    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(GetWildBattleTransition(), 0); if (!ADMG_EndStateAccessCheck(queued)) return; }
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
    TryUpdateGymLeaderRematchFromWild();
}

void DoStandardWildBattle_Debug(void)
{
    LockPlayerFieldControls();
    FreezeObjectEvents();
    StopPlayerAvatar();
    gMain.savedCallback = CB2_EndWildBattle;
    gBattleTypeFlags = 0;
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
    {
        VarSet(VAR_TEMP_PLAYING_PYRAMID_MUSIC, 0);
        gBattleTypeFlags |= BATTLE_TYPE_PYRAMID;
    }
    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask_Debug(GetWildBattleTransition(), 0); if (!ADMG_EndStateAccessCheck(queued)) return; }
    //IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    //IncrementGameStat(GAME_STAT_WILD_BATTLES);
    //IncrementDailyWildBattles();
    //TryUpdateGymLeaderRematchFromWild();
}

void BattleSetup_StartRoamerBattle(void)
{
    LockPlayerFieldControls();
    FreezeObjectEvents();
    StopPlayerAvatar();
    gMain.savedCallback = CB2_EndWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_ROAMER;
    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(GetWildBattleTransition(), 0); if (!ADMG_EndStateAccessCheck(queued)) return; }
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
    TryUpdateGymLeaderRematchFromWild();
}

static void DoSafariBattle(void)
{
    LockPlayerFieldControls();
    FreezeObjectEvents();
    StopPlayerAvatar();
    gMain.savedCallback = CB2_EndSafariBattle;
    gBattleTypeFlags = BATTLE_TYPE_SAFARI;
    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(GetWildBattleTransition(), 0); if (!ADMG_EndStateAccessCheck(queued)) return; }
}

static void DoGhostBattle(void)
{
    LockPlayerFieldControls();
    FreezeObjectEvents();
    StopPlayerAvatar();
    gMain.savedCallback = CB2_EndWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_GHOST;
    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(GetWildBattleTransition(), 0); if (!ADMG_EndStateAccessCheck(queued)) return; }
    SetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_NICKNAME, gText_Ghost);
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
}

static void DoBattlePikeWildBattle(void)
{
    LockPlayerFieldControls();
    FreezeObjectEvents();
    StopPlayerAvatar();
    gMain.savedCallback = CB2_EndWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_PIKE;
    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(GetWildBattleTransition(), 0); if (!ADMG_EndStateAccessCheck(queued)) return; }
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
    TryUpdateGymLeaderRematchFromWild();
}

static bool8 DoTrainerBattle(void)
{
    u64 generation = ADMG_BeginStateAccessCheck();
    struct ADMG_BattleTrainerActors snapshot;
    enum TrainerId partner; bool8 deferred;
    if (!ADMG_TrySelectFieldBattlePartner(TRUE, &partner, &deferred)
     || !ADMG_TryCaptureBattleTrainerActors(gBattleTypeFlags, TRAINER_BATTLE_PARAM.opponentA,
        TRAINER_BATTLE_PARAM.opponentB, partner, &snapshot)
     || !ADMG_EndStateAccessCheck(generation)) goto rejected;

    if (IsNativeTrainerBattleContent(TRAINER_BATTLE_PARAM.opponentA))
        ADMG_BeginNativeTrainerBattle(TRAINER_BATTLE_PARAM.opponentA);
    else ADMG_DiscardNativeBattleAssociation();
    if (!ADMG_EndStateAccessCheck(generation) || !ADMG_FieldBattleActorsCurrent(&snapshot)) goto rejected;

    CreateNPCTrainerParty(&gParties[B_TRAINER_OPPONENT_A][0], TRAINER_BATTLE_PARAM.opponentA);
    if (!ADMG_EndStateAccessCheck(generation) || !ADMG_FieldBattleActorsCurrent(&snapshot)) goto rejected;
    if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS && !BATTLE_TWO_VS_ONE_OPPONENT)
        CreateNPCTrainerParty(&gParties[B_TRAINER_OPPONENT_B][0], TRAINER_BATTLE_PARAM.opponentB);
    if (!ADMG_EndStateAccessCheck(generation) || !ADMG_FieldBattleActorsCurrent(&snapshot)) goto rejected;
    enum BattleTransition transition = GetTrainerBattleTransition();
    if (!ADMG_EndStateAccessCheck(generation) || !ADMG_FieldBattleActorsCurrent(&snapshot)) goto rejected;
    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(transition, 0); if (!ADMG_EndStateAccessCheck(queued)) goto rejected; }
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_TRAINER_BATTLES);
    TryUpdateGymLeaderRematchFromTrainer();
    return TRUE;
rejected:
    ADMG_DiscardNativeBattleAssociation();
    return FALSE;
}

static void DoBattlePyramidTrainerHillBattle(void)
{
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
        { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(GetSpecialBattleTransition(B_TRANSITION_GROUP_B_PYRAMID), 0); if (!ADMG_EndStateAccessCheck(queued)) return; }
    else
        { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(GetSpecialBattleTransition(B_TRANSITION_GROUP_TRAINER_HILL), 0); if (!ADMG_EndStateAccessCheck(queued)) return; }

    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_TRAINER_BATTLES);
    TryUpdateGymLeaderRematchFromTrainer();
}

// Initiates battle where Wally catches Ralts
void StartWallyTutorialBattle(void)
{
    CreateMaleMon(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_RALTS, 5);
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_ReturnToFieldContinueScriptPlayMapMusic;
    gBattleTypeFlags = BATTLE_TYPE_CATCH_TUTORIAL;
    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(B_TRANSITION_SLICE, 0); if (!ADMG_EndStateAccessCheck(queued)) return; }
}

void StartOldManTutorialBattle(void)
{
    CreateMaleMon(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_WEEDLE, 5);
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_ReturnToFieldContinueScriptPlayMapMusic;
    gBattleTypeFlags = BATTLE_TYPE_CATCH_TUTORIAL;
    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(B_TRANSITION_SLICE, 0); if (!ADMG_EndStateAccessCheck(queued)) return; }
}

void BattleSetup_StartScriptedWildBattle(void)
{
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_EndScriptedWildBattle;
    gBattleTypeFlags = 0;
    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(GetWildBattleTransition(), 0); if (!ADMG_EndStateAccessCheck(queued)) return; }
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
    TryUpdateGymLeaderRematchFromWild();
}

void BattleSetup_StartScriptedDoubleWildBattle(void)
{
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_EndScriptedWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_DOUBLE;
    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(GetWildBattleTransition(), 0); if (!ADMG_EndStateAccessCheck(queued)) return; }
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
    TryUpdateGymLeaderRematchFromWild();
}

void StartMarowakBattle(void)
{
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_EndMarowakBattle;
    gBattleTypeFlags = BATTLE_TYPE_GHOST;

    if (CheckBagHasItem(ITEM_SILPH_SCOPE, 1))
    {
        u32 personality = GetMonPersonality(SPECIES_MAROWAK, MON_FEMALE, NATURE_SERIOUS, RANDOM_UNOWN_LETTER);

        CreateMonWithIVsPersonality(&gParties[B_TRAINER_OPPONENT_A][0], SPECIES_MAROWAK, 30, 31, personality);
    }

    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(GetWildBattleTransition(), 0); if (!ADMG_EndStateAccessCheck(queued)) return; }
    SetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_NICKNAME, gText_Ghost);
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
}

void BattleSetup_StartLatiBattle(void)
{
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_EndScriptedWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_LEGENDARY;
    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(GetWildBattleTransition(), 0); if (!ADMG_EndStateAccessCheck(queued)) return; }
    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
    TryUpdateGymLeaderRematchFromWild();
}

void BattleSetup_StartLegendaryBattle(void)
{
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_EndScriptedWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_LEGENDARY;

    switch (GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES))
    {
    case SPECIES_GROUDON:
    case SPECIES_GROUDON_PRIMAL:
        { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(B_TRANSITION_GROUDON, MUS_VS_KYOGRE_GROUDON); if (!ADMG_EndStateAccessCheck(queued)) return; }
        break;
    case SPECIES_KYOGRE:
    case SPECIES_KYOGRE_PRIMAL:
        { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(B_TRANSITION_KYOGRE, MUS_VS_KYOGRE_GROUDON); if (!ADMG_EndStateAccessCheck(queued)) return; }
        break;
    case SPECIES_RAYQUAZA:
    case SPECIES_RAYQUAZA_MEGA:
        { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(B_TRANSITION_RAYQUAZA, MUS_VS_RAYQUAZA); if (!ADMG_EndStateAccessCheck(queued)) return; }
        break;
    case SPECIES_DEOXYS_NORMAL:
    case SPECIES_DEOXYS_ATTACK:
    case SPECIES_DEOXYS_DEFENSE:
    case SPECIES_DEOXYS_SPEED:
        { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(B_TRANSITION_BLUR, MUS_RG_VS_DEOXYS); if (!ADMG_EndStateAccessCheck(queued)) return; }
        break;
    case SPECIES_LUGIA:
    case SPECIES_HO_OH:
    default:
        { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(B_TRANSITION_BLUR, MUS_RG_VS_LEGEND); if (!ADMG_EndStateAccessCheck(queued)) return; }
        break;
    case SPECIES_MEW:
        { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(B_TRANSITION_GRID_SQUARES, MUS_VS_MEW); if (!ADMG_EndStateAccessCheck(queued)) return; }
        break;
    }

    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
    TryUpdateGymLeaderRematchFromWild();
}

void StartGroudonKyogreBattle(void)
{
    LockPlayerFieldControls();
    gMain.savedCallback = CB2_EndScriptedWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_LEGENDARY;

    if (gGameVersion == VERSION_RUBY)
        CreateBattleStartTask(B_TRANSITION_ANGLED_WIPES, MUS_VS_KYOGRE_GROUDON); // GROUDON
    else
        CreateBattleStartTask(B_TRANSITION_RIPPLE, MUS_VS_KYOGRE_GROUDON); // KYOGRE

    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
    TryUpdateGymLeaderRematchFromWild();
}

void StartRegiBattle(void)
{
    enum BattleTransition transitionId;
    enum Species species;

    LockPlayerFieldControls();
    gMain.savedCallback = CB2_EndScriptedWildBattle;
    gBattleTypeFlags = BATTLE_TYPE_LEGENDARY;

    species = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES);
    switch (species)
    {
    case SPECIES_REGIROCK:
        transitionId = B_TRANSITION_REGIROCK;
        break;
    case SPECIES_REGICE:
        transitionId = B_TRANSITION_REGICE;
        break;
    case SPECIES_REGISTEEL:
        transitionId = B_TRANSITION_REGISTEEL;
        break;
    default:
        transitionId = B_TRANSITION_GRID_SQUARES;
        break;
    }
    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask(transitionId, MUS_VS_REGI); if (!ADMG_EndStateAccessCheck(queued)) return; }

    IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
    IncrementGameStat(GAME_STAT_WILD_BATTLES);
    IncrementDailyWildBattles();
    TryUpdateGymLeaderRematchFromWild();
}

static void DowngradeBadPoison(void)
{
    u8 i;
    u32 status = STATUS1_POISON;
    if (B_TOXIC_REVERSAL < GEN_5)
        return;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SANITY_HAS_SPECIES) && GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_STATUS) == STATUS1_TOXIC_POISON)
            SetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_STATUS, &status);
    }
}

static void CB2_EndWildBattle(void)
{
    CpuFill16(0, (void *)(BG_PLTT), BG_PLTT_SIZE);
    ResetOamRange(0, 128);

    if (IsNPCFollowerWildBattle())
    {
        RestorePartyAfterFollowerNPCBattle();
        if (ADMG_GetFeatureBinding(ADMG_BINDING_FNPC_FLAG_HEAL_AFTER_FOLLOWER_BATTLE) != 0
         && (ADMG_GetFeatureBinding(ADMG_BINDING_FNPC_FLAG_HEAL_AFTER_FOLLOWER_BATTLE) == FNPC_ALWAYS
         || FlagGet(ADMG_GetFeatureBinding(ADMG_BINDING_FNPC_FLAG_HEAL_AFTER_FOLLOWER_BATTLE))))
            HealPlayerParty();
    }

    if (IsPlayerDefeated(gBattleOutcome) == TRUE && CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE && !InBattlePike())
    {
        SetMainCallback2(CB2_WhiteOut);
    }
    else
    {
        SetMainCallback2(CB2_ReturnToField);
        DowngradeBadPoison();
        gFieldCallback = FieldCB_ReturnToFieldNoScriptCheckMusic;
    }
}

static void CB2_EndScriptedWildBattle(void)
{
    CpuFill16(0, (void *)(BG_PLTT), BG_PLTT_SIZE);
    ResetOamRange(0, 128);

    if (IsPlayerDefeated(gBattleOutcome) == TRUE)
    {
        if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
            SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
        else
            SetMainCallback2(CB2_WhiteOut);
    }
    else
    {
        DowngradeBadPoison();
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
    }
}

static void CB2_EndMarowakBattle(void)
{
    CpuFill16(0, (void *)BG_PLTT, BG_PLTT_SIZE);
    ResetOamRange(0, 128);

    if (IsPlayerDefeated(gBattleOutcome))
    {
        SetMainCallback2(CB2_WhiteOut);
    }
    else
    {
        // If result is TRUE player didnt defeat Marowak, force player back from stairs
        if (gBattleOutcome == B_OUTCOME_WON)
            gSpecialVar_Result = FALSE;
        else
            gSpecialVar_Result = TRUE;
        DowngradeBadPoison();
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
    }
}

enum BattleEnvironments BattleSetup_GetEnvironmentId(void)
{
    u16 tileBehavior;
    s16 x, y;

    if (ShouldUseFishingEnvironmentInBattle())
        GetXYCoordsOneStepInFrontOfPlayer(&x, &y);
    else
        PlayerGetDestCoords(&x, &y);

    tileBehavior = MapGridGetMetatileBehaviorAt(x, y);

    if (MetatileBehavior_IsTallGrass(tileBehavior))
        return BATTLE_ENVIRONMENT_GRASS;
    if (MetatileBehavior_IsLongGrass(tileBehavior))
        return BATTLE_ENVIRONMENT_LONG_GRASS;
    if (MetatileBehavior_IsSandOrDeepSand(tileBehavior))
        return BATTLE_ENVIRONMENT_SAND;

    switch (gMapHeader.mapType)
    {
    case MAP_TYPE_TOWN:
    case MAP_TYPE_CITY:
    case MAP_TYPE_ROUTE:
        break;
    case MAP_TYPE_UNDERGROUND:
        if (MetatileBehavior_IsIndoorEncounter(tileBehavior))
            return BATTLE_ENVIRONMENT_BUILDING;
        if (MetatileBehavior_IsSurfableWaterOrUnderwater(tileBehavior))
            return BATTLE_ENVIRONMENT_POND;
        return BATTLE_ENVIRONMENT_CAVE;
    case MAP_TYPE_INDOOR:
    case MAP_TYPE_SECRET_BASE:
        return BATTLE_ENVIRONMENT_BUILDING;
    case MAP_TYPE_UNDERWATER:
        return BATTLE_ENVIRONMENT_UNDERWATER;
    case MAP_TYPE_OCEAN_ROUTE:
        if (MetatileBehavior_IsSurfableWaterOrUnderwater(tileBehavior))
            return BATTLE_ENVIRONMENT_WATER;
        return BATTLE_ENVIRONMENT_PLAIN;
    }
    if (MetatileBehavior_IsDeepOrOceanWater(tileBehavior))
        return BATTLE_ENVIRONMENT_WATER;
    if (MetatileBehavior_IsSurfableWaterOrUnderwater(tileBehavior))
        return BATTLE_ENVIRONMENT_POND;
    if (MetatileBehavior_IsMountain(tileBehavior))
        return BATTLE_ENVIRONMENT_MOUNTAIN;
    if (TestPlayerAvatarFlags(PLAYER_AVATAR_FLAG_SURFING))
    {
        // Is BRIDGE_TYPE_POND_*?
        if (MetatileBehavior_GetBridgeType(tileBehavior) != BRIDGE_TYPE_OCEAN)
            return BATTLE_ENVIRONMENT_POND;

        if (MetatileBehavior_IsBridgeOverWater(tileBehavior) == TRUE)
            return BATTLE_ENVIRONMENT_WATER;
    }
    if (gSaveBlock1Ptr->location.mapId == MAP_ROUTE113)
        return BATTLE_ENVIRONMENT_SAND;
    if (GetSavedWeather() == WEATHER_SANDSTORM)
        return BATTLE_ENVIRONMENT_SAND;

    return BATTLE_ENVIRONMENT_PLAIN;
}

static enum TransitionType GetBattleTransitionTypeByMap(void)
{
    u16 tileBehavior;
    s16 x, y;

    PlayerGetDestCoords(&x, &y);
    tileBehavior = MapGridGetMetatileBehaviorAt(x, y);

    if (GetFlashLevel())
        return TRANSITION_TYPE_FLASH;

    if (MetatileBehavior_IsSurfableWaterOrUnderwater(tileBehavior))
        return TRANSITION_TYPE_WATER;

    switch (gMapHeader.mapType)
    {
    case MAP_TYPE_UNDERGROUND:
        return TRANSITION_TYPE_CAVE;
    case MAP_TYPE_UNDERWATER:
        return TRANSITION_TYPE_WATER;
    default:
        return TRANSITION_TYPE_NORMAL;
    }
}

static u16 GetSumOfPlayerPartyLevel(u8 numMons)
{
    u8 sum = 0;
    int i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        enum Species species = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_SPECIES_OR_EGG);

        if (species != SPECIES_EGG && species != SPECIES_NONE && GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HP) != 0)
        {
            sum += GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_LEVEL);
            if (--numMons == 0)
                break;
        }
    }
    return sum;
}

// Keep the full trainer ID; added trainers start at 0x10000
static u8 GetSumOfEnemyPartyLevel(enum TrainerId opponentId, u8 numMons)
{
    u8 i;
    u8 sum;
    u32 count = numMons;
    const struct TrainerMon *party;

    if (GetTrainerPartySizeFromId(opponentId) < count)
        count = GetTrainerPartySizeFromId(opponentId);

    sum = 0;

    party = GetTrainerPartyFromId(opponentId);
    for (i = 0; i < count && party != NULL; i++)
        sum += party[i].lvl;

    return sum;
}

enum BattleTransition GetWildBattleTransition(void)
{
    u8 transitionType = GetBattleTransitionTypeByMap();
    u8 enemyLevel = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL);
    u8 playerLevel = GetSumOfPlayerPartyLevel(1);

    if (enemyLevel < playerLevel)
    {
        if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
            return B_TRANSITION_BLUR;
        else
            return sBattleTransitionTable_Wild[transitionType][0];
    }
    else
    {
        if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
            return B_TRANSITION_GRID_SQUARES;
        else
            return sBattleTransitionTable_Wild[transitionType][1];
    }
}

enum BattleTransition GetTrainerBattleTransition(void)
{
    u8 minPartyCount = 1;
    u8 transitionType;
    u8 enemyLevel;
    u8 playerLevel;
    u32 trainerId = SanitizeTrainerId(TRAINER_BATTLE_PARAM.opponentA);
    enum TrainerClassID trainerClass = GetTrainerClassFromId(TRAINER_BATTLE_PARAM.opponentA);

    if (DoesTrainerHaveMugshot(trainerId))
        return B_TRANSITION_MUGSHOT;

    if (trainerClass == TRAINER_CLASS_TEAM_MAGMA
        || trainerClass == TRAINER_CLASS_MAGMA_LEADER
        || trainerClass == TRAINER_CLASS_MAGMA_ADMIN)
        return B_TRANSITION_MAGMA;

    if (trainerClass == TRAINER_CLASS_TEAM_AQUA
        || trainerClass == TRAINER_CLASS_AQUA_LEADER
        || trainerClass == TRAINER_CLASS_AQUA_ADMIN)
        return B_TRANSITION_AQUA;

    switch (GetTrainerBattleType(trainerId))
    {
    case TRAINER_BATTLE_TYPE_SINGLES:
        minPartyCount = 1;
        break;
    case TRAINER_BATTLE_TYPE_DOUBLES:
        minPartyCount = 2; // double battles always at least have 2 PokÃ©mon.
        break;
    }

    transitionType = GetBattleTransitionTypeByMap();
    enemyLevel = GetSumOfEnemyPartyLevel(trainerId, minPartyCount);
    playerLevel = GetSumOfPlayerPartyLevel(minPartyCount);

    if (enemyLevel < playerLevel)
        return sBattleTransitionTable_Trainer[transitionType][0];
    else
        return sBattleTransitionTable_Trainer[transitionType][1];
}

#define RANDOM_TRANSITION(table) (table[Random() % ARRAY_COUNT(table)])
enum BattleTransition GetSpecialBattleTransition(enum BattleTransitionGroup id)
{
    u16 var;
    u8 enemyLevel = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL);
    u8 playerLevel = GetSumOfPlayerPartyLevel(1);

    if (enemyLevel < playerLevel)
    {
        switch (id)
        {
        case B_TRANSITION_GROUP_TRAINER_HILL:
        case B_TRANSITION_GROUP_SECRET_BASE:
        case B_TRANSITION_GROUP_E_READER:
            return B_TRANSITION_POKEBALLS_TRAIL;
        case B_TRANSITION_GROUP_B_PYRAMID:
            return RANDOM_TRANSITION(sBattleTransitionTable_BattlePyramid);
        case B_TRANSITION_GROUP_B_DOME:
            return RANDOM_TRANSITION(sBattleTransitionTable_BattleDome);
        default:
            break;
        }

        if (VarGet(VAR_FRONTIER_BATTLE_MODE) != FRONTIER_MODE_LINK_MULTIS)
            return RANDOM_TRANSITION(sBattleTransitionTable_BattleFrontier);
    }
    else
    {
        switch (id)
        {
        case B_TRANSITION_GROUP_TRAINER_HILL:
        case B_TRANSITION_GROUP_SECRET_BASE:
        case B_TRANSITION_GROUP_E_READER:
            return B_TRANSITION_BIG_POKEBALL;
        case B_TRANSITION_GROUP_B_PYRAMID:
            return RANDOM_TRANSITION(sBattleTransitionTable_BattlePyramid);
        case B_TRANSITION_GROUP_B_DOME:
            return RANDOM_TRANSITION(sBattleTransitionTable_BattleDome);
        default:
            break;
        }

        if (VarGet(VAR_FRONTIER_BATTLE_MODE) != FRONTIER_MODE_LINK_MULTIS)
            return RANDOM_TRANSITION(sBattleTransitionTable_BattleFrontier);
    }

    var = gSaveBlock2Ptr->frontier.trainerIds[gSaveBlock2Ptr->frontier.curChallengeBattleNum * 2 + 0]
        + gSaveBlock2Ptr->frontier.trainerIds[gSaveBlock2Ptr->frontier.curChallengeBattleNum * 2 + 1];

    return sBattleTransitionTable_BattleFrontier[var % ARRAY_COUNT(sBattleTransitionTable_BattleFrontier)];
}

void ChooseStarter(void)
{
    SetMainCallback2(CB2_ChooseStarter);
    gMain.savedCallback = CB2_GiveStarter;
}

static void CB2_GiveStarter(void)
{
    u16 starterMon;

    *ADMG_REQUIRE_VAR_SPAN(VAR_STARTER_MON, 1) = gSpecialVar_Result;
    starterMon = GetStarterPokemon(gSpecialVar_Result);
    ScriptGiveMon(starterMon, 5, ITEM_NONE);
    ResetTasks();
    PlayBattleBGM();
    SetMainCallback2(CB2_StartFirstBattle);
    BattleTransition_Start(B_TRANSITION_BLUR);
}

static void CB2_StartFirstBattle(void)
{
    UpdatePaletteFade();
    RunTasks();

    if (IsBattleTransitionDone() == TRUE)
    {
        gBattleTypeFlags = BATTLE_TYPE_FIRST_BATTLE;
        gMain.savedCallback = CB2_EndFirstBattle;
        FreeAllWindowBuffers();
        SetMainCallback2(CB2_InitBattle);
        RestartWildEncounterImmunitySteps();
        ClearPoisonStepCounter();
        IncrementGameStat(GAME_STAT_TOTAL_BATTLES);
        IncrementGameStat(GAME_STAT_WILD_BATTLES);
        IncrementDailyWildBattles();
        TryUpdateGymLeaderRematchFromWild();
    }
}

static void CB2_EndFirstBattle(void)
{
    Overworld_ClearSavedMusic();
    DowngradeBadPoison();
    SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
}

static void TryUpdateGymLeaderRematchFromWild(void)
{
    if (GetGameStat(GAME_STAT_WILD_BATTLES) % 60 == 0)
        UpdateGymLeaderRematch();
}

static void TryUpdateGymLeaderRematchFromTrainer(void)
{
    if (GetGameStat(GAME_STAT_TRAINER_BATTLES) % 20 == 0)
        UpdateGymLeaderRematch();
}

// Keep defeat flags 32-bit: added trainers use ADMG_ADDED_TRAINER_FLAGS_START (0x10000) and above
// Returning u16 would truncate the flag ID
static u32 GetTrainerAFlag(void)
{
    u32 id;
    return ADMG_TryGetTrainerDefeatFlagId(TRAINER_BATTLE_PARAM.opponentA, &id) ? id : 0xFFFF;
}

static u32 GetTrainerBFlag(void)
{
    u32 id;
    return ADMG_TryGetTrainerDefeatFlagId(TRAINER_BATTLE_PARAM.opponentB, &id) ? id : 0xFFFF;
}

static bool32 IsPlayerDefeated(u32 battleOutcome)
{
    switch (battleOutcome)
    {
    case B_OUTCOME_LOST:
    case B_OUTCOME_DREW:
    case B_OUTCOME_FORFEITED:
        return TRUE;
    case B_OUTCOME_WON:
    case B_OUTCOME_RAN:
    case B_OUTCOME_PLAYER_TELEPORTED:
    case B_OUTCOME_MON_FLED:
    case B_OUTCOME_CAUGHT:
        return FALSE;
    default:
        return FALSE;
    }
}

void InitTrainerBattleParameter(void)
{
    memset(gTrainerBattleParameter.data, 0, sizeof(TrainerBattleParameter));
    sTrainerBattleEndScript = NULL;
}

void TrainerBattleLoadArgs(const u8 *data)
{
    TrainerBattleParameter decoded;
    if (data == NULL) { ADMG_StateAccessRejected(0, __func__); return; }
    memcpy(decoded.data, data, sizeof(decoded));
    if (!ADMG_TryValidateOrdinaryTrainerOperands(decoded.params.opponentA, decoded.params.opponentB)) return;
    InitTrainerBattleParameter();
    memcpy(gTrainerBattleParameter.data, decoded.data, sizeof(decoded));
    sTrainerBattleEndScript = (u8*)data + sizeof(TrainerBattleParameter);
}

// loads trainer A parameter to trainer B. Used for second trainer in trainer_see.c
void TrainerBattleLoadArgsSecondTrainer(const u8 *data)
{
    if (data == NULL) { ADMG_StateAccessRejected(0, __func__); return; }
    TrainerBattleParameter decoded;
    memcpy(decoded.data, data, sizeof(decoded));
    TrainerBattleParameter *temp = &decoded;
    if (!ADMG_TryValidateOrdinaryTrainerOperands(temp->params.opponentA, TRAINER_NONE)
     || !ADMG_TryValidateOrdinaryTrainerOperands(TRAINER_BATTLE_PARAM.opponentA, temp->params.opponentA)) return;

    TRAINER_BATTLE_PARAM.playMusicB = temp->params.playMusicA;
    TRAINER_BATTLE_PARAM.objEventLocalIdB = temp->params.objEventLocalIdA;
    TRAINER_BATTLE_PARAM.opponentB = temp->params.opponentA;
    TRAINER_BATTLE_PARAM.introTextB = temp->params.introTextA;
    TRAINER_BATTLE_PARAM.defeatTextB = temp->params.defeatTextA;
    TRAINER_BATTLE_PARAM.battleScriptRetAddrB = temp->params.battleScriptRetAddrA;
}

void SetMapVarsToTrainerA(void)
{
    if (TRAINER_BATTLE_PARAM.objEventLocalIdA != LOCALID_NONE)
    {
        gSpecialVar_LastTalked = TRAINER_BATTLE_PARAM.objEventLocalIdA;
        gSelectedObjectEvent = GetObjectEventIdByLocalIdAndMap(TRAINER_BATTLE_PARAM.objEventLocalIdA, gSaveBlock1Ptr->location.mapId);
    }
}

void SetMapVarsToTrainerB(void)
{
    if (TRAINER_BATTLE_PARAM.objEventLocalIdB != LOCALID_NONE)
    {
        gSpecialVar_LastTalked = TRAINER_BATTLE_PARAM.objEventLocalIdB;
        gSelectedObjectEvent = GetObjectEventIdByLocalIdAndMap(TRAINER_BATTLE_PARAM.objEventLocalIdB, gSaveBlock1Ptr->location.mapId);
    }
}

// If a snippet won't fit, reject the battle instead of silently dropping it
static bool8 sADMG_TrainerScriptDropped;
#define PUSH(script) { if (!ScriptStackPush(scrStack, script)) sADMG_TrainerScriptDropped = TRUE; }

#define PUSH_IF_SET(script, cond) \
if (cond)           \
{                   \
    PUSH(script)    \
}

#define PUSH_IF_ELSE(script, alt, cond) \
if (cond)           \
{                   \
    PUSH(script)    \
}                   \
else                \
{                   \
    PUSH(alt)       \
}                   \

static void BattleSetup_ConfigureApproachingFacilityTrainerBattle(TrainerBattleParameter *battleParams, struct ScriptStack *scrStack)
{
    SetMapVarsToTrainerA();

    PUSH(EventSnippet_StartTrainerApproach)
    PUSH(EventSnippet_PlayTrainerEncounterMusic)
    PUSH(EventSnippet_TrainerApproach)
    PUSH(EventSnippet_ShowTrainerIntroMsg)

    if (gNoOfApproachingTrainers > 1) 
    {
        SetMapVarsToTrainerB();

        PUSH(EventSnippet_PrepareSecondTrainerApproach)
        PUSH(EventSnippet_PlayTrainerEncounterMusic)
        PUSH(EventSnippet_TrainerApproach)
        PUSH(EventSnippet_ShowTrainerIntroMsg)
    }

    PUSH(EventSnippet_DoTrainerBattle)
    PUSH(EventSnippet_EndTrainerBattle)
}

static void BattleSetup_ConfigureFacilityTrainerBattle(TrainerBattleParameter *battleParams, struct ScriptStack *scrStack)
{
    bool8 defeated;
    if (!ADMG_TryReadCurrentTrainerFlag(&defeated))
        return;
    SetMapVarsToTrainerA();

    PUSH(EventSnippet_Lock)
    PUSH(EventSnippet_FacePlayer)
    PUSH(EventSnippet_RevealTrainer)

    if (defeated) {
        PUSH(EventSnippet_GotoPostBattleScript)
        return;
    }

    PUSH(EventSnippet_PlayTrainerEncounterMusic)
    PUSH(EventSnippet_SetTrainerFacingDirection)
    PUSH(EventSnippet_ShowTrainerIntroMsg)
    PUSH(EventSnippet_DoTrainerBattle)
    PUSH(EventSnippet_EndTrainerBattle)

    return;
}

static void BattleSetup_ConfigureApproachingTrainerBattle(TrainerBattleParameter *battleParams, struct ScriptStack *scrStack)
{
    const struct Trainer *checked;
    if (!ADMG_TryGetTrainerStruct(battleParams->params.opponentA, &checked)
     || (battleParams->params.opponentB != TRAINER_NONE
      && battleParams->params.opponentB != 0xFFFF
      && !ADMG_TryGetTrainerStruct(battleParams->params.opponentB, &checked)))
        return;

    SetMapVarsToTrainerA();

    PUSH       (EventSnippet_StartTrainerApproach)
    PUSH_IF_SET(EventSnippet_PlayTrainerEncounterMusic, battleParams->params.playMusicA)
    PUSH       (EventSnippet_TrainerApproach)
    PUSH_IF_SET(EventSnippet_ShowTrainerIntroMsg, battleParams->params.introTextA)

    if (gNoOfApproachingTrainers > 1) 
    {
        SetMapVarsToTrainerB();

        PUSH       (EventSnippet_PrepareSecondTrainerApproach)
        PUSH_IF_SET(EventSnippet_PlayTrainerEncounterMusic, battleParams->params.playMusicB)
        PUSH       (EventSnippet_TrainerApproach)
        PUSH_IF_SET(EventSnippet_ShowTrainerIntroMsg, battleParams->params.introTextB)
    }

    PUSH(EventSnippet_DoTrainerBattle)
    PUSH(EventSnippet_EndTrainerBattle)
    return;
}

static void BattleSetup_ConfigureTrainerBattle(TrainerBattleParameter *battleParams, struct ScriptStack *scrStack)
{
    ADMG_DiscardNativeBattleAssociation();

    const struct Trainer *checked;
    if (!ADMG_TryGetTrainerStruct(battleParams->params.opponentA, &checked)
     || (battleParams->params.opponentB != TRAINER_NONE
      && battleParams->params.opponentB != 0xFFFF
      && !ADMG_TryGetTrainerStruct(battleParams->params.opponentB, &checked)))
        return;

    bool8 defeated = FALSE;
    if (!battleParams->params.skipFlagCheck && !ADMG_TryReadCurrentTrainerFlag(&defeated))
        return;
    SetMapVarsToTrainerA();

    PUSH       (EventSnippet_Lock)
    PUSH_IF_SET(EventSnippet_FacePlayer, battleParams->params.facePlayer)
    PUSH       (EventSnippet_RevealTrainer)

    bool32 isTrainerDefeated = !battleParams->params.skipFlagCheck && defeated;

    if ((isTrainerDefeated && !battleParams->params.isRematch)
    || (!IsTrainerReadyForRematch() && battleParams->params.isRematch)) {
        PUSH(EventSnippet_GotoPostBattleScript)
        return;
    }

    if (battleParams->params.isDoubleBattle && !HasEnoughMonsForDoubleBattle2())
    {
        PUSH(EventSnippet_NotEnoughMonsForDoubleBattle)
        return;
    }

    if (battleParams->params.opponentA != 0 && battleParams->params.opponentB != 0)
    {
        gNoOfApproachingTrainers = 2;
    }
    
#if FREE_MATCH_CALL == FALSE
    if (battleParams->params.isRematch && !IsNativeTrainerContent(battleParams->params.opponentA))
    {
        battleParams->params.opponentA = GetRematchTrainerId(battleParams->params.opponentA);
    }
#endif //FREE_MATCH_CALL
    if (IsNativeTrainerContent(battleParams->params.opponentA))
    {
        if (battleParams->params.isRematch)
        {
            u16 selected = GetRematchTrainerId(battleParams->params.opponentA);
            if (selected == TRAINER_NONE)
            {
                PUSH(EventSnippet_GotoPostBattleScript)
                return;
            }
            battleParams->params.opponentA = selected;
        }
        /* Association comes from the explicit configured NPC operand.
         * A direct battle never silently borrows ambient LastTalked state. */
        if (battleParams->params.objEventLocalIdA != LOCALID_NONE)
        {
            struct ADMG_RematchContext context;
            if (ADMG_TryCaptureNativeRematchContext(&context))
                (void)ADMG_TryBindNativeBattleNpc(&context,
                    battleParams->params.opponentA, battleParams->params.objEventLocalIdA);
        }
    }
    
    PUSH_IF_SET(EventSnippet_PlayTrainerEncounterMusic, battleParams->params.playMusicA)
    PUSH_IF_SET(EventSnippet_SetTrainerFacingDirection, battleParams->params.facePlayer);
    PUSH_IF_SET(EventSnippet_ShowTrainerIntroMsg, battleParams->params.introTextA)
    PUSH_IF_ELSE(EventSnippet_DoRematchTrainerBattle, EventSnippet_DoTrainerBattle, battleParams->params.isRematch)
    PUSH_IF_ELSE(EventSnippet_GotoPostBattleScript, EventSnippet_EndTrainerBattle, battleParams->params.continueScript)

    return;
}

ADMG_NATIVE_ENTRY ADMG_Word ADMG_ComposeTrainerBattleScript(ADMG_Word kind, struct ScriptStack *stack, ADMG_Word contextDepth)
{
    return TRUE;
}

// Reject empty lists and dropped snippets before calling the composition provider
// After composition, the list must still be nonempty and fit alongside the existing call frames
// Each address must be in ROM or the native memory range
// Replace a rejected list with the releaseall/end snippet
static void ADMG_FinishTrainerBattleScript(u32 kind, struct ScriptStack *stack, u32 contextDepth)
{
    bool8 accepted = !sADMG_TrainerScriptDropped && stack->stackDepth != 0
        && ADMG_ComposeTrainerBattleScript(kind, stack, contextDepth)
        && stack->stackDepth != 0 && stack->stackDepth <= ADMG_TRAINER_SCRIPT_CAPACITY
        && contextDepth + stack->stackDepth <= ADMG_TRAINER_SCRIPT_CAPACITY;
    for (u32 i = 0; accepted && i < stack->stackDepth; i++)
    {
        u32 address = (u32)stack->stack[i];
        if (!((address >= ROM_START && address < ROM_END) || ADMG_IsNativeAddress(address)))
            accepted = FALSE;
    }
    sADMG_TrainerScriptDropped = FALSE;
    if (accepted)
        return;
    ADMG_StateAccessRejected(kind, __func__);
    InitScriptStack(stack);
    ScriptStackPush(stack, EventSnippet_ADMG_RefusedTrainerBattle);
}

void ConfigureTrainerBattle(struct ScriptContext *ctx)
{
    TrainerBattleParameter decoded;
    struct ScriptStack trainerBattleScriptStack;
    if (ctx == NULL || ctx->scriptPtr == NULL) { ADMG_StateAccessRejected(0, __func__); return; }
    InitScriptStack(&trainerBattleScriptStack);
    sADMG_TrainerScriptDropped = FALSE;
    memcpy(decoded.data, ctx->scriptPtr, sizeof(decoded));
    if (!ADMG_TryValidateOrdinaryTrainerOperands(decoded.params.opponentA, decoded.params.opponentB))
    {
        // Select the refusal snippet instead of treating trainerbattle operands as script commands
        ADMG_FinishTrainerBattleScript(ADMG_TRAINER_SCRIPT_TALK, &trainerBattleScriptStack, ctx->stackDepth);
        ScriptContext_SetupContextFromStack(&trainerBattleScriptStack, ctx);
        return;
    }

    InitTrainerBattleParameter();

    TrainerBattleParameter *battleParams = (TrainerBattleParameter*)(ctx->scriptPtr);
    TrainerBattleLoadArgs(battleParams->data);

    BattleSetup_ConfigureTrainerBattle(&gTrainerBattleParameter, &trainerBattleScriptStack);
    ADMG_FinishTrainerBattleScript(ADMG_TRAINER_SCRIPT_TALK, &trainerBattleScriptStack, ctx->stackDepth);
    ScriptContext_SetupContextFromStack(&trainerBattleScriptStack, ctx);
    ScriptContext_Enable();
}

void ConfigureApproachingTrainerBattle(struct ApproachingTrainer *approachingTrainer)
{
    TrainerBattleParameter first, second;
    if (approachingTrainer == NULL || gNoOfApproachingTrainers == 0
     || gNoOfApproachingTrainers > NUM_APPROACHING_TRAINER || approachingTrainer[0].trainerScriptPtr == NULL)
    { ADMG_StateAccessRejected(gNoOfApproachingTrainers, __func__); return; }
    memcpy(first.data, approachingTrainer[0].trainerScriptPtr + TRAINERBATTLE_OPCODE_OFFSET, sizeof(first));
    if (!ADMG_TryValidateOrdinaryTrainerOperands(first.params.opponentA, first.params.opponentB)) return;
    if (gNoOfApproachingTrainers > 1)
    {
        if (approachingTrainer[1].trainerScriptPtr == NULL) { ADMG_StateAccessRejected(1, __func__); return; }
        memcpy(second.data, approachingTrainer[1].trainerScriptPtr + TRAINERBATTLE_OPCODE_OFFSET, sizeof(second));
        if (!ADMG_TryValidateOrdinaryTrainerOperands(second.params.opponentA, TRAINER_NONE)
         || !ADMG_TryValidateOrdinaryTrainerOperands(first.params.opponentA, second.params.opponentA)) return;
    }

    InitTrainerBattleParameter();

    struct ScriptStack trainerBattleScriptStack;
    InitScriptStack(&trainerBattleScriptStack);
    sADMG_TrainerScriptDropped = FALSE;

    TrainerBattleParameter *battleParams = (TrainerBattleParameter*)(approachingTrainer[0].trainerScriptPtr + TRAINERBATTLE_OPCODE_OFFSET);
    TrainerBattleLoadArgs(battleParams->data);

    if (gNoOfApproachingTrainers > 1)
    {
        battleParams = (TrainerBattleParameter*)(approachingTrainer[1].trainerScriptPtr + TRAINERBATTLE_OPCODE_OFFSET);
        TrainerBattleLoadArgsSecondTrainer(battleParams->data);
    }

    BattleSetup_ConfigureApproachingTrainerBattle(&gTrainerBattleParameter, &trainerBattleScriptStack);
    ADMG_FinishTrainerBattleScript(ADMG_TRAINER_SCRIPT_APPROACH, &trainerBattleScriptStack, ADMG_GetGlobalScriptStackDepth());
    ScriptContext_SetupGlobalContextFromStack(&trainerBattleScriptStack);
    ScriptContext_Enable();
}

static void SetFacilityOpponent(u8 facility, u8 localId, bool8 isTrainerA)
{
    u64 generation = ADMG_BeginStateAccessCheck();
    enum TrainerId trainerId = TRAINER_NONE;

    switch (facility) {
        case FACILITY_BATTLE_PYRAMID:
            trainerId = LocalIdToPyramidTrainerId(localId);
            break;
        case FACILITY_BATTLE_TRAINER_HILL:
            trainerId = LocalIdToHillTrainerId(localId);
            break;
        default:
            errorf("Invalid facility: %d", facility);
            ADMG_StateAccessRejected(facility, __func__);
            return;
    } 

    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (facility == FACILITY_BATTLE_PYRAMID)
    {
        const struct BattleFrontierTrainer *trainer;
        if (!ADMG_TryGetActiveFacilityTrainerRow(trainerId, &trainer)) return;
    }

    if (isTrainerA) {
        TRAINER_BATTLE_PARAM.opponentA = trainerId;
        TRAINER_BATTLE_PARAM.objEventLocalIdA = localId;
    } else {
        TRAINER_BATTLE_PARAM.opponentB = trainerId;
        TRAINER_BATTLE_PARAM.objEventLocalIdB = localId;
    }
   
}

void ConfigureFacilityTrainerBattle(u8 facility, const u8* scriptEndPtr)
{
    if (gSpecialVar_LastTalked > UCHAR_MAX)
    { ADMG_StateAccessRejected(gSpecialVar_LastTalked, __func__); return; }
    TrainerBattleParameter previous = gTrainerBattleParameter;
    const u8 *previousEnd = sTrainerBattleEndScript;
    struct SaveBlock1 *save1 = gSaveBlock1Ptr;
    struct SaveBlock2 *save2 = gSaveBlock2Ptr;
    const struct BattleFrontierTrainer *table = gFacilityTrainers;
    u16 inputLocalId = gSpecialVar_LastTalked;
    u8 selectedObject = gSelectedObjectEvent;
    u8 selectedLocal = selectedObject < OBJECT_EVENTS_COUNT ? gObjectEvents[selectedObject].localId : 0;
    u64 generation = ADMG_BeginStateAccessCheck();
    InitTrainerBattleParameter();

    struct ScriptStack trainerBattleScriptStack;
    InitScriptStack(&trainerBattleScriptStack);
    sADMG_TrainerScriptDropped = FALSE;

    SetFacilityOpponent(facility, gSpecialVar_LastTalked, TRUE);
    if (!ADMG_EndStateAccessCheck(generation)) goto ADMG_reject;
    if (gSaveBlock1Ptr != save1 || gSaveBlock2Ptr != save2 || gSpecialVar_LastTalked != inputLocalId
        || gSelectedObjectEvent != selectedObject
        || (selectedObject < OBJECT_EVENTS_COUNT && gObjectEvents[selectedObject].localId != selectedLocal)
        || (facility == FACILITY_BATTLE_PYRAMID && gFacilityTrainers != table))
    { ADMG_StateAccessRejected(inputLocalId, __func__); goto ADMG_reject; }

    sTrainerBattleEndScript = (u8*)scriptEndPtr;

    BattleSetup_ConfigureFacilityTrainerBattle(&gTrainerBattleParameter, &trainerBattleScriptStack);
    if (!ADMG_EndStateAccessCheck(generation)) goto ADMG_reject;
    ADMG_FinishTrainerBattleScript(ADMG_TRAINER_SCRIPT_FACILITY, &trainerBattleScriptStack, ADMG_GetGlobalScriptStackDepth());
    ScriptContext_SetupGlobalContextFromStack(&trainerBattleScriptStack);
    ScriptContext_Enable();
    return;
ADMG_reject:
    gTrainerBattleParameter = previous;
    sTrainerBattleEndScript = (u8 *)previousEnd;
}

void ConfigureApproachingFacilityTrainerBattle(struct ApproachingTrainer *approachingTrainer)
{
    if (approachingTrainer == NULL || gNoOfApproachingTrainers > NUM_APPROACHING_TRAINER
        || approachingTrainer[0].trainerScriptPtr == NULL
        || approachingTrainer[0].objectEventId >= OBJECT_EVENTS_COUNT
        || (gNoOfApproachingTrainers > 1 && (approachingTrainer[1].trainerScriptPtr == NULL
            || approachingTrainer[1].objectEventId >= OBJECT_EVENTS_COUNT
            || gApproachingTrainerId + 1 >= NUM_APPROACHING_TRAINER)))
    { ADMG_StateAccessRejected(gNoOfApproachingTrainers, __func__); return; }
    TrainerBattleParameter previous = gTrainerBattleParameter;
    const u8 *previousEnd = sTrainerBattleEndScript;
    u8 previousApproaching = gApproachingTrainerId;
    u8 inputCount = gNoOfApproachingTrainers;
    u8 used = inputCount > 1 ? 2 : 1;
    const u8 *scripts[NUM_APPROACHING_TRAINER];
    u8 objects[NUM_APPROACHING_TRAINER], locals[NUM_APPROACHING_TRAINER];
    for (u32 i = 0; i < used; i++)
    {
        scripts[i] = approachingTrainer[i].trainerScriptPtr;
        objects[i] = approachingTrainer[i].objectEventId;
        locals[i] = gObjectEvents[objects[i]].localId;
    }
    u64 generation = ADMG_BeginStateAccessCheck();
    u8 facility, localId;
    const u8 *scriptEndPtr;

    InitTrainerBattleParameter();

    struct ScriptStack trainerBattleScriptStack;
    InitScriptStack(&trainerBattleScriptStack);
    sADMG_TrainerScriptDropped = FALSE;

    facility = *(approachingTrainer[0].trainerScriptPtr + FACILITYBATTLE_OPCODE_OFFSET);
    localId = gObjectEvents[approachingTrainer[0].objectEventId].localId;
    scriptEndPtr = approachingTrainer[0].trainerScriptPtr + FACILITYBATTLE_OPCODE_OFFSET + 1; 

    SetFacilityOpponent(facility, localId, TRUE);
    if (!ADMG_EndStateAccessCheck(generation)) goto ADMG_reject;
    if (gNoOfApproachingTrainers != inputCount || gApproachingTrainerId != previousApproaching)
    { ADMG_StateAccessRejected(inputCount, __func__); goto ADMG_reject; }
    for (u32 i = 0; i < used; i++)
    {
        if (approachingTrainer[i].trainerScriptPtr != scripts[i] || approachingTrainer[i].objectEventId != objects[i]
            || gObjectEvents[objects[i]].localId != locals[i])
        { ADMG_StateAccessRejected(objects[i], __func__); goto ADMG_reject; }
    }
    
    if (inputCount > 1)
    {
        gApproachingTrainerId++;
        facility = *(approachingTrainer[1].trainerScriptPtr + FACILITYBATTLE_OPCODE_OFFSET);
        localId = gObjectEvents[approachingTrainer[1].objectEventId].localId;
        scriptEndPtr = approachingTrainer[1].trainerScriptPtr + FACILITYBATTLE_OPCODE_OFFSET + 1; 

        SetFacilityOpponent(facility, localId, FALSE);
        if (!ADMG_EndStateAccessCheck(generation)) goto ADMG_reject;
        if (gNoOfApproachingTrainers != inputCount || gApproachingTrainerId != previousApproaching + 1)
        { ADMG_StateAccessRejected(inputCount, __func__); goto ADMG_reject; }
        for (u32 i = 0; i < used; i++)
        {
            if (approachingTrainer[i].trainerScriptPtr != scripts[i] || approachingTrainer[i].objectEventId != objects[i]
                || gObjectEvents[objects[i]].localId != locals[i])
            { ADMG_StateAccessRejected(objects[i], __func__); goto ADMG_reject; }
        }
    }

    sTrainerBattleEndScript = (u8*)scriptEndPtr;
    gApproachingTrainerId = 0;

    BattleSetup_ConfigureApproachingFacilityTrainerBattle(&gTrainerBattleParameter, &trainerBattleScriptStack);
    if (!ADMG_EndStateAccessCheck(generation)) goto ADMG_reject;
    ADMG_FinishTrainerBattleScript(ADMG_TRAINER_SCRIPT_FACILITY_APPROACH, &trainerBattleScriptStack, ADMG_GetGlobalScriptStackDepth());
    ScriptContext_SetupGlobalContextFromStack(&trainerBattleScriptStack);
    ScriptContext_Enable();
    return;
ADMG_reject:
    gTrainerBattleParameter = previous;
    sTrainerBattleEndScript = (u8 *)previousEnd;
    gApproachingTrainerId = previousApproaching;
}

bool32 GetTrainerFlagFromScriptPointer(const u8 *data)
{
    TrainerBattleParameter *temp = (TrainerBattleParameter*)(data + TRAINERBATTLE_OPCODE_OFFSET);
    u32 id;
    if (!ADMG_TryGetTrainerDefeatFlagId(temp->params.opponentA, &id))
        return FALSE;
    return FlagGet(id);
}

bool32 GetRematchFromScriptPointer(const u8 *data)
{
#if FREE_MATCH_CALL
    return FALSE;
#else
    TrainerBattleParameter *temp = (TrainerBattleParameter*)(data + TRAINERBATTLE_OPCODE_OFFSET);
    return ShouldTryRematchBattleForTrainerId(temp->params.opponentA);
#endif
}

// Set trainer's movement type so they stop and remain facing that direction
// Note: Only for trainers who are spoken to directly
//       For trainers who spot the player this is handled by PlayerFaceApproachingTrainer
void SetTrainerFacingDirection(void)
{
    assertf(gSelectedObjectEvent != gPlayerAvatar.objectEventId, "trainer script that needs to be used from an object event was called from player")
    {
        return;
    }
    struct ObjectEvent *objectEvent = &gObjectEvents[gSelectedObjectEvent];
    SetTrainerMovementType(objectEvent, GetTrainerFacingDirectionMovementType(objectEvent->facingDirection));
}

u8 GetRivalBattleFlags(void)
{
    return TRAINER_BATTLE_PARAM.rivalBattleFlags;
}

bool8 GetTrainerFlag(void)
{
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
        return GetBattlePyramidTrainerFlag(gSelectedObjectEvent);
    else if (InTrainerHill())
        return GetHillTrainerFlag(gSelectedObjectEvent);
    else
        return FlagGet(GetTrainerAFlag());
}

static void SetBattledTrainersFlags(void)
{
    u32 first = GetTrainerAFlag();
    u32 second = 0;
    /* 65535 is the existing partner sentinel, not a second trainer flag. */
    if (TRAINER_BATTLE_PARAM.opponentB != 0 && TRAINER_BATTLE_PARAM.opponentB != 0xFFFF)
        second = GetTrainerBFlag();
    if (!ADMG_IsFlagIdValid(first) || !ADMG_IsFlagIdValid(second))
        return;
    if (second != 0)
        FlagSet(second);
    FlagSet(first);
}

static void UNUSED SetBattledTrainerFlag(void)
{
    FlagSet(GetTrainerAFlag());
}

bool8 HasTrainerBeenFought(u32 trainerId)
{
    u32 id;
    if (!ADMG_TryGetTrainerDefeatFlagId(trainerId, &id))
        return FALSE;
    return FlagGet(id);
}

void SetTrainerFlag(u32 trainerId)
{
    u32 id;
    if (ADMG_TryGetTrainerDefeatFlagId(trainerId, &id))
        FlagSet(id);
}

void ClearTrainerFlag(u32 trainerId)
{
    u32 id;
    if (ADMG_TryGetTrainerDefeatFlagId(trainerId, &id))
        FlagClear(id);
}

bool8 ADMG_TryReadLegacyTrainerFlag(u32 trainerId, bool8 *value)
{
    u32 id;
    if (value == NULL || !ADMG_TryGetTrainerDefeatFlagId(trainerId, &id))
        return FALSE;
    *value = HasTrainerBeenFought(trainerId);
    return TRUE;
}

bool8 ADMG_TryReadCurrentTrainerFlag(bool8 *value)
{
    u32 id;
    if (value == NULL)
        return FALSE;
    /* Existing facility dispatch owns its separate flag domain. */
    if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE && !InTrainerHill()
     && !ADMG_TryGetTrainerDefeatFlagId(TRAINER_BATTLE_PARAM.opponentA, &id))
        return FALSE;
    u64 generation = ADMG_BeginStateAccessCheck();
    struct SaveBlock1 *save1 = gSaveBlock1Ptr;
    struct SaveBlock2 *save2 = gSaveBlock2Ptr;
    enum TrainerId identity = TRAINER_BATTLE_PARAM.opponentA;
    u16 localId = TRAINER_BATTLE_PARAM.objEventLocalIdA;
    u8 selected = gSelectedObjectEvent;
    u8 selectedLocal = selected < OBJECT_EVENTS_COUNT ? gObjectEvents[selected].localId : 0;
    bool8 result = GetTrainerFlag();
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock1Ptr != save1 || gSaveBlock2Ptr != save2 || TRAINER_BATTLE_PARAM.opponentA != identity
        || TRAINER_BATTLE_PARAM.objEventLocalIdA != localId || gSelectedObjectEvent != selected
        || (selected < OBJECT_EVENTS_COUNT && gObjectEvents[selected].localId != selectedLocal))
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    *value = result;
    return TRUE;
}

bool8 ADMG_TryReadTrainerFlagFromScript(const u8 *data, bool8 *value)
{
    const TrainerBattleParameter *params;
    u32 id;
    if (data == NULL || value == NULL)
        return FALSE;
    params = (const TrainerBattleParameter *)(data + TRAINERBATTLE_OPCODE_OFFSET);
    if (!ADMG_TryGetTrainerDefeatFlagId(params->params.opponentA, &id))
        return FALSE;
    *value = GetTrainerFlagFromScriptPointer(data);
    return TRUE;
}

/* Transient caller ownership only; no game-state or save schema change. */
struct ADMG_PyramidTrainerBattleInput
{
    struct SaveBlock1 *save1;
    struct SaveBlock2 *save2;
    u64 generation;
    u32 mode, previousFlags;
    enum TrainerId a, b;
    u16 lastTalked;
    u8 count, selectedA, selectedCurrent, localA, approachB, localB;
    bool8 actorsReady, countersPublished, musicTouched, bPublished, marked;
    struct ADMG_BattleTrainerActors actors;
    u32 *music;
    u16 previousMusic;
    struct Pokemon parties[2][PARTY_SIZE];
    u8 partyCounts[2];
    u8 movement[OBJECT_EVENTS_COUNT];
    typeof(gObjectEvents[0].initialCoords) coords[OBJECT_EVENTS_COUNT];
    u8 templateMovement;
    typeof(gSaveBlock2Ptr->frontier.pyramidTrainerFlags) pyramidFlags;
    u8 previousRetScripts;
    bool8 previousCheckB;
    u16 previousFace;
    MainCallback previousCallback;
};

static bool8 ADMG_PyramidTrainerBattleCurrent(const struct ADMG_PyramidTrainerBattleInput *input)
{
    if (gSaveBlock1Ptr != input->save1 || gSaveBlock2Ptr != input->save2
        || input->save2->frontier.lvlMode != input->mode
        || TRAINER_BATTLE_PARAM.opponentA != input->a
        || (input->count == 2 && TRAINER_BATTLE_PARAM.opponentB != input->b)
        || gNoOfApproachingTrainers != (input->countersPublished ? 0 : input->count)
        || gSpecialVar_LastTalked != input->lastTalked || gSelectedObjectEvent != input->selectedCurrent
        || gObjectEvents[input->selectedA].localId != input->localA
        || (input->count == 2 && (gApproachingTrainers[1].objectEventId != input->approachB
            || (input->approachB < OBJECT_EVENTS_COUNT && gObjectEvents[input->approachB].localId != input->localB)))
        || CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE)
    { ADMG_StateAccessRejected(input->a, __func__); return FALSE; }
    return (!input->actorsReady || ADMG_FieldBattleActorsCurrent(&input->actors))
        && ADMG_EndStateAccessCheck(input->generation);
}

static bool8 ADMG_TryAdmitPyramidTrainerBattle(struct ADMG_PyramidTrainerBattleInput *input)
{
    memset(input, 0, sizeof(*input));
    input->save1 = gSaveBlock1Ptr; input->save2 = gSaveBlock2Ptr;
    input->a = TRAINER_BATTLE_PARAM.opponentA; input->b = TRAINER_BATTLE_PARAM.opponentB;
    input->count = gNoOfApproachingTrainers;
    input->selectedA = input->selectedCurrent = gSelectedObjectEvent;
    input->lastTalked = gSpecialVar_LastTalked;
    input->generation = ADMG_BeginStateAccessCheck();
    if (input->count > NUM_APPROACHING_TRAINER || !ADMG_TryAdmitPyramidTrainerMarking(input->count == 2)) return FALSE;
    input->mode = input->save2->frontier.lvlMode;
    input->localA = gObjectEvents[input->selectedA].localId;
    input->approachB = gApproachingTrainers[1].objectEventId;
    input->localB = input->approachB < OBJECT_EVENTS_COUNT ? gObjectEvents[input->approachB].localId : 0;
    struct ADMG_FacilityPartyInput party;
    if (!ADMG_TryAdmitFacilityParty(input->a, B_TRAINER_OPPONENT_A, 1, FALSE, 0, &party)
        || (input->count == 2 && !ADMG_TryAdmitFacilityParty(input->b, B_TRAINER_OPPONENT_B, 1, FALSE, 0, &party))
        || !ADMG_PyramidTrainerBattleCurrent(input)) return FALSE;
    input->music = GetVarPointer(VAR_TEMP_PLAYING_PYRAMID_MUSIC);
    if (!ADMG_PyramidTrainerBattleCurrent(input)) return FALSE;
    if (input->music == NULL)
    { ADMG_StateAccessRejected(VAR_TEMP_PLAYING_PYRAMID_MUSIC, __func__); return FALSE; }
    input->previousMusic = *input->music;
    input->previousFlags = gBattleTypeFlags;
    memcpy(input->parties[0], gParties[B_TRAINER_OPPONENT_A], sizeof(input->parties[0]));
    memcpy(input->parties[1], gParties[B_TRAINER_OPPONENT_B], sizeof(input->parties[1]));
    input->partyCounts[0] = gPartiesCount[B_TRAINER_OPPONENT_A]; input->partyCounts[1] = gPartiesCount[B_TRAINER_OPPONENT_B];
    for (u32 i = 0; i < OBJECT_EVENTS_COUNT; i++)
    { input->movement[i] = gObjectEvents[i].movementType; input->coords[i] = gObjectEvents[i].initialCoords; }
    input->templateMovement = input->save1->objectEventTemplates[input->lastTalked - 1].movementType;
    input->pyramidFlags = input->save2->frontier.pyramidTrainerFlags;
    input->previousRetScripts = sNoOfPossibleTrainerRetScripts;
    input->previousCheckB = sShouldCheckTrainerBScript;
    input->previousFace = gWhichTrainerToFaceAfterBattle;
    input->previousCallback = gMain.savedCallback;
    return TRUE;
}

static void ADMG_RestorePyramidTrainerBattle(struct ADMG_PyramidTrainerBattleInput *input)
{
    gBattleTypeFlags = input->previousFlags;
    if (input->bPublished) TRAINER_BATTLE_PARAM.opponentB = input->b;
    if (input->musicTouched) *input->music = input->previousMusic;
    memcpy(gParties[B_TRAINER_OPPONENT_A], input->parties[0], sizeof(input->parties[0]));
    memcpy(gParties[B_TRAINER_OPPONENT_B], input->parties[1], sizeof(input->parties[1]));
    gPartiesCount[B_TRAINER_OPPONENT_A] = input->partyCounts[0]; gPartiesCount[B_TRAINER_OPPONENT_B] = input->partyCounts[1];
    input->save2->frontier.pyramidTrainerFlags = input->pyramidFlags;
    input->save1->objectEventTemplates[input->lastTalked - 1].movementType = input->templateMovement;
    gObjectEvents[input->selectedA].movementType = input->movement[input->selectedA];
    gObjectEvents[input->selectedA].initialCoords = input->coords[input->selectedA];
    if (input->marked)
    {
        gObjectEvents[input->selectedCurrent].movementType = input->movement[input->selectedCurrent];
        gObjectEvents[input->selectedCurrent].initialCoords = input->coords[input->selectedCurrent];
    }
    gSelectedObjectEvent = input->selectedA;
    if (input->countersPublished)
    {
        sNoOfPossibleTrainerRetScripts = input->previousRetScripts;
        sShouldCheckTrainerBScript = input->previousCheckB;
        gWhichTrainerToFaceAfterBattle = input->previousFace;
        gMain.savedCallback = input->previousCallback;
        gNoOfApproachingTrainers = input->count;
    }
}

struct ADMG_HillTrainerBattleInput
{
    struct SaveBlock1 *save1;
    struct SaveBlock2 *save2;
    u64 generation;
    u32 layout, previousFlags, currentFlags;
    enum TrainerId a, b;
    u8 count, previousTrainerFlags, markMask;
    bool8 bPublished, marked, countersPublished, actorsReady;
    struct ADMG_BattleTrainerActors actors;
    struct Pokemon parties[2][PARTY_SIZE];
    u8 partyCounts[2], previousRetScripts;
    bool8 previousCheckB;
    u16 previousFace;
    MainCallback previousCallback;
};
static struct ADMG_HillTrainerBattleInput *sADMG_HillTrainerBattle;

static bool8 ADMG_HillTrainerBattleCurrent(const struct ADMG_HillTrainerBattleInput *input)
{
    if (!ADMG_EndStateAccessCheck(input->generation)) return FALSE;
    if (sADMG_HillTrainerBattle != input || gSaveBlock1Ptr != input->save1
        || gSaveBlock2Ptr != input->save2 || gMapHeader.mapLayoutId != input->layout
        || gBattleTypeFlags != input->currentFlags
        || TRAINER_BATTLE_PARAM.opponentA != input->a
        || (input->count == 2 && TRAINER_BATTLE_PARAM.opponentB != input->b)
        || (input->bPublished && TRAINER_BATTLE_PARAM.opponentB != 0xFFFF)
        || gNoOfApproachingTrainers != (input->countersPublished ? 0 : input->count)
        || CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || !InTrainerHillChallenge())
    { ADMG_StateAccessRejected(input->a, __func__); return FALSE; }
    u8 mask;
    if (!ADMG_TryGetHillTrainerFlagMask(input->a, input->b, input->count == 2, &mask)) return FALSE;
    if (mask != input->markMask)
    { ADMG_StateAccessRejected(input->a, __func__); return FALSE; }
    return (!input->actorsReady || ADMG_FieldBattleActorsCurrent(&input->actors))
        && ADMG_EndStateAccessCheck(input->generation);
}

static bool8 ADMG_TryAdmitHillTrainerBattle(struct ADMG_HillTrainerBattleInput *input)
{
    memset(input, 0, sizeof(*input));
    input->save1 = gSaveBlock1Ptr; input->save2 = gSaveBlock2Ptr;
    input->a = TRAINER_BATTLE_PARAM.opponentA; input->b = TRAINER_BATTLE_PARAM.opponentB;
    input->layout = gMapHeader.mapLayoutId; input->count = gNoOfApproachingTrainers;
    input->generation = ADMG_BeginStateAccessCheck();
    if (input->count > NUM_APPROACHING_TRAINER || input->save2 == NULL)
    { ADMG_StateAccessRejected(input->count, __func__); return FALSE; }
    if (!ADMG_TryAdmitHillParty(input->a, B_TRAINER_OPPONENT_A)
        || (input->count == 2 && !ADMG_TryAdmitHillParty(input->b, B_TRAINER_OPPONENT_B))
        || !ADMG_TryGetHillTrainerFlagMask(input->a, input->b, input->count == 2, &input->markMask)) return FALSE;
    input->previousFlags = input->currentFlags = gBattleTypeFlags;
    input->previousTrainerFlags = input->save2->frontier.trainerFlags;
    memcpy(input->parties[0], gParties[B_TRAINER_OPPONENT_A], sizeof(input->parties[0]));
    memcpy(input->parties[1], gParties[B_TRAINER_OPPONENT_B], sizeof(input->parties[1]));
    input->partyCounts[0] = gPartiesCount[B_TRAINER_OPPONENT_A]; input->partyCounts[1] = gPartiesCount[B_TRAINER_OPPONENT_B];
    input->previousRetScripts = sNoOfPossibleTrainerRetScripts;
    input->previousCheckB = sShouldCheckTrainerBScript;
    input->previousFace = gWhichTrainerToFaceAfterBattle;
    input->previousCallback = gMain.savedCallback;
    return ADMG_EndStateAccessCheck(input->generation);
}

static void ADMG_RestoreHillTrainerBattle(struct ADMG_HillTrainerBattleInput *input)
{
    gBattleTypeFlags = input->previousFlags;
    if (input->bPublished) TRAINER_BATTLE_PARAM.opponentB = input->b;
    memcpy(gParties[B_TRAINER_OPPONENT_A], input->parties[0], sizeof(input->parties[0]));
    memcpy(gParties[B_TRAINER_OPPONENT_B], input->parties[1], sizeof(input->parties[1]));
    gPartiesCount[B_TRAINER_OPPONENT_A] = input->partyCounts[0]; gPartiesCount[B_TRAINER_OPPONENT_B] = input->partyCounts[1];
    if (input->marked)
        input->save2->frontier.trainerFlags = (input->save2->frontier.trainerFlags & ~input->markMask)
            | (input->previousTrainerFlags & input->markMask);
    if (input->countersPublished)
    {
        sNoOfPossibleTrainerRetScripts = input->previousRetScripts;
        sShouldCheckTrainerBScript = input->previousCheckB;
        gWhichTrainerToFaceAfterBattle = input->previousFace;
        gMain.savedCallback = input->previousCallback;
        gNoOfApproachingTrainers = input->count;
    }
}

static void ADMG_StartHillTrainerBattle(void) __attribute__((noinline));

static void ADMG_RunTrainerBattleStart(struct ADMG_PyramidTrainerBattleInput *admitted)
{
    struct ADMG_HillTrainerBattleInput *hill = admitted == NULL ? sADMG_HillTrainerBattle : NULL;
    if (hill != NULL && !ADMG_HillTrainerBattleCurrent(hill)) return;
    if (admitted != NULL && !ADMG_PyramidTrainerBattleCurrent(admitted)) return;
    const struct Trainer *checked;
    if (hill == NULL && CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE && !InTrainerHillChallenge())
    {
        if (!ADMG_TryGetTrainerStruct(TRAINER_BATTLE_PARAM.opponentA, &checked)
         || (gNoOfApproachingTrainers == 2
          && !ADMG_TryGetTrainerStruct(TRAINER_BATTLE_PARAM.opponentB, &checked)))
            return;
    }

    if (hill != NULL && !ADMG_HillTrainerBattleCurrent(hill)) return;

    if (gNoOfApproachingTrainers == 2)
    {
        if (FollowerNPCIsBattlePartner())
            {
                gBattleTypeFlags = (BATTLE_TYPE_MULTI | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_TRAINER);
                if (hill != NULL) hill->currentFlags = (BATTLE_TYPE_MULTI | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_TRAINER);
            }
        else
            {
                gBattleTypeFlags = (BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_TRAINER);
                if (hill != NULL) hill->currentFlags = (BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_TRAINER);
            }
    }
    else
    {
        if (FollowerNPCIsBattlePartner())
        {
            {
                gBattleTypeFlags = (BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TRAINER);
                if (hill != NULL) hill->currentFlags = (BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TRAINER);
            }
            TRAINER_BATTLE_PARAM.opponentB = 0xFFFF;
            if (admitted != NULL) admitted->bPublished = TRUE;
            if (hill != NULL) hill->bPublished = TRUE;
        }
        else
        {
            {
                gBattleTypeFlags = (BATTLE_TYPE_TRAINER);
                if (hill != NULL) hill->currentFlags = (BATTLE_TYPE_TRAINER);
            }
        }
    }

    if (TRAINER_BATTLE_PARAM.earlyRival && GetRivalBattleFlags() & RIVAL_BATTLE_TUTORIAL)
        {
            gBattleTypeFlags |= BATTLE_TYPE_FIRST_BATTLE;
            if (hill != NULL) hill->currentFlags |= BATTLE_TYPE_FIRST_BATTLE;
        }

    if (admitted != NULL && !ADMG_PyramidTrainerBattleCurrent(admitted)) goto ADMG_reject;
    if (hill != NULL && !ADMG_HillTrainerBattleCurrent(hill)) goto ADMG_reject;

    if (hill == NULL && CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
    {
        if (admitted != NULL) admitted->musicTouched = TRUE;
        bool8 musicSet = VarSet(VAR_TEMP_PLAYING_PYRAMID_MUSIC, 0);
        if (admitted != NULL && (!musicSet || !ADMG_PyramidTrainerBattleCurrent(admitted)))
        { if (!musicSet) ADMG_StateAccessRejected(VAR_TEMP_PLAYING_PYRAMID_MUSIC, __func__); goto ADMG_reject; }
        {
            gBattleTypeFlags |= BATTLE_TYPE_PYRAMID;
            if (hill != NULL) hill->currentFlags |= BATTLE_TYPE_PYRAMID;
        }
        if (admitted != NULL)
        {
            enum TrainerId partner; bool8 deferred;
            if (!ADMG_TrySelectFieldBattlePartner(TRUE, &partner, &deferred)
                || !ADMG_TryCaptureBattleTrainerActors(gBattleTypeFlags, TRAINER_BATTLE_PARAM.opponentA,
                    TRAINER_BATTLE_PARAM.opponentB, partner, &admitted->actors)) goto ADMG_reject;
            admitted->actorsReady = TRUE;
            if (!ADMG_PyramidTrainerBattleCurrent(admitted)) goto ADMG_reject;
        }

        if (gNoOfApproachingTrainers == 2)
        {
            FillFrontierTrainersParties(1);
            if (admitted != NULL && !ADMG_PyramidTrainerBattleCurrent(admitted)) goto ADMG_reject;
            ZeroMonData(&gParties[B_TRAINER_OPPONENT_A][1]);
            ZeroMonData(&gParties[B_TRAINER_OPPONENT_A][2]);
            ZeroMonData(&gParties[B_TRAINER_OPPONENT_B][1]);
            ZeroMonData(&gParties[B_TRAINER_OPPONENT_B][2]);
        }
        else
        {
            FillFrontierTrainerParty(1);
            if (admitted != NULL && !ADMG_PyramidTrainerBattleCurrent(admitted)) goto ADMG_reject;
            ZeroMonData(&gParties[B_TRAINER_OPPONENT_A][1]);
            ZeroMonData(&gParties[B_TRAINER_OPPONENT_A][2]);
        }

        if (admitted != NULL && !ADMG_PyramidTrainerBattleCurrent(admitted)) goto ADMG_reject;
        MarkApproachingPyramidTrainersAsBattled();
        if (admitted != NULL)
        {
            if (!ADMG_EndStateAccessCheck(admitted->generation)) goto ADMG_reject;
            if (gSelectedObjectEvent >= OBJECT_EVENTS_COUNT)
            { ADMG_StateAccessRejected(gSelectedObjectEvent, __func__); goto ADMG_reject; }
            admitted->selectedCurrent = gSelectedObjectEvent;
            admitted->marked = TRUE;
            if (!ADMG_PyramidTrainerBattleCurrent(admitted)) goto ADMG_reject;
        }
    }
    else if (hill != NULL || InTrainerHillChallenge())
    {
        {
            gBattleTypeFlags |= BATTLE_TYPE_TRAINER_HILL;
            if (hill != NULL) hill->currentFlags |= BATTLE_TYPE_TRAINER_HILL;
        }

        if (hill != NULL)
        {
            enum TrainerId partner; bool8 deferred;
            if (!ADMG_TrySelectFieldBattlePartner(TRUE, &partner, &deferred)
                || !ADMG_TryCaptureBattleTrainerActors(gBattleTypeFlags, TRAINER_BATTLE_PARAM.opponentA,
                    TRAINER_BATTLE_PARAM.opponentB, partner, &hill->actors)) goto ADMG_reject;
            hill->actorsReady = TRUE;
            if (!ADMG_HillTrainerBattleCurrent(hill)) goto ADMG_reject;
        }

        if (gNoOfApproachingTrainers == 2)
            FillHillTrainersParties();
        else
            FillHillTrainerParty();

        if (hill != NULL)
        {
            if (!ADMG_HillTrainerBattleCurrent(hill)) goto ADMG_reject;
            hill->marked = TRUE;
        }
        SetHillTrainerFlag();
        if (hill != NULL && !ADMG_HillTrainerBattleCurrent(hill)) goto ADMG_reject;
    }
    else if (GetTrainerBattleType(TRAINER_BATTLE_PARAM.opponentA) == TRAINER_BATTLE_TYPE_DOUBLES)
    {
        {
            gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
            if (hill != NULL) hill->currentFlags |= BATTLE_TYPE_DOUBLE;
        }
    }

    if (admitted != NULL)
    {
        if (!ADMG_PyramidTrainerBattleCurrent(admitted)) goto ADMG_reject;
        admitted->countersPublished = TRUE;
    }
    if (hill != NULL)
    {
        if (!ADMG_HillTrainerBattleCurrent(hill)) goto ADMG_reject;
        hill->countersPublished = TRUE;
    }
    sNoOfPossibleTrainerRetScripts = gNoOfApproachingTrainers;
    gNoOfApproachingTrainers = 0;
    sShouldCheckTrainerBScript = FALSE;
    gWhichTrainerToFaceAfterBattle = 0;
    gMain.savedCallback = CB2_EndTrainerBattle;

    if (hill != NULL || CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || InTrainerHillChallenge())
        DoBattlePyramidTrainerHillBattle();
    else
        DoTrainerBattle();

    if (admitted != NULL && !ADMG_PyramidTrainerBattleCurrent(admitted)) goto ADMG_reject;
    if (hill != NULL && !ADMG_HillTrainerBattleCurrent(hill)) goto ADMG_reject;

    ScriptContext_Stop();
    return;
ADMG_reject:
    if (admitted != NULL) ADMG_RestorePyramidTrainerBattle(admitted);
    else if (hill != NULL) ADMG_RestoreHillTrainerBattle(hill);
}

/* Keep the larger owned snapshot off the ordinary caller's stack. */
static void ADMG_StartPyramidTrainerBattle(void) __attribute__((noinline));
static void ADMG_StartPyramidTrainerBattle(void)
{
    struct ADMG_PyramidTrainerBattleInput admitted;
    if (!ADMG_TryAdmitPyramidTrainerBattle(&admitted)) return;
    ADMG_RunTrainerBattleStart(&admitted);
}

static void ADMG_StartHillTrainerBattle(void)
{
    struct ADMG_HillTrainerBattleInput admitted;
    if (!ADMG_TryAdmitHillTrainerBattle(&admitted)) return;
    struct ADMG_HillTrainerBattleInput *previous = sADMG_HillTrainerBattle;
    sADMG_HillTrainerBattle = &admitted;
    ADMG_RunTrainerBattleStart(NULL);
    sADMG_HillTrainerBattle = previous;
}

void BattleSetup_StartTrainerBattle(void)
{
    if (gNoOfApproachingTrainers > NUM_APPROACHING_TRAINER)
    { ADMG_StateAccessRejected(gNoOfApproachingTrainers, __func__); return; }
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE) ADMG_StartPyramidTrainerBattle();
    else if (InTrainerHillChallenge()) ADMG_StartHillTrainerBattle();
    else ADMG_RunTrainerBattleStart(NULL);
}

static void CB2_EndDebugBattle(void)
{
    if (gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
    {
        for (u32 i = 0; i < 3; i++)
        {
            u16 monId = gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1;
            if (monId < PARTY_SIZE)
                SavePlayerPartyMon(gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1, &gParties[B_TRAINER_PLAYER][i]);
        }
        LoadPlayerParty();
    }
    SetMainCallback2(CB2_EndTrainerBattle);
}

void BattleSetup_StartTrainerBattle_Debug(void)
{
    sNoOfPossibleTrainerRetScripts = gNoOfApproachingTrainers;
    gNoOfApproachingTrainers = 0;
    sShouldCheckTrainerBScript = FALSE;
    gWhichTrainerToFaceAfterBattle = 0;
    gMain.savedCallback = CB2_EndDebugBattle;

    { u64 queued = ADMG_BeginStateAccessCheck(); CreateBattleStartTask_Debug(GetWildBattleTransition(), 0); if (!ADMG_EndStateAccessCheck(queued)) return; }

    ScriptContext_Stop();
}

static void SaveChangesToPlayerParty(void)
{
    u8 i = 0, j = 0;
    u8 participatedPokemon = VarGet(ADMG_GetFeatureBinding(ADMG_BINDING_B_VAR_SKY_BATTLE));
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if ((participatedPokemon >> i & 1) == 1)
        {
            SavePlayerPartyMon(i, &gParties[B_TRAINER_PLAYER][j]);
            j++;
        }
    }
}

static void HandleBattleVariantEndParty(void)
{
    if (ADMG_GetFeatureBinding(ADMG_BINDING_B_FLAG_SKY_BATTLE) == 0 || !FlagGet(ADMG_GetFeatureBinding(ADMG_BINDING_B_FLAG_SKY_BATTLE)))
        return;
    SaveChangesToPlayerParty();
    LoadPlayerParty();
    FlagClear(ADMG_GetFeatureBinding(ADMG_BINDING_B_FLAG_SKY_BATTLE));
}

static void CB2_EndTrainerBattle(void)
{
    HandleBattleVariantEndParty();

    gIsDebugBattle = FALSE;
    if (FollowerNPCIsBattlePartner())
    {
        RestorePartyAfterFollowerNPCBattle();
        if (ADMG_GetFeatureBinding(ADMG_BINDING_FNPC_FLAG_HEAL_AFTER_FOLLOWER_BATTLE) != 0
         && (ADMG_GetFeatureBinding(ADMG_BINDING_FNPC_FLAG_HEAL_AFTER_FOLLOWER_BATTLE) == FNPC_ALWAYS
         || FlagGet(ADMG_GetFeatureBinding(ADMG_BINDING_FNPC_FLAG_HEAL_AFTER_FOLLOWER_BATTLE))))
            HealPlayerParty();
    }


    if (!TRAINER_BATTLE_PARAM.earlyRival && IsNativeTrainerBattleContent(TRAINER_BATTLE_PARAM.opponentA))
    {
        if (IsPlayerDefeated(gBattleOutcome))
        {
            (void)ADMG_TryCompleteNativeBattleNpc(TRAINER_BATTLE_PARAM.opponentA, FALSE);
            SetMainCallback2(CB2_WhiteOut);
        }
        else
        {
            SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
            if (!NativeBattleDefeatPairValid())
            {
                ADMG_DiscardNativeBattleAssociation();
                DowngradeBadPoison();
                return;
            }
            SetBattledTrainersFlags();
            ADMG_DiscardNativeBattleAssociation();
            DowngradeBadPoison();
        }
        return;
    }
    if (TRAINER_BATTLE_PARAM.earlyRival && IsNativeTrainerBattleContent(TRAINER_BATTLE_PARAM.opponentA))
        ADMG_DiscardNativeBattleAssociation();
    if (TRAINER_BATTLE_PARAM.earlyRival)
    {
        if (IsPlayerDefeated(gBattleOutcome) == TRUE)
        {
            gSpecialVar_Result = TRUE;
            if (GetRivalBattleFlags() & RIVAL_BATTLE_HEAL_AFTER)
            {
                HealPlayerParty();
            }
            else
            {
                SetMainCallback2(CB2_WhiteOut);
                return;
            }
        }
        else
        {
            gSpecialVar_Result = FALSE;
        }
        DowngradeBadPoison();
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
        SetBattledTrainerFlag();
    }
    else if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_SECRET_BASE)
    {
        DowngradeBadPoison();
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
    }
    else if (IsPlayerDefeated(gBattleOutcome))
    {
        if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE || InTrainerHillChallenge() || FlagGet(ADMG_GetFeatureBinding(ADMG_BINDING_B_FLAG_NO_WHITEOUT)))
            SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
        else
            SetMainCallback2(CB2_WhiteOut);
    }
    else
    {
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
        DowngradeBadPoison();
        if (CurrentBattlePyramidLocation() == PYRAMID_LOCATION_NONE && !InTrainerHillChallenge())
        {
            RegisterTrainerInMatchCall();
            SetBattledTrainersFlags();
        }
    }
}

static void CB2_EndRematchBattle(void)
{
    if (IsNativeTrainerBattleContent(TRAINER_BATTLE_PARAM.opponentA))
    {
        if (IsPlayerDefeated(gBattleOutcome))
        {
            (void)ADMG_TryCompleteNativeBattleNpc(TRAINER_BATTLE_PARAM.opponentA, FALSE);
            SetMainCallback2(CB2_WhiteOut);
        }
        else
        {
            SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
            if (!NativeBattleDefeatPairValid())
            {
                ADMG_DiscardNativeBattleAssociation();
                DowngradeBadPoison();
                return;
            }
            SetBattledTrainersFlags();
            (void)ADMG_TryCompleteNativeBattleNpc(TRAINER_BATTLE_PARAM.opponentA, TRUE);
            SetBattledTrainersFlags();
            DowngradeBadPoison();
        }
        return;
    }

    if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_SECRET_BASE)
    {
        DowngradeBadPoison();
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
    }
    else if (IsPlayerDefeated(gBattleOutcome) == TRUE)
    {
        SetMainCallback2(CB2_WhiteOut);
    }
    else
    {
        SetMainCallback2(CB2_ReturnToFieldContinueScriptPlayMapMusic);
        RegisterTrainerInMatchCall();
        SetBattledTrainersFlags();
        HandleRematchVarsOnBattleEnd();
        DowngradeBadPoison();
    }
}

void BattleSetup_StartRematchBattle(void)
{
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    if (GetTrainerBattleType(TRAINER_BATTLE_PARAM.opponentA) == TRAINER_BATTLE_TYPE_DOUBLES)
        gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
    
    gMain.savedCallback = CB2_EndRematchBattle;
    DoTrainerBattle();
    ScriptContext_Stop();
}

void ShowTrainerIntroSpeech(void)
{
    if (CurrentBattlePyramidLocation() != PYRAMID_LOCATION_NONE)
    {
        if (gNoOfApproachingTrainers > NUM_APPROACHING_TRAINER)
        { ADMG_StateAccessRejected(gNoOfApproachingTrainers, __func__); return; }
        u32 localId;
        if (gNoOfApproachingTrainers <= 1)
            localId = gSpecialVar_LastTalked;
        else
        {
            if (gApproachingTrainerId >= NUM_APPROACHING_TRAINER
                || gApproachingTrainers[gApproachingTrainerId].objectEventId >= OBJECT_EVENTS_COUNT)
            { ADMG_StateAccessRejected(gApproachingTrainerId, __func__); return; }
            localId = gObjectEvents[gApproachingTrainers[gApproachingTrainerId].objectEventId].localId;
        }
        if (localId > UCHAR_MAX)
        { ADMG_StateAccessRejected(localId, __func__); return; }
        u64 generation = ADMG_BeginStateAccessCheck();
        enum TrainerId trainerId = LocalIdToPyramidTrainerId(localId);
        if (!ADMG_EndStateAccessCheck(generation)) return;
        CopyPyramidTrainerSpeechBefore(trainerId);
        if (!ADMG_EndStateAccessCheck(generation)) return;

        ShowFieldMessageFromBuffer();
    }
    else if (InTrainerHillChallenge())
    {
        if (gNoOfApproachingTrainers > NUM_APPROACHING_TRAINER)
        { ADMG_StateAccessRejected(gNoOfApproachingTrainers, __func__); return; }
        u32 count = gNoOfApproachingTrainers;
        u32 approach = gApproachingTrainerId, object = 0;
        u32 localId;
        if (gNoOfApproachingTrainers <= 1) localId = gSpecialVar_LastTalked;
        else
        {
            if (gApproachingTrainerId >= NUM_APPROACHING_TRAINER
                || gApproachingTrainers[gApproachingTrainerId].objectEventId >= OBJECT_EVENTS_COUNT)
            { ADMG_StateAccessRejected(gApproachingTrainerId, __func__); return; }
            localId = gObjectEvents[gApproachingTrainers[gApproachingTrainerId].objectEventId].localId;
            object = gApproachingTrainers[gApproachingTrainerId].objectEventId;
        }
        if (localId > UCHAR_MAX)
        { ADMG_StateAccessRejected(localId, __func__); return; }
        u64 generation = ADMG_BeginStateAccessCheck();
        enum TrainerId trainerId = LocalIdToHillTrainerId(localId);
        if (!ADMG_EndStateAccessCheck(generation)) return;
        if (gNoOfApproachingTrainers != count
            || (count <= 1 && gSpecialVar_LastTalked != localId)
            || (count > 1 && (gApproachingTrainerId != approach
                || gApproachingTrainers[approach].objectEventId != object
                || gObjectEvents[object].localId != localId)))
        { ADMG_StateAccessRejected(localId, __func__); return; }
        CopyTrainerHillTrainerText(TRAINER_HILL_TEXT_INTRO, trainerId);
        if (!ADMG_EndStateAccessCheck(generation)) return;
        if (gNoOfApproachingTrainers != count
            || (count <= 1 && gSpecialVar_LastTalked != localId)
            || (count > 1 && (gApproachingTrainerId != approach
                || gApproachingTrainers[approach].objectEventId != object
                || gObjectEvents[object].localId != localId)))
        { ADMG_StateAccessRejected(localId, __func__); return; }
        ShowFieldMessageFromBuffer();
    }
    else
    {
        ShowFieldMessage(GetIntroSpeechOfApproachingTrainer());
    }
}

const u8 *BattleSetup_GetScriptAddrAfterBattle(void)
{
    if (sTrainerBattleEndScript != NULL)
        return sTrainerBattleEndScript;
    else
        return EventScript_TestSignpostMsg;
}

const u8 *BattleSetup_GetTrainerPostBattleScript(void)
{
    if (sShouldCheckTrainerBScript)
    {
        sShouldCheckTrainerBScript = FALSE;
        if (TRAINER_BATTLE_PARAM.battleScriptRetAddrB != NULL)
        {
            gWhichTrainerToFaceAfterBattle = 1;
            return TRAINER_BATTLE_PARAM.battleScriptRetAddrB;
        }
    }
    else
    {
        if (TRAINER_BATTLE_PARAM.battleScriptRetAddrA != NULL)
        {
            gWhichTrainerToFaceAfterBattle = 0;
            return TRAINER_BATTLE_PARAM.battleScriptRetAddrA;
        }
    }

    return EventScript_TryGetTrainerScript;
}

void ShowTrainerCantBattleSpeech(void)
{
    ShowFieldMessage(GetTrainerCantBattleSpeech());
}

void PlayTrainerEncounterMusic(void)
{
    enum TrainerId trainerId;
    u16 music;

    if (gApproachingTrainerId == 0)
        trainerId = TRAINER_BATTLE_PARAM.opponentA;
    else
        trainerId = TRAINER_BATTLE_PARAM.opponentB;

    u64 generation = ADMG_BeginStateAccessCheck();
    u32 encounterMusic = GetTrainerEncounterMusicId(trainerId);
    if (!ADMG_EndStateAccessCheck(generation)) return;
    switch (encounterMusic)
    {
    case TRAINER_ENCOUNTER_MUSIC_MALE:
        music = MUS_ENCOUNTER_MALE;
        break;
    case TRAINER_ENCOUNTER_MUSIC_FEMALE:
        music = MUS_ENCOUNTER_FEMALE;
        break;
    case TRAINER_ENCOUNTER_MUSIC_GIRL:
        music = MUS_ENCOUNTER_GIRL;
        break;
    case TRAINER_ENCOUNTER_MUSIC_INTENSE:
        music = MUS_ENCOUNTER_INTENSE;
        break;
    case TRAINER_ENCOUNTER_MUSIC_COOL:
        music = MUS_ENCOUNTER_COOL;
        break;
    case TRAINER_ENCOUNTER_MUSIC_AQUA:
        music = MUS_ENCOUNTER_AQUA;
        break;
    case TRAINER_ENCOUNTER_MUSIC_MAGMA:
        music = MUS_ENCOUNTER_MAGMA;
        break;
    case TRAINER_ENCOUNTER_MUSIC_SWIMMER:
        music = MUS_ENCOUNTER_SWIMMER;
        break;
    case TRAINER_ENCOUNTER_MUSIC_TWINS:
        music = MUS_ENCOUNTER_TWINS;
        break;
    case TRAINER_ENCOUNTER_MUSIC_ELITE_FOUR:
        music = MUS_ENCOUNTER_ELITE_FOUR;
        break;
    case TRAINER_ENCOUNTER_MUSIC_HIKER:
        music = MUS_ENCOUNTER_HIKER;
        break;
    case TRAINER_ENCOUNTER_MUSIC_INTERVIEWER:
        music = MUS_ENCOUNTER_INTERVIEWER;
        break;
    case TRAINER_ENCOUNTER_MUSIC_RICH:
        music = MUS_ENCOUNTER_RICH;
        break;
    default:
        music = MUS_ENCOUNTER_SUSPICIOUS;
    }
    PlayNewMapMusic(music);

}

static const u8 *ReturnEmptyStringIfNull(const u8 *string)
{
    if (string == NULL)
        return gText_EmptyString2;
    else
        return string;
}

static const u8 *GetIntroSpeechOfApproachingTrainer(void)
{
    if (gApproachingTrainerId == 0)
    {
        if (OW_NAME_BOX_NPC_TRAINER)
            gSpeakerName = GetTrainerNameFromId(TRAINER_BATTLE_PARAM.opponentA);

        return ReturnEmptyStringIfNull(TRAINER_BATTLE_PARAM.introTextA);
    }
    else
    {
        if (OW_NAME_BOX_NPC_TRAINER)
            gSpeakerName = GetTrainerNameFromId(TRAINER_BATTLE_PARAM.opponentB);

        return ReturnEmptyStringIfNull(TRAINER_BATTLE_PARAM.introTextB);
    }
}

const u8 *GetTrainerALoseText(void)
{
    const u8 *string;

    if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_SECRET_BASE)
        string = GetSecretBaseTrainerLoseText();
    else
        string = TRAINER_BATTLE_PARAM.defeatTextA;

    StringExpandPlaceholders(gStringVar4, ReturnEmptyStringIfNull(string));
    return gStringVar4;
}

const u8 *GetTrainerBLoseText(void)
{
    StringExpandPlaceholders(gStringVar4, ReturnEmptyStringIfNull(TRAINER_BATTLE_PARAM.defeatTextB));
    return gStringVar4;
}

const u8 *GetTrainerWonSpeech(void)
{
    StringExpandPlaceholders(gStringVar4, ReturnEmptyStringIfNull(TRAINER_BATTLE_PARAM.victoryText));
    return gStringVar4;
}

static const u8 *GetTrainerCantBattleSpeech(void)
{
    return ReturnEmptyStringIfNull(TRAINER_BATTLE_PARAM.cannotBattleText);
}

s32 FirstBattleTrainerIdToRematchTableId(const struct RematchTrainer *table, enum TrainerId trainerId)
{
    s32 i;

    for (i = 0; i < REMATCH_TABLE_ENTRIES; i++)
    {
        if (table[i].trainerIds[0] == trainerId)
            return i;
    }

    return -1;
}

s32 TrainerIdToRematchTableId(const struct RematchTrainer *table, enum TrainerId trainerId)
{
    s32 i, j;

    for (i = 0; i < REMATCH_TABLE_ENTRIES; i++)
    {
        for (j = 0; j < REMATCHES_COUNT; j++)
        {
            if (table[i].trainerIds[j] == 0) break; // one line required to match -g
            if (table[i].trainerIds[j] == trainerId)
                return i;
        }
    }

    return -1;
}

// Returns TRUE if the given trainer (by their entry in the rematch table) is not allowed to have rematches.
// This applies to the Elite Four and Victory Road Wally (if he's not been defeated yet)
static inline bool32 IsRematchForbidden(s32 rematchTableId)
{
    if (rematchTableId >= REMATCH_ELITE_FOUR_ENTRIES)
        return TRUE;
    else if (rematchTableId == REMATCH_WALLY_VR)
        return !FlagGet(FLAG_DEFEATED_WALLY_VICTORY_ROAD);
    else
        return FALSE;
}

static void SetRematchIdForTrainer(const struct RematchTrainer *table, u32 tableId)
{
#if FREE_MATCH_CALL == FALSE
    s32 i;

    for (i = 1; i < REMATCHES_COUNT; i++)
    {
        u16 trainerId = table[tableId].trainerIds[i];

        if (trainerId == 0)
            break;
        if (!HasTrainerBeenFought(trainerId))
            break;
    }

    gSaveBlock1Ptr->trainerRematches[tableId] = i;
#endif //FREE_MATCH_CALL
}

static inline bool32 DoesCurrentMapMatchRematchTrainerMap(s32 i, const struct RematchTrainer *table, MapId mapId)
{
    return table[i].mapId == mapId;
}

bool32 TrainerIsMatchCallRegistered(s32 i)
{
    u16 flagId;
    bool8 registered;
    if (!ADMG_TryGetIndexedFlagId(TRAINER_REGISTERED_FLAGS_START, (u32)i,
            REMATCH_TABLE_ENTRIES, 1, &flagId)
     || !ADMG_TryReadFlag(flagId, &registered))
        return FALSE;
    return registered;
}

#if FREE_MATCH_CALL == FALSE
static bool32 UpdateRandomTrainerRematches(const struct RematchTrainer *table, MapId mapId)
{
    s32 i;

    if (CheckBagHasItem(ITEM_VS_SEEKER, 1) && ADMG_GetFeatureBinding(ADMG_BINDING_I_VS_SEEKER_CHARGING) != 0)
        return FALSE;

    for (i = 0; i < REMATCH_SPECIAL_TRAINER_START; i++)
    {
        if (!DoesCurrentMapMatchRematchTrainerMap(i,table,mapId) || IsRematchForbidden(i))
            continue; // Only check permitted trainers within the current map.

        if (gSaveBlock1Ptr->trainerRematches[i] != 0)
        {
            // Trainer already wants a rematch. Don't bother updating it.
            return TRUE;
        }
        else if (TrainerIsMatchCallRegistered(i) && ((Random() % 100) <= 30))
            // 31% chance of getting a rematch.
        {
            SetRematchIdForTrainer(table, i);
            return TRUE;
        }
    }

    return FALSE;
}
#endif //FREE_MATCH_CALL

void UpdateRematchIfDefeated(s32 rematchTableId)
{
    if (HasTrainerBeenFought(gRematchTable[rematchTableId].trainerIds[0]) == TRUE)
        SetRematchIdForTrainer(gRematchTable, rematchTableId);
}

static bool8 IsFirstTrainerIdReadyForRematch(const struct RematchTrainer *table, enum TrainerId firstBattleTrainerId)
{
    s32 tableId = FirstBattleTrainerIdToRematchTableId(table, firstBattleTrainerId);

    if (tableId == -1)
        return FALSE;
    if (tableId >= MAX_REMATCH_ENTRIES)
        return FALSE;
#if FREE_MATCH_CALL == FALSE
    if (gSaveBlock1Ptr->trainerRematches[tableId] == 0)
        return FALSE;
#endif //FREE_MATCH_CALL

    return TRUE;
}

static bool8 IsTrainerReadyForRematch_(const struct RematchTrainer *table, enum TrainerId trainerId)
{
    s32 tableId = TrainerIdToRematchTableId(table, trainerId);

    if (tableId == -1)
        return FALSE;
    if (tableId >= MAX_REMATCH_ENTRIES)
        return FALSE;
#if FREE_MATCH_CALL == FALSE
    if (gSaveBlock1Ptr->trainerRematches[tableId] == 0)
        return FALSE;
#endif //FREE_MATCH_CALL

    return TRUE;
}

u16 GetRematchTrainerIdFromTable(const struct RematchTrainer *table, enum TrainerId firstBattleTrainerId)
{
    const struct RematchTrainer *trainerEntry;
    s32 i;
    s32 tableId = FirstBattleTrainerIdToRematchTableId(table, firstBattleTrainerId);

    if (tableId == -1)
        return FALSE;

    trainerEntry = &table[tableId];
    for (i = 1; i < REMATCHES_COUNT; i++)
    {
        if (trainerEntry->trainerIds[i] == 0) // previous entry was this trainer's last one
            return trainerEntry->trainerIds[i - 1];
        if (!HasTrainerBeenFought(trainerEntry->trainerIds[i]))
            return trainerEntry->trainerIds[i];
    }

    return trainerEntry->trainerIds[REMATCHES_COUNT - 1]; // already beaten at max stage
}

static u16 GetLastBeatenRematchTrainerIdFromTable(const struct RematchTrainer *table, u16 firstBattleTrainerId)
{
    const struct RematchTrainer *trainerEntry;
    s32 i;
    s32 tableId = FirstBattleTrainerIdToRematchTableId(table, firstBattleTrainerId);

    if (tableId == -1)
        return FALSE;

    trainerEntry = &table[tableId];
    for (i = 1; i < REMATCHES_COUNT; i++)
    {
        if (trainerEntry->trainerIds[i] == 0) // previous entry was this trainer's last one
            return trainerEntry->trainerIds[i - 1];
        if (!HasTrainerBeenFought(trainerEntry->trainerIds[i]))
            return trainerEntry->trainerIds[i - 1];
    }

    return trainerEntry->trainerIds[REMATCHES_COUNT - 1]; // already beaten at max stage
}

static void ClearTrainerWantRematchState(const struct RematchTrainer *table, enum TrainerId firstBattleTrainerId)
{
#if FREE_MATCH_CALL == FALSE
    s32 tableId = TrainerIdToRematchTableId(table, firstBattleTrainerId);

    if (tableId != -1)
        gSaveBlock1Ptr->trainerRematches[tableId] = 0;
#endif //FREE_MATCH_CALL
}

void ClearCurrentTrainerWantRematchVsSeeker(void)
{
    if (IsNativeTrainerBattleContent(TRAINER_BATTLE_PARAM.opponentA)) return;

#if FREE_MATCH_CALL == FALSE
    if ((gBattleTypeFlags & BATTLE_TYPE_TRAINER) && FlagGet(ADMG_GetFeatureBinding(ADMG_BINDING_I_VS_SEEKER_CHARGING)) && (ADMG_GetFeatureBinding(ADMG_BINDING_I_VS_SEEKER_CHARGING) != 0))
    {
        // Clear each rematch row whose teams include the opponent
        // The saved byte holds a rematch level, so compare trainer IDs in the table instead
        for (u32 i = 0; i < REMATCH_TABLE_ENTRIES && i < MAX_REMATCH_ENTRIES; i++)
        {
            for (u32 j = 0; j < REMATCHES_COUNT; j++)
            {
                if (gRematchTable[i].trainerIds[j] == TRAINER_BATTLE_PARAM.opponentA)
                    gSaveBlock1Ptr->trainerRematches[i] = 0;
            }
        }
    }
#endif //FREE_MATCH_CALL
}

static u32 GetTrainerMatchCallFlag(u32 trainerId)
{
    s32 i;

    for (i = 0; i < REMATCH_TABLE_ENTRIES; i++)
    {
        if (gRematchTable[i].trainerIds[0] == trainerId)
            return TRAINER_REGISTERED_FLAGS_START + i;
    }

    return 0xFFFF;
}

static void RegisterTrainerInMatchCall(void)
{
    if (FlagGet(FLAG_HAS_MATCH_CALL))
    {
        u32 matchCallFlagId = GetTrainerMatchCallFlag(TRAINER_BATTLE_PARAM.opponentA);
        if (matchCallFlagId != 0xFFFF)
            FlagSet(matchCallFlagId);
    }
}

static bool8 WasSecondRematchWon(const struct RematchTrainer *table, enum TrainerId firstBattleTrainerId)
{
    s32 tableId = FirstBattleTrainerIdToRematchTableId(table, firstBattleTrainerId);

    if (tableId == -1)
        return FALSE;
    if (!HasTrainerBeenFought(table[tableId].trainerIds[1]))
        return FALSE;
#if FREE_MATCH_CALL == FALSE
    if (ADMG_GetFeatureBinding(ADMG_BINDING_I_VS_SEEKER_CHARGING))
    {
        if (gSaveBlock1Ptr->trainerRematches[tableId] == 0)
            return FALSE;
    }
#endif
    return TRUE;
}

#if FREE_MATCH_CALL == FALSE
static bool32 HasEnoughBadgesForRematch(void)
{
    s32 i, count;

    for (count = 0, i = 0; i < ARRAY_COUNT(gBadgeFlags); i++)
    {
        if (FlagGet(gBadgeFlags[i]) == TRUE)
        {
            if (++count >= OW_REMATCH_BADGE_COUNT)
                return TRUE;
        }
    }

    return FALSE;
}
#endif //FREE_MATCH_CALL

#define STEP_COUNTER_MAX 255

void IncrementRematchStepCounter(void)
{
#if FREE_MATCH_CALL == FALSE
    if (!HasEnoughBadgesForRematch())
        return;

    if (IsVsSeekerEnabled())
        return;

    if (gSaveBlock1Ptr->trainerRematchStepCounter >= STEP_COUNTER_MAX)
        gSaveBlock1Ptr->trainerRematchStepCounter = STEP_COUNTER_MAX;
    else
        gSaveBlock1Ptr->trainerRematchStepCounter++;
#endif //FREE_MATCH_CALL
}

#if FREE_MATCH_CALL == FALSE
static bool32 IsRematchStepCounterMaxed(void)
{
    if (HasEnoughBadgesForRematch() && gSaveBlock1Ptr->trainerRematchStepCounter >= STEP_COUNTER_MAX)
        return TRUE;
    else
        return FALSE;
}

void TryUpdateRandomTrainerRematches(MapId mapId)
{
    if (IsRematchStepCounterMaxed() && UpdateRandomTrainerRematches(gRematchTable, mapId) == TRUE)
        gSaveBlock1Ptr->trainerRematchStepCounter = 0;
}
#endif //FREE_MATCH_CALL

ADMG_NATIVE_ENTRY u16 GetRematchTrainerId(enum TrainerId trainerId)
{
    struct ADMG_TrainerRef ref;
    u16 selected;
    if (!ADMG_TryResolveTrainer(trainerId, &ref))
        return 0;
    if (ref.owner == ADMG_TRAINER_KANTO)
        return ADMG_TryGetNativeRematchTrainer(trainerId, &selected) ? selected : 0;
    if (ref.owner != ADMG_TRAINER_HOENN)
    {
        ADMG_StateAccessRejected(trainerId, __func__);
        return 0;
    }
    if (FlagGet(ADMG_GetFeatureBinding(ADMG_BINDING_I_VS_SEEKER_CHARGING)) && (ADMG_GetFeatureBinding(ADMG_BINDING_I_VS_SEEKER_CHARGING) != 0))
        return GetRematchTrainerIdVSSeeker(trainerId);
    return GetRematchTrainerIdFromTable(gRematchTable, trainerId);
}

u16 GetLastBeatenRematchTrainerId(u16 trainerId)
{
    return GetLastBeatenRematchTrainerIdFromTable(gRematchTable, trainerId);
}

bool8 ShouldTryRematchBattle(void)
{
    return ShouldTryRematchBattleForTrainerId(TRAINER_BATTLE_PARAM.opponentA);
}

bool8 ShouldTryRematchBattleForTrainerId(enum TrainerId trainerId)
{
    struct ADMG_TrainerRef ref;
    struct ADMG_RematchContext context;
    bool8 result;

    if (!ADMG_TryResolveTrainer(trainerId, &ref))
        return FALSE;
    if (ref.owner == ADMG_TRAINER_KANTO)
    {
        if (!ADMG_TryCaptureNativeRematchContext(&context))
            return FALSE;
        return ADMG_TryShouldRematchNativeNpc(&context,
            trainerId, gSpecialVar_LastTalked, &result) && result;
    }
    if (ref.owner != ADMG_TRAINER_HOENN)
    {
        ADMG_StateAccessRejected(trainerId, __func__);
        return FALSE;
    }

    if (IsFirstTrainerIdReadyForRematch(gRematchTable, trainerId))
        return TRUE;

    return WasSecondRematchWon(gRematchTable, trainerId);
}

bool8 IsTrainerReadyForRematch(void)
{
    struct ADMG_TrainerRef ref;
    struct ADMG_RematchContext context;
    bool8 result;
    enum TrainerId trainerId = TRAINER_BATTLE_PARAM.opponentA;

    if (!ADMG_TryResolveTrainer(trainerId, &ref))
        return FALSE;
    if (ref.owner == ADMG_TRAINER_KANTO)
    {
        if (!ADMG_TryCaptureNativeRematchContext(&context))
            return FALSE;
        return ADMG_TryIsNativeNpcReady(&context,
            trainerId, gSpecialVar_LastTalked, &result) && result;
    }
    // Added trainers have no rematch table entry, so return FALSE without reporting a rejection
    if (ref.owner == ADMG_TRAINER_ADDED)
        return FALSE;
    if (ref.owner != ADMG_TRAINER_HOENN)
    {
        ADMG_StateAccessRejected(trainerId, __func__);
        return FALSE;
    }

    return IsTrainerReadyForRematch_(gRematchTable, TRAINER_BATTLE_PARAM.opponentA);
}

static void HandleRematchVarsOnBattleEnd(void)
{
    if ((gBattleTypeFlags & BATTLE_TYPE_TRAINER) && (ADMG_GetFeatureBinding(ADMG_BINDING_I_VS_SEEKER_CHARGING) != 0))
        ClearRematchMovementByTrainerId();

    ClearTrainerWantRematchState(gRematchTable, TRAINER_BATTLE_PARAM.opponentA);
    SetBattledTrainersFlags();
}

void ShouldTryGetTrainerScript(void)
{
    if (sNoOfPossibleTrainerRetScripts > 1)
    {
        sNoOfPossibleTrainerRetScripts = 0;
        sShouldCheckTrainerBScript = TRUE;
        gSpecialVar_Result = TRUE;
    }
    else
    {
        sShouldCheckTrainerBScript = FALSE;
        gSpecialVar_Result = FALSE;
    }
}

u16 CountMaxPossibleRematch(u16 trainerId)
{
    for (u32 i = 1; i < REMATCHES_COUNT; i++)
    {
        if (gRematchTable[trainerId].trainerIds[i] == 0)
            return i;
    }
    return REMATCHES_COUNT - 1;
}

u16 CountBattledRematchTeams(u16 trainerId)
{
    if (HasTrainerBeenFought(gRematchTable[trainerId].trainerIds[0]) != TRUE)
        return 0;

    for (u32 i = 1; i < REMATCHES_COUNT; i++)
    {
        if (gRematchTable[trainerId].trainerIds[i] == 0)
            return i;
        if (!HasTrainerBeenFought(gRematchTable[trainerId].trainerIds[i]))
            return i;
    }

    return REMATCHES_COUNT - 1;
}

void SetMultiTrainerBattle(struct ScriptContext *ctx)
{
    if (ctx == NULL || ctx->scriptPtr == NULL)
    { ADMG_StateAccessRejected(0, __func__); ADMG_StopInvalidStateCommand(ctx); return; }
    /* event.inc writes two halfword/word pairs followed by a partner ordinal.
     * The battle mode is assigned afterwards; trainer words remain transport
     * until the existing mode-aware consumer admits their used roles. */
    const u32 partnerOffset = 2 * sizeof(enum TrainerId) + 2 * sizeof(u32);
    struct ScriptContext partnerContext = *ctx;
    partnerContext.scriptPtr += partnerOffset;
    u32 partnerOrdinal = ScriptPeekHalfword(&partnerContext);
    if (partnerOrdinal >= PARTNER_COUNT)
    { ADMG_StateAccessRejected(partnerOrdinal, __func__); ADMG_StopInvalidStateCommand(ctx); return; }

    InitTrainerBattleParameter();

    TRAINER_BATTLE_PARAM.opponentA = ScriptReadWord(ctx);
    TRAINER_BATTLE_PARAM.defeatTextA = (u8*)ScriptReadWord(ctx);
    TRAINER_BATTLE_PARAM.opponentB = ScriptReadWord(ctx);
    TRAINER_BATTLE_PARAM.defeatTextB = (u8*)ScriptReadWord(ctx);
    gPartnerTrainerId = TRAINER_PARTNER(ScriptReadHalfword(ctx));
};

void CreateNPCTrainerPartyFromTrainer(struct Pokemon *party, const struct Trainer *trainer)
{
    s32 i;
    u8 monsCount;

    ZeroPartyMons(party);

    monsCount = trainer->partySize;
    if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS && (B_MULTI_HALF_TEAMS || trainer->multiTeamSize == MULTI_TEAM_SIZE_HALF))
    {
        if (monsCount > PARTY_SIZE / 2)
            monsCount = PARTY_SIZE / 2;
    }

    u32 monIndices[monsCount];
    struct TrainerGenerator *trainerGen = AllocZeroed(sizeof(struct TrainerGenerator));
    MakeTrainerGenerator(trainerGen, trainer);
    DoTrainerPartyPool(trainer, monIndices, monsCount, gBattleTypeFlags);

    for (i = 0; i < monsCount; i++)
    {
        u32 monIndex = monIndices[i];
        GenerateMonFromTrainerMon(&party[i], &trainer->party[monIndex], trainerGen);
    }
    Free(trainerGen);
}

static void CreateNPCTrainerParty(struct Pokemon *party, enum TrainerId trainerNum)
{
    const struct Trainer *trainer, *origTrainer;
    struct Trainer tempTrainer;
    if (!ADMG_TryGetTrainerStruct(trainerNum, &trainer))
        return;
    if (!trainer->overrideTrainer)
    {
        CreateNPCTrainerPartyFromTrainer(party, trainer);
        return;
    }
    if (!ADMG_TryGetTrainerStruct(trainer->overrideTrainer, &origTrainer))
        return;
    memcpy(&tempTrainer, trainer, sizeof(struct Trainer));
    tempTrainer.party = origTrainer->party;
    tempTrainer.poolSize = origTrainer->poolSize;
    if (tempTrainer.partySize == 0)
        tempTrainer.partySize = origTrainer->partySize;
    CreateNPCTrainerPartyFromTrainer(party, &tempTrainer);
}

void CreateTrainerPartyForPlayer(void)
{
    const struct Trainer *trainer;
    if (!ADMG_TryGetTrainerStruct(gSpecialVar_0x8004, &trainer))
        return;
    Script_RequestEffects(SCREFF_V1);
    gPartnerTrainerId = gSpecialVar_0x8004;
    CreateNPCTrainerPartyFromTrainer(gParties[B_TRAINER_PLAYER], trainer);
}

static bool8 IsNativeTrainerContent(u32 trainerId)
{
    struct ADMG_TrainerRef ref;
    if (trainerId < ADMG_KANTO_TRAINER_FIRST || trainerId > ADMG_KANTO_TRAINER_LAST)
        return FALSE;
    return ADMG_TryResolveTrainer(trainerId, &ref) && ref.owner == ADMG_TRAINER_KANTO;
}

/* Indexed facility/network actors retain their existing identity domains.
 * Configuration classifies the canonical operand independently of old flags. */
static bool8 IsNativeTrainerBattleContent(u32 trainerId)
{
    if (gBattleTypeFlags & (BATTLE_TYPE_FRONTIER | BATTLE_TYPE_LINK
        | BATTLE_TYPE_RECORDED_LINK | BATTLE_TYPE_EREADER_TRAINER | BATTLE_TYPE_TRAINER_HILL))
        return FALSE;
    return IsNativeTrainerContent(trainerId);
}

/* The existing defeat writer preflights both actors. Completion must use the
 * same validation boundary before changing the captured NPC's readiness. */
static bool8 NativeBattleDefeatPairValid(void)
{
    u32 first, second = 0;
    if (!ADMG_TryGetTrainerDefeatFlagId(TRAINER_BATTLE_PARAM.opponentA, &first))
        return FALSE;
    if (TRAINER_BATTLE_PARAM.opponentB != TRAINER_NONE && TRAINER_BATTLE_PARAM.opponentB != 0xFFFF
     && !ADMG_TryGetTrainerDefeatFlagId(TRAINER_BATTLE_PARAM.opponentB, &second))
        return FALSE;
    if (!ADMG_IsFlagIdValid(first) || !ADMG_IsFlagIdValid(second))
    {
        ADMG_StateAccessRejected(TRAINER_BATTLE_PARAM.opponentA, __func__);
        return FALSE;
    }
    return TRUE;
}

ADMG_NATIVE_ENTRY void ADMG_ClearNativeTrainerBattleRematches(void)
{
    struct ADMG_RematchContext context;
    u32 localId;
    if ((gBattleTypeFlags & BATTLE_TYPE_TRAINER)
     && IsNativeTrainerBattleContent(TRAINER_BATTLE_PARAM.opponentA)
     && ADMG_TryGetNativeBattleNpc(TRAINER_BATTLE_PARAM.opponentA, &context, &localId))
        (void)ADMG_TryClearNativeRematchFamilyMovement(TRAINER_BATTLE_PARAM.opponentA);
}

ADMG_RETAIN_FUNCTION(ADMG_ComposeTrainerBattleScript);
ADMG_RETAIN_FUNCTION(ADMG_ScriptStartMultiBattle);
