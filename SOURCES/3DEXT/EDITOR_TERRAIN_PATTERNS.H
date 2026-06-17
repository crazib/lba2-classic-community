#ifndef EDITOR_TERRAIN_PATTERNS_H
#define EDITOR_TERRAIN_PATTERNS_H

#include <SYSTEM/ADELINE_TYPES.H>

#define TERRAIN_REGION_TYPE_GRASS 0
#define TERRAIN_REGION_TYPE_PAVEMENT 1
#define TERRAIN_REGION_TYPE_GRASS_INSET 2
#define TERRAIN_REGION_TYPE_ASPHALT 3
#define TERRAIN_REGION_TYPE_GRASS_PAVEMENT 4
#define TERRAIN_REGION_TYPE_RIPRAP 5
#define TERRAIN_REGION_TYPE_PAVEMENT_ALT 6
#define TERRAIN_REGION_TYPE_ROCKS 7
#define TERRAIN_REGION_TYPE_CLIFFSIDE 8
#define TERRAIN_REGION_TYPE_DIRT 9
#define TERRAIN_REGION_TYPE_COUNT 10

typedef struct {
    S32 X0;
    S32 Z0;
    S32 X1;
    S32 Z1;
    S32 AnchorX;
    S32 AnchorZ;
    S32 NorthWest;
    S32 North;
    S32 NorthEast;
    S32 West;
    S32 East;
    S32 SouthWest;
    S32 South;
    S32 SouthEast;
} T_EDITOR_TERRAIN_PATTERN_REGION;

typedef struct {
    S32 Texture0;
    S32 Texture1;
    S32 Sens0;
    S32 Sens1;
    S32 Footstep0;
    S32 Footstep1;
} T_EDITOR_TERRAIN_PATTERN_CELL;

extern void EditorTerrainPatternPavement(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                         S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell);
extern void EditorTerrainPatternPavementAlt(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                            S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell);
extern void EditorTerrainPatternGrassInset(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                           S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell);
extern void EditorTerrainPatternAsphalt(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                        S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell);
extern void EditorTerrainPatternGrassPavement(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                              S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell);
extern void EditorTerrainPatternRocks(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                      S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell);
extern void EditorTerrainPatternCliffside(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                          S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell);
extern void EditorTerrainPatternDirt(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                     S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell);

#endif
