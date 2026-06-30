#ifndef ZONE_EDITOR_H
#define ZONE_EDITOR_H

#include <SYSTEM/ADELINE_TYPES.H>

#ifdef LBA2CC_EDITOR
extern void ZoneOverlaySet(S32 ids, S32 boxes);
extern void ZoneOverlaySetMaxDist(S32 maxdist);
extern void ZoneOverlaySetType(S32 type);
extern S32 ZoneOverlayGetIds(void);
extern S32 ZoneOverlayGetBoxes(void);
extern S32 ZoneOverlayGetMaxDist(void);
extern S32 ZoneOverlayGetType(void);
extern void ZoneOverlayDraw(void);
extern void ZoneEditorBeginPick(void);
extern void ZoneEditorClearAllSelected(void);
extern void ZoneEditorSelectOnly(S32 index);
extern void ZoneEditorToggleSelectedByIndex(S32 index);
extern S32 ZoneEditorHasAnySelected(void);
extern T_ZONE *ZoneEditorGetSelectedZone(void);
extern S32 ZoneEditorLogSelected(void);
extern S32 ZoneEditorCreateZone(const T_ZONE *zone);
extern S32 ZoneEditorCreateLinkedDecorZone(const T_ZONE *zone, S32 decor_cube_x, S32 decor_cube_y, S32 decor_num, S32 decor_body, S32 decor_role, S32 decor_x, S32 decor_y, S32 decor_z, S32 decor_beta);
extern void ZoneEditorSyncLinkedDecorZones(S32 decor_cube_x, S32 decor_cube_y, S32 decor_num, S32 decor_body, S32 decor_x, S32 decor_y, S32 decor_z, S32 decor_beta);
extern S32 ZoneEditorSetSelectedField(S32 field, S32 value);
extern S32 ZoneEditorSaveRuntimeEdits(const char *island_dir, S32 island);
extern void ZoneEditorApplyRuntimeEditsForLoadedScene(S32 cube);
extern void ZoneEditorReleaseOwnedZoneList(void);
extern void ZoneEditorClearRuntimeEdits(void);
extern void ZoneEditorLoadAuthoringEditsForCurrentIsland(void);

#define ZONE_EDITOR_FIELD_TYPE 0
#define ZONE_EDITOR_FIELD_NUM 1
#define ZONE_EDITOR_FIELD_INFO0 2
#define ZONE_EDITOR_FIELD_INFO1 3
#define ZONE_EDITOR_FIELD_INFO2 4
#define ZONE_EDITOR_FIELD_X0 5
#define ZONE_EDITOR_FIELD_Y0 6
#define ZONE_EDITOR_FIELD_Z0 7
#define ZONE_EDITOR_FIELD_X1 8
#define ZONE_EDITOR_FIELD_Y1 9
#define ZONE_EDITOR_FIELD_Z1 10
#define ZONE_EDITOR_FIELD_COUNT 11
#endif

#endif /* ZONE_EDITOR_H */
