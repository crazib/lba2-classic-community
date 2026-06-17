#include "EDITOR_TERRAIN_PATTERNS.H"

#include <assert.h>

static void assert_rocks_cell(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                              S32 x, S32 z, S32 texture0, S32 texture1) {
    T_EDITOR_TERRAIN_PATTERN_CELL cell;

    EditorTerrainPatternRocks(region, x, z, &cell);
    assert(cell.Texture0 == texture0);
    assert(cell.Texture1 == texture1);
    assert(cell.Sens0 == 0);
    assert(cell.Sens1 == 1);
    assert(cell.Footstep0 == 5);
    assert(cell.Footstep1 == 5);
}

static void assert_cliffside_cell(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                  S32 x, S32 z, S32 texture0, S32 texture1, S32 sens1) {
    T_EDITOR_TERRAIN_PATTERN_CELL cell;

    EditorTerrainPatternCliffside(region, x, z, &cell);
    assert(cell.Texture0 == texture0);
    assert(cell.Texture1 == texture1);
    assert(cell.Sens0 == 0);
    assert(cell.Sens1 == sens1);
    assert(cell.Footstep0 == 5);
    assert(cell.Footstep1 == 5);
}

static void assert_dirt_cell(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                             S32 x, S32 z) {
    T_EDITOR_TERRAIN_PATTERN_CELL cell;

    EditorTerrainPatternDirt(region, x, z, &cell);
    assert(cell.Texture0 == -1);
    assert(cell.Texture1 == -1);
    assert(cell.Sens0 == 0);
    assert(cell.Sens1 == 0);
    assert(cell.Footstep0 == 0);
    assert(cell.Footstep1 == 0);
}

int main(void) {
    T_EDITOR_TERRAIN_PATTERN_REGION region = {};

    region.AnchorX = 35;
    region.AnchorZ = 12;

    assert_rocks_cell(&region, 35, 12, 58, 59);
    assert_rocks_cell(&region, 36, 12, 45, 46);
    assert_rocks_cell(&region, 37, 12, 38, 39);
    assert_rocks_cell(&region, 38, 12, 56, 57);
    assert_rocks_cell(&region, 35, 13, 53, 54);
    assert_rocks_cell(&region, 36, 13, 49, 50);
    assert_rocks_cell(&region, 37, 13, 40, 41);
    assert_rocks_cell(&region, 38, 13, 55, 52);
    assert_rocks_cell(&region, 39, 14, 58, 59);

    region.AnchorX = 26;
    region.AnchorZ = 24;

    assert_cliffside_cell(&region, 26, 24, 64, 66, 1);
    assert_cliffside_cell(&region, 27, 24, 67, 65, 1);
    assert_cliffside_cell(&region, 26, 25, 60, 61, 0);
    assert_cliffside_cell(&region, 27, 25, 62, 63, 0);
    assert_cliffside_cell(&region, 28, 26, 64, 66, 1);

    assert_dirt_cell(&region, 26, 24);
    assert_dirt_cell(&region, 33, 46);

    return 0;
}
