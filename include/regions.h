#ifndef GUARD_REGIONS_H
#define GUARD_REGIONS_H

#include "global.h"
#include "constants/regions.h"

enum KantoSubRegion GetKantoSubregion(u32 mapSecId);
// The region of an added section (FIRST_ADDED_MAPSEC and up): its row's, or Hoenn.
enum Region GetAddedMapSecRegion(u32 mapSecId);

static inline enum Region GetRegionForSectionId(u32 sectionId)
{
    if (sectionId >= FIRST_ADDED_MAPSEC)
        return GetAddedMapSecRegion(sectionId);
    if (sectionId >= KANTO_MAPSEC_START && sectionId < MAPSEC_SPECIAL_AREA)
        return REGION_KANTO;
    return REGION_HOENN;
}

static inline enum Region GetCurrentRegion(void)
{
    return GetRegionForSectionId(gMapHeader.regionMapSectionId);
}

#endif // GUARD_REGIONS_H
