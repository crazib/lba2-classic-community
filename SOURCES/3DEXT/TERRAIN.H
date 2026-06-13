#ifndef TERRAIN_H
#define TERRAIN_H

#define Y_SEA -50

extern S32 Sky_Y;
extern S32 Reglage;

/*--------------------------------------------------------------------------*/
extern void DrawSkySeaZBuf(void);
/*--------------------------------------------------------------------------*/
extern void ClsTerrainZBuf(void);
/*--------------------------------------------------------------------------*/
extern void DrawSeenSea(S32 y);
/*--------------------------------------------------------------------------*/
extern void DrawHorizon2ZBuf(void);
/*--------------------------------------------------------------------------*/
extern void AffichageTerrainZBuf(void);
/*--------------------------------------------------------------------------*/
extern void AffichageTerrainRapide(void);
/*--------------------------------------------------------------------------*/
extern void CopyBlock256(U8 *sour, U8 *dest);
/*--------------------------------------------------------------------------*/
extern void ClearAniPoly(void);
/*--------------------------------------------------------------------------*/
extern void DoAnimatedPolys();
/*--------------------------------------------------------------------------*/
extern void DrawAnimatedPolys();
/*--------------------------------------------------------------------------*/
extern void FiltreAnimatedPolys();
/*--------------------------------------------------------------------------*/

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
extern void TerrainRegionEditorSetSelect(S32 select);
extern S32 TerrainRegionEditorGetSelect(void);
extern void TerrainRegionEditorBeginFrame(void);
extern void TerrainRegionEditorEndFrame(void);
extern void TerrainRegionEditorOverlayDraw(void);
extern S32 TerrainRegionEditorHasSelected(void);
extern void TerrainRegionEditorClearSelection(void);
extern S32 TerrainRegionEditorGetFieldValue(S32 field);
extern S32 TerrainRegionEditorSetFieldValue(S32 field, S32 value);
extern S32 TerrainRegionEditorActivateField(S32 field);

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

#define TERRAIN_EDITOR_FIELD_CUBE_X 0
#define TERRAIN_EDITOR_FIELD_CUBE_Y 1
#define TERRAIN_EDITOR_FIELD_X 2
#define TERRAIN_EDITOR_FIELD_Z 3
#define TERRAIN_EDITOR_FIELD_HEIGHT 4
#define TERRAIN_EDITOR_FIELD_INTENSITY 5
#define TERRAIN_EDITOR_FIELD_COUNT 6

#define TERRAIN_REGION_FIELD_BAKE 0
#define TERRAIN_REGION_FIELD_TYPE 1
#define TERRAIN_REGION_FIELD_SEED 2
#define TERRAIN_REGION_FIELD_PATCHINESS 3
#define TERRAIN_REGION_FIELD_RANDOMIZE 4
#define TERRAIN_REGION_FIELD_X0 5
#define TERRAIN_REGION_FIELD_Z0 6
#define TERRAIN_REGION_FIELD_X1 7
#define TERRAIN_REGION_FIELD_Z1 8
#define TERRAIN_REGION_FIELD_COUNT 9
#endif

#endif // TERRAIN_H
