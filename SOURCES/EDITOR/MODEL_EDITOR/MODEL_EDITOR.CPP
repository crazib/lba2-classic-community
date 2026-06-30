#include "MODEL_EDITOR.H"

#include "../../C_EXTERN.H"
#include "../../DIRECTORIES.H"
#include "../FREECAM_MATH.H"
#include "../../INPUT.H"
#include "../../INTEXT.H"

#include <3D/CAMERA.H>
#include <3D/PROJ.H>
#include <3D/LPROJ.H>
#include <3D/LIROT3D.H>
#include <3D/LROT3D.H>
#include <ANIM.H>
#include <OBJECT/AFF_OBJ.H>
#include <POLYGON/POLY.H>
#include <SVGA/AFFSTR.H>
#include <SVGA/BOX.H>
#include <SVGA/CLIP.H>
#include <SVGA/DIRTYBOX.H>
#include <SVGA/FIL_LINE.H>
#include <SVGA/GPRINTF.H>
#include <SVGA/RECT.H>
#include <SVGA/VIDEO.H>
#include <SYSTEM/FILES.H>
#include <SYSTEM/INPUT.H>
#include <SYSTEM/KEYBOARD.H>
#include <SYSTEM/KEYBOARD_KEYS.H>
#include <SYSTEM/LIMITS.H>
#include <SYSTEM/MOUSE.H>
#include <SYSTEM/TIMER.H>
#include <SYSTEM/WINDOW.H>
#include <SYSTEM/HQFILE.H>
#include <SYSTEM/HQRLOAD.H>
#include <SYSTEM/HQRRESS.H>
#include <SYSTEM/N_MALLOC.H>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

extern S32 PalLevel;
extern TYPE_PT Obj_ListProjectedPoints[];
extern TYPE_MAT TabMat[];

#define MODEL_ARCHIVE_BODY 0
#define MODEL_ARCHIVE_OBJFIX 1
#define MODEL_ARCHIVE_BODY_MOD 2
#define MODEL_ARCHIVE_OBJFIX_MOD 3
#define MODEL_ARCHIVE_COUNT 4

#define MODEL_MODE_VIEW 0
#define MODEL_MODE_EDIT 1

#define MODEL_SELECT_VERTEX 0
#define MODEL_SELECT_FACE 1
#define MODEL_SELECT_SPHERE 2
#define MODEL_SELECT_LINE 3
#define MODEL_SELECT_BONE 4

#define MODEL_SELECT_MASK_VERTEX (1 << MODEL_SELECT_VERTEX)
#define MODEL_SELECT_MASK_FACE (1 << MODEL_SELECT_FACE)
#define MODEL_SELECT_MASK_SPHERE (1 << MODEL_SELECT_SPHERE)
#define MODEL_SELECT_MASK_LINE (1 << MODEL_SELECT_LINE)
#define MODEL_SELECT_MASK_BONE (1 << MODEL_SELECT_BONE)

#define MODEL_FIELD_ARCHIVE 0
#define MODEL_FIELD_BODY_ID 1
#define MODEL_FIELD_ANIM_ID 2

#define MODEL_EDIT_FIELD_TYPE 0
#define MODEL_EDIT_FIELD_COLOR 1
#define MODEL_EDIT_FIELD_NORMAL 2
#define MODEL_EDIT_FIELD_P1 3
#define MODEL_EDIT_FIELD_P2 4
#define MODEL_EDIT_FIELD_PARENT 5
#define MODEL_EDIT_FIELD_ORG_POINT 6
#define MODEL_EDIT_FIELD_BONE_INDEX 7

#define MODEL_NUMBER_BODY_ID 0
#define MODEL_NUMBER_ANIM_ID 1
#define MODEL_NUMBER_FACE_TYPE 2
#define MODEL_NUMBER_FACE_COLOR 3
#define MODEL_NUMBER_FACE_NORMAL 4
#define MODEL_NUMBER_SPHERE_COLOR 5
#define MODEL_NUMBER_LINE_COLOR 6
#define MODEL_NUMBER_LINE_P1 7
#define MODEL_NUMBER_LINE_P2 8
#define MODEL_NUMBER_BONE_PARENT 9
#define MODEL_NUMBER_BONE_ORG_POINT 10
#define MODEL_NUMBER_BONE_INDEX 11

#define MODEL_MASK_QUADRILATERE (1 << 15)
#define MODEL_MASK_ENVIRONMENT (1 << 14)
#define MODEL_FACE_LIGHT 0
#define MODEL_FACE_TEXTURE 1
#define MODEL_FACE_ENV 2
#define MODEL_POLY_DITHER_TABLE 7
#define MODEL_MAX_POLY_TYPE 23
#define MODEL_MAX_POINTS 550 * 10
#define MODEL_MAX_SORT_ENTRIES 550 * 10
#define MODEL_MAX_GROUPS 30
#define MODEL_PROJECTED_POINT_LIMIT 550
#define MODEL_SELECTED_FACE_FILL 248
#define MODEL_SELECTED_FACE_LINE 249
#define MODEL_WIRE_COLOR 250
#define MODEL_TEXT_MUTED 251

#define MODEL_CAMERA_ORBIT 0
#define MODEL_CAMERA_FREE_LOCKED 1
#define MODEL_CAMERA_FREE_NAV 2

#define MODEL_FREECAM_MOVE_PER_MS 8.0f
#define MODEL_FREECAM_FAST_MULTIPLIER 4.0f
#define MODEL_FREECAM_SLOW_MULTIPLIER 0.25f
#define MODEL_FREECAM_MOUSE_SENSITIVITY 3.0f
#define MODEL_FREECAM_PITCH_LIMIT 1000
#define MODEL_FREECAM_MAX_DT 100

#define MODEL_STATE_FILE "model_editor_state.txt"
#define MODEL_MAX_ANIM_CHOICES 4096

typedef struct
{
    U16 TypePoly;
    U16 NbPoly;
    U32 OffNextType;
} ModelPolyHeader;

typedef struct
{
    U16 OrgGroupe;
    U16 OrgPoint;
    U16 NbPts;
    U16 NbNorm;
} ModelGroupInfo;

typedef struct
{
    U16 P1;
    U16 P2;
    U16 P3;
    U16 __padding;
    U16 Couleur;
    U16 Normale;
} ModelPoly3Light;

typedef struct
{
    U16 P1;
    U16 P2;
    U16 P3;
    U16 HandleText;
    U16 Couleur;
    U16 Normale;
    U16 U1;
    U16 V1;
    U16 U2;
    U16 V2;
    U16 U3;
    U16 V3;
} ModelPoly3Texture;

typedef struct
{
    U16 P1;
    U16 P2;
    U16 P3;
    U16 HandleEnv;
    U16 Couleur;
    U16 Normale;
    U16 Scale;
    U16 __padding;
} ModelPoly3Env;

typedef struct
{
    U16 P1;
    U16 P2;
    U16 P3;
    U16 P4;
    U16 Couleur;
    U16 Normale;
} ModelPoly4Light;

typedef struct
{
    U16 P1;
    U16 P2;
    U16 P3;
    U16 P4;
    U16 Couleur;
    U16 Normale;
    U16 U1;
    U16 V1;
    U16 U2;
    U16 V2;
    U16 U3;
    U16 V3;
    U16 U4;
    U16 V4;
    U16 HandleText;
    U16 __padding;
} ModelPoly4Texture;

typedef struct
{
    U16 P1;
    U16 P2;
    U16 P3;
    U16 P4;
    U16 Couleur;
    U16 Normale;
    U16 Scale;
    U16 HandleEnv;
} ModelPoly4Env;

typedef struct
{
    U16 Type;
    U16 Coul;
    U16 P1;
    U16 P2;
} ModelLineDisk;

typedef struct
{
    S32 index;
    S32 offset;
    U16 type;
    U16 color;
    U16 p1;
    U16 p2;
} ModelLineInfo;

typedef struct
{
    U16 Type;
    U16 Coul;
    U16 P1;
    U16 Rayon;
} ModelSphereDisk;

typedef struct
{
    S32 index;
    S32 offset;
    U16 type;
    U16 color;
    U16 point;
    U16 radius;
} ModelSphereInfo;

typedef struct
{
    S32 index;
    S32 header_offset;
    S32 offset;
    U16 raw_type_poly;
    S32 type_poly;
    S32 face_type;
    S32 is_quad;
    S32 point_count;
    U16 points[4];
    U16 color;
    U16 normal;
    U16 texture;
    U16 scale;
    U16 u[4];
    U16 v[4];
} ModelFaceInfo;

typedef struct
{
    S32 count;
    S32 type_value;
    S32 type_mixed;
    S32 color_value;
    S32 color_mixed;
    S32 raw_color_value;
    S32 raw_color_mixed;
    S32 p1_value;
    S32 p1_mixed;
    S32 p2_value;
    S32 p2_mixed;
} ModelLineSelectionStats;

typedef struct
{
    S32 count;
    S32 type_value;
    S32 type_mixed;
    S32 color_value;
    S32 color_mixed;
    S32 raw_color_value;
    S32 raw_color_mixed;
    S32 normal_value;
    S32 normal_mixed;
    S32 texture_value;
    S32 texture_mixed;
    S32 point_count_value;
    S32 point_count_mixed;
} ModelFaceSelectionStats;

typedef struct
{
    S32 count;
    S32 parent_value;
    S32 parent_mixed;
    S32 point_value;
    S32 point_mixed;
    S32 nb_pts_value;
    S32 nb_pts_mixed;
    S32 nb_norm_value;
    S32 nb_norm_mixed;
} ModelBoneSelectionStats;

typedef struct
{
    S32 list[256];
    S32 count;
    S32 per_row;
    S32 swatch_w;
    S32 swatch_h;
    S32 item_width;
    S32 row_height;
    S32 width;
    S32 height;
    S32 x0;
    S32 y0;
} ModelColorPanelLayout;

typedef struct
{
    T_OBJ_POINT *points;
    S32 *point_sources;
    U8 *point_selected;
    S32 point_count;
    ModelFaceInfo *faces;
    S32 face_count;
    ModelLineInfo *lines;
    S32 line_count;
    ModelSphereInfo *spheres;
    S32 sphere_count;
    ModelGroupInfo *bones;
    S32 *bone_sources;
    S32 bone_count;
    S32 bone_points_copied;
    S32 select_mask;
    S32 source_archive_base;
    S32 source_body_id;
} ModelClipboard;

typedef struct
{
    S32 *indices;
    T_OBJ_POINT *original_points;
    S32 count;
    S32 capacity;
    S32 start_mouse_x;
    S32 start_mouse_y;
    S32 axis;
} ModelGrabState;

static S32 ModelArchive = MODEL_ARCHIVE_BODY;
static S32 ModelBodyId = 0;
static S32 ModelMode = MODEL_MODE_VIEW;
static S32 ModelSelectMode = MODEL_SELECT_FACE;
static S32 ModelSelectMask = MODEL_SELECT_MASK_FACE;
static S32 ModelInspectorVisible = TRUE;
static S32 ModelInspectorField = MODEL_FIELD_ARCHIVE;
static S32 ModelLastKey = 0;
static U32 ModelLastInput = 0;
static S32 ModelLastClick = 0;
static S32 ModelLastSave = FALSE;
static S32 ModelBlockInputUntilRelease = FALSE;
static S32 ModelBoxSelectMode = FALSE;
static S32 ModelBoxSelecting = FALSE;
static S32 ModelBoxX0 = 0;
static S32 ModelBoxY0 = 0;
static S32 ModelBoxX1 = 0;
static S32 ModelBoxY1 = 0;
static S32 ModelGrabMode = FALSE;
static S32 ModelEditingNumber = FALSE;
static S32 ModelNumberTarget = MODEL_NUMBER_BODY_ID;
static char ModelNumberText[16];
static S32 ModelAlpha = 260;
static S32 ModelBeta = 0;
static S32 ModelPanX = 0;
static S32 ModelPanY = 0;
static S32 ModelZoom = 14500;
static S32 ModelOrtho = FALSE;
static S32 ModelTargetX = 0;
static S32 ModelTargetY = 0;
static S32 ModelTargetZ = 0;
static S32 ModelCameraMode = MODEL_CAMERA_ORBIT;
static S32 ModelFreeLastToggle = FALSE;
static U32 ModelFreeLastTime = 0;
static float ModelFreeX = 0.0f;
static float ModelFreeY = 0.0f;
static float ModelFreeZ = 0.0f;
static S32 ModelFreeAlpha = 0;
static S32 ModelFreeBeta = 0;
static S32 ModelFreeGamma = 0;
static S32 ModelSavedCameraMode = MODEL_CAMERA_ORBIT;
static S32 ModelSavedAlpha = 0;
static S32 ModelSavedBeta = 0;
static S32 ModelSavedPanX = 0;
static S32 ModelSavedPanY = 0;
static S32 ModelSavedZoom = 0;
static S32 ModelSavedOrtho = FALSE;
static float ModelSavedFreeX = 0.0f;
static float ModelSavedFreeY = 0.0f;
static float ModelSavedFreeZ = 0.0f;
static S32 ModelSavedFreeAlpha = 0;
static S32 ModelSavedFreeBeta = 0;
static S32 ModelSavedFreeGamma = 0;
static S32 ModelSelectedFace = -1;
static S32 ModelSelectedVertex = -1;
static S32 ModelSelectedSphere = -1;
static S32 ModelSelectedLine = -1;
static S32 ModelBodyArchiveId = 0;
static S32 ModelObjFixArchiveId = 0;
static S32 ModelBodyCount = 0;
static S32 ModelObjFixCount = 0;
static S32 ModelModBodyAvailable = FALSE;
static S32 ModelModObjFixAvailable = FALSE;
static S32 ModelFloorGrid = TRUE;
static S32 ModelXray = FALSE;
static S32 ModelAnimId = -1;
static S32 ModelAnimCount = 0;
static S32 ModelAnimChoices[MODEL_MAX_ANIM_CHOICES];
static S32 ModelAnimChoiceCount = 0;
static S32 ModelAnimChoiceFromFiche = FALSE;
static S32 ModelAnimChoiceBodyGroups = -1;
static S32 ModelAnimChoiceArchiveBase = -1;
static S32 ModelAnimChoiceBodyId = -1;
static T_OBJ_3D ModelPreviewObj;
static S32 ModelPreviewReady = FALSE;
static S32 ModelPreviewArchiveBase = -1;
static S32 ModelPreviewBodyId = -1;
static T_BODY_HEADER *ModelPreviewBody = NULL;
static S32 ModelPreviewAnimId = -2;
static S32 ModelLoadedArchive = -1;
static S32 ModelLoadedBodyId = -1;
static T_BODY_HEADER *ModelLoadedBody = NULL;
static U32 ModelLoadedBodySize = 0;
static U8 *ModelVertexSelection = NULL;
static U8 *ModelFaceSelection = NULL;
static U8 *ModelSphereSelection = NULL;
static U8 *ModelLineSelection = NULL;
static U8 *ModelBoneSelection = NULL;
static S32 ModelVertexSelectionCapacity = 0;
static S32 ModelFaceSelectionCapacity = 0;
static S32 ModelSphereSelectionCapacity = 0;
static S32 ModelLineSelectionCapacity = 0;
static S32 ModelBoneSelectionCapacity = 0;
static S32 ModelSelectedVertexCount = 0;
static S32 ModelSelectedFaceCount = 0;
static S32 ModelSelectedSphereCount = 0;
static S32 ModelSelectedLineCount = 0;
static S32 ModelSelectedBoneCount = 0;
static ModelFaceInfo ModelSelectedFaceInfo;
static ModelSphereInfo ModelSelectedSphereInfo;
static ModelLineInfo ModelSelectedLineInfo;
static S32 ModelSelectedBone = -1;
static ModelClipboard ModelClip;
static ModelGrabState ModelGrab;
static char ModelStatusText[160];
static U32 ModelStatusUntil = 0;

static void ModelClearFaceInfo(ModelFaceInfo *info);
static void ModelClearSphereInfo(ModelSphereInfo *info);
static void ModelClearLineInfo(ModelLineInfo *info);
static void ModelSyncBodyIdFromArchive(void);
static void ModelStoreBodyIdForArchive(void);
static S32 ModelArchiveAvailable(S32 archive);
static void ModelClampBodyId(void);
static S16 ModelClampS16(S32 value);
static S32 ModelClampEditValue(T_BODY_HEADER *body, S32 target, S32 value);
static void ModelResetAnimPreview(void);
static void ModelRefreshAnimChoices(T_BODY_HEADER *body);
static void ModelChangeAnimId(S32 delta, T_BODY_HEADER *body);
static S32 ModelFindFaceByIndex(T_BODY_HEADER *body, S32 wanted, ModelFaceInfo *out_info);
static S32 ModelReadLineInfo(T_BODY_HEADER *body, S32 index, ModelLineInfo *out_info);
static S32 ModelReadSphereInfo(T_BODY_HEADER *body, S32 index, ModelSphereInfo *out_info);
static S32 ModelBoneOriginPoint(T_BODY_HEADER *body, S32 index);
static void ModelSetSingleBoneSelection(T_BODY_HEADER *body, S32 index);
static void ModelCancelGrab(T_BODY_HEADER *body);

#include "MODEL_EDITOR_CORE.INC"
#include "MODEL_EDITOR_BODY.INC"
#include "MODEL_EDITOR_ANIM.INC"
#include "MODEL_EDITOR_CLIPBOARD.INC"
#include "MODEL_EDITOR_SELECTION.INC"
#include "MODEL_EDITOR_RENDER.INC"
#include "MODEL_EDITOR_INPUT.INC"
#include "MODEL_EDITOR_RUN.INC"
