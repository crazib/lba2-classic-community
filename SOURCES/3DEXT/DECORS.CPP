#include "DEFINES.H"

#include <SYSTEM/KEYBOARD.H>
#include <SYSTEM/KEYBOARD_KEYS.H>
#include <SYSTEM/LOG.H>
#include <SYSTEM/MOUSE.H>
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
extern S32 ZoneEditorHasAnySelected(void);

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

#define EXT_DECOR_EDITOR_MAX_EDITS 4096
#define EXT_DECOR_EDITOR_MAX_SELECTED 256

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
    S32 CubeX;
    S32 CubeY;
    S32 Num;
    S32 Body;
    S32 BaseX;
    S32 BaseY;
    S32 BaseZ;
    S32 BaseXMin;
    S32 BaseYMin;
    S32 BaseZMin;
    S32 BaseXMax;
    S32 BaseYMax;
    S32 BaseZMax;
    S32 Dx;
    S32 Dy;
    S32 Dz;
} T_EXT_DECOR_EDITOR_EDIT;

static T_EXT_DECOR_EDITOR_EDIT ExtDecorEditorEdits[EXT_DECOR_EDITOR_MAX_EDITS];

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
#define EXT_DECOR_EDITOR_NUDGE_SHIFT (5 * EXT_DECOR_EDITOR_NUDGE)

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
    }
    ExtDecorEditorLastClick = Click;
    ExtDecorEditorLastUp = CheckKey(K_UP) ? TRUE : FALSE;
    ExtDecorEditorLastDown = CheckKey(K_DOWN) ? TRUE : FALSE;
    ExtDecorEditorLastLeft = CheckKey(K_LEFT) ? TRUE : FALSE;
    ExtDecorEditorLastRight = CheckKey(K_RIGHT) ? TRUE : FALSE;
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
    return ZoneEditorHasAnySelected();
}

void ExtDecorEditorClearSelection(void) {
    memset(ExtDecorEditorSelList, 0, sizeof(ExtDecorEditorSelList));
    ZoneEditorClearAllSelected();
}

S32 ExtDecorEditorGetPickPending(void) {
    return ExtDecorEditorPickPending;
}

void ExtDecorEditorResolveClickPick(S32 zone_index, S32 zone_dist_sq) {
    S32 decor_hit;
    S32 zone_hit;
    S32 decor_wins;

    if (!ExtDecorEditorPickPending)
        return;

    decor_hit = ExtDecorEditorPickBestNum >= 0;
    zone_hit = zone_index >= 0;

    if (!decor_hit AND !zone_hit) {
        if (!ExtDecorEditorPickShift)
            ExtDecorEditorClearSelection();
        ExtDecorEditorPickPending = FALSE;
        return;
    }

    decor_wins = decor_hit AND (!zone_hit OR ExtDecorEditorPickBestDist <= zone_dist_sq);

    if (ExtDecorEditorPickShift) {
        if (decor_wins) {
            S32 i;
            S32 found = FALSE;

            for (i = 0; i < EXT_DECOR_EDITOR_MAX_SELECTED; i++) {
                T_EXT_DECOR_EDITOR_SEL *sel = &ExtDecorEditorSelList[i];

                if (!sel->Used)
                    continue;
                if (sel->CubeX == ExtDecorEditorPickBestCubeX
                        AND sel->CubeY == ExtDecorEditorPickBestCubeY
                        AND sel->Num == ExtDecorEditorPickBestNum
                        AND sel->Body == ExtDecorEditorPickBestBody) {
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
        } else {
            ZoneEditorToggleSelectedByIndex(zone_index);
        }
    } else {
        memset(ExtDecorEditorSelList, 0, sizeof(ExtDecorEditorSelList));
        ZoneEditorClearAllSelected();
        if (decor_wins) {
            ExtDecorEditorSelList[0].Used = TRUE;
            ExtDecorEditorSelList[0].CubeX = ExtDecorEditorPickBestCubeX;
            ExtDecorEditorSelList[0].CubeY = ExtDecorEditorPickBestCubeY;
            ExtDecorEditorSelList[0].Num = ExtDecorEditorPickBestNum;
            ExtDecorEditorSelList[0].Body = ExtDecorEditorPickBestBody;
        } else {
            ZoneEditorSelectOnly(zone_index);
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
        if (sel->CubeX == CurrentCubeX
                AND sel->CubeY == CurrentCubeY
                AND sel->Num == num
                AND sel->Body == body)
            return TRUE;
    }
    return FALSE;
}

static T_EXT_DECOR_EDITOR_EDIT *ExtDecorEditorFindEdit(S32 cube_x, S32 cube_y, S32 num, S32 body) {
    S32 i;

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_EDITS; i++) {
        T_EXT_DECOR_EDITOR_EDIT *edit = &ExtDecorEditorEdits[i];
        if (edit->Used
                AND edit->CubeX == cube_x
                AND edit->CubeY == cube_y
                AND edit->Num == num
                AND edit->Body == body)
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
            edit->CubeX = cube_x;
            edit->CubeY = cube_y;
            edit->Num = num;
            edit->Body = body;
            edit->BaseX = ptrobj->Xworld;
            edit->BaseY = ptrobj->Yworld;
            edit->BaseZ = ptrobj->Zworld;
            edit->BaseXMin = ptrobj->XMin;
            edit->BaseYMin = ptrobj->YMin;
            edit->BaseZMin = ptrobj->ZMin;
            edit->BaseXMax = ptrobj->XMax;
            edit->BaseYMax = ptrobj->YMax;
            edit->BaseZMax = ptrobj->ZMax;
            edit->Dx = 0;
            edit->Dy = 0;
            edit->Dz = 0;
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

static void ExtDecorEditorApplyEditToObject(T_EXT_DECOR_EDITOR_EDIT *edit, T_DECORS *ptrobj) {
    ptrobj->Xworld = edit->BaseX + edit->Dx;
    ptrobj->Yworld = edit->BaseY + edit->Dy;
    ptrobj->Zworld = edit->BaseZ + edit->Dz;
    ptrobj->XMin = edit->BaseXMin + edit->Dx;
    ptrobj->YMin = edit->BaseYMin + edit->Dy;
    ptrobj->ZMin = edit->BaseZMin + edit->Dz;
    ptrobj->XMax = edit->BaseXMax + edit->Dx;
    ptrobj->YMax = edit->BaseYMax + edit->Dy;
    ptrobj->ZMax = edit->BaseZMax + edit->Dz;
}

void ExtDecorEditorApplyRuntimeEditsForLoadedCube(S32 cube_x, S32 cube_y) {
    S32 i;
    T_DECORS *ptrobj;

    if (!NbObjDecors OR !ListDecors)
        return;

    ptrobj = ListDecors;
    for (i = 0; i < NbObjDecors; i++, ptrobj++) {
        T_EXT_DECOR_EDITOR_EDIT *edit;

        edit = ExtDecorEditorFindEdit(cube_x, cube_y, i, ptrobj->Body & 0xFFFF);
        if (edit)
            ExtDecorEditorApplyEditToObject(edit, ptrobj);
    }
}

static void ExtDecorEditorMoveAllSelected(S32 dx, S32 dz) {
    S32 i;

    for (i = 0; i < EXT_DECOR_EDITOR_MAX_SELECTED; i++) {
        T_EXT_DECOR_EDITOR_SEL *sel = &ExtDecorEditorSelList[i];
        T_EXT_DECOR_EDITOR_EDIT *edit;
        T_DECORS *ptrobj;

        if (!sel->Used)
            continue;

        edit = ExtDecorEditorFindEdit(sel->CubeX, sel->CubeY, sel->Num, sel->Body);
        if (sel->CubeX == CurrentCubeX
                AND sel->CubeY == CurrentCubeY
                AND sel->Num >= 0
                AND sel->Num < NbObjDecors) {
            ptrobj = &ListDecors[sel->Num];
            if ((ptrobj->Body & 0xFFFF) == sel->Body)
                edit = ExtDecorEditorGetOrCreateEdit(sel->CubeX, sel->CubeY, sel->Num, ptrobj);
        }
        if (!edit)
            continue;

        edit->Dx += dx;
        edit->Dz += dz;

        if (sel->CubeX == CurrentCubeX
                AND sel->CubeY == CurrentCubeY
                AND sel->Num >= 0
                AND sel->Num < NbObjDecors) {
            ptrobj = &ListDecors[sel->Num];
            if ((ptrobj->Body & 0xFFFF) == sel->Body)
                ExtDecorEditorApplyEditToObject(edit, ptrobj);
        }
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
                ExtDecorEditorPickBestCubeX = CurrentCubeX;
                ExtDecorEditorPickBestCubeY = CurrentCubeY;
                ExtDecorEditorPickBestNum = ptrtri->Num;
                ExtDecorEditorPickBestBody = ptrtri->PtrDec->Body & 0xFFFF;
            }
        }
    }
}

void ExtDecorEditorBeginFrame(void) {
    S32 click;
    S32 up;
    S32 down;
    S32 left;
    S32 right;
    S32 step;

    if (!ExtDecorEditorSelect)
        return;

    click = Click;
    ExtDecorEditorPickPending = FALSE;
    if ((click & 1) AND !(ExtDecorEditorLastClick & 1)) {
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

    up = CheckKey(K_UP) ? TRUE : FALSE;
    down = CheckKey(K_DOWN) ? TRUE : FALSE;
    left = CheckKey(K_LEFT) ? TRUE : FALSE;
    right = CheckKey(K_RIGHT) ? TRUE : FALSE;
    step = (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? EXT_DECOR_EDITOR_NUDGE_SHIFT : EXT_DECOR_EDITOR_NUDGE;

    if (up AND !ExtDecorEditorLastUp)
        ExtDecorEditorMoveAllSelected(0, step);
    if (down AND !ExtDecorEditorLastDown)
        ExtDecorEditorMoveAllSelected(0, -step);
    if (left AND !ExtDecorEditorLastLeft)
        ExtDecorEditorMoveAllSelected(-step, 0);
    if (right AND !ExtDecorEditorLastRight)
        ExtDecorEditorMoveAllSelected(step, 0);
    ExtDecorEditorLastUp = up;
    ExtDecorEditorLastDown = down;
    ExtDecorEditorLastLeft = left;
    ExtDecorEditorLastRight = right;
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

    if (ExtDecorEditorSelect) {
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
                                CurrentCubeX, CurrentCubeY, ptrtri->Num, ptrobj->Body & 0xFFFF);
                } else {
                    CoulText(EXT_DECOR_OVERLAY_TEXT, EXT_DECOR_OVERLAY_BACK);
                    GraphPrintf(FALSE, xs, label_y, "c%d,%d #%d b%d",
                                CurrentCubeX, CurrentCubeY, ptrtri->Num, ptrobj->Body & 0xFFFF);
                }
            }
        }
    }

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
        UnsetClip();
        ptrobj = ptrtri->PtrDec;

        SetCLUT(PalLevel +
                16 * BoundRegleTrois(0, 15,
                                     clipzstart,
                                     ptrtri->Zrot - StartZFog));

        if (BodyDisplay(ptrobj->Xworld,
                        ptrobj->Yworld,
                        ptrobj->Zworld,
                        0,
                        (ptrobj->Beta & 0xFFFF), 0,
                        HQR_Get(HQR_Isle_Obj, ptrobj->Body & 0xFFFF))) {
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
