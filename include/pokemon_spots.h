#ifndef GUARD_POKEMON_SPOTS_H
#define GUARD_POKEMON_SPOTS_H

#include "gba/types.h"
#include "constants/species.h"


enum SpotScale
{
    SCALE_1 = 1,
    SCALE_2 = 2,
    SCALE_4 = 4
};

struct MonSpot {
    u8 x, y;
    const u32* image;
};

struct MonSpotTemplate {
    const struct MonSpot* spots;
    u8 count;
    enum SpotScale scale;
    s8 xOffsetFrame2;
    s8 yOffsetFrame2;
    u8 firstColor;
    u8 lastColor;
    u8 colorAdjust;
};

bool32 ShouldDrawSpotsOnSpecies(enum Species species);
void DrawPokemonSpotsBothFrames(u32 personality, enum Species species, u8 *dest);
const struct MonSpotTemplate *ADMG_GetSpeciesSpots(u32 species, const struct MonSpotTemplate *source);

#endif /* end of include guard: GUARD_POKEMON_SPOTS_H */
