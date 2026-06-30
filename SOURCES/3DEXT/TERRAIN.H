#ifndef TERRAIN_H
#define TERRAIN_H

#ifdef LBA2CC_EDITOR
#include "../EDITOR/TERRAIN_EDITOR.H"
#include "../EDITOR/TERRAIN_TEXTURE_EDITOR.H"
#include "../EDITOR/TERRAIN_REGION_EDITOR.H"
#endif

#define Y_SEA -50

extern S32 Sky_Y;
extern S32 Reglage;

/*--------------------------------------------------------------------------*/
extern void DrawSkySeaZBuf(void);
/*--------------------------------------------------------------------------*/
extern void ClsTerrainZBuf(void);
/*--------------------------------------------------------------------------*/
extern void DrawSeenSea(S32 y);
/*--------------------------------------------------------------------------*/
extern void DrawHorizon2ZBuf(void);
/*--------------------------------------------------------------------------*/
extern void AffichageTerrainZBuf(void);
/*--------------------------------------------------------------------------*/
extern void AffichageTerrainRapide(void);
/*--------------------------------------------------------------------------*/
extern void CopyBlock256(U8 *sour, U8 *dest);
/*--------------------------------------------------------------------------*/
extern void ClearAniPoly(void);
/*--------------------------------------------------------------------------*/
extern void DoAnimatedPolys();
/*--------------------------------------------------------------------------*/
extern void DrawAnimatedPolys();
/*--------------------------------------------------------------------------*/
extern void FiltreAnimatedPolys();
/*--------------------------------------------------------------------------*/

#endif // TERRAIN_H
