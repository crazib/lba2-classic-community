#ifndef EXT_DECOR_EDITOR_H
#define EXT_DECOR_EDITOR_H

#include <SYSTEM/ADELINE_TYPES.H>

#ifdef LBA2CC_EDITOR
extern void ExtDecorOverlaySet(S32 ids, S32 boxes);
extern void ExtDecorOverlaySetMaxDist(S32 maxdist);
extern S32 ExtDecorOverlayGetIds(void);
extern S32 ExtDecorOverlayGetBoxes(void);
extern S32 ExtDecorOverlayGetMaxDist(void);
extern void ExtDecorEditorSetSelect(S32 select);
extern S32 ExtDecorEditorGetSelect(void);
extern S32 ExtDecorEditorHasSelection(void);
extern void ExtDecorEditorClearSelection(void);
extern S32 ExtDecorEditorGetPickPending(void);
extern void ExtDecorEditorResolveClickPick(S32 zone_index, S32 zone_dist_sq, S32 actor_index, S32 actor_dist_sq, S32 waypoint_index, S32 waypoint_dist_sq);
extern S32 ExtDecorEditorConsumesInput(void);
extern S32 ExtDecorEditorGetPlacementPoint(S32 *x, S32 *y, S32 *z);
extern void ExtDecorEditorClearRuntimeEdits(void);
extern void ExtDecorEditorReleaseOwnedDecorList(void);
extern void ExtDecorEditorLoadAuthoringEditsForCurrentIsland(void);
extern void ExtDecorEditorSetLoadedCube(S32 cube_x, S32 cube_y);
extern void ExtDecorEditorApplyRuntimeEditsForLoadedCube(S32 cube_x, S32 cube_y);
extern void ExtDecorEditorBeginFrame(void);
extern void ExtDecorEditorEndFrame(void);
extern void ExtDecorOverlayDraw(void);
extern void ExtDecorEditorDeleteSelected(void);
extern void ExtDecorEditorDuplicateSelected(void);
extern void ExtDecorEditorPlacePaletteInDeletedSlot(void);
extern void ExtDecorEditorPaletteNext(S32 dir);
extern void ExtDecorEditorPaletteLibraryNext(S32 dir);
extern S32 ExtDecorEditorGetPaletteLibrary(void);
extern const char *ExtDecorEditorGetPaletteLibraryName(void);
extern S32 ExtDecorEditorGetPaletteBody(void);
extern void ExtDecorEditorPaletteDraw(S32 x0, S32 y0);
extern S32 ExtDecorEditorSaveRuntimeEdits(const char *island_dir, S32 island);
extern T_DECORS *ExtDecorEditorGetSelectedObject(void);
extern S32 ExtDecorEditorLogSelected(void);
extern S32 ExtDecorEditorSetSelectedField(S32 field, S32 value);

#define EXT_DECOR_FIELD_BODY 0
#define EXT_DECOR_FIELD_ROTATION 1
#define EXT_DECOR_FIELD_X 2
#define EXT_DECOR_FIELD_Y 3
#define EXT_DECOR_FIELD_Z 4
#define EXT_DECOR_FIELD_COUNT 5
#endif

#endif /* EXT_DECOR_EDITOR_H */
