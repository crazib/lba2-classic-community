#ifndef TERRAIN_EDITOR_H
#define TERRAIN_EDITOR_H

#include <SYSTEM/ADELINE_TYPES.H>

#ifdef LBA2CC_EDITOR
extern void TerrainEditorSetSelect(S32 select);
extern S32 TerrainEditorGetSelect(void);
extern void TerrainEditorClearSelection(void);
extern S32 TerrainEditorConsumesInput(void);
extern void TerrainEditorClearRuntimeEdits(void);
extern void TerrainEditorLoadAuthoringEditsForCurrentIsland(void);
extern void TerrainEditorApplyRuntimeEditsForLoadedCube(S32 cube_x, S32 cube_y);
extern void TerrainEditorSetLoadedCube(S32 cube_x, S32 cube_y);
extern S32 TerrainEditorSaveRuntimeEdits(const char *island_dir, S32 island);
extern void TerrainEditorBeginFrame(void);
extern void TerrainEditorEndFrame(void);
extern void TerrainEditorOverlayDraw(void);
extern S32 TerrainEditorGetSelectedCount(void);
extern S32 TerrainEditorGetSelectedFieldValue(S32 field, S32 *mixed);
extern S32 TerrainEditorSetSelectedField(S32 field, S32 value);

#define TERRAIN_EDITOR_FIELD_CUBE_X 0
#define TERRAIN_EDITOR_FIELD_CUBE_Y 1
#define TERRAIN_EDITOR_FIELD_X 2
#define TERRAIN_EDITOR_FIELD_Z 3
#define TERRAIN_EDITOR_FIELD_HEIGHT 4
#define TERRAIN_EDITOR_FIELD_INTENSITY 5
#define TERRAIN_EDITOR_FIELD_COUNT 6
#endif

#endif /* TERRAIN_EDITOR_H */
