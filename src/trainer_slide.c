#include "global.h"
#include "admg_state.h"
#include "admg_trainer_actor.h"
#include "battle.h"
#include "battle_ai_util.h"
#include "battle_anim.h"
#include "battle_controllers.h"
#include "battle_message.h"
#include "battle_setup.h"
#include "battle_tower.h"
#include "battle_z_move.h"
#include "data.h"
#include "event_data.h"
#include "frontier_util.h"
#include "graphics.h"
#include "international_string_util.h"
#include "item.h"
#include "link.h"
#include "menu.h"
#include "palette.h"
#include "party_menu.h"
#include "recorded_battle.h"
#include "string_util.h"
#include "strings.h"
#include "test_runner.h"
#include "text.h"
#include "trainer_hill.h"
#include "window.h"
#include "line_break.h"
#include "constants/abilities.h"
#include "constants/battle_dome.h"
#include "constants/battle_string_ids.h"
#include "constants/comparison_operators.h"
#include "constants/flags.h"
#include "constants/frontier_util.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/opponents.h"
#include "constants/species.h"
#include "constants/trainers.h"
#include "constants/trainer_hill.h"
#include "constants/vars.h"
#include "constants/weather.h"
#include "trainer_slide.h"
#include "battle_message.h"
#include "admg_native.h"
#include "admg_registry.h"
#include "admg_retention.h"
#include "admg_trainer.h"

static u32 BattlerHPPercentage(enum BattlerId battler, enum ComparisonOperators operation, u32 threshold);
static u32 GetPartyMonCount(u32 lastId, struct Pokemon *party, bool32 onlyAlive);
static bool32 DoesTrainerHaveSlideMessage(enum DifficultyLevel difficulty, u32 trainerId, u32 slideId);
static bool32 ShouldRunTrainerSlideLandsFirstCriticalHit(enum BattlerId battler, enum TrainerSlideType slideId);
static bool32 ShouldRunTrainerSlideLandsFirstSuperEffectiveHit(enum BattlerId battler, enum TrainerSlideType slideId);
static bool32 ShouldRunTrainerSlideLandsFirstSTABMove(u32 lastId, enum BattlerId battler, enum TrainerSlideType slideId);
static bool32 ShouldRunTrainerSlideLandsFirstDown(u32 lastId, enum BattlerId battler, enum TrainerSlideType slideId);
static bool32 ShouldRunTrainerSlideMonUnaffected(u32 lastId, enum BattlerId battler, enum TrainerSlideType slideId);
static bool32 ShouldRunTrainerSlideLastSwitchIn(enum BattlerId battler, enum TrainerSlideType slideId);
static bool32 ShouldRunTrainerSlideLastHalfHP(u32 lastId, enum BattlerId battler, enum TrainerSlideType slideId);
static bool32 ShouldRunTrainerSlideLastLowHp(u32 lastId, enum BattlerId battler, enum TrainerSlideType slideId);
static void SetTrainerSlideParameters(enum BattlerId battler, u32* lastId, u32* trainerId, u32* retValue);
static bool32 IsSlideInitalizedOrPlayed(enum BattlerId battler, enum TrainerSlideType slideId);

// Partner trainers must be added as TRAINER_PARTNER(PARTNER_XXXX)
static const u8* const sTrainerSlides[DIFFICULTY_COUNT][TRAINER_PARTNER(PARTNER_COUNT)][TRAINER_SLIDE_COUNT] =
{
    [DIFFICULTY_NORMAL] =
    {
    },
};

/* Canonical Kanto resource owner; this pinned source supplies no slide text.
 * Empty defaults retain the same callable capability without activating it. */
static const u8* const sADMG_KantoTrainerSlides[DIFFICULTY_COUNT][ADMG_KANTO_TRAINERS_COUNT][TRAINER_SLIDE_COUNT] = {0};

static const u8* const sFrontierTrainerSlides[DIFFICULTY_COUNT][FRONTIER_TRAINERS_COUNT][TRAINER_SLIDE_COUNT] =
{
    [DIFFICULTY_NORMAL] =
    {
    },
};

#define TRAINER_RED_TEST    1
#define TRAINER_LEAF_TEST   2
#define PARTNER_STEVEN_TEST 1

static const u8* const sTestTrainerSlides[DIFFICULTY_COUNT][MAX_TRAINERS_COUNT_EMERALD + PARTNER_COUNT][TRAINER_SLIDE_COUNT] =
{
#include "../test/battle/trainer_slides.h"
};

static u32 BattlerHPPercentage(enum BattlerId battler, enum ComparisonOperators operation, u32 threshold)
{
    if ((u32)battler >= MAX_BATTLERS_COUNT || threshold == 0)
    { ADMG_StateAccessRejected((u32)battler, __func__); return FALSE; }
    switch (operation)
    {
    case LESS_THAN:
        return gBattleMons[battler].hp < (gBattleMons[battler].maxHP / threshold);
    case EQUAL:
        return gBattleMons[battler].hp == (gBattleMons[battler].maxHP / threshold);
    case GREATER_THAN:
        return gBattleMons[battler].hp > (gBattleMons[battler].maxHP / threshold);
    case LESS_THAN_OR_EQUAL:
        return gBattleMons[battler].hp <= (gBattleMons[battler].maxHP / threshold);
    case GREATER_THAN_OR_EQUAL:
        return gBattleMons[battler].hp >= (gBattleMons[battler].maxHP / threshold);
    case NOT_EQUAL:
    default:
        return gBattleMons[battler].hp != (gBattleMons[battler].maxHP / threshold);
    }
}

static const s8 sMultiBattleOrder[] = {0, 2, 3, 1, 4, 5};

static u32 GetPartyMonCount(u32 lastId, struct Pokemon *party, bool32 onlyAlive)
{
    if (lastId > PARTY_SIZE || (lastId != 0 && party == NULL))
    { ADMG_StateAccessRejected(lastId, __func__); return 0; }
    u32 count = 0;

    for (u32 i = 0; i < lastId; i++)
    {
        enum Species species = GetMonData(&party[i], MON_DATA_SPECIES_OR_EGG);
        if (species != SPECIES_NONE
                && species != SPECIES_EGG
                && (!onlyAlive || GetMonData(&party[i], MON_DATA_HP)))
        {
            count++;
        }
    }

    return count;
}

static bool8 ADMG_TrainerSlideResourceBounds(enum DifficultyLevel difficulty, u32 trainerId, u32 slideId)
{
    if ((u32)difficulty >= DIFFICULTY_COUNT || slideId >= TRAINER_SLIDE_COUNT || trainerId > 0xFFFF)
    { ADMG_StateAccessRejected(trainerId, __func__); return FALSE; }
    return TRUE;
}

static bool8 ADMG_TrainerSlideBorrowCurrent(void);

static bool8 ADMG_TrainerSlideLookupCurrent(u32 flags, u64 generation)
{
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gBattleTypeFlags != flags)
    { ADMG_StateAccessRejected(flags, __func__); return FALSE; }
    return ADMG_TrainerSlideBorrowCurrent();
}

/* Stack-borrowed actor authority; no save or persistent state. */
struct ADMG_TrainerSlideActor
{
    struct ADMG_TrainerActorRef actor;
    u32 flags, domain, role;
    struct BattleStruct *battle;
    u32 battler, position, controller, target;
    bool8 debug;
};
static const struct ADMG_TrainerSlideActor *sADMG_BorrowedSlideActor;

static bool8 ADMG_TrainerSlideBorrowCurrent(void)
{
    const struct ADMG_TrainerSlideActor *input = sADMG_BorrowedSlideActor;
    if (input == NULL) return TRUE;
    if (gBattleStruct != input->battle || gBattleTypeFlags != input->flags
        || gBattlerPositions[input->battler] != input->position
        || gBattlerBattleController[input->battler] != input->controller
        || gIsDebugBattle != input->debug || gBattleScripting.battler != input->battler
        || (input->target == TRAINER_SLIDE_TARGET_TRAINER_A && TRAINER_BATTLE_PARAM.opponentA != input->actor.identifier)
        || (input->target == TRAINER_SLIDE_TARGET_TRAINER_B && TRAINER_BATTLE_PARAM.opponentB != input->actor.identifier)
        || (input->target == TRAINER_SLIDE_TARGET_TRAINER_PARTNER && gPartnerTrainerId != input->actor.identifier))
    { ADMG_StateAccessRejected(input->actor.identifier, __func__); return FALSE; }
    return TRUE;
}

static bool8 ADMG_TryBorrowedTrainerSlideArray(enum DifficultyLevel difficulty, u32 identity,
    const u8* const **row)
{
    const struct ADMG_TrainerSlideActor *input = sADMG_BorrowedSlideActor;
    if (input == NULL || input->actor.identifier != identity) return FALSE;
    *row = NULL;
    u64 generation = ADMG_BeginStateAccessCheck();
    if (!ADMG_TrainerSlideLookupCurrent(input->flags, generation)) return TRUE;
    u32 index = input->actor.index;
    switch (input->actor.kind)
    {
    case ADMG_ACTOR_HOENN:
        if (index >= TRAINERS_COUNT_EMERALD || identity != index) goto ADMG_reject;
        break;
    case ADMG_ACTOR_NAMED_PARTNER:
        if (index == PARTNER_NONE || index >= PARTNER_COUNT
            || identity != MAX_TRAINERS_COUNT_EMERALD + index) goto ADMG_reject;
        break;
    case ADMG_ACTOR_KANTO:
        if (index == 0 || index >= ARRAY_COUNT(sADMG_KantoTrainerSlides[0])
            || identity != ADMG_KANTO_TRAINER_FIRST + index) goto ADMG_reject;
        break;
    case ADMG_ACTOR_FRONTIER:
        if (index >= FRONTIER_TRAINERS_COUNT || identity != index) goto ADMG_reject;
        break;
    default:
        return TRUE; /* No ordinary resource row for saved/special descriptors. */
    }
#if TESTING
    bool8 enabled = FlagGet(TESTING_FLAG_TRAINER_SLIDES);
    if (!ADMG_TrainerSlideLookupCurrent(input->flags, generation) || !enabled) return TRUE;
    if (input->actor.kind == ADMG_ACTOR_KANTO) *row = sADMG_KantoTrainerSlides[difficulty][index];
    else if (identity < ARRAY_COUNT(sTestTrainerSlides[0])) *row = sTestTrainerSlides[difficulty][identity];
#else
    if (input->actor.kind == ADMG_ACTOR_KANTO) *row = sADMG_KantoTrainerSlides[difficulty][index];
    else if (input->actor.kind == ADMG_ACTOR_FRONTIER)
    {
        if (!(input->role == ADMG_ACTOR_OPPONENT && input->flags & BATTLE_TYPE_TRAINER_HILL))
            *row = sFrontierTrainerSlides[difficulty][index];
    }
    else if (identity < ARRAY_COUNT(sTrainerSlides[0])) *row = sTrainerSlides[difficulty][identity];
    else goto ADMG_reject;
#endif
    return TRUE;
ADMG_reject:
    ADMG_StateAccessRejected(identity, __func__);
    return TRUE;
}

static const u8* const *GetTrainerSlideArray(enum DifficultyLevel difficulty, u32 trainerId, u32 slideId)
{
    if (!ADMG_TrainerSlideResourceBounds(difficulty, trainerId, slideId)) return NULL;
    const u8* const *borrowed;
    if (ADMG_TryBorrowedTrainerSlideArray(difficulty, trainerId, &borrowed)) return borrowed;
    u64 generation = ADMG_BeginStateAccessCheck();
    u32 flags = gBattleTypeFlags;
    bool8 facility = (flags & BATTLE_TYPE_FRONTIER) != 0;
    bool8 kanto = trainerId >= ADMG_KANTO_TRAINER_FIRST
        && trainerId < ADMG_KANTO_TRAINER_FIRST + ADMG_KANTO_TRAINERS_COUNT;
    bool8 named = IsPartnerTrainerId(trainerId);
    if (facility && !kanto && !named)
    {
        struct ADMG_TrainerActorRef actor;
        if (!ADMG_TryClassifyTrainerActor(trainerId, ADMG_ACTOR_FACILITY, ADMG_ACTOR_OPPONENT, &actor)) return NULL;
#if !TESTING
        /* Saved friends/apprentices and special descriptors have no row in the
         * original ordinary Frontier slide table. Their absence is not a row. */
        if (actor.kind != ADMG_ACTOR_FRONTIER) return NULL;
#endif
    }
    else
    {
        struct ADMG_TrainerRef ref;
        if (!ADMG_TryResolveTrainer(trainerId, &ref)) return NULL;
        // Added trainers have no compiled slide row; return NULL without reporting a rejection
        if (ref.owner == ADMG_TRAINER_ADDED) return NULL;
        if (ref.owner == ADMG_TRAINER_KANTO)
        {
            if (ref.nativeIndex >= ARRAY_COUNT(sADMG_KantoTrainerSlides[0]))
            { ADMG_StateAccessRejected(trainerId, __func__); return NULL; }
            if (!ADMG_TrainerSlideLookupCurrent(flags, generation)) return NULL;
#if TESTING
            bool8 enabled = FlagGet(TESTING_FLAG_TRAINER_SLIDES);
            if (!ADMG_TrainerSlideLookupCurrent(flags, generation) || !enabled) return NULL;
#endif
            return sADMG_KantoTrainerSlides[difficulty][ref.nativeIndex];
        }
    }
    if (!ADMG_TrainerSlideLookupCurrent(flags, generation)) return NULL;
#if TESTING
    if (trainerId >= ARRAY_COUNT(sTestTrainerSlides[0])) return NULL;
    bool8 enabled = FlagGet(TESTING_FLAG_TRAINER_SLIDES);
    if (!ADMG_TrainerSlideLookupCurrent(flags, generation)) return NULL;
    return enabled ? sTestTrainerSlides[difficulty][trainerId] : NULL;
#else
    if (facility && !named)
    {
        if (trainerId >= ARRAY_COUNT(sFrontierTrainerSlides[0]))
        { ADMG_StateAccessRejected(trainerId, __func__); return NULL; }
        return sFrontierTrainerSlides[difficulty][trainerId];
    }
    if (trainerId >= ARRAY_COUNT(sTrainerSlides[0]))
    { ADMG_StateAccessRejected(trainerId, __func__); return NULL; }
    return sTrainerSlides[difficulty][trainerId];
#endif
}

// Hook for registered trainer slide text; return NULL to use the compiled tables
// ADMG_RunTrainerSlide sets gBattleScripting.battler, which is passed here as battler
// For TRAINER_SLIDE_ADMG_CUSTOM, the provider decides whether its condition holds this turn
ADMG_NATIVE_ENTRY const u8 *ADMG_GetRegisteredTrainerSlide(ADMG_Word difficulty, ADMG_Word trainerId, ADMG_Word slideId, ADMG_Word battler)
{
    return NULL;
}

static const u8 *ADMG_RegisteredTrainerSlide(enum DifficultyLevel difficulty, u32 trainerId, u32 slideId)
{
    u64 generation = ADMG_BeginStateAccessCheck();
    u32 flags = gBattleTypeFlags;
    const u8 *text = ADMG_GetRegisteredTrainerSlide(difficulty, trainerId, slideId, gBattleScripting.battler);
    if (text == NULL) return NULL;
    if (!ADMG_EndStateAccessCheck(generation) || gBattleTypeFlags != flags
     || !(((u32)text >= ROM_START && (u32)text < ROM_END) || ADMG_IsNativeAddress((u32)text)))
    { ADMG_StateAccessRejected(trainerId, __func__); return NULL; }
    return text;
}

static bool32 DoesTrainerHaveSlideMessage(enum DifficultyLevel difficulty, u32 trainerId, u32 slideId)
{
    if (!ADMG_TrainerSlideResourceBounds(difficulty, trainerId, slideId)) return FALSE;
    if (ADMG_RegisteredTrainerSlide(difficulty, trainerId, slideId) != NULL) return TRUE;
    u64 generation = ADMG_BeginStateAccessCheck();
    u32 flags = gBattleTypeFlags;
    const u8* const *trainerSlides = GetTrainerSlideArray(difficulty, trainerId, slideId);
    if (!ADMG_TrainerSlideLookupCurrent(flags, generation)) return FALSE;
    const u8* const *trainerSlidesNormal = GetTrainerSlideArray(DIFFICULTY_NORMAL, trainerId, slideId);
    if (!ADMG_TrainerSlideLookupCurrent(flags, generation)) return FALSE;
#if TESTING
    if (VarGet(TESTING_VAR_TRAINER_SLIDES) != slideId || !ADMG_TrainerSlideLookupCurrent(flags, generation)) return FALSE;
#endif
    return (trainerSlides != NULL && trainerSlides[slideId] != NULL)
        || (trainerSlidesNormal != NULL && trainerSlidesNormal[slideId] != NULL);
}

void SetTrainerSlideMessage(enum DifficultyLevel difficulty, u32 trainerId, u32 slideId)
{
    if (!ADMG_TrainerSlideResourceBounds(difficulty, trainerId, slideId)) return;
    struct BattleStruct *battle = gBattleStruct;
    u32 flags = gBattleTypeFlags;
    if (battle == NULL) { ADMG_StateAccessRejected(trainerId, __func__); return; }
    const u8 *registered = ADMG_RegisteredTrainerSlide(difficulty, trainerId, slideId);
    if (registered != NULL)
    {
        if (gBattleStruct != battle) { ADMG_StateAccessRejected(trainerId, __func__); return; }
        battle->trainerSlideMsg = registered;
        return;
    }
    u64 generation = ADMG_BeginStateAccessCheck();
    const u8* const *trainerSlides = GetTrainerSlideArray(difficulty, trainerId, slideId);
    if (!ADMG_TrainerSlideLookupCurrent(flags, generation)) return;
    if (gBattleStruct != battle)
    { ADMG_StateAccessRejected(trainerId, __func__); return; }
    const u8* const *trainerSlidesNormal = GetTrainerSlideArray(DIFFICULTY_NORMAL, trainerId, slideId);
    if (!ADMG_TrainerSlideLookupCurrent(flags, generation)) return;
    if (gBattleStruct != battle || gBattleTypeFlags != flags)
    { ADMG_StateAccessRejected(trainerId, __func__); return; }
    if (trainerSlides != NULL && trainerSlides[slideId] != NULL)
        battle->trainerSlideMsg = trainerSlides[slideId];
    else
        battle->trainerSlideMsg = trainerSlidesNormal != NULL ? trainerSlidesNormal[slideId] : NULL;
}

static bool32 ShouldRunTrainerSlideLandsFirstCriticalHit(enum BattlerId battler, enum TrainerSlideType slideId)
{
    return IsTrainerSlideInitialized(battler, slideId);
}

static bool32 ShouldRunTrainerSlideLandsFirstSuperEffectiveHit(enum BattlerId battler, enum TrainerSlideType slideId)
{
    if (slideId == TRAINER_SLIDE_DEFENDER_TAKES_FIRST_SUPER_EFFECTIVE_HIT)
    {
        if (GetBattlerSide(battler) == GetBattlerSide(gBattlerAttacker))
            return FALSE;
    }
    else
    {
        if (GetBattlerSide(battler) != GetBattlerSide(gBattlerAttacker))
            return FALSE;
    }

    if (!IsTrainerSlideInitialized(battler, slideId))
        return FALSE;

    if (!IsBattlerAlive(battler))
        return FALSE;

    return TRUE;
}

static bool32 ShouldRunTrainerSlideLandsFirstSTABMove(u32 lastId, enum BattlerId battler, enum TrainerSlideType slideId)
{
    if (!IsTrainerSlideInitialized(battler, slideId))
        return FALSE;

    if (GetPartyMonCount(lastId, GetBattlerParty(battler), TRUE) != GetPartyMonCount(lastId, GetBattlerParty(battler), FALSE))
        return FALSE;

    return TRUE;
}

static bool32 ShouldRunTrainerSlideLandsFirstDown(u32 lastId, enum BattlerId battler, enum TrainerSlideType slideId)
{
    if (gBattlerFainted == GetPartnerBattler(battler))
        return FALSE;

    enum BattlerId partyBattler = battler;

    if (slideId == TRAINER_SLIDE_ATTACKER_LANDS_FIRST_DOWN)
        partyBattler = gBattlerFainted;

    return ((GetPartyMonCount(lastId, GetBattlerParty(partyBattler), TRUE) == (GetPartyMonCount(lastId, GetBattlerParty(partyBattler), FALSE) - 1)));
}

static bool32 ShouldRunTrainerSlideMonUnaffected(u32 lastId, enum BattlerId battler, enum TrainerSlideType slideId)
{
    if (!IsTrainerSlideInitialized(battler, slideId))
        return FALSE;

    return (GetPartyMonCount(lastId, GetBattlerParty(battler), TRUE) == GetPartyMonCount(lastId, GetBattlerParty(battler), FALSE));
}

static bool32 ShouldRunTrainerSlideLastSwitchIn(enum BattlerId battler, enum TrainerSlideType slideId)
{
    if (slideId == TRAINER_SLIDE_OPPONENT_LAST_SWITCHIN)
    {
        enum BattlerId oppositeBattler = GetOppositeBattler(battler);
        enum BattlerId oppositePartner = IsDoubleBattle() ? GetPartnerBattler(oppositeBattler) : oppositeBattler;

        return (CountUsablePartyMons(oppositeBattler) == 0 && CountUsablePartyMons(oppositePartner) == 0);
    }
    return !CanBattlerSwitch(battler);
}

static bool32 ShouldRunTrainerSlideLastHalfHP(u32 lastId, enum BattlerId battler, enum TrainerSlideType slideId)
{
    if (IsSlideInitalizedOrPlayed(battler, slideId))
        return FALSE;

    if (slideId == TRAINER_SLIDE_OPPONENT_LAST_HALF_HP)
    {
        enum BattlerId oppositeBattler = GetOppositeBattler(battler);
        enum BattlerId oppositePartner = GetPartnerBattler(oppositeBattler);

        bool32 oppositeLastMon = GetPartyMonCount(lastId, GetBattlerParty(oppositeBattler), TRUE) == 1;
        bool32 partnerLastMon = GetPartyMonCount(lastId, GetBattlerParty(oppositePartner), TRUE) == 1;

        if (!(oppositeLastMon || partnerLastMon))
            return FALSE;

        return ((oppositeLastMon && BattlerHPPercentage(oppositeBattler, GREATER_THAN, 4) && BattlerHPPercentage(oppositeBattler, LESS_THAN_OR_EQUAL, 2))
         || (partnerLastMon && BattlerHPPercentage(oppositePartner, GREATER_THAN, 4) && BattlerHPPercentage(oppositePartner, LESS_THAN_OR_EQUAL, 2)));
    }
    else
    {
        if (GetPartyMonCount(lastId, GetBattlerParty(battler), TRUE) != 1)
            return FALSE;

        if (BattlerHPPercentage(battler, GREATER_THAN, 2))
            return FALSE;

        return (BattlerHPPercentage(battler, GREATER_THAN, 4));
    }
}

static bool32 ShouldRunTrainerSlideLastLowHp(u32 lastId, enum BattlerId battler, enum TrainerSlideType slideId)
{
    if (IsSlideInitalizedOrPlayed(battler, slideId))
        return FALSE;

    if (slideId == TRAINER_SLIDE_OPPONENT_LAST_LOW_HP)
    {
        enum BattlerId oppositeBattler = GetOppositeBattler(battler);
        enum BattlerId oppositePartner = GetPartnerBattler(oppositeBattler);

        bool32 oppositeLastMon = GetPartyMonCount(lastId, GetBattlerParty(oppositeBattler), TRUE) == 1;
        bool32 partnerLastMon = GetPartyMonCount(lastId, GetBattlerParty(oppositePartner), TRUE) == 1;

        if (!(oppositeLastMon || partnerLastMon))
            return FALSE;

        if (!IsBattlerAlive(battler))
            return FALSE;

        return ((oppositeLastMon && BattlerHPPercentage(oppositeBattler, LESS_THAN_OR_EQUAL, 4))
         || (partnerLastMon && BattlerHPPercentage(oppositePartner, LESS_THAN_OR_EQUAL, 4)));
    }
    else
    {
        if (GetPartyMonCount(lastId, GetBattlerParty(battler), TRUE) != 1)
            return FALSE;

        if (!IsBattlerAlive(battler))
            return FALSE;

        return (BattlerHPPercentage(battler, LESS_THAN_OR_EQUAL, 4));
    }
}

static void SetTrainerSlideParameters(enum BattlerId battler, u32* lastId, u32* trainerId, u32* retValue)
{
    if ((u32)battler >= MAX_BATTLERS_COUNT || lastId == NULL || trainerId == NULL || retValue == NULL)
    { ADMG_StateAccessRejected((u32)battler, __func__); return; }
    if ((battler & BIT_SIDE) == B_SIDE_OPPONENT)
    {
        if (gBattleTypeFlags & BATTLE_TYPE_TWO_OPPONENTS)
        {
            if (!AreMultiPartiesFullTeams())
                *lastId = MULTI_PARTY_SIZE;
            if (GetBattlerTrainer(battler) == B_TRAINER_OPPONENT_B)
            {
                *trainerId = TRAINER_BATTLE_PARAM.opponentB;
                *retValue = TRAINER_SLIDE_TARGET_TRAINER_B;
            }
        }
    }
    else if (GetBattlerTrainer(battler) == B_TRAINER_PARTNER && gBattleTypeFlags & BATTLE_TYPE_INGAME_PARTNER)
    {
        if (!AreMultiPartiesFullTeams())
            *lastId = MULTI_PARTY_SIZE;
        *trainerId = gPartnerTrainerId;
        *retValue = TRAINER_SLIDE_TARGET_TRAINER_PARTNER;
    }
    else if (BattlerIsPlayer(battler))
    {
        *trainerId = TRAINER_NONE;
        *retValue = TRAINER_SLIDE_TARGET_NONE;
    }
}

static bool8 ADMG_TrainerSlideStatusBounds(enum BattlerId battler, enum TrainerSlideType slideId);

static bool8 ADMG_TrainerSlideCallerCurrent(enum BattlerId battler, struct BattleStruct *battle,
    u32 flags, u32 position, u32 controller, u32 identity, u32 target, u64 generation,
    const struct ADMG_TrainerSlideActor *scope, bool8 published, bool8 debug, u32 count)
{
    if (gBattleStruct != battle || gBattleTypeFlags != flags
        || gBattlerPositions[battler] != position || gBattlerBattleController[battler] != controller
        || gIsDebugBattle != debug || sADMG_BorrowedSlideActor != scope
        || (published && gBattleScripting.battler != battler)
        || (count <= MAX_BATTLERS_COUNT && gBattlersCount != count)
        || target > TRAINER_SLIDE_TARGET_TRAINER_PARTNER
        || (target == TRAINER_SLIDE_TARGET_TRAINER_A && TRAINER_BATTLE_PARAM.opponentA != identity)
        || (target == TRAINER_SLIDE_TARGET_TRAINER_B && TRAINER_BATTLE_PARAM.opponentB != identity)
        || (target == TRAINER_SLIDE_TARGET_TRAINER_PARTNER && gPartnerTrainerId != identity))
    { ADMG_StateAccessRejected(identity, __func__); return FALSE; }
    return ADMG_EndStateAccessCheck(generation);
}

static enum TrainerSlideTargets ADMG_RunTrainerSlide(enum BattlerId battler, enum TrainerSlideType slideId, struct ADMG_TrainerSlideActor *owner)
{
    if ((u32)battler >= MAX_BATTLERS_COUNT)
    { ADMG_StateAccessRejected((u32)battler, __func__); return TRAINER_SLIDE_TARGET_NONE; }
    u64 generation = ADMG_BeginStateAccessCheck();
    const struct ADMG_TrainerSlideActor *scope = sADMG_BorrowedSlideActor;
    bool8 published = FALSE, debug = gIsDebugBattle;
    u32 count = MAX_BATTLERS_COUNT + 1;
    u32 flags = gBattleTypeFlags;
    u32 position = gBattlerPositions[battler];
    u32 controller = gBattlerBattleController[battler];
    /* Upstream answers these without reading a position. A single battle
     * leaves battlers 2 and 3 absent (position 0xFF); they return here with
     * no rejection. Every battler that continues is still checked below. */
    if (!(gBattleTypeFlags & BATTLE_TYPE_TRAINER))
        return TRAINER_SLIDE_TARGET_NONE;
    if (!IsDoubleBattle() && (battler > B_BATTLER_1))
        return TRAINER_SLIDE_TARGET_NONE;
    if (position >= MAX_BATTLERS_COUNT)
    { ADMG_StateAccessRejected(position, __func__); return TRAINER_SLIDE_TARGET_NONE; }
    if (BattlerIsPlayer(battler))
        return TRAINER_SLIDE_TARGET_NONE;

    u32 lastId = PARTY_SIZE, trainerId = TRAINER_BATTLE_PARAM.opponentA;
    u32 retValue = TRAINER_SLIDE_TARGET_TRAINER_A;
    bool32 shouldRun = FALSE;

    if (GetBattlerTrainer(battler) == B_TRAINER_PLAYER)
        return TRAINER_SLIDE_TARGET_NONE;

    SetTrainerSlideParameters(battler, &lastId, &trainerId, &retValue);
    if (IsSpecialTrainer(trainerId))
        return TRAINER_SLIDE_TARGET_NONE;

    if (!ADMG_TrainerSlideStatusBounds(battler, slideId)) return TRAINER_SLIDE_TARGET_NONE;
    struct BattleStruct *battle = gBattleStruct;
    if (lastId > PARTY_SIZE)
    { ADMG_StateAccessRejected(lastId, __func__); return TRAINER_SLIDE_TARGET_NONE; }
    enum DifficultyLevel difficulty = GetCurrentDifficultyLevel();
    if (!ADMG_TrainerSlideCallerCurrent(battler, battle, flags, position, controller,
        trainerId, retValue, generation, scope, published, debug, count)) return TRAINER_SLIDE_TARGET_NONE;
    u32 previousBattler = gBattleScripting.battler;
    u32 statusIndex = (u32)slideId / TRAINER_SLIDES_PER_ARRAY;
    u32 statusMask = 1u << ((u32)slideId % TRAINER_SLIDES_PER_ARRAY);
    u32 previousOwnBit = battle->slideMessageStatus.messagePlayed[battler][statusIndex] & statusMask;
    u32 partnerBattler = 0, previousPartnerBit = 0;
    bool8 markedOwn = FALSE, markedPartner = FALSE, messageTouched = FALSE;
    const u8 *previousMessage = battle->trainerSlideMsg;
    /* Network actor words remain opaque transport; no content row is inferred
     * from a coincidentally equal person/recorded-link word. Explicit resource
     * queries and existing named hooks remain available to content callers. */
    if (flags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK | BATTLE_TYPE_TOWER_LINK_MULTI))
    {
        gBattleScripting.battler = battler;
        return TRAINER_SLIDE_TARGET_NONE;
    }
    // Ignore the played flag for custom slides so the provider can check its condition again
    bool8 played = slideId == TRAINER_SLIDE_ADMG_CUSTOM ? FALSE : IsTrainerSlidePlayed(battler, slideId);
    if (!ADMG_TrainerSlideCallerCurrent(battler, battle, flags, position, controller,
        trainerId, retValue, generation, scope, published, debug, count)) return TRAINER_SLIDE_TARGET_NONE;
    if (!played && retValue != TRAINER_SLIDE_TARGET_NONE)
    {
        if (!ADMG_TrainerSlideResourceBounds(difficulty, trainerId, slideId)) return TRAINER_SLIDE_TARGET_NONE;
        owner->flags = flags;
        owner->battle = battle; owner->battler = battler; owner->position = position;
        owner->controller = controller; owner->target = retValue; owner->debug = debug;
        owner->role = retValue == TRAINER_SLIDE_TARGET_TRAINER_PARTNER ? ADMG_ACTOR_PARTNER : ADMG_ACTOR_OPPONENT;
        owner->domain = owner->role == ADMG_ACTOR_PARTNER
            ? (IsAiVsAiBattle() ? ADMG_ACTOR_CONTENT : ADMG_ACTOR_FACILITY)
            : flags & (BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_HILL) ? ADMG_ACTOR_FACILITY : ADMG_ACTOR_CONTENT;
        if (owner->role == ADMG_ACTOR_OPPONENT && flags & BATTLE_TYPE_TRAINER_HILL
            && (trainerId == 0 || trainerId > HILL_TRAINERS_PER_FLOOR))
        { ADMG_StateAccessRejected(trainerId, __func__); return TRAINER_SLIDE_TARGET_NONE; }
        if (!ADMG_TryClassifyTrainerActor(trainerId, owner->domain, owner->role, &owner->actor)
            || !ADMG_TrainerSlideCallerCurrent(battler, battle, flags, position, controller,
                trainerId, retValue, generation, scope, published, debug, count)) return TRAINER_SLIDE_TARGET_NONE;
        scope = owner;
        sADMG_BorrowedSlideActor = scope;
    }
    gBattleScripting.battler = battler;
    published = TRUE;
    bool8 hasMessage = played || retValue == TRAINER_SLIDE_TARGET_NONE ? FALSE : DoesTrainerHaveSlideMessage(difficulty, trainerId, slideId);
    if (!ADMG_TrainerSlideCallerCurrent(battler, battle, flags, position, controller,
        trainerId, retValue, generation, scope, published, debug, count)) goto ADMG_reject;

    if (played || !hasMessage) return TRAINER_SLIDE_TARGET_NONE;
    count = gBattlersCount;
    if (count > MAX_BATTLERS_COUNT)
    { ADMG_StateAccessRejected(count, __func__); goto ADMG_reject; }
    if ((slideId == TRAINER_SLIDE_DEFENDER_TAKES_FIRST_SUPER_EFFECTIVE_HIT
          || slideId == TRAINER_SLIDE_ATTACKER_LANDS_FIRST_SUPER_EFFECTIVE_HIT)
        && (u32)gBattlerAttacker >= MAX_BATTLERS_COUNT)
    { ADMG_StateAccessRejected((u32)gBattlerAttacker, __func__); goto ADMG_reject; }
    if ((slideId == TRAINER_SLIDE_DEFENDER_TAKES_FIRST_DOWN
          || slideId == TRAINER_SLIDE_ATTACKER_LANDS_FIRST_DOWN)
        && (u32)gBattlerFainted >= MAX_BATTLERS_COUNT)
    { ADMG_StateAccessRejected((u32)gBattlerFainted, __func__); goto ADMG_reject; }

    switch (slideId)
    {
        case TRAINER_SLIDE_DEFENDER_TAKES_FIRST_CRITICAL_HIT:
        case TRAINER_SLIDE_ATTACKER_LANDS_FIRST_CRITICAL_HIT:
            shouldRun = ShouldRunTrainerSlideLandsFirstCriticalHit(battler, slideId);
            break;
        case TRAINER_SLIDE_DEFENDER_TAKES_FIRST_SUPER_EFFECTIVE_HIT:
        case TRAINER_SLIDE_ATTACKER_LANDS_FIRST_SUPER_EFFECTIVE_HIT:
            shouldRun = ShouldRunTrainerSlideLandsFirstSuperEffectiveHit(battler, slideId);
            break;
        case TRAINER_SLIDE_DEFENDER_TAKES_FIRST_STAB_MOVE:
        case TRAINER_SLIDE_ATTACKER_LANDS_FIRST_STAB_MOVE:
            shouldRun = ShouldRunTrainerSlideLandsFirstSTABMove(lastId, battler, slideId);
            break;
        case TRAINER_SLIDE_DEFENDER_TAKES_FIRST_DOWN:
        case TRAINER_SLIDE_ATTACKER_LANDS_FIRST_DOWN:
            shouldRun = ShouldRunTrainerSlideLandsFirstDown(lastId, battler, slideId);
            break;
        case TRAINER_SLIDE_OPPONENT_MON_UNAFFECTED:
        case TRAINER_SLIDE_SELF_MON_UNAFFECTED:
            shouldRun = ShouldRunTrainerSlideMonUnaffected(lastId, battler, slideId);
            break;
        case TRAINER_SLIDE_OPPONENT_LAST_SWITCHIN:
        case TRAINER_SLIDE_SELF_LAST_SWITCHIN:
            shouldRun = ShouldRunTrainerSlideLastSwitchIn(battler, slideId);
            break;
        case TRAINER_SLIDE_OPPONENT_LAST_HALF_HP:
        case TRAINER_SLIDE_SELF_LAST_HALF_HP:
            shouldRun = ShouldRunTrainerSlideLastHalfHP(lastId, battler, slideId);
            break;
        case TRAINER_SLIDE_OPPONENT_LAST_LOW_HP:
        case TRAINER_SLIDE_SELF_LAST_LOW_HP:
            shouldRun = ShouldRunTrainerSlideLastLowHp(lastId, battler, slideId);
            break;
        case TRAINER_SLIDE_BEFORE_FIRST_TURN:
        case TRAINER_SLIDE_ATTACKER_MEGA_EVOLUTION:
        case TRAINER_SLIDE_ATTACKER_Z_MOVE:
        case TRAINER_SLIDE_ATTACKER_DYNAMAX:
        case TRAINER_SLIDE_ATTACKER_TERA:
        case TRAINER_SLIDE_OPPONENT_MEGA_EVOLUTION:
        case TRAINER_SLIDE_OPPONENT_Z_MOVE:
        case TRAINER_SLIDE_OPPONENT_DYNAMAX:
        case TRAINER_SLIDE_OPPONENT_TERA:
        case TRAINER_SLIDE_ADMG_CUSTOM: // The provider handles the custom condition
            shouldRun = TRUE;
            break;
        default:
            return TRAINER_SLIDE_TARGET_NONE;
    }

    if (!ADMG_TrainerSlideCallerCurrent(battler, battle, flags, position, controller,
        trainerId, retValue, generation, scope, published, debug, count)) goto ADMG_reject;
    if (shouldRun == FALSE) return TRAINER_SLIDE_TARGET_NONE;

    // Preserve the original single-trainer double marking through named calls.
    partnerBattler = GetPartnerBattler(battler);
    if (partnerBattler >= MAX_BATTLERS_COUNT)
    { ADMG_StateAccessRejected(partnerBattler, __func__); goto ADMG_reject; }
    bool8 shared = GetBattlerTrainer(battler) == GetBattlerTrainer(partnerBattler);
    if (!ADMG_TrainerSlideCallerCurrent(battler, battle, flags, position, controller,
        trainerId, retValue, generation, scope, published, debug, count)) goto ADMG_reject;
    if (shared)
    {
        previousPartnerBit = battle->slideMessageStatus.messagePlayed[partnerBattler][statusIndex] & statusMask;
        markedPartner = TRUE;
        MarkTrainerSlideAsPlayed(partnerBattler, slideId);
        if (!ADMG_TrainerSlideCallerCurrent(battler, battle, flags, position, controller,
            trainerId, retValue, generation, scope, published, debug, count)) goto ADMG_reject;
    }
    markedOwn = TRUE;
    MarkTrainerSlideAsPlayed(battler, slideId);
    if (!ADMG_TrainerSlideCallerCurrent(battler, battle, flags, position, controller,
        trainerId, retValue, generation, scope, published, debug, count)) goto ADMG_reject;
    messageTouched = TRUE;
    SetTrainerSlideMessage(difficulty, trainerId, slideId);
    if (!ADMG_TrainerSlideCallerCurrent(battler, battle, flags, position, controller,
        trainerId, retValue, generation, scope, published, debug, count)) goto ADMG_reject;
    if (battle->trainerSlideMsg == NULL)
    { ADMG_StateAccessRejected(trainerId, __func__); goto ADMG_reject; }
    return retValue;
ADMG_reject:
    if (markedOwn)
        battle->slideMessageStatus.messagePlayed[battler][statusIndex]
            = (battle->slideMessageStatus.messagePlayed[battler][statusIndex] & ~statusMask) | previousOwnBit;
    if (markedPartner)
        battle->slideMessageStatus.messagePlayed[partnerBattler][statusIndex]
            = (battle->slideMessageStatus.messagePlayed[partnerBattler][statusIndex] & ~statusMask) | previousPartnerBit;
    if (messageTouched) battle->trainerSlideMsg = previousMessage;
    gBattleScripting.battler = previousBattler;
    return TRAINER_SLIDE_TARGET_NONE;
}

enum TrainerSlideTargets ShouldDoTrainerSlide(enum BattlerId battler, enum TrainerSlideType slideId)
{
    const struct ADMG_TrainerSlideActor *previous = sADMG_BorrowedSlideActor;
    struct ADMG_TrainerSlideActor owner;
    enum TrainerSlideTargets result = ADMG_RunTrainerSlide(battler, slideId, &owner);
    sADMG_BorrowedSlideActor = previous;
    return result;
}

static bool32 IsSlideInitalizedOrPlayed(enum BattlerId battler, enum TrainerSlideType slideId)
{
    if (IsTrainerSlideInitialized(battler, slideId))
        return TRUE;

    if (IsTrainerSlidePlayed(battler, slideId))
        return TRUE;

    return FALSE;
}

static bool8 ADMG_TrainerSlideStatusBounds(enum BattlerId battler, enum TrainerSlideType slideId)
{
    if ((u32)battler >= MAX_BATTLERS_COUNT || (u32)slideId >= TRAINER_SLIDE_COUNT || gBattleStruct == NULL)
    { ADMG_StateAccessRejected((u32)slideId, __func__); return FALSE; }
    return TRUE;
}

void TryInitializeFirstSTABMoveTrainerSlide(enum BattlerId battlerDef, enum BattlerId battlerAtk, enum Type moveType)
{
    if (gBattleStruct == NULL || (u32)battlerDef >= MAX_BATTLERS_COUNT || (u32)battlerAtk >= MAX_BATTLERS_COUNT)
    { ADMG_StateAccessRejected(0, __func__); return; }
    /*enum TrainerSlideType slideId = TRAINER_SLIDE_ATTACKER_LANDS_FIRST_STAB_MOVE;
    enum BattlerId slideBattler = battlerAtk;

    if (BattlerIsPlayer(battlerAtk))
    {
        slideId = TRAINER_SLIDE_DEFENDER_TAKES_FIRST_STAB_MOVE;
        slideBattler = battlerDef;
    }

    if (IsSlideInitalizedOrPlayed(slideBattler, slideId))
        return;

    if (IS_BATTLER_OF_TYPE(battlerAtk, moveType) == FALSE)
        return;

    InitalizeTrainerSlide(slideBattler, slideId);*/

    enum TrainerSlideType slideId = TRAINER_SLIDE_ATTACKER_LANDS_FIRST_STAB_MOVE;
    enum BattlerId slideBattler = battlerAtk;
    bool32 skip = FALSE;

    if (IsSlideInitalizedOrPlayed(slideBattler, slideId))
        skip = TRUE;

    if (IS_BATTLER_OF_TYPE(battlerAtk, moveType) == FALSE)
        return;

    if (GetBattlerSide(battlerDef) == GetBattlerSide(battlerAtk))
        return;

    if (!skip)
        InitalizeTrainerSlide(slideBattler, slideId);

    slideId = TRAINER_SLIDE_DEFENDER_TAKES_FIRST_STAB_MOVE;
    slideBattler = battlerDef;

    if (IsSlideInitalizedOrPlayed(slideBattler, slideId))
        return;

    InitalizeTrainerSlide(slideBattler, slideId);
}

void TryInitializeTrainerSlidePlayerLandsFirstCriticalHit(enum BattlerId target)
{
    if (gBattleStruct == NULL || (u32)target >= MAX_BATTLERS_COUNT)
    { ADMG_StateAccessRejected(0, __func__); return; }
    enum TrainerSlideType slideId = TRAINER_SLIDE_DEFENDER_TAKES_FIRST_CRITICAL_HIT;

    if (IsSlideInitalizedOrPlayed(target, slideId))
        return;

    if (IsOnPlayerSide(target))
        return;

    InitalizeTrainerSlide(target, slideId);
}

void TryInitializeTrainerSlideEnemyLandsFirstCriticalHit(enum BattlerId target)
{
    if (gBattleStruct == NULL || (u32)target >= MAX_BATTLERS_COUNT || (u32)gBattlerAttacker >= MAX_BATTLERS_COUNT)
    { ADMG_StateAccessRejected(0, __func__); return; }
    enum TrainerSlideType slideId = TRAINER_SLIDE_ATTACKER_LANDS_FIRST_CRITICAL_HIT;

    if (IsSlideInitalizedOrPlayed(gBattlerAttacker, slideId))
        return;

    if (!IsOnPlayerSide(target))
        return;

    InitalizeTrainerSlide(gBattlerAttacker, slideId);
}

void TryInitializeTrainerSlideLandsFirstSuperEffectiveHit(enum BattlerId battlerDef, enum BattlerId battlerAtk)
{
    if (gBattleStruct == NULL || (u32)battlerDef >= MAX_BATTLERS_COUNT || (u32)battlerAtk >= MAX_BATTLERS_COUNT)
    { ADMG_StateAccessRejected(0, __func__); return; }
    enum TrainerSlideType slideId = TRAINER_SLIDE_ATTACKER_LANDS_FIRST_SUPER_EFFECTIVE_HIT;
    enum BattlerId slideBattler = battlerAtk;
    bool32 skip = FALSE;

    if (IsSlideInitalizedOrPlayed(slideBattler, slideId))
        skip = TRUE;

    if (GetBattlerSide(battlerDef) == GetBattlerSide(battlerAtk))
        return;

    if (!skip)
        InitalizeTrainerSlide(slideBattler, slideId);

    slideId = TRAINER_SLIDE_DEFENDER_TAKES_FIRST_SUPER_EFFECTIVE_HIT;
    slideBattler = battlerDef;

    if (IsSlideInitalizedOrPlayed(slideBattler, slideId))
        return;

    InitalizeTrainerSlide(slideBattler, slideId);
}

void TryInitializeTrainerSlideMonUnaffected(enum BattlerId battlerDef, enum BattlerId battlerAtk)
{
    if (gBattleStruct == NULL || (u32)battlerDef >= MAX_BATTLERS_COUNT || (u32)battlerAtk >= MAX_BATTLERS_COUNT)
    { ADMG_StateAccessRejected(0, __func__); return; }
    enum TrainerSlideType slideId = TRAINER_SLIDE_OPPONENT_MON_UNAFFECTED;
    enum BattlerId slideBattler = battlerAtk;
    bool32 skip = FALSE;

    if (IsSlideInitalizedOrPlayed(slideBattler, slideId))
        skip = TRUE;

    if (GetBattlerSide(battlerDef) == GetBattlerSide(battlerAtk))
        return;

    if (!skip)
        InitalizeTrainerSlide(slideBattler, slideId);

    slideId = TRAINER_SLIDE_SELF_MON_UNAFFECTED;
    slideBattler = battlerDef;

    if (IsSlideInitalizedOrPlayed(slideBattler, slideId))
        return;

    InitalizeTrainerSlide(slideBattler, slideId);
}

bool32 IsTrainerSlideInitialized(enum BattlerId battler, enum TrainerSlideType slideId)
{
    if (!ADMG_TrainerSlideStatusBounds(battler, slideId)) return FALSE;
    u32 arrayIndex = slideId / TRAINER_SLIDES_PER_ARRAY;
    u32 bitPosition = slideId % TRAINER_SLIDES_PER_ARRAY;

    return (gBattleStruct->slideMessageStatus.messageInitalized[battler][arrayIndex] & (1 << bitPosition)) != 0;
}

bool32 IsTrainerSlidePlayed(enum BattlerId battler, enum TrainerSlideType slideId)
{
    if (!ADMG_TrainerSlideStatusBounds(battler, slideId)) return FALSE;
    u32 arrayIndex = slideId / TRAINER_SLIDES_PER_ARRAY;
    u32 bitPosition = slideId % TRAINER_SLIDES_PER_ARRAY;

    return (gBattleStruct->slideMessageStatus.messagePlayed[battler][arrayIndex] & (1 << bitPosition)) != 0;
}

void InitalizeTrainerSlide(enum BattlerId battler, enum TrainerSlideType slideId)
{
    if (!ADMG_TrainerSlideStatusBounds(battler, slideId)) return;
    u32 arrayIndex = slideId / TRAINER_SLIDES_PER_ARRAY;
    u32 bitPosition = slideId % TRAINER_SLIDES_PER_ARRAY;

    gBattleStruct->slideMessageStatus.messageInitalized[battler][arrayIndex] |= (1 << bitPosition);
}

void MarkInitializedTrainerSlidesAsPlayed(enum BattlerId battler, enum TrainerSlideType slideId)
{
    if (!ADMG_TrainerSlideStatusBounds(battler, slideId)) return;
    u32 arrayIndex = slideId / TRAINER_SLIDES_PER_ARRAY;
    u32 bitPosition = slideId % TRAINER_SLIDES_PER_ARRAY;

    if (IsTrainerSlideInitialized(battler, slideId) && !IsTrainerSlidePlayed(battler, slideId))
        gBattleStruct->slideMessageStatus.messagePlayed[battler][arrayIndex] |= (1 << bitPosition);
}

void MarkTrainerSlideAsPlayed(enum BattlerId battler, enum TrainerSlideType slideId)
{
    if (!ADMG_TrainerSlideStatusBounds(battler, slideId)) return;
    u32 arrayIndex = slideId / TRAINER_SLIDES_PER_ARRAY;
    u32 bitPosition = slideId % TRAINER_SLIDES_PER_ARRAY;

    gBattleStruct->slideMessageStatus.messagePlayed[battler][arrayIndex] |= (1 << bitPosition);
}
ADMG_RETAIN_FUNCTION(ADMG_GetRegisteredTrainerSlide);
