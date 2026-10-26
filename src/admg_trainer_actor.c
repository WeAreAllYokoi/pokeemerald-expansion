#include "global.h"
#include "admg_trainer_actor.h"
#include "admg_state.h"
#include "debug.h"
#include "constants/battle.h"
#include "constants/battle_partner.h"
#include "constants/trainers.h"

static bool8 RejectActor(u32 identifier, const char *operation)
{
    ADMG_StateAccessRejected(identifier, operation);
    return FALSE;
}

bool8 ADMG_TryClassifyTrainerActor(u32 identifier, u32 domain, u32 role, struct ADMG_TrainerActorRef *actor)
{
    struct ADMG_TrainerActorRef selected;
    struct ADMG_TrainerRef content;
    if (actor == NULL || identifier > 0xFFFF || domain > ADMG_ACTOR_NETWORK || role > ADMG_ACTOR_UNUSED)
        return RejectActor(identifier, __func__);
    selected.identifier = identifier;
    selected.index = 0;
    if (role == ADMG_ACTOR_UNUSED)
    {
        /* An unused transport word is preserved, not interpreted as an actor. */
        selected.kind = ADMG_ACTOR_NONE;
    }
    else if (domain == ADMG_ACTOR_NETWORK)
    {
        /* Person/OT IDs are separate u32 fields. This u16 word is opaque in a
         * network actor path and must not be used to select a content row. */
        selected.kind = ADMG_ACTOR_NETWORK_PLAYER;
    }
    else if (role == ADMG_ACTOR_PARTNER && identifier > MAX_TRAINERS_COUNT_EMERALD + PARTNER_NONE
          && identifier < MAX_TRAINERS_COUNT_EMERALD + PARTNER_COUNT)
    {
        if (!ADMG_TryResolveTrainer(identifier, &content) || content.owner != ADMG_TRAINER_PARTNER)
            return RejectActor(identifier, __func__);
        selected.kind = ADMG_ACTOR_NAMED_PARTNER;
        selected.index = content.nativeIndex;
    }
    else if (role == ADMG_ACTOR_PARTNER && identifier > ADMG_KANTO_TRAINER_FIRST
          && identifier < ADMG_KANTO_TRAINER_FIRST + ADMG_KANTO_TRAINERS_COUNT)
    {
        /* Retain the C3 checked native-row partner generator. This is ordinary
         * content used in a partner role, not a named-partner alias. */
        if (!ADMG_TryResolveTrainer(identifier, &content) || content.owner != ADMG_TRAINER_KANTO)
            return RejectActor(identifier, __func__);
        selected.kind = ADMG_ACTOR_KANTO;
        selected.index = content.nativeIndex;
    }
    else if (domain == ADMG_ACTOR_FACILITY)
    {
        if (identifier < FRONTIER_TRAINERS_COUNT)
        { selected.kind = ADMG_ACTOR_FRONTIER; selected.index = identifier; }
        else if (identifier < TRAINER_RECORD_MIXING_APPRENTICE)
        { selected.kind = ADMG_ACTOR_RECORD_FRIEND; selected.index = identifier - TRAINER_RECORD_MIXING_FRIEND; }
        else if (identifier < TRAINER_EREADER)
        { selected.kind = ADMG_ACTOR_APPRENTICE; selected.index = identifier - TRAINER_RECORD_MIXING_APPRENTICE; }
        else if (identifier == TRAINER_EREADER)
            selected.kind = ADMG_ACTOR_EREADER;
        else if (identifier == TRAINER_FRONTIER_BRAIN && role == ADMG_ACTOR_OPPONENT)
            selected.kind = ADMG_ACTOR_BRAIN;
        else
            return RejectActor(identifier, __func__);
    }
    else if (role == ADMG_ACTOR_OPPONENT && identifier == TRAINER_SECRET_BASE)
        selected.kind = ADMG_ACTOR_SECRET_BASE;
    else if (role == ADMG_ACTOR_OPPONENT && identifier == TRAINER_LINK_OPPONENT)
        selected.kind = ADMG_ACTOR_LINK;
    else if (role == ADMG_ACTOR_OPPONENT && identifier == TRAINER_UNION_ROOM)
        selected.kind = ADMG_ACTOR_UNION_ROOM;
    else
    {
        if ((identifier == TRAINER_NONE && !TESTING && !gIsDebugBattle)
         || !ADMG_TryResolveTrainer(identifier, &content) || content.owner == ADMG_TRAINER_PARTNER)
            return RejectActor(identifier, __func__);
        selected.kind = content.owner == ADMG_TRAINER_KANTO ? ADMG_ACTOR_KANTO : ADMG_ACTOR_HOENN;
        selected.index = content.nativeIndex;
    }
    *actor = selected;
    return TRUE;
}

bool8 ADMG_TryValidateOrdinaryTrainerOperands(u32 opponentA, u32 opponentB)
{
    struct ADMG_TrainerActorRef actor;
    if (!ADMG_TryClassifyTrainerActor(opponentA, ADMG_ACTOR_CONTENT, ADMG_ACTOR_OPPONENT, &actor)
     || actor.kind == ADMG_ACTOR_SECRET_BASE || actor.kind == ADMG_ACTOR_LINK || actor.kind == ADMG_ACTOR_UNION_ROOM)
        return RejectActor(opponentA, __func__);
    if (!ADMG_TryClassifyTrainerActor(opponentB, ADMG_ACTOR_CONTENT,
        opponentB == TRAINER_NONE || opponentB == 0xFFFF ? ADMG_ACTOR_UNUSED : ADMG_ACTOR_OPPONENT, &actor)) return FALSE;
    if (actor.kind == ADMG_ACTOR_SECRET_BASE || actor.kind == ADMG_ACTOR_LINK || actor.kind == ADMG_ACTOR_UNION_ROOM)
        return RejectActor(opponentB, __func__);
    return TRUE;
}

/* The original SPECIAL_BATTLE_EREADER root: an ordinary trainer battle flagged
 * as the saved e-Reader opponent, outside Frontier, Hill, Secret Base, link,
 * partner and two-opponent contexts, and only with the e-Reader backing. */
static bool8 ADMG_IsEReaderSpecialBattle(u32 flags)
{
#if FREE_BATTLE_TOWER_E_READER == FALSE
    return (flags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_EREADER_TRAINER)) == (BATTLE_TYPE_TRAINER | BATTLE_TYPE_EREADER_TRAINER)
        && !(flags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK | BATTLE_TYPE_TOWER_LINK_MULTI
            | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_HILL | BATTLE_TYPE_SECRET_BASE
            | BATTLE_TYPE_TWO_OPPONENTS | BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_MULTI));
#else
    return FALSE;
#endif
}

bool8 ADMG_TryCaptureBattleTrainerActors(u32 flags, u32 opponentA, u32 opponentB, u32 partner,
    struct ADMG_BattleTrainerActors *snapshot)
{
    if (snapshot == NULL) return RejectActor(opponentA, __func__);
    struct ADMG_TrainerActorRef actor;
    if (ADMG_IsEReaderSpecialBattle(flags))
    {
        /* SPECIAL_BATTLE_EREADER publishes opponent word zero and the saved
         * e-Reader record supplies the opponent. Admit only that exact word,
         * only in this context; the unused words keep their original values. */
        if (opponentA != 0) return RejectActor(opponentA, __func__);
        if (!ADMG_TryClassifyTrainerActor(opponentB, ADMG_ACTOR_CONTENT, ADMG_ACTOR_UNUSED, &actor)
         || !ADMG_TryClassifyTrainerActor(partner, ADMG_ACTOR_FACILITY, ADMG_ACTOR_UNUSED, &actor))
            return FALSE;
        snapshot->flags = flags;
        snapshot->opponentA = opponentA; snapshot->opponentB = opponentB;
        snapshot->partner = partner;
        snapshot->uses = 1;
        return TRUE;
    }
    u32 domain = flags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK | BATTLE_TYPE_TOWER_LINK_MULTI)
        ? ADMG_ACTOR_NETWORK : flags & (BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_HILL)
        ? ADMG_ACTOR_FACILITY : ADMG_ACTOR_CONTENT;
    u32 opponentRole = flags & (BATTLE_TYPE_TRAINER | BATTLE_TYPE_FRONTIER | BATTLE_TYPE_TRAINER_HILL)
        ? ADMG_ACTOR_OPPONENT : ADMG_ACTOR_UNUSED;
    u32 secondRole = flags & BATTLE_TYPE_TWO_OPPONENTS ? ADMG_ACTOR_OPPONENT : ADMG_ACTOR_UNUSED;
    u32 partnerRole = flags & (BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_MULTI) ? ADMG_ACTOR_PARTNER : ADMG_ACTOR_UNUSED;
    if (opponentB == 0xFFFF && flags & BATTLE_TYPE_INGAME_PARTNER)
        secondRole = ADMG_ACTOR_UNUSED;
    if (!ADMG_TryClassifyTrainerActor(opponentA, domain, opponentRole, &actor)
     || !ADMG_TryClassifyTrainerActor(opponentB, domain, secondRole, &actor)
     || !ADMG_TryClassifyTrainerActor(partner, domain == ADMG_ACTOR_NETWORK ? domain : ADMG_ACTOR_FACILITY, partnerRole, &actor))
        return FALSE;
    snapshot->flags = flags;
    snapshot->opponentA = opponentA; snapshot->opponentB = opponentB;
    snapshot->partner = partner;
    snapshot->uses = (opponentRole != ADMG_ACTOR_UNUSED)
        | ((secondRole != ADMG_ACTOR_UNUSED) << 1) | ((partnerRole != ADMG_ACTOR_UNUSED) << 2);
    return TRUE;
}

bool8 ADMG_TryValidateBattleTrainerActors(u32 flags, u32 opponentA, u32 opponentB, u32 partner)
{
    struct ADMG_BattleTrainerActors snapshot;
    return ADMG_TryCaptureBattleTrainerActors(flags, opponentA, opponentB, partner, &snapshot);
}

bool8 ADMG_BattleTrainerActorsCurrent(const struct ADMG_BattleTrainerActors *snapshot,
    u32 flags, u32 opponentA, u32 opponentB, u32 partner)
{
    struct ADMG_BattleTrainerActors current;
    if (snapshot == NULL) return RejectActor(opponentA, __func__);
    if (!ADMG_TryCaptureBattleTrainerActors(flags, opponentA, opponentB, partner, &current)) return FALSE;
    if (flags != snapshot->flags || current.uses != snapshot->uses)
        return RejectActor(flags, __func__);
    if ((snapshot->uses & 1) && opponentA != snapshot->opponentA) return RejectActor(opponentA, __func__);
    if ((snapshot->uses & 2) && opponentB != snapshot->opponentB) return RejectActor(opponentB, __func__);
    if ((snapshot->uses & 4) && partner != snapshot->partner) return RejectActor(partner, __func__);
    return TRUE;
}

bool8 ADMG_IsBattleFacilityActor(u32 flags, u32 role, u32 identifier, u32 kind)
{
    struct ADMG_TrainerActorRef actor;
    if (kind == ADMG_ACTOR_EREADER && role == ADMG_ACTOR_OPPONENT && ADMG_IsEReaderSpecialBattle(flags))
        return identifier == 0;
    if ((role == ADMG_ACTOR_OPPONENT && !(flags & BATTLE_TYPE_FRONTIER))
     || flags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK | BATTLE_TYPE_TOWER_LINK_MULTI)
     || role > ADMG_ACTOR_PARTNER
     || (role == ADMG_ACTOR_PARTNER && !(flags & (BATTLE_TYPE_INGAME_PARTNER | BATTLE_TYPE_MULTI))))
        return FALSE;
    if (!ADMG_TryClassifyTrainerActor(identifier, ADMG_ACTOR_FACILITY, role, &actor))
        return FALSE;
    return actor.kind == kind;
}

bool8 ADMG_IsKantoPartnerActor(u32 flags, u32 identifier)
{
    struct ADMG_TrainerRef ref;
    if (!(flags & BATTLE_TYPE_INGAME_PARTNER)
     || flags & (BATTLE_TYPE_LINK | BATTLE_TYPE_RECORDED_LINK | BATTLE_TYPE_TOWER_LINK_MULTI)
     || identifier <= ADMG_KANTO_TRAINER_FIRST
     || identifier >= ADMG_KANTO_TRAINER_FIRST + ADMG_KANTO_TRAINERS_COUNT)
        return FALSE;
    return ADMG_TryResolveTrainer(identifier, &ref) && ref.owner == ADMG_TRAINER_KANTO;
}

bool8 ADMG_IsPartnerPictureIdentity(u32 identifier, bool32 contentAI)
{
    struct ADMG_TrainerRef ref;
    /* Presentation can query the brain descriptor even though the original
     * partner party generator does not construct a brain party. */
    if (identifier <= TRAINER_FRONTIER_BRAIN)
        return TRUE;
    if (identifier > 0xFFFF)
        return FALSE;
    if ((identifier > MAX_TRAINERS_COUNT_EMERALD + PARTNER_NONE
      && identifier < MAX_TRAINERS_COUNT_EMERALD + PARTNER_COUNT)
     || (identifier > ADMG_KANTO_TRAINER_FIRST
      && identifier < ADMG_KANTO_TRAINER_FIRST + ADMG_KANTO_TRAINERS_COUNT)
     || (contentAI && identifier < TRAINERS_COUNT_EMERALD))
        return ADMG_TryResolveTrainer(identifier, &ref);
    return FALSE;
}
