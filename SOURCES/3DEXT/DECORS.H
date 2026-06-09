#ifndef DECORS_H
#define DECORS_H

/*--------------------------------------------------------------------------*/
extern void FixeObjetsDecorsInvisibles(void);
/*--------------------------------------------------------------------------*/
extern int sub_qsort_decors(const void *a, const void *b);
/*--------------------------------------------------------------------------*/
extern int sub_qsort_decors_zbuf(const void *a, const void *b);
/*--------------------------------------------------------------------------*/
extern void AffichageObjetDecorsZV(void);
/*--------------------------------------------------------------------------*/
extern void AffichageObjetDecorsZBuf(void);
/*--------------------------------------------------------------------------*/
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
extern void ExtDecorEditorResolveClickPick(S32 zone_index, S32 zone_dist_sq);
extern S32 ExtDecorEditorConsumesInput(void);
extern void ExtDecorEditorClearRuntimeEdits(void);
extern void ExtDecorEditorApplyRuntimeEditsForLoadedCube(S32 cube_x, S32 cube_y);
extern void ExtDecorEditorBeginFrame(void);
extern void ExtDecorEditorEndFrame(void);
extern void ExtDecorOverlayDraw(void);
/*--------------------------------------------------------------------------*/
#endif
extern S32 TestZVDecors(S32 xw, S32 yw, S32 zw, T_DECORS *objet);

#endif // DECORS_H
