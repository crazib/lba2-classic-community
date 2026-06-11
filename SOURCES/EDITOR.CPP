#include "EDITOR.H"

#ifdef LBA2CC_EDITOR

#include "C_EXTERN.H"
#include "DIRECTORIES.H"
#include "3DEXT/DECORS.H"
#include "3DEXT/TERRAIN.H"

#include <SVGA/SCREEN.H>
#include <SVGA/DIRTYBOX.H>
#include <SYSTEM/FILES.H>
#include <SYSTEM/KEYBOARD.H>
#include <SYSTEM/KEYBOARD_KEYS.H>
#include <SYSTEM/INPUT.H>
#include <SYSTEM/LIMITS.H>
#include <SYSTEM/LOG.H>
#include <SYSTEM/MOUSE.H>

#include <stdio.h>
#include <string.h>

extern S32 CubeMode;
extern S32 Island;
extern void ZoneEditorClearAllSelected(void);

static S32 EditorVisible = FALSE;
static S32 EditorLastSave = FALSE;
static S32 EditorLastNextTool = FALSE;
static S32 EditorLastPrevTool = FALSE;
static S32 EditorLastDecorOverlay = FALSE;
static S32 EditorLastZoneOverlay = FALSE;
static S32 EditorLastOverlayDetail = FALSE;
static S32 EditorLastEscape = FALSE;
static S32 EditorLastEnter = FALSE;
static S32 EditorSuppressEscapeUntilRelease = FALSE;
static S32 EditorSuppressEnterUntilRelease = FALSE;
static S32 EditorMouseShown = FALSE;
static EditorTool EditorActiveTool = EDITOR_TOOL_SELECT_MOVE;
static S32 EditorOverlayFlags = EDITOR_OVERLAY_DECOR | EDITOR_OVERLAY_ZONES;
static S32 EditorOverlayDetailIds = FALSE;
static U32 EditorStatusTimerEnd = 0;
static char EditorStatus[220] = "";

static void EditorTrimLine(char *text) {
    size_t len;

    while (*text == ' ' || *text == '\t')
        memmove(text, text + 1, strlen(text));

    len = strlen(text);
    while (len > 0 && (text[len - 1] == '\r' || text[len - 1] == '\n' || text[len - 1] == ' ' || text[len - 1] == '\t')) {
        text[len - 1] = '\0';
        len--;
    }
}

void Editor_SetStatus(const char *text) {
    if (!text)
        return;
    strncpy(EditorStatus, text, sizeof(EditorStatus));
    EditorStatus[sizeof(EditorStatus) - 1] = '\0';
    EditorStatusTimerEnd = TimerRefHR + 1000;
}

static S32 Editor_ReadOutputRoot(char *outPath, S32 pathMax) {
    char pref[ADELINE_MAX_PATH];
    char path[ADELINE_MAX_PATH];
    FILE *f;

    GetDefaultUserDir(pref);
    if (pref[0] == '\0') {
        Editor_SetStatus("Editor save refused: app-support path unavailable");
        Log_Warn("[editor] editor_output_dir.txt read failed: app-support path unavailable");
        return FALSE;
    }

    snprintf(path, sizeof(path), "%seditor_output_dir.txt", pref);
    f = fopen(path, "rb");
    if (!f) {
        Editor_SetStatus("Editor save refused: editor_output_dir.txt missing");
        Log_Warn("[editor] editor_output_dir.txt missing at %s", path);
        return FALSE;
    }

    if (!fgets(outPath, pathMax, f)) {
        fclose(f);
        Editor_SetStatus("Editor save refused: editor_output_dir.txt is empty");
        Log_Warn("[editor] editor_output_dir.txt is empty at %s", path);
        return FALSE;
    }
    fclose(f);

    outPath[pathMax - 1] = '\0';
    EditorTrimLine(outPath);
    if (outPath[0] == '\0') {
        Editor_SetStatus("Editor save refused: editor_output_dir.txt is empty");
        Log_Warn("[editor] editor_output_dir.txt is empty at %s", path);
        return FALSE;
    }
    if (!IsDirectory(outPath)) {
        Editor_SetStatus("Editor save refused: editor_output_dir.txt target is missing");
        Log_Warn("[editor] editor_output_dir.txt target is not a directory: %s", outPath);
        return FALSE;
    }
    return TRUE;
}

static const char *Editor_IslandFolder(S32 island) {
    if (island == 0)
        return "citadel";
    if (island == 12)
        return "proxima";
    return NULL;
}

S32 Editor_ResolveOutputDir(char *outPath, S32 pathMax) {
    char root[ADELINE_MAX_PATH];
    const char *folder;

    if (!Editor_ReadOutputRoot(root, sizeof(root)))
        return FALSE;

    folder = Editor_IslandFolder(Island);
    if (!folder) {
        Editor_SetStatus("Editor save refused: no mod output folder for this island");
        Log_Warn("[editor] no mod output folder for island %d", (int)Island);
        return FALSE;
    }

    if (root[strlen(root) - 1] == '/' || root[strlen(root) - 1] == '\\')
        snprintf(outPath, pathMax, "%s%s", root, folder);
    else
        snprintf(outPath, pathMax, "%s%s%s", root, ADELINE_PATH_SEP, folder);
    outPath[pathMax - 1] = '\0';

    if (!IsDirectory(outPath)) {
        Editor_SetStatus("Editor save refused: island output folder is missing");
        Log_Warn("[editor] island output folder is not a directory: %s", outPath);
        return FALSE;
    }

    return TRUE;
}

static void Editor_Save(void) {
    char outPath[ADELINE_MAX_PATH];
    char status[260];

    if (!Editor_ResolveOutputDir(outPath, sizeof(outPath)))
        return;

    if (!ExtDecorEditorSaveRuntimeEdits(outPath, Island))
        return;
    if (!TerrainEditorSaveRuntimeEdits(outPath, Island))
        return;

    snprintf(status, sizeof(status), "Saved editor edits: %s", outPath);
    Editor_SetStatus(status);
    Log_Info("[editor] Ctrl+S saved editor output to: %s", outPath);
}

EditorTool Editor_GetTool(void) {
    return EditorActiveTool;
}

S32 Editor_ToolIs(EditorTool tool) {
    return EditorVisible AND EditorActiveTool == tool;
}

S32 Editor_HasOverlay(S32 overlay) {
    return (EditorOverlayFlags & overlay) ? TRUE : FALSE;
}

static const char *Editor_ToolName(EditorTool tool) {
    switch (tool) {
    case EDITOR_TOOL_SELECT_MOVE:
        return "Select / Move";
    case EDITOR_TOOL_DECOR_PLACE:
        return "Place Decor";
    case EDITOR_TOOL_TERRAIN_EDIT:
        return "Edit Terrain";
    case EDITOR_TOOL_TEXTURE_SELECT:
        return "Texture Terrain";
    }
    return "Unknown";
}

static void Editor_SetDefaultOverlaysForTool(void) {
    if (!EditorVisible) {
        EditorOverlayFlags = 0;
        return;
    }

    if (EditorActiveTool == EDITOR_TOOL_SELECT_MOVE)
        EditorOverlayFlags = EDITOR_OVERLAY_DECOR | EDITOR_OVERLAY_ZONES;
    else if (EditorActiveTool == EDITOR_TOOL_DECOR_PLACE)
        EditorOverlayFlags = EDITOR_OVERLAY_DECOR;
    else
        EditorOverlayFlags = 0;
}

static void Editor_ApplyToolState(void) {
    S32 exterior = CubeMode == CUBE_EXTERIEUR;
    S32 decor_select = FALSE;
    S32 terrain_select = FALSE;
    S32 texture_select = FALSE;

    if (EditorVisible AND exterior) {
        if (EditorActiveTool == EDITOR_TOOL_SELECT_MOVE)
            decor_select = TRUE;
        else if (EditorActiveTool == EDITOR_TOOL_DECOR_PLACE)
            decor_select = TRUE;
        else if (EditorActiveTool == EDITOR_TOOL_TERRAIN_EDIT)
            terrain_select = TRUE;
        else if (EditorActiveTool == EDITOR_TOOL_TEXTURE_SELECT)
            texture_select = TRUE;
    }

    ExtDecorEditorSetSelect(decor_select);
    TerrainEditorSetSelect(terrain_select);
    TerrainTextureEditorSetSelect(texture_select);

    if (EditorVisible AND exterior) {
        S32 decor = EditorOverlayFlags & EDITOR_OVERLAY_DECOR;
        S32 zones = EditorOverlayFlags & EDITOR_OVERLAY_ZONES;

        ExtDecorOverlaySet((decor AND EditorOverlayDetailIds), decor);
        ZoneOverlaySet((zones AND EditorOverlayDetailIds), zones);
        ZoneOverlaySetType(-1);
    } else {
        ExtDecorOverlaySet(FALSE, FALSE);
        ZoneOverlaySet(FALSE, FALSE);
    }

    if (EditorVisible) {
        ShowMouse(true);
        EditorMouseShown = TRUE;
    } else {
        ShowMouse(false);
        EditorMouseShown = FALSE;
    }
}

static void Editor_SetOff(void);

static void Editor_SetTool(EditorTool tool) {
    if (tool < EDITOR_TOOL_SELECT_MOVE OR tool > EDITOR_TOOL_TEXTURE_SELECT)
        tool = EDITOR_TOOL_SELECT_MOVE;

    EditorVisible = TRUE;
    EditorActiveTool = tool;
    Editor_SetDefaultOverlaysForTool();
    if (EditorActiveTool == EDITOR_TOOL_DECOR_PLACE)
        ExtDecorEditorClearSelection();
    Editor_ApplyToolState();
    Editor_SetStatus(Editor_ToolName(EditorActiveTool));
    FirstTime = AFF_ALL_FLIP;
}

static void Editor_CycleTool(S32 dir) {
    S32 index;

    index = EditorVisible ? ((S32)EditorActiveTool + 1) : 0;
    index += dir;
    if (index < 0)
        index = (S32)EDITOR_TOOL_TEXTURE_SELECT + 1;
    if (index > (S32)EDITOR_TOOL_TEXTURE_SELECT + 1)
        index = 0;

    if (index == 0) {
        Editor_SetOff();
    } else {
        Editor_SetTool((EditorTool)(index - 1));
    }
}

static void Editor_SetOff(void) {
    EditorVisible = FALSE;
    Editor_SetDefaultOverlaysForTool();
    Editor_ApplyToolState();
    Editor_SetStatus("Editor Off");
    FirstTime = AFF_ALL_FLIP;
}

static void Editor_ToggleOverlay(S32 overlay) {
    EditorOverlayFlags ^= overlay;
    Editor_ApplyToolState();
    if (overlay == EDITOR_OVERLAY_DECOR)
        Editor_SetStatus((EditorOverlayFlags & EDITOR_OVERLAY_DECOR) ? "Decor overlay enabled" : "Decor overlay disabled");
    else if (overlay == EDITOR_OVERLAY_ZONES)
        Editor_SetStatus((EditorOverlayFlags & EDITOR_OVERLAY_ZONES) ? "Zone overlay enabled" : "Zone overlay disabled");
    FirstTime = AFF_ALL_FLIP;
}

static void Editor_ToggleOverlayDetail(void) {
    EditorOverlayDetailIds = !EditorOverlayDetailIds;
    Editor_ApplyToolState();
    Editor_SetStatus(EditorOverlayDetailIds ? "Overlay detail: boxes + ids" : "Overlay detail: boxes only");
    FirstTime = AFF_ALL_FLIP;
}

static void Editor_ClearAllSelection(void) {
    ExtDecorEditorClearSelection();
    TerrainEditorClearSelection();
    TerrainTextureEditorClearSelection();
    ZoneEditorClearAllSelected();
    FirstTime = AFF_ALL_FLIP;
}

static void Editor_ConsumeCurrentFrameInput(void) {
    Key = 0;
    MyKey = 0;
    Input = 0;
}

S32 Editor_IsActive(void) {
    return EditorVisible
           OR ExtDecorEditorGetSelect()
           OR TerrainEditorGetSelect()
           OR TerrainTextureEditorGetSelect();
}

void Editor_UpdateInput(void) {
    S32 save;
    S32 next_tool;
    S32 prev_tool;
    S32 decor_overlay;
    S32 zone_overlay;
    S32 overlay_detail;
    S32 escape;
    S32 enter;
    S32 ctrl;

    next_tool = CheckKey(K_PERIOD) ? TRUE : FALSE;
    prev_tool = CheckKey(K_COMMA) ? TRUE : FALSE;
    if (next_tool AND !EditorLastNextTool)
        Editor_CycleTool(1);
    if (prev_tool AND !EditorLastPrevTool)
        Editor_CycleTool(-1);
    EditorLastNextTool = next_tool;
    EditorLastPrevTool = prev_tool;

    ctrl = (CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT)) ? TRUE : FALSE;
    decor_overlay = CheckKey(K_D) ? TRUE : FALSE;
    zone_overlay = CheckKey(K_Z) ? TRUE : FALSE;
    overlay_detail = CheckKey(K_I) ? TRUE : FALSE;

    if (EditorVisible AND !ctrl AND decor_overlay AND !EditorLastDecorOverlay)
        Editor_ToggleOverlay(EDITOR_OVERLAY_DECOR);
    if (EditorVisible AND zone_overlay AND !EditorLastZoneOverlay)
        Editor_ToggleOverlay(EDITOR_OVERLAY_ZONES);
    if (EditorVisible AND overlay_detail AND !EditorLastOverlayDetail)
        Editor_ToggleOverlayDetail();
    EditorLastDecorOverlay = decor_overlay;
    EditorLastZoneOverlay = zone_overlay;
    EditorLastOverlayDetail = overlay_detail;

    escape = CheckKey(K_ESC) ? TRUE : FALSE;
    if (Editor_IsActive() AND escape AND !EditorLastEscape) {
        Editor_SetOff();
        EditorSuppressEscapeUntilRelease = TRUE;
    }
    if (escape AND EditorSuppressEscapeUntilRelease)
        Editor_ConsumeCurrentFrameInput();
    if (!escape)
        EditorSuppressEscapeUntilRelease = FALSE;
    EditorLastEscape = escape;

    enter = (CheckKey(K_ENTER) OR CheckKey(K_NUMPAD_ENTER)) ? TRUE : FALSE;
    if (Editor_IsActive() AND enter AND !EditorLastEnter) {
        Editor_ClearAllSelection();
        EditorSuppressEnterUntilRelease = TRUE;
    }
    if (enter AND EditorSuppressEnterUntilRelease)
        Editor_ConsumeCurrentFrameInput();
    if (!enter)
        EditorSuppressEnterUntilRelease = FALSE;
    EditorLastEnter = enter;

    save = (CheckKey(K_S) AND ctrl) ? TRUE : FALSE;
    if (save AND !EditorLastSave AND Editor_IsActive())
        Editor_Save();
    EditorLastSave = save;

    if (Editor_IsActive()) {
        ShowMouse(true);
        EditorMouseShown = TRUE;
    } else if (!Editor_IsActive() AND EditorMouseShown) {
        ShowMouse(false);
        EditorMouseShown = FALSE;
    }
}

static void EditorFillRect(S32 x0, S32 y0, S32 x1, S32 y1, S32 color) {
    S32 y;

    if (x0 < 0)
        x0 = 0;
    if (y0 < 0)
        y0 = 0;
    if (x1 >= ModeDesiredX)
        x1 = ModeDesiredX - 1;
    if (y1 >= ModeDesiredY)
        y1 = ModeDesiredY - 1;
    for (y = y0; y <= y1; y++)
        Line(x0, y, x1, y, color);
}

void Editor_DrawOverlay(void) {
    S32 status_y;

    UnsetClip();

    if (Editor_ToolIs(EDITOR_TOOL_DECOR_PLACE)) {
        EditorFillRect(8, 8, 80, 24, 0);
        Rect(8, 8, 80, 24, 15);
        CoulText(15, -1);
        GraphPrintf(FALSE, 14, 13, "Body %d", (int)ExtDecorEditorGetPaletteBody());
    }

    if (EditorStatus[0] AND TimerRefHR <= EditorStatusTimerEnd) {
        status_y = ModeDesiredY - 18;
        if (status_y < 0)
            status_y = 0;
        CoulText(15, 0);
        GraphPrintf(FALSE, 8, status_y, "%s", EditorStatus);
    }

    if (EditorVisible OR(EditorStatus[0] AND TimerRefHR <= EditorStatusTimerEnd))
        BoxStaticAdd(0, 0, ModeDesiredX - 1, ModeDesiredY - 1);
    UnsetClip();
}

#endif
