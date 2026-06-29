static U8 *ModelGetAnim(S32 anim_id, S32 *out_size) {
    S32 size = 0;
    U8 *anim;

    if (out_size)
        *out_size = 0;
    if (!HQR_Anims OR anim_id < 0 OR anim_id >= ModelAnimCount)
        return NULL;
    anim = (U8 *)HQR_Get(HQR_Anims, anim_id, &size);
    if (!anim OR size < 8)
        return NULL;
    if (out_size)
        *out_size = size;
    return anim;
}

static S32 ModelAnimGroupCount(S32 anim_id) {
    S32 size;
    U16 *anim = (U16 *)ModelGetAnim(anim_id, &size);
    S32 expected_size;

    if (!anim)
        return -1;
    if (anim[0] <= 0 OR anim[1] <= 0 OR anim[1] > MODEL_MAX_GROUPS)
        return -1;
    expected_size = 8 + anim[0] * (anim[1] * 8 + 8);
    if (expected_size > size)
        return -1;
    return anim[1];
}

static S32 ModelAnimValid(S32 anim_id) {
    return ModelAnimGroupCount(anim_id) > 0 ? TRUE : FALSE;
}

static S32 ModelAnimChoiceContains(S32 anim_id) {
    S32 i;

    for (i = 0; i < ModelAnimChoiceCount; i++) {
        if (ModelAnimChoices[i] == anim_id)
            return TRUE;
    }
    return FALSE;
}

static void ModelAddAnimChoice(S32 anim_id) {
    if (!ModelAnimValid(anim_id))
        return;
    if (ModelAnimChoiceCount >= MODEL_MAX_ANIM_CHOICES)
        return;
    if (ModelAnimChoiceContains(anim_id))
        return;
    ModelAnimChoices[ModelAnimChoiceCount++] = anim_id;
}

static S32 ModelFile3DCount(void) {
    U32 first_offset;

    if (!BufferFile3D)
        return 0;
    first_offset = *(U32 *)BufferFile3D;
    if (first_offset == 0 OR(first_offset & 3))
        return 0;
    return (S32)(first_offset / 4);
}

static S32 ModelFicheReferencesBody(U8 *fiche, S32 body_id) {
    U8 *ptr = fiche;
    S32 guard = 0;

    if (!fiche)
        return FALSE;
    while (guard++ < 10000) {
        U8 command = *ptr++;

        if (command == 255)
            break;
        if (command == F_BODY) {
            U8 size;
            S16 hqr_body;

            ptr++;
            size = *ptr++;
            if (size >= 3) {
                hqr_body = *(S16 *)ptr;
                if (hqr_body == body_id)
                    return TRUE;
            }
            ptr += size - 1;
        } else if (command == F_ANIM) {
            ptr += 2;
            ptr += *ptr;
        } else {
            ptr++;
            ptr += *ptr;
        }
    }
    return FALSE;
}

static void ModelCollectFicheAnimations(U8 *fiche) {
    U8 *ptr = fiche;
    S32 guard = 0;

    if (!fiche)
        return;
    while (guard++ < 10000) {
        U8 command = *ptr++;

        if (command == 255)
            break;
        if (command == F_ANIM) {
            U8 size;
            S16 hqr_anim;

            ptr += 2;
            size = *ptr++;
            if (size >= 3) {
                hqr_anim = *(S16 *)ptr;
                ModelAddAnimChoice(hqr_anim);
            }
            ptr += size - 1;
        } else if (command == F_BODY) {
            ptr++;
            ptr += *ptr;
        } else {
            ptr++;
            ptr += *ptr;
        }
    }
}

static void ModelRefreshFicheAnimChoices(T_BODY_HEADER *body) {
    S32 count;
    S32 i;

    if (!body OR ModelArchiveBase(ModelArchive) != MODEL_ARCHIVE_BODY)
        return;
    count = ModelFile3DCount();
    for (i = 0; i < count; i++) {
        U8 *fiche = LoadFile3D(i);

        if (ModelFicheReferencesBody(fiche, ModelBodyId)) {
            ModelCollectFicheAnimations(fiche);
        }
    }
    if (ModelAnimChoiceCount > 0)
        ModelAnimChoiceFromFiche = TRUE;
}

static void ModelResetAnimPreview(void) {
    ObjectClear(&ModelPreviewObj);
    ModelPreviewReady = FALSE;
    ModelPreviewArchiveBase = -1;
    ModelPreviewBodyId = -1;
    ModelPreviewBody = NULL;
    ModelPreviewAnimId = -2;
}

static void ModelRefreshAnimChoices(T_BODY_HEADER *body) {
    S32 i;
    S32 old_anim = ModelAnimId;

    if (ModelAnimCount <= 0 AND HQR_Anims)
        ModelAnimCount = HQF_NbRes(HQR_Anims->Name);

    if (ModelAnimChoiceArchiveBase == ModelArchiveBase(ModelArchive)
            AND ModelAnimChoiceBodyId == ModelBodyId
            AND ModelAnimChoiceBodyGroups == (body ? body->NbGroupes : -1)) {
        return;
    }

    ModelAnimChoiceCount = 0;
    ModelAnimChoiceFromFiche = FALSE;
    ModelAnimChoiceArchiveBase = ModelArchiveBase(ModelArchive);
    ModelAnimChoiceBodyId = ModelBodyId;
    ModelAnimChoiceBodyGroups = body ? body->NbGroupes : -1;

    ModelRefreshFicheAnimChoices(body);
    if (ModelAnimChoiceCount <= 0 AND body AND ModelArchiveBase(ModelArchive) == MODEL_ARCHIVE_BODY) {
        for (i = 0; i < ModelAnimCount AND ModelAnimChoiceCount < MODEL_MAX_ANIM_CHOICES; i++) {
            ModelAddAnimChoice(i);
        }
    }

    if (ModelAnimChoiceCount <= 0) {
        ModelAnimId = -1;
    } else if (ModelAnimId != -1 AND !ModelAnimValid(ModelAnimId)) {
        ModelAnimId = -1;
    }

    if (old_anim != ModelAnimId)
        ModelResetAnimPreview();
}

static S32 ModelAnimChoiceIndex(S32 anim_id) {
    S32 i;

    for (i = 0; i < ModelAnimChoiceCount; i++) {
        if (ModelAnimChoices[i] == anim_id)
            return i;
    }
    return -1;
}

static void ModelChangeAnimId(S32 delta, T_BODY_HEADER *body) {
    S32 index;

    ModelRefreshAnimChoices(body);
    if (ModelAnimChoiceCount <= 0)
        return;
    index = ModelAnimChoiceIndex(ModelAnimId);
    if (index < 0)
        index = delta < 0 ? ModelAnimChoiceCount - 1 : 0;
    else
        index += delta;
    while (index < 0)
        index += ModelAnimChoiceCount;
    while (index >= ModelAnimChoiceCount)
        index -= ModelAnimChoiceCount;
    ModelAnimId = ModelAnimChoices[index];
    ModelResetAnimPreview();
}

static void ModelSetAnimIdFromNumber(S32 value, T_BODY_HEADER *body) {
    S32 i;

    ModelRefreshAnimChoices(body);
    if (value == -1) {
        ModelAnimId = -1;
        ModelResetAnimPreview();
        return;
    }
    if (ModelAnimChoiceCount <= 0) {
        ModelAnimId = -1;
        ModelResetAnimPreview();
        return;
    }
    if (ModelAnimValid(value)) {
        ModelAnimId = value;
        ModelResetAnimPreview();
        return;
    }
    for (i = 0; i < ModelAnimChoiceCount; i++) {
        if (ModelAnimChoices[i] >= value) {
            ModelAnimId = ModelAnimChoices[i];
            ModelResetAnimPreview();
            ModelSetStatus("Animation %d is not available; using %d", (int)value, (int)ModelAnimId);
            return;
        }
    }
    ModelAnimId = ModelAnimChoices[ModelAnimChoiceCount - 1];
    ModelResetAnimPreview();
    ModelSetStatus("Animation %d is not available; using %d", (int)value, (int)ModelAnimId);
}

static S32 ModelEnsureAnimPreview(T_BODY_HEADER *body) {
    if (!body OR !BufferTexture OR ModelArchiveBase(ModelArchive) != MODEL_ARCHIVE_BODY)
        return FALSE;
    ModelRefreshAnimChoices(body);
    if (ModelAnimId < 0)
        return FALSE;
    if (ModelPreviewReady
            AND ModelPreviewArchiveBase == ModelArchiveBase(ModelArchive)
            AND ModelPreviewBodyId == ModelBodyId
            AND ModelPreviewBody == body
            AND ModelPreviewAnimId == ModelAnimId) {
        return TRUE;
    }

    ObjectClear(&ModelPreviewObj);
    ModelPreviewObj.Texture = BufferTexture;
    ModelPreviewObj.NextTexture = BufferTexture;
    ObjectInitBody(&ModelPreviewObj, body);
    ObjectInitAnim(&ModelPreviewObj, (void *)(uintptr_t)(U32)ModelAnimId);
    ModelPreviewObj.X = 0;
    ModelPreviewObj.Y = 0;
    ModelPreviewObj.Z = 0;
    ModelPreviewObj.Alpha = 0;
    ModelPreviewObj.Beta = 0;
    ModelPreviewObj.Gamma = 0;

    ModelPreviewReady = TRUE;
    ModelPreviewArchiveBase = ModelArchiveBase(ModelArchive);
    ModelPreviewBodyId = ModelBodyId;
    ModelPreviewBody = body;
    ModelPreviewAnimId = ModelAnimId;
    return TRUE;
}

static T_OBJ_3D *ModelAnimPreviewObject(T_BODY_HEADER *body) {
    if (!ModelEnsureAnimPreview(body))
        return NULL;
    ObjectSetInterAnim(&ModelPreviewObj);
    ModelPreviewObj.X = 0;
    ModelPreviewObj.Y = 0;
    ModelPreviewObj.Z = 0;
    return &ModelPreviewObj;
}
