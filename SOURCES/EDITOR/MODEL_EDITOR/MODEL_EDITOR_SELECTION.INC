static void ModelToggleFaceSelection(T_BODY_HEADER *body, S32 index, const ModelFaceInfo *info) {
    if (!body OR !ModelFaceSelection)
        return;
    if (index < 0 OR index >= body->NbPolys OR index >= ModelFaceSelectionCapacity)
        return;
    if (ModelFaceSelection[index]) {
        ModelFaceSelection[index] = 0;
        if (ModelSelectedFaceCount > 0)
            ModelSelectedFaceCount--;
        if (ModelSelectedFace == index) {
            ModelSelectedFace = -1;
            ModelClearFaceInfo(&ModelSelectedFaceInfo);
        }
    } else {
        ModelFaceSelection[index] = 1;
        ModelSelectedFaceCount++;
        ModelSelectedFace = index;
        ModelSelectedFaceInfo = *info;
    }
}

static void ModelSelectFacesByColor(T_BODY_HEADER *body, S32 color, S32 add_to_selection) {
    S32 i;

    if (!body OR !ModelFaceSelection)
        return;
    if (!add_to_selection)
        ModelClearFaceSelection();
    for (i = 0; i < body->NbPolys AND i < ModelFaceSelectionCapacity; i++) {
        ModelFaceInfo info;

        if (!ModelFindFaceByIndex(body, i, &info))
            continue;
        if ((info.color & 0xff) == (color & 0xff)) {
            if (!ModelFaceSelection[i]) {
                ModelFaceSelection[i] = 1;
                ModelSelectedFaceCount++;
            }
            ModelSelectedFace = i;
            ModelSelectedFaceInfo = info;
        }
    }
}

static const char *ModelFaceTypeName(S32 face_type) {
    if (face_type == MODEL_FACE_TEXTURE)
        return "texture";
    if (face_type == MODEL_FACE_ENV)
        return "env";
    return "light";
}

static void ModelDumpSelectedFace(const ModelFaceInfo *info, T_BODY_HEADER *body) {
    T_OBJ_POINT *points = (T_OBJ_POINT *)((U8 *)body + body->OffPoints);
    S32 i;

    Log_Info("[model] face #%d header=%d offset=%d raw_type=%u type_poly=%d shape=%s kind=%s color=%u raw_color=%u normal=%u texture=%u scale=%u points=%d",
             (int)info->index,
             (int)info->header_offset,
             (int)info->offset,
             (unsigned)info->raw_type_poly,
             (int)info->type_poly,
             info->is_quad ? "quad" : "tri",
             ModelFaceTypeName(info->face_type),
             (unsigned)(info->color & 0xff),
             (unsigned)info->color,
             (unsigned)info->normal,
             (unsigned)info->texture,
             (unsigned)info->scale,
             (int)info->point_count);

    for (i = 0; i < info->point_count; i++) {
        U16 point = info->points[i];

        if (point >= body->NbPoints OR point >= MODEL_MAX_POINTS) {
            Log_Info("[model]   p%d=%u invalid uv=(%u,%u)",
                     (int)i,
                     (unsigned)point,
                     (unsigned)info->u[i],
                     (unsigned)info->v[i]);
            continue;
        }
        Log_Info("[model]   p%d=%u obj=(%d,%d,%d g%d) screen=(%d,%d) rotated=(%d,%d,%d) uv=(%u,%u)",
                 (int)i,
                 (unsigned)point,
                 (int)points[point].X,
                 (int)points[point].Y,
                 (int)points[point].Z,
                 (int)points[point].Group,
                 (int)Obj_ListProjectedPoints[point].X,
                 (int)Obj_ListProjectedPoints[point].Y,
                 (int)Obj_ListRotatedPoints[point].X,
                 (int)Obj_ListRotatedPoints[point].Y,
                 (int)Obj_ListRotatedPoints[point].Z,
                 (unsigned)info->u[i],
                 (unsigned)info->v[i]);
    }
}

static void ModelDumpSelectedFaces(T_BODY_HEADER *body) {
    S32 i;
    S32 dumped = 0;

    if (!body OR !ModelFaceSelection OR ModelSelectedFaceCount <= 0) {
        Log_Info("[model] no selected faces to dump");
        ModelSetStatus("No selected faces to dump");
        return;
    }

    Log_Info("[model] selected faces dump body=%d archive=%d selected=%d total=%d",
             (int)ModelBodyId,
             (int)ModelArchive,
             (int)ModelSelectedFaceCount,
             (int)body->NbPolys);
    for (i = 0; i < body->NbPolys AND i < ModelFaceSelectionCapacity; i++) {
        ModelFaceInfo info;

        if (!ModelFaceSelection[i])
            continue;
        if (!ModelFindFaceByIndex(body, i, &info)) {
            Log_Info("[model] face #%d selected but unreadable", (int)i);
            continue;
        }
        ModelDumpSelectedFace(&info, body);
        dumped++;
    }
    ModelSetStatus("Dumped %d selected faces to console", (int)dumped);
}

static void ModelSetSingleSphereSelection(T_BODY_HEADER *body, S32 index, const ModelSphereInfo *info) {
    if (!body OR !ModelSphereSelection)
        return;
    ModelClearSphereSelection();
    if (index >= 0 AND index < body->NbSpheres AND index < ModelSphereSelectionCapacity) {
        ModelSphereSelection[index] = 1;
        ModelSelectedSphereCount = 1;
        ModelSelectedSphere = index;
        ModelSelectedSphereInfo = *info;
    }
}

static void ModelToggleSphereSelection(T_BODY_HEADER *body, S32 index, const ModelSphereInfo *info) {
    if (!body OR !ModelSphereSelection)
        return;
    if (index < 0 OR index >= body->NbSpheres OR index >= ModelSphereSelectionCapacity)
        return;
    if (ModelSphereSelection[index]) {
        ModelSphereSelection[index] = 0;
        if (ModelSelectedSphereCount > 0)
            ModelSelectedSphereCount--;
        if (ModelSelectedSphere == index) {
            ModelSelectedSphere = -1;
            ModelClearSphereInfo(&ModelSelectedSphereInfo);
        }
    } else {
        ModelSphereSelection[index] = 1;
        ModelSelectedSphereCount++;
        ModelSelectedSphere = index;
        ModelSelectedSphereInfo = *info;
    }
}

static void ModelSetSingleLineSelection(T_BODY_HEADER *body, S32 index, const ModelLineInfo *info) {
    if (!body OR !ModelLineSelection)
        return;
    ModelClearLineSelection();
    if (index >= 0 AND index < body->NbLines AND index < ModelLineSelectionCapacity) {
        ModelLineSelection[index] = 1;
        ModelSelectedLineCount = 1;
        ModelSelectedLine = index;
        ModelSelectedLineInfo = *info;
    }
}

static void ModelToggleLineSelection(T_BODY_HEADER *body, S32 index, const ModelLineInfo *info) {
    if (!body OR !ModelLineSelection)
        return;
    if (index < 0 OR index >= body->NbLines OR index >= ModelLineSelectionCapacity)
        return;
    if (ModelLineSelection[index]) {
        ModelLineSelection[index] = 0;
        if (ModelSelectedLineCount > 0)
            ModelSelectedLineCount--;
        if (ModelSelectedLine == index) {
            ModelSelectedLine = -1;
            ModelClearLineInfo(&ModelSelectedLineInfo);
        }
    } else {
        ModelLineSelection[index] = 1;
        ModelSelectedLineCount++;
        ModelSelectedLine = index;
        ModelSelectedLineInfo = *info;
    }
}

static void ModelSetSingleBoneSelection(T_BODY_HEADER *body, S32 index) {
    if (!body OR !ModelBoneSelection)
        return;
    ModelClearBoneSelection();
    if (index >= 0 AND index < body->NbGroupes AND index < ModelBoneSelectionCapacity) {
        ModelBoneSelection[index] = 1;
        ModelSelectedBoneCount = 1;
        ModelSelectedBone = index;
    }
}

static void ModelToggleBoneSelection(T_BODY_HEADER *body, S32 index) {
    if (!body OR !ModelBoneSelection)
        return;
    if (index < 0 OR index >= body->NbGroupes OR index >= ModelBoneSelectionCapacity)
        return;
    if (ModelBoneSelection[index]) {
        ModelBoneSelection[index] = 0;
        if (ModelSelectedBoneCount > 0)
            ModelSelectedBoneCount--;
        if (ModelSelectedBone == index)
            ModelSelectedBone = -1;
    } else {
        ModelBoneSelection[index] = 1;
        ModelSelectedBoneCount++;
        ModelSelectedBone = index;
    }
}

static void ModelAddFocusPoint(T_BODY_HEADER *body, U16 point, S64 *x, S64 *y, S64 *z, S32 *count) {
    if (!body OR point >= body->NbPoints)
        return;
    if (point >= MODEL_MAX_POINTS)
        return;
    LongInverseRotatePoint(&MatriceWorld,
                           Obj_ListRotatedPoints[point].X - CameraXr,
                           Obj_ListRotatedPoints[point].Y - CameraYr,
                           Obj_ListRotatedPoints[point].Z - CameraZr);
    *x += X0;
    *y += Y0;
    *z += Z0;
    (*count)++;
}

static void ModelFocusZoom(S32 zoom) {
    if (ModelZoom > zoom)
        ModelZoom = zoom;
    ModelClampZoom();
}

static void ModelFocusSelected(T_BODY_HEADER *body) {
    S64 x = 0;
    S64 y = 0;
    S64 z = 0;
    S32 count = 0;
    S32 i;

    if (!body)
        return;
    if (ModelSelectMode == MODEL_SELECT_VERTEX) {
        if (!ModelVertexSelection OR ModelSelectedVertexCount <= 0) {
            ModelSetStatus("Focus refused: no vertex selected");
            return;
        }
        for (i = 0; i < body->NbPoints AND i < MODEL_MAX_POINTS AND i < ModelVertexSelectionCapacity; i++) {
            if (ModelVertexSelection[i])
                ModelAddFocusPoint(body, (U16)i, &x, &y, &z, &count);
        }
        if (count <= 0) {
            ModelSetStatus("Focus refused: bad vertex");
            return;
        }
        ModelTargetX = (S32)(x / count);
        ModelTargetY = (S32)(y / count);
        ModelTargetZ = (S32)(z / count);
        ModelFocusZoom(3500);
        ModelCameraMode = MODEL_CAMERA_ORBIT;
        return;
    }

    if (ModelSelectMode == MODEL_SELECT_SPHERE) {
        if (!ModelSphereSelection OR ModelSelectedSphereCount <= 0) {
            ModelSetStatus("Focus refused: no sphere selected");
            return;
        }
        for (i = 0; i < body->NbSpheres AND i < ModelSphereSelectionCapacity; i++) {
            ModelSphereInfo info;

            if (!ModelSphereSelection[i])
                continue;
            if (!ModelReadSphereInfo(body, i, &info))
                continue;
            ModelAddFocusPoint(body, info.point, &x, &y, &z, &count);
        }
        if (count <= 0) {
            ModelSetStatus("Focus refused: bad sphere");
            return;
        }
        ModelTargetX = (S32)(x / count);
        ModelTargetY = (S32)(y / count);
        ModelTargetZ = (S32)(z / count);
        ModelFocusZoom(5000);
        ModelCameraMode = MODEL_CAMERA_ORBIT;
        return;
    }

    if (ModelSelectMode == MODEL_SELECT_LINE) {
        if (!ModelLineSelection OR ModelSelectedLineCount <= 0) {
            ModelSetStatus("Focus refused: no line selected");
            return;
        }
        for (i = 0; i < body->NbLines AND i < ModelLineSelectionCapacity; i++) {
            ModelLineInfo info;

            if (!ModelLineSelection[i])
                continue;
            if (!ModelReadLineInfo(body, i, &info))
                continue;
            ModelAddFocusPoint(body, info.p1, &x, &y, &z, &count);
            ModelAddFocusPoint(body, info.p2, &x, &y, &z, &count);
        }
        if (count <= 0) {
            ModelSetStatus("Focus refused: bad line");
            return;
        }
        ModelTargetX = (S32)(x / count);
        ModelTargetY = (S32)(y / count);
        ModelTargetZ = (S32)(z / count);
        ModelFocusZoom(5000);
        ModelCameraMode = MODEL_CAMERA_ORBIT;
        return;
    }

    if (ModelSelectMode == MODEL_SELECT_BONE) {
        if (!ModelBoneSelection OR ModelSelectedBoneCount <= 0) {
            ModelSetStatus("Focus refused: no bone selected");
            return;
        }
        for (i = 0; i < body->NbGroupes AND i < ModelBoneSelectionCapacity; i++) {
            S32 point;

            if (!ModelBoneSelection[i])
                continue;
            point = ModelBoneOriginPoint(body, i);
            if (point >= 0)
                ModelAddFocusPoint(body, (U16)point, &x, &y, &z, &count);
        }
        if (count <= 0) {
            ModelSetStatus("Focus refused: bad bone");
            return;
        }
        ModelTargetX = (S32)(x / count);
        ModelTargetY = (S32)(y / count);
        ModelTargetZ = (S32)(z / count);
        ModelFocusZoom(5000);
        ModelCameraMode = MODEL_CAMERA_ORBIT;
        return;
    }

    if (!ModelFaceSelection OR ModelSelectedFaceCount <= 0) {
        ModelSetStatus("Focus refused: no face selected");
        return;
    }
    for (i = 0; i < body->NbPolys AND i < ModelFaceSelectionCapacity; i++) {
        ModelFaceInfo info;
        S32 p;

        if (!ModelFaceSelection[i])
            continue;
        if (!ModelFindFaceByIndex(body, i, &info))
            continue;
        for (p = 0; p < info.point_count; p++) {
            U16 point = info.points[p];

            if (point >= body->NbPoints)
                continue;
            ModelAddFocusPoint(body, point, &x, &y, &z, &count);
        }
    }
    if (count <= 0) {
        ModelSetStatus("Focus refused: bad selection");
        return;
    }
    ModelTargetX = (S32)(x / count);
    ModelTargetY = (S32)(y / count);
    ModelTargetZ = (S32)(z / count);
    ModelFocusZoom(5000);
    ModelCameraMode = MODEL_CAMERA_ORBIT;
}

static S32 ModelFindVertexAt(T_BODY_HEADER *body, S32 x, S32 y) {
    S32 i;
    S32 best = -1;
    S32 best_dist = 9 * 9;

    if (!body)
        return -1;
    for (i = 0; i < body->NbPoints AND i < MODEL_MAX_POINTS; i++) {
        S32 dx = Obj_ListProjectedPoints[i].X - x;
        S32 dy = Obj_ListProjectedPoints[i].Y - y;
        S32 dist = dx * dx + dy * dy;

        if (dist <= best_dist) {
            best = i;
            best_dist = dist;
        }
    }
    return best;
}

static void ModelBoxBounds(S32 *x0, S32 *y0, S32 *x1, S32 *y1) {
    *x0 = ModelBoxX0 < ModelBoxX1 ? ModelBoxX0 : ModelBoxX1;
    *x1 = ModelBoxX0 < ModelBoxX1 ? ModelBoxX1 : ModelBoxX0;
    *y0 = ModelBoxY0 < ModelBoxY1 ? ModelBoxY0 : ModelBoxY1;
    *y1 = ModelBoxY0 < ModelBoxY1 ? ModelBoxY1 : ModelBoxY0;
}

static S32 ModelPointInBox(S32 x, S32 y, S32 x0, S32 y0, S32 x1, S32 y1) {
    return (x >= x0 AND x <= x1 AND y >= y0 AND y <= y1) ? TRUE : FALSE;
}

static S32 ModelFaceIntersectsBox(const ModelFaceInfo *info, T_BODY_HEADER *body, S32 x0, S32 y0, S32 x1, S32 y1) {
    S32 i;
    S32 cx = 0;
    S32 cy = 0;

    if (!ModelFaceValid(info, body))
        return FALSE;
    for (i = 0; i < info->point_count; i++) {
        S32 x = Obj_ListProjectedPoints[info->points[i]].X;
        S32 y = Obj_ListProjectedPoints[info->points[i]].Y;

        if (ModelPointInBox(x, y, x0, y0, x1, y1))
            return TRUE;
        cx += x;
        cy += y;
    }
    cx /= info->point_count;
    cy /= info->point_count;
    return ModelPointInBox(cx, cy, x0, y0, x1, y1);
}

static S32 ModelLineIntersectsBox(const ModelLineInfo *info, S32 x0, S32 y0, S32 x1, S32 y1) {
    S32 ax = Obj_ListProjectedPoints[info->p1].X;
    S32 ay = Obj_ListProjectedPoints[info->p1].Y;
    S32 bx = Obj_ListProjectedPoints[info->p2].X;
    S32 by = Obj_ListProjectedPoints[info->p2].Y;

    if (ModelPointInBox(ax, ay, x0, y0, x1, y1) OR ModelPointInBox(bx, by, x0, y0, x1, y1))
        return TRUE;
    if (ax > bx) {
        S32 tmp = ax;
        ax = bx;
        bx = tmp;
    }
    if (ay > by) {
        S32 tmp = ay;
        ay = by;
        by = tmp;
    }
    return !(bx < x0 OR ax > x1 OR by < y0 OR ay > y1);
}

static void ModelApplyBoxSelection(T_BODY_HEADER *body, S32 add_to_selection) {
    S32 x0;
    S32 y0;
    S32 x1;
    S32 y1;
    S32 i;

    if (!body)
        return;
    ModelBoxBounds(&x0, &y0, &x1, &y1);
    if (ModelSelectActive(MODEL_SELECT_VERTEX)) {
        if (!add_to_selection)
            ModelClearVertexSelection();
        for (i = 0; i < body->NbPoints AND i < MODEL_MAX_POINTS AND i < ModelVertexSelectionCapacity; i++) {
            if (ModelPointInBox(Obj_ListProjectedPoints[i].X, Obj_ListProjectedPoints[i].Y, x0, y0, x1, y1) AND !ModelVertexSelection[i]) {
                ModelVertexSelection[i] = 1;
                ModelSelectedVertexCount++;
                ModelSelectedVertex = i;
            }
        }
    }
    if (ModelSelectActive(MODEL_SELECT_FACE)) {
        if (!add_to_selection)
            ModelClearFaceSelection();
        for (i = 0; i < body->NbPolys AND i < ModelFaceSelectionCapacity; i++) {
            ModelFaceInfo info;

            if (!ModelFindFaceByIndex(body, i, &info))
                continue;
            if (!ModelFaceVisibleInOverlay(&info, body) OR !ModelFaceIntersectsBox(&info, body, x0, y0, x1, y1) OR ModelFaceSelection[i])
                continue;
            ModelFaceSelection[i] = 1;
            ModelSelectedFaceCount++;
            ModelSelectedFace = i;
            ModelSelectedFaceInfo = info;
        }
    }
    if (ModelSelectActive(MODEL_SELECT_SPHERE)) {
        if (!add_to_selection)
            ModelClearSphereSelection();
        for (i = 0; i < body->NbSpheres AND i < ModelSphereSelectionCapacity; i++) {
            ModelSphereInfo info;

            if (!ModelReadSphereInfo(body, i, &info))
                continue;
            if (!ModelPointInBox(Obj_ListProjectedPoints[info.point].X, Obj_ListProjectedPoints[info.point].Y, x0, y0, x1, y1) OR ModelSphereSelection[i])
                continue;
            ModelSphereSelection[i] = 1;
            ModelSelectedSphereCount++;
            ModelSelectedSphere = i;
            ModelSelectedSphereInfo = info;
        }
    }
    if (ModelSelectActive(MODEL_SELECT_LINE)) {
        if (!add_to_selection)
            ModelClearLineSelection();
        for (i = 0; i < body->NbLines AND i < ModelLineSelectionCapacity; i++) {
            ModelLineInfo info;

            if (!ModelReadLineInfo(body, i, &info))
                continue;
            if (!ModelLineIntersectsBox(&info, x0, y0, x1, y1) OR ModelLineSelection[i])
                continue;
            ModelLineSelection[i] = 1;
            ModelSelectedLineCount++;
            ModelSelectedLine = i;
            ModelSelectedLineInfo = info;
        }
    }
    if (ModelSelectActive(MODEL_SELECT_BONE)) {
        if (!add_to_selection)
            ModelClearBoneSelection();
        for (i = 0; i < body->NbGroupes AND i < ModelBoneSelectionCapacity; i++) {
            S32 point = ModelBoneOriginPoint(body, i);

            if (point < 0)
                continue;
            if (!ModelPointInBox(Obj_ListProjectedPoints[point].X, Obj_ListProjectedPoints[point].Y, x0, y0, x1, y1) OR ModelBoneSelection[i])
                continue;
            ModelBoneSelection[i] = 1;
            ModelSelectedBoneCount++;
            ModelSelectedBone = i;
        }
    }
}
