#ifndef EDITOR_H
#define EDITOR_H

#include <SYSTEM/ADELINE_TYPES.H>

#ifdef LBA2CC_EDITOR

typedef enum EditorTool
{
    EDITOR_TOOL_SELECT_MOVE = 0,
    EDITOR_TOOL_DECOR_PLACE,
    EDITOR_TOOL_TERRAIN_EDIT,
    EDITOR_TOOL_TEXTURE_SELECT
} EditorTool;

#define EDITOR_OVERLAY_DECOR (1 << 0)
#define EDITOR_OVERLAY_ZONES (1 << 1)

extern void Editor_UpdateInput(void);
extern void Editor_DrawOverlay(void);
extern S32 Editor_IsActive(void);
extern S32 Editor_ResolveOutputDir(char *outPath, S32 pathMax);
extern void Editor_SetStatus(const char *text);
extern EditorTool Editor_GetTool(void);
extern S32 Editor_ToolIs(EditorTool tool);
extern S32 Editor_HasOverlay(S32 overlay);

#else

#define Editor_UpdateInput() ((void)0)
#define Editor_DrawOverlay() ((void)0)
#define Editor_IsActive() (0)
#define Editor_ResolveOutputDir(outPath, pathMax) (0)
#define Editor_SetStatus(text) ((void)0)
#define Editor_GetTool() (0)
#define Editor_ToolIs(tool) (0)
#define Editor_HasOverlay(overlay) (0)

#endif

#endif /* EDITOR_H */
