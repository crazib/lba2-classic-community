#include "DEFINES.H"
#include "../EDITOR_JSON.H"
#include "../EDITOR.H"
#include "../PERSO.H"

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
    for (S32 n = 9; n > 0; n--, TerrainTri++) {
        S32 x, y;

        x = CurrentCubeX + TerrainTri->x;
        y = CurrentCubeY + TerrainTri->y;

        if (x >= 0 AND x <= 15 AND y >= 0 AND y <= 15) {
            if (TerrainTri->x OR TerrainTri->y) {
                // PreClipping du Cube
                /*				if( PtrProjectPoint( 0, 0, 0 )
				OR  PtrProjectPoint( 32767, 0, 0 )
				OR  PtrProjectPoint( 32767, 0, 32767 )
				OR  PtrProjectPoint( 0, 0, 32767 ) )
*/
                {
#ifdef LBA_GAME
                    SetPosCamera(cx - OffPosCamera[TerrainTri->x + 1] + TerrainTri->x, cy, cz - OffPosCamera[TerrainTri->y + 1] + TerrainTri->y);
#else
                    SetPosCamera(cx - OffPosCamera[TerrainTri->x + 1] + TerrainTri->x * Reglage, cy, cz - OffPosCamera[TerrainTri->y + 1] + TerrainTri->y * Reglage);
#endif
                    if (LoadCube(x, y)) {
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
static S32 TerrainTextureEditorPickPending = FALSE;
static S32 TerrainTextureEditorPickShift = FALSE;
static S32 TerrainTextureEditorPickBestCubeX = 0;
static S32 TerrainTextureEditorPickBestCubeY = 0;
static S32 TerrainTextureEditorPickBestIndex = -1;
static S32 TerrainTextureEditorTexDefCount = 0;
static S32 TerrainTextureEditorDragActive = FALSE;
static S32 TerrainTextureEditorDragSelectPending = FALSE;
static S32 TerrainTextureEditorDragShift = FALSE;
static S32 TerrainTextureEditorDragX0 = 0;
static S32 TerrainTextureEditorDragY0 = 0;
static S32 TerrainTextureEditorDragX1 = 0;
static S32 TerrainTextureEditorDragY1 = 0;
static S32 TerrainTextureEditorPalettePressEntry = -2;

#define TERRAIN_EDITOR_MAX_SELECTED 4096
#define TERRAIN_EDITOR_MAX_EDITS 16384
#define TERRAIN_EDITOR_PICK_RADIUS 18
#define TERRAIN_EDITOR_NUDGE 256
#define TERRAIN_EDITOR_NUDGE_SHIFT (5 * TERRAIN_EDITOR_NUDGE)
#define TERRAIN_EDITOR_TEXT 0
#define TERRAIN_EDITOR_BACK 14
#define TERRAIN_OVERLAY_TEXT 15
#define TERRAIN_OVERLAY_BACK 0
#define TERRAIN_EDITOR_DRAG_MARK 15
#define TERRAIN_EDITOR_DRAG_MIN 4
#define TERRAIN_TEXTURE_EDITOR_MAX_SELECTED 4096
#define TERRAIN_TEXTURE_EDITOR_MAX_EDITS 16384
#define TERRAIN_TEXTURE_INTENSITY_MAX_EDITS 16384
#define TERRAIN_TEXTURE_EDITOR_MARK 12
#define TERRAIN_TEXTURE_DRAG_MARK 15
#define TERRAIN_TEXTURE_DRAG_MIN 4
#define TERRAIN_TEXTURE_PALETTE_SWATCH 16
#define TERRAIN_TEXTURE_PALETTE_GAP 3
#define TERRAIN_TEXTURE_PALETTE_MIN_COLS 4
#define TERRAIN_TEXTURE_PALETTE_PAD 4
#define TERRAIN_TEXTURE_PALETTE_MAX_WIDTH_DIV 2
#define TERRAIN_TEXTURE_PALETTE_BACK 0
#define TERRAIN_TEXTURE_PALETTE_BORDER 15
#define TERRAIN_TEXTURE_PALETTE_SELECT 12
#define TERRAIN_TEXTURE_PALETTE_NONE -2
#define TERRAIN_TEXTURE_PALETTE_DEAD -3

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

static T_TERRAIN_EDITOR_SEL TerrainEditorSelList[TERRAIN_EDITOR_MAX_SELECTED];
static T_TERRAIN_EDITOR_EDIT TerrainEditorEdits[TERRAIN_EDITOR_MAX_EDITS];
static T_TERRAIN_TEXTURE_EDITOR_SEL TerrainTextureEditorSelList[TERRAIN_TEXTURE_EDITOR_MAX_SELECTED];
static T_TERRAIN_TEXTURE_EDITOR_EDIT TerrainTextureEditorEdits[TERRAIN_TEXTURE_EDITOR_MAX_EDITS];
static T_TERRAIN_TEXTURE_INTENSITY_EDIT TerrainTextureIntensityEdits[TERRAIN_TEXTURE_INTENSITY_MAX_EDITS];

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
                    AND TerrainEditorEdits[i].CubeX == cube_x
                    AND TerrainEditorEdits[i].CubeY == cube_y)
                has_cube = TRUE;
        }
        for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_EDITS; i++) {
            if (TerrainTextureEditorEdits[i].Used
                    AND TerrainTextureEditorEdits[i].CubeX == cube_x
                    AND TerrainTextureEditorEdits[i].CubeY == cube_y)
                has_cube = TRUE;
        }
        for (i = 0; i < TERRAIN_TEXTURE_INTENSITY_MAX_EDITS; i++) {
            if (TerrainTextureIntensityEdits[i].Used
                    AND TerrainTextureIntensityEdits[i].CubeX == cube_x
                    AND TerrainTextureIntensityEdits[i].CubeY == cube_y)
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
                    OR other->CubeX != cube_x
                    OR other->CubeY != cube_y)
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
                    OR other->CubeX != cube_x
                    OR other->CubeY != cube_y)
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
                    OR other->CubeX != cube_x
                    OR other->CubeY != cube_y)
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

void TerrainEditorBeginFrame(void) {
    S32 click;
    S32 up;
    S32 down;
    S32 step;

    if (!TerrainEditorSelect)
        return;

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
    step = (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? TERRAIN_EDITOR_NUDGE_SHIFT : TERRAIN_EDITOR_NUDGE;

    if (up AND !TerrainEditorLastUp)
        TerrainEditorMoveSelected(step);
    if (down AND !TerrainEditorLastDown)
        TerrainEditorMoveSelected(-step);

    TerrainEditorLastUp = up;
    TerrainEditorLastDown = down;
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
    ShowMouse(TerrainTextureEditorSelect ? true : false);
}

S32 TerrainTextureEditorGetSelect(void) {
    return TerrainTextureEditorSelect;
}

S32 TerrainTextureEditorConsumesInput(void) {
    return TerrainTextureEditorSelect;
}

void TerrainTextureEditorSetTexDefCount(S32 count) {
    TerrainTextureEditorTexDefCount = count > 0 ? count : 0;
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
            if (poly->Bank != edit->BasePoly.Bank
                    OR poly->TexFlag != edit->BasePoly.TexFlag
                    OR poly->PolyFlag != edit->BasePoly.PolyFlag
                    OR poly->SampleStep != edit->BasePoly.SampleStep
                    OR poly->CodeJeu != edit->BasePoly.CodeJeu
                    OR poly->Sens != edit->BasePoly.Sens
                    OR poly->Col != edit->BasePoly.Col
                    OR poly->Dummy != edit->BasePoly.Dummy
                    OR poly->IndexTex != edit->BasePoly.IndexTex) {
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
            poly->TexFlag = 0;
            if (!poly->PolyFlag)
                poly->PolyFlag = 1;
        } else {
            poly->IndexTex++;
        }
    } else {
        if (poly->IndexTex == 0) {
            poly->TexFlag = 0;
            if (!poly->PolyFlag)
                poly->PolyFlag = 1;
        } else {
            poly->IndexTex--;
        }
    }
}

static void TerrainTextureEditorCycleSelected(S32 dir) {
    S32 i;

    if (!MapPolyGround)
        return;

    for (i = 0; i < TERRAIN_TEXTURE_EDITOR_MAX_SELECTED; i++) {
        T_TERRAIN_TEXTURE_EDITOR_SEL *sel = &TerrainTextureEditorSelList[i];
        T_TERRAIN_TEXTURE_EDITOR_EDIT *edit;

        if (!sel->Used)
            continue;

        edit = TerrainTextureEditorFindEdit(sel->CubeX, sel->CubeY, sel->Index);
        if (sel->CubeX == CurrentCubeX
                AND sel->CubeY == CurrentCubeY
                AND sel->Index >= 0
                AND sel->Index < NB_COTE * NB_COTE * 2)
            edit = TerrainTextureEditorGetOrCreateEdit(sel->CubeX, sel->CubeY, sel->Index, &MapPolyGround[sel->Index]);
        if (!edit)
            continue;

        TerrainTextureEditorCycleEdit(edit, dir);

        if (sel->CubeX == CurrentCubeX
                AND sel->CubeY == CurrentCubeY
                AND sel->Index >= 0
                AND sel->Index < NB_COTE * NB_COTE * 2)
            TerrainTextureEditorApplyEditToPoly(edit, &MapPolyGround[sel->Index]);
    }
}

static void TerrainTextureEditorSetEditTexture(T_TERRAIN_TEXTURE_EDITOR_EDIT *edit, S32 texture_index) {
    T_HALF_POLY *poly;

    poly = &edit->Poly;
    if (texture_index < 0) {
        poly->TexFlag = 0;
        if (!poly->PolyFlag)
            poly->PolyFlag = 1;
        return;
    }

    if (TerrainTextureEditorTexDefCount <= 0)
        return;
    if (texture_index >= TerrainTextureEditorTexDefCount)
        return;

    poly->TexFlag = 1;
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
        if (sel->CubeX == CurrentCubeX
                AND sel->CubeY == CurrentCubeY
                AND sel->Index >= 0
                AND sel->Index < NB_COTE * NB_COTE * 2)
            edit = TerrainTextureEditorGetOrCreateEdit(sel->CubeX, sel->CubeY, sel->Index, &MapPolyGround[sel->Index]);
        if (!edit)
            continue;

        TerrainTextureEditorSetEditTexture(edit, texture_index);

        if (sel->CubeX == CurrentCubeX
                AND sel->CubeY == CurrentCubeY
                AND sel->Index >= 0
                AND sel->Index < NB_COTE * NB_COTE * 2)
            TerrainTextureEditorApplyEditToPoly(edit, &MapPolyGround[sel->Index]);
    }
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

        entry = poly->TexFlag ? (S32)poly->IndexTex : -1;
        if (texture_entry == -3) {
            texture_entry = entry;
        } else if (texture_entry != entry) {
            return -3;
        }
    }

    return texture_entry;
}

static S32 TerrainTextureEditorPaletteRows(void) {
    S32 panel_h;
    S32 rows;

    panel_h = (S32)ModeDesiredY - TERRAIN_TEXTURE_PALETTE_PAD * 2;
    rows = (panel_h - TERRAIN_TEXTURE_PALETTE_PAD * 2 + TERRAIN_TEXTURE_PALETTE_GAP)
           / (TERRAIN_TEXTURE_PALETTE_SWATCH + TERRAIN_TEXTURE_PALETTE_GAP);
    if (rows < 1)
        rows = 1;
    return rows;
}

static S32 TerrainTextureEditorPaletteColumns(void) {
    S32 rows;
    S32 wanted;
    S32 max_cols;

    rows = TerrainTextureEditorPaletteRows();
    wanted = (TerrainTextureEditorTexDefCount + 1 + rows - 1) / rows;
    if (wanted < TERRAIN_TEXTURE_PALETTE_MIN_COLS)
        wanted = TERRAIN_TEXTURE_PALETTE_MIN_COLS;

    max_cols = (((S32)ModeDesiredX / TERRAIN_TEXTURE_PALETTE_MAX_WIDTH_DIV)
                - TERRAIN_TEXTURE_PALETTE_PAD * 2
                + TERRAIN_TEXTURE_PALETTE_GAP)
               / (TERRAIN_TEXTURE_PALETTE_SWATCH + TERRAIN_TEXTURE_PALETTE_GAP);
    if (max_cols < TERRAIN_TEXTURE_PALETTE_MIN_COLS)
        max_cols = TERRAIN_TEXTURE_PALETTE_MIN_COLS;
    if (wanted > max_cols)
        wanted = max_cols;

    return wanted;
}

static S32 TerrainTextureEditorPaletteVisibleCount(void) {
    S32 count;

    count = TerrainTextureEditorPaletteRows() * TerrainTextureEditorPaletteColumns();
    if (count > TerrainTextureEditorTexDefCount + 1)
        count = TerrainTextureEditorTexDefCount + 1;
    return count;
}

static void TerrainTextureEditorPaletteRect(S32 *x0, S32 *y0, S32 *x1, S32 *y1) {
    S32 width;
    S32 visible_count;
    S32 rows;
    S32 height;
    S32 cols;

    cols = TerrainTextureEditorPaletteColumns();
    width = TERRAIN_TEXTURE_PALETTE_PAD * 2
            + cols * TERRAIN_TEXTURE_PALETTE_SWATCH
            + (cols - 1) * TERRAIN_TEXTURE_PALETTE_GAP;
    visible_count = TerrainTextureEditorPaletteVisibleCount();
    rows = (visible_count + cols - 1) / cols;
    if (rows < 1)
        rows = 1;
    height = TERRAIN_TEXTURE_PALETTE_PAD * 2
             + rows * TERRAIN_TEXTURE_PALETTE_SWATCH
             + (rows - 1) * TERRAIN_TEXTURE_PALETTE_GAP;

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
    return entry - 1;
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

    if (!TerrainTextureEditorSelect)
        return;

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
            if (TerrainTextureEditorPalettePressEntry != TERRAIN_TEXTURE_PALETTE_DEAD
                    AND entry == TerrainTextureEditorPalettePressEntry)
                TerrainTextureEditorApplyTextureSelected(entry);
            TerrainTextureEditorPalettePressEntry = TERRAIN_TEXTURE_PALETTE_NONE;
        }
        TerrainTextureEditorLastClick = click;
        TerrainTextureEditorLastUp = CheckKey(K_UP) ? TRUE : FALSE;
        TerrainTextureEditorLastDown = CheckKey(K_DOWN) ? TRUE : FALSE;
        TerrainTextureEditorLastLeft = CheckKey(K_LEFT) ? TRUE : FALSE;
        TerrainTextureEditorLastRight = CheckKey(K_RIGHT) ? TRUE : FALSE;
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
}

static void TerrainTextureEditorSelectOnly(S32 cube_x, S32 cube_y, S32 index) {
    TerrainTextureEditorClearSelection();
    TerrainTextureEditorSelList[0].Used = TRUE;
    TerrainTextureEditorSelList[0].CubeX = cube_x;
    TerrainTextureEditorSelList[0].CubeY = cube_y;
    TerrainTextureEditorSelList[0].Index = index;
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
        TerrainTextureEditorToggleSelected(TerrainTextureEditorPickBestCubeX,
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
            if (TerrainTextureEditorPickShift)
                TerrainTextureEditorToggleSelected(cube_x, cube_y, index);
            else
                TerrainTextureEditorAddSelected(cube_x, cube_y, index);
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
        TerrainTextureEditorPickBestIndex = index;
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

static void TerrainTextureEditorDrawSelectedList(void) {
    S32 i;

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

static void TerrainTextureEditorDrawPalette(void) {
    S32 panel_x0;
    S32 panel_y0;
    S32 panel_x1;
    S32 panel_y1;
    S32 visible_count;
    S32 selected_entry;
    S32 entry;
    S32 cols;

    TerrainTextureEditorPaletteRect(&panel_x0, &panel_y0, &panel_x1, &panel_y1);
    TerrainTextureEditorFillRect(panel_x0, panel_y0, panel_x1, panel_y1, TERRAIN_TEXTURE_PALETTE_BACK);
    Rect(panel_x0, panel_y0, panel_x1, panel_y1, TERRAIN_TEXTURE_PALETTE_BORDER);

    visible_count = TerrainTextureEditorPaletteVisibleCount();
    selected_entry = TerrainTextureEditorSelectedTextureEntry();
    cols = TerrainTextureEditorPaletteColumns();

    for (entry = 0; entry < visible_count; entry++) {
        S32 col;
        S32 row;
        S32 sx;
        S32 sy;
        S32 texture_entry;
        S32 border;

        col = entry % cols;
        row = entry / cols;
        sx = panel_x0 + TERRAIN_TEXTURE_PALETTE_PAD
             + col * (TERRAIN_TEXTURE_PALETTE_SWATCH + TERRAIN_TEXTURE_PALETTE_GAP);
        sy = panel_y0 + TERRAIN_TEXTURE_PALETTE_PAD
             + row * (TERRAIN_TEXTURE_PALETTE_SWATCH + TERRAIN_TEXTURE_PALETTE_GAP);
        texture_entry = entry - 1;

        if (texture_entry < 0)
            TerrainTextureEditorDrawNoTextureSwatch(sx, sy);
        else
            TerrainTextureEditorDrawTextureSwatch(sx, sy, texture_entry);

        border = TERRAIN_TEXTURE_PALETTE_BORDER;
        if (selected_entry == texture_entry OR TerrainTextureEditorPalettePressEntry == texture_entry)
            border = TERRAIN_TEXTURE_PALETTE_SELECT;
        Rect(sx - 1,
             sy - 1,
             sx + TERRAIN_TEXTURE_PALETTE_SWATCH,
             sy + TERRAIN_TEXTURE_PALETTE_SWATCH,
             border);
    }
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
