#ifndef GUARD_MASS_OUTBREAK_H
#define GUARD_MASS_OUTBREAK_H

#include "global.h"
#include "constants/mass_outbreak.h"

struct MassOutbreak
{
    enum Species species;
    enum Move moves[MAX_MON_MOVES];
    u8 level;
    u8 probability;
    u8 duration;
    MapId location; // Full 32-bit map ID, including maps added at runtime
};

// An outbreak request with 32-bit fields and no "keep" sentinels
struct ADMG_OutbreakRequest
{
    u32 species;
    u32 moves[MAX_MON_MOVES];
    u32 level;
    u32 probability; // percent, 0-100
    u32 daysLeft;    // 1-0xFFFF (SaveBlock1.outbreakDaysLeft)
    u32 location;    // MapId
};

enum ADMG_OutbreakStartStatus
{
    ADMG_OUTBREAK_STARTED,
    ADMG_OUTBREAK_ALREADY_ACTIVE,
    ADMG_OUTBREAK_INVALID_SPECIES,
    ADMG_OUTBREAK_INVALID_LEVEL,
    ADMG_OUTBREAK_INVALID_MOVE,
    ADMG_OUTBREAK_INVALID_PROBABILITY,
    ADMG_OUTBREAK_INVALID_DURATION,
    ADMG_OUTBREAK_INVALID_MAP,
    ADMG_OUTBREAK_INVALID_INDEX,
    ADMG_OUTBREAK_UNAVAILABLE,
};

u32 ADMG_ValidateOutbreak(const struct ADMG_OutbreakRequest *request);
u32 ADMG_StartOutbreak(const struct ADMG_OutbreakRequest *request, u32 replace);
u32 ADMG_StartStaticOutbreak(u32 index, u32 replace);
u32 ADMG_ReadActiveOutbreak(struct ADMG_OutbreakRequest *out);
u32 ADMG_EndOutbreak(void);

void PrepareTvShowForRandomOutbreak(TVShow *show);
void UpdateMassOutbreakDaysLeft(u32 days);
bool32 IsMassOutbreakActive(void);
bool8 SetUpMassOutbreakEncounter(u8 flags);
bool8 DoMassOutbreakEncounterTest(void);
void StartStaticMassOutbreak(enum MassOutbreakIndex outbreakIdx);
enum Species GetStaticOutbreakSpecies(enum MassOutbreakIndex outbreakIdx);
u8 *GetStaticOutbreakMapName(u8 *dest, enum MassOutbreakIndex outbreakIdx);
void ZeroMassOutbreak(void);
struct MassOutbreak GetStaticOutbreak(enum MassOutbreakIndex outbreakIdx);

#endif // GUARD_MASS_OUTBREAK_H

