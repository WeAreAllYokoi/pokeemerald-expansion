#include "global.h"
#include "battle_tower.h"
#include "admg_trainer_actor.h"
#include "admg_state.h"
#include "admg_trainer.h"
#include "main.h"
#include "battle.h"
#include "battle_partner.h"
#include "battle_frontier.h"
#include "data.h"
#include "frontier_util.h"
#include "difficulty.h"
#include "malloc.h"
#include "string_util.h"
#include "trainer_util.h"
#include "text.h"

#include "constants/abilities.h"
#include "constants/apprentice.h"
#include "constants/trainers.h"
#include "constants/battle_ai.h"

#if !TESTING
const struct Trainer gBattlePartners[DIFFICULTY_COUNT][PARTNER_COUNT] =
{
#include "data/battle_partners.h"
};
#endif

#define STEVEN_OTID 61226

static bool8 ADMG_RunPartnerParty(u32 trainerId, bool8 admissionOnly)
{
    struct ADMG_TrainerActorRef actor;
    if (!ADMG_TryClassifyTrainerActor(trainerId, ADMG_ACTOR_FACILITY, ADMG_ACTOR_PARTNER, &actor)) return FALSE;
    const u32 kind = actor.kind;
    const u32 actorIndex = actor.index;
    s32 i, j;
    u32 ivs, level = 0;

    u8 trainerName[(PLAYER_NAME_LENGTH * 3) + 1];
    const struct Trainer *checkedPartner = NULL;
    struct Trainer admittedPartner;
    const struct BattleFrontierTrainer *admittedFacility = NULL;
    const struct TrainerMon *admittedMons[FRONTIER_MULTI_PARTY_SIZE] = {0};
    u16 admittedSlots[FRONTIER_MULTI_PARTY_SIZE] = {0};
    bool8 admittedGender[FRONTIER_MULTI_PARTY_SIZE] = {0};
    struct EmeraldBattleTowerRecord admittedRecord;
    struct Apprentice admittedApprentice;
    struct SaveBlock2 *save = gSaveBlock2Ptr;
    s32 admittedLastIndex = 0;
    u64 generation = ADMG_BeginStateAccessCheck();
    if (gSaveBlock2Ptr == NULL)
    {
        ADMG_StateAccessRejected(trainerId, __func__);
        return FALSE;
    }
    if (kind == ADMG_ACTOR_NAMED_PARTNER || kind == ADMG_ACTOR_KANTO)
    {
        if (!ADMG_TryGetTrainerStruct(trainerId, &checkedPartner))
            return FALSE;
    }
    else if (trainerId > TRAINER_EREADER)
    {
        ADMG_StateAccessRejected(trainerId, __func__);
        return FALSE;
    }
    if (checkedPartner != NULL)
    {
        if (!admissionOnly) AreMultiPartiesFullTeams();
        if (!ADMG_EndStateAccessCheck(generation))
            return FALSE;
    }
    if (checkedPartner != NULL)
    {
        /* CRC inputs include every byte, including padding and pointers. */
        memcpy(&admittedPartner, checkedPartner, sizeof(admittedPartner));
        admittedLastIndex = admissionOnly ? 0 : (AreMultiPartiesFullTeams() ? PARTY_SIZE : MULTI_PARTY_SIZE);
        if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
        if (admittedPartner.trainerClass >= TRAINER_CLASS_COUNT
         || (admittedPartner.partySize != 0 && admittedPartner.party == NULL))
        { ADMG_StateAccessRejected(trainerId, __func__); return FALSE; }
    }
    if (kind == ADMG_ACTOR_FRONTIER)
    {
        const struct BattleFrontierTrainer *table;
        const struct TrainerMon *mons;
        u32 trainerCount, monCount;
        if (!ADMG_TrySelectFacilityTrainerTables(save->frontier.lvlMode == FRONTIER_LVL_TENT,
                &table, &mons, &trainerCount)
         || !ADMG_TryGetFacilityTrainerRowFromTable(table, trainerCount, trainerId, &admittedFacility)
         || !ADMG_TryGetFacilityMonTableCount(mons, &monCount)) return FALSE;
        if (admittedFacility->facilityClass >= FACILITY_CLASSES_COUNT)
        { ADMG_StateAccessRejected(admittedFacility->facilityClass, __func__); return FALSE; }
        for (u32 slot = 0; slot < FRONTIER_MULTI_PARTY_SIZE; ++slot)
        {
            admittedSlots[slot] = save->frontier.trainerIds[18 + slot];
            if (!ADMG_TryGetFacilityMonRowFromTable(mons, monCount, admittedSlots[slot], &admittedMons[slot])) return FALSE;
        }
    }
    else if (kind == ADMG_ACTOR_RECORD_FRIEND)
    {
        memcpy(&admittedRecord, &save->frontier.towerRecords[actorIndex], sizeof(admittedRecord));
        bool8 terminated = FALSE;
        for (u32 byte = 0; byte < sizeof(admittedRecord.name); ++byte)
            if (admittedRecord.name[byte] == EOS) { terminated = TRUE; break; }
        if (!terminated) { ADMG_StateAccessRejected(trainerId, __func__); return FALSE; }
        for (u32 slot = 0; slot < FRONTIER_MULTI_PARTY_SIZE; ++slot)
        {
            admittedSlots[slot] = save->frontier.trainerIds[18 + slot];
            if (admittedSlots[slot] >= ARRAY_COUNT(admittedRecord.party))
            { ADMG_StateAccessRejected(admittedSlots[slot], __func__); return FALSE; }
        }
    }
    else if (kind == ADMG_ACTOR_APPRENTICE)
    {
        memcpy(&admittedApprentice, &save->apprentices[actorIndex], sizeof(admittedApprentice));
        if (admittedApprentice.id >= NUM_APPRENTICES)
        { ADMG_StateAccessRejected(admittedApprentice.id, __func__); return FALSE; }
        for (u32 slot = 0; slot < FRONTIER_MULTI_PARTY_SIZE; ++slot)
        {
            admittedSlots[slot] = save->frontier.trainerIds[18 + slot];
            if (admittedSlots[slot] >= ARRAY_COUNT(admittedApprentice.party))
            { ADMG_StateAccessRejected(admittedSlots[slot], __func__); return FALSE; }
        }
    }
    if (admissionOnly)
    {
        if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
        if (gSaveBlock2Ptr != save) { ADMG_StateAccessRejected(trainerId, __func__); return FALSE; }
        return TRUE;
    }
    SetFacilityPtrsGetLevel();
    if (kind == ADMG_ACTOR_FRONTIER) level = SetFacilityPtrsGetLevel();
    if (kind == ADMG_ACTOR_FRONTIER || kind == ADMG_ACTOR_RECORD_FRIEND || kind == ADMG_ACTOR_APPRENTICE)
        for (u32 slot = 0; slot < FRONTIER_MULTI_PARTY_SIZE; ++slot)
            admittedGender[slot] = IsFrontierTrainerFemale(trainerId);
    if (!ADMG_EndStateAccessCheck(generation)) return FALSE;
    if (gSaveBlock2Ptr != save) { ADMG_StateAccessRejected(trainerId, __func__); return FALSE; }
    ZeroPartyMons(gParties[B_TRAINER_PARTNER]);

    if (kind == ADMG_ACTOR_NAMED_PARTNER || kind == ADMG_ACTOR_KANTO)
    {
        s32 lastIndex = admittedLastIndex;

        const struct Trainer *partner = &admittedPartner;
        struct TrainerGenerator partnerGen;
        MakePartnerGenerator(&partnerGen, partner);
        if (trainerId == TRAINER_PARTNER(PARTNER_STEVEN))
            partnerGen.otID = OTID_STRUCT_PRESET(STEVEN_OTID);
        for (i = 0; i < lastIndex && i < partner->partySize; i++)
        {
            GenerateMonFromTrainerMon(&gParties[B_TRAINER_PARTNER][i], &partner->party[i], &partnerGen);
        }
    }
    else if (kind == ADMG_ACTOR_EREADER)
    {
        // Scrapped, lol.
        trainerName[0] = gGameLanguage;
    }
    else if (kind == ADMG_ACTOR_FRONTIER)
    {
        ivs = GetFrontierTrainerFixedIvs(trainerId);
        u32 otID = Random32();
        for (i = 0; i < FRONTIER_MULTI_PARTY_SIZE; i++)
        {
            CreateFacilityMon(admittedMons[i], level, ivs, otID, 0, &gParties[B_TRAINER_PARTNER][i]);
            for (j = 0; j < PLAYER_NAME_LENGTH + 1; j++)
                trainerName[j] = admittedFacility->trainerName[j];
            SetMonData(&gParties[B_TRAINER_PARTNER][i], MON_DATA_OT_NAME, &trainerName);
            j = admittedGender[i];
            SetMonData(&gParties[B_TRAINER_PARTNER][i], MON_DATA_OT_GENDER, &j);
        }
    }
    else if (kind == ADMG_ACTOR_RECORD_FRIEND)
    {
        trainerId -= TRAINER_RECORD_MIXING_FRIEND;
        for (i = 0; i < FRONTIER_MULTI_PARTY_SIZE; i++)
        {
            struct EmeraldBattleTowerRecord *record = &admittedRecord;
            struct BattleTowerPokemon monData = record->party[admittedSlots[i]];
            StringCopy(trainerName, record->name);
            if (record->language == LANGUAGE_JAPANESE)
            {
                if (monData.nickname[0] != EXT_CTRL_CODE_BEGIN || monData.nickname[1] != EXT_CTRL_CODE_JPN)
                {
                    monData.nickname[5] = EOS;
                    ConvertInternationalString(monData.nickname, LANGUAGE_JAPANESE);
                }
            }
            else
            {
                if (monData.nickname[0] == EXT_CTRL_CODE_BEGIN && monData.nickname[1] == EXT_CTRL_CODE_JPN)
                    trainerName[5] = EOS;
            }
            CreateBattleTowerMon_HandleLevel(&gParties[B_TRAINER_PARTNER][i], &monData, TRUE);
            SetMonData(&gParties[B_TRAINER_PARTNER][i], MON_DATA_OT_NAME, trainerName);
            j = admittedGender[i];
            SetMonData(&gParties[B_TRAINER_PARTNER][i], MON_DATA_OT_GENDER, &j);
        }
    }
    else
    {
        trainerId -= TRAINER_RECORD_MIXING_APPRENTICE;
        for (i = 0; i < FRONTIER_MULTI_PARTY_SIZE; i++)
        {
            CreateApprenticeMon(&gParties[B_TRAINER_PARTNER][i], &admittedApprentice, admittedSlots[i]);
            j = admittedGender[i];
            SetMonData(&gParties[B_TRAINER_PARTNER][i], MON_DATA_OT_GENDER, &j);
        }
    }
    return TRUE;
}

bool8 ADMG_TryAdmitPartnerParty(u32 trainerId)
{
    return ADMG_RunPartnerParty(trainerId, TRUE);
}

ADMG_NATIVE_ENTRY void FillPartnerParty(u16 trainerId)
{
    ADMG_RunPartnerParty(trainerId, FALSE);
}
