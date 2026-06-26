#include "EDITOR.H"

#ifdef LBA2CC_EDITOR

#include "ACTOR_EDITOR.H"
#include "EDITOR_JSON.H"
#include "WAYPOINT_EDITOR.H"
#include "C_EXTERN.H"
#include "DIRECTORIES.H"
#include "DISKFUNC.H"
#include "FREECAM.H"
#include "HQR_NAMES.H"
#include "MESSAGE.H"
#include "OBJECT.H"
#include "3DEXT/DECORS.H"
#include "3DEXT/EDITOR_TERRAIN_PATTERNS.H"
#include "3DEXT/LOADISLE.H"
#include "3DEXT/TERRAIN.H"

#include <3D/LIGHT.H>
#include <3D/ROT2D.H>
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
#include <stdlib.h>
#include <string.h>

extern S32 CubeMode;
extern S32 Island;
extern S32 CurrentCubeX;
extern S32 CurrentCubeY;
extern S32 *ListCubeInfos;
extern U8 *PtrSce;
extern void ZoneEditorClearAllSelected(void);

static S32 EditorVisible = FALSE;
static S32 EditorLastSave = FALSE;
static S32 EditorLastNew = FALSE;
static S32 EditorLastDeleteActor = FALSE;
static S32 EditorLastNextTool = FALSE;
static S32 EditorLastPrevTool = FALSE;
static S32 EditorLastDecorOverlay = FALSE;
static S32 EditorLastZoneOverlay = FALSE;
static S32 EditorLastActorOverlay = FALSE;
static S32 EditorLastWaypointOverlay = FALSE;
static S32 EditorLastOverlayDetail = FALSE;
static S32 EditorLastCubeBoundaryOverlay = FALSE;
static S32 EditorLastEscape = FALSE;
static S32 EditorLastEnter = FALSE;
static S32 EditorSuppressEscapeUntilRelease = FALSE;
static S32 EditorSuppressEnterUntilRelease = FALSE;
static S32 EditorMouseShown = FALSE;
static S32 EditorCameraPanActive = FALSE;
static S32 EditorInspectorVisible = FALSE;
static S32 EditorInspectorPreferredVisible = FALSE;
static S32 EditorInspectorFocused = FALSE;
static S32 EditorInspectorAscii = FALSE;
static S32 EditorInspectorAsciiMemo = FALSE;
static S32 EditorInspectorPlasmaActive = FALSE;
static S32 EditorInspectorRow = 0;
static S32 EditorInspectorLastKind = 0;
static S32 EditorInspectorLastToggle = FALSE;
static S32 EditorInspectorLastTab = FALSE;
static S32 EditorInspectorLastUp = FALSE;
static S32 EditorInspectorLastDown = FALSE;
static S32 EditorInspectorLastLeft = FALSE;
static S32 EditorInspectorLastRight = FALSE;
static S32 EditorInspectorLastEnter = FALSE;
static S32 EditorInspectorLastBackspace = FALSE;
static S32 EditorInspectorEscapeConsumedUntilRelease = FALSE;
static char EditorInspectorAsciiText[16] = "";
static EditorTool EditorActiveTool = EDITOR_TOOL_SELECT_MOVE;
static S32 EditorOverlayFlags = EDITOR_OVERLAY_DECOR | EDITOR_OVERLAY_ZONES | EDITOR_OVERLAY_ACTORS | EDITOR_OVERLAY_WAYPOINTS;
static S32 EditorOverlayDetailIds = FALSE;
static S32 EditorCubeBoundaryOverlay = FALSE;
static S32 EditorNewMenuActive = FALSE;
static S32 EditorNewMenuRow = 0;
static S32 EditorNewMenuLastUp = FALSE;
static S32 EditorNewMenuLastDown = FALSE;
static S32 EditorNewMenuLastEnter = FALSE;
static S32 EditorNewMenuLastEscape = FALSE;
static S32 EditorNewMenuLastActor = FALSE;
static S32 EditorNewMenuLastWaypoint = FALSE;
static S32 EditorNewMenuLastZone = FALSE;
static S32 EditorNewPlacement = 0;
static S32 ActorEditorLastUp = FALSE;
static S32 ActorEditorLastDown = FALSE;
static S32 ActorEditorLastLeft = FALSE;
static S32 ActorEditorLastRight = FALSE;
static S32 ActorEditorLastPageUp = FALSE;
static S32 ActorEditorLastPageDown = FALSE;
static S32 WaypointEditorLastUp = FALSE;
static S32 WaypointEditorLastDown = FALSE;
static S32 WaypointEditorLastLeft = FALSE;
static S32 WaypointEditorLastRight = FALSE;
static S32 WaypointEditorLastPageUp = FALSE;
static S32 WaypointEditorLastPageDown = FALSE;
static U32 EditorStatusTimerEnd = 0;
static char EditorStatus[220] = "";

typedef struct {
    S32 VueOffsetX;
    S32 VueOffsetY;
    S32 VueOffsetZ;
    S32 AlphaCam;
    S32 BetaCam;
    S32 GammaCam;
    S32 VueDistance;
    float OrthoScale;
    S32 Ortho;
} T_EDITOR_CAMERA_STATE;

static T_EDITOR_CAMERA_STATE EditorGameCamera;
static T_EDITOR_CAMERA_STATE EditorCamera;
static S32 EditorGameCubeX = 0;
static S32 EditorGameCubeY = 0;
static S32 EditorCameraOrtho = FALSE;
static S32 EditorRememberedOrtho = FALSE;
static S32 EditorHasGameCamera = FALSE;
static S32 EditorHasCamera = FALSE;
static S32 EditorLastViewFront = FALSE;
static S32 EditorLastViewRight = FALSE;
static S32 EditorLastViewTop = FALSE;
static S32 EditorLastViewOrtho = FALSE;
static S32 EditorLastViewReset = FALSE;
static S32 EditorLastZoomIn = FALSE;
static S32 EditorLastZoomOut = FALSE;

static const char *EditorInspectorTextureFieldNames[TERRAIN_TEXTURE_FIELD_COUNT] = {
    "Color bank",
    "Texture flag",
    "Polygon flag",
    "Sample step",
    "Game code",
    "Diagonal direction",
    "Collision",
    "Dummy",
    "Texture index"
};

static const S32 EditorInspectorTextureFieldMax[TERRAIN_TEXTURE_FIELD_COUNT] = {
    15, 3, 3, 15, 15, 1, 1, 1, 8191
};

static const char *EditorInspectorTerrainFieldNames[TERRAIN_EDITOR_FIELD_COUNT] = {
    "Cube X", "Cube Y", "X", "Z", "Height", "Intensity"
};

static const char *EditorInspectorSceneFieldNames[2] = {
    "AlphaLight", "BetaLight"
};

static const char *EditorInspectorTexFlagNames[4] = {
    "None", "Sad", "Flat", "Gouraud"
};

static const char *EditorInspectorGameCodeNames[16] = {
    "None",
    "Water",
    "Electric",
    "Conveyer West",
    "Conveyer East",
    "Conveyer North",
    "Conveyer South",
    "Labyrinth",
    "Invalid Position",
    "Lava",
    "Unused",
    "Gas",
    "Shallow Water",
    "Animated Lava",
    "Animated Gas",
    "Animated Water"
};

static const char *EditorInspectorDecorFieldNames[EXT_DECOR_FIELD_COUNT] = {
    "Body", "Rotation", "X", "Y", "Z"
};

static const char *EditorInspectorActorFieldNames[] = {
    "Index", "Model", "Body", "Animation", "X", "Y", "Z", "Beta",
    "Life", "Armour", "Move", "Raw flags",
    "Object collision", "Brick collision", "Zone checks", "Sprite clipping",
    "Pushable", "Low collision", "Game-code checks", "Floor-only collision",
    "Unused flag 8", "Invisible", "3D sprite", "Fallable", "No shadow",
    "Background object", "Carrier", "Minimum ZV", "Invalid position",
    "No hit reaction", "3DS animation", "No pre-clip",
    "Exterior Z-buffer", "In-water Z-buffer"
};

static const char *EditorInspectorWaypointFieldNames[] = {
    "Index", "X", "Y", "Z"
};

static const char *EditorInspectorZoneFieldNames[8] = {
    "Type", "Num", "X0", "Y0", "Z0", "X1", "Y1", "Z1"
};

static const char *EditorInspectorMessageZoneFieldNames[ZONE_EDITOR_FIELD_COUNT + 1] = {
    "Type", "Message ID", "Text", "Text Color", "Camera Zone", "Direction",
    "X0", "Y0", "Z0", "X1", "Y1", "Z1"
};

static const char *EditorInspectorZoneTypeNames[10] = {
    "Teleport", "Camera", "Sceneric", "Fragment", "Bonus",
    "Text", "Ladder", "Conveyor", "Spike", "Rail"
};

static const char *EditorInspectorMessageDirectionNames[4] = {
    "North", "South", "East", "West"
};

static const char *EditorInspectorRotationNames[4] = {
    "0", "90", "180", "270"
};

static const char *EditorInspectorTerrainRegionFieldNames[TERRAIN_REGION_FIELD_COUNT] = {
    "Bake", "Type", "Seed", "Patchiness", "Randomize Seed", "X0", "Z0", "X1", "Z1"
};

static const char *EditorInspectorTerrainRegionTypeNames[TERRAIN_REGION_TYPE_COUNT] = {
    "Grass Patch", "Pavement", "Grass Inset", "Asphalt", "Grass Pavement", "Riprap", "Pavement (Alt)", "Rocks", "Cliffside", "Dirt"
};

typedef struct {
    S32 Used;
    S32 CubeX;
    S32 CubeY;
    S32 AlphaLight;
    S32 BetaLight;
} T_EDITOR_SCENE_EDIT;

#define EDITOR_SCENE_MAX_EDITS 256
#define EDITOR_INSPECTOR_NONE 0
#define EDITOR_INSPECTOR_TEXTURE 1
#define EDITOR_INSPECTOR_DECOR 2
#define EDITOR_INSPECTOR_ZONE 3
#define EDITOR_INSPECTOR_TERRAIN_REGION 4
#define EDITOR_INSPECTOR_TERRAIN 5
#define EDITOR_INSPECTOR_SCENE 6
#define EDITOR_INSPECTOR_ACTOR 7
#define EDITOR_INSPECTOR_WAYPOINT 8
#define EDITOR_WAYPOINT_FIELD_INDEX 0
#define EDITOR_WAYPOINT_FIELD_X 1
#define EDITOR_WAYPOINT_FIELD_Y 2
#define EDITOR_WAYPOINT_FIELD_Z 3
#define EDITOR_WAYPOINT_FIELD_COUNT 4
#define EDITOR_INSPECTOR_ZONE_TEXT_FIELD 2
#define EDITOR_INSPECTOR_SCENE_ALPHA_LIGHT 0
#define EDITOR_INSPECTOR_SCENE_BETA_LIGHT 1
#define EDITOR_FRONT_ALPHA 0
#define EDITOR_RIGHT_ALPHA 0
#define EDITOR_TOP_ALPHA 1024
#define EDITOR_VIEW_BETA_FRONT 0
#define EDITOR_VIEW_BETA_RIGHT 1024
#define EDITOR_PERSPECTIVE_ZOOM_STEP 1000
#define EDITOR_PERSPECTIVE_ZOOM_MIN 1000
#define EDITOR_PERSPECTIVE_ZOOM_MAX 50000
#define EDITOR_ORTHO_SCALE_STEP 1.5f
#define EDITOR_ORTHO_SCALE_MIN (1.0f / 512.0f)
#define EDITOR_ORTHO_SCALE_MAX (1.0f / 4.0f)
#define EDITOR_CUBE_BOUNDARY_COLOR 7
#define EDITOR_ACTOR_BOX_COLOR 10
#define EDITOR_ACTOR_BOX_AXIS_COLOR 14
#define EDITOR_ACTOR_TEXT_COLOR 10
#define EDITOR_ACTOR_TEXT_BACK -1
#define EDITOR_ACTOR_FALLBACK_HALF_WIDTH 128
#define EDITOR_ACTOR_FALLBACK_HEIGHT 512
#define EDITOR_ACTOR_PICK_RADIUS 12
#define EDITOR_ACTOR_NUDGE 64
#define EDITOR_ACTOR_NUDGE_SHIFT 256
#define EDITOR_ACTOR_NUDGE_ALT 1
#define EDITOR_WAYPOINT_COLOR 11
#define EDITOR_WAYPOINT_AXIS_COLOR 13
#define EDITOR_WAYPOINT_HALF_SIZE 96
#define EDITOR_ISLAND_MAP_SIZE 16
#define EDITOR_RESTORE_SAVED_CAMERA FALSE
#define EDITOR_NEW_NONE 0
#define EDITOR_NEW_ACTOR 1
#define EDITOR_NEW_WAYPOINT 2
#define EDITOR_NEW_ZONE 3

static T_EDITOR_SCENE_EDIT EditorSceneEdits[EDITOR_SCENE_MAX_EDITS];

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

static void EditorCaptureCamera(T_EDITOR_CAMERA_STATE *camera) {
    camera->VueOffsetX = VueOffsetX;
    camera->VueOffsetY = VueOffsetY;
    camera->VueOffsetZ = VueOffsetZ;
    camera->AlphaCam = AlphaCam;
    camera->BetaCam = BetaCam;
    camera->GammaCam = GammaCam;
    camera->VueDistance = VueDistance;
    camera->OrthoScale = OrthoScale;
    camera->Ortho = EditorCameraOrtho;
}

static void EditorApplyCamera(const T_EDITOR_CAMERA_STATE *camera) {
    VueOffsetX = camera->VueOffsetX;
    VueOffsetY = camera->VueOffsetY;
    VueOffsetZ = camera->VueOffsetZ;
    AlphaCam = camera->AlphaCam;
    BetaCam = camera->BetaCam;
    GammaCam = camera->GammaCam;
    VueDistance = camera->VueDistance;
    OrthoScale = camera->OrthoScale;
    EditorCameraOrtho = camera->Ortho;
}

static void EditorApplyGameCamera(const T_EDITOR_CAMERA_STATE *camera) {
    VueOffsetX = camera->VueOffsetX;
    VueOffsetY = camera->VueOffsetY;
    VueOffsetZ = camera->VueOffsetZ;
    AlphaCam = camera->AlphaCam;
    BetaCam = camera->BetaCam;
    GammaCam = camera->GammaCam;
    VueDistance = camera->VueDistance;
    OrthoScale = camera->OrthoScale;
}

static void EditorSaveGameCamera(void) {
    EditorCaptureCamera(&EditorGameCamera);
    EditorGameCubeX = CurrentCubeX;
    EditorGameCubeY = CurrentCubeY;
    EditorHasGameCamera = TRUE;
}

static void EditorSaveEditorCamera(void) {
    EditorCaptureCamera(&EditorCamera);
    EditorHasCamera = TRUE;
}

static void EditorApplyCurrentCamera(void) {
    SetFollowCamera(VueOffsetX, VueOffsetY, VueOffsetZ, AlphaCam, BetaCam, GammaCam, VueDistance);
    EditorSaveEditorCamera();
}

static void EditorSyncLoadedCubeToCamera(void) {
    return; // FIXME: cette fonction fait planter le jeu !!!!!

    S32 cube_x;
    S32 cube_y;

    if (!EditorVisible OR CubeMode != CUBE_EXTERIEUR)
        return;

    cube_x = VueOffsetX / SIZE_BRICK_XZ;
    cube_y = VueOffsetZ / SIZE_BRICK_XZ;
    if (cube_x == CurrentCubeX AND cube_y == CurrentCubeY)
        return;

    CurrentCubeX = cube_x;
    CurrentCubeY = cube_y;
    LoadCube(CurrentCubeX, CurrentCubeY);
}

static void EditorRestoreGameCamera(void) {
    if (EditorHasGameCamera) {
        SetProjection(XCentre, YCentre, NearClip, LFactorX, LFactorY);
        EditorApplyGameCamera(&EditorGameCamera);
        SetFollowCamera(VueOffsetX, VueOffsetY, VueOffsetZ, AlphaCam, BetaCam, GammaCam, VueDistance);
    }
}

static void EditorRestoreEditorCamera(void) {
    if (EditorHasCamera) {
        EditorApplyCamera(&EditorCamera);
        if (EditorCameraOrtho)
            SetOrthoProjection(XCentre, YCentre, NearClip);
        else
            SetProjection(XCentre, YCentre, NearClip, LFactorX, LFactorY);
        SetFollowCamera(VueOffsetX, VueOffsetY, VueOffsetZ, AlphaCam, BetaCam, GammaCam, VueDistance);
    }
}

static void EditorSetCameraPerspective(void) {
    EditorCameraOrtho = FALSE;
    EditorRememberedOrtho = FALSE;
    SetProjection(XCentre, YCentre, NearClip, LFactorX, LFactorY);
    EditorApplyCurrentCamera();
}

static void EditorSetCameraOrtho(void) {
    EditorCameraOrtho = TRUE;
    EditorRememberedOrtho = TRUE;
    SetOrthoProjection(XCentre, YCentre, NearClip);
    EditorApplyCurrentCamera();
}

static void EditorSetCameraPreset(S32 alpha, S32 beta) {
    EditorCameraOrtho = EditorRememberedOrtho;
    if (EditorCameraOrtho)
        SetOrthoProjection(XCentre, YCentre, NearClip);
    else
        SetProjection(XCentre, YCentre, NearClip, LFactorX, LFactorY);
    AlphaCam = alpha;
    BetaCam = beta;
    GammaCam = 0;
    EditorApplyCurrentCamera();
}

static void EditorZoomCamera(S32 zoom_in) {
    if (EditorCameraOrtho) {
        if (zoom_in)
            OrthoScale *= EDITOR_ORTHO_SCALE_STEP;
        else
            OrthoScale /= EDITOR_ORTHO_SCALE_STEP;
        if (OrthoScale < EDITOR_ORTHO_SCALE_MIN)
            OrthoScale = EDITOR_ORTHO_SCALE_MIN;
        if (OrthoScale > EDITOR_ORTHO_SCALE_MAX)
            OrthoScale = EDITOR_ORTHO_SCALE_MAX;
    } else {
        if (zoom_in) {
            if (VueDistance > EDITOR_PERSPECTIVE_ZOOM_MIN)
                VueDistance -= EDITOR_PERSPECTIVE_ZOOM_STEP;
        } else {
            if (VueDistance < EDITOR_PERSPECTIVE_ZOOM_MAX)
                VueDistance += EDITOR_PERSPECTIVE_ZOOM_STEP;
        }
    }
    EditorApplyCurrentCamera();
}

static void EditorResetCameraToPlayer(void) {
    if (!EditorHasGameCamera)
        return;

    EditorCameraOrtho = FALSE;
    EditorRememberedOrtho = FALSE;
    CurrentCubeX = EditorGameCubeX;
    CurrentCubeY = EditorGameCubeY;
    SetProjection(XCentre, YCentre, NearClip, LFactorX, LFactorY);
    EditorApplyGameCamera(&EditorGameCamera);
    SetFollowCamera(VueOffsetX, VueOffsetY, VueOffsetZ, AlphaCam, BetaCam, GammaCam, VueDistance);
    LoadCube(CurrentCubeX, CurrentCubeY);
    EditorSaveEditorCamera();
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

S32 Editor_FindExteriorScene(S32 cube_x, S32 cube_y) {
    char path[ADELINE_MAX_PATH];
    U8 *saved_ptr_sce;
    U32 header_size;
    U32 offset;
    S32 scene_count;
    S32 scene;
    FILE *f;

    GetResPath(path, sizeof(path), SCENE_HQR_NAME);
    f = fopen(path, "rb");
    if (!f)
        return 0;
    if (fread(&header_size, sizeof(header_size), 1, f) != 1
            OR header_size < sizeof(U32) * 2
            OR(header_size % sizeof(U32)) != 0) {
        fclose(f);
        return 0;
    }

    saved_ptr_sce = PtrSce;
    scene_count = (S32)(header_size / sizeof(U32)) - 1;
    for (scene = 1; scene < scene_count; scene++) {
        S32 x;
        S32 y;

        if (fseek(f, (scene + 1) * sizeof(U32), SEEK_SET) != 0
                OR fread(&offset, sizeof(offset), 1, f) != 1
                OR !offset)
            continue;
        if (LoadSceneCubeXY(scene, &x, &y) AND x == cube_x AND y == cube_y) {
            PtrSce = saved_ptr_sce;
            fclose(f);
            return scene;
        }
    }
    PtrSce = saved_ptr_sce;
    fclose(f);
    return 0;
}

void EditorSceneClearRuntimeEdits(void) {
    memset(EditorSceneEdits, 0, sizeof(EditorSceneEdits));
}

static T_EDITOR_SCENE_EDIT *EditorSceneFindEdit(S32 cube_x, S32 cube_y) {
    S32 i;

    for (i = 0; i < EDITOR_SCENE_MAX_EDITS; i++) {
        T_EDITOR_SCENE_EDIT *edit = &EditorSceneEdits[i];
        if (edit->Used AND edit->CubeX == cube_x AND edit->CubeY == cube_y)
            return edit;
    }
    return NULL;
}

static T_EDITOR_SCENE_EDIT *EditorSceneGetOrCreateEdit(S32 cube_x, S32 cube_y) {
    S32 i;
    T_EDITOR_SCENE_EDIT *edit;

    edit = EditorSceneFindEdit(cube_x, cube_y);
    if (edit)
        return edit;

    for (i = 0; i < EDITOR_SCENE_MAX_EDITS; i++) {
        edit = &EditorSceneEdits[i];
        if (!edit->Used) {
            edit->Used = TRUE;
            edit->CubeX = cube_x;
            edit->CubeY = cube_y;
            edit->AlphaLight = AlphaLight & 4095;
            edit->BetaLight = BetaLight & 4095;
            return edit;
        }
    }

    Log_Warn("[editor] scene edit table is full; cannot track c%d,%d",
             (int)cube_x,
             (int)cube_y);
    return NULL;
}

void EditorSceneApplyRuntimeEditsForLoadedCube(S32 cube_x, S32 cube_y) {
    T_EDITOR_SCENE_EDIT *edit;

    edit = EditorSceneFindEdit(cube_x, cube_y);
    if (!edit)
        return;

    AlphaLight = edit->AlphaLight & 4095;
    BetaLight = edit->BetaLight & 4095;
    if (ListCubeInfos) {
        ListCubeInfos[0] = (ListCubeInfos[0] & 0xFFFF0000) | (AlphaLight & 0xFFFF);
        ListCubeInfos[1] = BetaLight;
    }
    SetLightVector(AlphaLight, BetaLight, 0);
}

void EditorSceneLoadAuthoringEditsForCurrentIsland(void) {
    char dir[ADELINE_MAX_PATH];
    char path[ADELINE_MAX_PATH];
    char *json;
    jsmntok_t *tokens;
    S32 count;
    S32 island_value;
    S32 cubes;
    S32 cube_index;
    S32 loaded = 0;
    S32 skipped = 0;

    if (!Editor_ResolveOutputDir(dir, sizeof(dir)))
        return;
    snprintf(path, sizeof(path), "%s%sscene_edits.json", dir, ADELINE_PATH_SEP);
    path[sizeof(path) - 1] = '\0';

    if (!EditorJsonParseFile(path, &json, &tokens, &count)) {
        if (EditorJsonFileExists(path))
            Log_Warn("[editor] cannot load scene edits: invalid JSON: %s", path);
        return;
    }

    if (EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, 0, "island"), &island_value)
            AND island_value != Island) {
        Log_Warn("[editor] scene edits ignored: %s is for island %d, current island is %d",
                 path,
                 (int)island_value,
                 (int)Island);
        free(tokens);
        free(json);
        return;
    }

    cubes = EditorJsonFindValue(json, tokens, count, 0, "cubes");
    if (cubes >= 0 AND tokens[cubes].type == JSMN_ARRAY) {
        S32 cubes_end = EditorJsonTokenEnd(tokens, count, cubes);
        for (cube_index = cubes + 1; cube_index < cubes_end; cube_index = EditorJsonTokenEnd(tokens, count, cube_index)) {
            S32 cube_xy[2];
            S32 light;
            S32 alpha;
            S32 beta;
            T_EDITOR_SCENE_EDIT *edit;

            if (tokens[cube_index].parent != cubes)
                continue;
            if (tokens[cube_index].type != JSMN_OBJECT) {
                skipped++;
                continue;
            }
            if (!EditorJsonParseIntArray(json, tokens, count, EditorJsonFindValue(json, tokens, count, cube_index, "cube"), cube_xy, 2)) {
                skipped++;
                continue;
            }
            light = EditorJsonFindValue(json, tokens, count, cube_index, "light");
            if (light < 0 OR tokens[light].type != JSMN_OBJECT) {
                skipped++;
                continue;
            }
            if (!EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, light, "alpha_light"), &alpha)
                    OR !EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, light, "beta_light"), &beta)) {
                skipped++;
                continue;
            }
            edit = EditorSceneGetOrCreateEdit(cube_xy[0], cube_xy[1]);
            if (!edit) {
                skipped++;
                continue;
            }
            edit->AlphaLight = alpha & 4095;
            edit->BetaLight = beta & 4095;
            loaded++;
        }
    }

    Log_Info("[editor] loaded scene edits: %s (loaded=%d skipped=%d)", path, (int)loaded, (int)skipped);
    free(tokens);
    free(json);
}

static S32 EditorSceneSaveRuntimeEdits(const char *island_dir, S32 island) {
    char path[ADELINE_MAX_PATH];
    FILE *f;
    S32 i;
    S32 first = TRUE;
    S32 count = 0;

    if (!island_dir OR !island_dir[0])
        return FALSE;

    snprintf(path, sizeof(path), "%s%sscene_edits.json", island_dir, ADELINE_PATH_SEP);
    path[sizeof(path) - 1] = '\0';

    f = fopen(path, "wb");
    if (!f) {
        Editor_SetStatus("Scene save failed: cannot open scene_edits.json");
        Log_Warn("[editor] cannot write scene edits: %s", path);
        return FALSE;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"island\": %d,\n", (int)island);
    fprintf(f, "  \"description\": \"Authoring edits exported by the scene inspector.\",\n");
    fprintf(f, "  \"cubes\": [\n");

    for (i = 0; i < EDITOR_SCENE_MAX_EDITS; i++) {
        T_EDITOR_SCENE_EDIT *edit = &EditorSceneEdits[i];

        if (!edit->Used)
            continue;

        if (!first)
            fprintf(f, ",\n");
        first = FALSE;
        count++;
        fprintf(f,
                "    {\"cube\": [%d, %d], \"light\": {\"alpha_light\": %d, \"beta_light\": %d}}",
                (int)edit->CubeX,
                (int)edit->CubeY,
                (int)(edit->AlphaLight & 4095),
                (int)(edit->BetaLight & 4095));
    }

    fprintf(f, "\n");
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);

    Log_Info("[editor] saved %d scene edit(s) to %s", (int)count, path);
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
    if (!ZoneEditorSaveRuntimeEdits(outPath, Island))
        return;
    if (!EditorSceneSaveRuntimeEdits(outPath, Island))
        return;
    if (!ActorEditorSaveRuntimeEdits(outPath, Island))
        return;
    if (!WaypointEditorSaveRuntimeEdits(outPath, Island))
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

S32 Editor_IsCameraPanActive(void) {
    return EditorCameraPanActive;
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
    case EDITOR_TOOL_TERRAIN_REGION:
        return "Auto Terrain";
    }
    return "Unknown";
}

static void Editor_SetDefaultOverlaysForTool(void) {
    if (!EditorVisible) {
        EditorOverlayFlags = 0;
        return;
    }

    if (EditorActiveTool == EDITOR_TOOL_SELECT_MOVE)
        EditorOverlayFlags = EDITOR_OVERLAY_DECOR | EDITOR_OVERLAY_ZONES | EDITOR_OVERLAY_ACTORS | EDITOR_OVERLAY_WAYPOINTS;
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
    S32 terrain_region_select = FALSE;

    if (EditorVisible AND exterior) {
        if (EditorActiveTool == EDITOR_TOOL_SELECT_MOVE)
            decor_select = TRUE;
        else if (EditorActiveTool == EDITOR_TOOL_DECOR_PLACE)
            decor_select = TRUE;
        else if (EditorActiveTool == EDITOR_TOOL_TERRAIN_EDIT)
            terrain_select = TRUE;
        else if (EditorActiveTool == EDITOR_TOOL_TEXTURE_SELECT)
            texture_select = TRUE;
        else if (EditorActiveTool == EDITOR_TOOL_TERRAIN_REGION)
            terrain_region_select = TRUE;
    }

    ExtDecorEditorSetSelect(decor_select);
    TerrainEditorSetSelect(terrain_select);
    TerrainTextureEditorSetSelect(texture_select);
    TerrainRegionEditorSetSelect(terrain_region_select);

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
    S32 was_visible = EditorVisible;

    if (tool < EDITOR_TOOL_SELECT_MOVE OR tool > EDITOR_TOOL_TERRAIN_REGION)
        tool = EDITOR_TOOL_SELECT_MOVE;

    if (!was_visible) {
        EditorSaveGameCamera();
        if (EDITOR_RESTORE_SAVED_CAMERA AND EditorHasCamera)
            EditorRestoreEditorCamera();
        else
            EditorSaveEditorCamera();
        EditorSyncLoadedCubeToCamera();
    }

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
        index = (S32)EDITOR_TOOL_TERRAIN_REGION + 1;
    if (index > (S32)EDITOR_TOOL_TERRAIN_REGION + 1)
        index = 0;

    if (index == 0) {
        Editor_SetOff();
    } else {
        Editor_SetTool((EditorTool)(index - 1));
    }
}

static void Editor_SetOff(void) {
    if (EditorVisible) {
        EditorSaveEditorCamera();
        EditorRestoreGameCamera();
        if (EditorHasGameCamera) {
            CurrentCubeX = EditorGameCubeX;
            CurrentCubeY = EditorGameCubeY;
            LoadCube(CurrentCubeX, CurrentCubeY);
        }
    }
    EditorCameraPanActive = FALSE;
    EditorCameraOrtho = FALSE;
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
    else if (overlay == EDITOR_OVERLAY_ACTORS)
        Editor_SetStatus((EditorOverlayFlags & EDITOR_OVERLAY_ACTORS) ? "Actor overlay enabled" : "Actor overlay disabled");
    else if (overlay == EDITOR_OVERLAY_WAYPOINTS)
        Editor_SetStatus((EditorOverlayFlags & EDITOR_OVERLAY_WAYPOINTS) ? "Waypoint overlay enabled" : "Waypoint overlay disabled");
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
    TerrainRegionEditorClearSelection();
    ZoneEditorClearAllSelected();
    FirstTime = AFF_ALL_FLIP;
}

static void Editor_ConsumeCurrentFrameInput(void);

static void EditorBeginNewPlacement(S32 type) {
    EditorNewMenuActive = FALSE;
    EditorNewPlacement = type;
    ExtDecorEditorClearSelection();
    if (type == EDITOR_NEW_ACTOR) {
        ActorEditorBeginPlace();
        if (!ActorEditorIsPlacing())
            EditorNewPlacement = EDITOR_NEW_NONE;
    } else if (type == EDITOR_NEW_WAYPOINT) {
        Editor_SetStatus("New Waypoint: click a world position");
    } else if (type == EDITOR_NEW_ZONE) {
        Editor_SetStatus("New Zone: click a world position");
    }
    FirstTime = AFF_ALL_FLIP;
}

static S32 EditorUpdateNewMenuInput(void) {
    S32 up = CheckKey(K_UP) ? TRUE : FALSE;
    S32 down = CheckKey(K_DOWN) ? TRUE : FALSE;
    S32 enter = (CheckKey(K_ENTER) OR CheckKey(K_NUMPAD_ENTER)) ? TRUE : FALSE;
    S32 escape = CheckKey(K_ESC) ? TRUE : FALSE;
    S32 actor = CheckKey(K_A) ? TRUE : FALSE;
    S32 waypoint = CheckKey(K_W) ? TRUE : FALSE;
    S32 zone = CheckKey(K_Z) ? TRUE : FALSE;
    S32 choice = EDITOR_NEW_NONE;

    if (up AND !EditorNewMenuLastUp)
        EditorNewMenuRow = (EditorNewMenuRow + 2) % 3;
    if (down AND !EditorNewMenuLastDown)
        EditorNewMenuRow = (EditorNewMenuRow + 1) % 3;
    if (actor AND !EditorNewMenuLastActor)
        choice = EDITOR_NEW_ACTOR;
    else if (waypoint AND !EditorNewMenuLastWaypoint)
        choice = EDITOR_NEW_WAYPOINT;
    else if (zone AND !EditorNewMenuLastZone)
        choice = EDITOR_NEW_ZONE;
    else if (enter AND !EditorNewMenuLastEnter)
        choice = EditorNewMenuRow + 1;
    if (escape AND !EditorNewMenuLastEscape) {
        EditorNewMenuActive = FALSE;
        EditorSuppressEscapeUntilRelease = TRUE;
        EditorLastEscape = TRUE;
        Editor_SetStatus("New cancelled");
        FirstTime = AFF_ALL_FLIP;
    } else if (choice != EDITOR_NEW_NONE) {
        EditorBeginNewPlacement(choice);
    }

    EditorNewMenuLastUp = up;
    EditorNewMenuLastDown = down;
    EditorNewMenuLastEnter = enter;
    EditorNewMenuLastEscape = escape;
    EditorNewMenuLastActor = actor;
    EditorNewMenuLastWaypoint = waypoint;
    EditorNewMenuLastZone = zone;
    EditorLastActorOverlay = actor;
    EditorLastWaypointOverlay = waypoint;
    EditorLastZoneOverlay = zone;
    Editor_ConsumeCurrentFrameInput();
    return TRUE;
}

S32 Editor_WorldPlacementActive(void) {
    return EditorNewPlacement != EDITOR_NEW_NONE;
}

void Editor_PlaceAtWorld(S32 x, S32 y, S32 z) {
    if (EditorNewPlacement == EDITOR_NEW_ACTOR) {
        ActorEditorPlaceAt(x, y, z);
    } else if (EditorNewPlacement == EDITOR_NEW_WAYPOINT) {
        if (WaypointEditorCreateAt(x, y, z) >= 0)
            Editor_SetStatus("New waypoint added and selected");
    } else if (EditorNewPlacement == EDITOR_NEW_ZONE) {
        T_ZONE zone;
        memset(&zone, 0, sizeof(zone));
        zone.X0 = x - 256;
        zone.Y0 = y;
        zone.Z0 = z - 256;
        zone.X1 = x + 256;
        zone.Y1 = y + 512;
        zone.Z1 = z + 256;
        zone.Type = 2;
        zone.Num = 0;
        if (ZoneEditorCreateZone(&zone) >= 0)
            Editor_SetStatus("New zone added and selected");
    }
    if (!ActorEditorIsPlacing())
        EditorNewPlacement = EDITOR_NEW_NONE;
    FirstTime = AFF_ALL_FLIP;
}

static void Editor_ConsumeCurrentFrameInput(void) {
    Key = 0;
    MyKey = 0;
    Input = 0;
}

static S32 EditorInspectorKind(void) {
    if (Editor_ToolIs(EDITOR_TOOL_TEXTURE_SELECT) AND TerrainTextureEditorGetSelectedHalfPoly())
        return EDITOR_INSPECTOR_TEXTURE;
    if (Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE)) {
        if (WaypointEditorGetSelectedIndex() >= 0)
            return EDITOR_INSPECTOR_WAYPOINT;
        if (ActorEditorGetSelectedIndex() >= 0)
            return EDITOR_INSPECTOR_ACTOR;
        if (ExtDecorEditorGetSelectedObject())
            return EDITOR_INSPECTOR_DECOR;
        if (ZoneEditorGetSelectedZone())
            return EDITOR_INSPECTOR_ZONE;
        return EDITOR_INSPECTOR_SCENE;
    }
    if (Editor_ToolIs(EDITOR_TOOL_TERRAIN_REGION) AND TerrainRegionEditorHasSelected())
        return EDITOR_INSPECTOR_TERRAIN_REGION;
    if (Editor_ToolIs(EDITOR_TOOL_TERRAIN_EDIT) AND TerrainEditorGetSelectedCount() > 0)
        return EDITOR_INSPECTOR_TERRAIN;
    return EDITOR_INSPECTOR_NONE;
}

static S32 EditorInspectorSupportsCurrentTool(void) {
    return Editor_ToolIs(EDITOR_TOOL_TEXTURE_SELECT)
           OR Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE)
           OR Editor_ToolIs(EDITOR_TOOL_TERRAIN_REGION)
           OR Editor_ToolIs(EDITOR_TOOL_TERRAIN_EDIT);
}

static S32 EditorInspectorZoneIsMessage(void) {
    T_ZONE *zone = ZoneEditorGetSelectedZone();
    return zone AND zone->Type == 5;
}

static S32 EditorInspectorZoneModelField(S32 field) {
    if (EditorInspectorZoneIsMessage())
        return field > EDITOR_INSPECTOR_ZONE_TEXT_FIELD ? field - 1 : field;
    return field < ZONE_EDITOR_FIELD_INFO0 ? field : field + 3;
}

static S32 EditorInspectorFieldCount(S32 kind) {
    if (kind == EDITOR_INSPECTOR_TEXTURE)
        return TERRAIN_TEXTURE_FIELD_COUNT;
    if (kind == EDITOR_INSPECTOR_DECOR)
        return EXT_DECOR_FIELD_COUNT;
    if (kind == EDITOR_INSPECTOR_ACTOR)
        return ACTOR_EDITOR_FIELD_COUNT;
    if (kind == EDITOR_INSPECTOR_WAYPOINT)
        return EDITOR_WAYPOINT_FIELD_COUNT;
    if (kind == EDITOR_INSPECTOR_ZONE)
        return EditorInspectorZoneIsMessage() ? ZONE_EDITOR_FIELD_COUNT + 1 : 8;
    if (kind == EDITOR_INSPECTOR_TERRAIN_REGION)
        return TERRAIN_REGION_FIELD_COUNT;
    if (kind == EDITOR_INSPECTOR_TERRAIN)
        return TERRAIN_EDITOR_FIELD_COUNT;
    if (kind == EDITOR_INSPECTOR_SCENE)
        return 2;
    return 0;
}

static const char *EditorInspectorFieldName(S32 kind, S32 field) {
    if (kind == EDITOR_INSPECTOR_TEXTURE)
        return EditorInspectorTextureFieldNames[field];
    if (kind == EDITOR_INSPECTOR_DECOR)
        return EditorInspectorDecorFieldNames[field];
    if (kind == EDITOR_INSPECTOR_ACTOR)
        return EditorInspectorActorFieldNames[field];
    if (kind == EDITOR_INSPECTOR_WAYPOINT)
        return EditorInspectorWaypointFieldNames[field];
    if (kind == EDITOR_INSPECTOR_TERRAIN_REGION)
        return EditorInspectorTerrainRegionFieldNames[field];
    if (kind == EDITOR_INSPECTOR_TERRAIN)
        return EditorInspectorTerrainFieldNames[field];
    if (kind == EDITOR_INSPECTOR_SCENE)
        return EditorInspectorSceneFieldNames[field];
    if (EditorInspectorZoneIsMessage())
        return EditorInspectorMessageZoneFieldNames[field];
        return EditorInspectorZoneFieldNames[field];
}

static S32 EditorInspectorFieldIsReadOnly(S32 kind, S32 field) {
    if (kind == EDITOR_INSPECTOR_ACTOR AND field == ACTOR_EDITOR_FIELD_INDEX)
        return TRUE;
    if (kind == EDITOR_INSPECTOR_ACTOR
            AND(field == ACTOR_EDITOR_FIELD_FLAG_FIRST + 10
                OR field == ACTOR_EDITOR_FIELD_FLAG_FIRST + 18))
        return TRUE;
    if (kind == EDITOR_INSPECTOR_WAYPOINT AND field == EDITOR_WAYPOINT_FIELD_INDEX)
        return TRUE;
    if (kind == EDITOR_INSPECTOR_ZONE
            AND EditorInspectorZoneIsMessage()
            AND field == EDITOR_INSPECTOR_ZONE_TEXT_FIELD)
        return TRUE;
    if (kind == EDITOR_INSPECTOR_TERRAIN_REGION
            AND field == TERRAIN_REGION_FIELD_PATCHINESS
            AND TerrainRegionEditorGetFieldValue(TERRAIN_REGION_FIELD_TYPE) != 0)
        return TRUE;
    if (kind == EDITOR_INSPECTOR_TERRAIN AND field < TERRAIN_EDITOR_FIELD_HEIGHT)
        return TRUE;
    return kind == EDITOR_INSPECTOR_TERRAIN_REGION
           AND field >= TERRAIN_REGION_FIELD_X0;
}

static S32 EditorInspectorNextEditableField(S32 kind, S32 field, S32 direction, S32 field_count) {
    do {
        field += direction;
        if (field < 0)
            field = field_count - 1;
        if (field >= field_count)
            field = 0;
    } while (EditorInspectorFieldIsReadOnly(kind, field));
    return field;
}

static S32 EditorInspectorFieldValue(S32 kind, S32 field) {
    if (kind == EDITOR_INSPECTOR_TEXTURE) {
        return TerrainTextureEditorGetSelectedFieldValue(field, NULL);
    } else if (kind == EDITOR_INSPECTOR_DECOR) {
        T_DECORS *decor = ExtDecorEditorGetSelectedObject();
        if (!decor)
            return 0;
        switch (field) {
        case EXT_DECOR_FIELD_BODY: return decor->Body & 0xFFFF;
        case EXT_DECOR_FIELD_ROTATION: return ((decor->Beta & 4095) + 512) / 1024 & 3;
        case EXT_DECOR_FIELD_X: return decor->Xworld;
        case EXT_DECOR_FIELD_Y: return decor->Yworld;
        case EXT_DECOR_FIELD_Z: return decor->Zworld;
        }
    } else if (kind == EDITOR_INSPECTOR_ZONE) {
        T_ZONE *zone = ZoneEditorGetSelectedZone();
        if (!zone)
            return 0;
        if (EditorInspectorFieldIsReadOnly(kind, field))
            return 0;
        field = EditorInspectorZoneModelField(field);
        switch (field) {
        case ZONE_EDITOR_FIELD_TYPE: return zone->Type;
        case ZONE_EDITOR_FIELD_NUM: return zone->Num;
        case ZONE_EDITOR_FIELD_INFO0: return zone->Info0;
        case ZONE_EDITOR_FIELD_INFO1: return zone->Info1;
        case ZONE_EDITOR_FIELD_INFO2: return zone->Info2;
        case ZONE_EDITOR_FIELD_X0: return zone->X0;
        case ZONE_EDITOR_FIELD_Y0: return zone->Y0;
        case ZONE_EDITOR_FIELD_Z0: return zone->Z0;
        case ZONE_EDITOR_FIELD_X1: return zone->X1;
        case ZONE_EDITOR_FIELD_Y1: return zone->Y1;
        case ZONE_EDITOR_FIELD_Z1: return zone->Z1;
        }
    } else if (kind == EDITOR_INSPECTOR_ACTOR) {
        return ActorEditorGetFieldValue(field);
    } else if (kind == EDITOR_INSPECTOR_WAYPOINT) {
        return WaypointEditorGetFieldValue(field);
    } else if (kind == EDITOR_INSPECTOR_TERRAIN_REGION) {
        return TerrainRegionEditorGetFieldValue(field);
    } else if (kind == EDITOR_INSPECTOR_TERRAIN) {
        return TerrainEditorGetSelectedFieldValue(field, NULL);
    } else if (kind == EDITOR_INSPECTOR_SCENE) {
        if (field == EDITOR_INSPECTOR_SCENE_ALPHA_LIGHT)
            return AlphaLight;
        if (field == EDITOR_INSPECTOR_SCENE_BETA_LIGHT)
            return BetaLight;
    }
    return 0;
}

static S32 EditorInspectorFieldMixed(S32 kind, S32 field) {
    S32 mixed = FALSE;

    if (kind == EDITOR_INSPECTOR_TEXTURE)
        TerrainTextureEditorGetSelectedFieldValue(field, &mixed);
    else if (kind == EDITOR_INSPECTOR_TERRAIN)
        TerrainEditorGetSelectedFieldValue(field, &mixed);
    return mixed;
}

static S32 EditorInspectorFieldIsEnum(S32 kind, S32 field) {
    field = kind == EDITOR_INSPECTOR_ZONE ? EditorInspectorZoneModelField(field) : field;
    return (kind == EDITOR_INSPECTOR_ACTOR
            AND field >= ACTOR_EDITOR_FIELD_FLAG_FIRST
            AND field <= ACTOR_EDITOR_FIELD_FLAG_LAST)
           OR(kind == EDITOR_INSPECTOR_TEXTURE AND field == TERRAIN_TEXTURE_FIELD_TEX_FLAG)
           OR(kind == EDITOR_INSPECTOR_TEXTURE AND field == TERRAIN_TEXTURE_FIELD_CODE_JEU)
           OR(kind == EDITOR_INSPECTOR_DECOR AND field == EXT_DECOR_FIELD_ROTATION)
           OR(kind == EDITOR_INSPECTOR_ZONE AND field == ZONE_EDITOR_FIELD_TYPE)
           OR(kind == EDITOR_INSPECTOR_ZONE AND EditorInspectorZoneIsMessage() AND field == ZONE_EDITOR_FIELD_INFO2)
           OR(kind == EDITOR_INSPECTOR_TERRAIN_REGION AND field == TERRAIN_REGION_FIELD_TYPE);
}

static S32 EditorInspectorMessageDirectionIndex(S32 value) {
    if (value == 1)
        return 0;
    if (value == 2)
        return 1;
    if (value == 4)
        return 2;
    if (value == 8)
        return 3;
    return -1;
}

static S32 EditorInspectorFieldWraps(S32 kind, S32 field);
static S32 EditorInspectorFieldMax(S32 kind, S32 field);

static S32 EditorInspectorStepValue(S32 kind, S32 field, S32 direction) {
    S32 model_field = kind == EDITOR_INSPECTOR_ZONE ? EditorInspectorZoneModelField(field) : field;
    S32 value = EditorInspectorFieldValue(kind, field);

    if (kind == EDITOR_INSPECTOR_ZONE
            AND EditorInspectorZoneIsMessage()
            AND model_field == ZONE_EDITOR_FIELD_INFO2) {
        S32 index = EditorInspectorMessageDirectionIndex(value);
        if (index < 0)
            return direction > 0 ? 1 : 8;
        index = (index + direction + 4) % 4;
        return 1 << index;
    }
    value += direction;
    if (EditorInspectorFieldWraps(kind, field) AND value < 0)
        value = EditorInspectorFieldMax(kind, field);
    if (EditorInspectorFieldWraps(kind, field) AND value > EditorInspectorFieldMax(kind, field))
        value = 0;
    return value;
}

static S32 EditorInspectorFieldWraps(S32 kind, S32 field) {
    return kind == EDITOR_INSPECTOR_TEXTURE OR kind == EDITOR_INSPECTOR_SCENE OR EditorInspectorFieldIsEnum(kind, field);
}

static S32 EditorInspectorFieldMax(S32 kind, S32 field) {
    field = kind == EDITOR_INSPECTOR_ZONE ? EditorInspectorZoneModelField(field) : field;
    if (kind == EDITOR_INSPECTOR_TEXTURE)
        return EditorInspectorTextureFieldMax[field];
    if (kind == EDITOR_INSPECTOR_ACTOR
            AND field >= ACTOR_EDITOR_FIELD_FLAG_FIRST
            AND field <= ACTOR_EDITOR_FIELD_FLAG_LAST)
        return 1;
    if (kind == EDITOR_INSPECTOR_DECOR AND field == EXT_DECOR_FIELD_ROTATION)
        return 3;
    if (kind == EDITOR_INSPECTOR_ZONE AND field == ZONE_EDITOR_FIELD_TYPE)
        return 9;
    if (kind == EDITOR_INSPECTOR_TERRAIN_REGION AND field == TERRAIN_REGION_FIELD_TYPE)
        return 6;
    if (kind == EDITOR_INSPECTOR_TERRAIN_REGION AND field == TERRAIN_REGION_FIELD_PATCHINESS)
        return 100;
    if (kind == EDITOR_INSPECTOR_SCENE)
        return 4095;
    return 0;
}

static S32 EditorInspectorSetSceneField(S32 field, S32 value) {
    T_EDITOR_SCENE_EDIT *edit;

    value &= 4095;
    edit = EditorSceneGetOrCreateEdit(CurrentCubeX, CurrentCubeY);
    if (!edit)
        return FALSE;

    if (field == EDITOR_INSPECTOR_SCENE_ALPHA_LIGHT) {
        edit->AlphaLight = value;
    } else if (field == EDITOR_INSPECTOR_SCENE_BETA_LIGHT) {
        edit->BetaLight = value;
    } else {
        return FALSE;
    }
    EditorSceneApplyRuntimeEditsForLoadedCube(CurrentCubeX, CurrentCubeY);
    FirstTime = AFF_ALL_FLIP;
    return TRUE;
}

static S32 EditorInspectorSetField(S32 kind, S32 field, S32 value) {
    if (EditorInspectorFieldIsReadOnly(kind, field))
        return FALSE;
    if (kind == EDITOR_INSPECTOR_TEXTURE)
        return TerrainTextureEditorSetSelectedField(field, value);
    if (kind == EDITOR_INSPECTOR_DECOR)
        return ExtDecorEditorSetSelectedField(field, value);
    if (kind == EDITOR_INSPECTOR_ACTOR) {
        return ActorEditorSetFieldValue(field, value);
    }
    if (kind == EDITOR_INSPECTOR_WAYPOINT) {
        return WaypointEditorSetFieldValue(field, value);
    }
    if (kind == EDITOR_INSPECTOR_ZONE)
        return ZoneEditorSetSelectedField(EditorInspectorZoneModelField(field), value);
    if (kind == EDITOR_INSPECTOR_TERRAIN_REGION)
        return TerrainRegionEditorSetFieldValue(field, value);
    if (kind == EDITOR_INSPECTOR_TERRAIN)
        return TerrainEditorSetSelectedField(field, value);
    if (kind == EDITOR_INSPECTOR_SCENE)
        return EditorInspectorSetSceneField(field, value);
    return FALSE;
}

static S32 EditorInspectorFieldIsAction(S32 kind, S32 field) {
    return kind == EDITOR_INSPECTOR_TERRAIN_REGION
           AND(field == TERRAIN_REGION_FIELD_RANDOMIZE OR field == TERRAIN_REGION_FIELD_BAKE);
}

static void EditorInspectorFormatZoneText(char *dst, S32 dst_size) {
    T_ZONE *zone = ZoneEditorGetSelectedZone();
    char raw[256];
    S32 src;
    S32 out;
    S32 space;

    if (!zone OR zone->Type != 5) {
        dst[0] = '\0';
        return;
    }

    InitDial(START_FILE_ISLAND + Island);
    if (!GetMultiText(zone->Num, raw)) {
        snprintf(dst, dst_size, "Text: <missing>");
        return;
    }

    snprintf(dst, dst_size, "Text: ");
    out = (S32)strlen(dst);
    space = FALSE;
    for (src = 0; raw[src] AND out + 1 < dst_size; src++) {
        unsigned char c = (unsigned char)raw[src];
        if (c < 32 OR c == 127) {
            space = TRUE;
        } else {
            if (space AND out > 0 AND dst[out - 1] != ' ' AND out + 1 < dst_size)
                dst[out++] = ' ';
            dst[out++] = (char)c;
            space = FALSE;
        }
    }
    dst[out] = '\0';
}

static void EditorInspectorSetAscii(S32 active) {
    active = active ? TRUE : FALSE;
    if (active AND !EditorInspectorAscii) {
        EditorInspectorAsciiMemo = AsciiMode;
        AsciiMode = TRUE;
    } else if (!active AND EditorInspectorAscii) {
        AsciiMode = EditorInspectorAsciiMemo;
    }
    EditorInspectorAscii = active;
    if (!active)
        EditorInspectorAsciiText[0] = '\0';
}

static void EditorInspectorSetPlasma(S32 active) {
    active = active ? TRUE : FALSE;
    if (active AND !EditorInspectorPlasmaActive)
        InitPlasmaMenu();
    else if (!active AND EditorInspectorPlasmaActive)
        ClearPlasmaMenu();
    EditorInspectorPlasmaActive = active;
}

static S32 EditorUpdateInspectorInput(void) {
    S32 toggle;
    S32 tab;
    S32 up;
    S32 down;
    S32 left;
    S32 right;
    S32 enter;
    S32 backspace;
    S32 delete_key;
    S32 escape;
    S32 kind;
    S32 field_count;
    S32 shift;

    shift = (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? TRUE : FALSE;
    toggle = (CheckKey(K_CARRE) AND !shift) ? TRUE : FALSE;
    tab = CheckKey(K_TAB) ? TRUE : FALSE;
    up = CheckKey(K_UP) ? TRUE : FALSE;
    down = CheckKey(K_DOWN) ? TRUE : FALSE;
    left = CheckKey(K_LEFT) ? TRUE : FALSE;
    right = CheckKey(K_RIGHT) ? TRUE : FALSE;
    enter = (CheckKey(K_ENTER) OR CheckKey(K_NUMPAD_ENTER)) ? TRUE : FALSE;
    backspace = CheckKey(K_BACKSPACE) ? TRUE : FALSE;
    delete_key = (backspace OR CheckKey(K_SUPPR)) ? TRUE : FALSE;
    escape = CheckKey(K_ESC) ? TRUE : FALSE;

    if (!EditorInspectorSupportsCurrentTool()) {
        EditorInspectorVisible = FALSE;
        EditorInspectorFocused = FALSE;
        EditorInspectorSetAscii(FALSE);
        EditorInspectorSetPlasma(FALSE);
    } else if (toggle AND !EditorInspectorLastToggle) {
        EditorInspectorPreferredVisible = !EditorInspectorPreferredVisible;
        EditorInspectorVisible = EditorInspectorPreferredVisible;
        EditorInspectorFocused = FALSE;
        EditorInspectorSetAscii(FALSE);
        EditorInspectorSetPlasma(EditorInspectorVisible);
        Editor_SetStatus(EditorInspectorVisible ? "Inspector shown" : "Inspector hidden");
    } else if (EditorInspectorPreferredVisible AND !EditorInspectorVisible) {
        EditorInspectorVisible = TRUE;
        EditorInspectorFocused = FALSE;
        EditorInspectorSetPlasma(TRUE);
    }

    kind = EditorInspectorKind();
    field_count = EditorInspectorFieldCount(kind);
    if (kind != EditorInspectorLastKind) {
        EditorInspectorRow = 0;
        EditorInspectorSetAscii(FALSE);
        if (kind == EDITOR_INSPECTOR_NONE)
            EditorInspectorFocused = FALSE;
        EditorInspectorLastKind = kind;
    }

    if (EditorInspectorVisible AND field_count > 0 AND tab AND !EditorInspectorLastTab AND !EditorInspectorAscii) {
        EditorInspectorFocused = !EditorInspectorFocused;
        if (EditorInspectorFocused) {
            EditorInspectorLastUp = FALSE;
            EditorInspectorLastDown = FALSE;
            EditorInspectorLastLeft = FALSE;
            EditorInspectorLastRight = FALSE;
            EditorInspectorLastEnter = FALSE;
        }
        Editor_SetStatus(EditorInspectorFocused ? "Inspector focused" : "Inspector focus released");
    }

    if (EditorInspectorVisible AND EditorInspectorFocused AND escape AND !EditorInspectorEscapeConsumedUntilRelease) {
        if (EditorInspectorAscii) {
            EditorInspectorSetAscii(FALSE);
        } else {
            EditorInspectorFocused = FALSE;
            Editor_SetStatus("Inspector focus released");
        }
        EditorInspectorEscapeConsumedUntilRelease = TRUE;
    }
    if (!escape)
        EditorInspectorEscapeConsumedUntilRelease = FALSE;

    if (field_count > 0 AND EditorInspectorRow >= field_count)
        EditorInspectorRow = field_count - 1;
    if (field_count > 0 AND EditorInspectorFieldIsReadOnly(kind, EditorInspectorRow))
        EditorInspectorRow = EditorInspectorNextEditableField(kind, EditorInspectorRow, 1, field_count);
    if (EditorInspectorVisible AND EditorInspectorFocused AND field_count > 0) {
        if (EditorInspectorAscii) {
            S32 ascii;
            size_t len;

            if (enter AND !EditorInspectorLastEnter) {
                if (EditorInspectorAsciiText[0])
                    EditorInspectorSetField(kind, EditorInspectorRow, atoi(EditorInspectorAsciiText));
                EditorInspectorSetAscii(FALSE);
            } else if (backspace AND !EditorInspectorLastBackspace) {
                len = strlen(EditorInspectorAsciiText);
                if (len > 0)
                    EditorInspectorAsciiText[len - 1] = '\0';
            } else {
                ascii = GetAscii();
                if ((ascii >= '0' AND ascii <= '9') OR(ascii == '-' AND EditorInspectorAsciiText[0] == '\0')) {
                    len = strlen(EditorInspectorAsciiText);
                    if (len + 1 < sizeof(EditorInspectorAsciiText)) {
                        EditorInspectorAsciiText[len] = (char)ascii;
                        EditorInspectorAsciiText[len + 1] = '\0';
                    }
                }
            }
        } else {
            if (kind == EDITOR_INSPECTOR_ACTOR
                    AND delete_key
                    AND !EditorInspectorLastBackspace) {
                ActorEditorDeleteSelected();
                EditorInspectorFocused = FALSE;
            } else if (up AND !EditorInspectorLastUp) {
                EditorInspectorRow = EditorInspectorNextEditableField(kind, EditorInspectorRow, -1, field_count);
            }
            if (down AND !EditorInspectorLastDown) {
                EditorInspectorRow = EditorInspectorNextEditableField(kind, EditorInspectorRow, 1, field_count);
            }
            if (!EditorInspectorFieldIsReadOnly(kind, EditorInspectorRow) AND left AND !EditorInspectorLastLeft) {
                EditorInspectorSetField(kind, EditorInspectorRow, EditorInspectorStepValue(kind, EditorInspectorRow, -1));
            }
            if (!EditorInspectorFieldIsReadOnly(kind, EditorInspectorRow) AND right AND !EditorInspectorLastRight) {
                EditorInspectorSetField(kind, EditorInspectorRow, EditorInspectorStepValue(kind, EditorInspectorRow, 1));
            }
            if (enter AND !EditorInspectorLastEnter AND EditorInspectorFieldIsAction(kind, EditorInspectorRow)) {
                TerrainRegionEditorActivateField(EditorInspectorRow);
            } else if (!EditorInspectorFieldIsReadOnly(kind, EditorInspectorRow)
                    AND enter AND !EditorInspectorLastEnter
                        AND !EditorInspectorFieldIsEnum(kind, EditorInspectorRow)) {
                EditorInspectorAsciiText[0] = '\0';
                EditorInspectorSetAscii(TRUE);
            }
        }
    }

    EditorInspectorLastToggle = toggle;
    EditorInspectorLastTab = tab;
    EditorInspectorLastUp = up;
    EditorInspectorLastDown = down;
    EditorInspectorLastLeft = left;
    EditorInspectorLastRight = right;
    EditorInspectorLastEnter = enter;
    EditorInspectorLastBackspace = delete_key;

    return (EditorInspectorVisible AND EditorInspectorFocused) OR EditorInspectorEscapeConsumedUntilRelease;
}

static void ActorEditorRememberMovementKeys(void) {
    ActorEditorLastUp = CheckKey(K_UP) ? TRUE : FALSE;
    ActorEditorLastDown = CheckKey(K_DOWN) ? TRUE : FALSE;
    ActorEditorLastLeft = CheckKey(K_LEFT) ? TRUE : FALSE;
    ActorEditorLastRight = CheckKey(K_RIGHT) ? TRUE : FALSE;
    ActorEditorLastPageUp = CheckKey(K_PAGE_UP) ? TRUE : FALSE;
    ActorEditorLastPageDown = CheckKey(K_PAGE_DOWN) ? TRUE : FALSE;
}

static void WaypointEditorRememberMovementKeys(void) {
    WaypointEditorLastUp = CheckKey(K_UP) ? TRUE : FALSE;
    WaypointEditorLastDown = CheckKey(K_DOWN) ? TRUE : FALSE;
    WaypointEditorLastLeft = CheckKey(K_LEFT) ? TRUE : FALSE;
    WaypointEditorLastRight = CheckKey(K_RIGHT) ? TRUE : FALSE;
    WaypointEditorLastPageUp = CheckKey(K_PAGE_UP) ? TRUE : FALSE;
    WaypointEditorLastPageDown = CheckKey(K_PAGE_DOWN) ? TRUE : FALSE;
}

static void ActorEditorUpdateMovement(void) {
    S32 up;
    S32 down;
    S32 left;
    S32 right;
    S32 page_up;
    S32 page_down;
    S32 step;
    S32 shift;
    S32 alt;

    if (!Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE)
            OR ActorEditorGetSelectedIndex() < 0
            OR Editor_IsCameraPanActive()) {
        ActorEditorRememberMovementKeys();
        return;
    }

    up = CheckKey(K_UP) ? TRUE : FALSE;
    down = CheckKey(K_DOWN) ? TRUE : FALSE;
    left = CheckKey(K_LEFT) ? TRUE : FALSE;
    right = CheckKey(K_RIGHT) ? TRUE : FALSE;
    page_up = CheckKey(K_PAGE_UP) ? TRUE : FALSE;
    page_down = CheckKey(K_PAGE_DOWN) ? TRUE : FALSE;
    shift = (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? TRUE : FALSE;
    alt = (CheckKey(K_ALT) OR CheckKey(K_ALT_LEFT) OR CheckKey(K_ALT_RIGHT)) ? TRUE : FALSE;
    step = alt ? EDITOR_ACTOR_NUDGE_ALT : (shift ? EDITOR_ACTOR_NUDGE_SHIFT : EDITOR_ACTOR_NUDGE);

    if (up AND !ActorEditorLastUp)
        ActorEditorNudgeSelected(0, 0, -step);
    if (down AND !ActorEditorLastDown)
        ActorEditorNudgeSelected(0, 0, step);
    if (left AND !ActorEditorLastLeft)
        ActorEditorNudgeSelected(-step, 0, 0);
    if (right AND !ActorEditorLastRight)
        ActorEditorNudgeSelected(step, 0, 0);
    if (page_up AND !ActorEditorLastPageUp)
        ActorEditorNudgeSelected(0, step, 0);
    if (page_down AND !ActorEditorLastPageDown)
        ActorEditorNudgeSelected(0, -step, 0);

    ActorEditorLastUp = up;
    ActorEditorLastDown = down;
    ActorEditorLastLeft = left;
    ActorEditorLastRight = right;
    ActorEditorLastPageUp = page_up;
    ActorEditorLastPageDown = page_down;
}

static void WaypointEditorUpdateMovement(void) {
    S32 up;
    S32 down;
    S32 left;
    S32 right;
    S32 page_up;
    S32 page_down;
    S32 step;
    S32 shift;
    S32 alt;

    if (!Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE)
            OR !ListBrickTrack
            OR WaypointEditorGetSelectedIndex() < 0
            OR Editor_IsCameraPanActive()) {
        WaypointEditorRememberMovementKeys();
        return;
    }

    up = CheckKey(K_UP) ? TRUE : FALSE;
    down = CheckKey(K_DOWN) ? TRUE : FALSE;
    left = CheckKey(K_LEFT) ? TRUE : FALSE;
    right = CheckKey(K_RIGHT) ? TRUE : FALSE;
    page_up = CheckKey(K_PAGE_UP) ? TRUE : FALSE;
    page_down = CheckKey(K_PAGE_DOWN) ? TRUE : FALSE;
    shift = (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? TRUE : FALSE;
    alt = (CheckKey(K_ALT) OR CheckKey(K_ALT_LEFT) OR CheckKey(K_ALT_RIGHT)) ? TRUE : FALSE;
    step = alt ? EDITOR_ACTOR_NUDGE_ALT : (shift ? EDITOR_ACTOR_NUDGE_SHIFT : EDITOR_ACTOR_NUDGE);

    if (up AND !WaypointEditorLastUp)
        WaypointEditorNudgeSelected(0, 0, -step);
    if (down AND !WaypointEditorLastDown)
        WaypointEditorNudgeSelected(0, 0, step);
    if (left AND !WaypointEditorLastLeft)
        WaypointEditorNudgeSelected(-step, 0, 0);
    if (right AND !WaypointEditorLastRight)
        WaypointEditorNudgeSelected(step, 0, 0);
    if (page_up AND !WaypointEditorLastPageUp)
        WaypointEditorNudgeSelected(0, step, 0);
    if (page_down AND !WaypointEditorLastPageDown)
        WaypointEditorNudgeSelected(0, -step, 0);

    WaypointEditorLastUp = up;
    WaypointEditorLastDown = down;
    WaypointEditorLastLeft = left;
    WaypointEditorLastRight = right;
    WaypointEditorLastPageUp = page_up;
    WaypointEditorLastPageDown = page_down;
}

S32 Editor_IsActive(void) {
    return EditorVisible
           OR ExtDecorEditorGetSelect()
           OR TerrainEditorGetSelect()
           OR TerrainTextureEditorGetSelect();
}

S32 Editor_InspectorConsumesInput(void) {
    return EditorNewMenuActive
           OR FreeCam_IsActive()
           OR(EditorInspectorVisible AND EditorInspectorFocused);
}

void Editor_UpdateFreeCam(void) {
    S32 event;

    if (EditorNewMenuActive)
        return;
    event = FreeCam_UpdateEditor(Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE));
    if (event == FREECAM_EVENT_ENTERED)
        Editor_SetStatus("Free camera enabled");
    else if (event == FREECAM_EVENT_EXITED)
        Editor_SetStatus("Free camera disabled");
    else if (event == FREECAM_EVENT_FOG_DISABLED)
        Editor_SetStatus("Free camera fog disabled");
    else if (event == FREECAM_EVENT_FOG_ENABLED)
        Editor_SetStatus("Free camera fog enabled");
    else if (event == FREECAM_EVENT_ACTORS_HIDDEN)
        Editor_SetStatus("Free camera actors hidden");
    else if (event == FREECAM_EVENT_ACTORS_SHOWN)
        Editor_SetStatus("Free camera actors shown");
    // else if (event == FREECAM_EVENT_SCREENSHOT)
        // Editor_SetStatus("Free camera screenshot saved");

    if (FreeCam_IsActive()) {
        ShowMouse(false);
        EditorMouseShown = FALSE;
    } else if (event == FREECAM_EVENT_EXITED) {
        ShowMouse(true);
        EditorMouseShown = TRUE;
    }
}

void Editor_UpdateInput(void) {
    S32 save;
    S32 new_item;
    S32 delete_actor;
    S32 next_tool;
    S32 prev_tool;
    S32 decor_overlay;
    S32 zone_overlay;
    S32 actor_overlay;
    S32 waypoint_overlay;
    S32 overlay_detail;
    S32 cube_boundary_overlay;
    S32 escape;
    S32 enter;
    S32 ctrl;
    S32 alt;
    S32 shift;
    S32 left;
    S32 pan_dx;
    S32 pan_dz;
    S32 pan_step;
    S32 pan_angle;
    S32 view_front;
    S32 view_right;
    S32 view_top;
    S32 view_ortho;
    S32 view_reset;
    new_item = CheckKey(K_N) ? TRUE : FALSE;
    if (EditorNewMenuActive) {
        EditorUpdateNewMenuInput();
        EditorLastNew = new_item;
        return;
    }
    if (Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE)
            AND new_item
            AND !EditorLastNew
            AND !(EditorInspectorVisible AND EditorInspectorFocused)
            AND !Editor_WorldPlacementActive()) {
        EditorNewMenuActive = TRUE;
        EditorNewMenuRow = 0;
        EditorNewMenuLastUp = CheckKey(K_UP) ? TRUE : FALSE;
        EditorNewMenuLastDown = CheckKey(K_DOWN) ? TRUE : FALSE;
        EditorNewMenuLastEnter = (CheckKey(K_ENTER) OR CheckKey(K_NUMPAD_ENTER)) ? TRUE : FALSE;
        EditorNewMenuLastEscape = CheckKey(K_ESC) ? TRUE : FALSE;
        EditorNewMenuLastActor = CheckKey(K_A) ? TRUE : FALSE;
        EditorNewMenuLastWaypoint = CheckKey(K_W) ? TRUE : FALSE;
        EditorNewMenuLastZone = CheckKey(K_Z) ? TRUE : FALSE;
        Editor_SetStatus("New...");
        Editor_ConsumeCurrentFrameInput();
        EditorLastNew = new_item;
        FirstTime = AFF_ALL_FLIP;
        return;
    }
    EditorLastNew = new_item;

    if (FreeCam_IsActive()) {
        ActorEditorRememberMovementKeys();
        WaypointEditorRememberMovementKeys();
        Editor_ConsumeCurrentFrameInput();
        return;
    }

    if (EditorUpdateInspectorInput()) {
        ActorEditorRememberMovementKeys();
        WaypointEditorRememberMovementKeys();
        Editor_ConsumeCurrentFrameInput();
        return;
    }

    next_tool = CheckKey(K_PERIOD) ? TRUE : FALSE;
    prev_tool = CheckKey(K_COMMA) ? TRUE : FALSE;
    if (next_tool AND !EditorLastNextTool)
        Editor_CycleTool(1);
    if (prev_tool AND !EditorLastPrevTool)
        Editor_CycleTool(-1);
    EditorLastNextTool = next_tool;
    EditorLastPrevTool = prev_tool;

    ctrl = (CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT)) ? TRUE : FALSE;
    alt = (CheckKey(K_ALT) OR CheckKey(K_ALT_LEFT) OR CheckKey(K_ALT_RIGHT)) ? TRUE : FALSE;
    shift = (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? TRUE : FALSE;
    decor_overlay = CheckKey(K_D) ? TRUE : FALSE;
    zone_overlay = CheckKey(K_Z) ? TRUE : FALSE;
    actor_overlay = CheckKey(K_A) ? TRUE : FALSE;
    waypoint_overlay = CheckKey(K_W) ? TRUE : FALSE;
    overlay_detail = CheckKey(K_I) ? TRUE : FALSE;
    cube_boundary_overlay = CheckKey(K_C) ? TRUE : FALSE;

    if (EditorVisible AND !ctrl AND decor_overlay AND !EditorLastDecorOverlay)
        Editor_ToggleOverlay(EDITOR_OVERLAY_DECOR);
    if (EditorVisible AND zone_overlay AND !EditorLastZoneOverlay)
        Editor_ToggleOverlay(EDITOR_OVERLAY_ZONES);
    if (EditorVisible AND !ctrl AND actor_overlay AND !EditorLastActorOverlay)
        Editor_ToggleOverlay(EDITOR_OVERLAY_ACTORS);
    if (EditorVisible AND actor_overlay)
        Editor_ConsumeCurrentFrameInput();
    if (EditorVisible AND waypoint_overlay AND !EditorLastWaypointOverlay)
        Editor_ToggleOverlay(EDITOR_OVERLAY_WAYPOINTS);
    if (EditorVisible AND waypoint_overlay)
        Editor_ConsumeCurrentFrameInput();
    if (EditorVisible AND overlay_detail AND !EditorLastOverlayDetail)
        Editor_ToggleOverlayDetail();
    if (EditorVisible AND !ctrl AND cube_boundary_overlay AND !EditorLastCubeBoundaryOverlay) {
        EditorCubeBoundaryOverlay = !EditorCubeBoundaryOverlay;
        Editor_SetStatus(EditorCubeBoundaryOverlay ? "Cube boundaries enabled" : "Cube boundaries disabled");
        FirstTime = AFF_ALL_FLIP;
    }
    EditorLastDecorOverlay = decor_overlay;
    EditorLastZoneOverlay = zone_overlay;
    EditorLastActorOverlay = actor_overlay;
    EditorLastWaypointOverlay = waypoint_overlay;
    EditorLastOverlayDetail = overlay_detail;
    EditorLastCubeBoundaryOverlay = cube_boundary_overlay;

    escape = CheckKey(K_ESC) ? TRUE : FALSE;
    if (Editor_WorldPlacementActive() AND escape AND !EditorLastEscape) {
        ActorEditorCancelPlace();
        EditorNewPlacement = EDITOR_NEW_NONE;
        Editor_SetStatus("New placement cancelled");
        EditorSuppressEscapeUntilRelease = TRUE;
        Editor_ConsumeCurrentFrameInput();
        FirstTime = AFF_ALL_FLIP;
    }
    if (Editor_IsActive() AND escape AND !EditorLastEscape) {
        if (!EditorSuppressEscapeUntilRelease) {
            Editor_SetOff();
            EditorSuppressEscapeUntilRelease = TRUE;
        }
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

    delete_actor = (CheckKey(K_SUPPR) OR CheckKey(K_BACKSPACE)) ? TRUE : FALSE;
    if (delete_actor
            AND !EditorLastDeleteActor
            AND Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE)
            AND ActorEditorGetSelectedIndex() >= 0)
        ActorEditorDeleteSelected();
    EditorLastDeleteActor = delete_actor;

    view_front = (CheckKey(K_1) OR CheckKey(K_NUMPAD_1)) ? TRUE : FALSE;
    view_right = (CheckKey(K_3) OR CheckKey(K_NUMPAD_3)) ? TRUE : FALSE;
    view_top = (CheckKey(K_7) OR CheckKey(K_NUMPAD_7)) ? TRUE : FALSE;
    view_ortho = (CheckKey(K_5) OR CheckKey(K_NUMPAD_5)) ? TRUE : FALSE;
    view_reset = CheckKey(K_R) ? TRUE : FALSE;

    S32 zoom_in;
    S32 zoom_out;

    zoom_in = (CheckKey(K_EQUALS) OR CheckKey(K_KP_PLUS)) ? TRUE : FALSE;
    zoom_out = (CheckKey(K_MOINS) OR CheckKey(K_KP_MOINS)) ? TRUE : FALSE;

    if (Editor_IsActive() AND zoom_in AND !EditorLastZoomIn) {
        EditorZoomCamera(TRUE);
    }

    if (Editor_IsActive() AND zoom_out AND !EditorLastZoomOut) {
        EditorZoomCamera(FALSE);
    }

    EditorLastZoomIn = zoom_in;
    EditorLastZoomOut = zoom_out;

    if (Editor_IsActive() AND view_front AND !EditorLastViewFront) {
        EditorSetCameraPreset(EDITOR_FRONT_ALPHA, EDITOR_VIEW_BETA_FRONT);
    }
    if (Editor_IsActive() AND view_right AND !EditorLastViewRight) {
        EditorSetCameraPreset(EDITOR_RIGHT_ALPHA, EDITOR_VIEW_BETA_RIGHT);
    }
    if (Editor_IsActive() AND view_top AND !EditorLastViewTop) {
        EditorSetCameraPreset(EDITOR_TOP_ALPHA, EDITOR_VIEW_BETA_FRONT);
    }
    if (Editor_IsActive() AND view_ortho AND !EditorLastViewOrtho) {
        if (EditorCameraOrtho)
            EditorSetCameraPerspective();
        else
            EditorSetCameraOrtho();
    }
    if (Editor_IsActive() AND view_reset AND !EditorLastViewReset)
        EditorResetCameraToPlayer();

    EditorLastViewFront = view_front;
    EditorLastViewRight = view_right;
    EditorLastViewTop = view_top;
    EditorLastViewOrtho = view_ortho;
    EditorLastViewReset = view_reset;

    // -------------------------
    // Shift + Alt + Arrow key camera pan
    // -------------------------
    S32 arrow_left;
    S32 arrow_right;
    S32 arrow_up;
    S32 arrow_down;
    S32 arrow_dx = 0;
    S32 arrow_dz = 0;
    S32 arrow_step;
    S32 arrow_angle;

    if (Editor_IsActive() AND shift AND alt) {
        arrow_left = CheckKey(K_LEFT) ? TRUE : FALSE;
        arrow_right = CheckKey(K_RIGHT) ? TRUE : FALSE;
        arrow_up = CheckKey(K_UP) ? TRUE : FALSE;
        arrow_down = CheckKey(K_DOWN) ? TRUE : FALSE;

        if (arrow_left OR arrow_right OR arrow_up OR arrow_down) {
            arrow_step = VueDistance / 64;
            if (arrow_step < 32) arrow_step = 32;
            if (arrow_step > 128) arrow_step = 128;

            arrow_angle = (2048 - BetaCam) & 4095;

            if (arrow_left) {
                Rotate(arrow_step, 0, arrow_angle);
                arrow_dx += X0;
                arrow_dz += Z0;
            }

            if (arrow_right) {
                Rotate(-arrow_step, 0, arrow_angle);
                arrow_dx += X0;
                arrow_dz += Z0;
            }

            if (arrow_up) {
                Rotate(0, -arrow_step, arrow_angle);
                arrow_dx += X0;
                arrow_dz += Z0;
            }

            if (arrow_down) {
                Rotate(0, arrow_step, arrow_angle);
                arrow_dx += X0;
                arrow_dz += Z0;
            }

            VueOffsetX += arrow_dx;
            VueOffsetZ -= arrow_dz;

            SetFollowCamera(VueOffsetX, VueOffsetY, VueOffsetZ,
                            AlphaCam, BetaCam, GammaCam, VueDistance);

            EditorSaveEditorCamera();
            EditorSyncLoadedCubeToCamera();
            Editor_ConsumeCurrentFrameInput();
        }
    }

    ManageMouse();
    left = (Click & 1) ? TRUE : FALSE;
    if (!Editor_IsActive()) {
        EditorCameraPanActive = FALSE;
    } else if (left AND shift AND alt) {
        EditorCameraPanActive = TRUE;
    } else if (!left) {
        EditorCameraPanActive = FALSE;
    }

    if (EditorCameraPanActive AND EditorVisible AND CubeMode == CUBE_EXTERIEUR) {
        pan_step = VueDistance / 1024;
        if (pan_step < 64) pan_step = 64;
        if (pan_step > 512) pan_step = 512;

        pan_angle = (2048 - BetaCam) & 4095;

        Rotate(MouseXDep * pan_step, 0, pan_angle);
        pan_dx = X0;
        pan_dz = Z0;

        Rotate(0, -MouseYDep * pan_step, pan_angle);
        pan_dx += X0;
        pan_dz += Z0;

        VueOffsetX += pan_dx;
        VueOffsetZ -= pan_dz;
        SetFollowCamera(VueOffsetX, VueOffsetY, VueOffsetZ, AlphaCam, BetaCam, GammaCam, VueDistance);
        EditorSaveEditorCamera();
        EditorSyncLoadedCubeToCamera();
        Editor_ConsumeCurrentFrameInput();
    }

    ActorEditorUpdateMovement();
    WaypointEditorUpdateMovement();

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

static S32 EditorIslandHasCube(S32 cube_x, S32 cube_y) {
    if (cube_x < 0 OR cube_x >= EDITOR_ISLAND_MAP_SIZE OR cube_y < 0 OR cube_y >= EDITOR_ISLAND_MAP_SIZE)
        return FALSE;
    return (IsleMapIndex[cube_y * EDITOR_ISLAND_MAP_SIZE + cube_x] & 127) != 0;
}

static void EditorDrawCubeBoundarySegment(S32 x0, S32 z0, S32 x1, S32 z1) {
    S32 screen_x0;
    S32 screen_y0;
    S32 screen_x1;
    S32 screen_y1;

    LongWorldRotatePoint(x0, 0, z0);
    if (!LongProjectPoint(X0, Y0, Z0))
        return;
    screen_x0 = Xp;
    screen_y0 = Yp;

    LongWorldRotatePoint(x1, 0, z1);
    if (!LongProjectPoint(X0, Y0, Z0))
        return;
    screen_x1 = Xp;
    screen_y1 = Yp;

    Line(screen_x0, screen_y0, screen_x1, screen_y1, EDITOR_CUBE_BOUNDARY_COLOR);
}

static void EditorDrawCubeBoundaryLabel(S32 cube_x, S32 cube_y, S32 x, S32 z) {
    S32 screen_x;
    S32 screen_y;

    LongWorldRotatePoint(x, 0, z);
    if (!LongProjectPoint(X0, Y0, Z0))
        return;
    screen_x = Xp;
    screen_y = Yp;
    if (screen_x < 0 OR screen_x >= (S32)ModeDesiredX
            OR screen_y < 0 OR screen_y >= (S32)ModeDesiredY)
        return;
    CoulText(EDITOR_CUBE_BOUNDARY_COLOR, -1);
    GraphPrintf(FALSE, screen_x, screen_y, "c%d,%d", (int)cube_x, (int)cube_y);
}

static void EditorDrawCubeBoundaries(void) {
    S32 cube_x;
    S32 cube_y;
    S32 cube_size;

    if (!EditorVisible OR !EditorCubeBoundaryOverlay OR CubeMode != CUBE_EXTERIEUR)
        return;

    cube_size = NB_COTE * SIZE_BRICK_XZ;
    for (cube_y = 0; cube_y < EDITOR_ISLAND_MAP_SIZE; cube_y++) {
        for (cube_x = 0; cube_x < EDITOR_ISLAND_MAP_SIZE; cube_x++) {
            S32 x0;
            S32 z0;
            S32 x1;
            S32 z1;

            if (!EditorIslandHasCube(cube_x, cube_y))
                continue;

            x0 = (cube_x - CurrentCubeX) * cube_size;
            z0 = (cube_y - CurrentCubeY) * cube_size;
            x1 = x0 + cube_size;
            z1 = z0 + cube_size;

            EditorDrawCubeBoundarySegment(x0, z0, x1, z0);
            EditorDrawCubeBoundarySegment(x0, z0, x0, z1);
            if (!EditorIslandHasCube(cube_x, cube_y + 1))
                EditorDrawCubeBoundarySegment(x0, z1, x1, z1);
            if (!EditorIslandHasCube(cube_x + 1, cube_y))
                EditorDrawCubeBoundarySegment(x1, z0, x1, z1);
            if (EditorOverlayDetailIds)
                EditorDrawCubeBoundaryLabel(cube_x, cube_y, x0 + cube_size / 2, z0 + cube_size / 2);
        }
    }
}

static S32 EditorProjectPoint(S32 x, S32 y, S32 z, S32 *screen_x, S32 *screen_y) {
    LongWorldRotatePoint(x, y, z);
    if (!LongProjectPoint(X0, Y0, Z0))
        return FALSE;
    *screen_x = Xp;
    *screen_y = Yp;
    return TRUE;
}

static void EditorDrawActorBox(T_OBJET *actor, S32 selected) {
    S32 x0 = actor->Obj.X + actor->XMin;
    S32 x1 = actor->Obj.X + actor->XMax;
    S32 y0 = actor->Obj.Y + actor->YMin;
    S32 y1 = actor->Obj.Y + actor->YMax;
    S32 z0 = actor->Obj.Z + actor->ZMin;
    S32 z1 = actor->Obj.Z + actor->ZMax;
    S32 sx[8];
    S32 sy[8];

    if (x0 == x1 AND y0 == y1 AND z0 == z1) {
        x0 = actor->Obj.X - EDITOR_ACTOR_FALLBACK_HALF_WIDTH;
        x1 = actor->Obj.X + EDITOR_ACTOR_FALLBACK_HALF_WIDTH;
        y0 = actor->Obj.Y;
        y1 = actor->Obj.Y + EDITOR_ACTOR_FALLBACK_HEIGHT;
        z0 = actor->Obj.Z - EDITOR_ACTOR_FALLBACK_HALF_WIDTH;
        z1 = actor->Obj.Z + EDITOR_ACTOR_FALLBACK_HALF_WIDTH;
    }

    if (!EditorProjectPoint(x0, y0, z0, &sx[0], &sy[0])
            OR !EditorProjectPoint(x1, y0, z0, &sx[1], &sy[1])
            OR !EditorProjectPoint(x1, y0, z1, &sx[2], &sy[2])
            OR !EditorProjectPoint(x0, y0, z1, &sx[3], &sy[3])
            OR !EditorProjectPoint(x0, y1, z0, &sx[4], &sy[4])
            OR !EditorProjectPoint(x1, y1, z0, &sx[5], &sy[5])
            OR !EditorProjectPoint(x1, y1, z1, &sx[6], &sy[6])
            OR !EditorProjectPoint(x0, y1, z1, &sx[7], &sy[7]))
        return;

    Line(sx[0], sy[0], sx[1], sy[1], selected ? 12 : EDITOR_ACTOR_BOX_COLOR);
    Line(sx[1], sy[1], sx[2], sy[2], selected ? 12 : EDITOR_ACTOR_BOX_COLOR);
    Line(sx[2], sy[2], sx[3], sy[3], selected ? 12 : EDITOR_ACTOR_BOX_COLOR);
    Line(sx[3], sy[3], sx[0], sy[0], selected ? 12 : EDITOR_ACTOR_BOX_COLOR);
    Line(sx[0], sy[0], sx[4], sy[4], selected ? 15 : EDITOR_ACTOR_BOX_AXIS_COLOR);
    Line(sx[1], sy[1], sx[5], sy[5], selected ? 15 : EDITOR_ACTOR_BOX_AXIS_COLOR);
    Line(sx[2], sy[2], sx[6], sy[6], selected ? 15 : EDITOR_ACTOR_BOX_AXIS_COLOR);
    Line(sx[3], sy[3], sx[7], sy[7], selected ? 15 : EDITOR_ACTOR_BOX_AXIS_COLOR);
    Line(sx[4], sy[4], sx[5], sy[5], selected ? 15 : EDITOR_ACTOR_BOX_AXIS_COLOR);
    Line(sx[5], sy[5], sx[6], sy[6], selected ? 15 : EDITOR_ACTOR_BOX_AXIS_COLOR);
    Line(sx[6], sy[6], sx[7], sy[7], selected ? 15 : EDITOR_ACTOR_BOX_AXIS_COLOR);
    Line(sx[7], sy[7], sx[4], sy[4], selected ? 15 : EDITOR_ACTOR_BOX_AXIS_COLOR);
}

S32 ActorEditorFindPick(S32 *dist_sq) {
    S32 i;
    S32 best_index = -1;
    S32 best_dist = EDITOR_ACTOR_PICK_RADIUS * EDITOR_ACTOR_PICK_RADIUS;

    if (!Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE)
            OR !Editor_HasOverlay(EDITOR_OVERLAY_ACTORS)
            OR !ExtDecorEditorGetPickPending()) {
        *dist_sq = best_dist;
        return -1;
    }

    for (i = 1; i < NbObjets; i++) {
        T_OBJET *actor = &ListObjet[i];
        S32 screen_x;
        S32 screen_y;
        S32 dx;
        S32 dy;
        S32 dist;

        if (ActorEditorIsDeleted(i))
            continue;
        if (!EditorProjectPoint(actor->Obj.X, actor->Obj.Y + (actor->YMin + actor->YMax) / 2, actor->Obj.Z, &screen_x, &screen_y))
            continue;
        dx = screen_x - (S32)MouseX;
        dy = screen_y - (S32)MouseY;
        dist = dx * dx + dy * dy;
        if (dist <= best_dist) {
            best_dist = dist;
            best_index = i;
        }
    }

    *dist_sq = best_dist;
    return best_index;
}

S32 WaypointEditorFindPick(S32 *dist_sq) {
    S32 i;
    S32 best_index = -1;
    S32 best_dist = EDITOR_ACTOR_PICK_RADIUS * EDITOR_ACTOR_PICK_RADIUS;

    if (!Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE)
            OR !Editor_HasOverlay(EDITOR_OVERLAY_WAYPOINTS)
            OR !ExtDecorEditorGetPickPending()
            OR !ListBrickTrack) {
        *dist_sq = best_dist;
        return -1;
    }

    for (i = 0; i < NbBrickTrack; i++) {
        T_TRACK *waypoint = &ListBrickTrack[i];
        S32 screen_x;
        S32 screen_y;
        S32 dx;
        S32 dy;
        S32 dist;

        if (!EditorProjectPoint(waypoint->X, waypoint->Y, waypoint->Z, &screen_x, &screen_y))
            continue;
        dx = screen_x - (S32)MouseX;
        dy = screen_y - (S32)MouseY;
        dist = dx * dx + dy * dy;
        if (dist <= best_dist) {
            best_dist = dist;
            best_index = i;
        }
    }

    *dist_sq = best_dist;
    return best_index;
}

static void EditorDrawActors(void) {
    S32 i;

    if (!Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE)
            OR !Editor_HasOverlay(EDITOR_OVERLAY_ACTORS)
            OR NbObjets <= 0)
        return;

    for (i = 1; i < NbObjets; i++) {
        T_OBJET *actor = &ListObjet[i];
        S32 screen_x;
        S32 screen_y;

        if (ActorEditorIsDeleted(i))
            continue;
        EditorDrawActorBox(actor, i == ActorEditorGetSelectedIndex());
        if (!EditorOverlayDetailIds)
            continue;
        if (!EditorProjectPoint(actor->Obj.X, actor->Obj.Y + actor->YMax, actor->Obj.Z, &screen_x, &screen_y))
            continue;
        CoulText(i == ActorEditorGetSelectedIndex() ? 12 : EDITOR_ACTOR_TEXT_COLOR, EDITOR_ACTOR_TEXT_BACK);
        GraphPrintf(FALSE, screen_x, screen_y - 12, "a%d", (int)i);
    }
}

static void EditorDrawWaypoints(void) {
    S32 i;

    if (!Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE)
            OR !Editor_HasOverlay(EDITOR_OVERLAY_WAYPOINTS)
            OR !ListBrickTrack
            OR NbBrickTrack <= 0)
        return;

    for (i = 0; i < NbBrickTrack; i++) {
        T_TRACK *waypoint = &ListBrickTrack[i];
        S32 selected = i == WaypointEditorGetSelectedIndex();
        S32 center_x;
        S32 center_y;
        S32 x0;
        S32 y0;
        S32 x1;
        S32 y1;

        if (!EditorProjectPoint(waypoint->X, waypoint->Y, waypoint->Z, &center_x, &center_y))
            continue;

        if (EditorProjectPoint(waypoint->X - EDITOR_WAYPOINT_HALF_SIZE, waypoint->Y, waypoint->Z, &x0, &y0)
                AND EditorProjectPoint(waypoint->X + EDITOR_WAYPOINT_HALF_SIZE, waypoint->Y, waypoint->Z, &x1, &y1))
            Line(x0, y0, x1, y1, selected ? 12 : EDITOR_WAYPOINT_COLOR);
        if (EditorProjectPoint(waypoint->X, waypoint->Y - EDITOR_WAYPOINT_HALF_SIZE, waypoint->Z, &x0, &y0)
                AND EditorProjectPoint(waypoint->X, waypoint->Y + EDITOR_WAYPOINT_HALF_SIZE, waypoint->Z, &x1, &y1))
            Line(x0, y0, x1, y1, selected ? 15 : EDITOR_WAYPOINT_AXIS_COLOR);
        if (EditorProjectPoint(waypoint->X, waypoint->Y, waypoint->Z - EDITOR_WAYPOINT_HALF_SIZE, &x0, &y0)
                AND EditorProjectPoint(waypoint->X, waypoint->Y, waypoint->Z + EDITOR_WAYPOINT_HALF_SIZE, &x1, &y1))
            Line(x0, y0, x1, y1, selected ? 12 : EDITOR_WAYPOINT_COLOR);

        Rect(center_x - 2, center_y - 2, center_x + 2, center_y + 2, selected ? 15 : EDITOR_WAYPOINT_AXIS_COLOR);
        if (EditorOverlayDetailIds) {
            CoulText(selected ? 12 : EDITOR_WAYPOINT_COLOR, -1);
            GraphPrintf(FALSE, center_x, center_y - 12, "w%d", (int)i);
        }
    }
}

static void EditorDrawWorldPlacement(void) {
    S32 x;
    S32 y;
    S32 z;
    S32 screen_x;
    S32 screen_y;

    if (!Editor_WorldPlacementActive()
            OR !ExtDecorEditorGetPlacementPoint(&x, &y, &z)
            OR !EditorProjectPoint(x, y, z, &screen_x, &screen_y))
        return;
    Line(screen_x - 8, screen_y, screen_x - 2, screen_y, 12);
    Line(screen_x + 2, screen_y, screen_x + 8, screen_y, 12);
    Line(screen_x, screen_y - 8, screen_x, screen_y - 2, 12);
    Line(screen_x, screen_y + 2, screen_x, screen_y + 8, 12);
    Rect(screen_x - 3, screen_y - 3, screen_x + 3, screen_y + 3, 14);
}

static void EditorDrawNewMenu(void) {
    static const char *items[] = {"Actor", "Waypoint", "Zone"};
    S32 width = 220;
    S32 height = 94;
    S32 x0 = ((S32)ModeDesiredX - width) / 2;
    S32 y0 = ((S32)ModeDesiredY - height) / 2;
    S32 x1 = x0 + width;
    S32 y1 = y0 + height;
    S32 i;

    if (!EditorNewMenuActive)
        return;
    BackupAngles(x0, y0, x1, y1);
    ShadeBoxBlk(x0, y0, x1, y1, MENU_SHADE_LVL);
    DrawCadre(x0, y0, x1, y1, ALL_ANGLES | CADRE_RED);
    RestoreAngles(x0, y0, x1, y1);
    CoulText(LBAWHITE, -1);
    GraphPrintf(FALSE, x0 + 12, y0 + 10, "New...");
    for (i = 0; i < 3; i++) {
        S32 row_y = y0 + 32 + i * 18;
        if (i == EditorNewMenuRow) {
            BackupAngles(x0 + 8, row_y - 3, x1 - 8, row_y + 12);
            DrawFireBar(x0 + 8, row_y - 3, x1 - 8, row_y + 12, FIRE_SELECT_MENU);
            DrawCadre(x0 + 8, row_y - 3, x1 - 8, row_y + 12, ALL_ANGLES);
            RestoreAngles(x0 + 8, row_y - 3, x1 - 8, row_y + 12);
        }
        CoulText(LBAWHITE, -1);
        GraphPrintf(FALSE, x0 + 16, row_y, "%s", items[i]);
    }
}

void Editor_DrawOverlay(void) {
    S32 status_y;

    UnsetClip();
    EditorDrawCubeBoundaries();
    EditorDrawActors();
    EditorDrawWaypoints();
    EditorDrawWorldPlacement();

    if (Editor_ToolIs(EDITOR_TOOL_DECOR_PLACE)) {
        EditorFillRect(8, 8, 120, 40, 0);
        Rect(8, 8, 120, 40, 15);
        CoulText(15, -1);
        GraphPrintf(FALSE, 14, 13, "%s", ExtDecorEditorGetPaletteLibraryName());
        GraphPrintf(FALSE, 14, 13 + 16, "Body %d", (int)ExtDecorEditorGetPaletteBody());
    }

    if (EditorInspectorVisible) {
        S32 kind = EditorInspectorKind();
        S32 field_count = EditorInspectorFieldCount(kind);
        if (field_count > 0) {
            char text[128];
            S32 i;
            S32 header_rows = (kind == EDITOR_INSPECTOR_TEXTURE
                               OR kind == EDITOR_INSPECTOR_TERRAIN
                               OR kind == EDITOR_INSPECTOR_SCENE
                               OR kind == EDITOR_INSPECTOR_ACTOR)
                                  ? 1
                                  : 0;
            S32 first_row = 0;
            S32 visible_rows;
            S32 panel_x0 = 8;
            S32 panel_y0 = 8;
            S32 panel_x1 = 260;
            S32 panel_y1;

            if (panel_x1 >= ModeDesiredX)
                panel_x1 = ModeDesiredX - 8;
            visible_rows = (ModeDesiredY - panel_y0 - 12 - header_rows * 14) / 14;
            if (visible_rows > field_count)
                visible_rows = field_count;
            if (visible_rows < 1)
                visible_rows = 1;
            if (EditorInspectorRow >= visible_rows) {
                first_row = EditorInspectorRow - visible_rows / 2;
                if (first_row + visible_rows > field_count)
                    first_row = field_count - visible_rows;
            }
            panel_y1 = panel_y0 + 8 + (visible_rows + header_rows) * 14;
            BackupAngles(panel_x0, panel_y0, panel_x1, panel_y1);
            ShadeBoxBlk(panel_x0, panel_y0, panel_x1, panel_y1, MENU_SHADE_LVL);
            DrawCadre(panel_x0, panel_y0, panel_x1, panel_y1, ALL_ANGLES | (EditorInspectorFocused ? CADRE_RED : 0));
            RestoreAngles(panel_x0, panel_y0, panel_x1, panel_y1);

            if (kind == EDITOR_INSPECTOR_TEXTURE) {
                S32 selected_count = TerrainTextureEditorGetSelectedCount();
                CoulText(LBAWHITE, -1);
                GraphPrintf(FALSE, panel_x0 + 9, panel_y0 + 8, "%d triangle%s selected",
                            (int)selected_count,
                            selected_count == 1 ? "" : "s");
            } else if (kind == EDITOR_INSPECTOR_TERRAIN) {
                S32 selected_count = TerrainEditorGetSelectedCount();
                CoulText(LBAWHITE, -1);
                GraphPrintf(FALSE, panel_x0 + 9, panel_y0 + 8, "%d point%s selected",
                            (int)selected_count,
                            selected_count == 1 ? "" : "s");
            } else if (kind == EDITOR_INSPECTOR_SCENE) {
                CoulText(LBAWHITE, -1);
                GraphPrintf(FALSE, panel_x0 + 9, panel_y0 + 8, "Scene c%d,%d", (int)CurrentCubeX, (int)CurrentCubeY);
            } else if (kind == EDITOR_INSPECTOR_ACTOR) {
                CoulText(LBAWHITE, -1);
                GraphPrintf(FALSE, panel_x0 + 9, panel_y0 + 8, "Initial values:");
            }

            for (i = first_row; i < first_row + visible_rows; i++) {
                S32 y0 = panel_y0 + 6 + (header_rows + i - first_row) * 14;
                S32 y1 = y0 + 12;
                S32 value = EditorInspectorFieldValue(kind, i);
                S32 mixed = EditorInspectorFieldMixed(kind, i);

                if (EditorInspectorFocused AND i == EditorInspectorRow AND !EditorInspectorFieldIsReadOnly(kind, i)) {
                    BackupAngles(panel_x0 + 4, y0, panel_x1 - 4, y1);
                    DrawFireBar(panel_x0 + 4, y0, panel_x1 - 4, y1, FIRE_SELECT_MENU);
                    DrawCadre(panel_x0 + 4, y0, panel_x1 - 4, y1, ALL_ANGLES);
                    RestoreAngles(panel_x0 + 4, y0, panel_x1 - 4, y1);
                }
                if (kind == EDITOR_INSPECTOR_TEXTURE AND i == TERRAIN_TEXTURE_FIELD_BANK AND !mixed) {
                    T_HALF_POLY *poly = TerrainTextureEditorGetSelectedHalfPoly();
                    Rect(panel_x1 - 28, y0 + 1, panel_x1 - 8, y1 - 1, 0);
                    Box(panel_x1 - 27, y0 + 2, panel_x1 - 9, y1 - 2, (poly->Bank << 4) + 11);
                }
                if (kind == EDITOR_INSPECTOR_ZONE AND EditorInspectorFieldIsReadOnly(kind, i))
                    EditorInspectorFormatZoneText(text, sizeof(text));
                else if (EditorInspectorFieldIsAction(kind, i))
                    snprintf(text, sizeof(text), "%s", EditorInspectorFieldName(kind, i));
                else if (EditorInspectorAscii AND i == EditorInspectorRow)
                    snprintf(text, sizeof(text), "%s: %s_", EditorInspectorFieldName(kind, i), EditorInspectorAsciiText);
                else if (mixed)
                    snprintf(text, sizeof(text), "%s: --", EditorInspectorFieldName(kind, i));
                else if (kind == EDITOR_INSPECTOR_TEXTURE AND i == TERRAIN_TEXTURE_FIELD_TEX_FLAG)
                    snprintf(text, sizeof(text), "%s: %s (%d)", EditorInspectorFieldName(kind, i), EditorInspectorTexFlagNames[value], (int)value);
                else if (kind == EDITOR_INSPECTOR_TEXTURE AND i == TERRAIN_TEXTURE_FIELD_CODE_JEU)
                    snprintf(text, sizeof(text), "%s: %s (%d)", EditorInspectorFieldName(kind, i), EditorInspectorGameCodeNames[(value >= 0 AND value < 16) ? value : 0], (int)value);
                else if (kind == EDITOR_INSPECTOR_DECOR AND i == EXT_DECOR_FIELD_ROTATION)
                    snprintf(text, sizeof(text), "%s: %s", EditorInspectorFieldName(kind, i), EditorInspectorRotationNames[value]);
                else if (kind == EDITOR_INSPECTOR_ACTOR
                        AND i >= ACTOR_EDITOR_FIELD_FLAG_FIRST
                        AND i <= ACTOR_EDITOR_FIELD_FLAG_LAST)
                    snprintf(text, sizeof(text), "%s: %s", EditorInspectorFieldName(kind, i), value ? "On" : "Off");
                else if (kind == EDITOR_INSPECTOR_ZONE AND i == ZONE_EDITOR_FIELD_TYPE)
                    snprintf(text, sizeof(text), "%s: %s (%d)", EditorInspectorFieldName(kind, i), (value >= 0 AND value < 10) ? EditorInspectorZoneTypeNames[value] : "Unknown", (int)value);
                else if (kind == EDITOR_INSPECTOR_ZONE
                        AND EditorInspectorZoneIsMessage()
                        AND EditorInspectorZoneModelField(i) == ZONE_EDITOR_FIELD_INFO2) {
                    S32 direction = EditorInspectorMessageDirectionIndex(value);
                    snprintf(text, sizeof(text), "%s: %s (%d)", EditorInspectorFieldName(kind, i), direction >= 0 ? EditorInspectorMessageDirectionNames[direction] : "Unknown", (int)value);
                }
                else if (kind == EDITOR_INSPECTOR_TERRAIN_REGION AND i == TERRAIN_REGION_FIELD_TYPE)
                    snprintf(text, sizeof(text), "%s: %s", EditorInspectorFieldName(kind, i), EditorInspectorTerrainRegionTypeNames[(value >= 0 AND value < TERRAIN_REGION_TYPE_COUNT) ? value : 0]);
                else
                    snprintf(text, sizeof(text), "%s: %d", EditorInspectorFieldName(kind, i), (int)value);

                CoulText(LBAWHITE, -1);
                MemoClip();
                SetClip(panel_x0 + 7, y0 + 1, panel_x1 - 7, y1 - 1);
                GraphPrintf(FALSE, panel_x0 + 9, y0 + 2, "%s", text);
                RestoreClip();
            }
        }
    }

    EditorDrawNewMenu();

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
