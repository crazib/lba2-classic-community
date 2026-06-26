#ifndef WAYPOINT_EDITOR_H
#define WAYPOINT_EDITOR_H

#include <SYSTEM/ADELINE_TYPES.H>

#ifdef LBA2CC_EDITOR

extern void WaypointEditorOnSceneLoaded(S32 scene);
extern void WaypointEditorClearSelection(void);
extern S32 WaypointEditorGetSelectedIndex(void);
extern void WaypointEditorResolvePick(S32 index, S32 toggle);
extern S32 WaypointEditorGetFieldValue(S32 field);
extern S32 WaypointEditorSetFieldValue(S32 field, S32 value);
extern void WaypointEditorNudgeSelected(S32 dx, S32 dy, S32 dz);
extern S32 WaypointEditorCreateAt(S32 x, S32 y, S32 z);
extern S32 WaypointEditorSaveRuntimeEdits(const char *island_dir, S32 island);

#else

#define WaypointEditorOnSceneLoaded(scene) ((void)0)
#define WaypointEditorClearSelection() ((void)0)
#define WaypointEditorGetSelectedIndex() (-1)
#define WaypointEditorResolvePick(index, toggle) ((void)0)
#define WaypointEditorGetFieldValue(field) (0)
#define WaypointEditorSetFieldValue(field, value) (0)
#define WaypointEditorNudgeSelected(dx, dy, dz) ((void)0)
#define WaypointEditorCreateAt(x, y, z) (-1)
#define WaypointEditorSaveRuntimeEdits(island_dir, island) (1)

#endif

#endif
