static void ModelStartNumberEdit(S32 target, S32 current_value, S32 mixed) {
    ModelNumberTarget = target;
    if (mixed)
        ModelNumberText[0] = 0;
    else
        snprintf(ModelNumberText, sizeof(ModelNumberText), "%d", (int)current_value);
    ModelEditingNumber = TRUE;
}

static void ModelCommitNumberEdit(void) {
    S32 value = atoi(ModelNumberText);

    if (ModelNumberTarget == MODEL_NUMBER_BODY_ID) {
        ModelBodyId = value;
        ModelClampBodyId();
        ModelClearVertexSelection();
        ModelClearFaceSelection();
        ModelClearSphereSelection();
        ModelClearLineSelection();
        ModelClearBoneSelection();
        ModelRefreshAnimChoices(ModelGetBody());
        ModelResetAnimPreview();
    } else if (ModelNumberTarget == MODEL_NUMBER_ANIM_ID) {
        ModelSetAnimIdFromNumber(value, ModelGetBody());
    } else if (ModelNumberTarget == MODEL_NUMBER_SPHERE_COLOR) {
        T_BODY_HEADER *body = ModelGetBody();

        value = ModelClampEditValue(body, ModelNumberTarget, value);
        ModelApplySphereColor(body, value);
    } else if (ModelNumberTarget == MODEL_NUMBER_LINE_COLOR) {
        T_BODY_HEADER *body = ModelGetBody();

        value = ModelClampEditValue(body, ModelNumberTarget, value);
        ModelApplyLineColor(body, value);
    } else if (ModelNumberTarget == MODEL_NUMBER_LINE_P1 OR ModelNumberTarget == MODEL_NUMBER_LINE_P2) {
        T_BODY_HEADER *body = ModelGetBody();

        value = ModelClampEditValue(body, ModelNumberTarget, value);
        ModelApplyLinePoint(body, ModelNumberTarget, value);
    } else if (ModelNumberTarget == MODEL_NUMBER_BONE_PARENT OR ModelNumberTarget == MODEL_NUMBER_BONE_ORG_POINT) {
        T_BODY_HEADER *body = ModelGetBody();

        value = ModelClampEditValue(body, ModelNumberTarget, value);
        ModelApplyBoneField(body, ModelNumberTarget, value);
    } else if (ModelNumberTarget == MODEL_NUMBER_BONE_INDEX) {
        T_BODY_HEADER *body = ModelGetBody();

        ModelSelectBoneIndex(body, value);
    } else {
        T_BODY_HEADER *body = ModelGetBody();

        value = ModelClampEditValue(body, ModelNumberTarget, value);
        ModelApplyFaceEdit(body, ModelNumberTarget, value);
    }
    ModelEditingNumber = FALSE;
}

static char ModelDigitFromKey(S32 key) {
    switch (key) {
    case K_0:
    case K_NUMPAD_0:
        return '0';
    case K_1:
    case K_NUMPAD_1:
        return '1';
    case K_2:
    case K_NUMPAD_2:
        return '2';
    case K_3:
    case K_NUMPAD_3:
        return '3';
    case K_4:
    case K_NUMPAD_4:
        return '4';
    case K_5:
    case K_NUMPAD_5:
        return '5';
    case K_6:
    case K_NUMPAD_6:
        return '6';
    case K_7:
    case K_NUMPAD_7:
        return '7';
    case K_8:
    case K_NUMPAD_8:
        return '8';
    case K_9:
    case K_NUMPAD_9:
        return '9';
    }
    return 0;
}

static void ModelHandleNumberEdit(S32 key_edge, S32 key) {
    S32 len;
    char digit;

    if (!key_edge)
        return;
    if (key == K_ESC) {
        ModelEditingNumber = FALSE;
        return;
    }
    if (key == K_ENTER OR key == K_NUMPAD_ENTER) {
        ModelCommitNumberEdit();
        return;
    }
    if (key == K_BACKSPACE) {
        len = (S32)strlen(ModelNumberText);
        if (len > 0)
            ModelNumberText[len - 1] = 0;
        return;
    }
    if (key == K_MOINS OR key == K_NUMPAD_MOINS OR key == K_KP_MOINS) {
        len = (S32)strlen(ModelNumberText);
        if (len == 0) {
            ModelNumberText[0] = '-';
            ModelNumberText[1] = 0;
        }
        return;
    }
    digit = ModelDigitFromKey(key);
    if (digit) {
        len = (S32)strlen(ModelNumberText);
        if (len < (S32)sizeof(ModelNumberText) - 1) {
            ModelNumberText[len] = digit;
            ModelNumberText[len + 1] = 0;
        }
    }
}

static void ModelHandleCamera(void) {
    S32 wheel_x = ConsumeMouseWheelX();
    S32 wheel_y = ConsumeMouseWheelY();
    float pinch = ConsumeMousePinchScale();
    S32 shift = ModelShiftDown();
    S32 alt = ModelAltDown();

    if (ModelCameraMode == MODEL_CAMERA_FREE_NAV)
        return;

    if (pinch != 1.0f) {
        ModelCameraMode = MODEL_CAMERA_ORBIT;
        if (pinch < 0.1f)
            pinch = 0.1f;
        if (pinch > 10.0f)
            pinch = 10.0f;
        ModelZoom = (S32)((float)ModelZoom / pinch);
        ModelClampZoom();
    }
    if (wheel_x == 0 AND wheel_y == 0)
        return;
    ModelCameraMode = MODEL_CAMERA_ORBIT;
    if (alt) {
        ModelAdjustZoom(-(wheel_y * 600 + wheel_x * 300));
    } else if (shift) {
        ModelPanX -= wheel_x * 18;
        ModelPanY += wheel_y * 18;
    } else {
        ModelBeta -= wheel_x * 80;
        ModelAlpha += wheel_y * 60;
        ModelBeta &= 4095;
        if (ModelAlpha < -900)
            ModelAlpha = -900;
        if (ModelAlpha > 900)
            ModelAlpha = 900;
    }
}

static void ModelUpdateFreecamNav(void) {
    U32 dt;
    S32 shift;
    S32 alt;
    float mouse_x = 0.0f;
    float mouse_y = 0.0f;
    S32 forward;
    S32 right;
    S32 vertical;
    float speed;
    T_FREECAM_VECTOR movement;

    if (ModelCameraMode != MODEL_CAMERA_FREE_NAV)
        return;

    shift = ModelShiftDown();
    alt = ModelAltDown();
    dt = TimerRefHR - ModelFreeLastTime;
    ModelFreeLastTime = TimerRefHR;
    if (dt > MODEL_FREECAM_MAX_DT)
        dt = MODEL_FREECAM_MAX_DT;

    Window_GetRelativeMouseDelta(&mouse_x, &mouse_y);
    ModelFreeBeta = FreeCam_NormalizeAngle(ModelFreeBeta + (S32)(mouse_x * MODEL_FREECAM_MOUSE_SENSITIVITY));
    ModelFreeAlpha += (S32)(mouse_y * MODEL_FREECAM_MOUSE_SENSITIVITY);
    if (ModelFreeAlpha < -MODEL_FREECAM_PITCH_LIMIT)
        ModelFreeAlpha = -MODEL_FREECAM_PITCH_LIMIT;
    if (ModelFreeAlpha > MODEL_FREECAM_PITCH_LIMIT)
        ModelFreeAlpha = MODEL_FREECAM_PITCH_LIMIT;

    forward = (CheckKey(K_W) ? 1 : 0) - (CheckKey(K_S) ? 1 : 0);
    right = (CheckKey(K_D) ? 1 : 0) - (CheckKey(K_A) ? 1 : 0);
    vertical = (CheckKey(K_E) ? 1 : 0) - (CheckKey(K_Q) ? 1 : 0);
    speed = MODEL_FREECAM_MOVE_PER_MS * (float)dt;
    if (shift)
        speed *= MODEL_FREECAM_FAST_MULTIPLIER;
    if (alt)
        speed *= MODEL_FREECAM_SLOW_MULTIPLIER;

    movement = FreeCam_MovementVector(ModelFreeAlpha, ModelFreeBeta, forward, right, vertical);
    ModelFreeX += movement.x * speed;
    ModelFreeY += movement.y * speed;
    ModelFreeZ += movement.z * speed;

    Input = 0;
    MyKey = 0;
    Key = 0;
    Click = 0;
    memset(TabKeys, 0, sizeof(TabKeys));
}

static S32 ModelEditTargetFromField(S32 field) {
    if (field == MODEL_EDIT_FIELD_TYPE)
        return MODEL_NUMBER_FACE_TYPE;
    if (field == MODEL_EDIT_FIELD_COLOR)
        return MODEL_NUMBER_FACE_COLOR;
    return MODEL_NUMBER_FACE_NORMAL;
}

static S32 ModelEditFieldValue(const ModelFaceSelectionStats *stats, S32 field, S32 *mixed) {
    if (field == MODEL_EDIT_FIELD_TYPE) {
        *mixed = stats->type_mixed;
        return stats->type_value;
    }
    if (field == MODEL_EDIT_FIELD_COLOR) {
        *mixed = stats->color_mixed;
        return stats->color_value;
    }
    *mixed = stats->normal_mixed;
    return stats->normal_value;
}

static S32 ModelLineTargetFromField(S32 field) {
    if (field == MODEL_EDIT_FIELD_P1)
        return MODEL_NUMBER_LINE_P1;
    if (field == MODEL_EDIT_FIELD_P2)
        return MODEL_NUMBER_LINE_P2;
    return MODEL_NUMBER_LINE_COLOR;
}

static S32 ModelLineFieldValue(const ModelLineSelectionStats *stats, S32 field, S32 *mixed) {
    if (field == MODEL_EDIT_FIELD_P1) {
        *mixed = stats->p1_mixed;
        return stats->p1_value;
    }
    if (field == MODEL_EDIT_FIELD_P2) {
        *mixed = stats->p2_mixed;
        return stats->p2_value;
    }
    *mixed = stats->color_mixed;
    return stats->color_value;
}

static S32 ModelBoneTargetFromField(S32 field) {
    if (field == MODEL_EDIT_FIELD_BONE_INDEX)
        return MODEL_NUMBER_BONE_INDEX;
    if (field == MODEL_EDIT_FIELD_ORG_POINT)
        return MODEL_NUMBER_BONE_ORG_POINT;
    return MODEL_NUMBER_BONE_PARENT;
}

static S32 ModelBoneFieldValue(const ModelBoneSelectionStats *stats, S32 field, S32 *mixed) {
    if (field == MODEL_EDIT_FIELD_BONE_INDEX) {
        *mixed = ModelSelectedBoneCount == 1 ? FALSE : TRUE;
        return ModelSelectedBone;
    }
    if (field == MODEL_EDIT_FIELD_ORG_POINT) {
        *mixed = stats->point_mixed;
        return stats->point_value;
    }
    *mixed = stats->parent_mixed;
    return stats->parent_value;
}

static S32 ModelClampEditValue(T_BODY_HEADER *body, S32 target, S32 value) {
    if (target == MODEL_NUMBER_FACE_TYPE) {
        if (value < 0)
            value = 0;
        if (value > MODEL_MAX_POLY_TYPE)
            value = MODEL_MAX_POLY_TYPE;
    } else if (target == MODEL_NUMBER_FACE_COLOR OR target == MODEL_NUMBER_SPHERE_COLOR OR target == MODEL_NUMBER_LINE_COLOR) {
        if (value < 0)
            value = 0;
        if (value > 255)
            value = 255;
    } else if (target == MODEL_NUMBER_FACE_NORMAL) {
        if (value < 0)
            value = 0;
        if (body AND body->NbNormales > 0 AND value >= body->NbNormales)
            value = body->NbNormales - 1;
    } else if (target == MODEL_NUMBER_LINE_P1 OR target == MODEL_NUMBER_LINE_P2) {
        S32 max_point = body ? body->NbPoints - 1 : 0;

        if (max_point >= MODEL_MAX_POINTS)
            max_point = MODEL_MAX_POINTS - 1;
        if (max_point < 0)
            max_point = 0;
        if (value < 0)
            value = 0;
        if (value > max_point)
            value = max_point;
    } else if (target == MODEL_NUMBER_BONE_PARENT) {
        S32 max_group = body ? body->NbGroupes - 1 : 0;

        if (max_group < 0)
            max_group = 0;
        if (value < 0)
            value = 0;
        if (value > max_group)
            value = max_group;
    } else if (target == MODEL_NUMBER_BONE_ORG_POINT) {
        S32 max_point = body ? body->NbPoints - 1 : 0;

        if (max_point >= MODEL_MAX_POINTS)
            max_point = MODEL_MAX_POINTS - 1;
        if (max_point < 0)
            max_point = 0;
        if (value < 0)
            value = 0;
        if (value > max_point)
            value = max_point;
    } else if (target == MODEL_NUMBER_BONE_INDEX) {
        S32 max_group = body ? body->NbGroupes - 1 : 0;

        if (max_group < 0)
            max_group = 0;
        if (value < 0)
            value = 0;
        if (value > max_group)
            value = max_group;
    }
    return value;
}

static void ModelAdjustEditField(T_BODY_HEADER *body, S32 delta) {
    ModelFaceSelectionStats stats;
    S32 mixed;
    S32 value;
    S32 target;

    ModelCollectFaceSelectionStats(body, &stats);
    if (stats.count <= 0)
        return;
    value = ModelEditFieldValue(&stats, ModelInspectorField, &mixed);
    if (mixed)
        return;
    target = ModelEditTargetFromField(ModelInspectorField);
    value = ModelClampEditValue(body, target, value + delta);
    ModelApplyFaceEdit(body, target, value);
}

static void ModelStartEditFieldNumber(T_BODY_HEADER *body) {
    ModelFaceSelectionStats stats;
    S32 mixed;
    S32 value;

    ModelCollectFaceSelectionStats(body, &stats);
    if (stats.count <= 0)
        return;
    value = ModelEditFieldValue(&stats, ModelInspectorField, &mixed);
    ModelStartNumberEdit(ModelEditTargetFromField(ModelInspectorField), value, mixed);
}

static void ModelAdjustSphereColorField(T_BODY_HEADER *body, S32 delta) {
    ModelLineSelectionStats stats;

    ModelCollectSphereSelectionStats(body, &stats);
    if (stats.count <= 0 OR stats.color_mixed)
        return;
    ModelApplySphereColor(body, ModelClampEditValue(body, MODEL_NUMBER_SPHERE_COLOR, stats.color_value + delta));
}

static void ModelStartSphereColorNumber(T_BODY_HEADER *body) {
    ModelLineSelectionStats stats;

    ModelCollectSphereSelectionStats(body, &stats);
    if (stats.count <= 0)
        return;
    ModelStartNumberEdit(MODEL_NUMBER_SPHERE_COLOR, stats.color_value, stats.color_mixed);
}

static void ModelAdjustLineColorField(T_BODY_HEADER *body, S32 delta) {
    ModelLineSelectionStats stats;

    ModelCollectLineSelectionStats(body, &stats);
    if (stats.count <= 0 OR stats.color_mixed)
        return;
    ModelApplyLineColor(body, ModelClampEditValue(body, MODEL_NUMBER_LINE_COLOR, stats.color_value + delta));
}

static void ModelStartLineColorNumber(T_BODY_HEADER *body) {
    ModelLineSelectionStats stats;

    ModelCollectLineSelectionStats(body, &stats);
    if (stats.count <= 0)
        return;
    ModelStartNumberEdit(MODEL_NUMBER_LINE_COLOR, stats.color_value, stats.color_mixed);
}

static void ModelAdjustLineField(T_BODY_HEADER *body, S32 delta) {
    ModelLineSelectionStats stats;
    S32 mixed;
    S32 value;
    S32 target;

    ModelCollectLineSelectionStats(body, &stats);
    if (stats.count <= 0)
        return;
    value = ModelLineFieldValue(&stats, ModelInspectorField, &mixed);
    if (mixed)
        return;
    target = ModelLineTargetFromField(ModelInspectorField);
    value = ModelClampEditValue(body, target, value + delta);
    if (target == MODEL_NUMBER_LINE_COLOR)
        ModelApplyLineColor(body, value);
    else
        ModelApplyLinePoint(body, target, value);
}

static void ModelStartLineFieldNumber(T_BODY_HEADER *body) {
    ModelLineSelectionStats stats;
    S32 mixed;
    S32 value;

    ModelCollectLineSelectionStats(body, &stats);
    if (stats.count <= 0)
        return;
    value = ModelLineFieldValue(&stats, ModelInspectorField, &mixed);
    ModelStartNumberEdit(ModelLineTargetFromField(ModelInspectorField), value, mixed);
}

static void ModelAdjustBoneField(T_BODY_HEADER *body, S32 delta) {
    ModelBoneSelectionStats stats;
    S32 mixed;
    S32 value;
    S32 target;

    if (ModelInspectorField == MODEL_EDIT_FIELD_BONE_INDEX) {
        value = ModelSelectedBone >= 0 ? ModelSelectedBone : 0;
        ModelSelectBoneIndex(body, value + delta);
        return;
    }

    ModelCollectBoneSelectionStats(body, &stats);
    if (stats.count <= 0)
        return;
    value = ModelBoneFieldValue(&stats, ModelInspectorField, &mixed);
    if (mixed)
        return;
    target = ModelBoneTargetFromField(ModelInspectorField);
    value = ModelClampEditValue(body, target, value + delta);
    ModelApplyBoneField(body, target, value);
}

static void ModelStartBoneFieldNumber(T_BODY_HEADER *body) {
    ModelBoneSelectionStats stats;
    S32 mixed;
    S32 value;

    if (ModelInspectorField == MODEL_EDIT_FIELD_BONE_INDEX) {
        value = ModelSelectedBone >= 0 ? ModelSelectedBone : 0;
        ModelStartNumberEdit(MODEL_NUMBER_BONE_INDEX, value, ModelSelectedBoneCount == 1 ? FALSE : TRUE);
        return;
    }

    ModelCollectBoneSelectionStats(body, &stats);
    if (stats.count <= 0)
        return;
    value = ModelBoneFieldValue(&stats, ModelInspectorField, &mixed);
    ModelStartNumberEdit(ModelBoneTargetFromField(ModelInspectorField), value, mixed);
}

static void ModelHandleInput(S32 *done) {
    U32 input_edge;
    S32 key = MyKey;
    S32 key_edge = (key != 0 AND key != ModelLastKey) ? TRUE : FALSE;
    S32 click_edge;
    S32 release_edge;
    S32 shift;
    S32 alt;
    S32 ctrl;
    S32 save;

    MyGetInput();
    key = MyKey;
    key_edge = (key != 0 AND key != ModelLastKey) ? TRUE : FALSE;
    input_edge = Input & ~ModelLastInput;
    click_edge = ((Click & 1) AND !(ModelLastClick & 1)) ? TRUE : FALSE;
    release_edge = (!(Click & 1) AND(ModelLastClick & 1)) ? TRUE : FALSE;
    shift = ModelShiftDown();
    alt = ModelAltDown();
    ctrl = ModelCtrlDown();
    save = (ctrl AND CheckKey(K_S)) ? TRUE : FALSE;

    ModelHandleCamera();
    ModelUpdateFreecamNav();

    if (ModelBlockInputUntilRelease) {
        if (MyKey == 0 AND Click == 0 AND Input == 0) {
            ModelBlockInputUntilRelease = FALSE;
        } else {
            goto update_last;
        }
    }

    {
        S32 free_toggle = (shift AND CheckKey(K_CARRE)) ? TRUE : FALSE;

        if (free_toggle AND !ModelFreeLastToggle) {
            if (ModelCameraMode == MODEL_CAMERA_FREE_NAV)
                ModelCommitFreecam();
            else
                ModelStartFreecam();
            goto update_last;
        }
        ModelFreeLastToggle = free_toggle;
    }

    if (ModelCameraMode == MODEL_CAMERA_FREE_NAV) {
        if (key_edge AND key == K_ESC) {
            ModelCancelFreecam();
            goto update_last;
        }
        if (key_edge AND(key == K_ENTER OR key == K_NUMPAD_ENTER)) {
            ModelCommitFreecam();
            goto update_last;
        }
        if (click_edge) {
            ModelCommitFreecam();
            goto update_last;
        }
        goto update_last;
    }

    if (save AND !ModelLastSave) {
        ModelSaveCurrentBody();
        goto update_last;
    }

    if (ctrl AND key_edge AND key == K_C) {
        ModelCopySelection(ModelGetBody(), shift AND ModelSelectActive(MODEL_SELECT_BONE));
        goto update_last;
    }
    if (ctrl AND key_edge AND key == K_V) {
        ModelPasteClipboard(ModelGetBody());
        goto update_last;
    }

    if (ModelEditingNumber) {
        ModelHandleNumberEdit(key_edge, key);
        goto update_last;
    }

    if (ModelGrabMode) {
        T_BODY_HEADER *body = ModelGetBody();

        if (key_edge AND key == K_ESC) {
            ModelCancelGrab(body);
            goto update_last;
        }
        if ((key_edge AND(key == K_ENTER OR key == K_NUMPAD_ENTER)) OR click_edge) {
            ModelCommitGrab(body);
            goto update_last;
        }
        if (key_edge AND key == K_X)
            ModelGrab.axis = 0;
        if (key_edge AND key == K_Y)
            ModelGrab.axis = 1;
        if (key_edge AND key == K_Z)
            ModelGrab.axis = 2;
        ModelApplyGrab(body);
        goto update_last;
    }

    if (ModelBoxSelectMode) {
        if (key_edge AND key == K_ESC) {
            ModelBoxSelectMode = FALSE;
            ModelBoxSelecting = FALSE;
            goto update_last;
        }
        if (Click & 1) {
            ModelBoxX1 = (S32)MouseX;
            ModelBoxY1 = (S32)MouseY;
        }
        if (click_edge) {
            ModelBoxSelecting = TRUE;
            ModelBoxX0 = (S32)MouseX;
            ModelBoxY0 = (S32)MouseY;
            ModelBoxX1 = ModelBoxX0;
            ModelBoxY1 = ModelBoxY0;
            goto update_last;
        }
        if (release_edge AND ModelBoxSelecting) {
            ModelBoxX1 = (S32)MouseX;
            ModelBoxY1 = (S32)MouseY;
            ModelApplyBoxSelection(ModelGetBody(), shift);
            ModelBoxSelectMode = FALSE;
            ModelBoxSelecting = FALSE;
            goto update_last;
        }
        goto update_last;
    }

    if (input_edge & I_MENUS OR(key_edge AND key == K_ESC)) {
        *done = TRUE;
        goto update_last;
    }
    if (key_edge AND key == K_CARRE) {
        ModelInspectorVisible = ModelInspectorVisible ? FALSE : TRUE;
    }
    if (key_edge AND key == K_F AND shift) {
        ModelFloorGrid = ModelFloorGrid ? FALSE : TRUE;
    }
    if (ModelMode == MODEL_MODE_EDIT AND key_edge AND !alt AND !ctrl AND key == K_B) {
        ModelBoxSelectMode = TRUE;
        ModelBoxSelecting = FALSE;
        goto update_last;
    }
    if (ModelMode == MODEL_MODE_EDIT AND key_edge AND !alt AND !ctrl AND key == K_G) {
        ModelStartGrab(ModelGetBody());
        goto update_last;
    }
    if (key_edge AND !alt AND !ctrl AND key == K_L) {
        ModelDumpSelectedFaces(ModelGetBody());
        goto update_last;
    }
    if (ModelMode == MODEL_MODE_EDIT AND key_edge AND !alt AND !ctrl AND key == K_A) {
        ModelSelectAllCurrent(ModelGetBody());
        goto update_last;
    }
    if (key_edge AND key == K_Z AND alt) {
        ModelXray = ModelXray ? FALSE : TRUE;
        ModelClearFaceSelection();
    }
    if (key_edge AND key == K_TAB AND shift) {
        ModelMode = ModelMode == MODEL_MODE_VIEW ? MODEL_MODE_EDIT : MODEL_MODE_VIEW;
        ModelInspectorField = ModelMode == MODEL_MODE_VIEW ? MODEL_FIELD_ARCHIVE : MODEL_EDIT_FIELD_TYPE;
        ModelResetAnimPreview();
    }
    if (ModelMode == MODEL_MODE_EDIT AND alt AND key_edge AND(key == K_1 OR key == K_NUMPAD_1)) {
        ModelSetSelectMode(MODEL_SELECT_VERTEX, shift);
        ModelInspectorField = MODEL_EDIT_FIELD_TYPE;
        if (!shift) {
            ModelClearFaceSelection();
            ModelClearSphereSelection();
            ModelClearLineSelection();
            ModelClearBoneSelection();
        } else {
            ModelClearFaceSelection();
        }
    }
    if (ModelMode == MODEL_MODE_EDIT AND alt AND key_edge AND(key == K_2 OR key == K_NUMPAD_2)) {
        ModelSetSelectMode(MODEL_SELECT_FACE, shift);
        ModelInspectorField = MODEL_EDIT_FIELD_TYPE;
        if (!shift) {
            ModelClearVertexSelection();
            ModelClearSphereSelection();
            ModelClearLineSelection();
            ModelClearBoneSelection();
        } else {
            ModelClearVertexSelection();
        }
    }
    if (ModelMode == MODEL_MODE_EDIT AND alt AND key_edge AND(key == K_3 OR key == K_NUMPAD_3)) {
        ModelSetSelectMode(MODEL_SELECT_SPHERE, shift);
        ModelInspectorField = MODEL_EDIT_FIELD_COLOR;
        if (!shift) {
            ModelClearVertexSelection();
            ModelClearFaceSelection();
            ModelClearLineSelection();
            ModelClearBoneSelection();
        }
    }
    if (ModelMode == MODEL_MODE_EDIT AND alt AND key_edge AND(key == K_4 OR key == K_NUMPAD_4)) {
        ModelSetSelectMode(MODEL_SELECT_LINE, shift);
        ModelInspectorField = MODEL_EDIT_FIELD_COLOR;
        if (!shift) {
            ModelClearVertexSelection();
            ModelClearFaceSelection();
            ModelClearSphereSelection();
            ModelClearBoneSelection();
        }
    }
    if (ModelMode == MODEL_MODE_EDIT AND alt AND key_edge AND(key == K_5 OR key == K_NUMPAD_5)) {
        ModelSetSelectMode(MODEL_SELECT_BONE, shift);
        ModelInspectorField = MODEL_EDIT_FIELD_TYPE;
        if (!shift) {
            ModelClearVertexSelection();
            ModelClearFaceSelection();
            ModelClearSphereSelection();
            ModelClearLineSelection();
        }
    }
    if (!alt AND key_edge AND(key == K_1 OR key == K_NUMPAD_1)) {
        ModelSetFrontView();
    }
    if (!alt AND key_edge AND(key == K_3 OR key == K_NUMPAD_3)) {
        ModelSetRightView();
    }
    if (!alt AND key_edge AND(key == K_7 OR key == K_NUMPAD_7)) {
        ModelSetTopView();
    }
    if (!alt AND key_edge AND(key == K_5 OR key == K_NUMPAD_5)) {
        ModelCameraMode = MODEL_CAMERA_ORBIT;
        ModelOrtho = ModelOrtho ? FALSE : TRUE;
    }
    if (!alt AND key_edge AND(key == K_0 OR key == K_NUMPAD_0)) {
        ModelResetCamera();
    }
    if (!ctrl AND key_edge AND(key == K_EQUALS OR key == K_NUMPAD_PLUS OR key == K_KP_PLUS)) {
        ModelAdjustZoom(-900);
    }
    if (!ctrl AND key_edge AND(key == K_MOINS OR key == K_NUMPAD_MOINS OR key == K_KP_MOINS)) {
        ModelAdjustZoom(900);
    }
    if (!ctrl AND key_edge AND(key == K_PERIOD OR key == K_NUMPAD_POINT)) {
        ModelFocusSelected(ModelGetBody());
    }

    if (ModelMode == MODEL_MODE_VIEW AND ModelInspectorVisible) {
        if (input_edge & I_UP OR(key_edge AND key == K_GRAY_UP)) {
            ModelInspectorField--;
            if (ModelInspectorField < MODEL_FIELD_ARCHIVE)
                ModelInspectorField = MODEL_FIELD_ANIM_ID;
        }
        if (input_edge & I_DOWN OR(key_edge AND key == K_GRAY_DOWN)) {
            ModelInspectorField++;
            if (ModelInspectorField > MODEL_FIELD_ANIM_ID)
                ModelInspectorField = MODEL_FIELD_ARCHIVE;
        }
        if (input_edge & I_LEFT OR(key_edge AND key == K_GRAY_LEFT)) {
            if (ModelInspectorField == MODEL_FIELD_ARCHIVE)
                ModelChangeArchive(-1);
            else if (ModelInspectorField == MODEL_FIELD_BODY_ID)
                ModelChangeBodyId(-1);
            else
                ModelChangeAnimId(-1, ModelGetBody());
        }
        if (input_edge & I_RIGHT OR(key_edge AND key == K_GRAY_RIGHT)) {
            if (ModelInspectorField == MODEL_FIELD_ARCHIVE)
                ModelChangeArchive(1);
            else if (ModelInspectorField == MODEL_FIELD_BODY_ID)
                ModelChangeBodyId(1);
            else
                ModelChangeAnimId(1, ModelGetBody());
        }
        if (ModelInspectorField == MODEL_FIELD_BODY_ID AND key_edge AND(key == K_ENTER OR key == K_NUMPAD_ENTER)) {
            ModelStartNumberEdit(MODEL_NUMBER_BODY_ID, ModelBodyId, FALSE);
        }
        if (ModelInspectorField == MODEL_FIELD_ANIM_ID AND key_edge AND(key == K_ENTER OR key == K_NUMPAD_ENTER)) {
            ModelStartNumberEdit(MODEL_NUMBER_ANIM_ID, ModelAnimId, FALSE);
        }
    }

    if (ModelMode == MODEL_MODE_EDIT AND ModelSelectMode == MODEL_SELECT_FACE AND !ModelSelectIsMultiType() AND ModelInspectorVisible) {
        T_BODY_HEADER *body = ModelGetBody();

        if (input_edge & I_UP OR(key_edge AND key == K_GRAY_UP)) {
            ModelInspectorField--;
            if (ModelInspectorField < MODEL_EDIT_FIELD_TYPE)
                ModelInspectorField = MODEL_EDIT_FIELD_NORMAL;
        }
        if (input_edge & I_DOWN OR(key_edge AND key == K_GRAY_DOWN)) {
            ModelInspectorField++;
            if (ModelInspectorField > MODEL_EDIT_FIELD_NORMAL)
                ModelInspectorField = MODEL_EDIT_FIELD_TYPE;
        }
        if (input_edge & I_LEFT OR(key_edge AND key == K_GRAY_LEFT)) {
            ModelAdjustEditField(body, -1);
        }
        if (input_edge & I_RIGHT OR(key_edge AND key == K_GRAY_RIGHT)) {
            ModelAdjustEditField(body, 1);
        }
        if (key_edge AND(key == K_ENTER OR key == K_NUMPAD_ENTER)) {
            ModelStartEditFieldNumber(body);
        }
    }

    if (ModelMode == MODEL_MODE_EDIT AND ModelSelectMode == MODEL_SELECT_SPHERE AND !ModelSelectIsMultiType() AND ModelInspectorVisible) {
        T_BODY_HEADER *body = ModelGetBody();

        ModelInspectorField = MODEL_EDIT_FIELD_COLOR;
        if (input_edge & I_LEFT OR(key_edge AND key == K_GRAY_LEFT)) {
            ModelAdjustSphereColorField(body, -1);
        }
        if (input_edge & I_RIGHT OR(key_edge AND key == K_GRAY_RIGHT)) {
            ModelAdjustSphereColorField(body, 1);
        }
        if (key_edge AND(key == K_ENTER OR key == K_NUMPAD_ENTER)) {
            ModelStartSphereColorNumber(body);
        }
    }

    if (ModelMode == MODEL_MODE_EDIT AND ModelSelectMode == MODEL_SELECT_LINE AND !ModelSelectIsMultiType() AND ModelInspectorVisible) {
        T_BODY_HEADER *body = ModelGetBody();

        if (ModelInspectorField != MODEL_EDIT_FIELD_COLOR AND ModelInspectorField != MODEL_EDIT_FIELD_P1 AND ModelInspectorField != MODEL_EDIT_FIELD_P2)
            ModelInspectorField = MODEL_EDIT_FIELD_COLOR;
        if (input_edge & I_UP OR(key_edge AND key == K_GRAY_UP)) {
            if (ModelInspectorField == MODEL_EDIT_FIELD_COLOR)
                ModelInspectorField = MODEL_EDIT_FIELD_P2;
            else if (ModelInspectorField == MODEL_EDIT_FIELD_P1)
                ModelInspectorField = MODEL_EDIT_FIELD_COLOR;
            else
                ModelInspectorField = MODEL_EDIT_FIELD_P1;
        }
        if (input_edge & I_DOWN OR(key_edge AND key == K_GRAY_DOWN)) {
            if (ModelInspectorField == MODEL_EDIT_FIELD_COLOR)
                ModelInspectorField = MODEL_EDIT_FIELD_P1;
            else if (ModelInspectorField == MODEL_EDIT_FIELD_P1)
                ModelInspectorField = MODEL_EDIT_FIELD_P2;
            else
                ModelInspectorField = MODEL_EDIT_FIELD_COLOR;
        }
        if (input_edge & I_LEFT OR(key_edge AND key == K_GRAY_LEFT)) {
            ModelAdjustLineField(body, -1);
        }
        if (input_edge & I_RIGHT OR(key_edge AND key == K_GRAY_RIGHT)) {
            ModelAdjustLineField(body, 1);
        }
        if (key_edge AND(key == K_ENTER OR key == K_NUMPAD_ENTER)) {
            ModelStartLineFieldNumber(body);
        }
    }

    if (ModelMode == MODEL_MODE_EDIT AND ModelSelectMode == MODEL_SELECT_BONE AND !ModelSelectIsMultiType() AND ModelInspectorVisible) {
        T_BODY_HEADER *body = ModelGetBody();

        if (ModelInspectorField != MODEL_EDIT_FIELD_BONE_INDEX AND ModelInspectorField != MODEL_EDIT_FIELD_PARENT AND ModelInspectorField != MODEL_EDIT_FIELD_ORG_POINT)
            ModelInspectorField = MODEL_EDIT_FIELD_BONE_INDEX;
        if (input_edge & I_UP OR(key_edge AND key == K_GRAY_UP)) {
            if (ModelInspectorField == MODEL_EDIT_FIELD_BONE_INDEX)
                ModelInspectorField = MODEL_EDIT_FIELD_ORG_POINT;
            else if (ModelInspectorField == MODEL_EDIT_FIELD_PARENT)
                ModelInspectorField = MODEL_EDIT_FIELD_BONE_INDEX;
            else
                ModelInspectorField = MODEL_EDIT_FIELD_PARENT;
        }
        if (input_edge & I_DOWN OR(key_edge AND key == K_GRAY_DOWN)) {
            if (ModelInspectorField == MODEL_EDIT_FIELD_BONE_INDEX)
                ModelInspectorField = MODEL_EDIT_FIELD_PARENT;
            else if (ModelInspectorField == MODEL_EDIT_FIELD_PARENT)
                ModelInspectorField = MODEL_EDIT_FIELD_ORG_POINT;
            else
                ModelInspectorField = MODEL_EDIT_FIELD_BONE_INDEX;
        }
        if (input_edge & I_LEFT OR(key_edge AND key == K_GRAY_LEFT)) {
            ModelAdjustBoneField(body, -1);
        }
        if (input_edge & I_RIGHT OR(key_edge AND key == K_GRAY_RIGHT)) {
            ModelAdjustBoneField(body, 1);
        }
        if (key_edge AND(key == K_ENTER OR key == K_NUMPAD_ENTER)) {
            ModelStartBoneFieldNumber(body);
        }
    }

    if (ModelMode == MODEL_MODE_EDIT AND click_edge) {
        T_BODY_HEADER *body = ModelGetBody();
        S32 hit = FALSE;

        if (ModelSelectActive(MODEL_SELECT_VERTEX)) {
            S32 vertex = ModelFindVertexAt(body, (S32)MouseX, (S32)MouseY);

            if (vertex >= 0) {
                if (shift)
                    ModelToggleVertexSelection(body, vertex);
                else
                    ModelSetSingleVertexSelection(body, vertex);
                hit = TRUE;
            }
        }
        if (ModelSelectActive(MODEL_SELECT_FACE)) {
            ModelFaceInfo info;
            S32 color;
            S32 face;

            if (body AND ModelColorPanelHit(body, (S32)MouseX, (S32)MouseY, &color)) {
                ModelSelectFacesByColor(body, color, shift);
                goto update_last;
            }
            face = body ? ModelFindFaceAt(body, (S32)MouseX, (S32)MouseY, &info) : -1;

            if (face >= 0) {
                if (shift)
                    ModelToggleFaceSelection(body, face, &info);
                else
                    ModelSetSingleFaceSelection(body, face, &info);
                hit = TRUE;
            }
        }
        if (ModelSelectActive(MODEL_SELECT_SPHERE)) {
            ModelSphereInfo info;
            S32 sphere;

            sphere = body ? ModelFindSphereAt(body, (S32)MouseX, (S32)MouseY, &info) : -1;
            if (sphere >= 0) {
                if (shift)
                    ModelToggleSphereSelection(body, sphere, &info);
                else
                    ModelSetSingleSphereSelection(body, sphere, &info);
                hit = TRUE;
            }
        }
        if (ModelSelectActive(MODEL_SELECT_LINE)) {
            ModelLineInfo info;
            S32 line;

            line = body ? ModelFindLineAt(body, (S32)MouseX, (S32)MouseY, &info) : -1;
            if (line >= 0) {
                if (shift)
                    ModelToggleLineSelection(body, line, &info);
                else
                    ModelSetSingleLineSelection(body, line, &info);
                hit = TRUE;
            }
        }
        if (ModelSelectActive(MODEL_SELECT_BONE)) {
            S32 bone = body ? ModelFindBoneAt(body, (S32)MouseX, (S32)MouseY) : -1;

            if (bone >= 0) {
                if (shift)
                    ModelToggleBoneSelection(body, bone);
                else
                    ModelSetSingleBoneSelection(body, bone);
                hit = TRUE;
            }
        }
        if (!hit AND !shift) {
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
        }
    }

update_last:
    ModelLastInput = Input;
    ModelLastKey = MyKey;
    ModelLastClick = Click;
    ModelLastSave = save;
}
