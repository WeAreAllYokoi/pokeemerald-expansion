/* Layouts of the C structs whose instances are emitted as assembly (map data from tools/mapjson, tilesets, sound headers),
   compiled against a source tree's headers. asm_layout_probe.sh compares two trees; a struct whose layout differs needs
   its assembly emitter changed with it. */
#include "global.h"
#include "global.fieldmap.h"
#include "gba/m4a_internal.h"
#include <stddef.h>
#define S(T) sizeof(struct T)
#define O(T,m) offsetof(struct T, m)
#ifdef FIRST_ADDED_MAP
/* From recipe 37 a map is one 32-bit MapId: the former group and number members both read as its offset, so their rows
   show where the map went */
#define mapGroup mapId
#define mapNum mapId
#endif
const unsigned int gProbe[] = {
 S(MapHeader), O(MapHeader,music), O(MapHeader,mapLayoutId), O(MapHeader,regionMapSectionId), O(MapHeader,weather),
 O(MapHeader,mapType), O(MapHeader,floorNumber), O(MapHeader,nightMusic), O(MapHeader,battleType),
 S(MapLayout), O(MapLayout,width), O(MapLayout,height), O(MapLayout,border), O(MapLayout,map), O(MapLayout,primaryTileset),
 O(MapLayout,secondaryTileset), O(MapLayout,isFrlg), O(MapLayout,borderWidth), O(MapLayout,borderHeight),
 S(ObjectEventTemplate), O(ObjectEventTemplate,localId), O(ObjectEventTemplate,graphicsId), O(ObjectEventTemplate,kind),
 O(ObjectEventTemplate,x), O(ObjectEventTemplate,y), O(ObjectEventTemplate,script), O(ObjectEventTemplate,flagId),
 S(WarpEvent), O(WarpEvent,elevation), O(WarpEvent,warpId), O(WarpEvent,mapNum), O(WarpEvent,mapGroup),
 S(CoordEvent), O(CoordEvent,elevation), O(CoordEvent,trigger), O(CoordEvent,index), O(CoordEvent,script),
 S(BgEvent), O(BgEvent,elevation), O(BgEvent,kind), O(BgEvent,bgUnion),
 S(MapEvents), O(MapEvents,objectEvents), O(MapEvents,bgEvents),
 S(MapConnection), O(MapConnection,offset), O(MapConnection,mapGroup), O(MapConnection,mapNum),
 S(MapConnections), O(MapConnections,connections),
 S(Tileset),
 S(WaveData), S(ToneData), S(SongHeader), O(SongHeader,tone), O(SongHeader,part), S(Song), O(Song,ms), O(Song,me),
};
