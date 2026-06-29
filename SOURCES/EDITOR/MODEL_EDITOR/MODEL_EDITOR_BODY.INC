static void ModelApplyEditorPalette(void) {
    PalOne(MODEL_SELECTED_FACE_FILL, 120, 92, 32);
    PalOne(MODEL_SELECTED_FACE_LINE, 255, 230, 80);
    PalOne(MODEL_WIRE_COLOR, 118, 118, 118);
    PalOne(MODEL_TEXT_MUTED, 170, 170, 170);
}

static S32 ModelFaceTypeFromHeader(ModelPolyHeader *header, S32 is_quad) {
    S32 mask = 0;

    memcpy(&mask, header, sizeof(S32));
    if (mask & MODEL_MASK_ENVIRONMENT)
        return MODEL_FACE_ENV;
    if ((header->TypePoly & 0xff) > MODEL_POLY_DITHER_TABLE)
        return MODEL_FACE_TEXTURE;
    return MODEL_FACE_LIGHT;
}

static S32 ModelFaceRecordSize(S32 face_type, S32 is_quad) {
    if (face_type == MODEL_FACE_TEXTURE)
        return is_quad ? (S32)sizeof(ModelPoly4Texture) : (S32)sizeof(ModelPoly3Texture);
    if (face_type == MODEL_FACE_ENV)
        return is_quad ? (S32)sizeof(ModelPoly4Env) : (S32)sizeof(ModelPoly3Env);
    return is_quad ? (S32)sizeof(ModelPoly4Light) : (S32)sizeof(ModelPoly3Light);
}

static void ModelClearFaceInfo(ModelFaceInfo *info) {
    memset(info, 0, sizeof(*info));
    info->index = -1;
    info->texture = 0xffff;
}

static void ModelReadFaceInfo(U8 *ptr, S32 index, S32 header_offset, S32 offset, U16 raw_type_poly, S32 face_type, S32 is_quad, ModelFaceInfo *info) {
    ModelClearFaceInfo(info);
    info->index = index;
    info->header_offset = header_offset;
    info->offset = offset;
    info->raw_type_poly = raw_type_poly;
    info->type_poly = raw_type_poly & 0xff;
    info->face_type = face_type;
    info->is_quad = is_quad;
    info->point_count = is_quad ? 4 : 3;

    if (face_type == MODEL_FACE_TEXTURE AND is_quad) {
        ModelPoly4Texture *p = (ModelPoly4Texture *)ptr;
        info->points[0] = p->P1;
        info->points[1] = p->P2;
        info->points[2] = p->P3;
        info->points[3] = p->P4;
        info->color = p->Couleur;
        info->normal = p->Normale;
        info->texture = p->HandleText;
        info->u[0] = p->U1;
        info->v[0] = p->V1;
        info->u[1] = p->U2;
        info->v[1] = p->V2;
        info->u[2] = p->U3;
        info->v[2] = p->V3;
        info->u[3] = p->U4;
        info->v[3] = p->V4;
    } else if (face_type == MODEL_FACE_TEXTURE) {
        ModelPoly3Texture *p = (ModelPoly3Texture *)ptr;
        info->points[0] = p->P1;
        info->points[1] = p->P2;
        info->points[2] = p->P3;
        info->color = p->Couleur;
        info->normal = p->Normale;
        info->texture = p->HandleText;
        info->u[0] = p->U1;
        info->v[0] = p->V1;
        info->u[1] = p->U2;
        info->v[1] = p->V2;
        info->u[2] = p->U3;
        info->v[2] = p->V3;
    } else if (face_type == MODEL_FACE_ENV AND is_quad) {
        ModelPoly4Env *p = (ModelPoly4Env *)ptr;
        info->points[0] = p->P1;
        info->points[1] = p->P2;
        info->points[2] = p->P3;
        info->points[3] = p->P4;
        info->color = p->Couleur;
        info->normal = p->Normale;
        info->texture = p->HandleEnv;
        info->scale = p->Scale;
    } else if (face_type == MODEL_FACE_ENV) {
        ModelPoly3Env *p = (ModelPoly3Env *)ptr;
        info->points[0] = p->P1;
        info->points[1] = p->P2;
        info->points[2] = p->P3;
        info->color = p->Couleur;
        info->normal = p->Normale;
        info->texture = p->HandleEnv;
        info->scale = p->Scale;
    } else if (is_quad) {
        ModelPoly4Light *p = (ModelPoly4Light *)ptr;
        info->points[0] = p->P1;
        info->points[1] = p->P2;
        info->points[2] = p->P3;
        info->points[3] = p->P4;
        info->color = p->Couleur;
        info->normal = p->Normale;
    } else {
        ModelPoly3Light *p = (ModelPoly3Light *)ptr;
        info->points[0] = p->P1;
        info->points[1] = p->P2;
        info->points[2] = p->P3;
        info->color = p->Couleur;
        info->normal = p->Normale;
    }
}

static S32 ModelFaceValid(const ModelFaceInfo *info, T_BODY_HEADER *body) {
    S32 i;

    if (!body OR info->index < 0)
        return FALSE;
    for (i = 0; i < info->point_count; i++) {
        if (info->points[i] >= body->NbPoints)
            return FALSE;
        if (info->points[i] >= MODEL_MAX_POINTS)
            return FALSE;
    }
    return TRUE;
}

static S64 ModelFaceScreenArea(const ModelFaceInfo *info) {
    S32 i;
    S64 area = 0;

    for (i = 0; i < info->point_count; i++) {
        U16 p0 = info->points[i];
        U16 p1 = info->points[(i + 1) % info->point_count];

        area += (S64)Obj_ListProjectedPoints[p0].X * Obj_ListProjectedPoints[p1].Y;
        area -= (S64)Obj_ListProjectedPoints[p1].X * Obj_ListProjectedPoints[p0].Y;
    }
    return area;
}

static S32 ModelFaceVisibleInOverlay(const ModelFaceInfo *info, T_BODY_HEADER *body) {
    if (!ModelFaceValid(info, body))
        return FALSE;
    if (ModelXray)
        return TRUE;
    return ModelFaceScreenArea(info) < 0 ? TRUE : FALSE;
}

static S64 ModelEdge(S32 ax, S32 ay, S32 bx, S32 by, S32 px, S32 py) {
    return (S64)(px - ax) * (by - ay) - (S64)(py - ay) * (bx - ax);
}

static S32 ModelPointInTri(S32 px, S32 py, U16 p0, U16 p1, U16 p2) {
    S32 ax = Obj_ListProjectedPoints[p0].X;
    S32 ay = Obj_ListProjectedPoints[p0].Y;
    S32 bx = Obj_ListProjectedPoints[p1].X;
    S32 by = Obj_ListProjectedPoints[p1].Y;
    S32 cx = Obj_ListProjectedPoints[p2].X;
    S32 cy = Obj_ListProjectedPoints[p2].Y;
    S64 e0 = ModelEdge(ax, ay, bx, by, px, py);
    S64 e1 = ModelEdge(bx, by, cx, cy, px, py);
    S64 e2 = ModelEdge(cx, cy, ax, ay, px, py);

    return ((e0 >= 0 AND e1 >= 0 AND e2 >= 0) OR(e0 <= 0 AND e1 <= 0 AND e2 <= 0)) ? TRUE : FALSE;
}

static S32 ModelFaceContainsPoint(const ModelFaceInfo *info, S32 x, S32 y, T_BODY_HEADER *body) {
    if (!ModelFaceValid(info, body))
        return FALSE;
    if (ModelPointInTri(x, y, info->points[0], info->points[1], info->points[2]))
        return TRUE;
    if (info->point_count == 4 AND ModelPointInTri(x, y, info->points[0], info->points[2], info->points[3]))
        return TRUE;
    return FALSE;
}

static S32 ModelFaceDepth(const ModelFaceInfo *info) {
    S32 i;
    S32 z = 0;

    for (i = 0; i < info->point_count; i++) {
        z += Obj_ListRotatedPoints[info->points[i]].Z;
    }
    return z / info->point_count;
}

static S32 ModelFindFaceAt(T_BODY_HEADER *body, S32 x, S32 y, ModelFaceInfo *out_info) {
    U8 *lines = (U8 *)body + body->OffLines;
    U8 *current = (U8 *)body + body->OffPolys;
    S32 body_offset = body->OffPolys;
    S32 face_index = 0;
    S32 best_face = -1;
    S32 best_depth = -0x7fffffff;

    ModelClearFaceInfo(out_info);
    while (current < lines AND face_index < body->NbPolys) {
        ModelPolyHeader *header = (ModelPolyHeader *)current;
        S32 mask = 0;
        S32 is_quad;
        S32 face_type;
        S32 size;
        S32 header_offset = body_offset;
        S32 i;

        memcpy(&mask, header, sizeof(S32));
        is_quad = (mask & MODEL_MASK_QUADRILATERE) ? TRUE : FALSE;
        face_type = ModelFaceTypeFromHeader(header, is_quad);
        current += sizeof(ModelPolyHeader);
        body_offset += sizeof(ModelPolyHeader);
        size = ModelFaceRecordSize(face_type, is_quad);

        for (i = 0; i < header->NbPoly AND face_index < body->NbPolys; i++) {
            ModelFaceInfo info;
            S32 depth;

            ModelReadFaceInfo(current, face_index, header_offset, body_offset, header->TypePoly, face_type, is_quad, &info);
            if (ModelFaceVisibleInOverlay(&info, body) AND ModelFaceContainsPoint(&info, x, y, body)) {
                depth = ModelFaceDepth(&info);
                if (best_face < 0 OR depth > best_depth) {
                    best_face = face_index;
                    best_depth = depth;
                    *out_info = info;
                }
            }
            current += size;
            body_offset += size;
            face_index++;
        }
    }
    return best_face;
}

static S32 ModelFindFaceByIndex(T_BODY_HEADER *body, S32 wanted, ModelFaceInfo *out_info) {
    U8 *lines = (U8 *)body + body->OffLines;
    U8 *current = (U8 *)body + body->OffPolys;
    S32 body_offset = body->OffPolys;
    S32 face_index = 0;

    ModelClearFaceInfo(out_info);
    while (current < lines AND face_index < body->NbPolys) {
        ModelPolyHeader *header = (ModelPolyHeader *)current;
        S32 mask = 0;
        S32 is_quad;
        S32 face_type;
        S32 size;
        S32 header_offset = body_offset;
        S32 i;

        memcpy(&mask, header, sizeof(S32));
        is_quad = (mask & MODEL_MASK_QUADRILATERE) ? TRUE : FALSE;
        face_type = ModelFaceTypeFromHeader(header, is_quad);
        current += sizeof(ModelPolyHeader);
        body_offset += sizeof(ModelPolyHeader);
        size = ModelFaceRecordSize(face_type, is_quad);

        for (i = 0; i < header->NbPoly AND face_index < body->NbPolys; i++) {
            if (face_index == wanted) {
                ModelReadFaceInfo(current, face_index, header_offset, body_offset, header->TypePoly, face_type, is_quad, out_info);
                return TRUE;
            }
            current += size;
            body_offset += size;
            face_index++;
        }
    }
    return FALSE;
}

static void ModelWriteFaceColor(T_BODY_HEADER *body, const ModelFaceInfo *info, S32 value) {
    U8 *ptr = (U8 *)body + info->offset;
    U16 color;

    if (!ModelFaceValid(info, body))
        return;
    value &= 0xff;
    if (info->face_type == MODEL_FACE_TEXTURE AND info->is_quad)
        color = ((ModelPoly4Texture *)ptr)->Couleur;
    else if (info->face_type == MODEL_FACE_TEXTURE)
        color = ((ModelPoly3Texture *)ptr)->Couleur;
    else if (info->face_type == MODEL_FACE_ENV AND info->is_quad)
        color = ((ModelPoly4Env *)ptr)->Couleur;
    else if (info->face_type == MODEL_FACE_ENV)
        color = ((ModelPoly3Env *)ptr)->Couleur;
    else if (info->is_quad)
        color = ((ModelPoly4Light *)ptr)->Couleur;
    else
        color = ((ModelPoly3Light *)ptr)->Couleur;

    color = (U16)((color & 0xff00) | value);
    if (info->face_type == MODEL_FACE_TEXTURE AND info->is_quad)
        ((ModelPoly4Texture *)ptr)->Couleur = color;
    else if (info->face_type == MODEL_FACE_TEXTURE)
        ((ModelPoly3Texture *)ptr)->Couleur = color;
    else if (info->face_type == MODEL_FACE_ENV AND info->is_quad)
        ((ModelPoly4Env *)ptr)->Couleur = color;
    else if (info->face_type == MODEL_FACE_ENV)
        ((ModelPoly3Env *)ptr)->Couleur = color;
    else if (info->is_quad)
        ((ModelPoly4Light *)ptr)->Couleur = color;
    else
        ((ModelPoly3Light *)ptr)->Couleur = color;
}

static void ModelWriteFaceNormal(T_BODY_HEADER *body, const ModelFaceInfo *info, S32 value) {
    U8 *ptr = (U8 *)body + info->offset;

    if (!ModelFaceValid(info, body))
        return;
    if (value < 0)
        value = 0;
    if (body->NbNormales > 0 AND value >= body->NbNormales)
        value = body->NbNormales - 1;

    if (info->face_type == MODEL_FACE_TEXTURE AND info->is_quad)
        ((ModelPoly4Texture *)ptr)->Normale = (U16)value;
    else if (info->face_type == MODEL_FACE_TEXTURE)
        ((ModelPoly3Texture *)ptr)->Normale = (U16)value;
    else if (info->face_type == MODEL_FACE_ENV AND info->is_quad)
        ((ModelPoly4Env *)ptr)->Normale = (U16)value;
    else if (info->face_type == MODEL_FACE_ENV)
        ((ModelPoly3Env *)ptr)->Normale = (U16)value;
    else if (info->is_quad)
        ((ModelPoly4Light *)ptr)->Normale = (U16)value;
    else
        ((ModelPoly3Light *)ptr)->Normale = (U16)value;
}

static void ModelWriteFaceType(T_BODY_HEADER *body, const ModelFaceInfo *info, S32 value) {
    ModelPolyHeader *header;

    if (!ModelFaceValid(info, body))
        return;
    if (value < 0)
        value = 0;
    if (value > MODEL_MAX_POLY_TYPE)
        value = MODEL_MAX_POLY_TYPE;
    header = (ModelPolyHeader *)((U8 *)body + info->header_offset);
    header->TypePoly = (U16)((header->TypePoly & 0xff00) | (value & 0xff));
}

static void ModelApplyFaceEdit(T_BODY_HEADER *body, S32 target, S32 value) {
    S32 i;

    if (!body OR !ModelFaceSelection)
        return;
    for (i = 0; i < body->NbPolys AND i < ModelFaceSelectionCapacity; i++) {
        ModelFaceInfo info;

        if (!ModelFaceSelection[i])
            continue;
        if (!ModelFindFaceByIndex(body, i, &info))
            continue;
        if (target == MODEL_NUMBER_FACE_TYPE)
            ModelWriteFaceType(body, &info, value);
        else if (target == MODEL_NUMBER_FACE_COLOR)
            ModelWriteFaceColor(body, &info, value);
        else if (target == MODEL_NUMBER_FACE_NORMAL)
            ModelWriteFaceNormal(body, &info, value);
    }
    if (ModelSelectedFace >= 0)
        ModelFindFaceByIndex(body, ModelSelectedFace, &ModelSelectedFaceInfo);
}

static void ModelInitFaceSelectionStats(ModelFaceSelectionStats *stats) {
    memset(stats, 0, sizeof(*stats));
    stats->type_value = -1;
    stats->color_value = -1;
    stats->raw_color_value = -1;
    stats->normal_value = -1;
    stats->texture_value = -1;
    stats->point_count_value = -1;
}

static void ModelAccumulateFaceSelectionStats(ModelFaceSelectionStats *stats, const ModelFaceInfo *info) {
    S32 color = info->color & 0xff;

    if (stats->count == 0) {
        stats->type_value = info->type_poly;
        stats->color_value = color;
        stats->raw_color_value = info->color;
        stats->normal_value = info->normal;
        stats->texture_value = info->texture;
        stats->point_count_value = info->point_count;
    } else {
        if (stats->type_value != info->type_poly)
            stats->type_mixed = TRUE;
        if (stats->color_value != color)
            stats->color_mixed = TRUE;
        if (stats->raw_color_value != info->color)
            stats->raw_color_mixed = TRUE;
        if (stats->normal_value != info->normal)
            stats->normal_mixed = TRUE;
        if (stats->texture_value != info->texture)
            stats->texture_mixed = TRUE;
        if (stats->point_count_value != info->point_count)
            stats->point_count_mixed = TRUE;
    }
    stats->count++;
}

static void ModelCollectFaceSelectionStats(T_BODY_HEADER *body, ModelFaceSelectionStats *stats) {
    S32 i;

    ModelInitFaceSelectionStats(stats);
    if (!body OR !ModelFaceSelection)
        return;
    for (i = 0; i < body->NbPolys AND i < ModelFaceSelectionCapacity; i++) {
        ModelFaceInfo info;

        if (!ModelFaceSelection[i])
            continue;
        if (ModelFindFaceByIndex(body, i, &info))
            ModelAccumulateFaceSelectionStats(stats, &info);
    }
}

static void ModelInitSphereSelectionStats(ModelLineSelectionStats *stats) {
    memset(stats, 0, sizeof(*stats));
    stats->type_value = -1;
    stats->color_value = -1;
    stats->raw_color_value = -1;
    stats->p1_value = -1;
    stats->p2_value = -1;
}

static void ModelAccumulateSphereSelectionStats(ModelLineSelectionStats *stats, const ModelSphereInfo *info) {
    S32 color = info->color & 0xff;

    if (stats->count == 0) {
        stats->type_value = info->type;
        stats->color_value = color;
        stats->raw_color_value = info->color;
        stats->p1_value = info->point;
        stats->p2_value = info->radius;
    } else {
        if (stats->type_value != info->type)
            stats->type_mixed = TRUE;
        if (stats->color_value != color)
            stats->color_mixed = TRUE;
        if (stats->raw_color_value != info->color)
            stats->raw_color_mixed = TRUE;
        if (stats->p1_value != info->point)
            stats->p1_mixed = TRUE;
        if (stats->p2_value != info->radius)
            stats->p2_mixed = TRUE;
    }
    stats->count++;
}

static void ModelCollectSphereSelectionStats(T_BODY_HEADER *body, ModelLineSelectionStats *stats) {
    S32 i;

    ModelInitSphereSelectionStats(stats);
    if (!body OR !ModelSphereSelection)
        return;
    for (i = 0; i < body->NbSpheres AND i < ModelSphereSelectionCapacity; i++) {
        ModelSphereInfo info;

        if (!ModelSphereSelection[i])
            continue;
        if (ModelReadSphereInfo(body, i, &info))
            ModelAccumulateSphereSelectionStats(stats, &info);
    }
}

static void ModelInitLineSelectionStats(ModelLineSelectionStats *stats) {
    memset(stats, 0, sizeof(*stats));
    stats->type_value = -1;
    stats->color_value = -1;
    stats->raw_color_value = -1;
    stats->p1_value = -1;
    stats->p2_value = -1;
}

static void ModelAccumulateLineSelectionStats(ModelLineSelectionStats *stats, const ModelLineInfo *info) {
    S32 color = info->color & 0xff;

    if (stats->count == 0) {
        stats->type_value = info->type;
        stats->color_value = color;
        stats->raw_color_value = info->color;
        stats->p1_value = info->p1;
        stats->p2_value = info->p2;
    } else {
        if (stats->type_value != info->type)
            stats->type_mixed = TRUE;
        if (stats->color_value != color)
            stats->color_mixed = TRUE;
        if (stats->raw_color_value != info->color)
            stats->raw_color_mixed = TRUE;
        if (stats->p1_value != info->p1)
            stats->p1_mixed = TRUE;
        if (stats->p2_value != info->p2)
            stats->p2_mixed = TRUE;
    }
    stats->count++;
}

static void ModelCollectLineSelectionStats(T_BODY_HEADER *body, ModelLineSelectionStats *stats) {
    S32 i;

    ModelInitLineSelectionStats(stats);
    if (!body OR !ModelLineSelection)
        return;
    for (i = 0; i < body->NbLines AND i < ModelLineSelectionCapacity; i++) {
        ModelLineInfo info;

        if (!ModelLineSelection[i])
            continue;
        if (ModelReadLineInfo(body, i, &info))
            ModelAccumulateLineSelectionStats(stats, &info);
    }
}

static void ModelInitBoneSelectionStats(ModelBoneSelectionStats *stats) {
    memset(stats, 0, sizeof(*stats));
    stats->parent_value = -1;
    stats->point_value = -1;
    stats->nb_pts_value = -1;
    stats->nb_norm_value = -1;
}

static void ModelAccumulateBoneSelectionStats(ModelBoneSelectionStats *stats, const ModelGroupInfo *group) {
    if (stats->count == 0) {
        stats->parent_value = group->OrgGroupe;
        stats->point_value = group->OrgPoint;
        stats->nb_pts_value = group->NbPts;
        stats->nb_norm_value = group->NbNorm;
    } else {
        if (stats->parent_value != group->OrgGroupe)
            stats->parent_mixed = TRUE;
        if (stats->point_value != group->OrgPoint)
            stats->point_mixed = TRUE;
        if (stats->nb_pts_value != group->NbPts)
            stats->nb_pts_mixed = TRUE;
        if (stats->nb_norm_value != group->NbNorm)
            stats->nb_norm_mixed = TRUE;
    }
    stats->count++;
}

static void ModelCollectBoneSelectionStats(T_BODY_HEADER *body, ModelBoneSelectionStats *stats) {
    ModelGroupInfo *groups;
    S32 i;

    ModelInitBoneSelectionStats(stats);
    if (!body OR !ModelBoneSelection OR body->NbGroupes <= 0)
        return;
    groups = (ModelGroupInfo *)((U8 *)body + body->OffGroupes);
    for (i = 0; i < body->NbGroupes AND i < ModelBoneSelectionCapacity; i++) {
        if (!ModelBoneSelection[i])
            continue;
        ModelAccumulateBoneSelectionStats(stats, groups + i);
    }
}

static void ModelWriteBoneField(T_BODY_HEADER *body, S32 index, S32 target, S32 value) {
    ModelGroupInfo *groups;

    if (!body OR index < 0 OR index >= body->NbGroupes)
        return;
    value = ModelClampEditValue(body, target, value);
    groups = (ModelGroupInfo *)((U8 *)body + body->OffGroupes);
    if (target == MODEL_NUMBER_BONE_PARENT)
        groups[index].OrgGroupe = (U16)value;
    else if (target == MODEL_NUMBER_BONE_ORG_POINT)
        groups[index].OrgPoint = (U16)value;
}

static void ModelApplyBoneField(T_BODY_HEADER *body, S32 target, S32 value) {
    S32 i;

    if (!body OR !ModelBoneSelection)
        return;
    for (i = 0; i < body->NbGroupes AND i < ModelBoneSelectionCapacity; i++) {
        if (!ModelBoneSelection[i])
            continue;
        ModelWriteBoneField(body, i, target, value);
    }
}

static void ModelSelectBoneIndex(T_BODY_HEADER *body, S32 value) {
    if (!body)
        return;
    value = ModelClampEditValue(body, MODEL_NUMBER_BONE_INDEX, value);
    ModelSetSingleBoneSelection(body, value);
}

static void ModelSetSingleVertexSelection(T_BODY_HEADER *body, S32 index) {
    if (!body OR !ModelVertexSelection)
        return;
    ModelClearVertexSelection();
    if (index >= 0 AND index < body->NbPoints AND index < ModelVertexSelectionCapacity) {
        ModelVertexSelection[index] = 1;
        ModelSelectedVertexCount = 1;
        ModelSelectedVertex = index;
    }
}

static void ModelToggleVertexSelection(T_BODY_HEADER *body, S32 index) {
    if (!body OR !ModelVertexSelection)
        return;
    if (index < 0 OR index >= body->NbPoints OR index >= ModelVertexSelectionCapacity)
        return;
    if (ModelVertexSelection[index]) {
        ModelVertexSelection[index] = 0;
        if (ModelSelectedVertexCount > 0)
            ModelSelectedVertexCount--;
        if (ModelSelectedVertex == index)
            ModelSelectedVertex = -1;
    } else {
        ModelVertexSelection[index] = 1;
        ModelSelectedVertexCount++;
        ModelSelectedVertex = index;
    }
}

static void ModelSetSingleFaceSelection(T_BODY_HEADER *body, S32 index, const ModelFaceInfo *info) {
    if (!body OR !ModelFaceSelection)
        return;
    ModelClearFaceSelection();
    if (index >= 0 AND index < body->NbPolys AND index < ModelFaceSelectionCapacity) {
        ModelFaceSelection[index] = 1;
        ModelSelectedFaceCount = 1;
        ModelSelectedFace = index;
        ModelSelectedFaceInfo = *info;
    }
}

static void ModelSelectAllCurrent(T_BODY_HEADER *body) {
    S32 i;
    S32 all_selected = TRUE;
    S32 point_count;

    if (!body)
        return;
    point_count = body->NbPoints < MODEL_MAX_POINTS ? body->NbPoints : MODEL_MAX_POINTS;
    if (ModelVertexSelectionCapacity < point_count)
        point_count = ModelVertexSelectionCapacity;
    if (ModelSelectActive(MODEL_SELECT_VERTEX) AND ModelSelectedVertexCount != point_count)
        all_selected = FALSE;
    if (ModelSelectActive(MODEL_SELECT_FACE) AND ModelSelectedFaceCount != body->NbPolys)
        all_selected = FALSE;
    if (ModelSelectActive(MODEL_SELECT_SPHERE) AND ModelSelectedSphereCount != body->NbSpheres)
        all_selected = FALSE;
    if (ModelSelectActive(MODEL_SELECT_LINE) AND ModelSelectedLineCount != body->NbLines)
        all_selected = FALSE;
    if (ModelSelectActive(MODEL_SELECT_BONE) AND ModelSelectedBoneCount != body->NbGroupes)
        all_selected = FALSE;

    if (all_selected) {
        if (ModelSelectActive(MODEL_SELECT_VERTEX))
            ModelClearVertexSelection();
        if (ModelSelectActive(MODEL_SELECT_FACE))
            ModelClearFaceSelection();
        if (ModelSelectActive(MODEL_SELECT_SPHERE))
            ModelClearSphereSelection();
        if (ModelSelectActive(MODEL_SELECT_LINE))
            ModelClearLineSelection();
        if (ModelSelectActive(MODEL_SELECT_BONE))
            ModelClearBoneSelection();
        return;
    }

    if (ModelSelectActive(MODEL_SELECT_VERTEX)) {
        ModelClearVertexSelection();
        for (i = 0; i < point_count AND i < ModelVertexSelectionCapacity; i++)
            ModelVertexSelection[i] = 1;
        ModelSelectedVertexCount = point_count;
        ModelSelectedVertex = point_count > 0 ? point_count - 1 : -1;
    }
    if (ModelSelectActive(MODEL_SELECT_FACE)) {
        ModelClearFaceSelection();
        for (i = 0; i < body->NbPolys AND i < ModelFaceSelectionCapacity; i++) {
            ModelFaceInfo info;

            ModelFaceSelection[i] = 1;
            if (ModelFindFaceByIndex(body, i, &info)) {
                ModelSelectedFace = i;
                ModelSelectedFaceInfo = info;
            }
        }
        ModelSelectedFaceCount = body->NbPolys;
    }
    if (ModelSelectActive(MODEL_SELECT_SPHERE)) {
        ModelClearSphereSelection();
        for (i = 0; i < body->NbSpheres AND i < ModelSphereSelectionCapacity; i++) {
            ModelSphereInfo info;

            ModelSphereSelection[i] = 1;
            if (ModelReadSphereInfo(body, i, &info)) {
                ModelSelectedSphere = i;
                ModelSelectedSphereInfo = info;
            }
        }
        ModelSelectedSphereCount = body->NbSpheres;
    }
    if (ModelSelectActive(MODEL_SELECT_LINE)) {
        ModelClearLineSelection();
        for (i = 0; i < body->NbLines AND i < ModelLineSelectionCapacity; i++) {
            ModelLineInfo info;

            ModelLineSelection[i] = 1;
            if (ModelReadLineInfo(body, i, &info)) {
                ModelSelectedLine = i;
                ModelSelectedLineInfo = info;
            }
        }
        ModelSelectedLineCount = body->NbLines;
    }
    if (ModelSelectActive(MODEL_SELECT_BONE)) {
        ModelClearBoneSelection();
        for (i = 0; i < body->NbGroupes AND i < ModelBoneSelectionCapacity; i++) {
            ModelBoneSelection[i] = 1;
            ModelSelectedBone = i;
        }
        ModelSelectedBoneCount = body->NbGroupes;
    }
}

