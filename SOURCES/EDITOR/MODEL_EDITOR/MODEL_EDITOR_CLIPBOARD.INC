static void ModelClearClipboard(void) {
    if (ModelClip.points)
        Free(ModelClip.points);
    if (ModelClip.point_sources)
        Free(ModelClip.point_sources);
    if (ModelClip.point_selected)
        Free(ModelClip.point_selected);
    if (ModelClip.faces)
        Free(ModelClip.faces);
    if (ModelClip.lines)
        Free(ModelClip.lines);
    if (ModelClip.spheres)
        Free(ModelClip.spheres);
    if (ModelClip.bones)
        Free(ModelClip.bones);
    if (ModelClip.bone_sources)
        Free(ModelClip.bone_sources);
    memset(&ModelClip, 0, sizeof(ModelClip));
}

static void ModelClearGrabState(void) {
    if (ModelGrab.indices)
        Free(ModelGrab.indices);
    if (ModelGrab.original_points)
        Free(ModelGrab.original_points);
    memset(&ModelGrab, 0, sizeof(ModelGrab));
    ModelGrab.axis = -1;
}

static S32 ModelClipboardFindPoint(const S32 *source_points, S32 count, S32 point) {
    S32 i;

    for (i = 0; i < count; i++) {
        if (source_points[i] == point)
            return i;
    }
    return -1;
}

static S32 ModelClipboardFindPointSource(S32 point) {
    S32 i;

    if (!ModelClip.point_sources)
        return -1;
    for (i = 0; i < ModelClip.point_count; i++) {
        if (ModelClip.point_sources[i] == point)
            return i;
    }
    return -1;
}

static S32 ModelClipboardFindBoneSource(S32 bone) {
    S32 i;

    if (!ModelClip.bone_sources)
        return -1;
    for (i = 0; i < ModelClip.bone_count; i++) {
        if (ModelClip.bone_sources[i] == bone)
            return i;
    }
    return -1;
}

static S32 ModelClipboardBonePointCount(void) {
    S32 count = 0;
    S32 i;

    if (!ModelClip.bone_points_copied)
        return 0;
    for (i = 0; i < ModelClip.bone_count; i++)
        count += ModelClip.bones[i].NbPts;
    return count;
}

static S32 ModelGroupPointStart(T_BODY_HEADER *body, S32 group_index) {
    ModelGroupInfo *groups;
    S32 start = 0;
    S32 i;

    if (!body OR group_index < 0 OR group_index >= body->NbGroupes)
        return -1;
    groups = (ModelGroupInfo *)((U8 *)body + body->OffGroupes);
    for (i = 0; i < group_index; i++)
        start += groups[i].NbPts;
    return start;
}

static S32 ModelAddUniquePoint(S32 *points, S32 *count, S32 capacity, S32 point) {
    S32 i;

    if (point < 0 OR point >= capacity)
        return FALSE;
    for (i = 0; i < *count; i++) {
        if (points[i] == point)
            return FALSE;
    }
    points[*count] = point;
    (*count)++;
    return TRUE;
}

static S32 ModelCollectSelectedPointIndices(T_BODY_HEADER *body, S32 *points, S32 capacity) {
    S32 count = 0;
    S32 i;

    if (!body OR !points)
        return 0;
    if (ModelSelectActive(MODEL_SELECT_VERTEX)) {
        for (i = 0; i < body->NbPoints AND i < ModelVertexSelectionCapacity; i++) {
            if (ModelVertexSelection[i])
                ModelAddUniquePoint(points, &count, capacity, i);
        }
    }
    if (ModelSelectActive(MODEL_SELECT_FACE)) {
        for (i = 0; i < body->NbPolys AND i < ModelFaceSelectionCapacity; i++) {
            ModelFaceInfo info;
            S32 p;

            if (!ModelFaceSelection[i])
                continue;
            if (!ModelFindFaceByIndex(body, i, &info))
                continue;
            for (p = 0; p < info.point_count; p++)
                ModelAddUniquePoint(points, &count, capacity, info.points[p]);
        }
    }
    if (ModelSelectActive(MODEL_SELECT_LINE)) {
        for (i = 0; i < body->NbLines AND i < ModelLineSelectionCapacity; i++) {
            ModelLineInfo info;

            if (!ModelLineSelection[i])
                continue;
            if (!ModelReadLineInfo(body, i, &info))
                continue;
            ModelAddUniquePoint(points, &count, capacity, info.p1);
            ModelAddUniquePoint(points, &count, capacity, info.p2);
        }
    }
    if (ModelSelectActive(MODEL_SELECT_SPHERE)) {
        for (i = 0; i < body->NbSpheres AND i < ModelSphereSelectionCapacity; i++) {
            ModelSphereInfo info;

            if (!ModelSphereSelection[i])
                continue;
            if (!ModelReadSphereInfo(body, i, &info))
                continue;
            ModelAddUniquePoint(points, &count, capacity, info.point);
        }
    }
    return count;
}

static S32 ModelClipboardAddPointEx(T_BODY_HEADER *body, S32 *source_points, S32 *count, S32 point, S32 keep_local) {
    T_OBJ_POINT *points;
    S32 existing;

    if (!body OR point < 0 OR point >= body->NbPoints)
        return -1;
    existing = ModelClipboardFindPoint(source_points, *count, point);
    if (existing >= 0)
        return existing;
    points = (T_OBJ_POINT *)((U8 *)body + body->OffPoints);
    source_points[*count] = point;
    if (ModelClip.point_sources)
        ModelClip.point_sources[*count] = point;
    ModelClip.points[*count] = points[point];
    if (!keep_local AND ModelArchiveBase(ModelArchive) == MODEL_ARCHIVE_BODY AND point < MODEL_MAX_POINTS) {
        LongInverseRotatePoint(&MatriceWorld,
                               Obj_ListRotatedPoints[point].X,
                               Obj_ListRotatedPoints[point].Y,
                               Obj_ListRotatedPoints[point].Z);
        ModelClip.points[*count].X = ModelClampS16(X0);
        ModelClip.points[*count].Y = ModelClampS16(Y0);
        ModelClip.points[*count].Z = ModelClampS16(Z0);
    }
    (*count)++;
    return *count - 1;
}

static S32 ModelClipboardAddPoint(T_BODY_HEADER *body, S32 *source_points, S32 *count, S32 point) {
    return ModelClipboardAddPointEx(body, source_points, count, point, FALSE);
}

static void ModelCopySelection(T_BODY_HEADER *body, S32 bones_only) {
    ModelGroupInfo *groups;
    S32 *source_points;
    S32 i;

    if (!body)
        return;
    ModelClearClipboard();
    groups = body->NbGroupes > 0 ? (ModelGroupInfo *)((U8 *)body + body->OffGroupes) : NULL;
    source_points = (S32 *)Malloc(sizeof(S32) * body->NbPoints);
    ModelClip.points = (T_OBJ_POINT *)Malloc(sizeof(T_OBJ_POINT) * body->NbPoints);
    ModelClip.point_sources = (S32 *)Malloc(sizeof(S32) * body->NbPoints);
    ModelClip.point_selected = (U8 *)Malloc(body->NbPoints);
    ModelClip.faces = ModelSelectedFaceCount > 0 ? (ModelFaceInfo *)Malloc(sizeof(ModelFaceInfo) * ModelSelectedFaceCount) : NULL;
    ModelClip.lines = ModelSelectedLineCount > 0 ? (ModelLineInfo *)Malloc(sizeof(ModelLineInfo) * ModelSelectedLineCount) : NULL;
    ModelClip.spheres = ModelSelectedSphereCount > 0 ? (ModelSphereInfo *)Malloc(sizeof(ModelSphereInfo) * ModelSelectedSphereCount) : NULL;
    ModelClip.bones = ModelSelectedBoneCount > 0 ? (ModelGroupInfo *)Malloc(sizeof(ModelGroupInfo) * ModelSelectedBoneCount) : NULL;
    ModelClip.bone_sources = ModelSelectedBoneCount > 0 ? (S32 *)Malloc(sizeof(S32) * ModelSelectedBoneCount) : NULL;
    ModelClip.select_mask = ModelSelectMask;
    ModelClip.source_archive_base = ModelArchiveBase(ModelArchive);
    ModelClip.source_body_id = ModelBodyId;
    if (!source_points OR !ModelClip.points OR !ModelClip.point_sources OR !ModelClip.point_selected OR
        (ModelSelectedBoneCount > 0 AND(!ModelClip.bones OR !ModelClip.bone_sources))) {
        if (source_points)
            Free(source_points);
        ModelClearClipboard();
        ModelSetStatus("Copy failed: out of memory");
        return;
    }
    memset(ModelClip.point_selected, 0, body->NbPoints);

    if (ModelSelectActive(MODEL_SELECT_BONE) AND ModelClip.bones AND ModelClip.bone_sources AND groups) {
        for (i = 0; i < body->NbGroupes AND i < ModelBoneSelectionCapacity; i++) {
            ModelGroupInfo group;
            S32 start;
            S32 p;

            if (!ModelBoneSelection[i])
                continue;
            group = groups[i];
            if (!bones_only) {
                start = ModelGroupPointStart(body, i);
                for (p = 0; p < group.NbPts AND start + p < body->NbPoints; p++)
                    ModelClipboardAddPointEx(body, source_points, &ModelClip.point_count, start + p, TRUE);
                ModelClip.bone_points_copied = TRUE;
            } else {
                group.NbPts = 0;
            }
            group.NbNorm = 0;
            ModelClip.bone_sources[ModelClip.bone_count] = i;
            ModelClip.bones[ModelClip.bone_count++] = group;
        }
    }

    if (ModelSelectActive(MODEL_SELECT_VERTEX)) {
        for (i = 0; i < body->NbPoints AND i < ModelVertexSelectionCapacity; i++) {
            if (ModelVertexSelection[i]) {
                S32 clip_point = ModelClipboardAddPoint(body, source_points, &ModelClip.point_count, i);

                if (clip_point >= 0)
                    ModelClip.point_selected[clip_point] = 1;
            }
        }
    }
    if (ModelSelectActive(MODEL_SELECT_FACE) AND ModelClip.faces) {
        for (i = 0; i < body->NbPolys AND i < ModelFaceSelectionCapacity; i++) {
            ModelFaceInfo info;
            S32 p;

            if (!ModelFaceSelection[i])
                continue;
            if (!ModelFindFaceByIndex(body, i, &info))
                continue;
            for (p = 0; p < info.point_count; p++)
                info.points[p] = (U16)ModelClipboardAddPoint(body, source_points, &ModelClip.point_count, info.points[p]);
            ModelClip.faces[ModelClip.face_count++] = info;
        }
    }
    if (ModelSelectActive(MODEL_SELECT_LINE) AND ModelClip.lines) {
        for (i = 0; i < body->NbLines AND i < ModelLineSelectionCapacity; i++) {
            ModelLineInfo info;

            if (!ModelLineSelection[i])
                continue;
            if (!ModelReadLineInfo(body, i, &info))
                continue;
            info.p1 = (U16)ModelClipboardAddPoint(body, source_points, &ModelClip.point_count, info.p1);
            info.p2 = (U16)ModelClipboardAddPoint(body, source_points, &ModelClip.point_count, info.p2);
            ModelClip.lines[ModelClip.line_count++] = info;
        }
    }
    if (ModelSelectActive(MODEL_SELECT_SPHERE) AND ModelClip.spheres) {
        for (i = 0; i < body->NbSpheres AND i < ModelSphereSelectionCapacity; i++) {
            ModelSphereInfo info;

            if (!ModelSphereSelection[i])
                continue;
            if (!ModelReadSphereInfo(body, i, &info))
                continue;
            info.point = (U16)ModelClipboardAddPoint(body, source_points, &ModelClip.point_count, info.point);
            ModelClip.spheres[ModelClip.sphere_count++] = info;
        }
    }
    Free(source_points);
    ModelSetStatus("Copied %d vertices, %d faces, %d spheres, %d lines, %d bones",
                   (int)ModelClip.point_count, (int)ModelClip.face_count,
                   (int)ModelClip.sphere_count, (int)ModelClip.line_count,
                   (int)ModelClip.bone_count);
}

static U16 ModelPastePointIndex(S32 point_base, const S32 *point_remap, U16 point) {
    if (point_remap AND point < ModelClip.point_count)
        return (U16)point_remap[point];
    return (U16)(point_base + point);
}

static S32 ModelTwinsenRemappingActive(T_BODY_HEADER *body) {
    if (!body)
        return FALSE;
    if (ModelClip.source_archive_base != MODEL_ARCHIVE_BODY OR ModelClip.source_body_id != 18)
        return FALSE;
    if (ModelArchiveBase(ModelArchive) != MODEL_ARCHIVE_BODY OR ModelBodyId != 0)
        return FALSE;
    return TRUE;
}

static S32 ModelApplyTwinsenLineEndpointRemappingHacks(T_BODY_HEADER *body, S32 endpoint, S32 clip_point) {
    S32 source_point;

    if (!ModelTwinsenRemappingActive(body))
        return -1;
    if (!ModelClip.point_sources OR clip_point < 0 OR clip_point >= ModelClip.point_count)
        return -1;
    source_point = ModelClip.point_sources[clip_point];
    if (endpoint == 1 AND source_point == 29 AND body->NbPoints > 32)
        return 32;
    if (endpoint == 2 AND source_point == 28 AND body->NbPoints > 31)
        return 31;
    return -1;
}

static S32 ModelFindTwinsenLineEndpointRemappingHack(T_BODY_HEADER *body, S32 clip_point) {
    S32 i;

    for (i = 0; i < ModelClip.line_count; i++) {
        S32 mapped;

        if (ModelClip.lines[i].p1 == clip_point) {
            mapped = ModelApplyTwinsenLineEndpointRemappingHacks(body, 1, clip_point);
            if (mapped >= 0)
                return mapped;
        }
        if (ModelClip.lines[i].p2 == clip_point) {
            mapped = ModelApplyTwinsenLineEndpointRemappingHacks(body, 2, clip_point);
            if (mapped >= 0)
                return mapped;
        }
    }
    return -1;
}

static S32 ModelFindNearbyPastePoint(T_BODY_HEADER *body, const T_OBJ_POINT *point) {
    T_OBJ_POINT *points;
    S32 best = -1;
    S32 best_dist = 8 * 8 + 1;
    S32 i;

    if (!body OR !point)
        return -1;
    points = (T_OBJ_POINT *)((U8 *)body + body->OffPoints);
    for (i = 0; i < body->NbPoints; i++) {
        S32 dx = (S32)points[i].X - (S32)point->X;
        S32 dy = (S32)points[i].Y - (S32)point->Y;
        S32 dz = (S32)points[i].Z - (S32)point->Z;
        S32 dist;

        dist = dx * dx + dy * dy + dz * dz;
        if (dist <= 8 * 8 AND dist < best_dist) {
            best = i;
            best_dist = dist;
        }
    }
    return best;
}

static S32 ModelPreparePastePointRemap(T_BODY_HEADER *body, T_OBJ_POINT *paste_points, S32 bone_point_count, S32 *point_remap, S32 *out_point_count) {
    U8 *line_points;
    S32 merged = 0;
    S32 kept = 0;
    S32 i;

    if (!body OR !paste_points OR !point_remap OR !out_point_count)
        return 0;
    *out_point_count = ModelClip.point_count;
    line_points = (U8 *)Malloc(ModelClip.point_count);
    if (!line_points)
        return 0;
    memset(line_points, 0, ModelClip.point_count);
    for (i = 0; i < ModelClip.line_count; i++) {
        if (ModelClip.lines[i].p1 < ModelClip.point_count)
            line_points[ModelClip.lines[i].p1] = 1;
        if (ModelClip.lines[i].p2 < ModelClip.point_count)
            line_points[ModelClip.lines[i].p2] = 1;
    }
    for (i = 0; i < ModelClip.point_count; i++) {
        S32 merge_point = -1;

        if (i >= bone_point_count AND line_points[i])
            merge_point = ModelFindTwinsenLineEndpointRemappingHack(body, i);
        if (merge_point < 0 AND i >= bone_point_count AND line_points[i])
            merge_point = ModelFindNearbyPastePoint(body, paste_points + i);
        if (merge_point >= 0) {
            point_remap[i] = merge_point;
            merged++;
        } else {
            if (kept != i)
                paste_points[kept] = paste_points[i];
            point_remap[i] = body->NbPoints + kept;
            kept++;
        }
    }
    *out_point_count = kept;
    Free(line_points);
    return merged;
}

static void ModelWriteClipboardFace(U8 *dst, const ModelFaceInfo *info, S32 point_base, const S32 *point_remap) {
    if (info->face_type == MODEL_FACE_TEXTURE AND info->is_quad) {
        ModelPoly4Texture *p = (ModelPoly4Texture *)dst;
        p->P1 = ModelPastePointIndex(point_base, point_remap, info->points[0]);
        p->P2 = ModelPastePointIndex(point_base, point_remap, info->points[1]);
        p->P3 = ModelPastePointIndex(point_base, point_remap, info->points[2]);
        p->P4 = ModelPastePointIndex(point_base, point_remap, info->points[3]);
        p->Couleur = info->color;
        p->Normale = info->normal;
        p->U1 = info->u[0];
        p->V1 = info->v[0];
        p->U2 = info->u[1];
        p->V2 = info->v[1];
        p->U3 = info->u[2];
        p->V3 = info->v[2];
        p->U4 = info->u[3];
        p->V4 = info->v[3];
        p->HandleText = info->texture;
        p->__padding = 0;
    } else if (info->face_type == MODEL_FACE_TEXTURE) {
        ModelPoly3Texture *p = (ModelPoly3Texture *)dst;
        p->P1 = ModelPastePointIndex(point_base, point_remap, info->points[0]);
        p->P2 = ModelPastePointIndex(point_base, point_remap, info->points[1]);
        p->P3 = ModelPastePointIndex(point_base, point_remap, info->points[2]);
        p->HandleText = info->texture;
        p->Couleur = info->color;
        p->Normale = info->normal;
        p->U1 = info->u[0];
        p->V1 = info->v[0];
        p->U2 = info->u[1];
        p->V2 = info->v[1];
        p->U3 = info->u[2];
        p->V3 = info->v[2];
    } else if (info->face_type == MODEL_FACE_ENV AND info->is_quad) {
        ModelPoly4Env *p = (ModelPoly4Env *)dst;
        p->P1 = ModelPastePointIndex(point_base, point_remap, info->points[0]);
        p->P2 = ModelPastePointIndex(point_base, point_remap, info->points[1]);
        p->P3 = ModelPastePointIndex(point_base, point_remap, info->points[2]);
        p->P4 = ModelPastePointIndex(point_base, point_remap, info->points[3]);
        p->Couleur = info->color;
        p->Normale = info->normal;
        p->Scale = info->scale;
        p->HandleEnv = info->texture;
    } else if (info->face_type == MODEL_FACE_ENV) {
        ModelPoly3Env *p = (ModelPoly3Env *)dst;
        p->P1 = ModelPastePointIndex(point_base, point_remap, info->points[0]);
        p->P2 = ModelPastePointIndex(point_base, point_remap, info->points[1]);
        p->P3 = ModelPastePointIndex(point_base, point_remap, info->points[2]);
        p->HandleEnv = info->texture;
        p->Couleur = info->color;
        p->Normale = info->normal;
        p->Scale = info->scale;
        p->__padding = 0;
    } else if (info->is_quad) {
        ModelPoly4Light *p = (ModelPoly4Light *)dst;
        p->P1 = ModelPastePointIndex(point_base, point_remap, info->points[0]);
        p->P2 = ModelPastePointIndex(point_base, point_remap, info->points[1]);
        p->P3 = ModelPastePointIndex(point_base, point_remap, info->points[2]);
        p->P4 = ModelPastePointIndex(point_base, point_remap, info->points[3]);
        p->Couleur = info->color;
        p->Normale = info->normal;
    } else {
        ModelPoly3Light *p = (ModelPoly3Light *)dst;
        p->P1 = ModelPastePointIndex(point_base, point_remap, info->points[0]);
        p->P2 = ModelPastePointIndex(point_base, point_remap, info->points[1]);
        p->P3 = ModelPastePointIndex(point_base, point_remap, info->points[2]);
        p->__padding = 0;
        p->Couleur = info->color;
        p->Normale = info->normal;
    }
}

static S32 ModelClipboardPolyBytes(void) {
    S32 i;
    S32 size = 0;

    for (i = 0; i < ModelClip.face_count; i++)
        size += (S32)sizeof(ModelPolyHeader) + ModelFaceRecordSize(ModelClip.faces[i].face_type, ModelClip.faces[i].is_quad);
    return size;
}

static void ModelRecomputeBounds(T_BODY_HEADER *body) {
    T_OBJ_POINT *points;
    S32 i;

    if (!body OR body->NbPoints <= 0)
        return;
    points = (T_OBJ_POINT *)((U8 *)body + body->OffPoints);
    body->XMin = body->XMax = points[0].X;
    body->YMin = body->YMax = points[0].Y;
    body->ZMin = body->ZMax = points[0].Z;
    for (i = 1; i < body->NbPoints; i++) {
        if (points[i].X < body->XMin)
            body->XMin = points[i].X;
        if (points[i].X > body->XMax)
            body->XMax = points[i].X;
        if (points[i].Y < body->YMin)
            body->YMin = points[i].Y;
        if (points[i].Y > body->YMax)
            body->YMax = points[i].Y;
        if (points[i].Z < body->ZMin)
            body->ZMin = points[i].Z;
        if (points[i].Z > body->ZMax)
            body->ZMax = points[i].Z;
    }
}

static S16 ModelClampS16(S32 value) {
    if (value < -32768)
        return -32768;
    if (value > 32767)
        return 32767;
    return (S16)value;
}

static void ModelAttachPastedPointsToBodyGroup(T_BODY_HEADER *body, S32 point_base, S32 point_count, S32 group_index) {
    ModelGroupInfo *groups;
    ModelGroupInfo *group;
    T_OBJ_POINT *points;
    S32 i;

    if (!body OR point_count <= 0)
        return;
    if (ModelArchiveBase(ModelArchive) != MODEL_ARCHIVE_BODY)
        return;
    if (body->NbGroupes <= 0 OR group_index < 0 OR group_index >= body->NbGroupes)
        return;
    groups = (ModelGroupInfo *)((U8 *)body + body->OffGroupes);
    group = groups + group_index;
    if ((S32)group->NbPts + point_count > 0xffff)
        return;
    group->NbPts = (U16)(group->NbPts + point_count);

    points = (T_OBJ_POINT *)((U8 *)body + body->OffPoints);
    for (i = 0; i < point_count AND point_base + i < body->NbPoints; i++)
        points[point_base + i].Group = (S16)group_index;
}

static void ModelConvertPastedPointsToBodyGroup(T_BODY_HEADER *body, T_OBJ_POINT *points, S32 point_count) {
    TYPE_MAT *mat;
    S32 group_index;
    S32 i;

    if (!body OR !points OR point_count <= 0)
        return;
    if (ModelArchiveBase(ModelArchive) != MODEL_ARCHIVE_BODY)
        return;
    if (body->NbGroupes <= 0)
        return;

    group_index = body->NbGroupes - 1;
    mat = &TabMat[group_index];
    for (i = 0; i < point_count; i++) {
        S32 rx;
        S32 ry;
        S32 rz;

        LongRotatePoint(&MatriceWorld, points[i].X, points[i].Y, points[i].Z);
        rx = X0;
        ry = Y0;
        rz = Z0;
        LongInverseRotatePoint(mat,
                               rx - (S32)mat->F.TX,
                               ry - (S32)mat->F.TY,
                               rz - (S32)mat->F.TZ);
        points[i].X = ModelClampS16(X0);
        points[i].Y = ModelClampS16(Y0);
        points[i].Z = ModelClampS16(Z0);
        points[i].Group = (S16)group_index;
    }
}

static S32 ModelApplyTwinsenRemappingHacks(T_BODY_HEADER *body, ModelGroupInfo *group) {
    if (!body OR !group)
        return FALSE;
    if (!ModelTwinsenRemappingActive(body))
        return FALSE;
    if (group->OrgPoint == 72 AND body->NbPoints > 77) {
        group->OrgPoint = 77;
        return TRUE;
    }
    if (group->OrgPoint == 43 AND body->NbPoints > 195) {
        group->OrgPoint = 195;
        return TRUE;
    }
    return FALSE;
}

static void ModelWriteClipboardBones(T_BODY_HEADER *body, U8 *dst, S32 group_base, S32 point_base, const S32 *point_remap) {
    S32 i;

    if (!body OR !dst)
        return;
    for (i = 0; i < ModelClip.bone_count; i++) {
        ModelGroupInfo group = ModelClip.bones[i];
        S32 parent = ModelClipboardFindBoneSource(group.OrgGroupe);
        S32 point = ModelClipboardFindPointSource(group.OrgPoint);

        if (parent >= 0)
            group.OrgGroupe = (U16)(group_base + parent);
        else if (group.OrgGroupe >= body->NbGroupes)
            group.OrgGroupe = 0;

        if (!ModelApplyTwinsenRemappingHacks(body, &group)) {
            if (point >= 0)
                group.OrgPoint = (U16)(point_base + point);
            else if (group.OrgPoint >= body->NbPoints)
                group.OrgPoint = 0;
        }

        memcpy(dst + i * (S32)sizeof(ModelGroupInfo), &group, sizeof(ModelGroupInfo));
    }
}

static void ModelAssignPastedBonePointGroups(T_OBJ_POINT *points, S32 point_count, S32 group_base) {
    S32 point = 0;
    S32 i;

    if (!points)
        return;
    for (i = 0; i < ModelClip.bone_count; i++) {
        S32 p;

        for (p = 0; p < ModelClip.bones[i].NbPts AND point < point_count; p++) {
            points[point].Group = (S16)(group_base + i);
            point++;
        }
    }
}

static void ModelSelectPastedGeometry(T_BODY_HEADER *body, S32 point_base, S32 face_base, S32 line_base, S32 sphere_base, S32 bone_base, const S32 *point_remap) {
    S32 i;

    if (!body)
        return;
    ModelSelectMask = ModelClip.select_mask;
    if (ModelSelectMask == 0)
        ModelSelectMask = MODEL_SELECT_MASK_VERTEX;
    if (ModelSelectActive(MODEL_SELECT_VERTEX))
        ModelSelectMode = MODEL_SELECT_VERTEX;
    if (ModelSelectActive(MODEL_SELECT_FACE))
        ModelSelectMode = MODEL_SELECT_FACE;
    if (ModelSelectActive(MODEL_SELECT_SPHERE))
        ModelSelectMode = MODEL_SELECT_SPHERE;
    if (ModelSelectActive(MODEL_SELECT_LINE))
        ModelSelectMode = MODEL_SELECT_LINE;
    if (ModelSelectActive(MODEL_SELECT_BONE))
        ModelSelectMode = MODEL_SELECT_BONE;

    ModelClearVertexSelection();
    ModelClearFaceSelection();
    ModelClearSphereSelection();
    ModelClearLineSelection();
    ModelClearBoneSelection();

    if (ModelSelectActive(MODEL_SELECT_VERTEX)) {
        for (i = 0; i < ModelClip.point_count; i++) {
            S32 point = point_remap ? point_remap[i] : point_base + i;

            if (point < 0 OR point >= body->NbPoints OR point >= ModelVertexSelectionCapacity)
                continue;
            if (!ModelClip.point_selected OR ModelClip.point_selected[i]) {
                ModelVertexSelection[point] = 1;
                ModelSelectedVertexCount++;
                ModelSelectedVertex = point;
            }
        }
    }
    if (ModelSelectActive(MODEL_SELECT_FACE)) {
        for (i = 0; i < ModelClip.face_count AND face_base + i < body->NbPolys AND face_base + i < ModelFaceSelectionCapacity; i++) {
            ModelFaceInfo info;

            ModelFaceSelection[face_base + i] = 1;
            ModelSelectedFaceCount++;
            if (ModelFindFaceByIndex(body, face_base + i, &info)) {
                ModelSelectedFace = face_base + i;
                ModelSelectedFaceInfo = info;
            }
        }
    }
    if (ModelSelectActive(MODEL_SELECT_SPHERE)) {
        for (i = 0; i < ModelClip.sphere_count AND sphere_base + i < body->NbSpheres AND sphere_base + i < ModelSphereSelectionCapacity; i++) {
            ModelSphereInfo info;

            ModelSphereSelection[sphere_base + i] = 1;
            ModelSelectedSphereCount++;
            if (ModelReadSphereInfo(body, sphere_base + i, &info)) {
                ModelSelectedSphere = sphere_base + i;
                ModelSelectedSphereInfo = info;
            }
        }
    }
    if (ModelSelectActive(MODEL_SELECT_LINE)) {
        for (i = 0; i < ModelClip.line_count AND line_base + i < body->NbLines AND line_base + i < ModelLineSelectionCapacity; i++) {
            ModelLineInfo info;

            ModelLineSelection[line_base + i] = 1;
            ModelSelectedLineCount++;
            if (ModelReadLineInfo(body, line_base + i, &info)) {
                ModelSelectedLine = line_base + i;
                ModelSelectedLineInfo = info;
            }
        }
    }
    if (ModelSelectActive(MODEL_SELECT_BONE)) {
        for (i = 0; i < ModelClip.bone_count AND bone_base + i < body->NbGroupes AND bone_base + i < ModelBoneSelectionCapacity; i++) {
            ModelBoneSelection[bone_base + i] = 1;
            ModelSelectedBoneCount++;
            ModelSelectedBone = bone_base + i;
        }
    }
}

static void ModelPasteClipboard(T_BODY_HEADER *body) {
    U32 old_size;
    U32 group_bytes;
    U32 point_bytes;
    U32 poly_bytes;
    U32 line_bytes;
    U32 sphere_bytes;
    U32 new_size;
    U32 group_end;
    U32 point_end;
    U8 *new_body;
    U8 *dst;
    S32 group_base;
    S32 point_base;
    S32 face_base;
    S32 line_base;
    S32 sphere_base;
    S32 bone_point_count;
    S32 loose_point_count;
    S32 loose_point_base;
    S32 pasted_point_count;
    S32 merged_point_count;
    S32 *point_remap;
    T_OBJ_POINT *paste_points;
    S32 i;

    if (!body OR ModelLoadedBodySize == 0)
        return;
    if (ModelClip.point_count <= 0 AND ModelClip.face_count <= 0 AND ModelClip.line_count <= 0 AND ModelClip.sphere_count <= 0 AND ModelClip.bone_count <= 0) {
        ModelSetStatus("Paste refused: clipboard empty");
        return;
    }
    if (ModelClip.bone_count > 0 AND ModelArchiveBase(ModelArchive) != MODEL_ARCHIVE_BODY) {
        ModelSetStatus("Paste refused: bones need BODY archive");
        return;
    }
    if (body->NbGroupes + ModelClip.bone_count > MODEL_MAX_GROUPS) {
        ModelSetStatus("Paste refused: %d bones exceeds %d",
                       (int)(body->NbGroupes + ModelClip.bone_count), (int)MODEL_MAX_GROUPS);
        return;
    }
    if (body->NbPoints + ModelClip.point_count > MODEL_MAX_POINTS) {
        ModelSetStatus("Paste refused: %d points exceeds %d",
                       (int)(body->NbPoints + ModelClip.point_count), (int)MODEL_MAX_POINTS);
        return;
    }
    if (body->NbPolys + body->NbLines + body->NbSpheres +
            ModelClip.face_count + ModelClip.line_count + ModelClip.sphere_count >
        MODEL_MAX_SORT_ENTRIES) {
        ModelSetStatus("Paste refused: %d draw entries exceeds %d",
                       (int)(body->NbPolys + body->NbLines + body->NbSpheres +
                             ModelClip.face_count + ModelClip.line_count + ModelClip.sphere_count),
                       (int)MODEL_MAX_SORT_ENTRIES);
        return;
    }
    point_remap = ModelClip.point_count > 0 ? (S32 *)Malloc(sizeof(S32) * ModelClip.point_count) : NULL;
    paste_points = ModelClip.point_count > 0 ? (T_OBJ_POINT *)Malloc(sizeof(T_OBJ_POINT) * ModelClip.point_count) : NULL;
    if (ModelClip.point_count > 0 AND(!point_remap OR !paste_points)) {
        if (point_remap)
            Free(point_remap);
        if (paste_points)
            Free(paste_points);
        ModelSetStatus("Paste failed: out of memory");
        return;
    }
    point_base = body->NbPoints;
    bone_point_count = ModelClipboardBonePointCount();
    if (bone_point_count > ModelClip.point_count)
        bone_point_count = ModelClip.point_count;
    if (ModelClip.point_count > 0) {
        memcpy(paste_points, ModelClip.points, sizeof(T_OBJ_POINT) * ModelClip.point_count);
        if (bone_point_count > 0)
            ModelAssignPastedBonePointGroups(paste_points, bone_point_count, body->NbGroupes);
        if (ModelClip.point_count - bone_point_count > 0)
            ModelConvertPastedPointsToBodyGroup(body, paste_points + bone_point_count, ModelClip.point_count - bone_point_count);
        merged_point_count = ModelPreparePastePointRemap(body, paste_points, bone_point_count, point_remap, &pasted_point_count);
    } else {
        merged_point_count = 0;
        pasted_point_count = 0;
    }

    old_size = ModelLoadedBodySize;
    group_bytes = (U32)(ModelClip.bone_count * (S32)sizeof(ModelGroupInfo));
    point_bytes = (U32)(pasted_point_count * (S32)sizeof(T_OBJ_POINT));
    poly_bytes = (U32)ModelClipboardPolyBytes();
    line_bytes = (U32)(ModelClip.line_count * (S32)sizeof(ModelLineDisk));
    sphere_bytes = (U32)(ModelClip.sphere_count * (S32)sizeof(ModelSphereDisk));
    new_size = old_size + group_bytes + point_bytes + poly_bytes + line_bytes + sphere_bytes;
    new_body = (U8 *)Malloc(new_size + RECOVER_AREA);
    if (!new_body) {
        if (point_remap)
            Free(point_remap);
        if (paste_points)
            Free(paste_points);
        ModelSetStatus("Paste failed: out of memory");
        return;
    }

    group_end = body->OffGroupes + body->NbGroupes * (S32)sizeof(ModelGroupInfo);
    point_end = body->OffPoints + body->NbPoints * (S32)sizeof(T_OBJ_POINT);
    group_base = body->NbGroupes;
    face_base = body->NbPolys;
    line_base = body->NbLines;
    sphere_base = body->NbSpheres;
    loose_point_count = pasted_point_count - bone_point_count;
    if (loose_point_count < 0)
        loose_point_count = 0;
    loose_point_base = point_base + bone_point_count;
    dst = new_body;
    memcpy(dst, body, group_end);
    dst += group_end;
    if (group_bytes > 0) {
        ModelWriteClipboardBones(body, dst, group_base, point_base, point_remap);
        dst += group_bytes;
    }
    memcpy(dst, (U8 *)body + group_end, point_end - group_end);
    dst += point_end - group_end;
    if (point_bytes > 0) {
        memcpy(dst, paste_points, point_bytes);
        dst += point_bytes;
    }
    memcpy(dst, (U8 *)body + point_end, body->OffLines - point_end);
    dst += body->OffLines - point_end;
    for (i = 0; i < ModelClip.face_count; i++) {
        ModelPolyHeader *header = (ModelPolyHeader *)dst;
        S32 face_size = ModelFaceRecordSize(ModelClip.faces[i].face_type, ModelClip.faces[i].is_quad);

        header->TypePoly = ModelClip.faces[i].raw_type_poly;
        header->NbPoly = 1;
        header->OffNextType = 0;
        dst += sizeof(ModelPolyHeader);
        ModelWriteClipboardFace(dst, &ModelClip.faces[i], point_base, point_remap);
        dst += face_size;
    }
    memcpy(dst, (U8 *)body + body->OffLines, body->OffSpheres - body->OffLines);
    dst += body->OffSpheres - body->OffLines;
    for (i = 0; i < ModelClip.line_count; i++) {
        ModelLineDisk *line = (ModelLineDisk *)dst;

        line->Type = ModelClip.lines[i].type;
        line->Coul = ModelClip.lines[i].color;
        line->P1 = ModelPastePointIndex(point_base, point_remap, ModelClip.lines[i].p1);
        line->P2 = ModelPastePointIndex(point_base, point_remap, ModelClip.lines[i].p2);
        dst += sizeof(ModelLineDisk);
    }
    memcpy(dst, (U8 *)body + body->OffSpheres, body->OffTextures - body->OffSpheres);
    dst += body->OffTextures - body->OffSpheres;
    for (i = 0; i < ModelClip.sphere_count; i++) {
        ModelSphereDisk *sphere = (ModelSphereDisk *)dst;

        sphere->Type = ModelClip.spheres[i].type;
        sphere->Coul = ModelClip.spheres[i].color;
        sphere->P1 = ModelPastePointIndex(point_base, point_remap, ModelClip.spheres[i].point);
        sphere->Rayon = ModelClip.spheres[i].radius;
        dst += sizeof(ModelSphereDisk);
    }
    memcpy(dst, (U8 *)body + body->OffTextures, old_size - body->OffTextures);

    {
        T_BODY_HEADER *new_header = (T_BODY_HEADER *)new_body;

        new_header->NbGroupes += ModelClip.bone_count;
        new_header->NbPoints += pasted_point_count;
        new_header->NbPolys += ModelClip.face_count;
        new_header->NbLines += ModelClip.line_count;
        new_header->NbSpheres += ModelClip.sphere_count;
        new_header->OffPoints += group_bytes;
        new_header->OffNormales += group_bytes + point_bytes;
        new_header->OffNormFaces += group_bytes + point_bytes;
        new_header->OffPolys += group_bytes + point_bytes;
        new_header->OffLines += group_bytes + point_bytes + poly_bytes;
        new_header->OffSpheres += group_bytes + point_bytes + poly_bytes + line_bytes;
        new_header->OffTextures += group_bytes + point_bytes + poly_bytes + line_bytes + sphere_bytes;
        if (loose_point_count > 0)
            ModelAttachPastedPointsToBodyGroup(new_header, loose_point_base, loose_point_count, body->NbGroupes - 1);
        ModelRecomputeBounds(new_header);
    }

    Free(ModelLoadedBody);
    ModelLoadedBody = (T_BODY_HEADER *)new_body;
    ModelLoadedBodySize = new_size;
    ModelEnsureVertexSelection(ModelLoadedBody);
    ModelEnsureFaceSelection(ModelLoadedBody);
    ModelEnsureSphereSelection(ModelLoadedBody);
    ModelEnsureLineSelection(ModelLoadedBody);
    ModelEnsureBoneSelection(ModelLoadedBody);
    ModelSelectPastedGeometry(ModelLoadedBody, point_base, face_base, line_base, sphere_base, group_base, point_remap);
    if (merged_point_count > 0)
        ModelSetStatus("Pasted %d vertices, %d faces, %d spheres, %d lines, %d bones; merged %d vertices",
                       (int)pasted_point_count, (int)ModelClip.face_count,
                       (int)ModelClip.sphere_count, (int)ModelClip.line_count,
                       (int)ModelClip.bone_count, (int)merged_point_count);
    else
        ModelSetStatus("Pasted %d vertices, %d faces, %d spheres, %d lines, %d bones",
                   (int)pasted_point_count, (int)ModelClip.face_count,
                   (int)ModelClip.sphere_count, (int)ModelClip.line_count,
                   (int)ModelClip.bone_count);
    if (point_remap)
        Free(point_remap);
    if (paste_points)
        Free(paste_points);
}

static void ModelStartGrab(T_BODY_HEADER *body) {
    T_OBJ_POINT *points;
    S32 i;

    if (!body OR body->NbPoints <= 0)
        return;
    ModelClearGrabState();
    ModelGrab.indices = (S32 *)Malloc(sizeof(S32) * body->NbPoints);
    ModelGrab.original_points = (T_OBJ_POINT *)Malloc(sizeof(T_OBJ_POINT) * body->NbPoints);
    if (!ModelGrab.indices OR !ModelGrab.original_points) {
        ModelClearGrabState();
        ModelSetStatus("Grab failed: out of memory");
        return;
    }
    ModelGrab.capacity = body->NbPoints;
    ModelGrab.count = ModelCollectSelectedPointIndices(body, ModelGrab.indices, body->NbPoints);
    if (ModelGrab.count <= 0) {
        ModelClearGrabState();
        ModelSetStatus("Grab refused: nothing selected");
        return;
    }
    points = (T_OBJ_POINT *)((U8 *)body + body->OffPoints);
    for (i = 0; i < ModelGrab.count; i++)
        ModelGrab.original_points[i] = points[ModelGrab.indices[i]];
    ModelGrab.start_mouse_x = (S32)MouseX;
    ModelGrab.start_mouse_y = (S32)MouseY;
    ModelGrab.axis = -1;
    ModelGrabMode = TRUE;
    ModelSetStatus("Grab");
}

static void ModelApplyGrab(T_BODY_HEADER *body) {
    T_OBJ_POINT *points;
    S32 dx;
    S32 dy;
    S32 scale;
    S32 move_x;
    S32 move_y;
    S32 move_z;
    S32 i;

    if (!ModelGrabMode OR !body OR !ModelGrab.indices OR !ModelGrab.original_points)
        return;
    points = (T_OBJ_POINT *)((U8 *)body + body->OffPoints);
    dx = (S32)MouseX - ModelGrab.start_mouse_x;
    dy = (S32)MouseY - ModelGrab.start_mouse_y;
    scale = ModelZoom / 2500;
    if (scale < 1)
        scale = 1;
    if (scale > 20)
        scale = 20;
    SaveCamera();
    ModelApplyCamera();
    LongInverseRotatePoint(&MatriceWorld, dx * scale, -dy * scale, 0);
    move_x = X0;
    move_y = Y0;
    move_z = Z0;
    RestoreCamera();
    if (ModelGrab.axis == 0) {
        move_y = 0;
        move_z = 0;
    } else if (ModelGrab.axis == 1) {
        move_x = 0;
        move_z = 0;
    } else if (ModelGrab.axis == 2) {
        move_x = 0;
        move_y = 0;
    }
    for (i = 0; i < ModelGrab.count; i++) {
        S32 index = ModelGrab.indices[i];

        if (index < 0 OR index >= body->NbPoints)
            continue;
        points[index].X = ModelClampS16((S32)ModelGrab.original_points[i].X + move_x);
        points[index].Y = ModelClampS16((S32)ModelGrab.original_points[i].Y + move_y);
        points[index].Z = ModelClampS16((S32)ModelGrab.original_points[i].Z + move_z);
    }
    ModelRecomputeBounds(body);
}

static void ModelCancelGrab(T_BODY_HEADER *body) {
    T_OBJ_POINT *points;
    S32 i;

    if (body AND ModelGrab.indices AND ModelGrab.original_points) {
        points = (T_OBJ_POINT *)((U8 *)body + body->OffPoints);
        for (i = 0; i < ModelGrab.count; i++) {
            S32 index = ModelGrab.indices[i];

            if (index >= 0 AND index < body->NbPoints)
                points[index] = ModelGrab.original_points[i];
        }
        ModelRecomputeBounds(body);
    }
    ModelGrabMode = FALSE;
    ModelClearGrabState();
}

static void ModelCommitGrab(T_BODY_HEADER *body) {
    if (body)
        ModelRecomputeBounds(body);
    ModelGrabMode = FALSE;
    ModelClearGrabState();
}
