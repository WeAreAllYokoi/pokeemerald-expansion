#ifndef ADMG_TRAINER_ACTOR_H
#define ADMG_TRAINER_ACTOR_H

#include "admg_trainer.h"

/* Internal consumer domains. These are not persisted IDs or a public mod API.
 * Facility ordinals overlap Hoenn content and therefore require an explicit
 * transport domain. Current map, campaign and badge state never select it. */
enum ADMG_TrainerActorDomain { ADMG_ACTOR_CONTENT, ADMG_ACTOR_FACILITY, ADMG_ACTOR_NETWORK };
enum ADMG_TrainerActorRole { ADMG_ACTOR_OPPONENT, ADMG_ACTOR_PARTNER, ADMG_ACTOR_UNUSED };
enum ADMG_TrainerActorKind
{
    ADMG_ACTOR_NONE, ADMG_ACTOR_HOENN, ADMG_ACTOR_KANTO, ADMG_ACTOR_NAMED_PARTNER,
    ADMG_ACTOR_FRONTIER, ADMG_ACTOR_RECORD_FRIEND, ADMG_ACTOR_APPRENTICE,
    ADMG_ACTOR_EREADER, ADMG_ACTOR_BRAIN, ADMG_ACTOR_SECRET_BASE,
    ADMG_ACTOR_LINK, ADMG_ACTOR_UNION_ROOM, ADMG_ACTOR_NETWORK_PLAYER
};
struct ADMG_TrainerActorRef { u16 identifier; u16 index; enum ADMG_TrainerActorKind kind; };

bool8 ADMG_TryClassifyTrainerActor(u32 identifier, u32 domain, u32 role, struct ADMG_TrainerActorRef *actor);
bool8 ADMG_TryValidateOrdinaryTrainerOperands(u32 opponentA, u32 opponentB);
/* Internal ephemeral operand snapshot; no save representation or public SDK. */
struct ADMG_BattleTrainerActors { u32 flags; enum TrainerId opponentA, opponentB, partner; u8 uses; };
bool8 ADMG_TryCaptureBattleTrainerActors(u32 flags, u32 opponentA, u32 opponentB, u32 partner,
    struct ADMG_BattleTrainerActors *snapshot);
bool8 ADMG_BattleTrainerActorsCurrent(const struct ADMG_BattleTrainerActors *snapshot,
    u32 flags, u32 opponentA, u32 opponentB, u32 partner);

bool8 ADMG_TryValidateBattleTrainerActors(u32 battleFlags, u32 opponentA, u32 opponentB, u32 partner);
bool8 ADMG_IsBattleFacilityActor(u32 battleFlags, u32 role, u32 identifier, u32 kind);
bool8 ADMG_IsKantoPartnerActor(u32 battleFlags, u32 identifier);
bool8 ADMG_IsPartnerPictureIdentity(u32 identifier, bool32 contentAI);

#endif
