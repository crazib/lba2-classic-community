void ModelEditor_Run(void) {
    S32 done = FALSE;

    ModelBodyCount = HQR_Bodys ? HQF_NbRes(HQR_Bodys->Name) : 0;
    ModelObjFixCount = HQR_ObjFix ? HQF_NbRes(HQR_ObjFix->Name) : 0;
    ModelAnimCount = HQR_Anims ? HQF_NbRes(HQR_Anims->Name) : 0;
    ModelScanModArchives();
    ModelResetCamera();
    ModelArchive = MODEL_ARCHIVE_BODY;
    ModelBodyArchiveId = 0;
    ModelObjFixArchiveId = 0;
    ModelBodyId = 0;
    ModelAnimId = -1;
    ModelAnimChoiceCount = 0;
    ModelAnimChoiceFromFiche = FALSE;
    ModelAnimChoiceBodyGroups = -1;
    ModelAnimChoiceArchiveBase = -1;
    ModelAnimChoiceBodyId = -1;
    ModelResetAnimPreview();
    ModelFloorGrid = TRUE;
    ModelLoadState();
    ModelMode = MODEL_MODE_VIEW;
    ModelSelectMode = MODEL_SELECT_FACE;
    ModelSelectMask = MODEL_SELECT_MASK_FACE;
    ModelInspectorVisible = TRUE;
    ModelInspectorField = MODEL_FIELD_ARCHIVE;
    ModelXray = FALSE;
    ModelBoxSelectMode = FALSE;
    ModelBoxSelecting = FALSE;
    ModelEditingNumber = FALSE;
    ModelLastInput = 0;
    ModelLastKey = 0;
    ModelLastClick = 0;
    ModelLastSave = FALSE;
    ModelBlockInputUntilRelease = FALSE;
    ModelStatusText[0] = '\0';
    ModelStatusUntil = 0;
    ModelClearVertexSelection();
    ModelClearFaceSelection();
    ModelClearSphereSelection();
    ModelClearLineSelection();
    InitWaitNoInput(ALL_INPUTS);
    InitWaitNoKey();
    ShowMouse(TRUE);
    ModelApplyEditorPalette();

    while (!done) {
        T_BODY_HEADER *body;

        ManageTime();
        ModelHandleInput(&done);
        body = ModelGetBody();

        Box(0, 0, (S32)ModeDesiredX - 1, (S32)ModeDesiredY - 1, 0);
        ModelApplyEditorPalette();
        ModelDrawGrid();
        ModelDrawBody(body);
        ModelDrawBoxSelection();
        ModelApplyEditorPalette();
        ModelDrawColorPanel(body);
        ModelDrawInspector(body);
        ModelDrawStatus();
        BoxStaticAdd(0, 0, (S32)ModeDesiredX - 1, (S32)ModeDesiredY - 1);
        BoxUpdate();
    }

    if (ModelCameraMode == MODEL_CAMERA_FREE_NAV)
        ModelSetRelativeMouse(FALSE);
    ModelCancelGrab(ModelLoadedBody);
    ModelSaveState();
    if (ModelLoadedBody) {
        Free(ModelLoadedBody);
        ModelLoadedBody = NULL;
    }
    ModelLoadedBodySize = 0;
    if (ModelVertexSelection) {
        Free(ModelVertexSelection);
        ModelVertexSelection = NULL;
    }
    if (ModelFaceSelection) {
        Free(ModelFaceSelection);
        ModelFaceSelection = NULL;
    }
    if (ModelSphereSelection) {
        Free(ModelSphereSelection);
        ModelSphereSelection = NULL;
    }
    if (ModelLineSelection) {
        Free(ModelLineSelection);
        ModelLineSelection = NULL;
    }
    if (ModelBoneSelection) {
        Free(ModelBoneSelection);
        ModelBoneSelection = NULL;
    }
    ModelVertexSelectionCapacity = 0;
    ModelFaceSelectionCapacity = 0;
    ModelSphereSelectionCapacity = 0;
    ModelLineSelectionCapacity = 0;
    ModelBoneSelectionCapacity = 0;
    ModelSelectedVertexCount = 0;
    ModelSelectedFaceCount = 0;
    ModelClearSphereSelection();
    ModelClearLineSelection();
    ModelClearBoneSelection();
    ModelClearGrabState();
    ModelResetAnimPreview();
    ModelLoadedArchive = -1;
    ModelLoadedBodyId = -1;
}
