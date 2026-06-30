#include "WAYPOINT_EDITOR.H"

#ifdef LBA2CC_EDITOR

#include "../C_EXTERN.H"
#include "../DIRECTORIES.H"
#include "../EDITOR.H"
#include "EDITOR_JSON.H"

#include <SYSTEM/FILES.H>
#include <SYSTEM/LOG.H>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern S32 Island;

typedef struct {
    S32 Used;
    S32 Scene;
    S32 Index;
    S32 Added;
    S32 BaseX;
    S32 BaseY;
    S32 BaseZ;
    S32 X;
    S32 Y;
    S32 Z;
} T_WAYPOINT_EDITOR_EDIT;

#define WAYPOINT_EDITOR_MAX_EDITS 1024

static T_WAYPOINT_EDITOR_EDIT WaypointEditorEdits[WAYPOINT_EDITOR_MAX_EDITS];
static T_TRACK *WaypointEditorOwnedList = NULL;
static S32 WaypointEditorScene = -1;
static S32 WaypointEditorIsland = -1;
static S32 WaypointEditorLoadedAuthoring = FALSE;
static S32 WaypointEditorSelectedIndex = -1;

static T_WAYPOINT_EDITOR_EDIT *WaypointEditorFindEdit(S32 scene, S32 index) {
    S32 i;

    for (i = 0; i < WAYPOINT_EDITOR_MAX_EDITS; i++) {
        T_WAYPOINT_EDITOR_EDIT *edit = &WaypointEditorEdits[i];
        if (edit->Used AND edit->Scene == scene AND edit->Index == index)
            return edit;
    }
    return NULL;
}

static T_WAYPOINT_EDITOR_EDIT *WaypointEditorGetOrCreateEdit(S32 index) {
    S32 i;
    T_WAYPOINT_EDITOR_EDIT *edit;
    T_TRACK *waypoint;

    edit = WaypointEditorFindEdit(WaypointEditorScene, index);
    if (edit)
        return edit;
    if (!ListBrickTrack OR index < 0 OR index >= NbBrickTrack)
        return NULL;
    waypoint = &ListBrickTrack[index];
    for (i = 0; i < WAYPOINT_EDITOR_MAX_EDITS; i++) {
        edit = &WaypointEditorEdits[i];
        if (!edit->Used) {
            memset(edit, 0, sizeof(*edit));
            edit->Used = TRUE;
            edit->Scene = WaypointEditorScene;
            edit->Index = index;
            edit->BaseX = edit->X = waypoint->X;
            edit->BaseY = edit->Y = waypoint->Y;
            edit->BaseZ = edit->Z = waypoint->Z;
            return edit;
        }
    }
    Editor_SetStatus("Waypoint edit refused: edit table is full");
    return NULL;
}

static S32 WaypointEditorResizeLoadedList(S32 count) {
    T_TRACK *list;

    if (count < 0 OR count > MAX_TRACKS)
        return FALSE;
    list = (T_TRACK *)malloc(sizeof(T_TRACK) * count);
    if (!list)
        return FALSE;
    if (ListBrickTrack AND NbBrickTrack > 0)
        memcpy(list, ListBrickTrack, sizeof(T_TRACK) * NbBrickTrack);
    if (WaypointEditorOwnedList)
        free(WaypointEditorOwnedList);
    WaypointEditorOwnedList = list;
    ListBrickTrack = list;
    return TRUE;
}

static void WaypointEditorApplyLoadedEdits(void) {
    S32 appended;
    S32 i;

    do {
        appended = FALSE;
        for (i = 0; i < WAYPOINT_EDITOR_MAX_EDITS; i++) {
            T_WAYPOINT_EDITOR_EDIT *edit = &WaypointEditorEdits[i];
            if (!edit->Used
                    OR edit->Scene != WaypointEditorScene
                    OR !edit->Added
                    OR edit->Index != NbBrickTrack)
                continue;
            if (!WaypointEditorResizeLoadedList(NbBrickTrack + 1))
                return;
            ListBrickTrack[NbBrickTrack].X = edit->X;
            ListBrickTrack[NbBrickTrack].Y = edit->Y;
            ListBrickTrack[NbBrickTrack].Z = edit->Z;
            NbBrickTrack++;
            appended = TRUE;
        }
    } while (appended);

    for (i = 0; i < WAYPOINT_EDITOR_MAX_EDITS; i++) {
        T_WAYPOINT_EDITOR_EDIT *edit = &WaypointEditorEdits[i];
        if (!edit->Used
                OR edit->Scene != WaypointEditorScene
                OR edit->Index < 0
                OR edit->Index >= NbBrickTrack)
            continue;
        ListBrickTrack[edit->Index].X = edit->X;
        ListBrickTrack[edit->Index].Y = edit->Y;
        ListBrickTrack[edit->Index].Z = edit->Z;
    }
}

static void WaypointEditorLoadAuthoringEdits(void) {
    char dir[ADELINE_MAX_PATH];
    char path[ADELINE_MAX_PATH];
    char *json;
    jsmntok_t *tokens;
    S32 count;
    S32 file_island;
    S32 scenes;
    S32 scene_token;

    if (WaypointEditorLoadedAuthoring AND WaypointEditorIsland == Island)
        return;
    memset(WaypointEditorEdits, 0, sizeof(WaypointEditorEdits));
    WaypointEditorIsland = Island;
    WaypointEditorLoadedAuthoring = TRUE;
    if (!Editor_ResolveOutputDir(dir, sizeof(dir)))
        return;
    snprintf(path, sizeof(path), "%s%swaypoint_edits.json", dir, ADELINE_PATH_SEP);
    path[sizeof(path) - 1] = '\0';
    if (!EditorJsonParseFile(path, &json, &tokens, &count))
        return;
    if (EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, 0, "island"), &file_island)
            AND file_island != Island) {
        free(tokens);
        free(json);
        return;
    }
    scenes = EditorJsonFindValue(json, tokens, count, 0, "scenes");
    if (scenes >= 0 AND tokens[scenes].type == JSMN_ARRAY) {
        S32 scenes_end = EditorJsonTokenEnd(tokens, count, scenes);
        for (scene_token = scenes + 1; scene_token < scenes_end; scene_token = EditorJsonTokenEnd(tokens, count, scene_token)) {
            S32 scene;
            S32 waypoints;
            S32 waypoint_token;
            if (tokens[scene_token].parent != scenes OR tokens[scene_token].type != JSMN_OBJECT)
                continue;
            if (!EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, scene_token, "scene"), &scene))
                continue;
            waypoints = EditorJsonFindValue(json, tokens, count, scene_token, "waypoints");
            if (waypoints < 0 OR tokens[waypoints].type != JSMN_ARRAY)
                continue;
            {
                S32 waypoints_end = EditorJsonTokenEnd(tokens, count, waypoints);
                for (waypoint_token = waypoints + 1; waypoint_token < waypoints_end; waypoint_token = EditorJsonTokenEnd(tokens, count, waypoint_token)) {
                    T_WAYPOINT_EDITOR_EDIT *edit = NULL;
                    S32 index;
                    S32 position[3];
                    S32 expect[3];
                    S32 added = FALSE;
                    S32 i;
                    if (tokens[waypoint_token].parent != waypoints OR tokens[waypoint_token].type != JSMN_OBJECT)
                        continue;
                    if (!EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, waypoint_token, "waypoint"), &index)
                            OR !EditorJsonParseIntArray(json, tokens, count, EditorJsonFindValue(json, tokens, count, waypoint_token, "position"), position, 3))
                        continue;
                    edit = WaypointEditorFindEdit(scene, index);
                    if (!edit) {
                        for (i = 0; i < WAYPOINT_EDITOR_MAX_EDITS; i++) {
                            if (!WaypointEditorEdits[i].Used) {
                                edit = &WaypointEditorEdits[i];
                                break;
                            }
                        }
                    }
                    if (!edit)
                        continue;
                    memset(edit, 0, sizeof(*edit));
                    edit->Used = TRUE;
                    edit->Scene = scene;
                    edit->Index = index;
                    edit->X = position[0];
                    edit->Y = position[1];
                    edit->Z = position[2];
                    EditorJsonParseBool(json, tokens, count, EditorJsonFindValue(json, tokens, count, waypoint_token, "added"), &added);
                    edit->Added = added;
                    if (EditorJsonParseIntArray(json, tokens, count, EditorJsonFindValue(json, tokens, count, waypoint_token, "expect_position"), expect, 3)) {
                        edit->BaseX = expect[0];
                        edit->BaseY = expect[1];
                        edit->BaseZ = expect[2];
                    } else {
                        edit->BaseX = edit->X;
                        edit->BaseY = edit->Y;
                        edit->BaseZ = edit->Z;
                    }
                }
            }
        }
    }
    Log_Info("[editor] loaded waypoint edits: %s", path);
    free(tokens);
    free(json);
}

void WaypointEditorOnSceneLoaded(S32 scene) {
    WaypointEditorScene = scene;
    WaypointEditorSelectedIndex = -1;
    WaypointEditorLoadAuthoringEdits();
    WaypointEditorApplyLoadedEdits();
}

void WaypointEditorClearSelection(void) {
    WaypointEditorSelectedIndex = -1;
}

S32 WaypointEditorGetSelectedIndex(void) {
    if (!ListBrickTrack
            OR WaypointEditorSelectedIndex < 0
            OR WaypointEditorSelectedIndex >= NbBrickTrack)
        return -1;
    return WaypointEditorSelectedIndex;
}

void WaypointEditorResolvePick(S32 index, S32 toggle) {
    if (!ListBrickTrack OR index < 0 OR index >= NbBrickTrack)
        return;
    if (toggle AND WaypointEditorSelectedIndex == index)
        WaypointEditorSelectedIndex = -1;
    else
        WaypointEditorSelectedIndex = index;
}

S32 WaypointEditorGetFieldValue(S32 field) {
    T_TRACK *waypoint;
    if (WaypointEditorGetSelectedIndex() < 0)
        return 0;
    waypoint = &ListBrickTrack[WaypointEditorSelectedIndex];
    if (field == 0)
        return WaypointEditorSelectedIndex;
    if (field == 1)
        return waypoint->X;
    if (field == 2)
        return waypoint->Y;
    if (field == 3)
        return waypoint->Z;
    return 0;
}

S32 WaypointEditorSetFieldValue(S32 field, S32 value) {
    T_WAYPOINT_EDITOR_EDIT *edit;
    if (WaypointEditorGetSelectedIndex() < 0 OR field < 1 OR field > 3)
        return FALSE;
    edit = WaypointEditorGetOrCreateEdit(WaypointEditorSelectedIndex);
    if (!edit)
        return FALSE;
    if (field == 1)
        edit->X = value;
    else if (field == 2)
        edit->Y = value;
    else
        edit->Z = value;
    ListBrickTrack[WaypointEditorSelectedIndex].X = edit->X;
    ListBrickTrack[WaypointEditorSelectedIndex].Y = edit->Y;
    ListBrickTrack[WaypointEditorSelectedIndex].Z = edit->Z;
    FirstTime = AFF_ALL_FLIP;
    return TRUE;
}

void WaypointEditorNudgeSelected(S32 dx, S32 dy, S32 dz) {
    T_WAYPOINT_EDITOR_EDIT *edit;
    if (WaypointEditorGetSelectedIndex() < 0)
        return;
    edit = WaypointEditorGetOrCreateEdit(WaypointEditorSelectedIndex);
    if (!edit)
        return;
    edit->X += dx;
    edit->Y += dy;
    edit->Z += dz;
    ListBrickTrack[WaypointEditorSelectedIndex].X = edit->X;
    ListBrickTrack[WaypointEditorSelectedIndex].Y = edit->Y;
    ListBrickTrack[WaypointEditorSelectedIndex].Z = edit->Z;
    FirstTime = AFF_ALL_FLIP;
}

S32 WaypointEditorCreateAt(S32 x, S32 y, S32 z) {
    T_WAYPOINT_EDITOR_EDIT *edit;
    S32 index;
    S32 i;

    if (NbBrickTrack >= MAX_TRACKS OR !WaypointEditorResizeLoadedList(NbBrickTrack + 1))
        return -1;
    index = NbBrickTrack++;
    ListBrickTrack[index].X = x;
    ListBrickTrack[index].Y = y;
    ListBrickTrack[index].Z = z;
    edit = NULL;
    for (i = 0; i < WAYPOINT_EDITOR_MAX_EDITS; i++) {
        if (!WaypointEditorEdits[i].Used) {
            edit = &WaypointEditorEdits[i];
            break;
        }
    }
    if (!edit)
        return -1;
    memset(edit, 0, sizeof(*edit));
    edit->Used = TRUE;
    edit->Scene = WaypointEditorScene;
    edit->Index = index;
    edit->Added = TRUE;
    edit->X = x;
    edit->Y = y;
    edit->Z = z;
    WaypointEditorSelectedIndex = index;
    FirstTime = AFF_ALL_FLIP;
    return index;
}

S32 WaypointEditorSaveRuntimeEdits(const char *island_dir, S32 island) {
    char path[ADELINE_MAX_PATH];
    FILE *f;
    S32 scene;
    S32 first_scene = TRUE;

    snprintf(path, sizeof(path), "%s%swaypoint_edits.json", island_dir, ADELINE_PATH_SEP);
    path[sizeof(path) - 1] = '\0';
    f = fopen(path, "wb");
    if (!f)
        return FALSE;
    fprintf(f, "{\n  \"island\": %d,\n  \"description\": \"Authoring edits exported by the waypoint editor.\",\n  \"scenes\": [\n", (int)island);
    for (scene = 0; scene < 256; scene++) {
        S32 i;
        S32 first = TRUE;
        S32 found = FALSE;
        for (i = 0; i < WAYPOINT_EDITOR_MAX_EDITS; i++) {
            if (WaypointEditorEdits[i].Used AND WaypointEditorEdits[i].Scene == scene) {
                found = TRUE;
                break;
            }
        }
        if (!found)
            continue;
        if (!first_scene)
            fprintf(f, ",\n");
        first_scene = FALSE;
        fprintf(f, "    {\n      \"scene\": %d,\n      \"waypoints\": [\n", (int)scene);
        for (i = 0; i < WAYPOINT_EDITOR_MAX_EDITS; i++) {
            T_WAYPOINT_EDITOR_EDIT *edit = &WaypointEditorEdits[i];
            if (!edit->Used OR edit->Scene != scene)
                continue;
            if (!first)
                fprintf(f, ",\n");
            first = FALSE;
            fprintf(f, "        {\"waypoint\": %d, ", (int)edit->Index);
            if (edit->Added)
                fprintf(f, "\"added\": true, ");
            else
                fprintf(f, "\"expect_position\": [%d, %d, %d], ", (int)edit->BaseX, (int)edit->BaseY, (int)edit->BaseZ);
            fprintf(f, "\"position\": [%d, %d, %d]}", (int)edit->X, (int)edit->Y, (int)edit->Z);
        }
        fprintf(f, "\n      ]\n    }");
    }
    fprintf(f, "\n  ]\n}\n");
    fclose(f);
    Log_Info("[editor] saved waypoint edits: %s", path);
    return TRUE;
}

#endif
