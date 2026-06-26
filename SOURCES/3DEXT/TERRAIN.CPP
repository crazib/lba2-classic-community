#include "DEFINES.H"
#include "../COMMON.H"
#include "../EDITOR_JSON.H"
#include "../EDITOR.H"
#include "../PERSO.H"
#include "EDITOR_TERRAIN_PATTERNS.H"

#include <SVGA/SCREEN.H>
#ifdef LBA2CC_EDITOR
#include <SYSTEM/KEYBOARD.H>
#include <SYSTEM/KEYBOARD_KEYS.H>
#include <SYSTEM/LIMITS.H>
#include <SYSTEM/FILES.H>
#include <SYSTEM/LOG.H>
#include <SYSTEM/MOUSE.H>
#endif
#include <string.h>

#define LIMITE_LOIN (60 * 1024)
#define COLOR_LIMITE 255
#define SIZE_TEXTURE 64

#define WORLD_SIZE 32768
#define NB_POLYS_COTE 4

// Taille 2D mini pour accepter un poly animé
#define ANI_POLY_STEP_Y 2
#define ANI_POLY_STEP_X 5

#include "DRAWSKY.CPP"

S32 Sky_Y;
//S32	NbDrawnPolys ;

S32 FlagMasque = TRUE;

#if defined(DEBUG_TOOLS) && defined(LBA_GAME)
extern S32 DebugFps;
#endif

#ifdef LBA2CC_EDITOR
extern S32 Island;
#endif

#ifndef LBA_GAME
S32 Reglage = 0;
#endif

#ifdef LBA_EDITOR // Special PERSO.EXE
#define INFO_DECORS 1024
extern S32 FlagInfos;
#endif

//****************************************************************************
//****************************************************************************

S32 ListTypePolygone[4] = {0, POLY_SOLID, POLY_GOURAUD_TABLE, POLY_DITHER_TABLE};
S32 ListTypePTexture[] = {
    0,
    POLY_TEXTURE,
    POLY_TEXTURE_FLAT,
    POLY_TEXTURE_GOURAUD,
    0,
    POLY_TEXTURE_INCRUST,
    POLY_TEXTURE_INCRUST_FLAT,
    POLY_TEXTURE_INCRUST_GOURAUD,
    0,
    POLY_TEXTURE_INCRUST,
    POLY_TEXTURE_INCRUST_FLAT,
    POLY_TEXTURE_INCRUST_GOURAUD,
    0,
    POLY_TEXTURE_INCRUST,
    POLY_TEXTURE_INCRUST_FLAT,
    POLY_TEXTURE_INCRUST_GOURAUD,
};

/*══════════════════════════════════════════════════════════════════════════*
		   ▀▀█▀▀ █▀▀▀▀ █▀▀▀█ █▀▀▀█ █▀▀▀█  █    ██▄ █
		     ██  ██▀▀  ██▀█▀ ██▀█▀ ██▀▀█  ██   ██▀██
		     ▀▀  ▀▀▀▀▀ ▀▀  ▀ ▀▀  ▀ ▀▀  ▀  ▀▀   ▀▀  ▀
 *══════════════════════════════════════════════════════════════════════════*/

/*──────────────────────────────────────────────────────────────────────────*/

void DrawSkySeaZBuf() {
    // draw Sky
    //	Sky_Y = 12500 ;

    PtrMap = SkySeaTexture;
    RepMask = 0x7F7F;

    DrawSky(Sky_Y);
    //	DrawSea( Y_SEA ) ;
}

/*──────────────────────────────────────────────────────────────────────────*/

void ClsTerrainZBuf() {
    SetClearColor((U8)FogCoul);
    Cls();
    SetClearColor(0);

    memset(PtrZBuffer, 255, SIZE_Z_BUFFER); // Clear ZBuffer
}

/*──────────────────────────────────────────────────────────────────────────*/
static inline void DrawFeuillePolyZBuf(S32 xi, S32 zi, T_SOMMET_ROT *ptrs) {
    S32 color;
    T_HALF_POLY *ptrh;
    S32 f0, f1, f2, f3;
    S32 i0, i1, i2, i3;
    S32 x0, z0;
    S32 x1, z1;
    S32 x2, z2;
    S32 x3, z3;
    U8 type;
    U16 fog;

    ptrh = &MapPolyGround[zi * (64 * 2) + xi * 2];

    // optimisation globale affichage terrain
    if (!(ptrh->PolyFlag + ptrh->TexFlag +
          (ptrh + 1)->PolyFlag + (ptrh + 1)->TexFlag)) {
        return;
    }

    x0 = ptrs[0].X2D;
    z0 = ptrs[0].Y2D;
    x1 = ptrs[65].X2D;
    z1 = ptrs[65].Y2D;
    x3 = ptrs[1].X2D;
    z3 = ptrs[1].Y2D;
    x2 = ptrs[66].X2D;
    z2 = ptrs[66].Y2D;

    f0 = ptrs[0].Zrot;

    /*	PtrCLUTGouraud = PtrCLUTFog +
		(16*256) * BoundRegleTrois(0,15,
					ClipZFar - StartZFog,
					f0 - StartZFog ) ;
*/
    fog = (U16)(PalLevel + 16 * BoundRegleTrois(0, 15,
                                                ClipZFar - StartZFog,
                                                f0 - StartZFog));
    SetCLUT(fog);

    f0 = GET_ZO(f0);
    f1 = GET_ZO(ptrs[65].Zrot);
    f3 = GET_ZO(ptrs[1].Zrot);
    f2 = GET_ZO(ptrs[66].Zrot);

    {
        U8 *i = &MapIntensity[zi * 65 + xi];

        i0 = (i[0] & 15) * 256 - 255;
        i1 = (i[65] & 15) * 256 - 255;
        i3 = (i[1] & 15) * 256 - 255;
        i2 = (i[65 + 1] & 15) * 256 - 255;

        if (i0 < 0)
            i0 = 0; // groumpf
        if (i1 < 0)
            i1 = 0; // groumpf
        if (i2 < 0)
            i2 = 0; // groumpf
        if (i3 < 0)
            i3 = 0; // groumpf

/*		i0 = i[0]*256 		;
		i1 = i[65]*256		;
		i3 = i[1]*256 		;
		i2 = i[65+1]*256 	;
*/	}

if (ptrh->Sens == 0) {
    if (ptrh->PolyFlag + ptrh->TexFlag) {
        // type 0 triangle 0----------------------------

        Tab_Points[0].Pt_XE = (S16)x0;
        Tab_Points[0].Pt_YE = (S16)z0;
        Tab_Points[1].Pt_XE = (S16)x1;
        Tab_Points[1].Pt_YE = (S16)z1;
        Tab_Points[2].Pt_XE = (S16)x2;
        Tab_Points[2].Pt_YE = (S16)z2;

        if (TestVuePoly(Tab_Points)) {
            color = (ptrh->Bank << 4) + 11;

            Tab_Points[0].Pt_Light = (S16)i0;
            Tab_Points[1].Pt_Light = (S16)i1;
            Tab_Points[2].Pt_Light = (S16)i2;

            Tab_Points[0].Pt_ZO = (U16)f0;
            Tab_Points[1].Pt_ZO = (U16)f1;
            Tab_Points[2].Pt_ZO = (U16)f2;

            if (ptrh->PolyFlag) {
                if (ptrh->PolyFlag == 1) {
                    color += ((i0 + i1 + i2) * 21845) >> 24; // *21845 >> 16 EQU / 3
                }

                Fill_Poly(ListTypePolygone[ptrh->PolyFlag],
                          color, 3,
                          Tab_Points);
            }

            if (ptrh->TexFlag) {
                T_HALF_TEX *ptrt;

                ptrt = &ListTexDef[ptrh->IndexTex];

                Tab_Points[0].Pt_MapU = ptrt->Tx0;
                Tab_Points[0].Pt_MapV = ptrt->Ty0;
                Tab_Points[1].Pt_MapU = ptrt->Tx1;
                Tab_Points[1].Pt_MapV = ptrt->Ty1;
                Tab_Points[2].Pt_MapU = ptrt->Tx2;
                Tab_Points[2].Pt_MapV = ptrt->Ty2;

                if (ptrh->TexFlag == 2) {
                    color = ((i0 + i1 + i2) * 21845) >> 24; // *21845 >> 16 EQU / 3
                }

                type = (U8)ListTypePTexture[ptrh->TexFlag + (ptrh->PolyFlag << 2)];

                if (Fill_Poly(type, color, 3, Tab_Points)) {
                    if (MaxPolySea AND(ptrh->CodeJeu & 12) == 12) { // hihi je suis un polygone animé...

                        if (NbAniPoly < MaxPolySea - 1) {
                            PtrAniPoly->X0 = (S16)Min3(x0, x1, x2);
                            PtrAniPoly->Y0 = (S16)Min3(z0, z1, z2);
                            PtrAniPoly->X1 = (S16)Max3(x0, x1, x2);
                            PtrAniPoly->Y1 = (S16)Max3(z0, z1, z2);

                            if (((PtrAniPoly->Y1 - PtrAniPoly->Y0) > ANI_POLY_STEP_Y)
                                    AND((PtrAniPoly->X1 - PtrAniPoly->X0) > ANI_POLY_STEP_X)) {
                                PtrAniPoly->Type = type;
                                PtrAniPoly->Color = (U8)color;

                                PtrAniPoly->Fog = fog;

                                PtrAniPoly->Tab_Points[0] = Tab_Points[0];
                                PtrAniPoly->Tab_Points[1] = Tab_Points[1];
                                PtrAniPoly->Tab_Points[2] = Tab_Points[2];

                                NbAniPoly++;
                                PtrAniPoly++;
                            }
                        }
#ifdef DEBUG_TOOLS
                        else {
                            Message("MaxPolySea Atteint !", TRUE);
                        }
#endif
                    }
                }
            }
        }
    }
    // type 0 triangle 1----------------------------------

    ptrh++;

    if (ptrh->PolyFlag + ptrh->TexFlag) {
        Tab_Points[0].Pt_XE = (S16)x2;
        Tab_Points[0].Pt_YE = (S16)z2;
        Tab_Points[1].Pt_XE = (S16)x3;
        Tab_Points[1].Pt_YE = (S16)z3;
        Tab_Points[2].Pt_XE = (S16)x0;
        Tab_Points[2].Pt_YE = (S16)z0;

        if (TestVuePoly(Tab_Points)) {
            color = (ptrh->Bank << 4) + 11;

            Tab_Points[0].Pt_Light = (S16)i2;
            Tab_Points[1].Pt_Light = (S16)i3;
            Tab_Points[2].Pt_Light = (S16)i0;

            Tab_Points[0].Pt_ZO = (U16)f2;
            Tab_Points[1].Pt_ZO = (U16)f3;
            Tab_Points[2].Pt_ZO = (U16)f0;

            if (ptrh->PolyFlag) {
                if (ptrh->PolyFlag == 1) {
                    color += ((i0 + i2 + i3) * 21845) >> 24; // *21845 >> 16 EQU / 3
                }
                Fill_Poly(ListTypePolygone[ptrh->PolyFlag],
                          color, 3,
                          Tab_Points);
            }

            if (ptrh->TexFlag) {
                T_HALF_TEX *ptrt;

                ptrt = &ListTexDef[ptrh->IndexTex];

                Tab_Points[0].Pt_MapU = ptrt->Tx0;
                Tab_Points[0].Pt_MapV = ptrt->Ty0;
                Tab_Points[1].Pt_MapU = ptrt->Tx1;
                Tab_Points[1].Pt_MapV = ptrt->Ty1;
                Tab_Points[2].Pt_MapU = ptrt->Tx2;
                Tab_Points[2].Pt_MapV = ptrt->Ty2;

                if (ptrh->TexFlag == 2) {
                    color = ((i0 + i2 + i3) * 21845) >> 24; // *21845 >> 16 EQU / 3
                }

                type = (U8)ListTypePTexture[ptrh->TexFlag + (ptrh->PolyFlag << 2)];

                if (Fill_Poly(type, color, 3, Tab_Points)) {
                    if (MaxPolySea AND(ptrh->CodeJeu & 12) == 12) { // hihi je suis un polygone animé...
                        if (NbAniPoly < MaxPolySea - 1) {
                            PtrAniPoly->X0 = (S16)Min3(x0, x2, x3);
                            PtrAniPoly->Y0 = (S16)Min3(z0, z2, z3);
                            PtrAniPoly->X1 = (S16)Max3(x0, x2, x3);
                            PtrAniPoly->Y1 = (S16)Max3(z0, z2, z3);

                            if (((PtrAniPoly->Y1 - PtrAniPoly->Y0) > ANI_POLY_STEP_Y)
                                    AND((PtrAniPoly->X1 - PtrAniPoly->X0) > ANI_POLY_STEP_X)) {
                                PtrAniPoly->Type = type;
                                PtrAniPoly->Color = (U8)color;

                                PtrAniPoly->Fog = fog;

                                PtrAniPoly->Tab_Points[0] = Tab_Points[0];
                                PtrAniPoly->Tab_Points[1] = Tab_Points[1];
                                PtrAniPoly->Tab_Points[2] = Tab_Points[2];

                                NbAniPoly++;
                                PtrAniPoly++;
                            }
                        }
#ifdef DEBUG_TOOLS
                        else {
                            Message("MaxPolySea Atteint !", TRUE);
                        }
#endif
                    }
                }
            }
        }
    }
} else // type == 1---------------------------------------------------
{
    // type 1 triangle 0

    if (ptrh->PolyFlag + ptrh->TexFlag) {
        Tab_Points[0].Pt_XE = (S16)x3;
        Tab_Points[0].Pt_YE = (S16)z3;
        Tab_Points[1].Pt_XE = (S16)x0;
        Tab_Points[1].Pt_YE = (S16)z0;
        Tab_Points[2].Pt_XE = (S16)x1;
        Tab_Points[2].Pt_YE = (S16)z1;

        if (TestVuePoly(Tab_Points)) {
            color = (ptrh->Bank << 4) + 11;

            Tab_Points[0].Pt_Light = (S16)i3;
            Tab_Points[1].Pt_Light = (S16)i0;
            Tab_Points[2].Pt_Light = (S16)i1;

            Tab_Points[0].Pt_ZO = (U16)f3;
            Tab_Points[1].Pt_ZO = (U16)f0;
            Tab_Points[2].Pt_ZO = (U16)f1;

            if (ptrh->PolyFlag) {
                if (ptrh->PolyFlag == 1) {
                    color += ((i0 + i1 + i3) * 21845) >> 24; // *21845 >> 16 EQU / 3
                }
                Fill_Poly(ListTypePolygone[ptrh->PolyFlag],
                          color, 3,
                          Tab_Points);
            }

            if (ptrh->TexFlag) {
                T_HALF_TEX *ptrt;

                ptrt = &ListTexDef[ptrh->IndexTex];

                Tab_Points[0].Pt_MapU = ptrt->Tx0;
                Tab_Points[0].Pt_MapV = ptrt->Ty0;
                Tab_Points[1].Pt_MapU = ptrt->Tx1;
                Tab_Points[1].Pt_MapV = ptrt->Ty1;
                Tab_Points[2].Pt_MapU = ptrt->Tx2;
                Tab_Points[2].Pt_MapV = ptrt->Ty2;

                if (ptrh->TexFlag == 2) {
                    color = ((i0 + i1 + i3) * 21845) >> 24; // *21845 >> 16 EQU / 3
                }

                type = (U8)ListTypePTexture[ptrh->TexFlag + (ptrh->PolyFlag << 2)];

                if (Fill_Poly(type, color, 3, Tab_Points)) {
                    if (MaxPolySea AND(ptrh->CodeJeu & 12) == 12) { // hihi je suis un polygone animé...
                        if (NbAniPoly < MaxPolySea - 1) {
                            PtrAniPoly->X0 = (S16)Min3(x0, x1, x3);
                            PtrAniPoly->Y0 = (S16)Min3(z0, z1, z3);
                            PtrAniPoly->X1 = (S16)Max3(x0, x1, x3);
                            PtrAniPoly->Y1 = (S16)Max3(z0, z1, z3);

                            if (((PtrAniPoly->Y1 - PtrAniPoly->Y0) > ANI_POLY_STEP_Y)
                                    AND((PtrAniPoly->X1 - PtrAniPoly->X0) > ANI_POLY_STEP_X)) {
                                PtrAniPoly->Type = type;
                                PtrAniPoly->Color = (U8)color;

                                PtrAniPoly->Fog = fog;

                                PtrAniPoly->Tab_Points[0] = Tab_Points[0];
                                PtrAniPoly->Tab_Points[1] = Tab_Points[1];
                                PtrAniPoly->Tab_Points[2] = Tab_Points[2];

                                NbAniPoly++;
                                PtrAniPoly++;
                            }
                        }
#ifdef DEBUG_TOOLS
                        else {
                            Message("MaxPolySea Atteint !", TRUE);
                        }
#endif
                    }
                }
            }
        }
    }
    // type 1 triangle 1

    ptrh++;

    if (ptrh->PolyFlag + ptrh->TexFlag) {
        Tab_Points[0].Pt_XE = (S16)x1;
        Tab_Points[0].Pt_YE = (S16)z1;
        Tab_Points[1].Pt_XE = (S16)x2;
        Tab_Points[1].Pt_YE = (S16)z2;
        Tab_Points[2].Pt_XE = (S16)x3;
        Tab_Points[2].Pt_YE = (S16)z3;

        if (TestVuePoly(Tab_Points)) {
            color = (ptrh->Bank << 4) + 11;

            Tab_Points[0].Pt_Light = (S16)i1;
            Tab_Points[1].Pt_Light = (S16)i2;
            Tab_Points[2].Pt_Light = (S16)i3;

            Tab_Points[0].Pt_ZO = (U16)f1;
            Tab_Points[1].Pt_ZO = (U16)f2;
            Tab_Points[2].Pt_ZO = (U16)f3;

            if (ptrh->PolyFlag) {
                if (ptrh->PolyFlag == 1) {
                    color += ((i1 + i2 + i3) * 21845) >> 24; // *21845 >> 16 EQU / 3
                }

                Fill_Poly(ListTypePolygone[ptrh->PolyFlag],
                          color, 3,
                          Tab_Points);
            }

            if (ptrh->TexFlag) {
                T_HALF_TEX *ptrt;

                ptrt = &ListTexDef[ptrh->IndexTex];

                Tab_Points[0].Pt_MapU = ptrt->Tx0;
                Tab_Points[0].Pt_MapV = ptrt->Ty0;
                Tab_Points[1].Pt_MapU = ptrt->Tx1;
                Tab_Points[1].Pt_MapV = ptrt->Ty1;
                Tab_Points[2].Pt_MapU = ptrt->Tx2;
                Tab_Points[2].Pt_MapV = ptrt->Ty2;

                if (ptrh->TexFlag == 2) {
                    color = ((i1 + i2 + i3) * 21845) >> 24; // *21845 >> 16 EQU / 3
                }

                type = (U8)ListTypePTexture[ptrh->TexFlag + (ptrh->PolyFlag << 2)];

                if (Fill_Poly(type, color, 3, Tab_Points)) {
                    if (MaxPolySea AND(ptrh->CodeJeu & 12) == 12) { // hihi je suis un polygone animé...
                        if (NbAniPoly < MaxPolySea - 1) {
                            PtrAniPoly->X0 = (S16)Min3(x1, x2, x3);
                            PtrAniPoly->Y0 = (S16)Min3(z1, z2, z3);
                            PtrAniPoly->X1 = (S16)Max3(x1, x2, x3);
                            PtrAniPoly->Y1 = (S16)Max3(z1, z2, z3);

                            if (((PtrAniPoly->Y1 - PtrAniPoly->Y0) > ANI_POLY_STEP_Y)
                                    AND((PtrAniPoly->X1 - PtrAniPoly->X0) > ANI_POLY_STEP_X)) {
                                PtrAniPoly->Type = type;
                                PtrAniPoly->Color = (U8)color;

                                PtrAniPoly->Fog = fog;

                                PtrAniPoly->Tab_Points[0] = Tab_Points[0];
                                PtrAniPoly->Tab_Points[1] = Tab_Points[1];
                                PtrAniPoly->Tab_Points[2] = Tab_Points[2];

                                NbAniPoly++;
                                PtrAniPoly++;
                            }
                        }
#ifdef DEBUG_TOOLS
                        else {
                            Message("MaxPolySea Atteint !", TRUE);
                        }
#endif
                    }
                }
            }
        }
    }
}

#if !(defined(DEMO) || defined(LBA_GAME))
if (FillFeuille & 4) {
    DrawColFeuille3D(x0, z0,
                     x1, z1,
                     x2, z2,
                     x3, z3,
                     MapPolyGround[zi * 64 * 2 + xi * 2].Sens,
                     CYAN,
                     MapPolyGround[zi * 64 * 2 + xi * 2].Col,
                     MapPolyGround[zi * 64 * 2 + xi * 2 + 1].Col);
}

if (FillFeuille & 2) {
    DrawFeuille3D(x0, z0,
                  x1, z1,
                  x2, z2,
                  x3, z3,
                  MapPolyGround[zi * 64 * 2 + xi * 2].Sens,
                  GREEN,
                  FALSE);
}
#endif
}

/*──────────────────────────────────────────────────────────────────────────*/
static inline void FillBlackPolyZBuf(S32 xi, S32 zi, T_SOMMET_ROT *ptrs, U8 *ptra) {
    T_HALF_POLY *ptrh;
    //	S32 		f0, f1, f2, f3 	;
    //	S32 		f00, f10, f20, f30 ;
    S32 x0, z0;
    S32 x1, z1;
    S32 x2, z2;
    S32 x3, z3;
    U8 b0, b1, b2, b3;

    ptrh = &MapPolyGround[zi * (64 * 2) + xi * 2];

    x0 = ptrs[0].X2D;
    z0 = ptrs[0].Y2D;
    b0 = (U8)(ptra[0] & 32);
    x1 = ptrs[65].X2D;
    z1 = ptrs[65].Y2D;
    b1 = (U8)(ptra[65] & 32);
    x3 = ptrs[1].X2D;
    z3 = ptrs[1].Y2D;
    b3 = (U8)(ptra[1] & 32);
    x2 = ptrs[66].X2D;
    z2 = ptrs[66].Y2D;
    b2 = (U8)(ptra[66] & 32);

    /*
	xi *= 512 ;
	zi *= 512 ;

	f0 = GET_ZO(ptrs[0].Zrot)	;
	f1 = GET_ZO(ptrs[65].Zrot)	;
	f3 = GET_ZO(ptrs[1].Zrot)	;
	f2 = GET_ZO(ptrs[66].Zrot)	;

	LongWorldRotatePoint(xi, 0, zi)	;
	f00 = GET_ZO(CameraZr - Z0)	;
	LongWorldRotatePoint(xi, 0, zi+512);
	f10 = GET_ZO(CameraZr - Z0)	;
	LongWorldRotatePoint(xi+512, 0, zi);
	f30 = GET_ZO(CameraZr - Z0)	;
	LongWorldRotatePoint(xi+512, 0, zi+512);
	f20 = GET_ZO(CameraZr - Z0)	;
*/

    if (!(b0 | b1)) {
        Tab_Points[0].Pt_XE = (S16)x0;
        Tab_Points[0].Pt_YE = (S16)z0;
        Tab_Points[1].Pt_XE = (S16)x1;
        Tab_Points[1].Pt_YE = (S16)z1;
        Tab_Points[2].Pt_XE = (S16)x1;
        Tab_Points[2].Pt_YE = (S16)((S32)ModeDesiredY - 1);
        Tab_Points[3].Pt_XE = (S16)x0;
        Tab_Points[3].Pt_YE = (S16)((S32)ModeDesiredY - 1);

        Tab_Points[0].Pt_Light = (S16)0;
        Tab_Points[1].Pt_Light = (S16)0;
        Tab_Points[2].Pt_Light = (S16)0;
        Tab_Points[3].Pt_Light = (S16)0;

        Tab_Points[0].Pt_ZO = (U16)0;
        Tab_Points[1].Pt_ZO = (U16)0;
        Tab_Points[2].Pt_ZO = (U16)0;
        Tab_Points[3].Pt_ZO = (U16)0;

        Fill_Poly(0, 0, 4, Tab_Points);
    }

    if (!(b1 | b2)) {
        Tab_Points[0].Pt_XE = (S16)x1;
        Tab_Points[0].Pt_YE = (S16)z1;
        Tab_Points[1].Pt_XE = (S16)x2;
        Tab_Points[1].Pt_YE = (S16)z2;
        Tab_Points[2].Pt_XE = (S16)x2;
        Tab_Points[2].Pt_YE = (S16)((S32)ModeDesiredY - 1);
        Tab_Points[3].Pt_XE = (S16)x1;
        Tab_Points[3].Pt_YE = (S16)((S32)ModeDesiredY - 1);

        Tab_Points[0].Pt_Light = (S16)0;
        Tab_Points[1].Pt_Light = (S16)0;
        Tab_Points[2].Pt_Light = (S16)0;
        Tab_Points[3].Pt_Light = (S16)0;

        Tab_Points[0].Pt_ZO = (U16)0;
        Tab_Points[1].Pt_ZO = (U16)0;
        Tab_Points[2].Pt_ZO = (U16)0;
        Tab_Points[3].Pt_ZO = (U16)0;

        Fill_Poly(0, 0, 4, Tab_Points);
    }

    if (!(b2 | b3)) {
        Tab_Points[0].Pt_XE = (S16)x2;
        Tab_Points[0].Pt_YE = (S16)z2;
        Tab_Points[1].Pt_XE = (S16)x3;
        Tab_Points[1].Pt_YE = (S16)z3;
        Tab_Points[2].Pt_XE = (S16)x3;
        Tab_Points[2].Pt_YE = (S16)((S32)ModeDesiredY - 1);
        Tab_Points[3].Pt_XE = (S16)x2;
        Tab_Points[3].Pt_YE = (S16)((S32)ModeDesiredY - 1);

        Tab_Points[0].Pt_Light = (S16)0;
        Tab_Points[1].Pt_Light = (S16)0;
        Tab_Points[2].Pt_Light = (S16)0;
        Tab_Points[3].Pt_Light = (S16)0;

        Tab_Points[0].Pt_ZO = (U16)0;
        Tab_Points[1].Pt_ZO = (U16)0;
        Tab_Points[2].Pt_ZO = (U16)0;
        Tab_Points[3].Pt_ZO = (U16)0;

        Fill_Poly(0, 0, 4, Tab_Points);
    }

    if (!(b3 | b0)) {
        Tab_Points[0].Pt_XE = (S16)x3;
        Tab_Points[0].Pt_YE = (S16)z3;
        Tab_Points[1].Pt_XE = (S16)x0;
        Tab_Points[1].Pt_YE = (S16)z0;
        Tab_Points[2].Pt_XE = (S16)x0;
        Tab_Points[2].Pt_YE = (S16)((S32)ModeDesiredY - 1);
        Tab_Points[3].Pt_XE = (S16)x3;
        Tab_Points[3].Pt_YE = (S16)((S32)ModeDesiredY - 1);

        Tab_Points[0].Pt_Light = (S16)0;
        Tab_Points[1].Pt_Light = (S16)0;
        Tab_Points[2].Pt_Light = (S16)0;
        Tab_Points[3].Pt_Light = (S16)0;

        Tab_Points[0].Pt_ZO = (U16)0;
        Tab_Points[1].Pt_ZO = (U16)0;
        Tab_Points[2].Pt_ZO = (U16)0;
        Tab_Points[3].Pt_ZO = (U16)0;

        Fill_Poly(0, 0, 4, Tab_Points);
    }
}
/*──────────────────────────────────────────────────────────────────────────*/
struct S_Terrain {
    S8 x;
    S8 y;
};

struct S_Terrain TerrNO[9] = {
    {-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {0, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};

struct S_Terrain TerrNE[9] = {
    {1, -1}, {0, -1}, {-1, -1}, {1, 0}, {0, 0}, {-1, 0}, {1, 1}, {0, 1}, {-1, 1}};

struct S_Terrain TerrSO[9] = {
    {-1, 1}, {0, 1}, {1, 1}, {-1, 0}, {0, 0}, {1, 0}, {-1, -1}, {0, -1}, {1, -1}};

struct S_Terrain TerrSE[9] = {
    {1, 1}, {0, 1}, {-1, 1}, {1, 0}, {0, 0}, {-1, 0}, {1, -1}, {0, -1}, {-1, -1}};

struct S_Terrain *TerrainTri;

S32 OffPosCamera[3] = {
    // offsets camera precalculés
    -(512 * 64), // -1
    0,           //  0
    (512 * 64)   //  1
};

extern S32 PtrProjectPoint(S32 xw, S32 yw, S32 zw);

void DrawHorizon2ZBuf() {
    S32 flag = FALSE;
    S32 cx = CameraX;
    S32 cy = CameraY;
    S32 cz = CameraZ;
    S32 terrain_count = 9;
#ifdef LBA2CC_EDITOR
    struct S_Terrain editor_terrain[81];
#endif

    if (BetaCam >= 0 AND BetaCam < 1024) {
        TerrainTri = TerrSO;
    } else if (BetaCam >= 1024 AND BetaCam < 2048) {
        TerrainTri = TerrNO;
    } else if (BetaCam >= 2048 AND BetaCam < 3072) {
        TerrainTri = TerrNE;
    } else if (BetaCam >= 3072 AND BetaCam < 4096) {
        TerrainTri = TerrSE;
    }
    /*
	if( BetaCam>=0 AND BetaCam<1024 )
	{
		TerrainTri = TerrNE ;
	}
	else if( BetaCam>=1024 AND BetaCam<2048 )
	{
		TerrainTri = TerrSE ;
	}
	else if( BetaCam>=2048 AND BetaCam<3072 )
	{
		TerrainTri = TerrSO ;
	}
	else if( BetaCam>=3072 AND BetaCam<4096 )
	{
		TerrainTri = TerrNO ;
	}
*/
#ifdef LBA2CC_EDITOR
    // TODO: Other islands show black void artifacts with the wider editor view, so keep it gated to Proxima.
    if (ExteriorWideViewActive() OR(Editor_IsActive() AND Island == 12)) {
        S32 x_start;
        S32 x_step;
        S32 y_start;
        S32 y_step;
        S32 x;
        S32 y;
        S32 n = 0;

        S32 radius = ExteriorWideViewActive() ? 4 : 2;
        x_start = (BetaCam >= 2048) ? radius : -radius;
        x_step = (BetaCam >= 2048) ? -1 : 1;
        y_start = (BetaCam < 1024 OR BetaCam >= 3072) ? radius : -radius;
        y_step = (BetaCam < 1024 OR BetaCam >= 3072) ? -1 : 1;
        for (y = y_start; y >= -radius AND y <= radius; y += y_step) {
            for (x = x_start; x >= -radius AND x <= radius; x += x_step) {
                editor_terrain[n].x = (S8)x;
                editor_terrain[n].y = (S8)y;
                n++;
            }
        }
        TerrainTri = editor_terrain;
        terrain_count = (radius * 2 + 1) * (radius * 2 + 1);
    }
#endif
    for (S32 n = terrain_count; n > 0; n--, TerrainTri++) {
        S32 x, y;

        x = CurrentCubeX + TerrainTri->x;
        y = CurrentCubeY + TerrainTri->y;

        if (TerrainTri->x OR TerrainTri->y) {
            if ((x >= 0 AND x <= 15 AND y >= 0 AND y <= 15) OR ExteriorWideViewActive()) {
                // PreClipping du Cube
                /*				if( PtrProjectPoint( 0, 0, 0 )
				OR  PtrProjectPoint( 32767, 0, 0 )
				OR  PtrProjectPoint( 32767, 0, 32767 )
				OR  PtrProjectPoint( 0, 0, 32767 ) )
*/
                {
#ifdef LBA2CC_EDITOR
                    if (Editor_IsActive() AND Island == 12) {
#ifdef LBA_GAME
                        SetPosCamera(cx - TerrainTri->x * WORLD_SIZE + TerrainTri->x, cy, cz - TerrainTri->y * WORLD_SIZE + TerrainTri->y);
#else
                        SetPosCamera(cx - TerrainTri->x * WORLD_SIZE + TerrainTri->x * Reglage, cy, cz - TerrainTri->y * WORLD_SIZE + TerrainTri->y * Reglage);
#endif
                    } else
#endif
                    {
#ifdef LBA_GAME
                    SetPosCamera(cx - OffPosCamera[TerrainTri->x + 1] + TerrainTri->x, cy, cz - OffPosCamera[TerrainTri->y + 1] + TerrainTri->y);
#else
                    SetPosCamera(cx - OffPosCamera[TerrainTri->x + 1] + TerrainTri->x * Reglage, cy, cz - OffPosCamera[TerrainTri->y + 1] + TerrainTri->y * Reglage);
#endif
                    }
                    if (x >= 0 AND x <= 15 AND y >= 0 AND y <= 15 AND LoadCube(x, y)) {
                        AffichageTerrainZBuf();
                        flag = TRUE;
                    } else
                        DrawOneSea(Y_SEA);
                }
            }
        }
    }

    SetPosCamera(cx, cy, cz);

    if (flag) {
        if (!LoadCube(CurrentCubeX, CurrentCubeY)) {
            // on est dans un cube phantom !!!!!!!
            InitSeaCube();
            DrawOneSea(Y_SEA);
        }
    }
}

/*──────────────────────────────────────────────────────────────────────────*/
U32 MaskVisible(U8 *in_ptr) {
    // This code conversion could be improved...

    U8 *firstRow = in_ptr;
    U8 *secondRow = in_ptr + 65;

    U8 maskH = *firstRow;
    U8 maskL = *firstRow;

    maskH |= *secondRow;
    maskL &= *secondRow;

    maskH |= *(firstRow + 1);
    maskL &= *(firstRow + 1);

    maskH |= *(secondRow + 1);
    maskL &= *(secondRow + 1);

    U32 finalMask = 0;
    finalMask |= (maskH << 8);
    finalMask |= maskL;
    finalMask &= 0x20FF;

    return finalMask;
}

/*──────────────────────────────────────────────────────────────────────────*/
void AffichageTerrainZBuf(void) {
    S16 *ptrm;
    U8 *ptra;
    T_SOMMET_ROT *ptrs;

    // affichage des objets du decors
    AffichageObjetDecorsZBuf();

    // calcul les coordonnées 2D de chaque points
    ptra = &SommetAffichable[0][0];
    ptrm = MapSommetY;
    ptrs = &SommetRot[0][0];

    for (S32 z = 0; z <= NB_COTE * 512; z += 512) {
        for (S32 x = 0; x <= NB_COTE * 512; x += 512) {
            S32 zr;

            LongWorldRotatePoint(x, *ptrm, z);

            zr = CameraZr - Z0;

            // cone clipping

            if (zr <= NearClip) {
                *ptra = 32;
            } else if ((zr < ClipZFar)
                           AND LongProjectPoint(X0, Y0, Z0)) {
                U8 flag;

                flag = 16;

                if (Xp < ClipXMin)
                    flag |= 1;
                if (Xp > ClipXMax)
                    flag |= 2;
                if (Yp < ClipYMin)
                    flag |= 4;
                if (Yp > ClipYMax)
                    flag |= 8;

                *ptra = flag;

                ptrs->Zrot = zr;

                ptrs->X2D = (S16)Xp;
                ptrs->Y2D = (S16)Yp;

            } else {
                *ptra = 0;
            }

            ptra++;
            ptrm++;
            ptrs++;
        }
    }

    PtrMap = GroundTexture;
    RepMask = 0xFFFF;

    ptra = &SommetAffichable[0][0];
    ptrs = &SommetRot[0][0];

    for (S32 z = 0; z < NB_COTE; z++) {
        for (S32 x = 0; x < NB_COTE; x++) {
            S32 mask;

            mask = MaskVisible(ptra);

            switch (mask & 0xFF) {
            case 32:
                break;
            case 16:
                DrawFeuillePolyZBuf(x, z, ptrs);
                break;
            case 0:
                if (mask) {
                    FillBlackPolyZBuf(x, z, ptrs, ptra);
                }
                break;
            }

            ptra++;
            ptrs++;
        }

        ptra++;
        ptrs++;
    }

    DrawOneSea(Y_SEA);

#ifdef LBA_EDITOR
    if (FlagInfos & INFO_DECORS) {
        AffichageObjetDecorsZV();
    }
#endif
#ifdef LBA2CC_EDITOR
    ExtDecorOverlayDraw();
    TerrainEditorOverlayDraw();
    TerrainTextureEditorOverlayDraw();
    TerrainRegionEditorOverlayDraw();
#endif

    BoxStaticAdd(0, 0, ModeDesiredX - 1, ModeDesiredY - 1);
}

/*

void	AffichageTerrainZBuf( void )
{
	S16		*ptrm	;
	U8		*ptra	;
	T_SOMMET_ROT	*ptrs	;

	// affichage des objets du decors
	AffichageObjetDecorsZBuf() ;

	// calcul les coordonnées 2D de chaque points
	ptra = &SommetAffichable[0][0];
	ptrm = MapSommetY ;
	ptrs = &SommetRot[0][0] ;

	for (S32 z=0; z<=NB_COTE*512; z+=512)
	{
		for (S32 x=0; x<=NB_COTE*512; x+=512)
		{
			S32	zr ;

			LongWorldRotatePoint(x, *ptrm, z);

			zr = CameraZr - Z0 ;// l'ecran

			// cone clipping

			if( (zr<ClipZFar)
			AND (zr>NearClip)
			AND LongProjectPoint( X0,Y0,Z0 ) )
			{
				U8	flag	;

				flag = 16	;

				if( Xp < ClipXMin )	flag |= 1 ;
				if( Xp > ClipXMax )	flag |= 2 ;
				if( Yp < ClipYMin )	flag |= 4 ;
				if( Yp > ClipYMax )	flag |= 8 ;

				*ptra		= flag 	;

				ptrs->Zrot 	= zr;

				// Add 0.5 bias before truncation to reduce edge artifacts from coordinate precision loss
				ptrs->X2D	= (S16)(Xp + 0.5);
				ptrs->Y2D	= (S16)(Yp + 0.5);

			}
			else
			{
				*ptra = 0	;
			}

			ptra++;
			ptrm++;
			ptrs++;
		}
	}

	PtrMap = GroundTexture ;
	RepMask = 0xFFFF ;

	ptra  = &SommetAffichable[0][0] ;
	ptrs  = &SommetRot[0][0] ;

	for(z=0; z<NB_COTE; z++)
	{
		for(S32 x=0; x<NB_COTE; x++)
		{
			if (MaskVisible(ptra) == 16)
			{
				DrawFeuillePolyZBuf( x, z, ptrs) ;
			}

			ptra++	;
			ptrs++	;
		}

		ptra++	;
		ptrs++	;
	}

	BoxStaticAdd( 0, 0, ModeDesiredX-1, ModeDesiredY-1 ) ;
}

*/
/*──────────────────────────────────────────────────────────────────────────*/
//****************************************************************************

void AffichageTerrainRapide(void) {
    for (S32 xi = 0; xi <= NB_COTE; xi++) {
        for (S32 zi = 0; zi <= NB_COTE; zi++) {
            LongWorldRotatePoint(xi * 512,
                                 (S32)MapSommetY[zi * 65 + xi],
                                 zi * 512);

            if (LongProjectPoint(X0, Y0, Z0)) {
                Plot(Xp, Yp, WHITE);
            }
        }
    }
}

/*──────────────────────────────────────────────────────────────────────────*/
/*──────────────────────────────────────────────────────────────────────────*/

void CopyBlock256(U8 *sour, U8 *dest) {
    S32 y;

    for (y = 0; y < 32; y++) {
        memcpy(dest, sour, 32);
        dest += 256;
        sour += 256;
    }
}

/*──────────────────────────────────────────────────────────────────────────*/
S32 RefreshAniPoly = TRUE;
S32 NbToRefresh = 0;

/*──────────────────────────────────────────────────────────────────────────*/
void ClearAniPoly(void) {
    NbAniPoly = 0;
    PtrAniPoly = PtrPolySea;
    RefreshAniPoly = NbAniPoly;
    NbToRefresh = NbAniPoly / 4;
}

/*──────────────────────────────────────────────────────────────────────────*/
void DoAnimatedPolys() {
    if (!NbAniPoly OR !MaxPolySea)
        return;

    if (TimerRefHR > TimerAniPoly) {
        CntAniPoly8++;
        CntAniPoly8 &= 7; // 0-8
        CntAniPoly6++;
        CntAniPoly6 %= 6; // 0-5

        TimerAniPoly = TimerRefHR + 1000 / 4; // voir pour nb frame/sec

        if (OffsetAniPoly1 != -1) {
            CopyBlock256(SkySeaTexture + 128 * 256 + CntAniPoly8 * 32,
                         GroundTexture + OffsetAniPoly1);
        }

        if (OffsetAniPoly2 != -1) {
            CopyBlock256(SkySeaTexture + (128 + 32) * 256 + CntAniPoly8 * 32,
                         GroundTexture + OffsetAniPoly2);
        }

        if (OffsetAniPoly3 != -1) {
            CopyBlock256(SkySeaTexture + (128 + 32 + 32) * 256 + 32 * 2 + CntAniPoly6 * 32,
                         GroundTexture + OffsetAniPoly3);
        }

        if (OffsetAniPoly4 != -1) {
            CopyBlock256(SkySeaTexture + (128 + 32 + 32 + 32) * 256 + 32 * 2 + CntAniPoly6 * 32,
                         GroundTexture + OffsetAniPoly4);
        }

        RefreshAniPoly = NbAniPoly;
        NbToRefresh = NbAniPoly / 4;
    }
}

/*──────────────────────────────────────────────────────────────────────────*/

void DrawAnimatedPolys() {
    S32 n;
    S_ANI_POLY *ptr;

    if (!NbAniPoly OR !MaxPolySea OR !RefreshAniPoly)
        return;

    PtrMap = GroundTexture;
    RepMask = 0xFFFF;

    Switch_Fillers(FILL_POLY_FOG_NZW);

    if (NbToRefresh > RefreshAniPoly)
        NbToRefresh = RefreshAniPoly;

    ptr = PtrAniPoly = (S_ANI_POLY *)&PtrPolySea[RefreshAniPoly - 1];
    for (n = 0; n < NbToRefresh; n++, ptr--) {
        UnsetClip();

        SetClip(ptr->X0, ptr->Y0, ptr->X1, ptr->Y1);

        if (ClipXMin <= ClipXMax
                            AND ClipYMin <= ClipYMax) {
            SetCLUT(ptr->Fog);

            Fill_Poly(ptr->Type, ptr->Color, 3, ptr->Tab_Points);

            BoxStaticAdd(ClipXMin, ClipYMin, ClipXMax, ClipYMax);
        }

        RefreshAniPoly--;
    }

    // protec au cas ou
    if (RefreshAniPoly < 0)
        RefreshAniPoly = 0;

    // Met a jour Screen pour recouvrements
    BoxBlitStaticListToScreen();

    Switch_Fillers(FILL_POLY_TEXTURES);
    SetCLUT(PalLevel);

    UnsetClip();
}

/*──────────────────────────────────────────────────────────────────────────*/

void FiltreAnimatedPolys() {
    S32 n;
    S_ANI_POLY *ptr;

    if (!NbAniPoly OR !MaxPolySea)
        return;

    ptr = PtrAniPoly = (S_ANI_POLY *)PtrPolySea;

    for (n = 0; n < NbAniPoly;) {
        Fill_Poly(POLY_FLAG_ZBUFFER, 0, 3, ptr->Tab_Points);

        if (IsPolygonHidden) {
            if (NbAniPoly > 1) {
                memcpy((char *)ptr, (char *)&PtrPolySea[NbAniPoly - 1], sizeof(S_ANI_POLY));
            }
            NbAniPoly--;
        } else {
            n++;
            ptr++;
        }
    }
}

//****************************************************************************
//****************************************************************************

#ifdef LBA2CC_EDITOR
static S32 TerrainEditorSelect = FALSE;
static S32 TerrainEditorLastClick = 0;
static S32 TerrainEditorLastUp = 0;
static S32 TerrainEditorLastDown = 0;
static S32 TerrainEditorLastSelectAll = 0;
static S32 TerrainEditorLastFlattenZero = 0;
static S32 TerrainEditorLastJitter = 0;
static S32 TerrainEditorLastTransition = 0;
static U32 TerrainEditorJitterCounter = 0;
static S32 TerrainEditorPickPending = FALSE;
static S32 TerrainEditorPickShift = FALSE;
static S32 TerrainEditorPickBestDist = 0;
static S32 TerrainEditorPickBestCubeX = 0;
static S32 TerrainEditorPickBestCubeY = 0;
static S32 TerrainEditorPickBestIndex = -1;
static S32 TerrainEditorLoadedCubeX = -1;
static S32 TerrainEditorLoadedCubeY = -1;
static S32 TerrainEditorDragActive = FALSE;
static S32 TerrainEditorDragSelectPending = FALSE;
static S32 TerrainEditorDragShift = FALSE;
static S32 TerrainEditorDragX0 = 0;
static S32 TerrainEditorDragY0 = 0;
static S32 TerrainEditorDragX1 = 0;
static S32 TerrainEditorDragY1 = 0;
static S32 TerrainTextureEditorSelect = FALSE;
static S32 TerrainTextureEditorLastClick = 0;
static S32 TerrainTextureEditorLastUp = 0;
static S32 TerrainTextureEditorLastDown = 0;
static S32 TerrainTextureEditorLastLeft = 0;
static S32 TerrainTextureEditorLastRight = 0;
static S32 TerrainTextureEditorLastSelectAll = 0;
static S32 TerrainTextureEditorLastT = 0;
static S32 TerrainTextureEditorLastQ = 0;
static S32 TerrainTextureEditorLastL = 0;
static S32 TerrainTextureEditorLastFullPalette = 0;
static S32 TerrainTextureEditorLastPalettePrev = 0;
static S32 TerrainTextureEditorLastPaletteNext = 0;
static S32 TerrainTextureEditorQuadMode = TRUE;
static S32 TerrainTextureEditorFullPalette = FALSE;
static S32 TerrainTextureEditorPalettePage = 0;
static S32 TerrainRegionEditorSelect = FALSE;
static S32 TerrainRegionEditorLastClick = 0;
static S32 TerrainRegionEditorLastDelete = 0;
static S32 TerrainRegionEditorLastM = 0;
static S32 TerrainRegionEditorLastQ = 0;
static S32 TerrainRegionEditorLastF = 0;
static S32 TerrainRegionEditorPaintMode = TRUE;
static S32 TerrainRegionEditorQuadMode = TRUE;
static S32 TerrainRegionEditorRectFill = FALSE;
static S32 TerrainRegionEditorDragActive = FALSE;
static S32 TerrainRegionEditorDragX0 = 0;
static S32 TerrainRegionEditorDragZ0 = 0;
static S32 TerrainRegionEditorDragX1 = 0;
static S32 TerrainRegionEditorDragZ1 = 0;
static S32 TerrainRegionEditorDragHalf = -1;
static S32 TerrainRegionEditorDragErase = FALSE;
static S32 TerrainRegionEditorPaletteType = 0;
static S32 TerrainRegionEditorPalettePressType = -1;
static S32 TerrainRegionEditorSelected = -1;
static S32 TerrainRegionEditorPendingX0 = -1;
static S32 TerrainRegionEditorPendingZ0 = -1;
static S32 TerrainRegionEditorPendingX1 = -1;
static S32 TerrainRegionEditorPendingZ1 = -1;
static S32 TerrainRegionEditorPendingSelected = -1;
static S32 TerrainTextureEditorPickPending = FALSE;
static S32 TerrainTextureEditorPickShift = FALSE;
static S32 TerrainTextureEditorPickBestCubeX = 0;
static S32 TerrainTextureEditorPickBestCubeY = 0;
static S32 TerrainTextureEditorPickBestIndex = -1;
static S32 TerrainTextureEditorTexDefCount = 0;
static S32 TerrainTextureEditorCubeTexDefCount = 0;
static S32 TerrainTextureEditorCatalogCount = 0;
static S32 TerrainTextureEditorDragActive = FALSE;
static S32 TerrainTextureEditorDragSelectPending = FALSE;
static S32 TerrainTextureEditorDragShift = FALSE;
static S32 TerrainTextureEditorDragX0 = 0;
static S32 TerrainTextureEditorDragY0 = 0;
static S32 TerrainTextureEditorDragX1 = 0;
static S32 TerrainTextureEditorDragY1 = 0;
static S32 TerrainTextureEditorPalettePressEntry = -2;
static U8 TerrainTextureEditorDragVisited[NB_COTE * NB_COTE];

#define TERRAIN_EDITOR_MAX_SELECTED ((NB_COTE + 1) * (NB_COTE + 1))
#define TERRAIN_EDITOR_MAX_EDITS 16384 * 4
#define TERRAIN_EDITOR_PICK_RADIUS 18
#define TERRAIN_EDITOR_NUDGE 256
#define TERRAIN_EDITOR_NUDGE_SHIFT (5 * TERRAIN_EDITOR_NUDGE)
#define TERRAIN_EDITOR_JITTER 128
#define TERRAIN_EDITOR_JITTER_SHIFT 512
#define TERRAIN_EDITOR_TEXT 0
#define TERRAIN_EDITOR_BACK 14
#define TERRAIN_OVERLAY_TEXT 15
#define TERRAIN_OVERLAY_BACK 0
#define TERRAIN_EDITOR_DRAG_MARK 15
#define TERRAIN_EDITOR_DRAG_MIN 4
#define TERRAIN_TEXTURE_EDITOR_MAX_SELECTED (TERRAIN_EDITOR_MAX_SELECTED * 2)
#define TERRAIN_TEXTURE_EDITOR_MAX_EDITS 65536
#define TERRAIN_TEXTURE_INTENSITY_MAX_EDITS 65536
#define TERRAIN_TEXTURE_EDITOR_MARK 12
#define TERRAIN_TEXTURE_EDITOR_BOUNDS_LIMIT 256
#define TERRAIN_TEXTURE_DRAG_MARK 15
#define TERRAIN_TEXTURE_DRAG_MIN 4
#define TERRAIN_TEXTURE_PALETTE_SWATCH 16
#define TERRAIN_TEXTURE_PALETTE_GAP 3
#define TERRAIN_TEXTURE_PALETTE_MIN_COLS 4
#define TERRAIN_TEXTURE_PALETTE_MAX_COLS 8
#define TERRAIN_TEXTURE_PALETTE_PAD 4
#define TERRAIN_TEXTURE_PALETTE_MAX_WIDTH_DIV 2
#define TERRAIN_TEXTURE_PALETTE_BACK 0
#define TERRAIN_TEXTURE_PALETTE_BORDER 15
#define TERRAIN_TEXTURE_PALETTE_SELECT 12
#define TERRAIN_TEXTURE_PALETTE_NONE -2
#define TERRAIN_TEXTURE_PALETTE_DEAD -3
#define TERRAIN_TEXTURE_MAX_TEXDEFS 8192
#define TERRAIN_REGION_MAX 128
#define TERRAIN_REGION_CELL_MAX 32768
#define TERRAIN_REGION_BASE_MAX (TERRAIN_REGION_CELL_MAX * 2)
#define TERRAIN_REGION_PREVIEW_TILE 8
#define TERRAIN_REGION_PALETTE_SWATCH (TERRAIN_REGION_PREVIEW_TILE * 4)
#define TERRAIN_REGION_PALETTE_GAP 3
#define TERRAIN_REGION_MARK 10
#define TERRAIN_REGION_SELECTED_MARK 12
#define TERRAIN_EDITOR_HQR_START_CUBE 3
#define TERRAIN_EDITOR_HQR_CUBE_TXD 3
#define TERRAIN_EDITOR_HQR_STEP_CUBE 6

typedef struct
{
    S32 Used;
    S32 CubeX;
    S32 CubeY;
    S32 Index;
} T_TERRAIN_EDITOR_SEL;

typedef struct
{
    S32 Used;
    S32 CubeX;
    S32 CubeY;
    S32 Index;
    S16 BaseY;
    S32 HasExpectY;
    S32 Dy;
    S32 SkipLogged;
} T_TERRAIN_EDITOR_EDIT;

typedef struct
{
    S32 Used;
    S32 CubeX;
    S32 CubeY;
    S32 Index;
} T_TERRAIN_TEXTURE_EDITOR_SEL;

typedef struct
{
    S32 Used;
    S32 CubeX;
    S32 CubeY;
    S32 Index;
    T_HALF_POLY BasePoly;
    T_HALF_POLY Poly;
    S32 HasExpectPoly;
    S32 SkipLogged;
} T_TERRAIN_TEXTURE_EDITOR_EDIT;

typedef struct
{
    S32 Used;
    S32 CubeX;
    S32 CubeY;
    S32 Index;
    U8 BaseIntensity;
    U8 Intensity;
    S32 HasExpectIntensity;
    S32 SkipLogged;
} T_TERRAIN_TEXTURE_INTENSITY_EDIT;

typedef struct
{
    S32 Used;
    S32 CubeX;
    S32 CubeY;
    S32 X0;
    S32 Z0;
    S32 X1;
    S32 Z1;
    S32 Type;
    S32 Seed;
    S32 Patchiness;
    S32 AnchorX;
    S32 AnchorZ;
} T_TERRAIN_REGION;

typedef struct
{
    S32 Used;
    S32 CubeX;
    S32 CubeY;
    S32 X;
    S32 Z;
    S32 Half;
    S32 Type;
    S32 Region;
} T_TERRAIN_REGION_CELL;

typedef struct
{
    S32 Used;
    S32 CubeX;
    S32 CubeY;
    S32 Index;
    T_HALF_POLY Poly;
} T_TERRAIN_REGION_BASE;

static T_TERRAIN_EDITOR_SEL TerrainEditorSelList[TERRAIN_EDITOR_MAX_SELECTED];
static T_TERRAIN_EDITOR_EDIT TerrainEditorEdits[TERRAIN_EDITOR_MAX_EDITS];
static T_TERRAIN_TEXTURE_EDITOR_SEL TerrainTextureEditorSelList[TERRAIN_TEXTURE_EDITOR_MAX_SELECTED];
static T_TERRAIN_TEXTURE_EDITOR_EDIT TerrainTextureEditorEdits[TERRAIN_TEXTURE_EDITOR_MAX_EDITS];
static T_TERRAIN_TEXTURE_INTENSITY_EDIT TerrainTextureIntensityEdits[TERRAIN_TEXTURE_INTENSITY_MAX_EDITS];
static T_TERRAIN_REGION TerrainRegions[TERRAIN_REGION_MAX];
static T_TERRAIN_REGION_CELL TerrainRegionCells[TERRAIN_REGION_CELL_MAX];
static T_TERRAIN_REGION_BASE TerrainRegionBases[TERRAIN_REGION_BASE_MAX];
static T_HALF_TEX TerrainTextureEditorCatalog[TERRAIN_TEXTURE_MAX_TEXDEFS];
static T_HALF_TEX TerrainTextureEditorLoadedTexDefs[TERRAIN_TEXTURE_MAX_TEXDEFS];

static void TerrainTextureEditorApplyTextureSelected(S32 texture_index);
static S32 TerrainTextureEditorSelectedTextureEntry(void);
static S32 TerrainTextureEditorPaletteTextureCount(void);
static void TerrainTextureEditorAddSelectionGroup(S32 cube_x, S32 cube_y, S32 index);

void TerrainEditorSetLoadedCube(S32 cube_x, S32 cube_y) {
    TerrainEditorLoadedCubeX = cube_x;
    TerrainEditorLoadedCubeY = cube_y;
}

static S32 TerrainEditorLoadedCurrentCube(void) {
    return TerrainEditorLoadedCubeX == CurrentCubeX AND TerrainEditorLoadedCubeY == CurrentCubeY;
}

static S32 TerrainEditorSamePoint(T_TERRAIN_EDITOR_SEL *sel, S32 cube_x, S32 cube_y, S32 index) {
    return sel->Used
               AND sel->CubeX == cube_x AND sel->CubeY == cube_y AND sel->Index == index;
}

static S32 TerrainEditorIsSelected(S32 cube_x, S32 cube_y, S32 index) {
    S32 i;

    for (i = 0; i < TERRAIN_EDITOR_MAX_SELECTED; i++) {
        if (TerrainEditorSamePoint(&TerrainEditorSelList[i], cube_x, cube_y, index))
            return TRUE;
    }
    return FALSE;
}

void TerrainEditorClearSelection(void) {
    memset(TerrainEditorSelList, 0, sizeof(TerrainEditorSelList));
}

void TerrainEditorSetSelect(S32 select) {
    TerrainEditorSelect = select ? TRUE : FALSE;
    if (!TerrainEditorSelect) {
        TerrainEditorClearSelection();
        TerrainEditorDragActive = FALSE;
        TerrainEditorDragSelectPending = FALSE;
    }
    TerrainEditorLastClick = Click;
    TerrainEditorLastUp = CheckKey(K_UP) ? TRUE : FALSE;
    TerrainEditorLastDown = CheckKey(K_DOWN) ? TRUE : FALSE;
    TerrainEditorLastSelectAll = (CheckKey(K_A) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
    TerrainEditorLastFlattenZero = (CheckKey(K_0) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
    TerrainEditorLastJitter = CheckKey(K_J) ? TRUE : FALSE;
    TerrainEditorLastTransition = CheckKey(K_T) ? TRUE : FALSE;
    ShowMouse(TerrainEditorSelect ? true : false);
}

S32 TerrainEditorGetSelect(void) {
    return TerrainEditorSelect;
}

S32 TerrainEditorConsumesInput(void) {
    return TerrainEditorSelect;
}

void TerrainEditorClearRuntimeEdits(void) {
    memset(TerrainEditorEdits, 0, sizeof(TerrainEditorEdits));
    memset(TerrainRegions, 0, sizeof(TerrainRegions));
    memset(TerrainRegionCells, 0, sizeof(TerrainRegionCells));
    memset(TerrainRegionBases, 0, sizeof(TerrainRegionBases));
    TerrainRegionEditorSelected = -1;
    TerrainEditorClearSelection();
}

static T_TERRAIN_EDITOR_EDIT *TerrainEditorFindEdit(S32 cube_x, S32 cube_y, S32 index) {
    S32 i;

    for (i = 0; i < TERRAIN_EDITOR_MAX_EDITS; i++) {
        T_TERRAIN_EDITOR_EDIT *edit = &TerrainEditorEdits[i];

        if (edit->Used
                AND edit->CubeX == cube_x AND edit->CubeY == cube_y AND edit->Index == index)
            return edit;
    }
    return NULL;
}

static T_TERRAIN_EDITOR_EDIT *TerrainEditorGetOrCreateEdit(S32 cube_x, S32 cube_y, S32 index, S16 *height) {
    S32 i;
    T_TERRAIN_EDITOR_EDIT *edit;

    edit = TerrainEditorFindEdit(cube_x, cube_y, index);
    if (edit)
        return edit;

    for (i = 0; i < TERRAIN_EDITOR_MAX_EDITS; i++) {
        edit = &TerrainEditorEdits[i];
        if (!edit->Used) {
            edit->Used = TRUE;
            edit->CubeX = cube_x;
            edit->CubeY = cube_y;
            edit->Index = index;
            edit->BaseY = *height;
            edit->HasExpectY = TRUE;
            edit->Dy = 0;
            edit->SkipLogged = FALSE;
            return edit;
        }
    }

    Log_Warn("teredit runtime edit table is full; cannot track c%d,%d v%d",
             (int)cube_x,
             (int)cube_y,
             (int)index);
    return NULL;
}

static void TerrainEditorApplyEditToPoint(T_TERRAIN_EDITOR_EDIT *edit, S16 *height) {
    S32 y;

    y = (S32)edit->BaseY + edit->Dy;
    if (y < -32768)
        y = -32768;
    if (y > 32767)
        y = 32767;
    *height = (S16)y;
}

void TerrainEditorApplyRuntimeEditsForLoadedCube(S32 cube_x, S32 cube_y) {
    S32 i;

    if (!MapSommetY)
        return;

    for (i = 0; i < TERRAIN_EDITOR_MAX_EDITS; i++) {
        T_TERRAIN_EDITOR_EDIT *edit = &TerrainEditorEdits[i];

        if (!edit->Used
                 OR edit->CubeX != cube_x OR edit->CubeY != cube_y OR edit->Index < 0 OR edit->Index >= (NB_COTE + 1) * (NB_COTE + 1))
            continue;
        if (edit->HasExpectY AND MapSommetY[edit->Index] != edit->BaseY) {
            if (!edit->SkipLogged) {
                Log_Warn("[editor] terrain edit skipped c%d,%d v%d: height %d does not match expected %d",
                         (int)cube_x,
                         (int)cube_y,
                         (int)edit->Index,
                         (int)MapSommetY[edit->Index],
                         (int)edit->BaseY);
                edit->SkipLogged = TRUE;
            }
            continue;
        }
        TerrainEditorApplyEditToPoint(edit, &MapSommetY[edit->Index]);
    }
}

static T_TERRAIN_EDITOR_EDIT *TerrainEditorImportedEdit(S32 cube_x, S32 cube_y, S32 index) {
    S32 i;
    T_TERRAIN_EDITOR_EDIT *edit;

    edit = TerrainEditorFindEdit(cube_x, cube_y, index);
    if (edit)
        return edit;

    for (i = 0; i < TERRAIN_EDITOR_MAX_EDITS; i++) {
        edit = &TerrainEditorEdits[i];
        if (!edit->Used) {
            memset(edit, 0, sizeof(*edit));
            edit->Used = TRUE;
            edit->CubeX = cube_x;
            edit->CubeY = cube_y;
            edit->Index = index;
            return edit;
        }
    }

    Log_Warn("[editor] terrain import skipped c%d,%d v%d: runtime edit table is full",
             (int)cube_x,
             (int)cube_y,
             (int)index);
    return NULL;
}

static S32 TerrainEditorImportHeight(const char *json,
                                     jsmntok_t *tokens,
                                     S32 count,
                                     S32 cube_x,
                                     S32 cube_y,
                                     S32 point,
                                     S32 *skipped) {
    S32 x;
    S32 z;
    S32 height;
    S32 expect;
    S32 has_expect;
    S32 index;
    T_TERRAIN_EDITOR_EDIT *edit;

    if (!EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, point, "x"), &x)
            OR !EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, point, "z"), &z)
            OR !EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, point, "height"), &height)) {
        (*skipped)++;
        return FALSE;
    }
    if (x < 0 OR x > NB_COTE OR z < 0 OR z > NB_COTE) {
        (*skipped)++;
        return FALSE;
    }
    if (height < -32768 OR height > 32767) {
        (*skipped)++;
        return FALSE;
    }
    has_expect = EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, point, "expect"), &expect);
    if (has_expect AND(expect < -32768 OR expect > 32767)) {
        (*skipped)++;
        return FALSE;
    }

    index = z * (NB_COTE + 1) + x;
    edit = TerrainEditorImportedEdit(cube_x, cube_y, index);
    if (!edit) {
        (*skipped)++;
        return FALSE;
    }

    edit->HasExpectY = has_expect;
    edit->BaseY = (S16)(edit->HasExpectY ? expect : height);
    edit->Dy = height - edit->BaseY;
    edit->SkipLogged = FALSE;
    return TRUE;
}

static void TerrainTextureEditorSetPolyField(T_HALF_POLY *poly, const char *json, jsmntok_t *tokens, S32 count, S32 object, const char *key, S32 *present) {
    S32 value;
    const char *field;

    if (!EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, object, key), &value))
        return;
    *present = TRUE;
    field = strncmp(key, "expect_", 7) == 0 ? key + 7 : key;
    if (strcmp(field, "bank") == 0)
        poly->Bank = (U32)value;
    else if (strcmp(field, "tex_flag") == 0)
        poly->TexFlag = (U32)value;
    else if (strcmp(field, "poly_flag") == 0)
        poly->PolyFlag = (U32)value;
    else if (strcmp(field, "sample_step") == 0)
        poly->SampleStep = (U32)value;
    else if (strcmp(field, "code_jeu") == 0)
        poly->CodeJeu = (U32)value;
    else if (strcmp(field, "sens") == 0)
        poly->Sens = (U32)value;
    else if (strcmp(field, "col") == 0)
        poly->Col = (U32)value;
    else if (strcmp(field, "dummy") == 0)
        poly->Dummy = (U32)value;
    else if (strcmp(field, "index_tex") == 0)
        poly->IndexTex = (U32)value;
}

static T_TERRAIN_TEXTURE_EDITOR_EDIT *TerrainTextureEditorFindEdit(S32 cube_x, S32 cube_y, S32 index);
static T_TERRAIN_TEXTURE_INTENSITY_EDIT *TerrainTextureEditorFindIntensityEdit(S32 cube_x, S32 cube_y, S32 index);

static T_TERRAIN_TEXTURE_EDITOR_EDIT *TerrainTextureEditorImportedEdit(S32 cube_x, S32 cube_y, S32 index) {
    S32 i;
    T_TERRAIN_TEXTURE_EDITOR_EDIT *edit;

    edit = TerrainTextureEditorFindEdit(cube_x, cube_y, index);
    if (edit)
        return edit;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_EDITS; i++) {
        edit = &TerrainTextureEditorEdits[i];
        if (!edit->Used) {
            memset(edit, 0, sizeof(*edit));
            edit->Used = TRUE;
            edit->CubeX = cube_x;
            edit->CubeY = cube_y;
            edit->Index = index;
            return edit;
        }
    }

    Log_Warn("[editor] texture import skipped c%d,%d t%d: runtime edit table is full",
             (int)cube_x,
             (int)cube_y,
             (int)index);
    return NULL;
}

static S32 TerrainTextureEditorImportTriangle(const char *json,
                                              jsmntok_t *tokens,
                                              S32 count,
                                              S32 cube_x,
                                              S32 cube_y,
                                              S32 triangle,
                                              S32 *skipped) {
    S32 index;
    S32 present = FALSE;
    S32 expect_present = FALSE;
    T_TERRAIN_TEXTURE_EDITOR_EDIT *edit;
    T_HALF_POLY poly;
    T_HALF_POLY expect_poly;

    memset(&poly, 0, sizeof(poly));
    memset(&expect_poly, 0, sizeof(expect_poly));

    if (!EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, triangle, "triangle"), &index)) {
        (*skipped)++;
        return FALSE;
    }
    if (index < 0 OR index >= NB_COTE * NB_COTE * 2) {
        (*skipped)++;
        return FALSE;
    }

    TerrainTextureEditorSetPolyField(&poly, json, tokens, count, triangle, "bank", &present);
    TerrainTextureEditorSetPolyField(&poly, json, tokens, count, triangle, "tex_flag", &present);
    TerrainTextureEditorSetPolyField(&poly, json, tokens, count, triangle, "poly_flag", &present);
    TerrainTextureEditorSetPolyField(&poly, json, tokens, count, triangle, "sample_step", &present);
    TerrainTextureEditorSetPolyField(&poly, json, tokens, count, triangle, "code_jeu", &present);
    TerrainTextureEditorSetPolyField(&poly, json, tokens, count, triangle, "sens", &present);
    TerrainTextureEditorSetPolyField(&poly, json, tokens, count, triangle, "col", &present);
    TerrainTextureEditorSetPolyField(&poly, json, tokens, count, triangle, "dummy", &present);
    TerrainTextureEditorSetPolyField(&poly, json, tokens, count, triangle, "index_tex", &present);

    TerrainTextureEditorSetPolyField(&expect_poly, json, tokens, count, triangle, "expect_bank", &expect_present);
    TerrainTextureEditorSetPolyField(&expect_poly, json, tokens, count, triangle, "expect_tex_flag", &expect_present);
    TerrainTextureEditorSetPolyField(&expect_poly, json, tokens, count, triangle, "expect_poly_flag", &expect_present);
    TerrainTextureEditorSetPolyField(&expect_poly, json, tokens, count, triangle, "expect_sample_step", &expect_present);
    TerrainTextureEditorSetPolyField(&expect_poly, json, tokens, count, triangle, "expect_code_jeu", &expect_present);
    TerrainTextureEditorSetPolyField(&expect_poly, json, tokens, count, triangle, "expect_sens", &expect_present);
    TerrainTextureEditorSetPolyField(&expect_poly, json, tokens, count, triangle, "expect_col", &expect_present);
    TerrainTextureEditorSetPolyField(&expect_poly, json, tokens, count, triangle, "expect_dummy", &expect_present);
    TerrainTextureEditorSetPolyField(&expect_poly, json, tokens, count, triangle, "expect_index_tex", &expect_present);

    if (!present) {
        (*skipped)++;
        return FALSE;
    }

    edit = TerrainTextureEditorImportedEdit(cube_x, cube_y, index);
    if (!edit) {
        (*skipped)++;
        return FALSE;
    }

    edit->Poly = poly;
    edit->BasePoly = expect_poly;
    edit->HasExpectPoly = expect_present;
    edit->SkipLogged = FALSE;
    return TRUE;
}

static T_TERRAIN_TEXTURE_INTENSITY_EDIT *TerrainTextureEditorImportedIntensityEdit(S32 cube_x, S32 cube_y, S32 index) {
    S32 i;
    T_TERRAIN_TEXTURE_INTENSITY_EDIT *edit;

    edit = TerrainTextureEditorFindIntensityEdit(cube_x, cube_y, index);
    if (edit)
        return edit;

    for (i = 0; i < TERRAIN_TEXTURE_INTENSITY_MAX_EDITS; i++) {
        edit = &TerrainTextureIntensityEdits[i];
        if (!edit->Used) {
            memset(edit, 0, sizeof(*edit));
            edit->Used = TRUE;
            edit->CubeX = cube_x;
            edit->CubeY = cube_y;
            edit->Index = index;
            return edit;
        }
    }

    Log_Warn("[editor] intensity import skipped c%d,%d i%d: runtime edit table is full",
             (int)cube_x,
             (int)cube_y,
             (int)index);
    return NULL;
}

static S32 TerrainTextureEditorImportIntensity(const char *json,
                                               jsmntok_t *tokens,
                                               S32 count,
                                               S32 cube_x,
                                               S32 cube_y,
                                               S32 point,
                                               S32 *skipped) {
    S32 x;
    S32 z;
    S32 intensity;
    S32 expect;
    S32 has_expect;
    S32 index;
    T_TERRAIN_TEXTURE_INTENSITY_EDIT *edit;

    if (!EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, point, "x"), &x)
            OR !EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, point, "z"), &z)
            OR !EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, point, "intensity"), &intensity)) {
        (*skipped)++;
        return FALSE;
    }
    if (x < 0 OR x > NB_COTE OR z < 0 OR z > NB_COTE OR intensity < 0 OR intensity > 255) {
        (*skipped)++;
        return FALSE;
    }
    has_expect = EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, point, "expect"), &expect);
    if (has_expect AND(expect < 0 OR expect > 255)) {
        (*skipped)++;
        return FALSE;
    }

    index = z * (NB_COTE + 1) + x;
    edit = TerrainTextureEditorImportedIntensityEdit(cube_x, cube_y, index);
    if (!edit) {
        (*skipped)++;
        return FALSE;
    }

    edit->HasExpectIntensity = has_expect;
    edit->BaseIntensity = (U8)(has_expect ? expect : 0);
    edit->Intensity = (U8)intensity;
    edit->SkipLogged = FALSE;
    return TRUE;
}

static void TerrainEditorImportCube(const char *json,
                                    jsmntok_t *tokens,
                                    S32 count,
                                    S32 cube,
                                    S32 *loaded,
                                    S32 *skipped) {
    S32 cube_xy[2];
    S32 terrain;
    S32 heights;
    S32 heights_end;
    S32 triangles;
    S32 triangles_end;
    S32 intensities;
    S32 intensities_end;
    S32 point;

    if (!EditorJsonParseIntArray(json, tokens, count, EditorJsonFindValue(json, tokens, count, cube, "cube"), cube_xy, 2)) {
        (*skipped)++;
        return;
    }

    terrain = EditorJsonFindValue(json, tokens, count, cube, "terrain");
    if (terrain < 0)
        terrain = cube;
    heights = EditorJsonFindValue(json, tokens, count, terrain, "heights");
    if (heights >= 0 AND tokens[heights].type == JSMN_ARRAY) {
        heights_end = EditorJsonTokenEnd(tokens, count, heights);
        for (point = heights + 1; point < heights_end; point = EditorJsonTokenEnd(tokens, count, point)) {
            if (tokens[point].parent != heights)
                continue;
            if (tokens[point].type != JSMN_OBJECT) {
                (*skipped)++;
                continue;
            }
            if (TerrainEditorImportHeight(json, tokens, count, cube_xy[0], cube_xy[1], point, skipped))
                (*loaded)++;
        }
    }

    triangles = EditorJsonFindValue(json, tokens, count, terrain, "triangles");
    if (triangles >= 0 AND tokens[triangles].type == JSMN_ARRAY) {
        triangles_end = EditorJsonTokenEnd(tokens, count, triangles);
        for (point = triangles + 1; point < triangles_end; point = EditorJsonTokenEnd(tokens, count, point)) {
            if (tokens[point].parent != triangles)
                continue;
            if (tokens[point].type != JSMN_OBJECT) {
                (*skipped)++;
                continue;
            }
            if (TerrainTextureEditorImportTriangle(json, tokens, count, cube_xy[0], cube_xy[1], point, skipped))
                (*loaded)++;
        }
    }

    intensities = EditorJsonFindValue(json, tokens, count, terrain, "intensities");
    if (intensities >= 0 AND tokens[intensities].type == JSMN_ARRAY) {
        intensities_end = EditorJsonTokenEnd(tokens, count, intensities);
        for (point = intensities + 1; point < intensities_end; point = EditorJsonTokenEnd(tokens, count, point)) {
            if (tokens[point].parent != intensities)
                continue;
            if (tokens[point].type != JSMN_OBJECT) {
                (*skipped)++;
                continue;
            }
            if (TerrainTextureEditorImportIntensity(json, tokens, count, cube_xy[0], cube_xy[1], point, skipped))
                (*loaded)++;
        }
    }
}

static void TerrainRegionEditorImportRegions(const char *json, jsmntok_t *tokens, S32 count, S32 *loaded, S32 *skipped) {
    S32 regions = EditorJsonFindValue(json, tokens, count, 0, "regions");
    S32 item;
    S32 end;

    if (regions < 0 OR tokens[regions].type != JSMN_ARRAY)
        return;
    end = EditorJsonTokenEnd(tokens, count, regions);
    for (item = regions + 1; item < end; item = EditorJsonTokenEnd(tokens, count, item)) {
        S32 cube[2];
        S32 bounds[4];
        S32 type;
        S32 seed;
        S32 patchiness;
        S32 i;

        if (tokens[item].parent != regions OR tokens[item].type != JSMN_OBJECT)
            continue;
        if (!EditorJsonParseIntArray(json, tokens, count, EditorJsonFindValue(json, tokens, count, item, "cube"), cube, 2)
                OR !EditorJsonParseIntArray(json, tokens, count, EditorJsonFindValue(json, tokens, count, item, "bounds"), bounds, 4)
                OR !EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, item, "type"), &type)
                OR !EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, item, "seed"), &seed)
                OR !EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, item, "patchiness"), &patchiness)
                OR cube[0] < 0 OR cube[0] >= SIZE_MAIN_MAP OR cube[1] < 0 OR cube[1] >= SIZE_MAIN_MAP
                OR bounds[0] < 0 OR bounds[0] >= NB_COTE OR bounds[1] < 0 OR bounds[1] >= NB_COTE
                OR bounds[2] < bounds[0] OR bounds[2] >= NB_COTE OR bounds[3] < bounds[1] OR bounds[3] >= NB_COTE) {
            (*skipped)++;
            continue;
        }
        for (i = 0; i < TERRAIN_REGION_MAX; i++) {
            T_TERRAIN_REGION *region = &TerrainRegions[i];
            if (region->Used)
                continue;
            region->Used = TRUE;
            region->CubeX = cube[0];
            region->CubeY = cube[1];
            region->X0 = bounds[0];
            region->Z0 = bounds[1];
            region->X1 = bounds[2];
            region->Z1 = bounds[3];
            region->Type = type >= 0 AND type < TERRAIN_REGION_TYPE_COUNT ? type : TERRAIN_REGION_TYPE_GRASS;
            region->Seed = seed;
            region->Patchiness = patchiness < 0 ? 0 : (patchiness > 100 ? 100 : patchiness);
            region->AnchorX = region->X0;
            region->AnchorZ = region->Z0;
            (*loaded)++;
            break;
        }
    }
}

static S32 TerrainRegionEditorImportCells(const char *json, jsmntok_t *tokens, S32 count, S32 *loaded, S32 *skipped) {
    S32 cells = EditorJsonFindValue(json, tokens, count, 0, "auto_cells");
    S32 item;
    S32 end;
    S32 imported = 0;

    if (cells < 0 OR tokens[cells].type != JSMN_ARRAY)
        return 0;
    end = EditorJsonTokenEnd(tokens, count, cells);
    for (item = cells + 1; item < end; item = EditorJsonTokenEnd(tokens, count, item)) {
        S32 values[7];
        S32 i;
        T_TERRAIN_REGION_CELL *cell = NULL;

        if (tokens[item].parent != cells
                OR !EditorJsonParseIntArray(json, tokens, count, item, values, 7)
                OR values[0] < 0 OR values[0] >= SIZE_MAIN_MAP OR values[1] < 0 OR values[1] >= SIZE_MAIN_MAP
                OR values[2] < 0 OR values[2] >= NB_COTE OR values[3] < 0 OR values[3] >= NB_COTE
                                                                                                                                                                                                                 OR values[4] < -1 OR values[4] > 1 OR values[5] < 0 OR values[5] >= TERRAIN_REGION_TYPE_COUNT) {
            (*skipped)++;
            continue;
        }
        for (i = 0; i < TERRAIN_REGION_CELL_MAX; i++) {
            if (!TerrainRegionCells[i].Used) {
                cell = &TerrainRegionCells[i];
                break;
            }
        }
        if (!cell)
            break;
        cell->Used = TRUE;
        cell->CubeX = values[0];
        cell->CubeY = values[1];
        cell->X = values[2];
        cell->Z = values[3];
        cell->Half = values[4];
        cell->Type = values[5];
        cell->Region = values[6];
        imported++;
        (*loaded)++;
    }
    return imported;
}

static void TerrainRegionEditorImportBases(const char *json, jsmntok_t *tokens, S32 count, S32 *loaded, S32 *skipped) {
    S32 bases = EditorJsonFindValue(json, tokens, count, 0, "auto_bases");
    S32 item;
    S32 end;

    if (bases < 0 OR tokens[bases].type != JSMN_ARRAY)
        return;
    end = EditorJsonTokenEnd(tokens, count, bases);
    for (item = bases + 1; item < end; item = EditorJsonTokenEnd(tokens, count, item)) {
        S32 values[12];
        S32 i;
        T_TERRAIN_REGION_BASE *base = NULL;

        if (tokens[item].parent != bases
                OR !EditorJsonParseIntArray(json, tokens, count, item, values, 12)
                OR values[0] < 0 OR values[0] >= SIZE_MAIN_MAP OR values[1] < 0 OR values[1] >= SIZE_MAIN_MAP
                OR values[2] < 0 OR values[2] >= NB_COTE * NB_COTE * 2) {
            (*skipped)++;
            continue;
        }
        for (i = 0; i < TERRAIN_REGION_BASE_MAX; i++) {
            if (!TerrainRegionBases[i].Used) {
                base = &TerrainRegionBases[i];
                break;
            }
        }
        if (!base)
            break;
        base->Used = TRUE;
        base->CubeX = values[0];
        base->CubeY = values[1];
        base->Index = values[2];
        base->Poly.Bank = (U32)values[3];
        base->Poly.TexFlag = (U32)values[4];
        base->Poly.PolyFlag = (U32)values[5];
        base->Poly.SampleStep = (U32)values[6];
        base->Poly.CodeJeu = (U32)values[7];
        base->Poly.Sens = (U32)values[8];
        base->Poly.Col = (U32)values[9];
        base->Poly.Dummy = (U32)values[10];
        base->Poly.IndexTex = (U32)values[11];
        (*loaded)++;
    }
}

static void TerrainRegionEditorMigrateRectangleRegions(void) {
    S32 i;
    S32 cell_index = 0;
    for (i = 0; i < TERRAIN_REGION_MAX; i++) {
        T_TERRAIN_REGION *region = &TerrainRegions[i];
        S32 x;
        S32 z;
        if (!region->Used)
            continue;
        for (z = region->Z0; z <= region->Z1; z++) {
            for (x = region->X0; x <= region->X1 AND cell_index < TERRAIN_REGION_CELL_MAX; x++) {
                T_TERRAIN_REGION_CELL *cell = &TerrainRegionCells[cell_index++];
                cell->Used = TRUE;
                cell->CubeX = region->CubeX;
                cell->CubeY = region->CubeY;
                cell->X = x;
                cell->Z = z;
                cell->Half = -1;
                cell->Type = region->Type;
                cell->Region = i;
            }
        }
    }
}

void TerrainEditorLoadAuthoringEditsForCurrentIsland(void) {
    char dir[ADELINE_MAX_PATH];
    char path[ADELINE_MAX_PATH];
    char *json;
    jsmntok_t *tokens;
    S32 count;
    S32 island_value;
    S32 cubes;
    S32 cube;
    S32 loaded = 0;
    S32 skipped = 0;

    if (!Editor_ResolveOutputDir(dir, sizeof(dir)))
        return;
    snprintf(path, sizeof(path), "%s%sterrain_edits.json", dir, ADELINE_PATH_SEP);
    path[sizeof(path) - 1] = '\0';

    if (!EditorJsonParseFile(path, &json, &tokens, &count)) {
        if (EditorJsonFileExists(path))
            Log_Warn("[editor] cannot load terrain edits: invalid JSON: %s", path);
        return;
    }

    if (EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, 0, "island"), &island_value)
            AND island_value != Island) {
        Log_Warn("[editor] terrain edits ignored: %s is for island %d, current island is %d",
                 path,
                 (int)island_value,
                 (int)Island);
        free(tokens);
        free(json);
        return;
    }

    cubes = EditorJsonFindValue(json, tokens, count, 0, "cubes");
    if (cubes >= 0 AND tokens[cubes].type == JSMN_ARRAY) {
        S32 cubes_end = EditorJsonTokenEnd(tokens, count, cubes);
        for (cube = cubes + 1; cube < cubes_end; cube = EditorJsonTokenEnd(tokens, count, cube)) {
            if (tokens[cube].parent != cubes)
                continue;
            if (tokens[cube].type != JSMN_OBJECT) {
                skipped++;
                continue;
            }
            TerrainEditorImportCube(json, tokens, count, cube, &loaded, &skipped);
        }
    } else {
        TerrainEditorImportCube(json, tokens, count, 0, &loaded, &skipped);
    }
    TerrainRegionEditorImportRegions(json, tokens, count, &loaded, &skipped);
    if (!TerrainRegionEditorImportCells(json, tokens, count, &loaded, &skipped))
        TerrainRegionEditorMigrateRectangleRegions();
    TerrainRegionEditorImportBases(json, tokens, count, &loaded, &skipped);

    Log_Info("[editor] loaded terrain edits: %s (loaded=%d skipped=%d)", path, (int)loaded, (int)skipped);
    free(tokens);
    free(json);
}

S32 TerrainEditorSaveRuntimeEdits(const char *island_dir, S32 island) {
    char path[ADELINE_MAX_PATH];
    FILE *f;
    S32 cube_x;
    S32 cube_y;
    S32 count = 0;
    S32 first_cube = TRUE;

    if (!island_dir OR !island_dir[0])
        return FALSE;

    snprintf(path, sizeof(path), "%s%sterrain_edits.json", island_dir, ADELINE_PATH_SEP);
    path[sizeof(path) - 1] = '\0';

    f = fopen(path, "wb");
    if (!f) {
        Editor_SetStatus("Terrain save failed: cannot open terrain_edits.json");
        Log_Warn("[editor] cannot write terrain edits: %s", path);
        return FALSE;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"island\": %d,\n", (int)island);
    fprintf(f, "  \"description\": \"Authoring edits exported by the terrain editor.\",\n");
    fprintf(f, "  \"cubes\": [\n");

    for (cube_y = 0; cube_y < 16; cube_y++) {
        for (cube_x = 0; cube_x < 16; cube_x++) {
        S32 i;
        S32 n;
        S32 has_cube = FALSE;
        S32 x;
        S32 z;
        S32 first_height = TRUE;
        S32 first_triangle = TRUE;
        S32 first_intensity = TRUE;

        for (i = 0; i < TERRAIN_EDITOR_MAX_EDITS; i++) {
            if (TerrainEditorEdits[i].Used
                        AND TerrainEditorEdits[i]
                            .CubeX == cube_x AND TerrainEditorEdits[i].CubeY == cube_y)
                has_cube = TRUE;
        }
        for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_EDITS; i++) {
            if (TerrainTextureEditorEdits[i].Used
                        AND TerrainTextureEditorEdits[i]
                            .CubeX == cube_x AND TerrainTextureEditorEdits[i].CubeY == cube_y)
                has_cube = TRUE;
        }
        for (i = 0; i < TERRAIN_TEXTURE_INTENSITY_MAX_EDITS; i++) {
            if (TerrainTextureIntensityEdits[i].Used
                        AND TerrainTextureIntensityEdits[i]
                            .CubeX == cube_x AND TerrainTextureIntensityEdits[i].CubeY == cube_y)
                has_cube = TRUE;
        }
        if (!has_cube)
            continue;

        if (!first_cube)
            fprintf(f, ",\n");
        first_cube = FALSE;

        fprintf(f, "    {\n");
        fprintf(f, "      \"cube\": [%d, %d],\n", (int)cube_x, (int)cube_y);
        fprintf(f, "      \"terrain\": {\n");
        fprintf(f, "        \"heights\": [\n");

        for (n = 0; n < TERRAIN_EDITOR_MAX_EDITS; n++) {
            T_TERRAIN_EDITOR_EDIT *other = &TerrainEditorEdits[n];
            S32 height;

            if (!other->Used
                         OR other->CubeX != cube_x OR other->CubeY != cube_y)
                continue;

            x = other->Index % (NB_COTE + 1);
            z = other->Index / (NB_COTE + 1);
            height = (S32)other->BaseY + other->Dy;
            if (height < -32768)
                height = -32768;
            if (height > 32767)
                height = 32767;

            if (!first_height)
                fprintf(f, ",\n");
            first_height = FALSE;
            count++;

            fprintf(f, "          {\"x\": %d, \"z\": %d, ", (int)x, (int)z);
            if (other->HasExpectY)
                fprintf(f, "\"expect\": %d, ", (int)other->BaseY);
            fprintf(f, "\"height\": %d}", (int)height);
        }

        fprintf(f, "\n");
        fprintf(f, "        ],\n");
        fprintf(f, "        \"triangles\": [\n");

        for (n = 0; n < TERRAIN_TEXTURE_EDITOR_MAX_EDITS; n++) {
            T_TERRAIN_TEXTURE_EDITOR_EDIT *other = &TerrainTextureEditorEdits[n];

            if (!other->Used
                         OR other->CubeX != cube_x OR other->CubeY != cube_y)
                continue;

            if (!first_triangle)
                fprintf(f, ",\n");
            first_triangle = FALSE;
            count++;

            fprintf(f, "          {\"triangle\": %d, ", (int)other->Index);
            if (other->HasExpectPoly) {
                fprintf(f,
                        "\"expect_bank\": %d, \"expect_tex_flag\": %d, \"expect_poly_flag\": %d, \"expect_sample_step\": %d, \"expect_code_jeu\": %d, \"expect_sens\": %d, \"expect_col\": %d, \"expect_dummy\": %d, \"expect_index_tex\": %d, ",
                        (int)other->BasePoly.Bank,
                        (int)other->BasePoly.TexFlag,
                        (int)other->BasePoly.PolyFlag,
                        (int)other->BasePoly.SampleStep,
                        (int)other->BasePoly.CodeJeu,
                        (int)other->BasePoly.Sens,
                        (int)other->BasePoly.Col,
                        (int)other->BasePoly.Dummy,
                        (int)other->BasePoly.IndexTex);
            }
            fprintf(f,
                    "\"bank\": %d, \"tex_flag\": %d, \"poly_flag\": %d, \"sample_step\": %d, \"code_jeu\": %d, \"sens\": %d, \"col\": %d, \"dummy\": %d, \"index_tex\": %d}",
                    (int)other->Poly.Bank,
                    (int)other->Poly.TexFlag,
                    (int)other->Poly.PolyFlag,
                    (int)other->Poly.SampleStep,
                    (int)other->Poly.CodeJeu,
                    (int)other->Poly.Sens,
                    (int)other->Poly.Col,
                    (int)other->Poly.Dummy,
                    (int)other->Poly.IndexTex);
        }

        fprintf(f, "\n");
        fprintf(f, "        ],\n");
        fprintf(f, "        \"intensities\": [\n");

        for (n = 0; n < TERRAIN_TEXTURE_INTENSITY_MAX_EDITS; n++) {
            T_TERRAIN_TEXTURE_INTENSITY_EDIT *other = &TerrainTextureIntensityEdits[n];

            if (!other->Used
                         OR other->CubeX != cube_x OR other->CubeY != cube_y)
                continue;

            x = other->Index % (NB_COTE + 1);
            z = other->Index / (NB_COTE + 1);

            if (!first_intensity)
                fprintf(f, ",\n");
            first_intensity = FALSE;
            count++;

            fprintf(f, "          {\"x\": %d, \"z\": %d, ", (int)x, (int)z);
            if (other->HasExpectIntensity)
                fprintf(f, "\"expect\": %d, ", (int)other->BaseIntensity);
            fprintf(f, "\"intensity\": %d}", (int)other->Intensity);
        }

        fprintf(f, "\n");
        fprintf(f, "        ]\n");
        fprintf(f, "      }\n");
        fprintf(f, "    }");
        }
    }

    fprintf(f, "\n");
    fprintf(f, "  ],\n");
    fprintf(f, "  \"regions\": [\n");
    {
        S32 first_region = TRUE;
        S32 i;
        for (i = 0; i < TERRAIN_REGION_MAX; i++) {
            T_TERRAIN_REGION *region = &TerrainRegions[i];
            if (!region->Used)
                continue;
            if (!first_region)
                fprintf(f, ",\n");
            first_region = FALSE;
            fprintf(f,
                    "    {\"cube\": [%d, %d], \"bounds\": [%d, %d, %d, %d], \"type\": %d, \"seed\": %d, \"patchiness\": %d}",
                    (int)region->CubeX, (int)region->CubeY,
                    (int)region->X0, (int)region->Z0, (int)region->X1, (int)region->Z1,
                    (int)region->Type, (int)region->Seed, (int)region->Patchiness);
            count++;
        }
    }
    fprintf(f, "\n");
    fprintf(f, "  ],\n");
    fprintf(f, "  \"auto_cells\": [\n");
    {
        S32 first_cell = TRUE;
        S32 i;
        for (i = 0; i < TERRAIN_REGION_CELL_MAX; i++) {
            T_TERRAIN_REGION_CELL *cell = &TerrainRegionCells[i];
            if (!cell->Used)
                continue;
            if (!first_cell)
                fprintf(f, ",\n");
            first_cell = FALSE;
            fprintf(f, "    [%d, %d, %d, %d, %d, %d, %d]",
                    (int)cell->CubeX, (int)cell->CubeY, (int)cell->X, (int)cell->Z,
                    (int)cell->Half, (int)cell->Type, (int)cell->Region);
            count++;
        }
    }
    fprintf(f, "\n");
    fprintf(f, "  ],\n");
    fprintf(f, "  \"auto_bases\": [\n");
    {
        S32 first_base = TRUE;
        S32 i;
        for (i = 0; i < TERRAIN_REGION_BASE_MAX; i++) {
            T_TERRAIN_REGION_BASE *base = &TerrainRegionBases[i];
            if (!base->Used)
                continue;
            if (!first_base)
                fprintf(f, ",\n");
            first_base = FALSE;
            fprintf(f, "    [%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d]",
                    (int)base->CubeX, (int)base->CubeY, (int)base->Index,
                    (int)base->Poly.Bank, (int)base->Poly.TexFlag, (int)base->Poly.PolyFlag,
                    (int)base->Poly.SampleStep, (int)base->Poly.CodeJeu, (int)base->Poly.Sens,
                    (int)base->Poly.Col, (int)base->Poly.Dummy, (int)base->Poly.IndexTex);
            count++;
        }
    }
    fprintf(f, "\n");
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);

    Log_Info("[editor] saved %d terrain edit(s) to %s", (int)count, path);
    return TRUE;
}

static void TerrainEditorMoveSelected(S32 dy) {
    S32 i;

    if (!MapSommetY)
        return;

    for (i = 0; i < TERRAIN_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_EDITOR_SEL *sel = &TerrainEditorSelList[i];
        T_TERRAIN_EDITOR_EDIT *edit;

        if (!sel->Used)
            continue;

        edit = TerrainEditorFindEdit(sel->CubeX, sel->CubeY, sel->Index);
        if (sel->CubeX == CurrentCubeX AND sel->CubeY == CurrentCubeY AND sel->Index >= 0 AND sel->Index < (NB_COTE + 1) * (NB_COTE + 1))
            edit = TerrainEditorGetOrCreateEdit(sel->CubeX, sel->CubeY, sel->Index, &MapSommetY[sel->Index]);
        if (!edit)
            continue;

        edit->Dy += dy;

        if (sel->CubeX == CurrentCubeX AND sel->CubeY == CurrentCubeY AND sel->Index >= 0 AND sel->Index < (NB_COTE + 1) * (NB_COTE + 1))
            TerrainEditorApplyEditToPoint(edit, &MapSommetY[sel->Index]);
    }
}

static void TerrainEditorSelectAll(void) {
    S32 xi;
    S32 zi;
    S32 selected = 0;

    if (!TerrainEditorLoadedCurrentCube()) {
        Editor_SetStatus("Terrain select all refused: current cube is not loaded");
        Log_Warn("[editor] terrain select all refused: loaded cube c%d,%d, current cube c%d,%d",
                 (int)TerrainEditorLoadedCubeX,
                 (int)TerrainEditorLoadedCubeY,
                 (int)CurrentCubeX,
                 (int)CurrentCubeY);
        return;
    }

    TerrainEditorClearSelection();

    for (zi = 0; zi <= NB_COTE; zi++) {
        for (xi = 0; xi <= NB_COTE; xi++) {
            S32 index = zi * (NB_COTE + 1) + xi;
            T_TERRAIN_EDITOR_SEL *sel;

            if (selected >= TERRAIN_EDITOR_MAX_SELECTED)
                break;

            sel = &TerrainEditorSelList[selected++];
            sel->Used = TRUE;
            sel->CubeX = CurrentCubeX;
            sel->CubeY = CurrentCubeY;
            sel->Index = index;
        }
    }

    Editor_SetStatus("Selected all terrain points in cube");
    Log_Info("[editor] selected %d terrain point(s) in c%d,%d",
             (int)selected,
             (int)CurrentCubeX,
             (int)CurrentCubeY);
}

static void TerrainTextureEditorSelectAll(void) {
    S32 index;
    S32 selected = 0;

    if (!TerrainEditorLoadedCurrentCube()) {
        Editor_SetStatus("Terrain select all refused: current cube is not loaded");
        Log_Warn("[editor] terrain select all refused: loaded cube c%d,%d, current cube c%d,%d",
                 (int)TerrainEditorLoadedCubeX,
                 (int)TerrainEditorLoadedCubeY,
                 (int)CurrentCubeX,
                 (int)CurrentCubeY);
        return;
    }

    TerrainTextureEditorClearSelection();

    for (index = 0; index < NB_COTE * NB_COTE * 2; index++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[selected++];
        sel->Used = TRUE;
        sel->CubeX = CurrentCubeX;
        sel->CubeY = CurrentCubeY;
        sel->Index = index;
    }

    Editor_SetStatus("Selected all terrain triangles in cube");
    Log_Info("[editor] selected %d terrain triangle(s) in c%d,%d",
             (int)selected,
             (int)CurrentCubeX,
             (int)CurrentCubeY);
}

static void TerrainEditorFlattenSelectedToZero(void) {
    S32 i;
    S32 flattened = 0;

    if (!TerrainEditorLoadedCurrentCube()) {
        Editor_SetStatus("Terrain flatten refused: current cube is not loaded");
        Log_Warn("[editor] terrain flatten refused: loaded cube c%d,%d, current cube c%d,%d",
                 (int)TerrainEditorLoadedCubeX,
                 (int)TerrainEditorLoadedCubeY,
                 (int)CurrentCubeX,
                 (int)CurrentCubeY);
        return;
    }
    if (!MapSommetY) {
        Editor_SetStatus("Terrain flatten refused: no height map loaded");
        Log_Warn("[editor] terrain flatten refused: no height map loaded");
        return;
    }

    for (i = 0; i < TERRAIN_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_EDITOR_SEL *sel = &TerrainEditorSelList[i];
        T_TERRAIN_EDITOR_EDIT *edit;
        S16 *height;

        if (!sel->Used)
            continue;
        if (sel->CubeX != CurrentCubeX OR sel->CubeY != CurrentCubeY)
            continue;
        if (sel->Index < 0 OR sel->Index >= (NB_COTE + 1) * (NB_COTE + 1))
            continue;

        height = &MapSommetY[sel->Index];
        edit = TerrainEditorFindEdit(sel->CubeX, sel->CubeY, sel->Index);
        if (!edit)
            edit = TerrainEditorGetOrCreateEdit(sel->CubeX, sel->CubeY, sel->Index, height);
        if (!edit)
            continue;

        edit->HasExpectY = TRUE;
        edit->BaseY = *height;
        edit->Dy = (S32)-*height;
        edit->SkipLogged = FALSE;
        TerrainEditorApplyEditToPoint(edit, height);
        flattened++;
    }

    if (!flattened) {
        Editor_SetStatus("Terrain flatten refused: no selected points");
        return;
    }

    Editor_SetStatus("Flattened selected terrain points to zero");
    Log_Info("[editor] flattened %d terrain point(s) to zero in c%d,%d",
             (int)flattened,
             (int)CurrentCubeX,
             (int)CurrentCubeY);
}

static U32 TerrainEditorJitterRandom(U32 *state) {
    *state = *state * 1664525U + 1013904223U;
    return *state;
}

static void TerrainEditorJitterSelected(S32 range) {
    S32 i;
    S32 jittered = 0;
    U32 seed;
    U32 state;

    if (!TerrainEditorLoadedCurrentCube() OR !MapSommetY) {
        Editor_SetStatus("Terrain jitter refused: current cube is not loaded");
        return;
    }

    seed = (U32)TimerRefHR ^ ((U32)CurrentCubeX << 24) ^ ((U32)CurrentCubeY << 16) ^ ++TerrainEditorJitterCounter;
    state = seed;
    for (i = 0; i < TERRAIN_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_EDITOR_SEL *sel = &TerrainEditorSelList[i];
        T_TERRAIN_EDITOR_EDIT *edit;
        S32 delta;

        if (!sel->Used OR sel->CubeX != CurrentCubeX OR sel->CubeY != CurrentCubeY)
            continue;
        if (sel->Index < 0 OR sel->Index >= (NB_COTE + 1) * (NB_COTE + 1))
            continue;

        edit = TerrainEditorFindEdit(sel->CubeX, sel->CubeY, sel->Index);
        if (!edit)
            edit = TerrainEditorGetOrCreateEdit(sel->CubeX, sel->CubeY, sel->Index, &MapSommetY[sel->Index]);
        if (!edit)
            continue;

        delta = (S32)(TerrainEditorJitterRandom(&state) % (U32)(range * 2 + 1)) - range;
        edit->Dy += delta;
        TerrainEditorApplyEditToPoint(edit, &MapSommetY[sel->Index]);
        jittered++;
    }

    if (!jittered) {
        Editor_SetStatus("Terrain jitter refused: no selected points");
        return;
    }
    Editor_SetStatus(range == TERRAIN_EDITOR_JITTER_SHIFT
                         ? "Applied strong terrain height jitter"
                         : "Applied terrain height jitter");
    Log_Info("[editor] jittered %d terrain point(s) range=[-%d,%d] seed=%u in c%d,%d",
             (int)jittered,
             (int)range,
             (int)range,
             (unsigned int)seed,
             (int)CurrentCubeX,
             (int)CurrentCubeY);
}

extern S32 ZoneEditorCreateZone(const T_ZONE *zone);

static void TerrainEditorCreateTransitionZone(void) {
    S32 i;
    S32 count = 0;
    S32 min_x = NB_COTE;
    S32 min_z = NB_COTE;
    S32 max_x = 0;
    S32 max_z = 0;
    S32 borders;
    S32 target_x = CurrentCubeX;
    S32 target_y = CurrentCubeY;
    S32 target_scene;
    const char *direction;
    T_ZONE zone;
    char status[160];

    if (!TerrainEditorLoadedCurrentCube()) {
        Editor_SetStatus("Transition zone refused: current cube is not loaded");
        return;
    }

    for (i = 0; i < TERRAIN_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_EDITOR_SEL *sel = &TerrainEditorSelList[i];
        S32 x;
        S32 z;

        if (!sel->Used)
            continue;
        if (sel->CubeX != CurrentCubeX OR sel->CubeY != CurrentCubeY) {
            Editor_SetStatus("Transition zone refused: selection spans multiple cubes");
            return;
        }
        if (sel->Index < 0 OR sel->Index >= (NB_COTE + 1) * (NB_COTE + 1))
            continue;
        x = sel->Index % (NB_COTE + 1);
        z = sel->Index / (NB_COTE + 1);
        if (x < min_x)
            min_x = x;
        if (x > max_x)
            max_x = x;
        if (z < min_z)
            min_z = z;
        if (z > max_z)
            max_z = z;
        count++;
    }

    if (!count) {
        Editor_SetStatus("Transition zone refused: no selected terrain points");
        return;
    }

    borders = (min_z == 0 ? 1 : 0)
              + (max_z == NB_COTE ? 1 : 0)
              + (min_x == 0 ? 1 : 0)
              + (max_x == NB_COTE ? 1 : 0);
    if (borders != 1) {
        Editor_SetStatus(borders ? "Transition zone refused: selection touches multiple borders"
                                 : "Transition zone refused: selection does not touch a cube border");
        return;
    }

    memset(&zone, 0, sizeof(zone));
    zone.Y0 = 0;
    zone.Y1 = 3200;
    zone.Type = 0;
    zone.Info7 = ZONE_INIT_ON | ZONE_ON;

    if (min_z == 0) {
        direction = "north";
        target_y--;
        zone.X0 = min_x * SIZE_BRICK_XZ;
        zone.X1 = max_x * SIZE_BRICK_XZ;
        zone.Z0 = 0;
        zone.Z1 = SIZE_BRICK_XZ;
        zone.Info2 = 32768 - 1024;
    } else if (max_z == NB_COTE) {
        direction = "south";
        target_y++;
        zone.X0 = min_x * SIZE_BRICK_XZ;
        zone.X1 = max_x * SIZE_BRICK_XZ;
        zone.Z0 = 32768 - SIZE_BRICK_XZ;
        zone.Z1 = 32767;
        zone.Info2 = SIZE_BRICK_XZ;
    } else if (min_x == 0) {
        direction = "west";
        target_x--;
        zone.X0 = 0;
        zone.X1 = SIZE_BRICK_XZ;
        zone.Z0 = min_z * SIZE_BRICK_XZ;
        zone.Z1 = max_z * SIZE_BRICK_XZ;
        zone.Info0 = 32768 - 1024;
    } else {
        direction = "east";
        target_x++;
        zone.X0 = 32768 - SIZE_BRICK_XZ;
        zone.X1 = 32767;
        zone.Z0 = min_z * SIZE_BRICK_XZ;
        zone.Z1 = max_z * SIZE_BRICK_XZ;
        zone.Info0 = SIZE_BRICK_XZ;
    }

    if (zone.X1 > 32767)
        zone.X1 = 32767;
    if (zone.Z1 > 32767)
        zone.Z1 = 32767;
    if (target_x < 0 OR target_x >= SIZE_MAIN_MAP OR target_y < 0 OR target_y >= SIZE_MAIN_MAP) {
        Editor_SetStatus("Transition zone refused: target cube is outside island map");
        return;
    }

    target_scene = Editor_FindExteriorScene(target_x, target_y);
    if (!target_scene) {
        snprintf(status, sizeof(status), "Transition zone refused: no scene for cube %d,%d", (int)target_x, (int)target_y);
        Editor_SetStatus(status);
        Log_Warn("[editor] %s", status);
        return;
    }
    zone.Num = (S16)target_scene;

    if (ZoneEditorCreateZone(&zone) < 0) {
        Editor_SetStatus("Transition zone refused: could not create zone");
        return;
    }

    snprintf(status, sizeof(status), "Created change cube zone: %s -> cube %d,%d", direction, (int)target_x, (int)target_y);
    Editor_SetStatus(status);
    Log_Info("[editor] %s (scene %d)", status, (int)target_scene);
}

void TerrainEditorBeginFrame(void) {
    S32 click;
    S32 up;
    S32 down;
    S32 step;
    S32 select_all;
    S32 flatten_zero;
    S32 jitter;
    S32 transition;
    S32 shift;

    if (!TerrainEditorSelect)
        return;
    if (Editor_IsCameraPanActive()) {
        TerrainEditorLastClick = Click;
        TerrainEditorLastUp = CheckKey(K_UP) ? TRUE : FALSE;
        TerrainEditorLastDown = CheckKey(K_DOWN) ? TRUE : FALSE;
        TerrainEditorLastSelectAll = (CheckKey(K_A) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
        TerrainEditorLastFlattenZero = (CheckKey(K_0) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
        TerrainEditorLastJitter = CheckKey(K_J) ? TRUE : FALSE;
        TerrainEditorLastTransition = CheckKey(K_T) ? TRUE : FALSE;
        return;
    }

    click = Click;
    TerrainEditorPickPending = FALSE;
    TerrainEditorDragSelectPending = FALSE;

    if ((click & 1) AND !(TerrainEditorLastClick & 1)) {
        TerrainEditorDragActive = TRUE;
        TerrainEditorDragShift = (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? TRUE : FALSE;
        TerrainEditorDragX0 = (S32)MouseX;
        TerrainEditorDragY0 = (S32)MouseY;
        TerrainEditorDragX1 = TerrainEditorDragX0;
        TerrainEditorDragY1 = TerrainEditorDragY0;
    }

    if (TerrainEditorDragActive AND(click & 1)) {
        TerrainEditorDragX1 = (S32)MouseX;
        TerrainEditorDragY1 = (S32)MouseY;
    }

    if (TerrainEditorDragActive AND !(click & 1) AND(TerrainEditorLastClick & 1)) {
        S32 dx;
        S32 dy;

        TerrainEditorDragX1 = (S32)MouseX;
        TerrainEditorDragY1 = (S32)MouseY;
        dx = TerrainEditorDragX1 - TerrainEditorDragX0;
        dy = TerrainEditorDragY1 - TerrainEditorDragY0;
        if (dx < 0)
            dx = -dx;
        if (dy < 0)
            dy = -dy;

        if (dx > TERRAIN_EDITOR_DRAG_MIN OR dy > TERRAIN_EDITOR_DRAG_MIN) {
            TerrainEditorDragSelectPending = TRUE;
            TerrainEditorPickShift = TerrainEditorDragShift;
            if (!TerrainEditorPickShift)
                TerrainEditorClearSelection();
        } else {
            TerrainEditorPickPending = TRUE;
            TerrainEditorPickShift = TerrainEditorDragShift;
            TerrainEditorPickBestDist = TERRAIN_EDITOR_PICK_RADIUS * TERRAIN_EDITOR_PICK_RADIUS;
            TerrainEditorPickBestIndex = -1;
        }
        TerrainEditorDragActive = FALSE;
    }
    TerrainEditorLastClick = click;

    up = CheckKey(K_UP) ? TRUE : FALSE;
    down = CheckKey(K_DOWN) ? TRUE : FALSE;
    if (Editor_InspectorConsumesInput()) {
        TerrainEditorLastUp = up;
        TerrainEditorLastDown = down;
        TerrainEditorLastSelectAll = (CheckKey(K_A) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
        TerrainEditorLastFlattenZero = (CheckKey(K_0) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
        TerrainEditorLastJitter = CheckKey(K_J) ? TRUE : FALSE;
        TerrainEditorLastTransition = CheckKey(K_T) ? TRUE : FALSE;
        return;
    }
    shift = (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? TRUE : FALSE;
    step = shift ? TERRAIN_EDITOR_NUDGE_SHIFT : TERRAIN_EDITOR_NUDGE;

    if (up AND !TerrainEditorLastUp)
        TerrainEditorMoveSelected(step);
    if (down AND !TerrainEditorLastDown)
        TerrainEditorMoveSelected(-step);

    TerrainEditorLastUp = up;
    TerrainEditorLastDown = down;

    select_all = (CheckKey(K_A) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
    flatten_zero = (CheckKey(K_0) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
    jitter = CheckKey(K_J) ? TRUE : FALSE;
    transition = CheckKey(K_T) ? TRUE : FALSE;

    if (select_all AND !TerrainEditorLastSelectAll)
        TerrainEditorSelectAll();

    if (flatten_zero AND !TerrainEditorLastFlattenZero)
        TerrainEditorFlattenSelectedToZero();

    if (jitter AND !TerrainEditorLastJitter)
        TerrainEditorJitterSelected(shift ? TERRAIN_EDITOR_JITTER_SHIFT : TERRAIN_EDITOR_JITTER);

    if (transition AND !TerrainEditorLastTransition)
        TerrainEditorCreateTransitionZone();

    TerrainEditorLastSelectAll = select_all;
    TerrainEditorLastFlattenZero = flatten_zero;
    TerrainEditorLastJitter = jitter;
    TerrainEditorLastTransition = transition;
}

static void TerrainEditorSelectOnly(S32 cube_x, S32 cube_y, S32 index) {
    TerrainEditorClearSelection();
    TerrainEditorSelList[0].Used = TRUE;
    TerrainEditorSelList[0].CubeX = cube_x;
    TerrainEditorSelList[0].CubeY = cube_y;
    TerrainEditorSelList[0].Index = index;
}

static void TerrainEditorAddSelected(S32 cube_x, S32 cube_y, S32 index) {
    S32 i;

    for (i = 0; i < TERRAIN_EDITOR_MAX_SELECTED; i++) {
        if (TerrainEditorSamePoint(&TerrainEditorSelList[i], cube_x, cube_y, index))
            return;
    }

    for (i = 0; i < TERRAIN_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_EDITOR_SEL *sel = &TerrainEditorSelList[i];

        if (sel->Used)
            continue;
        sel->Used = TRUE;
        sel->CubeX = cube_x;
        sel->CubeY = cube_y;
        sel->Index = index;
        return;
    }
}

static void TerrainEditorToggleSelected(S32 cube_x, S32 cube_y, S32 index) {
    S32 i;

    for (i = 0; i < TERRAIN_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_EDITOR_SEL *sel = &TerrainEditorSelList[i];

        if (TerrainEditorSamePoint(sel, cube_x, cube_y, index)) {
            sel->Used = FALSE;
            return;
        }
    }

    for (i = 0; i < TERRAIN_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_EDITOR_SEL *sel = &TerrainEditorSelList[i];

        if (sel->Used)
            continue;
        sel->Used = TRUE;
        sel->CubeX = cube_x;
        sel->CubeY = cube_y;
        sel->Index = index;
        return;
    }
}

void TerrainEditorEndFrame(void) {
    if (TerrainEditorDragSelectPending) {
        TerrainEditorDragSelectPending = FALSE;
        return;
    }

    if (!TerrainEditorSelect OR !TerrainEditorPickPending)
        return;

    if (TerrainEditorPickBestIndex < 0) {
        if (!TerrainEditorPickShift)
            TerrainEditorClearSelection();
    } else if (TerrainEditorPickShift) {
        TerrainEditorToggleSelected(TerrainEditorPickBestCubeX,
                                    TerrainEditorPickBestCubeY,
                                    TerrainEditorPickBestIndex);
    } else {
        TerrainEditorSelectOnly(TerrainEditorPickBestCubeX,
                                TerrainEditorPickBestCubeY,
                                TerrainEditorPickBestIndex);
    }

    TerrainEditorPickPending = FALSE;
}

static void TerrainEditorConsiderPick(S32 cube_x, S32 cube_y, S32 index, S32 xs, S32 ys) {
    S32 dx;
    S32 dy;
    S32 dist;

    if (TerrainEditorDragSelectPending) {
        S32 x0 = TerrainEditorDragX0;
        S32 y0 = TerrainEditorDragY0;
        S32 x1 = TerrainEditorDragX1;
        S32 y1 = TerrainEditorDragY1;

        if (x0 > x1) {
            S32 t = x0;
            x0 = x1;
            x1 = t;
        }
        if (y0 > y1) {
            S32 t = y0;
            y0 = y1;
            y1 = t;
        }

        if (xs >= x0 AND xs <= x1 AND ys >= y0 AND ys <= y1) {
            if (TerrainEditorPickShift)
                TerrainEditorToggleSelected(cube_x, cube_y, index);
            else
                TerrainEditorAddSelected(cube_x, cube_y, index);
            if (MapSommetY
                    AND index >= 0 AND index < (NB_COTE + 1) * (NB_COTE + 1))
                TerrainEditorGetOrCreateEdit(cube_x, cube_y, index, &MapSommetY[index]);
        }
    }

    if (!TerrainEditorPickPending)
        return;

    dx = xs - (S32)MouseX;
    dy = ys - (S32)MouseY;
    dist = dx * dx + dy * dy;
    if (dist <= TerrainEditorPickBestDist) {
        TerrainEditorPickBestDist = dist;
        TerrainEditorPickBestCubeX = cube_x;
        TerrainEditorPickBestCubeY = cube_y;
        TerrainEditorPickBestIndex = index;
        if (MapSommetY
                AND index >= 0 AND index < (NB_COTE + 1) * (NB_COTE + 1))
            TerrainEditorGetOrCreateEdit(cube_x, cube_y, index, &MapSommetY[index]);
    }
}

static void TerrainEditorDrawDragBox(void) {
    S32 x0;
    S32 y0;
    S32 x1;
    S32 y1;

    if (!TerrainEditorDragActive)
        return;

    x0 = TerrainEditorDragX0;
    y0 = TerrainEditorDragY0;
    x1 = TerrainEditorDragX1;
    y1 = TerrainEditorDragY1;

    if (x0 > x1) {
        S32 t = x0;
        x0 = x1;
        x1 = t;
    }
    if (y0 > y1) {
        S32 t = y0;
        y0 = y1;
        y1 = t;
    }

    Rect(x0, y0, x1, y1, TERRAIN_EDITOR_DRAG_MARK);
}

void TerrainEditorOverlayDraw(void) {
    S32 xi;
    S32 zi;

    if (!TerrainEditorSelect)
        return;
    if (!TerrainEditorLoadedCurrentCube())
        return;

    UnsetClip();

    for (zi = 0; zi <= NB_COTE; zi++) {
        for (xi = 0; xi <= NB_COTE; xi++) {
            S32 index = zi * (NB_COTE + 1) + xi;
            T_SOMMET_ROT *ptrs = &SommetRot[zi][xi];
            U8 visible = SommetAffichable[zi][xi];
            S32 xs;
            S32 ys;
            S32 selected;

            if (visible != 16)
                continue;

            xs = ptrs->X2D;
            ys = ptrs->Y2D;

            TerrainEditorConsiderPick(CurrentCubeX, CurrentCubeY, index, xs, ys);

            selected = TerrainEditorIsSelected(CurrentCubeX, CurrentCubeY, index);
            if (selected) {
                Rect(xs - 2, ys - 2, xs + 2, ys + 2, TERRAIN_EDITOR_BACK);
            } else {
                Plot(xs, ys, TERRAIN_OVERLAY_TEXT);
            }
        }
    }

    TerrainEditorDrawDragBox();

    UnsetClip();
}

static S32 TerrainTextureEditorSameTri(T_TERRAIN_TEXTURE_EDITOR_SEL *sel, S32 cube_x, S32 cube_y, S32 index) {
    return sel->Used
           AND sel->CubeX == cube_x AND sel->CubeY == cube_y AND sel->Index == index;
}

void TerrainTextureEditorClearSelection(void) {
    memset(TerrainTextureEditorSelList, 0, sizeof(TerrainTextureEditorSelList));
}

void TerrainTextureEditorSetSelect(S32 select) {
    TerrainTextureEditorSelect = select ? TRUE : FALSE;
    if (!TerrainTextureEditorSelect) {
        TerrainTextureEditorClearSelection();
        TerrainTextureEditorDragActive = FALSE;
        TerrainTextureEditorDragSelectPending = FALSE;
        TerrainTextureEditorPalettePressEntry = TERRAIN_TEXTURE_PALETTE_NONE;
    }
    TerrainTextureEditorLastClick = Click;
    TerrainTextureEditorLastUp = CheckKey(K_UP) ? TRUE : FALSE;
    TerrainTextureEditorLastDown = CheckKey(K_DOWN) ? TRUE : FALSE;
    TerrainTextureEditorLastLeft = CheckKey(K_LEFT) ? TRUE : FALSE;
    TerrainTextureEditorLastRight = CheckKey(K_RIGHT) ? TRUE : FALSE;
    TerrainTextureEditorLastSelectAll = (CheckKey(K_A) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
    TerrainTextureEditorLastQ = CheckKey(K_Q) ? TRUE : FALSE;
    TerrainTextureEditorLastL = CheckKey(K_L) ? TRUE : FALSE;
    TerrainTextureEditorLastFullPalette = (CheckKey(K_P) AND(CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT))) ? TRUE : FALSE;
    TerrainTextureEditorLastPalettePrev = CheckKey(K_LBRACKET) ? TRUE : FALSE;
    TerrainTextureEditorLastPaletteNext = CheckKey(K_RBRACKET) ? TRUE : FALSE;
    ShowMouse(TerrainTextureEditorSelect ? true : false);
}

S32 TerrainTextureEditorGetSelect(void) {
    return TerrainTextureEditorSelect;
}

S32 TerrainTextureEditorConsumesInput(void) {
    return TerrainTextureEditorSelect;
}

static S32 TerrainTextureEditorSameTexDef(const T_HALF_TEX *a, const T_HALF_TEX *b) {
    return memcmp(a, b, sizeof(T_HALF_TEX)) == 0;
}

static S32 TerrainTextureEditorFindTexDef(const T_HALF_TEX *defs, S32 count, const T_HALF_TEX *wanted) {
    S32 i;
    for (i = 0; i < count; i++) {
        if (TerrainTextureEditorSameTexDef(&defs[i], wanted))
            return i;
    }
    return -1;
}

static S32 TerrainTextureEditorFindCatalogPair(const T_HALF_TEX *first, const T_HALF_TEX *second) {
    S32 i;
    for (i = 0; i + 1 < TerrainTextureEditorCatalogCount; i += 2) {
        if (TerrainTextureEditorSameTexDef(&TerrainTextureEditorCatalog[i], first)
                AND TerrainTextureEditorSameTexDef(&TerrainTextureEditorCatalog[i + 1], second))
            return i;
    }
    return -1;
}

static S32 TerrainTextureEditorCubeTexDefEntry(S32 cube) {
    return TERRAIN_EDITOR_HQR_START_CUBE + TERRAIN_EDITOR_HQR_STEP_CUBE * (cube - 1) + TERRAIN_EDITOR_HQR_CUBE_TXD;
}

void TerrainTextureEditorBuildCatalog(void) {
    S32 cube_x;
    S32 cube_y;

    TerrainTextureEditorCatalogCount = 0;
    for (cube_y = 0; cube_y < SIZE_MAIN_MAP; cube_y++) {
        for (cube_x = 0; cube_x < SIZE_MAIN_MAP; cube_x++) {
            S32 cube = IsleMapIndex[cube_y * SIZE_MAIN_MAP + cube_x] & 127;
            S32 size = 0;
            S32 count;
            S32 i;
            T_HALF_TEX *defs;

            if (!cube)
                continue;
            defs = (T_HALF_TEX *)HQR_Get(HQR_ListTexDef, TerrainTextureEditorCubeTexDefEntry(cube), &size);
            count = size / (S32)sizeof(T_HALF_TEX);
            for (i = 0; i + 1 < count AND TerrainTextureEditorCatalogCount + 1 < TERRAIN_TEXTURE_MAX_TEXDEFS; i += 2) {
                if (TerrainTextureEditorFindCatalogPair(&defs[i], &defs[i + 1]) >= 0)
                    continue;
                TerrainTextureEditorCatalog[TerrainTextureEditorCatalogCount++] = defs[i];
                TerrainTextureEditorCatalog[TerrainTextureEditorCatalogCount++] = defs[i + 1];
            }
        }
    }
    Log_Info("[editor] built island terrain texture catalog: %d triangle definitions",
             (int)TerrainTextureEditorCatalogCount);
}

void TerrainTextureEditorSetTexDefs(T_HALF_TEX *defs, S32 count) {
    S32 i;

    if (count < 0)
        count = 0;
    if (count > TERRAIN_TEXTURE_MAX_TEXDEFS)
        count = TERRAIN_TEXTURE_MAX_TEXDEFS;
    if (count > 0)
        memcpy(TerrainTextureEditorLoadedTexDefs, defs, count * sizeof(T_HALF_TEX));

    TerrainTextureEditorCubeTexDefCount = count;
    TerrainTextureEditorTexDefCount = count;
    for (i = 0; i < TerrainTextureEditorCatalogCount AND TerrainTextureEditorTexDefCount < TERRAIN_TEXTURE_MAX_TEXDEFS; i++) {
        if (TerrainTextureEditorFindTexDef(TerrainTextureEditorLoadedTexDefs,
                                          TerrainTextureEditorTexDefCount,
                                          &TerrainTextureEditorCatalog[i]) >= 0)
            continue;
        TerrainTextureEditorLoadedTexDefs[TerrainTextureEditorTexDefCount++] = TerrainTextureEditorCatalog[i];
    }
    ListTexDef = TerrainTextureEditorLoadedTexDefs;
}

static S32 TerrainTextureEditorResolveCatalogTexture(S32 catalog_index) {
    if (catalog_index < 0 OR catalog_index >= TerrainTextureEditorCatalogCount)
        return -1;
    return TerrainTextureEditorFindTexDef(ListTexDef,
                                         TerrainTextureEditorTexDefCount,
                                         &TerrainTextureEditorCatalog[catalog_index]);
}

static T_TERRAIN_TEXTURE_EDITOR_EDIT *TerrainTextureEditorFindEdit(S32 cube_x, S32 cube_y, S32 index) {
    S32 i;
    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_EDITS; i++) {
        T_TERRAIN_TEXTURE_EDITOR_EDIT *edit = &TerrainTextureEditorEdits[i];

        if (edit->Used
                AND edit->CubeX == cube_x AND edit->CubeY == cube_y AND edit->Index == index)
            return edit;
    }
    return NULL;
}

static T_TERRAIN_TEXTURE_EDITOR_EDIT *TerrainTextureEditorGetOrCreateEdit(S32 cube_x, S32 cube_y, S32 index, T_HALF_POLY *poly) {
    S32 i;
    T_TERRAIN_TEXTURE_EDITOR_EDIT *edit;

    edit = TerrainTextureEditorFindEdit(cube_x, cube_y, index);
    if (edit)
        return edit;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_EDITS; i++) {
        edit = &TerrainTextureEditorEdits[i];
        if (!edit->Used) {
            edit->Used = TRUE;
            edit->CubeX = cube_x;
            edit->CubeY = cube_y;
            edit->Index = index;
            edit->BasePoly = *poly;
            edit->Poly = *poly;
            edit->HasExpectPoly = TRUE;
            edit->SkipLogged = FALSE;
            return edit;
        }
    }

    Log_Warn("texedit runtime edit table is full; cannot track c%d,%d t%d",
             (int)cube_x,
             (int)cube_y,
             (int)index);
    return NULL;
}

static void TerrainTextureEditorApplyEditToPoly(T_TERRAIN_TEXTURE_EDITOR_EDIT *edit, T_HALF_POLY *poly) {
    *poly = edit->Poly;
}

void TerrainTextureEditorApplyRuntimeEditsForLoadedCube(S32 cube_x, S32 cube_y) {
    S32 i;

    if (!MapPolyGround)
        return;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_EDITS; i++) {
        T_TERRAIN_TEXTURE_EDITOR_EDIT *edit = &TerrainTextureEditorEdits[i];

        if (!edit->Used
                OR edit->CubeX != cube_x OR edit->CubeY != cube_y OR edit->Index < 0 OR edit->Index >= NB_COTE * NB_COTE * 2)
            continue;
        if (edit->HasExpectPoly) {
            T_HALF_POLY *poly = &MapPolyGround[edit->Index];
            if (poly->Bank != edit->BasePoly.Bank OR poly->TexFlag != edit->BasePoly.TexFlag OR poly->PolyFlag != edit->BasePoly.PolyFlag OR poly->SampleStep != edit->BasePoly.SampleStep OR poly->CodeJeu != edit->BasePoly.CodeJeu OR poly->Sens != edit->BasePoly.Sens OR poly->Col != edit->BasePoly.Col OR poly->Dummy != edit->BasePoly.Dummy OR poly->IndexTex != edit->BasePoly.IndexTex) {
                if (!edit->SkipLogged) {
                    Log_Warn("[editor] texture edit skipped c%d,%d t%d: current poly does not match expected",
                             (int)cube_x,
                             (int)cube_y,
                             (int)edit->Index);
                    edit->SkipLogged = TRUE;
                }
                continue;
            }
        }
        TerrainTextureEditorApplyEditToPoly(edit, &MapPolyGround[edit->Index]);
    }

    if (MapIntensity) {
        for (i = 0; i < TERRAIN_TEXTURE_INTENSITY_MAX_EDITS; i++) {
            T_TERRAIN_TEXTURE_INTENSITY_EDIT *edit = &TerrainTextureIntensityEdits[i];

            if (!edit->Used
                    OR edit->CubeX != cube_x OR edit->CubeY != cube_y OR edit->Index < 0 OR edit->Index >= (NB_COTE + 1) * (NB_COTE + 1))
                continue;
            if (edit->HasExpectIntensity AND MapIntensity[edit->Index] != edit->BaseIntensity) {
                if (!edit->SkipLogged) {
                    Log_Warn("[editor] intensity edit skipped c%d,%d i%d: intensity %d does not match expected %d",
                             (int)cube_x,
                             (int)cube_y,
                             (int)edit->Index,
                             (int)MapIntensity[edit->Index],
                             (int)edit->BaseIntensity);
                    edit->SkipLogged = TRUE;
                }
                continue;
            }
            MapIntensity[edit->Index] = edit->Intensity;
        }
    }
}

void TerrainTextureEditorClearRuntimeEdits(void) {
    memset(TerrainTextureEditorEdits, 0, sizeof(TerrainTextureEditorEdits));
    memset(TerrainTextureIntensityEdits, 0, sizeof(TerrainTextureIntensityEdits));
    TerrainTextureEditorClearSelection();
}

static void TerrainTextureEditorCycleEdit(T_TERRAIN_TEXTURE_EDITOR_EDIT *edit, S32 dir) {
    T_HALF_POLY *poly;

    if (TerrainTextureEditorTexDefCount <= 0)
        return;

    poly = &edit->Poly;
    if (poly->TexFlag == 0) {
        poly->TexFlag = 1;
        poly->PolyFlag = edit->BasePoly.TexFlag ? edit->BasePoly.PolyFlag : 0;
        poly->IndexTex = dir > 0 ? 0 : (U32)(TerrainTextureEditorTexDefCount - 1);
        return;
    }

    if (dir > 0) {
        if ((S32)poly->IndexTex >= TerrainTextureEditorTexDefCount - 1) {
            poly->TexFlag = 2;
            if (!poly->PolyFlag)
                poly->PolyFlag = 3;
        } else {
            poly->IndexTex++;
        }
    } else {
        if (poly->IndexTex == 0) {
            poly->TexFlag = 2;
            if (!poly->PolyFlag)
                poly->PolyFlag = 3;
        } else {
            poly->IndexTex--;
        }
    }
}

static void TerrainTextureEditorCycleTexFlagEdit(T_TERRAIN_TEXTURE_EDITOR_EDIT *edit) {
    T_HALF_POLY *poly = &edit->Poly;
    poly->TexFlag = (poly->TexFlag + 1) % 4;
}

static void TerrainTextureEditorCycleSelected(S32 dir) {
    if (!MapPolyGround)
        return;

    {
        S32 count = TerrainTextureEditorPaletteTextureCount();
        S32 entry = TerrainTextureEditorSelectedTextureEntry();

        if (entry < -1)
            entry = 0;
        else if (dir > 0)
            entry = entry >= count - 1 ? -1 : entry + 1;
        else
            entry = entry < 0 ? count - 1 : entry - 1;
        TerrainTextureEditorApplyTextureSelected(entry);
    }
}

static void TerrainTextureEditorCycleSelectedTexFlag() {
    S32 i;

    if (!MapPolyGround)
        return;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];
        T_TERRAIN_TEXTURE_EDITOR_EDIT *edit;

        if (!sel->Used)
            continue;

        edit = TerrainTextureEditorFindEdit(sel->CubeX, sel->CubeY, sel->Index);
        if (sel->CubeX == CurrentCubeX AND sel->CubeY == CurrentCubeY AND sel->Index >= 0 AND sel->Index < NB_COTE * NB_COTE * 2)
            edit = TerrainTextureEditorGetOrCreateEdit(sel->CubeX, sel->CubeY, sel->Index, &MapPolyGround[sel->Index]);
        if (!edit)
            continue;

        TerrainTextureEditorCycleTexFlagEdit(edit);

        if (sel->CubeX == CurrentCubeX AND sel->CubeY == CurrentCubeY AND sel->Index >= 0 AND sel->Index < NB_COTE * NB_COTE * 2)
            TerrainTextureEditorApplyEditToPoly(edit, &MapPolyGround[sel->Index]);
    }
}

static void TerrainTextureEditorSetEditTexture(T_TERRAIN_TEXTURE_EDITOR_EDIT *edit, S32 texture_index) {
    T_HALF_POLY *poly;

    poly = &edit->Poly;
    if (texture_index < 0) {
        poly->TexFlag = 0;
        if (!poly->PolyFlag)
            poly->PolyFlag = 3;
        return;
    }

    if (TerrainTextureEditorTexDefCount <= 0)
        return;
    if (texture_index >= TerrainTextureEditorTexDefCount)
        return;

    poly->TexFlag = 2;
    poly->PolyFlag = edit->BasePoly.TexFlag ? edit->BasePoly.PolyFlag : 0;
    poly->IndexTex = (U32)texture_index;
}

static void TerrainTextureEditorApplyTextureSelected(S32 texture_index) {
    S32 i;

    if (!MapPolyGround)
        return;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];
        T_TERRAIN_TEXTURE_EDITOR_EDIT *edit;

        if (!sel->Used)
            continue;

        edit = TerrainTextureEditorFindEdit(sel->CubeX, sel->CubeY, sel->Index);
        if (sel->CubeX == CurrentCubeX AND sel->CubeY == CurrentCubeY AND sel->Index >= 0 AND sel->Index < NB_COTE * NB_COTE * 2)
            edit = TerrainTextureEditorGetOrCreateEdit(sel->CubeX, sel->CubeY, sel->Index, &MapPolyGround[sel->Index]);
        if (!edit)
            continue;

        if (texture_index >= 0) {
            S32 resolved_index = TerrainTextureEditorQuadMode ? texture_index * 2 + (sel->Index & 1) : texture_index;
            if (TerrainTextureEditorFullPalette)
                resolved_index = TerrainTextureEditorResolveCatalogTexture(resolved_index);
            TerrainTextureEditorSetEditTexture(edit, resolved_index);
        } else {
            TerrainTextureEditorSetEditTexture(edit, -1);
        }

        if (sel->CubeX == CurrentCubeX AND sel->CubeY == CurrentCubeY AND sel->Index >= 0 AND sel->Index < NB_COTE * NB_COTE * 2)
            TerrainTextureEditorApplyEditToPoly(edit, &MapPolyGround[sel->Index]);
    }
}

T_HALF_POLY *TerrainTextureEditorGetSelectedHalfPoly() {
    for (S32 i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];
        T_HALF_POLY *poly;

        if (!sel->Used)
            continue;
        if (sel->CubeX != CurrentCubeX OR sel->CubeY != CurrentCubeY)
            continue;
        if (sel->Index < 0 OR sel->Index >= NB_COTE * NB_COTE * 2)
            continue;

        poly = &MapPolyGround[sel->Index];
        if (poly)
            return poly;
    }

    return NULL;
}

S32 TerrainTextureEditorGetSelectedCount(void) {
    S32 i;
    S32 count = 0;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        if (TerrainTextureEditorSelList[i].Used)
            count++;
    }
    return count;
}

S32 TerrainTextureEditorGetQuadMode(void) {
    return TerrainTextureEditorQuadMode;
}

static void TerrainTextureEditorLogSelected(void) {
    S32 i;
    S32 count = 0;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];
        T_TERRAIN_TEXTURE_EDITOR_EDIT *edit;
        T_HALF_POLY *poly = NULL;
        S32 cell;
        S32 x;
        S32 z;
        S32 tri;

        if (!sel->Used)
            continue;

        edit = TerrainTextureEditorFindEdit(sel->CubeX, sel->CubeY, sel->Index);
        if (edit) {
            poly = &edit->Poly;
        } else if (sel->CubeX == CurrentCubeX AND sel->CubeY == CurrentCubeY AND sel->Index >= 0 AND sel->Index < NB_COTE * NB_COTE * 2 AND MapPolyGround) {
            poly = &MapPolyGround[sel->Index];
        }
        if (!poly)
            continue;
        if ((S32)poly->IndexTex < 0 OR(S32)poly->IndexTex >= TerrainTextureEditorTexDefCount)
            continue;

        cell = sel->Index / 2;
        x = cell % NB_COTE;
        z = cell / NB_COTE;
        tri = sel->Index & 1;
        Log_Raw("[c%d,%d %d,%d.%d] %d (sens=%d col=%d footstep=%d)",
                (int)sel->CubeX,
                (int)sel->CubeY,
                (int)x,
                (int)z,
                (int)tri,
                (int)TerrainTextureEditorFindTexDef(TerrainTextureEditorCatalog,
                                                    TerrainTextureEditorCatalogCount,
                                                    &ListTexDef[poly->IndexTex]),
                (int)poly->Sens,
                (int)poly->Col,
                (int)poly->SampleStep);
        count++;
    }

    Editor_SetStatus(count ? "Logged selected terrain textures" : "No terrain textures selected");
}

static S32 TerrainTextureEditorPolyFieldValue(T_HALF_POLY *poly, S32 field) {
    switch (field) {
    case TERRAIN_TEXTURE_FIELD_BANK:
        return poly->Bank;
    case TERRAIN_TEXTURE_FIELD_TEX_FLAG:
        return poly->TexFlag;
    case TERRAIN_TEXTURE_FIELD_POLY_FLAG:
        return poly->PolyFlag;
    case TERRAIN_TEXTURE_FIELD_SAMPLE_STEP:
        return poly->SampleStep;
    case TERRAIN_TEXTURE_FIELD_CODE_JEU:
        return poly->CodeJeu;
    case TERRAIN_TEXTURE_FIELD_SENS:
        return poly->Sens;
    case TERRAIN_TEXTURE_FIELD_COL:
        return poly->Col;
    case TERRAIN_TEXTURE_FIELD_DUMMY:
        return poly->Dummy;
    case TERRAIN_TEXTURE_FIELD_INDEX_TEX:
        if ((S32)poly->IndexTex < TerrainTextureEditorTexDefCount)
            return TerrainTextureEditorFindTexDef(TerrainTextureEditorCatalog,
                                                  TerrainTextureEditorCatalogCount,
                                                  &ListTexDef[poly->IndexTex]);
        return -1;
    }
    return 0;
}

S32 TerrainTextureEditorGetSelectedFieldValue(S32 field, S32 *mixed) {
    S32 i;
    S32 found = FALSE;
    S32 value = 0;

    if (mixed)
        *mixed = FALSE;
    if (!MapPolyGround OR field < 0 OR field >= TERRAIN_TEXTURE_FIELD_COUNT)
        return 0;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];
        T_TERRAIN_TEXTURE_EDITOR_EDIT *edit;
        T_HALF_POLY *poly;
        S32 current;

        if (!sel->Used OR sel->CubeX != CurrentCubeX OR sel->CubeY != CurrentCubeY)
            continue;
        if (sel->Index < 0 OR sel->Index >= NB_COTE * NB_COTE * 2)
            continue;

        poly = &MapPolyGround[sel->Index];
        edit = TerrainTextureEditorFindEdit(sel->CubeX, sel->CubeY, sel->Index);
        if (edit)
            poly = &edit->Poly;
        current = TerrainTextureEditorPolyFieldValue(poly, field);
        if (!found) {
            value = current;
            found = TRUE;
        } else if (value != current) {
            if (mixed)
                *mixed = TRUE;
            return value;
        }
    }
    return value;
}

static void TerrainTextureEditorSetPolyFieldValue(T_HALF_POLY *poly, S32 field, S32 value) {
    if (value < 0)
        value = 0;

    switch (field) {
    case TERRAIN_TEXTURE_FIELD_BANK:
        if (value > 15)
            value = 15;
        poly->Bank = (U32)value;
        break;
    case TERRAIN_TEXTURE_FIELD_TEX_FLAG:
        if (value > 3)
            value = 3;
        poly->TexFlag = (U32)value;
        break;
    case TERRAIN_TEXTURE_FIELD_POLY_FLAG:
        if (value > 3)
            value = 3;
        poly->PolyFlag = (U32)value;
        break;
    case TERRAIN_TEXTURE_FIELD_SAMPLE_STEP:
        if (value > 15)
            value = 15;
        poly->SampleStep = (U32)value;
        break;
    case TERRAIN_TEXTURE_FIELD_CODE_JEU:
        if (value > 15)
            value = 15;
        poly->CodeJeu = (U32)value;
        break;
    case TERRAIN_TEXTURE_FIELD_SENS:
        poly->Sens = (U32)(value ? 1 : 0);
        break;
    case TERRAIN_TEXTURE_FIELD_COL:
        poly->Col = (U32)(value ? 1 : 0);
        break;
    case TERRAIN_TEXTURE_FIELD_DUMMY:
        poly->Dummy = (U32)(value ? 1 : 0);
        break;
    case TERRAIN_TEXTURE_FIELD_INDEX_TEX:
        value = TerrainTextureEditorResolveCatalogTexture(value);
        if (value >= 0)
            poly->IndexTex = (U32)value;
        break;
    }
}

S32 TerrainTextureEditorSetSelectedField(S32 field, S32 value) {
    S32 i;
    S32 changed = 0;

    if (!MapPolyGround OR field < 0 OR field >= TERRAIN_TEXTURE_FIELD_COUNT)
        return FALSE;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];
        T_TERRAIN_TEXTURE_EDITOR_EDIT *edit;

        if (!sel->Used)
            continue;

        edit = TerrainTextureEditorFindEdit(sel->CubeX, sel->CubeY, sel->Index);
        if (sel->CubeX == CurrentCubeX AND sel->CubeY == CurrentCubeY AND sel->Index >= 0 AND sel->Index < NB_COTE * NB_COTE * 2)
            edit = TerrainTextureEditorGetOrCreateEdit(sel->CubeX, sel->CubeY, sel->Index, &MapPolyGround[sel->Index]);
        if (!edit)
            continue;

        TerrainTextureEditorSetPolyFieldValue(&edit->Poly, field, value);

        if (sel->CubeX == CurrentCubeX AND sel->CubeY == CurrentCubeY AND sel->Index >= 0 AND sel->Index < NB_COTE * NB_COTE * 2)
            TerrainTextureEditorApplyEditToPoly(edit, &MapPolyGround[sel->Index]);
        changed++;
    }

    return changed > 0;
}

static U32 TerrainRegionEditorHash(S32 seed, S32 x, S32 z) {
    U32 value = (U32)seed ^ ((U32)x * 0x45d9f3bU) ^ ((U32)z * 0x119de1f3U);
    value ^= value >> 16;
    value *= 0x45d9f3bU;
    value ^= value >> 16;
    return value;
}

static S32 TerrainRegionEditorPatternsAvailable(void) {
    return Island == 0 OR Island == 12;
}

static T_TERRAIN_REGION_CELL *TerrainRegionEditorFindCell(S32 cube_x, S32 cube_y, S32 x, S32 z, S32 half) {
    S32 i;
    for (i = 0; i < TERRAIN_REGION_CELL_MAX; i++) {
        T_TERRAIN_REGION_CELL *cell = &TerrainRegionCells[i];
        if (cell->Used AND cell->CubeX == cube_x AND cell->CubeY == cube_y AND cell->X == x AND cell->Z == z AND cell->Half == half)
            return cell;
    }
    return NULL;
}

static S32 TerrainRegionEditorCellCovers(T_TERRAIN_REGION_CELL *cell, S32 half) {
    return cell->Half < 0 OR cell->Half == half;
}

static S32 TerrainRegionEditorRegionCovers(T_TERRAIN_REGION *region, S32 x, S32 z, S32 half) {
    S32 i;
    S32 region_index = (S32)(region - TerrainRegions);
    for (i = 0; i < TERRAIN_REGION_CELL_MAX; i++) {
        T_TERRAIN_REGION_CELL *cell = &TerrainRegionCells[i];
        if (cell->Used AND cell->Region == region_index AND cell->X == x AND cell->Z == z AND TerrainRegionEditorCellCovers(cell, half))
            return TRUE;
    }
    return FALSE;
}

static T_TERRAIN_REGION_BASE *TerrainRegionEditorFindBase(S32 cube_x, S32 cube_y, S32 index) {
    S32 i;
    for (i = 0; i < TERRAIN_REGION_BASE_MAX; i++) {
        T_TERRAIN_REGION_BASE *base = &TerrainRegionBases[i];
        if (base->Used AND base->CubeX == cube_x AND base->CubeY == cube_y AND base->Index == index)
            return base;
    }
    return NULL;
}

static void TerrainRegionEditorCacheBase(S32 cube_x, S32 cube_y, S32 index) {
    S32 i;
    T_TERRAIN_TEXTURE_EDITOR_EDIT *edit;

    if (!MapPolyGround OR TerrainRegionEditorFindBase(cube_x, cube_y, index))
        return;
    for (i = 0; i < TERRAIN_REGION_BASE_MAX; i++) {
        T_TERRAIN_REGION_BASE *base = &TerrainRegionBases[i];
        if (base->Used)
            continue;
        edit = TerrainTextureEditorFindEdit(cube_x, cube_y, index);
        base->Used = TRUE;
        base->CubeX = cube_x;
        base->CubeY = cube_y;
        base->Index = index;
        base->Poly = edit ? edit->Poly : MapPolyGround[index];
        return;
    }
    Log_Warn("[editor] auto terrain base cache is full");
}

static void TerrainRegionEditorRestoreBase(T_TERRAIN_REGION_BASE *base, S32 release) {
    T_TERRAIN_TEXTURE_EDITOR_EDIT *edit;

    if (!base OR !base->Used OR base->CubeX != CurrentCubeX OR base->CubeY != CurrentCubeY OR !MapPolyGround)
        return;
    edit = TerrainTextureEditorGetOrCreateEdit(base->CubeX, base->CubeY, base->Index, &MapPolyGround[base->Index]);
    if (edit) {
        edit->Poly = base->Poly;
        TerrainTextureEditorApplyEditToPoly(edit, &MapPolyGround[base->Index]);
    }
    if (release)
        base->Used = FALSE;
}

static void TerrainRegionEditorSetCell(T_TERRAIN_REGION *region, S32 x, S32 z,
                                       S32 texture0, S32 texture1, S32 sens0, S32 sens1,
                                       S32 grass_edge, S32 footstep0, S32 footstep1) {
    S32 index0;
    S32 index1;
    T_TERRAIN_TEXTURE_EDITOR_EDIT *edit0;
    T_TERRAIN_TEXTURE_EDITOR_EDIT *edit1;

    if (!MapPolyGround OR region->CubeX != CurrentCubeX OR region->CubeY != CurrentCubeY)
        return;
    if (x < 0 OR x >= NB_COTE OR z < 0 OR z >= NB_COTE)
        return;

    index0 = (z * NB_COTE + x) * 2;
    index1 = index0 + 1;
    if (!TerrainRegionEditorRegionCovers(region, x, z, 0))
        edit0 = NULL;
    else {
        TerrainRegionEditorCacheBase(region->CubeX, region->CubeY, index0);
        edit0 = TerrainTextureEditorGetOrCreateEdit(region->CubeX, region->CubeY, index0, &MapPolyGround[index0]);
    }
    if (!TerrainRegionEditorRegionCovers(region, x, z, 1))
        edit1 = NULL;
    else {
        TerrainRegionEditorCacheBase(region->CubeX, region->CubeY, index1);
        edit1 = TerrainTextureEditorGetOrCreateEdit(region->CubeX, region->CubeY, index1, &MapPolyGround[index1]);
    }
    if (!edit0 AND !edit1)
        return;

    if (region->Type == TERRAIN_REGION_TYPE_DIRT) {
        if (edit0) {
            edit0->Poly.Bank = 1;
            edit0->Poly.TexFlag = 0;
            edit0->Poly.PolyFlag = 3;
            edit0->Poly.Sens = sens0 ? 1 : 0;
            edit0->Poly.Col = 0;
            edit0->Poly.CodeJeu = 0;
            edit0->Poly.SampleStep = 0;
            edit0->Poly.IndexTex = 0;
        }
        if (edit1) {
            edit1->Poly.Bank = 1;
            edit1->Poly.TexFlag = 0;
            edit1->Poly.PolyFlag = 3;
            edit1->Poly.Sens = sens1 ? 1 : 0;
            edit1->Poly.Col = 0;
            edit1->Poly.CodeJeu = 0;
            edit1->Poly.SampleStep = 0;
            edit1->Poly.IndexTex = 0;
        }
    } else {
        texture0 = TerrainTextureEditorResolveCatalogTexture(texture0);
        texture1 = TerrainTextureEditorResolveCatalogTexture(texture1);
        if (texture0 < 0 OR texture1 < 0)
            return;
        if (edit0) {
            TerrainTextureEditorSetEditTexture(edit0, texture0);
            edit0->Poly.Sens = sens0 ? 1 : 0;
            edit0->Poly.Col = 0;
            edit0->Poly.CodeJeu = 0;
            if (footstep0 >= 0)
                edit0->Poly.SampleStep = (U32)footstep0;
        }
        if (edit1) {
            TerrainTextureEditorSetEditTexture(edit1, texture1);
            edit1->Poly.Sens = sens1 ? 1 : 0;
            edit1->Poly.Col = 0;
            edit1->Poly.CodeJeu = 0;
            if (footstep1 >= 0)
                edit1->Poly.SampleStep = (U32)footstep1;
        }
    }
    if (region->Type == TERRAIN_REGION_TYPE_GRASS) {
        if (edit0) {
            edit0->Poly.Bank = 1;
            edit0->Poly.TexFlag = 3;
            edit0->Poly.PolyFlag = grass_edge ? 3 : 0;
            edit0->Poly.SampleStep = 2;
            edit0->Poly.CodeJeu = 0;
        }
        if (edit1) {
            edit1->Poly.Bank = 1;
            edit1->Poly.TexFlag = 3;
            edit1->Poly.PolyFlag = grass_edge ? 3 : 0;
            edit1->Poly.SampleStep = 2;
            edit1->Poly.CodeJeu = 0;
        }
    }
    if (region->Type == TERRAIN_REGION_TYPE_PAVEMENT) {
        if (edit0)
            edit0->Poly.TexFlag = 3;
        if (edit1)
            edit1->Poly.TexFlag = 3;
    }
    if (region->Type == TERRAIN_REGION_TYPE_ROCKS OR region->Type == TERRAIN_REGION_TYPE_CLIFFSIDE) {
        if (edit0)
            edit0->Poly.Col = 1;
        if (edit1)
            edit1->Poly.Col = 1;
    }
    if (edit0)
        TerrainTextureEditorApplyEditToPoly(edit0, &MapPolyGround[index0]);
    if (edit1)
        TerrainTextureEditorApplyEditToPoly(edit1, &MapPolyGround[index1]);
}

static S32 TerrainRegionEditorGrassCellPresent(T_TERRAIN_REGION *region, S32 x, S32 z) {
    U32 chance;

    if (!TerrainRegionEditorRegionCovers(region, x, z, 0)
            AND !TerrainRegionEditorRegionCovers(region, x, z, 1))
        return FALSE;
    if (x == region->X0 AND z == region->Z0)
        return TRUE;
    chance = TerrainRegionEditorHash(region->Seed, x, z) % 100;
    return chance >= (U32)region->Patchiness;
}

static void TerrainRegionEditorGrassTextures(T_TERRAIN_REGION *region, S32 x, S32 z,
                                             S32 *texture0, S32 *texture1, S32 *sens) {
    static const S32 interior[4][4] = {
        {16, 22, 11, 9},
        {18, 24, 7, 5},
        {20, 26, 28, 30},
        {14, 32, 34, 36}};
    S32 north = z > region->Z0 AND TerrainRegionEditorGrassCellPresent(region, x, z - 1);
    S32 south = z < region->Z1 AND TerrainRegionEditorGrassCellPresent(region, x, z + 1);
    S32 west = x > region->X0 AND TerrainRegionEditorGrassCellPresent(region, x - 1, z);
    S32 east = x < region->X1 AND TerrainRegionEditorGrassCellPresent(region, x + 1, z);
    S32 base;
    U32 variant;

    *sens = 0;
    if (!north AND !west) {
        *texture0 = 75;
        *texture1 = 76;
        return;
    }
    if (!north AND !east) {
        *texture0 = 78;
        *texture1 = 77;
        return;
    }
    if (!south AND !west) {
        *texture0 = 77;
        *texture1 = 78;
        return;
    }
    if (!south AND !east) {
        *texture0 = 76;
        *texture1 = 75;
        return;
    }
    if (!north) {
        base = (TerrainRegionEditorHash(region->Seed, x, z) & 1) ? 79 : 81;
    } else if (!south) {
        base = (TerrainRegionEditorHash(region->Seed, x, z) & 1) ? 145 : 143;
    } else if (!west) {
        base = (z & 1) ? 2 : 0;
    } else if (!east) {
        base = (z & 1) ? 140 : 138;
    } else {
        base = interior[(z - region->Z0) & 3][(x - region->X0) & 3];
        variant = TerrainRegionEditorHash(region->Seed + 17, x, z) % 19;
        if (variant == 0 AND(base == 16 OR base == 20))
            base = 111;
        else if (variant == 1 AND(base == 18 OR base == 14))
            base = 12;
        else if (variant == 2 AND base == 34)
            base = 113;
        *sens = 1;
    }

    *texture0 = base;
    if (base == 11 OR base == 9 OR base == 7 OR base == 5)
        *texture1 = base - 1;
    else
        *texture1 = base + 1;
}

static S32 TerrainRegionEditorRiprapCellPresent(T_TERRAIN_REGION *region, S32 x, S32 z) {
    return TerrainRegionEditorRegionCovers(region, x, z, 0)
           OR TerrainRegionEditorRegionCovers(region, x, z, 1);
}

static S32 TerrainRegionEditorRiprapDistance(T_TERRAIN_REGION *region, S32 x, S32 z, S32 dx, S32 dz) {
    S32 distance;
    for (distance = 1; distance <= 2; distance++) {
        if (!TerrainRegionEditorRiprapCellPresent(region, x + dx * distance, z + dz * distance))
            return distance;
    }
    return 3;
}

static S32 TerrainRegionEditorRiprapTextures(T_TERRAIN_REGION *region, S32 x, S32 z,
                                             S32 *texture0, S32 *texture1, S32 *sens) {
    S32 north = TerrainRegionEditorRiprapDistance(region, x, z, 0, -1);
    S32 east = TerrainRegionEditorRiprapDistance(region, x, z, 1, 0);
    S32 south = TerrainRegionEditorRiprapDistance(region, x, z, 0, 1);
    S32 west = TerrainRegionEditorRiprapDistance(region, x, z, -1, 0);
    S32 nearest = north;
    S32 base;

    if (east < nearest)
        nearest = east;
    if (south < nearest)
        nearest = south;
    if (west < nearest)
        nearest = west;
    if (nearest > 2)
        return FALSE;

    *sens = 0;
    if (north == nearest AND west == nearest) {
        base = 630;
    } else if (south == nearest AND east == nearest) {
        base = 630;
        *sens = 1;
    } else if (north == nearest AND east == nearest) {
        base = 632;
        *sens = 1;
    } else if (south == nearest AND west == nearest) {
        base = 632;
    } else if (north == nearest) {
        base = 560;
    } else if (east == nearest) {
        base = 562;
        *sens = 1;
    } else if (south == nearest) {
        base = 564;
    } else {
        base = 566;
        *sens = 1;
    }
    *texture0 = base;
    *texture1 = base + 1;
    return TRUE;
}

static void TerrainRegionEditorBake(T_TERRAIN_REGION *region) {
    T_EDITOR_TERRAIN_PATTERN_REGION pattern_region;
    U8 present[NB_COTE][NB_COTE];
    S32 region_index;
    S32 i;
    S32 x;
    S32 z;
    S32 count = 0;

    if (!region OR region->CubeX != CurrentCubeX OR region->CubeY != CurrentCubeY) {
        Editor_SetStatus("Terrain Region bake requires its cube to be loaded");
        return;
    }
    if (!TerrainRegionEditorPatternsAvailable()) {
        Editor_SetStatus("Terrain Region patterns are only available on Citadel and Proxima");
        return;
    }

    memset(present, 0, sizeof(present));
    region_index = (S32)(region - TerrainRegions);
    for (i = 0; i < TERRAIN_REGION_CELL_MAX; i++) {
        T_TERRAIN_REGION_CELL *cell = &TerrainRegionCells[i];
        if (cell->Used AND cell->Region == region_index)
            present[cell->Z][cell->X] = TRUE;
    }

    pattern_region.X0 = region->X0;
    pattern_region.Z0 = region->Z0;
    pattern_region.X1 = region->X1;
    pattern_region.Z1 = region->Z1;
    pattern_region.AnchorX = region->AnchorX;
    pattern_region.AnchorZ = region->AnchorZ;

    for (z = region->Z0; z <= region->Z1; z++) {
        for (x = region->X0; x <= region->X1; x++) {
            T_EDITOR_TERRAIN_PATTERN_CELL pattern_cell;
            S32 texture0;
            S32 texture1;
            S32 sens;
            S32 sens0;
            S32 sens1;
            S32 grass_edge = FALSE;
            S32 footstep0 = -1;
            S32 footstep1 = -1;

            if (!present[z][x])
                continue;
            pattern_region.NorthWest = x > 0 AND z > 0 AND present[z - 1][x - 1];
            pattern_region.North = z > 0 AND present[z - 1][x];
            pattern_region.NorthEast = x < NB_COTE - 1 AND z > 0 AND present[z - 1][x + 1];
            pattern_region.West = x > 0 AND present[z][x - 1];
            pattern_region.East = x < NB_COTE - 1 AND present[z][x + 1];
            pattern_region.SouthWest = x > 0 AND z < NB_COTE - 1 AND present[z + 1][x - 1];
            pattern_region.South = z < NB_COTE - 1 AND present[z + 1][x];
            pattern_region.SouthEast = x < NB_COTE - 1 AND z < NB_COTE - 1 AND present[z + 1][x + 1];
            if (region->Type == TERRAIN_REGION_TYPE_GRASS) {
                if (!TerrainRegionEditorGrassCellPresent(region, x, z))
                    continue;
                grass_edge = !(z > region->Z0 AND TerrainRegionEditorGrassCellPresent(region, x, z - 1))
                             OR !(z < region->Z1 AND TerrainRegionEditorGrassCellPresent(region, x, z + 1))
                             OR !(x > region->X0 AND TerrainRegionEditorGrassCellPresent(region, x - 1, z))
                             OR !(x < region->X1 AND TerrainRegionEditorGrassCellPresent(region, x + 1, z));
                TerrainRegionEditorGrassTextures(region, x, z, &texture0, &texture1, &sens);
                sens0 = sens1 = sens;
            } else if (region->Type == TERRAIN_REGION_TYPE_GRASS_INSET) {
                EditorTerrainPatternGrassInset(&pattern_region, x, z, &pattern_cell);
                texture0 = pattern_cell.Texture0;
                texture1 = pattern_cell.Texture1;
                sens0 = pattern_cell.Sens0;
                sens1 = pattern_cell.Sens1;
            } else if (region->Type == TERRAIN_REGION_TYPE_ASPHALT) {
                EditorTerrainPatternAsphalt(&pattern_region, x, z, &pattern_cell);
                texture0 = pattern_cell.Texture0;
                texture1 = pattern_cell.Texture1;
                sens0 = pattern_cell.Sens0;
                sens1 = pattern_cell.Sens1;
            } else if (region->Type == TERRAIN_REGION_TYPE_GRASS_PAVEMENT) {
                EditorTerrainPatternGrassPavement(&pattern_region, x, z, &pattern_cell);
                texture0 = pattern_cell.Texture0;
                texture1 = pattern_cell.Texture1;
                sens0 = pattern_cell.Sens0;
                sens1 = pattern_cell.Sens1;
                footstep0 = pattern_cell.Footstep0;
                footstep1 = pattern_cell.Footstep1;
            } else if (region->Type == TERRAIN_REGION_TYPE_RIPRAP) {
                if (!TerrainRegionEditorRiprapTextures(region, x, z, &texture0, &texture1, &sens))
                    continue;
                sens0 = sens1 = sens;
            } else if (region->Type == TERRAIN_REGION_TYPE_PAVEMENT_ALT) {
                EditorTerrainPatternPavementAlt(&pattern_region, x, z, &pattern_cell);
                texture0 = pattern_cell.Texture0;
                texture1 = pattern_cell.Texture1;
                sens0 = pattern_cell.Sens0;
                sens1 = pattern_cell.Sens1;
                footstep0 = pattern_cell.Footstep0;
                footstep1 = pattern_cell.Footstep1;
            } else if (region->Type == TERRAIN_REGION_TYPE_ROCKS) {
                EditorTerrainPatternRocks(&pattern_region, x, z, &pattern_cell);
                texture0 = pattern_cell.Texture0;
                texture1 = pattern_cell.Texture1;
                sens0 = pattern_cell.Sens0;
                sens1 = pattern_cell.Sens1;
                footstep0 = pattern_cell.Footstep0;
                footstep1 = pattern_cell.Footstep1;
            } else if (region->Type == TERRAIN_REGION_TYPE_CLIFFSIDE) {
                EditorTerrainPatternCliffside(&pattern_region, x, z, &pattern_cell);
                texture0 = pattern_cell.Texture0;
                texture1 = pattern_cell.Texture1;
                sens0 = pattern_cell.Sens0;
                sens1 = pattern_cell.Sens1;
                footstep0 = pattern_cell.Footstep0;
                footstep1 = pattern_cell.Footstep1;
            } else if (region->Type == TERRAIN_REGION_TYPE_DIRT) {
                EditorTerrainPatternDirt(&pattern_region, x, z, &pattern_cell);
                texture0 = pattern_cell.Texture0;
                texture1 = pattern_cell.Texture1;
                sens0 = pattern_cell.Sens0;
                sens1 = pattern_cell.Sens1;
                footstep0 = pattern_cell.Footstep0;
                footstep1 = pattern_cell.Footstep1;
            } else {
                EditorTerrainPatternPavement(&pattern_region, x, z, &pattern_cell);
                texture0 = pattern_cell.Texture0;
                texture1 = pattern_cell.Texture1;
                sens0 = pattern_cell.Sens0;
                sens1 = pattern_cell.Sens1;
                footstep0 = pattern_cell.Footstep0;
                footstep1 = pattern_cell.Footstep1;
            }
            TerrainRegionEditorSetCell(region, x, z, texture0, texture1, sens0, sens1,
                                       grass_edge, footstep0, footstep1);
            count++;
        }
    }

    if (region->Type == TERRAIN_REGION_TYPE_GRASS)
        Editor_SetStatus("Baked Grass Patch region");
    else if (region->Type == TERRAIN_REGION_TYPE_GRASS_INSET)
        Editor_SetStatus("Baked Grass Inset region");
    else if (region->Type == TERRAIN_REGION_TYPE_ASPHALT)
        Editor_SetStatus("Baked Asphalt region");
    else if (region->Type == TERRAIN_REGION_TYPE_GRASS_PAVEMENT)
        Editor_SetStatus("Baked Grass Pavement region");
    else if (region->Type == TERRAIN_REGION_TYPE_RIPRAP)
        Editor_SetStatus("Baked Riprap region");
    else if (region->Type == TERRAIN_REGION_TYPE_PAVEMENT_ALT)
        Editor_SetStatus("Baked Pavement (Alt) region");
    else if (region->Type == TERRAIN_REGION_TYPE_ROCKS)
        Editor_SetStatus("Baked Rocks region");
    else if (region->Type == TERRAIN_REGION_TYPE_CLIFFSIDE)
        Editor_SetStatus("Baked Cliffside region");
    else if (region->Type == TERRAIN_REGION_TYPE_DIRT)
        Editor_SetStatus("Baked Dirt region");
    else
        Editor_SetStatus("Baked Pavement region");
    Log_Info("[editor] baked terrain region type=%d cells=%d seed=%d patchiness=%d",
             (int)region->Type, (int)count, (int)region->Seed, (int)region->Patchiness);
}

static S32 TerrainRegionEditorTriangleOwned(S32 cube_x, S32 cube_y, S32 index) {
    S32 i;
    S32 x = (index / 2) % NB_COTE;
    S32 z = (index / 2) / NB_COTE;
    S32 half = index & 1;
    for (i = 0; i < TERRAIN_REGION_CELL_MAX; i++) {
        T_TERRAIN_REGION_CELL *cell = &TerrainRegionCells[i];
        if (cell->Used AND cell->CubeX == cube_x AND cell->CubeY == cube_y AND cell->X == x AND cell->Z == z AND TerrainRegionEditorCellCovers(cell, half))
            return TRUE;
    }
    return FALSE;
}

static S32 TerrainRegionEditorCellsConnect(T_TERRAIN_REGION_CELL *a, T_TERRAIN_REGION_CELL *b) {
    S32 dx;
    S32 dz;

    if (!a->Used OR !b->Used OR a->CubeX != b->CubeX OR a->CubeY != b->CubeY OR a->Type != b->Type)
        return FALSE;
    dx = a->X - b->X;
    dz = a->Z - b->Z;
    if (dx == 0 AND dz == 0)
        return TRUE;
    return (dx == 0 AND(dz == -1 OR dz == 1))
           OR(dz == 0 AND(dx == -1 OR dx == 1));
}

static void TerrainRegionEditorRebuildAndBakeCurrentCube(void) {
    static S32 assigned[TERRAIN_REGION_CELL_MAX];
    static S32 queue[TERRAIN_REGION_CELL_MAX];
    T_TERRAIN_REGION old_regions[TERRAIN_REGION_MAX];
    S32 old_selected = TerrainRegionEditorSelected;
    S32 new_selected = -1;
    S32 i;

    memcpy(old_regions, TerrainRegions, sizeof(old_regions));
    memset(TerrainRegions, 0, sizeof(TerrainRegions));
    memset(assigned, 0, sizeof(assigned));

    for (i = 0; i < TERRAIN_REGION_BASE_MAX; i++) {
        T_TERRAIN_REGION_BASE *base = &TerrainRegionBases[i];
        if (base->Used AND base->CubeX == CurrentCubeX AND base->CubeY == CurrentCubeY)
            TerrainRegionEditorRestoreBase(base, FALSE);
    }

    for (i = 0; i < TERRAIN_REGION_CELL_MAX; i++) {
        T_TERRAIN_REGION_CELL *start = &TerrainRegionCells[i];
        S32 overlap[TERRAIN_REGION_MAX];
        S32 head = 0;
        S32 tail = 0;
        S32 region_index;
        S32 best_old = -1;
        S32 best_overlap = 0;
        S32 j;

        if (!start->Used OR assigned[i])
            continue;
        for (region_index = 0; region_index < TERRAIN_REGION_MAX; region_index++) {
            if (!TerrainRegions[region_index].Used)
                break;
        }
        if (region_index >= TERRAIN_REGION_MAX)
            break;
        memset(overlap, 0, sizeof(overlap));
        queue[tail++] = i;
        assigned[i] = TRUE;
        TerrainRegions[region_index].Used = TRUE;
        TerrainRegions[region_index].CubeX = start->CubeX;
        TerrainRegions[region_index].CubeY = start->CubeY;
        TerrainRegions[region_index].X0 = TerrainRegions[region_index].X1 = start->X;
        TerrainRegions[region_index].Z0 = TerrainRegions[region_index].Z1 = start->Z;
        TerrainRegions[region_index].Type = start->Type;
        while (head < tail) {
            S32 cell_index = queue[head++];
            T_TERRAIN_REGION_CELL *cell = &TerrainRegionCells[cell_index];
            S32 old_region = cell->Region;
            cell->Region = region_index;
            if (old_region == old_selected)
                new_selected = region_index;
            if (cell->X < TerrainRegions[region_index].X0)
                TerrainRegions[region_index].X0 = cell->X;
            if (cell->X > TerrainRegions[region_index].X1)
                TerrainRegions[region_index].X1 = cell->X;
            if (cell->Z < TerrainRegions[region_index].Z0)
                TerrainRegions[region_index].Z0 = cell->Z;
            if (cell->Z > TerrainRegions[region_index].Z1)
                TerrainRegions[region_index].Z1 = cell->Z;
            if (old_region >= 0 AND old_region < TERRAIN_REGION_MAX)
                overlap[old_region]++;
            for (j = 0; j < TERRAIN_REGION_CELL_MAX; j++) {
                if (!assigned[j] AND TerrainRegionEditorCellsConnect(cell, &TerrainRegionCells[j])) {
                    assigned[j] = TRUE;
                    queue[tail++] = j;
                }
            }
        }
        for (j = 0; j < TERRAIN_REGION_MAX; j++) {
            if (overlap[j] > best_overlap) {
                best_overlap = overlap[j];
                best_old = j;
            }
        }
        if (best_old >= 0 AND old_regions[best_old].Used) {
            TerrainRegions[region_index].Seed = old_regions[best_old].Seed;
            TerrainRegions[region_index].Patchiness = old_regions[best_old].Patchiness;
            TerrainRegions[region_index].AnchorX = old_regions[best_old].AnchorX;
            TerrainRegions[region_index].AnchorZ = old_regions[best_old].AnchorZ;
        } else {
            TerrainRegions[region_index].Seed = (S32)(TimerRefHR ^ ((U32)start->X << 16) ^ ((U32)start->Z << 8));
            TerrainRegions[region_index].Patchiness = start->Type == TERRAIN_REGION_TYPE_GRASS ? 10 : 0;
            TerrainRegions[region_index].AnchorX = start->X;
            TerrainRegions[region_index].AnchorZ = start->Z;
        }
    }

    TerrainRegionEditorSelected = new_selected;
    for (i = 0; i < TERRAIN_REGION_MAX; i++) {
        if (TerrainRegions[i].Used AND TerrainRegions[i].CubeX == CurrentCubeX AND TerrainRegions[i].CubeY == CurrentCubeY)
            TerrainRegionEditorBake(&TerrainRegions[i]);
    }
    for (i = 0; i < TERRAIN_REGION_BASE_MAX; i++) {
        T_TERRAIN_REGION_BASE *base = &TerrainRegionBases[i];
        if (base->Used AND base->CubeX == CurrentCubeX AND base->CubeY == CurrentCubeY AND !TerrainRegionEditorTriangleOwned(base->CubeX, base->CubeY, base->Index))
            TerrainRegionEditorRestoreBase(base, TRUE);
    }
}

static T_TERRAIN_REGION_CELL *TerrainRegionEditorAllocCell(void) {
    S32 i;
    for (i = 0; i < TERRAIN_REGION_CELL_MAX; i++) {
        if (!TerrainRegionCells[i].Used)
            return &TerrainRegionCells[i];
    }
    return NULL;
}

static S32 TerrainRegionEditorPaintCell(S32 x, S32 z, S32 half, S32 type) {
    T_TERRAIN_REGION_CELL *whole = TerrainRegionEditorFindCell(CurrentCubeX, CurrentCubeY, x, z, -1);
    T_TERRAIN_REGION_CELL *cell;
    S32 i;
    S32 changed = FALSE;

    if (half < 0) {
        for (i = 0; i < TERRAIN_REGION_CELL_MAX; i++) {
            cell = &TerrainRegionCells[i];
            if (cell->Used AND cell->CubeX == CurrentCubeX AND cell->CubeY == CurrentCubeY AND cell->X == x AND cell->Z == z) {
                if (cell->Half == -1 AND cell->Type == type)
                    return FALSE;
                cell->Used = FALSE;
                changed = TRUE;
            }
        }
        cell = TerrainRegionEditorAllocCell();
    } else {
        if (whole) {
            S32 old_type = whole->Type;
            whole->Used = FALSE;
            cell = TerrainRegionEditorAllocCell();
            if (cell) {
                cell->Used = TRUE;
                cell->CubeX = CurrentCubeX;
                cell->CubeY = CurrentCubeY;
                cell->X = x;
                cell->Z = z;
                cell->Half = half ^ 1;
                cell->Type = old_type;
                cell->Region = -1;
            }
        }
        cell = TerrainRegionEditorFindCell(CurrentCubeX, CurrentCubeY, x, z, half);
        if (cell AND cell->Type == type)
            return FALSE;
        if (!cell)
            cell = TerrainRegionEditorAllocCell();
    }
    if (!cell)
        return changed;
    cell->Used = TRUE;
    cell->CubeX = CurrentCubeX;
    cell->CubeY = CurrentCubeY;
    cell->X = x;
    cell->Z = z;
    cell->Half = half;
    cell->Type = type;
    cell->Region = -1;
    return TRUE;
}

static S32 TerrainRegionEditorEraseCell(S32 x, S32 z, S32 half) {
    S32 i;
    S32 changed = FALSE;
    for (i = 0; i < TERRAIN_REGION_CELL_MAX; i++) {
        T_TERRAIN_REGION_CELL *cell = &TerrainRegionCells[i];
        if (!cell->Used OR cell->CubeX != CurrentCubeX OR cell->CubeY != CurrentCubeY OR cell->X != x OR cell->Z != z)
            continue;
        if (half < 0 OR cell->Half == half) {
            cell->Used = FALSE;
            changed = TRUE;
        } else if (cell->Half < 0) {
            cell->Half = half ^ 1;
            changed = TRUE;
        }
    }
    return changed;
}

S32 TerrainRegionEditorHasSelected(void) {
    return TerrainRegionEditorSelected >= 0 AND TerrainRegionEditorSelected < TERRAIN_REGION_MAX
                                                                                  AND TerrainRegions[TerrainRegionEditorSelected]
                                                                                      .Used;
}

void TerrainRegionEditorClearSelection(void) {
    TerrainRegionEditorSelected = -1;
}

S32 TerrainRegionEditorGetFieldValue(S32 field) {
    T_TERRAIN_REGION *region;

    if (!TerrainRegionEditorHasSelected())
        return 0;
    region = &TerrainRegions[TerrainRegionEditorSelected];
    switch (field) {
    case TERRAIN_REGION_FIELD_TYPE:
        return region->Type;
    case TERRAIN_REGION_FIELD_SEED:
        return region->Seed;
    case TERRAIN_REGION_FIELD_PATCHINESS:
        return region->Patchiness;
    case TERRAIN_REGION_FIELD_X0:
        return region->X0;
    case TERRAIN_REGION_FIELD_Z0:
        return region->Z0;
    case TERRAIN_REGION_FIELD_X1:
        return region->X1;
    case TERRAIN_REGION_FIELD_Z1:
        return region->Z1;
    }
    return 0;
}

S32 TerrainRegionEditorSetFieldValue(S32 field, S32 value) {
    T_TERRAIN_REGION *region;

    if (!TerrainRegionEditorHasSelected())
        return FALSE;
    region = &TerrainRegions[TerrainRegionEditorSelected];
    if (field == TERRAIN_REGION_FIELD_TYPE) {
        S32 i;
        if (value < 0)
            value = 0;
        if (value >= TERRAIN_REGION_TYPE_COUNT)
            value = TERRAIN_REGION_TYPE_COUNT - 1;
        region->Type = value;
        for (i = 0; i < TERRAIN_REGION_CELL_MAX; i++) {
            if (TerrainRegionCells[i].Used AND TerrainRegionCells[i].Region == TerrainRegionEditorSelected)
                TerrainRegionCells[i].Type = value;
        }
        TerrainRegionEditorPaletteType = region->Type;
        TerrainRegionEditorRebuildAndBakeCurrentCube();
        return TRUE;
    }
    if (field == TERRAIN_REGION_FIELD_SEED) {
        region->Seed = value;
        TerrainRegionEditorRebuildAndBakeCurrentCube();
        return TRUE;
    }
    if (field == TERRAIN_REGION_FIELD_PATCHINESS) {
        if (region->Type != TERRAIN_REGION_TYPE_GRASS)
            return FALSE;
        if (value < 0)
            value = 0;
        if (value > 100)
            value = 100;
        region->Patchiness = value;
        TerrainRegionEditorRebuildAndBakeCurrentCube();
        return TRUE;
    }
    return FALSE;
}

S32 TerrainRegionEditorActivateField(S32 field) {
    T_TERRAIN_REGION *region;

    if (!TerrainRegionEditorHasSelected())
        return FALSE;
    region = &TerrainRegions[TerrainRegionEditorSelected];
    if (field == TERRAIN_REGION_FIELD_RANDOMIZE) {
        region->Seed = (S32)(TimerRefHR ^ ((U32)region->X0 << 16) ^ ((U32)region->Z0 << 8));
        TerrainRegionEditorRebuildAndBakeCurrentCube();
        Editor_SetStatus("Randomized Terrain Region seed");
        return TRUE;
    }
    if (field == TERRAIN_REGION_FIELD_BAKE) {
        TerrainRegionEditorBake(region);
        return TRUE;
    }
    return FALSE;
}

static S32 TerrainTextureEditorSelectedTextureEntry(void) {
    S32 i;
    S32 texture_entry = -3;

    if (!MapPolyGround)
        return -3;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];
        T_HALF_POLY *poly;
        T_TERRAIN_TEXTURE_EDITOR_EDIT *edit;
        S32 entry;

        if (!sel->Used)
            continue;
        if (sel->CubeX != CurrentCubeX OR sel->CubeY != CurrentCubeY)
            continue;
        if (sel->Index < 0 OR sel->Index >= NB_COTE * NB_COTE * 2)
            continue;

        poly = &MapPolyGround[sel->Index];
        edit = TerrainTextureEditorFindEdit(sel->CubeX, sel->CubeY, sel->Index);
        if (edit)
            poly = &edit->Poly;

        if (!poly->TexFlag) {
            entry = -1;
        } else if (TerrainTextureEditorFullPalette) {
            entry = TerrainTextureEditorFindTexDef(TerrainTextureEditorCatalog,
                                                   TerrainTextureEditorCatalogCount,
                                                   &ListTexDef[poly->IndexTex]);
        } else {
            entry = poly->IndexTex;
        }
        if (TerrainTextureEditorQuadMode AND entry >= 0)
            entry /= 2;
        if (texture_entry == -3) {
            texture_entry = entry;
        } else if (texture_entry != entry) {
            return -3;
        }
    }

    return texture_entry;
}

static S32 TerrainTextureEditorPaletteTextureCount(void) {
    if (TerrainTextureEditorQuadMode)
        return (TerrainTextureEditorFullPalette ? TerrainTextureEditorCatalogCount : TerrainTextureEditorCubeTexDefCount) / 2;
    return TerrainTextureEditorFullPalette ? TerrainTextureEditorCatalogCount : TerrainTextureEditorCubeTexDefCount;
}

static S32 TerrainTextureEditorPaletteRows(void) {
    S32 panel_h;
    S32 rows;

    panel_h = (S32)ModeDesiredY - TERRAIN_TEXTURE_PALETTE_PAD * 2;
    rows = (panel_h - TERRAIN_TEXTURE_PALETTE_PAD * 2 + TERRAIN_TEXTURE_PALETTE_GAP) / (TERRAIN_TEXTURE_PALETTE_SWATCH + TERRAIN_TEXTURE_PALETTE_GAP);
    if (rows < 1)
        rows = 1;
    return rows;
}

static S32 TerrainTextureEditorPaletteColumns(void) {
    S32 rows;
    S32 wanted;
    S32 max_cols;

    rows = TerrainTextureEditorPaletteRows();
    wanted = (TerrainTextureEditorPaletteTextureCount() + 1 + rows - 1) / rows;
    if (wanted < TERRAIN_TEXTURE_PALETTE_MIN_COLS)
        wanted = TERRAIN_TEXTURE_PALETTE_MIN_COLS;

    max_cols = (((S32)ModeDesiredX / TERRAIN_TEXTURE_PALETTE_MAX_WIDTH_DIV) - TERRAIN_TEXTURE_PALETTE_PAD * 2 + TERRAIN_TEXTURE_PALETTE_GAP) / (TERRAIN_TEXTURE_PALETTE_SWATCH + TERRAIN_TEXTURE_PALETTE_GAP);
    if (max_cols < TERRAIN_TEXTURE_PALETTE_MIN_COLS)
        max_cols = TERRAIN_TEXTURE_PALETTE_MIN_COLS;
    if (max_cols > TERRAIN_TEXTURE_PALETTE_MAX_COLS)
        max_cols = TERRAIN_TEXTURE_PALETTE_MAX_COLS;
    if (wanted > max_cols)
        wanted = max_cols;

    return wanted;
}

static S32 TerrainTextureEditorPalettePageSize(void) {
    return TerrainTextureEditorPaletteRows() * TerrainTextureEditorPaletteColumns();
}

static S32 TerrainTextureEditorPalettePageCount(void) {
    S32 page_size = TerrainTextureEditorPalettePageSize();
    S32 total = TerrainTextureEditorPaletteTextureCount() + 1;

    if (page_size < 1)
        return 1;
    return (total + page_size - 1) / page_size;
}

static void TerrainTextureEditorPaletteClampPage(void) {
    S32 page_count = TerrainTextureEditorPalettePageCount();

    if (TerrainTextureEditorPalettePage < 0)
        TerrainTextureEditorPalettePage = page_count - 1;
    if (TerrainTextureEditorPalettePage >= page_count)
        TerrainTextureEditorPalettePage = 0;
}

static S32 TerrainTextureEditorPaletteVisibleCount(void) {
    S32 count = TerrainTextureEditorPalettePageSize();
    S32 remaining;

    TerrainTextureEditorPaletteClampPage();
    remaining = TerrainTextureEditorPaletteTextureCount() + 1 - TerrainTextureEditorPalettePage * TerrainTextureEditorPalettePageSize();
    if (count > remaining)
        count = remaining;
    return count;
}

static void TerrainTextureEditorPaletteRect(S32 *x0, S32 *y0, S32 *x1, S32 *y1) {
    S32 width;
    S32 visible_count;
    S32 rows;
    S32 height;
    S32 cols;

    cols = TerrainTextureEditorPaletteColumns();
    width = TERRAIN_TEXTURE_PALETTE_PAD * 2 + cols * TERRAIN_TEXTURE_PALETTE_SWATCH + (cols - 1) * TERRAIN_TEXTURE_PALETTE_GAP;
    visible_count = TerrainTextureEditorPaletteVisibleCount();
    rows = (visible_count + cols - 1) / cols;
    if (rows < 1)
        rows = 1;
    height = TERRAIN_TEXTURE_PALETTE_PAD * 2 + rows * TERRAIN_TEXTURE_PALETTE_SWATCH + (rows - 1) * TERRAIN_TEXTURE_PALETTE_GAP;

    *x1 = (S32)ModeDesiredX - TERRAIN_TEXTURE_PALETTE_PAD - 1;
    *x0 = *x1 - width + 1;
    *y0 = TERRAIN_TEXTURE_PALETTE_PAD;
    *y1 = *y0 + height - 1;
}

static S32 TerrainTextureEditorPaletteEntryAt(S32 x, S32 y) {
    S32 x0;
    S32 y0;
    S32 x1;
    S32 y1;
    S32 relx;
    S32 rely;
    S32 col;
    S32 row;
    S32 cellw;
    S32 cellh;
    S32 entry;
    S32 cols;

    TerrainTextureEditorPaletteRect(&x0, &y0, &x1, &y1);
    if (x < x0 OR x > x1 OR y < y0 OR y > y1)
        return TERRAIN_TEXTURE_PALETTE_NONE;

    relx = x - x0 - TERRAIN_TEXTURE_PALETTE_PAD;
    rely = y - y0 - TERRAIN_TEXTURE_PALETTE_PAD;
    if (relx < 0 OR rely < 0)
        return TERRAIN_TEXTURE_PALETTE_NONE;

    cellw = TERRAIN_TEXTURE_PALETTE_SWATCH + TERRAIN_TEXTURE_PALETTE_GAP;
    cellh = TERRAIN_TEXTURE_PALETTE_SWATCH + TERRAIN_TEXTURE_PALETTE_GAP;
    col = relx / cellw;
    row = rely / cellh;
    cols = TerrainTextureEditorPaletteColumns();
    if (col < 0 OR col >= cols)
        return TERRAIN_TEXTURE_PALETTE_NONE;
    if ((relx - col * cellw) >= TERRAIN_TEXTURE_PALETTE_SWATCH)
        return TERRAIN_TEXTURE_PALETTE_NONE;
    if ((rely - row * cellh) >= TERRAIN_TEXTURE_PALETTE_SWATCH)
        return TERRAIN_TEXTURE_PALETTE_NONE;

    entry = row * cols + col;
    if (entry < 0 OR entry >= TerrainTextureEditorPaletteVisibleCount())
        return TERRAIN_TEXTURE_PALETTE_NONE;
    return TerrainTextureEditorPalettePage * TerrainTextureEditorPalettePageSize() + entry - 1;
}

static S32 TerrainTextureEditorPaletteContainsPoint(S32 x, S32 y) {
    S32 x0;
    S32 y0;
    S32 x1;
    S32 y1;

    TerrainTextureEditorPaletteRect(&x0, &y0, &x1, &y1);
    return (x >= x0 AND x <= x1 AND y >= y0 AND y <= y1);
}

static T_TERRAIN_TEXTURE_INTENSITY_EDIT *TerrainTextureEditorFindIntensityEdit(S32 cube_x, S32 cube_y, S32 index) {
    S32 i;

    for (i = 0; i < TERRAIN_TEXTURE_INTENSITY_MAX_EDITS; i++) {
        T_TERRAIN_TEXTURE_INTENSITY_EDIT *edit = &TerrainTextureIntensityEdits[i];

        if (edit->Used
                AND edit->CubeX == cube_x AND edit->CubeY == cube_y AND edit->Index == index)
            return edit;
    }
    return NULL;
}

static T_TERRAIN_TEXTURE_INTENSITY_EDIT *TerrainTextureEditorGetOrCreateIntensityEdit(S32 cube_x, S32 cube_y, S32 index, U8 intensity) {
    S32 i;
    T_TERRAIN_TEXTURE_INTENSITY_EDIT *edit;

    edit = TerrainTextureEditorFindIntensityEdit(cube_x, cube_y, index);
    if (edit)
        return edit;

    for (i = 0; i < TERRAIN_TEXTURE_INTENSITY_MAX_EDITS; i++) {
        edit = &TerrainTextureIntensityEdits[i];
        if (!edit->Used) {
            edit->Used = TRUE;
            edit->CubeX = cube_x;
            edit->CubeY = cube_y;
            edit->Index = index;
            edit->BaseIntensity = intensity;
            edit->Intensity = intensity;
            edit->HasExpectIntensity = TRUE;
            edit->SkipLogged = FALSE;
            return edit;
        }
    }

    Log_Warn("texedit intensity edit table is full; cannot track c%d,%d i%d",
             (int)cube_x,
             (int)cube_y,
             (int)index);
    return NULL;
}

static void TerrainTextureEditorAdjustIntensityVertex(S32 cube_x, S32 cube_y, S32 index, S32 delta) {
    T_TERRAIN_TEXTURE_INTENSITY_EDIT *edit;
    U8 intensity;
    S32 low;

    if (!MapIntensity)
        return;
    if (index < 0 OR index >= (NB_COTE + 1) * (NB_COTE + 1))
        return;

    edit = TerrainTextureEditorGetOrCreateIntensityEdit(cube_x, cube_y, index, MapIntensity[index]);
    if (!edit)
        return;

    intensity = edit->Intensity;
    low = (intensity & 15) + delta;
    if (low < 0)
        low = 0;
    if (low > 15)
        low = 15;

    edit->Intensity = (U8)((intensity & 0xF0) | low);
    if (cube_x == CurrentCubeX AND cube_y == CurrentCubeY)
        MapIntensity[index] = edit->Intensity;
}

S32 TerrainEditorGetSelectedCount(void) {
    S32 i;
    S32 count = 0;

    for (i = 0; i < TERRAIN_EDITOR_MAX_SELECTED; i++) {
        if (TerrainEditorSelList[i].Used)
            count++;
    }
    return count;
}

static S32 TerrainEditorSelectedFieldValue(T_TERRAIN_EDITOR_SEL *sel, S32 field) {
    T_TERRAIN_EDITOR_EDIT *height_edit;
    T_TERRAIN_TEXTURE_INTENSITY_EDIT *intensity_edit;

    switch (field) {
    case TERRAIN_EDITOR_FIELD_CUBE_X:
        return sel->CubeX;
    case TERRAIN_EDITOR_FIELD_CUBE_Y:
        return sel->CubeY;
    case TERRAIN_EDITOR_FIELD_X:
        return sel->Index % (NB_COTE + 1);
    case TERRAIN_EDITOR_FIELD_Z:
        return sel->Index / (NB_COTE + 1);
    case TERRAIN_EDITOR_FIELD_HEIGHT:
        height_edit = TerrainEditorFindEdit(sel->CubeX, sel->CubeY, sel->Index);
        if (height_edit)
            return (S32)height_edit->BaseY + height_edit->Dy;
        if (MapSommetY AND sel->CubeX == CurrentCubeX AND sel->CubeY == CurrentCubeY)
            return MapSommetY[sel->Index];
        break;
    case TERRAIN_EDITOR_FIELD_INTENSITY:
        intensity_edit = TerrainTextureEditorFindIntensityEdit(sel->CubeX, sel->CubeY, sel->Index);
        if (intensity_edit)
            return intensity_edit->Intensity & 15;
        if (MapIntensity AND sel->CubeX == CurrentCubeX AND sel->CubeY == CurrentCubeY)
            return MapIntensity[sel->Index] & 15;
        break;
    }
    return 0;
}

S32 TerrainEditorGetSelectedFieldValue(S32 field, S32 *mixed) {
    S32 i;
    S32 found = FALSE;
    S32 value = 0;

    if (mixed)
        *mixed = FALSE;
    if (field < 0 OR field >= TERRAIN_EDITOR_FIELD_COUNT)
        return 0;

    for (i = 0; i < TERRAIN_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_EDITOR_SEL *sel = &TerrainEditorSelList[i];
        S32 current;

        if (!sel->Used OR sel->Index < 0 OR sel->Index >= (NB_COTE + 1) * (NB_COTE + 1))
            continue;
        current = TerrainEditorSelectedFieldValue(sel, field);
        if (!found) {
            value = current;
            found = TRUE;
        } else if (value != current) {
            if (mixed)
                *mixed = TRUE;
            return value;
        }
    }
    return value;
}

S32 TerrainEditorSetSelectedField(S32 field, S32 value) {
    S32 i;
    S32 changed = 0;

    if (field != TERRAIN_EDITOR_FIELD_HEIGHT AND field != TERRAIN_EDITOR_FIELD_INTENSITY)
        return FALSE;
    if (field == TERRAIN_EDITOR_FIELD_INTENSITY) {
        if (value < 0)
            value = 0;
        if (value > 15)
            value = 15;
    } else {
        if (value < -32768)
            value = -32768;
        if (value > 32767)
            value = 32767;
    }

    for (i = 0; i < TERRAIN_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_EDITOR_SEL *sel = &TerrainEditorSelList[i];

        if (!sel->Used OR sel->CubeX != CurrentCubeX OR sel->CubeY != CurrentCubeY OR sel->Index < 0 OR sel->Index >= (NB_COTE + 1) * (NB_COTE + 1))
            continue;
        if (field == TERRAIN_EDITOR_FIELD_HEIGHT AND MapSommetY) {
            T_TERRAIN_EDITOR_EDIT *edit = TerrainEditorGetOrCreateEdit(sel->CubeX, sel->CubeY, sel->Index,
                                                                       &MapSommetY[sel->Index]);
            if (!edit)
                continue;
            edit->Dy = value - edit->BaseY;
            edit->SkipLogged = FALSE;
            TerrainEditorApplyEditToPoint(edit, &MapSommetY[sel->Index]);
            changed++;
        } else if (field == TERRAIN_EDITOR_FIELD_INTENSITY AND MapIntensity) {
            T_TERRAIN_TEXTURE_INTENSITY_EDIT *edit =
                TerrainTextureEditorGetOrCreateIntensityEdit(sel->CubeX, sel->CubeY, sel->Index,
                                                             MapIntensity[sel->Index]);
            if (!edit)
                continue;
            edit->Intensity = (U8)((edit->Intensity & 0xF0) | value);
            edit->SkipLogged = FALSE;
            MapIntensity[sel->Index] = edit->Intensity;
            changed++;
        }
    }
    return changed > 0;
}

static void TerrainTextureEditorTriangleVertices(S32 tri_index, S32 *v0, S32 *v1, S32 *v2) {
    S32 cell;
    S32 tri;
    S32 xi;
    S32 zi;
    S32 p0;
    S32 p1;
    S32 p2;
    S32 p3;
    S32 sens;

    cell = tri_index / 2;
    tri = tri_index & 1;
    zi = cell / NB_COTE;
    xi = cell - zi * NB_COTE;

    p0 = zi * (NB_COTE + 1) + xi;
    p1 = (zi + 1) * (NB_COTE + 1) + xi;
    p2 = (zi + 1) * (NB_COTE + 1) + xi + 1;
    p3 = zi * (NB_COTE + 1) + xi + 1;
    sens = MapPolyGround[cell * 2].Sens;

    if (sens == 0) {
        if (tri == 0) {
            *v0 = p0;
            *v1 = p1;
            *v2 = p2;
        } else {
            *v0 = p2;
            *v1 = p3;
            *v2 = p0;
        }
    } else {
        if (tri == 0) {
            *v0 = p3;
            *v1 = p0;
            *v2 = p1;
        } else {
            *v0 = p1;
            *v1 = p2;
            *v2 = p3;
        }
    }
}

static void TerrainTextureEditorAdjustSelectedIntensity(S32 delta) {
    S32 i;

    if (!MapPolyGround OR !MapIntensity)
        return;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];
        S32 v0;
        S32 v1;
        S32 v2;

        if (!sel->Used)
            continue;
        if (sel->CubeX != CurrentCubeX OR sel->CubeY != CurrentCubeY)
            continue;
        if (sel->Index < 0 OR sel->Index >= NB_COTE * NB_COTE * 2)
            continue;

        TerrainTextureEditorTriangleVertices(sel->Index, &v0, &v1, &v2);
        TerrainTextureEditorAdjustIntensityVertex(sel->CubeX, sel->CubeY, v0, delta);
        TerrainTextureEditorAdjustIntensityVertex(sel->CubeX, sel->CubeY, v1, delta);
        TerrainTextureEditorAdjustIntensityVertex(sel->CubeX, sel->CubeY, v2, delta);
    }
}

void TerrainTextureEditorBeginFrame(void) {
    S32 click;
    S32 up;
    S32 down;
    S32 left;
    S32 right;
    S32 step;
    S32 t;
    S32 q;
    S32 l;
    S32 full_palette;
    S32 palette_prev;
    S32 palette_next;
    S32 select_all;

    if (!TerrainTextureEditorSelect)
        return;
    if (Editor_IsCameraPanActive()) {
        TerrainTextureEditorLastClick = Click;
        TerrainTextureEditorLastUp = CheckKey(K_UP) ? TRUE : FALSE;
        TerrainTextureEditorLastDown = CheckKey(K_DOWN) ? TRUE : FALSE;
        TerrainTextureEditorLastLeft = CheckKey(K_LEFT) ? TRUE : FALSE;
        TerrainTextureEditorLastRight = CheckKey(K_RIGHT) ? TRUE : FALSE;
        TerrainTextureEditorLastT = CheckKey(K_T) ? TRUE : FALSE;
        TerrainTextureEditorLastQ = CheckKey(K_Q) ? TRUE : FALSE;
        TerrainTextureEditorLastL = CheckKey(K_L) ? TRUE : FALSE;
        TerrainTextureEditorLastFullPalette = (CheckKey(K_P) AND(CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT))) ? TRUE : FALSE;
        TerrainTextureEditorLastPalettePrev = CheckKey(K_LBRACKET) ? TRUE : FALSE;
        TerrainTextureEditorLastPaletteNext = CheckKey(K_RBRACKET) ? TRUE : FALSE;
        TerrainTextureEditorLastSelectAll = (CheckKey(K_A) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
        TerrainTextureEditorPalettePressEntry = TERRAIN_TEXTURE_PALETTE_NONE;
        return;
    }

    q = CheckKey(K_Q) ? TRUE : FALSE;
    if (!Editor_InspectorConsumesInput() AND q AND !TerrainTextureEditorLastQ) {
        S32 i;
        TerrainTextureEditorQuadMode = !TerrainTextureEditorQuadMode;
        if (TerrainTextureEditorQuadMode) {
            for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
                T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];
                if (sel->Used)
                    TerrainTextureEditorAddSelectionGroup(sel->CubeX, sel->CubeY, sel->Index);
            }
        }
        TerrainTextureEditorPalettePage = 0;
        Editor_SetStatus(TerrainTextureEditorQuadMode ? "Terrain Texture: Quad mode" : "Terrain Texture: Triangle mode");
    }
    TerrainTextureEditorLastQ = q;

    full_palette = (CheckKey(K_P) AND(CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT))) ? TRUE : FALSE;
    if (!Editor_InspectorConsumesInput() AND full_palette AND !TerrainTextureEditorLastFullPalette) {
        TerrainTextureEditorFullPalette = !TerrainTextureEditorFullPalette;
        TerrainTextureEditorPalettePage = 0;
        TerrainTextureEditorPalettePressEntry = TERRAIN_TEXTURE_PALETTE_NONE;
        Editor_SetStatus(TerrainTextureEditorFullPalette ? "Terrain Texture: Full Palette" : "Terrain Texture: Cube Palette");
    }
    TerrainTextureEditorLastFullPalette = full_palette;

    palette_prev = CheckKey(K_LBRACKET) ? TRUE : FALSE;
    palette_next = CheckKey(K_RBRACKET) ? TRUE : FALSE;
    if (!Editor_InspectorConsumesInput() AND palette_prev AND !TerrainTextureEditorLastPalettePrev) {
        TerrainTextureEditorPalettePage--;
        TerrainTextureEditorPaletteClampPage();
        TerrainTextureEditorPalettePressEntry = TERRAIN_TEXTURE_PALETTE_NONE;
    }
    if (!Editor_InspectorConsumesInput() AND palette_next AND !TerrainTextureEditorLastPaletteNext) {
        TerrainTextureEditorPalettePage++;
        TerrainTextureEditorPaletteClampPage();
        TerrainTextureEditorPalettePressEntry = TERRAIN_TEXTURE_PALETTE_NONE;
    }
    TerrainTextureEditorLastPalettePrev = palette_prev;
    TerrainTextureEditorLastPaletteNext = palette_next;

    click = Click;
    TerrainTextureEditorPickPending = FALSE;
    TerrainTextureEditorDragSelectPending = FALSE;

    if ((click & 1) AND !(TerrainTextureEditorLastClick & 1)) {
        if (TerrainTextureEditorPaletteContainsPoint((S32)MouseX, (S32)MouseY)) {
            TerrainTextureEditorPalettePressEntry = TerrainTextureEditorPaletteEntryAt((S32)MouseX, (S32)MouseY);
            if (TerrainTextureEditorPalettePressEntry == TERRAIN_TEXTURE_PALETTE_NONE)
                TerrainTextureEditorPalettePressEntry = TERRAIN_TEXTURE_PALETTE_DEAD;
        } else {
            TerrainTextureEditorPalettePressEntry = TERRAIN_TEXTURE_PALETTE_NONE;
        }
    }

    if (TerrainTextureEditorPalettePressEntry != TERRAIN_TEXTURE_PALETTE_NONE) {
        if (!(click & 1) AND(TerrainTextureEditorLastClick & 1)) {
            S32 entry;

            entry = TerrainTextureEditorPaletteEntryAt((S32)MouseX, (S32)MouseY);
            if (TerrainTextureEditorPalettePressEntry != TERRAIN_TEXTURE_PALETTE_DEAD AND entry == TerrainTextureEditorPalettePressEntry)
                TerrainTextureEditorApplyTextureSelected(entry);
            TerrainTextureEditorPalettePressEntry = TERRAIN_TEXTURE_PALETTE_NONE;
        }
        TerrainTextureEditorLastClick = click;
        TerrainTextureEditorLastUp = CheckKey(K_UP) ? TRUE : FALSE;
        TerrainTextureEditorLastDown = CheckKey(K_DOWN) ? TRUE : FALSE;
        TerrainTextureEditorLastLeft = CheckKey(K_LEFT) ? TRUE : FALSE;
        TerrainTextureEditorLastRight = CheckKey(K_RIGHT) ? TRUE : FALSE;
        TerrainTextureEditorLastT = CheckKey(K_T) ? TRUE : FALSE;
        TerrainTextureEditorLastQ = CheckKey(K_Q) ? TRUE : FALSE;
        TerrainTextureEditorLastL = CheckKey(K_L) ? TRUE : FALSE;
        TerrainTextureEditorLastFullPalette = (CheckKey(K_P) AND(CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT))) ? TRUE : FALSE;
        TerrainTextureEditorLastPalettePrev = CheckKey(K_LBRACKET) ? TRUE : FALSE;
        TerrainTextureEditorLastPaletteNext = CheckKey(K_RBRACKET) ? TRUE : FALSE;
        return;
    }

    if ((click & 1) AND !(TerrainTextureEditorLastClick & 1)) {
        TerrainTextureEditorDragActive = TRUE;
        TerrainTextureEditorDragShift = (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? TRUE : FALSE;
        TerrainTextureEditorDragX0 = (S32)MouseX;
        TerrainTextureEditorDragY0 = (S32)MouseY;
        TerrainTextureEditorDragX1 = TerrainTextureEditorDragX0;
        TerrainTextureEditorDragY1 = TerrainTextureEditorDragY0;
    }

    if (TerrainTextureEditorDragActive AND(click & 1)) {
        TerrainTextureEditorDragX1 = (S32)MouseX;
        TerrainTextureEditorDragY1 = (S32)MouseY;
    }

    if (TerrainTextureEditorDragActive AND !(click & 1) AND(TerrainTextureEditorLastClick & 1)) {
        S32 dx;
        S32 dy;

        TerrainTextureEditorDragX1 = (S32)MouseX;
        TerrainTextureEditorDragY1 = (S32)MouseY;
        dx = TerrainTextureEditorDragX1 - TerrainTextureEditorDragX0;
        dy = TerrainTextureEditorDragY1 - TerrainTextureEditorDragY0;
        if (dx < 0)
            dx = -dx;
        if (dy < 0)
            dy = -dy;

        if (dx > TERRAIN_TEXTURE_DRAG_MIN OR dy > TERRAIN_TEXTURE_DRAG_MIN) {
            TerrainTextureEditorDragSelectPending = TRUE;
            TerrainTextureEditorPickShift = TerrainTextureEditorDragShift;
            memset(TerrainTextureEditorDragVisited, 0, sizeof(TerrainTextureEditorDragVisited));
            if (!TerrainTextureEditorPickShift)
                TerrainTextureEditorClearSelection();
        } else {
            TerrainTextureEditorPickPending = TRUE;
            TerrainTextureEditorPickShift = TerrainTextureEditorDragShift;
            TerrainTextureEditorPickBestIndex = -1;
        }
        TerrainTextureEditorDragActive = FALSE;
    }

    if (TerrainTextureEditorPickPending) {
        TerrainTextureEditorPickBestIndex = -1;
    }
    TerrainTextureEditorLastClick = click;

    if (Editor_InspectorConsumesInput()) {
        TerrainTextureEditorLastUp = CheckKey(K_UP) ? TRUE : FALSE;
        TerrainTextureEditorLastDown = CheckKey(K_DOWN) ? TRUE : FALSE;
        TerrainTextureEditorLastLeft = CheckKey(K_LEFT) ? TRUE : FALSE;
        TerrainTextureEditorLastRight = CheckKey(K_RIGHT) ? TRUE : FALSE;
        TerrainTextureEditorLastT = CheckKey(K_T) ? TRUE : FALSE;
        TerrainTextureEditorLastL = CheckKey(K_L) ? TRUE : FALSE;
        TerrainTextureEditorLastPalettePrev = CheckKey(K_LBRACKET) ? TRUE : FALSE;
        TerrainTextureEditorLastPaletteNext = CheckKey(K_RBRACKET) ? TRUE : FALSE;
        TerrainTextureEditorLastSelectAll = (CheckKey(K_A) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;
        return;
    }

    up = CheckKey(K_UP) ? TRUE : FALSE;
    down = CheckKey(K_DOWN) ? TRUE : FALSE;
    left = CheckKey(K_LEFT) ? TRUE : FALSE;
    right = CheckKey(K_RIGHT) ? TRUE : FALSE;
    step = (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? 4 : 1;

    if (up AND !TerrainTextureEditorLastUp)
        TerrainTextureEditorAdjustSelectedIntensity(step);
    if (down AND !TerrainTextureEditorLastDown)
        TerrainTextureEditorAdjustSelectedIntensity(-step);

    if (left AND !TerrainTextureEditorLastLeft)
        TerrainTextureEditorCycleSelected(-1);
    if (right AND !TerrainTextureEditorLastRight)
        TerrainTextureEditorCycleSelected(1);

    TerrainTextureEditorLastUp = up;
    TerrainTextureEditorLastDown = down;
    TerrainTextureEditorLastLeft = left;
    TerrainTextureEditorLastRight = right;

    select_all = (CheckKey(K_A) AND(CheckKey(K_CTRL) OR CheckKey(K_CTRL_LEFT) OR CheckKey(K_CTRL_RIGHT))) ? TRUE : FALSE;

    if (select_all AND !TerrainTextureEditorLastSelectAll)
        TerrainTextureEditorSelectAll();

    TerrainTextureEditorLastSelectAll = select_all;

    t = CheckKey(K_T) ? TRUE : FALSE;

    if (t AND !TerrainTextureEditorLastT)
        TerrainTextureEditorCycleSelectedTexFlag();

    TerrainTextureEditorLastT = t;

    l = CheckKey(K_L) ? TRUE : FALSE;
    if (l AND !TerrainTextureEditorLastL)
        TerrainTextureEditorLogSelected();
    TerrainTextureEditorLastL = l;
}

static void TerrainTextureEditorSelectOnly(S32 cube_x, S32 cube_y, S32 index) {
    TerrainTextureEditorClearSelection();
    TerrainTextureEditorSelList[0].Used = TRUE;
    TerrainTextureEditorSelList[0].CubeX = cube_x;
    TerrainTextureEditorSelList[0].CubeY = cube_y;
    TerrainTextureEditorSelList[0].Index = TerrainTextureEditorQuadMode ? (index & ~1) : index;
    if (TerrainTextureEditorQuadMode) {
        TerrainTextureEditorSelList[1].Used = TRUE;
        TerrainTextureEditorSelList[1].CubeX = cube_x;
        TerrainTextureEditorSelList[1].CubeY = cube_y;
        TerrainTextureEditorSelList[1].Index = (index & ~1) + 1;
    }
}

static void TerrainTextureEditorAddSelected(S32 cube_x, S32 cube_y, S32 index) {
    S32 i;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        if (TerrainTextureEditorSameTri(&TerrainTextureEditorSelList[i], cube_x, cube_y, index))
            return;
    }

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];

        if (sel->Used)
            continue;
        sel->Used = TRUE;
        sel->CubeX = cube_x;
        sel->CubeY = cube_y;
        sel->Index = index;
        return;
    }
}

static void TerrainTextureEditorToggleSelected(S32 cube_x, S32 cube_y, S32 index) {
    S32 i;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];

        if (TerrainTextureEditorSameTri(sel, cube_x, cube_y, index)) {
            sel->Used = FALSE;
            return;
        }
    }

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];

        if (sel->Used)
            continue;
        sel->Used = TRUE;
        sel->CubeX = cube_x;
        sel->CubeY = cube_y;
        sel->Index = index;
        return;
    }
}

static void TerrainTextureEditorAddSelectionGroup(S32 cube_x, S32 cube_y, S32 index) {
    if (TerrainTextureEditorQuadMode) {
        TerrainTextureEditorAddSelected(cube_x, cube_y, index & ~1);
        TerrainTextureEditorAddSelected(cube_x, cube_y, (index & ~1) + 1);
    } else {
        TerrainTextureEditorAddSelected(cube_x, cube_y, index);
    }
}

static void TerrainTextureEditorToggleSelectionGroup(S32 cube_x, S32 cube_y, S32 index) {
    if (TerrainTextureEditorQuadMode) {
        S32 base = index & ~1;
        S32 selected = FALSE;
        S32 i;

        for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
            if (TerrainTextureEditorSameTri(&TerrainTextureEditorSelList[i], cube_x, cube_y, base)
                    OR TerrainTextureEditorSameTri(&TerrainTextureEditorSelList[i], cube_x, cube_y, base + 1)) {
                selected = TRUE;
                break;
            }
        }
        if (selected) {
            for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
                if (TerrainTextureEditorSameTri(&TerrainTextureEditorSelList[i], cube_x, cube_y, base)
                        OR TerrainTextureEditorSameTri(&TerrainTextureEditorSelList[i], cube_x, cube_y, base + 1))
                    TerrainTextureEditorSelList[i].Used = FALSE;
            }
        } else {
            TerrainTextureEditorAddSelectionGroup(cube_x, cube_y, base);
        }
    } else {
        TerrainTextureEditorToggleSelected(cube_x, cube_y, index);
    }
}

void TerrainTextureEditorEndFrame(void) {
    if (TerrainTextureEditorDragSelectPending) {
        TerrainTextureEditorDragSelectPending = FALSE;
        return;
    }

    if (!TerrainTextureEditorSelect OR !TerrainTextureEditorPickPending)
        return;

    if (TerrainTextureEditorPickBestIndex < 0) {
        if (!TerrainTextureEditorPickShift)
            TerrainTextureEditorClearSelection();
    } else if (TerrainTextureEditorPickShift) {
        TerrainTextureEditorToggleSelectionGroup(TerrainTextureEditorPickBestCubeX,
                                                 TerrainTextureEditorPickBestCubeY,
                                                 TerrainTextureEditorPickBestIndex);
    } else {
        TerrainTextureEditorSelectOnly(TerrainTextureEditorPickBestCubeX,
                                       TerrainTextureEditorPickBestCubeY,
                                       TerrainTextureEditorPickBestIndex);
    }

    TerrainTextureEditorPickPending = FALSE;
}

static S32 TerrainTextureEditorTriArea(S32 ax, S32 ay, S32 bx, S32 by, S32 cx, S32 cy) {
    return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

static S32 TerrainTextureEditorPointInTri(S32 px, S32 py, S32 ax, S32 ay, S32 bx, S32 by, S32 cx, S32 cy) {
    S32 a;
    S32 b;
    S32 c;

    a = TerrainTextureEditorTriArea(ax, ay, bx, by, px, py);
    b = TerrainTextureEditorTriArea(bx, by, cx, cy, px, py);
    c = TerrainTextureEditorTriArea(cx, cy, ax, ay, px, py);

    return ((a >= 0 AND b >= 0 AND c >= 0) OR(a <= 0 AND b <= 0 AND c <= 0));
}

static void TerrainTextureEditorConsiderTri(S32 cube_x, S32 cube_y, S32 index,
                                            S32 ax, S32 ay, S32 bx, S32 by, S32 cx, S32 cy) {
    S32 mx;
    S32 my;

    mx = (ax + bx + cx) / 3;
    my = (ay + by + cy) / 3;

    if (TerrainTextureEditorDragSelectPending) {
        S32 x0 = TerrainTextureEditorDragX0;
        S32 y0 = TerrainTextureEditorDragY0;
        S32 x1 = TerrainTextureEditorDragX1;
        S32 y1 = TerrainTextureEditorDragY1;

        if (x0 > x1) {
            S32 t = x0;
            x0 = x1;
            x1 = t;
        }
        if (y0 > y1) {
            S32 t = y0;
            y0 = y1;
            y1 = t;
        }

        if (mx >= x0 AND mx <= x1 AND my >= y0 AND my <= y1) {
            S32 cell = index / 2;
            if (!TerrainTextureEditorQuadMode OR !TerrainTextureEditorDragVisited[cell]) {
                if (TerrainTextureEditorPickShift)
                    TerrainTextureEditorToggleSelectionGroup(cube_x, cube_y, index);
                else
                    TerrainTextureEditorAddSelectionGroup(cube_x, cube_y, index);
                if (TerrainTextureEditorQuadMode)
                    TerrainTextureEditorDragVisited[cell] = TRUE;
            }
            if (MapPolyGround
                    AND index >= 0 AND index < NB_COTE * NB_COTE * 2)
                TerrainTextureEditorGetOrCreateEdit(cube_x, cube_y, index, &MapPolyGround[index]);
        }
    }

    if (!TerrainTextureEditorPickPending)
        return;

    if (TerrainTextureEditorPointInTri((S32)MouseX, (S32)MouseY, ax, ay, bx, by, cx, cy)) {
        TerrainTextureEditorPickBestCubeX = cube_x;
        TerrainTextureEditorPickBestCubeY = cube_y;
        TerrainTextureEditorPickBestIndex = TerrainTextureEditorQuadMode ? (index & ~1) : index;
        if (MapPolyGround
                AND index >= 0 AND index < NB_COTE * NB_COTE * 2)
            TerrainTextureEditorGetOrCreateEdit(cube_x, cube_y, index, &MapPolyGround[index]);
    }
}

static void TerrainTextureEditorDrawSelectedTri(S32 ax, S32 ay, S32 bx, S32 by, S32 cx, S32 cy) {
    Line(ax, ay, bx, by, TERRAIN_TEXTURE_EDITOR_MARK);
    Line(bx, by, cx, cy, TERRAIN_TEXTURE_EDITOR_MARK);
    Line(cx, cy, ax, ay, TERRAIN_TEXTURE_EDITOR_MARK);
}

static void TerrainTextureEditorAddBoundsPoint(S32 x, S32 y, S32 *x0, S32 *y0, S32 *x1, S32 *y1, S32 *has_bounds) {
    if (!*has_bounds) {
        *x0 = x;
        *y0 = y;
        *x1 = x;
        *y1 = y;
        *has_bounds = TRUE;
        return;
    }

    if (x < *x0)
        *x0 = x;
    if (y < *y0)
        *y0 = y;
    if (x > *x1)
        *x1 = x;
    if (y > *y1)
        *y1 = y;
}

static void TerrainTextureEditorDrawSelectedIndex(S32 index) {
    S32 cell;
    S32 tri;
    S32 xi;
    S32 zi;
    T_SOMMET_ROT *ptrs;
    U8 *ptra;
    S32 x0;
    S32 y0;
    S32 x1;
    S32 y1;
    S32 x2;
    S32 y2;
    S32 x3;
    S32 y3;
    S32 sens;

    if (!MapPolyGround)
        return;
    if (index < 0 OR index >= NB_COTE * NB_COTE * 2)
        return;

    cell = index / 2;
    tri = index & 1;
    zi = cell / NB_COTE;
    xi = cell - zi * NB_COTE;
    ptrs = &SommetRot[zi][xi];
    ptra = &SommetAffichable[zi][xi];
    x0 = ptrs[0].X2D;
    y0 = ptrs[0].Y2D;
    x1 = ptrs[65].X2D;
    y1 = ptrs[65].Y2D;
    x2 = ptrs[66].X2D;
    y2 = ptrs[66].Y2D;
    x3 = ptrs[1].X2D;
    y3 = ptrs[1].Y2D;
    sens = MapPolyGround[cell * 2].Sens;

    if (sens == 0) {
        if (tri == 0) {
            if ((ptra[0] & 16) AND(ptra[65] & 16) AND(ptra[66] & 16))
                TerrainTextureEditorDrawSelectedTri(x0, y0, x1, y1, x2, y2);
        } else {
            if ((ptra[66] & 16) AND(ptra[1] & 16) AND(ptra[0] & 16))
                TerrainTextureEditorDrawSelectedTri(x2, y2, x3, y3, x0, y0);
        }
    } else {
        if (tri == 0) {
            if ((ptra[1] & 16) AND(ptra[0] & 16) AND(ptra[65] & 16))
                TerrainTextureEditorDrawSelectedTri(x3, y3, x0, y0, x1, y1);
        } else {
            if ((ptra[65] & 16) AND(ptra[66] & 16) AND(ptra[1] & 16))
                TerrainTextureEditorDrawSelectedTri(x1, y1, x2, y2, x3, y3);
        }
    }
}

static S32 TerrainTextureEditorAddSelectedIndexBounds(S32 index, S32 *bounds_x0, S32 *bounds_y0, S32 *bounds_x1, S32 *bounds_y1) {
    S32 cell;
    S32 tri;
    S32 xi;
    S32 zi;
    T_SOMMET_ROT *ptrs;
    U8 *ptra;
    S32 has_bounds = FALSE;
    S32 sens;

    if (!MapPolyGround)
        return FALSE;
    if (index < 0 OR index >= NB_COTE * NB_COTE * 2)
        return FALSE;

    cell = index / 2;
    tri = index & 1;
    zi = cell / NB_COTE;
    xi = cell - zi * NB_COTE;
    ptrs = &SommetRot[zi][xi];
    ptra = &SommetAffichable[zi][xi];
    sens = MapPolyGround[cell * 2].Sens;

    if (sens == 0) {
        if (tri == 0) {
            if ((ptra[0] & 16) AND(ptra[65] & 16) AND(ptra[66] & 16)) {
                TerrainTextureEditorAddBoundsPoint(ptrs[0].X2D, ptrs[0].Y2D, bounds_x0, bounds_y0, bounds_x1, bounds_y1, &has_bounds);
                TerrainTextureEditorAddBoundsPoint(ptrs[65].X2D, ptrs[65].Y2D, bounds_x0, bounds_y0, bounds_x1, bounds_y1, &has_bounds);
                TerrainTextureEditorAddBoundsPoint(ptrs[66].X2D, ptrs[66].Y2D, bounds_x0, bounds_y0, bounds_x1, bounds_y1, &has_bounds);
            }
        } else {
            if ((ptra[66] & 16) AND(ptra[1] & 16) AND(ptra[0] & 16)) {
                TerrainTextureEditorAddBoundsPoint(ptrs[66].X2D, ptrs[66].Y2D, bounds_x0, bounds_y0, bounds_x1, bounds_y1, &has_bounds);
                TerrainTextureEditorAddBoundsPoint(ptrs[1].X2D, ptrs[1].Y2D, bounds_x0, bounds_y0, bounds_x1, bounds_y1, &has_bounds);
                TerrainTextureEditorAddBoundsPoint(ptrs[0].X2D, ptrs[0].Y2D, bounds_x0, bounds_y0, bounds_x1, bounds_y1, &has_bounds);
            }
        }
    } else {
        if (tri == 0) {
            if ((ptra[1] & 16) AND(ptra[0] & 16) AND(ptra[65] & 16)) {
                TerrainTextureEditorAddBoundsPoint(ptrs[1].X2D, ptrs[1].Y2D, bounds_x0, bounds_y0, bounds_x1, bounds_y1, &has_bounds);
                TerrainTextureEditorAddBoundsPoint(ptrs[0].X2D, ptrs[0].Y2D, bounds_x0, bounds_y0, bounds_x1, bounds_y1, &has_bounds);
                TerrainTextureEditorAddBoundsPoint(ptrs[65].X2D, ptrs[65].Y2D, bounds_x0, bounds_y0, bounds_x1, bounds_y1, &has_bounds);
            }
        } else {
            if ((ptra[65] & 16) AND(ptra[66] & 16) AND(ptra[1] & 16)) {
                TerrainTextureEditorAddBoundsPoint(ptrs[65].X2D, ptrs[65].Y2D, bounds_x0, bounds_y0, bounds_x1, bounds_y1, &has_bounds);
                TerrainTextureEditorAddBoundsPoint(ptrs[66].X2D, ptrs[66].Y2D, bounds_x0, bounds_y0, bounds_x1, bounds_y1, &has_bounds);
                TerrainTextureEditorAddBoundsPoint(ptrs[1].X2D, ptrs[1].Y2D, bounds_x0, bounds_y0, bounds_x1, bounds_y1, &has_bounds);
            }
        }
    }

    return has_bounds;
}

static void TerrainTextureEditorDrawSelectedList(void) {
    S32 i;
    S32 selected_count = 0;
    S32 bounds_x0 = 0;
    S32 bounds_y0 = 0;
    S32 bounds_x1 = 0;
    S32 bounds_y1 = 0;
    S32 has_bounds = FALSE;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];

        if (!sel->Used)
            continue;
        if (sel->CubeX != CurrentCubeX OR sel->CubeY != CurrentCubeY)
            continue;

        selected_count++;
    }

    if (selected_count > TERRAIN_TEXTURE_EDITOR_BOUNDS_LIMIT) {
        for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
            T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];

            if (!sel->Used)
                continue;
            if (sel->CubeX != CurrentCubeX OR sel->CubeY != CurrentCubeY)
                continue;
            if (TerrainTextureEditorAddSelectedIndexBounds(sel->Index, &bounds_x0, &bounds_y0, &bounds_x1, &bounds_y1))
                has_bounds = TRUE;
        }

        if (has_bounds)
            Rect(bounds_x0, bounds_y0, bounds_x1, bounds_y1, TERRAIN_TEXTURE_EDITOR_MARK);
        return;
    }

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];

        if (!sel->Used)
            continue;
        if (sel->CubeX != CurrentCubeX OR sel->CubeY != CurrentCubeY)
            continue;
        TerrainTextureEditorDrawSelectedIndex(sel->Index);
    }
}

static void TerrainTextureEditorDrawDragBox(void) {
    S32 x0;
    S32 y0;
    S32 x1;
    S32 y1;

    if (!TerrainTextureEditorDragActive)
        return;

    x0 = TerrainTextureEditorDragX0;
    y0 = TerrainTextureEditorDragY0;
    x1 = TerrainTextureEditorDragX1;
    y1 = TerrainTextureEditorDragY1;

    if (x0 > x1) {
        S32 t = x0;
        x0 = x1;
        x1 = t;
    }
    if (y0 > y1) {
        S32 t = y0;
        y0 = y1;
        y1 = t;
    }

    Rect(x0, y0, x1, y1, TERRAIN_TEXTURE_DRAG_MARK);
}

static void TerrainTextureEditorFillRect(S32 x0, S32 y0, S32 x1, S32 y1, S32 color) {
    S32 x;
    S32 y;

    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            Plot(x, y, color);
        }
    }
}

static void TerrainTextureEditorDrawNoTextureSwatch(S32 x0, S32 y0) {
    S32 x;
    S32 y;
    S32 x1;
    S32 y1;

    x1 = x0 + TERRAIN_TEXTURE_PALETTE_SWATCH - 1;
    y1 = y0 + TERRAIN_TEXTURE_PALETTE_SWATCH - 1;

    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            Plot(x, y, ((x + y) & 4) ? 8 : 0);
        }
    }
    Line(x0 + 3, y0 + 3, x1 - 3, y1 - 3, 15);
    Line(x0 + 3, y1 - 3, x1 - 3, y0 + 3, 15);
}

static void TerrainTextureEditorDrawTextureSwatch(S32 x0, S32 y0, S32 texture_index) {
    T_HALF_TEX *tex;
    S32 min_u;
    S32 min_v;
    S32 max_u;
    S32 max_v;
    S32 x;
    S32 y;
    S32 u;
    S32 v;

    if (!GroundTexture OR !ListTexDef OR texture_index < 0 OR texture_index >= TerrainTextureEditorTexDefCount) {
        TerrainTextureEditorFillRect(x0,
                                     y0,
                                     x0 + TERRAIN_TEXTURE_PALETTE_SWATCH - 1,
                                     y0 + TERRAIN_TEXTURE_PALETTE_SWATCH - 1,
                                     0);
        return;
    }

    tex = &ListTexDef[texture_index];
    min_u = (S32)tex->Tx0;
    max_u = min_u;
    min_v = (S32)tex->Ty0;
    max_v = min_v;

    if ((S32)tex->Tx1 < min_u)
        min_u = (S32)tex->Tx1;
    if ((S32)tex->Tx2 < min_u)
        min_u = (S32)tex->Tx2;
    if ((S32)tex->Tx1 > max_u)
        max_u = (S32)tex->Tx1;
    if ((S32)tex->Tx2 > max_u)
        max_u = (S32)tex->Tx2;

    if ((S32)tex->Ty1 < min_v)
        min_v = (S32)tex->Ty1;
    if ((S32)tex->Ty2 < min_v)
        min_v = (S32)tex->Ty2;
    if ((S32)tex->Ty1 > max_v)
        max_v = (S32)tex->Ty1;
    if ((S32)tex->Ty2 > max_v)
        max_v = (S32)tex->Ty2;

    if (max_u <= min_u)
        max_u = min_u + 256;
    if (max_v <= min_v)
        max_v = min_v + 256;

    for (y = 0; y < TERRAIN_TEXTURE_PALETTE_SWATCH; y++) {
        for (x = 0; x < TERRAIN_TEXTURE_PALETTE_SWATCH; x++) {
            u = (min_u + ((max_u - min_u) * x) / (TERRAIN_TEXTURE_PALETTE_SWATCH - 1)) >> 8;
            v = (min_v + ((max_v - min_v) * y) / (TERRAIN_TEXTURE_PALETTE_SWATCH - 1)) >> 8;
            Plot(x0 + x, y0 + y, GroundTexture[((v & 255) << 8) + (u & 255)]);
        }
    }
}

static void TerrainTextureEditorDrawCatalogSwatch(S32 x0, S32 y0, S32 catalog_index) {
    S32 local_index = TerrainTextureEditorResolveCatalogTexture(catalog_index);
    TerrainTextureEditorDrawTextureSwatch(x0, y0, local_index);
}

static void TerrainTextureEditorDrawPalette(void) {
    char page_text[32];
    S32 panel_x0;
    S32 panel_y0;
    S32 panel_x1;
    S32 panel_y1;
    S32 visible_count;
    S32 selected_entry;
    S32 entry;
    S32 cols;
    S32 page_count;
    S32 page_offset;

    TerrainTextureEditorPaletteClampPage();
    TerrainTextureEditorPaletteRect(&panel_x0, &panel_y0, &panel_x1, &panel_y1);
    TerrainTextureEditorFillRect(panel_x0, panel_y0, panel_x1, panel_y1, TERRAIN_TEXTURE_PALETTE_BACK);
    Rect(panel_x0, panel_y0, panel_x1, panel_y1, TERRAIN_TEXTURE_PALETTE_BORDER);

    visible_count = TerrainTextureEditorPaletteVisibleCount();
    selected_entry = TerrainTextureEditorSelectedTextureEntry();
    cols = TerrainTextureEditorPaletteColumns();
    page_count = TerrainTextureEditorPalettePageCount();
    page_offset = TerrainTextureEditorPalettePage * TerrainTextureEditorPalettePageSize();

    for (entry = 0; entry < visible_count; entry++) {
        S32 col;
        S32 row;
        S32 sx;
        S32 sy;
        S32 texture_entry;
        S32 border;

        col = entry % cols;
        row = entry / cols;
        sx = panel_x0 + TERRAIN_TEXTURE_PALETTE_PAD + col * (TERRAIN_TEXTURE_PALETTE_SWATCH + TERRAIN_TEXTURE_PALETTE_GAP);
        sy = panel_y0 + TERRAIN_TEXTURE_PALETTE_PAD + row * (TERRAIN_TEXTURE_PALETTE_SWATCH + TERRAIN_TEXTURE_PALETTE_GAP);
        texture_entry = page_offset + entry - 1;

        if (texture_entry < 0)
            TerrainTextureEditorDrawNoTextureSwatch(sx, sy);
        else if (TerrainTextureEditorFullPalette)
            TerrainTextureEditorDrawCatalogSwatch(sx, sy, TerrainTextureEditorQuadMode ? texture_entry * 2 : texture_entry);
        else
            TerrainTextureEditorDrawTextureSwatch(sx, sy, TerrainTextureEditorQuadMode ? texture_entry * 2 : texture_entry);

        border = TERRAIN_TEXTURE_PALETTE_BORDER;
        if (selected_entry == texture_entry OR TerrainTextureEditorPalettePressEntry == texture_entry)
            border = TERRAIN_TEXTURE_PALETTE_SELECT;
        Rect(sx - 1,
             sy - 1,
             sx + TERRAIN_TEXTURE_PALETTE_SWATCH,
             sy + TERRAIN_TEXTURE_PALETTE_SWATCH,
             border);
    }

    if (page_count > 1) {
        snprintf(page_text, sizeof(page_text), "%d/%d", (int)TerrainTextureEditorPalettePage + 1, (int)page_count);
        CoulText(15, -1);
        GraphPrintf(FALSE,
                    panel_x0,
                    (S32)ModeDesiredY - 12,
                    "%s",
                    page_text);
    }
}

static void TerrainRegionEditorPaletteRect(S32 *x0, S32 *y0, S32 *x1, S32 *y1) {
    *x1 = ModeDesiredX - 8;
    *x0 = *x1 - TERRAIN_REGION_PALETTE_SWATCH * 2 - TERRAIN_REGION_PALETTE_GAP - 8;
    *y0 = 8;
    *y1 = *y0 + TERRAIN_REGION_PALETTE_SWATCH * 6 + TERRAIN_REGION_PALETTE_GAP * 5 + 8;
}

static S32 TerrainRegionEditorPaletteTypeAt(S32 x, S32 y) {
    S32 x0;
    S32 y0;
    S32 x1;
    S32 y1;
    S32 col;

    TerrainRegionEditorPaletteRect(&x0, &y0, &x1, &y1);
    col = x >= x0 + 4 + TERRAIN_REGION_PALETTE_SWATCH + TERRAIN_REGION_PALETTE_GAP AND x < x0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 2 + TERRAIN_REGION_PALETTE_GAP;
    if (!col AND(x < x0 + 4 OR x >= x0 + 4 + TERRAIN_REGION_PALETTE_SWATCH))
        return -1;
    if (col) {
        if (y >= y0 + 4 AND y < y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH)
            return TERRAIN_REGION_TYPE_PAVEMENT_ALT;
        else if (y >= y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH + TERRAIN_REGION_PALETTE_GAP AND y < y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 2 + TERRAIN_REGION_PALETTE_GAP)
            return TERRAIN_REGION_TYPE_ROCKS;
        else if (y >= y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 2 + TERRAIN_REGION_PALETTE_GAP * 2 AND y < y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 3 + TERRAIN_REGION_PALETTE_GAP * 2)
            return TERRAIN_REGION_TYPE_CLIFFSIDE;
        else if (y >= y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 3 + TERRAIN_REGION_PALETTE_GAP * 3 AND y < y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 4 + TERRAIN_REGION_PALETTE_GAP * 3)
            return TERRAIN_REGION_TYPE_DIRT;
        else
            return -1;
    }
    if (y >= y0 + 4 AND y < y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH)
        return TERRAIN_REGION_TYPE_GRASS;
    if (y >= y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH + TERRAIN_REGION_PALETTE_GAP AND y < y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 2 + TERRAIN_REGION_PALETTE_GAP)
        return TERRAIN_REGION_TYPE_PAVEMENT;
    if (y >= y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 2 + TERRAIN_REGION_PALETTE_GAP * 2 AND y < y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 3 + TERRAIN_REGION_PALETTE_GAP * 2)
        return TERRAIN_REGION_TYPE_GRASS_INSET;
    if (y >= y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 3 + TERRAIN_REGION_PALETTE_GAP * 3 AND y < y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 4 + TERRAIN_REGION_PALETTE_GAP * 3)
        return TERRAIN_REGION_TYPE_ASPHALT;
    if (y >= y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 4 + TERRAIN_REGION_PALETTE_GAP * 4 AND y < y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 5 + TERRAIN_REGION_PALETTE_GAP * 4)
        return TERRAIN_REGION_TYPE_GRASS_PAVEMENT;
    if (y >= y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 5 + TERRAIN_REGION_PALETTE_GAP * 5 AND y < y0 + 4 + TERRAIN_REGION_PALETTE_SWATCH * 6 + TERRAIN_REGION_PALETTE_GAP * 5)
        return TERRAIN_REGION_TYPE_RIPRAP;
    return -1;
}

static S64 TerrainRegionEditorPreviewEdge(S32 ax, S32 ay, S32 bx, S32 by, S32 px, S32 py) {
    return (S64)(px - ax) * (by - ay) - (S64)(py - ay) * (bx - ax);
}

static void TerrainRegionEditorDrawPreviewTriangle(S32 x0, S32 y0,
                                                   S32 ax, S32 ay, S32 bx, S32 by, S32 cx, S32 cy,
                                                   S32 texture) {
    S32 local_texture = TerrainTextureEditorResolveCatalogTexture(texture);
    T_HALF_TEX *tex;
    S64 area;
    S32 x;
    S32 y;

    if (!GroundTexture OR !ListTexDef OR local_texture < 0 OR local_texture >= TerrainTextureEditorTexDefCount)
        return;
    tex = &ListTexDef[local_texture];
    area = TerrainRegionEditorPreviewEdge(ax, ay, bx, by, cx, cy);
    if (!area)
        return;

    for (y = 0; y < TERRAIN_REGION_PREVIEW_TILE; y++) {
        for (x = 0; x < TERRAIN_REGION_PREVIEW_TILE; x++) {
            S64 w0 = TerrainRegionEditorPreviewEdge(bx, by, cx, cy, x, y);
            S64 w1 = TerrainRegionEditorPreviewEdge(cx, cy, ax, ay, x, y);
            S64 w2 = TerrainRegionEditorPreviewEdge(ax, ay, bx, by, x, y);
            S32 u;
            S32 v;

            if ((area > 0 AND(w0 < 0 OR w1 < 0 OR w2 < 0))
                    OR(area < 0 AND(w0 > 0 OR w1 > 0 OR w2 > 0)))
                continue;
            u = (S32)((w0 * tex->Tx0 + w1 * tex->Tx1 + w2 * tex->Tx2) / area) >> 8;
            v = (S32)((w0 * tex->Ty0 + w1 * tex->Ty1 + w2 * tex->Ty2) / area) >> 8;
            Plot(x0 + x, y0 + y, GroundTexture[((v & 255) << 8) + (u & 255)]);
        }
    }
}

static void TerrainRegionEditorDrawPreviewQuad(S32 x0, S32 y0, S32 texture0, S32 texture1, S32 sens) {
    S32 edge = TERRAIN_REGION_PREVIEW_TILE - 1;

    TerrainTextureEditorFillRect(x0, y0, x0 + edge, y0 + edge, 0);
    if (sens == 0) {
        TerrainRegionEditorDrawPreviewTriangle(x0, y0, 0, 0, 0, edge, edge, edge, texture0);
        TerrainRegionEditorDrawPreviewTriangle(x0, y0, edge, edge, edge, 0, 0, 0, texture1);
    } else {
        TerrainRegionEditorDrawPreviewTriangle(x0, y0, edge, 0, 0, 0, 0, edge, texture0);
        TerrainRegionEditorDrawPreviewTriangle(x0, y0, 0, edge, edge, edge, edge, 0, texture1);
    }
}

static void TerrainRegionEditorDrawPreview(S32 x0, S32 y0, S32 type) {
    static const S32 grass[4][4] = {
        {16, 22, 11, 9},
        {18, 24, 7, 5},
        {20, 26, 28, 30},
        {14, 32, 34, 36}};
    static const S32 pavement[4][4] = {
        {398, 366, 368, 450},
        {400, 358, 360, 452},
        {398, 366, 368, 450},
        {400, 358, 360, 452}};
    static const S32 grass_inset[4][4] = {
        {589, 590, 592, 594},
        {596, 20, 26, 580},
        {598, 14, 32, 582},
        {595, 584, 586, 588}};
    static const S32 asphalt[4][4] = {
        {634, 636, 634, 636},
        {638, 640, 638, 640},
        {634, 636, 634, 636},
        {638, 640, 638, 640}};
    static const S32 grass_pavement0[4][4] = {
        {462, 428, 430, 442},
        {434, 436, 438, 440},
        {434, 436, 438, 440},
        {464, 439, 437, 444}};
    static const S32 grass_pavement1[4][4] = {
        {463, 429, 431, 443},
        {435, 437, 439, 441},
        {435, 437, 439, 441},
        {465, 438, 436, 445}};
    static const S32 riprap[4][4] = {
        {630, 560, 560, 632},
        {566, 0, 0, 562},
        {566, 0, 0, 562},
        {632, 564, 564, 630}};
    static const S32 pavement_alt[4][4] = {
        {614, 616, 622, 624},
        {618, 620, 626, 628},
        {614, 616, 622, 624},
        {618, 620, 626, 628}};
    static const S32 rocks[4][4] = {
        {58, 45, 38, 56},
        {53, 49, 40, 55},
        {58, 45, 38, 56},
        {53, 49, 40, 55}};
    static const S32 cliffside0[4][4] = {
        {64, 67, 64, 67},
        {60, 62, 60, 62},
        {64, 67, 64, 67},
        {60, 62, 60, 62}};
    static const S32 cliffside1[4][4] = {
        {66, 65, 66, 65},
        {61, 63, 61, 63},
        {66, 65, 66, 65},
        {61, 63, 61, 63}};
    S32 x;
    S32 y;

    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            S32 texture0;
            S32 texture1;
            if (type == TERRAIN_REGION_TYPE_GRASS)
                texture0 = grass[y][x];
            else if (type == TERRAIN_REGION_TYPE_DIRT) {
                TerrainTextureEditorFillRect(x0 + x * TERRAIN_REGION_PREVIEW_TILE,
                                             y0 + y * TERRAIN_REGION_PREVIEW_TILE,
                                             x0 + (x + 1) * TERRAIN_REGION_PREVIEW_TILE - 1,
                                             y0 + (y + 1) * TERRAIN_REGION_PREVIEW_TILE - 1,
                                             (1 << 4) + 11);
                continue;
            } else if (type == TERRAIN_REGION_TYPE_GRASS_INSET)
                texture0 = grass_inset[y][x];
            else if (type == TERRAIN_REGION_TYPE_ASPHALT)
                texture0 = asphalt[y][x];
            else if (type == TERRAIN_REGION_TYPE_GRASS_PAVEMENT)
                texture0 = grass_pavement0[y][x];
            else if (type == TERRAIN_REGION_TYPE_RIPRAP)
                texture0 = riprap[y][x];
            else if (type == TERRAIN_REGION_TYPE_PAVEMENT_ALT)
                texture0 = pavement_alt[y][x];
            else if (type == TERRAIN_REGION_TYPE_ROCKS)
                texture0 = rocks[y][x];
            else if (type == TERRAIN_REGION_TYPE_CLIFFSIDE)
                texture0 = cliffside0[y][x];
            else
                texture0 = pavement[y][x];

            if (type == TERRAIN_REGION_TYPE_GRASS_PAVEMENT)
                texture1 = grass_pavement1[y][x];
            else if (type == TERRAIN_REGION_TYPE_RIPRAP AND texture0 == 0)
                texture1 = 0;
            else if (type == TERRAIN_REGION_TYPE_ROCKS AND texture0 == 55)
                texture1 = 52;
            else if (type == TERRAIN_REGION_TYPE_CLIFFSIDE)
                texture1 = cliffside1[y][x];
            else if ((type == TERRAIN_REGION_TYPE_GRASS AND(texture0 == 11 OR texture0 == 9 OR texture0 == 7 OR texture0 == 5))
                    OR(type == TERRAIN_REGION_TYPE_GRASS_INSET
                        AND(texture0 == 589 OR texture0 == 595 OR texture0 == 11 OR texture0 == 7)))
                texture1 = texture0 - 1;
            else
                texture1 = texture0 + 1;
            if (type == TERRAIN_REGION_TYPE_RIPRAP AND texture0 == 0) {
                TerrainTextureEditorFillRect(x0 + x * TERRAIN_REGION_PREVIEW_TILE,
                                             y0 + y * TERRAIN_REGION_PREVIEW_TILE,
                                             x0 + (x + 1) * TERRAIN_REGION_PREVIEW_TILE - 1,
                                             y0 + (y + 1) * TERRAIN_REGION_PREVIEW_TILE - 1,
                                             0);
                continue;
            }
            TerrainRegionEditorDrawPreviewQuad(x0 + x * TERRAIN_REGION_PREVIEW_TILE,
                                               y0 + y * TERRAIN_REGION_PREVIEW_TILE,
                                               texture0, texture1, 0);
        }
    }
}

static void TerrainRegionEditorDrawPalette(void) {
    S32 x0;
    S32 y0;
    S32 x1;
    S32 y1;
    S32 grass_y;
    S32 pavement_y;
    S32 grass_inset_y;
    S32 asphalt_y;
    S32 grass_pavement_y;
    S32 riprap_y;
    S32 alt_x;
    S32 rocks_y;
    S32 cliffside_y;
    S32 dirt_y;

    TerrainRegionEditorPaletteRect(&x0, &y0, &x1, &y1);
    TerrainTextureEditorFillRect(x0, y0, x1, y1, TERRAIN_TEXTURE_PALETTE_BACK);
    Rect(x0, y0, x1, y1, TERRAIN_TEXTURE_PALETTE_BORDER);
    grass_y = y0 + 4;
    pavement_y = grass_y + TERRAIN_REGION_PALETTE_SWATCH + TERRAIN_REGION_PALETTE_GAP;
    grass_inset_y = pavement_y + TERRAIN_REGION_PALETTE_SWATCH + TERRAIN_REGION_PALETTE_GAP;
    asphalt_y = grass_inset_y + TERRAIN_REGION_PALETTE_SWATCH + TERRAIN_REGION_PALETTE_GAP;
    grass_pavement_y = asphalt_y + TERRAIN_REGION_PALETTE_SWATCH + TERRAIN_REGION_PALETTE_GAP;
    riprap_y = grass_pavement_y + TERRAIN_REGION_PALETTE_SWATCH + TERRAIN_REGION_PALETTE_GAP;
    alt_x = x0 + 4 + TERRAIN_REGION_PALETTE_SWATCH + TERRAIN_REGION_PALETTE_GAP;
    rocks_y = pavement_y;
    cliffside_y = grass_inset_y;
    dirt_y = asphalt_y;
    TerrainRegionEditorDrawPreview(x0 + 4, grass_y, TERRAIN_REGION_TYPE_GRASS);
    TerrainRegionEditorDrawPreview(x0 + 4, pavement_y, TERRAIN_REGION_TYPE_PAVEMENT);
    TerrainRegionEditorDrawPreview(x0 + 4, grass_inset_y, TERRAIN_REGION_TYPE_GRASS_INSET);
    TerrainRegionEditorDrawPreview(x0 + 4, asphalt_y, TERRAIN_REGION_TYPE_ASPHALT);
    TerrainRegionEditorDrawPreview(x0 + 4, grass_pavement_y, TERRAIN_REGION_TYPE_GRASS_PAVEMENT);
    TerrainRegionEditorDrawPreview(x0 + 4, riprap_y, TERRAIN_REGION_TYPE_RIPRAP);
    TerrainRegionEditorDrawPreview(alt_x, grass_y, TERRAIN_REGION_TYPE_PAVEMENT_ALT);
    TerrainRegionEditorDrawPreview(alt_x, rocks_y, TERRAIN_REGION_TYPE_ROCKS);
    TerrainRegionEditorDrawPreview(alt_x, cliffside_y, TERRAIN_REGION_TYPE_CLIFFSIDE);
    TerrainRegionEditorDrawPreview(alt_x, dirt_y, TERRAIN_REGION_TYPE_DIRT);
    Rect(x0 + 3, grass_y - 1,
         x0 + 4 + TERRAIN_REGION_PALETTE_SWATCH,
         grass_y + TERRAIN_REGION_PALETTE_SWATCH,
         TerrainRegionEditorPaletteType == TERRAIN_REGION_TYPE_GRASS ? TERRAIN_TEXTURE_PALETTE_SELECT : TERRAIN_TEXTURE_PALETTE_BORDER);
    Rect(x0 + 3, pavement_y - 1,
         x0 + 4 + TERRAIN_REGION_PALETTE_SWATCH,
         pavement_y + TERRAIN_REGION_PALETTE_SWATCH,
         TerrainRegionEditorPaletteType == TERRAIN_REGION_TYPE_PAVEMENT ? TERRAIN_TEXTURE_PALETTE_SELECT : TERRAIN_TEXTURE_PALETTE_BORDER);
    Rect(x0 + 3, grass_inset_y - 1,
         x0 + 4 + TERRAIN_REGION_PALETTE_SWATCH,
         grass_inset_y + TERRAIN_REGION_PALETTE_SWATCH,
         TerrainRegionEditorPaletteType == TERRAIN_REGION_TYPE_GRASS_INSET ? TERRAIN_TEXTURE_PALETTE_SELECT : TERRAIN_TEXTURE_PALETTE_BORDER);
    Rect(x0 + 3, asphalt_y - 1,
         x0 + 4 + TERRAIN_REGION_PALETTE_SWATCH,
         asphalt_y + TERRAIN_REGION_PALETTE_SWATCH,
         TerrainRegionEditorPaletteType == TERRAIN_REGION_TYPE_ASPHALT ? TERRAIN_TEXTURE_PALETTE_SELECT : TERRAIN_TEXTURE_PALETTE_BORDER);
    Rect(x0 + 3, grass_pavement_y - 1,
         x0 + 4 + TERRAIN_REGION_PALETTE_SWATCH,
         grass_pavement_y + TERRAIN_REGION_PALETTE_SWATCH,
         TerrainRegionEditorPaletteType == TERRAIN_REGION_TYPE_GRASS_PAVEMENT ? TERRAIN_TEXTURE_PALETTE_SELECT : TERRAIN_TEXTURE_PALETTE_BORDER);
    Rect(x0 + 3, riprap_y - 1,
         x0 + 4 + TERRAIN_REGION_PALETTE_SWATCH,
         riprap_y + TERRAIN_REGION_PALETTE_SWATCH,
         TerrainRegionEditorPaletteType == TERRAIN_REGION_TYPE_RIPRAP ? TERRAIN_TEXTURE_PALETTE_SELECT : TERRAIN_TEXTURE_PALETTE_BORDER);
    Rect(alt_x - 1, grass_y - 1,
         alt_x + TERRAIN_REGION_PALETTE_SWATCH,
         grass_y + TERRAIN_REGION_PALETTE_SWATCH,
         TerrainRegionEditorPaletteType == TERRAIN_REGION_TYPE_PAVEMENT_ALT ? TERRAIN_TEXTURE_PALETTE_SELECT : TERRAIN_TEXTURE_PALETTE_BORDER);
    Rect(alt_x - 1, rocks_y - 1,
         alt_x + TERRAIN_REGION_PALETTE_SWATCH,
         rocks_y + TERRAIN_REGION_PALETTE_SWATCH,
         TerrainRegionEditorPaletteType == TERRAIN_REGION_TYPE_ROCKS ? TERRAIN_TEXTURE_PALETTE_SELECT : TERRAIN_TEXTURE_PALETTE_BORDER);
    Rect(alt_x - 1, cliffside_y - 1,
         alt_x + TERRAIN_REGION_PALETTE_SWATCH,
         cliffside_y + TERRAIN_REGION_PALETTE_SWATCH,
         TerrainRegionEditorPaletteType == TERRAIN_REGION_TYPE_CLIFFSIDE ? TERRAIN_TEXTURE_PALETTE_SELECT : TERRAIN_TEXTURE_PALETTE_BORDER);
    Rect(alt_x - 1, dirt_y - 1,
         alt_x + TERRAIN_REGION_PALETTE_SWATCH,
         dirt_y + TERRAIN_REGION_PALETTE_SWATCH,
         TerrainRegionEditorPaletteType == TERRAIN_REGION_TYPE_DIRT ? TERRAIN_TEXTURE_PALETTE_SELECT : TERRAIN_TEXTURE_PALETTE_BORDER);
}

static S32 TerrainRegionEditorPointInCell(S32 xi, S32 zi, S32 x, S32 y) {
    T_SOMMET_ROT *ptrs = &SommetRot[zi][xi];
    U8 *ptra = &SommetAffichable[zi][xi];

    if (!(ptra[0] & 16) OR !(ptra[65] & 16) OR !(ptra[66] & 16) OR !(ptra[1] & 16))
        return FALSE;
    return TerrainTextureEditorPointInTri(x, y,
                                          ptrs[0].X2D, ptrs[0].Y2D,
                                          ptrs[65].X2D, ptrs[65].Y2D,
                                          ptrs[66].X2D, ptrs[66].Y2D)
           OR TerrainTextureEditorPointInTri(x, y,
                                             ptrs[66].X2D, ptrs[66].Y2D,
                                             ptrs[1].X2D, ptrs[1].Y2D,
                                             ptrs[0].X2D, ptrs[0].Y2D);
}

static S32 TerrainRegionEditorPickCell(S32 *picked_x, S32 *picked_z, S32 *picked_half) {
    S32 xi;
    S32 zi;

    for (zi = 0; zi < NB_COTE; zi++) {
        for (xi = 0; xi < NB_COTE; xi++) {
            T_SOMMET_ROT *ptrs = &SommetRot[zi][xi];
            U8 *ptra = &SommetAffichable[zi][xi];
            S32 sens;

            if (!(ptra[0] & 16) OR !(ptra[65] & 16) OR !(ptra[66] & 16) OR !(ptra[1] & 16))
                continue;
            if (!TerrainRegionEditorPointInCell(xi, zi, (S32)MouseX, (S32)MouseY))
                continue;
            *picked_x = xi;
            *picked_z = zi;
            *picked_half = -1;
            if (!TerrainRegionEditorQuadMode) {
                sens = MapPolyGround[(zi * NB_COTE + xi) * 2].Sens;
                if (sens == 0)
                    *picked_half = TerrainTextureEditorPointInTri((S32)MouseX, (S32)MouseY,
                                                                 ptrs[0].X2D, ptrs[0].Y2D,
                                                                 ptrs[65].X2D, ptrs[65].Y2D,
                                                                 ptrs[66].X2D, ptrs[66].Y2D)
                                       ? 0
                                       : 1;
                else
                    *picked_half = TerrainTextureEditorPointInTri((S32)MouseX, (S32)MouseY,
                                                                 ptrs[1].X2D, ptrs[1].Y2D,
                                                                 ptrs[0].X2D, ptrs[0].Y2D,
                                                                 ptrs[65].X2D, ptrs[65].Y2D)
                                       ? 0
                                       : 1;
            }
            return TRUE;
        }
    }
    return FALSE;
}

static S32 TerrainRegionEditorProjectPoint(S32 x, S32 y, S32 z, S32 *screen_x, S32 *screen_y) {
    LongWorldRotatePoint(x, y, z);
    if (!LongProjectPoint(X0, Y0, Z0))
        return FALSE;
    *screen_x = Xp;
    *screen_y = Yp;
    return TRUE;
}

static void TerrainRegionEditorDrawBox(T_TERRAIN_REGION *region, S32 color) {
    S32 x0 = region->X0 * 512;
    S32 z0 = region->Z0 * 512;
    S32 x1 = (region->X1 + 1) * 512;
    S32 z1 = (region->Z1 + 1) * 512;
    S32 y = 0;
    S32 height = 256;
    S32 sx[8];
    S32 sy[8];
    S32 i;

    if (MapSommetY)
        y = MapSommetY[region->Z0 * (NB_COTE + 1) + region->X0];
    if (!TerrainRegionEditorProjectPoint(x0, y, z0, &sx[0], &sy[0])
            OR !TerrainRegionEditorProjectPoint(x1, y, z0, &sx[1], &sy[1])
            OR !TerrainRegionEditorProjectPoint(x1, y, z1, &sx[2], &sy[2])
            OR !TerrainRegionEditorProjectPoint(x0, y, z1, &sx[3], &sy[3])
            OR !TerrainRegionEditorProjectPoint(x0, y + height, z0, &sx[4], &sy[4])
            OR !TerrainRegionEditorProjectPoint(x1, y + height, z0, &sx[5], &sy[5])
            OR !TerrainRegionEditorProjectPoint(x1, y + height, z1, &sx[6], &sy[6])
            OR !TerrainRegionEditorProjectPoint(x0, y + height, z1, &sx[7], &sy[7]))
        return;

    for (i = 0; i < 4; i++) {
        S32 next = (i + 1) & 3;
        Line(sx[i], sy[i], sx[next], sy[next], color);
        Line(sx[i + 4], sy[i + 4], sx[next + 4], sy[next + 4], color);
        Line(sx[i], sy[i], sx[i + 4], sy[i + 4], color);
    }
}

void TerrainRegionEditorSetSelect(S32 select) {
    TerrainRegionEditorSelect = select ? TRUE : FALSE;
    if (!TerrainRegionEditorSelect) {
        TerrainRegionEditorDragActive = FALSE;
        TerrainRegionEditorPalettePressType = -1;
    }
    TerrainRegionEditorLastClick = Click;
    TerrainRegionEditorLastDelete = (CheckKey(K_SUPPR) OR CheckKey(K_BACKSPACE)) ? TRUE : FALSE;
    TerrainRegionEditorLastM = CheckKey(K_M) ? TRUE : FALSE;
    TerrainRegionEditorLastQ = CheckKey(K_Q) ? TRUE : FALSE;
    TerrainRegionEditorLastF = CheckKey(K_F) ? TRUE : FALSE;
}

S32 TerrainRegionEditorGetSelect(void) {
    return TerrainRegionEditorSelect;
}

void TerrainRegionEditorBeginFrame(void) {
    S32 click;
    S32 delete_key;
    S32 m;
    S32 q;
    S32 f;
    S32 x;
    S32 z;
    S32 half;
    S32 changed = FALSE;
    S32 i;

    if (!TerrainRegionEditorSelect)
        return;
    m = CheckKey(K_M) ? TRUE : FALSE;
    q = CheckKey(K_Q) ? TRUE : FALSE;
    f = CheckKey(K_F) ? TRUE : FALSE;
    if (!Editor_InspectorConsumesInput() AND m AND !TerrainRegionEditorLastM) {
        TerrainRegionEditorPaintMode = !TerrainRegionEditorPaintMode;
        TerrainRegionEditorDragActive = FALSE;
        Editor_SetStatus(TerrainRegionEditorPaintMode ? "Auto Terrain: Paint mode" : "Auto Terrain: Select mode");
    }
    if (!Editor_InspectorConsumesInput() AND q AND !TerrainRegionEditorLastQ) {
        TerrainRegionEditorQuadMode = !TerrainRegionEditorQuadMode;
        TerrainRegionEditorDragActive = FALSE;
        Editor_SetStatus(TerrainRegionEditorQuadMode ? "Auto Terrain paint unit: Quad" : "Auto Terrain paint unit: Tri");
    }
    if (TerrainRegionEditorPaintMode AND !Editor_InspectorConsumesInput() AND f AND !TerrainRegionEditorLastF) {
        TerrainRegionEditorRectFill = !TerrainRegionEditorRectFill;
        TerrainRegionEditorDragActive = FALSE;
        Editor_SetStatus(TerrainRegionEditorRectFill ? "Auto Terrain brush: Rect Fill" : "Auto Terrain brush: Freehand");
    }
    TerrainRegionEditorLastM = m;
    TerrainRegionEditorLastQ = q;
    TerrainRegionEditorLastF = f;
    delete_key = (CheckKey(K_SUPPR) OR CheckKey(K_BACKSPACE)) ? TRUE : FALSE;
    if (!TerrainRegionEditorPaintMode AND !Editor_InspectorConsumesInput()
            AND delete_key AND !TerrainRegionEditorLastDelete AND TerrainRegionEditorHasSelected()) {
        for (i = 0; i < TERRAIN_REGION_CELL_MAX; i++) {
            if (TerrainRegionCells[i].Used AND TerrainRegionCells[i].Region == TerrainRegionEditorSelected)
                TerrainRegionCells[i].Used = FALSE;
        }
        TerrainRegionEditorSelected = -1;
        TerrainRegionEditorRebuildAndBakeCurrentCube();
        Editor_SetStatus("Deleted Auto Terrain region");
    }
    TerrainRegionEditorLastDelete = delete_key;
    if (Editor_IsCameraPanActive()) {
        TerrainRegionEditorDragActive = FALSE;
        TerrainRegionEditorLastClick = Click;
        return;
    }

    click = Click;
    if (TerrainRegionEditorPaintMode AND(click & 1) AND !(TerrainRegionEditorLastClick & 1)) {
        TerrainRegionEditorPalettePressType = TerrainRegionEditorPaletteTypeAt((S32)MouseX, (S32)MouseY);
    }
    if (TerrainRegionEditorPaintMode AND TerrainRegionEditorPalettePressType < 0) {
        if (TerrainRegionEditorRectFill) {
            if (!TerrainRegionEditorDragActive
                    AND((click & 1 AND !(TerrainRegionEditorLastClick & 1))
                        OR(click & 2 AND !(TerrainRegionEditorLastClick & 2)))
                    AND TerrainRegionEditorPickCell(&x, &z, &half)) {
                TerrainRegionEditorDragActive = TRUE;
                TerrainRegionEditorDragX0 = TerrainRegionEditorDragX1 = x;
                TerrainRegionEditorDragZ0 = TerrainRegionEditorDragZ1 = z;
                TerrainRegionEditorDragHalf = half;
                TerrainRegionEditorDragErase = (click & 2) ? TRUE : FALSE;
            }
            if (TerrainRegionEditorDragActive AND TerrainRegionEditorPickCell(&x, &z, &half)) {
                TerrainRegionEditorDragX1 = x;
                TerrainRegionEditorDragZ1 = z;
            }
            if (TerrainRegionEditorDragActive
                    AND((TerrainRegionEditorDragErase AND !(click & 2))
                        OR(!TerrainRegionEditorDragErase AND !(click & 1)))) {
                S32 x0 = TerrainRegionEditorDragX0 < TerrainRegionEditorDragX1 ? TerrainRegionEditorDragX0 : TerrainRegionEditorDragX1;
                S32 x1 = TerrainRegionEditorDragX0 > TerrainRegionEditorDragX1 ? TerrainRegionEditorDragX0 : TerrainRegionEditorDragX1;
                S32 z0 = TerrainRegionEditorDragZ0 < TerrainRegionEditorDragZ1 ? TerrainRegionEditorDragZ0 : TerrainRegionEditorDragZ1;
                S32 z1 = TerrainRegionEditorDragZ0 > TerrainRegionEditorDragZ1 ? TerrainRegionEditorDragZ0 : TerrainRegionEditorDragZ1;
                S32 fill_x;
                S32 fill_z;
                for (fill_z = z0; fill_z <= z1; fill_z++) {
                    for (fill_x = x0; fill_x <= x1; fill_x++) {
                        if (TerrainRegionEditorDragErase)
                            changed |= TerrainRegionEditorEraseCell(fill_x, fill_z, TerrainRegionEditorDragHalf);
                        else
                            changed |= TerrainRegionEditorPaintCell(fill_x, fill_z, TerrainRegionEditorDragHalf, TerrainRegionEditorPaletteType);
                    }
                }
                TerrainRegionEditorDragActive = FALSE;
                if (changed)
                    TerrainRegionEditorRebuildAndBakeCurrentCube();
            }
        } else if (TerrainRegionEditorPickCell(&x, &z, &half)) {
            if (click & 1)
                changed = TerrainRegionEditorPaintCell(x, z, half, TerrainRegionEditorPaletteType);
            else if (click & 2)
                changed = TerrainRegionEditorEraseCell(x, z, half);
            if (changed)
                TerrainRegionEditorRebuildAndBakeCurrentCube();
        }
    }
    if (!(click & 1) AND(TerrainRegionEditorLastClick & 1)
            AND TerrainRegionEditorPalettePressType >= 0) {
        if (TerrainRegionEditorPaletteTypeAt((S32)MouseX, (S32)MouseY) == TerrainRegionEditorPalettePressType)
            TerrainRegionEditorPaletteType = TerrainRegionEditorPalettePressType;
        TerrainRegionEditorPalettePressType = -1;
    }
    if (!TerrainRegionEditorPaintMode AND(click & 1) AND !(TerrainRegionEditorLastClick & 1)
            AND TerrainRegionEditorPickCell(&x, &z, &half)) {
        TerrainRegionEditorSelected = -1;
        for (i = TERRAIN_REGION_CELL_MAX - 1; i >= 0; i--) {
            T_TERRAIN_REGION_CELL *cell = &TerrainRegionCells[i];
            if (cell->Used AND cell->CubeX == CurrentCubeX AND cell->CubeY == CurrentCubeY AND cell->X == x AND cell->Z == z AND TerrainRegionEditorCellCovers(cell, half)) {
                TerrainRegionEditorSelected = cell->Region;
                TerrainRegionEditorPaletteType = cell->Type;
                break;
            }
        }
    }
    TerrainRegionEditorLastClick = click;
}

void TerrainRegionEditorEndFrame(void) {
}

static S32 TerrainRegionEditorTypeColor(S32 type) {
    static const S32 colors[TERRAIN_REGION_TYPE_COUNT] = {10, 14, 11, 8, 9, 6, 13, 12, 15, 5};
    return type >= 0 AND type < TERRAIN_REGION_TYPE_COUNT ? colors[type] : TERRAIN_REGION_MARK;
}

static void TerrainRegionEditorDrawCell(T_TERRAIN_REGION_CELL *cell, S32 selected) {
    T_SOMMET_ROT *ptrs;
    S32 color;
    S32 sens;

    if (!cell->Used OR cell->CubeX != CurrentCubeX OR cell->CubeY != CurrentCubeY)
        return;
    ptrs = &SommetRot[cell->Z][cell->X];
    color = selected ? TERRAIN_REGION_SELECTED_MARK : TerrainRegionEditorTypeColor(cell->Type);
    sens = MapPolyGround[(cell->Z * NB_COTE + cell->X) * 2].Sens;
    if (cell->Half < 0) {
        Line(ptrs[0].X2D, ptrs[0].Y2D, ptrs[65].X2D, ptrs[65].Y2D, color);
        Line(ptrs[65].X2D, ptrs[65].Y2D, ptrs[66].X2D, ptrs[66].Y2D, color);
        Line(ptrs[66].X2D, ptrs[66].Y2D, ptrs[1].X2D, ptrs[1].Y2D, color);
        Line(ptrs[1].X2D, ptrs[1].Y2D, ptrs[0].X2D, ptrs[0].Y2D, color);
    } else if (sens == 0 AND cell->Half == 0) {
        Line(ptrs[0].X2D, ptrs[0].Y2D, ptrs[65].X2D, ptrs[65].Y2D, color);
        Line(ptrs[65].X2D, ptrs[65].Y2D, ptrs[66].X2D, ptrs[66].Y2D, color);
        Line(ptrs[66].X2D, ptrs[66].Y2D, ptrs[0].X2D, ptrs[0].Y2D, color);
    } else if (sens == 0) {
        Line(ptrs[66].X2D, ptrs[66].Y2D, ptrs[1].X2D, ptrs[1].Y2D, color);
        Line(ptrs[1].X2D, ptrs[1].Y2D, ptrs[0].X2D, ptrs[0].Y2D, color);
        Line(ptrs[0].X2D, ptrs[0].Y2D, ptrs[66].X2D, ptrs[66].Y2D, color);
    } else if (cell->Half == 0) {
        Line(ptrs[1].X2D, ptrs[1].Y2D, ptrs[0].X2D, ptrs[0].Y2D, color);
        Line(ptrs[0].X2D, ptrs[0].Y2D, ptrs[65].X2D, ptrs[65].Y2D, color);
        Line(ptrs[65].X2D, ptrs[65].Y2D, ptrs[1].X2D, ptrs[1].Y2D, color);
    } else {
        Line(ptrs[65].X2D, ptrs[65].Y2D, ptrs[66].X2D, ptrs[66].Y2D, color);
        Line(ptrs[66].X2D, ptrs[66].Y2D, ptrs[1].X2D, ptrs[1].Y2D, color);
        Line(ptrs[1].X2D, ptrs[1].Y2D, ptrs[65].X2D, ptrs[65].Y2D, color);
    }
}

void TerrainRegionEditorOverlayDraw(void) {
    S32 i;

    if (!TerrainRegionEditorSelect OR !TerrainEditorLoadedCurrentCube())
        return;

    UnsetClip();
    for (i = 0; i < TERRAIN_REGION_CELL_MAX; i++) {
        T_TERRAIN_REGION_CELL *cell = &TerrainRegionCells[i];
        TerrainRegionEditorDrawCell(cell, cell->Region == TerrainRegionEditorSelected);
    }
    if (TerrainRegionEditorPaintMode) {
        Line((S32)MouseX - 8, (S32)MouseY, (S32)MouseX - 2, (S32)MouseY, 12);
        Line((S32)MouseX + 2, (S32)MouseY, (S32)MouseX + 8, (S32)MouseY, 12);
        Line((S32)MouseX, (S32)MouseY - 8, (S32)MouseX, (S32)MouseY - 2, 12);
        Line((S32)MouseX, (S32)MouseY + 2, (S32)MouseX, (S32)MouseY + 8, 12);
    }
    if (TerrainRegionEditorHasSelected())
        TerrainRegionEditorDrawBox(&TerrainRegions[TerrainRegionEditorSelected], TERRAIN_REGION_SELECTED_MARK);
    if (TerrainRegionEditorPaintMode AND TerrainRegionEditorRectFill AND TerrainRegionEditorDragActive) {
        T_TERRAIN_REGION preview;
        memset(&preview, 0, sizeof(preview));
        preview.X0 = TerrainRegionEditorDragX0 < TerrainRegionEditorDragX1 ? TerrainRegionEditorDragX0 : TerrainRegionEditorDragX1;
        preview.X1 = TerrainRegionEditorDragX0 > TerrainRegionEditorDragX1 ? TerrainRegionEditorDragX0 : TerrainRegionEditorDragX1;
        preview.Z0 = TerrainRegionEditorDragZ0 < TerrainRegionEditorDragZ1 ? TerrainRegionEditorDragZ0 : TerrainRegionEditorDragZ1;
        preview.Z1 = TerrainRegionEditorDragZ0 > TerrainRegionEditorDragZ1 ? TerrainRegionEditorDragZ0 : TerrainRegionEditorDragZ1;
        TerrainRegionEditorDrawBox(&preview, TerrainRegionEditorDragErase ? TERRAIN_REGION_SELECTED_MARK : TerrainRegionEditorTypeColor(TerrainRegionEditorPaletteType));
    }
    if (TerrainRegionEditorPaintMode)
        TerrainRegionEditorDrawPalette();
    UnsetClip();
}

void TerrainTextureEditorOverlayDraw(void) {
    S32 xi;
    S32 zi;

    if (!TerrainTextureEditorSelect)
        return;
    if (!TerrainEditorLoadedCurrentCube())
        return;

    UnsetClip();

    if (TerrainTextureEditorPickPending OR TerrainTextureEditorDragSelectPending) {
        for (zi = 0; zi < NB_COTE; zi++) {
            for (xi = 0; xi < NB_COTE; xi++) {
                S32 index0 = zi * NB_COTE * 2 + xi * 2;
                S32 index1 = index0 + 1;
                T_SOMMET_ROT *ptrs = &SommetRot[zi][xi];
                U8 *ptra = &SommetAffichable[zi][xi];
                S32 x0 = ptrs[0].X2D;
                S32 y0 = ptrs[0].Y2D;
                S32 x1 = ptrs[65].X2D;
                S32 y1 = ptrs[65].Y2D;
                S32 x2 = ptrs[66].X2D;
                S32 y2 = ptrs[66].Y2D;
                S32 x3 = ptrs[1].X2D;
                S32 y3 = ptrs[1].Y2D;
                S32 sens = MapPolyGround[index0].Sens;

                if (sens == 0) {
                    if ((ptra[0] & 16) AND(ptra[65] & 16) AND(ptra[66] & 16))
                        TerrainTextureEditorConsiderTri(CurrentCubeX, CurrentCubeY, index0, x0, y0, x1, y1, x2, y2);

                    if ((ptra[66] & 16) AND(ptra[1] & 16) AND(ptra[0] & 16))
                        TerrainTextureEditorConsiderTri(CurrentCubeX, CurrentCubeY, index1, x2, y2, x3, y3, x0, y0);
                } else {
                    if ((ptra[1] & 16) AND(ptra[0] & 16) AND(ptra[65] & 16))
                        TerrainTextureEditorConsiderTri(CurrentCubeX, CurrentCubeY, index0, x3, y3, x0, y0, x1, y1);

                    if ((ptra[65] & 16) AND(ptra[66] & 16) AND(ptra[1] & 16))
                        TerrainTextureEditorConsiderTri(CurrentCubeX, CurrentCubeY, index1, x1, y1, x2, y2, x3, y3);
                }
            }
        }
    }

    TerrainTextureEditorDrawSelectedList();
    TerrainTextureEditorDrawDragBox();
    TerrainTextureEditorDrawPalette();

    UnsetClip();
}
#endif
