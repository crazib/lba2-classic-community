#include "EDITOR_TERRAIN_PATTERNS.H"

static void EditorTerrainPatternDefaults(T_EDITOR_TERRAIN_PATTERN_CELL *cell) {
    cell->Sens0 = 0;
    cell->Sens1 = 1;
    cell->Footstep0 = -1;
    cell->Footstep1 = -1;
}

static void EditorTerrainPatternPavementSens(T_EDITOR_TERRAIN_PATTERN_CELL *cell) {
    if (cell->Texture0 == 360 OR cell->Texture0 == 368
            OR cell->Texture0 == 383 OR cell->Texture0 == 392 OR cell->Texture0 == 446
            OR cell->Texture0 == 450 OR cell->Texture0 == 452)
        cell->Sens1 = 0;
}

void EditorTerrainPatternPavement(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                  S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell) {
    static const S32 interior[2][4] = {
        {358, 360, 362, 364},
        {366, 368, 370, 372}
    };
    S32 base;

    EditorTerrainPatternDefaults(cell);
    cell->Footstep0 = 5;
    cell->Footstep1 = 5;

    if (!region->North AND !region->West) {
        cell->Texture0 = 385;
        cell->Texture1 = 384;
    } else if (!region->North AND !region->East) {
        cell->Texture0 = 387;
        cell->Texture1 = 386;
    } else if (!region->South AND !region->West) {
        cell->Texture0 = 392;
        cell->Texture1 = 393;
    } else if (!region->South AND !region->East) {
        cell->Texture0 = 446;
        cell->Texture1 = 447;
    } else if (!region->North) {
        base = ((x - region->X0 - 1) & 1) ? 460 : 458;
        cell->Texture0 = base;
        cell->Texture1 = base + 1;
    } else if (!region->South) {
        base = ((x - region->X0 - 1) & 1) ? 456 : 454;
        cell->Texture0 = base;
        cell->Texture1 = base + 1;
    } else if (!region->West) {
        base = ((z - region->Z0 - 1) & 1) ? 400 : 398;
        cell->Texture0 = base;
        cell->Texture1 = base + 1;
    } else if (!region->East) {
        base = ((z - region->Z0 - 1) & 1) ? 452 : 450;
        cell->Texture0 = base;
        cell->Texture1 = base + 1;
    } else if (!region->NorthWest) {
        cell->Texture0 = 383;
        cell->Texture1 = 382;
    } else if (!region->NorthEast) {
        cell->Texture0 = 385;
        cell->Texture1 = 384;
    } else if (!region->SouthWest) {
        cell->Texture0 = 393;
        cell->Texture1 = 392;
    } else if (!region->SouthEast) {
        cell->Texture0 = 397;
        cell->Texture1 = 396;
    } else {
        base = interior[(z - region->Z0 - 1) & 1][(x - region->X0 - 1) & 3];
        cell->Texture0 = base;
        cell->Texture1 = base + 1;
    }
    EditorTerrainPatternPavementSens(cell);
}

void EditorTerrainPatternPavementAlt(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                     S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell) {
    static const S32 base[2][4] = {
        {614, 616, 622, 624},
        {618, 620, 626, 628}
    };
    S32 texture = base[(z - region->AnchorZ) & 1][(x - region->AnchorX) & 3];

    EditorTerrainPatternDefaults(cell);
    cell->Texture0 = texture;
    cell->Texture1 = texture + 1;
    cell->Footstep0 = 5;
    cell->Footstep1 = 5;
}

void EditorTerrainPatternGrassInset(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                    S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell) {
    static const S32 interior[4][3] = {
        {20, 26, 28},
        {14, 32, 34},
        {16, 22, 11},
        {18, 24, 7}
    };
    S32 rel_x = x - region->X0;
    S32 rel_z = z - region->Z0;
    S32 width = region->X1 - region->X0 + 1;
    S32 height = region->Z1 - region->Z0 + 1;
    S32 base;

    EditorTerrainPatternDefaults(cell);
    if (rel_z == 0) {
        if (rel_x == 0) {
            cell->Texture0 = 589;
            cell->Texture1 = 588;
            return;
        }
        if (rel_x == width - 1) {
            cell->Texture0 = 594;
            cell->Texture1 = 595;
            return;
        }
        base = ((rel_x - 1) & 1) ? 592 : 590;
    } else if (rel_z == height - 1) {
        if (rel_x == 0) {
            cell->Texture0 = 595;
            cell->Texture1 = 594;
            return;
        }
        if (rel_x == width - 1) {
            cell->Texture0 = 588;
            cell->Texture1 = 589;
            return;
        }
        base = ((rel_x - 1) & 1) ? 586 : 584;
    } else if (rel_x == 0) {
        base = (rel_z & 1) ? 596 : 598;
    } else if (rel_x == width - 1) {
        base = (rel_z & 1) ? 580 : 582;
    } else {
        base = interior[(rel_z - 1) & 3][(rel_x - 1) % 3];
    }

    cell->Texture0 = base;
    cell->Texture1 = (base == 11 OR base == 7) ? base - 1 : base + 1;
}

void EditorTerrainPatternAsphalt(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                 S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell) {
    static const S32 base[2][2] = {
        {634, 636},
        {638, 640}
    };
    S32 texture = base[(z - region->Z0) & 1][(x - region->X0) & 1];

    EditorTerrainPatternDefaults(cell);
    cell->Texture0 = texture;
    cell->Texture1 = texture + 1;
}

void EditorTerrainPatternGrassPavement(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                       S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell) {
    static const S32 top[4] = {428, 430, 432, 426};
    static const S32 middle[4] = {434, 436, 438, 440};
    static const S32 bottom0[4] = {439, 437, 435, 441};
    static const S32 bottom1[4] = {438, 436, 434, 440};
    S32 rel_x = x - region->X0;

    EditorTerrainPatternDefaults(cell);
    cell->Footstep0 = 5;
    cell->Footstep1 = 5;
    if (z == region->Z0) {
        if (x == region->X0) {
            cell->Texture0 = 462;
            cell->Texture1 = 463;
        } else if (x == region->X1) {
            cell->Texture0 = 442;
            cell->Texture1 = 443;
            cell->Sens1 = 0;
            cell->Footstep1 = 2;
        } else {
            cell->Texture0 = top[(rel_x - 1) & 3];
            cell->Texture1 = cell->Texture0 + 1;
        }
    } else if (z == region->Z1) {
        cell->Sens1 = 0;
        if (x == region->X0) {
            cell->Texture0 = 464;
            cell->Texture1 = 465;
            cell->Footstep0 = 2;
        } else if (x == region->X1) {
            cell->Texture0 = 444;
            cell->Texture1 = 445;
        } else {
            cell->Texture0 = bottom0[(rel_x - 1) & 3];
            cell->Texture1 = bottom1[(rel_x - 1) & 3];
        }
    } else {
        cell->Texture0 = middle[rel_x & 3];
        cell->Texture1 = cell->Texture0 + 1;
        if (x == region->X1)
            cell->Sens1 = 0;
    }
}

void EditorTerrainPatternRocks(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                               S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell) {
    static const S32 texture0[2][4] = {
        {58, 45, 38, 56},
        {53, 49, 40, 55}
    };
    static const S32 texture1[2][4] = {
        {59, 46, 39, 57},
        {54, 50, 41, 52}
    };
    S32 row = (z - region->AnchorZ) & 1;
    S32 col = (x - region->AnchorX) & 3;

    EditorTerrainPatternDefaults(cell);
    cell->Texture0 = texture0[row][col];
    cell->Texture1 = texture1[row][col];
    cell->Footstep0 = 5;
    cell->Footstep1 = 5;
}

void EditorTerrainPatternCliffside(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                                   S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell) {
    static const S32 texture0[2][2] = {
        {64, 67},
        {60, 62}
    };
    static const S32 texture1[2][2] = {
        {66, 65},
        {61, 63}
    };
    S32 row = (z - region->AnchorZ) & 1;
    S32 col = (x - region->AnchorX) & 1;

    EditorTerrainPatternDefaults(cell);
    cell->Texture0 = texture0[row][col];
    cell->Texture1 = texture1[row][col];
    cell->Sens1 = row ? 0 : 1;
    cell->Footstep0 = 5;
    cell->Footstep1 = 5;
}

void EditorTerrainPatternDirt(const T_EDITOR_TERRAIN_PATTERN_REGION *region,
                              S32 x, S32 z, T_EDITOR_TERRAIN_PATTERN_CELL *cell) {
    (void)region;
    (void)x;
    (void)z;

    EditorTerrainPatternDefaults(cell);
    cell->Texture0 = -1;
    cell->Texture1 = -1;
    cell->Sens1 = 0;
    cell->Footstep0 = 0;
    cell->Footstep1 = 0;
}
