static void ModelSetStatus(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(ModelStatusText, sizeof(ModelStatusText), fmt, ap);
    va_end(ap);
    ModelStatusUntil = TimerRefHR + 2500;
}

static S32 ModelSelectBit(S32 mode) {
    return 1 << mode;
}

static S32 ModelSelectActive(S32 mode) {
    return (ModelSelectMask & ModelSelectBit(mode)) ? TRUE : FALSE;
}

static S32 ModelSelectActiveCount(void) {
    S32 count = 0;

    if (ModelSelectActive(MODEL_SELECT_VERTEX))
        count++;
    if (ModelSelectActive(MODEL_SELECT_FACE))
        count++;
    if (ModelSelectActive(MODEL_SELECT_SPHERE))
        count++;
    if (ModelSelectActive(MODEL_SELECT_LINE))
        count++;
    if (ModelSelectActive(MODEL_SELECT_BONE))
        count++;
    return count;
}

static S32 ModelSelectIsMultiType(void) {
    return ModelSelectActiveCount() > 1 ? TRUE : FALSE;
}

static void ModelSetSelectMode(S32 mode, S32 add_to_existing) {
    S32 bit = ModelSelectBit(mode);

    if (!add_to_existing) {
        ModelSelectMask = bit;
    } else {
        if (mode == MODEL_SELECT_VERTEX)
            ModelSelectMask &= ~MODEL_SELECT_MASK_FACE;
        if (mode == MODEL_SELECT_FACE)
            ModelSelectMask &= ~MODEL_SELECT_MASK_VERTEX;
        ModelSelectMask |= bit;
    }
    ModelSelectMode = mode;
    if (ModelSelectMask == 0)
        ModelSelectMask = bit;
}

static void ModelTrimLine(char *text) {
    S32 len = (S32)strlen(text);

    while (len > 0 AND(text[len - 1] == '\n' OR text[len - 1] == '\r' OR text[len - 1] == ' ' OR text[len - 1] == '\t')) {
        text[len - 1] = '\0';
        len--;
    }
}

static S32 ModelReadOutputRoot(char *outPath, S32 pathMax) {
    char pref[ADELINE_MAX_PATH];
    char path[ADELINE_MAX_PATH];
    FILE *f;

    GetDefaultUserDir(pref);
    if (pref[0] == '\0') {
        ModelSetStatus("Save refused: app-support path unavailable");
        return FALSE;
    }

    snprintf(path, sizeof(path), "%seditor_output_dir.txt", pref);
    f = fopen(path, "rb");
    if (!f) {
        ModelSetStatus("Save refused: editor_output_dir.txt missing");
        return FALSE;
    }
    if (!fgets(outPath, pathMax, f)) {
        fclose(f);
        ModelSetStatus("Save refused: editor_output_dir.txt is empty");
        return FALSE;
    }
    fclose(f);

    outPath[pathMax - 1] = '\0';
    ModelTrimLine(outPath);
    if (outPath[0] == '\0') {
        ModelSetStatus("Save refused: editor_output_dir.txt is empty");
        return FALSE;
    }
    if (!IsDirectory(outPath)) {
        ModelSetStatus("Save refused: output dir missing");
        return FALSE;
    }
    return TRUE;
}

static S32 ModelStatePath(char *outPath, S32 pathMax) {
    char pref[ADELINE_MAX_PATH];

    GetDefaultUserDir(pref);
    if (pref[0] == '\0')
        return FALSE;
    snprintf(outPath, pathMax, "%s%s", pref, MODEL_STATE_FILE);
    return TRUE;
}

static void ModelSaveState(void) {
    char path[ADELINE_MAX_PATH];
    FILE *f;

    ModelStoreBodyIdForArchive();
    if (!ModelStatePath(path, sizeof(path)))
        return;
    f = fopen(path, "wb");
    if (!f)
        return;
    fprintf(f, "archive %d\n", (int)ModelArchive);
    fprintf(f, "body %d\n", (int)ModelBodyArchiveId);
    fprintf(f, "objfix %d\n", (int)ModelObjFixArchiveId);
    fprintf(f, "floor_grid %d\n", (int)ModelFloorGrid);
    fclose(f);
}

static void ModelLoadState(void) {
    char path[ADELINE_MAX_PATH];
    char key[32];
    int value;
    FILE *f;

    if (!ModelStatePath(path, sizeof(path)))
        return;
    f = fopen(path, "rb");
    if (!f)
        return;
    while (fscanf(f, "%31s %d", key, (int *)&value) == 2) {
        if (!strcmp(key, "archive"))
            ModelArchive = value;
        else if (!strcmp(key, "body"))
            ModelBodyArchiveId = value;
        else if (!strcmp(key, "objfix"))
            ModelObjFixArchiveId = value;
        else if (!strcmp(key, "floor_grid"))
            ModelFloorGrid = value ? TRUE : FALSE;
    }
    fclose(f);

    if (ModelArchive < 0 OR ModelArchive >= MODEL_ARCHIVE_COUNT OR !ModelArchiveAvailable(ModelArchive))
        ModelArchive = MODEL_ARCHIVE_BODY;
    ModelSyncBodyIdFromArchive();
    ModelClampBodyId();
}

static void ModelJoinPath(char *outPath, S32 outMax, const char *root, const char *file) {
    S32 len = (S32)strlen(root);

    if (len > 0 AND(root[len - 1] == '/' OR root[len - 1] == '\\'))
        snprintf(outPath, outMax, "%s%s", root, file);
    else
        snprintf(outPath, outMax, "%s/%s", root, file);
}

static S32 ModelArchiveBase(S32 archive) {
    if (archive == MODEL_ARCHIVE_BODY_MOD)
        return MODEL_ARCHIVE_BODY;
    if (archive == MODEL_ARCHIVE_OBJFIX_MOD)
        return MODEL_ARCHIVE_OBJFIX;
    return archive;
}

static S32 *ModelArchiveBodyIdPtr(S32 archive) {
    return ModelArchiveBase(archive) == MODEL_ARCHIVE_BODY ? &ModelBodyArchiveId : &ModelObjFixArchiveId;
}

static void ModelSyncBodyIdFromArchive(void) {
    ModelBodyId = *ModelArchiveBodyIdPtr(ModelArchive);
}

static void ModelStoreBodyIdForArchive(void) {
    *ModelArchiveBodyIdPtr(ModelArchive) = ModelBodyId;
}

static S32 ModelArchiveOrderIndex(S32 archive) {
    static const S32 order[MODEL_ARCHIVE_COUNT] = {
        MODEL_ARCHIVE_BODY,
        MODEL_ARCHIVE_BODY_MOD,
        MODEL_ARCHIVE_OBJFIX,
        MODEL_ARCHIVE_OBJFIX_MOD
    };
    S32 i;

    for (i = 0; i < MODEL_ARCHIVE_COUNT; i++) {
        if (order[i] == archive)
            return i;
    }
    return 0;
}

static S32 ModelArchiveFromOrderIndex(S32 index) {
    static const S32 order[MODEL_ARCHIVE_COUNT] = {
        MODEL_ARCHIVE_BODY,
        MODEL_ARCHIVE_BODY_MOD,
        MODEL_ARCHIVE_OBJFIX,
        MODEL_ARCHIVE_OBJFIX_MOD
    };

    if (index < 0)
        index += MODEL_ARCHIVE_COUNT;
    if (index >= MODEL_ARCHIVE_COUNT)
        index -= MODEL_ARCHIVE_COUNT;
    return order[index];
}

static void ModelLooseFilename(S32 archive, S32 body_id, char *outName, S32 outMax) {
    if (ModelArchiveBase(archive) == MODEL_ARCHIVE_BODY)
        snprintf(outName, outMax, "MOD_BODY_%d.O3D", (int)body_id);
    else
        snprintf(outName, outMax, "MOD_OBJFIX_%d.O3D", (int)body_id);
}

static S32 ModelLoosePath(S32 archive, S32 body_id, char *outPath, S32 outMax, S32 show_errors) {
    char root[ADELINE_MAX_PATH];
    char file[64];

    if (!ModelReadOutputRoot(root, sizeof(root))) {
        if (!show_errors)
            ModelStatusUntil = 0;
        return FALSE;
    }
    ModelLooseFilename(archive, body_id, file, sizeof(file));
    ModelJoinPath(outPath, outMax, root, file);
    return TRUE;
}

static const char *ModelArchiveName(void) {
    if (ModelArchive == MODEL_ARCHIVE_BODY)
        return "BODY.HQR";
    if (ModelArchive == MODEL_ARCHIVE_OBJFIX)
        return "OBJFIX.HQR";
    if (ModelArchive == MODEL_ARCHIVE_BODY_MOD)
        return "BODY.HQR (Mod)";
    return "OBJFIX.HQR (Mod)";
}

static T_HQR_HEADER *ModelArchiveHeader(void) {
    return ModelArchiveBase(ModelArchive) == MODEL_ARCHIVE_BODY ? HQR_Bodys : HQR_ObjFix;
}

static S32 ModelArchiveCount(void) {
    return ModelArchiveBase(ModelArchive) == MODEL_ARCHIVE_BODY ? ModelBodyCount : ModelObjFixCount;
}

static S32 ModelArchiveAvailable(S32 archive) {
    if (archive == MODEL_ARCHIVE_BODY)
        return ModelBodyCount > 0 ? TRUE : FALSE;
    if (archive == MODEL_ARCHIVE_OBJFIX)
        return ModelObjFixCount > 0 ? TRUE : FALSE;
    if (archive == MODEL_ARCHIVE_BODY_MOD)
        return ModelModBodyAvailable;
    if (archive == MODEL_ARCHIVE_OBJFIX_MOD)
        return ModelModObjFixAvailable;
    return FALSE;
}

static void ModelScanModArchives(void) {
    char root[ADELINE_MAX_PATH];
    char path[ADELINE_MAX_PATH];
    char file[64];
    S32 i;

    ModelModBodyAvailable = FALSE;
    ModelModObjFixAvailable = FALSE;
    if (!ModelReadOutputRoot(root, sizeof(root))) {
        ModelStatusUntil = 0;
        return;
    }
    for (i = 0; i < ModelBodyCount; i++) {
        ModelLooseFilename(MODEL_ARCHIVE_BODY, i, file, sizeof(file));
        ModelJoinPath(path, sizeof(path), root, file);
        if (FileSize(path) > 0) {
            ModelModBodyAvailable = TRUE;
            break;
        }
    }
    for (i = 0; i < ModelObjFixCount; i++) {
        ModelLooseFilename(MODEL_ARCHIVE_OBJFIX, i, file, sizeof(file));
        ModelJoinPath(path, sizeof(path), root, file);
        if (FileSize(path) > 0) {
            ModelModObjFixAvailable = TRUE;
            break;
        }
    }
}

static void ModelClearFaceSelection(void) {
    if (ModelFaceSelection AND ModelFaceSelectionCapacity > 0)
        memset(ModelFaceSelection, 0, ModelFaceSelectionCapacity);
    ModelSelectedFaceCount = 0;
    ModelSelectedFace = -1;
    ModelClearFaceInfo(&ModelSelectedFaceInfo);
}

static void ModelClearVertexSelection(void) {
    if (ModelVertexSelection AND ModelVertexSelectionCapacity > 0)
        memset(ModelVertexSelection, 0, ModelVertexSelectionCapacity);
    ModelSelectedVertexCount = 0;
    ModelSelectedVertex = -1;
}

static void ModelClearSphereInfo(ModelSphereInfo *info) {
    memset(info, 0, sizeof(*info));
    info->index = -1;
}

static void ModelClearSphereSelection(void) {
    if (ModelSphereSelection AND ModelSphereSelectionCapacity > 0)
        memset(ModelSphereSelection, 0, ModelSphereSelectionCapacity);
    ModelSelectedSphereCount = 0;
    ModelSelectedSphere = -1;
    ModelClearSphereInfo(&ModelSelectedSphereInfo);
}

static void ModelClearLineInfo(ModelLineInfo *info) {
    memset(info, 0, sizeof(*info));
    info->index = -1;
}

static void ModelClearLineSelection(void) {
    if (ModelLineSelection AND ModelLineSelectionCapacity > 0)
        memset(ModelLineSelection, 0, ModelLineSelectionCapacity);
    ModelSelectedLineCount = 0;
    ModelSelectedLine = -1;
    ModelClearLineInfo(&ModelSelectedLineInfo);
}

static void ModelClearBoneSelection(void) {
    if (ModelBoneSelection AND ModelBoneSelectionCapacity > 0)
        memset(ModelBoneSelection, 0, ModelBoneSelectionCapacity);
    ModelSelectedBoneCount = 0;
    ModelSelectedBone = -1;
}

static S32 ModelEnsureFaceSelection(T_BODY_HEADER *body) {
    S32 count = body ? body->NbPolys : 0;

    if (count <= 0) {
        ModelClearFaceSelection();
        return FALSE;
    }
    if (ModelFaceSelectionCapacity < count) {
        if (ModelFaceSelection)
            Free(ModelFaceSelection);
        ModelFaceSelection = (U8 *)Malloc(count);
        if (!ModelFaceSelection) {
            ModelFaceSelectionCapacity = 0;
            ModelSelectedFaceCount = 0;
            return FALSE;
        }
        ModelFaceSelectionCapacity = count;
    }
    memset(ModelFaceSelection, 0, count);
    ModelSelectedFaceCount = 0;
    return TRUE;
}

static S32 ModelEnsureVertexSelection(T_BODY_HEADER *body) {
    S32 count = body ? body->NbPoints : 0;

    if (count <= 0) {
        ModelClearVertexSelection();
        return FALSE;
    }
    if (ModelVertexSelectionCapacity < count) {
        if (ModelVertexSelection)
            Free(ModelVertexSelection);
        ModelVertexSelection = (U8 *)Malloc(count);
        if (!ModelVertexSelection) {
            ModelVertexSelectionCapacity = 0;
            ModelSelectedVertexCount = 0;
            return FALSE;
        }
        ModelVertexSelectionCapacity = count;
    }
    memset(ModelVertexSelection, 0, count);
    ModelSelectedVertexCount = 0;
    return TRUE;
}

static S32 ModelEnsureSphereSelection(T_BODY_HEADER *body) {
    S32 count = body ? body->NbSpheres : 0;

    if (count <= 0) {
        ModelClearSphereSelection();
        return FALSE;
    }
    if (ModelSphereSelectionCapacity < count) {
        if (ModelSphereSelection)
            Free(ModelSphereSelection);
        ModelSphereSelection = (U8 *)Malloc(count);
        if (!ModelSphereSelection) {
            ModelSphereSelectionCapacity = 0;
            ModelSelectedSphereCount = 0;
            return FALSE;
        }
        ModelSphereSelectionCapacity = count;
    }
    memset(ModelSphereSelection, 0, count);
    ModelSelectedSphereCount = 0;
    return TRUE;
}

static S32 ModelEnsureLineSelection(T_BODY_HEADER *body) {
    S32 count = body ? body->NbLines : 0;

    if (count <= 0) {
        ModelClearLineSelection();
        return FALSE;
    }
    if (ModelLineSelectionCapacity < count) {
        if (ModelLineSelection)
            Free(ModelLineSelection);
        ModelLineSelection = (U8 *)Malloc(count);
        if (!ModelLineSelection) {
            ModelLineSelectionCapacity = 0;
            ModelSelectedLineCount = 0;
            return FALSE;
        }
        ModelLineSelectionCapacity = count;
    }
    memset(ModelLineSelection, 0, count);
    ModelSelectedLineCount = 0;
    return TRUE;
}

static S32 ModelEnsureBoneSelection(T_BODY_HEADER *body) {
    S32 count = body ? body->NbGroupes : 0;

    if (count <= 0) {
        ModelClearBoneSelection();
        return FALSE;
    }
    if (ModelBoneSelectionCapacity < count) {
        if (ModelBoneSelection)
            Free(ModelBoneSelection);
        ModelBoneSelection = (U8 *)Malloc(count);
        if (!ModelBoneSelection) {
            ModelBoneSelectionCapacity = 0;
            ModelSelectedBoneCount = 0;
            return FALSE;
        }
        ModelBoneSelectionCapacity = count;
    }
    memset(ModelBoneSelection, 0, count);
    ModelSelectedBoneCount = 0;
    return TRUE;
}

static S32 ModelFaceSelected(S32 index) {
    if (!ModelFaceSelection OR index < 0 OR index >= ModelFaceSelectionCapacity)
        return FALSE;
    return ModelFaceSelection[index] ? TRUE : FALSE;
}

static S32 ModelVertexSelected(S32 index) {
    if (!ModelVertexSelection OR index < 0 OR index >= ModelVertexSelectionCapacity)
        return FALSE;
    return ModelVertexSelection[index] ? TRUE : FALSE;
}

static S32 ModelSphereSelected(S32 index) {
    if (!ModelSphereSelection OR index < 0 OR index >= ModelSphereSelectionCapacity)
        return FALSE;
    return ModelSphereSelection[index] ? TRUE : FALSE;
}

static S32 ModelLineSelected(S32 index) {
    if (!ModelLineSelection OR index < 0 OR index >= ModelLineSelectionCapacity)
        return FALSE;
    return ModelLineSelection[index] ? TRUE : FALSE;
}

static S32 ModelBoneSelected(S32 index) {
    if (!ModelBoneSelection OR index < 0 OR index >= ModelBoneSelectionCapacity)
        return FALSE;
    return ModelBoneSelection[index] ? TRUE : FALSE;
}

static S32 ModelLineValid(const ModelLineInfo *info, T_BODY_HEADER *body) {
    if (!body OR !info)
        return FALSE;
    if (info->index < 0 OR info->index >= body->NbLines)
        return FALSE;
    if (info->p1 >= body->NbPoints OR info->p2 >= body->NbPoints)
        return FALSE;
    if (info->p1 >= MODEL_MAX_POINTS OR info->p2 >= MODEL_MAX_POINTS)
        return FALSE;
    return TRUE;
}

static S32 ModelReadLineInfo(T_BODY_HEADER *body, S32 index, ModelLineInfo *out_info) {
    ModelLineDisk *line;

    ModelClearLineInfo(out_info);
    if (!body OR index < 0 OR index >= body->NbLines)
        return FALSE;
    line = (ModelLineDisk *)((U8 *)body + body->OffLines);
    line += index;
    out_info->index = index;
    out_info->offset = body->OffLines + index * (S32)sizeof(ModelLineDisk);
    out_info->type = line->Type;
    out_info->color = line->Coul;
    out_info->p1 = line->P1;
    out_info->p2 = line->P2;
    if (!ModelLineValid(out_info, body)) {
        ModelClearLineInfo(out_info);
        return FALSE;
    }
    return TRUE;
}

static S64 ModelDistanceToSegmentSquared(S32 px, S32 py, S32 ax, S32 ay, S32 bx, S32 by) {
    S64 vx = bx - ax;
    S64 vy = by - ay;
    S64 wx = px - ax;
    S64 wy = py - ay;
    S64 len2 = vx * vx + vy * vy;
    S64 t;
    S64 cx;
    S64 cy;
    S64 dx;
    S64 dy;

    if (len2 <= 0) {
        dx = px - ax;
        dy = py - ay;
        return dx * dx + dy * dy;
    }
    t = (wx * vx + wy * vy) * 1024 / len2;
    if (t < 0)
        t = 0;
    if (t > 1024)
        t = 1024;
    cx = ax + vx * t / 1024;
    cy = ay + vy * t / 1024;
    dx = px - cx;
    dy = py - cy;
    return dx * dx + dy * dy;
}

static S32 ModelFindLineAt(T_BODY_HEADER *body, S32 x, S32 y, ModelLineInfo *out_info) {
    S32 i;
    S32 best = -1;
    S64 best_dist = 7 * 7;

    ModelClearLineInfo(out_info);
    if (!body)
        return -1;
    for (i = 0; i < body->NbLines; i++) {
        ModelLineInfo info;
        S64 dist;

        if (!ModelReadLineInfo(body, i, &info))
            continue;
        dist = ModelDistanceToSegmentSquared(x, y,
                                             Obj_ListProjectedPoints[info.p1].X,
                                             Obj_ListProjectedPoints[info.p1].Y,
                                             Obj_ListProjectedPoints[info.p2].X,
                                             Obj_ListProjectedPoints[info.p2].Y);
        if (dist <= best_dist) {
            best = i;
            best_dist = dist;
            *out_info = info;
        }
    }
    return best;
}

static void ModelWriteLineColor(T_BODY_HEADER *body, const ModelLineInfo *info, S32 value) {
    ModelLineDisk *line;

    if (!ModelLineValid(info, body))
        return;
    value &= 0xff;
    line = (ModelLineDisk *)((U8 *)body + info->offset);
    line->Coul = (U16)((line->Coul & 0xff00) | value);
}

static void ModelWriteLinePoint(T_BODY_HEADER *body, const ModelLineInfo *info, S32 target, S32 value) {
    ModelLineDisk *line;

    if (!ModelLineValid(info, body))
        return;
    value = ModelClampEditValue(body, target, value);
    line = (ModelLineDisk *)((U8 *)body + info->offset);
    if (target == MODEL_NUMBER_LINE_P1)
        line->P1 = (U16)value;
    else if (target == MODEL_NUMBER_LINE_P2)
        line->P2 = (U16)value;
}

static S32 ModelSphereValid(const ModelSphereInfo *info, T_BODY_HEADER *body) {
    if (!body OR !info)
        return FALSE;
    if (info->index < 0 OR info->index >= body->NbSpheres)
        return FALSE;
    if (info->point >= body->NbPoints)
        return FALSE;
    if (info->point >= MODEL_MAX_POINTS)
        return FALSE;
    return TRUE;
}

static S32 ModelReadSphereInfo(T_BODY_HEADER *body, S32 index, ModelSphereInfo *out_info) {
    ModelSphereDisk *sphere;

    ModelClearSphereInfo(out_info);
    if (!body OR index < 0 OR index >= body->NbSpheres)
        return FALSE;
    sphere = (ModelSphereDisk *)((U8 *)body + body->OffSpheres);
    sphere += index;
    out_info->index = index;
    out_info->offset = body->OffSpheres + index * (S32)sizeof(ModelSphereDisk);
    out_info->type = sphere->Type;
    out_info->color = sphere->Coul;
    out_info->point = sphere->P1;
    out_info->radius = sphere->Rayon;
    if (!ModelSphereValid(out_info, body)) {
        ModelClearSphereInfo(out_info);
        return FALSE;
    }
    return TRUE;
}

static S32 ModelSpherePickRadius(const ModelSphereInfo *info) {
    S32 radius;

    if (!info)
        return 8;
    radius = info->radius / 16;
    if (radius < 8)
        radius = 8;
    if (radius > 22)
        radius = 22;
    return radius;
}

static S32 ModelFindSphereAt(T_BODY_HEADER *body, S32 x, S32 y, ModelSphereInfo *out_info) {
    S32 i;
    S32 best = -1;
    S32 best_dist = 0x7fffffff;

    ModelClearSphereInfo(out_info);
    if (!body)
        return -1;
    for (i = 0; i < body->NbSpheres; i++) {
        ModelSphereInfo info;
        S32 radius;
        S32 dx;
        S32 dy;
        S32 dist;

        if (!ModelReadSphereInfo(body, i, &info))
            continue;
        radius = ModelSpherePickRadius(&info);
        dx = Obj_ListProjectedPoints[info.point].X - x;
        dy = Obj_ListProjectedPoints[info.point].Y - y;
        dist = dx * dx + dy * dy;
        if (dist <= radius * radius AND dist < best_dist) {
            best = i;
            best_dist = dist;
            *out_info = info;
        }
    }
    return best;
}

static void ModelWriteSphereColor(T_BODY_HEADER *body, const ModelSphereInfo *info, S32 value) {
    ModelSphereDisk *sphere;

    if (!ModelSphereValid(info, body))
        return;
    value &= 0xff;
    sphere = (ModelSphereDisk *)((U8 *)body + info->offset);
    sphere->Coul = (U16)((sphere->Coul & 0xff00) | value);
}

static void ModelApplySphereColor(T_BODY_HEADER *body, S32 value) {
    S32 i;

    if (!body OR !ModelSphereSelection)
        return;
    for (i = 0; i < body->NbSpheres AND i < ModelSphereSelectionCapacity; i++) {
        ModelSphereInfo info;

        if (!ModelSphereSelection[i])
            continue;
        if (ModelReadSphereInfo(body, i, &info))
            ModelWriteSphereColor(body, &info, value);
    }
    if (ModelSelectedSphere >= 0)
        ModelReadSphereInfo(body, ModelSelectedSphere, &ModelSelectedSphereInfo);
}

static void ModelApplyLineColor(T_BODY_HEADER *body, S32 value) {
    S32 i;

    if (!body OR !ModelLineSelection)
        return;
    for (i = 0; i < body->NbLines AND i < ModelLineSelectionCapacity; i++) {
        ModelLineInfo info;

        if (!ModelLineSelection[i])
            continue;
        if (ModelReadLineInfo(body, i, &info))
            ModelWriteLineColor(body, &info, value);
    }
    if (ModelSelectedLine >= 0)
        ModelReadLineInfo(body, ModelSelectedLine, &ModelSelectedLineInfo);
}

static void ModelApplyLinePoint(T_BODY_HEADER *body, S32 target, S32 value) {
    S32 i;

    if (!body OR !ModelLineSelection)
        return;
    for (i = 0; i < body->NbLines AND i < ModelLineSelectionCapacity; i++) {
        ModelLineInfo info;

        if (!ModelLineSelection[i])
            continue;
        if (ModelReadLineInfo(body, i, &info))
            ModelWriteLinePoint(body, &info, target, value);
    }
    if (ModelSelectedLine >= 0)
        ModelReadLineInfo(body, ModelSelectedLine, &ModelSelectedLineInfo);
}

static T_BODY_HEADER *ModelLoadLooseBody(S32 archive, S32 body_id, U32 *out_size) {
    char path[ADELINE_MAX_PATH];
    U32 size;
    FILE *f;
    U8 *data;

    *out_size = 0;
    if (!ModelLoosePath(archive, body_id, path, sizeof(path), FALSE))
        return NULL;
    size = FileSize(path);
    if (size == 0)
        return NULL;
    data = (U8 *)Malloc(size + RECOVER_AREA);
    if (!data)
        return NULL;
    f = fopen(path, "rb");
    if (!f) {
        Free(data);
        return NULL;
    }
    if (fread(data, 1, size, f) != size) {
        fclose(f);
        Free(data);
        return NULL;
    }
    fclose(f);
    *out_size = size;
    return (T_BODY_HEADER *)data;
}

static T_BODY_HEADER *ModelGetBody(void) {
    T_HQR_HEADER *hqr = ModelArchiveHeader();
    T_BODY_HEADER *body;
    U32 body_size = 0;

    if (!hqr)
        return NULL;
    if (ModelBodyId < 0 OR ModelBodyId >= ModelArchiveCount())
        return NULL;
    if (ModelLoadedBody AND ModelLoadedArchive == ModelArchive AND ModelLoadedBodyId == ModelBodyId)
        return ModelLoadedBody;

    if (ModelLoadedBody) {
        Free(ModelLoadedBody);
        ModelLoadedBody = NULL;
        ModelLoadedBodySize = 0;
    }
    ModelResetAnimPreview();
    ModelClearVertexSelection();
    ModelClearFaceSelection();
    ModelClearSphereSelection();
    ModelClearLineSelection();
    ModelClearBoneSelection();

    body = NULL;
    if (ModelArchive == MODEL_ARCHIVE_BODY_MOD OR ModelArchive == MODEL_ARCHIVE_OBJFIX_MOD)
        body = ModelLoadLooseBody(ModelArchive, ModelBodyId, &body_size);
    if (!body) {
        body = (T_BODY_HEADER *)LoadMalloc_HQR(hqr->Name, ModelBodyId);
        body_size = LoadMallocFileSize;
    }
    if (!body) {
        ModelLoadedArchive = -1;
        ModelLoadedBodyId = -1;
        ModelLoadedBodySize = 0;
        return NULL;
    }

    ModelLoadedArchive = ModelArchive;
    ModelLoadedBodyId = ModelBodyId;
    ModelLoadedBody = body;
    ModelLoadedBodySize = body_size;
    ModelEnsureVertexSelection(body);
    ModelEnsureFaceSelection(body);
    ModelEnsureSphereSelection(body);
    ModelEnsureLineSelection(body);
    ModelEnsureBoneSelection(body);
    ModelRefreshAnimChoices(body);
    return ModelLoadedBody;
}

static S32 ModelShiftDown(void) {
    return (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? TRUE : FALSE;
}

static S32 ModelAltDown(void) {
    return (CheckKey(K_ALT) OR CheckKey(K_ALT_LEFT) OR CheckKey(K_ALT_RIGHT)) ? TRUE : FALSE;
}

static S32 ModelCtrlDown(void) {
    return (CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT)) ? TRUE : FALSE;
}

static S32 ModelSaveCurrentBody(void) {
    T_BODY_HEADER *body = ModelGetBody();
    char path[ADELINE_MAX_PATH];
    FILE *f;

    if (!body OR ModelLoadedBodySize == 0) {
        ModelSetStatus("Save refused: no model loaded");
        return FALSE;
    }
    if (!ModelLoosePath(ModelArchive, ModelBodyId, path, sizeof(path), TRUE))
        return FALSE;

    f = fopen(path, "wb");
    if (!f) {
        ModelSetStatus("Save failed: cannot write file");
        return FALSE;
    }
    if (fwrite(body, 1, ModelLoadedBodySize, f) != ModelLoadedBodySize) {
        fclose(f);
        ModelSetStatus("Save failed: write incomplete");
        return FALSE;
    }
    fclose(f);

    if (ModelArchiveBase(ModelArchive) == MODEL_ARCHIVE_BODY) {
        ModelModBodyAvailable = TRUE;
        ModelArchive = MODEL_ARCHIVE_BODY_MOD;
    } else {
        ModelModObjFixAvailable = TRUE;
        ModelArchive = MODEL_ARCHIVE_OBJFIX_MOD;
    }
    ModelLoadedArchive = ModelArchive;
    ModelSetStatus("Saved %s", path);
    return TRUE;
}

static void ModelClampBodyId(void) {
    S32 count = ModelArchiveCount();

    if (count <= 0) {
        ModelBodyId = 0;
        ModelStoreBodyIdForArchive();
        return;
    }
    if (ModelBodyId < 0)
        ModelBodyId = count - 1;
    if (ModelBodyId >= count)
        ModelBodyId = 0;
    ModelStoreBodyIdForArchive();
}

static void ModelChangeBodyId(S32 delta) {
    ModelBodyId += delta;
    ModelClampBodyId();
    ModelCancelGrab(ModelLoadedBody);
    ModelBoxSelectMode = FALSE;
    ModelBoxSelecting = FALSE;
    ModelClearVertexSelection();
    ModelClearFaceSelection();
    ModelClearSphereSelection();
    ModelClearLineSelection();
    ModelClearBoneSelection();
    ModelAnimChoiceBodyId = -1;
    ModelResetAnimPreview();
}

static void ModelChangeArchive(S32 delta) {
    S32 order_index;
    S32 i;

    ModelStoreBodyIdForArchive();
    order_index = ModelArchiveOrderIndex(ModelArchive);
    for (i = 0; i < MODEL_ARCHIVE_COUNT; i++) {
        order_index += delta;
        if (order_index >= MODEL_ARCHIVE_COUNT)
            order_index = 0;
        if (order_index < 0)
            order_index = MODEL_ARCHIVE_COUNT - 1;
        ModelArchive = ModelArchiveFromOrderIndex(order_index);
        if (ModelArchiveAvailable(ModelArchive))
            break;
    }
    ModelSyncBodyIdFromArchive();
    ModelClampBodyId();
    ModelCancelGrab(ModelLoadedBody);
    ModelBoxSelectMode = FALSE;
    ModelBoxSelecting = FALSE;
    ModelClearVertexSelection();
    ModelClearFaceSelection();
    ModelClearSphereSelection();
    ModelClearLineSelection();
    ModelClearBoneSelection();
    ModelAnimChoiceBodyId = -1;
    ModelResetAnimPreview();
}

static void ModelApplyCamera(void) {
    if (ModelCameraMode != MODEL_CAMERA_ORBIT) {
        SetProjection((S32)ModeDesiredX / 2, (S32)ModeDesiredY / 2, 128, 1024, 1024);
        SetPosCamera((S32)ModelFreeX, (S32)ModelFreeY, (S32)ModelFreeZ);
        SetAngleCamera(FreeCam_NormalizeAngle(ModelFreeAlpha), ModelFreeBeta, ModelFreeGamma);
    } else if (ModelOrtho)
        SetOrthoProjection((S32)ModeDesiredX / 2 + ModelPanX, (S32)ModeDesiredY / 2 + ModelPanY, 128);
    else
        SetProjection((S32)ModeDesiredX / 2 + ModelPanX, (S32)ModeDesiredY / 2 + ModelPanY, 128, 1024, 1024);
    if (ModelCameraMode == MODEL_CAMERA_ORBIT)
        SetFollowCamera(ModelTargetX, ModelTargetY, ModelTargetZ, ModelAlpha, ModelBeta, 0, ModelZoom);
}

static void ModelClampZoom(void) {
    if (ModelZoom < 350)
        ModelZoom = 350;
    if (ModelZoom > 50000)
        ModelZoom = 50000;
}

static void ModelAdjustZoom(S32 delta) {
    ModelCameraMode = MODEL_CAMERA_ORBIT;
    ModelZoom += delta;
    ModelClampZoom();
}

static void ModelResetCamera(void) {
    ModelAlpha = 260;
    ModelBeta = 0;
    ModelPanX = 0;
    ModelPanY = 0;
    ModelZoom = 14500;
    ModelOrtho = FALSE;
    ModelTargetX = 0;
    ModelTargetY = 0;
    ModelTargetZ = 0;
    ModelCameraMode = MODEL_CAMERA_ORBIT;
}

static void ModelSetFrontView(void) {
    ModelCameraMode = MODEL_CAMERA_ORBIT;
    ModelAlpha = 0;
    ModelBeta = 0;
}

static void ModelSetRightView(void) {
    ModelCameraMode = MODEL_CAMERA_ORBIT;
    ModelAlpha = 0;
    ModelBeta = 1024;
}

static void ModelSetTopView(void) {
    ModelCameraMode = MODEL_CAMERA_ORBIT;
    ModelAlpha = 1024;
    ModelBeta = 0;
}

static void ModelSetRelativeMouse(S32 enabled) {
    Window_SetRelativeMouseMode(enabled ? true : false);
    {
        float unused_x;
        float unused_y;

        Window_GetRelativeMouseDelta(&unused_x, &unused_y);
    }
}

static void ModelSaveCameraForFreecamCancel(void) {
    ModelSavedCameraMode = ModelCameraMode;
    ModelSavedAlpha = ModelAlpha;
    ModelSavedBeta = ModelBeta;
    ModelSavedPanX = ModelPanX;
    ModelSavedPanY = ModelPanY;
    ModelSavedZoom = ModelZoom;
    ModelSavedOrtho = ModelOrtho;
    ModelSavedFreeX = ModelFreeX;
    ModelSavedFreeY = ModelFreeY;
    ModelSavedFreeZ = ModelFreeZ;
    ModelSavedFreeAlpha = ModelFreeAlpha;
    ModelSavedFreeBeta = ModelFreeBeta;
    ModelSavedFreeGamma = ModelFreeGamma;
}

static void ModelRestoreCameraAfterFreecamCancel(void) {
    ModelCameraMode = ModelSavedCameraMode == MODEL_CAMERA_FREE_NAV ? MODEL_CAMERA_ORBIT : ModelSavedCameraMode;
    ModelAlpha = ModelSavedAlpha;
    ModelBeta = ModelSavedBeta;
    ModelPanX = ModelSavedPanX;
    ModelPanY = ModelSavedPanY;
    ModelZoom = ModelSavedZoom;
    ModelOrtho = ModelSavedOrtho;
    ModelFreeX = ModelSavedFreeX;
    ModelFreeY = ModelSavedFreeY;
    ModelFreeZ = ModelSavedFreeZ;
    ModelFreeAlpha = ModelSavedFreeAlpha;
    ModelFreeBeta = ModelSavedFreeBeta;
    ModelFreeGamma = ModelSavedFreeGamma;
}

static void ModelStartFreecam(void) {
    ModelApplyCamera();
    ModelSaveCameraForFreecamCancel();
    ModelFreeX = (float)CameraX;
    ModelFreeY = (float)CameraY;
    ModelFreeZ = (float)CameraZ;
    ModelFreeAlpha = FreeCam_SignedPitch(CameraAlpha);
    ModelFreeBeta = FreeCam_NormalizeAngle(CameraBeta);
    ModelFreeGamma = FreeCam_NormalizeAngle(CameraGamma);
    ModelCameraMode = MODEL_CAMERA_FREE_NAV;
    ModelFreeLastTime = TimerRefHR;
    ShowMouse(FALSE);
    ModelSetRelativeMouse(TRUE);
}

static void ModelSeedOrbitFromFreecam(void) {
    T_FREECAM_VECTOR forward;

    ModelAlpha = ModelFreeAlpha;
    if (ModelAlpha < -900)
        ModelAlpha = -900;
    if (ModelAlpha > 900)
        ModelAlpha = 900;
    ModelBeta = FreeCam_NormalizeAngle(ModelFreeBeta);
    forward = FreeCam_MovementVector(ModelAlpha, ModelBeta, 1, 0, 0);
    ModelTargetX = (S32)(ModelFreeX + forward.x * (float)ModelZoom);
    ModelTargetY = (S32)(ModelFreeY + forward.y * (float)ModelZoom);
    ModelTargetZ = (S32)(ModelFreeZ + forward.z * (float)ModelZoom);
    ModelPanX = 0;
    ModelPanY = 0;
    ModelOrtho = FALSE;
}

static void ModelCommitFreecam(void) {
    ModelSeedOrbitFromFreecam();
    ModelCameraMode = MODEL_CAMERA_FREE_LOCKED;
    ModelBlockInputUntilRelease = TRUE;
    ModelSetRelativeMouse(FALSE);
    ShowMouse(TRUE);
}

static void ModelCancelFreecam(void) {
    ModelRestoreCameraAfterFreecamCancel();
    ModelBlockInputUntilRelease = TRUE;
    ModelSetRelativeMouse(FALSE);
    ShowMouse(TRUE);
}
