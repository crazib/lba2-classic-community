#ifndef EDITOR_H
#define EDITOR_H

#include <SYSTEM/ADELINE_TYPES.H>

#ifdef LBA2CC_EDITOR

typedef enum EditorTool
{
    EDITOR_TOOL_SELECT_MOVE = 0,
    EDITOR_TOOL_DECOR_PLACE,
    EDITOR_TOOL_TERRAIN_EDIT,
    EDITOR_TOOL_TEXTURE_SELECT,
    EDITOR_TOOL_TERRAIN_REGION
} EditorTool;

#define EDITOR_OVERLAY_DECOR (1 << 0)
#define EDITOR_OVERLAY_ZONES (1 << 1)
#define EDITOR_OVERLAY_ACTORS (1 << 2)
#define EDITOR_OVERLAY_WAYPOINTS (1 << 3)

extern void Editor_UpdateInput(void);
extern void Editor_UpdateFreeCam(void);
extern void Editor_DrawOverlay(void);
extern bool Editor_IsActive(void);
extern bool Editor_IsCameraPanActive(void);
extern S32 Editor_ResolveOutputDir(char *outPath, S32 pathMax);
extern S32 Editor_FindExteriorScene(S32 cube_x, S32 cube_y);
extern void Editor_SetStatus(const char *text);
extern EditorTool Editor_GetTool(void);
extern bool Editor_ToolIs(EditorTool tool);
extern bool Editor_HasOverlay(S32 overlay);
extern bool Editor_CubeAllocateModeActive(void);
extern bool Editor_InspectorConsumesInput(void);
extern bool Editor_WorldPlacementActive(void);
extern void Editor_PlaceAtWorld(S32 x, S32 y, S32 z);
extern S32 Editor_SelectPickPending(void);
extern void Editor_ResolveSelectPick(S32 zone_index, S32 zone_dist_sq, S32 actor_index, S32 actor_dist_sq, S32 waypoint_index, S32 waypoint_dist_sq);
extern S32 ActorEditorFindPick(S32 *dist_sq);
extern S32 WaypointEditorFindPick(S32 *dist_sq);

#else

#define Editor_UpdateInput() ((void)0)
#define Editor_UpdateFreeCam() ((void)0)
#define Editor_DrawOverlay() ((void)0)
#define Editor_IsActive() (0)
#define Editor_IsCameraPanActive() (0)
#define Editor_ResolveOutputDir(outPath, pathMax) (0)
#define Editor_FindExteriorScene(cube_x, cube_y) (0)
#define Editor_SetStatus(text) ((void)0)
#define Editor_GetTool() (0)
#define Editor_ToolIs(tool) (0)
#define Editor_HasOverlay(overlay) (0)
#define Editor_CubeAllocateModeActive() (0)
#define Editor_InspectorConsumesInput() (0)
#define Editor_WorldPlacementActive() (0)
#define Editor_PlaceAtWorld(x, y, z) ((void)0)
#define Editor_SelectPickPending() (0)
#define Editor_ResolveSelectPick(zone_index, zone_dist_sq, actor_index, actor_dist_sq, waypoint_index, waypoint_dist_sq) ((void)0)
#define ActorEditorFindPick(dist_sq) (-1)
#define WaypointEditorFindPick(dist_sq) (-1)

#endif

#endif /* EDITOR_H */
