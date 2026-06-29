#include "DEFINES.H"

#include "EDITOR_DECOR_BOUNDS.H"

#include "../ACTOR_EDITOR.H"
#include "../COMMON.H"
#include "../EDITOR_JSON.H"
#include "../EDITOR.H"
#include "../WAYPOINT_EDITOR.H"
#include "LOADISLE.H"

#include <SYSTEM/KEYBOARD.H>
#include <SYSTEM/KEYBOARD_KEYS.H>
#include <SYSTEM/LOG.H>
#include <SYSTEM/MOUSE.H>
#include <SYSTEM/LIMITS.H>
#include <SYSTEM/FILES.H>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*══════════════════════════════════════════════════════════════════════════*
		      █▀▀▀▄ █▀▀▀▀ █▀▀▀▀ █▀▀▀█ █▀▀▀█ ██▀▀▀
		      ██  █ ██▀▀  ██    ██  █ ██▀█▀ ▀▀▀▀█
		      ▀▀▀▀  ▀▀▀▀▀ ▀▀▀▀▀ ▀▀▀▀▀ ▀▀  ▀ ▀▀▀▀▀
 *══════════════════════════════════════════════════════════════════════════*/
/*──────────────────────────────────────────────────────────────────────────*/

#ifdef LBA_EDITOR // Special PERSO.EXE
#define INFO_DECORS 1024
extern S32 FlagInfos;

#define SIZE_NAME 19

typedef struct
{
    char Name[SIZE_NAME + 1];
    S16 Value;
} T_VAR;

extern T_VAR ListVarGame[];
#else
extern S16 ListVarGame[];
#endif

extern S32 FlagMasque;

#define MAX_SOUS_OBJET 20
#define SIZE_BRICK_Y 256

#ifdef LBA2CC_EDITOR
extern void ZoneOverlaySet(S32 ids, S32 boxes);
extern void ZoneOverlaySetType(S32 type);
extern void ZoneEditorBeginPick(void);
extern void ZoneEditorClearAllSelected(void);
extern void ZoneEditorSelectOnly(S32 index);
extern void ZoneEditorToggleSelectedByIndex(S32 index);
extern S32 ZoneEditorLogSelected(void);
extern S32 ZoneEditorCreateLinkedDecorZone(const T_ZONE *zone, S32 decor_cube_x, S32 decor_cube_y, S32 decor_num, S32 decor_body, S32 decor_role, S32 decor_x, S32 decor_y, S32 decor_z, S32 decor_beta);
extern void ZoneEditorSyncLinkedDecorZones(S32 decor_cube_x, S32 decor_cube_y, S32 decor_num, S32 decor_body, S32 decor_x, S32 decor_y, S32 decor_z, S32 decor_beta);
extern void ActorEditorClearSelection(void);
extern void ActorEditorResolvePick(S32 index, S32 toggle);
extern void WaypointEditorClearSelection(void);
extern void WaypointEditorResolvePick(S32 index, S32 toggle);
extern S32 ZoneEditorHasAnySelected(void);
extern S32 Island;
extern char IslandName[ADELINE_MAX_PATH];
extern S32 NumCube;
extern S32 StartXCube;
extern S32 StartYCube;
extern S32 StartZCube;
extern S32 AlphaCam;
extern S32 BetaCam;
extern S32 GammaCam;
extern S32 VueDistance;
extern S16 NbZones;
extern T_ZONE *ListZone;

static S32 ExtDecorOverlayIds = FALSE;
static S32 ExtDecorOverlayBoxes = FALSE;
static S32 ExtDecorOverlayMaxDist = 0;
static S32 ExtDecorEditorSelect = FALSE;
static S32 ExtDecorEditorLastClick = 0;
static S32 ExtDecorEditorLastUp = 0;
static S32 ExtDecorEditorLastDown = 0;
static S32 ExtDecorEditorLastLeft = 0;
static S32 ExtDecorEditorLastRight = 0;
static S32 ExtDecorEditorPickPending = FALSE;
static S32 ExtDecorEditorPickShift = FALSE;
static S32 ExtDecorEditorPickBestDist = 0;
static S32 ExtDecorEditorPickBestCubeX = 0;
static S32 ExtDecorEditorPickBestCubeY = 0;
static S32 ExtDecorEditorPickBestNum = -1;
static S32 ExtDecorEditorPickBestBody = -1;
static S32 ExtDecorEditorPickPasses = 0;
static S32 ExtDecorEditorPickObjects = 0;
static S32 ExtDecorEditorPickPoints = 0;
static S32 ExtDecorEditorLastDelete = FALSE;
static S32 ExtDecorEditorLastDuplicate = FALSE;
static S32 ExtDecorEditorLastInsert = FALSE;
static S32 ExtDecorEditorLastSelectAll = FALSE;
static S32 ExtDecorEditorLastPalettePrev = FALSE;
static S32 ExtDecorEditorLastPaletteNext = FALSE;
static S32 ExtDecorEditorLastPageUp = FALSE;
static S32 ExtDecorEditorLastPageDown = FALSE;
static S32 ExtDecorEditorLastLog = FALSE;
static S32 ExtDecorEditorPaletteLibrary = 0;
static S32 ExtDecorEditorPaletteBody = 0;
static S32 ExtDecorEditorPlaceValid = FALSE;
static S32 ExtDecorEditorPlaceX = 0;
static S32 ExtDecorEditorPlaceY = 0;
static S32 ExtDecorEditorPlaceZ = 0;
static S32 ExtDecorEditorPlaceBeta = 0;
static S32 ExtDecorEditorLoadedCubeX = 0;
static S32 ExtDecorEditorLoadedCubeY = 0;
static T_DECORS *ExtDecorEditorOwnedList = NULL;

static void ExtDecorEditorPlaceAtPreview(void);

#define EXT_DECOR_EDITOR_MAX_EDITS 4096
#define EXT_DECOR_EDITOR_MAX_SELECTED 256

#define EXT_DECOR_LINK_ROLE_SIGN_TEXT_A 1
#define EXT_DECOR_LINK_ROLE_SIGN_CAMERA 2
#define EXT_DECOR_LINK_ROLE_SIGN_TEXT_B 3
#define EXT_DECOR_LINK_ROLE_BONUS 4

typedef struct
{
    S32 Used;
    S32 CubeX;
    S32 CubeY;
    S32 Num;
    S32 Body;
} T_EXT_DECOR_EDITOR_SEL;

static T_EXT_DECOR_EDITOR_SEL ExtDecorEditorSelList[EXT_DECOR_EDITOR_MAX_SELECTED];

typedef struct
{
    S32 Used;
    S32 Deleted;
    S32 CubeX;
    S32 CubeY;
    S32 Num;
    S32 Body;
    S32 HasExpectBody;
    S32 NewBody;
    S32 BaseBodyRaw;
    S32 BaseCodeJeu;
    S32 BaseBeta;
    S32 BaseX;
    S32 BaseY;
    S32 BaseZ;
    S32 BaseXMin;
    S32 BaseYMin;
    S32 BaseZMin;
    S32 BaseXMax;
    S32 BaseYMax;
    S32 BaseZMax;
    S32 DBeta;
    S32 Dx;
    S32 Dy;
    S32 Dz;
    S32 SkipLogged;
} T_EXT_DECOR_EDITOR_EDIT;

static T_EXT_DECOR_EDITOR_EDIT ExtDecorEditorEdits[EXT_DECOR_EDITOR_MAX_EDITS];

static S32 ExtDecorEditorAppendObjectFromEdit(T_EXT_DECOR_EDITOR_EDIT *edit);
static void ExtDecorEditorSelectSlot(S32 num);
static void ExtDecorEditorUpdatePlacementPreview(void);

#define EXT_DECOR_OVERLAY_TEXT 15
#define EXT_DECOR_OVERLAY_BOX 15
#define EXT_DECOR_OVERLAY_BOX_AXIS 248
#define EXT_DECOR_OVERLAY_BACK 0
#define EXT_DECOR_EDITOR_TEXT 0
#define EXT_DECOR_EDITOR_BACK 14
#define EXT_DECOR_EDITOR_BOX 14
#define EXT_DECOR_EDITOR_BOX_AXIS 12
#define EXT_DECOR_EDITOR_PICK_RADIUS 32
#define EXT_DECOR_EDITOR_NUDGE 512
#define EXT_DECOR_EDITOR_NUDGE_ALT 32
#define EXT_DECOR_EDITOR_NUDGE_SHIFT (5 * EXT_DECOR_EDITOR_NUDGE)
#define EXT_DECOR_EDITOR_ROTATE_STEP 1024
#define EXT_DECOR_BODY_LIBRARY_BASE 10000
#define EXT_DECOR_BODY_LIBRARY_STRIDE 200

void ExtDecorEditorSetLoadedCube(S32 cube_x, S32 cube_y) {
    ExtDecorEditorLoadedCubeX = cube_x;
    ExtDecorEditorLoadedCubeY = cube_y;
}

static S32 ExtDecorEditorLoadedIsCurrentCube(void) {
    return ExtDecorEditorLoadedCubeX == CurrentCubeX AND ExtDecorEditorLoadedCubeY == CurrentCubeY;
}

static void ExtDecorEditorDecodeBody(S32 raw_body, S32 *library, S32 *body) {
    S32 encoded;

    raw_body &= 0xFFFF;
    *library = 0;
    *body = raw_body;
    if (raw_body <= EXT_DECOR_BODY_LIBRARY_BASE)
        return;

    encoded = raw_body - EXT_DECOR_BODY_LIBRARY_BASE;
    *library = encoded / EXT_DECOR_BODY_LIBRARY_STRIDE;
    *body = encoded % EXT_DECOR_BODY_LIBRARY_STRIDE;
    if (*library <= 0 OR *library >= MAX_ISLAND) {
        *library = -1;
        *body = -1;
    }
}

static S32 ExtDecorEditorEncodeBody(S32 library, S32 body) {
    if (library <= 0)
        return body & 0xFFFF;
    return EXT_DECOR_BODY_LIBRARY_BASE + library * EXT_DECOR_BODY_LIBRARY_STRIDE + (body % EXT_DECOR_BODY_LIBRARY_STRIDE);
}

void ExtDecorOverlaySet(S32 ids, S32 boxes) {
    ExtDecorOverlayIds = ids ? TRUE : FALSE;
    ExtDecorOverlayBoxes = boxes ? TRUE : FALSE;
}

void ExtDecorOverlaySetMaxDist(S32 maxdist) {
    ExtDecorOverlayMaxDist = maxdist > 0 ? maxdist : 0;
}

S32 ExtDecorOverlayGetIds(void) {
    return ExtDecorOverlayIds;
}

S32 ExtDecorOverlayGetBoxes(void) {
    return ExtDecorOverlayBoxes;
}

S32 ExtDecorOverlayGetMaxDist(void) {
    return ExtDecorOverlayMaxDist;
}

void ExtDecorEditorSetSelect(S32 select) {
    ExtDecorEditorSelect = select ? TRUE : FALSE;
    if (ExtDecorEditorSelect) {
        ExtDecorOverlaySet(TRUE, TRUE);
        ZoneOverlaySet(TRUE, TRUE);
        ZoneOverlaySetType(-1);
    } else {
        ExtDecorOverlaySet(FALSE, FALSE);
        ZoneOverlaySet(FALSE, FALSE);
        memset(ExtDecorEditorSelList, 0, sizeof(ExtDecorEditorSelList));
        ZoneEditorClearAllSelected();
        ActorEditorClearSelection();
        WaypointEditorClearSelection();
    }
    ExtDecorEditorLastClick = Click;
    ExtDecorEditorLastUp = CheckKey(K_UP) ? TRUE : FALSE;
    ExtDecorEditorLastDown = CheckKey(K_DOWN) ? TRUE : FALSE;
    ExtDecorEditorLastLeft = CheckKey(K_LEFT) ? TRUE : FALSE;
    ExtDecorEditorLastRight = CheckKey(K_RIGHT) ? TRUE : FALSE;
    ExtDecorEditorLastSelectAll = (CheckKey(K_A) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
    ShowMouse(ExtDecorEditorSelect ? true : false);
}

S32 ExtDecorEditorGetSelect(void) {
    return ExtDecorEditorSelect;
}

S32 ExtDecorEditorHasSelection(void) {
    S32 i;

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_SELECTED; i++) {
        if (ExtDecorEditorSelList[i].Used)
            return TRUE;
    }
    return ZoneEditorHasAnySelected()
           OR ActorEditorGetSelectedIndex() >= 0
           OR WaypointEditorGetSelectedIndex() >= 0;
}

void ExtDecorEditorClearSelection(void) {
    memset(ExtDecorEditorSelList, 0, sizeof(ExtDecorEditorSelList));
    ZoneEditorClearAllSelected();
    ActorEditorClearSelection();
    WaypointEditorClearSelection();
}

S32 ExtDecorEditorGetPickPending(void) {
    return ExtDecorEditorPickPending;
}

void ExtDecorEditorResolveClickPick(S32 zone_index, S32 zone_dist_sq, S32 actor_index, S32 actor_dist_sq, S32 waypoint_index, S32 waypoint_dist_sq) {
    S32 decor_hit;
    S32 zone_hit;
    S32 actor_hit;
    S32 waypoint_hit;
    S32 winner;

    if (!ExtDecorEditorPickPending)
        return;

    decor_hit = ExtDecorEditorPickBestNum >= 0;
    zone_hit = zone_index >= 0;
    actor_hit = actor_index >= 0;
    waypoint_hit = waypoint_index >= 0;

    if (!decor_hit AND !zone_hit AND !actor_hit AND !waypoint_hit) {
        if (!ExtDecorEditorPickShift)
            ExtDecorEditorClearSelection();
        ExtDecorEditorPickPending = FALSE;
        return;
    }

    winner = 0;
    if (decor_hit)
        winner = 1;
    if (zone_hit AND(!winner OR zone_dist_sq < ExtDecorEditorPickBestDist))
        winner = 2;
    if (actor_hit
            AND(!winner
                OR(winner == 1 AND actor_dist_sq < ExtDecorEditorPickBestDist)
                OR(winner == 2 AND actor_dist_sq < zone_dist_sq)))
        winner = 3;
    if (waypoint_hit
            AND(!winner
                OR(winner == 1 AND waypoint_dist_sq < ExtDecorEditorPickBestDist)
                OR(winner == 2 AND waypoint_dist_sq < zone_dist_sq)
                OR(winner == 3 AND waypoint_dist_sq < actor_dist_sq)))
        winner = 4;

    if (ExtDecorEditorPickShift) {
        if (winner == 1) {
            S32 i;
            S32 found = FALSE;

            for (i = 0; i < EXT_DECOR_EDITOR_MAX_SELECTED; i++) {
                T_EXT_DECOR_EDITOR_SEL *sel = &ExtDecorEditorSelList[i];

                if (!sel->Used)
                    continue;
                if (sel->CubeX == ExtDecorEditorPickBestCubeX AND sel->CubeY == ExtDecorEditorPickBestCubeY AND sel->Num == ExtDecorEditorPickBestNum AND sel->Body == ExtDecorEditorPickBestBody) {
                    sel->Used = FALSE;
                    found = TRUE;
                    break;
                }
            }
            if (!found) {
                for (i = 0; i < EXT_DECOR_EDITOR_MAX_SELECTED; i++) {
                    T_EXT_DECOR_EDITOR_SEL *sel = &ExtDecorEditorSelList[i];

                    if (sel->Used)
                        continue;
                    sel->Used = TRUE;
                    sel->CubeX = ExtDecorEditorPickBestCubeX;
                    sel->CubeY = ExtDecorEditorPickBestCubeY;
                    sel->Num = ExtDecorEditorPickBestNum;
                    sel->Body = ExtDecorEditorPickBestBody;
                    break;
                }
            }
        } else if (winner == 2) {
            ZoneEditorToggleSelectedByIndex(zone_index);
        } else if (winner == 3) {
            ActorEditorResolvePick(actor_index, TRUE);
        } else {
            WaypointEditorResolvePick(waypoint_index, TRUE);
        }
    } else {
        memset(ExtDecorEditorSelList, 0, sizeof(ExtDecorEditorSelList));
        ZoneEditorClearAllSelected();
        ActorEditorClearSelection();
        WaypointEditorClearSelection();
        if (winner == 1) {
            ExtDecorEditorSelList[0].Used = TRUE;
            ExtDecorEditorSelList[0].CubeX = ExtDecorEditorPickBestCubeX;
            ExtDecorEditorSelList[0].CubeY = ExtDecorEditorPickBestCubeY;
            ExtDecorEditorSelList[0].Num = ExtDecorEditorPickBestNum;
            ExtDecorEditorSelList[0].Body = ExtDecorEditorPickBestBody;
        } else if (winner == 2) {
            ZoneEditorSelectOnly(zone_index);
        } else if (winner == 3) {
            ActorEditorResolvePick(actor_index, FALSE);
        } else {
            WaypointEditorResolvePick(waypoint_index, FALSE);
        }
    }

    ExtDecorEditorPickPending = FALSE;
}

S32 ExtDecorEditorConsumesInput(void) {
    return ExtDecorEditorSelect;
}

void ExtDecorEditorClearRuntimeEdits(void) {
    memset(ExtDecorEditorEdits, 0, sizeof(ExtDecorEditorEdits));
    ExtDecorEditorClearSelection();
    ExtDecorEditorReleaseOwnedDecorList();
}
#endif

/*══════════════════════════════════════════════════════════════════════════*/
/*══════════════════════════════════════════════════════════════════════════*/
// affiche les objets de decors

#ifdef LBA_EDITOR // uniquement PERSO
extern void ClearScreenMinMax();
extern void AdjustScreenMinMax();

S32 NumDecorsDraw;

//****************************************************************************
void DrawCubeNoBox(S32 ox, S32 oy, S32 oz,
                   S32 xw0, S32 yw0, S32 zw0,
                   S32 xw1, S32 yw1, S32 zw1) {
    S32 x0, y0, x1, y1, x2, y2, x3, y3, x4, y4, x5, y5, x6, y6, x7, y7;
    S16 coul = 11;
    S16 coul1 = 13;

    ClearScreenMinMax();
    LongWorldRotatePoint(ox, oy, oz);
    if (!LongProjectPoint(X0, Y0, Z0))
        return;
    AdjustScreenMinMax();
    Rect(Xp - 3, Yp - 3, Xp + 3, Yp + 3, coul); // point org

    CoulText(GREEN, -1);
    GraphPrintf(FALSE, Xp, Yp, "%d", NumDecorsDraw);

    LongWorldRotatePoint(xw0, yw0, zw0);
    if (!LongProjectPoint(X0, Y0, Z0))
        return;
    AdjustScreenMinMax();
    x0 = Xp;
    y0 = Yp;
    LongWorldRotatePoint(xw1, yw0, zw0);
    if (!LongProjectPoint(X0, Y0, Z0))
        return;
    AdjustScreenMinMax();
    x1 = Xp;
    y1 = Yp;
    LongWorldRotatePoint(xw1, yw0, zw1);
    if (!LongProjectPoint(X0, Y0, Z0))
        return;
    AdjustScreenMinMax();
    x2 = Xp;
    y2 = Yp;
    LongWorldRotatePoint(xw0, yw0, zw1);
    if (!LongProjectPoint(X0, Y0, Z0))
        return;
    AdjustScreenMinMax();
    x3 = Xp;
    y3 = Yp;
    LongWorldRotatePoint(xw0, yw1, zw0);
    if (!LongProjectPoint(X0, Y0, Z0))
        return;
    AdjustScreenMinMax();
    x4 = Xp;
    y4 = Yp;
    LongWorldRotatePoint(xw1, yw1, zw0);
    if (!LongProjectPoint(X0, Y0, Z0))
        return;
    AdjustScreenMinMax();
    x5 = Xp;
    y5 = Yp;
    LongWorldRotatePoint(xw1, yw1, zw1);
    if (!LongProjectPoint(X0, Y0, Z0))
        return;
    AdjustScreenMinMax();
    x6 = Xp;
    y6 = Yp;
    LongWorldRotatePoint(xw0, yw1, zw1);
    if (!LongProjectPoint(X0, Y0, Z0))
        return;
    AdjustScreenMinMax();
    x7 = Xp;
    y7 = Yp;

    Line(x0, y0, x1, y1, coul);
    Line(x1, y1, x2, y2, coul);
    Line(x2, y2, x3, y3, coul);
    Line(x3, y3, x0, y0, coul);

    Line(x0, y0, x4, y4, coul1);
    Line(x1, y1, x5, y5, coul1);
    Line(x2, y2, x6, y6, coul1);
    Line(x3, y3, x7, y7, coul1);
    Line(x4, y4, x5, y5, coul1);
    Line(x5, y5, x6, y6, coul1);
    Line(x6, y6, x7, y7, coul1);
    Line(x7, y7, x4, y4, coul1);
}

// Version ZVs absolues
void AffZVObjet(T_DECORS *ptrobj) {
    DrawCubeNoBox(ptrobj->Xworld, ptrobj->Yworld, ptrobj->Zworld,
                  ptrobj->XMin, ptrobj->YMin, ptrobj->ZMin,
                  ptrobj->XMax, ptrobj->YMax, ptrobj->ZMax);
}
#endif

/*══════════════════════════════════════════════════════════════════════════*/
#ifdef LBA2CC_EDITOR
static S32 ExtDecorOverlayProject(S32 x, S32 y, S32 z, S32 *xs, S32 *ys) {
    LongWorldRotatePoint(x, y, z);
    if (!LongProjectPoint(X0, Y0, Z0))
        return FALSE;
    if (Xp < 0 OR Xp >= (S32)ModeDesiredX OR Yp < 0 OR Yp >= (S32)ModeDesiredY)
        return FALSE;
    *xs = Xp;
    *ys = Yp;
    return TRUE;
}

static void ExtDecorOverlayDrawBoxColor(T_DECORS *ptrobj, S16 coul, S16 coul1) {
    S32 x0, y0, x1, y1, x2, y2, x3, y3, x4, y4, x5, y5, x6, y6, x7, y7;

    if (!ExtDecorOverlayProject(ptrobj->XMin, ptrobj->YMin, ptrobj->ZMin, &x0, &y0))
        return;
    if (!ExtDecorOverlayProject(ptrobj->XMax, ptrobj->YMin, ptrobj->ZMin, &x1, &y1))
        return;
    if (!ExtDecorOverlayProject(ptrobj->XMax, ptrobj->YMin, ptrobj->ZMax, &x2, &y2))
        return;
    if (!ExtDecorOverlayProject(ptrobj->XMin, ptrobj->YMin, ptrobj->ZMax, &x3, &y3))
        return;
    if (!ExtDecorOverlayProject(ptrobj->XMin, ptrobj->YMax, ptrobj->ZMin, &x4, &y4))
        return;
    if (!ExtDecorOverlayProject(ptrobj->XMax, ptrobj->YMax, ptrobj->ZMin, &x5, &y5))
        return;
    if (!ExtDecorOverlayProject(ptrobj->XMax, ptrobj->YMax, ptrobj->ZMax, &x6, &y6))
        return;
    if (!ExtDecorOverlayProject(ptrobj->XMin, ptrobj->YMax, ptrobj->ZMax, &x7, &y7))
        return;

    Line(x0, y0, x1, y1, coul);
    Line(x1, y1, x2, y2, coul);
    Line(x2, y2, x3, y3, coul);
    Line(x3, y3, x0, y0, coul);

    Line(x0, y0, x4, y4, coul1);
    Line(x1, y1, x5, y5, coul1);
    Line(x2, y2, x6, y6, coul1);
    Line(x3, y3, x7, y7, coul1);
    Line(x4, y4, x5, y5, coul1);
    Line(x5, y5, x6, y6, coul1);
    Line(x6, y6, x7, y7, coul1);
    Line(x7, y7, x4, y4, coul1);
}

static void ExtDecorOverlayDrawBox(T_DECORS *ptrobj) {
    ExtDecorOverlayDrawBoxColor(ptrobj, EXT_DECOR_OVERLAY_BOX, EXT_DECOR_OVERLAY_BOX_AXIS);
}

static S32 ExtDecorEditorIsSelectedObject(T_DECORS *ptrobj, S32 num) {
    S32 i;
    S32 body;

    if (!ExtDecorEditorSelect)
        return FALSE;

    body = ptrobj->Body & 0xFFFF;
    for (i = 0; i < EXT_DECOR_EDITOR_MAX_SELECTED; i++) {
        T_EXT_DECOR_EDITOR_SEL *sel = &ExtDecorEditorSelList[i];

        if (!sel->Used)
            continue;
        if (sel->CubeX == ExtDecorEditorLoadedCubeX AND sel->CubeY == ExtDecorEditorLoadedCubeY AND sel->Num == num AND sel->Body == body)
            return TRUE;
    }
    return FALSE;
}

static T_EXT_DECOR_EDITOR_EDIT *ExtDecorEditorFindEdit(S32 cube_x, S32 cube_y, S32 num, S32 body) {
    S32 i;

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_EDITS; i++) {
        T_EXT_DECOR_EDITOR_EDIT *edit = &ExtDecorEditorEdits[i];
        if (edit->Used
                AND edit->CubeX == cube_x AND edit->CubeY == cube_y AND edit->Num == num AND edit->Body == body)
            return edit;
    }

    return NULL;
}

static T_EXT_DECOR_EDITOR_EDIT *ExtDecorEditorGetOrCreateEdit(S32 cube_x, S32 cube_y, S32 num, T_DECORS *ptrobj) {
    S32 i;
    S32 body;
    T_EXT_DECOR_EDITOR_EDIT *edit;

    body = ptrobj->Body & 0xFFFF;
    edit = ExtDecorEditorFindEdit(cube_x, cube_y, num, body);
    if (edit)
        return edit;

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_EDITS; i++) {
        edit = &ExtDecorEditorEdits[i];
        if (!edit->Used) {
            edit->Used = TRUE;
            edit->Deleted = FALSE;
            edit->CubeX = cube_x;
            edit->CubeY = cube_y;
            edit->Num = num;
            edit->Body = body;
            edit->HasExpectBody = TRUE;
            edit->NewBody = body;
            edit->BaseBodyRaw = ptrobj->Body;
            edit->BaseCodeJeu = ptrobj->CodeJeu;
            edit->BaseBeta = ptrobj->Beta;
            edit->BaseX = ptrobj->Xworld;
            edit->BaseY = ptrobj->Yworld;
            edit->BaseZ = ptrobj->Zworld;
            edit->BaseXMin = ptrobj->XMin;
            edit->BaseYMin = ptrobj->YMin;
            edit->BaseZMin = ptrobj->ZMin;
            edit->BaseXMax = ptrobj->XMax;
            edit->BaseYMax = ptrobj->YMax;
            edit->BaseZMax = ptrobj->ZMax;
            edit->DBeta = 0;
            edit->Dx = 0;
            edit->Dy = 0;
            edit->Dz = 0;
            edit->SkipLogged = FALSE;
            return edit;
        }
    }

    Log_Warn("extedit runtime edit table is full; cannot track c%d,%d #%d b%d",
             (int)cube_x,
             (int)cube_y,
             (int)num,
             (int)body);
    return NULL;
}

static void ExtDecorEditorRotateXZ(S32 x, S32 z, S32 turn, S32 *out_x, S32 *out_z) {
    turn &= 3;
    if (turn == 0) {
        *out_x = x;
        *out_z = z;
    } else if (turn == 1) {
        *out_x = z;
        *out_z = -x;
    } else if (turn == 2) {
        *out_x = -x;
        *out_z = -z;
    } else {
        *out_x = -z;
        *out_z = x;
    }
}

static S32 ExtDecorEditorAngleDeltaToTurn(S32 delta) {
    delta &= 4095;
    return ((delta + 512) / 1024) & 3;
}

static S32 ExtDecorEditorRotateDirection(S32 direction, S32 turn) {
    S32 i;

    turn &= 3;
    for (i = 0; i < turn; i++) {
        if (direction == 1)
            direction = 8;
        else if (direction == 8)
            direction = 2;
        else if (direction == 2)
            direction = 4;
        else if (direction == 4)
            direction = 1;
    }
    return direction;
}

static void ExtDecorEditorSetRotatedBounds(T_ZONE *zone, S32 x, S32 y, S32 z, S32 beta, S32 x0, S32 y0, S32 z0, S32 x1, S32 y1, S32 z1) {
    S32 turn;
    S32 xs[4];
    S32 zs[4];
    S32 i;

    turn = ExtDecorEditorAngleDeltaToTurn(beta - 2048);
    ExtDecorEditorRotateXZ(x0, z0, turn, &xs[0], &zs[0]);
    ExtDecorEditorRotateXZ(x0, z1, turn, &xs[1], &zs[1]);
    ExtDecorEditorRotateXZ(x1, z0, turn, &xs[2], &zs[2]);
    ExtDecorEditorRotateXZ(x1, z1, turn, &xs[3], &zs[3]);

    zone->X0 = xs[0] + x;
    zone->X1 = zone->X0;
    zone->Z0 = zs[0] + z;
    zone->Z1 = zone->Z0;
    for (i = 1; i < 4; i++) {
        S32 xr = xs[i] + x;
        S32 zr = zs[i] + z;

        if (xr < zone->X0)
            zone->X0 = xr;
        if (xr > zone->X1)
            zone->X1 = xr;
        if (zr < zone->Z0)
            zone->Z0 = zr;
        if (zr > zone->Z1)
            zone->Z1 = zr;
    }
    zone->Y0 = y + y0;
    zone->Y1 = y + y1;
}

static S32 ExtDecorEditorNextCameraZoneNum(void) {
    S32 i;
    S32 max_num = 0;

    if (!ListZone)
        return 1;

    for (i = 0; i < NbZones; i++) {
        if (ListZone[i].Type == 1 AND ListZone[i].Num > max_num)
            max_num = ListZone[i].Num;
    }
    return max_num + 1;
}

static S32 ExtDecorEditorIsCitadelProximaBody(S32 library) {
    if (library == 0)
        return Island == 0 OR Island == 12;
    return library == 0 OR library == 12;
}

static void ExtDecorEditorMaybeCreateLinkedZones(S32 slot, T_DECORS *obj) {
    S32 library;
    S32 body_index;
    S32 body;
    S32 beta;
    S32 turn;
    T_ZONE zone;

    if (!obj)
        return;

    ExtDecorEditorDecodeBody(obj->Body, &library, &body_index);
    body = obj->Body & 0xFFFF;
    beta = obj->Beta & 4095;
    turn = ExtDecorEditorAngleDeltaToTurn(beta - 2048);

    if (body_index == 5 AND ExtDecorEditorIsCitadelProximaBody(library)) {
        S32 camera_num = ExtDecorEditorNextCameraZoneNum();

        memset(&zone, 0, sizeof(zone));
        zone.Type = 5;
        zone.Num = 0;
        zone.Info0 = 12;
        zone.Info1 = camera_num;
        zone.Info2 = ExtDecorEditorRotateDirection(1, turn);
        zone.Info6 = 15000;
        zone.Info7 = 1;
        ExtDecorEditorSetRotatedBounds(&zone, obj->Xworld, obj->Yworld, obj->Zworld, beta, -432, 0, 256, 384, 1216, 768);
        ZoneEditorCreateLinkedDecorZone(&zone, ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, slot, body, EXT_DECOR_LINK_ROLE_SIGN_TEXT_A, obj->Xworld, obj->Yworld, obj->Zworld, beta);

        memset(&zone, 0, sizeof(zone));
        zone.Type = 1;
        zone.Num = camera_num;
        zone.Info0 = StartXCube;
        zone.Info1 = StartYCube;
        zone.Info2 = StartZCube;
        zone.Info3 = AlphaCam;
        zone.Info4 = BetaCam;
        zone.Info5 = GammaCam;
        zone.Info6 = VueDistance;
        zone.Info7 = 1;
        ExtDecorEditorSetRotatedBounds(&zone, obj->Xworld, obj->Yworld, obj->Zworld, beta, -256, 0, 256, 256, 256, 768);
        ZoneEditorCreateLinkedDecorZone(&zone, ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, slot, body, EXT_DECOR_LINK_ROLE_SIGN_CAMERA, obj->Xworld, obj->Yworld, obj->Zworld, beta);

        memset(&zone, 0, sizeof(zone));
        zone.Type = 5;
        zone.Num = 0;
        zone.Info0 = 12;
        zone.Info1 = camera_num;
        zone.Info2 = ExtDecorEditorRotateDirection(2, turn);
        zone.Info6 = 15000;
        zone.Info7 = 1;
        ExtDecorEditorSetRotatedBounds(&zone, obj->Xworld, obj->Yworld, obj->Zworld, beta, -464, 0, -768, 432, 1184, -256);
        ZoneEditorCreateLinkedDecorZone(&zone, ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, slot, body, EXT_DECOR_LINK_ROLE_SIGN_TEXT_B, obj->Xworld, obj->Yworld, obj->Zworld, beta);
        ExtDecorEditorSelectSlot(slot);
        Log_Info("[editor] created sign zones for decor c%d,%d #%d", (int)ExtDecorEditorLoadedCubeX, (int)ExtDecorEditorLoadedCubeY, (int)slot);
    } else if (body_index == 34) {
        memset(&zone, 0, sizeof(zone));
        zone.Type = 4;
        zone.Num = 0;
        zone.Info0 = 112;
        zone.Info1 = 1;
        ExtDecorEditorSetRotatedBounds(&zone, obj->Xworld, obj->Yworld, obj->Zworld, beta, -1040, -47, -880, 860, 603, 1020);
        ZoneEditorCreateLinkedDecorZone(&zone, ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, slot, body, EXT_DECOR_LINK_ROLE_BONUS, obj->Xworld, obj->Yworld, obj->Zworld, beta);
        ExtDecorEditorSelectSlot(slot);
        Log_Info("[editor] created trashcan bonus zone for decor c%d,%d #%d", (int)ExtDecorEditorLoadedCubeX, (int)ExtDecorEditorLoadedCubeY, (int)slot);
    } else if (body_index == 41) {
        memset(&zone, 0, sizeof(zone));
        zone.Type = 4;
        zone.Num = 0;
        zone.Info0 = 112;
        zone.Info1 = 1;
        ExtDecorEditorSetRotatedBounds(&zone, obj->Xworld, obj->Yworld, obj->Zworld, beta, -1024, 0, -1056, 928, 650, 896);
        ZoneEditorCreateLinkedDecorZone(&zone, ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, slot, body, EXT_DECOR_LINK_ROLE_BONUS, obj->Xworld, obj->Yworld, obj->Zworld, beta);
        ExtDecorEditorSelectSlot(slot);
        Log_Info("[editor] created planter bonus zone for decor c%d,%d #%d", (int)ExtDecorEditorLoadedCubeX, (int)ExtDecorEditorLoadedCubeY, (int)slot);
    }
}

static void ExtDecorEditorApplyEditBounds(T_EXT_DECOR_EDITOR_EDIT *edit, T_DECORS *ptrobj) {
    S32 turn;
    S32 xs[4];
    S32 zs[4];
    S32 i;

    turn = (((edit->DBeta & 4095) + 512) / 1024) & 3;
    ExtDecorEditorRotateXZ(edit->BaseXMin - edit->BaseX, edit->BaseZMin - edit->BaseZ, turn, &xs[0], &zs[0]);
    ExtDecorEditorRotateXZ(edit->BaseXMin - edit->BaseX, edit->BaseZMax - edit->BaseZ, turn, &xs[1], &zs[1]);
    ExtDecorEditorRotateXZ(edit->BaseXMax - edit->BaseX, edit->BaseZMin - edit->BaseZ, turn, &xs[2], &zs[2]);
    ExtDecorEditorRotateXZ(edit->BaseXMax - edit->BaseX, edit->BaseZMax - edit->BaseZ, turn, &xs[3], &zs[3]);

    ptrobj->XMin = xs[0];
    ptrobj->XMax = xs[0];
    ptrobj->ZMin = zs[0];
    ptrobj->ZMax = zs[0];
    for (i = 1; i < 4; i++) {
        if (xs[i] < ptrobj->XMin)
            ptrobj->XMin = xs[i];
        if (xs[i] > ptrobj->XMax)
            ptrobj->XMax = xs[i];
        if (zs[i] < ptrobj->ZMin)
            ptrobj->ZMin = zs[i];
        if (zs[i] > ptrobj->ZMax)
            ptrobj->ZMax = zs[i];
    }

    ptrobj->XMin += edit->BaseX + edit->Dx;
    ptrobj->XMax += edit->BaseX + edit->Dx;
    ptrobj->YMin = edit->BaseYMin + edit->Dy;
    ptrobj->YMax = edit->BaseYMax + edit->Dy;
    ptrobj->ZMin += edit->BaseZ + edit->Dz;
    ptrobj->ZMax += edit->BaseZ + edit->Dz;
}

static void ExtDecorEditorSyncLinkedZonesForEdit(T_EXT_DECOR_EDITOR_EDIT *edit) {
    ZoneEditorSyncLinkedDecorZones(edit->CubeX,
                                   edit->CubeY,
                                   edit->Num,
                                   edit->Body,
                                   edit->BaseX + edit->Dx,
                                   edit->BaseY + edit->Dy,
                                   edit->BaseZ + edit->Dz,
                                   (edit->BaseBeta + edit->DBeta) & 4095);
}

static void ExtDecorEditorApplyEditToObject(T_EXT_DECOR_EDITOR_EDIT *edit, T_DECORS *ptrobj) {
    if (edit->Deleted) {
        ptrobj->Body |= DEC_INVISIBLE;
        return;
    }

    ptrobj->Body = (ptrobj->Body & ~0xFFFF) | (edit->NewBody & 0xFFFF);
    ptrobj->Body &= ~DEC_INVISIBLE;
    ptrobj->Beta = (edit->BaseBeta + edit->DBeta) & 0xFFFF;
    ptrobj->Xworld = edit->BaseX + edit->Dx;
    ptrobj->Yworld = edit->BaseY + edit->Dy;
    ptrobj->Zworld = edit->BaseZ + edit->Dz;
    ExtDecorEditorApplyEditBounds(edit, ptrobj);
    ExtDecorEditorSyncLinkedZonesForEdit(edit);
}

void ExtDecorEditorReleaseOwnedDecorList(void) {
    if (!ExtDecorEditorOwnedList)
        return;
    if (ListDecors == ExtDecorEditorOwnedList)
        ListDecors = NULL;
    free(ExtDecorEditorOwnedList);
    ExtDecorEditorOwnedList = NULL;
}

static T_EXT_DECOR_EDITOR_EDIT *ExtDecorEditorFindDeletedEdit(S32 cube_x, S32 cube_y, S32 num) {
    S32 i;

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_EDITS; i++) {
        T_EXT_DECOR_EDITOR_EDIT *edit = &ExtDecorEditorEdits[i];
        if (edit->Used
                AND edit->Deleted
                    AND edit->CubeX == cube_x AND edit->CubeY == cube_y AND edit->Num == num)
            return edit;
    }
    return NULL;
}

static T_EXT_DECOR_EDITOR_EDIT *ExtDecorEditorFindAnyEdit(S32 cube_x, S32 cube_y, S32 num) {
    S32 i;

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_EDITS; i++) {
        T_EXT_DECOR_EDITOR_EDIT *edit = &ExtDecorEditorEdits[i];
        if (edit->Used
                AND edit->CubeX == cube_x AND edit->CubeY == cube_y AND edit->Num == num)
            return edit;
    }
    return NULL;
}

static S32 ExtDecorEditorObjectDeleted(S32 cube_x, S32 cube_y, S32 num) {
    return ExtDecorEditorFindDeletedEdit(cube_x, cube_y, num) != NULL;
}

void ExtDecorEditorApplyRuntimeEditsForLoadedCube(S32 cube_x, S32 cube_y) {
    S32 i;
    T_DECORS *ptrobj;

    ptrobj = ListDecors;
    for (i = 0; i < NbObjDecors AND ptrobj; i++, ptrobj++) {
        T_EXT_DECOR_EDITOR_EDIT *edit;
        S32 current_body;

        edit = ExtDecorEditorFindAnyEdit(cube_x, cube_y, i);
        if (!edit)
            continue;

        current_body = ptrobj->Body & 0xFFFF;
        if (edit->HasExpectBody AND current_body != edit->Body) {
            if (!edit->SkipLogged) {
                Log_Warn("[editor] decor edit skipped c%d,%d #%d: body %d does not match expected %d",
                         (int)cube_x,
                         (int)cube_y,
                         (int)i,
                         (int)current_body,
                         (int)edit->Body);
                edit->SkipLogged = TRUE;
            }
            continue;
        }

        ExtDecorEditorApplyEditToObject(edit, ptrobj);
    }

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_EDITS; i++) {
        T_EXT_DECOR_EDITOR_EDIT *edit = &ExtDecorEditorEdits[i];
        if (edit->Used
                AND edit->CubeX == cube_x AND edit->CubeY == cube_y AND edit->Num >= NbObjDecors) {
            if (!edit->HasExpectBody AND edit->Num == NbObjDecors) {
                ExtDecorEditorAppendObjectFromEdit(edit);
                continue;
            }
            if (!edit->SkipLogged) {
                Log_Warn("[editor] decor edit skipped c%d,%d #%d: object index is out of range [0,%d)",
                         (int)cube_x,
                         (int)cube_y,
                         (int)edit->Num,
                         (int)NbObjDecors);
                edit->SkipLogged = TRUE;
            }
        }
    }
}

static T_EXT_DECOR_EDITOR_EDIT *ExtDecorEditorImportedEdit(S32 cube_x, S32 cube_y, S32 num) {
    S32 i;
    T_EXT_DECOR_EDITOR_EDIT *edit;

    edit = ExtDecorEditorFindAnyEdit(cube_x, cube_y, num);
    if (edit)
        return edit;

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_EDITS; i++) {
        edit = &ExtDecorEditorEdits[i];
        if (!edit->Used) {
            memset(edit, 0, sizeof(*edit));
            edit->Used = TRUE;
            edit->CubeX = cube_x;
            edit->CubeY = cube_y;
            edit->Num = num;
            return edit;
        }
    }

    Log_Warn("[editor] decor import skipped c%d,%d #%d: runtime edit table is full",
             (int)cube_x,
             (int)cube_y,
             (int)num);
    return NULL;
}

static S32 ExtDecorEditorImportObject(const char *json,
                                      jsmntok_t *tokens,
                                      S32 count,
                                      S32 cube_x,
                                      S32 cube_y,
                                      S32 object,
                                      S32 *skipped) {
    S32 num;
    S32 body;
    S32 body_raw;
    S32 code_jeu;
    S32 beta_raw;
    S32 dbeta;
    S32 deleted;
    S32 pos[3];
    S32 zv[6];
    S32 value;
    T_EXT_DECOR_EDITOR_EDIT *edit;

    if (!EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, object, "object"), &num)) {
        (*skipped)++;
        return FALSE;
    }
    if (!EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, object, "body"), &body)) {
        (*skipped)++;
        return FALSE;
    }

    value = EditorJsonFindValue(json, tokens, count, object, "body_raw");
    if (!EditorJsonParseInt(json, tokens, count, value, &body_raw))
        body_raw = body;
    value = EditorJsonFindValue(json, tokens, count, object, "code_jeu");
    if (!EditorJsonParseInt(json, tokens, count, value, &code_jeu))
        code_jeu = 0;
    value = EditorJsonFindValue(json, tokens, count, object, "beta_raw");
    if (!EditorJsonParseInt(json, tokens, count, value, &beta_raw))
        beta_raw = 0;
    value = EditorJsonFindValue(json, tokens, count, object, "dbeta");
    if (!EditorJsonParseInt(json, tokens, count, value, &dbeta))
        dbeta = 0;
    dbeta &= 4095;
    value = EditorJsonFindValue(json, tokens, count, object, "deleted");
    if (!EditorJsonParseBool(json, tokens, count, value, &deleted))
        deleted = FALSE;

    if (!EditorJsonParseIntArray(json, tokens, count, EditorJsonFindValue(json, tokens, count, object, "position"), pos, 3)) {
        (*skipped)++;
        return FALSE;
    }
    if (!EditorJsonParseIntArray(json, tokens, count, EditorJsonFindValue(json, tokens, count, object, "zv"), zv, 6)) {
        (*skipped)++;
        return FALSE;
    }

    edit = ExtDecorEditorImportedEdit(cube_x, cube_y, num);
    if (!edit) {
        (*skipped)++;
        return FALSE;
    }

    edit->Deleted = deleted ? TRUE : FALSE;
    edit->Body = body;
    edit->HasExpectBody = EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, object, "expect_body"), &edit->Body);
    edit->NewBody = body;
    edit->BaseBodyRaw = body_raw;
    edit->BaseCodeJeu = code_jeu;
    edit->BaseBeta = beta_raw;
    edit->BaseX = pos[0];
    edit->BaseY = pos[1];
    edit->BaseZ = pos[2];
    edit->BaseXMin = zv[0];
    edit->BaseYMin = zv[1];
    edit->BaseZMin = zv[2];
    edit->BaseXMax = zv[3];
    edit->BaseYMax = zv[4];
    edit->BaseZMax = zv[5];
    edit->DBeta = dbeta;
    edit->Dx = 0;
    edit->Dy = 0;
    edit->Dz = 0;
    edit->SkipLogged = FALSE;
    return TRUE;
}

void ExtDecorEditorLoadAuthoringEditsForCurrentIsland(void) {
    char dir[ADELINE_MAX_PATH];
    char path[ADELINE_MAX_PATH];
    char *json;
    jsmntok_t *tokens;
    S32 count;
    S32 root;
    S32 island_value;
    S32 cubes;
    S32 cube_index;
    S32 loaded = 0;
    S32 skipped = 0;

    if (!Editor_ResolveOutputDir(dir, sizeof(dir)))
        return;
    snprintf(path, sizeof(path), "%s%sdecor_edits.json", dir, ADELINE_PATH_SEP);
    path[sizeof(path) - 1] = '\0';

    if (!EditorJsonParseFile(path, &json, &tokens, &count)) {
        if (EditorJsonFileExists(path))
            Log_Warn("[editor] cannot load decor edits: invalid JSON: %s", path);
        return;
    }

    root = 0;
    if (EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, root, "island"), &island_value)
            AND island_value != Island) {
        Log_Warn("[editor] decor edits ignored: %s is for island %d, current island is %d",
                 path,
                 (int)island_value,
                 (int)Island);
        free(tokens);
        free(json);
        return;
    }

    cubes = EditorJsonFindValue(json, tokens, count, root, "cubes");
    if (cubes >= 0 AND tokens[cubes].type == JSMN_ARRAY) {
        S32 cubes_end = EditorJsonTokenEnd(tokens, count, cubes);

        for (cube_index = cubes + 1; cube_index < cubes_end; cube_index = EditorJsonTokenEnd(tokens, count, cube_index)) {
            S32 cube_xy[2];
            S32 objects;
            S32 object_index;
            S32 objects_end;

            if (tokens[cube_index].parent != cubes)
                continue;
            if (tokens[cube_index].type != JSMN_OBJECT) {
                skipped++;
                continue;
            }
            if (!EditorJsonParseIntArray(json, tokens, count, EditorJsonFindValue(json, tokens, count, cube_index, "cube"), cube_xy, 2)) {
                skipped++;
                continue;
            }
            objects = EditorJsonFindValue(json, tokens, count, cube_index, "objects");
            if (objects < 0 OR tokens[objects].type != JSMN_ARRAY) {
                skipped++;
                continue;
            }
            objects_end = EditorJsonTokenEnd(tokens, count, objects);
            for (object_index = objects + 1; object_index < objects_end; object_index = EditorJsonTokenEnd(tokens, count, object_index)) {
                if (tokens[object_index].parent != objects)
                    continue;
                if (tokens[object_index].type != JSMN_OBJECT) {
                    skipped++;
                    continue;
                }
                if (ExtDecorEditorImportObject(json, tokens, count, cube_xy[0], cube_xy[1], object_index, &skipped))
                    loaded++;
            }
        }
    }

    Log_Info("[editor] loaded decor edits: %s (loaded=%d skipped=%d)", path, (int)loaded, (int)skipped);
    free(tokens);
    free(json);
}

static T_DECORS *ExtDecorEditorSelectedObject(void) {
    S32 i;
    T_DECORS *selected = NULL;

    if (!ListDecors)
        return NULL;

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_SELECTED; i++) {
        T_EXT_DECOR_EDITOR_SEL *sel = &ExtDecorEditorSelList[i];

        if (!sel->Used)
            continue;
        if (sel->CubeX == ExtDecorEditorLoadedCubeX AND sel->CubeY == ExtDecorEditorLoadedCubeY AND sel->Num >= 0 AND sel->Num < NbObjDecors AND((ListDecors[sel->Num].Body & 0xFFFF) == sel->Body)) {
            if (selected)
                return NULL;
            selected = &ListDecors[sel->Num];
        }
    }
    return selected;
}

T_DECORS *ExtDecorEditorGetSelectedObject(void) {
    return ExtDecorEditorSelectedObject();
}

S32 ExtDecorEditorLogSelected(void) {
    S32 i;
    S32 count = 0;

    if (!ListDecors OR NbObjDecors <= 0)
        return 0;

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_SELECTED; i++) {
        T_EXT_DECOR_EDITOR_SEL *sel = &ExtDecorEditorSelList[i];
        T_DECORS *obj;

        if (!sel->Used)
            continue;
        if (sel->CubeX != ExtDecorEditorLoadedCubeX OR sel->CubeY != ExtDecorEditorLoadedCubeY)
            continue;
        if (sel->Num < 0 OR sel->Num >= NbObjDecors)
            continue;

        obj = &ListDecors[sel->Num];
        if ((obj->Body & 0xFFFF) != sel->Body)
            continue;

        Log_Raw("[decor c%d,%d #%d] body=%d body_raw=%d code_jeu=%d beta=%d pos=[%d,%d,%d] zv=[%d,%d,%d,%d,%d,%d]",
                (int)sel->CubeX,
                (int)sel->CubeY,
                (int)sel->Num,
                (int)(obj->Body & 0xFFFF),
                (int)obj->Body,
                (int)obj->CodeJeu,
                (int)obj->Beta,
                (int)obj->Xworld,
                (int)obj->Yworld,
                (int)obj->Zworld,
                (int)obj->XMin,
                (int)obj->YMin,
                (int)obj->ZMin,
                (int)obj->XMax,
                (int)obj->YMax,
                (int)obj->ZMax);
        count++;
    }

    return count;
}

S32 ExtDecorEditorSetSelectedField(S32 field, S32 value) {
    T_DECORS *obj = ExtDecorEditorSelectedObject();
    T_EXT_DECOR_EDITOR_EDIT *edit;
    S32 i;
    S32 num;

    if (!obj OR field < 0 OR field >= EXT_DECOR_FIELD_COUNT)
        return FALSE;

    num = (S32)(obj - ListDecors);
    edit = ExtDecorEditorFindAnyEdit(ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, num);
    if (!edit)
        edit = ExtDecorEditorGetOrCreateEdit(ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, num, obj);
    if (!edit)
        return FALSE;

    switch (field) {
    case EXT_DECOR_FIELD_BODY:
        if (value < 0)
            value = 0;
        if (value > 0xFFFF)
            value = 0xFFFF;
        edit->NewBody = value;
        break;
    case EXT_DECOR_FIELD_ROTATION:
        value &= 3;
        edit->DBeta = ((value * 1024) - edit->BaseBeta) & 4095;
        break;
    case EXT_DECOR_FIELD_X:
        edit->Dx = value - edit->BaseX;
        break;
    case EXT_DECOR_FIELD_Y:
        edit->Dy = value - edit->BaseY;
        break;
    case EXT_DECOR_FIELD_Z:
        edit->Dz = value - edit->BaseZ;
        break;
    }

    ExtDecorEditorApplyEditToObject(edit, obj);
    for (i = 0; i < EXT_DECOR_EDITOR_MAX_SELECTED; i++) {
        if (ExtDecorEditorSelList[i].Used
                AND ExtDecorEditorSelList[i]
                    .CubeX == ExtDecorEditorLoadedCubeX AND ExtDecorEditorSelList[i].CubeY == ExtDecorEditorLoadedCubeY AND ExtDecorEditorSelList[i].Num == num)
            ExtDecorEditorSelList[i].Body = obj->Body & 0xFFFF;
    }
    return TRUE;
}

static T_DECORS *ExtDecorEditorAnySelectedObject(void) {
    S32 i;

    if (!ListDecors)
        return NULL;

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_SELECTED; i++) {
        T_EXT_DECOR_EDITOR_SEL *sel = &ExtDecorEditorSelList[i];

        if (!sel->Used)
            continue;
        if (sel->CubeX == ExtDecorEditorLoadedCubeX AND sel->CubeY == ExtDecorEditorLoadedCubeY AND sel->Num >= 0 AND sel->Num < NbObjDecors)
            return &ListDecors[sel->Num];
    }
    return NULL;
}

static S32 ExtDecorEditorCurrentPlaceBody(void) {
    T_DECORS *src = ExtDecorEditorAnySelectedObject();

    if (Editor_ToolIs(EDITOR_TOOL_DECOR_PLACE))
        return ExtDecorEditorEncodeBody(ExtDecorEditorPaletteLibrary, ExtDecorEditorPaletteBody);
    if (src)
        return src->Body & 0xFFFF;
    return ExtDecorEditorEncodeBody(ExtDecorEditorPaletteLibrary, ExtDecorEditorPaletteBody);
}

static S32 ExtDecorEditorFirstDeletedSlot(void) {
    S32 i;

    for (i = 0; i < NbObjDecors; i++) {
        if (ExtDecorEditorObjectDeleted(ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, i))
            return i;
    }
    return -1;
}

static void ExtDecorEditorMoveObjectTo(T_DECORS *obj, S32 x, S32 y, S32 z) {
    S32 dx;
    S32 dy;
    S32 dz;

    dx = x - obj->Xworld;
    dy = y - obj->Yworld;
    dz = z - obj->Zworld;

    obj->Xworld += dx;
    obj->Yworld += dy;
    obj->Zworld += dz;
    obj->XMin += dx;
    obj->YMin += dy;
    obj->ZMin += dz;
    obj->XMax += dx;
    obj->YMax += dy;
    obj->ZMax += dz;
}

static S32 ExtDecorEditorResizeLoadedList(S32 new_count) {
    T_DECORS *new_list;

    if (new_count <= NbObjDecors)
        return TRUE;

    new_list = (T_DECORS *)malloc((size_t)new_count * sizeof(T_DECORS));
    if (!new_list) {
        Editor_SetStatus("Decor append failed: out of memory");
        Log_Warn("[editor] decor append failed: out of memory");
        return FALSE;
    }

    if (ListDecors AND NbObjDecors > 0)
        memcpy(new_list, ListDecors, (size_t)NbObjDecors * sizeof(T_DECORS));
    if (ExtDecorEditorOwnedList)
        free(ExtDecorEditorOwnedList);
    ExtDecorEditorOwnedList = new_list;
    ListDecors = new_list;
    NbObjDecors = new_count;
    return TRUE;
}

static S32 ExtDecorEditorInitDefaultObject(T_DECORS *obj, S32 body, S32 beta, S32 x, S32 y, S32 z) {
    S32 library;
    S32 body_index;
    S32 bounds_island;
    const char *bounds_island_name;

    memset(obj, 0, sizeof(*obj));
    obj->Body = body & 0xFFFF;
    obj->Xworld = x;
    obj->Yworld = y;
    obj->Zworld = z;
    obj->Beta = beta & 4095;
    ExtDecorEditorDecodeBody(body, &library, &body_index);
    if (body_index < 0) {
        Editor_SetStatus("Decor place refused: invalid encoded body");
        Log_Warn("[editor] decor place refused: invalid encoded body %d", (int)(body & 0xFFFF));
        return FALSE;
    }
    bounds_island = library > 0 ? library : Island;
    bounds_island_name = library > 0 ? IleLst[library] : IslandName;
    if (!EditorDecorBoundsApply(bounds_island_name, bounds_island, body_index, obj->Beta, x, y, z, obj)) {
        Editor_SetStatus("Decor place refused: no inferred bounds for body");
        Log_Warn("[editor] decor place refused: no inferred bounds for island %d %s body %d raw %d",
                 (int)bounds_island,
                 bounds_island_name,
                 (int)body_index,
                 (int)(body & 0xFFFF));
        return FALSE;
    }
    return TRUE;
}

static S32 ExtDecorEditorAppendObject(T_DECORS *src, S32 body, S32 beta, S32 x, S32 y, S32 z, S32 select) {
    S32 slot = NbObjDecors;
    T_EXT_DECOR_EDITOR_EDIT *edit;

    if (!ExtDecorEditorResizeLoadedList(NbObjDecors + 1))
        return -1;

    if (src) {
        ListDecors[slot] = *src;
        ListDecors[slot].Body = (ListDecors[slot].Body & ~0xFFFF) | (body & 0xFFFF);
        ListDecors[slot].Body &= ~DEC_INVISIBLE;
        ListDecors[slot].Beta = beta;
        ExtDecorEditorMoveObjectTo(&ListDecors[slot], x, y, z);
    } else {
        if (!ExtDecorEditorInitDefaultObject(&ListDecors[slot], body, beta, x, y, z)) {
            NbObjDecors = slot;
            return -1;
        }
    }

    edit = ExtDecorEditorGetOrCreateEdit(ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, slot, &ListDecors[slot]);
    if (edit) {
        edit->HasExpectBody = FALSE;
        edit->Body = body & 0xFFFF;
        edit->NewBody = body & 0xFFFF;
    }
    if (select)
        ExtDecorEditorSelectSlot(slot);
    return slot;
}

static S32 ExtDecorEditorAppendObjectFromEdit(T_EXT_DECOR_EDITOR_EDIT *edit) {
    S32 slot;

    if (edit->Num != NbObjDecors)
        return FALSE;

    slot = ExtDecorEditorAppendObject(NULL,
                                      edit->NewBody,
                                      (edit->BaseBeta + edit->DBeta) & 0xFFFF,
                                      edit->BaseX + edit->Dx,
                                      edit->BaseY + edit->Dy,
                                      edit->BaseZ + edit->Dz,
                                      FALSE);
    if (slot < 0)
        return FALSE;

    ListDecors[slot].Body = (edit->BaseBodyRaw & ~0xFFFF) | (edit->NewBody & 0xFFFF);
    if (edit->Deleted)
        ListDecors[slot].Body |= DEC_INVISIBLE;
    else
        ListDecors[slot].Body &= ~DEC_INVISIBLE;
    ListDecors[slot].CodeJeu = edit->BaseCodeJeu;
    ListDecors[slot].Beta = (edit->BaseBeta + edit->DBeta) & 0xFFFF;
    ListDecors[slot].Xworld = edit->BaseX + edit->Dx;
    ListDecors[slot].Yworld = edit->BaseY + edit->Dy;
    ListDecors[slot].Zworld = edit->BaseZ + edit->Dz;
    ExtDecorEditorApplyEditBounds(edit, &ListDecors[slot]);
    edit->Num = slot;
    return TRUE;
}

static void ExtDecorEditorSelectSlot(S32 num) {
    memset(ExtDecorEditorSelList, 0, sizeof(ExtDecorEditorSelList));
    ZoneEditorClearAllSelected();
    if (num < 0 OR num >= NbObjDecors OR !ListDecors)
        return;
    ExtDecorEditorSelList[0].Used = TRUE;
    ExtDecorEditorSelList[0].CubeX = ExtDecorEditorLoadedCubeX;
    ExtDecorEditorSelList[0].CubeY = ExtDecorEditorLoadedCubeY;
    ExtDecorEditorSelList[0].Num = num;
    ExtDecorEditorSelList[0].Body = ListDecors[num].Body & 0xFFFF;
}

static void ExtDecorEditorSelectAll(void) {
    S32 i;
    S32 selected = 0;

    memset(ExtDecorEditorSelList, 0, sizeof(ExtDecorEditorSelList));
    ZoneEditorClearAllSelected();
    ActorEditorClearSelection();
    WaypointEditorClearSelection();
    if (!ListDecors OR NbObjDecors <= 0) {
        Editor_SetStatus("No exterior decor objects in cube");
        return;
    }

    for (i = 0; i < NbObjDecors; i++) {
        T_DECORS *obj = &ListDecors[i];
        T_EXT_DECOR_EDITOR_SEL *sel;

        if (obj->Body & DEC_INVISIBLE)
            continue;
        if (selected >= EXT_DECOR_EDITOR_MAX_SELECTED)
            break;

        sel = &ExtDecorEditorSelList[selected++];
        sel->Used = TRUE;
        sel->CubeX = ExtDecorEditorLoadedCubeX;
        sel->CubeY = ExtDecorEditorLoadedCubeY;
        sel->Num = i;
        sel->Body = obj->Body & 0xFFFF;
    }

    if (selected > 0) {
        Editor_SetStatus("Selected all exterior decor objects in cube");
        Log_Info("[editor] selected %d exterior decor object(s) in c%d,%d",
                 (int)selected,
                 (int)ExtDecorEditorLoadedCubeX,
                 (int)ExtDecorEditorLoadedCubeY);
    } else {
        Editor_SetStatus("No exterior decor objects in cube");
    }
}

static void ExtDecorEditorResetSlotEdit(S32 num, T_DECORS *obj) {
    S32 i;
    T_EXT_DECOR_EDITOR_EDIT *edit;

    edit = ExtDecorEditorFindDeletedEdit(ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, num);
    if (!edit)
        edit = ExtDecorEditorFindAnyEdit(ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, num);

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_EDITS; i++) {
        T_EXT_DECOR_EDITOR_EDIT *other = &ExtDecorEditorEdits[i];
        if (other != edit AND other->Used AND other->CubeX == ExtDecorEditorLoadedCubeX AND other->CubeY == ExtDecorEditorLoadedCubeY AND other->Num == num)
            other->Used = FALSE;
    }

    if (!edit)
        edit = ExtDecorEditorGetOrCreateEdit(ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, num, obj);
    if (edit) {
        edit->Deleted = FALSE;
        edit->NewBody = obj->Body & 0xFFFF;
        edit->BaseBodyRaw = obj->Body;
        edit->BaseCodeJeu = obj->CodeJeu;
        edit->BaseBeta = obj->Beta;
        edit->BaseX = obj->Xworld;
        edit->BaseY = obj->Yworld;
        edit->BaseZ = obj->Zworld;
        edit->BaseXMin = obj->XMin;
        edit->BaseYMin = obj->YMin;
        edit->BaseZMin = obj->ZMin;
        edit->BaseXMax = obj->XMax;
        edit->BaseYMax = obj->YMax;
        edit->BaseZMax = obj->ZMax;
        edit->DBeta = 0;
        edit->Dx = 0;
        edit->Dy = 0;
        edit->Dz = 0;
    }
}

static void ExtDecorEditorSetSlotBody(T_DECORS *obj, S32 body) {
    obj->Body = body & 0xFFFF;
}

static void ExtDecorEditorPlaceObjectInDeletedSlot(T_DECORS *src) {
    S32 slot;

    if (!ListDecors) {
        Editor_SetStatus("Decor add refused: no exterior decor list loaded");
        Log_Warn("[editor] decor add refused: no exterior decor list loaded");
        return;
    }

    slot = ExtDecorEditorFirstDeletedSlot();
    if (slot < 0) {
        S32 x;
        S32 y;
        S32 z;

        if (src) {
            x = src->Xworld + 512;
            y = src->Yworld;
            z = src->Zworld + 512;
        } else {
            ExtDecorEditorUpdatePlacementPreview();
            if (!ExtDecorEditorPlaceValid) {
                Editor_SetStatus("Decor add refused: no placement point");
                Log_Warn("[editor] decor add refused: no placement point");
                return;
            }
            x = ExtDecorEditorPlaceX;
            y = ExtDecorEditorPlaceY;
            z = ExtDecorEditorPlaceZ;
        }

        slot = ExtDecorEditorAppendObject(src,
                                          src ? (src->Body & 0xFFFF) : ExtDecorEditorCurrentPlaceBody(),
                                          src ? src->Beta : ExtDecorEditorPlaceBeta,
                                          x, y, z, TRUE);
        if (slot < 0)
            return;
        Editor_SetStatus("Decor appended");
        Log_Info("[editor] decor appended in c%d,%d slot %d body %d",
                 (int)ExtDecorEditorLoadedCubeX,
                 (int)ExtDecorEditorLoadedCubeY,
                 (int)slot,
                 (int)(ListDecors[slot].Body & 0xFFFF));
        ExtDecorEditorMaybeCreateLinkedZones(slot, &ListDecors[slot]);
        return;
    }

    if (src) {
        ListDecors[slot] = *src;
        ListDecors[slot].Xworld += 512;
        ListDecors[slot].Zworld += 512;
        ListDecors[slot].XMin += 512;
        ListDecors[slot].XMax += 512;
        ListDecors[slot].ZMin += 512;
        ListDecors[slot].ZMax += 512;
        ListDecors[slot].Body &= 0xFFFF;
    } else {
        if (!ExtDecorEditorInitDefaultObject(&ListDecors[slot],
                                             ExtDecorEditorCurrentPlaceBody(),
                                             ExtDecorEditorPlaceBeta,
                                             ListDecors[slot].Xworld,
                                             ListDecors[slot].Yworld,
                                             ListDecors[slot].Zworld))
            return;
    }

    ExtDecorEditorResetSlotEdit(slot, &ListDecors[slot]);
    ExtDecorEditorSelectSlot(slot);
    Editor_SetStatus("Decor placed into deleted slot");
    Log_Info("[editor] decor placed in c%d,%d slot %d body %d",
             (int)ExtDecorEditorLoadedCubeX,
             (int)ExtDecorEditorLoadedCubeY,
             (int)slot,
             (int)(ListDecors[slot].Body & 0xFFFF));
    ExtDecorEditorMaybeCreateLinkedZones(slot, &ListDecors[slot]);
}

static void ExtDecorEditorPlaceObjectAt(T_DECORS *src, S32 body, S32 beta, S32 x, S32 y, S32 z) {
    S32 slot;

    if (!ListDecors AND NbObjDecors > 0) {
        Editor_SetStatus("Decor place refused: no exterior decor list loaded");
        Log_Warn("[editor] decor place refused: no exterior decor list loaded");
        return;
    }

    slot = ExtDecorEditorFirstDeletedSlot();
    if (slot < 0) {
        slot = ExtDecorEditorAppendObject(src, body, beta, x, y, z, TRUE);
        if (slot < 0)
            return;
        Log_Info("[editor] decor appended at mouse in c%d,%d slot %d body %d pos [%d,%d,%d]",
                 (int)ExtDecorEditorLoadedCubeX,
                 (int)ExtDecorEditorLoadedCubeY,
                 (int)slot,
                 (int)(ListDecors[slot].Body & 0xFFFF),
                 (int)x,
                 (int)y,
                 (int)z);
        ExtDecorEditorMaybeCreateLinkedZones(slot, &ListDecors[slot]);
        return;
    }

    if (src) {
        ListDecors[slot] = *src;
        ListDecors[slot].Body = (ListDecors[slot].Body & ~0xFFFF) | (body & 0xFFFF);
        ListDecors[slot].Body &= ~DEC_INVISIBLE;
        ListDecors[slot].Beta = beta;
        ExtDecorEditorMoveObjectTo(&ListDecors[slot], x, y, z);
    } else {
        if (!ExtDecorEditorInitDefaultObject(&ListDecors[slot], body, beta, x, y, z))
            return;
    }

    ExtDecorEditorResetSlotEdit(slot, &ListDecors[slot]);
    ExtDecorEditorSelectSlot(slot);
    Editor_SetStatus("Decor placed at mouse");
    Log_Info("[editor] decor placed at mouse in c%d,%d slot %d body %d pos [%d,%d,%d]",
             (int)ExtDecorEditorLoadedCubeX,
             (int)ExtDecorEditorLoadedCubeY,
             (int)slot,
             (int)(ListDecors[slot].Body & 0xFFFF),
             (int)x,
             (int)y,
             (int)z);
    ExtDecorEditorMaybeCreateLinkedZones(slot, &ListDecors[slot]);
}

void ExtDecorEditorDeleteSelected(void) {
    S32 i;
    S32 deleted = 0;

    if (!ListDecors)
        return;

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_SELECTED; i++) {
        T_EXT_DECOR_EDITOR_SEL *sel = &ExtDecorEditorSelList[i];
        T_EXT_DECOR_EDITOR_EDIT *edit;

        if (!sel->Used)
            continue;
        if (sel->CubeX != ExtDecorEditorLoadedCubeX OR sel->CubeY != ExtDecorEditorLoadedCubeY OR sel->Num < 0 OR sel->Num >= NbObjDecors)
            continue;
        if ((ListDecors[sel->Num].Body & DEC_INVISIBLE)
                OR((ListDecors[sel->Num].Body & 0xFFFF) != sel->Body)) {
            sel->Used = FALSE;
            continue;
        }

        edit = ExtDecorEditorFindEdit(sel->CubeX, sel->CubeY, sel->Num, sel->Body);
        if (!edit)
            edit = ExtDecorEditorFindAnyEdit(sel->CubeX, sel->CubeY, sel->Num);
        if (!edit)
            edit = ExtDecorEditorGetOrCreateEdit(sel->CubeX, sel->CubeY, sel->Num, &ListDecors[sel->Num]);
        if (edit)
            edit->Deleted = TRUE;
        ListDecors[sel->Num].Body |= DEC_INVISIBLE;
        sel->Used = FALSE;
        deleted++;
    }

    if (deleted) {
        Editor_SetStatus("Deleted selected exterior decor");
        Log_Info("[editor] deleted %d exterior decor object(s)", (int)deleted);
    } else {
        Editor_SetStatus("No selected exterior decor to delete");
    }
}

void ExtDecorEditorDuplicateSelected(void) {
    T_DECORS *src = ExtDecorEditorSelectedObject();

    if (!src) {
        Editor_SetStatus("Decor duplicate refused: no selected object");
        Log_Warn("[editor] decor duplicate refused: no selected object");
        return;
    }
    ExtDecorEditorPlaceObjectInDeletedSlot(src);
}

void ExtDecorEditorPlacePaletteInDeletedSlot(void) {
    ExtDecorEditorPlaceObjectInDeletedSlot(NULL);
}

static void ExtDecorEditorEnsurePaletteLibrary(void) {
    if (ExtDecorEditorPaletteLibrary < 0 OR ExtDecorEditorPaletteLibrary >= MAX_ISLAND)
        ExtDecorEditorPaletteLibrary = 0;
}

void ExtDecorEditorPaletteNext(S32 dir) {
    ExtDecorEditorPaletteBody += dir;
    if (ExtDecorEditorPaletteBody < 0)
        ExtDecorEditorPaletteBody = 0;
    if (ExtDecorEditorPaletteBody > 199)
        ExtDecorEditorPaletteBody = 199;
}

void ExtDecorEditorPaletteLibraryNext(S32 dir) {
    ExtDecorEditorEnsurePaletteLibrary();
    ExtDecorEditorPaletteLibrary += dir;
    if (ExtDecorEditorPaletteLibrary < 0)
        ExtDecorEditorPaletteLibrary = 0;
    if (ExtDecorEditorPaletteLibrary >= MAX_ISLAND)
        ExtDecorEditorPaletteLibrary = MAX_ISLAND - 1;
}

S32 ExtDecorEditorGetPaletteLibrary(void) {
    ExtDecorEditorEnsurePaletteLibrary();
    return ExtDecorEditorPaletteLibrary;
}

const char *ExtDecorEditorGetPaletteLibraryName(void) {
    ExtDecorEditorEnsurePaletteLibrary();
    if (ExtDecorEditorPaletteLibrary == 0)
        return "Current Island";
    return IleLst[ExtDecorEditorPaletteLibrary];
}

S32 ExtDecorEditorGetPaletteBody(void) {
    return ExtDecorEditorPaletteBody;
}

void ExtDecorEditorPaletteDraw(S32 x0, S32 y0) {
    S32 i;
    S32 seen[8];
    S32 seen_count = 0;

    if (x0 < 8)
        x0 = 8;

    for (i = 0; i < NbObjDecors && seen_count < 8; i++) {
        S32 body = ListDecors[i].Body & 0xFFFF;
        S32 n;
        S32 found = FALSE;

        for (n = 0; n < seen_count; n++) {
            if (seen[n] == body)
                found = TRUE;
        }
        if (!found)
            seen[seen_count++] = body;
    }

    Box(x0, y0, x0 + 178, y0 + 114, 0);
    Rect(x0, y0, x0 + 178, y0 + 114, 15);
    CoulText(15, -1);
    GraphPrintf(FALSE, x0 + 6, y0 + 6, "Decor palette");
    GraphPrintf(FALSE, x0 + 6, y0 + 22, "Body %d  [ / ]", (int)ExtDecorEditorPaletteBody);
    GraphPrintf(FALSE, x0 + 6, y0 + 38, "%s  Shift+[ / ]", ExtDecorEditorGetPaletteLibraryName());
    GraphPrintf(FALSE, x0 + 6, y0 + 54, "P fills deleted slot");
    GraphPrintf(FALSE, x0 + 6, y0 + 70, "Delete frees a slot");
    GraphPrintf(FALSE, x0 + 6, y0 + 84, "Seen:");
    for (i = 0; i < seen_count && i < 4; i++)
        GraphPrintf(FALSE, x0 + 42 + i * 32, y0 + 84, "%d", (int)seen[i]);
    for (i = 4; i < seen_count; i++)
        GraphPrintf(FALSE, x0 + 42 + (i - 4) * 32, y0 + 98, "%d", (int)seen[i]);
}

S32 ExtDecorEditorSaveRuntimeEdits(const char *island_dir, S32 island) {
    char path[ADELINE_MAX_PATH];
    FILE *f;
    S32 i;
    S32 first = TRUE;
    S32 count = 0;

    if (!island_dir OR !island_dir[0])
        return FALSE;

    snprintf(path, sizeof(path), "%s%sdecor_edits.json", island_dir, ADELINE_PATH_SEP);
    path[sizeof(path) - 1] = '\0';

    f = fopen(path, "wb");
    if (!f) {
        Editor_SetStatus("Decor save failed: cannot open decor_edits.json");
        Log_Warn("[editor] cannot write decor edits: %s", path);
        return FALSE;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"island\": %d,\n", (int)island);
    fprintf(f, "  \"description\": \"Authoring edits exported by the exterior decor editor.\",\n");
    fprintf(f, "  \"cubes\": [\n");

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_EDITS; i++) {
        T_EXT_DECOR_EDITOR_EDIT *edit = &ExtDecorEditorEdits[i];
        S32 x;
        S32 y;
        S32 z;
        S32 xmin;
        S32 ymin;
        S32 zmin;
        S32 xmax;
        S32 ymax;
        S32 zmax;
        S32 body_raw;

        if (!edit->Used)
            continue;

        if (!first)
            fprintf(f, ",\n");
        first = FALSE;
        count++;

        x = edit->BaseX + edit->Dx;
        y = edit->BaseY + edit->Dy;
        z = edit->BaseZ + edit->Dz;
        xmin = edit->BaseXMin + edit->Dx;
        ymin = edit->BaseYMin + edit->Dy;
        zmin = edit->BaseZMin + edit->Dz;
        xmax = edit->BaseXMax + edit->Dx;
        ymax = edit->BaseYMax + edit->Dy;
        zmax = edit->BaseZMax + edit->Dz;
        body_raw = (edit->BaseBodyRaw & ~0xFFFF) | (edit->NewBody & 0xFFFF);

        fprintf(f, "    {\n");
        fprintf(f, "      \"cube\": [%d, %d],\n", (int)edit->CubeX, (int)edit->CubeY);
        fprintf(f, "      \"objects\": [\n");
        fprintf(f, "        {\n");
        fprintf(f, "          \"object\": %d,\n", (int)edit->Num);
        if (edit->HasExpectBody)
            fprintf(f, "          \"expect_body\": %d,\n", (int)edit->Body);
        fprintf(f, "          \"body\": %d,\n", (int)(edit->NewBody & 0xFFFF));
        fprintf(f, "          \"body_raw\": %d,\n", (int)body_raw);
        fprintf(f, "          \"code_jeu\": %d,\n", (int)edit->BaseCodeJeu);
        fprintf(f, "          \"beta_raw\": %d,\n", (int)edit->BaseBeta);
        fprintf(f, "          \"dbeta\": %d,\n", (int)edit->DBeta);
        fprintf(f, "          \"deleted\": %s,\n", edit->Deleted ? "true" : "false");
        fprintf(f, "          \"position\": [%d, %d, %d],\n", (int)x, (int)y, (int)z);
        fprintf(f, "          \"zv\": [%d, %d, %d, %d, %d, %d]\n",
                (int)xmin, (int)ymin, (int)zmin, (int)xmax, (int)ymax, (int)zmax);
        fprintf(f, "        }\n");
        fprintf(f, "      ]\n");
        fprintf(f, "    }");
    }

    fprintf(f, "\n");
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);

    Log_Info("[editor] saved %d decor edit(s) to %s", (int)count, path);
    return TRUE;
}

static void ExtDecorEditorMoveAllSelected(S32 dx, S32 dy, S32 dz) {
    S32 i;

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_SELECTED; i++) {
        T_EXT_DECOR_EDITOR_SEL *sel = &ExtDecorEditorSelList[i];
        T_EXT_DECOR_EDITOR_EDIT *edit;
        T_DECORS *ptrobj;

        if (!sel->Used)
            continue;

        if (sel->Num >= 0 AND sel->Num < NbObjDecors AND ExtDecorEditorObjectDeleted(sel->CubeX, sel->CubeY, sel->Num))
            continue;

        edit = ExtDecorEditorFindEdit(sel->CubeX, sel->CubeY, sel->Num, sel->Body);
        if (!edit)
            edit = ExtDecorEditorFindAnyEdit(sel->CubeX, sel->CubeY, sel->Num);
        if (sel->CubeX == ExtDecorEditorLoadedCubeX AND sel->CubeY == ExtDecorEditorLoadedCubeY AND sel->Num >= 0 AND sel->Num < NbObjDecors) {
            ptrobj = &ListDecors[sel->Num];
            if ((ptrobj->Body & 0xFFFF) == sel->Body)
                edit = ExtDecorEditorGetOrCreateEdit(sel->CubeX, sel->CubeY, sel->Num, ptrobj);
        }
        if (!edit)
            continue;

        edit->Dx += dx;
        edit->Dy += dy;
        edit->Dz += dz;

        if (sel->CubeX == ExtDecorEditorLoadedCubeX AND sel->CubeY == ExtDecorEditorLoadedCubeY AND sel->Num >= 0 AND sel->Num < NbObjDecors) {
            ptrobj = &ListDecors[sel->Num];
            if ((ptrobj->Body & 0xFFFF) == sel->Body)
                ExtDecorEditorApplyEditToObject(edit, ptrobj);
        }
    }
}

static void ExtDecorEditorRotateAllSelected(S32 dbeta) {
    S32 i;
    S32 rotated = 0;

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_SELECTED; i++) {
        T_EXT_DECOR_EDITOR_SEL *sel = &ExtDecorEditorSelList[i];
        T_EXT_DECOR_EDITOR_EDIT *edit;
        T_DECORS *ptrobj;

        if (!sel->Used)
            continue;

        if (sel->Num >= 0 AND sel->Num < NbObjDecors AND ExtDecorEditorObjectDeleted(sel->CubeX, sel->CubeY, sel->Num))
            continue;

        edit = ExtDecorEditorFindEdit(sel->CubeX, sel->CubeY, sel->Num, sel->Body);
        if (!edit)
            edit = ExtDecorEditorFindAnyEdit(sel->CubeX, sel->CubeY, sel->Num);
        if (sel->CubeX == ExtDecorEditorLoadedCubeX AND sel->CubeY == ExtDecorEditorLoadedCubeY AND sel->Num >= 0 AND sel->Num < NbObjDecors) {
            ptrobj = &ListDecors[sel->Num];
            if ((ptrobj->Body & 0xFFFF) == sel->Body)
                edit = ExtDecorEditorGetOrCreateEdit(sel->CubeX, sel->CubeY, sel->Num, ptrobj);
        }
        if (!edit)
            continue;

        edit->DBeta = (edit->DBeta + dbeta) & 4095;

        if (sel->CubeX == ExtDecorEditorLoadedCubeX AND sel->CubeY == ExtDecorEditorLoadedCubeY AND sel->Num >= 0 AND sel->Num < NbObjDecors) {
            ptrobj = &ListDecors[sel->Num];
            if ((ptrobj->Body & 0xFFFF) == sel->Body)
                ExtDecorEditorApplyEditToObject(edit, ptrobj);
        }
        rotated++;
    }

    if (rotated) {
        Editor_SetStatus("Rotated selected exterior decor");
        Log_Info("[editor] rotated %d exterior decor object(s) by %d beta units", (int)rotated, (int)dbeta);
    }
}

static void ExtDecorEditorConsiderCurrentCubePick(void) {
    S32 i;
    T_LIST_TRI *ptrtri;
    T_DECORS *ptrobj;

    if (!ExtDecorEditorPickPending)
        return;

    ptrtri = ListTriExt;
    ExtDecorEditorPickPasses++;
    for (i = 0; i < NbObjScreen; i++, ptrtri++) {
        S32 xs[10];
        S32 ys[10];
        S32 nb;
        S32 n;

        if (ExtDecorOverlayMaxDist > 0 AND ptrtri->Zrot > ExtDecorOverlayMaxDist)
            continue;

        ptrobj = ptrtri->PtrDec;
        if (!(ptrobj->Body & DEC_DRAWN))
            continue;
        ExtDecorEditorPickObjects++;

        nb = 0;
        if (ExtDecorOverlayProject(ptrobj->Xworld, ptrobj->Yworld, ptrobj->Zworld, &xs[nb], &ys[nb])) {
            nb++;
            xs[nb] = xs[0];
            ys[nb] = ys[0] - 12;
            nb++;
        }
        if (ExtDecorOverlayProject((ptrobj->XMin + ptrobj->XMax) / 2,
                                   (ptrobj->YMin + ptrobj->YMax) / 2,
                                   (ptrobj->ZMin + ptrobj->ZMax) / 2,
                                   &xs[nb], &ys[nb]))
            nb++;
        if (ExtDecorOverlayProject(ptrobj->XMin, ptrobj->YMin, ptrobj->ZMin, &xs[nb], &ys[nb]))
            nb++;
        if (ExtDecorOverlayProject(ptrobj->XMax, ptrobj->YMin, ptrobj->ZMin, &xs[nb], &ys[nb]))
            nb++;
        if (ExtDecorOverlayProject(ptrobj->XMax, ptrobj->YMin, ptrobj->ZMax, &xs[nb], &ys[nb]))
            nb++;
        if (ExtDecorOverlayProject(ptrobj->XMin, ptrobj->YMin, ptrobj->ZMax, &xs[nb], &ys[nb]))
            nb++;
        if (ExtDecorOverlayProject(ptrobj->XMin, ptrobj->YMax, ptrobj->ZMin, &xs[nb], &ys[nb]))
            nb++;
        if (ExtDecorOverlayProject(ptrobj->XMax, ptrobj->YMax, ptrobj->ZMax, &xs[nb], &ys[nb]))
            nb++;

        for (n = 0; n < nb; n++) {
            S32 dx;
            S32 dy;
            S32 dist;

            ExtDecorEditorPickPoints++;
            dx = xs[n] - (S32)MouseX;
            dy = ys[n] - (S32)MouseY;
            dist = dx * dx + dy * dy;
            if (dist <= ExtDecorEditorPickBestDist) {
                ExtDecorEditorPickBestDist = dist;
                ExtDecorEditorPickBestCubeX = ExtDecorEditorLoadedCubeX;
                ExtDecorEditorPickBestCubeY = ExtDecorEditorLoadedCubeY;
                ExtDecorEditorPickBestNum = ptrtri->Num;
                ExtDecorEditorPickBestBody = ptrtri->PtrDec->Body & 0xFFFF;
            }
        }
    }
}

static void ExtDecorEditorUpdatePlacementPreview(void) {
    S32 xi;
    S32 zi;
    S32 best_dist = 0x7FFFFFFF;
    S32 found = FALSE;

    ExtDecorEditorPlaceValid = FALSE;
    if (!ExtDecorEditorLoadedIsCurrentCube())
        return;
    if (!MapSommetY)
        return;

    for (zi = 0; zi <= NB_COTE; zi++) {
        for (xi = 0; xi <= NB_COTE; xi++) {
            S32 index = zi * (NB_COTE + 1) + xi;
            S32 x = xi * 512;
            S32 y = MapSommetY[index];
            S32 z = zi * 512;
            S32 dx;
            S32 dy;
            S32 dist;

            LongWorldRotatePoint(x, y, z);
            if (!LongProjectPoint(X0, Y0, Z0))
                continue;
            if (Xp < 0 OR Xp >= (S32)ModeDesiredX OR Yp < 0 OR Yp >= (S32)ModeDesiredY)
                continue;

            dx = Xp - (S32)MouseX;
            dy = Yp - (S32)MouseY;
            dist = dx * dx + dy * dy;
            if (!found OR dist < best_dist) {
                found = TRUE;
                best_dist = dist;
                ExtDecorEditorPlaceX = x;
                ExtDecorEditorPlaceY = y;
                ExtDecorEditorPlaceZ = z;
            }
        }
    }

    ExtDecorEditorPlaceValid = found;
}

S32 ExtDecorEditorGetPlacementPoint(S32 *x, S32 *y, S32 *z) {
    if (!ExtDecorEditorPlaceValid)
        return FALSE;
    *x = ExtDecorEditorPlaceX;
    *y = ExtDecorEditorPlaceY;
    *z = ExtDecorEditorPlaceZ;
    return TRUE;
}

static void ExtDecorEditorDrawPlacementPreview(void) {
    S32 xs;
    S32 ys;
    S32 body;
    S32 library;
    S32 body_index;
    T_HQR_HEADER *body_hqr;
    void *body_ptr;
    U8 *old_obj_ptr_map;

    if (!ExtDecorEditorSelect OR !ExtDecorEditorPlaceValid)
        return;

    body = ExtDecorEditorCurrentPlaceBody();
    ExtDecorEditorDecodeBody(body, &library, &body_index);
    body_hqr = body_index >= 0 ? GetDecorLibraryHQR(library) : NULL;
    body_ptr = body_hqr ? HQR_Get(body_hqr, body_index) : NULL;
    if (body_ptr) {
        old_obj_ptr_map = ObjPtrMap;
        ObjPtrMap = (library > 0 AND library != Island) ? ObjTextures[library] : ObjTexture;
        UnsetClip();
        SetCLUT(PalLevel);
        BodyDisplay(ExtDecorEditorPlaceX,
                    ExtDecorEditorPlaceY,
                    ExtDecorEditorPlaceZ,
                    0,
                    ExtDecorEditorPlaceBeta,
                    0,
                    body_ptr);
        ObjPtrMap = old_obj_ptr_map;
    }

    if (!ExtDecorOverlayProject(ExtDecorEditorPlaceX, ExtDecorEditorPlaceY, ExtDecorEditorPlaceZ, &xs, &ys))
        return;
    Line(xs - 8, ys, xs - 2, ys, 12);
    Line(xs + 2, ys, xs + 8, ys, 12);
    Line(xs, ys - 8, xs, ys - 2, 12);
    Line(xs, ys + 2, xs, ys + 8, 12);
    Rect(xs - 3, ys - 3, xs + 3, ys + 3, 14);
    CoulText(12, 0);
    //GraphPrintf(FALSE, xs + 8, ys - 8, "place b%d", (int)body);
}

static void ExtDecorEditorPlaceAtPreview(void) {
    T_DECORS *src;
    S32 body;

    if (!ExtDecorEditorLoadedIsCurrentCube()) {
        Editor_SetStatus("Decor place refused: current cube is not loaded");
        Log_Warn("[editor] decor place refused: loaded cube c%d,%d, current cube c%d,%d",
                 (int)ExtDecorEditorLoadedCubeX,
                 (int)ExtDecorEditorLoadedCubeY,
                 (int)CurrentCubeX,
                 (int)CurrentCubeY);
        return;
    }

    if (!ExtDecorEditorPlaceValid) {
        Editor_SetStatus("Decor place refused: no placement point");
        Log_Warn("[editor] decor place refused: no placement point");
        return;
    }

    src = Editor_ToolIs(EDITOR_TOOL_DECOR_PLACE) ? NULL : ExtDecorEditorAnySelectedObject();
    body = ExtDecorEditorCurrentPlaceBody();
    ExtDecorEditorPlaceObjectAt(src, body, ExtDecorEditorPlaceBeta, ExtDecorEditorPlaceX, ExtDecorEditorPlaceY, ExtDecorEditorPlaceZ);
}

void ExtDecorEditorBeginFrame(void) {
    S32 click;
    S32 up;
    S32 down;
    S32 left;
    S32 right;
    S32 step;
    S32 del;
    S32 duplicate;
    S32 insert;
    S32 select_all;
    S32 pal_prev;
    S32 pal_next;
    S32 page_up;
    S32 page_down;
    S32 log;
    S32 shift;
    S32 alt;
    S32 camera_pan_keys;

    if (!ExtDecorEditorSelect)
        return;
    if (Editor_IsCameraPanActive()) {
        ExtDecorEditorLastClick = Click;
        ExtDecorEditorLastUp = CheckKey(K_UP) ? TRUE : FALSE;
        ExtDecorEditorLastDown = CheckKey(K_DOWN) ? TRUE : FALSE;
        ExtDecorEditorLastLeft = CheckKey(K_LEFT) ? TRUE : FALSE;
        ExtDecorEditorLastRight = CheckKey(K_RIGHT) ? TRUE : FALSE;
        ExtDecorEditorLastDelete = (CheckKey(K_SUPPR) OR CheckKey(K_BACKSPACE)) ? TRUE : FALSE;
        ExtDecorEditorLastDuplicate = (CheckKey(K_D) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
        ExtDecorEditorLastSelectAll = (CheckKey(K_A) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
        ExtDecorEditorLastInsert = (CheckKey(K_P) OR CheckKey(K_INSER)) ? TRUE : FALSE;
        ExtDecorEditorLastPalettePrev = CheckKey(K_LBRACKET) ? TRUE : FALSE;
        ExtDecorEditorLastPaletteNext = CheckKey(K_RBRACKET) ? TRUE : FALSE;
        ExtDecorEditorLastPageUp = CheckKey(K_PAGE_UP) ? TRUE : FALSE;
        ExtDecorEditorLastPageDown = CheckKey(K_PAGE_DOWN) ? TRUE : FALSE;
        ExtDecorEditorLastLog = CheckKey(K_L) ? TRUE : FALSE;
        return;
    }

    click = Click;
    ExtDecorEditorPickPending = FALSE;
    if (Editor_WorldPlacementActive()) {
        ExtDecorEditorUpdatePlacementPreview();
        if ((click & 1) AND !(ExtDecorEditorLastClick & 1) AND ExtDecorEditorPlaceValid)
            Editor_PlaceAtWorld(ExtDecorEditorPlaceX, ExtDecorEditorPlaceY, ExtDecorEditorPlaceZ);
    } else if (Editor_ToolIs(EDITOR_TOOL_DECOR_PLACE)) {
        ExtDecorEditorUpdatePlacementPreview();
        if ((click & 1) AND !(ExtDecorEditorLastClick & 1))
            ExtDecorEditorPlaceAtPreview();
    } else if (Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE) AND !Editor_CubeAllocateModeActive() AND(click & 1) AND !(ExtDecorEditorLastClick & 1)) {
        ExtDecorEditorPickPending = TRUE;
        ExtDecorEditorPickShift = (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? TRUE : FALSE;
        ExtDecorEditorPickBestDist = EXT_DECOR_EDITOR_PICK_RADIUS * EXT_DECOR_EDITOR_PICK_RADIUS;
        ExtDecorEditorPickBestNum = -1;
        ExtDecorEditorPickBestBody = -1;
        ExtDecorEditorPickPasses = 0;
        ExtDecorEditorPickObjects = 0;
        ExtDecorEditorPickPoints = 0;
        ZoneEditorBeginPick();
    }
    ExtDecorEditorLastClick = click;

    if (Editor_InspectorConsumesInput()) {
        ExtDecorEditorLastUp = CheckKey(K_UP) ? TRUE : FALSE;
        ExtDecorEditorLastDown = CheckKey(K_DOWN) ? TRUE : FALSE;
        ExtDecorEditorLastLeft = CheckKey(K_LEFT) ? TRUE : FALSE;
        ExtDecorEditorLastRight = CheckKey(K_RIGHT) ? TRUE : FALSE;
        ExtDecorEditorLastDelete = (CheckKey(K_SUPPR) OR CheckKey(K_BACKSPACE)) ? TRUE : FALSE;
        ExtDecorEditorLastDuplicate = (CheckKey(K_D) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
        ExtDecorEditorLastSelectAll = (CheckKey(K_A) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
        ExtDecorEditorLastInsert = (CheckKey(K_P) OR CheckKey(K_INSER)) ? TRUE : FALSE;
        ExtDecorEditorLastPalettePrev = CheckKey(K_LBRACKET) ? TRUE : FALSE;
        ExtDecorEditorLastPaletteNext = CheckKey(K_RBRACKET) ? TRUE : FALSE;
        ExtDecorEditorLastPageUp = CheckKey(K_PAGE_UP) ? TRUE : FALSE;
        ExtDecorEditorLastPageDown = CheckKey(K_PAGE_DOWN) ? TRUE : FALSE;
        ExtDecorEditorLastLog = CheckKey(K_L) ? TRUE : FALSE;
        return;
    }

    up = CheckKey(K_UP) ? TRUE : FALSE;
    down = CheckKey(K_DOWN) ? TRUE : FALSE;
    left = CheckKey(K_LEFT) ? TRUE : FALSE;
    right = CheckKey(K_RIGHT) ? TRUE : FALSE;
    shift = (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? TRUE : FALSE;
    alt = (CheckKey(K_ALT) OR CheckKey(K_ALT_LEFT) OR CheckKey(K_ALT_RIGHT)) ? TRUE : FALSE;
    camera_pan_keys = shift AND alt;
    if (camera_pan_keys)
        step = 0;
    else if (alt)
        step = EXT_DECOR_EDITOR_NUDGE_ALT;
    else if (shift)
        step = EXT_DECOR_EDITOR_NUDGE_SHIFT;
    else
        step = EXT_DECOR_EDITOR_NUDGE;

    if (!camera_pan_keys AND Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE) AND up AND !ExtDecorEditorLastUp)
        ExtDecorEditorMoveAllSelected(0, 0, -step);
    if (!camera_pan_keys AND Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE) AND down AND !ExtDecorEditorLastDown)
        ExtDecorEditorMoveAllSelected(0, 0, step);
    if (!camera_pan_keys AND Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE) AND left AND !ExtDecorEditorLastLeft)
        ExtDecorEditorMoveAllSelected(-step, 0, 0);
    if (!camera_pan_keys AND Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE) AND right AND !ExtDecorEditorLastRight)
        ExtDecorEditorMoveAllSelected(step, 0, 0);
    if (!camera_pan_keys AND Editor_ToolIs(EDITOR_TOOL_DECOR_PLACE) AND left AND !ExtDecorEditorLastLeft)
        ExtDecorEditorPlaceBeta = (ExtDecorEditorPlaceBeta - EXT_DECOR_EDITOR_ROTATE_STEP) & 4095;
    if (!camera_pan_keys AND Editor_ToolIs(EDITOR_TOOL_DECOR_PLACE) AND right AND !ExtDecorEditorLastRight)
        ExtDecorEditorPlaceBeta = (ExtDecorEditorPlaceBeta + EXT_DECOR_EDITOR_ROTATE_STEP) & 4095;
    ExtDecorEditorLastUp = up;
    ExtDecorEditorLastDown = down;
    ExtDecorEditorLastLeft = left;
    ExtDecorEditorLastRight = right;

    page_up = CheckKey(K_PAGE_UP) ? TRUE : FALSE;
    page_down = CheckKey(K_PAGE_DOWN) ? TRUE : FALSE;
    if (!camera_pan_keys AND Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE) AND page_up AND !ExtDecorEditorLastPageUp)
        ExtDecorEditorMoveAllSelected(0, step, 0);
    if (!camera_pan_keys AND Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE) AND page_down AND !ExtDecorEditorLastPageDown)
        ExtDecorEditorMoveAllSelected(0, -step, 0);
    ExtDecorEditorLastPageUp = page_up;
    ExtDecorEditorLastPageDown = page_down;

    del = (CheckKey(K_SUPPR) OR CheckKey(K_BACKSPACE)) ? TRUE : FALSE;
    duplicate = (CheckKey(K_D) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
    select_all = (CheckKey(K_A) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
    insert = (CheckKey(K_P) OR CheckKey(K_INSER)) ? TRUE : FALSE;
    pal_prev = CheckKey(K_LBRACKET) ? TRUE : FALSE;
    pal_next = CheckKey(K_RBRACKET) ? TRUE : FALSE;
    log = CheckKey(K_L) ? TRUE : FALSE;

    if (Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE) AND del AND !ExtDecorEditorLastDelete)
        ExtDecorEditorDeleteSelected();
    if (Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE) AND duplicate AND !ExtDecorEditorLastDuplicate)
        ExtDecorEditorDuplicateSelected();
    if (Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE) AND select_all AND !ExtDecorEditorLastSelectAll)
        ExtDecorEditorSelectAll();
    if (insert AND !ExtDecorEditorLastInsert) {
        if (Editor_ToolIs(EDITOR_TOOL_DECOR_PLACE))
            ExtDecorEditorPlaceAtPreview();
        else if (Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE))
            ExtDecorEditorPlacePaletteInDeletedSlot();
    }
    if (Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE) AND pal_prev AND !ExtDecorEditorLastPalettePrev)
        ExtDecorEditorRotateAllSelected(-EXT_DECOR_EDITOR_ROTATE_STEP);
    if (Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE) AND pal_next AND !ExtDecorEditorLastPaletteNext)
        ExtDecorEditorRotateAllSelected(EXT_DECOR_EDITOR_ROTATE_STEP);
    if (Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE) AND log AND !ExtDecorEditorLastLog) {
        S32 zones = ZoneEditorLogSelected();
        S32 decors = ExtDecorEditorLogSelected();

        if (zones OR decors)
            Editor_SetStatus("Logged selected zones and decor objects");
        else
            Editor_SetStatus("No selected zones or decor objects to log");
    }
    if (Editor_ToolIs(EDITOR_TOOL_DECOR_PLACE) AND pal_prev AND !ExtDecorEditorLastPalettePrev) {
        if (shift)
            ExtDecorEditorPaletteLibraryNext(-1);
        else
            ExtDecorEditorPaletteNext(-1);
    }
    if (Editor_ToolIs(EDITOR_TOOL_DECOR_PLACE) AND pal_next AND !ExtDecorEditorLastPaletteNext) {
        if (shift)
            ExtDecorEditorPaletteLibraryNext(1);
        else
            ExtDecorEditorPaletteNext(1);
    }

    ExtDecorEditorLastDelete = del;
    ExtDecorEditorLastDuplicate = duplicate;
    ExtDecorEditorLastInsert = insert;
    ExtDecorEditorLastSelectAll = select_all;
    ExtDecorEditorLastPalettePrev = pal_prev;
    ExtDecorEditorLastPaletteNext = pal_next;
    ExtDecorEditorLastLog = log;
}

void ExtDecorEditorEndFrame(void) {
}

void ExtDecorOverlayDraw(void) {
    S32 i;
    T_LIST_TRI *ptrtri;
    T_DECORS *ptrobj;

    if (!ExtDecorOverlayIds AND !ExtDecorOverlayBoxes AND !ExtDecorEditorSelect)
        return;

    UnsetClip();

    if (Editor_WorldPlacementActive()) {
        ExtDecorEditorUpdatePlacementPreview();
    } else if (Editor_ToolIs(EDITOR_TOOL_DECOR_PLACE)) {
        ExtDecorEditorUpdatePlacementPreview();
    } else if (Editor_ToolIs(EDITOR_TOOL_SELECT_MOVE)) {
        ExtDecorEditorConsiderCurrentCubePick();
    }

    ptrtri = ListTriExt;
    for (i = 0; i < NbObjScreen; i++, ptrtri++) {
        S32 xs, ys;
        S32 selected;

        if (ExtDecorOverlayMaxDist > 0 AND ptrtri->Zrot > ExtDecorOverlayMaxDist)
            continue;

        ptrobj = ptrtri->PtrDec;
        if (!(ptrobj->Body & DEC_DRAWN))
            continue;

        selected = ExtDecorEditorIsSelectedObject(ptrobj, ptrtri->Num);

        if (ExtDecorOverlayBoxes) {
            if (selected)
                ExtDecorOverlayDrawBoxColor(ptrobj, EXT_DECOR_EDITOR_BOX, EXT_DECOR_EDITOR_BOX_AXIS);
            else
                ExtDecorOverlayDrawBox(ptrobj);
        }

        if (ExtDecorOverlayIds) {
            if (ExtDecorOverlayProject(ptrobj->Xworld, ptrobj->Yworld, ptrobj->Zworld, &xs, &ys)) {
                S32 label_y = ys - 12;
                if (label_y < 0)
                    label_y = ys;
                if (selected) {
                    CoulText(EXT_DECOR_EDITOR_TEXT, EXT_DECOR_EDITOR_BACK);
                    GraphPrintf(FALSE, xs, label_y, "c%d,%d #%d b%d",
                                ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, ptrtri->Num, ptrobj->Body & 0xFFFF);
                } else {
                    CoulText(EXT_DECOR_OVERLAY_TEXT, EXT_DECOR_OVERLAY_BACK);
                    GraphPrintf(FALSE, xs, label_y, "c%d,%d #%d b%d",
                                ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, ptrtri->Num, ptrobj->Body & 0xFFFF);
                }
            }
        }
    }

    if (Editor_ToolIs(EDITOR_TOOL_DECOR_PLACE))
        ExtDecorEditorDrawPlacementPreview();

    UnsetClip();
}
#endif

/*══════════════════════════════════════════════════════════════════════════*/
void FixeObjetsDecorsInvisibles(void) {
    S32 i;
    T_DECORS *ptrobj;
    S32 numvar;

    ptrobj = ListDecors;

    for (i = 0; i < NbObjDecors; i++, ptrobj++) {
        ptrobj->Body &= ~(DEC_DRAWN | DEC_INVISIBLE);

        // Regarde si objet visible/invisible
        numvar = ptrobj->Beta >> 16;

        if (numvar) {
#ifdef LBA_EDITOR
            if (((numvar < 0) AND !ListVarGame[-numvar].Value)
                    OR((numvar >= 0) AND ListVarGame[numvar].Value))
#else
            if (((numvar < 0) AND !ListVarGame[-numvar])
                    OR((numvar >= 0) AND ListVarGame[numvar]))
#endif
                ptrobj->Body |= DEC_INVISIBLE;
        }
    }
}

/*══════════════════════════════════════════════════════════════════════════*/
// sous routine de tri temporaire

int sub_qsort_decors(const void *a, const void *b) {
    return (((T_LIST_TRI *)a)->Zrot - ((T_LIST_TRI *)b)->Zrot);
}

/*══════════════════════════════════════════════════════════════════════════*/
// sous routine de tri temporaire

int sub_qsort_decors_zbuf(const void *a, const void *b) {
    return (((T_LIST_TRI *)b)->Zrot - ((T_LIST_TRI *)a)->Zrot);
}

/*══════════════════════════════════════════════════════════════════════════*/

// Version ZVs absolues
int sub_qsort_zv_decors(const void *a, const void *b) {
    T_DECORS *ptrobja;
    T_DECORS *ptrobjb;

    S32 x0a, y0a, z0a, x1a, y1a, z1a;
    S32 x0b, y0b, z0b, x1b, y1b, z1b;

    ptrobja = ((T_LIST_TRI *)a)->PtrDec;
    ptrobjb = ((T_LIST_TRI *)b)->PtrDec;

    x0a = ptrobja->XMin;
    y0a = ptrobja->YMin;
    z0a = ptrobja->ZMin;
    x1a = ptrobja->XMax;
    y1a = ptrobja->YMax;
    z1a = ptrobja->ZMax;

    x0b = ptrobjb->XMin;
    y0b = ptrobjb->YMin;
    z0b = ptrobjb->ZMin;
    x1b = ptrobjb->XMax;
    y1b = ptrobjb->YMax;
    z1b = ptrobjb->ZMax;

    if (x0a >= x1b)
        goto pas_rec_sur_x;
    if (x1a <= x0b)
        goto pas_rec_sur_x;

    if (z0a >= z1b)
        goto pas_rec_sur_z;
    if (z1a <= z0b)
        goto pas_rec_sur_z;

    //	ordre sur y

    // affinement decision sur une des 2 faces paralelle
    if (abs(y0a - CameraY) < abs(y1a - CameraY))
        return (abs(y0b - CameraY) - abs(y0a - CameraY));
    else
        return (abs(y1b - CameraY) - abs(y1a - CameraY));

pas_rec_sur_z:

    //	ordre sur z

    if (abs(z0a - CameraZ) < abs(z1a - CameraZ))
        return (abs(z0b - CameraZ) - abs(z0a - CameraZ));
    else
        return (abs(z1b - CameraZ) - abs(z1a - CameraZ));

pas_rec_sur_x:

    if (z0a >= z1b)
        goto pas_rec_sur_z_2;
    if (z1a <= z0b)
        goto pas_rec_sur_z_2;

    if (abs(x0a - CameraZ) < abs(x1a - CameraZ))
        return (abs(x0b - CameraX) - abs(x0a - CameraX));
    else
        return (abs(x1b - CameraX) - abs(x1a - CameraX));

pas_rec_sur_z_2:

    // plus de methode dispo (obj en collision), utilise zrot

    return (((T_LIST_TRI *)b)->Zrot - ((T_LIST_TRI *)a)->Zrot);
}

/*══════════════════════════════════════════════════════════════════════════*/

#ifdef LBA_EDITOR
void AffichageObjetDecorsZV(void) {
    S32 i;
    T_LIST_TRI *ptrtri;
    T_DECORS *ptrobj;

    // affiche les zv des objets statiques
    ptrtri = ListTriExt;
    for (i = 0; i < NbObjScreen; i++, ptrtri++) {
        ptrobj = ptrtri->PtrDec;

        if (ptrobj->Body & DEC_DRAWN) {
            UnsetClip();
            NumDecorsDraw = ptrtri->Num;
            AffZVObjet(ptrobj);
        }
    }

    UnsetClip();
}
#endif

/*══════════════════════════════════════════════════════════════════════════*/

// Version masques compris

void AffichageObjetDecorsZBuf(void) {
    S32 i;
    T_LIST_TRI *ptrtri;
    T_DECORS *ptrobj;
    S32 clipzstart;
    S32 numvar;
    S32 zr;

    ObjPtrMap = ObjTexture; // texture obj

    ptrtri = ListTriExt;
    ptrobj = ListDecors;

    // projection des centres
    NbObjScreen = 0;
    for (i = 0; i < NbObjDecors; i++, ptrobj++) {
        ptrobj->Body &= ~(DEC_DRAWN | DEC_INVISIBLE);

        // Regarde si objet visible/invisible
        numvar = ptrobj->Beta >> 16;

        if (numvar) {
#ifdef LBA_EDITOR
            if (((numvar < 0) AND !ListVarGame[-numvar].Value)
                    OR((numvar >= 0) AND ListVarGame[numvar].Value))
#else
            if (((numvar < 0) AND !ListVarGame[-numvar])
                    OR((numvar >= 0) AND ListVarGame[numvar]))
#endif
                ptrobj->Body |= DEC_INVISIBLE;
            //			else	ptrobj->Body &= ~(DEC_INVISIBLE) ;
        }
#ifdef LBA2CC_EDITOR
        if (ExtDecorEditorObjectDeleted(ExtDecorEditorLoadedCubeX, ExtDecorEditorLoadedCubeY, i))
            ptrobj->Body |= DEC_INVISIBLE;
#endif

        // SEB le 12/04/96
        if (!(ptrobj->Body & DEC_INVISIBLE)) {
            LongWorldRotatePoint(ptrobj->XMin,
                                 ptrobj->YMin,
                                 ptrobj->ZMin);
            zr = CameraZr - Z0;
            if (zr > ClipZFar)
                continue;

            LongWorldRotatePoint(ptrobj->XMin,
                                 ptrobj->YMin,
                                 ptrobj->ZMax);
            zr = CameraZr - Z0;
            if (zr > ClipZFar)
                continue;

            LongWorldRotatePoint(ptrobj->XMax,
                                 ptrobj->YMin,
                                 ptrobj->ZMin);
            zr = CameraZr - Z0;
            if (zr > ClipZFar)
                continue;

            LongWorldRotatePoint(ptrobj->XMax,
                                 ptrobj->YMin,
                                 ptrobj->ZMax);
            zr = CameraZr - Z0;
            if (zr > ClipZFar)
                continue;

            // point chaud obj pour calcul profondeur fog
            LongWorldRotatePoint(ptrobj->Xworld,
                                 ptrobj->Yworld,
                                 ptrobj->Zworld);

            if (LongProjectPoint(X0, Y0, Z0)) {
                // Test preclipping a mettre ici
                //				if( (Xp>VIEW_EXT_X0) AND (Xp<VIEW_EXT_X1)
                //				AND (Yp>VIEW_EXT_Y0) AND (Yp<VIEW_EXT_Y1) )
                {
                    // add dans listtri
                    ptrtri->Zrot = CameraZr - Z0;
                    ptrtri->Num = i;
                    ptrtri->PtrDec = ptrobj;
                    ptrtri++;
                    NbObjScreen++;
                }
            }
        }
    }

    if (!NbObjScreen)
        return;

    // trie les objet en z 'front to back'
    qsort(ListTriExt, NbObjScreen, sizeof(T_LIST_TRI), sub_qsort_decors_zbuf);

    clipzstart = ClipZFar - StartZFog;

    //	Switch_Fillers( FILL_POLY_FOG_ZBUFFER );

    // affiche les objets statiques
    ptrtri = ListTriExt;
    for (i = 0; i < NbObjScreen; i++, ptrtri++) {
        S32 library;
        S32 body_index;
        T_HQR_HEADER *body_hqr;
        void *body_ptr;

        UnsetClip();
        ptrobj = ptrtri->PtrDec;

        SetCLUT(PalLevel +
                16 * BoundRegleTrois(0, 15,
                                     clipzstart,
                                     ptrtri->Zrot - StartZFog));

        ExtDecorEditorDecodeBody(ptrobj->Body, &library, &body_index);
        if (body_index < 0)
            continue;
        body_hqr = GetDecorLibraryHQR(library);
        if (!body_hqr)
            continue;
        body_ptr = HQR_Get(body_hqr, body_index);
        if (!body_ptr)
            continue;
        ObjPtrMap = (library > 0 AND library != Island) ? ObjTextures[library] : ObjTexture;

        if (BodyDisplay(ptrobj->Xworld,
                        ptrobj->Yworld,
                        ptrobj->Zworld,
                        0,
                        (ptrobj->Beta & 0xFFFF), 0,
                        body_ptr)) {
            SetClip(ScreenXMin, ScreenYMin, ScreenXMax, ScreenYMax);
            if (ClipXMin <= ClipXMax
                                AND ClipYMin <= ClipYMax) {
                ptrobj->Body |= DEC_DRAWN;
            }
        }
    }

    //	Switch_Fillers( FILL_TEXTURES_ON );

    UnsetClip();
}

/*══════════════════════════════════════════════════════════════════════════*
	  █▀▀▀▀ █▀▀▀█ █     █      █    ██▀▀▀  █    █▀▀▀█ ██▄ █ ██▀▀▀
	  ██    ██  █ ██    ██     ██   ▀▀▀▀█  ██   ██  █ ██▀██ ▀▀▀▀█
	  ▀▀▀▀▀ ▀▀▀▀▀ ▀▀▀▀▀ ▀▀▀▀▀  ▀▀   ▀▀▀▀▀  ▀▀   ▀▀▀▀▀ ▀▀  ▀ ▀▀▀▀▀
 *══════════════════════════════════════════════════════════════════════════*/
/*──────────────────────────────────────────────────────────────────────────*/

// Version ZVs absolues précalculées
// on teste par rapport a un seul point
S32 TestZVDecors(S32 xw, S32 yw, S32 zw, T_DECORS *objet) {
    if (objet->Body & DEC_INVISIBLE)
        return FALSE;

    if (objet->XMin > xw)
        return FALSE;
    if (objet->XMax < xw)
        return FALSE;

    if (objet->YMin > yw)
        return FALSE;
    if (objet->YMax < yw)
        return FALSE;

    if (objet->ZMin > zw)
        return FALSE;
    if (objet->ZMax < zw)
        return FALSE;

    return TRUE;
}
