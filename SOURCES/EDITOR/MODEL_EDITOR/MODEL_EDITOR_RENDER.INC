static void ModelDrawBoxSelection(void) {
    S32 x0;
    S32 y0;
    S32 x1;
    S32 y1;

    if (!ModelBoxSelectMode)
        return;
    if (!ModelBoxSelecting)
        return;
    ModelBoxBounds(&x0, &y0, &x1, &y1);
    Line(x0, y0, x1, y0, LBAWHITE);
    Line(x1, y0, x1, y1, LBAWHITE);
    Line(x1, y1, x0, y1, LBAWHITE);
    Line(x0, y1, x0, y0, LBAWHITE);
}

static void ModelDrawFaceOutline(const ModelFaceInfo *info, T_BODY_HEADER *body, S32 color) {
    S32 i;

    if (!ModelFaceValid(info, body))
        return;
    for (i = 0; i < info->point_count; i++) {
        U16 p0 = info->points[i];
        U16 p1 = info->points[(i + 1) % info->point_count];
        Line(Obj_ListProjectedPoints[p0].X, Obj_ListProjectedPoints[p0].Y,
             Obj_ListProjectedPoints[p1].X, Obj_ListProjectedPoints[p1].Y, color);
        Line(Obj_ListProjectedPoints[p0].X + 1, Obj_ListProjectedPoints[p0].Y,
             Obj_ListProjectedPoints[p1].X + 1, Obj_ListProjectedPoints[p1].Y, color);
    }
}

static S32 ModelLerpX(S32 x0, S32 y0, S32 x1, S32 y1, S32 y) {
    if (y1 == y0)
        return x0;
    return x0 + (S32)(((S64)(x1 - x0) * (y - y0)) / (y1 - y0));
}

static void ModelFillTri(U16 p0, U16 p1, U16 p2, S32 color) {
    S32 x[3];
    S32 y[3];
    S32 min_y;
    S32 max_y;
    S32 sy;

    x[0] = Obj_ListProjectedPoints[p0].X;
    y[0] = Obj_ListProjectedPoints[p0].Y;
    x[1] = Obj_ListProjectedPoints[p1].X;
    y[1] = Obj_ListProjectedPoints[p1].Y;
    x[2] = Obj_ListProjectedPoints[p2].X;
    y[2] = Obj_ListProjectedPoints[p2].Y;

    min_y = y[0];
    max_y = y[0];
    if (y[1] < min_y)
        min_y = y[1];
    if (y[2] < min_y)
        min_y = y[2];
    if (y[1] > max_y)
        max_y = y[1];
    if (y[2] > max_y)
        max_y = y[2];
    if (min_y < 0)
        min_y = 0;
    if (max_y >= (S32)ModeDesiredY)
        max_y = (S32)ModeDesiredY - 1;

    for (sy = min_y; sy <= max_y; sy++) {
        S32 ix[2];
        S32 count = 0;
        S32 i;

        for (i = 0; i < 3; i++) {
            S32 j = (i + 1) % 3;

            if ((y[i] <= sy AND y[j] > sy) OR(y[j] <= sy AND y[i] > sy)) {
                if (count < 2) {
                    ix[count] = ModelLerpX(x[i], y[i], x[j], y[j], sy);
                    count++;
                }
            }
        }
        if (count == 2) {
            S32 xa = ix[0];
            S32 xb = ix[1];

            if (xa > xb) {
                S32 tmp = xa;
                xa = xb;
                xb = tmp;
            }
            if (xa < 0)
                xa = 0;
            if (xb >= (S32)ModeDesiredX)
                xb = (S32)ModeDesiredX - 1;
            if (xa <= xb)
                Line(xa, sy, xb, sy, color);
        }
    }
}

static void ModelFillFace(const ModelFaceInfo *info, T_BODY_HEADER *body, S32 color) {
    if (!ModelFaceValid(info, body))
        return;
    ModelFillTri(info->points[0], info->points[1], info->points[2], color);
    if (info->point_count == 4)
        ModelFillTri(info->points[0], info->points[2], info->points[3], color);
}

static void ModelDrawFaceOutlinesPass(T_BODY_HEADER *body, S32 selected_pass) {
    U8 *lines = (U8 *)body + body->OffLines;
    U8 *current = (U8 *)body + body->OffPolys;
    S32 body_offset = body->OffPolys;
    S32 face_index = 0;

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

            ModelReadFaceInfo(current, face_index, header_offset, body_offset, header->TypePoly, face_type, is_quad, &info);
            if (ModelFaceVisibleInOverlay(&info, body)) {
                if (ModelFaceSelected(face_index)) {
                    if (selected_pass)
                        ModelDrawFaceOutline(&info, body, LBAWHITE);
                } else if (!selected_pass) {
                    ModelDrawFaceOutline(&info, body, MODEL_WIRE_COLOR);
                }
            }
            current += size;
            body_offset += size;
            face_index++;
        }
    }
}

static void ModelDrawAllFaceOutlines(T_BODY_HEADER *body) {
    ModelDrawFaceOutlinesPass(body, FALSE);
    ModelDrawFaceOutlinesPass(body, TRUE);
}

static void ModelDrawVertices(T_BODY_HEADER *body) {
    S32 i;

    if (!body)
        return;
    for (i = 0; i < body->NbPoints AND i < MODEL_MAX_POINTS; i++) {
        S32 color = ModelVertexSelected(i) ? 12 : 15;
        S32 x = Obj_ListProjectedPoints[i].X;
        S32 y = Obj_ListProjectedPoints[i].Y;

        Box(x - 1, y - 1, x + 1, y + 1, color);
    }
}

static void ModelDrawOneSphereSelection(const ModelSphereInfo *info) {
    S32 x;
    S32 y;
    S32 r;

    x = Obj_ListProjectedPoints[info->point].X;
    y = Obj_ListProjectedPoints[info->point].Y;
    r = ModelSpherePickRadius(info) + 2;
    Line(x - r, y - r, x + r, y - r, LBAWHITE);
    Line(x + r, y - r, x + r, y + r, LBAWHITE);
    Line(x + r, y + r, x - r, y + r, LBAWHITE);
    Line(x - r, y + r, x - r, y - r, LBAWHITE);
    Line(x - 2, y, x + 2, y, LBAWHITE);
    Line(x, y - 2, x, y + 2, LBAWHITE);
}

static void ModelDrawSphereSelection(T_BODY_HEADER *body) {
    S32 i;

    if (!body OR !ModelSphereSelection)
        return;
    for (i = 0; i < body->NbSpheres AND i < ModelSphereSelectionCapacity; i++) {
        ModelSphereInfo info;

        if (!ModelSphereSelection[i])
            continue;
        if (ModelReadSphereInfo(body, i, &info))
            ModelDrawOneSphereSelection(&info);
    }
}

static void ModelDrawLineSelection(T_BODY_HEADER *body) {
    S32 i;

    if (!body OR !ModelLineSelection)
        return;
    for (i = 0; i < body->NbLines AND i < ModelLineSelectionCapacity; i++) {
        ModelLineInfo info;

        if (!ModelLineSelection[i])
            continue;
        if (!ModelReadLineInfo(body, i, &info))
            continue;
        Line(Obj_ListProjectedPoints[info.p1].X, Obj_ListProjectedPoints[info.p1].Y,
             Obj_ListProjectedPoints[info.p2].X, Obj_ListProjectedPoints[info.p2].Y, LBAWHITE);
        Line(Obj_ListProjectedPoints[info.p1].X + 1, Obj_ListProjectedPoints[info.p1].Y,
             Obj_ListProjectedPoints[info.p2].X + 1, Obj_ListProjectedPoints[info.p2].Y, LBAWHITE);
    }
}

static S32 ModelBoneOriginPoint(T_BODY_HEADER *body, S32 index) {
    ModelGroupInfo *groups;

    if (!body OR index < 0 OR index >= body->NbGroupes)
        return -1;
    groups = (ModelGroupInfo *)((U8 *)body + body->OffGroupes);
    if (groups[index].OrgPoint >= body->NbPoints OR groups[index].OrgPoint >= MODEL_PROJECTED_POINT_LIMIT)
        return -1;
    return groups[index].OrgPoint;
}

static void ModelDrawBones(T_BODY_HEADER *body) {
    ModelGroupInfo *groups;
    S32 i;

    if (!body OR body->NbGroupes <= 0)
        return;
    if (ModelArchiveBase(ModelArchive) != MODEL_ARCHIVE_BODY)
        return;
    groups = (ModelGroupInfo *)((U8 *)body + body->OffGroupes);
    for (i = 0; i < body->NbGroupes; i++) {
        S32 point = ModelBoneOriginPoint(body, i);
        S32 color = ModelBoneSelected(i) ? LBAWHITE : MODEL_WIRE_COLOR;

        if (point < 0)
            continue;
        if (i > 0 AND groups[i].OrgGroupe < body->NbGroupes) {
            S32 parent_point = ModelBoneOriginPoint(body, groups[i].OrgGroupe);

            if (parent_point >= 0) {
                Line(Obj_ListProjectedPoints[parent_point].X, Obj_ListProjectedPoints[parent_point].Y,
                     Obj_ListProjectedPoints[point].X, Obj_ListProjectedPoints[point].Y, color);
                if (ModelBoneSelected(i))
                    Line(Obj_ListProjectedPoints[parent_point].X + 1, Obj_ListProjectedPoints[parent_point].Y,
                         Obj_ListProjectedPoints[point].X + 1, Obj_ListProjectedPoints[point].Y, color);
            }
        }
        Box(Obj_ListProjectedPoints[point].X - 2, Obj_ListProjectedPoints[point].Y - 2,
            Obj_ListProjectedPoints[point].X + 2, Obj_ListProjectedPoints[point].Y + 2, color);
    }
}

static S32 ModelFindBoneAt(T_BODY_HEADER *body, S32 x, S32 y) {
    S32 i;
    S32 best = -1;
    S32 best_dist = 10 * 10;

    if (!body OR ModelArchiveBase(ModelArchive) != MODEL_ARCHIVE_BODY)
        return -1;
    for (i = 0; i < body->NbGroupes; i++) {
        S32 point = ModelBoneOriginPoint(body, i);
        S32 dx;
        S32 dy;
        S32 dist;

        if (point < 0)
            continue;
        dx = Obj_ListProjectedPoints[point].X - x;
        dy = Obj_ListProjectedPoints[point].Y - y;
        dist = dx * dx + dy * dy;
        if (dist <= best_dist) {
            best = i;
            best_dist = dist;
        }
    }
    return best;
}

static S32 ModelProjectPoint(S32 x, S32 y, S32 z, S32 *screen_x, S32 *screen_y) {
    LongWorldRotatePoint(x, y, z);
    if (!LongProjectPoint(X0, Y0, Z0))
        return FALSE;
    *screen_x = Xp;
    *screen_y = Yp;
    return TRUE;
}

static void ModelDrawGrid(void) {
    S32 i;

    if (!ModelFloorGrid)
        return;

    SaveCamera();
    ModelApplyCamera();
    for (i = -12; i <= 12; i++) {
        S32 x0;
        S32 y0;
        S32 x1;
        S32 y1;
        S32 p = i * 500;

        if (ModelProjectPoint(-6000, 0, p, &x0, &y0) AND ModelProjectPoint(6000, 0, p, &x1, &y1))
            Line(x0, y0, x1, y1, MODEL_WIRE_COLOR);
        if (ModelProjectPoint(p, 0, -6000, &x0, &y0) AND ModelProjectPoint(p, 0, 6000, &x1, &y1))
            Line(x0, y0, x1, y1, MODEL_WIRE_COLOR);
    }
    RestoreCamera();
}

static S32 ModelDrawBody(T_BODY_HEADER *body) {
    U8 *old_obj_ptr_map;
    Func_TransNumPtr *old_trans_fct_body;
    S32 ret;

    if (!body OR !BufferTexture)
        return FALSE;

    SaveCamera();
    ModelApplyCamera();
    old_obj_ptr_map = ObjPtrMap;
    old_trans_fct_body = TransFctBody;
    UnsetClip();
    SetCLUT(PalLevel);

    if (ModelArchiveBase(ModelArchive) == MODEL_ARCHIVE_BODY) {
        T_OBJ_3D obj;
        T_OBJ_3D *display_obj = NULL;

        if (ModelMode == MODEL_MODE_VIEW)
            display_obj = ModelAnimPreviewObject(body);
        if (!display_obj) {
            memset(&obj, 0, sizeof(obj));
            obj.Body.Ptr = body;
            obj.Texture = BufferTexture;
            obj.NbGroups = body->NbGroupes;
            display_obj = &obj;
        }
        TransFctBody = NULL;
        ret = ObjectDisplay(display_obj);
        TransFctBody = old_trans_fct_body;
    } else {
        ObjPtrMap = BufferTexture;
        ret = BodyDisplay(0, 0, 0, 0, 0, 0, body);
    }

    ObjPtrMap = old_obj_ptr_map;
    TransFctBody = old_trans_fct_body;
    RestoreCamera();

    if (ret AND ModelMode == MODEL_MODE_EDIT) {
        ModelApplyEditorPalette();
        if (ModelSelectActive(MODEL_SELECT_VERTEX))
            ModelDrawVertices(body);
        if (ModelSelectActive(MODEL_SELECT_FACE))
            ModelDrawAllFaceOutlines(body);
        if (ModelSelectActive(MODEL_SELECT_SPHERE))
            ModelDrawSphereSelection(body);
        if (ModelSelectActive(MODEL_SELECT_LINE))
            ModelDrawLineSelection(body);
        if (ModelSelectActive(MODEL_SELECT_BONE))
            ModelDrawBones(body);
    }
    return ret ? TRUE : FALSE;
}

static void ModelCollectBodyColors(T_BODY_HEADER *body, U8 colors[256]) {
    U8 *lines;
    U8 *current;
    S32 body_offset;
    S32 face_index = 0;

    memset(colors, 0, 256);
    if (!body)
        return;
    lines = (U8 *)body + body->OffLines;
    current = (U8 *)body + body->OffPolys;
    body_offset = body->OffPolys;

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

            ModelReadFaceInfo(current, face_index, header_offset, body_offset, header->TypePoly, face_type, is_quad, &info);
            if (ModelFaceValid(&info, body))
                colors[info.color & 0xff] = 1;
            current += size;
            body_offset += size;
            face_index++;
        }
    }
}

static S32 ModelPrepareColorPanel(T_BODY_HEADER *body, ModelColorPanelLayout *layout) {
    U8 colors[256];
    S32 i;
    S32 digit_w = 8;
    S32 max_digits = 1;
    S32 rows;
    S32 max_panel_width;

    if (!body OR ModelMode != MODEL_MODE_EDIT OR !ModelSelectActive(MODEL_SELECT_FACE))
        return FALSE;

    memset(layout, 0, sizeof(*layout));
    layout->swatch_w = 8;
    layout->swatch_h = 9;
    layout->row_height = 11;
    layout->x0 = 8;

    ModelCollectBodyColors(body, colors);
    for (i = 0; i < 256; i++) {
        if (colors[i])
            layout->list[layout->count++] = i;
    }
    if (layout->count <= 0)
        return FALSE;

    for (i = 0; i < layout->count; i++) {
        if (layout->list[i] >= 100)
            max_digits = 3;
        else if (layout->list[i] >= 10 AND max_digits < 2)
            max_digits = 2;
    }
    layout->item_width = layout->swatch_w + 3 + max_digits * digit_w + 7;
    max_panel_width = (S32)ModeDesiredX - layout->x0 - 8;
    layout->per_row = (max_panel_width - 14) / layout->item_width;
    if (layout->per_row > 6)
        layout->per_row = 6;
    if (layout->per_row < 1)
        layout->per_row = 1;
    if (layout->per_row > layout->count)
        layout->per_row = layout->count;
    rows = (layout->count + layout->per_row - 1) / layout->per_row;
    layout->width = layout->per_row * layout->item_width + 14;
    layout->height = rows * layout->row_height + 14;
    layout->y0 = (S32)ModeDesiredY - layout->height - 8;
    if (layout->y0 < 8)
        layout->y0 = 8;
    return TRUE;
}

static void ModelDrawColorPanel(T_BODY_HEADER *body) {
    ModelColorPanelLayout layout;
    S32 i;

    if (!ModelPrepareColorPanel(body, &layout))
        return;

    ShadeBoxBlk(layout.x0, layout.y0, layout.x0 + layout.width, layout.y0 + layout.height, MENU_SHADE_LVL);
    DrawCadre(layout.x0, layout.y0, layout.x0 + layout.width, layout.y0 + layout.height, ALL_ANGLES);
    for (i = 0; i < layout.count; i++) {
        S32 col = layout.list[i];
        S32 row = i / layout.per_row;
        S32 column = i % layout.per_row;
        S32 x = layout.x0 + 7 + column * layout.item_width;
        S32 y = layout.y0 + 7 + row * layout.row_height;

        Box(x, y, x + layout.swatch_w - 1, y + layout.swatch_h - 1, col);
        CoulText(LBAWHITE, -1);
        GraphPrintf(FALSE, x + layout.swatch_w + 3, y - 1, "%d", (int)col);
    }
    /* snprintf(text, sizeof(text), "%d colors", (int)count);
       CoulText(MODEL_TEXT_MUTED, -1);
       GraphPrintf(FALSE, x0 + 9, y0 + height - 12, "%s", text); */
}

static S32 ModelColorPanelHit(T_BODY_HEADER *body, S32 x, S32 y, S32 *out_color) {
    ModelColorPanelLayout layout;
    S32 i;

    if (!ModelPrepareColorPanel(body, &layout))
        return FALSE;
    if (x < layout.x0 OR x > layout.x0 + layout.width OR y < layout.y0 OR y > layout.y0 + layout.height)
        return FALSE;
    for (i = 0; i < layout.count; i++) {
        S32 row = i / layout.per_row;
        S32 column = i % layout.per_row;
        S32 item_x0 = layout.x0 + 7 + column * layout.item_width;
        S32 item_y0 = layout.y0 + 7 + row * layout.row_height;
        S32 item_x1 = item_x0 + layout.item_width - 1;
        S32 item_y1 = item_y0 + layout.row_height - 1;

        if (x >= item_x0 AND x <= item_x1 AND y >= item_y0 AND y <= item_y1) {
            *out_color = layout.list[i];
            return TRUE;
        }
    }
    return FALSE;
}

static void ModelDrawField(S32 x0, S32 x1, S32 *y, S32 selected, const char *fmt, const char *value) {
    if (selected)
        DrawCadre(x0 + 4, *y - 3, x1 - 4, *y + 10, ALL_ANGLES);
    GraphPrintf(FALSE, x0 + 9, *y, fmt, value);
    *y += 13;
}

static const char *ModelSelectModeName(void) {
    if (ModelSelectMode == MODEL_SELECT_VERTEX)
        return "Vertex";
    if (ModelSelectMode == MODEL_SELECT_FACE)
        return "Face";
    if (ModelSelectMode == MODEL_SELECT_SPHERE)
        return "Sphere";
    if (ModelSelectMode == MODEL_SELECT_BONE)
        return "Bone";
    return "Line";
}

static const char *ModelFaceTypeLabel(S32 type) {
    static const char *labels[MODEL_MAX_POLY_TYPE + 1] = {
        "Solid",
        "Flat",
        "Solid",
        "Solid",
        "Gouraud",
        "Gouraud",
        "Gouraud",
        "Gouraud",
        "Texture Solid",
        "Texture Flat",
        "Texture Gouraud",
        "Texture Gouraud",
        "Texture Solid",
        "Texture Flat",
        "Texture Gouraud",
        "Texture Gouraud",
        "TextureZ Solid",
        "TextureZ Flat",
        "TextureZ Gouraud",
        "TextureZ Gouraud",
        "TextureZ Solid",
        "TextureZ Flat",
        "TextureZ Gouraud",
        "TextureZ Gouraud"
    };

    if (type < 0 OR type > MODEL_MAX_POLY_TYPE)
        return "Unknown";
    return labels[type];
}

static void ModelFormatSelectedSummary(char *out_text, S32 out_max, S32 selected, S32 total, const char *singular, const char *plural) {
    snprintf(out_text, out_max, "%d of %d %s selected", (int)selected, (int)total, total == 1 ? singular : plural);
}

static void ModelDrawSelectionSummaryLine(S32 x, S32 *y, S32 selected, S32 total, const char *singular, const char *plural) {
    char text[64];

    ModelFormatSelectedSummary(text, sizeof(text), selected, total, singular, plural);
    CoulText(MODEL_TEXT_MUTED, -1);
    GraphPrintf(FALSE, x, *y, "%s", text);
    *y += 12;
}

static void ModelDrawActiveSelectionSummaries(T_BODY_HEADER *body, S32 x, S32 *y) {
    if (ModelSelectActive(MODEL_SELECT_VERTEX))
        ModelDrawSelectionSummaryLine(x, y, ModelSelectedVertexCount, body ? body->NbPoints : 0, "vertex", "vertices");
    if (ModelSelectActive(MODEL_SELECT_FACE))
        ModelDrawSelectionSummaryLine(x, y, ModelSelectedFaceCount, body ? body->NbPolys : 0, "face", "faces");
    if (ModelSelectActive(MODEL_SELECT_SPHERE))
        ModelDrawSelectionSummaryLine(x, y, ModelSelectedSphereCount, body ? body->NbSpheres : 0, "sphere", "spheres");
    if (ModelSelectActive(MODEL_SELECT_LINE))
        ModelDrawSelectionSummaryLine(x, y, ModelSelectedLineCount, body ? body->NbLines : 0, "line", "lines");
    if (ModelSelectActive(MODEL_SELECT_BONE))
        ModelDrawSelectionSummaryLine(x, y, ModelSelectedBoneCount, body ? body->NbGroupes : 0, "bone", "bones");
    CoulText(LBAWHITE, -1);
}

static void ModelDrawInspector(T_BODY_HEADER *body) {
    S32 x0 = 8;
    S32 y0 = 8;
    S32 x1 = 238;
    S32 y1 = ModelMode == MODEL_MODE_VIEW ? 182 : 232;
    S32 y = y0 + 10;
    char value[64];

    if (!ModelInspectorVisible)
        return;
    if (x1 >= ModeDesiredX)
        x1 = ModeDesiredX - 8;

    ShadeBoxBlk(x0, y0, x1, y1, MENU_SHADE_LVL);
    DrawCadre(x0, y0, x1, y1, ModelCameraMode == MODEL_CAMERA_FREE_NAV ? ALL_ANGLES : ALL_ANGLES | CADRE_RED);
    CoulText(LBAWHITE, -1);

    if (ModelMode == MODEL_MODE_VIEW) {
        ModelDrawField(x0, x1, &y, ModelInspectorField == MODEL_FIELD_ARCHIVE, "Archive: %s", ModelArchiveName());
        CoulText(MODEL_TEXT_MUTED, -1);
        GraphPrintf(FALSE, x0 + 9, y, "Entries: %d", (int)ModelArchiveCount());
        y += 18;
        CoulText(LBAWHITE, -1);
        if (ModelEditingNumber AND ModelNumberTarget == MODEL_NUMBER_BODY_ID) {
            snprintf(value, sizeof(value), "%s_", ModelNumberText);
        } else {
            snprintf(value, sizeof(value), "%d", (int)ModelBodyId);
        }
        ModelDrawField(x0, x1, &y, ModelInspectorField == MODEL_FIELD_BODY_ID, "Body ID: %s", value);
        ModelRefreshAnimChoices(body);
        if (ModelArchiveBase(ModelArchive) == MODEL_ARCHIVE_BODY) {
            if (ModelEditingNumber AND ModelNumberTarget == MODEL_NUMBER_ANIM_ID)
                snprintf(value, sizeof(value), "%s_", ModelNumberText);
            else if (ModelAnimId >= 0)
                snprintf(value, sizeof(value), "%d", (int)ModelAnimId);
            else
                snprintf(value, sizeof(value), "-1");
            ModelDrawField(x0, x1, &y, ModelInspectorField == MODEL_FIELD_ANIM_ID, "Animation: %s", value);
            CoulText(MODEL_TEXT_MUTED, -1);
            GraphPrintf(FALSE, x0 + 9, y, "%s: %d", ModelAnimChoiceFromFiche ? "Fiche anims" : "Animations", (int)ModelAnimChoiceCount);
            y += 12;
            CoulText(LBAWHITE, -1);
        }
        y += 4;
        CoulText(MODEL_TEXT_MUTED, -1);
        if (body) {
            GraphPrintf(FALSE, x0 + 9, y, "Points: %d", (int)body->NbPoints);
            y += 12;
            GraphPrintf(FALSE, x0 + 9, y, "Bones: %d", (int)body->NbGroupes);
            y += 12;
            GraphPrintf(FALSE, x0 + 9, y, "Polygons: %d", (int)body->NbPolys);
            y += 12;
            GraphPrintf(FALSE, x0 + 9, y, "Spheres: %d", (int)body->NbSpheres);
            y += 12;
            GraphPrintf(FALSE, x0 + 9, y, "Lines: %d", (int)body->NbLines);
            y += 12;
            GraphPrintf(FALSE, x0 + 9, y, "Textures: %d", (int)body->NbTextures);
        } else {
            GraphPrintf(FALSE, x0 + 9, y, "Missing entry");
        }
    } else if (ModelSelectIsMultiType()) {
        ModelDrawActiveSelectionSummaries(body, x0 + 9, &y);
    } else {
        if (ModelSelectMode == MODEL_SELECT_VERTEX) {
            ModelFormatSelectedSummary(value, sizeof(value), ModelSelectedVertexCount, body ? body->NbPoints : 0, "vertex", "vertices");
            CoulText(MODEL_TEXT_MUTED, -1);
            GraphPrintf(FALSE, x0 + 9, y, "%s", value);
            y += 16;
            if (ModelSelectedVertexCount > 0) {
                CoulText(MODEL_TEXT_MUTED, -1);
                if (ModelSelectedVertexCount == 1 AND ModelSelectedVertex >= 0)
                    GraphPrintf(FALSE, x0 + 9, y, "Index: %d", (int)ModelSelectedVertex);
                else
                    GraphPrintf(FALSE, x0 + 9, y, "Index: --");
                y += 12;
            }
            CoulText(LBAWHITE, -1);
            if (body AND ModelSelectedVertex >= 0 AND ModelSelectedVertex < body->NbPoints) {
                T_OBJ_POINT *points = (T_OBJ_POINT *)((U8 *)body + body->OffPoints);
                T_OBJ_POINT *p = &points[ModelSelectedVertex];
                GraphPrintf(FALSE, x0 + 9, y, "X:%d Y:%d Z:%d", (int)p->X, (int)p->Y, (int)p->Z);
                y += 12;
                GraphPrintf(FALSE, x0 + 9, y, "Group: %d", (int)p->Group);
            }
        } else if (ModelSelectMode == MODEL_SELECT_FACE) {
            ModelFaceSelectionStats stats;

            ModelCollectFaceSelectionStats(body, &stats);
            ModelFormatSelectedSummary(value, sizeof(value), stats.count, body ? body->NbPolys : 0, "face", "faces");
            CoulText(MODEL_TEXT_MUTED, -1);
            GraphPrintf(FALSE, x0 + 9, y, "%s", value);
            y += 16;
            CoulText(LBAWHITE, -1);
            if (stats.count > 0) {
                if (ModelEditingNumber AND ModelNumberTarget == MODEL_NUMBER_FACE_TYPE)
                    snprintf(value, sizeof(value), "%s_", ModelNumberText);
                else if (stats.type_mixed)
                    snprintf(value, sizeof(value), "--");
                else
                    snprintf(value, sizeof(value), "%s (%d)", ModelFaceTypeLabel(stats.type_value), (int)stats.type_value);
                ModelDrawField(x0, x1, &y, ModelInspectorField == MODEL_EDIT_FIELD_TYPE, "Type: %s", value);

                if (ModelEditingNumber AND ModelNumberTarget == MODEL_NUMBER_FACE_COLOR)
                    snprintf(value, sizeof(value), "%s_", ModelNumberText);
                else if (stats.color_mixed)
                    snprintf(value, sizeof(value), "--");
                else
                    snprintf(value, sizeof(value), "%d", (int)stats.color_value);
                ModelDrawField(x0, x1, &y, ModelInspectorField == MODEL_EDIT_FIELD_COLOR, "Color: %s", value);

                if (ModelEditingNumber AND ModelNumberTarget == MODEL_NUMBER_FACE_NORMAL)
                    snprintf(value, sizeof(value), "%s_", ModelNumberText);
                else if (stats.normal_mixed)
                    snprintf(value, sizeof(value), "--");
                else
                    snprintf(value, sizeof(value), "%d", (int)stats.normal_value);
                ModelDrawField(x0, x1, &y, ModelInspectorField == MODEL_EDIT_FIELD_NORMAL, "Normal: %s", value);

                CoulText(MODEL_TEXT_MUTED, -1);
                if (stats.texture_mixed)
                    GraphPrintf(FALSE, x0 + 9, y, "Texture: --");
                else
                    GraphPrintf(FALSE, x0 + 9, y, "Texture: %d", (int)stats.texture_value);
                y += 12;
                if (stats.point_count_mixed)
                    GraphPrintf(FALSE, x0 + 9, y, "Points: --");
                else
                    GraphPrintf(FALSE, x0 + 9, y, "Points: %d", (int)stats.point_count_value);
                y += 12;
                if (stats.raw_color_mixed)
                    GraphPrintf(FALSE, x0 + 9, y, "Raw color word: --");
                else
                    GraphPrintf(FALSE, x0 + 9, y, "Raw color word: %d", (int)stats.raw_color_value);
            }
        } else if (ModelSelectMode == MODEL_SELECT_SPHERE) {
            ModelLineSelectionStats stats;

            ModelCollectSphereSelectionStats(body, &stats);
            ModelFormatSelectedSummary(value, sizeof(value), stats.count, body ? body->NbSpheres : 0, "sphere", "spheres");
            CoulText(MODEL_TEXT_MUTED, -1);
            GraphPrintf(FALSE, x0 + 9, y, "%s", value);
            y += 16;
            CoulText(LBAWHITE, -1);
            if (stats.count > 0) {
                if (ModelEditingNumber AND ModelNumberTarget == MODEL_NUMBER_SPHERE_COLOR)
                    snprintf(value, sizeof(value), "%s_", ModelNumberText);
                else if (stats.color_mixed)
                    snprintf(value, sizeof(value), "--");
                else
                    snprintf(value, sizeof(value), "%d", (int)stats.color_value);
                ModelDrawField(x0, x1, &y, ModelInspectorField == MODEL_EDIT_FIELD_COLOR, "Color: %s", value);

                CoulText(MODEL_TEXT_MUTED, -1);
                if (stats.type_mixed)
                    GraphPrintf(FALSE, x0 + 9, y, "Type: --");
                else
                    GraphPrintf(FALSE, x0 + 9, y, "Type: %d", (int)stats.type_value);
                y += 12;
                if (stats.p1_mixed)
                    GraphPrintf(FALSE, x0 + 9, y, "Point: --");
                else
                    GraphPrintf(FALSE, x0 + 9, y, "Point: %d", (int)stats.p1_value);
                y += 12;
                if (stats.p2_mixed)
                    GraphPrintf(FALSE, x0 + 9, y, "Radius: --");
                else
                    GraphPrintf(FALSE, x0 + 9, y, "Radius: %d", (int)stats.p2_value);
                y += 12;
                if (stats.raw_color_mixed)
                    GraphPrintf(FALSE, x0 + 9, y, "Raw color word: --");
                else
                    GraphPrintf(FALSE, x0 + 9, y, "Raw color word: %d", (int)stats.raw_color_value);
            }
        } else if (ModelSelectMode == MODEL_SELECT_LINE) {
            ModelLineSelectionStats stats;

            ModelCollectLineSelectionStats(body, &stats);
            ModelFormatSelectedSummary(value, sizeof(value), stats.count, body ? body->NbLines : 0, "line", "lines");
            CoulText(MODEL_TEXT_MUTED, -1);
            GraphPrintf(FALSE, x0 + 9, y, "%s", value);
            y += 16;
            CoulText(LBAWHITE, -1);
            if (stats.count > 0) {
                if (ModelEditingNumber AND ModelNumberTarget == MODEL_NUMBER_LINE_COLOR)
                    snprintf(value, sizeof(value), "%s_", ModelNumberText);
                else if (stats.color_mixed)
                    snprintf(value, sizeof(value), "--");
                else
                    snprintf(value, sizeof(value), "%d", (int)stats.color_value);
                ModelDrawField(x0, x1, &y, ModelInspectorField == MODEL_EDIT_FIELD_COLOR, "Color: %s", value);

                if (ModelEditingNumber AND ModelNumberTarget == MODEL_NUMBER_LINE_P1)
                    snprintf(value, sizeof(value), "%s_", ModelNumberText);
                else if (stats.p1_mixed)
                    snprintf(value, sizeof(value), "--");
                else
                    snprintf(value, sizeof(value), "%d", (int)stats.p1_value);
                ModelDrawField(x0, x1, &y, ModelInspectorField == MODEL_EDIT_FIELD_P1, "P1: %s", value);

                if (ModelEditingNumber AND ModelNumberTarget == MODEL_NUMBER_LINE_P2)
                    snprintf(value, sizeof(value), "%s_", ModelNumberText);
                else if (stats.p2_mixed)
                    snprintf(value, sizeof(value), "--");
                else
                    snprintf(value, sizeof(value), "%d", (int)stats.p2_value);
                ModelDrawField(x0, x1, &y, ModelInspectorField == MODEL_EDIT_FIELD_P2, "P2: %s", value);

                CoulText(MODEL_TEXT_MUTED, -1);
                if (stats.type_mixed)
                    GraphPrintf(FALSE, x0 + 9, y, "Type: --");
                else
                    GraphPrintf(FALSE, x0 + 9, y, "Type: %d", (int)stats.type_value);
                y += 12;
                if (stats.raw_color_mixed)
                    GraphPrintf(FALSE, x0 + 9, y, "Raw color word: --");
                else
                    GraphPrintf(FALSE, x0 + 9, y, "Raw color word: %d", (int)stats.raw_color_value);
            }
        } else if (ModelSelectMode == MODEL_SELECT_BONE) {
            ModelGroupInfo *groups = body ? (ModelGroupInfo *)((U8 *)body + body->OffGroupes) : NULL;
            ModelBoneSelectionStats stats;

            ModelCollectBoneSelectionStats(body, &stats);
            ModelFormatSelectedSummary(value, sizeof(value), ModelSelectedBoneCount, body ? body->NbGroupes : 0, "bone", "bones");
            CoulText(MODEL_TEXT_MUTED, -1);
            GraphPrintf(FALSE, x0 + 9, y, "%s", value);
            y += 16;
            CoulText(LBAWHITE, -1);
            if (body AND groups) {
                if (ModelEditingNumber AND ModelNumberTarget == MODEL_NUMBER_BONE_INDEX)
                    snprintf(value, sizeof(value), "%s_", ModelNumberText);
                else if (ModelSelectedBoneCount == 1 AND ModelSelectedBone >= 0 AND ModelSelectedBone < body->NbGroupes)
                    snprintf(value, sizeof(value), "%d", (int)ModelSelectedBone);
                else
                    snprintf(value, sizeof(value), "--");
                ModelDrawField(x0, x1, &y, ModelInspectorField == MODEL_EDIT_FIELD_BONE_INDEX, "Index: %s", value);

                if (stats.count > 0) {
                    CoulText(LBAWHITE, -1);
                    if (ModelEditingNumber AND ModelNumberTarget == MODEL_NUMBER_BONE_PARENT)
                        snprintf(value, sizeof(value), "%s_", ModelNumberText);
                    else if (stats.parent_mixed)
                        snprintf(value, sizeof(value), "--");
                    else
                        snprintf(value, sizeof(value), "%d", (int)stats.parent_value);
                    ModelDrawField(x0, x1, &y, ModelInspectorField == MODEL_EDIT_FIELD_PARENT, "Parent Index: %s", value);

                    if (ModelEditingNumber AND ModelNumberTarget == MODEL_NUMBER_BONE_ORG_POINT)
                        snprintf(value, sizeof(value), "%s_", ModelNumberText);
                    else if (stats.point_mixed)
                        snprintf(value, sizeof(value), "--");
                    else
                        snprintf(value, sizeof(value), "%d", (int)stats.point_value);
                    ModelDrawField(x0, x1, &y, ModelInspectorField == MODEL_EDIT_FIELD_ORG_POINT, "Vertex Index: %s", value);

                    CoulText(MODEL_TEXT_MUTED, -1);
                    if (stats.nb_pts_mixed)
                        GraphPrintf(FALSE, x0 + 9, y, "NbPts: --");
                    else
                        GraphPrintf(FALSE, x0 + 9, y, "NbPts: %d", (int)stats.nb_pts_value);
                    y += 12;
                    if (stats.nb_norm_mixed)
                        GraphPrintf(FALSE, x0 + 9, y, "NbNorm: --");
                    else
                        GraphPrintf(FALSE, x0 + 9, y, "NbNorm: %d", (int)stats.nb_norm_value);
                }
            }
        }
    }
}

static void ModelDrawStatus(void) {
    if (ModelStatusText[0] == '\0' OR TimerRefHR >= ModelStatusUntil)
        return;
    CoulText(LBAWHITE, -1);
    GraphPrintf(FALSE, 8, (S32)ModeDesiredY - 16, "%s", ModelStatusText);
}
