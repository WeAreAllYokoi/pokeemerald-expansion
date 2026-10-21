#include "global.h"
#include "admg_feature_policies.h"
#include "admg_retention.h"
#include "event_data.h"
#include "admg_state.h"
#include "field_screen_effect.h"
#include "field_weather.h"
#include "fldeff.h"
#include "gpu_regs.h"
#include "io_reg.h"
#include "main.h"
#include "malloc.h"
#include "map_preview_screen.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "rtc.h"
#include "region_map.h"
#include "script.h"
#include "string_util.h"
#include "constants/region_map_sections.h"
#include "constants/rgb.h"

static EWRAM_DATA bool8 sHasVisitedMapBefore = FALSE;
static EWRAM_DATA bool8 sAllocedBg0TilemapBuffer = FALSE;
static EWRAM_DATA const struct MapPreviewScreen *sActivePreview = NULL;

static void Task_MapPreviewScreen_FadeIn(u8 taskId);
static void Task_MapPreviewScreen_Script(u8 taskId);

static const u8 sViridianForestMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/viridian_forest/tiles.png", ".gbapal");
static const u8 sViridianForestMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/viridian_forest/tiles.png", ".4bpp.smol");
static const u8 sViridianForestMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/viridian_forest/tilemap.bin.smolTM");
static const u8 sRockTunnelMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/rock_tunnel/tiles.png", ".gbapal");
static const u8 sRockTunnelMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/rock_tunnel/tiles.png", ".4bpp.smol");
static const u8 sRockTunnelMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/rock_tunnel/tilemap.bin.smolTM");
static const u8 sRocketHideoutMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/rocket_hideout/tiles.png", ".gbapal");
static const u8 sRocketHideoutMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/rocket_hideout/tiles.png", ".4bpp.smol");
static const u8 sRocketHideoutMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/rocket_hideout/tilemap.bin.smolTM");
static const u8 sPowerPlantMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/power_plant/tiles.png", ".gbapal");
static const u8 sPowerPlantMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/power_plant/tiles.png", ".4bpp.smol");
static const u8 sPowerPlantMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/power_plant/tilemap.bin.smolTM");
static const u8 sPokemonMansionMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/pokemon_mansion/tiles.png", ".gbapal");
static const u8 sPokemonMansionMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/pokemon_mansion/tiles.png", ".4bpp.smol");
static const u8 sPokemonMansionMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/pokemon_mansion/tilemap.bin.smolTM");
static const u8 sPokemonTowerMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/pokemon_tower/tiles.png", ".gbapal");
static const u8 sPokemonTowerMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/pokemon_tower/tiles.png", ".4bpp.smol");
static const u8 sPokemonTowerMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/pokemon_tower/tilemap.bin.smolTM");
static const u8 sSilphCoMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/silph_co/tiles.png", ".gbapal");
static const u8 sSilphCoMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/silph_co/tiles.png", ".4bpp.smol");
static const u8 sSilphCoMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/silph_co/tilemap.bin.smolTM");
static const u8 sMtMoonMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/mt_moon/tiles.png", ".gbapal");
static const u8 sMtMoonMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/mt_moon/tiles.png", ".4bpp.smol");
static const u8 sMtMoonMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/mt_moon/tilemap.bin.smolTM");
static const u8 sSeafoamIslandsMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/seafoam_islands/tiles.png", ".gbapal");
static const u8 sSeafoamIslandsMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/seafoam_islands/tiles.png", ".4bpp.smol");
static const u8 sSeafoamIslandsMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/seafoam_islands/tilemap.bin.smolTM");
static const u8 sRocketWarehouseMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/rocket_warehouse/tiles.png", ".gbapal");
static const u8 sRocketWarehouseMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/rocket_warehouse/tiles.png", ".4bpp.smol");
static const u8 sRocketWarehouseMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/rocket_warehouse/tilemap.bin.smolTM");
static const u8 sVictoryRoadMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/victory_road/tiles.png", ".gbapal");
static const u8 sVictoryRoadMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/victory_road/tiles.png", ".4bpp.smol");
static const u8 sVictoryRoadMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/victory_road/tilemap.bin.smolTM");
static const u8 sMtEmberMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/mt_ember/tiles.png", ".gbapal");
static const u8 sMtEmberMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/mt_ember/tiles.png", ".4bpp.smol");
static const u8 sMtEmberMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/mt_ember/tilemap.bin.smolTM");
static const u8 sSafariZoneMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/safari_zone/tiles.png", ".gbapal");
static const u8 sSafariZoneMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/safari_zone/tiles.png", ".4bpp.smol");
static const u8 sSafariZoneMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/safari_zone/tilemap.bin.smolTM");
static const u8 sMoneanChamberMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/monean_chamber/tiles.png", ".gbapal");
static const u8 sMoneanChamberMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/monean_chamber/tiles.png", ".4bpp.smol");
static const u8 sMoneanChamberMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/monean_chamber/tilemap.bin.smolTM");
static const u8 sDottedHoleMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/dotted_hole/tiles.png", ".gbapal");
static const u8 sDottedHoleMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/dotted_hole/tiles.png", ".4bpp.smol");
static const u8 sDottedHoleMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/dotted_hole/tilemap.bin.smolTM");
static const u8 sCeruleanCaveMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/cerulean_cave/tiles.png", ".gbapal");
static const u8 sCeruleanCaveMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/cerulean_cave/tiles.png", ".4bpp.smol");
static const u8 sCeruleanCaveMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/cerulean_cave/tilemap.bin.smolTM");
static const u8 sDiglettsCaveMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/digletts_cave/tiles.png", ".gbapal");
static const u8 sDiglettsCaveMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/digletts_cave/tiles.png", ".4bpp.smol");
static const u8 sDiglettsCaveMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/digletts_cave/tilemap.bin.smolTM");
static const u8 sLostCaveMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/lost_cave/tiles.png", ".gbapal");
static const u8 sLostCaveMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/lost_cave/tiles.png", ".4bpp.smol");
static const u8 sLostCaveMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/lost_cave/tilemap.bin.smolTM");
static const u8 sBerryForestMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/berry_forest/tiles.png", ".gbapal");
static const u8 sBerryForestMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/berry_forest/tiles.png", ".4bpp.smol");
static const u8 sBerryForestMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/berry_forest/tilemap.bin.smolTM");
static const u8 sIcefallCaveMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/icefall_cave/tiles.png", ".gbapal");
static const u8 sIcefallCaveMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/icefall_cave/tiles.png", ".4bpp.smol");
static const u8 sIcefallCaveMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/icefall_cave/tilemap.bin.smolTM");
static const u8 sAlteringCaveMapPreviewPalette[] = INCGFX_U8("graphics/map_preview/altering_cave/tiles.png", ".gbapal");
static const u8 sAlteringCaveMapPreviewTiles[] = INCGFX_U8("graphics/map_preview/altering_cave/tiles.png", ".4bpp.smol");
static const u8 sAlteringCaveMapPreviewTilemap[] = INCBIN_U8("graphics/map_preview/altering_cave/tilemap.bin.smolTM");

static const struct MapPreviewScreen sMapPreviewScreenData[MPS_COUNT] = {
    [MPS_VIRIDIAN_FOREST] = {
        .mapsec = MAPSEC_VIRIDIAN_FOREST,
        .type = MPS_TYPE_FADE_IN,
        .flagId = FLAG_WORLD_MAP_VIRIDIAN_FOREST,
        .tilesptr = sViridianForestMapPreviewTiles,
        .tilemapptr = sViridianForestMapPreviewTilemap,
        .palptr = sViridianForestMapPreviewPalette
    },
    [MPS_MT_MOON] = {
        .mapsec = MAPSEC_MT_MOON,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_MT_MOON_1F,
        .tilesptr = sMtMoonMapPreviewTiles,
        .tilemapptr = sMtMoonMapPreviewTilemap,
        .palptr = sMtMoonMapPreviewPalette
    },
    [MPS_DIGLETTS_CAVE] = {
        .mapsec = MAPSEC_DIGLETTS_CAVE,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_DIGLETTS_CAVE_B1F,
        .tilesptr = sDiglettsCaveMapPreviewTiles,
        .tilemapptr = sDiglettsCaveMapPreviewTilemap,
        .palptr = sDiglettsCaveMapPreviewPalette
    },
    [MPS_ROCK_TUNNEL] = {
        .mapsec = MAPSEC_ROCK_TUNNEL,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_ROCK_TUNNEL_1F,
        .tilesptr = sRockTunnelMapPreviewTiles,
        .tilemapptr = sRockTunnelMapPreviewTilemap,
        .palptr = sRockTunnelMapPreviewPalette
    },
    [MPS_POKEMON_TOWER] = {
        .mapsec = MAPSEC_POKEMON_TOWER,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_POKEMON_TOWER_1F,
        .tilesptr = sPokemonTowerMapPreviewTiles,
        .tilemapptr = sPokemonTowerMapPreviewTilemap,
        .palptr = sPokemonTowerMapPreviewPalette
    },
    [MPS_SAFARI_ZONE] = {
        .mapsec = MAPSEC_KANTO_SAFARI_ZONE,
        .type = MPS_TYPE_FADE_IN,
        .flagId = FLAG_WORLD_MAP_SAFARI_ZONE_CENTER,
        .tilesptr = sSafariZoneMapPreviewTiles,
        .tilemapptr = sSafariZoneMapPreviewTilemap,
        .palptr = sSafariZoneMapPreviewPalette
    },
    [MPS_SEAFOAM_ISLANDS] = {
        .mapsec = MAPSEC_SEAFOAM_ISLANDS,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_SEAFOAM_ISLANDS_1F,
        .tilesptr = sSeafoamIslandsMapPreviewTiles,
        .tilemapptr = sSeafoamIslandsMapPreviewTilemap,
        .palptr = sSeafoamIslandsMapPreviewPalette
    },
    [MPS_POKEMON_MANSION] = {
        .mapsec = MAPSEC_POKEMON_MANSION,
        .type = MPS_TYPE_FADE_IN,
        .flagId = FLAG_WORLD_MAP_POKEMON_MANSION_1F,
        .tilesptr = sPokemonMansionMapPreviewTiles,
        .tilemapptr = sPokemonMansionMapPreviewTilemap,
        .palptr = sPokemonMansionMapPreviewPalette
    },
    [MPS_ROCKET_HIDEOUT] = {
        .mapsec = MAPSEC_ROCKET_HIDEOUT,
        .type = MPS_TYPE_FADE_IN,
        .flagId = FLAG_WORLD_MAP_ROCKET_HIDEOUT_B1F,
        .tilesptr = sRocketHideoutMapPreviewTiles,
        .tilemapptr = sRocketHideoutMapPreviewTilemap,
        .palptr = sRocketHideoutMapPreviewPalette
    },
    [MPS_SILPH_CO] = {
        .mapsec = MAPSEC_SILPH_CO,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_SILPH_CO_1F,
        .tilesptr = sSilphCoMapPreviewTiles,
        .tilemapptr = sSilphCoMapPreviewTilemap,
        .palptr = sSilphCoMapPreviewPalette
    },
    [MPS_VICTORY_ROAD] = {
        .mapsec = MAPSEC_KANTO_VICTORY_ROAD,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_VICTORY_ROAD_1F,
        .tilesptr = sVictoryRoadMapPreviewTiles,
        .tilemapptr = sVictoryRoadMapPreviewTilemap,
        .palptr = sVictoryRoadMapPreviewPalette
    },
    [MPS_CERULEAN_CAVE] = {
        .mapsec = MAPSEC_CERULEAN_CAVE,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_CERULEAN_CAVE_1F,
        .tilesptr = sCeruleanCaveMapPreviewTiles,
        .tilemapptr = sCeruleanCaveMapPreviewTilemap,
        .palptr = sCeruleanCaveMapPreviewPalette
    },
    [MPS_POWER_PLANT] = {
        .mapsec = MAPSEC_POWER_PLANT,
        .type = MPS_TYPE_FADE_IN,
        .flagId = FLAG_WORLD_MAP_POWER_PLANT,
        .tilesptr = sPowerPlantMapPreviewTiles,
        .tilemapptr = sPowerPlantMapPreviewTilemap,
        .palptr = sPowerPlantMapPreviewPalette
    },
    [MPS_MT_EMBER] = {
        .mapsec = MAPSEC_MT_EMBER,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_MT_EMBER_EXTERIOR,
        .tilesptr = sMtEmberMapPreviewTiles,
        .tilemapptr = sMtEmberMapPreviewTilemap,
        .palptr = sMtEmberMapPreviewPalette
    },
    [MPS_ROCKET_WAREHOUSE] = {
        .mapsec = MAPSEC_ROCKET_WAREHOUSE,
        .type = MPS_TYPE_FADE_IN,
        .flagId = FLAG_WORLD_MAP_THREE_ISLAND_BERRY_FOREST,
        .tilesptr = sRocketWarehouseMapPreviewTiles,
        .tilemapptr = sRocketWarehouseMapPreviewTilemap,
        .palptr = sRocketWarehouseMapPreviewPalette
    },
    [MPS_MONEAN_CHAMBER] = {
        .mapsec = MAPSEC_MONEAN_CHAMBER,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_SEVEN_ISLAND_TANOBY_RUINS_MONEAN_CHAMBER,
        .tilesptr = sMoneanChamberMapPreviewTiles,
        .tilemapptr = sMoneanChamberMapPreviewTilemap,
        .palptr = sMoneanChamberMapPreviewPalette
    },
    [MPS_DOTTED_HOLE] = {
        .mapsec = MAPSEC_DOTTED_HOLE,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_SIX_ISLAND_DOTTED_HOLE_1F,
        .tilesptr = sDottedHoleMapPreviewTiles,
        .tilemapptr = sDottedHoleMapPreviewTilemap,
        .palptr = sDottedHoleMapPreviewPalette
    },
    [MPS_BERRY_FOREST] = {
        .mapsec = MAPSEC_BERRY_FOREST,
        .type = MPS_TYPE_FADE_IN,
        .flagId = FLAG_WORLD_MAP_THREE_ISLAND_BERRY_FOREST,
        .tilesptr = sBerryForestMapPreviewTiles,
        .tilemapptr = sBerryForestMapPreviewTilemap,
        .palptr = sBerryForestMapPreviewPalette
    },
    [MPS_ICEFALL_CAVE] = {
        .mapsec = MAPSEC_ICEFALL_CAVE,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_FOUR_ISLAND_ICEFALL_CAVE_ENTRANCE,
        .tilesptr = sIcefallCaveMapPreviewTiles,
        .tilemapptr = sIcefallCaveMapPreviewTilemap,
        .palptr = sIcefallCaveMapPreviewPalette
    },
    [MPS_LOST_CAVE] = {
        .mapsec = MAPSEC_LOST_CAVE,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_FIVE_ISLAND_LOST_CAVE_ENTRANCE,
        .tilesptr = sLostCaveMapPreviewTiles,
        .tilemapptr = sLostCaveMapPreviewTilemap,
        .palptr = sLostCaveMapPreviewPalette
    },
    [MPS_ALTERING_CAVE] = {
        .mapsec = MAPSEC_ALTERING_CAVE_FRLG,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_SIX_ISLAND_ALTERING_CAVE,
        .tilesptr = sAlteringCaveMapPreviewTiles,
        .tilemapptr = sAlteringCaveMapPreviewTilemap,
        .palptr = sAlteringCaveMapPreviewPalette
    },
    [MPS_PATTERN_BUSH] = {
        .mapsec = MAPSEC_PATTERN_BUSH,
        .type = MPS_TYPE_FADE_IN,
        .flagId = FLAG_WORLD_MAP_SIX_ISLAND_PATTERN_BUSH,
        .tilesptr = sViridianForestMapPreviewTiles,
        .tilemapptr = sViridianForestMapPreviewTilemap,
        .palptr = sViridianForestMapPreviewPalette
    },
    [MPS_LIPTOO_CHAMBER] = {
        .mapsec = MAPSEC_LIPTOO_CHAMBER,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_SEVEN_ISLAND_TANOBY_RUINS_MONEAN_CHAMBER,
        .tilesptr = sMoneanChamberMapPreviewTiles,
        .tilemapptr = sMoneanChamberMapPreviewTilemap,
        .palptr = sMoneanChamberMapPreviewPalette
    },
    [MPS_WEEPTH_CHAMBER] = {
        .mapsec = MAPSEC_WEEPTH_CHAMBER,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_SEVEN_ISLAND_TANOBY_RUINS_MONEAN_CHAMBER,
        .tilesptr = sMoneanChamberMapPreviewTiles,
        .tilemapptr = sMoneanChamberMapPreviewTilemap,
        .palptr = sMoneanChamberMapPreviewPalette
    },
    [MPS_TDILFORD_CHAMBER] = {
        .mapsec = MAPSEC_DILFORD_CHAMBER,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_SEVEN_ISLAND_TANOBY_RUINS_MONEAN_CHAMBER,
        .tilesptr = sMoneanChamberMapPreviewTiles,
        .tilemapptr = sMoneanChamberMapPreviewTilemap,
        .palptr = sMoneanChamberMapPreviewPalette
    },
    [MPS_SCUFIB_CHAMBER] = {
        .mapsec = MAPSEC_SCUFIB_CHAMBER,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_SEVEN_ISLAND_TANOBY_RUINS_MONEAN_CHAMBER,
        .tilesptr = sMoneanChamberMapPreviewTiles,
        .tilemapptr = sMoneanChamberMapPreviewTilemap,
        .palptr = sMoneanChamberMapPreviewPalette
    },
    [MPS_RIXY_CHAMBER] = {
        .mapsec = MAPSEC_RIXY_CHAMBER,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_SEVEN_ISLAND_TANOBY_RUINS_MONEAN_CHAMBER,
        .tilesptr = sMoneanChamberMapPreviewTiles,
        .tilemapptr = sMoneanChamberMapPreviewTilemap,
        .palptr = sMoneanChamberMapPreviewPalette
    },
    [MPS_VIAPOIS_CHAMBER] = {
        .mapsec = MAPSEC_VIAPOIS_CHAMBER,
        .type = MPS_TYPE_CAVE,
        .flagId = FLAG_WORLD_MAP_SEVEN_ISLAND_TANOBY_RUINS_MONEAN_CHAMBER,
        .tilesptr = sMoneanChamberMapPreviewTiles,
        .tilemapptr = sMoneanChamberMapPreviewTilemap,
        .palptr = sMoneanChamberMapPreviewPalette
    }
};

static const struct WindowTemplate sMapNameWindow = {
    .bg = 0,
    .tilemapLeft = 0,
    .tilemapTop = 0,
    .width = 13,
    .height = 2,
    .paletteNum = 14,
    .baseBlock = 0x1C2
};

static const struct WindowTemplate sMapNameWindowLarge = {
    .bg = 0,
    .tilemapLeft = 0,
    .tilemapTop = 0,
    .width = 22,
    .height = 2,
    .paletteNum = 14,
    .baseBlock = 0x259
};

static const struct BgTemplate sMapPreviewBgTemplate[1] = {
    {
        .mapBaseIndex = 31
    }
};

bool32 ShouldRunMapPreview(void)
{
    if (ADMG_GetFeaturePolicy(ADMG_POLICY_MAP_PREVIEWS) && FlagGet(FLAG_HIDE_MAP_NAME_POPUP) != TRUE && GetLastUsedWarpMapSectionId() != gMapHeader.regionMapSectionId)
        return TRUE;

    return FALSE;
}

ADMG_NATIVE_ENTRY const struct MapPreviewScreen *ADMG_GetRegisteredMapPreview(u32 section, u32 timeOfDay)
{
    return NULL;
}

ADMG_NATIVE_ENTRY const struct MapPreviewScreen *ADMG_GetRetainedMapPreview(u32 id)
{
    return id < MPS_COUNT ? &sMapPreviewScreenData[id] : NULL;
}

static const struct MapPreviewScreen *GetMapPreview(mapsec_u8_t mapsec)
{
    s32 i;
    const struct MapPreviewScreen *registered;
    if (sActivePreview != NULL && sActivePreview->mapsec == mapsec)
        return sActivePreview;
    registered = ADMG_GetRegisteredMapPreview(mapsec, GetTimeOfDay());
    if (registered != NULL && registered->mapsec == mapsec && registered->type <= MPS_TYPE_BASIC
     && registered->tilesptr != NULL && registered->tilemapptr != NULL && registered->palptr != NULL)
        return registered;

    for (i = 0; i < MPS_COUNT; i++)
    {
        if (sMapPreviewScreenData[i].mapsec == mapsec)
        {
            return &sMapPreviewScreenData[i];
        }
    }
    return NULL;
}

bool32 CurrentMapHasPreviewScreen(enum MapPreviewScreenType type)
{
    const struct MapPreviewScreen *preview = GetMapPreview(gMapHeader.regionMapSectionId);
    if (preview != NULL)
    {
        if (type == MPS_TYPE_ANY)
            return TRUE;
        else
            return preview->type == type ? TRUE : FALSE;
    }
    else
    {
        return FALSE;
    }
}

void MapPreview_InitBgs(void)
{
    InitBgsFromTemplates(0, sMapPreviewBgTemplate, NELEMS(sMapPreviewBgTemplate));
    ShowBg(0);
}

void MapPreview_LoadGfx(mapsec_u8_t mapsec)
{
    const struct MapPreviewScreen *preview = GetMapPreview(mapsec);
    if (preview != NULL)
    {
        sActivePreview = preview;
        ResetTempTileDataBuffers();
        if (preview->usesAllPalettes)
            LoadPalette(preview->palptr, BG_PLTT_ID(0), 16 * PLTT_SIZE_4BPP);
        else
            LoadPalette(preview->palptr, BG_PLTT_ID(13), 3 * PLTT_SIZE_4BPP);

        DecompressAndCopyTileDataToVram(0, preview->tilesptr, 0, 0, 0);
        if (GetBgTilemapBuffer(0) == NULL)
        {
            SetBgTilemapBuffer(0, Alloc(BG_SCREEN_SIZE));
            sAllocedBg0TilemapBuffer = TRUE;
        }
        else
        {
            sAllocedBg0TilemapBuffer = FALSE;
        }
        CopyToBgTilemapBuffer(0, preview->tilemapptr, 0, 0x000);
        CopyBgTilemapBufferToVram(0);
    }
}

void MapPreview_Unload(s32 windowId)
{
    if (windowId != WINDOW_NONE)
        RemoveWindow(windowId);
    if (sAllocedBg0TilemapBuffer)
    {
        Free(GetBgTilemapBuffer(0));
        UnsetBgTilemapBuffer(0);
    }
    sAllocedBg0TilemapBuffer = FALSE;
    sActivePreview = NULL;
}

bool32 MapPreview_IsGfxLoadFinished(void)
{
    return FreeTempTileDataBuffersIfPossible();
}

u16 MapPreview_CreateMapNameWindow(mapsec_u8_t mapsec)
{
    u16 windowId;
    u32 xctr;
    s32 stringWidth;
    u32 fontId;
    #ifdef BUGFIX
    // Fixes access violations indicated below.
    u8 color[3];
    #else
    u8 color[0];
    #endif

    GetMapName(gStringVar4, mapsec, 0);
    // Use a longer window size if the map name is too long to fit.
    fontId = GetFontIdToFit(gStringVar4, FONT_NORMAL, 0, 104);
    stringWidth = GetStringWidth(fontId, gStringVar4, 0);
    if (stringWidth > 104)
    {
        windowId = AddWindow(&sMapNameWindowLarge);
        xctr = 177 - stringWidth;
    }
    else
    {
        xctr = 104 - stringWidth;
        windowId = AddWindow(&sMapNameWindow);
    }
    FillWindowPixelBuffer(windowId, PIXEL_FILL(1));
    PutWindowTilemap(windowId);
    color[0] = TEXT_COLOR_WHITE; // Access violation
    color[1] = TEXT_COLOR_RED; // Access violation
    color[2] = TEXT_COLOR_LIGHT_GRAY; // Access violation
    AddTextPrinterParameterized4(windowId, fontId, xctr / 2, 2, 0, 0, color/* Access violation */, -1, gStringVar4);
    return windowId;
}

#define tState      data[0]
#define tCounter    data[1]
#define tDuration   data[2]
#define tMapSecId   gTasks[taskId].contentIds[0]
#define tWindowId   data[4]

void RunMapPreviewScreenNonFade(mapsec_u8_t mapSecId)
{
    u8 taskId = CreateTask(Task_MapPreviewScreen_NonFade, 0);
    gTasks[taskId].contentIds[0] = mapSecId;
    gTasks[taskId].tWindowId = WINDOW_NONE;
}

void Task_MapPreviewScreen_NonFade(u8 taskId)
{
    s16 *data = gTasks[taskId].data;
    switch (tState)
    {
    case 0:
        SetWordTaskArg(taskId, 5, (uintptr_t)gMain.vblankCallback);
        SetVBlankCallback(NULL);
        MapPreview_InitBgs();
        MapPreview_LoadGfx(tMapSecId);
        BlendPalettes(PALETTES_ALL, 0x10, RGB_WHITE);
        tState++;
        break;
    case 1:
        if (!MapPreview_IsGfxLoadFinished())
        {
            const struct MapPreviewScreen *preview = GetMapPreview(tMapSecId);
            if (preview != NULL && !preview->nameDisabled)
            {
                tWindowId = MapPreview_CreateMapNameWindow(tMapSecId);
                CopyWindowToVram(tWindowId, COPYWIN_FULL);
            }
            tState++;
        }
        break;
    case 2:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            if (CurrentMapHasPreviewScreen(MPS_TYPE_CAVE))
                BeginNormalPaletteFade(PALETTES_ALL, -1, 16, 0, RGB_WHITE);
            else
                BeginNormalPaletteFade(PALETTES_ALL, -1, 16, 0, RGB_BLACK);

            SetVBlankCallback((IntrCallback)GetWordTaskArg(taskId, 5));
            tState++;
        }
        break;
    case 3:
        if (!UpdatePaletteFade())
        {
            tDuration = MapPreview_GetDuration(tMapSecId);
            tState++;
        }
        break;
    case 4:
        tCounter++;
        if (tCounter > tDuration || JOY_NEW(B_BUTTON))
        {
            if (CurrentMapHasPreviewScreen(MPS_TYPE_CAVE))
                BeginNormalPaletteFade(PALETTES_ALL, -2, 0, 16, RGB_WHITE);
            else
                BeginNormalPaletteFade(PALETTES_ALL, MPS_BASIC_FADE_SPEED, 0, 16, RGB_BLACK);
            tState++;
        }
        break;
    case 5:
        if (!UpdatePaletteFade())
        {
            int i;
            u16 window = tWindowId;
            for (i = 0; i < 16; i++)
            {
                data[i] = 0;
            }
            MapPreview_Unload(window);
            if (CurrentMapHasPreviewScreen(MPS_TYPE_CAVE))
                gTasks[taskId].func = Task_EnterCaveTransition2;
            else
                SetMainCallback2(gMain.savedCallback);
        }
        break;
    }
}

#undef tDuration
#undef tMapSecId
#undef tWindowId

#define tBGPriority     data[2]
#define tDISPCNT        data[3]
#define tBLDCNT         data[4]
#define tBLDALPHA       data[5]
#define tWININ          data[6]
#define tWINOUT         data[7]
#define tBlendTgt1      data[8]
#define tBlendTgt2      data[9]
#define tDuration       data[10]
#define tWindowId       data[11]

void RunMapPreviewScreenFadeIn(mapsec_u8_t mapsec)
{
    u8 taskId;
    const struct MapPreviewScreen *preview = GetMapPreview(mapsec);
    if (preview == NULL) return;

    taskId = CreateTask(Task_MapPreviewScreen_FadeIn, 0);
    gTasks[taskId].tBGPriority = GetBgAttribute(0, BG_ATTR_PRIORITY);
    gTasks[taskId].tBLDCNT = GetGpuReg(REG_OFFSET_BLDCNT);
    gTasks[taskId].tBLDALPHA = GetGpuReg(REG_OFFSET_BLDALPHA);
    gTasks[taskId].tDISPCNT = GetGpuReg(REG_OFFSET_DISPCNT);
    gTasks[taskId].tWININ = GetGpuReg(REG_OFFSET_WININ);
    gTasks[taskId].tWINOUT = GetGpuReg(REG_OFFSET_WINOUT);
    gTasks[taskId].tDuration = MapPreview_GetDuration(mapsec);
    gTasks[taskId].tBlendTgt1 = 16;
    gTasks[taskId].tBlendTgt2 = 0;
    SetBgAttribute(0, BG_ATTR_PRIORITY, 0);
    SetGpuRegBits(REG_OFFSET_WININ, WININ_WIN0_CLR | WININ_WIN1_CLR);
    SetGpuRegBits(REG_OFFSET_WINOUT, WINOUT_WIN01_CLR);
    gTasks[taskId].tWindowId = WINDOW_NONE;
    if (!preview->nameDisabled)
        gTasks[taskId].tWindowId = MapPreview_CreateMapNameWindow(mapsec);

    LockPlayerFieldControls();
}

bool32 FadeInMapPreviewScreenIsRunning(void)
{
    return FuncIsActiveTask(Task_MapPreviewScreen_FadeIn);
}

static void Task_MapPreviewScreen_FadeIn(u8 taskId)
{
    s16 * data;

    data = gTasks[taskId].data;
    switch (tState)
    {
    case 0:
        if (!MapPreview_IsGfxLoadFinished() && !IsDma3ManagerBusyWithBgCopy())
        {
            if (tWindowId != WINDOW_NONE)
                CopyWindowToVram(tWindowId, COPYWIN_FULL);
            tState++;
        }
        break;
    case 1:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            FadeInFromBlack();
            tState++;
        }
        break;
    case 2:
        if (IsWeatherNotFadingIn())
        {
            Overworld_PlaySpecialMapMusic();
            tState++;
        }
        break;
    case 3:
        tCounter++;
        if (tCounter > tDuration || JOY_NEW(B_BUTTON))
        {
            SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3 | BLDCNT_TGT2_OBJ | BLDCNT_TGT2_BD);
            SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(16, 0));
            tCounter = 0;
            tState++;
        }
        break;
    case 4:
        switch (tCounter)
        {
        case 0:
            tBlendTgt2++;
            if (tBlendTgt2 > 16)
            {
                tBlendTgt2 = 16;
            }
            break;
        case 1:
            tBlendTgt1--;
            if (tBlendTgt1 < 0)
            {
                tBlendTgt1 = 0;
            }
            break;
        }
        tCounter = (tCounter + 1) % 3;
        SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(tBlendTgt1, tBlendTgt2));
        if (tBlendTgt1 == 0 && tBlendTgt2 == 16)
        {
            FillBgTilemapBufferRect_Palette0(0, 0, 0, 0, 32, 32);
            CopyBgTilemapBufferToVram(0);
            tState++;
        }
        break;
    case 5:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            MapPreview_Unload(tWindowId);
            SetBgAttribute(0, BG_ATTR_PRIORITY, tBGPriority);
            SetGpuReg(REG_OFFSET_DISPCNT, tDISPCNT);
            SetGpuReg(REG_OFFSET_BLDCNT, tBLDCNT);
            SetGpuReg(REG_OFFSET_BLDALPHA, tBLDALPHA);
            SetGpuReg(REG_OFFSET_WININ, tWININ);
            SetGpuReg(REG_OFFSET_WINOUT, tWINOUT);
            UnlockPlayerFieldControls();
            DestroyTask(taskId);
        }
        break;
    }
}

#undef tBGPriority
#undef tDISPCNT
#undef tBLDCNT
#undef tBLDALPHA
#undef tWININ
#undef tWINOUT
#undef tBlendTgt1
#undef tBlendTgt2
#undef tDuration
#undef tWindowId

u16 MapPreview_GetDuration(mapsec_u8_t mapsec)
{
    u16 flagId;
    bool8 visited;
    const struct MapPreviewScreen *preview = GetMapPreview(mapsec);
    if (preview == NULL)
        return 0;
    flagId = preview->flagId;

    if (flagId == MPS_FLAG_NULL)
    {
        return preview->duration ? preview->duration : MPS_DURATION_NO_FLAG;
    }
    else
    {
        if (!ADMG_TryReadFlag(flagId, &visited))
            return 0;
        if (!visited)
        {
            FlagSet(flagId);
            return preview->duration ? preview->duration : MPS_DURATION_LONG;
        }
        else
        {
            return preview->duration ? preview->duration : MPS_DURATION_SHORT;
        }
    }
}

void MapPreview_SetFlag(u16 flagId)
{
    bool8 visited;
    if (!ADMG_TryReadFlag(flagId, &visited))
        return;
    if (!visited)
    {
        sHasVisitedMapBefore = TRUE;
    }
    else
    {
        sHasVisitedMapBefore = FALSE;
    }
    FlagSet(flagId);
}

static void VblankCB_MapPreviewScript(void)
{
    TransferPlttBuffer();
}

static void CB2_MapPreviewScript(void)
{
    RunTasks();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

#define tTaskStep       data[0]
#define tFrameCounter   data[1]
#define tWindowId       data[2]
#define tDuration       data[3]

static void Task_MapPreviewScreen_Script(u8 taskId)
{
    s16 *data;

    data = gTasks[taskId].data;
    switch (tTaskStep)
    {
    case 0:
        if (!UpdatePaletteFade())
        {
            SetVBlankCallback(NULL);
            MapPreview_LoadGfx(gTasks[taskId].contentIds[0]);
            BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
            gMain.savedCallback = gTasks[taskId].contentIds[2]
                ? (MainCallback)(uintptr_t)gTasks[taskId].contentIds[1] : CB2_ReturnToFieldContinueScript;
            SetMainCallback2(CB2_MapPreviewScript);
            tTaskStep++;
        }
        break;
    case 1:
        if (!MapPreview_IsGfxLoadFinished() && !IsDma3ManagerBusyWithBgCopy())
        {
            const struct MapPreviewScreen *preview = GetMapPreview(gTasks[taskId].contentIds[0]);
            if (preview != NULL && !preview->nameDisabled)
            {
                tWindowId = MapPreview_CreateMapNameWindow(gTasks[taskId].contentIds[0]);
                CopyWindowToVram(tWindowId, COPYWIN_FULL);
            }
            tTaskStep++;
        }
        break;
    case 2:
        if (!IsDma3ManagerBusyWithBgCopy())
        {
            SetVBlankCallback(VblankCB_MapPreviewScript);
            FadeInFromBlack();
            tTaskStep++;
        }
        break;
    case 3:
        tFrameCounter++;
        if (tFrameCounter > tDuration || JOY_NEW(B_BUTTON))
        {
            BeginNormalPaletteFade(PALETTES_ALL, MPS_BASIC_FADE_SPEED, 0, 16, RGB_BLACK);
            tFrameCounter = 0;
            tTaskStep++;
        }
        break;
    case 4:
        if (!UpdatePaletteFade())
        {
            MapPreview_Unload(tWindowId);
            DestroyTask(taskId);
            SetMainCallback2(gMain.savedCallback);
        }
        break;
    }
}

void MapPreviewScript(struct ScriptContext *ctx)
{
    u32 duration = ScriptReadHalfword(ctx);
    u32 taskId;

    if (!CurrentMapHasPreviewScreen(MPS_TYPE_ANY))
        return;

    ScriptContext_Stop();
    FadeScreen(FADE_TO_BLACK, 0);
    taskId = CreateTask(Task_MapPreviewScreen_Script, 0);
    gTasks[taskId].contentIds[0] = gMapHeader.regionMapSectionId;
    gTasks[taskId].tWindowId = WINDOW_NONE;
    gTasks[taskId].tDuration = duration;
}

ADMG_NATIVE_ENTRY u32 ADMG_IsMapPreviewActive(void)
{
    return FuncIsActiveTask(Task_MapPreviewScreen_Script)
        || FuncIsActiveTask(Task_MapPreviewScreen_FadeIn)
        || FuncIsActiveTask(Task_MapPreviewScreen_NonFade);
}

ADMG_NATIVE_ENTRY u32 ADMG_TryStartAutomaticMapPreview(void)
{
    if (!ShouldRunMapPreview() || !CurrentMapHasPreviewScreen(MPS_TYPE_FADE_IN)) return FALSE;
    MapPreview_LoadGfx(gMapHeader.regionMapSectionId);
    RunMapPreviewScreenFadeIn(gMapHeader.regionMapSectionId);
    return TRUE;
}

ADMG_NATIVE_ENTRY u32 ADMG_RequestMapPreview(u32 section, void (*callback)(void))
{
    u32 taskId;
    const struct MapPreviewScreen *preview;
    if (ADMG_IsMapPreviewActive()) return ADMG_MAP_PREVIEW_BUSY;
    if (gSaveBlock1Ptr == NULL || gSaveBlock2Ptr == NULL || gMain.callback2 != CB2_Overworld
     || gMain.inBattle || ScriptContext_IsEnabled() || ArePlayerFieldControlsLocked())
        return ADMG_MAP_PREVIEW_FIELD_NOT_IDLE;
    preview = GetMapPreview(section);
    if (preview == NULL || callback == NULL) return ADMG_MAP_PREVIEW_MISSING;
    taskId = CreateTask(Task_MapPreviewScreen_Script, 0);
    if (taskId >= NUM_TASKS) return ADMG_MAP_PREVIEW_TASK_CAPACITY;
    sActivePreview = preview;
    gTasks[taskId].contentIds[0] = section;
    gTasks[taskId].contentIds[1] = (uintptr_t)callback;
    gTasks[taskId].contentIds[2] = TRUE;
    gTasks[taskId].tWindowId = WINDOW_NONE;
    gTasks[taskId].tDuration = MapPreview_GetDuration(section);
    LockPlayerFieldControls();
    FadeScreen(FADE_TO_BLACK, 0);
    return ADMG_MAP_PREVIEW_STARTED;
}

ADMG_RETAIN_FUNCTION(ADMG_GetRegisteredMapPreview);
ADMG_RETAIN_FUNCTION(ADMG_GetRetainedMapPreview);
ADMG_RETAIN_FUNCTION(ADMG_RequestMapPreview);
ADMG_RETAIN_FUNCTION(ADMG_IsMapPreviewActive);
ADMG_RETAIN_FUNCTION(ADMG_TryStartAutomaticMapPreview);
ADMG_RETAIN_FUNCTION(ShouldRunMapPreview);
ADMG_RETAIN_FUNCTION(MapPreviewScript);
ADMG_RETAIN_FUNCTION(RunMapPreviewScreenNonFade);
ADMG_RETAIN_FUNCTION(RunMapPreviewScreenFadeIn);
ADMG_RETAIN_DATA(sMapPreviewScreenData);
