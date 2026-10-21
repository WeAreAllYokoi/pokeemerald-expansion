#include "global.h"
#include "admg_state.h"
#include "battle_tower.h"
#include "apprentice.h"
#include "event_data.h"
#include "battle_setup.h"
#include "battle_special.h"
#include "overworld.h"
#include "random.h"
#include "text.h"
#include "main.h"
#include "international_string_util.h"
#include "battle.h"
#include "battle_partner.h"
#include "frontier_util.h"
#include "strings.h"
#include "recorded_battle.h"
#include "easy_chat.h"
#include "gym_leader_rematch.h"
#include "battle_transition.h"
#include "trainer_see.h"
#include "new_game.h"
#include "string_util.h"
#include "data.h"
#include "link.h"
#include "field_message_box.h"
#include "tv.h"
#include "battle_factory.h"
#include "constants/abilities.h"
#include "constants/apprentice.h"
#include "constants/battle_ai.h"
#include "constants/battle_dome.h"
#include "constants/battle_frontier.h"
#include "constants/battle_frontier_mons.h"
#include "constants/battle_move_effects.h"
#include "constants/battle_tent.h"
#include "constants/battle_tent_mons.h"
#include "constants/battle_tent_trainers.h"
#include "constants/battle_tower.h"
#include "constants/battle_partner.h"
#include "constants/frontier_util.h"
#include "constants/items.h"
#include "constants/trainers.h"
#include "constants/event_objects.h"
#include "constants/moves.h"
#include "test/battle.h"

// This file's functions.
static void InitTowerChallenge(void);
static void GetTowerData(void);
static void SetTowerData(void);
static void SetNextTowerOpponent(void);
static void SetTowerBattleWon(void);
static void AwardBattleTowerRibbons(void);
static void SaveTowerChallenge(void);
static void GetOpponentIntroSpeech(void);
static void BattleTowerNop1(void);
static void BattleTowerNop2(void);
ADMG_NATIVE_ENTRY void LoadMultiPartnerCandidatesData(void);
ADMG_NATIVE_ENTRY void ShowPartnerCandidateMessage(void);
ADMG_NATIVE_ENTRY void LoadLinkMultiOpponentsData(void);
static void TowerTryCloseLink(void);
ADMG_NATIVE_ENTRY void SetMultiPartnerGfx(void);
ADMG_NATIVE_ENTRY void SetTowerInterviewData(void);
static void ValidateBattleTowerRecordChecksums(void);
static void SaveCurrentWinStreak(void);
static void ValidateApprenticesChecksums(void);
static void SetNextBattleTentOpponent(void);
static void ClearBattleTowerRecord(struct EmeraldBattleTowerRecord *record);
ADMG_NATIVE_ENTRY void FillTentTrainerParty_(enum TrainerId trainerId, u8 firstMonId, u8 monCount);

#include "data/battle_frontier/battle_frontier_trainer_mons.h"
#include "data/battle_frontier/battle_frontier_trainers.h"
#include "data/battle_frontier/battle_frontier_mons.h"

const struct FacilityClass gTowerMaleFacilityClasses[FACILITY_CLASSES_MALE] =
{
    {FACILITY_CLASS_RUIN_MANIAC, OBJ_EVENT_GFX_HIKER},
    {FACILITY_CLASS_TUBER_M, OBJ_EVENT_GFX_TUBER_M},
    {FACILITY_CLASS_COOLTRAINER_M, OBJ_EVENT_GFX_MAN_3},
    {FACILITY_CLASS_RICH_BOY, OBJ_EVENT_GFX_RICH_BOY},
    {FACILITY_CLASS_POKEMANIAC, OBJ_EVENT_GFX_MANIAC},
    {FACILITY_CLASS_SWIMMER_M, OBJ_EVENT_GFX_RUNNING_TRIATHLETE_M},
    {FACILITY_CLASS_BLACK_BELT, OBJ_EVENT_GFX_BLACK_BELT},
    {FACILITY_CLASS_GUITARIST, OBJ_EVENT_GFX_MAN_5},
    {FACILITY_CLASS_KINDLER, OBJ_EVENT_GFX_MAN_5},
    {FACILITY_CLASS_CAMPER, OBJ_EVENT_GFX_CAMPER},
    {FACILITY_CLASS_BUG_MANIAC, OBJ_EVENT_GFX_MANIAC},
    {FACILITY_CLASS_PSYCHIC_M, OBJ_EVENT_GFX_PSYCHIC_M},
    {FACILITY_CLASS_GENTLEMAN, OBJ_EVENT_GFX_GENTLEMAN},
    {FACILITY_CLASS_SCHOOL_KID_M, OBJ_EVENT_GFX_SCHOOL_KID_M},
    {FACILITY_CLASS_POKEFAN_M, OBJ_EVENT_GFX_POKEFAN_M},
    {FACILITY_CLASS_EXPERT_M, OBJ_EVENT_GFX_EXPERT_M},
    {FACILITY_CLASS_YOUNGSTER, OBJ_EVENT_GFX_YOUNGSTER},
    {FACILITY_CLASS_FISHERMAN, OBJ_EVENT_GFX_FISHERMAN},
    {FACILITY_CLASS_CYCLING_TRIATHLETE_M, OBJ_EVENT_GFX_CYCLING_TRIATHLETE_M},
    {FACILITY_CLASS_RUNNING_TRIATHLETE_M, OBJ_EVENT_GFX_RUNNING_TRIATHLETE_M},
    {FACILITY_CLASS_SWIMMING_TRIATHLETE_M, OBJ_EVENT_GFX_RUNNING_TRIATHLETE_M},
    {FACILITY_CLASS_DRAGON_TAMER, OBJ_EVENT_GFX_MAN_3},
    {FACILITY_CLASS_BIRD_KEEPER, OBJ_EVENT_GFX_MAN_5},
    {FACILITY_CLASS_NINJA_BOY, OBJ_EVENT_GFX_NINJA_BOY},
    {FACILITY_CLASS_SAILOR, OBJ_EVENT_GFX_SAILOR},
    {FACILITY_CLASS_COLLECTOR, OBJ_EVENT_GFX_MANIAC},
    {FACILITY_CLASS_PKMN_BREEDER_M, OBJ_EVENT_GFX_MAN_4},
    {FACILITY_CLASS_PKMN_RANGER_M, OBJ_EVENT_GFX_CAMPER},
    {FACILITY_CLASS_BUG_CATCHER, OBJ_EVENT_GFX_BUG_CATCHER},
    {FACILITY_CLASS_HIKER, OBJ_EVENT_GFX_HIKER},
};

const struct FacilityClass gTowerFemaleFacilityClasses[FACILITY_CLASSES_FEMALE] =
{
    {FACILITY_CLASS_AROMA_LADY, OBJ_EVENT_GFX_WOMAN_2},
    {FACILITY_CLASS_TUBER_F, OBJ_EVENT_GFX_TUBER_F},
    {FACILITY_CLASS_COOLTRAINER_F, OBJ_EVENT_GFX_WOMAN_5},
    {FACILITY_CLASS_HEX_MANIAC, OBJ_EVENT_GFX_HEX_MANIAC},
    {FACILITY_CLASS_LADY, OBJ_EVENT_GFX_WOMAN_2},
    {FACILITY_CLASS_BEAUTY, OBJ_EVENT_GFX_BEAUTY},
    {FACILITY_CLASS_PSYCHIC_F, OBJ_EVENT_GFX_LASS},
    {FACILITY_CLASS_SCHOOL_KID_F, OBJ_EVENT_GFX_GIRL_3},
    {FACILITY_CLASS_POKEFAN_F, OBJ_EVENT_GFX_POKEFAN_F},
    {FACILITY_CLASS_EXPERT_F, OBJ_EVENT_GFX_EXPERT_F},
    {FACILITY_CLASS_CYCLING_TRIATHLETE_F, OBJ_EVENT_GFX_CYCLING_TRIATHLETE_F},
    {FACILITY_CLASS_RUNNING_TRIATHLETE_F, OBJ_EVENT_GFX_RUNNING_TRIATHLETE_F},
    {FACILITY_CLASS_SWIMMING_TRIATHLETE_F, OBJ_EVENT_GFX_RUNNING_TRIATHLETE_F},
    {FACILITY_CLASS_BATTLE_GIRL, OBJ_EVENT_GFX_GIRL_3},
    {FACILITY_CLASS_PARASOL_LADY, OBJ_EVENT_GFX_WOMAN_5},
    {FACILITY_CLASS_SWIMMER_F, OBJ_EVENT_GFX_RUNNING_TRIATHLETE_F},
    {FACILITY_CLASS_PICNICKER, OBJ_EVENT_GFX_PICNICKER},
    {FACILITY_CLASS_PKMN_BREEDER_F, OBJ_EVENT_GFX_WOMAN_2},
    {FACILITY_CLASS_PKMN_RANGER_F, OBJ_EVENT_GFX_PICNICKER},
    {FACILITY_CLASS_LASS, OBJ_EVENT_GFX_LASS},
};

// Excludes the unused RS_FACILITY_CLASS_BOARDER_1 and _2
static const u8 sRubyFacilityClassToEmerald[RS_FACILITY_CLASSES_COUNT - 2][2] =
{
    {RS_FACILITY_CLASS_AQUA_LEADER_ARCHIE, FACILITY_CLASS_AQUA_LEADER_ARCHIE},
    {RS_FACILITY_CLASS_AQUA_GRUNT_M, FACILITY_CLASS_AQUA_GRUNT_M},
    {RS_FACILITY_CLASS_AQUA_GRUNT_F, FACILITY_CLASS_AQUA_GRUNT_F},
    {RS_FACILITY_CLASS_AROMA_LADY, FACILITY_CLASS_AROMA_LADY},
    {RS_FACILITY_CLASS_RUIN_MANIAC, FACILITY_CLASS_RUIN_MANIAC},
    {RS_FACILITY_CLASS_INTERVIEWER, FACILITY_CLASS_INTERVIEWER},
    {RS_FACILITY_CLASS_TUBER_F, FACILITY_CLASS_TUBER_F},
    {RS_FACILITY_CLASS_TUBER_M, FACILITY_CLASS_TUBER_M},
    {RS_FACILITY_CLASS_COOLTRAINER_M, FACILITY_CLASS_COOLTRAINER_M},
    {RS_FACILITY_CLASS_COOLTRAINER_F, FACILITY_CLASS_COOLTRAINER_F},
    {RS_FACILITY_CLASS_HEX_MANIAC, FACILITY_CLASS_HEX_MANIAC},
    {RS_FACILITY_CLASS_LADY, FACILITY_CLASS_LADY},
    {RS_FACILITY_CLASS_BEAUTY, FACILITY_CLASS_BEAUTY},
    {RS_FACILITY_CLASS_RICH_BOY, FACILITY_CLASS_RICH_BOY},
    {RS_FACILITY_CLASS_POKEMANIAC, FACILITY_CLASS_POKEMANIAC},
    {RS_FACILITY_CLASS_SWIMMER_M, FACILITY_CLASS_SWIMMER_M},
    {RS_FACILITY_CLASS_BLACK_BELT, FACILITY_CLASS_BLACK_BELT},
    {RS_FACILITY_CLASS_GUITARIST, FACILITY_CLASS_GUITARIST},
    {RS_FACILITY_CLASS_KINDLER, FACILITY_CLASS_KINDLER},
    {RS_FACILITY_CLASS_CAMPER, FACILITY_CLASS_CAMPER},
    {RS_FACILITY_CLASS_BUG_MANIAC, FACILITY_CLASS_BUG_MANIAC},
    {RS_FACILITY_CLASS_PSYCHIC_M, FACILITY_CLASS_PSYCHIC_M},
    {RS_FACILITY_CLASS_PSYCHIC_F, FACILITY_CLASS_PSYCHIC_F},
    {RS_FACILITY_CLASS_GENTLEMAN, FACILITY_CLASS_GENTLEMAN},
    {RS_FACILITY_CLASS_ELITE_FOUR_M, FACILITY_CLASS_ELITE_FOUR_SIDNEY},
    {RS_FACILITY_CLASS_ELITE_FOUR_F, FACILITY_CLASS_ELITE_FOUR_PHOEBE},
    {RS_FACILITY_CLASS_LEADER_F, FACILITY_CLASS_LEADER_ROXANNE},
    {RS_FACILITY_CLASS_LEADER_M, FACILITY_CLASS_LEADER_BRAWLY},
    {RS_FACILITY_CLASS_LEADER_MF, FACILITY_CLASS_LEADER_TATE_AND_LIZA},
    {RS_FACILITY_CLASS_SCHOOL_KID_M, FACILITY_CLASS_SCHOOL_KID_M},
    {RS_FACILITY_CLASS_SCHOOL_KID_F, FACILITY_CLASS_SCHOOL_KID_F},
    {RS_FACILITY_CLASS_SR_AND_JR, FACILITY_CLASS_SR_AND_JR},
    {RS_FACILITY_CLASS_POKEFAN_M, FACILITY_CLASS_POKEFAN_M},
    {RS_FACILITY_CLASS_POKEFAN_F, FACILITY_CLASS_POKEFAN_F},
    {RS_FACILITY_CLASS_EXPERT_M, FACILITY_CLASS_EXPERT_M},
    {RS_FACILITY_CLASS_EXPERT_F, FACILITY_CLASS_EXPERT_F},
    {RS_FACILITY_CLASS_YOUNGSTER, FACILITY_CLASS_YOUNGSTER},
    {RS_FACILITY_CLASS_CHAMPION, FACILITY_CLASS_CHAMPION_WALLACE},
    {RS_FACILITY_CLASS_FISHERMAN, FACILITY_CLASS_FISHERMAN},
    {RS_FACILITY_CLASS_CYCLING_TRIATHLETE_M, FACILITY_CLASS_CYCLING_TRIATHLETE_M},
    {RS_FACILITY_CLASS_CYCLING_TRIATHLETE_F, FACILITY_CLASS_CYCLING_TRIATHLETE_F},
    {RS_FACILITY_CLASS_RUNNING_TRIATHLETE_M, FACILITY_CLASS_RUNNING_TRIATHLETE_M},
    {RS_FACILITY_CLASS_RUNNING_TRIATHLETE_F, FACILITY_CLASS_RUNNING_TRIATHLETE_F},
    {RS_FACILITY_CLASS_SWIMMING_TRIATHLETE_M, FACILITY_CLASS_SWIMMING_TRIATHLETE_M},
    {RS_FACILITY_CLASS_SWIMMING_TRIATHLETE_F, FACILITY_CLASS_SWIMMING_TRIATHLETE_F},
    {RS_FACILITY_CLASS_DRAGON_TAMER, FACILITY_CLASS_DRAGON_TAMER},
    {RS_FACILITY_CLASS_BIRD_KEEPER, FACILITY_CLASS_BIRD_KEEPER},
    {RS_FACILITY_CLASS_NINJA_BOY, FACILITY_CLASS_NINJA_BOY},
    {RS_FACILITY_CLASS_BATTLE_GIRL, FACILITY_CLASS_BATTLE_GIRL},
    {RS_FACILITY_CLASS_PARASOL_LADY, FACILITY_CLASS_PARASOL_LADY},
    {RS_FACILITY_CLASS_SWIMMER_F, FACILITY_CLASS_SWIMMER_F},
    {RS_FACILITY_CLASS_PICNICKER, FACILITY_CLASS_PICNICKER},
    {RS_FACILITY_CLASS_TWINS, FACILITY_CLASS_TWINS},
    {RS_FACILITY_CLASS_SAILOR, FACILITY_CLASS_SAILOR},
    {RS_FACILITY_CLASS_COLLECTOR, FACILITY_CLASS_COLLECTOR},
    {RS_FACILITY_CLASS_WALLY, FACILITY_CLASS_WALLY},
    {RS_FACILITY_CLASS_BRENDAN_1, FACILITY_CLASS_BRENDAN},
    {RS_FACILITY_CLASS_BRENDAN_2, FACILITY_CLASS_BRENDAN_2},
    {RS_FACILITY_CLASS_BRENDAN_3, FACILITY_CLASS_BRENDAN_3},
    {RS_FACILITY_CLASS_MAY_1, FACILITY_CLASS_MAY},
    {RS_FACILITY_CLASS_MAY_2, FACILITY_CLASS_MAY_2},
    {RS_FACILITY_CLASS_MAY_3, FACILITY_CLASS_MAY_3},
    {RS_FACILITY_CLASS_PKMN_BREEDER_M, FACILITY_CLASS_PKMN_BREEDER_M},
    {RS_FACILITY_CLASS_PKMN_BREEDER_F, FACILITY_CLASS_PKMN_BREEDER_F},
    {RS_FACILITY_CLASS_PKMN_RANGER_M, FACILITY_CLASS_PKMN_RANGER_M},
    {RS_FACILITY_CLASS_PKMN_RANGER_F, FACILITY_CLASS_PKMN_RANGER_F},
    {RS_FACILITY_CLASS_MAGMA_LEADER, FACILITY_CLASS_MAGMA_LEADER_MAXIE},
    {RS_FACILITY_CLASS_MAGMA_GRUNT_M, FACILITY_CLASS_MAGMA_GRUNT_M},
    {RS_FACILITY_CLASS_MAGMA_GRUNT_F, FACILITY_CLASS_MAGMA_GRUNT_F},
    {RS_FACILITY_CLASS_LASS, FACILITY_CLASS_LASS},
    {RS_FACILITY_CLASS_BUG_CATCHER, FACILITY_CLASS_BUG_CATCHER},
    {RS_FACILITY_CLASS_HIKER, FACILITY_CLASS_HIKER},
    {RS_FACILITY_CLASS_YOUNG_COUPLE, FACILITY_CLASS_YOUNG_COUPLE},
    {RS_FACILITY_CLASS_OLD_COUPLE, FACILITY_CLASS_OLD_COUPLE},
    {RS_FACILITY_CLASS_SIS_AND_BRO, FACILITY_CLASS_SIS_AND_BRO},
};

#define PARTNER_TEXTS(name)                                             \
    BattleFrontier_BattleTowerMultiPartnerRoom_Text_##name##Intro,      \
    BattleFrontier_BattleTowerMultiPartnerRoom_Text_##name##Mon1,       \
    BattleFrontier_BattleTowerMultiPartnerRoom_Text_##name##Mon2Ask,    \
    BattleFrontier_BattleTowerMultiPartnerRoom_Text_##name##Accept,     \
    BattleFrontier_BattleTowerMultiPartnerRoom_Text_##name##Reject

static const u8 *const sPartnerApprenticeTexts1[] =
{
    PARTNER_TEXTS(Apprentice1)
};

static const u8 *const sPartnerApprenticeTexts2[] =
{
    PARTNER_TEXTS(Apprentice2)
};

static const u8 *const sPartnerApprenticeTexts3[] =
{
    PARTNER_TEXTS(Apprentice3)
};

static const u8 *const sPartnerApprenticeTexts4[] =
{
    PARTNER_TEXTS(Apprentice4)
};

static const u8 *const sPartnerApprenticeTexts5[] =
{
    PARTNER_TEXTS(Apprentice5)
};

static const u8 *const sPartnerApprenticeTexts6[] =
{
     PARTNER_TEXTS(Apprentice6)
};

static const u8 *const sPartnerApprenticeTexts7[] =
{
     PARTNER_TEXTS(Apprentice7)
};

static const u8 *const sPartnerApprenticeTexts8[] =
{
     PARTNER_TEXTS(Apprentice8)
};

static const u8 *const sPartnerApprenticeTexts9[] =
{
     PARTNER_TEXTS(Apprentice9)
};

static const u8 *const sPartnerApprenticeTexts10[] =
{
     PARTNER_TEXTS(Apprentice10)
};

static const u8 *const sPartnerApprenticeTexts11[] =
{
     PARTNER_TEXTS(Apprentice11)
};

static const u8 *const sPartnerApprenticeTexts12[] =
{
     PARTNER_TEXTS(Apprentice12)
};

static const u8 *const sPartnerApprenticeTexts13[] =
{
     PARTNER_TEXTS(Apprentice13)
};

static const u8 *const sPartnerApprenticeTexts14[] =
{
     PARTNER_TEXTS(Apprentice14)
};

static const u8 *const sPartnerApprenticeTexts15[] =
{
     PARTNER_TEXTS(Apprentice15)
};

static const u8 *const sPartnerApprenticeTexts16[] =
{
     PARTNER_TEXTS(Apprentice16)
};

static const u8 *const sPartnerTextsLass[] =
{
    PARTNER_TEXTS(Lass)
};

static const u8 *const sPartnerTextsYoungster[] =
{
    PARTNER_TEXTS(Youngster)
};

static const u8 *const sPartnerTextsHiker[] =
{
    PARTNER_TEXTS(Hiker)
};

static const u8 *const sPartnerTextsBeauty[] =
{
    PARTNER_TEXTS(Beauty)
};

static const u8 *const sPartnerTextsFisherman[] =
{
    PARTNER_TEXTS(Fisherman)
};

static const u8 *const sPartnerTextsLady[] =
{
    PARTNER_TEXTS(Lady)
};

static const u8 *const sPartnerTextsCyclingTriathleteF[] =
{
    PARTNER_TEXTS(CyclingTriathleteF)
};

static const u8 *const sPartnerTextsBugCatcher[] =
{
    PARTNER_TEXTS(BugCatcher)
};

static const u8 *const sPartnerTextsSchoolKidM[] =
{
    PARTNER_TEXTS(SchoolKidM)
};

static const u8 *const sPartnerTextsRichBoy[] =
{
    PARTNER_TEXTS(RichBoy)
};

static const u8 *const sPartnerTextsBlackBelt[] =
{
    PARTNER_TEXTS(BlackBelt)
};

static const u8 *const sPartnerTextsTuberF[] =
{
    PARTNER_TEXTS(TuberF)
};

static const u8 *const sPartnerTextsHexManiac[] =
{
    PARTNER_TEXTS(HexManiac)
};

static const u8 *const sPartnerTextsPkmnBreederM[] =
{
    PARTNER_TEXTS(PkmnBreederM)
};

static const u8 *const sPartnerTextsRunningTriathleteF[] =
{
    PARTNER_TEXTS(RunningTriathleteF)
};

static const u8 *const sPartnerTextsRunningTriathleteM[] =
{
    PARTNER_TEXTS(RunningTriathleteM)
};

static const u8 *const sPartnerTextsBattleGirl[] =
{
    PARTNER_TEXTS(BattleGirl)
};

static const u8 *const sPartnerTextsCyclingTriathleteM[] =
{
    PARTNER_TEXTS(CyclingTriathleteM)
};

static const u8 *const sPartnerTextsTuberM[] =
{
    PARTNER_TEXTS(TuberM)
};

static const u8 *const sPartnerTextsGuitarist[] =
{
    PARTNER_TEXTS(Guitarist)
};

static const u8 *const sPartnerTextsGentleman[] =
{
    PARTNER_TEXTS(Gentleman)
};

static const u8 *const sPartnerTextsPokefanM[] =
{
    PARTNER_TEXTS(PokefanM)
};

static const u8 *const sPartnerTextsExpertM[] =
{
    PARTNER_TEXTS(ExpertM)
};

static const u8 *const sPartnerTextsExpertF[] =
{
    PARTNER_TEXTS(ExpertF)
};

static const u8 *const sPartnerTextsDragonTamer[] =
{
    PARTNER_TEXTS(DragonTamer)
};

static const u8 *const sPartnerTextsBirdKeeper[] =
{
    PARTNER_TEXTS(BirdKeeper)
};

static const u8 *const sPartnerTextsNinjaBoy[] =
{
    PARTNER_TEXTS(NinjaBoy)
};

static const u8 *const sPartnerTextsParasolLady[] =
{
    PARTNER_TEXTS(ParasolLady)
};

static const u8 *const sPartnerTextsBugManiac[] =
{
    PARTNER_TEXTS(BugManiac)
};

static const u8 *const sPartnerTextsSailor[] =
{
    PARTNER_TEXTS(Sailor)
};

static const u8 *const sPartnerTextsCollector[] =
{
    PARTNER_TEXTS(Collector)
};

static const u8 *const sPartnerTextsPkmnRangerM[] =
{
    PARTNER_TEXTS(PkmnRangerM)
};

static const u8 *const sPartnerTextsPkmnRangerF[] =
{
    PARTNER_TEXTS(PkmnRangerF)
};

static const u8 *const sPartnerTextsAromaLady[] =
{
    PARTNER_TEXTS(AromaLady)
};

static const u8 *const sPartnerTextsRuinManiac[] =
{
    PARTNER_TEXTS(RuinManiac)
};

static const u8 *const sPartnerTextsCoolTrainerM[] =
{
    PARTNER_TEXTS(CoolTrainerM)
};

static const u8 *const sPartnerTextsCoolTrainerF[] =
{
    PARTNER_TEXTS(CoolTrainerF)
};

static const u8 *const sPartnerTextsPokemaniac[] =
{
    PARTNER_TEXTS(Pokemaniac)
};

static const u8 *const sPartnerTextsKindler[] =
{
    PARTNER_TEXTS(Kindler)
};

static const u8 *const sPartnerTextsCamper[] =
{
    PARTNER_TEXTS(Camper)
};

static const u8 *const sPartnerTextsPicnicker[] =
{
    PARTNER_TEXTS(Picnicker)
};

static const u8 *const sPartnerTextsPsychicM[] =
{
    PARTNER_TEXTS(PsychicM)
};

static const u8 *const sPartnerTextsPsychicF[] =
{
    PARTNER_TEXTS(PsychicF)
};

static const u8 *const sPartnerTextsSchoolKidF[] =
{
    PARTNER_TEXTS(SchoolKidF)
};

static const u8 *const sPartnerTextsPkmnBreederF[] =
{
    PARTNER_TEXTS(PkmnBreederF)
};

static const u8 *const sPartnerTextsPokefanF[] =
{
    PARTNER_TEXTS(PokefanF)
};

static const u8 *const sPartnerTextsSwimmerF[] =
{
    PARTNER_TEXTS(SwimmerF)
};

static const u8 *const sPartnerTextsSwimmingTriathleteM[] =
{
    PARTNER_TEXTS(SwimmingTriathleteM)
};

static const u8 *const sPartnerTextsSwimmingTriathleteF[] =
{
    PARTNER_TEXTS(SwimmingTriathleteF)
};

static const u8 *const sPartnerTextsSwimmerM[] =
{
    PARTNER_TEXTS(SwimmerM)
};

struct
{
    u32 facilityClass;
    const u8 *const *strings;
    u32 stringCount;
} static const sPartnerTrainerTextTables[] =
{
    {FACILITY_CLASS_LASS,                  sPartnerTextsLass, ARRAY_COUNT(sPartnerTextsLass)},
    {FACILITY_CLASS_YOUNGSTER,             sPartnerTextsYoungster, ARRAY_COUNT(sPartnerTextsYoungster)},
    {FACILITY_CLASS_HIKER,                 sPartnerTextsHiker, ARRAY_COUNT(sPartnerTextsHiker)},
    {FACILITY_CLASS_BEAUTY,                sPartnerTextsBeauty, ARRAY_COUNT(sPartnerTextsBeauty)},
    {FACILITY_CLASS_FISHERMAN,             sPartnerTextsFisherman, ARRAY_COUNT(sPartnerTextsFisherman)},
    {FACILITY_CLASS_LADY,                  sPartnerTextsLady, ARRAY_COUNT(sPartnerTextsLady)},
    {FACILITY_CLASS_CYCLING_TRIATHLETE_F,  sPartnerTextsCyclingTriathleteF, ARRAY_COUNT(sPartnerTextsCyclingTriathleteF)},
    {FACILITY_CLASS_BUG_CATCHER,           sPartnerTextsBugCatcher, ARRAY_COUNT(sPartnerTextsBugCatcher)},
    {FACILITY_CLASS_SCHOOL_KID_M,          sPartnerTextsSchoolKidM, ARRAY_COUNT(sPartnerTextsSchoolKidM)},
    {FACILITY_CLASS_RICH_BOY,              sPartnerTextsRichBoy, ARRAY_COUNT(sPartnerTextsRichBoy)},
    {FACILITY_CLASS_BLACK_BELT,            sPartnerTextsBlackBelt, ARRAY_COUNT(sPartnerTextsBlackBelt)},
    {FACILITY_CLASS_TUBER_F,               sPartnerTextsTuberF, ARRAY_COUNT(sPartnerTextsTuberF)},
    {FACILITY_CLASS_HEX_MANIAC,            sPartnerTextsHexManiac, ARRAY_COUNT(sPartnerTextsHexManiac)},
    {FACILITY_CLASS_PKMN_BREEDER_M,        sPartnerTextsPkmnBreederM, ARRAY_COUNT(sPartnerTextsPkmnBreederM)},
    {FACILITY_CLASS_RUNNING_TRIATHLETE_F,  sPartnerTextsRunningTriathleteF, ARRAY_COUNT(sPartnerTextsRunningTriathleteF)},
    {FACILITY_CLASS_RUNNING_TRIATHLETE_M,  sPartnerTextsRunningTriathleteM, ARRAY_COUNT(sPartnerTextsRunningTriathleteM)},
    {FACILITY_CLASS_BATTLE_GIRL,           sPartnerTextsBattleGirl, ARRAY_COUNT(sPartnerTextsBattleGirl)},
    {FACILITY_CLASS_CYCLING_TRIATHLETE_M,  sPartnerTextsCyclingTriathleteM, ARRAY_COUNT(sPartnerTextsCyclingTriathleteM)},
    {FACILITY_CLASS_TUBER_M,               sPartnerTextsTuberM, ARRAY_COUNT(sPartnerTextsTuberM)},
    {FACILITY_CLASS_GUITARIST,             sPartnerTextsGuitarist, ARRAY_COUNT(sPartnerTextsGuitarist)},
    {FACILITY_CLASS_GENTLEMAN,             sPartnerTextsGentleman, ARRAY_COUNT(sPartnerTextsGentleman)},
    {FACILITY_CLASS_POKEFAN_M,             sPartnerTextsPokefanM, ARRAY_COUNT(sPartnerTextsPokefanM)},
    {FACILITY_CLASS_EXPERT_M,              sPartnerTextsExpertM, ARRAY_COUNT(sPartnerTextsExpertM)},
    {FACILITY_CLASS_EXPERT_F,              sPartnerTextsExpertF, ARRAY_COUNT(sPartnerTextsExpertF)},
    {FACILITY_CLASS_DRAGON_TAMER,          sPartnerTextsDragonTamer, ARRAY_COUNT(sPartnerTextsDragonTamer)},
    {FACILITY_CLASS_BIRD_KEEPER,           sPartnerTextsBirdKeeper, ARRAY_COUNT(sPartnerTextsBirdKeeper)},
    {FACILITY_CLASS_NINJA_BOY,             sPartnerTextsNinjaBoy, ARRAY_COUNT(sPartnerTextsNinjaBoy)},
    {FACILITY_CLASS_PARASOL_LADY,          sPartnerTextsParasolLady, ARRAY_COUNT(sPartnerTextsParasolLady)},
    {FACILITY_CLASS_BUG_MANIAC,            sPartnerTextsBugManiac, ARRAY_COUNT(sPartnerTextsBugManiac)},
    {FACILITY_CLASS_SAILOR,                sPartnerTextsSailor, ARRAY_COUNT(sPartnerTextsSailor)},
    {FACILITY_CLASS_COLLECTOR,             sPartnerTextsCollector, ARRAY_COUNT(sPartnerTextsCollector)},
    {FACILITY_CLASS_PKMN_RANGER_M,         sPartnerTextsPkmnRangerM, ARRAY_COUNT(sPartnerTextsPkmnRangerM)},
    {FACILITY_CLASS_PKMN_RANGER_F,         sPartnerTextsPkmnRangerF, ARRAY_COUNT(sPartnerTextsPkmnRangerF)},
    {FACILITY_CLASS_AROMA_LADY,            sPartnerTextsAromaLady, ARRAY_COUNT(sPartnerTextsAromaLady)},
    {FACILITY_CLASS_RUIN_MANIAC,           sPartnerTextsRuinManiac, ARRAY_COUNT(sPartnerTextsRuinManiac)},
    {FACILITY_CLASS_COOLTRAINER_M,         sPartnerTextsCoolTrainerM, ARRAY_COUNT(sPartnerTextsCoolTrainerM)},
    {FACILITY_CLASS_COOLTRAINER_F,         sPartnerTextsCoolTrainerF, ARRAY_COUNT(sPartnerTextsCoolTrainerF)},
    {FACILITY_CLASS_POKEMANIAC,            sPartnerTextsPokemaniac, ARRAY_COUNT(sPartnerTextsPokemaniac)},
    {FACILITY_CLASS_KINDLER,               sPartnerTextsKindler, ARRAY_COUNT(sPartnerTextsKindler)},
    {FACILITY_CLASS_CAMPER,                sPartnerTextsCamper, ARRAY_COUNT(sPartnerTextsCamper)},
    {FACILITY_CLASS_PICNICKER,             sPartnerTextsPicnicker, ARRAY_COUNT(sPartnerTextsPicnicker)},
    {FACILITY_CLASS_PSYCHIC_M,             sPartnerTextsPsychicM, ARRAY_COUNT(sPartnerTextsPsychicM)},
    {FACILITY_CLASS_PSYCHIC_F,             sPartnerTextsPsychicF, ARRAY_COUNT(sPartnerTextsPsychicF)},
    {FACILITY_CLASS_SCHOOL_KID_F,          sPartnerTextsSchoolKidF, ARRAY_COUNT(sPartnerTextsSchoolKidF)},
    {FACILITY_CLASS_PKMN_BREEDER_F,        sPartnerTextsPkmnBreederF, ARRAY_COUNT(sPartnerTextsPkmnBreederF)},
    {FACILITY_CLASS_POKEFAN_F,             sPartnerTextsPokefanF, ARRAY_COUNT(sPartnerTextsPokefanF)},
    {FACILITY_CLASS_SWIMMER_F,             sPartnerTextsSwimmerF, ARRAY_COUNT(sPartnerTextsSwimmerF)},
    {FACILITY_CLASS_SWIMMING_TRIATHLETE_M, sPartnerTextsSwimmingTriathleteM, ARRAY_COUNT(sPartnerTextsSwimmingTriathleteM)},
    {FACILITY_CLASS_SWIMMING_TRIATHLETE_F, sPartnerTextsSwimmingTriathleteF, ARRAY_COUNT(sPartnerTextsSwimmingTriathleteF)},
    {FACILITY_CLASS_SWIMMER_M,             sPartnerTextsSwimmerM, ARRAY_COUNT(sPartnerTextsSwimmerM)}
};

static const u8 *const *const sPartnerApprenticeTextTables[NUM_APPRENTICES] =
{
    sPartnerApprenticeTexts1,
    sPartnerApprenticeTexts2,
    sPartnerApprenticeTexts3,
    sPartnerApprenticeTexts4,
    sPartnerApprenticeTexts5,
    sPartnerApprenticeTexts6,
    sPartnerApprenticeTexts7,
    sPartnerApprenticeTexts8,
    sPartnerApprenticeTexts9,
    sPartnerApprenticeTexts10,
    sPartnerApprenticeTexts11,
    sPartnerApprenticeTexts12,
    sPartnerApprenticeTexts13,
    sPartnerApprenticeTexts14,
    sPartnerApprenticeTexts15,
    sPartnerApprenticeTexts16
};
static const u32 sADMG_PartnerApprenticeTextCounts[] =
{
    ARRAY_COUNT(sPartnerApprenticeTexts1),
    ARRAY_COUNT(sPartnerApprenticeTexts2),
    ARRAY_COUNT(sPartnerApprenticeTexts3),
    ARRAY_COUNT(sPartnerApprenticeTexts4),
    ARRAY_COUNT(sPartnerApprenticeTexts5),
    ARRAY_COUNT(sPartnerApprenticeTexts6),
    ARRAY_COUNT(sPartnerApprenticeTexts7),
    ARRAY_COUNT(sPartnerApprenticeTexts8),
    ARRAY_COUNT(sPartnerApprenticeTexts9),
    ARRAY_COUNT(sPartnerApprenticeTexts10),
    ARRAY_COUNT(sPartnerApprenticeTexts11),
    ARRAY_COUNT(sPartnerApprenticeTexts12),
    ARRAY_COUNT(sPartnerApprenticeTexts13),
    ARRAY_COUNT(sPartnerApprenticeTexts14),
    ARRAY_COUNT(sPartnerApprenticeTexts15),
    ARRAY_COUNT(sPartnerApprenticeTexts16),
};


#include "data/battle_frontier/battle_tent.h"
#include "data/admg_facility_mon_extents.h"

static void (* const sBattleTowerFuncs[])(void) =
{
    [BATTLE_TOWER_FUNC_INIT]                = InitTowerChallenge,
    [BATTLE_TOWER_FUNC_GET_DATA]            = GetTowerData,
    [BATTLE_TOWER_FUNC_SET_DATA]            = SetTowerData,
    [BATTLE_TOWER_FUNC_SET_OPPONENT]        = SetNextTowerOpponent,
    [BATTLE_TOWER_FUNC_SET_BATTLE_WON]      = SetTowerBattleWon,
    [BATTLE_TOWER_FUNC_GIVE_RIBBONS]        = AwardBattleTowerRibbons,
    [BATTLE_TOWER_FUNC_SAVE]                = SaveTowerChallenge,
    [BATTLE_TOWER_FUNC_GET_OPPONENT_INTRO]  = GetOpponentIntroSpeech,
    [BATTLE_TOWER_FUNC_NOP]                 = BattleTowerNop1,
    [BATTLE_TOWER_FUNC_NOP2]                = BattleTowerNop2,
    [BATTLE_TOWER_FUNC_LOAD_PARTNERS]       = LoadMultiPartnerCandidatesData,
    [BATTLE_TOWER_FUNC_PARTNER_MSG]         = ShowPartnerCandidateMessage,
    [BATTLE_TOWER_FUNC_LOAD_LINK_OPPONENTS] = LoadLinkMultiOpponentsData,
    [BATTLE_TOWER_FUNC_TRY_CLOSE_LINK]      = TowerTryCloseLink,
    [BATTLE_TOWER_FUNC_SET_PARTNER_GFX]     = SetMultiPartnerGfx,
    [BATTLE_TOWER_FUNC_SET_INTERVIEW_DATA]  = SetTowerInterviewData,
};

static const u32 sWinStreakFlags[][2] =
{
    {STREAK_TOWER_SINGLES_50,     STREAK_TOWER_SINGLES_OPEN},
    {STREAK_TOWER_DOUBLES_50,     STREAK_TOWER_DOUBLES_OPEN},
    {STREAK_TOWER_MULTIS_50,      STREAK_TOWER_MULTIS_OPEN},
    {STREAK_TOWER_LINK_MULTIS_50, STREAK_TOWER_LINK_MULTIS_OPEN},
};

static const u32 sWinStreakMasks[][2] =
{
    {~(STREAK_TOWER_SINGLES_50),     ~(STREAK_TOWER_SINGLES_OPEN)},
    {~(STREAK_TOWER_DOUBLES_50),     ~(STREAK_TOWER_DOUBLES_OPEN)},
    {~(STREAK_TOWER_MULTIS_50),      ~(STREAK_TOWER_MULTIS_OPEN)},
    {~(STREAK_TOWER_LINK_MULTIS_50), ~(STREAK_TOWER_LINK_MULTIS_OPEN)},
};

// The challenge number at which an Apprentice can appear, depending on how many of their questions were answered
static const u8 sApprenticeChallengeThreshold[MAX_APPRENTICE_QUESTIONS] =
{
    1, 2, 3, 4, 5, 8, 9, 10, 11, 12
};

// Unclear why this was duplicated
static const u8 sBattleTowerPartySizes2[] =
{
    [FRONTIER_MODE_SINGLES]     = FRONTIER_PARTY_SIZE,
    [FRONTIER_MODE_DOUBLES]     = FRONTIER_DOUBLES_PARTY_SIZE,
    [FRONTIER_MODE_MULTIS]      = FRONTIER_MULTI_PARTY_SIZE,
    [FRONTIER_MODE_LINK_MULTIS] = FRONTIER_MULTI_PARTY_SIZE,
};

// Unknown, unused data
static const u16 sUnused[] = { 179, 141, 200, 183 };

static const u8 sBattleTowerPartySizes[FRONTIER_MODE_COUNT] =
{
    [FRONTIER_MODE_SINGLES]     = FRONTIER_PARTY_SIZE,
    [FRONTIER_MODE_DOUBLES]     = FRONTIER_DOUBLES_PARTY_SIZE,
    [FRONTIER_MODE_MULTIS]      = FRONTIER_MULTI_PARTY_SIZE,
    [FRONTIER_MODE_LINK_MULTIS] = FRONTIER_MULTI_PARTY_SIZE,
};

static const u16 sRecordTrainerSpeechWon[] =
{
    EC_WORD_YAY, EC_WORD_YAY, EC_WORD_EXCL_EXCL, EC_WORD_I_VE, EC_WORD_WON, EC_WORD_EXCL_EXCL
};

static const u16 sRecordTrainerSpeechLost[] =
{
    EC_WORD_TOO, EC_WORD_BAD, EC_WORD_ELLIPSIS, EC_WORD_WE, EC_WORD_LOST, EC_WORD_ELLIPSIS
};

// code
void CallBattleTowerFunc(void)
{
    sBattleTowerFuncs[gSpecialVar_0x8004]();
}

static void InitTowerChallenge(void)
{
    enum FrontierLevelMode lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    gSaveBlock2Ptr->frontier.challengeStatus = CHALLENGE_STATUS_SAVING;
    gSaveBlock2Ptr->frontier.curChallengeBattleNum = 0;
    gSaveBlock2Ptr->frontier.challengePaused = FALSE;
    gSaveBlock2Ptr->frontier.disableRecordBattle = FALSE;
    ResetFrontierTrainerIds();
    if (!(gSaveBlock2Ptr->frontier.winStreakActiveFlags & sWinStreakFlags[battleMode][lvlMode]))
        gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] = 0;

    ValidateBattleTowerRecordChecksums();
    SetDynamicWarp(0, gSaveBlock1Ptr->location.mapId, WARP_ID_NONE);
    TRAINER_BATTLE_PARAM.opponentA = 0;
}

static void GetTowerData(void)
{
    enum FrontierLevelMode lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    switch (gSpecialVar_0x8005)
    {
    case 0:
        break;
    case TOWER_DATA_WIN_STREAK:
        gSpecialVar_Result = GetCurrentBattleTowerWinStreak(lvlMode, battleMode);
        break;
    case TOWER_DATA_WIN_STREAK_ACTIVE:
        gSpecialVar_Result = ((gSaveBlock2Ptr->frontier.winStreakActiveFlags & sWinStreakFlags[battleMode][lvlMode]) != 0);
        break;
    case TOWER_DATA_LVL_MODE:
        gSaveBlock2Ptr->frontier.towerLvlMode = gSaveBlock2Ptr->frontier.lvlMode;
        break;
    }
}

static void SetTowerData(void)
{
    enum FrontierLevelMode lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    switch (gSpecialVar_0x8005)
    {
    case 0:
        break;
    case TOWER_DATA_WIN_STREAK:
        gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] = gSpecialVar_0x8006;
        break;
    case TOWER_DATA_WIN_STREAK_ACTIVE:
        if (gSpecialVar_0x8006)
            gSaveBlock2Ptr->frontier.winStreakActiveFlags |= sWinStreakFlags[battleMode][lvlMode];
        else
            gSaveBlock2Ptr->frontier.winStreakActiveFlags &= sWinStreakMasks[battleMode][lvlMode];
        break;
    case TOWER_DATA_LVL_MODE:
        gSaveBlock2Ptr->frontier.towerLvlMode = gSaveBlock2Ptr->frontier.lvlMode;
        break;
    }
}

static void SetTowerBattleWon(void)
{
#if FREE_BATTLE_TOWER_E_READER == FALSE
    if (TRAINER_BATTLE_PARAM.opponentA == TRAINER_EREADER)
        ClearEReaderTrainer(&gSaveBlock2Ptr->frontier.ereaderTrainer);
#endif //FREE_BATTLE_TOWER_E_READER

    // towerNumWins is never read outside this conditional
    if (gSaveBlock2Ptr->frontier.towerNumWins < MAX_STREAK)
        gSaveBlock2Ptr->frontier.towerNumWins++;

    gSaveBlock2Ptr->frontier.curChallengeBattleNum++;
    SaveCurrentWinStreak();
    gSpecialVar_Result = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
}

static bool8 ChooseSpecialBattleTowerTrainer(void)
{
    s32 i, j, validMons;
    s32 trainerIds[9];
    s32 idsCount = 0;
    s32 winStreak = 0;
    enum FrontierLevelMode lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u8 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);

    if (VarGet(VAR_FRONTIER_FACILITY) != FRONTIER_FACILITY_TOWER)
        return FALSE;

    winStreak = GetCurrentBattleTowerWinStreak(lvlMode, battleMode);
    for (i = 0; i < BATTLE_TOWER_RECORD_COUNT; i++)
    {
        u32 *record = (u32 *)(&gSaveBlock2Ptr->frontier.towerRecords[i]);
        u32 recordHasData = 0;
        u32 checksum = 0;
        for (j = 0; j < (sizeof(struct EmeraldBattleTowerRecord) - 4) / 4; j++) // - 4, because of the last field being the checksum itself.
        {
            recordHasData |= record[j];
            checksum += record[j];
        }
        validMons = 0;
        for (j = 0; j < MAX_FRONTIER_PARTY_SIZE; j++)
        {
            if (gSaveBlock2Ptr->frontier.towerRecords[i].party[j].species != SPECIES_NONE
                && gSaveBlock2Ptr->frontier.towerRecords[i].party[j].level <= GetFrontierEnemyMonLevel(lvlMode))
                validMons++;
        }

        if (validMons >= sBattleTowerPartySizes2[battleMode]
            && gSaveBlock2Ptr->frontier.towerRecords[i].winStreak == winStreak
            && gSaveBlock2Ptr->frontier.towerRecords[i].lvlMode == lvlMode
            && recordHasData
            && gSaveBlock2Ptr->frontier.towerRecords[i].checksum == checksum)
        {
            trainerIds[idsCount] = i + TRAINER_RECORD_MIXING_FRIEND;
            idsCount++;
        }
    }

    if (battleMode == FRONTIER_MODE_SINGLES)
    {
        ValidateApprenticesChecksums();
        for (i = 0; i < APPRENTICE_COUNT; i++)
        {
            if (gSaveBlock2Ptr->apprentices[i].lvlMode != 0
                && sApprenticeChallengeThreshold[gSaveBlock2Ptr->apprentices[i].numQuestions] == winStreak
                && gSaveBlock2Ptr->apprentices[i].lvlMode - 1 == lvlMode)
            {
                trainerIds[idsCount] = i + TRAINER_RECORD_MIXING_APPRENTICE;
                idsCount++;
            }
        }
    }

    if (idsCount != 0)
    {
        TRAINER_BATTLE_PARAM.opponentA = trainerIds[Random() % idsCount];
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static void SetNextTowerOpponent(void)
{
    enum FrontierLevelMode lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    if (lvlMode == FRONTIER_LVL_TENT)
    {
        SetNextBattleTentOpponent();
    }
    else
    {
        u16 id;
        u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
        u16 winStreak = GetCurrentFacilityWinStreak();
        u32 challengeNum = winStreak / FRONTIER_STAGES_PER_CHALLENGE;
        SetFacilityPtrsGetLevel();

        if (battleMode == FRONTIER_MODE_MULTIS || battleMode == FRONTIER_MODE_LINK_MULTIS)
        {
            id = gSaveBlock2Ptr->frontier.curChallengeBattleNum;
            TRAINER_BATTLE_PARAM.opponentA = gSaveBlock2Ptr->frontier.trainerIds[id * 2];
            TRAINER_BATTLE_PARAM.opponentB = gSaveBlock2Ptr->frontier.trainerIds[id * 2 + 1];
            SetBattleFacilityTrainerGfxId(TRAINER_BATTLE_PARAM.opponentA, 0);
            SetBattleFacilityTrainerGfxId(TRAINER_BATTLE_PARAM.opponentB, 1);
        }
        else if (ChooseSpecialBattleTowerTrainer())
        {
            SetBattleFacilityTrainerGfxId(TRAINER_BATTLE_PARAM.opponentA, 0);
            gSaveBlock2Ptr->frontier.trainerIds[gSaveBlock2Ptr->frontier.curChallengeBattleNum] = TRAINER_BATTLE_PARAM.opponentA;
        }
        else
        {
            s32 i;
            while (1)
            {
                id = GetRandomScaledFrontierTrainerId(challengeNum, gSaveBlock2Ptr->frontier.curChallengeBattleNum);

                // Ensure trainer wasn't previously fought in this challenge.
                for (i = 0; i < gSaveBlock2Ptr->frontier.curChallengeBattleNum; i++)
                {
                    if (gSaveBlock2Ptr->frontier.trainerIds[i] == id)
                        break;
                }
                if (i == gSaveBlock2Ptr->frontier.curChallengeBattleNum)
                    break;
            }

            TRAINER_BATTLE_PARAM.opponentA = id;
            SetBattleFacilityTrainerGfxId(TRAINER_BATTLE_PARAM.opponentA, 0);
            if (gSaveBlock2Ptr->frontier.curChallengeBattleNum + 1 < FRONTIER_STAGES_PER_CHALLENGE)
                gSaveBlock2Ptr->frontier.trainerIds[gSaveBlock2Ptr->frontier.curChallengeBattleNum] = TRAINER_BATTLE_PARAM.opponentA;
        }
    }
}



void PutNewBattleTowerRecord(struct EmeraldBattleTowerRecord *newRecordEm)
{
    u16 slotValues[6];
    u16 slotIds[6];
    s32 i, j, k;
    s32 slotsCount = 0;
    struct EmeraldBattleTowerRecord *newRecord = newRecordEm; // Needed to match.

    // Find a record slot of the same player and replace it.
    for (i = 0; i < BATTLE_TOWER_RECORD_COUNT; i++)
    {
        k = 0;
        for (j = 0; j < TRAINER_ID_LENGTH; j++)
        {
            if (gSaveBlock2Ptr->frontier.towerRecords[i].trainerId[j] != newRecord->trainerId[j])
                break;
        }
        if (j == TRAINER_ID_LENGTH)
        {
            for (k = 0; k < PLAYER_NAME_LENGTH; k++)
            {
            // Incorrect index being used
            #ifdef BUGFIX
                #define INDEX k
            #else
                #define INDEX j
            #endif
                if (gSaveBlock2Ptr->frontier.towerRecords[i].name[INDEX] != newRecord->name[INDEX])
                    break;
                if (newRecord->name[INDEX] == EOS)
                {
                    k = PLAYER_NAME_LENGTH;
                    break;
                }
            }
        }

        if (k == PLAYER_NAME_LENGTH)
            break;
    }
    if (i < BATTLE_TOWER_RECORD_COUNT)
    {
        gSaveBlock2Ptr->frontier.towerRecords[i] = *newRecord;
        return;
    }

    // Find an empty record slot.
    for (i = 0; i < BATTLE_TOWER_RECORD_COUNT; i++)
    {
        if (gSaveBlock2Ptr->frontier.towerRecords[i].winStreak == 0)
            break;
    }
    if (i < BATTLE_TOWER_RECORD_COUNT)
    {
        gSaveBlock2Ptr->frontier.towerRecords[i] = *newRecord;
        return;
    }

    // Find possible slots to replace the record.
    slotValues[0] = gSaveBlock2Ptr->frontier.towerRecords[0].winStreak;
    slotIds[0] = 0;
    slotsCount++;

    for (i = 1; i < BATTLE_TOWER_RECORD_COUNT; i++)
    {
        for (j = 0; j < slotsCount; j++)
        {
            if (gSaveBlock2Ptr->frontier.towerRecords[i].winStreak < slotValues[j])
            {
                j = 0;
                slotsCount = 1;
                slotValues[0] = gSaveBlock2Ptr->frontier.towerRecords[i].winStreak;
                slotIds[0] = i;
                break;
            }
            else if (gSaveBlock2Ptr->frontier.towerRecords[i].winStreak > slotValues[j])
            {
                break;
            }
        }

        if (j == slotsCount)
        {
            slotValues[slotsCount] = gSaveBlock2Ptr->frontier.towerRecords[i].winStreak;
            slotIds[slotsCount] = i;
            slotsCount++;
        }
    }

    i = Random() % slotsCount;
    gSaveBlock2Ptr->frontier.towerRecords[slotIds[i]] = *newRecord;
}

void FillTentTrainerParty(u8 monsCount)
{
    enum TrainerId identity = TRAINER_BATTLE_PARAM.opponentA;
    struct ADMG_FacilityPartyInput admitted;
    if (!ADMG_TryPrepareFacilityParty(identity, B_TRAINER_OPPONENT_A, monsCount, TRUE, 0, &admitted)) return;
    ZeroEnemyPartyMons();
    const struct ADMG_FacilityPartyInput *previous = ADMG_BorrowFacilityParty(&admitted);
    FillTentTrainerParty_(identity, 0, monsCount);
    ADMG_BorrowFacilityParty(previous);
}

static void GetOpponentIntroSpeech(void)
{
    enum TrainerId trainerId;

    if (gSpecialVar_0x8005)
        trainerId = TRAINER_BATTLE_PARAM.opponentB;
    else
        trainerId = TRAINER_BATTLE_PARAM.opponentA;

    if (gSaveBlock2Ptr == NULL || trainerId > TRAINER_FRONTIER_BRAIN)
    { ADMG_StateAccessRejected(trainerId, __func__); return; }
    const struct BattleFrontierTrainer *trainer = NULL;
    if (trainerId < FRONTIER_TRAINERS_COUNT && !ADMG_TryGetFacilityTrainerRow(trainerId, &trainer)) return;
    if (trainerId >= TRAINER_RECORD_MIXING_APPRENTICE && trainerId < TRAINER_EREADER
     && gSaveBlock2Ptr->apprentices[trainerId - TRAINER_RECORD_MIXING_APPRENTICE].id >= NUM_APPRENTICES)
    { ADMG_StateAccessRejected(trainerId, __func__); return; }
#if FREE_BATTLE_TOWER_E_READER == TRUE
    if (trainerId == TRAINER_EREADER) { ADMG_StateAccessRejected(trainerId, __func__); return; }
#endif
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    u64 generation = ADMG_BeginStateAccessCheck();
    SetFacilityPtrsGetLevel();
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (gSaveBlock2Ptr != save) { ADMG_StateAccessRejected(trainerId, __func__); return; }
    if (trainerId == TRAINER_FRONTIER_BRAIN)
    { CopyFrontierTrainerText(FRONTIER_BEFORE_TEXT, trainerId); return; }

#if FREE_BATTLE_TOWER_E_READER == FALSE
    if (trainerId == TRAINER_EREADER)
        FrontierSpeechToString(gSaveBlock2Ptr->frontier.ereaderTrainer.greeting);
    else if (trainerId < FRONTIER_TRAINERS_COUNT)
#else
    if (trainerId < FRONTIER_TRAINERS_COUNT)
#endif //FREE_BATTLE_TOWER_E_READER
        FrontierSpeechToString(trainer->speechBefore);
    else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
        FrontierSpeechToString(gSaveBlock2Ptr->frontier.towerRecords[trainerId - TRAINER_RECORD_MIXING_FRIEND].greeting);
    else
        BufferApprenticeChallengeText(trainerId - TRAINER_RECORD_MIXING_APPRENTICE);
}

static void SaveCurrentWinStreak(void)
{
    enum FrontierLevelMode lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u8 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    u16 winStreak = GetCurrentBattleTowerWinStreak(lvlMode, battleMode);

    if (gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] < winStreak)
        gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] = winStreak;
}

static void SaveBattleTowerRecord(void)
{
    s32 i;
    enum FrontierLevelMode lvlMode;
    u8 battleMode, class;
    struct EmeraldBattleTowerRecord *playerRecord = &gSaveBlock2Ptr->frontier.towerPlayer;

    ClearBattleTowerRecord(playerRecord);
    lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    if (gSaveBlock2Ptr->playerGender != MALE)
    {
        class = gTowerFemaleFacilityClasses[(gSaveBlock2Ptr->playerTrainerId[0] +
                                       gSaveBlock2Ptr->playerTrainerId[1] +
                                       gSaveBlock2Ptr->playerTrainerId[2] +
                                       gSaveBlock2Ptr->playerTrainerId[3]) % ARRAY_COUNT(gTowerFemaleFacilityClasses)].class;
    }
    else
    {
        class = gTowerMaleFacilityClasses[(gSaveBlock2Ptr->playerTrainerId[0] +
                                     gSaveBlock2Ptr->playerTrainerId[1] +
                                     gSaveBlock2Ptr->playerTrainerId[2] +
                                     gSaveBlock2Ptr->playerTrainerId[3]) % ARRAY_COUNT(gTowerMaleFacilityClasses)].class;
    }
    playerRecord->lvlMode = lvlMode;
    playerRecord->facilityClass = class;
    CopyTrainerId(playerRecord->trainerId, gSaveBlock2Ptr->playerTrainerId);
    StringCopy_PlayerName(playerRecord->name, gSaveBlock2Ptr->playerName);
    playerRecord->winStreak = GetCurrentBattleTowerWinStreak(lvlMode, battleMode);

    for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
    {
        playerRecord->greeting[i] = gSaveBlock1Ptr->easyChatBattleStart[i];
        playerRecord->speechWon[i] = gSaveBlock1Ptr->easyChatBattleWon[i];
        playerRecord->speechLost[i] = gSaveBlock1Ptr->easyChatBattleLost[i];
    }

    for (i = 0; i < MAX_FRONTIER_PARTY_SIZE; i++)
    {
        if (gSaveBlock2Ptr->frontier.selectedPartyMons[i] != 0)
            ConvertPokemonToBattleTowerPokemon(&gParties[B_TRAINER_PLAYER][gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1], &playerRecord->party[i]);
    }

    playerRecord->language = gGameLanguage;
    CalcEmeraldBattleTowerChecksum(&gSaveBlock2Ptr->frontier.towerPlayer);
    SaveCurrentWinStreak();
}

static void SaveTowerChallenge(void)
{
    enum FrontierLevelMode lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u16 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    s32 challengeNum = (signed)(gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode] / FRONTIER_STAGES_PER_CHALLENGE);

    if (gSpecialVar_0x8005 == 0 && (challengeNum > 1 || gSaveBlock2Ptr->frontier.curChallengeBattleNum != 0))
        SaveBattleTowerRecord();

    ClearEnemyPartyAfterChallenge();
    gSaveBlock2Ptr->frontier.challengeStatus = gSpecialVar_0x8005;
    VarSet(VAR_TEMP_CHALLENGE_STATUS, 0);
    gSaveBlock2Ptr->frontier.challengePaused = TRUE;
    SaveGameFrontier();
}

static void BattleTowerNop1(void)
{

}

static void BattleTowerNop2(void)
{

}

static u16 *sADMG_StoredPartnerOutput;

static bool8 ADMG_TryCaptureTowerContext(struct SaveBlock2 **save,
    enum FrontierLevelMode *level, u32 *battleMode)
{
    struct SaveBlock2 *selected = gSaveBlock2Ptr;
    if (selected == NULL) { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    enum FrontierLevelMode mode = selected->frontier.lvlMode;
    u64 generation = ADMG_BeginStateAccessCheck();
    u32 battle = VarGet(VAR_FRONTIER_BATTLE_MODE);
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != selected || selected->frontier.lvlMode != mode
     || battle >= ARRAY_COUNT(selected->frontier.towerWinStreaks)
     || (u32)mode >= ARRAY_COUNT(selected->frontier.towerWinStreaks[0]))
    { ADMG_StateAccessRejected(battle, __func__); return FALSE; }
    *save = selected; *level = mode; *battleMode = battle;
    return TRUE;
}

static bool8 ADMG_TryGetStoredPartnerChoices(u32 identity, u32 kind,
    u16 *choices, u32 capacity, u32 *choiceCount)
{
    if (choices == NULL || choiceCount == NULL
     || (kind != ADMG_ACTOR_APPRENTICE && kind != ADMG_ACTOR_RECORD_FRIEND))
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    struct ADMG_TrainerActorRef actor;
    if (!ADMG_TryClassifyTrainerActor(identity, ADMG_ACTOR_FACILITY, ADMG_ACTOR_PARTNER, &actor)) return FALSE;
    if (actor.kind != kind) { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    if (save == NULL) { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    enum FrontierLevelMode mode = save->frontier.lvlMode;
    u32 rows;
    if (kind == ADMG_ACTOR_APPRENTICE)
    {
        if (actor.index >= ARRAY_COUNT(save->apprentices))
        { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
        rows = ARRAY_COUNT(save->apprentices[0].party);
    }
    else
    {
        if (actor.index >= ARRAY_COUNT(save->frontier.towerRecords))
        { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
        rows = ARRAY_COUNT(save->frontier.towerRecords[0].party);
    }
    u16 selected[PARTY_SIZE];
    enum Species species[PARTY_SIZE];
    u8 levels[PARTY_SIZE];
    if (rows > ARRAY_COUNT(selected) || capacity < rows)
    { ADMG_StateAccessRejected(capacity, __func__); return FALSE; }
    u64 generation = ADMG_BeginStateAccessCheck();
    enum Species species1 = GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES);
    enum Species species2 = GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_SPECIES);
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != save || (kind == ADMG_ACTOR_RECORD_FRIEND && save->frontier.lvlMode != mode))
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    for (u32 i = 0; i < rows; ++i)
    {
        species[i] = kind == ADMG_ACTOR_APPRENTICE
            ? save->apprentices[actor.index].party[i].species
            : save->frontier.towerRecords[actor.index].party[i].species;
        if (kind == ADMG_ACTOR_RECORD_FRIEND) levels[i] = save->frontier.towerRecords[actor.index].party[i].level;
    }
    u32 count = 0;
    for (u32 i = 0; i < rows; ++i)
        if (species[i] != species1 && species[i] != species2
         && (kind == ADMG_ACTOR_APPRENTICE
          || (levels[i] <= GetFrontierEnemyMonLevel(mode) && species[i] != SPECIES_NONE)))
            selected[count++] = i;
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != save || (kind == ADMG_ACTOR_RECORD_FRIEND && save->frontier.lvlMode != mode))
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    for (u32 i = 0; i < count; ++i) choices[i] = selected[i];
    *choiceCount = count;
    return TRUE;
}

static bool8 ADMG_TrySelectStoredPartnerPair(u32 identity, u32 kind, u32 offset)
{
    if (offset > ARRAY_COUNT(gFrontierTempParty) || 2 > ARRAY_COUNT(gFrontierTempParty) - offset)
    { ADMG_StateAccessRejected(offset, __func__); return FALSE; }
    u16 choices[PARTY_SIZE];
    u32 count;
    if (!ADMG_TryGetStoredPartnerChoices(identity, kind, choices, ARRAY_COUNT(choices), &count)) return FALSE;
    if (count < 2) { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    u16 first = choices[Random() % count];
    u16 second;
    do { second = choices[Random() % count]; } while (first == second);
    u16 *output = sADMG_StoredPartnerOutput != NULL ? sADMG_StoredPartnerOutput : gFrontierTempParty;
    output[offset] = first;
    output[offset + 1] = second;
    return TRUE;
}

ADMG_NATIVE_ENTRY void GetApprenticeMultiPartnerParty(enum TrainerId trainerId)
{
    ADMG_TrySelectStoredPartnerPair(trainerId, ADMG_ACTOR_APPRENTICE, 0);
}

ADMG_NATIVE_ENTRY void GetRecordMixFriendMultiPartnerParty(enum TrainerId trainerId)
{
    ADMG_TrySelectStoredPartnerPair(trainerId, ADMG_ACTOR_RECORD_FRIEND, 2);
}

ADMG_NATIVE_ENTRY void LoadMultiPartnerCandidatesData(void)
{
    s32 i, j, k;
    u32 spArray[BATTLE_TOWER_RECORD_COUNT > APPRENTICE_COUNT ? BATTLE_TOWER_RECORD_COUNT : APPRENTICE_COUNT];
    s32 r10;
    enum TrainerId trainerId;
    u16 monId;
    enum FrontierLevelMode lvlMode;
    u32 battleMode;
    s32 challengeNum;
    enum Species species1, species2;
    struct ObjectEventTemplate *objEventTemplates;

    struct SaveBlock1 *mapSave = gSaveBlock1Ptr;
    struct SaveBlock2 *save;
    if (mapSave == NULL) { ADMG_StateAccessRejected(0, __func__); return; }
    if (!ADMG_TryCaptureTowerContext(&save, &lvlMode, &battleMode)) return;
    if (gSaveBlock1Ptr != mapSave) { ADMG_StateAccessRejected(battleMode, __func__); return; }
    enum TrainerId ids[ARRAY_COUNT(save->frontier.trainerIds)];
    u16 graphics[9];
    u16 temp[ARRAY_COUNT(gFrontierTempParty)];
    if (ARRAY_COUNT(mapSave->objectEventTemplates) < ARRAY_COUNT(graphics))
    { ADMG_StateAccessRejected(ARRAY_COUNT(graphics), __func__); return; }
    for (u32 slot = 0; slot < ARRAY_COUNT(ids); ++slot) ids[slot] = save->frontier.trainerIds[slot];
    for (u32 slot = 0; slot < ARRAY_COUNT(graphics); ++slot) graphics[slot] = mapSave->objectEventTemplates[slot].graphicsId;
    for (u32 slot = 0; slot < ARRAY_COUNT(temp); ++slot) temp[slot] = gFrontierTempParty[slot];
    const struct BattleFrontierTrainer *table = gFacilityTrainers;
    const struct TrainerMon *mons = gFacilityTrainerMons;
    u32 monCount;
    const struct BattleFrontierTrainer *initialRow;
    if (!ADMG_TryGetActiveFacilityTrainerRow(0, &initialRow)
     || !ADMG_TryGetFacilityMonTableCount(mons, &monCount)) return;
    u64 generation = ADMG_BeginStateAccessCheck();
    bool8 showApprentice = FALSE, showRecord = FALSE;
    u16 *previousOutput = sADMG_StoredPartnerOutput;
    sADMG_StoredPartnerOutput = temp;
    objEventTemplates = mapSave->objectEventTemplates;
    challengeNum = save->frontier.towerWinStreaks[battleMode][lvlMode] / FRONTIER_STAGES_PER_CHALLENGE;
    species1 = GetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_SPECIES);
    species2 = GetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_SPECIES);
    if (!ADMG_EndStateAccessCheck(generation)) goto restore;

    j = 0;
    do
    {
        do
        {
            trainerId = GetRandomScaledFrontierTrainerId(challengeNum, 0);
            const struct BattleFrontierTrainer *selected;
            if (gFacilityTrainers != table)
            { ADMG_StateAccessRejected(trainerId, __func__); goto restore; }
            if (!ADMG_TryGetActiveFacilityTrainerRow(trainerId, &selected)) goto restore;
            for (i = 0; i < j; i++)
            {
                if (ids[i] == trainerId)
                    break;
                const struct BattleFrontierTrainer *prior;
                if (!ADMG_TryGetActiveFacilityTrainerRow(ids[i], &prior)) goto restore;
                if (prior->facilityClass == selected->facilityClass)
                    break;
            }
        } while (i != j);
        ids[j] = trainerId;
        j++;
    } while (j < 6);

    r10 = 8;
    for (i = 0; i < 6; i++)
    {
        trainerId = ids[i];
        graphics[i + 1] = GetBattleFacilityTrainerGfxId(trainerId);
        if (!ADMG_EndStateAccessCheck(generation)) goto restore;
        for (j = 0; j < 2; j++)
        {
            while (1)
            {
                monId = GetRandomFrontierMonFromSet(trainerId);
                const struct TrainerMon *selected;
                if (!ADMG_EndStateAccessCheck(generation)) goto restore;
                if (gFacilityTrainerMons != mons)
                { ADMG_StateAccessRejected(monId, __func__); goto restore; }
                if (!ADMG_TryGetFacilityMonRowFromTable(mons, monCount, monId, &selected)) goto restore;
                const struct TrainerMon *prior;
                if (j % 2 != 0 && !ADMG_TryGetFacilityMonRowFromTable(mons, monCount, ids[r10 - 1], &prior)) goto restore;
                if (j % 2 != 0 && prior->heldItem == selected->heldItem)
                    continue;

                for (k = 8; k < r10; k++)
                {
                    if (!ADMG_TryGetFacilityMonRowFromTable(mons, monCount, ids[k], &prior)) goto restore;
                    if (prior->species == selected->species)
                        break;
                    if (species1 == selected->species)
                        break;
                    if (species2 == selected->species)
                        break;
                }
                if (k == r10)
                    break;
            }

            ids[r10] = monId;
            r10++;
        }
    }

    r10 = 0;
    if (!ADMG_EndStateAccessCheck(generation)) goto restore;
    if (gSaveBlock2Ptr != save || gSaveBlock1Ptr != mapSave || save->frontier.lvlMode != lvlMode)
    { ADMG_StateAccessRejected(battleMode, __func__); goto restore; }
    ValidateApprenticesChecksums();
    if (!ADMG_EndStateAccessCheck(generation)) goto restore;
    if (gSaveBlock2Ptr != save) { ADMG_StateAccessRejected(battleMode, __func__); goto restore; }
    for (i = 0; i < APPRENTICE_COUNT; i++)
    {
        if (save->apprentices[i].lvlMode != 0 && save->apprentices[i].numQuestions >= ARRAY_COUNT(sApprenticeChallengeThreshold))
        { ADMG_StateAccessRejected(save->apprentices[i].numQuestions, __func__); goto restore; }
        if (save->apprentices[i].lvlMode != 0
            && sApprenticeChallengeThreshold[save->apprentices[i].numQuestions] / FRONTIER_STAGES_PER_CHALLENGE <= challengeNum
            && save->apprentices[i].lvlMode - 1 == lvlMode)
        {
            k = 0;
            for (j = 0; j < MULTI_PARTY_SIZE; j++)
            {
                if (species1 != save->apprentices[i].party[j].species
                    && species2 != save->apprentices[i].party[j].species)
                {
                    k++;
                }
            }
            if (k > 2)
            {
                spArray[r10] = i + TRAINER_RECORD_MIXING_APPRENTICE;
                r10++;
            }
        }
    }
    if (r10 != 0)
    {
        ids[6] = spArray[Random() % r10];
        graphics[7] = GetBattleFacilityTrainerGfxId(ids[6]);
        if (!ADMG_EndStateAccessCheck(generation)) goto restore;
        showApprentice = TRUE;
        GetApprenticeMultiPartnerParty(ids[6]);
    }

    r10 = 0;
    for (i = 0; i < BATTLE_TOWER_RECORD_COUNT; i++)
    {
        u32 *record = (u32 *)(&save->frontier.towerRecords[i]);
        u32 recordHasData = 0;
        u32 checksum = 0;
        for (j = 0; j < (sizeof(struct EmeraldBattleTowerRecord) - 4) / 4; j++) // - 4, because of the last field being the checksum itself.
        {
            recordHasData |= record[j];
            checksum += record[j];
        }

        if (save->frontier.towerRecords[i].winStreak / FRONTIER_STAGES_PER_CHALLENGE <= challengeNum
            && save->frontier.towerRecords[i].lvlMode == lvlMode
            && recordHasData
            && save->frontier.towerRecords[i].checksum == checksum)
        {
            k = 0;
            for (j = 0; j < MAX_FRONTIER_PARTY_SIZE; j++)
            {
                if (species1 != save->frontier.towerRecords[i].party[j].species
                    && species2 != save->frontier.towerRecords[i].party[j].species
                    && save->frontier.towerRecords[i].party[j].level <= GetFrontierEnemyMonLevel(lvlMode)
                    && save->frontier.towerRecords[i].party[j].species != SPECIES_NONE)
                {
                    k++;
                }
            }
            if (k > 1)
            {
                spArray[r10] = i + TRAINER_RECORD_MIXING_FRIEND;
                r10++;
            }
        }
    }
    if (r10 != 0)
    {
        ids[7] = spArray[Random() % r10];
        graphics[8] = GetBattleFacilityTrainerGfxId(ids[7]);
        if (!ADMG_EndStateAccessCheck(generation)) goto restore;
        showRecord = TRUE;
        GetRecordMixFriendMultiPartnerParty(ids[7]);
    }

    if (!ADMG_EndStateAccessCheck(generation)) goto restore;
    if (gSaveBlock2Ptr != save || gSaveBlock1Ptr != mapSave || save->frontier.lvlMode != lvlMode)
    { ADMG_StateAccessRejected(battleMode, __func__); goto restore; }
    for (u32 slot = 0; slot < ARRAY_COUNT(ids); ++slot) save->frontier.trainerIds[slot] = ids[slot];
    for (u32 slot = 1; slot < ARRAY_COUNT(graphics); ++slot) objEventTemplates[slot].graphicsId = graphics[slot];
    for (u32 slot = 0; slot < ARRAY_COUNT(temp); ++slot) gFrontierTempParty[slot] = temp[slot];
    if (showApprentice) FlagClear(FLAG_HIDE_BATTLE_TOWER_MULTI_BATTLE_PARTNER_ALT_1);
    if (showRecord) FlagClear(FLAG_HIDE_BATTLE_TOWER_MULTI_BATTLE_PARTNER_ALT_2);
restore:
    sADMG_StoredPartnerOutput = previousOutput;
}

struct ADMG_PartnerDescription
{
    struct ADMG_TrainerActorRef actor;
    struct SaveBlock2 *save;
    enum FrontierLevelMode mode;
    enum Move move;
    enum Species species;
    struct Apprentice apprentice;
};

static bool8 ADMG_TryCapturePartnerDescription(u32 identity, u32 monId, u32 message,
    struct ADMG_PartnerDescription *input)
{
    if (input == NULL) { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    struct ADMG_PartnerDescription selected = {0};
    if (!ADMG_TryClassifyTrainerActor(identity, ADMG_ACTOR_FACILITY, ADMG_ACTOR_PARTNER, &selected.actor)) return FALSE;
    if (selected.actor.kind != ADMG_ACTOR_FRONTIER && selected.actor.kind != ADMG_ACTOR_RECORD_FRIEND
     && selected.actor.kind != ADMG_ACTOR_APPRENTICE && selected.actor.kind != ADMG_ACTOR_EREADER)
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    if (save == NULL) { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    selected.save = save; selected.mode = save->frontier.lvlMode;
    if (selected.actor.kind == ADMG_ACTOR_FRONTIER)
    {
        const struct BattleFrontierTrainer *table;
        const struct TrainerMon *mons, *row;
        u32 trainerCount, monCount;
        if (!ADMG_TrySelectFacilityTrainerTables(selected.mode == FRONTIER_LVL_TENT, &table, &mons, &trainerCount)
         || !ADMG_TryGetFacilityMonTableCount(mons, &monCount)
         || !ADMG_TryGetFacilityMonRowFromTable(mons, monCount, monId, &row)) return FALSE;
        selected.move = row->moves[0]; selected.species = row->species;
    }
    else if (selected.actor.kind != ADMG_ACTOR_EREADER)
    {
        if (message != PARTNER_MSGID_MON1 && message != PARTNER_MSGID_MON2_ASK)
        { ADMG_StateAccessRejected(message, __func__); return FALSE; }
        u32 offset = selected.actor.kind == ADMG_ACTOR_RECORD_FRIEND ? message + 1 : message - 1;
        if (offset >= ARRAY_COUNT(gFrontierTempParty))
        { ADMG_StateAccessRejected(offset, __func__); return FALSE; }
        u32 index = gFrontierTempParty[offset];
        if (selected.actor.kind == ADMG_ACTOR_RECORD_FRIEND)
        {
            if (index >= ARRAY_COUNT(save->frontier.towerRecords[0].party))
            { ADMG_StateAccessRejected(index, __func__); return FALSE; }
            const struct BattleTowerPokemon *row = &save->frontier.towerRecords[selected.actor.index].party[index];
            selected.move = row->moves[0]; selected.species = row->species;
        }
        else
        {
            memcpy(&selected.apprentice, &save->apprentices[selected.actor.index], sizeof(selected.apprentice));
            if (index >= ARRAY_COUNT(selected.apprentice.party))
            { ADMG_StateAccessRejected(index, __func__); return FALSE; }
            selected.move = selected.apprentice.party[index].moves[0]; selected.species = selected.apprentice.party[index].species;
        }
    }
    memcpy(input, &selected, sizeof(selected));
    return TRUE;
}

ADMG_NATIVE_ENTRY void GetPotentialPartnerMoveAndSpecies(enum TrainerId trainerId, u16 monId)
{
    struct ADMG_PartnerDescription input;
    if (!ADMG_TryCapturePartnerDescription(trainerId, monId, gSpecialVar_0x8005, &input)) return;
    u64 generation = ADMG_BeginStateAccessCheck();
    SetFacilityPtrsGetLevel();
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (gSaveBlock2Ptr != input.save || input.save->frontier.lvlMode != input.mode)
    { ADMG_StateAccessRejected(trainerId, __func__); return; }
    if (input.actor.kind == ADMG_ACTOR_APPRENTICE)
    {
        for (u32 i = 0; i < PLAYER_NAME_LENGTH; ++i) gStringVar3[i] = input.apprentice.playerName[i];
        gStringVar3[PLAYER_NAME_LENGTH] = EOS;
        ConvertInternationalString(gStringVar3, input.apprentice.language);
    }
    StringCopy(gStringVar1, GetMoveName(input.move));
    StringCopy(gStringVar2, GetSpeciesName(input.species));
}

// For multi battles in the Battle Tower, the player may choose a partner by talking to them
// These partners can be an NPC or a former/record-mixed Apprentice
// When talked to, their response consists of:
// PARTNER_MSGID_INTRO - A greeting
// PARTNER_MSGID_MON1 - Naming one Pokémon on their team, and a move it has
// PARTNER_MSGID_MON2_ASK - Naming a second Pokémon on their team, a move it has, and asking if they'd like to be their partner
// PARTNER_MSGID_ACCEPT - If the player agrees to be their partner
// PARTNER_MSGID_REJECT - If the player declines to be their partner
ADMG_NATIVE_ENTRY void ShowPartnerCandidateMessage(void)
{
    s32 i, j, partnerId;
    s32 monId;
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    enum FrontierLevelMode level = save != NULL ? save->frontier.lvlMode : FRONTIER_LVL_50;
    u32 message = gSpecialVar_0x8005;
    u32 talked = gSpecialVar_LastTalked;
    if (save == NULL || talked < 2 || talked - 2 >= 8 || message > PARTNER_MSGID_REJECT)
    { ADMG_StateAccessRejected(talked, __func__); return; }
    u32 slot = talked - 2;
    enum TrainerId trainerId = save->frontier.trainerIds[slot];
    struct ADMG_TrainerActorRef actor;
    if (!ADMG_TryClassifyTrainerActor(trainerId, ADMG_ACTOR_FACILITY, ADMG_ACTOR_PARTNER, &actor)) return;
    if (actor.kind != ADMG_ACTOR_FRONTIER && actor.kind != ADMG_ACTOR_RECORD_FRIEND
     && actor.kind != ADMG_ACTOR_APPRENTICE && actor.kind != ADMG_ACTOR_EREADER)
    { ADMG_StateAccessRejected(trainerId, __func__); return; }
    struct Apprentice apprentice;
    if (actor.kind == ADMG_ACTOR_APPRENTICE)
    {
        memcpy(&apprentice, &save->apprentices[actor.index], sizeof(apprentice));
        if (apprentice.id >= ARRAY_COUNT(sPartnerApprenticeTextTables))
        { ADMG_StateAccessRejected(apprentice.id, __func__); return; }
    }
    u64 generation = ADMG_BeginStateAccessCheck();
    u16 winStreak = GetCurrentFacilityWinStreak();
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (gSaveBlock2Ptr != save) { ADMG_StateAccessRejected(trainerId, __func__); return; }
    s32 challengeNum = winStreak / FRONTIER_STAGES_PER_CHALLENGE;
    s32 k = slot;

    for (partnerId = 0; partnerId < ARRAY_COUNT(sPartnerTrainerTextTables); partnerId++)
    {
        if (sPartnerTrainerTextTables[partnerId].facilityClass == GetFrontierTrainerFacilityClass(trainerId))
            break;
        if (!ADMG_EndStateAccessCheck(generation)) return;
    }
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (gSaveBlock2Ptr != save || save->frontier.lvlMode != level)
    { ADMG_StateAccessRejected(trainerId, __func__); return; }
    const u8 *messageText = NULL;
    if (actor.kind == ADMG_ACTOR_APPRENTICE)
    {
        if (message >= sADMG_PartnerApprenticeTextCounts[apprentice.id])
        { ADMG_StateAccessRejected(message, __func__); return; }
        messageText = sPartnerApprenticeTextTables[apprentice.id][message];
    }
    else if (actor.kind != ADMG_ACTOR_EREADER)
    {
        if ((u32)partnerId >= ARRAY_COUNT(sPartnerTrainerTextTables)
         || message >= sPartnerTrainerTextTables[partnerId].stringCount)
        { ADMG_StateAccessRejected(trainerId, __func__); return; }
        messageText = sPartnerTrainerTextTables[partnerId].strings[message];
    }

    switch (message)
    {
    case PARTNER_MSGID_INTRO:
        if (trainerId == TRAINER_EREADER)
            return;
        if (trainerId < FRONTIER_TRAINERS_COUNT)
        {
            GetFrontierTrainerName(gStringVar1, trainerId);
        }
        else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
        {
            GetFrontierTrainerName(gStringVar1, trainerId);
        }
        else
        {
            s32 i;
            for (i = 0; i < PLAYER_NAME_LENGTH; i++)
                gStringVar1[i] = apprentice.playerName[i];
            gStringVar1[i] = EOS;
            ConvertInternationalString(gStringVar1, apprentice.language);
            ConvertIntToDecimalStringN(gStringVar2, apprentice.number, STR_CONV_MODE_LEFT_ALIGN, 3);
            GetFrontierTrainerName(gStringVar3, trainerId);
        }
        break;
    case PARTNER_MSGID_MON1:
        monId = 0;
        if (actor.kind == ADMG_ACTOR_FRONTIER)
        {
            if (8 + slot * 2 >= ARRAY_COUNT(save->frontier.trainerIds))
            { ADMG_StateAccessRejected(slot, __func__); return; }
            monId = save->frontier.trainerIds[8 + slot * 2];
        }
        GetPotentialPartnerMoveAndSpecies(trainerId, monId);
        break;
    case PARTNER_MSGID_MON2_ASK:
        monId = 0;
        if (actor.kind == ADMG_ACTOR_FRONTIER)
        {
            if (9 + slot * 2 >= ARRAY_COUNT(save->frontier.trainerIds))
            { ADMG_StateAccessRejected(slot, __func__); return; }
            monId = save->frontier.trainerIds[9 + slot * 2];
        }
        GetPotentialPartnerMoveAndSpecies(trainerId, monId);
        break;
    case PARTNER_MSGID_ACCEPT:
        enum TrainerId selectedPair[2];
        if (trainerId < FRONTIER_TRAINERS_COUNT)
        {
            if (9 + slot * 2 >= ARRAY_COUNT(save->frontier.trainerIds))
            { ADMG_StateAccessRejected(slot, __func__); return; }
            selectedPair[0] = save->frontier.trainerIds[8 + k * 2];
            selectedPair[1] = save->frontier.trainerIds[9 + k * 2];
        }
        else if (trainerId < TRAINER_RECORD_MIXING_APPRENTICE)
        {
            selectedPair[0] = gFrontierTempParty[2];
            selectedPair[1] = gFrontierTempParty[3];
        }
        else
        {
            selectedPair[0] = gFrontierTempParty[0];
            selectedPair[1] = gFrontierTempParty[1];
        }
        if (actor.kind == ADMG_ACTOR_FRONTIER)
        {
            const struct BattleFrontierTrainer *table;
            const struct TrainerMon *mons, *row;
            u32 trainerCount, monCount;
            if (!ADMG_TrySelectFacilityTrainerTables(save->frontier.lvlMode == FRONTIER_LVL_TENT, &table, &mons, &trainerCount)
             || !ADMG_TryGetFacilityMonTableCount(mons, &monCount)) return;
            for (u32 pair = 0; pair < 2; ++pair)
                if (!ADMG_TryGetFacilityMonRowFromTable(mons, monCount, selectedPair[pair], &row)) return;
        }
        else if (actor.kind != ADMG_ACTOR_EREADER)
        {
            u32 count = actor.kind == ADMG_ACTOR_RECORD_FRIEND
                ? ARRAY_COUNT(save->frontier.towerRecords[0].party) : ARRAY_COUNT(apprentice.party);
            if (selectedPair[0] >= count || selectedPair[1] >= count)
            { ADMG_StateAccessRejected(trainerId, __func__); return; }
        }
        u16 selectedOpponents[FRONTIER_STAGES_PER_CHALLENGE * 2];
        for (k = 0; k < FRONTIER_STAGES_PER_CHALLENGE * 2; k++)
        {
            while (1)
            {
                i = GetRandomScaledFrontierTrainerId(challengeNum, k / 2);
                struct ADMG_TrainerActorRef opponent;
                if (!ADMG_TryClassifyTrainerActor(i, ADMG_ACTOR_FACILITY, ADMG_ACTOR_OPPONENT, &opponent)
                 || opponent.kind != ADMG_ACTOR_FRONTIER)
                { ADMG_StateAccessRejected(i, __func__); return; }
                if (trainerId == i)
                    continue;

                for (j = 0; j < k; j++)
                {
                    if (selectedOpponents[j] == i)
                        break;
                }
                if (j == k)
                    break;
            }
            selectedOpponents[k] = i;
        }
        if (!ADMG_EndStateAccessCheck(generation)) return;
        if (gSaveBlock2Ptr != save) { ADMG_StateAccessRejected(trainerId, __func__); return; }
        gPartnerTrainerId = trainerId;
        save->frontier.trainerIds[18] = selectedPair[0]; save->frontier.trainerIds[19] = selectedPair[1];
        for (u32 slot = 0; slot < ARRAY_COUNT(selectedOpponents); ++slot) save->frontier.trainerIds[slot] = selectedOpponents[slot];
        save->frontier.trainerIds[17] = trainerId;
        break;
    case PARTNER_MSGID_REJECT:
        break;
    }

    if (trainerId == TRAINER_EREADER)
        return;

    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (actor.kind == ADMG_ACTOR_EREADER) return;
    ShowFieldMessage(messageText);
}

static bool8 ADMG_TryGetTowerBattlePair(const u16 *ids, u32 count, u32 battle,
    u16 *first, u16 *second)
{
    if (ids == NULL || first == NULL || second == NULL || count < 2 || battle >= count / 2)
    { ADMG_StateAccessRejected(battle, __func__); return FALSE; }
    struct ADMG_TrainerActorRef a, b;
    u32 index = battle * 2;
    if (!ADMG_TryClassifyTrainerActor(ids[index], ADMG_ACTOR_FACILITY, ADMG_ACTOR_OPPONENT, &a)
     || !ADMG_TryClassifyTrainerActor(ids[index + 1], ADMG_ACTOR_FACILITY, ADMG_ACTOR_OPPONENT, &b)) return FALSE;
    *first = a.identifier; *second = b.identifier;
    return TRUE;
}

ADMG_NATIVE_ENTRY void LoadLinkMultiOpponentsData(void)
{
    s32 challengeNum;
    s32 i, j;
    s32 trainerId = 0;
    u32 state = gSpecialVar_Result;
    if (state > 6) { ADMG_StateAccessRejected(state, __func__); return; }
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    if (save == NULL && state <= 3) { ADMG_StateAccessRejected(state, __func__); return; }
    enum FrontierLevelMode lvlMode = save != NULL ? save->frontier.lvlMode : FRONTIER_LVL_50;
    u64 generation = ADMG_BeginStateAccessCheck();
    u32 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    u32 battleNum = save != NULL ? save->frontier.curChallengeBattleNum : 0;
    GetMultiplayerId();
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (gSaveBlock2Ptr != save || (save != NULL && save->frontier.lvlMode != lvlMode))
    { ADMG_StateAccessRejected(state, __func__); return; }

    switch (gSpecialVar_Result)
    {
    case 0:
        if (battleMode == FRONTIER_MODE_LINK_MULTIS)
        {
            if (battleMode >= ARRAY_COUNT(save->frontier.towerWinStreaks)
             || (u32)lvlMode >= ARRAY_COUNT(save->frontier.towerWinStreaks[0]))
            { ADMG_StateAccessRejected(battleMode, __func__); return; }
            challengeNum = save->frontier.towerWinStreaks[battleMode][lvlMode] / FRONTIER_STAGES_PER_CHALLENGE;
            if (IsLinkTaskFinished())
            {
                SendBlock(BitmaskAllOtherLinkPlayers(), &challengeNum, sizeof(challengeNum));
                gSpecialVar_Result = 1;
            }
        }
        else
        {
            gSpecialVar_Result = 6;
        }
        break;
    case 1:
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            enum TrainerId selected[ARRAY_COUNT(save->frontier.trainerIds)];
            for (u32 slot = 0; slot < ARRAY_COUNT(selected); ++slot) selected[slot] = save->frontier.trainerIds[slot];

            if (gBlockRecvBuffer[0][0] > gBlockRecvBuffer[1][0])
                challengeNum = gBlockRecvBuffer[0][0];
            else
                challengeNum = gBlockRecvBuffer[1][0];
            for (i = 0; i < FRONTIER_STAGES_PER_CHALLENGE * 2; i++)
            {
                do
                {
                    trainerId = GetRandomScaledFrontierTrainerId(challengeNum, i / 2);
                    struct ADMG_TrainerActorRef actor;
                    if (!ADMG_TryClassifyTrainerActor(trainerId, ADMG_ACTOR_FACILITY, ADMG_ACTOR_OPPONENT, &actor)
                     || actor.kind != ADMG_ACTOR_FRONTIER)
                    { ADMG_StateAccessRejected(trainerId, __func__); return; }
                    for (j = 0; j < i; j++)
                    {
                        if (selected[j] == trainerId)
                            break;
                    }
                } while (i != j);
                if (i == j) // This condition is always true, because of the loop above.
                    selected[i] = trainerId;
            }
            if (!ADMG_EndStateAccessCheck(generation)) return;
            if (gSaveBlock2Ptr != save || save->frontier.lvlMode != lvlMode)
            { ADMG_StateAccessRejected(state, __func__); return; }
            ResetBlockReceivedFlags();
            for (u32 slot = 0; slot < ARRAY_COUNT(selected); ++slot) save->frontier.trainerIds[slot] = selected[slot];
            gSpecialVar_Result = 2;
        }
        break;
    case 2:
        if (IsLinkTaskFinished())
        {
            SendBlock(BitmaskAllOtherLinkPlayers(), &save->frontier.trainerIds, sizeof(save->frontier.trainerIds));
            gSpecialVar_Result = 3;
        }
        break;
    case 3:
        if ((GetBlockReceivedStatus() & 3) == 3)
        {
            u16 selected[ARRAY_COUNT(save->frontier.trainerIds)];
            memcpy(selected, gBlockRecvBuffer, sizeof(selected));
            u16 a, b;
            if (!ADMG_TryGetTowerBattlePair(selected, ARRAY_COUNT(selected), battleNum, &a, &b)) return;
            ResetBlockReceivedFlags();
            if (!ADMG_EndStateAccessCheck(generation)) return;
            if (gSaveBlock2Ptr != save || save->frontier.lvlMode != lvlMode)
            { ADMG_StateAccessRejected(state, __func__); return; }
            memcpy(save->frontier.trainerIds, selected, sizeof(selected));
            TRAINER_BATTLE_PARAM.opponentA = a;
            TRAINER_BATTLE_PARAM.opponentB = b;
            SetBattleFacilityTrainerGfxId(TRAINER_BATTLE_PARAM.opponentA, 0);
            SetBattleFacilityTrainerGfxId(TRAINER_BATTLE_PARAM.opponentB, 1);
            if (gReceivedRemoteLinkPlayers && gWirelessCommType == 0)
                gSpecialVar_Result = 4;
            else
                gSpecialVar_Result = 6;
        }
        break;
    case 4:
        SetCloseLinkCallback();
        gSpecialVar_Result = 5;
        break;
    case 5:
        if (!gReceivedRemoteLinkPlayers)
        {
            gSpecialVar_Result = 6;
        }
        break;
    case 6:
        return;
    }
}

static void TowerTryCloseLink(void)
{
    if (gWirelessCommType != 0)
        SetCloseLinkCallback();
}

ADMG_NATIVE_ENTRY void SetMultiPartnerGfx(void)
{
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    if (save == NULL) { ADMG_StateAccessRejected(0, __func__); return; }
    struct ADMG_TrainerActorRef actor;
    enum TrainerId identity = save->frontier.trainerIds[17];
    if (!ADMG_TryClassifyTrainerActor(identity, ADMG_ACTOR_FACILITY, ADMG_ACTOR_PARTNER, &actor)) return;
    // 0xF below means use VAR_OBJ_GFX_ID_E.
    SetBattleFacilityTrainerGfxId(identity, 0xF);
}

ADMG_NATIVE_ENTRY void SetTowerInterviewData(void)
{
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    u64 generation = ADMG_BeginStateAccessCheck();
    u32 mode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (mode != FRONTIER_MODE_SINGLES) return;
    if (save == NULL || gSaveBlock2Ptr != save)
    { ADMG_StateAccessRejected(mode, __func__); return; }
    enum TrainerId identity = TRAINER_BATTLE_PARAM.opponentA;
    struct ADMG_TrainerActorRef actor;
    // The existing name/language interfaces also support disjoint native Kanto
    // content. Preserve that caller surface without interpreting facility
    // ordinals as Hoenn content or a named partner as an opponent.
    u32 domain = identity > ADMG_KANTO_TRAINER_FIRST
        && identity < ADMG_KANTO_TRAINER_FIRST + ADMG_KANTO_TRAINERS_COUNT
        ? ADMG_ACTOR_CONTENT : ADMG_ACTOR_FACILITY;
    if (!ADMG_TryClassifyTrainerActor(identity, domain, ADMG_ACTOR_OPPONENT, &actor)) return;
    u8 text[32] = {EOS};
    GetFrontierTrainerName(text, identity);
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (gSaveBlock2Ptr != save || TRAINER_BATTLE_PARAM.opponentA != identity)
    { ADMG_StateAccessRejected(identity, __func__); return; }
    if (!ADMG_TryValidateTrainerText(text, sizeof(text), identity, NULL)) return;
    StripExtCtrlCodes(text);
    u32 length = 0;
    while (length < ARRAY_COUNT(text) && text[length] != EOS) ++length;
    if (length >= ARRAY_COUNT(save->frontier.towerInterview.opponentName))
    { ADMG_StateAccessRejected(identity, __func__); return; }
    __typeof__(save->frontier.towerInterview) selected;
    memcpy(&selected, &save->frontier.towerInterview, sizeof(selected));
    StringCopy(selected.opponentName, text);
    GetBattleTowerTrainerLanguage(&selected.opponentLanguage, identity);
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (gSaveBlock2Ptr != save || TRAINER_BATTLE_PARAM.opponentA != identity)
    { ADMG_StateAccessRejected(identity, __func__); return; }
    struct Pokemon *opponent = GetBattlerMon(1);
    struct Pokemon *player = GetBattlerMon(0);
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (opponent == NULL || player == NULL)
    { ADMG_StateAccessRejected(identity, __func__); return; }
    selected.opponentSpecies = GetMonData(opponent, MON_DATA_SPECIES);
    selected.playerSpecies = GetMonData(player, MON_DATA_SPECIES);
    // Preserve the original index and byte extent; this is not a nickname remap.
    for (u32 i = 0; i < VANILLA_POKEMON_NAME_LENGTH + 1; ++i)
        selected.opponentMonNickname[i] = gBattleMons[0].nickname[i];
    u8 outcome = gBattleOutcome;
    if (!ADMG_EndStateAccessCheck(generation)) return;
    if (gSaveBlock2Ptr != save || TRAINER_BATTLE_PARAM.opponentA != identity)
    { ADMG_StateAccessRejected(identity, __func__); return; }
    memcpy(&save->frontier.towerInterview, &selected, sizeof(selected));
    save->frontier.towerBattleOutcome = outcome;
}

static void ValidateBattleTowerRecordChecksums(void)
{
    s32 i, j;
    u32 *record = (u32 *)(&gSaveBlock2Ptr->frontier.towerPlayer);
    u32 checksum = 0;

    for (j = 0; j < (sizeof(struct EmeraldBattleTowerRecord) - 4) / 4; j++) // - 4, because of the last field being the checksum itself.
    {
        checksum += record[j];
    }
    if (gSaveBlock2Ptr->frontier.towerPlayer.checksum != checksum)
        ClearBattleTowerRecord(&gSaveBlock2Ptr->frontier.towerPlayer);

    for (i = 0; i < BATTLE_TOWER_RECORD_COUNT; i++)
    {
        record = (u32 *)(&gSaveBlock2Ptr->frontier.towerRecords[i]);
        checksum = 0;
        for (j = 0; j < (sizeof(struct EmeraldBattleTowerRecord) - 4) / 4; j++) // - 4, because of the last field being the checksum itself.
        {
            checksum += record[j];
        }
        if (gSaveBlock2Ptr->frontier.towerRecords[i].checksum != checksum)
            ClearBattleTowerRecord(&gSaveBlock2Ptr->frontier.towerRecords[i]);
    }
}

void CalcEmeraldBattleTowerChecksum(struct EmeraldBattleTowerRecord *record)
{
    u32 i;

    record->checksum = 0;
    for (i = 0; i < (sizeof(struct EmeraldBattleTowerRecord) - 4) / 4; i++) // - 4, because of the last field being the checksum itself.
        record->checksum += ((u32 *)record)[i];
}

void CalcRubyBattleTowerChecksum(struct RSBattleTowerRecord *record)
{
    u32 i;

    record->checksum = 0;
    for (i = 0; i < (sizeof(struct RSBattleTowerRecord) - 4) / 4; i++) // - 4, because of the last field being the checksum itself.
        record->checksum += ((u32 *)record)[i];
}

static void ClearBattleTowerRecord(struct EmeraldBattleTowerRecord *record)
{
    u32 i;

    for (i = 0; i < sizeof(struct EmeraldBattleTowerRecord) / 4; i++)
        ((u32 *)record)[i] = 0;
}

u16 GetCurrentBattleTowerWinStreak(enum FrontierLevelMode lvlMode, u8 battleMode)
{
    u16 winStreak = gSaveBlock2Ptr->frontier.towerWinStreaks[battleMode][lvlMode];

    if (winStreak > MAX_STREAK)
        return MAX_STREAK;
    else
        return winStreak;
}

static u8 GetMonCountForBattleMode(u8 battleMode)
{
    u8 partySizes[ARRAY_COUNT(sBattleTowerPartySizes)];
    memcpy(partySizes, sBattleTowerPartySizes, sizeof(sBattleTowerPartySizes));

    if (battleMode < ARRAY_COUNT(sBattleTowerPartySizes))
        return partySizes[battleMode];
    else
        return FRONTIER_PARTY_SIZE;
}

struct RibbonCounter
{
    u8 partyIndex;
    u8 count;
};

static void AwardBattleTowerRibbons(void)
{
    s32 i;
    u32 partyIndex;
#ifdef BUGFIX
    struct RibbonCounter ribbons[MAX_FRONTIER_PARTY_SIZE];
#else
    struct RibbonCounter ribbons[3]; // BUG: 4 Pokémon can receive ribbons in a double battle mode.
#endif
    u8 ribbonType = 0;
    enum FrontierLevelMode lvlMode = gSaveBlock2Ptr->frontier.lvlMode;
    u8 battleMode = VarGet(VAR_FRONTIER_BATTLE_MODE);
    u8 monCount = GetMonCountForBattleMode(battleMode);

    if (lvlMode != FRONTIER_LVL_50)
        ribbonType = MON_DATA_VICTORY_RIBBON;
    else
        ribbonType = MON_DATA_WINNING_RIBBON;

    gSpecialVar_Result = FALSE;

    if (GetCurrentBattleTowerWinStreak(lvlMode, battleMode) > 55)
    {
        for (i = 0; i < monCount; i++)
        {
            partyIndex = gSaveBlock2Ptr->frontier.selectedPartyMons[i] - 1;
            ribbons[i].partyIndex = partyIndex;
            ribbons[i].count = 0;
            if (!GetMonData(GetSavedPlayerPartyMon(partyIndex), ribbonType))
            {
                gSpecialVar_Result = TRUE;
                SetMonData(GetSavedPlayerPartyMon(partyIndex), ribbonType, &gSpecialVar_Result);
                ribbons[i].count = GetRibbonCount(GetSavedPlayerPartyMon(partyIndex));
            }
        }
    }

    if (gSpecialVar_Result)
    {
        IncrementGameStat(GAME_STAT_RECEIVED_RIBBONS);
        for (i = 1; i < monCount; i++)
        {
            if (ribbons[i].count > ribbons[0].count)
            {
                struct RibbonCounter prevBest = ribbons[0];
                ribbons[0] = ribbons[i];
                ribbons[i] = prevBest;
            }
        }
        if (ribbons[0].count > NUM_CUTIES_RIBBONS)
        {
            TryPutSpotTheCutiesOnAir(GetSavedPlayerPartyMon(ribbons[0].partyIndex), ribbonType);
        }
    }
}



void TryHideBattleTowerReporter(void)
{
    if (gSaveBlock2Ptr->frontier.challengeStatus == CHALLENGE_STATUS_SAVING)
        HideBattleTowerReporter();
    if (FlagGet(FLAG_CANCEL_BATTLE_ROOM_CHALLENGE) == TRUE)
    {
        HideBattleTowerReporter();
        FlagClear(FLAG_CANCEL_BATTLE_ROOM_CHALLENGE);
    }
}

bool32 RubyBattleTowerRecordToEmerald(struct RSBattleTowerRecord *src, struct EmeraldBattleTowerRecord *dst)
{
    s32 i, validMons = 0;

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        if (src->party[i].species)
            validMons++;
    }

    if (validMons != FRONTIER_PARTY_SIZE)
    {
        memset(dst, 0, sizeof(*dst));
        return FALSE;
    }
    else
    {
        dst->lvlMode = src->lvlMode;
        dst->winStreak = src->winStreak;
        // UB: Reading outside the array. sRubyFacilityClassToEmerald has less than FACILITY_CLASSES_COUNT entries.
        #ifdef UBFIX
        for (i = 0; i < ARRAY_COUNT(sRubyFacilityClassToEmerald); i++)
        #else
        for (i = 0; i < FACILITY_CLASSES_COUNT; i++)
        #endif
        {
            if (sRubyFacilityClassToEmerald[i][0] == src->facilityClass)
                break;
        }
        if (i != FACILITY_CLASSES_COUNT)
            dst->facilityClass = sRubyFacilityClassToEmerald[i][1];
        else
            dst->facilityClass = FACILITY_CLASS_YOUNGSTER;

        for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
            dst->name[i] = src->name[i];
        for (i = 0; i < TRAINER_ID_LENGTH; i++)
            dst->trainerId[i] = src->trainerId[i];
        for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
            dst->greeting[i] = src->greeting[i];
        for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
            dst->speechWon[i] = sRecordTrainerSpeechWon[i];
        for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
            dst->speechLost[i] = sRecordTrainerSpeechLost[i];
        for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
            dst->party[i] = src->party[i];

        CpuFill32(0, &dst->party[FRONTIER_PARTY_SIZE], sizeof(dst->party[FRONTIER_PARTY_SIZE]));
        CalcEmeraldBattleTowerChecksum(dst);
        return TRUE;
    }
}

bool32 EmeraldBattleTowerRecordToRuby(struct EmeraldBattleTowerRecord *src, struct RSBattleTowerRecord *dst)
{
    s32 i, validMons = 0;

    for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
    {
        if (src->party[i].species)
            validMons++;
    }

    if (validMons != FRONTIER_PARTY_SIZE)
    {
        memset(dst, 0, sizeof(*dst));
        return FALSE;
    }
    else
    {
        dst->lvlMode = src->lvlMode;
        dst->winStreak = src->winStreak;
        // UB: Reading outside the array. sRubyFacilityClassToEmerald has less than FACILITY_CLASSES_COUNT entries.
        #ifdef UBFIX
        for (i = 0; i < ARRAY_COUNT(sRubyFacilityClassToEmerald); i++)
        #else
        for (i = 0; i < FACILITY_CLASSES_COUNT; i++)
        #endif
        {
            if (sRubyFacilityClassToEmerald[i][1] == src->facilityClass)
                break;
        }
        if (i != FACILITY_CLASSES_COUNT)
            dst->facilityClass = sRubyFacilityClassToEmerald[i][0];
        else
            dst->facilityClass = RS_FACILITY_CLASS_YOUNGSTER;

        for (i = 0; i < PLAYER_NAME_LENGTH + 1; i++)
            dst->name[i] = src->name[i];
        for (i = 0; i < TRAINER_ID_LENGTH; i++)
            dst->trainerId[i] = src->trainerId[i];
        for (i = 0; i < EASY_CHAT_BATTLE_WORDS_COUNT; i++)
            dst->greeting[i] = src->greeting[i];
        for (i = 0; i < FRONTIER_PARTY_SIZE; i++)
            dst->party[i] = src->party[i];

        CalcRubyBattleTowerChecksum(dst);
        return TRUE;
    }
}

void CalcApprenticeChecksum(struct Apprentice *apprentice)
{
    s32 i;

    apprentice->checksum = 0;
    for (i = 0; i < offsetof(struct Apprentice, checksum) / sizeof(u32); i++)
        apprentice->checksum += ((u32 *)apprentice)[i];
}

static void ClearApprentice(struct Apprentice *apprentice)
{
    s32 i;

    for (i = 0; i < sizeof(struct Apprentice) / sizeof(u32); i++)
        ((u32 *)apprentice)[i] = 0;
    ResetApprenticeStruct(apprentice);
}

static void ValidateApprenticesChecksums(void)
{
    s32 i, j;

    for (i = 0; i < APPRENTICE_COUNT; i++)
    {
        u32 *data = (u32 *) &gSaveBlock2Ptr->apprentices[i];
        u32 checksum = 0;
        for (j = 0; j < offsetof(struct Apprentice, checksum) / sizeof(u32); j++)
            checksum += data[j];
        if (gSaveBlock2Ptr->apprentices[i].checksum != checksum)
            ClearApprentice(&gSaveBlock2Ptr->apprentices[i]);
    }
}

ADMG_NATIVE_ENTRY void GetBattleTowerTrainerLanguage(u8 *dst, enum TrainerId trainerId)
{
    if (dst == NULL) { ADMG_StateAccessRejected(trainerId, __func__); return; }
    struct ADMG_TrainerActorRef actor;
    u8 language;
    if (IsPartnerTrainerId(trainerId)
     || (trainerId > ADMG_KANTO_TRAINER_FIRST && trainerId < ADMG_KANTO_TRAINER_FIRST + ADMG_KANTO_TRAINERS_COUNT))
    {
        struct ADMG_TrainerRef content;
        if (!ADMG_TryResolveTrainer(trainerId, &content)) return;
        /* Compiled content names use the same game-language authority as native NPCs. */
        language = gGameLanguage;
    }
    else
    {
        if (!ADMG_TryClassifyTrainerActor(trainerId, ADMG_ACTOR_FACILITY, ADMG_ACTOR_OPPONENT, &actor)) return;
        if (actor.kind == ADMG_ACTOR_FRONTIER || actor.kind == ADMG_ACTOR_EREADER || actor.kind == ADMG_ACTOR_BRAIN)
            language = gGameLanguage;
        else
        {
            u64 generation = ADMG_BeginStateAccessCheck();
            if (gBattleTypeFlags & BATTLE_TYPE_RECORDED)
                language = actor.kind == ADMG_ACTOR_RECORD_FRIEND
                    ? GetRecordedBattleRecordMixFriendLanguage() : GetRecordedBattleApprenticeLanguage();
            else
            {
                struct SaveBlock2 *save = gSaveBlock2Ptr;
                if (save == NULL) { ADMG_StateAccessRejected(trainerId, __func__); return; }
                if (actor.kind == ADMG_ACTOR_RECORD_FRIEND)
                {
                    if (actor.index >= ARRAY_COUNT(save->frontier.towerRecords))
                    { ADMG_StateAccessRejected(trainerId, __func__); return; }
                    language = save->frontier.towerRecords[actor.index].language;
                }
                else
                {
                    if (actor.index >= ARRAY_COUNT(save->apprentices))
                    { ADMG_StateAccessRejected(trainerId, __func__); return; }
                    language = save->apprentices[actor.index].language;
                }
            }
            if (!ADMG_EndStateAccessCheck(generation)) return;
        }
    }
    *dst = language;
}

static u16 GetBattleTentTrainerId(void)
{
    u32 facility = VarGet(VAR_FRONTIER_FACILITY);

    if (facility == FRONTIER_FACILITY_PALACE)       // Verdanturf Tent; uses Palace mechanics
        return Random() % NUM_BATTLE_TENT_TRAINERS;
    else if (facility == FRONTIER_FACILITY_ARENA)   // Fallarbor Tent; uses Arena mechanics
        return Random() % NUM_BATTLE_TENT_TRAINERS;
    else if (facility == FRONTIER_FACILITY_FACTORY) // Slateport Tent; uses Factory mechanics
        return Random() % NUM_BATTLE_TENT_TRAINERS;
    else if (facility == FRONTIER_FACILITY_TOWER)
        return 0;
    else
        return 0;
}

bool8 ADMG_TryGetFacilityMonSetFromSpan(const u16 *set, u32 extent,
    u32 monCount, const u16 **selectedSet, u32 *selectedCount)
{
    if (set == NULL || selectedSet == NULL || selectedCount == NULL || extent < 2)
    { ADMG_StateAccessRejected(extent, __func__); return FALSE; }
    for (u32 i = 0; i < extent - 1; ++i)
        if (set[i] == 0xFFFF || set[i] >= monCount)
        { ADMG_StateAccessRejected(set[i], __func__); return FALSE; }
    if (set[extent - 1] != 0xFFFF)
    { ADMG_StateAccessRejected(extent, __func__); return FALSE; }
    *selectedSet = set; *selectedCount = extent - 1;
    return TRUE;
}

bool8 ADMG_TryGetFacilityTrainerMonSet(const struct BattleFrontierTrainer *table,
    u32 trainerId, const struct TrainerMon **mons, const u16 **set, u32 *count)
{
    if (mons == NULL || set == NULL || count == NULL)
    { ADMG_StateAccessRejected(trainerId, __func__); return FALSE; }
    const u32 *extents;
    const struct TrainerMon *selectedMons;
    u32 trainerCount;
    if (table == gBattleFrontierTrainers)
    { extents = sADMG_gBattleFrontierTrainersMonSetExtents; selectedMons = gBattleFrontierMons; trainerCount = ARRAY_COUNT(gBattleFrontierTrainers); }
    else if (table == gSlateportBattleTentTrainers)
    { extents = sADMG_gSlateportBattleTentTrainersMonSetExtents; selectedMons = gSlateportBattleTentMons; trainerCount = ARRAY_COUNT(gSlateportBattleTentTrainers); }
    else if (table == gVerdanturfBattleTentTrainers)
    { extents = sADMG_gVerdanturfBattleTentTrainersMonSetExtents; selectedMons = gVerdanturfBattleTentMons; trainerCount = ARRAY_COUNT(gVerdanturfBattleTentTrainers); }
    else if (table == gFallarborBattleTentTrainers)
    { extents = sADMG_gFallarborBattleTentTrainersMonSetExtents; selectedMons = gFallarborBattleTentMons; trainerCount = ARRAY_COUNT(gFallarborBattleTentTrainers); }
    else { ADMG_StateAccessRejected(trainerId, __func__); return FALSE; }
    const struct BattleFrontierTrainer *trainer;
    u32 monCount, selectedCount;
    const u16 *selectedSet;
    if (!ADMG_TryGetFacilityTrainerRowFromTable(table, trainerCount, trainerId, &trainer)
     || !ADMG_TryGetFacilityMonTableCount(selectedMons, &monCount)
     || !ADMG_TryGetFacilityMonSetFromSpan(trainer->monSet, extents[trainerId], monCount, &selectedSet, &selectedCount)) return FALSE;
    *mons = selectedMons; *set = selectedSet; *count = selectedCount;
    return TRUE;
}

bool8 ADMG_TryGetFacilityMonTableCount(const struct TrainerMon *table, u32 *count)
{
    if (count == NULL) { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    u32 selected;
    if (table == gBattleFrontierMons) selected = ARRAY_COUNT(gBattleFrontierMons);
    else if (table == gSlateportBattleTentMons) selected = ARRAY_COUNT(gSlateportBattleTentMons);
    else if (table == gVerdanturfBattleTentMons) selected = ARRAY_COUNT(gVerdanturfBattleTentMons);
    else if (table == gFallarborBattleTentMons) selected = ARRAY_COUNT(gFallarborBattleTentMons);
    else { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    *count = selected;
    return TRUE;
}

bool8 ADMG_TryGetFacilityMonRowFromTable(const struct TrainerMon *table, u32 count,
    u32 monId, const struct TrainerMon **mon)
{
    if (table == NULL || mon == NULL || monId >= count)
    { ADMG_StateAccessRejected(monId, __func__); return FALSE; }
    *mon = &table[monId];
    return TRUE;
}

bool8 ADMG_TryGetFacilityTrainerRowFromTable(const struct BattleFrontierTrainer *table,
    u32 count, u32 trainerId, const struct BattleFrontierTrainer **trainer)
{
    if (table == NULL || trainer == NULL || trainerId >= count)
    { ADMG_StateAccessRejected(trainerId, __func__); return FALSE; }
    *trainer = &table[trainerId];
    return TRUE;
}

bool8 ADMG_TryGetActiveFacilityTrainerRow(u32 trainerId, const struct BattleFrontierTrainer **trainer)
{
    const struct BattleFrontierTrainer *table = gFacilityTrainers;
    u32 count;
    /* Built-in selected-table extents come from the compiled definitions.
     * The explicit table/count interface also admits other declared spans;
     * an unknown global pointer alone supplies no authoritative extent. */
    if (table == gBattleFrontierTrainers) count = ARRAY_COUNT(gBattleFrontierTrainers);
    else if (table == gSlateportBattleTentTrainers) count = ARRAY_COUNT(gSlateportBattleTentTrainers);
    else if (table == gVerdanturfBattleTentTrainers) count = ARRAY_COUNT(gVerdanturfBattleTentTrainers);
    else if (table == gFallarborBattleTentTrainers) count = ARRAY_COUNT(gFallarborBattleTentTrainers);
    else { ADMG_StateAccessRejected(trainerId, __func__); return FALSE; }
    return ADMG_TryGetFacilityTrainerRowFromTable(table, count, trainerId, trainer);
}

bool8 ADMG_TrySelectFacilityTrainerTables(bool8 tent,
    const struct BattleFrontierTrainer **trainers, const struct TrainerMon **mons, u32 *count)
{
    if (trainers == NULL || mons == NULL || count == NULL)
    { ADMG_StateAccessRejected(0, __func__); return FALSE; }
    const struct BattleFrontierTrainer *selected = gBattleFrontierTrainers;
    const struct TrainerMon *selectedMons = gBattleFrontierMons;
    u32 selectedCount = FRONTIER_TRAINERS_COUNT;
    if (tent)
    {
        u64 generation = ADMG_BeginStateAccessCheck();
        u32 facility = VarGet(VAR_FRONTIER_FACILITY);
        if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
        if (facility == FRONTIER_FACILITY_FACTORY)
        { selected = gSlateportBattleTentTrainers; selectedMons = gSlateportBattleTentMons; }
        else if (facility == FRONTIER_FACILITY_PALACE)
        { selected = gVerdanturfBattleTentTrainers; selectedMons = gVerdanturfBattleTentMons; }
        else if (facility == FRONTIER_FACILITY_ARENA)
        { selected = gFallarborBattleTentTrainers; selectedMons = gFallarborBattleTentMons; }
        /* Preserve the original default: any other facility selects Frontier. */
        if (selected != gBattleFrontierTrainers) selectedCount = NUM_BATTLE_TENT_TRAINERS;
    }
    *trainers = selected; *mons = selectedMons; *count = selectedCount;
    return TRUE;
}

u8 SetTentPtrsGetLevel(void)
{
    const struct BattleFrontierTrainer *trainers;
    const struct TrainerMon *mons;
    u32 count;
    if (!ADMG_TrySelectFacilityTrainerTables(TRUE, &trainers, &mons, &count)) return TENT_MIN_LEVEL;
    gFacilityTrainers = trainers; gFacilityTrainerMons = mons;
    u8 level = GetHighestLevelInPlayerParty();
    if (level < TENT_MIN_LEVEL) level = TENT_MIN_LEVEL;
    return level;
}

static void SetNextBattleTentOpponent(void)
{
    s32 i;
    u16 trainerId;

    do
    {
        trainerId = GetBattleTentTrainerId();
        for (i = 0; i < gSaveBlock2Ptr->frontier.curChallengeBattleNum; i++)
        {
            if (gSaveBlock2Ptr->frontier.trainerIds[i] == trainerId)
                break;
        }
    } while (i != gSaveBlock2Ptr->frontier.curChallengeBattleNum);

    TRAINER_BATTLE_PARAM.opponentA = trainerId;
    SetBattleFacilityTrainerGfxId(TRAINER_BATTLE_PARAM.opponentA, 0);
    if (gSaveBlock2Ptr->frontier.curChallengeBattleNum + 1 < TENT_STAGES_PER_CHALLENGE)
       gSaveBlock2Ptr->frontier.trainerIds[gSaveBlock2Ptr->frontier.curChallengeBattleNum] = TRAINER_BATTLE_PARAM.opponentA;
}

ADMG_NATIVE_ENTRY void FillTentTrainerParty_(enum TrainerId trainerId, u8 firstMonId, u8 monCount)
{
    /* This original private entry reads the opponent-A transport authority;
     * its trainerId argument was unused and stays non-authoritative. */
    ADMG_FillFacilityPartyFromArgs(trainerId, B_TRAINER_OPPONENT_A, monCount, TRUE, firstMonId);
}

void TrySetLinkBattleTowerEnemyPartyLevel(void)
{
    if (!TESTING)
    {
        if (gBattleTypeFlags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK))
        {
            u32 i;
            u8 enemyLevel = SetFacilityPtrsGetLevel();

            for (i = 0; i < PARTY_SIZE; i++)
            {
                enum Species species = GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_SPECIES);
                if (species)
                {
                    SetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_EXP, &gExperienceTables[gSpeciesInfo[species].growthRate][enemyLevel]);
                    CalculateMonStats(&gParties[B_TRAINER_OPPONENT_A][i]);
                }
            }
            for (i = 0; i < PARTY_SIZE; i++)
            {
                enum Species species = GetMonData(&gParties[B_TRAINER_OPPONENT_B][i], MON_DATA_SPECIES);
                if (species)
                {
                    SetMonData(&gParties[B_TRAINER_OPPONENT_B][i], MON_DATA_EXP, &gExperienceTables[gSpeciesInfo[species].growthRate][enemyLevel]);
                    CalculateMonStats(&gParties[B_TRAINER_OPPONENT_B][i]);
                }
            }
        }
    }
}
