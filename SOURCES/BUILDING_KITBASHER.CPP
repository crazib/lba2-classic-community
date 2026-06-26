#include "BUILDING_KITBASHER.H"

#include "BUILDING_KITBASHER_DATA.H"
#include "C_EXTERN.H"
#include "FLOW.H"
#include "GAMEMENU.H"
#include "INPUT.H"
#include "INVENT.H"
#include "INTEXT.H"

#include <SYSTEM/HQRRESS.H>

#include "3DEXT/LOADISLE.H"

#include <3D/CAMERA.H>
#include <3D/DATAMAT.H>
#include <3D/PROJ.H>
#include <OBJECT/AFF_OBJ.H>
#include <POLYGON/POLY.H>
#include <SVGA/AFFSTR.H>
#include <SVGA/BOX.H>
#include <SVGA/DIRTYBOX.H>
#include <SVGA/FIL_LINE.H>
#include <SVGA/GPRINTF.H>
#include <SVGA/PLOT.H>
#include <SVGA/RECT.H>
#include <SYSTEM/INPUT.H>
#include <SYSTEM/KEYBOARD.H>
#include <SYSTEM/KEYBOARD_KEYS.H>
#include <SYSTEM/LOG.H>
#include <SYSTEM/MOUSE.H>
#include <SYSTEM/TIMER.H>

#include <stdio.h>
#include <string.h>

extern S32 PalLevel;
extern U8 *ObjTexture;
extern T_HQR_HEADER *HQR_Isle_Obj;
extern TYPE_PT Obj_ListProjectedPoints[];

#define KIT_MODE_SOURCE 0
#define KIT_MODE_RECIPE 1

#define KIT_WALL_STONE_BRICK 0
#define KIT_WALL_HEAVY_STONE 1
#define KIT_WALL_PLACEHOLDER 2

#define KIT_COPING_NONE 0
#define KIT_COPING_STONE 1
#define KIT_COPING_BLUE 2
#define KIT_COPING_BLUE_HEAVY 3

#define KIT_ROOF_FLAT_WOOD 0
#define KIT_ROOF_FLAT_CONCRETE 1
#define KIT_ROOF_SLOPED_WOOD 2
#define KIT_ROOF_PYRAMID_BLUE 3
#define KIT_ROOF_HALF_CAPSULE_BLUE 4
#define KIT_ROOF_THATCHED 5
#define KIT_ROOF_PLACEHOLDER 6

#define KIT_DOOR_NONE 0
#define KIT_DOOR_REGULAR 1
#define KIT_DOOR_SHOP 2
#define KIT_DOOR_PHARMACY 3
#define KIT_DOOR_FERRY 4
#define KIT_DOOR_CAFE 5
#define KIT_DOOR_CRATE 6

#define KIT_MAX_SELECTED_FACES 128
#define KIT_MAX_FACE_TAGS 4096
#define KIT_MASK_QUADRILATERE (1 << 15)
#define KIT_MASK_ENVIRONMENT (1 << 14)
#define KIT_POLY_DITHER_TABLE 7
#define KIT_FACE_LIGHT 0
#define KIT_FACE_TEXTURE 1
#define KIT_FACE_ENV 2

typedef struct
{
    U16 TypePoly;
    U16 NbPoly;
    U32 OffNextType;
} KitPolyHeader;

typedef struct
{
    U16 P1;
    U16 P2;
    U16 P3;
    U16 __padding;
    U16 Couleur;
    U16 Normale;
} KitPoly3Light;

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
} KitPoly3Texture;

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
} KitPoly3Env;

typedef struct
{
    U16 P1;
    U16 P2;
    U16 P3;
    U16 P4;
    U16 Couleur;
    U16 Normale;
} KitPoly4Light;

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
} KitPoly4Texture;

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
} KitPoly4Env;

typedef struct KitFaceInfo
{
    S32 index;
    S32 offset;
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
} KitFaceInfo;

typedef struct KitFaceTag
{
    S32 used;
    S32 lm2_id;
    S32 face_index;
    S32 tag;
} KitFaceTag;

typedef struct BuildingSourceEntry
{
    const char *group;
    S32 lm2_id;
    const char *display_name;
    const char *wall_material;
    const char *coping_style;
    const char *roof_style;
    const char *door_type;
    const char *details;
    const char *missing_sides;
    const char *notes;
} BuildingSourceEntry;

typedef struct BuildingRecipe
{
    S32 width_segments;
    S32 depth_segments;
    S32 height_segments;
    S32 wall_material;
    S32 coping_style;
    S32 roof_style;
    S32 front_door;
    S32 front_door_position;
    S32 front_windows[8];
    S32 back_windows[8];
    S32 left_windows[8];
    S32 right_windows[8];
} BuildingRecipe;

static const BuildingSourceEntry BuildingSources[] = {
    {"citadel", 30, "Museum", "heavy_stone", "blue", "blue_roof", "regular",
     "Hole in roof; slanted front walls", "left, back, right",
     "Embedded partial building; use for heavy stone/blue roof tagging."},
    {"citadel", 32, "Pharmacy", "stone_brick", "blue", "flat_concrete", "pharmacy",
     "Door with pharmacy icon", "none",
     "Good complete stone-brick reference."},
    {"citadel", 33, "Baggage claim", "stone_brick", "blue", "flat_concrete", "crate",
     "Door with crate icon", "most left, back, some right",
     "Connected to pharmacy in-game; roof is not a complete square."},
    {"citadel", 35, "Kids' school", "heavy_stone", "blue_heavy", "pyramid_wood", "regular",
     "Two thin front windows", "left, back, half right",
     "Good heavy-stone + blue-heavy coping reference."},
    {"citadel", 48, "Ferry office", "stone_brick", "stone", "half_capsule_blue", "ferry",
     "Three mooring hooks; front window; two back windows", "none",
     "Good ferry icon and blue half-capsule roof reference."},
    {"citadel", 49, "Mr. Paul's house", "heavy_stone", "stone", "half_capsule_wood", "regular",
     "Mooring hook; left window and vines", "none",
     "Back texture tiling appears lower resolution."},
    {"citadel", 59, "Empty building", "stone_brick", "stone", "flat_wood", "none",
     "Stone coping on some sides", "side and back",
     "Partial shell useful for open-side tagging."},
    {"citadel", 60, "Shop", "stone_brick", "stone", "flat_wood", "shop",
     "Door with shop icon", "right, some left",
     "Good shop door/icon reference."},
    {"citadel", 61, "Shop corner piece", "stone_brick", "stone", "flat_wood", "none",
     "Ferry sign; trellis with flowers", "right",
     "Back wall is slightly sloped."},
    {"citadel", 80, "Neighbor's house", "heavy_stone", "none", "thatched", "regular",
     "Back and left windows; vines on right side", "none",
     "Good thatched-roof heavy-stone reference."},
};

static const char *WallMaterialNames[] = {"stone_brick", "heavy_stone", "placeholder"};
static const char *CopingStyleNames[] = {"none", "stone", "blue", "blue_heavy"};
static const char *RoofStyleNames[] = {
    "flat_wood", "flat_concrete", "sloped_wood", "pyramid_blue",
    "half_capsule_blue", "thatched", "placeholder"};
static const char *DoorTypeNames[] = {"none", "regular", "shop", "pharmacy", "ferry", "cafe", "crate"};
static const char *KitTagNames[] = {
    "None",
    "wall_heavy_stone",
    "wall_stone_brick",
    "coping_thin_blue",
    "coping_thick_blue",
    "coping_stone",
    "roof_flat_plain",
    "roof_flat_concrete",
    "roof_flat_wooden",
    "roof_blue_edge",
    "roof_wooden_half_capsule",
    "roof_wooden_pyramid",
    "roof_thatched",
    "door_regular",
    "door_with_window",
    "icon_shop",
    "icon_pharmacy",
    "icon_crate",
    "icon_ferry",
    "vines",
    "vines_flower",
    "trellis_vines",
    "trellis_flower",
    "trellis_flowers",
    "window",
    "window_thin",
    "mooring_hook",
};

static BuildingRecipe KitRecipe;
static S32 KitMode = KIT_MODE_SOURCE;
static S32 KitSourceIndex = 0;
static S32 KitRecipeField = 0;
static S32 KitRotation = 0;
static S32 KitZoom = 20;
static U32 KitLastInput = 0;
static S32 KitLastKey = 0;
static S32 KitLastClick = 0;
static S32 KitInspectorVisible = TRUE;
static S32 KitCurrentTag = 1;
static S32 KitSelectedFaces[KIT_MAX_SELECTED_FACES];
static S32 KitSelectedFaceCount = 0;
static KitFaceTag KitFaceTags[KIT_MAX_FACE_TAGS];

static void Kit_InitRecipe(void) {
    memset(&KitRecipe, 0, sizeof(KitRecipe));
    KitRecipe.width_segments = 3;
    KitRecipe.depth_segments = 2;
    KitRecipe.height_segments = 2;
    KitRecipe.wall_material = KIT_WALL_HEAVY_STONE;
    KitRecipe.coping_style = KIT_COPING_NONE;
    KitRecipe.roof_style = KIT_ROOF_THATCHED;
    KitRecipe.front_door = KIT_DOOR_REGULAR;
    KitRecipe.front_door_position = 1;
}

static S32 Kit_SourceCount(void) {
    return (S32)(sizeof(BuildingSources) / sizeof(BuildingSources[0]));
}

static S32 Kit_TagCount(void) {
    return (S32)(sizeof(KitTagNames) / sizeof(KitTagNames[0]));
}

static void Kit_ClampRecipe(void) {
    if (KitRecipe.width_segments < 1) KitRecipe.width_segments = 1;
    if (KitRecipe.width_segments > 8) KitRecipe.width_segments = 8;
    if (KitRecipe.depth_segments < 1) KitRecipe.depth_segments = 1;
    if (KitRecipe.depth_segments > 8) KitRecipe.depth_segments = 8;
    if (KitRecipe.height_segments < 1) KitRecipe.height_segments = 1;
    if (KitRecipe.height_segments > 6) KitRecipe.height_segments = 6;
    if (KitRecipe.wall_material < 0) KitRecipe.wall_material = KIT_WALL_PLACEHOLDER;
    if (KitRecipe.wall_material > KIT_WALL_PLACEHOLDER) KitRecipe.wall_material = 0;
    if (KitRecipe.coping_style < 0) KitRecipe.coping_style = KIT_COPING_BLUE_HEAVY;
    if (KitRecipe.coping_style > KIT_COPING_BLUE_HEAVY) KitRecipe.coping_style = 0;
    if (KitRecipe.roof_style < 0) KitRecipe.roof_style = KIT_ROOF_PLACEHOLDER;
    if (KitRecipe.roof_style > KIT_ROOF_PLACEHOLDER) KitRecipe.roof_style = 0;
    if (KitRecipe.front_door < 0) KitRecipe.front_door = KIT_DOOR_CRATE;
    if (KitRecipe.front_door > KIT_DOOR_CRATE) KitRecipe.front_door = 0;
    if (KitRecipe.front_door_position < 0) KitRecipe.front_door_position = KitRecipe.width_segments - 1;
    if (KitRecipe.front_door_position >= KitRecipe.width_segments) KitRecipe.front_door_position = 0;
}

static S32 Kit_ShiftDown(void) {
    return (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? TRUE : FALSE;
}

static void Kit_ClearSelection(void) {
    KitSelectedFaceCount = 0;
}

static S32 Kit_FindFaceTagSlot(S32 lm2_id, S32 face_index) {
    S32 i;

    for (i = 0; i < KIT_MAX_FACE_TAGS; i++) {
        if (KitFaceTags[i].used AND KitFaceTags[i].lm2_id == lm2_id AND KitFaceTags[i].face_index == face_index)
            return i;
    }
    return -1;
}

static S32 Kit_GetFaceTag(S32 lm2_id, S32 face_index) {
    S32 slot = Kit_FindFaceTagSlot(lm2_id, face_index);

    if (slot < 0)
        return 0;
    return KitFaceTags[slot].tag;
}

static void Kit_SetFaceTag(S32 lm2_id, S32 face_index, S32 tag) {
    S32 slot;
    S32 i;

    if (tag <= 0) {
        slot = Kit_FindFaceTagSlot(lm2_id, face_index);
        if (slot >= 0)
            KitFaceTags[slot].used = FALSE;
        return;
    }

    slot = Kit_FindFaceTagSlot(lm2_id, face_index);
    if (slot < 0) {
        for (i = 0; i < KIT_MAX_FACE_TAGS; i++) {
            if (!KitFaceTags[i].used) {
                slot = i;
                KitFaceTags[i].used = TRUE;
                KitFaceTags[i].lm2_id = lm2_id;
                KitFaceTags[i].face_index = face_index;
                break;
            }
        }
    }

    if (slot >= 0)
        KitFaceTags[slot].tag = tag;
}

static void Kit_SyncTagFromSelection(void) {
    const BuildingSourceEntry *entry = &BuildingSources[KitSourceIndex];
    S32 tag;

    if (KitSelectedFaceCount <= 0)
        return;

    tag = Kit_GetFaceTag(entry->lm2_id, KitSelectedFaces[KitSelectedFaceCount - 1]);
    if (tag > 0)
        KitCurrentTag = tag;
}

static void Kit_ApplyTagToSelection(void) {
    const BuildingSourceEntry *entry = &BuildingSources[KitSourceIndex];
    S32 i;

    for (i = 0; i < KitSelectedFaceCount; i++) {
        Kit_SetFaceTag(entry->lm2_id, KitSelectedFaces[i], KitCurrentTag);
    }
}

static void Kit_AdjustCurrentTag(S32 step) {
    KitCurrentTag += step;
    if (KitCurrentTag < 0)
        KitCurrentTag = Kit_TagCount() - 1;
    if (KitCurrentTag >= Kit_TagCount())
        KitCurrentTag = 0;
    Kit_ApplyTagToSelection();
}

static S32 Kit_IsFaceSelected(S32 face_index) {
    S32 i;

    for (i = 0; i < KitSelectedFaceCount; i++) {
        if (KitSelectedFaces[i] == face_index)
            return TRUE;
    }
    return FALSE;
}

static void Kit_ToggleSelectedFace(S32 face_index) {
    S32 i;

    for (i = 0; i < KitSelectedFaceCount; i++) {
        if (KitSelectedFaces[i] == face_index) {
            memmove(&KitSelectedFaces[i], &KitSelectedFaces[i + 1], (KitSelectedFaceCount - i - 1) * sizeof(KitSelectedFaces[0]));
            KitSelectedFaceCount--;
            return;
        }
    }

    if (KitSelectedFaceCount < KIT_MAX_SELECTED_FACES) {
        KitSelectedFaces[KitSelectedFaceCount] = face_index;
        KitSelectedFaceCount++;
    }
}

static void Kit_SelectSingleFace(S32 face_index) {
    KitSelectedFaceCount = 0;
    if (face_index >= 0) {
        KitSelectedFaces[0] = face_index;
        KitSelectedFaceCount = 1;
    }
}

static void *Kit_GetSourceBody(void) {
    const BuildingSourceEntry *entry = &BuildingSources[KitSourceIndex];
    T_HQR_HEADER *body_hqr;

    if (!HQR_Isle_Obj)
        return NULL;

    body_hqr = GetDecorLibraryHQR(0);
    if (!body_hqr OR !body_hqr->Name[0])
        return NULL;

    return HQR_Get(body_hqr, entry->lm2_id);
}

static S32 Kit_FaceRecordSize(S32 face_type, S32 is_quad) {
    if (is_quad) {
        if (face_type == KIT_FACE_LIGHT)
            return sizeof(KitPoly4Light);
        if (face_type == KIT_FACE_TEXTURE)
            return sizeof(KitPoly4Texture);
        return sizeof(KitPoly4Env);
    }

    if (face_type == KIT_FACE_LIGHT)
        return sizeof(KitPoly3Light);
    if (face_type == KIT_FACE_TEXTURE)
        return sizeof(KitPoly3Texture);
    return sizeof(KitPoly3Env);
}

static S32 Kit_FaceTypeFromHeader(KitPolyHeader *header, S32 is_quad) {
    S32 type_poly = header->TypePoly & 0xff;
    S32 mask = 0;

    memcpy(&mask, header, sizeof(S32));
    if (mask & KIT_MASK_ENVIRONMENT)
        return KIT_FACE_ENV;
    if (type_poly <= KIT_POLY_DITHER_TABLE)
        return KIT_FACE_LIGHT;
    return KIT_FACE_TEXTURE;
}

static void Kit_ReadFaceInfo(const void *poly, S32 face_index, S32 offset, S32 type_poly, S32 face_type, S32 is_quad, KitFaceInfo *info) {
    memset(info, 0, sizeof(*info));
    info->index = face_index;
    info->offset = offset;
    info->type_poly = type_poly;
    info->face_type = face_type;
    info->is_quad = is_quad;
    info->point_count = is_quad ? 4 : 3;

    if (is_quad) {
        if (face_type == KIT_FACE_LIGHT) {
            const KitPoly4Light *p = (const KitPoly4Light *)poly;
            info->points[0] = p->P1;
            info->points[1] = p->P2;
            info->points[2] = p->P3;
            info->points[3] = p->P4;
            info->color = p->Couleur;
            info->normal = p->Normale;
        } else if (face_type == KIT_FACE_TEXTURE) {
            const KitPoly4Texture *p = (const KitPoly4Texture *)poly;
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
        } else {
            const KitPoly4Env *p = (const KitPoly4Env *)poly;
            info->points[0] = p->P1;
            info->points[1] = p->P2;
            info->points[2] = p->P3;
            info->points[3] = p->P4;
            info->color = p->Couleur;
            info->normal = p->Normale;
            info->texture = p->HandleEnv;
            info->scale = p->Scale;
        }
    } else {
        if (face_type == KIT_FACE_LIGHT) {
            const KitPoly3Light *p = (const KitPoly3Light *)poly;
            info->points[0] = p->P1;
            info->points[1] = p->P2;
            info->points[2] = p->P3;
            info->color = p->Couleur;
            info->normal = p->Normale;
        } else if (face_type == KIT_FACE_TEXTURE) {
            const KitPoly3Texture *p = (const KitPoly3Texture *)poly;
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
        } else {
            const KitPoly3Env *p = (const KitPoly3Env *)poly;
            info->points[0] = p->P1;
            info->points[1] = p->P2;
            info->points[2] = p->P3;
            info->color = p->Couleur;
            info->normal = p->Normale;
            info->texture = p->HandleEnv;
            info->scale = p->Scale;
        }
    }
}

static const char *Kit_FaceTypeName(S32 face_type) {
    if (face_type == KIT_FACE_TEXTURE)
        return "texture";
    if (face_type == KIT_FACE_ENV)
        return "env";
    return "light";
}

static S32 Kit_PointValid(U16 point) {
    return Obj_ListProjectedPoints[point].X != -0x8000 OR Obj_ListProjectedPoints[point].Y != -0x8000;
}

static S32 Kit_FaceValid(const KitFaceInfo *info) {
    S32 i;

    for (i = 0; i < info->point_count; i++) {
        if (!Kit_PointValid(info->points[i]))
            return FALSE;
    }
    return TRUE;
}

static S64 Kit_Edge(S32 ax, S32 ay, S32 bx, S32 by, S32 px, S32 py) {
    return (S64)(px - ax) * (S64)(by - ay) - (S64)(py - ay) * (S64)(bx - ax);
}

static S32 Kit_PointInTri(S32 px, S32 py, U16 p0, U16 p1, U16 p2) {
    S32 ax = Obj_ListProjectedPoints[p0].X;
    S32 ay = Obj_ListProjectedPoints[p0].Y;
    S32 bx = Obj_ListProjectedPoints[p1].X;
    S32 by = Obj_ListProjectedPoints[p1].Y;
    S32 cx = Obj_ListProjectedPoints[p2].X;
    S32 cy = Obj_ListProjectedPoints[p2].Y;
    S64 e0 = Kit_Edge(ax, ay, bx, by, px, py);
    S64 e1 = Kit_Edge(bx, by, cx, cy, px, py);
    S64 e2 = Kit_Edge(cx, cy, ax, ay, px, py);

    return ((e0 >= 0 AND e1 >= 0 AND e2 >= 0) OR(e0 <= 0 AND e1 <= 0 AND e2 <= 0)) ? TRUE : FALSE;
}

static S32 Kit_FaceContainsPoint(const KitFaceInfo *info, S32 x, S32 y) {
    if (!Kit_FaceValid(info))
        return FALSE;
    if (Kit_PointInTri(x, y, info->points[0], info->points[1], info->points[2]))
        return TRUE;
    if (info->point_count == 4 AND Kit_PointInTri(x, y, info->points[0], info->points[2], info->points[3]))
        return TRUE;
    return FALSE;
}

static S32 Kit_FaceDepth(const KitFaceInfo *info) {
    S32 i;
    S32 z = 0;

    for (i = 0; i < info->point_count; i++) {
        z += Obj_ListRotatedPoints[info->points[i]].Z;
    }
    return z / info->point_count;
}

static S32 Kit_FindFaceAt(T_BODY_HEADER *body, S32 x, S32 y) {
    U8 *lines = (U8 *)body + body->OffLines;
    U8 *current = (U8 *)body + body->OffPolys;
    S32 body_offset = body->OffPolys;
    S32 face_index = 0;
    S32 best_face = -1;
    S32 best_depth = -0x7fffffff;

    while (current < lines) {
        KitPolyHeader *header = (KitPolyHeader *)current;
        S32 mask = 0;
        S32 is_quad;
        S32 face_type;
        S32 size;
        S32 i;

        memcpy(&mask, header, sizeof(S32));
        is_quad = (mask & KIT_MASK_QUADRILATERE) ? TRUE : FALSE;
        face_type = Kit_FaceTypeFromHeader(header, is_quad);
        current += sizeof(KitPolyHeader);
        body_offset += sizeof(KitPolyHeader);
        size = Kit_FaceRecordSize(face_type, is_quad);

        for (i = 0; i < header->NbPoly; i++) {
            KitFaceInfo info;
            S32 depth;

            Kit_ReadFaceInfo(current, face_index, body_offset, header->TypePoly & 0xff, face_type, is_quad, &info);
            if (Kit_FaceContainsPoint(&info, x, y)) {
                depth = Kit_FaceDepth(&info);
                if (best_face < 0 OR depth > best_depth) {
                    best_face = face_index;
                    best_depth = depth;
                }
            }
            current += size;
            body_offset += size;
            face_index++;
        }
    }

    return best_face;
}

static void Kit_DrawFaceOutline(const KitFaceInfo *info, S32 color) {
    S32 i;

    if (!Kit_FaceValid(info))
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

static void Kit_DrawTaggedFaceOutline(const KitFaceInfo *info) {
    S32 i;

    if (!Kit_FaceValid(info))
        return;

    for (i = 0; i < info->point_count; i++) {
        U16 p0 = info->points[i];
        U16 p1 = info->points[(i + 1) % info->point_count];
        S32 x0 = Obj_ListProjectedPoints[p0].X;
        S32 y0 = Obj_ListProjectedPoints[p0].Y;
        S32 x1 = Obj_ListProjectedPoints[p1].X;
        S32 y1 = Obj_ListProjectedPoints[p1].Y;
        S32 steps = abs(x1 - x0) > abs(y1 - y0) ? abs(x1 - x0) : abs(y1 - y0);
        S32 step;

        if (steps < 1)
            steps = 1;

        for (step = 0; step < steps; step += 8) {
            S32 end = step + 4;
            if (end > steps)
                end = steps;
            Line(x0 + (x1 - x0) * step / steps,
                 y0 + (y1 - y0) * step / steps,
                 x0 + (x1 - x0) * end / steps,
                 y0 + (y1 - y0) * end / steps,
                 12);
        }
    }
}

static void Kit_DrawSelectedFaces(T_BODY_HEADER *body) {
    const BuildingSourceEntry *entry = &BuildingSources[KitSourceIndex];
    U8 *lines = (U8 *)body + body->OffLines;
    U8 *current = (U8 *)body + body->OffPolys;
    S32 body_offset = body->OffPolys;
    S32 face_index = 0;

    while (current < lines) {
        KitPolyHeader *header = (KitPolyHeader *)current;
        S32 mask = 0;
        S32 is_quad;
        S32 face_type;
        S32 size;
        S32 i;

        memcpy(&mask, header, sizeof(S32));
        is_quad = (mask & KIT_MASK_QUADRILATERE) ? TRUE : FALSE;
        face_type = Kit_FaceTypeFromHeader(header, is_quad);
        current += sizeof(KitPolyHeader);
        body_offset += sizeof(KitPolyHeader);
        size = Kit_FaceRecordSize(face_type, is_quad);

        for (i = 0; i < header->NbPoly; i++) {
            S32 tag = Kit_GetFaceTag(entry->lm2_id, face_index);
            if (tag > 0 OR Kit_IsFaceSelected(face_index)) {
                KitFaceInfo info;
                Kit_ReadFaceInfo(current, face_index, body_offset, header->TypePoly & 0xff, face_type, is_quad, &info);
                if (tag > 0)
                    Kit_DrawTaggedFaceOutline(&info);
                if (Kit_IsFaceSelected(face_index))
                    Kit_DrawFaceOutline(&info, 14);
            }
            current += size;
            body_offset += size;
            face_index++;
        }
    }
}

static void Kit_Project(S32 x, S32 y, S32 z, S32 cx, S32 cy, S32 scale, S32 *sx, S32 *sy) {
    S32 rx = x;
    S32 rz = z;

    switch (KitRotation & 3) {
    case 1:
        rx = z;
        rz = -x;
        break;
    case 2:
        rx = -x;
        rz = -z;
        break;
    case 3:
        rx = -z;
        rz = x;
        break;
    }

    *sx = cx + (rx - rz) * scale;
    *sy = cy + (rx + rz) * scale / 2 - y * scale;
}

static S64 Kit_Edge(S32 ax, S32 ay, S32 bx, S32 by, S32 px, S32 py);
static void Kit_Face3(S32 x0, S32 y0, S32 z0,
                      S32 x1, S32 y1, S32 z1,
                      S32 x2, S32 y2, S32 z2,
                      S32 x3, S32 y3, S32 z3,
                      S32 cx, S32 cy, S32 scale, S32 color0, S32 color1, S32 pattern);

static void Kit_Line3(S32 x0, S32 y0, S32 z0, S32 x1, S32 y1, S32 z1, S32 cx, S32 cy, S32 scale, S32 color) {
    S32 sx0;
    S32 sy0;
    S32 sx1;
    S32 sy1;

    Kit_Project(x0, y0, z0, cx, cy, scale, &sx0, &sy0);
    Kit_Project(x1, y1, z1, cx, cy, scale, &sx1, &sy1);
    Line(sx0, sy0, sx1, sy1, color);
}

static void *Kit_GetRecipeTextureBody(void) {
    T_HQR_HEADER *body_hqr;

    if (!HQR_Isle_Obj)
        return NULL;

    body_hqr = GetDecorLibraryHQR(0);
    if (!body_hqr OR !body_hqr->Name[0])
        return NULL;

    return HQR_Get(body_hqr, 32);
}

static S32 Kit_SampleTexture(T_BODY_HEADER *texture_body, U16 texture, S32 u, S32 v, S32 fallback) {
    U32 *textures;
    U32 texture_info;
    U32 rep_mask;
    U32 map_index;
    U8 *texture_map;

    if (!ObjTexture OR !texture_body OR texture < 0 OR texture >= texture_body->NbTextures)
        return fallback;

    textures = (U32 *)((U8 *)texture_body + texture_body->OffTextures);
    texture_info = textures[texture];
    rep_mask = texture_info >> 16;
    texture_map = ObjTexture + (texture_info & 0xffff);
    map_index = rep_mask & ((((U32)u >> 8) & 0xff) | ((U32)v & 0xff00));

    return texture_map[map_index];
}

static S32 Kit_Min3(S32 a, S32 b, S32 c) {
    S32 r = a < b ? a : b;
    return r < c ? r : c;
}

static S32 Kit_Max3(S32 a, S32 b, S32 c) {
    S32 r = a > b ? a : b;
    return r > c ? r : c;
}

static void Kit_TexturedTri2(T_BODY_HEADER *texture_body,
                             S32 x0, S32 y0, S32 u0, S32 v0,
                             S32 x1, S32 y1, S32 u1, S32 v1,
                             S32 x2, S32 y2, S32 u2, S32 v2,
                             U16 texture, S32 fallback) {
    S32 min_x = Kit_Min3(x0, x1, x2);
    S32 max_x = Kit_Max3(x0, x1, x2);
    S32 min_y = Kit_Min3(y0, y1, y2);
    S32 max_y = Kit_Max3(y0, y1, y2);
    S64 denom = Kit_Edge(x0, y0, x1, y1, x2, y2);
    S32 x;
    S32 y;

    if (denom == 0)
        return;

    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= (S32)ModeDesiredX) max_x = (S32)ModeDesiredX - 1;
    if (max_y >= (S32)ModeDesiredY) max_y = (S32)ModeDesiredY - 1;

    for (y = min_y; y <= max_y; y++) {
        for (x = min_x; x <= max_x; x++) {
            S64 w0 = Kit_Edge(x1, y1, x2, y2, x, y);
            S64 w1 = Kit_Edge(x2, y2, x0, y0, x, y);
            S64 w2 = Kit_Edge(x0, y0, x1, y1, x, y);

            if ((denom > 0 AND w0 >= 0 AND w1 >= 0 AND w2 >= 0)
                OR(denom < 0 AND w0 <= 0 AND w1 <= 0 AND w2 <= 0)) {
                S32 u = (S32)((w0 * u0 + w1 * u1 + w2 * u2) / denom);
                S32 v = (S32)((w0 * v0 + w1 * v1 + w2 * v2) / denom);
                Plot(x, y, Kit_SampleTexture(texture_body, texture, u, v, fallback));
            }
        }
    }
}

static void Kit_TexturedQuad3(S32 x0, S32 y0, S32 z0, S32 u0, S32 v0,
                              S32 x1, S32 y1, S32 z1, S32 u1, S32 v1,
                              S32 x2, S32 y2, S32 z2, S32 u2, S32 v2,
                              S32 x3, S32 y3, S32 z3, S32 u3, S32 v3,
                              S32 cx, S32 cy, S32 scale, T_BODY_HEADER *texture_body,
                              U16 texture, S32 fallback0, S32 fallback1, S32 pattern) {
    S32 sx0;
    S32 sy0;
    S32 sx1;
    S32 sy1;
    S32 sx2;
    S32 sy2;
    S32 sx3;
    S32 sy3;

    if (!texture_body OR !ObjTexture) {
        Kit_Face3(x0, y0, z0, x1, y1, z1, x2, y2, z2, x3, y3, z3, cx, cy, scale, fallback0, fallback1, pattern);
        return;
    }

    Kit_Project(x0, y0, z0, cx, cy, scale, &sx0, &sy0);
    Kit_Project(x1, y1, z1, cx, cy, scale, &sx1, &sy1);
    Kit_Project(x2, y2, z2, cx, cy, scale, &sx2, &sy2);
    Kit_Project(x3, y3, z3, cx, cy, scale, &sx3, &sy3);

    Kit_TexturedTri2(texture_body, sx0, sy0, u0, v0, sx1, sy1, u1, v1, sx2, sy2, u2, v2, texture, fallback0);
    Kit_TexturedTri2(texture_body, sx0, sy0, u0, v0, sx2, sy2, u2, v2, sx3, sy3, u3, v3, texture, fallback0);
}

static void Kit_TexturedQuad2(T_BODY_HEADER *texture_body,
                              S32 x0, S32 y0, S32 u0, S32 v0,
                              S32 x1, S32 y1, S32 u1, S32 v1,
                              S32 x2, S32 y2, S32 u2, S32 v2,
                              S32 x3, S32 y3, S32 u3, S32 v3,
                              U16 texture, S32 fallback) {
    Kit_TexturedTri2(texture_body, x0, y0, u0, v0, x1, y1, u1, v1, x2, y2, u2, v2, texture, fallback);
    Kit_TexturedTri2(texture_body, x0, y0, u0, v0, x2, y2, u2, v2, x3, y3, u3, v3, texture, fallback);
}

static void Kit_ProjectRecipe(S32 x, S32 y, S32 z, S32 cx, S32 cy, S32 scale, S32 *sx, S32 *sy) {
    S32 rx = x;
    S32 rz = z;
    S32 camera_y;
    S32 camera_z;
    S32 factor;

    switch (KitRotation & 3) {
    case 1:
        rx = z;
        rz = -x;
        break;
    case 2:
        rx = -x;
        rz = -z;
        break;
    case 3:
        rx = -z;
        rz = x;
        break;
    }

    camera_y = y * 8 / 100 + rz * 3 / 100;
    camera_z = 260 + rz * 3 / 100 + y / 300;
    if (camera_z < 32)
        camera_z = 32;

    factor = scale * 150 / camera_z;
    *sx = cx + rx * factor / 100;
    *sy = cy - camera_y * factor / 8;
}

static void Kit_LineRecipe3(S32 x0, S32 y0, S32 z0, S32 x1, S32 y1, S32 z1, S32 cx, S32 cy, S32 scale, S32 color) {
    S32 sx0;
    S32 sy0;
    S32 sx1;
    S32 sy1;

    Kit_ProjectRecipe(x0, y0, z0, cx, cy, scale, &sx0, &sy0);
    Kit_ProjectRecipe(x1, y1, z1, cx, cy, scale, &sx1, &sy1);
    Line(sx0, sy0, sx1, sy1, color);
}

static void Kit_TexturedQuadRecipe3(S32 x0, S32 y0, S32 z0, S32 u0, S32 v0,
                                    S32 x1, S32 y1, S32 z1, S32 u1, S32 v1,
                                    S32 x2, S32 y2, S32 z2, S32 u2, S32 v2,
                                    S32 x3, S32 y3, S32 z3, S32 u3, S32 v3,
                                    S32 cx, S32 cy, S32 scale, T_BODY_HEADER *texture_body,
                                    U16 texture, S32 fallback0, S32 fallback1, S32 pattern) {
    S32 sx0;
    S32 sy0;
    S32 sx1;
    S32 sy1;
    S32 sx2;
    S32 sy2;
    S32 sx3;
    S32 sy3;

    if (!texture_body OR !ObjTexture) {
        S32 i;
        S32 steps = 28;
        Kit_ProjectRecipe(x0, y0, z0, cx, cy, scale, &sx0, &sy0);
        Kit_ProjectRecipe(x1, y1, z1, cx, cy, scale, &sx1, &sy1);
        Kit_ProjectRecipe(x2, y2, z2, cx, cy, scale, &sx2, &sy2);
        Kit_ProjectRecipe(x3, y3, z3, cx, cy, scale, &sx3, &sy3);

        for (i = 0; i <= steps; i++) {
            S32 ax = sx0 + (sx3 - sx0) * i / steps;
            S32 ay = sy0 + (sy3 - sy0) * i / steps;
            S32 bx = sx1 + (sx2 - sx1) * i / steps;
            S32 by = sy1 + (sy2 - sy1) * i / steps;
            S32 color = ((i + pattern) & 3) == 0 ? fallback1 : fallback0;
            Line(ax, ay, bx, by, color);
        }
        return;
    }

    Kit_ProjectRecipe(x0, y0, z0, cx, cy, scale, &sx0, &sy0);
    Kit_ProjectRecipe(x1, y1, z1, cx, cy, scale, &sx1, &sy1);
    Kit_ProjectRecipe(x2, y2, z2, cx, cy, scale, &sx2, &sy2);
    Kit_ProjectRecipe(x3, y3, z3, cx, cy, scale, &sx3, &sy3);

    Kit_TexturedTri2(texture_body, sx0, sy0, u0, v0, sx1, sy1, u1, v1, sx2, sy2, u2, v2, texture, fallback0);
    Kit_TexturedTri2(texture_body, sx0, sy0, u0, v0, sx2, sy2, u2, v2, sx3, sy3, u3, v3, texture, fallback0);
}

static void Kit_TemplateQuadRecipe3(const KitbashFaceTemplate *face,
                                    S32 x0, S32 y0, S32 z0,
                                    S32 x1, S32 y1, S32 z1,
                                    S32 x2, S32 y2, S32 z2,
                                    S32 x3, S32 y3, S32 z3,
                                    S32 cx, S32 cy, S32 scale, T_BODY_HEADER *texture_body,
                                    S32 fallback0, S32 fallback1, S32 pattern) {
    if (!face) {
        Kit_TexturedQuadRecipe3(x0, y0, z0, 45184, 22784,
                                x1, y1, z1, 59520, 22784,
                                x2, y2, z2, 59520, 2304,
                                x3, y3, z3, 45184, 2304,
                                cx, cy, scale, texture_body, 1, fallback0, fallback1, pattern);
        return;
    }

    Kit_TexturedQuadRecipe3(x0, y0, z0, face->p[0].u, face->p[0].v,
                            x1, y1, z1, face->p[1].u, face->p[1].v,
                            x2, y2, z2, face->p[2].u, face->p[2].v,
                            x3, y3, z3, face->p[3].u, face->p[3].v,
                            cx, cy, scale, texture_body, face->texture, fallback0, fallback1, pattern);
}

static void Kit_DrawCuboid(S32 cx, S32 cy, S32 width, S32 depth, S32 height, S32 scale, S32 color);

static void Kit_Face3(S32 x0, S32 y0, S32 z0,
                      S32 x1, S32 y1, S32 z1,
                      S32 x2, S32 y2, S32 z2,
                      S32 x3, S32 y3, S32 z3,
                      S32 cx, S32 cy, S32 scale, S32 color0, S32 color1, S32 pattern) {
    S32 sx0;
    S32 sy0;
    S32 sx1;
    S32 sy1;
    S32 sx2;
    S32 sy2;
    S32 sx3;
    S32 sy3;
    S32 i;
    S32 steps = 28;

    Kit_Project(x0, y0, z0, cx, cy, scale, &sx0, &sy0);
    Kit_Project(x1, y1, z1, cx, cy, scale, &sx1, &sy1);
    Kit_Project(x2, y2, z2, cx, cy, scale, &sx2, &sy2);
    Kit_Project(x3, y3, z3, cx, cy, scale, &sx3, &sy3);

    for (i = 0; i <= steps; i++) {
        S32 ax = sx0 + (sx3 - sx0) * i / steps;
        S32 ay = sy0 + (sy3 - sy0) * i / steps;
        S32 bx = sx1 + (sx2 - sx1) * i / steps;
        S32 by = sy1 + (sy2 - sy1) * i / steps;
        S32 color = ((i + pattern) & 3) == 0 ? color1 : color0;
        Line(ax, ay, bx, by, color);
    }
}

static S32 Kit_WallBaseColor(const char *material) {
    if (strcmp(material, "heavy_stone") == 0)
        return 7;
    if (strcmp(material, "placeholder") == 0)
        return 8;
    return 6;
}

static S32 Kit_RoofBaseColor(const char *roof) {
    if (strstr(roof, "blue"))
        return 9;
    if (strstr(roof, "thatched"))
        return 11;
    if (strstr(roof, "concrete"))
        return 8;
    return 4;
}

static void Kit_DrawTexturedCuboid(S32 cx, S32 cy, S32 width, S32 depth, S32 height, S32 scale,
                                   const char *wall_material, const char *roof_style) {
    S32 x0 = -width;
    S32 x1 = width;
    S32 z0 = -depth;
    S32 z1 = depth;
    S32 y0 = 0;
    S32 y1 = height;
    S32 wall = Kit_WallBaseColor(wall_material);
    S32 roof = Kit_RoofBaseColor(roof_style);

    Kit_Face3(x0, y0, z0, x1, y0, z0, x1, y1, z0, x0, y1, z0, cx, cy, scale, wall, wall + 1, 0);
    Kit_Face3(x1, y0, z0, x1, y0, z1, x1, y1, z1, x1, y1, z0, cx, cy, scale, wall + 1, wall, 1);
    Kit_Face3(x0, y1, z0, x1, y1, z0, x1, y1, z1, x0, y1, z1, cx, cy, scale, roof, roof + 1, 2);
    Kit_DrawCuboid(cx, cy, width, depth, height, scale, 15);
}

static void Kit_DrawRecipeTexturedCuboid(S32 cx, S32 cy, S32 width, S32 depth, S32 height, S32 scale) {
    T_BODY_HEADER *texture_body = (T_BODY_HEADER *)Kit_GetRecipeTextureBody();
    S32 unit = 100;
    S32 x0 = -width * unit;
    S32 x1 = width * unit;
    S32 z0 = -depth * unit;
    S32 z1 = depth * unit;
    S32 y0 = 0;
    S32 y1 = height * unit;
    S32 wall = Kit_WallBaseColor(WallMaterialNames[KitRecipe.wall_material]);
    S32 roof = Kit_RoofBaseColor(RoofStyleNames[KitRecipe.roof_style]);
    const KitbashFaceTemplate *heavy_wall0 = KitRecipe.wall_material == KIT_WALL_HEAVY_STONE ? KitbashData_FindFace("wall_heavy_stone", 0) : NULL;
    const KitbashFaceTemplate *heavy_wall1 = KitRecipe.wall_material == KIT_WALL_HEAVY_STONE ? KitbashData_FindFace("wall_heavy_stone", 1) : NULL;
    S32 seg;

    for (seg = 0; seg < KitRecipe.depth_segments; seg++) {
        S32 sz0 = z0 + seg * 2 * unit;
        S32 sz1 = sz0 + 2 * unit;

        Kit_TemplateQuadRecipe3(heavy_wall1, x1, y0, sz0,
                                x1, y0, sz1,
                                x1, y1, sz1,
                                x1, y1, sz0,
                                cx, cy, scale, texture_body, wall + 1, wall, seg);
        Kit_TemplateQuadRecipe3(heavy_wall1, x0, y0, sz1,
                                x0, y0, sz0,
                                x0, y1, sz0,
                                x0, y1, sz1,
                                cx, cy, scale, texture_body, wall + 1, wall, seg + 1);
    }

    for (seg = 0; seg < KitRecipe.width_segments; seg++) {
        S32 sx0 = x0 + seg * 2 * unit;
        S32 sx1 = sx0 + 2 * unit;

        Kit_TemplateQuadRecipe3(heavy_wall0, sx1, y0, z1,
                                sx0, y0, z1,
                                sx0, y1, z1,
                                sx1, y1, z1,
                                cx, cy, scale, texture_body, wall, wall + 1, seg + 2);
    }

    if (KitRecipe.roof_style == KIT_ROOF_THATCHED) {
        const KitbashFaceTemplate *roof0 = KitbashData_FindFace("roof_thatched", 0);
        const KitbashFaceTemplate *roof1 = KitbashData_FindFace("roof_thatched", 1);
        S32 xmid = (x0 + x1) / 2;
        Kit_TemplateQuadRecipe3(roof0, x0, y1, z0,
                                xmid, y1, z0,
                                xmid, y1, z1,
                                x0, y1, z1,
                                cx, cy, scale, texture_body, roof, roof + 1, 2);
        Kit_TemplateQuadRecipe3(roof1, xmid, y1, z0,
                                x1, y1, z0,
                                x1, y1, z1,
                                xmid, y1, z1,
                                cx, cy, scale, texture_body, roof, roof + 1, 3);
    } else if (KitRecipe.roof_style == KIT_ROOF_FLAT_WOOD) {
        S32 zmid = (z0 + z1) / 2;
        Kit_TexturedQuadRecipe3(x1, y1, zmid, 40050, 10582,
                                x0, y1, zmid, 19342, 10582,
                                x0, y1, z1, 19342, 28330,
                                x1, y1, z1, 40050, 28330,
                                cx, cy, scale, texture_body, 2, roof, roof + 1, 2);
        Kit_TexturedQuadRecipe3(x1, y1, z0, 44406, 40839,
                                x0, y1, z0, 44406, 22649,
                                x0, y1, zmid, 23178, 22649,
                                x1, y1, zmid, 23178, 40839,
                                cx, cy, scale, texture_body, 2, roof, roof + 1, 3);
    } else {
        Kit_TexturedQuadRecipe3(x0, y1, z0, 0, 0,
                                x1, y1, z0, 16384, 0,
                                x1, y1, z1, 16384, 16384,
                                x0, y1, z1, 0, 16384,
                                cx, cy, scale, texture_body, 1, roof, roof + 1, 2);
    }

    if (KitRecipe.coping_style == KIT_COPING_STONE) {
        S32 coping_y0 = y1 - 15;
        Kit_TexturedQuadRecipe3(x0, coping_y0, z0, 30150, 7290,
                                x1, coping_y0, z0, 30080, 37760,
                                x1, y1, z0, 32640, 37760,
                                x0, y1, z0, 32704, 7232,
                                cx, cy, scale, texture_body, 1, 8, 7, 4);
    }

    for (seg = 0; seg < KitRecipe.width_segments; seg++) {
        S32 sx0 = x0 + seg * 2 * unit;
        S32 sx1 = sx0 + 2 * unit;

        if (KitRecipe.front_door != KIT_DOOR_NONE AND seg == KitRecipe.front_door_position) {
            S32 door_x0 = sx0;
            S32 door_x1 = sx1;
            S32 door_w = door_x1 - door_x0;
            if (KitRecipe.front_door == KIT_DOOR_REGULAR) {
                const KitbashFaceTemplate *door0 = KitbashData_FindFace("door_regular", 0);
                const KitbashFaceTemplate *door1 = KitbashData_FindFace("door_regular", 1);
#define KIT_REG_DOOR_X(v) (door_x0 + (((v) + 1500) * door_w) / 1000)
#define KIT_REG_DOOR_Y(v) (y0 + ((v) * (y1 - y0)) / 2000)
                Kit_TemplateQuadRecipe3(door0, KIT_REG_DOOR_X(-1500), KIT_REG_DOOR_Y(2000), z0,
                                        KIT_REG_DOOR_X(-1000), KIT_REG_DOOR_Y(2000), z0,
                                        KIT_REG_DOOR_X(-1000), KIT_REG_DOOR_Y(0), z0,
                                        KIT_REG_DOOR_X(-1500), KIT_REG_DOOR_Y(0), z0,
                                        cx, cy, scale, texture_body, 14, 6, 0);
                Kit_TemplateQuadRecipe3(door1, KIT_REG_DOOR_X(-1000), KIT_REG_DOOR_Y(2000), z0,
                                        KIT_REG_DOOR_X(-500), KIT_REG_DOOR_Y(2000), z0,
                                        KIT_REG_DOOR_X(-500), KIT_REG_DOOR_Y(0), z0,
                                        KIT_REG_DOOR_X(-1000), KIT_REG_DOOR_Y(0), z0,
                                        cx, cy, scale, texture_body, 14, 6, 1);
#undef KIT_REG_DOOR_Y
#undef KIT_REG_DOOR_X
            } else {
#define KIT_DOOR_X(v) (door_x0 + (((v) + 750) * door_w) / 1200)
#define KIT_DOOR_Y(v) (y0 + ((v) * (y1 - y0)) / 2000)
                Kit_TexturedQuadRecipe3(KIT_DOOR_X(-500), KIT_DOOR_Y(700), z0, 58130, 23674,
                                    KIT_DOOR_X(-150), KIT_DOOR_Y(700), z0, 64968, 23674,
                                    KIT_DOOR_X(-150), KIT_DOOR_Y(1450), z0, 64968, 13708,
                                    KIT_DOOR_X(-500), KIT_DOOR_Y(1450), z0, 58130, 13708,
                                    cx, cy, scale, texture_body, 3, 14, 6, 0);
            Kit_TexturedQuadRecipe3(KIT_DOOR_X(-150), KIT_DOOR_Y(700), z0, 64972, 23692,
                                    KIT_DOOR_X(200), KIT_DOOR_Y(700), z0, 58368, 23680,
                                    KIT_DOOR_X(200), KIT_DOOR_Y(1450), z0, 58368, 13696,
                                    KIT_DOOR_X(-150), KIT_DOOR_Y(1450), z0, 64972, 13715,
                                    cx, cy, scale, texture_body, 3, 14, 6, 1);
            Kit_TexturedQuadRecipe3(KIT_DOOR_X(-500), KIT_DOOR_Y(1450), z0, 58130, 13708,
                                    KIT_DOOR_X(-750), KIT_DOOR_Y(2000), z0, 53376, 6400,
                                    KIT_DOOR_X(-750), KIT_DOOR_Y(0), z0, 53376, 33152,
                                    KIT_DOOR_X(-500), KIT_DOOR_Y(700), z0, 58130, 23674,
                                    cx, cy, scale, texture_body, 3, wall, wall + 1, 2);
            Kit_TexturedQuadRecipe3(KIT_DOOR_X(-500), KIT_DOOR_Y(1450), z0, 58130, 13708,
                                    KIT_DOOR_X(-150), KIT_DOOR_Y(1450), z0, 64968, 13708,
                                    KIT_DOOR_X(-150), KIT_DOOR_Y(2000), z0, 64968, 6400,
                                    KIT_DOOR_X(-750), KIT_DOOR_Y(2000), z0, 53248, 6400,
                                    cx, cy, scale, texture_body, 3, wall, wall + 1, 3);
            Kit_TexturedQuadRecipe3(KIT_DOOR_X(-750), KIT_DOOR_Y(0), z0, 53248, 32972,
                                    KIT_DOOR_X(-150), KIT_DOOR_Y(0), z0, 64968, 32972,
                                    KIT_DOOR_X(-150), KIT_DOOR_Y(700), z0, 64968, 23674,
                                    KIT_DOOR_X(-500), KIT_DOOR_Y(700), z0, 58130, 23674,
                                    cx, cy, scale, texture_body, 3, wall, wall + 1, 4);
            Kit_TexturedQuadRecipe3(KIT_DOOR_X(200), KIT_DOOR_Y(1450), z0, 58368, 13696,
                                    KIT_DOOR_X(450), KIT_DOOR_Y(2000), z0, 53247, 6400,
                                    KIT_DOOR_X(-150), KIT_DOOR_Y(2000), z0, 64972, 6400,
                                    KIT_DOOR_X(-150), KIT_DOOR_Y(1450), z0, 64972, 13715,
                                    cx, cy, scale, texture_body, 3, wall, wall + 1, 5);
            Kit_TexturedQuadRecipe3(KIT_DOOR_X(-150), KIT_DOOR_Y(0), z0, 64972, 33003,
                                    KIT_DOOR_X(450), KIT_DOOR_Y(0), z0, 53247, 33003,
                                    KIT_DOOR_X(200), KIT_DOOR_Y(700), z0, 58368, 23680,
                                    KIT_DOOR_X(-150), KIT_DOOR_Y(700), z0, 64972, 23692,
                                    cx, cy, scale, texture_body, 3, wall, wall + 1, 6);
            Kit_TexturedQuadRecipe3(KIT_DOOR_X(200), KIT_DOOR_Y(700), z0, 58368, 23680,
                                    KIT_DOOR_X(450), KIT_DOOR_Y(0), z0, 53247, 33003,
                                    KIT_DOOR_X(450), KIT_DOOR_Y(2000), z0, 53247, 6400,
                                    KIT_DOOR_X(200), KIT_DOOR_Y(1450), z0, 58368, 13696,
                                    cx, cy, scale, texture_body, 3, wall, wall + 1, 7);
#undef KIT_DOOR_Y
#undef KIT_DOOR_X
            }
        } else {
            Kit_TemplateQuadRecipe3(heavy_wall0, sx0, y0, z0,
                                    sx1, y0, z0,
                                    sx1, y1, z0,
                                    sx0, y1, z0,
                                    cx, cy, scale, texture_body, wall, wall + 1, seg);
        }
    }

    Kit_LineRecipe3(x0, y0, z0, x1, y0, z0, cx, cy, scale, 15);
    Kit_LineRecipe3(x1, y0, z0, x1, y0, z1, cx, cy, scale, 15);
    Kit_LineRecipe3(x1, y0, z1, x0, y0, z1, cx, cy, scale, 15);
    Kit_LineRecipe3(x0, y0, z1, x0, y0, z0, cx, cy, scale, 15);
    Kit_LineRecipe3(x0, y1, z0, x1, y1, z0, cx, cy, scale, 15);
    Kit_LineRecipe3(x1, y1, z0, x1, y1, z1, cx, cy, scale, 15);
    Kit_LineRecipe3(x1, y1, z1, x0, y1, z1, cx, cy, scale, 15);
    Kit_LineRecipe3(x0, y1, z1, x0, y1, z0, cx, cy, scale, 15);
    Kit_LineRecipe3(x0, y0, z0, x0, y1, z0, cx, cy, scale, 15);
    Kit_LineRecipe3(x1, y0, z0, x1, y1, z0, cx, cy, scale, 15);
    Kit_LineRecipe3(x1, y0, z1, x1, y1, z1, cx, cy, scale, 15);
    Kit_LineRecipe3(x0, y0, z1, x0, y1, z1, cx, cy, scale, 15);
}

static void Kit_DrawCuboid(S32 cx, S32 cy, S32 width, S32 depth, S32 height, S32 scale, S32 color) {
    S32 x0 = -width;
    S32 x1 = width;
    S32 z0 = -depth;
    S32 z1 = depth;
    S32 y0 = 0;
    S32 y1 = height;
    S32 i;

    Kit_Line3(x0, y0, z0, x1, y0, z0, cx, cy, scale, color);
    Kit_Line3(x1, y0, z0, x1, y0, z1, cx, cy, scale, color);
    Kit_Line3(x1, y0, z1, x0, y0, z1, cx, cy, scale, color);
    Kit_Line3(x0, y0, z1, x0, y0, z0, cx, cy, scale, color);
    Kit_Line3(x0, y1, z0, x1, y1, z0, cx, cy, scale, color);
    Kit_Line3(x1, y1, z0, x1, y1, z1, cx, cy, scale, color);
    Kit_Line3(x1, y1, z1, x0, y1, z1, cx, cy, scale, color);
    Kit_Line3(x0, y1, z1, x0, y1, z0, cx, cy, scale, color);
    Kit_Line3(x0, y0, z0, x0, y1, z0, cx, cy, scale, color);
    Kit_Line3(x1, y0, z0, x1, y1, z0, cx, cy, scale, color);
    Kit_Line3(x1, y0, z1, x1, y1, z1, cx, cy, scale, color);
    Kit_Line3(x0, y0, z1, x0, y1, z1, cx, cy, scale, color);

    for (i = 1; i < width; i++) {
        S32 px = x0 + i * 2;
        Kit_Line3(px, y0, z0, px, y1, z0, cx, cy, scale, 8);
        Kit_Line3(px, y0, z1, px, y1, z1, cx, cy, scale, 8);
    }
    for (i = 1; i < depth; i++) {
        S32 pz = z0 + i * 2;
        Kit_Line3(x0, y0, pz, x0, y1, pz, cx, cy, scale, 8);
        Kit_Line3(x1, y0, pz, x1, y1, pz, cx, cy, scale, 8);
    }
    for (i = 1; i < height; i++) {
        Kit_Line3(x0, i, z0, x1, i, z0, cx, cy, scale, 8);
        Kit_Line3(x1, i, z0, x1, i, z1, cx, cy, scale, 8);
        Kit_Line3(x1, i, z1, x0, i, z1, cx, cy, scale, 8);
        Kit_Line3(x0, i, z1, x0, i, z0, cx, cy, scale, 8);
    }
}

static void Kit_DrawDoor(S32 cx, S32 cy, S32 width, S32 depth, S32 scale) {
    S32 door_x = -width + KitRecipe.front_door_position * 2 + 1;
    S32 x0;
    S32 y0;
    S32 x1;
    S32 y1;
    S32 x2;
    S32 y2;
    S32 x3;
    S32 y3;

    if (KitRecipe.front_door == KIT_DOOR_NONE)
        return;

    Kit_Project(door_x - 1, 0, -depth, cx, cy, scale, &x0, &y0);
    Kit_Project(door_x + 1, 0, -depth, cx, cy, scale, &x1, &y1);
    Kit_Project(door_x + 1, 1, -depth, cx, cy, scale, &x2, &y2);
    Kit_Project(door_x - 1, 1, -depth, cx, cy, scale, &x3, &y3);
    Line(x0, y0, x1, y1, 14);
    Line(x1, y1, x2, y2, 14);
    Line(x2, y2, x3, y3, 14);
    Line(x3, y3, x0, y0, 14);
}

static S32 Kit_DrawSourceBody(void) {
    void *body_ptr;
    U8 *old_obj_ptr_map;
    S32 camera_zoom;
    S32 ret;

    if (!ObjTexture)
        return FALSE;

    body_ptr = Kit_GetSourceBody();
    if (!body_ptr)
        return FALSE;

    camera_zoom = 26000 - KitZoom * 350;
    if (camera_zoom < 3500)
        camera_zoom = 3500;

    SaveCamera();
    SetProjection((S32)ModeDesiredX / 2 + 95, (S32)ModeDesiredY / 2 + 70, 128, 1024, 1024);
    SetFollowCamera(0, 0, 0, 60 * MUL_ANGLE, 0, 0, camera_zoom);
    old_obj_ptr_map = ObjPtrMap;
    ObjPtrMap = ObjTexture;
    UnsetClip();
    SetCLUT(PalLevel);
    ret = BodyDisplay(0, 0, 0, 0, KitRotation * 1024, 0, body_ptr);
    if (ret)
        Kit_DrawSelectedFaces((T_BODY_HEADER *)body_ptr);
    ObjPtrMap = old_obj_ptr_map;
    RestoreCamera();

    return ret ? TRUE : FALSE;
}

static void Kit_DrawPreview(void) {
    S32 cx = (S32)ModeDesiredX / 2 + 70;
    S32 cy = (S32)ModeDesiredY / 2 + 100;
    S32 width = KitRecipe.width_segments;
    S32 depth = KitRecipe.depth_segments;
    S32 height = KitRecipe.height_segments;

    if (KitMode == KIT_MODE_SOURCE) {
        const BuildingSourceEntry *entry = &BuildingSources[KitSourceIndex];

        if (Kit_DrawSourceBody())
            return;

        width = 3;
        depth = 2;
        height = 2;
        Kit_DrawTexturedCuboid(cx, cy, width, depth, height, KitZoom, entry->wall_material, entry->roof_style);
        return;
    }

    Kit_DrawRecipeTexturedCuboid(cx, cy, width, depth, height, KitZoom);
}

static void Kit_DrawPanelLine(S32 x, S32 *y, const char *fmt, const char *value) {
    char text[160];

    snprintf(text, sizeof(text), fmt, value);
    GraphPrintf(FALSE, x, *y, "%s", text);
    *y += 12;
}

static void Kit_DrawInspector(void) {
    S32 x0 = 8;
    S32 y0 = 8;
    S32 x1 = 318;
    S32 y1 = (S32)ModeDesiredY - 10;
    S32 y = y0 + 10;

    if (x1 >= ModeDesiredX)
        x1 = ModeDesiredX - 8;

    BackupAngles(x0, y0, x1, y1);
    ShadeBoxBlk(x0, y0, x1, y1, MENU_SHADE_LVL);
    DrawCadre(x0, y0, x1, y1, ALL_ANGLES);
    RestoreAngles(x0, y0, x1, y1);

    CoulText(LBAWHITE, -1);
    GraphPrintf(FALSE, x0 + 9, y, "Building Kitbasher");
    y += 18;
    GraphPrintf(FALSE, x0 + 9, y, "Mode: %s", KitMode == KIT_MODE_SOURCE ? "Source Catalog" : "Recipe Editor");
    y += 18;

    if (KitMode == KIT_MODE_SOURCE) {
        const BuildingSourceEntry *entry = &BuildingSources[KitSourceIndex];
        GraphPrintf(FALSE, x0 + 9, y, "%d/%d  %s %d", (int)(KitSourceIndex + 1), (int)Kit_SourceCount(), entry->group, (int)entry->lm2_id);
        y += 12;
        Kit_DrawPanelLine(x0 + 9, &y, "Name: %s", entry->display_name);
        Kit_DrawPanelLine(x0 + 9, &y, "Wall: %s", entry->wall_material);
        Kit_DrawPanelLine(x0 + 9, &y, "Coping: %s", entry->coping_style);
        Kit_DrawPanelLine(x0 + 9, &y, "Roof: %s", entry->roof_style);
        Kit_DrawPanelLine(x0 + 9, &y, "Door: %s", entry->door_type);
        Kit_DrawPanelLine(x0 + 9, &y, "Missing: %s", entry->missing_sides);
        y += 4;
        Kit_DrawPanelLine(x0 + 9, &y, "Details: %s", entry->details);
        Kit_DrawPanelLine(x0 + 9, &y, "Notes: %s", entry->notes);
        y += 4;
        GraphPrintf(FALSE, x0 + 9, y, "Selected faces: %d", (int)KitSelectedFaceCount);
        y += 12;
        if (KitSelectedFaceCount > 0) {
            GraphPrintf(FALSE, x0 + 9, y, "Last selected: #%d", (int)KitSelectedFaces[KitSelectedFaceCount - 1]);
            y += 12;
        }
        if (KitSelectedFaceCount > 0) {
            BackupAngles(x0 + 4, y - 3, x1 - 4, y + 10);
            DrawFireBar(x0 + 4, y - 3, x1 - 4, y + 10, FIRE_SELECT_MENU);
            DrawCadre(x0 + 4, y - 3, x1 - 4, y + 10, ALL_ANGLES);
            RestoreAngles(x0 + 4, y - 3, x1 - 4, y + 10);
            CoulText(LBAWHITE, -1);
        }
        GraphPrintf(FALSE, x0 + 9, y, "Tag: %s", KitTagNames[KitCurrentTag]);
        y += 12;
    } else {
        const char *field_names[] = {
            "width_segments", "depth_segments", "height_segments", "wall_material",
            "coping_style", "roof_style", "front_door", "front_door_position"};
        char values[8][48];
        S32 i;

        snprintf(values[0], sizeof(values[0]), "%d", (int)KitRecipe.width_segments);
        snprintf(values[1], sizeof(values[1]), "%d", (int)KitRecipe.depth_segments);
        snprintf(values[2], sizeof(values[2]), "%d", (int)KitRecipe.height_segments);
        snprintf(values[3], sizeof(values[3]), "%s", WallMaterialNames[KitRecipe.wall_material]);
        snprintf(values[4], sizeof(values[4]), "%s", CopingStyleNames[KitRecipe.coping_style]);
        snprintf(values[5], sizeof(values[5]), "%s", RoofStyleNames[KitRecipe.roof_style]);
        snprintf(values[6], sizeof(values[6]), "%s", DoorTypeNames[KitRecipe.front_door]);
        snprintf(values[7], sizeof(values[7]), "%d", (int)KitRecipe.front_door_position);

        for (i = 0; i < 8; i++) {
            if (i == KitRecipeField) {
                BackupAngles(x0 + 4, y - 3, x1 - 4, y + 10);
                DrawFireBar(x0 + 4, y - 3, x1 - 4, y + 10, FIRE_SELECT_MENU);
                DrawCadre(x0 + 4, y - 3, x1 - 4, y + 10, ALL_ANGLES);
                RestoreAngles(x0 + 4, y - 3, x1 - 4, y + 10);
                CoulText(LBAWHITE, -1);
            } else {
                CoulText(LBAWHITE, -1);
            }
            GraphPrintf(FALSE, x0 + 9, y, "%s: %s", field_names[i], values[i]);
            y += 13;
        }
        CoulText(LBAWHITE, -1);
        y += 6;
        GraphPrintf(FALSE, x0 + 9, y, "Window/detail slots exist in state");
        y += 12;
        GraphPrintf(FALSE, x0 + 9, y, "but are not rendered in V1.");
    }

    y = y1 - 90;
    CoulText(LBAWHITE, -1);
    GraphPrintf(FALSE, x0 + 9, y, "Tab mode  Left/Right edit");
    y += 12;
    GraphPrintf(FALSE, x0 + 9, y, "Up/Down field  R rotate");
    y += 12;
    GraphPrintf(FALSE, x0 + 9, y, "+/- zoom  D dump  F face");
    y += 12;
    GraphPrintf(FALSE, x0 + 9, y, "Shift+F dump with marks");
    y += 12;
    GraphPrintf(FALSE, x0 + 9, y, "Click face  Shift+click add");
    y += 12;
    GraphPrintf(FALSE, x0 + 9, y, "` inspector");
    y += 12;
    GraphPrintf(FALSE, x0 + 9, y, "Esc back");
}

static void Kit_DumpFace(const KitFaceInfo *info, T_BODY_HEADER *body, S32 selected) {
    const BuildingSourceEntry *entry = &BuildingSources[KitSourceIndex];
    T_OBJ_POINT *points = (T_OBJ_POINT *)((U8 *)body + body->OffPoints);
    S32 tag = Kit_GetFaceTag(entry->lm2_id, info->index);
    S32 i;

    Log_Info("[kitbasher] %sface #%d tag=%s offset=%d type_poly=%d shape=%s kind=%s color=%u normal=%u texture=%u scale=%u",
             selected ? "* " : "",
             (int)info->index,
             KitTagNames[tag],
             (int)info->offset,
             (int)info->type_poly,
             info->is_quad ? "quad" : "tri",
             Kit_FaceTypeName(info->face_type),
             (unsigned)info->color,
             (unsigned)info->normal,
             (unsigned)info->texture,
             (unsigned)info->scale);

    for (i = 0; i < info->point_count; i++) {
        U16 p = info->points[i];
        Log_Info("[kitbasher]   p%d=%u obj=(%d,%d,%d g%d) screen=(%d,%d) rotated=(%d,%d,%d) uv=(%u,%u)",
                 (int)i,
                 (unsigned)p,
                 (int)points[p].X,
                 (int)points[p].Y,
                 (int)points[p].Z,
                 (int)points[p].Group,
                 (int)Obj_ListProjectedPoints[p].X,
                 (int)Obj_ListProjectedPoints[p].Y,
                 (int)Obj_ListRotatedPoints[p].X,
                 (int)Obj_ListRotatedPoints[p].Y,
                 (int)Obj_ListRotatedPoints[p].Z,
                 (unsigned)info->u[i],
                 (unsigned)info->v[i]);
    }
}

static void Kit_DumpTaggedCatalog(void) {
    const BuildingSourceEntry *entry = &BuildingSources[KitSourceIndex];
    S32 i;

    Log_Info("[kitbasher] tagged catalog for %s LM2 %d: %s", entry->group, (int)entry->lm2_id, entry->display_name);
    for (i = 0; i < KIT_MAX_FACE_TAGS; i++) {
        if (KitFaceTags[i].used AND KitFaceTags[i].lm2_id == entry->lm2_id) {
            Log_Info("[kitbasher] tag_entry { group=\"%s\", lm2_id=%d, face=%d, tag=\"%s\" }",
                     entry->group,
                     (int)entry->lm2_id,
                     (int)KitFaceTags[i].face_index,
                     KitTagNames[KitFaceTags[i].tag]);
        }
    }
}

static void Kit_DumpFaces(T_BODY_HEADER *body, S32 selected_only, S32 mark_selected) {
    U8 *lines = (U8 *)body + body->OffLines;
    U8 *current = (U8 *)body + body->OffPolys;
    S32 body_offset = body->OffPolys;
    S32 face_index = 0;
    S32 dumped = 0;

    while (current < lines) {
        KitPolyHeader *header = (KitPolyHeader *)current;
        S32 mask = 0;
        S32 is_quad;
        S32 face_type;
        S32 size;
        S32 i;

        memcpy(&mask, header, sizeof(S32));
        is_quad = (mask & KIT_MASK_QUADRILATERE) ? TRUE : FALSE;
        face_type = Kit_FaceTypeFromHeader(header, is_quad);
        current += sizeof(KitPolyHeader);
        body_offset += sizeof(KitPolyHeader);
        size = Kit_FaceRecordSize(face_type, is_quad);

        for (i = 0; i < header->NbPoly; i++) {
            S32 selected = Kit_IsFaceSelected(face_index);
            if (!selected_only OR selected) {
                KitFaceInfo info;
                Kit_ReadFaceInfo(current, face_index, body_offset, header->TypePoly & 0xff, face_type, is_quad, &info);
                Kit_DumpFace(&info, body, mark_selected AND selected);
                dumped++;
            }
            current += size;
            body_offset += size;
            face_index++;
        }
    }

    if (selected_only AND dumped == 0)
        Log_Info("[kitbasher] no selected source faces to dump");
}

static void Kit_DumpTextureUsage(T_BODY_HEADER *body) {
    U8 *lines = (U8 *)body + body->OffLines;
    U8 *current = (U8 *)body + body->OffPolys;
    S32 body_offset = body->OffPolys;
    U16 handles[256];
    S32 counts[256];
    S32 handle_count = 0;
    S32 face_index = 0;
    S32 i;

    memset(handles, 0, sizeof(handles));
    memset(counts, 0, sizeof(counts));

    while (current < lines) {
        KitPolyHeader *header = (KitPolyHeader *)current;
        S32 mask = 0;
        S32 is_quad;
        S32 face_type;
        S32 size;

        memcpy(&mask, header, sizeof(S32));
        is_quad = (mask & KIT_MASK_QUADRILATERE) ? TRUE : FALSE;
        face_type = Kit_FaceTypeFromHeader(header, is_quad);
        current += sizeof(KitPolyHeader);
        body_offset += sizeof(KitPolyHeader);
        size = Kit_FaceRecordSize(face_type, is_quad);

        for (i = 0; i < header->NbPoly; i++) {
            if (face_type == KIT_FACE_TEXTURE OR face_type == KIT_FACE_ENV) {
                KitFaceInfo info;
                S32 j;
                S32 found = FALSE;

                Kit_ReadFaceInfo(current, face_index, body_offset, header->TypePoly & 0xff, face_type, is_quad, &info);
                for (j = 0; j < handle_count; j++) {
                    if (handles[j] == info.texture) {
                        counts[j]++;
                        found = TRUE;
                        break;
                    }
                }
                if (!found AND handle_count < 256) {
                    handles[handle_count] = info.texture;
                    counts[handle_count] = 1;
                    handle_count++;
                }
            }
            current += size;
            body_offset += size;
            face_index++;
        }
    }

    Log_Info("[kitbasher] texture usage handles=%d", (int)handle_count);
    for (i = 0; i < handle_count; i++) {
        Log_Info("[kitbasher]   texture=%u faces=%d", (unsigned)handles[i], (int)counts[i]);
    }
}

static void Kit_DumpRecordCounts(T_BODY_HEADER *body) {
    U8 *lines = (U8 *)body + body->OffLines;
    U8 *current = (U8 *)body + body->OffPolys;
    S32 light_tri = 0;
    S32 light_quad = 0;
    S32 texture_tri = 0;
    S32 texture_quad = 0;
    S32 env_tri = 0;
    S32 env_quad = 0;

    while (current < lines) {
        KitPolyHeader *header = (KitPolyHeader *)current;
        S32 mask = 0;
        S32 is_quad;
        S32 face_type;
        S32 size;

        memcpy(&mask, header, sizeof(S32));
        is_quad = (mask & KIT_MASK_QUADRILATERE) ? TRUE : FALSE;
        face_type = Kit_FaceTypeFromHeader(header, is_quad);
        current += sizeof(KitPolyHeader);
        size = Kit_FaceRecordSize(face_type, is_quad);

        if (face_type == KIT_FACE_LIGHT AND is_quad)
            light_quad += header->NbPoly;
        else if (face_type == KIT_FACE_LIGHT)
            light_tri += header->NbPoly;
        else if (face_type == KIT_FACE_TEXTURE AND is_quad)
            texture_quad += header->NbPoly;
        else if (face_type == KIT_FACE_TEXTURE)
            texture_tri += header->NbPoly;
        else if (is_quad)
            env_quad += header->NbPoly;
        else
            env_tri += header->NbPoly;

        current += size * header->NbPoly;
    }

    Log_Info("[kitbasher] record counts light_tri=%d light_quad=%d texture_tri=%d texture_quad=%d env_tri=%d env_quad=%d",
             (int)light_tri, (int)light_quad, (int)texture_tri, (int)texture_quad, (int)env_tri, (int)env_quad);
}

static void Kit_DumpRawInfo(S32 mark_selected) {
    const BuildingSourceEntry *entry = &BuildingSources[KitSourceIndex];
    T_BODY_HEADER *body = (T_BODY_HEADER *)Kit_GetSourceBody();

    Log_Info("[kitbasher] source %s LM2 %d: %s", entry->group, (int)entry->lm2_id, entry->display_name);
    Log_Info("[kitbasher] tags wall=%s coping=%s roof=%s door=%s missing=%s",
             entry->wall_material, entry->coping_style, entry->roof_style, entry->door_type, entry->missing_sides);
    Log_Info("[kitbasher] details=%s", entry->details);
    Log_Info("[kitbasher] notes=%s", entry->notes);

    if (!body) {
        Log_Info("[kitbasher] raw model dump unavailable: source body library is not loaded yet");
        return;
    }

    Log_Info("[kitbasher] body bounds x=(%d,%d) y=(%d,%d) z=(%d,%d)",
             (int)body->XMin, (int)body->XMax, (int)body->YMin, (int)body->YMax, (int)body->ZMin, (int)body->ZMax);
    Log_Info("[kitbasher] body counts groups=%d vertices=%d polygons=%d lines=%d spheres=%d textures=%d",
             (int)body->NbGroupes, (int)body->NbPoints, (int)body->NbPolys,
             (int)body->NbLines, (int)body->NbSpheres, (int)body->NbTextures);
    Log_Info("[kitbasher] body offsets points=%d polys=%d lines=%d spheres=%d textures=%d",
             (int)body->OffPoints, (int)body->OffPolys, (int)body->OffLines,
             (int)body->OffSpheres, (int)body->OffTextures);
    Kit_DumpRecordCounts(body);
    Kit_DumpTextureUsage(body);
    Kit_DumpTaggedCatalog();
    Kit_DumpFaces(body, FALSE, mark_selected);
}

static void Kit_DumpSelectedFaces(void) {
    const BuildingSourceEntry *entry = &BuildingSources[KitSourceIndex];
    T_BODY_HEADER *body = (T_BODY_HEADER *)Kit_GetSourceBody();

    Log_Info("[kitbasher] selected faces for %s LM2 %d: %s", entry->group, (int)entry->lm2_id, entry->display_name);
    if (!body) {
        Log_Info("[kitbasher] selected face dump unavailable: source body library is not loaded yet");
        return;
    }

    Kit_DumpFaces(body, TRUE, TRUE);
}

static void Kit_AdjustRecipe(S32 step) {
    switch (KitRecipeField) {
    case 0:
        KitRecipe.width_segments += step;
        break;
    case 1:
        KitRecipe.depth_segments += step;
        break;
    case 2:
        KitRecipe.height_segments += step;
        break;
    case 3:
        KitRecipe.wall_material += step;
        break;
    case 4:
        KitRecipe.coping_style += step;
        break;
    case 5:
        KitRecipe.roof_style += step;
        break;
    case 6:
        KitRecipe.front_door += step;
        break;
    case 7:
        KitRecipe.front_door_position += step;
        break;
    }
    Kit_ClampRecipe();
}

static void Kit_HandleInput(S32 *done) {
    U32 input_edge;
    S32 key_edge;
    S32 key;
    S32 click_edge;
    S32 shift;

    MyGetInput();

    input_edge = Input & ~KitLastInput;
    key = MyKey;
    key_edge = (key != 0 AND key != KitLastKey) ? TRUE : FALSE;
    click_edge = ((Click & 1) AND !(KitLastClick & 1)) ? TRUE : FALSE;
    shift = Kit_ShiftDown();

    if (input_edge & I_MENUS OR(key_edge AND key == K_ESC)) {
        *done = TRUE;
        goto update_last_input;
    }
    if (key_edge AND key == K_TAB) {
        KitMode = KitMode == KIT_MODE_SOURCE ? KIT_MODE_RECIPE : KIT_MODE_SOURCE;
    }
    if (key_edge AND key == K_CARRE) {
        KitInspectorVisible = KitInspectorVisible ? FALSE : TRUE;
    }
    if (key_edge AND key == K_R) {
        KitRotation = (KitRotation + 1) & 3;
    }
    if (key_edge AND key == K_D) {
        Kit_DumpRawInfo(FALSE);
    }
    if (key_edge AND key == K_F) {
        if (shift)
            Kit_DumpRawInfo(TRUE);
        else
            Kit_DumpSelectedFaces();
    }
    if (key_edge AND(key == K_KP_PLUS OR key == K_NUMPAD_PLUS OR key == K_EQUALS)) {
        KitZoom += 2;
        if (KitZoom > 64) KitZoom = 64;
    }
    if (key_edge AND(key == K_NUMPAD_MOINS OR key == K_MOINS)) {
        KitZoom -= 2;
        if (KitZoom < 1) KitZoom = 1;
    }
    if (KitMode == KIT_MODE_SOURCE) {
        if (click_edge) {
            T_BODY_HEADER *body = (T_BODY_HEADER *)Kit_GetSourceBody();
            S32 face = body ? Kit_FindFaceAt(body, (S32)MouseX, (S32)MouseY) : -1;

            if (shift) {
                if (face >= 0)
                    Kit_ToggleSelectedFace(face);
            } else {
                Kit_SelectSingleFace(face);
            }
            Kit_SyncTagFromSelection();
        }
        if (input_edge & I_LEFT OR(key_edge AND key == K_GRAY_LEFT)) {
            if (KitSelectedFaceCount > 0) {
                Kit_AdjustCurrentTag(-1);
            } else {
                KitSourceIndex--;
                if (KitSourceIndex < 0)
                    KitSourceIndex = Kit_SourceCount() - 1;
                Kit_ClearSelection();
            }
        }
        if (input_edge & I_RIGHT OR(key_edge AND key == K_GRAY_RIGHT)) {
            if (KitSelectedFaceCount > 0) {
                Kit_AdjustCurrentTag(1);
            } else {
                KitSourceIndex++;
                if (KitSourceIndex >= Kit_SourceCount())
                    KitSourceIndex = 0;
                Kit_ClearSelection();
            }
        }
    } else {
        if (input_edge & I_UP OR(key_edge AND key == K_GRAY_UP)) {
            KitRecipeField--;
            if (KitRecipeField < 0)
                KitRecipeField = 7;
        }
        if (input_edge & I_DOWN OR(key_edge AND key == K_GRAY_DOWN)) {
            KitRecipeField++;
            if (KitRecipeField > 7)
                KitRecipeField = 0;
        }
        if (input_edge & I_LEFT OR(key_edge AND key == K_GRAY_LEFT)) {
            Kit_AdjustRecipe(-1);
        }
        if (input_edge & I_RIGHT OR(key_edge AND key == K_GRAY_RIGHT)) {
            Kit_AdjustRecipe(1);
        }
    }

update_last_input:
    KitLastInput = Input;
    KitLastKey = MyKey;
    KitLastClick = Click;
}

void BuildingKitbasher_Run(void) {
    S32 done = FALSE;

    Kit_InitRecipe();
    KitLastInput = 0;
    KitLastKey = 0;
    KitLastClick = 0;
    Kit_ClearSelection();
    InitWaitNoInput(ALL_INPUTS);
    InitWaitNoKey();

    while (!done) {
        ManageTime();
        Kit_HandleInput(&done);

        Box(0, 0, (S32)ModeDesiredX - 1, (S32)ModeDesiredY - 1, 0);
        Kit_DrawPreview();
        if (KitInspectorVisible)
            Kit_DrawInspector();
        BoxStaticAdd(0, 0, (S32)ModeDesiredX - 1, (S32)ModeDesiredY - 1);
        BoxUpdate();
    }

    WaitNoInput();
}
