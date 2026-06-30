static S32 ZoneOverlayIds = FALSE;
static S32 ZoneOverlayBoxes = FALSE;
static S32 ZoneOverlayMaxDist = 0;
static S32 ZoneOverlayType = -1;
static S32 ZoneEditorLastUp = 0;
static S32 ZoneEditorLastDown = 0;
static S32 ZoneEditorLastLeft = 0;
static S32 ZoneEditorLastRight = 0;
static S32 ZoneEditorPickBestDist = 0;
static S32 ZoneEditorPickBestIndex = -1;
static T_ZONE *ZoneEditorOwnedList = NULL;

#define ZONE_EDITOR_MAX_SELECTED 1024
#define ZONE_EDITOR_MAX_EDITS 4096

typedef struct
{
    S32 Used;
    S32 Cube;
    S32 Index;
    S16 Type;
    S32 HasExpectType;
    S16 TargetType;
    S32 HasTargetType;
    S16 Num;
    S32 HasExpectNum;
    S16 TargetNum;
    S32 HasTargetNum;
    S32 TargetInfo[8];
    S32 HasTargetInfo;
    S32 HasExpectBounds;
    S32 HasTargetBounds;
    S32 BaseX0;
    S32 BaseY0;
    S32 BaseZ0;
    S32 BaseX1;
    S32 BaseY1;
    S32 BaseZ1;
    S32 TargetX0;
    S32 TargetY0;
    S32 TargetZ0;
    S32 TargetX1;
    S32 TargetY1;
    S32 TargetZ1;
    S32 Dx;
    S32 Dz;
    S32 SkipLogged;
    S32 Added;
    T_ZONE AddedZone;
    S32 LinkedDecor;
    S32 LinkDecorCubeX;
    S32 LinkDecorCubeY;
    S32 LinkDecorNum;
    S32 LinkDecorBody;
    S32 LinkDecorRole;
    S32 LinkDecorX;
    S32 LinkDecorY;
    S32 LinkDecorZ;
    S32 LinkDecorBeta;
    S32 LinkBaseInfo2;
    S32 LinkX0;
    S32 LinkY0;
    S32 LinkZ0;
    S32 LinkX1;
    S32 LinkY1;
    S32 LinkZ1;
} T_ZONE_EDITOR_EDIT;

static T_ZONE_EDITOR_EDIT ZoneEditorEdits[ZONE_EDITOR_MAX_EDITS];

#define ZONE_OVERLAY_TEXT 15
#define ZONE_OVERLAY_BACK 0
#define ZONE_OVERLAY_BOX 15
#define ZONE_OVERLAY_BOX_AXIS 248
#define ZONE_EDITOR_TEXT 0
#define ZONE_EDITOR_BACK 14
#define ZONE_EDITOR_BOX 14
#define ZONE_EDITOR_BOX_AXIS 12
#define ZONE_EDITOR_PICK_RADIUS 32
#define ZONE_EDITOR_NUDGE 512
#define ZONE_EDITOR_NUDGE_SHIFT (5 * ZONE_EDITOR_NUDGE)

static S32 ZoneEditorSelected[ZONE_EDITOR_MAX_SELECTED];

extern S32 ExtDecorEditorGetSelect(void);
extern S32 ExtDecorEditorGetPickPending(void);
extern void ExtDecorEditorResolveClickPick(S32 zone_index, S32 zone_dist_sq, S32 actor_index, S32 actor_dist_sq, S32 waypoint_index, S32 waypoint_dist_sq);
extern S32 Editor_SelectPickPending(void);
extern void Editor_ResolveSelectPick(S32 zone_index, S32 zone_dist_sq, S32 actor_index, S32 actor_dist_sq, S32 waypoint_index, S32 waypoint_dist_sq);

void ZoneOverlaySet(S32 ids, S32 boxes) {
    ZoneOverlayIds = ids ? TRUE : FALSE;
    ZoneOverlayBoxes = boxes ? TRUE : FALSE;
}

void ZoneOverlaySetMaxDist(S32 maxdist) {
    ZoneOverlayMaxDist = maxdist > 0 ? maxdist : 0;
}

void ZoneOverlaySetType(S32 type) {
    ZoneOverlayType = type >= 0 ? type : -1;
}

S32 ZoneOverlayGetIds(void) {
    return ZoneOverlayIds;
}

S32 ZoneOverlayGetBoxes(void) {
    return ZoneOverlayBoxes;
}

S32 ZoneOverlayGetMaxDist(void) {
    return ZoneOverlayMaxDist;
}

S32 ZoneOverlayGetType(void) {
    return ZoneOverlayType;
}

static S32 ZoneEditorIsSelected(S32 index) {
    if (index < 0 OR index >= ZONE_EDITOR_MAX_SELECTED)
        return FALSE;
    return ZoneEditorSelected[index];
}

static void ZoneEditorToggleSelected(S32 index) {
    if (index < 0 OR index >= ZONE_EDITOR_MAX_SELECTED)
        return;
    ZoneEditorSelected[index] = ZoneEditorSelected[index] ? FALSE : TRUE;
}

void ZoneEditorClearAllSelected(void) {
    memset(ZoneEditorSelected, 0, sizeof(ZoneEditorSelected));
}

S32 ZoneEditorHasAnySelected(void) {
    S32 n;

    for (n = 0; n < ZONE_EDITOR_MAX_SELECTED; n++) {
        if (ZoneEditorSelected[n])
            return TRUE;
    }
    return FALSE;
}

void ZoneEditorSelectOnly(S32 index) {
    memset(ZoneEditorSelected, 0, sizeof(ZoneEditorSelected));
    if (index >= 0 AND index < ZONE_EDITOR_MAX_SELECTED)
        ZoneEditorSelected[index] = TRUE;
}

void ZoneEditorToggleSelectedByIndex(S32 index) {
    ZoneEditorToggleSelected(index);
}

void ZoneEditorBeginPick(void) {
    ZoneEditorPickBestDist = ZONE_EDITOR_PICK_RADIUS * ZONE_EDITOR_PICK_RADIUS;
    ZoneEditorPickBestIndex = -1;
}

static T_ZONE_EDITOR_EDIT *ZoneEditorFindEdit(S32 cube, S32 index) {
    S32 i;

    for (i = 0; i < ZONE_EDITOR_MAX_EDITS; i++) {
        T_ZONE_EDITOR_EDIT *edit = &ZoneEditorEdits[i];

        if (edit->Used AND edit->Cube == cube AND edit->Index == index)
            return edit;
    }

    return NULL;
}

void ZoneEditorReleaseOwnedZoneList(void) {
    if (!ZoneEditorOwnedList)
        return;
    if (ListZone == ZoneEditorOwnedList)
        ListZone = NULL;
    free(ZoneEditorOwnedList);
    ZoneEditorOwnedList = NULL;
}

static S32 ZoneEditorResizeLoadedList(S32 count) {
    T_ZONE *new_list;

    if (count < 0 OR count > MAX_ZONES)
        return FALSE;
    new_list = (T_ZONE *)malloc(sizeof(T_ZONE) * count);
    if (!new_list) {
        Editor_SetStatus("Zone create refused: not enough memory");
        return FALSE;
    }
    if (ListZone AND NbZones > 0)
        memcpy(new_list, ListZone, sizeof(T_ZONE) * NbZones);
    if (ZoneEditorOwnedList)
        free(ZoneEditorOwnedList);
    ZoneEditorOwnedList = new_list;
    ListZone = new_list;
    return TRUE;
}

static void ZoneEditorApplyEditToZone(T_ZONE_EDITOR_EDIT *edit, T_ZONE *ptrz) {
    if (edit->HasTargetBounds) {
        ptrz->X0 = edit->TargetX0;
        ptrz->Y0 = edit->TargetY0;
        ptrz->Z0 = edit->TargetZ0;
        ptrz->X1 = edit->TargetX1;
        ptrz->Y1 = edit->TargetY1;
        ptrz->Z1 = edit->TargetZ1;
    } else {
        ptrz->X0 = edit->BaseX0 + edit->Dx;
        ptrz->Y0 = edit->BaseY0;
        ptrz->Z0 = edit->BaseZ0 + edit->Dz;
        ptrz->X1 = edit->BaseX1 + edit->Dx;
        ptrz->Y1 = edit->BaseY1;
        ptrz->Z1 = edit->BaseZ1 + edit->Dz;
    }
    if (edit->HasTargetType)
        ptrz->Type = edit->TargetType;
    if (edit->HasTargetNum)
        ptrz->Num = edit->TargetNum;
    if (edit->HasTargetInfo) {
        ptrz->Info0 = edit->TargetInfo[0];
        ptrz->Info1 = edit->TargetInfo[1];
        ptrz->Info2 = edit->TargetInfo[2];
        ptrz->Info3 = edit->TargetInfo[3];
        ptrz->Info4 = edit->TargetInfo[4];
        ptrz->Info5 = edit->TargetInfo[5];
        ptrz->Info6 = edit->TargetInfo[6];
        ptrz->Info7 = edit->TargetInfo[7];
    }
}

static T_ZONE_EDITOR_EDIT *ZoneEditorGetOrCreateEdit(S32 cube, S32 index, T_ZONE *ptrz) {
    S32 i;
    T_ZONE_EDITOR_EDIT *edit;

    edit = ZoneEditorFindEdit(cube, index);
    if (edit)
        return edit;

    for (i = 0; i < ZONE_EDITOR_MAX_EDITS; i++) {
        edit = &ZoneEditorEdits[i];
        if (!edit->Used) {
            edit->Used = TRUE;
            edit->Cube = cube;
            edit->Index = index;
            edit->Type = ptrz->Type;
            edit->HasExpectType = TRUE;
            edit->TargetType = ptrz->Type;
            edit->HasTargetType = FALSE;
            edit->Num = ptrz->Num;
            edit->HasExpectNum = TRUE;
            edit->TargetNum = ptrz->Num;
            edit->HasTargetNum = FALSE;
            edit->TargetInfo[0] = ptrz->Info0;
            edit->TargetInfo[1] = ptrz->Info1;
            edit->TargetInfo[2] = ptrz->Info2;
            edit->TargetInfo[3] = ptrz->Info3;
            edit->TargetInfo[4] = ptrz->Info4;
            edit->TargetInfo[5] = ptrz->Info5;
            edit->TargetInfo[6] = ptrz->Info6;
            edit->TargetInfo[7] = ptrz->Info7;
            edit->HasTargetInfo = FALSE;
            edit->HasExpectBounds = TRUE;
            edit->HasTargetBounds = FALSE;
            edit->BaseX0 = ptrz->X0;
            edit->BaseY0 = ptrz->Y0;
            edit->BaseZ0 = ptrz->Z0;
            edit->BaseX1 = ptrz->X1;
            edit->BaseY1 = ptrz->Y1;
            edit->BaseZ1 = ptrz->Z1;
            edit->Dx = 0;
            edit->Dz = 0;
            edit->SkipLogged = FALSE;
            return edit;
        }
    }

    Log_Warn("zoneedit runtime edit table is full; cannot track cube %d z%d t%d n%d",
             (int)cube,
             (int)index,
             (int)ptrz->Type,
             (int)ptrz->Num);
    return NULL;
}

static void ZoneEditorRotateXZ(S32 x, S32 z, S32 turn, S32 *out_x, S32 *out_z) {
    turn &= 3;
    if (turn == 0) {
        *out_x = x;
        *out_z = z;
    } else if (turn == 1) {
        *out_x = z;
        *out_z = -x;
    } else if (turn == 2) {
        *out_x = -x;
        *out_z = -z;
    } else {
        *out_x = -z;
        *out_z = x;
    }
}

static S32 ZoneEditorAngleDeltaToTurn(S32 delta) {
    delta &= 4095;
    return ((delta + 512) / 1024) & 3;
}

static S32 ZoneEditorRotateDirection(S32 direction, S32 turn) {
    S32 i;

    turn &= 3;
    for (i = 0; i < turn; i++) {
        if (direction == 1)
            direction = 8;
        else if (direction == 8)
            direction = 2;
        else if (direction == 2)
            direction = 4;
        else if (direction == 4)
            direction = 1;
    }
    return direction;
}

static void ZoneEditorSetEditInfo(T_ZONE_EDITOR_EDIT *edit, T_ZONE *zone) {
    edit->TargetInfo[0] = zone->Info0;
    edit->TargetInfo[1] = zone->Info1;
    edit->TargetInfo[2] = zone->Info2;
    edit->TargetInfo[3] = zone->Info3;
    edit->TargetInfo[4] = zone->Info4;
    edit->TargetInfo[5] = zone->Info5;
    edit->TargetInfo[6] = zone->Info6;
    edit->TargetInfo[7] = zone->Info7;
    edit->HasTargetInfo = TRUE;
}

static void ZoneEditorSetLinkedBaseBounds(T_ZONE_EDITOR_EDIT *edit) {
    edit->LinkX0 = edit->TargetX0;
    edit->LinkY0 = edit->TargetY0;
    edit->LinkZ0 = edit->TargetZ0;
    edit->LinkX1 = edit->TargetX1;
    edit->LinkY1 = edit->TargetY1;
    edit->LinkZ1 = edit->TargetZ1;
    edit->LinkBaseInfo2 = edit->TargetInfo[2];
}

void ZoneEditorSyncLinkedDecorZones(S32 decor_cube_x, S32 decor_cube_y, S32 decor_num, S32 decor_body, S32 decor_x, S32 decor_y, S32 decor_z, S32 decor_beta) {
    S32 i;

    for (i = 0; i < ZONE_EDITOR_MAX_EDITS; i++) {
        T_ZONE_EDITOR_EDIT *edit = &ZoneEditorEdits[i];
        S32 turn;
        S32 xs[4];
        S32 zs[4];
        S32 n;

        if (!edit->Used OR !edit->LinkedDecor)
            continue;
        if (edit->LinkDecorCubeX != decor_cube_x OR edit->LinkDecorCubeY != decor_cube_y OR edit->LinkDecorNum != decor_num OR edit->LinkDecorBody != decor_body)
            continue;

        turn = ZoneEditorAngleDeltaToTurn(decor_beta - edit->LinkDecorBeta);
        ZoneEditorRotateXZ(edit->LinkX0 - edit->LinkDecorX, edit->LinkZ0 - edit->LinkDecorZ, turn, &xs[0], &zs[0]);
        ZoneEditorRotateXZ(edit->LinkX0 - edit->LinkDecorX, edit->LinkZ1 - edit->LinkDecorZ, turn, &xs[1], &zs[1]);
        ZoneEditorRotateXZ(edit->LinkX1 - edit->LinkDecorX, edit->LinkZ0 - edit->LinkDecorZ, turn, &xs[2], &zs[2]);
        ZoneEditorRotateXZ(edit->LinkX1 - edit->LinkDecorX, edit->LinkZ1 - edit->LinkDecorZ, turn, &xs[3], &zs[3]);

        edit->TargetX0 = xs[0] + decor_x;
        edit->TargetX1 = edit->TargetX0;
        edit->TargetZ0 = zs[0] + decor_z;
        edit->TargetZ1 = edit->TargetZ0;
        for (n = 1; n < 4; n++) {
            S32 x = xs[n] + decor_x;
            S32 z = zs[n] + decor_z;

            if (x < edit->TargetX0)
                edit->TargetX0 = x;
            if (x > edit->TargetX1)
                edit->TargetX1 = x;
            if (z < edit->TargetZ0)
                edit->TargetZ0 = z;
            if (z > edit->TargetZ1)
                edit->TargetZ1 = z;
        }
        edit->TargetY0 = edit->LinkY0 + decor_y - edit->LinkDecorY;
        edit->TargetY1 = edit->LinkY1 + decor_y - edit->LinkDecorY;
        edit->HasTargetBounds = TRUE;

        if (edit->HasTargetInfo)
            edit->TargetInfo[2] = ZoneEditorRotateDirection(edit->LinkBaseInfo2, turn);

        if (ListZone AND edit->Cube == NumCube AND edit->Index >= 0 AND edit->Index < NbZones)
            ZoneEditorApplyEditToZone(edit, &ListZone[edit->Index]);

        edit->LinkDecorX = decor_x;
        edit->LinkDecorY = decor_y;
        edit->LinkDecorZ = decor_z;
        edit->LinkDecorBeta = decor_beta;
        ZoneEditorSetLinkedBaseBounds(edit);
    }
}

void ZoneEditorApplyRuntimeEditsForLoadedScene(S32 cube) {
    S32 base_zones = NbZones;
    S32 appended;
    S32 matched = 0;
    S32 applied = 0;
    S32 added = 0;
    S32 skipped = 0;
    S32 i;

    do {
        appended = FALSE;
        for (i = 0; i < ZONE_EDITOR_MAX_EDITS; i++) {
            T_ZONE_EDITOR_EDIT *edit = &ZoneEditorEdits[i];

            if (!edit->Used OR !edit->Added OR edit->Cube != cube OR edit->Index != NbZones)
                continue;
            matched++;
            if (!ZoneEditorResizeLoadedList(NbZones + 1))
                return;
            ListZone[NbZones] = edit->AddedZone;
            NbZones++;
            added++;
            appended = TRUE;
        }
    } while (appended);

    for (i = 0; i < ZONE_EDITOR_MAX_EDITS; i++) {
        T_ZONE_EDITOR_EDIT *edit = &ZoneEditorEdits[i];
        T_ZONE *ptrz;
        S32 bounds_match;

        if (!edit->Used OR edit->Cube != cube)
            continue;
        matched++;
        if (edit->Index < 0 OR edit->Index >= NbZones OR(edit->Added AND edit->Index < base_zones)) {
            skipped++;
            if (!edit->SkipLogged) {
                Log_Warn("[editor] zone edit skipped scene %d z%d: zone index is out of range [0,%d)",
                         (int)cube,
                         (int)edit->Index,
                         (int)NbZones);
                edit->SkipLogged = TRUE;
            }
            continue;
        }

        ptrz = &ListZone[edit->Index];
        bounds_match = ptrz->X0 == edit->BaseX0
                       AND ptrz->Y0 == edit->BaseY0
                       AND ptrz->Z0 == edit->BaseZ0
                       AND ptrz->X1 == edit->BaseX1
                       AND ptrz->Y1 == edit->BaseY1
                       AND ptrz->Z1 == edit->BaseZ1;
        if ((edit->HasExpectType AND ptrz->Type != edit->Type)
                OR(edit->HasExpectNum AND ptrz->Num != edit->Num)
                OR(edit->HasExpectBounds AND !bounds_match)) {
            skipped++;
            if (!edit->SkipLogged) {
                Log_Warn("[editor] zone edit skipped scene %d z%d: current t%d n%d bounds [%d,%d,%d,%d,%d,%d] do not match expected",
                         (int)cube,
                         (int)edit->Index,
                         (int)ptrz->Type,
                         (int)ptrz->Num,
                         (int)ptrz->X0,
                         (int)ptrz->Y0,
                         (int)ptrz->Z0,
                         (int)ptrz->X1,
                         (int)ptrz->Y1,
                         (int)ptrz->Z1);
                edit->SkipLogged = TRUE;
            }
            continue;
        }

        ZoneEditorApplyEditToZone(edit, ptrz);
        applied++;
    }

    if (matched OR applied OR added OR skipped) {
        Log_Info("[editor] applied zone edits for scene %d: base=%d final=%d added=%d applied=%d skipped=%d",
                 (int)cube,
                 (int)base_zones,
                 (int)NbZones,
                 (int)added,
                 (int)applied,
                 (int)skipped);
    }
}

void ZoneEditorClearRuntimeEdits(void) {
    memset(ZoneEditorEdits, 0, sizeof(ZoneEditorEdits));
    ZoneEditorClearAllSelected();
}

static T_ZONE_EDITOR_EDIT *ZoneEditorImportedEdit(S32 scene, S32 index) {
    S32 i;
    T_ZONE_EDITOR_EDIT *edit;

    edit = ZoneEditorFindEdit(scene, index);
    if (edit)
        return edit;

    for (i = 0; i < ZONE_EDITOR_MAX_EDITS; i++) {
        edit = &ZoneEditorEdits[i];
        if (!edit->Used) {
            memset(edit, 0, sizeof(*edit));
            edit->Used = TRUE;
            edit->Cube = scene;
            edit->Index = index;
            return edit;
        }
    }

    Log_Warn("[editor] zone import skipped scene %d z%d: runtime edit table is full",
             (int)scene,
             (int)index);
    return NULL;
}

static S32 ZoneEditorImportZone(const char *json,
                                jsmntok_t *tokens,
                                S32 count,
                                S32 scene,
                                S32 zone,
                                S32 *skipped) {
    S32 index;
    S32 type;
    S32 target_type;
    S32 num;
    S32 target_num;
    S32 info[8];
    S32 expect_bounds[6];
    S32 bounds[6];
    S32 decor_link[5];
    S32 decor_anchor[4];
    T_ZONE_EDITOR_EDIT *edit;

    if (!EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, zone, "zone"), &index)) {
        (*skipped)++;
        return FALSE;
    }
    if (!EditorJsonParseIntArray(json, tokens, count, EditorJsonFindValue(json, tokens, count, zone, "bounds"), bounds, 6)) {
        (*skipped)++;
        return FALSE;
    }

    edit = ZoneEditorImportedEdit(scene, index);
    if (!edit) {
        (*skipped)++;
        return FALSE;
    }

    edit->HasExpectType = EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, zone, "expect_type"), &type);
    edit->Type = (S16)(edit->HasExpectType ? type : 0);
    edit->HasTargetType = EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, zone, "type"), &target_type);
    edit->TargetType = (S16)(edit->HasTargetType ? target_type : edit->Type);
    edit->HasExpectNum = EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, zone, "expect_num"), &num);
    edit->Num = (S16)(edit->HasExpectNum ? num : 0);
    edit->HasTargetNum = EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, zone, "num"), &target_num);
    edit->TargetNum = (S16)(edit->HasTargetNum ? target_num : edit->Num);
    edit->HasTargetInfo = EditorJsonParseIntArray(json, tokens, count, EditorJsonFindValue(json, tokens, count, zone, "info"), info, 8);
    if (edit->HasTargetInfo)
        memcpy(edit->TargetInfo, info, sizeof(info));
    edit->HasExpectBounds = EditorJsonParseIntArray(json, tokens, count, EditorJsonFindValue(json, tokens, count, zone, "expect_bounds"), expect_bounds, 6);
    if (edit->HasExpectBounds) {
        edit->BaseX0 = expect_bounds[0];
        edit->BaseY0 = expect_bounds[1];
        edit->BaseZ0 = expect_bounds[2];
        edit->BaseX1 = expect_bounds[3];
        edit->BaseY1 = expect_bounds[4];
        edit->BaseZ1 = expect_bounds[5];
    } else {
        edit->BaseX0 = bounds[0];
        edit->BaseY0 = bounds[1];
        edit->BaseZ0 = bounds[2];
        edit->BaseX1 = bounds[3];
        edit->BaseY1 = bounds[4];
        edit->BaseZ1 = bounds[5];
    }
    edit->Dx = bounds[0] - edit->BaseX0;
    edit->Dz = bounds[2] - edit->BaseZ0;
    edit->HasTargetBounds = TRUE;
    edit->TargetX0 = bounds[0];
    edit->TargetY0 = bounds[1];
    edit->TargetZ0 = bounds[2];
    edit->TargetX1 = bounds[3];
    edit->TargetY1 = bounds[4];
    edit->TargetZ1 = bounds[5];
    edit->SkipLogged = FALSE;
    {
        S32 added = FALSE;
        EditorJsonParseBool(json, tokens, count, EditorJsonFindValue(json, tokens, count, zone, "add"), &added);
        edit->Added = added;
    }
    if (edit->Added) {
        memset(&edit->AddedZone, 0, sizeof(edit->AddedZone));
        edit->AddedZone.X0 = bounds[0];
        edit->AddedZone.Y0 = bounds[1];
        edit->AddedZone.Z0 = bounds[2];
        edit->AddedZone.X1 = bounds[3];
        edit->AddedZone.Y1 = bounds[4];
        edit->AddedZone.Z1 = bounds[5];
        edit->AddedZone.Type = edit->TargetType;
        edit->AddedZone.Num = edit->TargetNum;
        if (edit->HasTargetInfo) {
            edit->AddedZone.Info0 = info[0];
            edit->AddedZone.Info1 = info[1];
            edit->AddedZone.Info2 = info[2];
            edit->AddedZone.Info3 = info[3];
            edit->AddedZone.Info4 = info[4];
            edit->AddedZone.Info5 = info[5];
            edit->AddedZone.Info6 = info[6];
            edit->AddedZone.Info7 = info[7];
        }
    }
    edit->LinkedDecor = EditorJsonParseIntArray(json, tokens, count, EditorJsonFindValue(json, tokens, count, zone, "decor_link"), decor_link, 5);
    if (edit->LinkedDecor) {
        edit->LinkDecorCubeX = decor_link[0];
        edit->LinkDecorCubeY = decor_link[1];
        edit->LinkDecorNum = decor_link[2];
        edit->LinkDecorBody = decor_link[3];
        edit->LinkDecorRole = decor_link[4];
        if (EditorJsonParseIntArray(json, tokens, count, EditorJsonFindValue(json, tokens, count, zone, "decor_anchor"), decor_anchor, 4)) {
            edit->LinkDecorX = decor_anchor[0];
            edit->LinkDecorY = decor_anchor[1];
            edit->LinkDecorZ = decor_anchor[2];
            edit->LinkDecorBeta = decor_anchor[3];
        } else {
            edit->LinkDecorX = (bounds[0] + bounds[3]) / 2;
            edit->LinkDecorY = bounds[1];
            edit->LinkDecorZ = (bounds[2] + bounds[5]) / 2;
            edit->LinkDecorBeta = 2048;
        }
        edit->LinkBaseInfo2 = edit->TargetInfo[2];
        edit->LinkX0 = bounds[0];
        edit->LinkY0 = bounds[1];
        edit->LinkZ0 = bounds[2];
        edit->LinkX1 = bounds[3];
        edit->LinkY1 = bounds[4];
        edit->LinkZ1 = bounds[5];
    }
    return TRUE;
}

static void ZoneEditorImportScene(const char *json,
                                  jsmntok_t *tokens,
                                  S32 count,
                                  S32 scene_obj,
                                  S32 *loaded,
                                  S32 *skipped) {
    S32 scene;
    S32 zones;
    S32 zone;
    S32 zones_end;

    if (!EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, scene_obj, "scene"), &scene)) {
        (*skipped)++;
        return;
    }

    zones = EditorJsonFindValue(json, tokens, count, scene_obj, "zones");
    if (zones < 0 OR tokens[zones].type != JSMN_ARRAY)
        return;

    zones_end = EditorJsonTokenEnd(tokens, count, zones);
    for (zone = zones + 1; zone < zones_end; zone = EditorJsonTokenEnd(tokens, count, zone)) {
        if (tokens[zone].parent != zones)
            continue;
        if (tokens[zone].type != JSMN_OBJECT) {
            (*skipped)++;
            continue;
        }
        if (ZoneEditorImportZone(json, tokens, count, scene, zone, skipped))
            (*loaded)++;
    }
}

void ZoneEditorLoadAuthoringEditsForCurrentIsland(void) {
    char dir[ADELINE_MAX_PATH];
    char path[ADELINE_MAX_PATH];
    char *json;
    jsmntok_t *tokens;
    S32 count;
    S32 island_value;
    S32 scenes;
    S32 scene;
    S32 loaded = 0;
    S32 skipped = 0;

    if (!Editor_ResolveOutputDir(dir, sizeof(dir)))
        return;
    snprintf(path, sizeof(path), "%s%szone_edits.json", dir, ADELINE_PATH_SEP);
    path[sizeof(path) - 1] = '\0';

    if (!EditorJsonParseFile(path, &json, &tokens, &count)) {
        if (EditorJsonFileExists(path))
            Log_Warn("[editor] cannot load zone edits: invalid JSON: %s", path);
        return;
    }

    if (EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, 0, "island"), &island_value)
            AND island_value != Island) {
        Log_Warn("[editor] zone edits ignored: %s is for island %d, current island is %d",
                 path,
                 (int)island_value,
                 (int)Island);
        free(tokens);
        free(json);
        return;
    }

    scenes = EditorJsonFindValue(json, tokens, count, 0, "scenes");
    if (scenes >= 0 AND tokens[scenes].type == JSMN_ARRAY) {
        S32 scenes_end = EditorJsonTokenEnd(tokens, count, scenes);
        for (scene = scenes + 1; scene < scenes_end; scene = EditorJsonTokenEnd(tokens, count, scene)) {
            if (tokens[scene].parent != scenes)
                continue;
            if (tokens[scene].type != JSMN_OBJECT) {
                skipped++;
                continue;
            }
            ZoneEditorImportScene(json, tokens, count, scene, &loaded, &skipped);
        }
    } else {
        ZoneEditorImportScene(json, tokens, count, 0, &loaded, &skipped);
    }

    Log_Info("[editor] loaded zone edits: %s (loaded=%d skipped=%d)", path, (int)loaded, (int)skipped);
    if (ListZone)
        ZoneEditorApplyRuntimeEditsForLoadedScene(NumCube);
    free(tokens);
    free(json);
}

S32 ZoneEditorSaveRuntimeEdits(const char *island_dir, S32 island) {
    char path[ADELINE_MAX_PATH];
    FILE *f;
    S32 i;
    S32 first = TRUE;
    S32 count = 0;

    if (!island_dir OR !island_dir[0])
        return FALSE;

    snprintf(path, sizeof(path), "%s%szone_edits.json", island_dir, ADELINE_PATH_SEP);
    path[sizeof(path) - 1] = '\0';
    f = fopen(path, "wb");
    if (!f) {
        Editor_SetStatus("Zone save failed: cannot open zone_edits.json");
        Log_Warn("[editor] cannot write zone edits: %s", path);
        return FALSE;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"island\": %d,\n", (int)island);
    fprintf(f, "  \"description\": \"Authoring edits exported by the zone editor.\",\n");
    fprintf(f, "  \"scenes\": [\n");
    for (i = 0; i < ZONE_EDITOR_MAX_EDITS; i++) {
        T_ZONE_EDITOR_EDIT *edit = &ZoneEditorEdits[i];
        S32 x0;
        S32 y0;
        S32 z0;
        S32 x1;
        S32 y1;
        S32 z1;

        if (!edit->Used)
            continue;
        if (!first)
            fprintf(f, ",\n");
        first = FALSE;
        count++;

        if (edit->HasTargetBounds) {
            x0 = edit->TargetX0;
            y0 = edit->TargetY0;
            z0 = edit->TargetZ0;
            x1 = edit->TargetX1;
            y1 = edit->TargetY1;
            z1 = edit->TargetZ1;
        } else {
            x0 = edit->BaseX0 + edit->Dx;
            y0 = edit->BaseY0;
            z0 = edit->BaseZ0 + edit->Dz;
            x1 = edit->BaseX1 + edit->Dx;
            y1 = edit->BaseY1;
            z1 = edit->BaseZ1 + edit->Dz;
        }

        fprintf(f, "    {\n");
        fprintf(f, "      \"scene\": %d,\n", (int)edit->Cube);
        fprintf(f, "      \"zones\": [\n");
        fprintf(f, "        {\n");
        fprintf(f, "          \"zone\": %d,\n", (int)edit->Index);
        if (edit->Added)
            fprintf(f, "          \"add\": true,\n");
        if (edit->HasExpectType)
            fprintf(f, "          \"expect_type\": %d,\n", (int)edit->Type);
        if (edit->HasTargetType)
            fprintf(f, "          \"type\": %d,\n", (int)edit->TargetType);
        if (edit->HasExpectNum)
            fprintf(f, "          \"expect_num\": %d,\n", (int)edit->Num);
        if (edit->HasTargetNum)
            fprintf(f, "          \"num\": %d,\n", (int)edit->TargetNum);
        if (edit->HasExpectBounds)
        fprintf(f, "          \"expect_bounds\": [%d, %d, %d, %d, %d, %d],\n",
                (int)edit->BaseX0, (int)edit->BaseY0, (int)edit->BaseZ0,
                (int)edit->BaseX1, (int)edit->BaseY1, (int)edit->BaseZ1);
        fprintf(f, "          \"bounds\": [%d, %d, %d, %d, %d, %d]%s\n",
                (int)x0, (int)y0, (int)z0, (int)x1, (int)y1, (int)z1,
                (edit->Added OR edit->HasTargetInfo OR edit->LinkedDecor) ? "," : "");
        if (edit->Added OR edit->HasTargetInfo) {
            fprintf(f, "          \"info\": [%d, %d, %d, %d, %d, %d, %d, %d]%s\n",
                    (int)edit->TargetInfo[0], (int)edit->TargetInfo[1],
                    (int)edit->TargetInfo[2], (int)edit->TargetInfo[3],
                    (int)edit->TargetInfo[4], (int)edit->TargetInfo[5],
                    (int)edit->TargetInfo[6], (int)edit->TargetInfo[7],
                    edit->LinkedDecor ? "," : "");
        }
        if (edit->LinkedDecor) {
            fprintf(f, "          \"decor_link\": [%d, %d, %d, %d, %d],\n",
                    (int)edit->LinkDecorCubeX, (int)edit->LinkDecorCubeY,
                    (int)edit->LinkDecorNum, (int)edit->LinkDecorBody,
                    (int)edit->LinkDecorRole);
            fprintf(f, "          \"decor_anchor\": [%d, %d, %d, %d]\n",
                    (int)edit->LinkDecorX, (int)edit->LinkDecorY,
                    (int)edit->LinkDecorZ, (int)edit->LinkDecorBeta);
        }
        fprintf(f, "        }\n");
        fprintf(f, "      ]\n");
        fprintf(f, "    }");
    }
    fprintf(f, "\n");
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);

    Log_Info("[editor] saved %d zone edit(s) to %s", (int)count, path);
    return TRUE;
}

T_ZONE *ZoneEditorGetSelectedZone(void) {
    S32 i;
    T_ZONE *selected = NULL;

    if (!ListZone)
        return NULL;

    for (i = 0; i < NbZones AND i < ZONE_EDITOR_MAX_SELECTED; i++) {
        if (!ZoneEditorSelected[i])
            continue;
        if (selected)
            return NULL;
        selected = &ListZone[i];
    }
    return selected;
}

S32 ZoneEditorLogSelected(void) {
    S32 i;
    S32 count = 0;

    if (!ListZone OR NbZones <= 0)
        return 0;

    for (i = 0; i < NbZones AND i < ZONE_EDITOR_MAX_SELECTED; i++) {
        T_ZONE *zone;

        if (!ZoneEditorSelected[i])
            continue;

        zone = &ListZone[i];
        Log_Raw("[zone s%d #%d] type=%d num=%d bounds=[%d,%d,%d,%d,%d,%d] info=[%d,%d,%d,%d,%d,%d,%d,%d]",
                (int)NumCube,
                (int)i,
                (int)zone->Type,
                (int)zone->Num,
                (int)zone->X0,
                (int)zone->Y0,
                (int)zone->Z0,
                (int)zone->X1,
                (int)zone->Y1,
                (int)zone->Z1,
                (int)zone->Info0,
                (int)zone->Info1,
                (int)zone->Info2,
                (int)zone->Info3,
                (int)zone->Info4,
                (int)zone->Info5,
                (int)zone->Info6,
                (int)zone->Info7);
        count++;
    }

    return count;
}

S32 ZoneEditorCreateZone(const T_ZONE *zone) {
    T_ZONE_EDITOR_EDIT *edit;
    S32 index;

    if (!zone OR NbZones >= MAX_ZONES)
        return -1;
    index = NbZones;
    if (!ZoneEditorResizeLoadedList(NbZones + 1))
        return -1;
    ListZone[index] = *zone;
    NbZones++;

    edit = ZoneEditorGetOrCreateEdit(NumCube, index, &ListZone[index]);
    if (!edit) {
        NbZones--;
        return -1;
    }
    edit->Added = TRUE;
    edit->AddedZone = *zone;
    edit->HasExpectType = FALSE;
    edit->HasExpectNum = FALSE;
    edit->HasExpectBounds = FALSE;
    edit->TargetType = zone->Type;
    edit->HasTargetType = TRUE;
    edit->TargetNum = zone->Num;
    edit->HasTargetNum = TRUE;
    edit->TargetInfo[0] = zone->Info0;
    edit->TargetInfo[1] = zone->Info1;
    edit->TargetInfo[2] = zone->Info2;
    edit->TargetInfo[3] = zone->Info3;
    edit->TargetInfo[4] = zone->Info4;
    edit->TargetInfo[5] = zone->Info5;
    edit->TargetInfo[6] = zone->Info6;
    edit->TargetInfo[7] = zone->Info7;
    edit->HasTargetInfo = TRUE;
    edit->TargetX0 = zone->X0;
    edit->TargetY0 = zone->Y0;
    edit->TargetZ0 = zone->Z0;
    edit->TargetX1 = zone->X1;
    edit->TargetY1 = zone->Y1;
    edit->TargetZ1 = zone->Z1;
    edit->HasTargetBounds = TRUE;
    ZoneEditorSelectOnly(index);
    return index;
}

S32 ZoneEditorCreateLinkedDecorZone(const T_ZONE *zone, S32 decor_cube_x, S32 decor_cube_y, S32 decor_num, S32 decor_body, S32 decor_role, S32 decor_x, S32 decor_y, S32 decor_z, S32 decor_beta) {
    T_ZONE_EDITOR_EDIT *edit;
    S32 index;

    index = ZoneEditorCreateZone(zone);
    if (index < 0)
        return -1;

    edit = ZoneEditorFindEdit(NumCube, index);
    if (!edit)
        return index;

    edit->LinkedDecor = TRUE;
    edit->LinkDecorCubeX = decor_cube_x;
    edit->LinkDecorCubeY = decor_cube_y;
    edit->LinkDecorNum = decor_num;
    edit->LinkDecorBody = decor_body;
    edit->LinkDecorRole = decor_role;
    edit->LinkDecorX = decor_x;
    edit->LinkDecorY = decor_y;
    edit->LinkDecorZ = decor_z;
    edit->LinkDecorBeta = decor_beta;
    ZoneEditorSetEditInfo(edit, &ListZone[index]);
    ZoneEditorSetLinkedBaseBounds(edit);
    return index;
}

S32 ZoneEditorSetSelectedField(S32 field, S32 value) {
    T_ZONE *zone = ZoneEditorGetSelectedZone();
    T_ZONE_EDITOR_EDIT *edit;
    S32 index;

    if (!zone OR field < 0 OR field >= ZONE_EDITOR_FIELD_COUNT)
        return FALSE;

    index = (S32)(zone - ListZone);
    edit = ZoneEditorGetOrCreateEdit(NumCube, index, zone);
    if (!edit)
        return FALSE;

    if (!edit->HasTargetBounds) {
        edit->TargetX0 = zone->X0;
        edit->TargetY0 = zone->Y0;
        edit->TargetZ0 = zone->Z0;
        edit->TargetX1 = zone->X1;
        edit->TargetY1 = zone->Y1;
        edit->TargetZ1 = zone->Z1;
    }

    switch (field) {
    case ZONE_EDITOR_FIELD_TYPE:
        if (value < 0)
            value = 0;
        if (value > 9)
            value = 9;
        edit->TargetType = (S16)value;
        edit->HasTargetType = TRUE;
        break;
    case ZONE_EDITOR_FIELD_NUM:
        edit->TargetNum = (S16)value;
        edit->HasTargetNum = TRUE;
        break;
    case ZONE_EDITOR_FIELD_INFO0:
        edit->TargetInfo[0] = value;
        edit->HasTargetInfo = TRUE;
        break;
    case ZONE_EDITOR_FIELD_INFO1:
        edit->TargetInfo[1] = value;
        edit->HasTargetInfo = TRUE;
        break;
    case ZONE_EDITOR_FIELD_INFO2:
        edit->TargetInfo[2] = value;
        edit->HasTargetInfo = TRUE;
        break;
    case ZONE_EDITOR_FIELD_X0:
        edit->TargetX0 = value;
        edit->HasTargetBounds = TRUE;
        break;
    case ZONE_EDITOR_FIELD_Y0:
        edit->TargetY0 = value;
        edit->HasTargetBounds = TRUE;
        break;
    case ZONE_EDITOR_FIELD_Z0:
        edit->TargetZ0 = value;
        edit->HasTargetBounds = TRUE;
        break;
    case ZONE_EDITOR_FIELD_X1:
        edit->TargetX1 = value;
        edit->HasTargetBounds = TRUE;
        break;
    case ZONE_EDITOR_FIELD_Y1:
        edit->TargetY1 = value;
        edit->HasTargetBounds = TRUE;
        break;
    case ZONE_EDITOR_FIELD_Z1:
        edit->TargetZ1 = value;
        edit->HasTargetBounds = TRUE;
        break;
    }

    if (edit->LinkedDecor AND edit->HasTargetBounds)
        ZoneEditorSetLinkedBaseBounds(edit);
    ZoneEditorApplyEditToZone(edit, zone);
    return TRUE;
}

static void ZoneEditorMoveSelected(S32 dx, S32 dz) {
    S32 n;
    S32 moved = FALSE;
    T_ZONE *ptrz;

    if (!ListZone OR NbZones <= 0)
        return;

    ptrz = ListZone;
    for (n = 0; n < NbZones; n++, ptrz++) {
        T_ZONE_EDITOR_EDIT *edit;

        if (!ZoneEditorIsSelected(n))
            continue;

        edit = ZoneEditorGetOrCreateEdit(NumCube, n, ptrz);
        if (!edit)
            continue;
        if (edit->LinkedDecor)
            continue;

        if (edit->HasTargetBounds) {
            edit->TargetX0 += dx;
            edit->TargetZ0 += dz;
            edit->TargetX1 += dx;
            edit->TargetZ1 += dz;
        } else {
            edit->Dx += dx;
            edit->Dz += dz;
        }
        ZoneEditorApplyEditToZone(edit, ptrz);
        moved = TRUE;
    }

    if (moved)
        FirstTime = AFF_ALL_FLIP;
}

static void ZoneEditorBeginFrame(void) {
    S32 up;
    S32 down;
    S32 left;
    S32 right;
    S32 step;

    if (!Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE))
        return;

    up = CheckKey(K_UP) ? TRUE : FALSE;
    down = CheckKey(K_DOWN) ? TRUE : FALSE;
    left = CheckKey(K_LEFT) ? TRUE : FALSE;
    right = CheckKey(K_RIGHT) ? TRUE : FALSE;
    if (Editor_InspectorConsumesInput()) {
        ZoneEditorLastUp = up;
        ZoneEditorLastDown = down;
        ZoneEditorLastLeft = left;
        ZoneEditorLastRight = right;
        return;
    }
    step = (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? ZONE_EDITOR_NUDGE_SHIFT : ZONE_EDITOR_NUDGE;

    if (up AND !ZoneEditorLastUp)
        ZoneEditorMoveSelected(0, -step);
    if (down AND !ZoneEditorLastDown)
        ZoneEditorMoveSelected(0, step);
    if (left AND !ZoneEditorLastLeft)
        ZoneEditorMoveSelected(-step, 0);
    if (right AND !ZoneEditorLastRight)
        ZoneEditorMoveSelected(step, 0);

    ZoneEditorLastUp = up;
    ZoneEditorLastDown = down;
    ZoneEditorLastLeft = left;
    ZoneEditorLastRight = right;
}

static void ZoneEditorConsiderPickPoint(S32 zone_index, S32 xs, S32 ys) {
    S32 dx;
    S32 dy;
    S32 dist;

    if (!Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE)
            OR(!ExtDecorEditorGetPickPending() AND !Editor_SelectPickPending()))
        return;

    dx = xs - (S32)MouseX;
    dy = ys - (S32)MouseY;
    dist = dx * dx + dy * dy;
    if (dist <= ZoneEditorPickBestDist) {
        ZoneEditorPickBestDist = dist;
        ZoneEditorPickBestIndex = zone_index;
    }
}

static void ZoneEditorEndFrame(void) {
    S32 actor_dist_sq;
    S32 actor_index;
    S32 waypoint_dist_sq;
    S32 waypoint_index;

    if (!Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE))
        return;
    actor_index = ActorEditorFindPick(&actor_dist_sq);
    waypoint_index = WaypointEditorFindPick(&waypoint_dist_sq);
    if (ExtDecorEditorGetPickPending())
        ExtDecorEditorResolveClickPick(ZoneEditorPickBestIndex, ZoneEditorPickBestDist, actor_index, actor_dist_sq, waypoint_index, waypoint_dist_sq);
    else
        Editor_ResolveSelectPick(ZoneEditorPickBestIndex, ZoneEditorPickBestDist, actor_index, actor_dist_sq, waypoint_index, waypoint_dist_sq);
}

static const char *ZoneOverlayMessageDir(S32 dir) {
    switch (dir) {
    case 1:
        return "N";
    case 2:
        return "S";
    case 4:
        return "E";
    case 8:
        return "W";
    }

    return "-";
}

static S32 ZoneOverlayProject(S32 x, S32 y, S32 z, S32 *xs, S32 *ys) {
    LongWorldRotatePoint(x, y, z);
    if (!LongProjectPoint(X0, Y0, Z0))
        return FALSE;
    if (Xp < 0 OR Xp >= (S32)ModeDesiredX OR Yp < 0 OR Yp >= (S32)ModeDesiredY)
        return FALSE;
    *xs = Xp;
    *ys = Yp;
    return TRUE;
}

static void ZoneOverlayDrawBoxColor(T_ZONE *ptrz, S16 coul, S16 coul1) {
    S32 x0, y0, x1, y1, x2, y2, x3, y3, x4, y4, x5, y5, x6, y6, x7, y7;

    if (!ZoneOverlayProject(ptrz->X0, ptrz->Y0, ptrz->Z0, &x0, &y0))
        return;
    if (!ZoneOverlayProject(ptrz->X1, ptrz->Y0, ptrz->Z0, &x1, &y1))
        return;
    if (!ZoneOverlayProject(ptrz->X1, ptrz->Y0, ptrz->Z1, &x2, &y2))
        return;
    if (!ZoneOverlayProject(ptrz->X0, ptrz->Y0, ptrz->Z1, &x3, &y3))
        return;
    if (!ZoneOverlayProject(ptrz->X0, ptrz->Y1, ptrz->Z0, &x4, &y4))
        return;
    if (!ZoneOverlayProject(ptrz->X1, ptrz->Y1, ptrz->Z0, &x5, &y5))
        return;
    if (!ZoneOverlayProject(ptrz->X1, ptrz->Y1, ptrz->Z1, &x6, &y6))
        return;
    if (!ZoneOverlayProject(ptrz->X0, ptrz->Y1, ptrz->Z1, &x7, &y7))
        return;

    Line(x0, y0, x1, y1, coul);
    Line(x1, y1, x2, y2, coul);
    Line(x2, y2, x3, y3, coul);
    Line(x3, y3, x0, y0, coul);

    Line(x0, y0, x4, y4, coul1);
    Line(x1, y1, x5, y5, coul1);
    Line(x2, y2, x6, y6, coul1);
    Line(x3, y3, x7, y7, coul1);
    Line(x4, y4, x5, y5, coul1);
    Line(x5, y5, x6, y6, coul1);
    Line(x6, y6, x7, y7, coul1);
    Line(x7, y7, x4, y4, coul1);
}

static void ZoneOverlayDrawBox(T_ZONE *ptrz) {
    ZoneOverlayDrawBoxColor(ptrz, ZONE_OVERLAY_BOX, ZONE_OVERLAY_BOX_AXIS);
}

static S32 ZoneOverlayInDistance(T_ZONE *ptrz) {
    S64 dx;
    S64 dy;
    S64 dz;
    S64 maxdist;

    if (ZoneOverlayMaxDist <= 0)
        return TRUE;

    dx = ((S64)ptrz->X0 + (S64)ptrz->X1) / 2 - (S64)ListObjet[NUM_PERSO].Obj.X;
    dy = ((S64)ptrz->Y0 + (S64)ptrz->Y1) / 2 - (S64)ListObjet[NUM_PERSO].Obj.Y;
    dz = ((S64)ptrz->Z0 + (S64)ptrz->Z1) / 2 - (S64)ListObjet[NUM_PERSO].Obj.Z;
    maxdist = (S64)ZoneOverlayMaxDist;

    return (dx * dx + dy * dy + dz * dz) <= (maxdist * maxdist);
}

void ZoneOverlayDraw(void) {
    S32 n;
    T_ZONE *ptrz;

    ZoneEditorBeginFrame();

    if (!ZoneOverlayIds AND !ZoneOverlayBoxes) {
        ZoneEditorEndFrame();
        return;
    }

    if (!ListZone OR NbZones <= 0) {
        ZoneEditorEndFrame();
        return;
    }

    UnsetClip();

    ptrz = ListZone;
    for (n = 0; n < NbZones; n++, ptrz++) {
        S32 center_x;
        S32 center_y;
        S32 center_z;
        S32 xs, ys;
        S32 selected;

        if (ZoneOverlayType >= 0 AND ptrz->Type != ZoneOverlayType)
            continue;

        if (!ZoneOverlayInDistance(ptrz))
            continue;

        selected = ZoneEditorIsSelected(n);

        if (ZoneOverlayBoxes) {
            if (selected)
                ZoneOverlayDrawBoxColor(ptrz, ZONE_EDITOR_BOX, ZONE_EDITOR_BOX_AXIS);
            else
                ZoneOverlayDrawBox(ptrz);
            UnsetClip();
        }

        center_x = (ptrz->X0 + ptrz->X1) / 2;
        center_y = (ptrz->Y0 + ptrz->Y1) / 2;
        center_z = (ptrz->Z0 + ptrz->Z1) / 2;

        if (ZoneOverlayProject(center_x, center_y, center_z, &xs, &ys)) {
            S32 label_y = ys - 12;
            if (label_y < 0)
                label_y = ys;
            ZoneEditorConsiderPickPoint(n, xs, ys);
            ZoneEditorConsiderPickPoint(n, xs, label_y);
            if (ZoneOverlayIds) {
                if (selected) {
                    CoulText(ZONE_EDITOR_TEXT, ZONE_EDITOR_BACK);
                } else {
                    CoulText(ZONE_OVERLAY_TEXT, ZONE_OVERLAY_BACK);
                }
                if (ptrz->Type == 5) {
                    GraphPrintf(FALSE, xs, label_y, "z%d t%d n%d d%s",
                                n, ptrz->Type, ptrz->Num, ZoneOverlayMessageDir(ptrz->Info2));
                } else {
                    GraphPrintf(FALSE, xs, label_y, "z%d t%d n%d",
                                n, ptrz->Type, ptrz->Num);
                }
            }
        }

        if (ZoneOverlayProject(ptrz->X0, ptrz->Y0, ptrz->Z0, &xs, &ys))
            ZoneEditorConsiderPickPoint(n, xs, ys);
        if (ZoneOverlayProject(ptrz->X1, ptrz->Y0, ptrz->Z0, &xs, &ys))
            ZoneEditorConsiderPickPoint(n, xs, ys);
        if (ZoneOverlayProject(ptrz->X1, ptrz->Y0, ptrz->Z1, &xs, &ys))
            ZoneEditorConsiderPickPoint(n, xs, ys);
        if (ZoneOverlayProject(ptrz->X0, ptrz->Y0, ptrz->Z1, &xs, &ys))
            ZoneEditorConsiderPickPoint(n, xs, ys);
        if (ZoneOverlayProject(ptrz->X0, ptrz->Y1, ptrz->Z0, &xs, &ys))
            ZoneEditorConsiderPickPoint(n, xs, ys);
        if (ZoneOverlayProject(ptrz->X1, ptrz->Y1, ptrz->Z1, &xs, &ys))
            ZoneEditorConsiderPickPoint(n, xs, ys);
    }

    ZoneEditorEndFrame();

    UnsetClip();
}
