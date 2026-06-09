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
extern void TerrainEditorApplyRuntimeEditsForLoadedCube(S32 cube_x, S32 cube_y);
extern void TerrainEditorSetLoadedCube(S32 cube_x, S32 cube_y);
extern void TerrainEditorBeginFrame(void);
extern void TerrainEditorEndFrame(void);
extern void TerrainEditorOverlayDraw(void);
extern void TerrainTextureEditorSetSelect(S32 select);
extern S32 TerrainTextureEditorGetSelect(void);
extern void TerrainTextureEditorClearSelection(void);
extern S32 TerrainTextureEditorConsumesInput(void);
extern void TerrainTextureEditorClearRuntimeEdits(void);
extern void TerrainTextureEditorApplyRuntimeEditsForLoadedCube(S32 cube_x, S32 cube_y);
extern void TerrainTextureEditorSetTexDefCount(S32 count);
extern void TerrainTextureEditorBeginFrame(void);
extern void TerrainTextureEditorEndFrame(void);
extern void TerrainTextureEditorOverlayDraw(void);
#endif

#endif // TERRAIN_H
