#include "global.h"
#include "admg_feature_records.h"
#include "event_data.h"
#include "admg_script.h"
#include "main.h"
#include "mass_outbreak.h"
#include "overworld.h"
#include "random.h"
#include "region_map.h"
#include "script.h"
#include "wild_encounter.h"

static const struct MassOutbreak sPokeOutbreakSpeciesList[OUTBREAK_COUNT] = {
    [OUTBREAK_ID_ROUTE102] = {
        .species = SPECIES_SEEDOT,
        .moves = {MOVE_BIDE, MOVE_HARDEN, MOVE_LEECH_SEED},
        .level = 3,
        .probability = 100,
        .duration = 1,
        .location = MAP_ROUTE102

    },
    [OUTBREAK_ID_ROUTE114] = {
        .species = SPECIES_NUZLEAF,
        .moves = {MOVE_HARDEN, MOVE_GROWTH, MOVE_NATURE_POWER, MOVE_LEECH_SEED},
        .level = 15,
        .probability = 50,
        .duration = 1,
        .location = MAP_ROUTE114,
    },
    [OUTBREAK_ID_ROUTE117] = {
        .species = SPECIES_SEEDOT,
        .moves = {MOVE_HARDEN, MOVE_GROWTH, MOVE_NATURE_POWER, MOVE_LEECH_SEED},
        .level = 13,
        .probability = 50,
        .duration = 1,
        .location = MAP_ROUTE117,
    },
    [OUTBREAK_ID_ROUTE120] = {
        .species = SPECIES_SEEDOT,
        .moves = {MOVE_GIGA_DRAIN, MOVE_FRUSTRATION, MOVE_SOLAR_BEAM, MOVE_LEECH_SEED},
        .level = 25,
        .probability = 50,
        .duration = 1,
        .location = MAP_ROUTE120,
    },
    [OUTBREAK_ID_ROUTE116] = {
        .species = SPECIES_SKITTY,
        .moves = {MOVE_GROWL, MOVE_TACKLE, MOVE_TAIL_WHIP, MOVE_ATTRACT},
        .level = 8,
        .probability = 50,
        .duration = 1,
        .location = MAP_ROUTE116,
    }
};

static const struct MassOutbreak *ADMG_GetStaticOutbreakEntry(u32 index)
{
    if (index >= ARRAY_COUNT(sPokeOutbreakSpeciesList))
    {
        MgbaPrintf(MGBA_LOG_WARN, "%s: invalid static outbreak index %lu", __func__, index);
        return NULL;
    }
    return &sPokeOutbreakSpeciesList[index];
}

ADMG_NATIVE_ENTRY ADMG_Word ADMG_CopyStaticOutbreak(ADMG_Word index, struct MassOutbreak *outbreak)
{
    const struct MassOutbreak *entry;
    if (outbreak == NULL)
        return FALSE;
    entry = ADMG_GetStaticOutbreakEntry(index);
    if (entry == NULL)
        return FALSE;
    *outbreak = *entry;
    return TRUE;
}

void ZeroMassOutbreak(void)
{
    gSaveBlock1Ptr->outbreakPokemonSpecies = 0;
    gSaveBlock1Ptr->outbreakLocation = 0;
    gSaveBlock1Ptr->outbreakPokemonLevel = 0;
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        gSaveBlock1Ptr->outbreakPokemonMoves[i] = 0;
    gSaveBlock1Ptr->outbreakPokemonProbability = 0;
    gSaveBlock1Ptr->outbreakDaysLeft = 0;
}

struct MassOutbreak GetStaticOutbreak(enum MassOutbreakIndex outbreakIdx)
{
    const struct MassOutbreak *outbreak = ADMG_GetStaticOutbreakEntry(outbreakIdx);
    return outbreak != NULL ? *outbreak : (struct MassOutbreak){0};
}

enum Species GetStaticOutbreakSpecies(enum MassOutbreakIndex outbreakIdx)
{
    const struct MassOutbreak *outbreak = ADMG_GetStaticOutbreakEntry(outbreakIdx);
    return outbreak != NULL ? outbreak->species : SPECIES_NONE;
}

u8 *GetStaticOutbreakMapName(u8 *dest, enum MassOutbreakIndex outbreakIdx)
{
    const struct MassOutbreak *outbreak = ADMG_GetStaticOutbreakEntry(outbreakIdx);
    if (outbreak == NULL || dest == NULL)
        return dest;
    return GetMapName(dest, GetMapHeader(outbreak->location)->regionMapSectionId, 0);
}

void PrepareTvShowForRandomOutbreak(TVShow *show)
{
    u32 outbreakIdx = RandomUniform(RNG_NONE, 0, ARRAY_COUNT(sPokeOutbreakSpeciesList));
    show->massOutbreak.outbreakIndex = outbreakIdx + 1;
    show->massOutbreak.species = sPokeOutbreakSpeciesList[outbreakIdx].species;
    show->massOutbreak.locationMap = sPokeOutbreakSpeciesList[outbreakIdx].location;
}

void StartMassOutbreak(struct MassOutbreak outbreak)
{
    gSaveBlock1Ptr->outbreakPokemonSpecies = outbreak.species;
    gSaveBlock1Ptr->outbreakLocation = outbreak.location;
    gSaveBlock1Ptr->outbreakPokemonLevel = outbreak.level;
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        gSaveBlock1Ptr->outbreakPokemonMoves[i] = outbreak.moves[i];
    }
    gSaveBlock1Ptr->outbreakPokemonProbability = outbreak.probability;
    gSaveBlock1Ptr->outbreakDaysLeft = outbreak.duration;
}

void StartStaticMassOutbreak(enum MassOutbreakIndex outbreakIdx)
{
    const struct MassOutbreak *outbreak = ADMG_GetStaticOutbreakEntry(outbreakIdx);
    if (outbreak == NULL || gSaveBlock1Ptr == NULL)
        return;
    StartMassOutbreak(*outbreak);
}

void UpdateMassOutbreakDaysLeft(u32 days)
{
    // Clear the outbreak data when it expires, including its species
    // TV won't count down to the next outbreak until outbreakPokemonSpecies is SPECIES_NONE
    if (gSaveBlock1Ptr->outbreakDaysLeft <= days)
        ZeroMassOutbreak();
    else
        gSaveBlock1Ptr->outbreakDaysLeft -= days;
}

bool32 IsMassOutbreakActive(void)
{
    return (gSaveBlock1Ptr->outbreakDaysLeft > 0);
}

bool8 SetUpMassOutbreakEncounter(u8 flags)
{
    if (flags & WILD_CHECK_REPEL && !IsWildLevelAllowedByRepel(gSaveBlock1Ptr->outbreakPokemonLevel))
        return FALSE;

    CreateWildMon(gSaveBlock1Ptr->outbreakPokemonSpecies, gSaveBlock1Ptr->outbreakPokemonLevel);
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        SetMonMoveSlot(&gParties[B_TRAINER_OPPONENT_A][0], gSaveBlock1Ptr->outbreakPokemonMoves[i], i);

    return TRUE;
}

bool8 DoMassOutbreakEncounterTest(void)
{
    if (gSaveBlock1Ptr->outbreakDaysLeft == 0)
        return FALSE;

    if (MAP_NUM(gSaveBlock1Ptr->location.mapId) != MAP_NUM(gSaveBlock1Ptr->outbreakLocation) || MAP_GROUP(gSaveBlock1Ptr->location.mapId) != MAP_GROUP(gSaveBlock1Ptr->outbreakLocation))
        return FALSE;

    return (RandomPercentage(RNG_NONE,  gSaveBlock1Ptr->outbreakPokemonProbability));
}

void ScrCmd_setstaticoutbreak(struct ScriptContext *ctx)
{
    ADMG_SCRIPT_REQUIRE_BACKING(ctx, gSaveBlock1Ptr);
    enum MassOutbreakIndex outbreakIdx = ADMG_SCRIPT_VALUE(ctx, ScriptReadHalfword(ctx));
    bool32 noReplacement = ScriptReadByte(ctx);

    if (outbreakIdx == OUTBREAK_COUNT)
        return;

    assertf(!IsMassOutbreakActive() || noReplacement == FALSE, "Trying to start a new outbreak while one is already in progress")
    {
        return;
    }

    assertf(outbreakIdx < OUTBREAK_COUNT, "Trying to read undefined outbreak index %d", outbreakIdx)
    {
        return;
    }
    StartStaticMassOutbreak(outbreakIdx);
}

static u32 ReadMapFromContext(struct ScriptContext *ctx)
{
    u32 map = ScriptReadHalfword(ctx);
    if (map == MAP_UNDEFINED)
        return MAP_UNDEFINED;
    return ADMG_SCRIPT_VALUE(ctx, map, MAP_UNDEFINED);
}

void ScrCmd_setdynamicoutbreak(struct ScriptContext *ctx)
{
    ADMG_SCRIPT_REQUIRE_BACKING(ctx, gSaveBlock1Ptr);
    enum Species species = ADMG_SCRIPT_VALUE(ctx, ScriptReadHalfword(ctx));
    u32 level = ADMG_SCRIPT_VALUE(ctx, ScriptReadHalfword(ctx));
    enum Move moves[MAX_MON_MOVES];
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        moves[i] = ADMG_SCRIPT_VALUE(ctx, ScriptReadHalfword(ctx));
    u32 probability = ADMG_SCRIPT_VALUE(ctx, ScriptReadHalfword(ctx));
    u32 daysLeft = ADMG_SCRIPT_VALUE(ctx, ScriptReadHalfword(ctx));
    u32 map = ReadMapFromContext(ctx);
    if (ctx->scriptPtr == NULL)
        return;
    bool32 noReplacement = ScriptReadByte(ctx);

    assertf(!IsMassOutbreakActive() || noReplacement == FALSE, "Trying to start a new outbreak while one is already in progress")
    {
        return;
    }

    assertf(IsSpeciesEnabled(species), "Trying to call setdynamicoutbreak with unknown species %d", species)
    {
        return;
    }
    assertf(level >= 1 && level <= MAX_LEVEL, "Trying to call setdynamicoutbreak with invalid level %d", level)
    {
        return;
    }
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        assertf(IsRegularMoveId(moves[i]), "Trying to call setdynamicoutbreak with invalid move %d", moves[i])
        {
            return;
        }
    }
    assertf(probability <= 100, "Trying to call setdynamicoutbreak with invalid percent chance %d", probability)
    {
        return;
    }

    gSaveBlock1Ptr->outbreakPokemonSpecies = species;
    gSaveBlock1Ptr->outbreakPokemonLevel = level;
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        gSaveBlock1Ptr->outbreakPokemonMoves[i] = moves[i];
    }
    gSaveBlock1Ptr->outbreakPokemonProbability = probability;
    gSaveBlock1Ptr->outbreakDaysLeft = daysLeft;
    gSaveBlock1Ptr->outbreakLocation = map;
}

void ScrCmd_editoutbreak(struct ScriptContext *ctx)
{
    ADMG_SCRIPT_REQUIRE_BACKING(ctx, gSaveBlock1Ptr);
    enum Species species = ADMG_SCRIPT_VALUE(ctx, ScriptReadHalfword(ctx));
    u32 level = ADMG_SCRIPT_VALUE(ctx, ScriptReadHalfword(ctx));
    enum Move moves[MAX_MON_MOVES];
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        moves[i] = ADMG_SCRIPT_VALUE(ctx, ScriptReadHalfword(ctx));
    }
    u32 probability = ADMG_SCRIPT_VALUE(ctx, ScriptReadHalfword(ctx));
    u32 daysLeft = ADMG_SCRIPT_VALUE(ctx, ScriptReadHalfword(ctx));
    u32 map = ReadMapFromContext(ctx);
    if (ctx->scriptPtr == NULL)
        return;

    assertf(IsMassOutbreakActive(), "Trying to edit outbreak but none is active")
    {
        return;
    }

    if (species != SPECIES_NONE)
    {
        if (IsSpeciesEnabled(species))
            gSaveBlock1Ptr->outbreakPokemonSpecies = species;
        else
            errorf("Trying to call editoutbreak with unknown species %d", species);
    }

    if (level != 0)
    {
        if (level <= MAX_LEVEL)
            gSaveBlock1Ptr->outbreakPokemonLevel = level;
        else
            errorf("Trying to call editoutbreak with invalid level %d", level);
    }

    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        if (moves[i] != MOVES_COUNT)
        {
            if (IsRegularMoveId(moves[i]))
                gSaveBlock1Ptr->outbreakPokemonMoves[i] = moves[i];
            else
                errorf("Trying to call editoutbreak with invalid move %d", moves[i]);
        }
    }

    if (probability != 0xFF)
    {
        if (probability <= 100)
            gSaveBlock1Ptr->outbreakPokemonProbability = probability;
        else
            errorf("Trying to call editoutbreak with invalid percent chance %d", probability);
    }

    if (daysLeft != 0x3FFF)
        gSaveBlock1Ptr->outbreakDaysLeft = daysLeft;

    if (map != MAP_UNDEFINED)
    {
        gSaveBlock1Ptr->outbreakLocation = map;
    }
}

void ScrCmd_checkhasactiveoutbreak(struct ScriptContext *ctx)
{
    ADMG_SCRIPT_REQUIRE_BACKING(ctx, gSaveBlock1Ptr);
    gSpecialVar_Result = IsMassOutbreakActive();
}

void ScrCmd_getmassoutbreakdata(struct ScriptContext *ctx)
{
    ADMG_SCRIPT_REQUIRE_BACKING(ctx, gSaveBlock1Ptr);
    u32 varId = ScriptReadHalfword(ctx);
    (void)ADMG_SCRIPT_POINTER(ctx, varId);
    enum MassOutbreakData dataField = ScriptReadByte(ctx);
    u32 value = 0;
    switch (dataField)
    {
    case OUTBREAK_DATA_SPECIES:
        value = gSaveBlock1Ptr->outbreakPokemonSpecies;
        break;
    case OUTBREAK_DATA_MOVE1:
        value = gSaveBlock1Ptr->outbreakPokemonMoves[0];
        break;
    case OUTBREAK_DATA_MOVE2:
        value = gSaveBlock1Ptr->outbreakPokemonMoves[1];
        break;
    case OUTBREAK_DATA_MOVE3:
        value = gSaveBlock1Ptr->outbreakPokemonMoves[2];
        break;
    case OUTBREAK_DATA_MOVE4:
        value = gSaveBlock1Ptr->outbreakPokemonMoves[3];
        break;
    case OUTBREAK_DATA_LEVEL:
        value = gSaveBlock1Ptr->outbreakPokemonLevel;
        break;
    case OUTBREAK_DATA_PROBABILITY:
        value = gSaveBlock1Ptr->outbreakPokemonProbability;
        break;
    case OUTBREAK_DATA_DAYS_LEFT:
        value = gSaveBlock1Ptr->outbreakDaysLeft;
        break;
    case OUTBREAK_DATA_MAP:
        value = MAP_NUM(gSaveBlock1Ptr->outbreakLocation) | (MAP_GROUP(gSaveBlock1Ptr->outbreakLocation) << 8);
        break;
    }
    VarSet(varId, value);
}

