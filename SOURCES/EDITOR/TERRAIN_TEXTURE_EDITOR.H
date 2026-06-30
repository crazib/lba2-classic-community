#ifndef TERRAIN_TEXTURE_EDITOR_H
#define TERRAIN_TEXTURE_EDITOR_H

#include <SYSTEM/ADELINE_TYPES.H>

#ifdef LBA2CC_EDITOR
extern void TerrainTextureEditorSetSelect(S32 select);
extern S32 TerrainTextureEditorGetSelect(void);
extern void TerrainTextureEditorClearSelection(void);
extern S32 TerrainTextureEditorConsumesInput(void);
extern void TerrainTextureEditorClearRuntimeEdits(void);
extern void TerrainTextureEditorApplyRuntimeEditsForLoadedCube(S32 cube_x, S32 cube_y);
extern void TerrainTextureEditorBuildCatalog(void);
extern void TerrainTextureEditorSetTexDefs(T_HALF_TEX *defs, S32 count);
extern T_HALF_POLY *TerrainTextureEditorGetSelectedHalfPoly();
extern S32 TerrainTextureEditorGetSelectedCount(void);
extern S32 TerrainTextureEditorGetSelectedFieldValue(S32 field, S32 *mixed);
extern S32 TerrainTextureEditorGetQuadMode(void);
extern S32 TerrainTextureEditorSetSelectedField(S32 field, S32 value);
extern void TerrainTextureEditorBeginFrame(void);
extern void TerrainTextureEditorEndFrame(void);
extern void TerrainTextureEditorOverlayDraw(void);

#define TERRAIN_TEXTURE_FIELD_BANK 0
#define TERRAIN_TEXTURE_FIELD_TEX_FLAG 1
#define TERRAIN_TEXTURE_FIELD_POLY_FLAG 2
#define TERRAIN_TEXTURE_FIELD_SAMPLE_STEP 3
#define TERRAIN_TEXTURE_FIELD_CODE_JEU 4
#define TERRAIN_TEXTURE_FIELD_SENS 5
#define TERRAIN_TEXTURE_FIELD_COL 6
#define TERRAIN_TEXTURE_FIELD_DUMMY 7
#define TERRAIN_TEXTURE_FIELD_INDEX_TEX 8
#define TERRAIN_TEXTURE_FIELD_COUNT 9
#endif

#endif /* TERRAIN_TEXTURE_EDITOR_H */
